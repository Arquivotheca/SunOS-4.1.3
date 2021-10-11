/* @(#) trace.h 1.1 92/07/30 Copyr 1987 Sun Micro */
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
#ifndef _lwp_trace_h
#define _lwp_trace_h

/* ex. usage: LWPTRACE(NUGGET, 3, ("message")); */
#ifdef LDEBUG
#define LWPTRACE(flag, lev, mesg) 				\
	if ((__Trace[(int)flag] >= lev)) {			\
		DISABLE(); 					\
		(void) printf mesg;				\
		ENABLE(); 					\
	}
#else
#define LWPTRACE(flag, lev, mesg)
#endif LDEBUG

/* debugging flags for the LWPTRACE macro */
typedef enum flag_type_t {
	tr_ABORT	=0,
	tr_EXCEPT	=1,
	tr_USER		=2,
	tr_SCHEDULE	=3,
	tr_PROCESS	=4,
	tr_ASM		=5,
	tr_ASYNCH	=6,
	tr_MESSAGE	=7,
	tr_SELECT	=8,
	tr_AGENT	=9,
	tr_CLOCK	=10,
	tr_CONDVAR	=11,
	tr_MONITOR	=12,
	tr_ALLOC	=13,
} flag_type_t;
#define MAXFLAGS	14		/* size of __Trace vector */

extern int __Trace[];

#endif /*!_lwp_trace_h*/
