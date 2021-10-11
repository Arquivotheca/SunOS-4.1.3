#ifndef lint
static char sccsid[] = "@(#)rasfilter_rgbtobgr.c	1.1 92/07/30 SMI";
#endif

/*
 * Program to convert 24 and 32 bit rasterfiles from RGB byte ordering
 * to BGR byte ordering (and vica versa). 32 Bit format assumes alpha is
 * most significant byte. Only reads stdin and writes to stdout(doesn't
 * take filenames as arguments).
 */

#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/varargs.h>
#include <rasterfile.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_io.h>
#include <pixrect/memvar.h>

#ifndef RT_FORMAT_RGB
#define RT_FORMAT_RGB 3
#endif

char           *malloc();
static void     rgbtobgr();	       /* swapping function */
static void     pass_thru();	       /* a dummy */
static void     error();

static int      step_over;	       /* how many bytes to the next pixel */


static char    *Progname;

main(argc, argv)
    int             argc;
    char           *argv[];

{
    struct rasterfile rh;
    colormap_t      colormap;
    int             i,
                    length,
                    n_read;
    unsigned char  *buf;
    void            (*filter_func) ();


    Progname = *argv;

    switch (argc) {
    case 3:
	if (freopen(argv[2], "w", stdout) == NULL)
	    error("Cannot open output file:%s.\n", argv[2]);
    case 2:
	if (freopen(argv[1], "r", stdin) == NULL)
	    error("Cannot open input file:%s.\n", argv[1]);
    case 1:
	break ;

    default:
	Usage();
    }

    if (pr_load_header(stdin, &rh) == PIX_ERR)
	error("Error reading rasterfile header.\n");

    filter_func = rgbtobgr;
    if (rh.ras_depth != 24 && rh.ras_depth != 32)
	filter_func = pass_thru;

    step_over = rh.ras_depth / 8;

    if (rh.ras_type == RT_STANDARD) {
	colormap.type = colormap.length = 0;
	colormap.map[0] = colormap.map[1] = colormap.map[2] = NULL;
	if (pr_load_colormap(stdin, &rh, &colormap) == PIX_ERR)
	    error("failed to load input file\n");
	rh.ras_type = RT_FORMAT_RGB;
    }
    else if (rh.ras_type == RT_FORMAT_RGB) {
	if (pr_load_colormap(stdin, &rh, &colormap) == PIX_ERR)
	    error("failed to load colormap\n");
	rh.ras_type = RT_STANDARD;
    }
    else
	error("unknown rasterfile type %d\n", rh.ras_type);


    if (pr_dump_header(stdout, &rh, &colormap) == PIX_ERR)
	error("Error writing rasterfile header.\n");

    length = mpr_linebytes(rh.ras_depth, rh.ras_width);
    buf = (unsigned char *) malloc((unsigned) length);

    for (i = 0; i < rh.ras_height; i++) {
	if ((n_read = fread((char *) buf, 1, length, stdin)) == 0)
	    error("Error reading rasterfile data.\n");
	(*filter_func) (buf, rh.ras_width);
	if (fwrite((char *) buf, 1, n_read, stdout) != n_read)
	    error("Error writing rasterfile data.\n");
    }
    return 0;			       /* for those who check the exit code */
}


static void
rgbtobgr(buf, count)
    unsigned char  *buf;
    register int    count;
{
    register unsigned char *red,
                   *blue,
                    temp;

    /* swap the value of red and blue */
    red = buf + step_over - 1;
    blue = buf + step_over - 3;
    while (count--) {
	temp = *red;
	*red = *blue;
	*blue = temp;
	blue += step_over;
	red += step_over;
    }
}

static void
pass_thru()
{
    /* nop */
}

/* VARARGS */
static void
error(va_alist)
va_dcl
{
    va_list         ap;
    char           *fmt;

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

static
Usage()
{
    char           *basename;

    if (basename = rindex(Progname, '/'))
	basename++;
    else
	basename = Progname;
    (void) fprintf(stderr, "Usage: %s [infile [outfile]]\n", basename);
    exit(1);
}
