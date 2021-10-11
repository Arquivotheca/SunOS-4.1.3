#if !defined(lint) && defined(sccs)
static	char sccsid[] = "@(#)wmgr_images.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Images for window manager cursors (in a separate module so that
 * they can be shared between processes, as the pixrects cannot).
 *
 * For 4.1 or 5.0 or SunView2: eliminate all of these global names!
 */

short	confirm_cursor_image[16] =  {
#include <images/confirm.cursor>
};

short	move_cursorimage[16] = {
#include <images/move.cursor>
};

short	move_h_cursorimage[16] = {
#include <images/move_h.cursor>
};

short	move_v_cursorimage[16] = {
#include <images/move_v.cursor>
};

short	stretch_v_cursorimage[16] = {
#include <images/stretch_v.cursor>
};

short	stretch_h_cursorimage[16] = {
#include <images/stretch_h.cursor>
};

short	stretchNW_cursorimage[16] = {
#include <images/stretchNW.cursor>
};

short	stretchNE_cursorimage[16] = {
#include <images/stretchNE.cursor>
};

short	stretchSE_cursorimage[16] = {
#include <images/stretchSE.cursor>
};				

short	stretchSW_cursorimage[16] = {
#include <images/stretchSW.cursor>
};				
