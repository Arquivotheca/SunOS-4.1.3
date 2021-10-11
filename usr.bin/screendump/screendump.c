#ifndef lint
static char sccsid[] = "@(#)screendump.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1984, 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Write frame buffer image to a file
 */

#include "screendump.h"
#include <pixrect/pr_planegroups.h>

#define	ERR_NOMEM	"Out of memory"

static Pixrect *combine();

#ifndef	MERGE
#define	screendump_main	main
#endif

screendump_main(argc, argv)
	int argc;
	char *argv[];
{
	int c;

	int copy = 1;
	int overlay = 0;
	int type = RT_STANDARD;
	char *display_device = "/dev/fb";
	int xywh[4];
	char groups[PIXPG_INVALID];

	Pixrect *pr, *enpr;
	int i;
	colormap_t *cmapp = 0, cmap;

	xywh[0] = 0;
	xywh[1] = 0;
	xywh[2] = -1;
	xywh[3] = -1;

	/*
	 * Process command-line arguments.
	 */
	Progname = basename(argv[0]);

	opterr = 0;

	while ((c = getopt(argc, argv, "cd:ef:ot:8Cx:y:w:h:X:Y:")) != EOF)
		switch (c) {
		case 'c':
			copy = 0;
			break;
		case 'e':
			type = RT_BYTE_ENCODED;
			break;
		case 'd':
		case 'f':
			display_device = optarg;
			break;
		case 'o':
			overlay = 1;
			break;
		case 't':
			type = atoi(optarg);
			break;
		case '8':
		case 'C':
			overlay = -1;
			break;
		case 'x':
		case 'y':
		case 'X':
		case 'w':
		case 'Y':
		case 'h':
			for (i = 0; "xyXYxywh"[i] != c; i++)
				;
			if ((xywh[i &= 3] = atoi(optarg)) < 0)
				error("Invalid %c value %d", c, xywh[i]);
			break;
		default:
			error((char *) 0, 
	"Usage: %s [-ceoC8] [-f display] [-t type] [-xyXY value] [file]",
				Progname);
			break;
		}

	/*
	 * Open the indicated display device.
	 */
	if (!(pr = pr_open(display_device)) ||
		!(pr = pr_region(pr, xywh[0], xywh[1], 
			xywh[2] >= 0 ? xywh[2] : pr->pr_size.x,
			xywh[3] >= 0 ? xywh[3] : pr->pr_size.y)))
		error("%s %s", PR_IO_ERR_DISPLAY, display_device);

	/*
	 * Open the output file if specified.
	 */
	if (optind < argc && 
		freopen(argv[optind], "w", stdout) == NULL) 
		error("%s %s", PR_IO_ERR_OUTFILE, argv[optind]);

	(void) pr_available_plane_groups(pr, sizeof groups, groups);
	if (overlay == -1) {
	     if (groups[PIXPG_24BIT_COLOR]) {
		 pr_set_plane_group(pr, PIXPG_24BIT_COLOR );
	     }
	     
	} else 	if (groups[PIXPG_OVERLAY] && groups[PIXPG_OVERLAY_ENABLE]) {
		int colorplane;
		
		colorplane = groups[PIXPG_24BIT_COLOR] ?
		    PIXPG_24BIT_COLOR : PIXPG_8BIT_COLOR;
		pr_set_plane_group(pr, PIXPG_OVERLAY_ENABLE);
		enpr = xmem_create(pr->pr_size.x, pr->pr_size.y, 1);
		pr_rop(enpr, 0, 0, pr->pr_size.x, pr->pr_size.y,
			PIX_SRC, pr, 0, 0);

		if (overlay ||
			(overlay = scan_enable(enpr))) {
			pr_set_plane_group(pr, overlay > 0 ? 
				PIXPG_OVERLAY : colorplane);
			pr_destroy(enpr);
		}
		else {
			pr = combine(pr, enpr, &cmap);
			cmapp = &cmap;
			copy = 0;
		}
	}

	/*
	 * Dump the full screen pixrect to the output file.
	 */
	if (pr_dump(pr, stdout, cmapp, type, copy)) 
		error(PR_IO_ERR_RASWRITE);

	exit(0);
}

static
scan_enable(pr)
	Pixrect *pr;
{
	Pixrect *tpr;
	int overlay;

	tpr = xmem_create(pr->pr_size.x, 16, pr->pr_depth);

	overlay = mash(pr, tpr, PIX_DST | PIX_SRC);

	pr_rop(tpr, 0, 0, tpr->pr_size.x, tpr->pr_size.y, 
		PIX_SRC | PIX_DONTCLIP, (Pixrect *) 0, 0, 0);

	overlay -= mash(pr, tpr, PIX_DST | PIX_NOT(PIX_SRC));

	return overlay;
}

static
mash(src, tmp, op)
	Pixrect *src;
	register Pixrect *tmp;
	int op;
{
	register int xy;

	for (xy = 0; xy < src->pr_size.y; xy += tmp->pr_size.y)
		pr_rop(tmp, 0, 0, tmp->pr_size.x, tmp->pr_size.y,
			op, src, 0, xy);

	/* these loops should be binary merges */
	for (xy = 1; xy < tmp->pr_size.y; xy++)
		pr_rop(tmp, 0, 0, tmp->pr_size.x, 1, 
			PIX_SRC | PIX_DST | PIX_DONTCLIP, tmp, 0, xy);

	for (xy = 32; xy < tmp->pr_size.x; xy += 32)
		pr_rop(tmp, 0, 0, 32, 1,
			PIX_SRC | PIX_DST | PIX_DONTCLIP, tmp, xy, 0);

	for (xy = 1; xy < tmp->pr_size.x && xy < 32; xy++)
		pr_rop(tmp, 0, 0, 1, 1,
			PIX_SRC | PIX_DST | PIX_DONTCLIP, tmp, xy, 0);

	return pr_get(tmp, 0, 0) != 0;
}

static Pixrect *
combine(pr, enpr, cmap)
	Pixrect *pr, *enpr;
	colormap_t *cmap;
{
	Pixrect *tpr;
	int w = pr->pr_size.x, h = pr->pr_size.y;
	int bg, fg;

	getcolors(pr, cmap, &bg, &fg);

	tpr = xmem_create(w, h, pr->pr_depth);
	pr_rop(tpr, 0, 0, w, h, PIX_SRC, pr, 0, 0);

	pr_rop(tpr, 0, 0, w, h, PIX_DST & PIX_NOT(PIX_SRC), enpr, 0, 0);

 	if (bg)
		pr_rop(tpr, 0, 0, w, h, 
			PIX_DST | PIX_SRC | PIX_COLOR(bg), enpr, 0, 0);

	pr_set_plane_group(pr, PIXPG_OVERLAY);
	pr_rop(enpr, 0, 0, w, h, PIX_DST & PIX_SRC, pr, 0, 0);

	pr_rop(tpr, 0, 0, w, h,
		PIX_DST ^ PIX_SRC | PIX_COLOR(bg ^ fg), enpr, 0, 0);

	return tpr;
}

#define	abs(x)	((t = (x)) >= 0 ? t : -t)

static
getcolors(pr, cmap, bgp, fgp)
	Pixrect *pr;
	colormap_t *cmap;
	int *bgp, *fgp;
{
	register int i, j, t, err;
	int bg, fg, bgerr = 9999, fgerr = 9999;
	u_char orgb[3][2];
	register u_char (*crgb)[256];

	pr_set_plane_group(pr, PIXPG_OVERLAY);
	pr_getcolormap(pr, 0, 2, orgb[0], orgb[1], orgb[2]);

	crgb = (u_char (*)[256]) xmalloc(256 * 3);
	cmap->type = RMT_EQUAL_RGB;
	cmap->length = 256;
	cmap->map[0] = crgb[0];
	cmap->map[1] = crgb[1];
	cmap->map[2] = crgb[2];

	pr_set_plane_group(pr, PIXPG_8BIT_COLOR);
	pr_getcolormap(pr, 0, 256, crgb[0], crgb[1], crgb[2]);

	for (i = 0; i < 256; i++) {
		for (err = 0, j = 0; j < 3; j++)
			err += abs(orgb[j][0] - crgb[j][i]);
		if (err < bgerr) {
			bg = i;
			bgerr = err;
		}
	}

	for (i = 255; i >= 0; i--) {
		for (err = 0, j = 0; j < 3; j++)
			err += abs(orgb[j][1] - crgb[j][i]);
		if (err < fgerr) {
			fg = i;
			fgerr = err;
		}
	}

	*bgp = bg;
	*fgp = fg;
}
