#ifndef lint
#ifdef sccs
static        char sccsid[] = "@(#)fullscreen.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Implements library routines for the full screen access facility
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/pr_dblbuf.h>
#ifndef PRE_IBIS
#include <pixrect/pr_planegroups.h>
#endif ndef PRE_IBIS
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_screen.h>
#include <suntool/fullscreen.h>
#include <sunwindow/pw_dblbuf.h>
#include <sbusdev/gtreg.h>
/*
 * When not zero will not actually acquire exclusive io lock
 * so that the debugger doesn't get hung.
 */
int	fullscreendebug;

struct	fullscreen *fsglobal;
static	struct fullscreen fsglobalstruct;
/* Note: fdflags and fullscreen_*ground should be in fullscreen struct */
static	int fdflags;
static	u_char	cached_red[256], cached_blue[256], cached_green[256];
static fullscreen_exposed();

extern	Pixrect	cached_mpr;
/* Status of dbl buffering read and write control bits */
static	int 	fs_dblbuf_read_state, fs_dblbuf_write_state;

/* this global variable is defined in pw_dbl.c */
extern struct rect *pw_fullscreen_dbl_rect;

extern struct pixwin *pw_open_fs();

struct	fullscreen *
fullscreen_init(windowfd)
	int	windowfd;
{
	struct  rect    *rt;
	int	scrx, scry;
	int	fullscreen_getclipping();
	struct	fullscreen *fs;
	

	/*
	 * By definition only one fs should be going on in a process at any
	 * one time.  fs is returned for convenience and fsglobal is available
	 * as a global variable.
	 */
	if (fsglobal)
		return(0);
	fs = &fsglobalstruct;

	/*
	 * Grab all input and disable anybody but windowfd from writing
	 * to screen while we are violating window overlapping.
	 * Do this first so that can mess around with the pixwin that we
	 * generate without fear of not being able to lock screen.
	 */
	if (!fullscreendebug)
		(void)win_grabio(windowfd);

	/*
	 * Setup global display data
	 */
	fs->fs_windowfd = windowfd;
	fs->fs_pixwin = pw_open_fs(windowfd);
	if (fs->fs_pixwin == 0) {
		if (!fullscreendebug)
			(void)win_releaseio(fs->fs_windowfd);
		return(0);
	}
	/* If it is a framebuffer with separate accelerator and direct
	 * ports, grab the hardware to prevent futher rendering - at
         * present, gt(hawk) is the only one using this
	*/
	pr_ioctl(fs->fs_pixwin->pw_pixrect, FB_GRABHW, 0);
	/*
	 * Remember cursor
	 */
	fs->fs_cachedcursor.cur_shape = &cached_mpr;
	(void)win_getcursor(windowfd, &fs->fs_cachedcursor);

	/*
	 * get the status of pixrect DBL_READ/WRITE bits 
	 * set read/write control bits to foreground.
	 */

	pw_fullscreen_dbl_rect = RECT_NULL;
	if (fs->fs_pixwin->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {
	        fs_dblbuf_read_state = pw_dbl_get(fs->fs_pixwin,
						  PW_DBL_READ);
		fs_dblbuf_write_state = pw_dbl_get(fs->fs_pixwin,
						   PW_DBL_WRITE);
		pw_dbl_set(fs->fs_pixwin,
			   PW_DBL_READ, PW_DBL_FORE,
			   PW_DBL_WRITE, PW_DBL_FORE,
			   0);
	  /* now get the rect of the dbl bufferer */
	  pw_fullscreen_dbl_rect =
	    (struct rect *)pw_dbl_rect(fs->fs_pixwin);
	}
	fs->fs_pixwin->pw_clipdata->pwcd_flags |= PWCD_IGNORE_DBLSET;

	/*
	 * Remember original input mask
	 */
	(void)win_getinputmask(windowfd, &fs->fs_cachedim,
			       &fs->fs_cachedinputnext); 
	(void)win_get_kbd_mask(windowfd, &fs->fs_cachedkbdim);
	/*
	 * Make use blocking io
	 */
	fdflags = fcntl(windowfd, F_GETFL, 0);
	if (fcntl(windowfd, F_SETFL, fdflags&(~FNDELAY)))
		perror("fullscreen_init (fcntl)");
	/*
	 * Get screen position from kernel
	 */
	(void)win_getscreenposition(windowfd, &scrx, &scry);
	rect_construct(&fs->fs_screenrect, 0-scrx, 0-scry,
	    fs->fs_pixwin->pw_pixrect->pr_width,
	    fs->fs_pixwin->pw_pixrect->pr_height);
	/*
	 * Deal with color (colormap and per planes mask).
	 * Note: Must do before call fullscreen_pw_getclipping so that
	 * change propagates to clipper pixrects.
	 */
	(void)pw_fullcolorinit(fs->fs_pixwin);
	/*
	 * Fixup clipping to be whole screen (since we have it locked)
	 */
	(void)fullscreen_pw_getclipping(fs);

	fsglobal = fs;
	return(fs);
}

/*
 * Assumes that no one will monkey with the loaded colormap from
 * time this called until pw_fullcolorrelease called.
 * Otherwise, things written with pw will change and when
 * restore colormap in pw_fullcolorrelease may restore wrong colors.
 * Currently, this assumption is not valid.
 */
/*ARGSUSED*/
static
pw_fullcolorinit(pw)
	struct	pixwin *pw;
{
#ifdef PRE_IBIS
	int	fullplanes = 255;
#else ndef PRE_IBIS
	int	fullplanes = PIX_ALL_PLANES;
#endif else ndef PRE_IBIS
	struct	screen screen;
	struct	singlecolor tmpcolor;

	/*
	 * Determine default foreground and background colors for screen.
	 */
	(void)win_screenget(pw->pw_clipdata->pwcd_windowfd, &screen);
	/*
	 * Make pixrect able to access entire depth of screen.
	 */
#ifdef  planes_fully_implemented 
	pr_putattributes(pw->pw_pixrect, &fullplanes);
#else
	(void)pw_full_putattributes(pw->pw_pixrect, &fullplanes);
#endif  planes_fully_implemented
	/*
	 * Set foreground and background of entire colormap to defaults
	 * because changing the planes has changed the effective
	 * foreground and background of pw.
	 * Cache the foreground and background of overwritten values first.
	 */
	(void)pr_getcolormap(pw->pw_pixrect, 0, 256, cached_red,
	    cached_green, cached_blue);

	if (screen.scr_flags & SCR_SWITCHBKGRDFRGRD) {
		tmpcolor = screen.scr_background;
		screen.scr_background = screen.scr_foreground;
		screen.scr_foreground = tmpcolor;
	}
	(void)pr_putcolormap(pw->pw_pixrect, 0, 1, &screen.scr_background.red,
	    &screen.scr_background.green, &screen.scr_background.blue);
	(void)pr_putcolormap(pw->pw_pixrect, 255, 1, &screen.scr_foreground.red,
	    &screen.scr_foreground.green, &screen.scr_foreground.blue);
	(void)pr_putcolormap(pw->pw_pixrect, 1, 1, &screen.scr_foreground.red,
	    &screen.scr_foreground.green, &screen.scr_foreground.blue);
	/*
	 * Since don't allow the colormap to be a different sense (inverted)
	 * from the screen, clear the inverted flag.
	 */
	pw->pw_clipdata->pwcd_flags &= ~PWCD_CURSOR_INVERTED;
}

static
pw_fullcolorrelease(pw)
	struct	pixwin *pw;
{
	(void)pr_putcolormap(pw->pw_pixrect, 0, 2, cached_red,
	    cached_green, cached_blue);
	(void)pr_putcolormap(pw->pw_pixrect, 255, 1, &cached_red[255],
	    &cached_green[255], &cached_blue[255]);
}

fullscreen_pw_getclipping(fs)
	struct	fullscreen *fs;
{
	struct	pixwin *pw = fs->fs_pixwin;
	int	clipidsave = pw->pw_clipdata->pwcd_clipid;
	struct	rect screenrect;

	/* Setup routine to call if pw's clipping changes */
	pw->pw_clipops->pwco_getclipping = fullscreen_exposed;
	/*
	 * free existing clipping
	 * Note: This stuff knows too much about clipping internals!
	 */
	pwco_reinitclipping(pw);
	/*
	 * Simulate copying full screen image from kernel
	 */
	(void)rl_initwithrect(
	    &fs->fs_screenrect, &pw->pw_clipdata->pwcd_clipping);
	pw->pw_clipdata->pwcd_clipid = clipidsave;
	/*
	 * Don't free rlto because its not using standard memory mgr.
	 */
	(void)win_getsize(fs->fs_windowfd, &screenrect);
	screenrect.r_left = -fs->fs_screenrect.r_left;
	screenrect.r_top = -fs->fs_screenrect.r_top;
	(void)_pw_setclippers(pw, &screenrect);
}

/*ARGSUSED*/
static
fullscreen_exposed(pw)
	struct	pixwin *pw;
{
	(void)fullscreen_pw_getclipping(fsglobal);
	return;
}

fullscreen_destroy(fs)
	struct	fullscreen *fs;
{
	(void)pw_fullcolorrelease(fs->fs_pixwin);
	/* restore read/write states for double buffering */
	if (fs->fs_pixwin->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {
	    pw_dbl_set(fs->fs_pixwin,
		PW_DBL_READ, fs_dblbuf_read_state,
		PW_DBL_WRITE, fs_dblbuf_write_state,
		0);

	    pw_fullscreen_dbl_rect = RECT_NULL;
	};
	fs->fs_pixwin->pw_clipdata->pwcd_flags &= ~PWCD_IGNORE_DBLSET;
	
	/* If the hardware has been grabbed and the accelerator port 
	 * frozen, unfreeze it 
	*/
	pr_ioctl(fs->fs_pixwin->pw_pixrect, FB_UNGRABHW, 0);
	(void)win_set_kbd_mask(fs->fs_windowfd, &fs->fs_cachedkbdim);
	(void)win_setinputmask(fs->fs_windowfd, &fs->fs_cachedim,
	    (struct inputmask *)0, fs->fs_cachedinputnext);
	(void)win_setcursor(fs->fs_windowfd, &fs->fs_cachedcursor);
	if (fcntl(fs->fs_windowfd, F_SETFL, fdflags))
		perror("fullscreen_init (fcntl)");
	if (!fullscreendebug)
		(void)win_releaseio(fs->fs_windowfd);
	(void)pw_close(fs->fs_pixwin);
	/* free(fs); */
	fsglobal = (struct fullscreen *) 0;
}

