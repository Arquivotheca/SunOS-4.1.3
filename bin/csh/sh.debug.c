
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)sh.debug.c 1.1 92/07/30 SMI; from UCB 5.4 3/29/86";
#endif


#include "sh.h"
#include <sgtty.h>


#ifdef TRACE
#include <stdio.h>
FILE *trace;
/*
 * Trace routines
 */
#define TRACEFILE "/tmp/trace.XXXXXX"

/*
 * Initialie trace file.
 * Called from main.
 */
trace_init()
{
	extern char *mktemp();
	char name[128];
	char *p;

	strcpy(name, TRACEFILE);
	p = mktemp(name);
	trace = fopen(p, "w");
}

/*
 * write message to trace file
 */
/*VARARGS1*/
tprintf(fmt,a,b,c,d,e,f,g,h,i,j)
     char *fmt;
{
	if (trace) {
		fprintf(trace, fmt, a,b,c,d,e,f,g,h,i,j);
		fflush(trace);
	}
}
#endif
