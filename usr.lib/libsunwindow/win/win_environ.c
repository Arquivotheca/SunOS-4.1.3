#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)win_environ.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Implement the win_environ.h & win_sig.h interfaces.
 * (see win_ttyenv.c for other functions)
 */

#include <sunwindow/rect.h>
#include <sunwindow/win_environ.h>
#include <sunwindow/win_struct.h>
extern char *sprintf();

/*
 * Public routines
 */
we_setparentwindow(windevname)
	char	*windevname;
{
	(void)setenv(WE_PARENT, windevname);
}

int
we_getparentwindow(windevname)
	char	*windevname;
{
	return(_we_setstrfromenvironment(WE_PARENT, windevname));
}

we_setinitdata(initialrect, initialsavedrect, iconic)
	struct	rect *initialrect, *initialsavedrect;
	int	iconic;
{
	char rectstr[60];

	rectstr[0] = '\0';
	(void)sprintf(rectstr, "%04d,%04d,%04d,%04d,%04d,%04d,%04d,%04d,%04ld",
		initialrect->r_left, initialrect->r_top,
		initialrect->r_width, initialrect->r_height,
		initialsavedrect->r_left, initialsavedrect->r_top,
		initialsavedrect->r_width, initialsavedrect->r_height,
		iconic);
	(void)setenv(WE_INITIALDATA, rectstr);
}

int
we_getinitdata(initialrect, initialsavedrect, iconic)
	struct	rect *initialrect, *initialsavedrect;
	int	*iconic;
{
	char rectstr[60];

	if (_we_setstrfromenvironment(WE_INITIALDATA, rectstr))
		return(-1);
	else {
		if (sscanf(rectstr, "%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd",
		    &initialrect->r_left, &initialrect->r_top,
		    &initialrect->r_width, &initialrect->r_height,
		    &initialsavedrect->r_left, &initialsavedrect->r_top,
		    &initialsavedrect->r_width, &initialsavedrect->r_height,
		    iconic)!=9)
			return(-1);
		return(0);
	}
}

we_clearinitdata()
{
	(void)unsetenv(WE_INITIALDATA);
}

we_setgfxwindow(windevname)
	char	*windevname;
{
	(void)setenv(WE_GFX, windevname);
}

int
we_getgfxwindow(windevname)
	char	*windevname;
{
	return(_we_setstrfromenvironment(WE_GFX, windevname));
}

we_setmywindow(windevname)
	char	*windevname;
{
	(void)setenv(WE_ME, windevname);
}

