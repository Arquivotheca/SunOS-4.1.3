#ifndef lint
static	char sccsid[] = "@(#)rastrepl.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1984, 1986 by Sun Microsystems, Inc.
 */

/*
 * Expand a raster by 2x - monochrome and 8-plane color
 *
 * The general approach is to load up the rasterfile to be replicated into
 *   a memory pixrect (using the general purpose pr_load_image routine in case
 *   the rasterfile is in a non-standard format), replicate the data into a
 *   memory pixrect twice as high and wide, and then dump that pixrect (using
 *   the general purpose pr_dump).
 * However, this approach is very expensive in terms of virtual memory, so
 *   for standard format rasterfiles the replication is done in a space
 *   efficient way by only reading in and replicating one line of the image
 *   at a time.
 */

#include "screendump.h"

#define ERR_RASDEPTH	"Input raster has incorrect depth"

static u_short expanded_byte [256] = {
	0x0000, 0x0003, 0x000c, 0x000f, 0x0030, 0x0033, 0x003c, 0x003f,
	0x00c0, 0x00c3, 0x00cc, 0x00cf, 0x00f0, 0x00f3, 0x00fc, 0x00ff,
	0x0300, 0x0303, 0x030c, 0x030f, 0x0330, 0x0333, 0x033c, 0x033f,
	0x03c0, 0x03c3, 0x03cc, 0x03cf, 0x03f0, 0x03f3, 0x03fc, 0x03ff,
	0x0c00, 0x0c03, 0x0c0c, 0x0c0f, 0x0c30, 0x0c33, 0x0c3c, 0x0c3f,
	0x0cc0, 0x0cc3, 0x0ccc, 0x0ccf, 0x0cf0, 0x0cf3, 0x0cfc, 0x0cff,
	0x0f00, 0x0f03, 0x0f0c, 0x0f0f, 0x0f30, 0x0f33, 0x0f3c, 0x0f3f,
	0x0fc0, 0x0fc3, 0x0fcc, 0x0fcf, 0x0ff0, 0x0ff3, 0x0ffc, 0x0fff,
	0x3000, 0x3003, 0x300c, 0x300f, 0x3030, 0x3033, 0x303c, 0x303f,
	0x30c0, 0x30c3, 0x30cc, 0x30cf, 0x30f0, 0x30f3, 0x30fc, 0x30ff,
	0x3300, 0x3303, 0x330c, 0x330f, 0x3330, 0x3333, 0x333c, 0x333f,
	0x33c0, 0x33c3, 0x33cc, 0x33cf, 0x33f0, 0x33f3, 0x33fc, 0x33ff,
	0x3c00, 0x3c03, 0x3c0c, 0x3c0f, 0x3c30, 0x3c33, 0x3c3c, 0x3c3f,
	0x3cc0, 0x3cc3, 0x3ccc, 0x3ccf, 0x3cf0, 0x3cf3, 0x3cfc, 0x3cff,
	0x3f00, 0x3f03, 0x3f0c, 0x3f0f, 0x3f30, 0x3f33, 0x3f3c, 0x3f3f,
	0x3fc0, 0x3fc3, 0x3fcc, 0x3fcf, 0x3ff0, 0x3ff3, 0x3ffc, 0x3fff,
	0xc000, 0xc003, 0xc00c, 0xc00f, 0xc030, 0xc033, 0xc03c, 0xc03f,
	0xc0c0, 0xc0c3, 0xc0cc, 0xc0cf, 0xc0f0, 0xc0f3, 0xc0fc, 0xc0ff,
	0xc300, 0xc303, 0xc30c, 0xc30f, 0xc330, 0xc333, 0xc33c, 0xc33f,
	0xc3c0, 0xc3c3, 0xc3cc, 0xc3cf, 0xc3f0, 0xc3f3, 0xc3fc, 0xc3ff,
	0xcc00, 0xcc03, 0xcc0c, 0xcc0f, 0xcc30, 0xcc33, 0xcc3c, 0xcc3f,
	0xccc0, 0xccc3, 0xcccc, 0xcccf, 0xccf0, 0xccf3, 0xccfc, 0xccff,
	0xcf00, 0xcf03, 0xcf0c, 0xcf0f, 0xcf30, 0xcf33, 0xcf3c, 0xcf3f,
	0xcfc0, 0xcfc3, 0xcfcc, 0xcfcf, 0xcff0, 0xcff3, 0xcffc, 0xcfff,
	0xf000, 0xf003, 0xf00c, 0xf00f, 0xf030, 0xf033, 0xf03c, 0xf03f,
	0xf0c0, 0xf0c3, 0xf0cc, 0xf0cf, 0xf0f0, 0xf0f3, 0xf0fc, 0xf0ff,
	0xf300, 0xf303, 0xf30c, 0xf30f, 0xf330, 0xf333, 0xf33c, 0xf33f,
	0xf3c0, 0xf3c3, 0xf3cc, 0xf3cf, 0xf3f0, 0xf3f3, 0xf3fc, 0xf3ff,
	0xfc00, 0xfc03, 0xfc0c, 0xfc0f, 0xfc30, 0xfc33, 0xfc3c, 0xfc3f,
	0xfcc0, 0xfcc3, 0xfccc, 0xfccf, 0xfcf0, 0xfcf3, 0xfcfc, 0xfcff,
	0xff00, 0xff03, 0xff0c, 0xff0f, 0xff30, 0xff33, 0xff3c, 0xff3f,
	0xffc0, 0xffc3, 0xffcc, 0xffcf, 0xfff0, 0xfff3, 0xfffc, 0xffff};

static void image_repl(), space_efficient_repl(), mapbits(), mapbytes();

#ifndef	MERGE
#define	rastrepl_main	main
#endif

rastrepl_main(argc, argv)
	int	argc;
	char	**argv;
{
	register Pixrect *input_pr, *output_pr;
	struct rasterfile rh;
	colormap_t colormap;

	/*
	 * Process command-line arguments.
	 */
	Progname = basename(argv[0]);

	switch (argc) {
	case 3:
		if (freopen(argv[2], "w", stdout) == NULL) 
			error("%s %s", PR_IO_ERR_OUTFILE, argv[2]);
	case 2:
		if (freopen(argv[1], "r", stdin) == NULL) 
			error("%s %s", PR_IO_ERR_INFILE, argv[1]);
	case 1:
		break;

	default:
		error((char *) 0, "Usage: %s [infile [outfile]]", Progname);
	}

	if (pr_load_header(stdin, &rh))
		error(PR_IO_ERR_RASREADH);

	colormap.type = RMT_NONE;
	if (pr_load_colormap(stdin, &rh, &colormap))
		error(PR_IO_ERR_RASREAD);

	switch (rh.ras_type) {
	case RT_OLD:	/* Fall through */
	case RT_STANDARD:
		space_efficient_repl(&rh, &colormap);
		exit(0);
	}

	if ((input_pr = pr_load_image(stdin, &rh, &colormap)) == NULL)
		error(PR_IO_ERR_RASREAD);

	if (input_pr->pr_depth != 1 && input_pr->pr_depth != 8)
		error(ERR_RASDEPTH);

	output_pr = 
		xmem_create(input_pr->pr_size.x * 2, input_pr->pr_size.y * 2,
			input_pr->pr_depth);

	image_repl(input_pr->pr_depth, 
		input_pr->pr_size.x, input_pr->pr_size.y,
		(u_char *) mpr_d(input_pr)->md_image, 
		mpr_d(input_pr)->md_linebytes,
		(u_char *) mpr_d(output_pr)->md_image, 
		mpr_d(output_pr)->md_linebytes);

	if (pr_dump(output_pr, stdout, &colormap, rh.ras_type, 0))
		error(PR_IO_ERR_RASWRITE);

	exit(0);
}

static void
space_efficient_repl(rh, colormap)
	struct rasterfile *rh;
	colormap_t *colormap;
{
	register int h, w;
	int nb, nob;
	char *from, *to;
	void (*mapper)();
	
	switch (rh->ras_depth) {
	case 1:
		mapper = mapbits;
		break;
	case 8:
		mapper = mapbytes;
		break;
	default:
		error(ERR_RASDEPTH);
	}

	w = rh->ras_width;
	nb = mpr_linebytes(w, rh->ras_depth);
	nob = mpr_linebytes(2*w, rh->ras_depth);
	h = rh->ras_height;

	rh->ras_width *= 2;
	rh->ras_height *= 2;
	rh->ras_length = h * 2 * nob;

	if (pr_dump_header(stdout, rh, colormap) == PIX_ERR)
		error(PR_IO_ERR_RASWRITEH);

	from = xmalloc(nb + nob);
	to = from + nb;

	while (h--) {
		if (fread(from, 1, nb, stdin) != nb)
			error(PR_IO_ERR_RASREAD);

                (*mapper)(from, to, nb);

		if (fwrite(to, 1, nob, stdout) != nob ||
			fwrite(to, 1, nob, stdout) != nob)
			error(PR_IO_ERR_RASWRITE);
        }
}

static void
mapbits(from, to, nb)
	register char *from, *to;
	register int nb;
{
	register u_short *tab = expanded_byte;

	while (--nb != -1) {
		register short x;

		x = tab[(u_char) *from++];
		*to++ = x >> 8;
		*to++ = x;
        }
}

static void
mapbytes(from, to, nb)
	register char *from, *to;
	register int nb;
{
	while (--nb != -1) {
		register char x;

		x = *from++;
		*to++ = x;
		*to++ = x;
        }
}

static void
image_repl(depth, w, h, in0, ilb, out0, olb)
	int depth;
	register int w, h;
	u_char *in0;
	register int ilb;
	register u_char *out0;
	register int olb;
{
	if (depth == 1) {
		register u_short *expand = expanded_byte;

		w = w + 7 >> 3;

		while (--h != -1) {
			register u_char *in = in0;
			register u_short *out = (u_short *) out0;
			register int x = w;

			while (--x != -1) 
				*out++ = expand[*in++];

			out = (u_short *) out0;
			(void) bcopy((char *) out, (char *) (out0 += olb), 
				w * 2);
			in0 += ilb;
			out0 += olb;
		}
	}
	else if (depth == 8) {
		while (--h != -1) {
			register u_char *in = in0;
			register u_char *out = out0;
			register int x = w;
			register u_char c;

			while (--x != -1) {
				c = *in++;
				*out++ = c;
				*out++ = c;
			}

			out = out0;
			(void) bcopy((char *) out, (char *) (out0 += olb), 
				w * 2);
			in0 += ilb;
			out0 += olb;
		}
	}
}
