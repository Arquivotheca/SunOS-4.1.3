#ifndef lint
static char sccsid[] = "@(#)rdate.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/* 
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 *  rdate - get date from remote machine
 *
 *     	sets time, obtaining value from host
 *	on the tcp/time socket.  Since timeserver returns
 *	with time of day in seconds since Jan 1, 1900,  must
 *	subtract 86400(365*70 + 17) to get time
 *	since Jan 1, 1970, which is what get/settimeofday
 *	uses.
 */
#define	TOFFSET	(86400*(365*70 + 17))
#define WRITTEN 440199955	/* tv_sec when program was written */

#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>

int timeout();

main(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr_in server;
	struct sockaddr from;
	int s, i;
	u_long time;
	struct servent *sp;
	struct protoent *pp;
	struct hostent *hp;
	struct timeval timestruct;

	if (argc != 2) {
		fprintf(stderr, "usage: rdate host\n");
		exit(1);
	}
	if ((hp = gethostbyname(argv[1])) == NULL) {
		fprintf(stderr, "Sorry, host %s not in hosts database\n",
		    argv[1]);
		exit(1);
	}
	if ((sp = getservbyname("time", "tcp")) == NULL) {
		fprintf(stderr,
		    "Sorry, time service not in services database\n");
		exit(1);
	}
	if ((pp = getprotobyname("tcp")) == NULL) {
		fprintf(stderr,
		    "Sorry, TCP protocol not in protocols database\n");
		exit(1);
	}
	if ((s = socket(AF_INET, SOCK_STREAM, pp->p_proto)) == 0) {
		perror("rdate: socket");
		exit(1);
	}

	bzero((char *)&server, sizeof (server));
	bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	server.sin_family = hp->h_addrtype;
	server.sin_port = sp->s_port;

	if (connect(s, &server, sizeof (server)) < 0) {
		perror("rdate: connect");
		exit(1);
	}

	alarm(10);			/* perhaps this should be larger? */
	signal(SIGALRM, timeout);

	if (read(s, (char *)&time, sizeof (int)) != sizeof (int)) {
		perror("rdate: read");
		exit(1);
	}
	time = ntohl(time) - TOFFSET;
	/* date must be later than when program was written */
	if (time < WRITTEN) {
		fprintf(stderr, "didn't get plausible time from %s\n",
		    argv[1]);
		exit(1);
	}
	timestruct.tv_usec = 0;
	timestruct.tv_sec = time;
	i = settimeofday(&timestruct, 0);
	if (i == -1)
		perror("couldn't set time of day");
	else
		printf("%s", ctime(&timestruct.tv_sec));
	exit(0);
	/* NOTREACHED */
}

timeout()
{

	fprintf(stderr, "couldn't contact time server\n");
	exit(1);
}
