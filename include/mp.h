/*	@(#)mp.h 1.1 92/07/30 SMI; from UCB 5.1 5/30/85	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 */

#ifndef _mp_h
#define _mp_h

struct mint {
	int len;
	short *val;
};
typedef struct mint MINT;

extern MINT *itom();
extern MINT *xtom();
extern char *mtox();
extern short *xalloc();
extern void mfree();

#define	FREE(x)	xfree(&(x))		/* Compatibility */

#endif /*!_mp_h*/
