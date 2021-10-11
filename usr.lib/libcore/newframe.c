/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
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
#ifndef lint
static char sccsid[] = "@(#)newframe.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

new_frame()
{
    int index;

    _core_critflag++;

    /*
     * set all currently selected view surfaces' repaint flags 
     */
    for (index = 0; index < MAXVSURF; index++) {
	_core_surface[index].nwframnd = _core_surface[index].selected;
    }
    _core_repaint(TRUE);
    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle) ();
    return (0);
}
