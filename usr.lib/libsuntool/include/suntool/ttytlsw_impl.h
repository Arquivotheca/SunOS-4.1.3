/*	@(#)ttytlsw_impl.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#ifdef	notdef

	This is a flavor of ttysw that knows about tool windows and allows
	tty based programs to set/get data about the tool window.

	This subwindow is designed primarily to be the only one in a
	tool. When multiple subwindows are in the tool and the size of
	the tool window is changed from the tty based program, the
	resultant tty subwindow wouldn't be what the tty program
	requested.

	The modification of the ttysw's behavior is done by interposing
	ttytlsw procedures in front of std ttysw procedures.  A struct
	ttytoolsubwindow pointer is placed in the ttysw_client field
	of the struct ttysubwindow.

#endif	notdef

enum ttytlsw_hdrstate { HS_BEGIN, HS_HEADER, HS_ICON, HS_ICONFILE, HS_FLUSH };

struct	ttytoolsubwindow {
	struct	tool *tool;		/* tool residing in */
	struct	ttysubwindow *ttysw;	/* ttysw behavior modifying */
	enum	ttytlsw_hdrstate hdrstate;/* which string trying to load */
	int	(*cached_destroyop)();	/* from toolsw->ts_destroy */
	int	(*cached_stringop)();	/* from ttysw->ttysw_stringop */
	int	(*cached_escapeop)();	/* from ttysw->ttysw_escapeop */
	char	*nameptr;		/* namebuf ptr */
	char	namebuf[256];		/* buffer for accumulating esc string */
};

