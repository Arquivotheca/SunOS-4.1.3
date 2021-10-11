/*	fontsample.c	1.2	84/03/12	*/

/*
 * sample fontfile ...
 *
 * Like the SAIL font catalog.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <suntool/gfxsw.h>
#include <stdio.h>

#define LEFTMAR 16
#define DEFAULT_FONT_DIR "/usr/demo/fonts/"

struct gfxsubwindow *gfx;
char *sampletext[] = {
"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
"abcdefghijklmnopqrstuvwxyz",
"0123456789",
"!\"#$%&'()*+,-./ :;<=>?@ [\]^_` {|}~\177\
\200\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\
\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037",
"",
"THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG.",
"The quick brown fox jumps over the lazy dog.",
"",
"	In Xanadu did Kublai Kahn",
"	A stately pleasure dome decree.",
"",
"#include <sys/types.h>",
"	for (c = cpr, d = disp; d-disp < 256; d++, c++) {",
"		if (d->nbytes) {",
"			int width = d->left+d->right, height = maxup + d->down;",
"			int w = (width+7)>>3, ww = ((width+15)>>3)&~1;",
"		}",
"	}",
0
};

PIXFONT	*pfstd;
int Xmax, Ymax;

main(argc, argv)
	int argc;
	char **argv;
{
	register struct pixrect *pr;
	register int i;

	argc--, argv++;
	gfx = gfxsw_init(0,argv,0);
	/* only set up pixrect for actual window;  not clear what to do
	 * rest of stuff */
	Xmax = gfx->gfx_rect.r_width;
	Ymax = gfx->gfx_rect.r_height;
	if (argc == 0)
		argc++;
	pr = (struct pixrect *)mem_create(Xmax, Ymax, 1);
	if (pr == NULL) {
		fprintf(stderr, "Can not allocate memory pixrect \n");
		exit(1);
	}

	pfstd = pf_open(NULL);
	for (i = 0; i < argc; i++)
		if (printsample(argv[i], pr) < 0)
			fprintf(stderr, "%s: can't sample %s\n", 
				argv[0],
				argv[i]);
}

printsample(fontname, pr)
	char *fontname;
	register struct pixrect *pr;
{
	register PIXFONT *pf;
	register int i, nlines;

	pf = pf_open(fontname);
	if (pf == 0)
		return (-1);
	nlines =
	    (770 - pfstd->pf_defaultsize.y) / pf->pf_defaultsize.y;
	pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y, PIX_SRC, 0, 0, 0);
	pf_text(pr, LEFTMAR, pfstd->pf_defaultsize.y,
	    PIX_SRC|PIX_DST, pfstd, fontname ? fontname : "default");
	for (i = 0; sampletext[i] && i < nlines; i++) {
		pf_text(pr, LEFTMAR, (i + 2) * pf->pf_defaultsize.y,
		    PIX_SRC|PIX_DST, pf, sampletext[i]);
	}
	pw_lock(gfx->gfx_pixwin,  &gfx->gfx_rect);
	pw_write(gfx->gfx_pixwin, 0, 0, Xmax, Ymax, PIX_SRC, pr, 0, 0);
	pw_unlock(gfx->gfx_pixwin);
	pf_close(pf);
}
