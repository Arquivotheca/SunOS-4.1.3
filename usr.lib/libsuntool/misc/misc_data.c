#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E%";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * misc_data.c: fix for shared libraries in SunOS4.0.  Code was 
 *		 isolated from cheap_text.c and csr_change.c
 */

#include <sys/types.h>
#include <fcntl.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/cursor_impl.h>
#include <suntool/cheap_text.h>

extern	text_obj *cheap_createtext();
extern	u_int	  cheap_commit();
extern	text_obj *cheap_destroy();
extern	u_int	  cheap_sizeof_entity();
extern	u_int	  cheap_get_length();
extern	u_int	  cheap_get_position();
extern	u_int	  cheap_set_position();
extern	u_int	  cheap_read();
extern	u_int	  cheap_replace();
extern	u_int	  cheap_edit();

text_ops	cheap_text_ops  =  {
    cheap_createtext,
    cheap_commit,
    cheap_destroy,
    cheap_sizeof_entity,
    cheap_get_length,
    cheap_get_position,
    cheap_set_position,
    cheap_read,
    cheap_replace,
    cheap_edit
};

static	short cached_cursorimage[CUR_MAXIMAGEWORDS];
Pixrect	cached_mpr;
mpr_static(cached_mpr, 16, 16, 1, cached_cursorimage);

static	short menu_cursorimage[12] = {
	0x0000,	/*              	  */
	0x0020,	/*            # 	  */
	0x0030,	/*            ##	  */
	0x0038,	/*            ###	  */
       	0x003c,	/*            ####	  */
	0xfffe,	/*  ###############	  */
	0xffff,	/*  ################ X 	  */
	0xfffe,	/*  ###############	  */
       	0x003c,	/*            ####	  */
	0x0038,	/*            ###	  */
	0x0030,	/*            ##	  */
	0x0020	/*            # 	  */
};
Pixrect menu_mpr;
mpr_static(menu_mpr, 16, 12, 1, menu_cursorimage);
struct	cursor menu_cursor = { 17, 6, PIX_SRC|PIX_DST, &menu_mpr, FULLSCREEN};
