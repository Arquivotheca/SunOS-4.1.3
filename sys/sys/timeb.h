/*	@(#)timeb.h 1.1 92/07/30 SMI; from UCB 4.2 81/02/19	*/

/*
 * Structure returned by ftime system call
 */

#ifndef _sys_timeb_h
#define _sys_timeb_h

struct timeb
{
	time_t	time;
	unsigned short millitm;
	short	timezone;
	short	dstflag;
};

#endif /*!_sys_timeb_h*/
