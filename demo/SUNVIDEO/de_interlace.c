
#ifndef lint
static char sccsid[] = "@(#)de_interlace.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>

#define RED(v)		((v) & 0xFF)
#define GREEN(v)	(((v)>>8) & 0xFF)
#define BLUE(v)		(((v)>>16) & 0xFF)


int interlace_method=2;

de_interlace(pr)
    struct pixrect *pr;
{
    switch (interlace_method) {
	case 0:{		       /* Fast and ugly */
		register int    y;

		for (y = 1; y < pr->pr_height; y += 2) {
		    pr_rop(pr, 0, y , pr->pr_width, 1, PIX_SRC, pr, 0, y-1);
		}
	    }
	    break;

	case 1:
	    {		       /* Slow, but a good job. */
		register int x, y, i;
		register unsigned char rmin, rmax, gmin, gmax, bmin, bmax;
		int v, linewords;
		register unsigned int *up, *lp;
		linewords = (mpr_d(pr)->md_linebytes) / 4;
		/* fix up casting to work correctly when regions are on */
		up = (unsigned int *)mpr_d(pr)->md_image +
		     mpr_d(pr)->md_offset.x +
		     linewords * mpr_d(pr)->md_offset.y;
		lp = up + 2*linewords;
		for (y = 1; y < pr->pr_height - 1; y += 2) {
		    for (x = 1; x < pr->pr_width - 1; x++) {
			bmin = gmin = rmin = 255;
			bmax = gmax = rmax = 0;
			for (i = 0; i < 2; i++) {
			    if (RED(up[x+i]) < rmin) {
				rmin = RED(up[x+i]);
			    }
			    if (GREEN(up[x+i]) < gmin) {
				gmin = GREEN(up[x+i]);
			    }
			    if (BLUE(up[x+i]) < bmin) {
				bmin = BLUE(up[x+i]);
			    }
			    if (RED(up[x+i]) > rmax) {
				rmax = RED(up[x+i]);
			    }
			    if (GREEN(up[x+i]) > gmax) {
				gmax = GREEN(up[x+i]);
			    }
			    if (BLUE(up[x+i]) > bmax) {
				bmax = BLUE(up[x+i]);
			    }
			    if (RED(lp[x+i]) < rmin) {
				rmin = RED(lp[x+i]);
			    }
			    if (GREEN(lp[x+i]) < gmin) {
				gmin = GREEN(lp[x+i]);
			    }
			    if (BLUE(lp[x+i]) < bmin) {
				bmin = BLUE(lp[x+i]);
			    }
			    if (RED(lp[x+i]) > rmax) {
				rmax = RED(lp[x+i]);
			    }
			    if (GREEN(lp[x+i]) > gmax) {
				gmax = GREEN(lp[x+i]);
			    }
			    if (BLUE(lp[x+i]) > bmax) {
				bmax = BLUE(lp[x+i]);
			    }
			}
			v = pr_get(pr, x, y);	/* Current value */
			if ((RED(v) < rmin) || (GREEN(v) < gmin) ||
			    (BLUE(v) < bmin) || (RED(v) > rmax) ||
			    (GREEN(v) > gmax) || (BLUE(v) > bmax)) {
			    /* value is out of range. replace it */
			    pr_put(pr, x, y,
				 ((RED(up[x]) + RED(lp[x])) >> 1) |
				 (((GREEN(up[x]) + GREEN(lp[x])) >> 1) << 8) | 
				 (((BLUE(up[x]) + BLUE(lp[x])) >> 1) << 16));

			}
		    }
		    up = lp;
		    lp += linewords*2;
		}
	    }
	    break;
	default:
	    break;
    }
}
