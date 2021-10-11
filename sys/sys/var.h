/*	@(#)var.h 1.1 92/07/30 SMI; from S5R3 sys/var.h 10.2	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _sys_var_h
#define _sys_var_h

/*
 * Streams Configuration Information
 *
 * This structure is degenerate and in the process of being
 * phased out.
 */
struct var {
	int	v_nstream;	/* Number of stream head structures.	*/
};

#ifdef	KERNEL

extern struct var v;
extern u_short	ndblks[];

#endif	KERNEL

#endif /*!_sys_var_h*/
