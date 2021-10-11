/*	@(#)vlimit.h 1.1 92/07/30 SMI; from UCB 4.3 81/04/13	*/

/*
 * Limits for u.u_limit[i], per process, inherited.
 */

#ifndef _sys_vlimit_h
#define _sys_vlimit_h

#define	LIM_NORAISE	0	/* if <> 0, can't raise limits */
#define	LIM_CPU		1	/* max secs cpu time */
#define	LIM_FSIZE	2	/* max size of file created */
#define	LIM_DATA	3	/* max growth of data space */
#define	LIM_STACK	4	/* max growth of stack */
#define	LIM_CORE	5	/* max size of ``core'' file */
#define	LIM_MAXRSS	6	/* max desired data+stack core usage */

#define	NLIMITS		6

#define	INFINITY	0x7fffffff

#endif /*!_sys_vlimit_h*/
