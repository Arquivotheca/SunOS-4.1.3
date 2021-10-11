/*	@(#)kernel.h 1.1 92/07/30 SMI; from UCB 4.8 83/05/30	*/

/*
 * Global variables for the kernel
 */

#ifndef _sys_kernel_h
#define _sys_kernel_h

u_long	rmalloc();

/* 1.1 */
char	hostname[MAXHOSTNAMELEN];
int	hostnamelen;
char	domainname[MAXHOSTNAMELEN];
int	domainnamelen;

/* 1.2 */
struct	timeval boottime;
struct	timeval time;
struct	timezone tz;			/* XXX */
int	hz;
int	phz;				/* alternate clock's frequency */
int	tick;				/* microseconds/tick; set in param.c */
int	lbolt;				/* awoken once a second */
int	realitexpire();

long	avenrun[3];			/* FSCALED average run queue lengths */

#ifdef GPROF
extern	int profiling;
extern	char *s_lowpc;
extern	u_long s_textsize;
extern	u_short *kcount;
#endif

#endif /*!_sys_kernel_h*/
