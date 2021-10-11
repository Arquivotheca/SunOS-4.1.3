/*	@(#)strstat.h 1.1 92/07/30 SMI; from S5R3 sys/strstat.h 10.2	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Streams Statistics header file.  This file defines
 * the counters that are maintained for statistics gathering
 * under Streams. 
 */

#ifndef _sys_strstat_h
#define _sys_strstat_h

typedef struct {
	int use;	/* current item usage count */
	int total;	/* total item usage count */
	int max;	/* maximum item usage count */
	int fail;	/* count of allocation failures */
} alcdat;

struct  strstat {
	alcdat stream;		/* stream allocation data */
	alcdat queue;		/* queue allocation data */
	alcdat mblock;		/* message block allocation data */
	alcdat dblock;		/* aggregate data block allocation data */
	alcdat linkblk;		/* linkblk allocation data */
	alcdat strevent;	/* strevent allocation data */
};


/* in the following macro, x is assumed to be of type alcdat */

#define BUMPUP(X)	{(X).use++;  (X).total++;\
			 (X).max=((X).use>(X).max?(X).use:(X).max); }


/* per-module statistics structure */

struct module_stat {
	long ms_pcnt;		/* count of calls to put proc */
	long ms_scnt;		/* count of calls to service proc */
	long ms_ocnt;		/* count of calls to open proc */
	long ms_ccnt;		/* count of calls to close proc */
	long ms_acnt;		/* count of calls to admin proc */
	char *ms_xptr;		/* pointer to private statistics */
	short ms_xsize;		/* length of private statistics buffer */
};

#endif /*!_sys_strstat_h*/
