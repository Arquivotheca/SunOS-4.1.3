/* @(#)except.h 1.1 92/07/30 */
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
#ifndef _lwp_except_h
#define _lwp_except_h

/* exception context */
typedef struct exccontext_t {
	struct exccontext_t *exc_next;	/* stack of exception handlers */
	int exc_sp;		/* stack at time of exc_handle */
	int exc_pc;		/* return addr from exc_handle() call */
	int exc_return;		/* normal return addr of handling procedure */
	int *exc_retaddr;	/* location of exc_return (for patching) */
	int exc_refcnt;		/* per-procedure count of handlers set */
	int exc_pattern;	/* pattern bound by exc_handle() */
	caddr_t (*exc_func)();	/* function bound by exc_handle() */
	caddr_t exc_env;	/* argument to the function */
} exccontext_t;
#define	EXCNULL	((exccontext_t *) 0)

/*
 * The CATCHALL pattern will match ANY pattern. If you allocate
 * some resource and must reclaim it despite possible
 * exceptions, use a CATCHALL handler to catch any exceptions.
 * 0 and -1 are not usable as patterns by exc_handle.
 * exc_handle returns:
 *	0 if the handler was established correctly
 *	-1 if there was an error in establishing the handler
 *	else pattern from excraise
 */
#define	EXITPATTERN	0		/* indicates an exit handler */

extern int __exccleanup();
extern void __exceptcleanup(/* corpse */);
extern void __init_exc();

#endif /*!_lwp_except_h*/
