/*
 * TTY interface to the "Etherfind" program.
 * Code in this module should be NOT specific to a particlar protocol.
 *
 * @(#)tty_main.c 1.1 92/07/30
 *
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <varargs.h>
#include <stdio.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "etherfind.h"

void flushit();

int printlength = 0;		/* bytes to print with x flag */

/*
 * read args from command line.
 */
main(argc, argv)
	char **argv;
{
    char *device = NULL;

    while (argc > 1) {
	if (argv[1][0] != '-' || argv[1][1] == 0 || argv[1][2] != 0)
			break;
	switch(argv[1][1]) {
		case 'c':
			if (argc < 3)
				usage();
			cflag++;
			cnt = atoi(argv[2]);
			argc--;
			argv++;
			break;

		case 'd':
			dflag++;
			break;

		case 'i':
			if (argc < 3)
				usage();
			device = argv[2];
			argc--;
			argv++;
			break;

		case 'l':
			if (argc < 3)
				usage();
			printlength = atoi(argv[2]);
			argc--;
			argv++;
			break;

		case 'n':
			nflag++;
			break;

		case 'p':
			pflag++;
			break;

		case 'r':
			rpcflag[0]++;
			break;

		case 't':
			timeflag[0]++;
			tflag++;
			break;

		case 'u':
			setlinebuf(stdout);
			break;

		case 'v':
			symflag[0]++;
			break;

		case 'x':
			xflag[0]++;
			break;

		default:
			goto done;
		}
		argc--;
		argv++;
	}
done:

	/*
	 * Determine which network interface to use if none given.
	 */
	if (device == NULL) {
		char cbuf[BUFSIZ];
		struct ifconf ifc;
		int s;

		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		    perror("etherfind: socket");
		    exit(1);
		}
		ifc.ifc_len = sizeof cbuf;
		ifc.ifc_buf = cbuf;
		if (ioctl(s, SIOCGIFCONF, (char *)&ifc) < 0) {
			perror("etherfind: ioctl");
			exit(1);
		}
		device = ifc.ifc_req->ifr_name;
		printf("Using interface %s\n", device);
		(void) close(s);
	}

	initIP();
	parseargs(argc, argv);
	if (!symflag[0]) {
		if (timeflag[0])
			printf("      ");
		printf(
"                                            icmp type\n");
		if (timeflag[0])
			printf("      ");
		printf(
" lnth proto         source     destination   src port   dst port\n");
  	}

	(void) signal(SIGINT, flushit);
	(void) signal(SIGTERM, flushit);

	main_loop(device);
	/* NOTREACHED */
}

/*
 * When in tty mode, we ignore index and just print everything out on
 * standard output.
 */
display(index, fmt, va_alist) 
    int index;
    char *fmt;
    va_dcl
{
    va_list ap;
    
    va_start(ap);
    _doprnt(fmt, ap, stdout);
    va_end(ap);
}

/*
 * flush the "log" - nothing to do in tty case 
 */
log_flush(index)
    int index;
{
}

/*
 * more dummy entry points for the trigger junk
 */
tr_display()
{
}

tr_triggered()
{
}

tr_save()
{
}
