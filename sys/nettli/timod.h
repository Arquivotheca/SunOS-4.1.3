/*	@(#)timod.h 1.1 92/07/30 SMI; from S5R3 sys/timod.h	10.3.1.1" */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:sys/timod.h	10.3.1.1" */

#ifndef _nettli_timod_h
#define _nettli_timod_h

/* internal flags */
#define USED		0x01	/* data structure in use          */
#define FATAL		0x02	/* fatal error M_ERROR occurred   */
#define WAITIOCACK	0x04	/* waiting for info for ioctl act */
#define MORE	        0x08	/* more data */



/* timod ioctls */
#define		TIMOD 		('T'<<8)
#define		TI_GETINFO	(TIMOD|100)
#define		TI_OPTMGMT	(TIMOD|101)
#define		TI_BIND		(TIMOD|102)
#define		TI_UNBIND	(TIMOD|103)


/* TI interface user level structure - one per open file */

struct _ti_user {
	ushort	ti_flags;	/* flags              */
	int	ti_rcvsize;	/* rcv buffer size    */
	char   *ti_rcvbuf;	/* rcv buffer         */
	int	ti_ctlsize;	/* ctl buffer size    */
	char   *ti_ctlbuf;	/* ctl buffer         */
	char   *ti_lookdbuf;	/* look data buffer   */
	char   *ti_lookcbuf;	/* look ctl buffer    */
	int	ti_lookdsize;  /* look data buf size */
	int	ti_lookcsize;  /* look ctl buf size  */
	int	ti_maxpsz;	/* TIDU size          */
	long	ti_servtype;	/* service type       */
	int     ti_lookflg;	/* buffered look flag */
};


/* This should be replaced */
#define OPENFILES     getdtablesize()

#endif /*!_nettli_timod_h*/
