/*	@(#)callout.h 1.1 92/07/30 SMI; from UCB 4.6 81/04/18	*/

/*
 * The callout structure is for a routine arranging to be called
 * by the clock interrupt (clock.c) with a specified argument,
 * in a specified amount of time.
 * Used, for example, to time tab delays on typewriters.
 */

#ifndef _sys_callout_h
#define _sys_callout_h

struct	callout {
	int	c_time;		/* incremental time */
	caddr_t	c_arg;		/* argument to routine */
	int	(*c_func)();	/* routine */
	struct	callout *c_next;
};
#ifdef KERNEL
struct	callout *callfree, *callout, calltodo;
int	ncallout;
#endif

#endif /*!_sys_callout_h*/
