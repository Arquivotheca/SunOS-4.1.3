
#ifndef lint
static  char sccsid[] = "@(#)int_test.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


/**********************************************************************/
/* Include files */
/**********************************************************************/

#include <stdio.h>
#include <strings.h>
#include <esd.h>

/**********************************************************************/
/* External variables */
/**********************************************************************/

extern
char			*timestamp();

extern
void			egret_test();

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
			errcnt = 0;

static
char			textbuf[512];

/*ARGSUSED*/
/**********************************************************************/
main(argc, argv)
/**********************************************************************/
int argc;
char **argv;
{

    int blc;

    program_name = argv[0];

    (void)catch_signals();

    fd = atoi(argv[1]);

    chksum_gen = atoi(argv[5]);

    (void)strcpy(textbuf, timestamp("CG12 Integration Test started.\n"));
    pmessage(textbuf);

    for(blc = atoi(argv[3]) ; blc ; blc--) {
	egret_test();
    }
    (void)strcpy(textbuf, timestamp("CG12 Integration Test completed: "));
    if (errcnt) {
	(void)strcat(textbuf, "failed.\n");
    } else {
	(void)strcat(textbuf, "ok.\n");
    }
    pmessage(textbuf);
    exit(0);
}

/**********************************************************************/
pmessage(text)
/**********************************************************************/
char *text;

{
    if(write(fd, text, strlen(text)) != strlen(text)) {
	(void)fprintf(stderr, errmsg_list[27]);
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
