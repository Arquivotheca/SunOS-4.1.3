/*	@(#)win_struct.h 1.1 92/07/30 SMI	*/
#ifndef sunwindow_win_struct_DEFINED
#define sunwindow_win_struct_DEFINED

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This header file defines the library interface to most of
 * the kernel functions of the window system.  The remainder
 * functions are defined in pixwin.h.
 *
 * The kernel maintains a tree structure per-screen of
 * the relationship between the windows on the screen.
 * The top-level window in each screen is called the
 * root window and is generally owned by a screen manager.
 * Subwindows of the root window are generally owned by
 * ``tools'', which themselves may consist of multiple
 * windows at deeper levels.  The window manager deals
 * with the tools, moving the deeper levels en-masse.
 *
 * A window is created by calling win_getnewwindow().
 * An existing window can be opened multiple times by multiple processes and
 * the file descriptor is that which is used as the "ticket" to change
 * something about a window.
 *
 * Each window tracks the cursor with a shape...
 *
 * Each window has a parent, a place within a set of siblings
 * (specified as the oldest and youngest siblings), and a
 * set of children (specified as a oldest and youngest child).
 * These links are maintained by the system, and checked for
 * consistency when windows are inserted into and deleted from
 * the window tree.
 *
 * Each window competes for input.  Whenever keyboard or mouse
 * actions occur, the system gives the window in which the cursor
 * is located a chance at the input.  If that window is not
 * interested in the input, then its parent is given a chance,
 * and so on.  If no one is interested, the input is discarded.
 * Input from a window may cause interrupts to be sent to the
 * ``owner process'' for the window.
 *
 * Each window also maintains a ``clipping list'' and a ``damaged list''.
 * The clipping list gives the set of rectangles which are currently
 * exposed for the window.  The damaged list gives the set of
 * rectangles which are exposed, but known to be damaged by
 * recent window rearrangements, and which should therefore be
 * repainted by the responsible processes.
 */

#define	WIN_NAMESIZE	20

/*
 * Link names.
 */
#define	WL_PARENT		0
#define	WL_OLDERSIB		1
#define	WL_YOUNGERSIB		2
#define	WL_OLDESTCHILD		3
#define	WL_YOUNGESTCHILD	4

#define	WL_ENCLOSING		WL_PARENT
#define	WL_COVERED		WL_OLDERSIB
#define	WL_COVERING		WL_YOUNGERSIB
#define	WL_BOTTOMCHILD		WL_OLDESTCHILD
#define	WL_TOPCHILD		WL_YOUNGESTCHILD

#define	WIN_LINKS		5
#define	WIN_NULLLINK		-1

/*
 * Flag to use during open of window device when it is supposed to be the
 * first time opened (exclusive open).  O_EXCL in file.h currently has wrong
 * mapping to FEXLOCK.  Need a FEXCLOPEN in file.h.
 */
#define	WIN_EXCLOPEN		0x10000

#ifdef cplus
/*
 * C Library routine specifications relating
 * to kernel supported window functions.
 */

/*
 * Error handling (overriding library error handler, returns previous handler)
 */
int	win_errorhandler(int *win_error(int errnum, winopnum));

/*
 * Tree operations.
 */
int	win_getlink(int windowfd, int linkname);
void	win_setlink(int windowfd, int linkname, number);
void	win_insert(int windowfd);
void	win_remove(int windowfd);
int	win_nextfree(int windowfd);

/* utilities */
void	win_numbertoname(int number, char *name);
int	win_nametonumber(char *name);
void	win_fdtoname(int windowfd, char *name);
int	win_fdtonumber(int windowfd);
int	win_getnewwindow();

/*
 * Mouse cursor operations.
 */
void	win_setmouseposition(int windowfd, short x, y);
void	win_setcursor(int windowfd, struct cursor *cursor);
void	win_getcursor(int windowfd, struct cursor *cursor);
int	win_findintersect(int windowfd, short x, y);

/*
 * Geometry operations.
 */
void	win_getrect(int windowfd, struct rect *rect);
void	win_setrect(int windowfd, struct rect *rect);
void	win_setsavedrect(int windowfd, struct rect *rect);
void	win_getsavedrect(int windowfd, struct rect *rect);

/* utilities */
void	win_getsize(int windowfd, struct rect *rect);
coord	win_getheight(int windowfd);
coord	win_getwidth(int windowfd);

/* blanket window operations */
int	win_insertblanket(int windowfd, parentfd);
void	win_removeblanket(int windowfd);
int	win_isblanket(int windowfd);

/*
 * Misc operations.
 */
int	win_getuserflags(int windowfd);
void	win_setuserflags(int windowfd, flags);
void	win_setowner(int windowfd, pid);
int	win_getowner(int windowfd);

/* utilities */
void	win_setuserflag(win windowfd, int flag, bool value);

/*
 * Input control.
 */
void	win_getinputmask(int windowfd, struct inputmask *im,
	    int *inputredirectwindownumber);
void	win_setinputmask(int windowfd, struct inputmask *im_set, *im_flush,
	    int inputredirectwindownumber);

/*
 * Kernel operations applying globally to a screen.
 */
void	win_lockdata(int windowfd);
void	win_computeclipping(int windowfd);
void	win_partialrepair(int windowfd, struct rect *rectok);
void	win_unlockdata(int windowfd);
void	win_grabio(int windowfd);
void	win_releaseio(int windowfd);

/*
 * Display management operations (see pixwin.h).
 */

/*
 * Screen creation, inquiry and deletion.
 */
int	win_screennew(struct screen *screen);
void	win_screenget(int windowfd, struct screen *screen);
void	win_screendestroy(int windowfd);
void	win_setscreenpositions(int windowfd, int neighbors[SCR_POSITIONS]);
void	win_getscreenpositions(int windowfd, int neighbors[SCR_POSITIONS]);
#endif cplus

#endif sunwindow_win_struct_DEFINED

