/* Copyright (C) 1986 Sun Microsystems Inc. */

#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/cntxt.h>
#include <lwp/mch.h>
#include <lwp/lwperror.h>
#include <lwp/process.h>
#include <lwp/schedule.h>
#include <lwp/monitor.h>
#ifndef lint
SCCSID(@(#) lwperror.c 1.1 92/07/30 Copyr 1987 Sun Micro);
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
	int pri;

	pri = splclock();
	if (err >= lwp_nerr) {
		(void) printf("Unknown error\n");
	} else {
		(void) printf("%s: %s\n", s, lwp_errlist[err]);
	}
	(void) splx(pri);
}
