
#ifndef lint
static  char sccsid[] = "@(#)sb_test.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/**********************************************************************/
/* Include files */
/**********************************************************************/

#include <stdio.h>
#include <strings.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <esd.h>

/**********************************************************************/
/* External variables */
/**********************************************************************/

extern
void			pmessage();

extern
char			*timestamp(),
			*sprintf();

extern
struct test		sb_testlist[];

/**********************************************************************/
/* Global variables */
/**********************************************************************/

char			*device_name = FRAME_BUFFER_DEVICE_NAME;

char			*program_name;

int			chksum_gen;	/* flag for chksum generation or
					   verification */


/**********************************************************************/
/* Static variables */
/**********************************************************************/

static
int			fd,
			errcnt;

static
char			textbuf[512];


/*ARGSUSED*/
/**********************************************************************/
main(argc, argv)
/**********************************************************************/
int argc;
char **argv;
{

    struct test *tl;
    int blc, flc;
    int it;
    int tconfig;
    char *errmsg;

    program_name = argv[0];

    (void)catch_signals();

    fd = atoi(argv[1]);
    tconfig = atoi(argv[4]);
    chksum_gen = atoi(argv[5]);

    (void)strcpy(textbuf, timestamp("CG12 Funtional Blocks Test started.\n"));
    pmessage(textbuf);

    (void)lock_desktop(device_name);

    for(blc = atoi(argv[3]) ; blc ; blc--) {
	for (tl = sb_testlist, it = 1; tl ; tl = tl->nexttest, it *=2) {
	    if (it & tconfig) {
		for(flc = atoi(argv[2]) ; flc ; flc--) {

		    errmsg = (tl->proc)();

		    (void)sprintf(textbuf, "+ %s: ", tl->testname);
		    (void)strcpy(textbuf, timestamp(textbuf));
		    if (errmsg) {
			(void)strcat(textbuf, "*** ");
			(void)strcat(textbuf, errmsg);
			errcnt++;
		    } else {
			(void)strcat(textbuf, "ok.\n");
		    }
		    pmessage(textbuf);
		}
	    }
	}
    }

    (void)unlock_desktop();

    (void)strcpy(textbuf, timestamp("CG12 Funtional Blocks Test completed: "));
    if (errcnt) {
	(void)strcat(textbuf, "failed.\n");
    } else {
	(void)strcat(textbuf, "ok.\n");
    }
    pmessage(textbuf);
    exit(0);
}

/**********************************************************************/
void
pmessage(text)
/**********************************************************************/
char *text;

{
    if(write(fd, text, strlen(text)) != strlen(text)) {
	(void)fprintf(stderr, errmsg_list[39]);
	exit(-1);
    }
}

/**********************************************************************/
tmessage(text)
/**********************************************************************/
char *text;

{
    pmessage(timestamp(text));
}

/**********************************************************************/
error_report(errtxt)
/**********************************************************************/
char *errtxt;

{
    static char errmsg[] = " *** ";
    char text[512];

    (void)strcpy(text, errmsg);
    (void)strcat(text, errtxt);
    pmessage(text);
    errcnt++;
}

/**********************************************************************/
fatal_error_exit(errtxt)
/**********************************************************************/
char *errtxt;

{
	(void)unlock_desktop();
	pmessage(timestamp(errtxt));
	(void)fprintf(stderr, errtxt);
	exit(-1);
}
