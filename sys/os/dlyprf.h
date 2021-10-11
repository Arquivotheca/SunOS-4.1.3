/*	@(#)dlyprf.h 1.1 92/07/30 SMI */
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef	__DLYPRF_H__
#define	__DLYPRF_H__

#ifdef	DLYPRF

#include <sys/time.h>

struct dpr {
	char           *fmt;
	struct timeval	stamp[1];
	int             index;
	int             a, b, c, d;
};

extern void     dlyprf();

extern int      dpput;
extern int      dpenable;
extern int      dpsize;
extern struct dpr *dpring;

#endif	/* DLYPRF */

#endif	/* __DLYPRF_H__ */
