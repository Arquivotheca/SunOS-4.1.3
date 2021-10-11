/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)flash.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

#include "curses.ext"

extern _outch();
extern int InputPending;

flash()
{
    int savexon = xon_xoff;
#ifdef DEBUG
	if(outf) fprintf(outf, "flash().\n");
#endif
    xon_xoff = 0;
    if (flash_screen)
	tputs (flash_screen, 0, _outch);
    else
	tputs (bell, 0, _outch);
    xon_xoff = savexon;
    __cflush();
    if (InputPending)
	doupdate();
}
