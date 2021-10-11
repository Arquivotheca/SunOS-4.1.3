#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)bouncedemo.c 1.1 92/07/30";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * 	Overview:	Bouncing ball demo in window
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/gfxsw.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

void	pw_use_fast_monochrome();

/* ARGSUSED */
#ifdef STANDALONE
main(argc, argv)
#else
int bouncedemo_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	short	x, y, vx, vy, z, ylastcount, ylast;
	short	Xmax, Ymax, size;
	struct	rect rect;
	struct	gfxsubwindow *gfx = gfxsw_init(0, argv);

	if (gfx == (struct gfxsubwindow *)0)
		EXIT(1);
	pw_use_fast_monochrome(gfx->gfx_pixwin);
Restart:
	(void)win_getsize(gfx->gfx_windowfd, &rect);
	Xmax = rect_right(&rect);
	Ymax = rect_bottom(&rect);
	if (Xmax < Ymax)
		size = Xmax/29+1;
	else
		size = Ymax/29+1;
	x=rect.r_left;
	y=rect.r_top;
	vx=4;
	vy=0;
	ylast=0;
	ylastcount=0;
	(void)pw_writebackground(gfx->gfx_pixwin, 0, 0, rect.r_width, rect.r_height,
	    PIX_SRC);
	while (gfx->gfx_reps) {
		if (gfx->gfx_flags&GFX_DAMAGED)
			(void)gfxsw_handlesigwinch(gfx);
		if (gfx->gfx_flags&GFX_RESTART) {
			gfx->gfx_flags &= ~GFX_RESTART;
			goto Restart;
		}
		if (y==ylast) {
			if (ylastcount++ > 5)
				goto Reset;
		} else {
			ylast = y;
			ylastcount = 0;
		}
		(void)pw_writebackground(gfx->gfx_pixwin, x, y, size, size,
		    PIX_NOT(PIX_DST));
		x=x+vx;
		if (x>(Xmax-size)) {
			/*
			 * Bounce off the right edge
			 */
			x=2*(Xmax-size)-x;
			vx= -vx;
		} else if (x<rect.r_left) {
			/*
			 * bounce off the left edge
			 */
			x= -x;
			vx= -vx;
		}
		vy=vy+1;
		y=y+vy;
		if (y>=(Ymax-size)) {
			/*
			 * bounce off the bottom edge
			 */
			y=Ymax-size;
			if (vy<size)
				vy=1-vy;
			else
				vy=vy / size - vy;
			if (vy==0)
				goto Reset;
		}
		for (z=0; z<=1000; z++);
		continue;
Reset:
		if (--gfx->gfx_reps <= 0)
			break;
		x=rect.r_left;
		y=rect.r_top;
		vx=4;
		vy=0;
		ylast=0;
		ylastcount=0;
	}
	(void)gfxsw_done(gfx);
	
	EXIT(0);
}

