
#ifndef lint
static  char sccsid[] = "@(#)getfname.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gttest.h>

/**********************************************************************/
char *getfname(fname, type)
/**********************************************************************/
char *fname;
int type;

{
    static  char image_directory[] = "/tmp", name[512];
    static  char red[]=".red", green[]=".green", blue[]=".blue";
    static  char hdl[]=".hdl";
    char *getenv(), *directory;

    func_name = "getfname";
    TRACE_IN

    directory = getenv("DLX_TEST_IMAGES");

    if (directory == 0) {
	directory = image_directory;
    }

    strcpy(name, directory);
    strcat(name, "/");
    strcat(name, fname);
    if (type == RED_CHK) {
	strcat(name, red);
    } else if (type == GREEN_CHK) {
	strcat(name, green);
    } else if (type == BLUE_CHK) {
	strcat(name, blue);
    } else if (type == HDL_CHK) {
	strcat(name, hdl);
    }


    TRACE_OUT
    return(name);
}
