
#ifndef lint
static  char sccsid[] = "@(#)getfname.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

/**********************************************************************/
char *getfname(fname, type)
/**********************************************************************/
char *fname;
int type;

{
    static  char image_directory[] = "/tmp", name[512];
    static  char red[]=".red", green[]=".green", blue[]=".blue";
    char *getenv(), *directory;

    func_name = "getfname";
    TRACE_IN

    directory = getenv("CG12_TEST_IMAGES");

    if (directory == 0) {
	directory = image_directory;
    }

    (void)strcpy(name, directory);
    (void)strcat(name, "/");
    (void)strcat(name, fname);
    if (type == RED_CHK) {
	(void)strcat(name, red);
    } else if (type == GREEN_CHK) {
	(void)strcat(name, green);
    } else if (type == BLUE_CHK) {
	(void)strcat(name, blue);
    }


    TRACE_OUT
    return(name);
}
