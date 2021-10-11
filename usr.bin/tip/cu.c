/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)cu.c 1.1 92/07/30 SMI"; /* from UCB 5.2 1/13/86 */
#endif

#include "tip.h"

void	cleanup();
void	timeout();

/*
 * Botch the interface to look like cu's
 */
cumain(argc, argv)
	char *argv[];
{
	register int i;
	static char sbuf[12];

	if (argc < 2) {
usage:
		fprintf(stderr,
		   "usage: cu telno [-t] [-s speed] [-a acu] [-l line] [-#]\n");
		exit(8);
	}
	CU = DV = NOSTR;
	for (; argc > 1; argv++, argc--) {
		if (argv[1][0] != '-')
			PN = argv[1];
		else if (argv[1][1] != '\0' && argv[1][2] != '\0') {
			fprintf(stderr, "cu: extra characters after flag: %s\n",
				argv[1]);
			goto usage;
		} else switch (argv[1][1]) {

		case 't':
			HW = 1, DU = -1;
			--argc;
			continue;

		case 'a':
			CU = argv[2]; ++argv; --argc;
			break;

		case 's':
			if (argc < 3)
				goto usage;
			if (speed(atoi(argv[2])) == 0) {
				fprintf(stderr, "cu: unsupported speed %s\n",
					argv[2]);
				exit(3);
			}
			BR = atoi(argv[2]); ++argv; --argc;
			break;

		case 'l':
			DV = argv[2]; ++argv; --argc;
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			if (CU)
				CU[strlen(CU)-1] = argv[1][1];
			if (DV)
				DV[strlen(DV)-1] = argv[1][1];
			break;

		default:
			fprintf(stderr, "cu: bad flag %s\n", argv[1]);
			goto usage;
		}
	}
	signal(SIGINT, cleanup);
	signal(SIGQUIT, cleanup);
	signal(SIGHUP, cleanup);
	signal(SIGTERM, cleanup);

	/*
	 * The "cu" host name is used to define the
	 * attributes of the generic dialer.
	 */
	sprintf(sbuf, "cu%d", BR);
	if ((i = hunt(sbuf)) == 0) {
		printf("all ports busy\n");
		exit(3);
	}
	if (i == -1) {
		printf("link down\n");
		delock(uucplock);
		exit(3);
	}
	setbuf(stdout, NULL);
	loginit();
	gid = getgid();
	egid = getegid();
	uid = getuid();
	euid = geteuid();
	setregid(egid, gid);
	setreuid(euid, uid);
	vinit();
	setparity("none");
	boolean(value(VERBOSE)) = 0;
	if (HW)
		ttysetup(speed(BR));
	if (connect()) {
		printf("Connect failed\n");
		setreuid(uid, euid);
		setregid(gid, egid);
		delock(uucplock);
		exit(1);
	}
	if (!HW)
		ttysetup(speed(BR));
}
