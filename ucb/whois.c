/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)whois.c 1.1 92/07/30 SMI"; /* from UCB 5.2 11/1/85 */
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>

#define	NICHOST	"nic.ddn.mil"

main(argc, argv)
	int argc;
	char *argv[];
{
	int s;
	register FILE *sfi, *sfo;
	register char c;
	char *host = NICHOST;
	struct sockaddr_in sin;
	struct hostent *hp;
	struct servent *sp;

	argc--, argv++;
	if (argc > 2 && strcmp(*argv, "-h") == 0) {
		argv++, argc--;
		host = *argv++;
		argc--;
	}
	if (argc != 1) {
		fprintf(stderr, "usage: whois [ -h host ] name\n");
		exit(1);
	}
	hp = gethostbyname(host);
	if (hp == NULL) {
		fprintf(stderr, "whois: %s: host unknown\n", host);
		exit(1);
	}
	host = hp->h_name;
	s = socket(hp->h_addrtype, SOCK_STREAM, 0, 0);
	if (s < 0) {
		perror("whois: socket");
		exit(2);
	}
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, &sin.sin_addr, hp->h_length);
	sp = getservbyname("whois", "tcp");
	if (sp == NULL) {
		sin.sin_port = htons(IPPORT_WHOIS);
	}
	else sin.sin_port = sp->s_port;
	if (connect(s, &sin, sizeof (sin), 0) < 0) {
		perror("whois: connect");
		exit(5);
	}
	sfi = fdopen(s, "r");
	sfo = fdopen(s, "w");
	if (sfi == NULL || sfo == NULL) {
		perror("fdopen");
		close(s);
		exit(1);
	}
	fprintf(sfo, "%s\r\n", *argv);
	fflush(sfo);
	while ((c = getc(sfi)) != EOF)
		putchar(c);
	exit(0);
	/* NOTREACHED */
}
