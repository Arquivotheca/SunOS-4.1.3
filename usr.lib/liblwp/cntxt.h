/* @(#)cntxt.h 1.1 92/07/30 Copyr 1987 Sun Micro */
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
/* context manipulation functions */

#ifndef _lwp_cntxt_h
#define _lwp_cntxt_h

typedef struct cntxt_actions_t {
	void (*cntxt_save)();		/* save context info */
	void (*cntxt_restore)();	/* restore context info */
	int cntxt_memtype;		/* memory type for the context */
	bool_t cntxt_optimize;		/* enable optimize on context switch */
} cntxt_actions_t;

/* per-thread context descriptor */
typedef struct context_t {
	__queue_t ctx_next;	/* chain of contexts belonging to thread */
	int ctx_type;		/* cookie bound to context memory */
	caddr_t ctx_mem;	/* context memory */
} context_t;
#define	CONTEXTNULL	((context_t *)0)

extern void __init_ctx();
extern void __freectxt();
extern void __ctx_change();
extern int CtxType;

#define	MAXSWTCHACTS	20

#endif /*!_lwp_cntxt_h*/
