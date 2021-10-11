#ifndef lint
static char sccsid[] = "@(#)rasfilter8to1.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * rasfilter8to1:
 * 
 * Converts an 8-bit rasterfile into a 1-bit RT_STANDARD 
 * rasterfile by thresholding or dithering.
 */

#include "screendump.h"

#define	ORDER	16	/* dither matrix order */
#define	WHITE	0	/* background color */
#define	BLACK	~0	/* foreground color */

struct thresh {
	unsigned int avg, red, grn, blu;
};

static void compute_map();
static void filter_image();
static void filter_file();

#ifndef	MERGE
#define	rasfilter8to1_main	main
#endif

rasfilter8to1_main(argc, argv)
	int	argc;
	char	*argv[];
{
	int c;
	struct thresh thresh;
	int dither = 0;

	colormap_t colormap;
	struct rasterfile rh;
	Pixrect *pr = 0;
	u_short map[ORDER][256];

	/*
	 * Process command-line arguments.
	 */
	Progname = basename(argv[0]);

	opterr = 0;

	thresh.avg = 128 * 3;
	thresh.red = thresh.grn = thresh.blu = 0;

	while ((c = getopt(argc, argv, "a:r:g:b:d")) != EOF)
		switch (c) {
		case 'a':
			thresh.avg = atoi(optarg) * 3;
			break;
		case 'r':
			thresh.red = atoi(optarg);
			break;
		case 'g':
			thresh.grn = atoi(optarg);
			break;
		case 'b':
			thresh.blu = atoi(optarg);
			break;
		case 'd':
			dither = ORDER;
			break;

		case '?':
			error((char *) 0, 
"Usage: %s [-d] [-a avg] [-r red] [-g grn] [-b blu] [infile [outfile]]",
				Progname);
		}

	/* open the input file if specified */
	if (optind < argc && 
		freopen(argv[optind++], "r", stdin) == NULL) 
		error("%s %s", PR_IO_ERR_INFILE, argv[--optind]);

	/* open the output file if specified */
	if (optind < argc && 
		freopen(argv[optind], "w", stdout) == NULL) 
		error("%s %s", PR_IO_ERR_OUTFILE, argv[optind]);

	/* Load the rasterfile header */
	if (pr_load_header(stdin, &rh))
		error(PR_IO_ERR_RASREADH);

	if (rh.ras_depth != 8)
		error("input is not 8 bits deep");

	/* Load the colormap */
	colormap.type = RMT_NONE;
	if (pr_load_colormap(stdin, &rh, &colormap))
		error(PR_IO_ERR_RASREAD);

	if (colormap.type != RMT_NONE &&
		(colormap.type != RMT_EQUAL_RGB || colormap.length < 256))
		error("input has unsupported colormap type or length");

	if (rh.ras_type != RT_OLD && rh.ras_type != RT_STANDARD &&
		!(pr = pr_load_image(stdin, &rh, &colormap)))
		error(PR_IO_ERR_RASREAD);

	/* Write new header */
	rh.ras_type = RT_STANDARD;
	rh.ras_depth = 1;
	rh.ras_length = mpr_linebytes(rh.ras_width, 1) * rh.ras_height;
	rh.ras_maptype = RMT_NONE;
	rh.ras_maplength = 0;

	if (pr_dump_header(stdout, &rh, (colormap_t *) 0) == PIX_ERR)
		error(PR_IO_ERR_RASWRITE);

	if (rh.ras_width <= 0 || rh.ras_height <= 0)
		exit(0);

	/* compute output value for each input value */
	compute_map(&colormap, map, dither, &thresh);

	/* map from an 8-bit deep raster to a 1-bit deep raster */
	if (pr) 
		filter_image(rh.ras_width, rh.ras_height, map, dither, 
			(u_char *) mpr_d(pr)->md_image, stdout,
			mpr_d(pr)->md_linebytes - rh.ras_width);
	else
		filter_file(rh.ras_width, rh.ras_height, map, dither, 
			stdin, stdout, 
			mpr_linebytes(rh.ras_width, 8) - rh.ras_width);

	exit(0);
}


/* Compute pixel values for each colormap entry */
static void
compute_map(colormap, map, order, thresh)
	register colormap_t *colormap;
	u_short (*map)[256];
	int order;
	register struct thresh *thresh;
{
	register int i;
	u_char tmp_rgb[256];

	/*
	 * Order 16 dither matrix.
	 * The [0][0] and [7][7] entries have been adjusted to allow
	 * noiseless black and white regions.
	 */
	static u_char dmat[ORDER][16] = {
  1, 128,  32, 160,   8, 136,  40, 168,   2, 130,  34, 162,  10, 138,  42, 170,
192,  64, 224,  96, 200,  72, 232, 104, 194,  66, 226,  98, 202,  74, 234, 106,
 48, 176,  16, 144,  56, 184,  24, 152,  50, 178,  18, 146,  58, 186,  26, 154,
240, 112, 208,  80, 248, 120, 216,  88, 242, 114, 210,  82, 250, 122, 218,  90,
 12, 140,  44, 172,   4, 132,  36, 164,  14, 142,  46, 174,   6, 134,  38, 166,
204,  76, 236, 108, 196,  68, 228, 100, 206,  78, 238, 110, 198,  70, 230, 102,
 60, 188,  28, 156,  52, 180,  20, 148,  62, 190,  30, 158,  54, 182,  22, 150,
252, 124, 220,  92, 244, 116, 212,  83, 254, 126, 222,  94, 246, 118, 214,  86,
  3, 131,  35, 163,  11, 139,  43, 171,   1, 129,  33, 161,   9, 137,  41, 169,
195,  67, 227,  99, 203,  75, 235, 107, 193,  65, 225,  97, 201,  73, 233, 105,
 51, 179,  19, 147,  59, 187,  27, 155,  49, 177,  17, 145,  57, 185,  25, 153,
243, 115, 211,  83, 251, 123, 219,  91, 241, 113, 209,  81, 249, 121, 217,  89,
 15, 143,  47, 175,   7, 135,  39, 167,  13, 141,  45, 173,   5, 133,  37, 165,
207,  79, 239, 111, 199,  71, 231, 103, 205,  77, 237, 109, 197,  69, 229, 101,
 63, 191,  31, 159,  55, 183,  23, 151,  61, 189,  29, 157,  53, 181,  21, 149,
255, 127, 223,  95, 247, 119, 215,  87, 253, 125, 221,  93, 245, 117, 213,  85
	};

	/* supply a colormap if the input file didn't have one */
	if (colormap->type == RMT_NONE) {
		colormap->map[0] = tmp_rgb;
		colormap->map[1] = tmp_rgb;
		colormap->map[2] = tmp_rgb;

		for (i = 0; i < 256; i++) 
			tmp_rgb[i] = i;
	}

	/* not dithering, check colormap entries against thresholds */
	if (order == 0) {
		for (i = 0; i < 256; i++)
			(*map)[i] = (colormap->map[0][i] + 
				colormap->map[1][i] + 
				colormap->map[2][i] >= thresh->avg &&
				colormap->map[0][i] >= thresh->red &&
				colormap->map[1][i] >= thresh->grn &&
				colormap->map[2][i] >= thresh->blu) ?
				WHITE : BLACK;
	}
	/* dithering */
	else {
		register u_char (*drow)[16];
		register int x;

		for (drow = dmat; *drow < dmat[order]; drow++, map++)
			for (i = 0; i < 256; i++) {
				(*map)[i] = BLACK;
				for (x = 0; x < 16; x++)
					if (colormap->map[0][i] + 
						colormap->map[1][i] + 
						colormap->map[2][i] >=
						(*drow)[x] * 3)
						(*map)[i] ^= (1 << 15) >> x;
			}
	}
}

static void
filter_image(w, h, map, order, in, out, pad)
	register int w, h;
	u_short (*map)[256];
	int order;
	register u_char *in;
	register FILE *out;
	int pad;
{
	int y = -1;

	while (--h != -1) {
		register int x;
		register u_short *mp, dmask, dtmp;

		if (++y >= order)
			y = 0;
		mp = map[y];

		x = w >> 4;
		while (--x != -1) {
			dmask = 1 << 15;
			dtmp = 0;
			do {
				dtmp |= dmask & mp[*in++];
			} while (dmask >>= 1);
			putc(dtmp >> 8, out);
			putc(dtmp, out);
		}

		if (x = w & 15) {
			dmask = 1 << 15;
			dtmp = 0;
			while (--x != -1) {
				dtmp |= dmask & mp[*in++];
				dmask >>= 1;
			}
			putc(dtmp >> 8, out);
			putc(dtmp, out);
		}

		in += pad;
	}
}

static void
filter_file(w, h, map, order, in, out, pad)
	register int w, h;
	u_short (*map)[256];
	int order;
	register FILE *in, *out;
	int pad;
{
	int y = -1;

	while (--h != -1) {
		register int x;
		register u_short *mp, dmask, dtmp;
		register int c;

		if (++y >= order)
			y = 0;
		mp = map[y];

		x = w >> 4;
		while (--x != -1) {
			dmask = 1 << 15;
			dtmp = 0;
			do {
				if ((c = getc(in)) == EOF)
					error(PR_IO_ERR_RASREAD);

				dtmp |= dmask & mp[c];
			} while (dmask >>= 1);
			putc(dtmp >> 8, out);
			putc(dtmp, out);
		}

		if (x = w & 15) {
			dmask = 1 << 15;
			dtmp = 0;
			while (--x != -1) {
				if ((c = getc(in)) == EOF)
					error(PR_IO_ERR_RASREAD);

				dtmp |= dmask & mp[c];
				dmask >>= 1;
			}
			putc(dtmp >> 8, out);
			putc(dtmp, out);
		}

		for (x = pad; x; x--)
			(void) getc(in);
	}
}
