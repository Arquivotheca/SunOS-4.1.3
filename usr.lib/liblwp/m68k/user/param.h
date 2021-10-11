/* @(#)param.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
#ifndef _lwp_param_h
#define _lwp_param_h

/*
 * Tunable constants
 */

#define	NUMAGENTS	20	/* initial allocation of agents */
#define	NUMAGTMSGS	25	/* initial allocation of agent messages */
#define	MAXALARMS	30	/* number of preallocated alarms */
#define	MAXCTX		10	/* number of cached contexts */
#define	MAXCONTEXT	30	/* number of cached per-process contexts */
#define	CONDVAR_GUESS	10	/* initial allocation of cv_t's */
#define	CVPS_GUESS	CONDVAR_GUESS	/* initial allocation of cvps_t's */
#define	NUMHANDLERS	20	/* number of preallocated exception contexts */
#define	MON_GUESS	20	/* number of monitors initially in cache */
#define	MONPS_GUESS	MON_GUESS	/* initial allocation of monps_t's */
#define	MAXLWPS		64	/* number of lwp contexts to preallocate */
#define	MAXMEMTYPES	30	/* maximum number of memory caches */
#define	MAXINTMEMTYPES	5	/* maximum number of interrupt caches */
#define MAXARGS		512	/* max. # args to lwp_create */
#define	MAXPRIO		1000	/* max # priorities. Must be at least 256 */
#define	MAXOBJS		100	/* initial allocation of objects */
#define	MAXSLAVES	20	/* maximum number of slave threads */
#define NEXITHANDLERS	20	/* maximum number of exit handlers */
#define	NUMLASTRITES	10	/* initial number of lastrites events */

#endif /*!_lwp_param_h*/
