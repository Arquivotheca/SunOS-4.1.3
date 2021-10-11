#ifndef lint
static char	sccsid[] = "@(#)gp1_cgi_control.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
 * Permission to use, copy, modify, and distribute this software for any 
 * purpose and without fee is hereby granted, provided that the above 
 * copyright notice appear in all copies and that both that copyright 
 * notice and this permission notice are retained, and that the name 
 * of Sun Microsystems, Inc., not be used in advertising or publicity 
 * pertaining to this software without specific, written prior permission. 
 * Sun Microsystems, Inc., makes no representations about the suitability 
 * of this software or the interface defined in this software for any 
 * purpose. It is provided "as is" without express or implied warranty.
 */

#include "cgipriv.h"
#include <sys/ioctl.h>
#include <sun/gpio.h>
#include <pixrect/gp1cmds.h>


/*
 * Both of the following routines return:
 *	0 if org_x and org_y are unchanged from last call
 *	1 if either org_x or org_y has changed
 *
 * Note that these can change even though no SIGWINCH is received, but
 * they cannot change when the screen is locked.  Thus it should suffice
 * to check the return code from _cgi_gp1_pw_lock and if it is non-zero
 * update the gp1 NDC to screen transform.
 */

_cgi_gp1_pw_lock(pw, r, ptr)
Pixwin         *pw;
Rect           *r;
Gp1_attr       *ptr;
{

    pw_lock(pw, r);
    if (pw->pw_clipdata->pwcd_clipid == ptr->clipid)
	return (0);
    return (_cgi_gp1_pw_updclplst(pw, ptr));
}

int             _cgi_gp1_stblk_alloc(fd)
int             fd;
{
    int             i;

    if (ioctl(fd, GP1IO_GET_STATIC_BLOCK, &i))
	return (-1);
    else
	return (i);
}

_cgi_gp1_stblk_free(fd, num)
int             num;
{
    if (ioctl(fd, GP1IO_FREE_STATIC_BLOCK, &num))
	return (-1);
    else
	return (0);
}
