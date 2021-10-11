/*	@(#)ptyvar.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Pseudo-tty driver data structures.
 */

#ifndef	_sys_ptyvar_h
#define	_sys_ptyvar_h

struct pty {
	long	pt_flags;		/* flag bits */
	mblk_t	*pt_stuffqfirst;	/* head of queue for ioctls */
	mblk_t	*pt_stuffqlast;		/* tail of queue for ioctls */
	int	pt_stuffqlen;		/* number of bytes of queued ioctls */
	tty_common_t pt_ttycommon;	/* data common to all tty drivers */
	int	pt_wbufcid;		/* id of pending write-side bufcall */
	struct proc *pt_selr;		/* proc selecting on controller read */
	struct proc *pt_selw;		/* proc selecting on controller write */
	struct proc *pt_sele;		/* proc selecting on exception */
	struct stdata *pt_stream;	/* XXX stream for the slave */
	short	pt_pgrp;		/* controller side process group */
	u_char	pt_send;		/* pending message to controller */
	u_char	pt_ucntl;		/* pending iocontrol for controller */
};

#define	PF_RCOLL	0x00000001	/* > 1 process selecting for read */
#define	PF_WCOLL	0x00000002	/* > 1 process selecting for write */
#define	PF_ECOLL	0x00000004	/* > 1 process selecting for excep. */
#define	PF_NBIO		0x00000008	/* non-blocking I/O on controller */
#define	PF_ASYNC	0x00000010	/* asynchronous I/O on controller */
#define	PF_WOPEN	0x00000020	/* waiting for open to complete */
#define	PF_CARR_ON	0x00000040	/* "carrier" is on (controller is open) */
#define	PF_SLAVEGONE	0x00000080	/* slave was open, but is now closed */
#define	PF_PKT		0x00000100	/* packet mode */
#define	PF_STOPPED	0x00000200	/* user told stopped */
#define	PF_REMOTE	0x00000400	/* remote and flow controlled input */
#define	PF_NOSTOP	0x00000800	/* slave is doing XON/XOFF */
#define	PF_UCNTL	0x00001000	/* user control mode */
#define	PF_43UCNTL	0x00002000	/* real 4.3 user control mode */

#endif	/* !_sys_ptyvar_h */
