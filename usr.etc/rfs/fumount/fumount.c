/*      @(#)fumount.c 1.1 92/07/30 SMI      */

/*	Copyright (c) 1987 Sun Microsystems			*/
/*	ported from System V.3.1				*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)fumount:fumount.c	1.9.3.1"

#include <stdio.h>
#include <sys/types.h>
#include <rfs/rfs_misc.h>
#include <rfs/message.h>
#include <rfs/rfsys.h>
#include <rfs/nserve.h>
#include <rfs/fumount.h>

#define MAXGRACE 3600

char *malloc(), *getnode();
char usage[] = "Usage: fumount [-w <seconds(1-%d)>] <resource>\n";

extern	int	optind,	opterr;
extern	char	*optarg;

struct clnts *client;

main(argc, argv)
	int argc;
	char **argv;
{
	char dom_resrc[(2 * MAXDNAME) + 2];
	char cmd[24+SZ_RES];
	char *grace = "0";
	int i, advflg = 0;

	while ((i = getopt(argc, argv, "w:")) != EOF)
		switch(i) {
 		case 'w':
			grace = optarg;
			if (((i = atoi(grace)) < 1) || (i > MAXGRACE)) {
				fprintf(stderr, usage, MAXGRACE);
				exit(1);
			}
			break;
		case '?':
			fprintf(stderr, usage, MAXGRACE);
			exit(1);
		}
	if (optind >= argc) {
		fprintf(stderr, usage, MAXGRACE);
		exit(1);
	}
	optarg = argv[optind];

	if (getuid() != 0) {
		fprintf(stderr, "fumount: must be super user\n");
		exit(1);
	}
	if (rfsys(RF_GETDNAME, dom_resrc, MAXDNAME) < 0) {
		fprintf(stderr, "fumount: can't get domain name.\n");
		perror("fumount");
		exit(1);
	}
	if (nlload()){
		fprintf(stderr, "fumount: can't get enough memory\n");
		exit(1);
	}
	if (getnodes(argv[optind], advflg) == 1) {
		fprintf(stderr, "fumount: %s not known\n", argv[optind]);
		exit(1);
	}

	strcat(dom_resrc, ".");
	strcat(dom_resrc, argv[optind]);

	sprintf(cmd, "unadv %s >/dev/null 2>&1\n", argv[optind]);
	if (system(cmd))
		advflg = NOTADV;

	/* execute remote warning script */
	for (i = 0; client[i].flags != EMPTY; i++)
		if (client[i].flags & KNOWN)
			sndmes(client[i].sysid, grace, dom_resrc);

	/* sleep 'grace' seconds */
	sleep(atoi(grace));

	/* blow them away */
	if (rfsys(RF_FUMOUNT, argv[optind]) == -1) {
		if (advflg & NOTADV) {
			fprintf(stderr, "fumount: %s not known\n", argv[optind]);
			exit(1);
		} else {
			if (client[0].flags != EMPTY) {	/* at least 1 client */
				perror("fumount: failure due to internal error");
				exit(1);
			}
		/* failure of rfsys is not an error if no clients were 
		   using the resource */
		}
	}
	exit(0);
	/* NOTREACHED */
}
