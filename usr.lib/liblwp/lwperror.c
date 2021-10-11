/* Copyright (C) 1986 Sun Microsystems Inc. */
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
#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/asynch.h>
#include <machlwp/machsig.h>
#include <machlwp/machdep.h>
#include <lwp/cntxt.h>
#include <lwp/lwperror.h>
#include <lwp/message.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/monitor.h>
#ifndef lint
SCCSID(@(#) lwperror.c 1.1 92/07/30 Copyr 1986 Sun Micro);
#endif lint

/*
 * PRIMITIVES contained herein:
 * lwp_perror(s)
 * lwp_errstr()
 */

/* message list for nugget errors reported to client */
STATIC char *lwp_errlist[] = {	/* indexed by lwp_geterr() values */
	"No Error",					/* 0 */
	"use of nonexistent object",			/* 1 */
	"receive timed out",				/* 2 */
	"attempt to destroy object in use",		/* 3 */
	"argument to primitive is invalid",		/* 4 */
	"can't get room to create object",		/* 5 */
	"object use without owning resource",		/* 6 */
	"use of illegal priority",			/* 7 */
	"possible reuse of existing object",		/* 8 */
	"attempt to use barren object",			/* 9 */
	0,
};
STATIC int lwp_nerr = { sizeof lwp_errlist/sizeof lwp_errlist[0] };

#include <stdio.h>

/*
 * lwp_errstr() -- PRIMITIVE.
 * return pointer to array of error messages.
 */
char **
lwp_errstr()
{
	return (lwp_errlist);
}

/*
 * lwp_perror() -- PRIMITIVE.
 * print the error message associated with an errant lwp primitive.
 */
void
lwp_perror(s)
	char *s;
{
	register int err = (int)lwp_geterr();

	LOCK();
	(void) fflush(stderr);
	if (err >= lwp_nerr) {
		(void) fprintf(stderr, "Unknown error\n");
	} else {
		(void) fprintf(stderr, "%s: %s\n", s, lwp_errlist[err]);
	}
	UNLOCK();
}
