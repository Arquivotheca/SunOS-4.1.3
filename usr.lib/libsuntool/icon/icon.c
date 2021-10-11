#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)icon.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 *  icon.c - Display icon.
 */

#include <stdio.h>
#include <pixrect/pixrect_hs.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/icon.h>

void
icon_display(icon, pixwin, x, y)
	register struct icon *icon;
	register struct pixwin *pixwin;
	register int x, y;
{
	struct	rect textrect;
	extern	struct pixrect *tool_bkgrd;

#ifdef i386
	pr_flip(icon->ic_mpr);
	pr_flip(icon->ic_background);
#endif
	if (icon->ic_flags & ICON_BKGRDGRY || icon->ic_flags & ICON_BKGRDPAT) {
		/*
		 * Cover the icon's rect with pattern
		 */
		(void)pw_replrop(pixwin, x, y,
		    icon->ic_width, icon->ic_height, PIX_SRC,
		    (icon->ic_flags & ICON_BKGRDGRY)? tool_bkgrd:
		    icon->ic_background, 0, 0);
	} else if (icon->ic_flags & ICON_BKGRDCLR ||
	    icon->ic_flags & ICON_BKGRDSET) {
		/*
		 * Cover the icon's rect with solid.
		 */
		(void)pw_writebackground(pixwin, x, y,
		    icon->ic_width, icon->ic_height,
		    (icon->ic_flags & ICON_BKGRDCLR)? PIX_CLR: PIX_SET);
	}
	if (icon->ic_mpr)
		/*
		 * Copy image over gray
		 */
		(void)pw_write(pixwin,
		    icon->ic_gfxrect.r_left+x, icon->ic_gfxrect.r_top+y,
		    icon->ic_gfxrect.r_width, icon->ic_gfxrect.r_height,
		    PIX_SRC, icon->ic_mpr, 0, 0);
	if (icon->ic_text && (icon->ic_text[0] != '\0')) {
                extern PIXFONT *pw_pfsysopen();
 
                if (!icon->ic_font) 
                        icon->ic_font = pw_pfsysopen();
		if (rect_isnull(&icon->ic_textrect))
			/* Set text rect to accomodate 1 line at bottom. */
			rect_construct(&icon->ic_textrect,
			    0, icon->ic_height-icon->ic_font->pf_defaultsize.y,
			    icon->ic_width, icon->ic_font->pf_defaultsize.y);
		/* Blank out area onto which text will go. */
		(void)pw_writebackground(pixwin,
		    icon->ic_textrect.r_left+x, icon->ic_textrect.r_top+y,
		    icon->ic_textrect.r_width, icon->ic_textrect.r_height,
		    PIX_CLR);
		/* Format text into textrect */
		textrect = icon->ic_textrect;
		textrect.r_left += x;
		textrect.r_top += y;
		(void)formatstringtorect(pixwin, icon->ic_text, icon->ic_font,
		    &textrect);
	}
}

