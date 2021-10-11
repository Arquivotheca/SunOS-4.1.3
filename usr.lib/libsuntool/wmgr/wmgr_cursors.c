#if !defined(lint) && defined(sccs)
static	char sccsid[] = "@(#)wmgr_cursors.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1985, 1987 by Sun Microsystems, Inc.
 */

/*
 * Window manager cursors.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/cursor_impl.h>

extern short	confirm_cursor_image[];
extern short	move_cursorimage[];
extern short	move_h_cursorimage[];
extern short	move_v_cursorimage[];
extern short	stretch_v_cursorimage[];
extern short	stretch_h_cursorimage[];
extern short	stretchNW_cursorimage[];
extern short	stretchNE_cursorimage[];
extern short	stretchSE_cursorimage[];
extern short	stretchSW_cursorimage[];

/*
 *	cursor displayed while waiting for a prompt to get a response
 */

mpr_static(confirm_cursor_mpr, 16, 16, 1, confirm_cursor_image);
struct cursor confirm_cursor={ 8, 8, PIX_SRC, &confirm_cursor_mpr, FULLSCREEN};

/*
 *	3 cursors for Move
 */

mpr_static(move_mpr, 16, 16, 1, move_cursorimage);
struct cursor wmgr_move_cursor = { 6, 7, PIX_SRC, &move_mpr, FULLSCREEN};

mpr_static(move_h_mpr, 16, 13, 1, move_h_cursorimage);
struct cursor wmgr_moveH_cursor = { 7, 6, PIX_SRC, &move_h_mpr, FULLSCREEN};

mpr_static(move_v_mpr, 13, 16, 1, move_v_cursorimage);
struct cursor wmgr_moveV_cursor = { 7, 7, PIX_SRC, &move_v_mpr, FULLSCREEN};

/*	cursors for use during a Stretch operation
 *	-- shape depends on part being moved
 *
 *	first a null one for the center:
 */

short	stretchMID_cursorimage[1] = { 0 };
mpr_static(stretchMID_mpr, 0, 0, 1, stretchMID_cursorimage);
struct cursor wmgr_stretchMID_cursor={0,0,PIX_SRC, &stretchMID_mpr, FULLSCREEN};

/*
 *	each pair opposite sides share a single image
 */

mpr_static(stretch_v_mpr, 8, 16, 1, stretch_v_cursorimage);
struct cursor wmgr_stretchE_cursor={ 4, 8, PIX_SRC, &stretch_v_mpr, FULLSCREEN};
struct cursor wmgr_stretchW_cursor={ 3, 8, PIX_SRC, &stretch_v_mpr, FULLSCREEN};

mpr_static(stretch_h_mpr, 16, 8, 1, stretch_h_cursorimage);
struct cursor wmgr_stretchN_cursor={ 8, 3, PIX_SRC, &stretch_h_mpr, FULLSCREEN};
struct cursor wmgr_stretchS_cursor={ 8, 4, PIX_SRC, &stretch_h_mpr, FULLSCREEN};



/*
 *	corners are treated individually
 */

mpr_static(stretchNW_mpr, 16, 16, 1, stretchNW_cursorimage);
struct cursor wmgr_stretchNW_cursor={3,3, PIX_SRC, &stretchNW_mpr, FULLSCREEN};

mpr_static(stretchNE_mpr, 16, 16, 1, stretchNE_cursorimage);
struct cursor wmgr_stretchNE_cursor={13,3, PIX_SRC, &stretchNE_mpr, FULLSCREEN};
mpr_static(stretchSE_mpr, 16, 16, 1, stretchSE_cursorimage);
struct cursor wmgr_stretchSE_cursor={13,13,PIX_SRC, &stretchSE_mpr, FULLSCREEN};

mpr_static(stretchSW_mpr, 16, 16, 1, stretchSW_cursorimage);
struct cursor wmgr_stretchSW_cursor={3,13, PIX_SRC, &stretchSW_mpr, FULLSCREEN};
