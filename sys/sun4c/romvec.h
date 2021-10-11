/* @(#)romvec.h 1.1 92/07/30 SMI */

#ifndef	_sun4c_romvec_h
#define	_sun4c_romvec_h

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

/*
 * Sun4c specific romvec routines
 * The following code is dropped directly into the sunromvec struct,
 * thus its strange appearance
 */
void	(*v_setcxsegmap)();	/* Set segment in any context. */

#endif	_sun4c_romvec_h
