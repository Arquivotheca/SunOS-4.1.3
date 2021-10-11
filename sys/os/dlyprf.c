#ifndef lint
static char    *sccsid = "@(#)dlyprf.c 1.1 92/07/30 SMI";
#endif
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <machine/clock.h>
#include <machine/cpu.h>
#ifdef	MULTIPROCESSOR
#include <os/atom.h>
#endif	MULTIPROCESSOR
#include <os/dlyprf.h>

/*
 * dlyprf: "delay printf" package. stashes away a set of events to be printed
 * later; each event is in the form of a precise time,
 * a printf-style format string, and up to four integer parameters.
 */
#ifndef DPSIZE
#define	DPSIZE	1024
#endif

#ifdef	MULTIPROCESSOR
atom_t		dpatom;
static int	dp_first = 1;
#endif	MULTIPROCESSOR

struct dpr     *dpring = 0;	/* [DPSIZE] */ 

int             dpput = 0;
int             dpenable = 1;
int             dpsize = DPSIZE;
int             dpcap = 0;

void					/* VARARGS */
dlyprf(fmt, a, b, c, d)
	char           *fmt;
	int             a, b, c, d;
{
	int             dpi;
	struct dpr     *dp;

	if (dpenable && dpring) {
#ifdef	MULTIPROCESSOR
		if (dp_first) {
			dp_first = 0;
			atom_init(dpatom);
		}
		atom_wcs(dpatom);
#endif	MULTIPROCESSOR
		dpi = dpput++;
		dp = &(dpring[dpi % dpsize]);
		dp->fmt = fmt;
		uniqtime(dp->stamp);
		dp->index = dpput-1;
		dp->a = a;
		dp->b = b;
		dp->c = c;
		dp->d = d;
		if (dpcap && (dpput >= dpcap))
			dpenable = 0;
#ifdef	MULTIPROCESSOR
		atom_clr(dpatom);
#endif	MULTIPROCESSOR
	}
}
