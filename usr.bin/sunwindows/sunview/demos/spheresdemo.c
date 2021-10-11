#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)spheresdemo.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * spheres -- draw a bunch of shaded spheres
 * Algorithm was done by Tom Duff, Lucasfilm Ltd., 1982
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/cms_rainbow.h>
#include <sunwindow/pixwin.h>
#include <suntool/gfxsw.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

/*
 * (NX, NY, NZ) is the light source vector -- length should be 100
 */
#define NX 48
#define NY -36
#define NZ 80

#define	BUF_BITWIDTH	16

static	struct pixrect *mpr;

/* ARGSUSED */
#ifdef STANDALONE
main(argc, argv)
#else
int spheresdemo_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char	**args;
	struct	rect rect;
	struct	gfxsubwindow *gfx = gfxsw_init(0, argv);
	int	cmssize, usefullgray = 0;

	if (gfx == (struct gfxsubwindow *)0)
		EXIT(1);
	for (args = argv;*args;args++) {
		if (strcmp(*args, "-g") == 0)
			usefullgray = 1;
	}
	cmssize = (usefullgray)? setupfullgraycolormap(gfx):
	    setuprainbowcolormap(gfx);
Restart:
	(void)win_getsize(gfx->gfx_windowfd, &rect);
	(void)pw_writebackground(gfx->gfx_pixwin, 0, 0, rect.r_width, rect.r_height,
	    PIX_SRC);
	mpr = mem_create(BUF_BITWIDTH, rect.r_height,
	    gfx->gfx_pixwin->pw_pixrect->pr_depth);
	/*
	 * Interprete reps as number of spheres.
	 */
	while (gfx->gfx_reps) {
		int	radius = r(0, min(rect.r_width/2, rect.r_height/2));
		int	xcenter = r(0, rect.r_width);
		int	ycenter = r(0, rect.r_height);
		int	color_local = r(0, gfx->gfx_reps)%cmssize;

		/*
		 * Don't use background colored sphere.
		 */
		if (color_local == 0)
			color_local++;
		/*
		 * Don't use tiny sphere.
		 */
		if (radius < 8)
			radius = 8;
		sphere(xcenter, ycenter, radius, gfx->gfx_pixwin, gfx, color_local);
		if (gfx->gfx_flags&GFX_RESTART) {
			gfx->gfx_flags &= ~GFX_RESTART;
			(void)pr_destroy(mpr);
			goto Restart;
		}
		gfx->gfx_reps--;
	}
	(void)pr_destroy(mpr);
	(void)gfxsw_done(gfx);
	
	EXIT(0);
}

static
sphere(x0, y0, rad, pixwin, gfx, color)
	register int x0, y0, rad;
	struct	pixwin *pixwin;
	struct	gfxsubwindow *gfx;
{
	/*register origin(x0, y0);*/
	register x, y, maxy;
	int mark, xbuf = 0;

	for(x = -rad;x<=rad;x++){
		if (gfx->gfx_flags&GFX_DAMAGED)
			(void)gfxsw_handlesigwinch(gfx);
		if (gfx->gfx_flags&GFX_RESTART) {
			return;
		}
		xbuf++;
		maxy=sqroot(rad*rad-x*x);
		(void)pw_vector(pixwin, x0+x, y0-maxy, x0+x, y0+maxy, PIX_CLR, 0);
		for(y = -maxy;y<=maxy;y++) {
			mark = r(0, rad*100)<=NX*x+NY*y
				+NZ*sqroot(rad*rad-x*x-y*y);
			if (mark)
				(void)pr_put(mpr, xbuf, y+y0, color);
		}
		if (xbuf == (mpr->pr_width-1)) {
			demoflushbuf(mpr, PIX_SRC|PIX_DST,
			    x+x0-mpr->pr_width, pixwin);
			xbuf = 0;
		}
	}
	demoflushbuf(mpr, PIX_SRC|PIX_DST, x+x0-(xbuf+2), pixwin);
}

static
demoflushbuf(mpr_local, op, x, pixwin)
	struct	pixrect *mpr_local;
	int	op;
	int	x;
	struct	pixwin *pixwin;
{
	register u_char *sptr = mprd8_addr(mpr_d(mpr_local), 0, 0, mpr_local->pr_depth),
	    *end = mprd8_addr(mpr_d(mpr_local), mpr_local->pr_width-1, 
	    	mpr_local->pr_height-1,	mpr_local->pr_depth);

	/*
	 * Flush the mpr to the pixwin.
	 */
	(void)pw_write(pixwin, x, 0, mpr_local->pr_width, mpr_local->pr_height, op,
	    mpr_local, 0, 0);
	/*
	 * Clear mpr with 0's
	 */
	while (sptr <= end)
		*sptr++ = 0;
}

static int
setuprainbowcolormap(gfx)
	struct	gfxsubwindow *gfx;
{
	u_char	red[CMS_RAINBOWSIZE], green[CMS_RAINBOWSIZE],
	    blue[CMS_RAINBOWSIZE]; 

	/*
	 * Initialize to rainbow cms.
	 */
	(void)pw_setcmsname(gfx->gfx_pixwin, CMS_RAINBOW);
	cms_rainbowsetup(red, green, blue);
	(void)pw_putcolormap(gfx->gfx_pixwin, 0, CMS_RAINBOWSIZE, red, green, blue);
	return(CMS_RAINBOWSIZE);
}

static int
setupfullgraycolormap(gfx)
	struct	gfxsubwindow *gfx;
{
#define	CMS_FULLGRAYSIZE	256
#define	CMS_FULLGRAY	"fullgray"
	u_char	red[CMS_FULLGRAYSIZE], green[CMS_FULLGRAYSIZE],
	    blue[CMS_FULLGRAYSIZE]; 
	register i;

	/*
	 * Initialize to full greyscale cms.
	 */
	(void)pw_setcmsname(gfx->gfx_pixwin, CMS_FULLGRAY);
	for (i = 0;i < CMS_FULLGRAYSIZE;i++) {
		red[i] = green[i] = blue[i] = i;
	}
	(void)pw_putcolormap(gfx->gfx_pixwin, 0, CMS_FULLGRAYSIZE, red, green, blue);
	return(CMS_FULLGRAYSIZE);
}

