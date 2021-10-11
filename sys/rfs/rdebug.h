/*	@(#)rdebug.h 1.1 92/07/30 SMI	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:sys/rdebug.h	10.7" */

#ifndef _rfs_rdebug_h
#define _rfs_rdebug_h

/* Debugging flags turned on and off by rdebug */
#define DB_SYSCALL	0x001	/* remote system calls */
#define DB_SD_RD	0x002	/* send and receive descriptors */
#define DB_SERVE	0x004	/* servers and server-control */
#define DB_MNT_ADV	0x008	/* advertising and remote mounts */
#define DB_RFSTART	0x010	/* rfstart, rfstop */
#define DB_RECOVER	0x020	/* recovery */
#define DB_RDUSER	0x040	/* rd_user structures */
#define DB_CACHE	0x080	/* client cache debugging */
#define DB_LOOPBCK	0x100	/* allow machine to mount own resources */
#define DB_COMM		0x200	/* communication interface */
#define DB_GDP		0x400	/* GDP */
#define DB_PACK		0x800	/* PACK stream interface */
#define DB_RMOVE	0x1000	/* rmove */
#define DB_RSC		0x2000	/* RSC ACK and NACK */
#define DB_RFSYS	0x4000	/* miscellaneous sys call */
#define DB_SIGNAL       0x8000  /* signal */
#define DB_GDPERR       0x10000  /* GDP temporary error */
#define NO_RETRANS      0x20000  /* turn off retransmission */
#define NO_RECOVER      0x40000  /* turn off recovery */
#define NO_MONITOR      0x80000  /* turn off monitor */
#define DB_CANON     	0x100000  /* canonical form conversion */
#define	DB_FSS		0x200000  /* dufst debugging */
#define TP_CONN		0x400000  /* transport layer connection attempts */
#define NP_SLOG		0x800000  /* npack stream log */
#define NP_ELOG		0x1000000  /* npack error log */
#define NP_STLOG	0x2000000  /* npack state log */
#define DB_VNODE	0x4000000  /* rfs vnodeops */
#define DB_SYNC		0x8000000  /* rfs sync call */
#define DB_RFSNODE	0x10000000  /* rfsnode manipulations */
#define DB_NOPRINT	0x80000000  /* don't print (but still goes to msgbuf) */


extern long dudebug;
extern int  noprintf;

#define DUDEBUG

#ifdef DUDEBUG
#define	DUPRINT1(x,y1) rfs_printf(x,1,y1);
#define	DUPRINT2(x,y1,y2) rfs_printf(x,2,y1,y2);
#define	DUPRINT3(x,y1,y2,y3) rfs_printf(x,3,y1,y2,y3);
#define	DUPRINT4(x,y1,y2,y3,y4) rfs_printf(x,4,y1,y2,y3,y4);
#define	DUPRINT5(x,y1,y2,y3,y4,y5) rfs_printf(x,5,y1,y2,y3,y4,y5);
#define DUPRINT6(x,y1,y2,y3,y4,y5,y6) rfs_printf(x,6,y1,y2,y3,y4,y5,y6);
#else
#define	DUPRINT1(x,y1)
#define	DUPRINT2(x,y1,y2)
#define	DUPRINT3(x,y1,y2,y3)
#define	DUPRINT4(x,y1,y2,y3,y4)
#define	DUPRINT5(x,y1,y2,y3,y4,y5)
#define	DUPRINT6(x,y1,y2,y3,y4,y5,y6)
#endif

#endif /*!_rfs_rdebug_h*/
