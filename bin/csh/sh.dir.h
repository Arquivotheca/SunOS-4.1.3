/*	@(#)sh.dir.h 1.1 92/07/30 SMI; from UCB 5.2 6/6/85	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Structure for entries in directory stack.
 */
struct	directory	{
	struct	directory *di_next;	/* next in loop */
	struct	directory *di_prev;	/* prev in loop */
	unsigned short *di_count;	/* refcount of processes */
 tchar *di_name;		/* actual name */
};
struct directory *dcwd;		/* the one we are in now */
