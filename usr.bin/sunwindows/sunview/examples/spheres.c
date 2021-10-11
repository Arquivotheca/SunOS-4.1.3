#ifndef lint
static char sccsid[] = "@(#)spheres.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*
 * spheres -- draw a bunch of shaded spheres Algorithm was done
 * by Tom Duff, Lucasfilm Ltd., 1982
 * Revised to use SunView canvas instead of gfxsw.
 */

#include <suntool/sunview.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#include <sunwindow/cms_rainbow.h>

static Notify_value my_frame_interposer();
static Notify_value my_animation();
static void     sphere();
static void     demoflushbuf();

#define	ITIMER_NULL	((struct itimerval *)0)

/*
 * (NX, NY, NZ) is the light source vector -- length should be
 * 100 
 */
#define NX 48
#define NY -36
#define NZ 80

#define	BUF_BITWIDTH	16

static struct pixrect *mpr;
static int      width;
static int      height;
static int      counter;
static Frame    frame;
static Canvas   canvas;
static int      cmssize;
static Pixwin  *pw;

static short    spheres_image[256] = {
#include "spheres.icon"
};

mpr_static(spheres_pixrect, 64, 64, 1, spheres_image);


main(argc, argv)
    int             argc;
    char          **argv;
{
    char          **args;
    int             usefullgray = 0;
    Icon            icon;

    icon = icon_create(ICON_IMAGE, &spheres_pixrect, 0);
    frame = window_create(NULL, FRAME,
			  FRAME_LABEL,		"spheres",
			  FRAME_ICON,		icon,
			  FRAME_ARGC_PTR_ARGV, 	&argc, argv,
			  0);
    canvas = window_create(frame, CANVAS,
      CANVAS_AUTO_EXPAND,	0,
      CANVAS_AUTO_SHRINK,	0,
      CANVAS_AUTO_CLEAR,	0,
      /*
       * Set SCROLL_LINE_HEIGHT to 1 so that clicking LEFT or RIGHT
       * in the scroll buttons scrolls the canvas by one pixel.
       */
      WIN_VERTICAL_SCROLLBAR,	scrollbar_create(SCROLL_LINE_HEIGHT, 1,
						 0),
      WIN_HORIZONTAL_SCROLLBAR,	scrollbar_create(SCROLL_LINE_HEIGHT, 1,
						 0),
      0);
    for (args = argv; *args; args++) {
	if (strcmp(*args, "-g") == 0)
	    usefullgray = 1;
    }
    /* Interpose in front of the frame's client event handler */
    (void) notify_interpose_event_func(frame, my_frame_interposer,
				       NOTIFY_SAFE);
    (void) notify_set_itimer_func(frame, my_animation,
	      ITIMER_REAL, &NOTIFY_POLLING_ITIMER, ITIMER_NULL);

    width = (int) window_get(canvas, CANVAS_WIDTH);
    height = (int) window_get(canvas, CANVAS_HEIGHT);
    pw = canvas_pixwin(canvas);
    cmssize = (usefullgray) ? setupfullgraycolormap(pw) :
	setuprainbowcolormap(pw);
    mpr = mem_create(BUF_BITWIDTH, height, pw->pw_pixrect->pr_depth);
    window_main_loop(frame);
    exit(0);
	/* NOTREACHED */
}

static int      radius;
static int      x0;		/* x center */
static int      y0;		/* y center */
static int      color;
static int      x;
static int      y;
static int      maxy;
static int      mark;
static int      xbuf;

/* ARGSUSED */
static          Notify_value
my_animation(client, itimer_type)
    Notify_client   client;
    int             itimer_type;
{
    register        i;

    if (x >= radius) {
	radius = r(0, min(width / 2, height / 2));
	x0 = r(0, width);
	y0 = r(0, height);
	color = r(0, cmssize + counter++) % cmssize;
	x = -radius;
	xbuf = 0;

	/*
	 * Don't use background colored sphere. 
	 */
	if (color == 0)
	    color++;
	/*
	 * Don't use tiny sphere. 
	 */
	if (radius < 8)
	    radius = 8;
    }
    for (i = 0; i < 5; i++) {
	xbuf++;
	maxy = sqroot(radius * radius - x * x);
	pw_vector(pw, x0 + x, y0 - maxy, x0 + x, y0 + maxy,
		  PIX_CLR, 0);
	for (y = -maxy; y <= maxy; y++) {
	    mark = r(0, radius * 100) <= NX * x + NY * y
		+ NZ * sqroot(radius * radius - x * x - y * y);
	    if (mark)
		pr_put(mpr, xbuf, y + y0, color);
	}
	if (xbuf == (mpr->pr_width - 1)) {
	    demoflushbuf(mpr, PIX_SRC | PIX_DST,
			 x + x0 - mpr->pr_width, pw);
	    xbuf = 0;
	    x++;
	    return (NOTIFY_DONE);
	}
	x++;
    }
    if (x >= radius)
	demoflushbuf(mpr, PIX_SRC | PIX_DST, x + x0 - (xbuf + 2),
		     pw);
    return (NOTIFY_DONE);
}


static void
demoflushbuf(mpr, op, x, pixwin)
    struct pixrect *mpr;
    int             op;
    int             x;
    struct pixwin  *pixwin;
{
    register u_char *sptr, *end;

    sptr = mprd8_addr(mpr_d(mpr), 0, 0, mpr->pr_depth);
    end = mprd8_addr(mpr_d(mpr), mpr->pr_width - 1,
      		     mpr->pr_height - 1, mpr->pr_depth);
    /*
     * Flush the mpr to the pixwin. 
     */
    pw_write(pixwin, x, 0, mpr->pr_width, mpr->pr_height, op,
	     mpr, 0, 0);
    /*
     * Clear mpr with 0's 
     */
    while (sptr <= end)
	*sptr++ = 0;
    /* Let user interact with tool */
    notify_dispatch();
}


static int
setuprainbowcolormap(pw)
    Pixwin         *pw;
{
    register u_char red[CMS_RAINBOWSIZE];
    register u_char green[CMS_RAINBOWSIZE];
    register u_char blue[CMS_RAINBOWSIZE];

    /*
     * Initialize to rainbow cms. 
     */
    pw_setcmsname(pw, CMS_RAINBOW);
    cms_rainbowsetup(red, green, blue);
    pw_putcolormap(pw, 0, CMS_RAINBOWSIZE, red, green, blue);
    return (CMS_RAINBOWSIZE);
}

static int
setupfullgraycolormap(pw)
    Pixwin         *pw;
{
#define	CMS_FULLGRAYSIZE	256
#define	CMS_FULLGRAY	"fullgray"
    register u_char red[CMS_FULLGRAYSIZE];
    register u_char green[CMS_FULLGRAYSIZE];
    register u_char blue[CMS_FULLGRAYSIZE];
    register        i;

    /*
     * Initialize to rainbow cms. 
     */
    pw_setcmsname(pw, CMS_FULLGRAY);
    for (i = 0; i < CMS_FULLGRAYSIZE; i++) {
	red[i] = green[i] = blue[i] = i;
    }
    pw_putcolormap(pw, 0, CMS_FULLGRAYSIZE, red, green, blue);
    return (CMS_FULLGRAYSIZE);
}


static          Notify_value
my_frame_interposer(frame, event, arg, type)
    Frame           frame;
    Event          *event;
    Notify_arg      arg;
    Notify_event_type type;
{
    int             closed_initial, closed_current;
    Notify_value    value;

    /* Determine initial state of frame */
    closed_initial = (int) window_get(frame, FRAME_CLOSED);
    /* Let frame operate on the event */
    value = notify_next_event_func(frame, event, arg, type);
    /* Determine current state of frame */
    closed_current = (int) window_get(frame, FRAME_CLOSED);
    /* Change animation if states differ */
    if (closed_initial != closed_current) {
	if (closed_current) {
	    /* Turn off animation because closed */
	    (void) notify_set_itimer_func(frame, my_animation,
			 ITIMER_REAL, ITIMER_NULL, ITIMER_NULL);
	} else {
	    /* Turn on animation because opened */
	    (void) notify_set_itimer_func(frame, my_animation,
			    ITIMER_REAL, &NOTIFY_POLLING_ITIMER,
					  ITIMER_NULL);
	}
    }
    return (value);
}
