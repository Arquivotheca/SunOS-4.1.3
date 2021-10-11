/*	@(#)tty.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef _sys_tty_h
#define _sys_tty_h

#ifndef _sys_ttycom_h
#include <sys/ttycom.h>
#endif

typedef struct tty_common {
	int	t_flags;
	queue_t	*t_readq;	/* stream's read queue */
	queue_t	*t_writeq;	/* stream's write queue */
	unsigned long t_iflag;	/* copy of iflag from tty modes */
	unsigned long t_cflag;	/* copy of cflag from tty modes */
	u_char	t_stopc;	/* copy of c_cc[VSTOP] from tty modes */
	u_char	t_startc;	/* copy of c_cc[VSTART] from tty modes */
	struct winsize t_size;	/* screen/page size */
	mblk_t	*t_iocpending;	/* ioctl reply pending successful allocation */
} tty_common_t;

#define	TS_XCLUDE	0x00000001	/* tty is open for exclusive use */
#define	TS_SOFTCAR	0x00000002	/* force carrier on */

extern void	ttycommon_close(/*tty_common_t *tc*/);
extern void	ttycommon_qfull(/*tty_common_t *tc, queue_t *q*/);
extern unsigned	ttycommon_ioctl(/*tty_common_t *tc, queue_t *q, mblk_t *mp*/);

/*
 * M_CTL message types.
 */
#define	MC_NOCANON	0	/* module below saying it will canonicalize */
#define	MC_DOCANON	1	/* module below saying it won't canonicalize */
#define	MC_CANONQUERY	2	/* module above asking whether module below canonicalizes */
#define	MC_SERVICEIMM	3	/* tell the ZS driver to return input immediately */
#define	MC_SERVICEDEF	4	/* tell the ZS driver it can wait */

#endif /*!_sys_tty_h*/
