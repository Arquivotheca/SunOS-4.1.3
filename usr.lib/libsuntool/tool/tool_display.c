#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_display.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Handle tool displaying and size changes.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <strings.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_input.h>
#include <sunwindow/cms.h>
#include <sunwindow/cms_mono.h> 
#include <sunwindow/win_screen.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/icon.h>
#include <suntool/tool.h>
#include <suntool/tool_impl.h>
#include <suntool/wmgr.h>
#include <suntool/tool_commons.h>
extern	struct pixfont *pf_sys;

static void flushline();

tool_handlesigwinchstd(tool)
	register struct	tool *tool;
{
	struct	rect rect;
	int	flags;

	tool->tl_flags &= ~TOOL_SIGWINCHPENDING;
	flags = win_getuserflags(tool->tl_windowfd);
	(void)win_getsize(tool->tl_windowfd, &rect);
	if ((flags&WMGR_ICONIC) && (~tool->tl_flags&TOOL_ICONIC)) {
		/*
		 * Tool has just gone iconic, so, add y offset to sws
		 * to move them out of the picture.
		 */
		(void)_tool_addyoffsettosws(tool, 2048);
		tool->tl_flags |= TOOL_ICONIC;
		tool->tl_rectcache = rect;
	} else if ((~flags&WMGR_ICONIC) && (tool->tl_flags&TOOL_ICONIC)) {
		/*
		 * Tool has just gone from iconic to normal, so, subtract
		 * y offset from sws to move them into the picture again.
		 */
		tool->tl_flags &= ~TOOL_ICONIC;
		(void)_tool_addyoffsettosws(tool, -2048);
		tool->tl_rectcache = rect;
	} if (!rect_equal(&tool->tl_rectcache, &rect)) {
		/*
		 * Size changed so adjust subwindows
		 */
		tool->tl_rectcache = rect;
		(void)tool_layoutsubwindows(tool);
	}
	/*
	 * Refresh tool now
	 */
	(void)pw_damaged(tool->tl_pixwin);
#ifdef solidborder
	(void)_tool_display(tool, FALSE);
#endif
	(void)pw_donedamaged(tool->tl_pixwin);
#ifndef solidborder
	(void)_tool_display(tool, FALSE);
#endif
	/*
	 * Refresh subwindows now
	 */
	(void)_tool_subwindowshandlesigwinch(tool);
	return;
}

tool_display(tool)
	struct	tool *tool;
{
	tool->tl_flags &= ~TOOL_REPAINT_LOCK;
	(void)_tool_display(tool, TRUE);
	return;
}

tool_displayicon(tool)
	register struct	tool *tool;
{
	if (tool->tl_flags&TOOL_ICONIC) {
		tool->tl_flags &= ~TOOL_REPAINT_LOCK;
		if (tool->tl_icon)
			icon_display(tool->tl_icon, tool->tl_pixwin, 0, 0);
		else
			(void)_tool_displaydefaulticon(tool);
	}
}

tool_displaynamestripe(tool)
	register struct	tool *tool;
{
	if (tool->tl_flags&TOOL_ICONIC) {
		/* Tag on icon may have changed */
		(void)tool_displayicon(tool);
		return;
	}
	if (tool->tl_flags&TOOL_NAMESTRIPE) {
		tool->tl_flags &= ~TOOL_REPAINT_LOCK;
		(void)pw_writebackground(tool->tl_pixwin, 0, 0,
		    tool->tl_rectcache.r_width,
		    tool_headerheight(tool->tl_flags&TOOL_NAMESTRIPE),
		    PIX_SET);
		if (tool->tl_name) {
		    (void)pw_text(tool->tl_pixwin,
			    TOOL_BORDERWIDTH-
				pf_sys->pf_char[*(tool->tl_name)].pc_home.x+1,
			    0-pf_sys->pf_char[*(tool->tl_name)].pc_home.y+1,
			    PIX_NOT(PIX_SRC), pf_sys, tool->tl_name);
		    if (tool->tl_flags&TOOL_EMBOLDEN_LABEL)
			(void)pw_text(tool->tl_pixwin,
				TOOL_BORDERWIDTH-
				 pf_sys->pf_char[*(tool->tl_name)].pc_home.x+2,
				0-pf_sys->pf_char[*(tool->tl_name)].pc_home.y+1,
				PIX_NOT(PIX_SRC)&PIX_DST,
				pf_sys, tool->tl_name);
		}
	}
}

tool_displaytoolonly(tool)
	register struct	tool *tool;
{
	if (tool->tl_flags&TOOL_ICONIC) {
		(void)tool_displayicon(tool);
		return;
	}
	tool->tl_flags &= ~TOOL_REPAINT_LOCK;
	(void)pw_lock(tool->tl_pixwin, &tool->tl_rectcache);
	(void)_tool_displayborders(tool);
	(void)tool_displaynamestripe(tool);
	(void)pw_unlock(tool->tl_pixwin);
	return;
}

/*
 * Utilities
 */
_tool_addyoffsettosws(tool, yoffset)
	struct	tool *tool;
	short	yoffset;
{
	register struct	toolsw *sw;
	struct	rect swrect;

	(void)win_lockdata(tool->tl_windowfd);
	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
		(void)win_getrect(sw->ts_windowfd, &swrect);
		swrect.r_top += yoffset;
		(void)win_setrect(sw->ts_windowfd, &swrect);
	}
	(void)win_unlockdata(tool->tl_windowfd);
	return;
}

_tool_display(tool, swstoo)
	register struct	tool *tool;
	bool	swstoo;
{
	if (tool->tl_flags&TOOL_ICONIC)
		(void)tool_displayicon(tool);
	else {
		if (swstoo==TRUE)
			(void)_tool_subwindowshandlesigwinch(tool);
		(void)tool_displaytoolonly(tool);
	}
	return;
}

_tool_displayborders(tool)
	register struct	tool *tool;
{
	struct	rect rect;
	short	stht = tool_stripeheight(tool);
	register struct	toolsw *sw;

	rect = tool->tl_rectcache;
	if (tool->tl_flags&TOOL_NAMESTRIPE) {
		rect.r_top += stht;
		rect.r_height -= stht;
	}
		    
	(void)pw_lock(tool->tl_pixwin, &rect);
	(void)_tool_draw_box(tool->tl_pixwin, PIX_SET, &rect, 2, BLACK);
	rect_marginadjust(&rect, -2);
	(void)pw_writebackground(tool->tl_pixwin, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_CLR);

	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
		if (((Toolsw_priv *)(LINT_CAST(sw->ts_priv)))->have_kbd_focus != TRUE) {
			(void)_tool_displayswborders(tool, sw);
		}
	}
	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
		if (((Toolsw_priv *)(LINT_CAST(sw->ts_priv)))->have_kbd_focus == TRUE) {
			(void)_tool_displayswborders(tool, sw);
		}
	}
	(void)pw_unlock(tool->tl_pixwin);
}

/*
 *  display borders of subwindow sw.
 *  if sw has keyboard focus, highlight the border.
 */
_tool_displayswborders(tool, sw)
	register struct	tool	*tool;
	register struct	toolsw	*sw;
{
        struct  rect rect;
        short   stht = tool_stripeheight(tool);
        short	hdrht = tool_headerheight(tool->tl_flags&TOOL_NAMESTRIPE);
        int	width;
	Toolsw_priv	*sw_priv;
        
        (void)win_getrect(sw->ts_windowfd, &rect);
	rect_marginadjust(&rect, 3);
	sw_priv = (Toolsw_priv *)(LINT_CAST(sw->ts_priv));
	
        if (tool->tl_flags&TOOL_NAMESTRIPE && rect.r_top+3 == hdrht) {
		rect.r_height -= 3;
		rect.r_top += 3;
		draw_3sides(tool->tl_pixwin, PIX_CLR, &rect, 1, WHITE);
		rect.r_height += 3;
		rect.r_top -= 3;
	} else {
		(void)_tool_draw_box(tool->tl_pixwin, PIX_CLR, &rect, 1, WHITE);
	}
	
	if (sw_priv != (Toolsw_priv *)0 && sw_priv->have_kbd_focus == TRUE) {
        	width = 3;
	} else {
        	rect_marginadjust(&rect, -1);
        	width = 2;
	}
        if (tool->tl_flags&TOOL_NAMESTRIPE && rect.r_top < stht) {
		draw_3sides(tool->tl_pixwin, PIX_SET, &rect, width, BLACK);
        } else {
		(void)_tool_draw_box(tool->tl_pixwin, PIX_SET, &rect, width, BLACK);
        }
}

_tool_subwindowshandlesigwinch(tool)
	struct	tool *tool;
{
	register struct	toolsw *sw;

	for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
		if (sw->ts_io.tio_handlesigwinch)
			sw->ts_io.tio_handlesigwinch(sw->ts_data);
		/* Notifier based subwindows are handled separately */
	}
	return;
}

_tool_displaydefaulticon(tool)
	register struct	tool *tool;
{
	struct	rect rect;

	rect = tool->tl_rectcache;
	(void)pw_writebackground(tool->tl_pixwin, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_SET);
	rect_marginadjust(&rect, -tool_borderwidth(tool));
	(void)pw_writebackground(tool->tl_pixwin, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_CLR);
	rect_marginadjust(&rect, -tool_borderwidth(tool));
	(void)formatstringtorect(tool->tl_pixwin, tool->tl_name, pf_sys, &rect);
	return;
}

/*
 * Display utilities.  Note: Belongs somewhere else.
 */
#define	newline(x, y, w, h, chrht, rect) \
	{ *x = rect->r_left; \
	  *y += chrht; \
	  *h -= chrht; \
	  *w = rect->r_width; \
	}

formatstringtorect(pixwin, s, font, rect)
	struct	pixwin *pixwin;
	char	*s;
	struct	pixfont *font;
	register struct	rect *rect;
{
	register char *charptr, *lineptr, *breakptr, c;
	short	x, y, w, h, chrht, chrwth;
	extern	PIXFONT *pw_pfnull();
#ifdef lint
	short dummy;
#endif

	if (s == 0)
		return;
	/* Use default system font if none supplied */
	font = pw_pfnull(font);
	x = rect->r_left;
#ifdef lint
	dummy = x;
	x = dummy;
#endif
	y = rect->r_top;
	w = rect->r_width;
	h = rect->r_height;
	chrht = font->pf_defaultsize.y;
	breakptr = lineptr = s;
	for (charptr = s;*charptr!='\0';charptr++) {
		c = (*charptr)&127;
		/*
		 * Setup to wrap on blanks
		 * Note: Need better break test.
		 */
		 if (c==' ') {
			breakptr = charptr;
		 }
		chrwth = font->pf_char[c].pc_adv.x;
		/*
		 * Wrap when not enough room for next char
		 */
	 	if (w<chrwth) {
			if (breakptr != lineptr) {
				flushline(pixwin, rect->r_left, y,
				    lineptr, breakptr, font);
				charptr = breakptr;
				lineptr = ++breakptr;
				continue;
			} else {
				flushline(pixwin, rect->r_left, y,
				    lineptr, charptr, font);
				breakptr = lineptr = charptr;
			}
			newline(&x, &y, &w, &h, chrht, rect);
		}
		/*
		 * Punt when run out of vertical space
		 */
		if (h<chrht)
			break;
		w -= chrwth;
		x += chrwth;
	}
	flushline(pixwin, rect->r_left, y, lineptr, charptr, font);
	/*
	 * Note: We should release default font if null font was specified
	 * (using pw_pfsysclose).  However, there is such an extreme
	 * performance problem if continually opening and closing the
	 * default font that we don't do this.  The reason that we can
	 * justify this is that the default font is usually shared between
	 * multiple packages, and once opened, is not a drain on resources.
	 */
}

static void
flushline(pixwin, x, y, lineptr, charptr, font)
	struct	pixwin *pixwin;
	int	x, y;
	char	*lineptr, *charptr;
	struct	pixfont *font;
{
#define	STRBUF_LEN	1000
	char	strbuf[STRBUF_LEN], *strbufptr = &strbuf[0];
	int	len = charptr - lineptr;

	if (charptr==0 || lineptr==0 || len > STRBUF_LEN)
		return;
	(void)strncpy(strbufptr, lineptr, len);
	*(strbufptr+len) = '\0';
	(void)pw_text(pixwin, x-font->pf_char[*strbufptr].pc_home.x,
	    y-font->pf_char[*strbufptr].pc_home.y, PIX_SRC, font, strbufptr);
}

/*ARGSUSED*/
_tool_draw_box(pixwin, op, r, w, color)
	register struct	pixwin *pixwin;
	register int	op;
	register struct rect *r;
	register int	w;
	int	color;
{
	struct rect rectlock;

	/*
	 * Draw top, left, right then bottom.
	 * Note: should be pw_writebackground.
	 */
	rectlock = *r;
	rect_marginadjust(&rectlock, w);
	(void)pw_lock(pixwin, &rectlock);
	(void)pw_writebackground(
	    pixwin, r->r_left, r->r_top, r->r_width, w, op);
	(void)pw_writebackground(pixwin, r->r_left, r->r_top + w,
	    w, r->r_height - 2*w, op);
	(void)pw_writebackground(pixwin, r->r_left + r->r_width - w, r->r_top + w,
	    w, r->r_height - 2*w, op);
	(void)pw_writebackground(pixwin, r->r_left, r->r_top + r->r_height - w,
	    r->r_width, w, op);
	(void)pw_unlock(pixwin);
	return;
}

/*
 * Draw the lower 3 sides of a box.
 */
/*ARGSUSED*/
static
draw_3sides(pixwin, op, r, w, color)
	register struct	pixwin *pixwin;
	int	op;
	register struct rect *r;
	register int	w;
	int	color;
{
	struct rect rectlock;

	/*
	 * Draw left, right then bottom.
	 */
	rectlock = *r;
	rect_marginadjust(&rectlock, w);
	(void)pw_lock(pixwin, &rectlock);
	(void)pw_writebackground(pixwin, r->r_left, r->r_top,
	    w, r->r_height - w, op);
	(void)pw_writebackground(pixwin, r->r_left + r->r_width - w, r->r_top,
	    w, r->r_height - w, op);
	(void)pw_writebackground(pixwin, r->r_left, r->r_top + r->r_height - w,
	    r->r_width, w, op);
	(void)pw_unlock(pixwin);
	return;
}
