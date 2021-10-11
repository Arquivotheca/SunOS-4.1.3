#ifndef lint
static char sccsid[] = "@(#)bounce.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Overview:	Bouncing ball demo in window.
 * Converted to use SunView by simulating the gfxsubwindow structure. 
 */

/* this replaces all includes */
#include <suntool/sunview.h>
#include <suntool/canvas.h>

/* straight from the Canvas chapter */
static void     repaint_proc();
static void     resize_proc();

/* straight from the Notifier chapter */
static Notify_value my_notice_destroy();
extern Notify_error notify_dispatch();

static int      my_done;

/* define my own gfxsubwindow struct */
struct gfxsubwindow {
	int             gfx_flags;
#define	GFX_RESTART	0x01
#define	GFX_DAMAGED	0x02
	int             gfx_reps;
	struct pixwin  *gfx_pixwin;
	struct rect     gfx_rect;
}               mygfx;
struct gfxsubwindow *gfx = &mygfx;

main(argc, argv)
	int             argc;
	char          **argv;
{
	short           x, y, vx, vy, z, ylastcount, ylast;
	short           Xmax, Ymax, size;
	/* WIN_RECT attribute returns a pointer */
	Rect           *rect;

	/* have to handle this arg that gfxsw_init used to process */
	int             retained;

	/*
	 * replace this call if (gfx == (struct gfxsubwindow *)0) exit(1);
	 * with ... 
	 */

	Frame           frame;
	Canvas          canvas;
	Pixwin         *pw;

	/* this arg was also dealt with by gfxsw_init */
	gfx->gfx_reps = 200000;

	frame = window_create(NULL, FRAME,
			      FRAME_LABEL, "bounce",
			      FRAME_ARGC_PTR_ARGV, &argc, argv,
			      WIN_ERROR_MSG, "Can't create frame",
			      0);
	for (--argc, ++argv; *argv; argv++) {
		/*
		 * handle the arguments that gfxsw_init(0, argv) used to do
		 * for you 
		 */
		if (strcmp(*argv, "-r") == 0)
			retained = 1;
		if (strcmp(*argv, "-n") == 0)
			if (argc > 1) {
				(void) sscanf(*(++argv), "%hD", &gfx->gfx_reps);
				argc++;
			}
	}

	canvas = window_create(frame, CANVAS,
			       CANVAS_RETAINED, retained,
			       CANVAS_RESIZE_PROC, resize_proc,
			       CANVAS_FAST_MONO, TRUE,
			       WIN_ERROR_MSG, "Can't create canvas",
			       0);

	/* only need to define a repaint proc if not retained */
	if (!retained) {
		window_set(canvas,
			   CANVAS_REPAINT_PROC, repaint_proc,
			   0);
	}
	pw = canvas_pixwin(canvas);

	gfx->gfx_pixwin = canvas_pixwin(canvas);

	/* Interpose my proc so I know that the tool is going away. */
	(void) notify_interpose_destroy_func(frame, my_notice_destroy);

	/*
	 * Note: instead of window_main_loop, just show the frame. The
	 * drawing loop is in control, not the notifier. 
	 */
	window_set(frame, WIN_SHOW, TRUE, 0);

Restart:
	rect = (Rect *) window_get(canvas, WIN_RECT);
	Xmax = rect_right(rect);
	Ymax = rect_bottom(rect);
	if (Xmax < Ymax)
		size = Xmax / 29 + 1;
	else
		size = Ymax / 29 + 1;
	/*
	 * the following were always 0 in a gfx subwindow (bouncedemo
	 * is confused on this point
	 */
	x = 0;
	y = 0;

	vx = 4;
	vy = 0;
	ylast = 0;
	ylastcount = 0;
	pw_writebackground(pw, 0, 0, rect->r_width, rect->r_height,
			   PIX_SRC);
	/*
	 * Call notify_dispatch() to dispatch events to the frame
	 * regularly.  This will call my resize and repaint procs and
	 * interposed notify_destroy_func if necessary.  The latter will
	 * set my_done to TRUE if it's time to finish.
	 */
	while (gfx->gfx_reps) {
		(void) notify_dispatch();
		if (my_done)
			break;
		/*
		 * this program is not concerned with damage, because either
		 * the canvas repairs the damage (if retained) or it just
		 * restarts, which is handled by GFX_RESTART 
		 */
		/*
		 * if (gfx->gfx_flags&GFX_DAMAGED) gfxsw_handlesigwinch(gfx); 
		 */
		if (gfx->gfx_flags & GFX_RESTART) {
			gfx->gfx_flags &= ~GFX_RESTART;
			goto Restart;
		}
		if (y == ylast) {
			if (ylastcount++ > 5)
				goto Reset;
		} else {
			ylast = y;
			ylastcount = 0;
		}
		pw_writebackground(pw, x, y, size, size,
				   PIX_NOT(PIX_DST));
		x = x + vx;
		if (x > (Xmax - size)) {
			/*
			 * Bounce off the right edge 
			 */
			x = 2 * (Xmax - size) - x;
			vx = -vx;
		} else if (x < 0) {
			/*
			 * bounce off the left edge 
			 */
			x = -x;
			vx = -vx;
		}
		vy = vy + 1;
		y = y + vy;
		if (y >= (Ymax - size)) {
			/*
			 * bounce off the bottom edge 
			 */
			y = Ymax - size;
			if (vy < size)
				vy = 1 - vy;
			else
				vy = vy / size - vy;
			if (vy == 0)
				goto Reset;
		}
		for (z = 0; z <= 1000; z++);
		continue;
Reset:
		if (--gfx->gfx_reps <= 0)
			break;
		x = 0;
		y = 0;
		vx = 4;
		vy = 0;
		ylast = 0;
		ylastcount = 0;
	}
	exit(0);
	/* NOTREACHED */
}

static void
repaint_proc( /* Ignore args */ )
{
	/* if repainting is required, just restart */
	gfx->gfx_flags |= GFX_RESTART;
}

static void
resize_proc( /* Ignore args */ )
{
	gfx->gfx_flags |= GFX_RESTART;
}

/* this is straight from the Notifier chapter */
static          Notify_value
my_notice_destroy(frame, status)
	Frame           frame;
	Destroy_status  status;
{
	if (status != DESTROY_CHECKING) {
		/* set my flag so that I terminate my loop soon */
		my_done = 1;
		/* Stop the notifier if blocked on read or select */
		(void) notify_stop();
	}
	/* Let frame get destroy event */
	return (notify_next_destroy_func(frame, status));
}
