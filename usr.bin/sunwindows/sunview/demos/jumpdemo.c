#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)jumpdemo.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * jumpdemo -- Jump(ToLightSpeed), pick random star field, as accelerate towards
 *	center track stars to edge of window.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/cms_rgb.h>
#include <suntool/gfxsw.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

#define	CONTINUOUSMIN	15
#define	CONTINUOUSMAX	200
#define	CONTINUOUSTRIGGER	200000

#define	REPMAX	500

struct star {
	int	xstart, ystart,		/* Initial position */
		xdelta, ydelta,		/* Increment */
		xcur, ycur;		/* Current postion */
	int	hidden;			/* TRUE when startrack is not visible*/
};

char	*calloc();
char	*sprintf();
char	*strncpy();

/* ARGSUSED */
#ifdef STANDALONE
main(argc, argv)
#else
int jumpdemo_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	struct	rect rect;
	struct	star *stars, *laststar, *s;
	extern	char *calloc();
	int	xmid, ymid;
	struct	gfxsubwindow *gfx = gfxsw_init(0, argv);
	int	continuous = (gfx->gfx_reps == CONTINUOUSTRIGGER);
	int	maxspeed, speed, accelerating, acceleration, firsttime;
	char	**args;
	int	drawvec;
	int	rotatecolormap = 0, cmssize;

	if (gfx == (struct gfxsubwindow *)0)
		EXIT(1);
	if (continuous)
		gfx->gfx_reps = CONTINUOUSMAX;
	for (args = argv;*args;args++) {
		if (strcmp(*args, "-c") == 0)
			rotatecolormap = 1;
	}
	cmssize = setupcolormap(gfx, rotatecolormap);
Restart:
	(void)pw_exposed(gfx->gfx_pixwin);
	speed = 0; accelerating = 1; firsttime = 1;
	drawvec = 1;
	(void)win_getsize(gfx->gfx_windowfd, &rect);
	gfx->gfx_reps = min(gfx->gfx_reps, REPMAX);
	stars = (struct star *) (LINT_CAST(
		calloc(1, (unsigned)(gfx->gfx_reps*sizeof(struct star)))));
	if (stars == 0) {
		(void)printf("Jump couldn't allocate memory\n");
		return;
	}
	/*
	 * Pick star postions.
	 */
	laststar = &stars[gfx->gfx_reps-1];
	xmid = rect.r_width/2;
	ymid = rect.r_height/2;
	if (gfx->gfx_reps < 25)
		acceleration = 6; 
	else if (gfx->gfx_reps < 100)
		acceleration = 12; 
	else
		acceleration = 18; 
	for (s = stars;s <= laststar;++s) {
		int xran, yran, xdif, ydif, distance;
		float fdistance, fxdif, fydif, fxdelta, fydelta;

		xran = r(0, rect.r_width);
		yran = r(0, rect.r_height);
		xdif = xran-xmid;
		ydif = yran-ymid;
		distance = sqroot(xdif*xdif+ydif*ydif);
		fdistance = distance;
		fxdif = xdif;
		fydif = ydif;
		fxdelta = (fxdif/fdistance)*acceleration*0x10000;
		fydelta = (fydif/fdistance)*acceleration*0x10000;
		s->xstart = xran<<16;
		s->ystart = yran<<16;
		s->xdelta = fxdelta;
		s->ydelta = fydelta;
		s->xcur = s->xstart;
		s->ycur = s->ystart;
		s->hidden = 0;
	}
	/*
	 * Jump to light speed until entered hyper space.
	 */
	maxspeed = max(rect.r_width, rect.r_height);
	(void)pw_writebackground(gfx->gfx_pixwin, 0, 0,
	    rect.r_width, rect.r_height, PIX_CLR);
Loop:
	while (accelerating && (speed++ < maxspeed)) {
		(void)pw_lock(gfx->gfx_pixwin, &rect);
		accelerating = accelerate(stars, laststar, gfx->gfx_pixwin,
		    firsttime, drawvec, &rect, cmssize);
		(void)pw_unlock(gfx->gfx_pixwin);
		if (gfx->gfx_flags&GFX_DAMAGED)
			(void)gfxsw_handlesigwinch(gfx);
		if (gfx->gfx_flags&GFX_RESTART) {
			gfx->gfx_flags &= ~GFX_RESTART;
			free((char *)(LINT_CAST(stars)));
			goto Restart;
		}
		if (firsttime) {
			firsttime = 0;
		}
	}
	if (drawvec && rotatecolormap)
		(void)pw_cyclecolormap(gfx->gfx_pixwin, 2*cmssize, 1, cmssize-2);
	if (!drawvec)
		goto Done;
	for (s = stars;s <= laststar;++s) {
		s->xcur = s->xstart;
		s->ycur = s->ystart;
		s->hidden = 0;
	}
	speed = 0;
	accelerating = 1;
	drawvec = 0;
	goto Loop;
Done:
	free((char *)(LINT_CAST(stars)));
	if (continuous) {
		if (--gfx->gfx_reps < CONTINUOUSMIN)
			gfx->gfx_reps = CONTINUOUSMAX;
		goto Restart;
	}
	(void)gfxsw_done(gfx);
	
	EXIT(0);
}

static int
accelerate(stars, laststar, pixwin, firsttime, drawvec, rect, cmssize)
	struct star *stars;
	register struct star *laststar;
	struct	pixwin *pixwin;
	struct	rect *rect;
	int	firsttime;
	int	drawvec, cmssize;
{
	register struct	star *s;
	register int	x0, y0, x1, y1;
	int	moved = 0, veccolor;

	for (s = stars;s <= laststar;++s) {
		if (s->hidden)
			continue;
		moved++;
		x0 = s->xcur>>16;
		y0 = s->ycur>>16;
		if (!firsttime) {
			s->xcur = s->xcur+s->xdelta;
			s->ycur = s->ycur+s->ydelta;
		}
		x1 = s->xcur>>16;
		y1 = s->ycur>>16;
		if (drawvec) {
			veccolor = (s-stars)%(cmssize-1);
			if (veccolor == 0)
				veccolor++;
			(void)pw_vector(pixwin, x0, y0, x1, y1, PIX_SRC, veccolor);
		} else
			(void)pw_vector(pixwin, x0, y0, x1, y1, PIX_CLR, 0);
		if (!rect_includespoint(rect, x1, y1))
			s->hidden = 1;
	}
	return(moved);
}

static int
setupcolormap(gfx, rotate)
	struct	gfxsubwindow *gfx;
	int	rotate;
{
	u_char	red[CMS_RGBSIZE], green[CMS_RGBSIZE],
	    blue[CMS_RGBSIZE]; 
	char	cmsname[CMS_NAMESIZE];

	/*
	 * Initialize to rgb cms.
	 */
	if (rotate)
		(void)sprintf(cmsname, "jumpdemo%10ld", getpid());
	else
		(void)strncpy(cmsname, CMS_RGB, CMS_NAMESIZE);
	(void)pw_setcmsname(gfx->gfx_pixwin, cmsname);
	cms_rgbsetup(red, green, blue);
	(void)pw_putcolormap(gfx->gfx_pixwin, 0, CMS_RGBSIZE, red, green, blue);
	return(CMS_RGBSIZE);
}

