#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttytlsw.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Flavor of ttysw that knows about tool windows and allows tty based
 *	programs to set/get data about the tool window (common routines).
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_lock.h>
#include <suntool/icon.h>
#include <suntool/icon_load.h>
#include <suntool/tool.h>
#include <suntool/wmgr.h>
#include <suntool/ttysw.h>
#include <suntool/ttysw_impl.h>
#include <suntool/ttytlsw_impl.h>

extern	char	* sprintf();
extern	char	* strncpy();

ttytlsw_setup(ttytlsw, ttysw)
	struct ttytoolsubwindow *ttytlsw;
	struct ttysubwindow *ttysw;
{
	int ttytlsw_string(), ttytlsw_escape();

	/* Initialize ttytlsw */
	ttytlsw->ttysw = ttysw;
	ttytlsw->hdrstate = HS_BEGIN;
	ttytlsw->cached_stringop = ttysw->ttysw_stringop;
	ttytlsw->cached_escapeop = ttysw->ttysw_escapeop;
	/* Setup ttytlsw handlers */
	ttysw->ttysw_client = (caddr_t) ttytlsw;
	ttysw->ttysw_stringop = ttytlsw_string;
	ttysw->ttysw_escapeop = ttytlsw_escape;
}

ttytlsw_cleanup(ttysw)
	struct ttysubwindow *ttysw;
{
	free(ttysw->ttysw_client);
	ttysw->ttysw_client = NULL;
}

ttytlsw_escape(ttysw_client, c, ac, av)
	Ttysubwindow ttysw_client;
	char c;
	register int ac, *av;
{
	struct ttytoolsubwindow *ttytlsw;
	struct tool *tool;
	int toolfd;
	int rootfd;
	char name[WIN_NAMESIZE];
	char buf[150];
	char *p;
	struct rect rect, orect;
	Ttysw	*ttysw;

	ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
	ttytlsw = (struct ttytoolsubwindow *)(LINT_CAST(ttysw->ttysw_client));
	if (c != 't')
		return((*ttytlsw->cached_escapeop)(ttysw, c, ac, av));
	tool = ttytlsw->tool;
	toolfd = tool->tl_windowfd;
	/*
	 * Get root window handle.
	 */
	(void)win_numbertoname(win_getlink(toolfd, WL_PARENT), name);
	if ((rootfd = open(name, O_RDONLY, 0)) < 0) {
		(void) fprintf(stderr, "ttytlswsw: can't find root window\n");
		return (TTY_DONE);
	}
	switch (av[0]) {
	case 1:		/* open */
		wmgr_open(toolfd, rootfd);
		break;
	case 2:		/* close */
		(void)wmgr_background_close(toolfd);	/* Hack: See wmgr_state.c */
		wmgr_close(toolfd, rootfd);
		break;
	case 3:		/* move */
		if (ac == 1 && wmgr_iswindowopen(toolfd)) {
			wmgr_move(toolfd);
			break;
		}
		(void)tool_getnormalrect(tool, &rect);
		orect = rect;
		if (av[1] < 0)
			av[1] = rect.r_top;
		if (ac < 3 || av[2] < 0)
			av[2] = rect.r_left;
		rect.r_top = av[1];
		rect.r_left = av[2];
		if (wmgr_iswindowopen(toolfd))
			wmgr_completechangerect(toolfd, &rect, &orect,
			    0, 0);
		else
			(void)tool_setnormalrect(tool, &rect);
		break;
	case 4:		/* stretch */
		if (ac == 1 && wmgr_iswindowopen(toolfd)) {
			wmgr_stretch(toolfd);
			break;
		}
		(void)tool_getnormalrect(tool, &rect);
		orect = rect;
		if (av[1] < 0)
			av[1] = rect.r_height;
		if (ac < 3 || av[2] < 0)
			av[2] = rect.r_width;
		rect.r_width = av[2];
		rect.r_height = av[1];
		if (wmgr_iswindowopen(toolfd))
			wmgr_completechangerect(toolfd, &rect, &orect,
			    0, 0);
		else
			(void)tool_setnormalrect(tool, &rect);
		break;
	case 5:		/* top */
		wmgr_top(toolfd, rootfd);
		break;
	case 6:		/* bottom */
		wmgr_bottom(toolfd, rootfd);
		break;
	case 7:		/* refresh */
		wmgr_refreshwindow(toolfd);
		break;
	case 8:		/* stretch, size in chars */
		if (ac == 1 && wmgr_iswindowopen(toolfd)) {
			wmgr_stretch(toolfd);
			break;
		}
		(void)tool_getnormalrect(tool, &rect);
		orect = rect;
		if (av[1] <= 0)
			av[1] = tool_linesfromheight(tool, rect.r_height);
		if (ac < 3 || av[2] <= 0)
			av[2] = tool_columnsfromwidth(tool, rect.r_width);
		rect.r_width = tool_widthfromcolumns(av[2]);
		rect.r_height = tool_heightfromlines(av[1],
					    tool->tl_flags&TOOL_NAMESTRIPE);
		if (wmgr_iswindowopen(toolfd))
			wmgr_completechangerect(toolfd, &rect, &orect,
			    0, 0);
		else
			(void)tool_setnormalrect(tool, &rect);
		break;
    /*
     * Report-generating tty escape sequences (case 11 thru 21)
     * must be handled delicately.
     */

	case 11:	/* report open or iconic */
		if (wmgr_iswindowopen(toolfd))
			p = "\33[1t";
		else
			p = "\33[2t";
		ttysw_append_to_ibuf(ttysw, p, 4);
	        break;
	case 13:	/* report position */
		(void)tool_getnormalrect(tool, &rect);
		(void) sprintf(buf, "\33[3;%d;%dt", rect.r_top, rect.r_left);
	        ttysw_append_to_ibuf(ttysw, buf, strlen(buf));
		break;
	case 14:	/* report size */
		(void)tool_getnormalrect(tool, &rect);
		(void) sprintf(buf, "\33[4;%d;%dt", rect.r_height, rect.r_width);
	        ttysw_append_to_ibuf(ttysw, buf, strlen(buf));
		break;
	case 18:	/* report size in chars */
		(void)tool_getnormalrect(tool, &rect);
		(void) sprintf(buf, "\33[8;%d;%dt",
		    tool_linesfromheight(tool, rect.r_height),
		    tool_columnsfromwidth(tool, rect.r_width));
	        ttysw_append_to_ibuf(ttysw, buf, strlen(buf));
		break;
	case 20: {	/* report icon label */
		struct icon *icon = ttytlsw->tool->tl_icon;

	        ttysw_append_to_ibuf(ttysw, "\033]L", 3);
		if (icon && icon->ic_text)
	                ttysw_append_to_ibuf(ttysw, icon->ic_text, 
			    strlen(icon->ic_text));
		else if (ttytlsw->tool->tl_name)
	                ttysw_append_to_ibuf(ttysw, ttytlsw->tool->tl_name,
			    strlen(ttytlsw->tool->tl_name));
	        ttysw_append_to_ibuf(ttysw, "\033\\", 2);
		break;
		}
	case 21:	/* report name stripe */
	        ttysw_append_to_ibuf(ttysw, "\033]l", 3);
		if (ttytlsw->tool->tl_name)
	                ttysw_append_to_ibuf(ttysw, ttytlsw->tool->tl_name,
			    strlen(ttytlsw->tool->tl_name));
	        ttysw_append_to_ibuf(ttysw, "\033\\", 2);
		break;
	default:
		return (TTY_OK);
	}
	(void) close(rootfd);
	return (TTY_DONE);
}

ttytlsw_string(data, type, c)
	caddr_t data;
	char type; 
        unsigned char c;
{
	struct ttysubwindow *ttysw = (struct ttysubwindow *)LINT_CAST(data);
	struct ttytoolsubwindow *ttytlsw =
	    (struct ttytoolsubwindow *)LINT_CAST(ttysw->ttysw_client);

	if (type != ']')
		return((*ttytlsw->cached_stringop)(data, type, c));
	switch (ttytlsw->hdrstate) {
	case HS_BEGIN:
		switch (c) {
		case 'l':
			ttytlsw->nameptr = ttytlsw->namebuf;
			ttytlsw->hdrstate = HS_HEADER;
			break;
		case 'I':
			ttytlsw->nameptr = ttytlsw->namebuf;
			ttytlsw->hdrstate = HS_ICONFILE;
			break;
		case 'L':
			ttytlsw->nameptr = ttytlsw->namebuf;
			ttytlsw->hdrstate = HS_ICON;
			break;
		case '\0':
			break;
		default:
			ttytlsw->hdrstate = HS_FLUSH;
			break;
		}
		break;
	case HS_HEADER:
	case HS_ICONFILE:
	case HS_ICON:
		if (isprint(c)) {
			if (ttytlsw->nameptr <
			    &ttytlsw->namebuf[sizeof (ttytlsw->namebuf) - 1])
				*ttytlsw->nameptr++ = c;
		} else if (c == '\0') {
			*ttytlsw->nameptr = '\0';
			switch (ttytlsw->hdrstate) {
			case HS_HEADER: {
				char namestripe[150];
				(void) strncpy(namestripe, ttytlsw->namebuf,
				    sizeof(namestripe));
				(void)tool_set_attributes(ttytlsw->tool,
				    WIN_LABEL, namestripe, 0);
				break;
				}
			case HS_ICONFILE: {
				char err[IL_ERRORMSG_SIZE];
				struct pixrect *mpr;
		      /* oldmpr not initialized to keep it in data segment */
                                static struct pixrect *oldmpr;

				if ((mpr = icon_load_mpr(ttytlsw->namebuf,
				    err)) == (struct pixrect *)0) {
					(void) fprintf(stderr, err);
				} 
				else {
				    (void)tool_set_attributes(ttytlsw->tool,
					    WIN_ICON_IMAGE, mpr, 0);
				    if (oldmpr)
					pr_destroy (oldmpr);
				    oldmpr = mpr;
				}
				break;
			}
			case HS_ICON: {
				char iconlabel[33];
				(void) strncpy(iconlabel, ttytlsw->namebuf,
				    sizeof(iconlabel));
				(void)tool_set_attributes(ttytlsw->tool,
				    WIN_ICON_LABEL, iconlabel, 0);
				break;
				}
			default: {}
			}
			ttytlsw->hdrstate = HS_BEGIN;
		}
		break;
	case HS_FLUSH:
		if (c == '\0')
			ttytlsw->hdrstate = HS_BEGIN;
		break;
	default:
		return((*ttytlsw->cached_stringop)(data, type, c));
	}
	return (TTY_DONE);
}

