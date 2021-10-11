
#ifndef lint
static char sccsid[] = "@(#)video_cbars.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

#include <sys/types.h>
#include <pixrect/pixrect.h>

static int colors []= {
    0xFFFFFF,	    /* White */
    0x00FFFF,
    0xFFFF00,
    0x00FF00,	    /* Green */
    0xFF00FF,
    0x0000FF,	    /* Red */
    0xFF0000,	    /* Blue */
};

make_cbars(pr)
    struct pixrect *pr;
{
    int bar_width, i, x;
    if (pr->pr_depth < 24) {
	printf("make_cbars: cannot make less that 24 bit color.\n");
    } else {
	x = 0;
	bar_width = pr->pr_width/7;
	for (i=0; i< 7; i++) {
	    pr_rop(pr, x, 0, bar_width, pr->pr_height, PIX_SRC|PIX_COLOR(colors[i]),
		   (struct pixrect *)0, 0, 0);
	    x += bar_width;
	}
    }
}
