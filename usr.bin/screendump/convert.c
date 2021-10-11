#ifndef lint
static	char sccsid[] = "@(#)convert.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1984, 1986 by Sun Microsystems, Inc.
 */

/*
 * Standard rasterfile encode/decode filter.
 */

#include <stdio.h>
#include <varargs.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_io.h>
#include <rasterfile.h>

#ifndef	MY_RAS_TYPE
#define MY_RAS_TYPE	RT_BYTE_ENCODED
#endif

static void error();
static char *Progname;


main(argc, argv)
	int argc;
	char *argv[];
{
	Pixrect *pr;
	struct rasterfile rh;
	colormap_t colormap;

	/*
	 * Process argc/argv.
	 */
	Progname = argv[0];

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

	/*
	 * Load the rasterfile header and check if the input file
	 * is in the standard format or the format we are supposed
	 * to be able to convert.
	 */
	if (pr_load_header(stdin, &rh))
		error(PR_IO_ERR_RASREAD);

	switch (rh.ras_type) {
	case RT_STANDARD:
	case MY_RAS_TYPE:
		break;

	default:
		error("Input raster has incorrect type %d", rh.ras_type);
	}

	/* load the colormap and image */
	colormap.type = RMT_NONE;
	if (pr_load_colormap(stdin, &rh, &colormap) ||
		!(pr = pr_load_std_image(stdin, &rh, &colormap)))
		error(PR_IO_ERR_RASREAD);

	/* dump the header, colormap, and image in the other format */
	if (pr_dump(pr, stdout, &colormap, 
		rh.ras_type == MY_RAS_TYPE ? RT_STANDARD : MY_RAS_TYPE, 0))
		error(PR_IO_ERR_RASWRITE);

	exit(0);
	/* NOTREACHED */
}

/*VARARGS*/
static void
error(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;

	va_start(ap);
	if (fmt = va_arg(ap, char *))
		(void) fprintf(stderr, "%s: ", Progname);
	else
		fmt = va_arg(ap, char *);

	(void) _doprnt(fmt, ap, stderr);
	va_end(ap);

	(void) fprintf(stderr, "\n");

	exit(1);
}
