/* @(#) stackdep.h 1.1 92/07/30 */
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
 * sparc requires stack be aligned on 8 byte boundary.
 * usage:
 *	stkalign_t foo[SIZE];
 * or
 *	stkalign_t *foo;
 *	foo = (stkalign_t *)malloc(sizeof(stkalign_t) * SIZE);
 */

#ifndef _lwp_sparc_stackdep_h
#define _lwp_sparc_stackdep_h

typedef	double		stkalign_t;		/* alignment for stack */

/* align adr to legal stack boundary */
#define	STACKALIGN(adr)	(((int)(adr)) & ~(sizeof (stkalign_t) - 1))

#define	MINSIGSTKSZ	8000	/* minimum size of signal stack in stkalign_t's */

/* for finding top of stack of stkalign_t's */
#define	STACKTOP(base, size) ((base) + (size))

#define	STACKDOWN	TRUE	/* TRUE if stacks grow down */

#define	PUSH(sp, obj, type)	*--((type *)(sp)) = (obj);
#define	POP(sp, obj, type)	(obj) = *((type *)(sp))++;

#endif /*!_lwp_sparc_stackdep_h*/
