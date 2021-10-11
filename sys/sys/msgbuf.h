/*	@(#)msgbuf.h 1.1 92/07/30 SMI; from UCB 7.1 6/4/86	*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef _sys_msgbuf_h
#define _sys_msgbuf_h

#define	MSG_MAGIC	0x063062
struct	msgbuf {
	struct	msgbuf_hd {
		long	msgh_magic;
		long	msgh_size;
		long	msgh_bufx;
		long	msgh_bufr;
	} msg_hd;
#define msg_magic	msg_hd.msgh_magic
#define msg_size	msg_hd.msgh_size
#define msg_bufx	msg_hd.msgh_bufx
#define msg_bufr	msg_hd.msgh_bufr
#ifdef KERNEL
	char	msg_bufc[MSG_BSIZE];	/* see <machine/param.h> */
#else
	char	msg_bufc[1];		/* actually longer (msg_size) */
#endif KERNEL
};
#ifdef KERNEL
extern struct	msgbuf msgbuf;
#endif

#endif /*!_sys_msgbuf_h*/
