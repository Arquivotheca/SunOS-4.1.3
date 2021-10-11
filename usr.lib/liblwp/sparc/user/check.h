/* @(#)check.h 1.1 92/07/30 */
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

/*
 * Compare sp to its lowest legal point.
 * If the stack exceeds its bounds, set a value and return.
 * This macro can be used in procedures used by a lwp to check the
 * integrity of the stack (protect against bumping into another lwp's stack).
 * The overhead (STKOVERHEAD) is silently allocated IN ADDITION TO
 * the amount specfied in lwp_create.
 * This ensures that even if CHECK just barely
 * succeeds, there is room to call the next procedure in most cases
 * without trashing anyone else's stack.
 * This only fails when you're up against the limit and
 * the next procedure has lots of arguments.
 * If you never call a procedure with more than 100 bytes of arguments or so,
 * there should be no problem.
 * Roughly, what we have here:
 *	register unsigned sp;
 *	if (sp <= (unsigned)((int)__CurStack + STKOVERHEAD)) {
 *		location = result;
 *		return;
 *	}
 * This macro is only useful when no mmgt-assisted red-zones are available.
 */

#ifndef _lwp_sparc_chk_h
#define _lwp_sparc_chk_h

#define CHECK(location, result) {			\
	if (!__stacksafe()) {				\
		location = result;			\
		return;					\
}

#endif /*!_lwp_sparc_chk_h*/
