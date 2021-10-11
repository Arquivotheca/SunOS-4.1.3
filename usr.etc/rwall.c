#ifndef lint
static  char sccsid[] = "@(#)rwall.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <utmp.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <rpcsvc/rwall.h>

#define	USERS	128
char who[9] = "???";
char *path;

main(argc, argv)
	char **argv;
{
	int msize;
	char buf[BUFSIZ];
	register i;
	struct	utmp utmp[USERS];
	FILE *f;
	int sline;
	char	hostname[256];
	int hflag;
	char *machine, *user, *domain;
	struct hostent *hp;
	
	if (argc < 2)
		usage();
	gethostname(hostname, sizeof (hostname));

	if((f = fopen("/etc/utmp", "r")) == NULL) {
		fprintf(stderr, "Cannot open /etc/utmp\n");
		exit(1);
	}
	sline = ttyslot(2); /* 'utmp' slot no. of sender */
	fread((char *)utmp, sizeof(struct utmp), USERS, f);
	fclose(f);
	if (sline)
		strncpy(who, utmp[sline].ut_name, sizeof(utmp[sline].ut_name));

	sprintf(buf, "From %s@%s:  ", who, hostname);
	msize = strlen(buf);
	while((i = getchar()) != EOF) {
		if (msize >= sizeof buf) {
			fprintf(stderr, "Message too long\n");
			exit(1);
		}
		buf[msize++] = i;
	}
	path = buf;
	
	hflag = 1;
	while(argc > 1) {
		if (argv[1][0] == '-') {
			switch (argv[1][1]) {
				case 'h':
					hflag = 1;
					break;
				case 'n':
					hflag = 0;
					break;
				default:
					usage();
					break;
			}
		}
		else if (hflag) {
			if ((hp = gethostbyname(argv[1])) == NULL)
				fprintf(stderr, "%s is unknown host\n",
				    argv[1]);
			else
				doit(hp);
		}
		else {
			setnetgrent(argv[1]);
			while (getnetgrent(&machine, &user, &domain)) {
				if (machine) {
					if ((hp = gethostbyname(machine))
					    == NULL)
						fprintf(stderr,
						    "%s is unknown host\n",
						    machine);
					else
						doit(hp);
				}
				else
					doall();
			}
			endnetgrent();
		}
		argc--;
		argv++;
	}
	exit(0);
	/* NOTREACHED */
}

/*
 * Saw a wild card, so do everything
 */
doall()
{
	struct hostent *hp;
	
	sethostent(0);
	while (hp = gethostent())
		doit(hp);
	exit(0);
}

/*
 * Clnt_call to a host that is down has a very long timeout
 * waiting for the portmapper, so we use rmtcall instead.   Since pertry
 * value of rmtcall is 3 secs, make timeout here 8 secs so that
 * you get 2 tries.
 */
doit(hp)
	struct hostent *hp;
{
	struct sockaddr_in server_addr;
	int socket, port;
	struct timeval timeout;
	enum clnt_stat stat;
	CLIENT *client;

	socket = RPC_ANYSOCK;
#ifdef DEBUG
	fprintf(stderr, "sending message to %s\n%s\n", hp->h_name, path);
	return;
#endif
	timeout.tv_usec = 0;
	timeout.tv_sec = 8;
	bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;
	stat = pmap_rmtcall(&server_addr, WALLPROG, WALLVERS, WALLPROC_WALL,
	    xdr_wrapstring, &path,  xdr_void, NULL, timeout, &port);
	if (stat != RPC_SUCCESS) {
		fprintf(stderr, "Couldn't contact %s: ", hp->h_name);
		clnt_perrno(stat);
		fprintf(stderr, "\n");
	}
}

usage()
{
	fprintf(stderr,
	    "Usage: rwall host .... [-n netgroup ....] [-h host ...]\n");
	exit(1);
}
