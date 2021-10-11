#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)prompt.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Implement the prompt interface described in win_menu.h.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_input.h>
#include <sunwindow/cms.h>		/* to satisfy win_ioctl.h */
#include <sunwindow/win_screen.h>	/* to satisfy win_ioctl.h */
#include <sunwindow/win_ioctl.h>	/* for journaling */
#include <suntool/menu.h>
#include <suntool/fullscreen.h>

#define	PROMPT_FUDGELINES	2

/*
 * journaling
 */
extern	int	sv_journal;
extern	void	win_sync();

extern int
menu_prompt(prompt, inputevent, iowindowfd)
	struct	prompt *prompt;
	struct	inputevent *inputevent;
	int	iowindowfd;
{
	struct	rect rectsaved, rect;
	struct	pr_size strsize;
	extern	struct pr_size pf_textwidth();
	int	extent;
	struct	fullscreen *fs;
	Pw_pixel_cache *pixel_cache;
	int	result = 0;

	/*
	 * Grab all input and disable anybody but windowfd from writing
	 * to screen while we are violating window overlapping.
	 */
	fs = fullscreen_init(iowindowfd);
	if (fs == 0)
		return(1);		/* Probably out of fd's. */
	/*
	 * Save existing image
	 */
	rect = prompt->prt_rect;
	strsize = pf_textwidth(
	    strlen(prompt->prt_text), prompt->prt_font, prompt->prt_text);
	extent = strsize.x;
	if (rect.r_width==PROMPT_FLEXIBLE && rect.r_height==PROMPT_FLEXIBLE) {
		rect.r_width = min(extent/3, fs->fs_screenrect.r_width/2);
		rect.r_height = (extent/rect.r_width+PROMPT_FUDGELINES)*
		    strsize.y+(6*MENU_BORDER);
	} else  {
		if (rect.r_width==PROMPT_FLEXIBLE)
			rect.r_width = extent/
			    (rect.r_height/strsize.y);
		if (rect.r_height==PROMPT_FLEXIBLE)
			rect.r_height = (extent/rect.r_width+PROMPT_FUDGELINES)*
			    strsize.y+(6*MENU_BORDER);
	}
	if (rect.r_top==PROMPT_FLEXIBLE)
		rect.r_top = fs->fs_screenrect.r_top+
		    fs->fs_screenrect.r_height/2-rect.r_height/2;
	if (rect.r_left==PROMPT_FLEXIBLE)
		rect.r_left = fs->fs_screenrect.r_left+
		    fs->fs_screenrect.r_width/2-rect.r_width/2;
	/*
	 * Constrain to be on screen. Note: need utility rect_constrain.
	 */
	(void)wmgr_constrainrect(&rect, &fs->fs_screenrect, 0, 0);
	if ((pixel_cache = pw_save_pixels(fs->fs_pixwin, &rect)) ==
	    PW_PIXEL_CACHE_NULL) {
		result = 2;
		goto RestoreState;
	}
	rectsaved = rect;
	/*
	 * Display prompt box
	 */
	(void)pw_preparesurface(fs->fs_pixwin, &rect);
	(void)pw_writebackground(fs->fs_pixwin, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_SET);
	rect_marginadjust(&rect, -MENU_BORDER);
	(void)pw_writebackground(fs->fs_pixwin, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_CLR);
	rect_marginadjust(&rect, -MENU_BORDER);
	(void)formatstringtorect(fs->fs_pixwin, prompt->prt_text,
	    prompt->prt_font, &rect);
	/*
	 * if journaling, send sync point.
	 */
	if (sv_journal)
		win_sync(WIN_SYNC_PROMPT, iowindowfd);
	/*
	 * Wait for any mouse button going down or keyboard character struck.
	 * Doesn't detect unencoded keyboard events as loop terminator.
	 * Ignores negative events.
	 */
	for (;;) {
		int	action;

		if (input_readevent(iowindowfd, inputevent)==-1)
			break;
		if (win_inputnegevent(inputevent))
			continue;
		action = event_action(inputevent);
		switch (action) {
		    case MS_LEFT:
		    case MS_MIDDLE:
		    case MS_RIGHT:
			goto Done;
		    default:
			if (action >= ASCII_FIRST &&
			    action <= ASCII_LAST)
				goto Done;
		}
	}
Done:
	/*
	 * Restore previous image
	 */
	pw_restore_pixels(fs->fs_pixwin, pixel_cache);
	/*
	 * Release grip on machine
	 */
RestoreState:
	(void)fullscreen_destroy(fs);
	return(result);
}

