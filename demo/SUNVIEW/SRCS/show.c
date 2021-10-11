/*	show.c	1.1	92/07/30 */

#include <stdio.h>
#include <sys/file.h>
#include <suntool/tool_hs.h>
#include <suntool/gfx_hs.h>


#define PIXWIDTH	512
#define PIXHEIGHT	480

struct gfxsubwindow	*picwin;

unsigned char	red [256], green [256], blue [256];
colormap_t	Colormap = { RMT_EQUAL_RGB, 256, red, green, blue };


extern struct gfxsubwindow	*gfxsw_init();


main (argc, argv)
    int argc;
    char *argv [];
{
    FILE	*input;
    int		image_count;
    int		dx, dy;
    int		w, h;
    char	*image_file;
    struct pixrect	*source_pixrect;

    if (argc < 2) {
        (void) fprintf (stderr,
	    "%s requires one or more image files as arguments.\n", *argv);
        exit (1);
    }

    if ((picwin = gfxsw_init (0, argv)) == (struct gfxsubwindow *) 0)
	exit (1);

    while (1) {
	for (image_count = 1; image_count < argc; image_count++) {
	    if (strcmp (argv [image_count], "-r") == 0)
		continue;
	    if (picwin->gfx_flags & GFX_DAMAGED) {
		(void) gfxsw_handlesigwinch (picwin);
		pw_write (picwin->gfx_pixwin, 0, 0, 1600, 1600,
		    PIX_SRC, 0, 0, 0);
	    }
	    if (picwin->gfx_flags & GFX_RESTART) {
		picwin->gfx_flags &= ~GFX_RESTART;
		continue;
	    }
	    image_file = argv [image_count];
	    if (NULL == (input = fopen (image_file, "r"))) {
		(void) fprintf (stderr, "Can't open %s.\n", image_file);
		exit (2);
	    }
	    source_pixrect = pr_load (input, &Colormap);
	    (void) fclose (input);
	    if (source_pixrect == NULL) {
		(void) fprintf (stderr, "NULL source_pixrect %s.\n",image_file);
		exit (3);
	    }
	    red [254] = 255;	red [255] = 0;
	    green [254] = 255;	green [255] = 0;
	    blue [254] = 255;	blue [255] = 0;
	    pw_setcmsname (picwin->gfx_pixwin, "show");
	    pw_putcolormap (picwin->gfx_pixwin, 0, 256, red, green, blue );
	    w = source_pixrect->pr_size.x;
	    h = source_pixrect->pr_size.y;
	    dx = (picwin->gfx_rect.r_width - w) / 2;
	    dy = (picwin->gfx_rect.r_height - h) / 2;
	    pw_rop (picwin->gfx_pixwin, dx, dy, w, h, PIX_SRC,
		source_pixrect, 0, 0);
	    if (source_pixrect)
		(void) pr_destroy (source_pixrect);
	    sleep (5);
	} /* for */
    } /* while */
} /* main */
