/*	@(#)debug.h 1.1 92/07/30 SMI; from S5R3 sys/debug.h 10.2	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _sys_debug_h
#define _sys_debug_h

#define	DEBUG

/*
 * When "lint" is being run, don't put the predicate in an "if",
 * so that you don't get complaints about "ASSERT(0);".
 */
#if defined(DEBUG)
# if defined(lint)
#define ASSERT(EX) assfail("EX", __FILE__, __LINE__)
# else
#define ASSERT(EX) if (EX);else assfail("EX", __FILE__, __LINE__)
# endif
#else
#define ASSERT(x)
#endif

#endif /*!_sys_debug_h*/
