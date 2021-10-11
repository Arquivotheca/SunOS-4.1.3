/*	@(#)win_environ.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * The window library uses the ENVIRONMENT to pass a limited amount of
 * window information to newly created child processes.
 * A collection of conventions dictate the use of these values.
 */

#define	WE_PARENT		"WINDOW_PARENT"
#define	WE_INITIALDATA		"WINDOW_INITIALDATA"
#define	WE_GFX			"WINDOW_GFX"
#define	WE_ME			"WINDOW_ME"

#ifdef	cplus
/*
 * C Library routines specifically related to ENVIRONMENT conventions.
 */

/*
 * Get/set parent of window that is being created.
 */
void	we_setparentwindow(char *windevname);
int	we_getparentwindow(char *windevname);

/*
 * Get/set initial rect (parent relative) of window that is being created,
 * initial saved rect and whether or not iconic.
 * Values should be cleared so that no one mistakenly uses this data once the
 * information is used once.
 */
void   we_setinitrect(struct rect *initialrect, *initialsavedrect, int *iconic);
int    we_getinitrect(struct rect *initialrect, *initialsavedrect, int *iconic);
void   we_clearinitdata();

/*
 * Get/set window that can be taken over by graphics programs.
 */
void	we_setgfxwindow(char *windevname);
int	we_getgfxwindow(char *windevname);

/*
 * Get/set window that this process is currently running in.
 * Used by tty programs when they get a SIGWINCH so can readjust notion
 * of terminal size (see following comment).
 */
void	we_setmywindow(char *windevname);
int	we_getmywindow(char *windevname);

#endif
