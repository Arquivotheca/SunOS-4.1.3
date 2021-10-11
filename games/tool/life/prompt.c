#ifndef lint
static  char sccsid[] = "@(#)prompt.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*** 
 ***   My new prompt package.
 ***   This should be stuck in a library somewhere
 ***
 ***   fullscreen_prompt_for_input(str1, str2, fd)
 ***		char	*str1, *str2;
 ***		int	fd;
 ***
 ***   changes cursor, goes fullscreen, and then puts
 ***   up a box with str1 on the top and prompts the user for a string
 ***   which it puts into str2.   Returns 0 if successful.
 ***
 ***   fullscreen_prompt(str1, str2, str3, event, fd)
 ***		char	*str1, *str2, *str3;
 ***		Event	*event; 
 ***		int	fd;
 ***
 ***   changes cursor, goes fullscreen, and then puts
 ***   up a box with str1 on the top, str2 under a mouse with
 ***   the left button depressed, and str3 under a mouse with
 ***   the right button depressed.  If str3 is null, then
 ***   the right hand mouse doesn't appear.  Any of the strings
 ***   can contain newlines.  Puts the event that caused
 ***   exit from fullscreen into event.  Returns 0 if successful.
 ***/

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/fullscreen.h>
#include <suntool/menu.h>
#include <ctype.h>

/* 
 * pixrects
 */
static short    stopped_image[] = {
#include "mouse.pr"
};
DEFINE_CURSOR_FROM_IMAGE(stopped, 7, 7, PIX_SRC | PIX_DST, stopped_image);
DEFINE_CURSOR(oldcursor, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

#define CTRL(x) (~0100 & (x))

static short    gray50_data[] = {
#include <images/square_50.pr>
};
mpr_static(gray50_patch, 16, 16, 1, gray50_data);

static short    cursor_data[] = {
#include "cursor.pr"
};
mpr_static(cursor_patch, 16, 16, 1, cursor_data);

static short mousel_data[]={
#include "ml.icon"
};
mpr_static(mousel_mpr, 64, 64, 1, mousel_data);

static short mouser_data1[]={
#include "mr.icon"
};
mpr_static(mouser_mpr, 64, 64, 1, mouser_data1);

#define MOUSEWIDTH	24	/* actual width of mouse in icon */
#define MOUSEHEIGHT	36	/* actual height of mouse in icon */
#define BORDER		10	/* white space surounding text, mouse icon */
#define SHADOW		15	/* thickness of shadow */

static	int	lines1, lines2, lines3;
static	int	rightoff;
static	struct	pr_size	fontsize;
static	struct	pixfont *defpf;
static	int	len;
static	int	wd1, wd2, wd3;

static		Pw_pixel_cache	*drawbox();

static	debug;

fullscreen_prompt_for_input(str1, str2, fd)
	char	*str1, *str2;
	int	fd;
{
	Event	ie;
	struct	rect rectsaved, rect;
	struct	pixwin *pw;
	extern	struct pr_size pf_textwidth();
	struct	fullscreen *fs;
	Pw_pixel_cache	*bitmap_under;
	char	*str, *strtmp;
	int	result = 0;
	int	adv;
	int	x, x1, y, y1;

	len = strlen(str1);
	strtmp = str = str2;
	debug = 0;
	fs = fullscreen_init(fd);
	if (fs == 0)
		return(1);		/* Probably out of fd's. */

	pw = fs->fs_pixwin;
	/* get rect for prompt box, not including shadow */
	makerect_input(fs, &rect);
	if ((bitmap_under = drawbox(pw, &rect, fd, &rectsaved, fs)) == NULL)
		goto RestoreState;

	/* fill in the prompt box */
	fillbox_input(pw, rect, str1);
	
	/* Stay in fullscreen until left or right button is pressed */
	/* XXX what about middle button? */

	adv = defpf->pf_char['A'].pc_adv.x;
	x1 = rect.r_left + 10;
	x = rect.r_left + 10;
	y = rect.r_top + 6*fontsize.y/2 - 3;
	y1 = rect.r_top + 6*fontsize.y/2 + 1;
	pw_write(pw, x, y1, 8, 4,
	    PIX_SRC^PIX_DST, &cursor_patch, 0, 0);

	for (;;) {
		if (input_readevent(fd, &ie) == -1)
			break;
		if (win_inputnegevent(&ie))
			continue;
		if (isascii(ie.ie_code)) {
			if (isprint(ie.ie_code)) {
				if (str2 - str >= len) {
					/*
					 * XXX pw_copy doesn't work for
					 * some reason
					 */
					/*pw_copy(pw, x1, y-fontsize.y,
					    (len+1)*fontsize.x, fontsize.y+2,
					    PIX_SRC, pw, x1+adv,
					    y-fontsize.y); */
					strtmp++;
					pw_text(pw, x1, y, PIX_SRC, defpf,
					    strtmp);
					pw_char(pw, x-adv, y, PIX_SRC^PIX_DST,
					    defpf, *(str2-1));
				} else {
					pw_write(pw, x, y1, 8, 4,
					    PIX_SRC^PIX_DST, &cursor_patch,
					    0, 0);
					x += adv;
					pw_write(pw, x, y1, 8, 4,
					    PIX_SRC^PIX_DST, &cursor_patch,
					    0, 0);
				}
				pw_char(pw, x-adv, y, PIX_SRC^PIX_DST,
				    defpf, ie.ie_code);
				*str2++ = ie.ie_code;
				*str2 = 0;
				continue;
			}
			else switch(ie.ie_code) {
			case '\n':
				goto Done;
				break;
			case '\r':
				goto Done;
				break;
			case '\f':
				goto Done;
				break;
			case CTRL('u'):
			case CTRL('U'):
				pw_write(pw, x, y1, 8, 4,
				    PIX_SRC^PIX_DST, &cursor_patch, 0, 0);
				while (--str2 >= strtmp) {
					x -= adv;
					pw_char(pw, x, y,
					    PIX_SRC ^ PIX_DST, defpf, *str2);
					*str2 = 0;
				}
				if (str2 >= str) {
					while (--str2 >= str)
						*str2 = 0;
				}
				pw_write(pw, x, y1, 8, 4,
				    PIX_SRC^PIX_DST, &cursor_patch, 0, 0);
				strtmp = str;
				str2++;
				break;
			case CTRL('h'):
			case CTRL('H'):
			case 0177:
				if (str2 <= str)
					continue;
				if (str2 - str > len) {
					strtmp--;
					*--str2 = 0;
					pw_text(pw, x1, y, PIX_SRC, defpf,
					    strtmp);
				} else {
					x -= adv;
					str2--;
					pw_write(pw, x, y1, 8, 4,
					    PIX_SRC^PIX_DST, &cursor_patch,
					    0, 0);
					pw_write(pw, x+adv, y1, 8, 4,
					    PIX_SRC^PIX_DST, &cursor_patch,
					    0, 0);
					pw_char(pw, x, y,
					    PIX_SRC ^ PIX_DST, defpf, *str2);
					*str2 = 0;
				}
				break;
			}
		}
		switch (ie.ie_code) {
		case MS_LEFT:
		case MS_RIGHT:
			goto Done;
		}
	}
Done:
	*str2 = 0;
	pw_restore_pixels(pw, bitmap_under);
	win_setcursor(fd, &oldcursor);
RestoreState:
	fullscreen_destroy(fs);

	if (debug)
		printf("\ngot this char %c\n", debug);

	return(result);
}

/* 
 * compute size of prompt box, and put it into rectp
 */
static
makerect_input(fs, rectp)
	struct	fullscreen	*fs;
	struct	rect		*rectp;
{	
	struct	rect	rect;

	if (defpf == NULL) {
		defpf = pw_pfsysopen();
		fontsize = pf_textwidth(1, defpf, " ");
	}
	if (len < 20)
		len = 20;
	rect.r_width = (len+2)*fontsize.x + 8*MENU_BORDER;
	rect.r_height = 3*fontsize.y + fontsize.y/2 + 8*MENU_BORDER;
	rect.r_top = fs->fs_screenrect.r_top+
	    fs->fs_screenrect.r_height/2-rect.r_height/2;
	rect.r_left = fs->fs_screenrect.r_left+
	    fs->fs_screenrect.r_width/2-rect.r_width/2;
	*rectp = rect;
}

/* 
 * Fill in the prompt box
 */
static
fillbox_input(pw, rect, str1)
	Pixwin	*pw;
	struct	rect rect;
	char	*str1;
{
	int	l,t;
	
	l = rect.r_left;
	t = rect.r_top;

	pw_text(pw,l + 10,t + 3*fontsize.y/2 -3,  PIX_SRC, defpf, str1);
}

fullscreen_prompt(str1, str2, str3, ie, fd)
	char	*str1, *str2, *str3;
	int	fd;
	struct	inputevent *ie;
{
	struct	rect rectsaved, rect;
	Pw_pixel_cache	*bitmap_under;
	extern	struct pr_size pf_textwidth();
	struct	fullscreen *fs;
	int	result = 0;

	fs = fullscreen_init(fd);
	if (fs == 0)
		return(1);		/* Probably out of fd's. */

	/* get rect for prompt box, not including shadow */
	makerect(fs, &rect, str1, str2, str3);
	if ((bitmap_under = drawbox(fs->fs_pixwin, &rect, fd,
	    &rectsaved, fs)) == NULL)
		goto RestoreState;

	/* fill in the prompt box */
	fillbox(fs->fs_pixwin, rect, str1, str2, str3);
	
	/* Stay in fullscreen until left or right button is pressed */
	/* XXX what about middle button? */
	for (;;) {
		if (input_readevent(fd, ie)==-1)
			break;
		if (win_inputnegevent(ie))
			continue;
		switch (ie->ie_code) {
		case '\n':
		case '\r':
		case '\f':
		case MS_RIGHT:
		case MS_LEFT:
		case MS_MIDDLE:
			goto Done;
		}
	}
Done:
	pw_restore_pixels(fs->fs_pixwin, bitmap_under);
	win_setcursor(fd, &oldcursor);
RestoreState:
	fullscreen_destroy(fs);
	return(result);
}

/* 
 * compute size of prompt box, and put it into rectp
 */
static
makerect(fs, rectp, str1, str2, str3)
	struct	fullscreen	*fs;
	struct	rect		*rectp;
	char			*str1, *str2, *str3;
{	
	struct	rect	rect;
	int		wd;
	register char	*p;

	if (defpf == NULL) {
		defpf = pw_pfsysopen();
		fontsize = pf_textwidth(1, defpf, " ");
	}
	wd1 = wd2 = wd3 = 0;
	lines1 = lines2 = lines3 = 1;
	wd = 0;
	for (p = str1; *p; p++) {
		wd++;
		if (*p == '\n') {
			wd1 = max(wd1, wd-1);
			wd = 0;
			lines1++;
		}
	}
	wd1 = max(wd1, wd);

	wd = 0;
	for (p = str2; *p; p++) {
		wd++;
		if (*p == '\n') {
			wd2 = max(wd2, wd-1);
			wd = 0;
			lines2++;
		}
	}
	wd2 = max(wd2, wd);

	wd = 0;
	for (p = str3; *p; p++) {
		wd++;
		if (*p == '\n') {
			wd3 = max(wd3, wd-1);
			wd = 0;
			lines3++;
		}
	}
	wd3 = max(wd3, wd);

	wd1 = wd1*fontsize.x + 2*BORDER;
	wd2 = max(MOUSEWIDTH, wd2*fontsize.x) + 2*BORDER;
	if (str3[0])
		wd3 = max(MOUSEWIDTH, wd3*fontsize.x) + 2*BORDER;
	else
		wd3 = 0;
	rightoff = wd2 + BORDER;
	wd = max(wd2 + wd3, wd1);
	
	rect.r_width = wd + 8*MENU_BORDER;
	rect.r_height = MOUSEHEIGHT
	    + (2 + lines1 + max(lines2, lines3))*fontsize.y + 8*MENU_BORDER;
	if (str1[0] == 0)
		rect.r_height -= ((1 + 2*lines1)*fontsize.y)/2;
	rect.r_top = fs->fs_screenrect.r_top+
	    fs->fs_screenrect.r_height/2-rect.r_height/2;
	rect.r_left = fs->fs_screenrect.r_left+
	    fs->fs_screenrect.r_width/2-rect.r_width/2;
	*rectp = rect;
}

/* 
 * Fill in the prompt box
 */
static
fillbox(pw, rect, str1, str2, str3)
	Pixwin	*pw;
	struct	rect rect;
	char	*str1, *str2, *str3;
{
	int	l,t, off;
	
	l = rect.r_left;
	t = rect.r_top;
	if (str1[0]) {
		off = (rect.r_width - wd1 + 1)/2 + BORDER;
		/* -3 is fudge factor to make things look nicer */
		pw_text_newline(pw, l + off, t + 3*fontsize.y/2 - 3,
		    PIX_SRC, defpf, str1);
	}
	else
		t -= 3*fontsize.y/2;

	if (str3[0]) {
		off = (rightoff - MOUSEWIDTH)/2;
		pw_write(pw, l + off, t + (lines1+1)*fontsize.y,
		    MOUSEWIDTH, MOUSEHEIGHT, PIX_SRC,  &mousel_mpr, 0, 0);
		off = (rightoff - wd2)/2 + BORDER;
		pw_text_newline(pw, l + off,
		    t + ((5 + 2*lines1)*fontsize.y)/2 + MOUSEHEIGHT,
		    PIX_SRC, defpf,  str2);

		off = (rect.r_width - rightoff - MOUSEWIDTH)/2;
		pw_write(pw, l + rightoff + off,
		    t + (lines1+1)*fontsize.y, MOUSEWIDTH, MOUSEHEIGHT,
		    PIX_SRC, &mouser_mpr, 0, 0);
		off = (rect.r_width - rightoff - wd3)/2 + BORDER;
		pw_text_newline(pw, l + rightoff + off,
		    t + ((5 + 2*lines1)*fontsize.y)/2 + MOUSEHEIGHT,
		    PIX_SRC, defpf, str3);
	} else {
		off = (rect.r_width - MOUSEWIDTH)/2;
		pw_write(pw, l + off, t + (lines1+1)*fontsize.y,
		    MOUSEWIDTH, MOUSEHEIGHT, PIX_SRC,  &mousel_mpr, 0, 0);
		off = (rect.r_width - wd2)/2 + BORDER;
		pw_text_newline(pw, l + off,
		    t + ((5 + 2*lines1)*fontsize.y)/2 + MOUSEHEIGHT,
		    PIX_SRC, defpf,  str2);
	}
}

/* 
 * Like pw_text, except that newlines are interpreted
 */
static
pw_text_newline(pw, xbasew, ybasew, op, pixfont, s)
	struct	pixwin	*pw;
	int		xbasew, ybasew;
	int		op;
	struct	pixfont	*pixfont;
	char		*s;
	
{
	register char	*p;

	for (p = s; *p; p++) {
		if (*p == '\n') {
			*p = 0;
			pw_text(pw,xbasew,ybasew,op,pixfont,s);
			*p = '\n';
			ybasew+=fontsize.y;
			s = p+1;
		}
	}
	pw_text(pw,xbasew,ybasew,op,pixfont,s);
}

static Pw_pixel_cache *
drawbox(pw, rectp, fd, rectsavep, fs)
	struct pixwin	*pw;
	struct	rect	*rectp;
	struct	rect	*rectsavep;
	struct	fullscreen *fs;
{
	Pw_pixel_cache	*bitmap_under;
	Pw_pixel_cache	*pw_save_pixels();
	struct	pixrect *pr;
	struct rect	rect;
	int	x,y;		/* XXX workaround */
	
	rect = *rectp;
	rect.r_width += SHADOW;
	rect.r_height += SHADOW;
	if ((bitmap_under = pw_save_pixels(pw, &rect)) == 0)
		return(0);
	*rectsavep = rect;
	rect.r_width -= SHADOW;
	rect.r_height -= SHADOW;

	/* change cursor */
 	win_getcursor(fd, &oldcursor);
	win_setcursor(fd, &stopped);

	/* draw shadow of prompt box */
	pw_preparesurface(pw, &rect);
	rect.r_left += SHADOW;
	rect.r_top += SHADOW;
	pw_preparesurface(pw, &rect);
	rect.r_left -= SHADOW;
	rect.r_top -= SHADOW;

	/* restore stuff bitmap_under shadow, since shadow will be OR'ed in */

	pr = pw_primary_cached_mpr(fs->fs_pixwin, bitmap_under);
	pw_write(pw, rect.r_left+rect.r_width, rect.r_top + SHADOW,
	    SHADOW, rect.r_height, PIX_SRC, pr,
	    rect.r_width, SHADOW);
	pw_write(pw, rect.r_left+SHADOW, rect.r_top + rect.r_height,
	    rect.r_width,  SHADOW, PIX_SRC, pr,
	    SHADOW, rect.r_height);

	/* 
	 * XXX Workaround for pw_replrop bug.  Get pixwin by hand instead
	 */
	/* pw_replrop(pw, rect.r_left + SHADOW, rect.r_top + SHADOW,
	    rect.r_width, rect.r_height, PIX_SRC|PIX_DST, &gray50_patch, 0, 0); */

	x = pw->pw_clipdata->pwcd_clipping.rl_bound.r_left;
	y = pw->pw_clipdata->pwcd_clipping.rl_bound.r_top;
	pr_replrop(pw->pw_clipdata->pwcd_prsingle,
	    rect.r_left + rect.r_width - x, rect.r_top + SHADOW - y,
	    SHADOW, rect.r_height, PIX_SRC|PIX_DST, &gray50_patch,
	    rect.r_left + rect.r_width - x, rect.r_top + SHADOW - y);
	pr_replrop(pw->pw_clipdata->pwcd_prsingle,
	    rect.r_left + SHADOW - x, rect.r_top + rect.r_height - y,
	    rect.r_width - SHADOW, SHADOW, PIX_SRC|PIX_DST, &gray50_patch,
	    rect.r_left + SHADOW - x, rect.r_top + rect.r_height - y);

	/* draw border of prompt box */
	pw_writebackground(pw, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_SET);
	rect_marginadjust(&rect, -MENU_BORDER);
	pw_writebackground(pw, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_CLR);
	rect_marginadjust(&rect, -MENU_BORDER);

	pw_writebackground(pw, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_SET);
	rect_marginadjust(&rect, -MENU_BORDER);
	pw_writebackground(pw, rect.r_left, rect.r_top,
	    rect.r_width, rect.r_height, PIX_CLR);
	rect_marginadjust(&rect, -MENU_BORDER);
	*rectp = rect;
	return(bitmap_under);
}
