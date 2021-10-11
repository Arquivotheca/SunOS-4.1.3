#ifndef lint
static  char sccsid[] = "@(#)stdethers.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

/*
 * Filter to convert addresses in /etc/ethers file to standard form
 */

main(argc, argv)
	int argc;
	char **argv;
{
	char buf[512];
	register char *line = buf;
	char hostname[256];
	register char *host = hostname;
	struct ether_addr e;
	register struct ether_addr *ep = &e;
	FILE *in;

	if (argc > 1) {
		in = fopen(argv[1], "r");
		if (in == NULL) {
			fprintf(stderr, "%s: can't open %s\n", argv[0], argv[1]);
			exit(1);
		}
	} else {
		in = stdin;
	}
	while (fscanf(in, "%[^\n] ", line) == 1) {
		if ((line[0] == '#') || (line[0] == 0))
			continue;
		if (ether_line(line, ep, host) == 0) {
			fprintf(stdout, "%s	%s\n", ether_ntoa(ep), host);
		} else {
			fprintf(stderr, "%s: ignoring line: %s\n", argv[0], line);
		}
	}
	exit(0);
	/* NOTREACHED */
}
