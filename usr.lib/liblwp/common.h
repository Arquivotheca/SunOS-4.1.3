/* @(#)common.h 1.1 92/07/30 Copyr 1987 Sun Micro */
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
#ifndef _lwp_common_h
#define _lwp_common_h

/*
 * Common definitions for lightweight process library.
 * The following conventions hold:
 * 1) all primitives begin with a decsriptive prefix, e.g., lwp_create.
 * 2) all internally used globals (functions and variables) that
 *	are shared between modules begin with __ (thus in assembly, ___)
 * 3) all globals private to a module are STATIC
 * 4) all global variables (whether private or not) have a capital letter
 *	as the first non-_ character (unless exported to the client).
 * 5) all macros are capitalized.
 * 6) all typedefs are _t
 */

#include <sys/types.h>

#ifndef MAX
#define	MAX(x,y)	(((x) > (y)) ? x : y)
#define	MIN(x,y)	(((x) < (y)) ? x : y)
#endif !MAX

#define	TRUE		1
#define	FALSE		0
#define SCCSID(arg)	static char Sccsid[]="arg"

typedef	char		bool_t;		/* boolean */

/* typed nulls */
#define	CHARNULL	((caddr_t) 0)
#define	CPTRNULL	((caddr_t *)0)
#define	INTNULL		((int *) 0)
#define	IFUNCNULL	((int (*)()) 0)
#define	CFUNCNULL	((caddr_t (*)()) 0)
#define	VFUNCNULL	((void (*)()) 0)

#define	UNSIGN(x)	((x) & 0xff)	/* avoid sign extension of a char */

/*
 * Generate lock.
 * We ignore wraparound for now, but protect low numbers that belong to
 * special threads
 */
extern int __UniqId;
#define	INVALIDLOCK	0
#define	UNIQUEID()	(++__UniqId)	/* should panic if wraparound */

/* to hide symbols if not debugging */
#ifdef	LDEBUG
#define	STATIC
#else	LDEBUG
#define	STATIC	static
#endif	LDEBUG

/* for copying structures (portably) */
#define	STRUCTASSIGN(s1, s2) bcopy ((caddr_t)s1, (caddr_t)s2, (int)sizeof (*s1))

/*
 * Initialization. If using shared libraries, it is more efficient to
 * bind in initialization initially
 */
#ifndef SHAREDLIB
extern int __LwpLibInit;
extern void __lwp_init();
#define	LWPINIT() {				\
	if (__LwpLibInit == 0) {		\
		__LwpLibInit++;			\
		__lwp_init();			\
	}					\
}
#else !SHAREDLIB
#define	LWPINIT()
#endif SHAREDLIB

#ifdef LDEBUG
#ifndef stderr
#include <stdio.h>
#endif stderr
#define ASSERT(ex) {if (!(ex)) {				\
	DISABLE();						\
	(void) fprintf						\
	  (stderr,"Assertion failed: file %s, line %d\n",	\
	  __FILE__, __LINE__);					\
	ENABLE();						\
	abort();						\
	}							\
}
#else LDEBUG
#define ASSERT(ex) ;
#endif LDEBUG

/*
 * common case for errors: __Curproc is being set and LOCKED
 */
#ifndef ERROR
#define	ERROR(cond, errtype) {						\
	if ((cond)) {							\
		SET_ERROR(__Curproc, (errtype));			\
		UNLOCK();						\
		return (-1);						\
	}								\
}
#endif !ERROR

/* as above, but no condition */
#define	TERROR(errtype) {						\
	SET_ERROR(__Curproc, (errtype));				\
	UNLOCK();							\
	return (-1);							\
}

/*
 * Indicate that the optimiser should not delay assignment to this variable.
 * Not currently supported.
 */
#define	volatile

#include <lwp/trace.h>
#include <lwp/lwp.h>

#if defined(mc68000)
#include <lwp/m68k/user/param.h>
#endif

#if defined(sparc)
#include <lwp/sparc/user/param.h>
#endif


#include <sys/varargs.h>
extern void __panic(/* msg */);

#endif /*!_lwp_common_h*/
