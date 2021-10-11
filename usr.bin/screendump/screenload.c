#ifndef lint
static char sccsid[] = "@(#)screenload.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1984, 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * screenload -- load the frame buffer with image from rasterfile
 */

#include "screendump.h"
#include <sys/file.h>
#include <sys/ioctl.h>	/* for fbio.h */
#include <sun/fbio.h>	/* for FBIO_WID_ALLOC */

#define WARN_MESSAGE_PAUSE	4

static short rootimage[] = {
	0x8888, 0x8888,
	0x8888, 0x8888,
	0x2222, 0x2222,
	0x2222, 0x2222
};

mpr_static_static(rootgray_pr, 32, 4, 1, rootimage);

static Pixrect *find_color_display();

#ifndef	MERGE
#define	screenload_main	main
#endif

screenload_main(argc, argv)
	int argc;
	char *argv[];
{
	extern long strtol();

	int c, errflg = 0;

	int fill = 1, fill_color = ~0;
	Pixrect *fill_pr = (Pixrect *) 0;
	char *display_device = "/dev/fb";
	int over_flag = 0;
	int pause_flag = 0;
	int rev_flag = 0;
	int warn_flag = 0;
	int xywh[4][2];
	char plngroups[PIXPG_LAST_PLUS_ONE];

	FILE *input_file = stdin;
	colormap_t colormap;
	struct rasterfile rh;
	struct pr_subregion dst;
	struct pr_prpos src;

	/*
	 * Process command-line arguments.
	 */
	Progname = basename(argv[0]);
	bzero((char *) xywh, sizeof xywh);

	opterr = 0;
	while ((c = getopt(argc, argv, "bdf:gh:i:noprwx:y:X:Y:")) != EOF)
		switch (c) {

		case 'b':
			fill_pr = (Pixrect *) 0;
			fill_color = ~0;
			break;

		case 'd':
			warn_flag++;
			break;

		case 'f':
			display_device = optarg;
			break;

		case 'g':
			fill_pr = &rootgray_pr;
			break;

		case 'h': {
			int h;
			short *image;

			if ((h = (int) strtol(optarg, (char **) 0, 16)) <= 0) {
				errflg++;
				break;
			}

			fill_pr = xmem_create(16, h, 1);
			image = mpr_d(fill_pr)->md_image;
			
			while (h-- && optind < argc) 
				*image++ = (short) strtol(argv[optind++],
					(char **) 0, 16);

			if (h > 0)
				errflg++;

			}
			break;

		case 'i':
			fill_color = atoi(optarg);
			break;

		case 'n':
			fill = 0;
			break;

		case 'o':
			over_flag++;
			break;

		case 'p':
			pause_flag++;
			break;

		case 'r':
			rev_flag++;
			break;

		case 'w':
			fill_pr = (Pixrect *) 0;
			fill_color = 0;
			break;

		case 'x':
		case 'y':
		case 'X':
		case 'Y': {
			int i;

			for (i = 0; "xyXY"[i] != c; i++)
				;
			xywh[i][0]++;
			xywh[i][1] = atoi(optarg);
			}
			break;

		default:
			errflg++;
			break;
		}

	if (errflg) 
		error((char *) 0, 
"Usage: %s [-dopr] [-f display] [-xyXY value]\n\
	[-i color] [-bgnw] [-h count data ...] [file]",
			Progname);

	if (optind < argc &&
		(input_file = fopen(argv[optind], "r")) == NULL) 
		error("%s %s", PR_IO_ERR_INFILE, argv[optind]);

	/* load the image from the file */
	colormap.type = RMT_NONE;
	if (pr_load_header(input_file, &rh) || 
	    pr_load_colormap(input_file, &rh, &colormap))
		error(PR_IO_ERR_RASREAD);


	/* open display device and set the right plane groups */
	if (!(dst.pr = find_color_display(display_device, rh.ras_depth)))
	    error(PR_IO_ERR_DISPLAY);
	pr_available_plane_groups(dst.pr, PIXPG_LAST_PLUS_ONE, plngroups);
	
	if ((src.pr = pr_load_image(input_file, &rh, &colormap)) == NULL)
	    error(PR_IO_ERR_RASREAD);
	(void) fclose(input_file);
	
	if (dst.pr->pr_depth != src.pr->pr_depth &&
	    src.pr->pr_depth != 1) {
		error (PR_IO_ERR_CDEPTH);
	}
	/* 
	 * If the input file has a colormap, try to write it to the
	 * display device.  We could probably do something more
	 * intelligent for a monochrome file displayed on a color device.
	 */
	if (colormap.type == RMT_EQUAL_RGB &&
	    ( dst.pr->pr_depth <= 9 ?
	     pr_putcolormap(dst.pr, 0, colormap.length, 
			    colormap.map[0], colormap.map[1], colormap.map[2])
	     :
	     pr_putlut( dst.pr, 0, colormap.length, 
		       colormap.map[0], colormap.map[1], colormap.map[2])
	     )
	    )
	    error(PR_IO_ERR_PIXRECT);

	/*
	 * Transfer the memory pixrect data to the display pixrect.
	 * Clip or fill the background as necessary for mismatched
	 * image/display dimensions.
	 */
	dst.pos.x = dst.pr->pr_size.x - src.pr->pr_size.x;
	dst.pos.y = dst.pr->pr_size.y - src.pr->pr_size.y;

	if (warn_flag && (dst.pos.x != 0 || dst.pos.y != 0)) {
		(void) fprintf(stderr, 
		"%s: warning - displaying %d x %d image on %d x %d screen\n",
			Progname,
			src.pr->pr_size.x, src.pr->pr_size.y, 
			dst.pr->pr_size.x, dst.pr->pr_size.y);
			sleep(WARN_MESSAGE_PAUSE);
	}

	dst.pos.x = xywh[0][0] ? 
		xywh[0][1] : dst.pos.x < 0 ? 0 : dst.pos.x / 2;
	dst.pos.y = xywh[1][0] ? 
		xywh[1][1] : dst.pos.y < 0 ? 0 : dst.pos.y / 2;
	dst.size.x = xywh[2][0] ? xywh[2][1] : src.pr->pr_size.x;
	dst.size.y = xywh[3][0] ? xywh[3][1] : src.pr->pr_size.y;

	src.pos.x = 0;
	src.pos.y = 0;

	pr_clip(&dst, &src);
	


	if (plngroups[PIXPG_OVERLAY_ENABLE])
	{
	    int                 pg;
	    int                 en_color;

	    pg = pr_get_plane_group(dst.pr);
	    pr_set_planes(dst.pr, PIXPG_OVERLAY_ENABLE, PIX_ALL_PLANES);

	    en_color = (pg == PIXPG_OVERLAY) ? 1 : 0;
	    pr_rop(dst.pr, dst.pos.x, dst.pos.y, dst.size.x, dst.size.y,
		PIX_SRC | PIX_COLOR(en_color), (Pixrect *) 0, 0, 0);
	    pr_set_planes(dst.pr, PIXPG_OVERLAY, PIX_ALL_PLANES);
	    if (!en_color)
		pr_rop(dst.pr, dst.pos.x, dst.pos.y, dst.size.x, dst.size.y,
		    PIX_SRC | PIX_COLOR(en_color), (Pixrect *) 0, 0, 0);
	    pr_set_planes(dst.pr, pg, PIX_ALL_PLANES);
	}

	if (plngroups[PIXPG_WID])
	{
	    int                 pg;
	    struct fb_wid_alloc wid;

	    pg = pr_get_plane_group(dst.pr);
	    wid.wa_type =
		(rh.ras_depth == 8) ? FB_WID_SHARED_8 : FB_WID_SHARED_24;
	    wid.wa_index = -1;
	    wid.wa_count = 1;
	    pr_ioctl(dst.pr, FBIO_WID_ALLOC, &wid);
	    pr_set_planes(dst.pr, PIXPG_WID, PIX_ALL_PLANES);
	    pr_rop(dst.pr, dst.pos.x, dst.pos.y, dst.size.x, dst.size.y,
		PIX_COLOR(wid.wa_index) | PIX_SRC, (Pixrect *) 0, 0, 0);
	    pr_set_planes(dst.pr, pg, PIX_ALL_PLANES);
	}

	if (fill && (
		dst.pos.x > 0 || dst.pos.y > 0 ||
		dst.size.x < dst.pr->pr_size.x || 
		dst.size.y < dst.pr->pr_size.y))
		(void) pr_replrop(dst.pr, 0, 0, 
			dst.pr->pr_size.x, dst.pr->pr_size.y, 
			PIX_SRC | PIX_COLOR(fill_color) | PIX_DONTCLIP,
			fill_pr, 0, 0);

	if (prs_rop(dst, PIX_COLOR(fill_color) |
		(rev_flag ? PIX_NOT(PIX_SRC) : PIX_SRC), src))
		error(PR_IO_ERR_PIXRECT);

	/*
	 * If pause option was given, wait for a newline to be typed on 
	 * standard input before exiting.
	 */
	if (pause_flag)
		while ((c = getchar()) != EOF && c != '\n')
			/* nothing */ ;

	exit(0);
}


/*
 * This used to just look for the first color framebuffer it could find
 * but now it will start /dev/fb and keep searching until some framebuffer
 * with the appropriate plane group is found.
 */

static Pixrect *
find_color_display(display, depth)
char	*display;
int	depth;
{
    static char    *color_list[] = {
	"",
	"/dev/fb",
	"/dev/bwtwo0",
	"/dev/cgtwelve0",
	"/dev/cgnine0",
	"/dev/cgeight0",
	"/dev/cgsix0",
	"/dev/cgfive0",
	"/dev/cgthree0",
	"/dev/gpone0a",
	"/dev/cgtwo0",
	"/dev/cgfour0",
	"/dev/cgone0",
	0
    };

    register char **p;
    register Pixrect *pr = 0;
    int             fd;
    char            plngroups[PIXPG_LAST_PLUS_ONE];

    color_list[0] = display;
    for (p = color_list; *p; p++)
	/*
	 * Try to open the file first to avoid the dumb messages pr_open
	 * prints.
	 */
	if ((fd = open (*p, O_RDWR)) >= 0) {
	    (void) close (fd);
	    if (pr = pr_open (*p)) {
		pr_available_plane_groups(pr, PIXPG_LAST_PLUS_ONE, plngroups);
		switch (depth)
		{
		    case 1:
			if (plngroups[PIXPG_OVERLAY])
			{
			    pr_set_planes(pr, PIXPG_OVERLAY, PIX_ALL_PLANES);
			    return pr;
			}
			return pr;

		    case 8:
			if (plngroups[PIXPG_8BIT_COLOR])
			{
			    pr_set_planes(pr, PIXPG_8BIT_COLOR, PIX_ALL_PLANES);
			    return pr;
			}
			break;

		    case 24:
		    case 32:
			if (plngroups[PIXPG_24BIT_COLOR])
			{
			    pr_set_planes(pr, PIXPG_24BIT_COLOR,PIX_ALL_PLANES);
			    return pr;
			}
		}
	    }
	}

    return 0;
}
