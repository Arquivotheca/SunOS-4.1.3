/*	@(#)ttydev.h 1.1 92/07/30 SMI; from UCB 4.3 83/05/18	*/

/*
 * Terminal definitions related to underlying hardware.
 */

#ifndef	__sys_ttydev_h
#define	__sys_ttydev_h

/*
 * Speeds
 */
#define	B0	0
#define	B50	1
#define	B75	2
#define	B110	3
#define	B134	4
#define	B150	5
#define	B200	6
#define	B300	7
#define	B600	8
#define	B1200	9
#define	B1800	10
#define	B2400	11
#define	B4800	12
#define	B9600	13
#define	B19200	14
#define	B38400	15
#ifndef	_POSIX_SOURCE
#define	EXTA	14
#define	EXTB	15
#endif

#ifdef	KERNEL
/*
 * Hardware bits.
 * SHOULD NOT BE HERE.
 */
#define	DONE	0200
#define	IENABLE	0100

/*
 * Modem control commands.
 */
#define	DMSET		0
#define	DMBIS		1
#define	DMBIC		2
#define	DMGET		3
#endif	/* KERNEL */

#endif	/* !__sys_ttydev_h */
