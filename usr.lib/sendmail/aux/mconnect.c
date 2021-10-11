#ifndef lint
static	char sccsid[] = "@(#)mconnect.c 1.1 92/07/30 SMI"; /* From UCB 3.4 1/5/83 */
#endif
/*
 * mconnect.c - A program to test out SMTP connections.
 * Usage: mconnect [host]
 *  ... SMTP dialog
 *  ^C or ^D or QUIT
 */

# include <stdio.h>
# include <signal.h>
# include <ctype.h>
# include <sgtty.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>

struct sockaddr_in	SendmailAddress;
struct sgttyb		TtyBuf;

main(argc, argv)
	int argc;
	char **argv;
{
	register int s;
	char *host = NULL;
	int pid;
	int on = 1;
	struct servent *sp;
	int raw = 0;
	char buf[1000];
	extern char *index();
	register FILE *f;
	register struct hostent *hp;
	u_long theaddr;
	extern u_long inet_addr();
	extern finis();

	(void) gtty(0, &TtyBuf);
	(void) signal(SIGINT, finis);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		perror("socket");
		exit(-1);
	}
	(void) setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));

	sp = getservbyname("smtp", "tcp");
	if (sp != NULL)
		SendmailAddress.sin_port = sp->s_port;

	while (--argc > 0)
	{
		register char *p = *++argv;

		if (*p == '-')
		{
			switch (*++p)
			{
			  case 'h':		/* host */
				break;

			  case 'p':		/* port */
				SendmailAddress.sin_port = htons(atoi(*++argv));
				argc--;
				break;

			  case 'r':		/* raw connection */
				raw = 1;
				TtyBuf.sg_flags &= ~CRMOD;
				stty(0, &TtyBuf);
				TtyBuf.sg_flags |= CRMOD;
				break;
			}
		}
		else if (host == NULL)
			host = p;
	}
	if (host == NULL)
		host = "localhost";

	hp = gethostbyname(host);
	if (hp == NULL)
	{
		/* Try for dotted pair or whatever */
		theaddr = inet_addr(host);
		SendmailAddress.sin_addr.s_addr = theaddr;
		if (-1 == theaddr) {
			fprintf(stderr, "mconnect: unknown host %s\r\n", host);
			finis();
		}
	} else {
		bcopy(hp->h_addr, &SendmailAddress.sin_addr, hp->h_length);
	}
	SendmailAddress.sin_family = AF_INET;

	printf("connecting to host %s (%s), port %d\r\n", host,
	       inet_ntoa(SendmailAddress.sin_addr),
	       ntohs(SendmailAddress.sin_port));
	if (connect(s, &SendmailAddress, sizeof SendmailAddress) < 0)
	{
		perror("connect");
		exit(-1);
	}

	/* good connection, fork both sides */
	printf("connection open\n");
	pid = fork();
	if (pid < 0)
	{
		perror("fork");
		exit(-1);
	}
	if (pid == 0)
	{
		/* child -- standard input to sendmail */
		int c;

		f = fdopen(s, "w");
		while ((c = fgetc(stdin)) >= 0)
		{
			if (!raw && c == '\n')
				fputc('\r', f);
			fputc(c, f);
			if (c == '\n')
				fflush(f);
		}
		shutdown(s,1);
		sleep(10);
	}
	else
	{
		/* parent -- sendmail to standard output */
		f = fdopen(s, "r");
		while (fgets(buf, sizeof buf, f) != NULL)
		{
			fputs(buf, stdout);
			fflush(stdout);
		}
		kill(pid, SIGTERM);
	}
	finis();
}

finis()
{
	stty(0, &TtyBuf);
	exit(0);
}
