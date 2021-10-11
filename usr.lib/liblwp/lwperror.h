/* @(#)lwperror.h 1.1 92/07/30 Copyr 1987 Sun Micro */
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
#ifndef _lwp_lwperror_h
#define _lwp_lwperror_h

typedef enum lwp_err_t {
	LE_NOERROR		=0,	/* No error */
	LE_NONEXIST		=1,	/* use of nonexistent object */
	LE_TIMEOUT		=2,	/* receive timed out */
	LE_INUSE		=3,	/* attempt to access object in use */
	LE_INVALIDARG		=4,	/* argument to primitive is invalid */
	LE_NOROOM		=5,	/* can't get room to create object */
	LE_NOTOWNED		=6,	/* object use without owning resource */
	LE_ILLPRIO		=7,	/* use of illegal priority */
	LE_REUSE		=8,	/* possible reuse of existing object */
	LE_NOWAIT		=9,	/* attempt to use barren object */
} lwp_err_t;

extern	char **lwp_errstr();	/* list of error messages */
extern	void lwp_perror();	/* print error message */

#ifndef _lwp_process_h
/* some defs a client may want */

extern	lwp_err_t lwp_geterr();	/* returns error for current lwp */
#endif /*!_lwp_process_h*/

#endif /*!_lwp_lwperror_h*/
