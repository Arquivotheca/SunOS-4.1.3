#ifndef lint
static	char sccsid[] = "@(#)cframedemo.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * 	Overview:	Frame displayer in windows.  Reads in all the
 *			files of form "frame.xxx" in working directory &
 *			displays them like a movie.
 *			See constants below for limits.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <pixrect/pixrect_hs.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <suntool/gfxsw.h>

#define	MAXFRAMES	1000
#define	FRAMEWIDTH	256
#define	FRAMEHEIGHT	256
#define	USEC_INC	50000
#define	SEC_INC		1

static	struct pixrect *mpr[MAXFRAMES];
static	struct timeval timeout = {SEC_INC,USEC_INC}, timeleft;
static	char s[] = "frame.xxx";
static	struct gfxsubwindow *gfx;
static	int frames, framenum = 1, ximage, yimage;
static	struct rect rect;
#define NCMAP	64
#define MAXSHADE 31
#define SHADEBITS 5
static u_char red[NCMAP], green[NCMAP], blue[NCMAP];


main(argc, argv)
	int argc;
	char **argv;
{
	int	fd, framedemo_selected();
	struct	inputmask im;
	fd_set	inbits, nullbits;

	for (frames = 0; frames < MAXFRAMES; frames++) {
		sprintf(&s[6], "%d", frames);
		fd = open(s, O_RDONLY, 0);
		if (fd == -1) {
			break;
		}
		mpr[frames] = mem_create(FRAMEWIDTH, FRAMEHEIGHT, 1);
		read(fd, mpr_d(mpr[frames])->md_image,
		    FRAMEWIDTH*FRAMEHEIGHT/8);
		close(fd);
		}
	if (frames <= 1) {
	    printf("Couldn't find any 'frame.xx' files in working directory\n");
	    return 1;
	}
	/*
	 * Initialize gfxsw
	 */
	gfx = gfxsw_init(0, argv);
	if (gfx == (struct gfxsubwindow *)0)
		return 2;
	/*
	 * Set up input mask
	 */
	input_imnull(&im);
	im.im_flags |= IM_ASCII;
	gfxsw_setinputmask(gfx, &im, &im, WIN_NULLLINK, 1, 0);
	/*
	 * Main loop
	 */
	framedemo_nextframe(1);
	timeleft = timeout;
	FD_ZERO (&inbits);
	FD_SET (gfx->gfx_windowfd, &inbits);
	FD_ZERO (&nullbits);
	gfxsw_select(gfx, framedemo_selected, inbits, nullbits, nullbits,
	    &timeleft);
	/*
	 * Cleanup
	 */
	gfxsw_done(gfx);
	return 0;
}

framedemo_selected(gfx, ibits, obits, ebits, timer)
	struct	gfxsubwindow *gfx;
	fd_set	*ibits, *obits, *ebits;
	struct	timeval **timer;
{
	if ((*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0)) ||
	    (gfx->gfx_flags & GFX_RESTART)) {
		/*
		 * Our timer expired or restart is true so show next frame
		 */
		if (gfx->gfx_reps)
			framedemo_nextframe(0);
		else
			gfxsw_selectdone(gfx);
	}
	if (FD_ISSET (gfx->gfx_windowfd, ibits)) {
		struct	inputevent event;

		/*
		 * Read input from window
		 */
		if (input_readevent(gfx->gfx_windowfd, &event)) {
			perror("framedemo");
			return;
		}
		switch (event.ie_code) {
		case 'f': /* faster usec timeout */
			if (timeout.tv_usec >= USEC_INC)
				timeout.tv_usec -= USEC_INC;
			else {
				if (timeout.tv_sec >= SEC_INC) {
					timeout.tv_sec -= SEC_INC;
					timeout.tv_usec = 1000000-USEC_INC;
				}
			}
			break;
		case 's': /* slower usec timeout */
			if (timeout.tv_usec < 1000000-USEC_INC)
				timeout.tv_usec += USEC_INC;
			else {
				timeout.tv_usec = 0;
				timeout.tv_sec += 1;
			}
			break;
		case 'F': /* faster sec timeout */
			if (timeout.tv_sec >= SEC_INC)
				timeout.tv_sec -= SEC_INC;
			break;
		case 'S': /* slower sec timeout */
			timeout.tv_sec += SEC_INC;
			break;
		case '?': /* Help */
			printf("'s' slower usec timeout\n'f' faster usec timeout\n'S' slower sec timeout\n'F' faster sec timeout\n");
			/*
			 * Don't reset timeout
			 */
			return;
		default:
			gfxsw_inputinterrupts(gfx, &event);
		}
	}
	FD_ZERO (ibits);
	FD_ZERO (obits);
	FD_ZERO (ebits);
	timeleft = timeout;
	*timer = &timeleft;
}

struct pixrect *spherepr;

framedemo_nextframe(firsttime)
	int	firsttime;
{
	int planes;
	int restarting;

	if (gfx->gfx_flags&GFX_DAMAGED)
		gfxsw_handlesigwinch(gfx);
	restarting = gfx->gfx_flags&GFX_RESTART;
	if (firsttime || restarting) {
		gfx->gfx_flags &= ~GFX_RESTART;
		win_getsize(gfx->gfx_windowfd, &rect);
		ximage = rect.r_width/2-FRAMEWIDTH/2;
		yimage = rect.r_height/2-FRAMEHEIGHT/2;
		load_colors();
		pw_setcmsname(gfx->gfx_pixwin, "spinning_world");
		pw_putcolormap(gfx->gfx_pixwin, 0, NCMAP, red, green, blue);

		/* bkgnd and sphere shading setup */
		planes = -1;
		pw_putattributes(gfx->gfx_pixwin, &planes);
		pw_writebackground(gfx->gfx_pixwin, 0, 0,
		    rect.r_width, rect.r_height, PIX_CLR);

		spherepr = mem_create( FRAMEWIDTH+3, FRAMEHEIGHT+3, 8);
		drawsphere(spherepr,FRAMEWIDTH/2+1,FRAMEHEIGHT/2+1,
			FRAMEWIDTH/2+1);
		pw_write(gfx->gfx_pixwin, ximage-1, yimage-1, FRAMEWIDTH+3,
			FRAMEHEIGHT+3, PIX_SRC, spherepr, 0, 0);
		if (spherepr) pr_destroy( spherepr);

		planes = 32;
		pw_putattributes(gfx->gfx_pixwin, &planes);
	}
	if (framenum >= frames) {
		framenum = 1;
		gfx->gfx_reps--;
	}
	pw_write(gfx->gfx_pixwin, ximage, yimage, FRAMEWIDTH, FRAMEHEIGHT,
	    PIX_SRC, mpr[framenum], 0, 0);
	if (!restarting)
		framenum++;
}
load_colors()
{
	int i;
	red[0] = green[0] = blue[0] = 150;
	red[NCMAP-1] = green[NCMAP-1] = blue[NCMAP-1] = 0;
	for (i=1; i<NCMAP; i++) {
		if (i <= MAXSHADE) {
			red[i] = 0;
			green[i] = (i & MAXSHADE) << (7-SHADEBITS);
			blue[i] =  ((i & MAXSHADE) << (8-SHADEBITS)) +7;
		} else {
			red[i] = (i & MAXSHADE) << (6-SHADEBITS);
			green[i] = ((i & MAXSHADE) << (8-SHADEBITS)) +7;
			blue[i] = 0;
		}
	}
}
#define SDERIV	-2
short circle[128];

#define mem_spot(pr, x, y, color)					\
	 *(mprd8_addr( mpr_d((pr)), (x), (y), 8)) = (color);

jsqrt(n) register unsigned n;
{
    register unsigned q,r,x2,x;
	unsigned t;

	if (n > 0xFFFE0000) return 0xFFFF;  /* algorithm won't cover this case*/
	if (n == 0xFFFE0000) return 0xFFFE; /* or this case */
	if (n < 2) return n;                /* or this case */
	t = x = n;
	while (t>>=2) x>>=1;
	x++;
	for(;;) {
		/* quotrem(n,x,q,r);      /* q = n/x, r = n%x */
		q = n/x;
		r = n%x;
		if (x <= q) {
			x2 = x+2;
			if (q < x2 || q==x2 && r==0) break;
		}
		x = (x + q)>>1;
	}
	return x;
}



drawsphere( dpr, x, y, radius)
	struct pixrect *dpr;
	int x, y, radius;
{
	int stheight;			/* starting height */
	register ifderiv, ofderiv;	/* derivatives of parab */
	register short u,v;		/* coordinates of pts on circle */
	register height;		/* height**2 of current pixel */
	register sheight;		/* actual height of pixel */
	register int color;
	short x1, x2, x3, x4, y1, y2, y3, y4;
	short inten;

	ofderiv = -1;
	stheight = radius * radius;
	makecircle(radius, x, y);
	for (v = 0; v <= radius; v++) {
		height = stheight;
		ifderiv = -1;
		stheight += ofderiv;
		ofderiv += SDERIV;
		x2 = x+v; x4 = x-v; y2 = y+v; y4 = y-v;
		for (u = 0; u <= v; u++) {
			if (v > circle[u])
				break;
			else if (height <= 0)
				sheight = 0;
			else
				sheight = jsqrt(height);

			inten = (sheight * MAXSHADE) / radius;
			color = (inten<1)?1:inten;

			x1 = x+u; x3 = x-u; y1 = y+u; y3 = y-u;
			mem_spot( dpr, x1, y2, color);
			mem_spot( dpr, x2, y1, color);
			if (u != 0) {
				mem_spot( dpr, x3, y2, color);
				mem_spot( dpr, x2, y3, color);
			}
			if (v != 0) {
				mem_spot( dpr, x1, y4, color);
				mem_spot( dpr, x4, y1, color);
			}
			if (u != 0 && v != 0) {
				mem_spot( dpr, x3, y4, color);
				mem_spot( dpr, x4, y3, color);
			}
			height += ifderiv;
			ifderiv += SDERIV;
		}
	}
}
makecircle(rad, x0, y0)
short   rad;
{
	int x, y, d;

	x = 0;
	y = rad;
	d = 3 - 2 * rad;
	while (x < y) {
		circle[x] = y;
		circle[y] = x;
		if (d < 0) {
			d = d + 4 * x + 6;
		} else {
			d = d + 4 * (x - y) + 10;
			y = y - 1;
		}
		x += 1;
	}
	if (x == y)
		circle[x] = y;
	else
		circle[x] = 0;
}
