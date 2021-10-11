/* @(#)common.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

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

#ifdef LDEBUG
#define ASSERT(ex) {if (!(ex)) {				\
	printf("Assertion failed: file %s, line %d\n",		\
		__FILE__, __LINE__);				\
	__panic("");						\
	}							\
}
#endif LDEBUG

#define	LWPINIT()
/*
 * common case for errors: __Curproc is being set and LOCKED
 */
#ifndef ERROR
#define	ERROR(cond, errtype) {						\
	if ((cond)) {							\
		SET_ERROR(__Curproc, (errtype));			\
		return (-1);						\
	}								\
}
#endif !ERROR

/* as above, but no condition */
#define	TERROR(errtype) {						\
	SET_ERROR(__Curproc, (errtype));				\
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
#include <lwp/m68k/param.h>
#endif

#if defined(sparc)
#include <lwp/sparc/param.h>
#endif


#include <sys/varargs.h>
extern void __panic(/* msg */);

#endif /*!_lwp_common_h*/
