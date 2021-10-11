/*	@(#)dk.h 1.1 90/03/23 SMI; from UCB 4.2 81/02/19	*/

/*
 * Instrumentation
 */

#ifndef _sys_dk_h
#define _sys_dk_h

#define	CPUSTATES	4

#define	CP_USER		0
#define	CP_NICE		1
#define	CP_SYS		2
#define	CP_IDLE		3

#ifdef	MULTIPROCESSOR
/*
 * Extended Processor Status Detail
 */
#define	XPSTATES	8	/* lots of room for expansion */
#define	XP_SPIN		0	/* blocked on kernel lock */
#define	XP_DISK		1	/* idle with disk active */
#define	XP_SERV		2	/* in crosscall service loop */
#endif	MULTIPROCESSOR

#define	DK_NDRIVE	32

#ifdef KERNEL

long	cp_time[CPUSTATES];		/* holds average of all cpus */

#ifdef	MULTIPROCESSOR
#include <machine/param.h>		/* to get NCPU */
long	mp_time[NCPU][CPUSTATES];	/* holds actual ticks for all cpus */
long	xp_time[NCPU][XPSTATES];	/* extended cpu states */
#endif	MULTIPROCESSOR

int	dk_busy;
long	dk_time[DK_NDRIVE];
long	dk_seek[DK_NDRIVE];
long	dk_xfer[DK_NDRIVE];
long	dk_wds[DK_NDRIVE];
long	dk_bps[DK_NDRIVE];
long	dk_read[DK_NDRIVE];

long	tk_nin;
long	tk_nout;
#endif

#endif /*!_sys_dk_h*/
