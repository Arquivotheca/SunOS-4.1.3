/* @(#) param.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
#ifndef __PARAM_H__
#define __PARAM_H__

/*
 * Tunable constants
 */

#define	MAXCTX		8	/* number of cached contexts */
#define	MAXCONTEXT	8	/* number of cached per-process contexts */
#define	CONDVAR_GUESS	2	/* initial allocation of cv_t's */
#define	CVPS_GUESS	CONDVAR_GUESS	/* initial allocation of cvps_t's */
#define	MON_GUESS	1	/* number of monitors initially in cache */
#define	MONPS_GUESS	MON_GUESS	/* initial allocation of monps_t's */
#define	MAXLWPS		2	/* number of lwp contexts to preallocate */
#define	MAXMEMTYPES	30	/* maximum number of memory caches */
#define	MAXINTMEMTYPES	5	/* maximum number of interrupt caches */
#define MAXARGS		10	/* max. # args to lwp_create */
#define	MAXPRIO		1000	/* max # priorities. Must be at least 256 */
#define	MAXOBJS		100	/* initial allocation of objects */

#endif __PARAM_H__
