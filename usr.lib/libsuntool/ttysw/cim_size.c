#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)cim_size.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Character image initialization, destruction and size changing routines
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_notify.h>
#include "sunwindow/sv_malloc.h"
#include <suntool/tool.h>
	/* tool.h must be before any indirect include of textsw.h */
#include <suntool/ttysw_impl.h>
#include <suntool/ttyansi.h>
#include <suntool/charimage.h>
#include <suntool/charscreen.h>

unsigned char **image;
char **screenmode;
int top, bottom, left, right;
int cursrow, curscol;

static	int maxright, maxbottom;
static	char *imagecharsfree;
static	char *modecharsfree;

/*
 * Initialize initial character image.
 */
imageinit(window_fd)
	int     window_fd;
{
	void	 imagealloc();
	int	 maximagewidth, maximageheight;

	if (wininit(window_fd, &maximagewidth, &maximageheight) == 0)
		return(0);
	top = left = 0;
	curscol = left;
	cursrow = top;
	maxright = x_to_col(maximagewidth);
	if (maxright > 255)
		maxright = 255;		/* line length is stored in a byte */
	maxbottom = y_to_row(maximageheight);
	imagealloc(&image, &screenmode);
	(void)pclearscreen(0, bottom+1);	/* +1 to get remnant at bottom */
	return(1);
}

/*
 * Allocate character image.
 */
void
imagealloc(nimage, nmode)
char ***nimage, ***nmode;
{
	register char	**newimage;
	register char	**newmode;
	register int	i;
	int	 	nchars;
	register char	*line;
	register char	*bold;
	extern char	*calloc();

	/*
	 * Determine new screen dimensions
	 */
	right = x_to_col(winwidthp);
	bottom = y_to_row(winheightp);
	/*
	 * Ensure has some non-zero dimension
	 */
	if (right < 1)
		right = 1;
	if (bottom < 1)
		bottom = 1;
	/*
	 * Bound new screen dimensions
	 */
	right = (right < maxright)? right: maxright;
	bottom = (bottom < maxbottom)? bottom: maxbottom;
	/*
	 * Let pty set terminal size
	 */
	(void)ttynewsize(right, bottom);
	/*
	 * Allocate line array and character storage
	 */
	nchars = right * bottom;
	newimage = (char **)LINT_CAST(sv_calloc(1, (unsigned)(bottom * sizeof (char *))));
	newmode = (char **)LINT_CAST(sv_calloc(1, bottom * sizeof(char *)));
	line = (char *)sv_calloc(1, (unsigned)(nchars + 2 * bottom));
	bold = (char *)sv_calloc(1, nchars + 2 * bottom);
	for( i = 0; i < bottom; i++ ) {
		newimage[i] = line + 1;
		newmode[i] = bold + 1;
		setlinelength(newimage[i], 0);
		line += right + 2;
		bold += right + 2;
	}
	/*
	 * Remember allocation pointer so can free correct one.
	 */
	imagecharsfree = newimage[0]-1;
	modecharsfree = newmode[0]-1;
	*nimage = newimage;
	*nmode = newmode;
}

/*
 * Free character image.
 */
imagefree(imagetofree, imagecharstofree, modetofree, modecharstofree)
	char	**imagetofree, *imagecharstofree;
	char	**modetofree, *modecharstofree;
{
	free((char *)imagecharstofree);
	free((char *)imagetofree);
	free((char *)modecharstofree);
	free((char *)modetofree);
}

/*
 * Called when screen changes size.  This will let lines get longer
 * (or shorter perhaps) but won't re-fold older lines.
 */
imagerepair(ttysw)
	Ttysw	*ttysw;
{
	unsigned char	**oldimage = image;
	char		*oldimagecharsfree = imagecharsfree;
	char		**oldmode = screenmode;
	char		*oldmodecharsfree = modecharsfree;
	register int	oldrow, row;
	int		oldbottom = bottom;
	int		topstart;
	int		clrbottom;

	/*
	 * Get new image and image description
	 */
	imagealloc(&image, &screenmode);
	/*
	 * Clear max of old/new screen (not image).
	 */
	(void)saveCursor();
	clrbottom = (oldbottom < bottom)? bottom+2: oldbottom+2;
	(void)pclearscreen(0, clrbottom);
	(void)restoreCursor();
	/*
	 * Find out where last line of text is (actual oldbottom).
	 */
	for (row = oldbottom;row > top;row--) {
		if (length(oldimage[row-1])) {
			oldbottom = row;
			break;
		}
	}
	/*
	 * Try to preserve bottom (south west gravity) text.
	 * This wouldn't work well for vi and other programs that
	 * know about the size of the terminal but aren't notified of changes.
	 * However, it should work in many cases  for straight tty programs
	 * like the shell.
	 */
	if (oldbottom > bottom)
		topstart = oldbottom-bottom;
	else
		topstart = 0;
	/*
	 * Fill in new screen from old
	 */
	ttysw->ttysw_lpp = 0;
	(void)cim_clear(top, bottom);
	for (oldrow = topstart, row = 0; oldrow < oldbottom; oldrow++, row++) {
		register int	sl = strlen(oldimage[oldrow]);
#ifdef	DEBUG_LINELENGTH_WHEN_WRAP
if (sl != length(oldimage[oldrow]))
	printf("real %ld saved %ld, oldrow %ld, oldbottom %ld bottom %ld\n", sl, length(oldimage[oldrow]), oldrow, oldbottom, bottom);
#endif
		if (sl > right) 
			sl = right;
		bcopy(oldimage[oldrow], image[row], sl);
		bcopy(oldmode[oldrow], screenmode[row], sl);
		setlinelength(image[row], sl);
	}
	/*
	 * Move the cursor to its new position in the new coordinate system.
	 * If the window is shrinking, and thus "topstart" is the number of
	 * rows by which it is shrinking, the row number is decreased by
	 * that number of rows; if the window is growing, and thus "topstart"
	 * is zero, the row number is unchanged.
	 * The column number is unchanged, unless the old column no longer
	 * exists (in which case the cursor is placed at the rightmost column).
	 */
	pos(curscol, cursrow - topstart);
	(void)imagefree(oldimage, oldimagecharsfree, oldmode, oldmodecharsfree);
	(void)pdisplayscreen(0);
}
