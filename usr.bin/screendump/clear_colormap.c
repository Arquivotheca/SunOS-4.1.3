#ifndef lint
static char sccsid[] = "@(#)clear_colormap.c 1.1 92/07/30 SMI";
#endif

/*
 * clear_colormap - clear the colormap
 */

#include "screendump.h"
#include <pixrect/pr_planegroups.h>

#ifndef	MERGE
#define	clear_colormap_main	main
#endif

#define TRUE_COLOR_SIZE 256

clear_colormap_main(argc, argv)
	int	argc;
	char	*argv[];
{
	int c;
	Pixrect *pr = NULL;
	char groups[PIXPG_INVALID];
	static u_char rgb[2] = { 255, 0 };

	char *dev = "/dev/fb";
	int clear = 1, overlay = 1;

	Progname = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "d:f:no")) != EOF)
		switch (c) {
		case 'd':
		case 'f':
			dev = optarg;
			break;
		case 'n':
			clear = 0;
			break;
		case 'o':
			overlay = 0;
			break;
		default:
			error((char *) 0, "Usage: %s [-f display] [-no]",
				Progname);
		}

	if (!(pr = pr_open(dev))) 
		error("%s %s", PR_IO_ERR_DISPLAY, dev);

	/* if it is true color, restore to linear ramp */
	pr_available_plane_groups(pr, PIXPG_INVALID, groups);
	if (groups[PIXPG_24BIT_COLOR]) {
	    char linear[TRUE_COLOR_SIZE];
	    register int i;
	    for (i = 0; i < TRUE_COLOR_SIZE; i++)
		linear[i] = i;
	    (void) pr_set_plane_group(pr, PIXPG_24BIT_COLOR);
	    (void) pr_putlut(pr, 0, TRUE_COLOR_SIZE, linear, linear, linear);
	}
	else {
	    /* reset colormap colors 0 and 1 */
	    (void) pr_putcolormap(pr, 0, 2, rgb, rgb, rgb);

            /* reset last colormap entry */
            (void) pr_putcolormap(pr, (1 << pr->pr_depth) - 1, 1,
                 &rgb[1], &rgb[1], &rgb[1]);
        }

	/* clear frame buffer */
	if (clear) 
		(void) pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y,
			PIX_CLR, (Pixrect *) 0, 0, 0);

	if (groups[PIXPG_OVERLAY] && groups[PIXPG_OVERLAY_ENABLE]) {
		/* reset overlay plane colormap */
		pr_set_plane_group(pr, PIXPG_OVERLAY);
		(void) pr_putcolormap(pr, 0, 2, rgb, rgb, rgb);

		/* clear overlay plane */
		if (clear) 
			(void) pr_rop(pr, 0, 0, 
				pr->pr_size.x, pr->pr_size.y,
				PIX_CLR, (Pixrect *) 0, 0, 0);

		/* set overlay enable plane */
		if (overlay) {
			pr_set_plane_group(pr, PIXPG_OVERLAY_ENABLE);
			(void) pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y,
					PIX_SET, (Pixrect *) 0, 0, 0);
		}
	}
        if (groups[PIXPG_VIDEO_ENABLE] && overlay && clear) {
	    pr_set_plane_group(pr, PIXPG_VIDEO_ENABLE);
	    pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y,
		   PIX_CLR, (Pixrect *) 0, 0, 0);
	}
	exit(0);
}
