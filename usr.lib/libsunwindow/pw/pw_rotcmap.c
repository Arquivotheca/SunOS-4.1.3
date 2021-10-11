#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)pw_rotcmap.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 * rotate a color map segment.
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>

pw_cyclecolormap(pw, cycles, begin, length)
	struct	pixwin *pw;
	int	cycles, begin;
	register int length;
{
	u_char	red[256], green[256], blue[256]; 
	register u_char	r, g, b; 
	register int	i;
	int	cycle, shift, planes;
	struct	colormapseg cms;

	(void)pw_getcmsdata(pw, &cms, &planes);
	if (cms.cms_size < 2)
		return;
	(void)pw_getcolormap(pw, begin, length, red, green, blue);
	for (cycle = 0; cycle < cycles; ++cycle) {
		for (shift = begin; shift < length-1; ++shift) {
			r = red[begin]; g = green[begin]; b = blue[begin];
			for (i = begin; i < length-1; ++i) {
				red[i] = red[i+1];
				green[i] = green[i+1];
				blue[i] = blue[i+1];
			}
			red[i] = r; green[i] = g; blue[i] = b;
			(void)pw_putcolormap(pw, begin, length, red, green, blue);
		}
	}
}

