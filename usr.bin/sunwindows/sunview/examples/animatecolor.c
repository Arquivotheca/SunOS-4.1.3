
/***************************************************************/
#ifndef lint
static char sccsid[] = "@(#)animatecolor.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/***************************************************************/

#include <suntool/sunview.h>
#include <suntool/canvas.h>


/***************************************************************/
/* You set MYCOLORS & MYNBITS according to how many colors     */
/* you are using; rest is just boilerplate, more or less;      */
/* it you define your colors.			               */
/***************************************************************/
/*
 * define the colors I want in the canvas; max 16, must be a
 * power of 2 
 */
#define MYCOLORS	4
/*
 * define the number of bits my colors take up -- MYCOLORS log 2;
 * maximum for animation to be possible is half screen's bits per
 * pixel -- 4 bits on current Sun color displays. 
 */
#define MYNBITS		2
/*
 * to "hide" one set of planes while displaying another takes a
 * large cms -- the square of the number of colors 
 */
#define MYCMS_SIZE	(MYCOLORS * MYCOLORS)

/*
 * when you write out a color pixel, you must write the color in
 * the appropriate planes.  This macro writes it in both sets 
 */
#define usecolor(i)	( (i) | ((i) << colorstuff.colorbits) )

struct colorstuff {
    /* desired colors */
    unsigned char   redcolors[MYCOLORS];
    unsigned char   greencolors[MYCOLORS];
    unsigned char   bluecolors[MYCOLORS];
    /* number of bits the desired colors take up */
    int             colorbits;
    /* colormap segment size */
    int             cms_size;
    /* 2 colormaps to support it */
    unsigned char   red[2][MYCMS_SIZE];
    unsigned char   green[2][MYCMS_SIZE];
    unsigned char   blue[2][MYCMS_SIZE];
    /* 2 masks to support it */
    int             enable_0_mask;
    int             enable_1_mask;
    /* current colormap -- 0 or 1 */
    int             cur_buff;
    /* plane mask to control which planes are written to */
    int             plane_mask;
};

struct colorstuff colorstuff = {
/* desired red colors */
				{0, 0, 255, 255},
/* desired green colors */
				{0, 255, 0, 192},
/* desired blue colors */
				{255, 0, 0, 192},
/* number of planes these colors take up */
				MYNBITS,
/* colormap segment size */
				MYCMS_SIZE,
/* rest filled in later */
};

static void     resize_proc();

/* stuff needed to do random numbers */
extern void     srandom();
extern int      getpid();
extern long     random();
extern char    *sprintf();

static Notify_value my_frame_interposer();
static Notify_value my_draw();

static Pixwin  *pw;
static int      times_drawn;
static int      Xmax, Ymax;

main(argc, argv)
    int             argc;
    char          **argv;
{
    Frame           base_frame;
    Canvas          canvas;

    base_frame = window_create(NULL, FRAME,
			       FRAME_LABEL, "animatecolor",
			       FRAME_ARGS, argc, argv,
			       0);
    canvas = window_create(base_frame, CANVAS,
			   CANVAS_RETAINED, TRUE,
			   CANVAS_RESIZE_PROC, resize_proc,
			   0);
    pw = (Pixwin *) canvas_pixwin(canvas);

    /* set up the canvas' colormap */
    doublebuff_init(&colorstuff);

    /* run the drawing routine as often as possible */
    (void) notify_set_itimer_func(base_frame, my_draw,
				  ITIMER_REAL,
				  &NOTIFY_POLLING_ITIMER,
				  ((struct itimerval *) 0));

    /* initialize the random function */
    srandom(getpid());
    window_main_loop(base_frame);
    exit(0);
	/* NOTREACHED */
}

/* ARGSUSED */
static          Notify_value
my_draw(client, itimer_type)
    Notify_client   client;
    int             itimer_type;
{
    /*
     * draw the squares, then swap the colormap to animate them 
     */
#define SQUARESIZE	50
#define MAX_VEL		(SQUARESIZE / 5)
    /* number of squares to animate */
#define NUMBER	(MYCOLORS - 1)

    static int      posx[NUMBER], posy[NUMBER];
    static int      vx[NUMBER], vy[NUMBER];
    static int      prevposx[NUMBER], prevposy[NUMBER];
    int             i;

    /* set the plane mask to be that which we are not viewing */
    pw_putattributes(pw, (colorstuff.cur_buff == 1) ?
      &(colorstuff.enable_1_mask): &(colorstuff.enable_0_mask));

    /* write to invisible planes */
    for (i = 0; i < NUMBER; i++) {
	if (!times_drawn) {
	    /* first time drawing */

	    posx[i] = (i + 1) * 100;
	    posy[i] = 50;
	    vx[i] = r(-MAX_VEL, MAX_VEL);
	    vy[i] = r(-MAX_VEL, MAX_VEL);
	}
	if (abs(vx[i]) > MAX_VEL) {
	    printf("Weird value (%d) for vx[%d]\n", vx[i], i);
	    vx[i] = r(-MAX_VEL, MAX_VEL);
	}
	posx[i] = posx[i] + vx[i];
	if (posx[i] < 0) {
	    /* Bounce off the left wall */
	    posx[i] = 0;
	    vx[i] = -vx[i];
	} else if (posx[i] > Xmax - SQUARESIZE) {
	    /* Bounce off the right wall */
	    vx[i] = -vx[i];
	    posx[i] = posx[i] + vx[i];
	}
	posy[i] = posy[i] + vy[i];
	if (posy[i] > Ymax - SQUARESIZE) {
	    /* Bounce off the top */
	    posy[i] = Ymax - SQUARESIZE;
	    vy[i] = -vy[i];
	} else if (posy[i] < 0) {
	    /* Bounce off the bottom */
	    posy[i] = 0;
	    vy[i] = -vy[i];
	}
	/* draw the square you can't see */
	pw_rop(pw, posx[i], posy[i], SQUARESIZE, SQUARESIZE,
	       PIX_SRC | PIX_COLOR(usecolor(i + 1)), NULL, 0, 0);
    }
    /*
     * swap the colormaps, and hey presto! should appear smoothly 
     */
    doublebuff_swap(&colorstuff);
    times_drawn++;

    /* set the plane mask to be that which we are not viewing */
    pw_putattributes(pw, (colorstuff.cur_buff == 1) ?
      &(colorstuff.enable_1_mask): &(colorstuff.enable_0_mask));

    /* erase now invisible planes */
    for (i = 0; i < NUMBER; i++) {

	if (times_drawn > 1) {
	    /* squares have been drawn before */
	    /* erase in the one you can't see */
	    pw_rop(pw, prevposx[i], prevposy[i],
		   SQUARESIZE, SQUARESIZE, PIX_CLR, NULL, 0, 0);
	}
	/* remember so can erase later */
	prevposx[i] = posx[i];
	prevposy[i] = posy[i];
    }

    /*
     * set the plane mask to be that which we are viewing, in
     * case screen has to be repaired between now an when we are
     * called again. 
     */
    pw_putattributes(pw, (colorstuff.cur_buff == 1) ?
      &(colorstuff.enable_0_mask): &(colorstuff.enable_1_mask));

}


/* random number calculator */
int
r(minr, maxr)
    int             minr, maxr;
{
    int             i;

    i = random() % (maxr - minr + 1);
    if (i < 0)
	return (i + maxr + 1);
    else
	return (i + minr);
}


/* ARGSUNUSED */
static void
resize_proc(canvas, width, height)
{
    times_drawn = 0;
    /* remember, pixels start at 0, not 1, in the pixwin */
    Xmax = width - 1;
    Ymax = height - 1;
}

/*
 * Do double buffering by changing the write enable planes and
 * the color maps. The application draws into a buffer which is
 * not visible and when the buffers are swapped the invisible one
 * become visible and the other become invis. 
 *
 * Start out drawing into buffer 1 which is the low-order buffer;
 * ie. the low-order planes. Things would not work if this is not
 * done because the devices start out be drawing with color 1
 * which will only hit the low-order planes. 
 *
 * Init double buffering: Allocate color maps for both buffers. Fill
 * in color maps. 
 */


doublebuff_init(colorstuff)
    struct colorstuff *colorstuff;
{
    /*
     * user has defined desired colors.  Set them up in the two
     * colormap segments 
     */
    int             index_1;
    int             index_2;
    int             i;
    char            cmsname[CMS_NAMESIZE];

    /* name colormap something unique */
    sprintf(cmsname, "animatecolor%D", getpid());
    pw_setcmsname(pw, cmsname);

    /*
     * for each index in each color table, figure out how it maps
     * into the original color table. 
     */
    for (i = 0; i < colorstuff->cms_size; i++) {
	/*
	 * first colormap will show color X whenever low order
	 * bits of color index are X 
	 */
	index_1 = i & ((1 << colorstuff->colorbits) - 1);
	/*
	 * second colormap will show color X whenever high order
	 * bits of color index are X 
	 */
	index_2 = i >> colorstuff->colorbits;

	colorstuff->red[0][i] = colorstuff->redcolors[index_1];
	colorstuff->green[0][i] = colorstuff->greencolors[index_1];
	colorstuff->blue[0][i] = colorstuff->bluecolors[index_1];

	colorstuff->red[1][i] = colorstuff->redcolors[index_2];
	colorstuff->green[1][i] = colorstuff->greencolors[index_2];
	colorstuff->blue[1][i] = colorstuff->bluecolors[index_2];
    }
    colorstuff->enable_1_mask = ((1 << colorstuff->colorbits) - 1)
	<< colorstuff->colorbits;
    colorstuff->enable_0_mask = ((1 << colorstuff->colorbits) - 1);

    /*
     * doublebuff_swap sets up the colormap. We want the drawing
     * to start off drawing into the 1st buffer, so set the
     * current buffer to 1 so that when doublebuff_swap is called
     * it will set up the first ([0] ) colormap. 
     */
    colorstuff->cur_buff == 1;
    doublebuff_swap(colorstuff);
}


/*
 * Routine to swap buffers by loading a color map that will show
 * the contents of the buffer that was not visible. Also, set the
 * write enable plane so that future writes will only effect the
 * planes which are not visible. 
 */
doublebuff_swap(colorstuff)
    struct colorstuff *colorstuff;
{
    if (colorstuff->cur_buff == 0) {
	/* display first buffer while writing to 2nd */
	/*
	 * Careful!  pw_putcolormap() wants an array or pointer
	 * passed, but the colormap arrays are 2-d 
	 */
	pw_putcolormap(pw, 0, colorstuff->cms_size,
		       colorstuff->red[0],
		       colorstuff->green[0],
		       colorstuff->blue[0]);
	/* set plane mask to write to second buffer */
	colorstuff->plane_mask = colorstuff->enable_1_mask;
	colorstuff->cur_buff = 1;
    } else {
	/* display second buffer while writing to first */
	pw_putcolormap(pw, 0, colorstuff->cms_size,
		       colorstuff->red[1],
		       colorstuff->green[1],
		       colorstuff->blue[1]);

	/* set plane mask to write to first buffer */
	colorstuff->plane_mask = colorstuff->enable_0_mask;
	colorstuff->cur_buff = 0;
    }
}
