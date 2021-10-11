/*	@(#)canvas_repaint.c 1.1 92/07/30 SMI	*/
#include <suntool/sunview.h>
#include <suntool/canvas.h>

static void repaint_canvas();

main(argc, argv)
int    argc;
char **argv;
{
    Frame frame;

    frame = window_create(NULL, FRAME, 0);
    window_create(frame, CANVAS,
        CANVAS_RETAINED,      FALSE,
        CANVAS_FIXED_IMAGE,   FALSE,
        CANVAS_REPAINT_PROC,  repaint_canvas,
        0);
    window_main_loop(frame);
    exit(0);
    /* NOTREACHED */
}

static void
repaint_canvas(canvas, pw, repaint_area)
    Canvas 	canvas;
    Pixwin     *pw;
    Rectlist   *repaint_area;
{
    int width   = (int)window_get(canvas, CANVAS_WIDTH);
    int height  = (int)window_get(canvas, CANVAS_HEIGHT);
    int margin  = 10;
    int xleft   = margin;
    int xright  = width - margin;
    int ytop    = margin;
    int ybottom = height - margin;

    /* draw box */
    pw_vector(pw, xleft, ytop, xright, ytop, PIX_SRC, 1);
    pw_vector(pw, xright, ytop, xright, ybottom, PIX_SRC, 1);
    pw_vector(pw, xright, ybottom, xleft, ybottom, PIX_SRC, 1);
    pw_vector(pw, xleft, ybottom, xleft, ytop, PIX_SRC, 1);

    /* draw diagonals */
    pw_vector(pw, xleft, ytop, xright, ybottom, PIX_SRC, 1);
    pw_vector(pw, xright, ytop, xleft, ybottom, PIX_SRC, 1);
}
