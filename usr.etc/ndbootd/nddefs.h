/*	@(#)1.1 nddefs.h 92/07/30 SMI
 *
 *	Copyright (c) 1986 by Sun Micorsystems, Inc.
 */

/*
 * This file contains definitions derived from <sun/ndio.h>.
 * Definitions not needed by the user-level ND Boot Server
 * (ndbootd.c) are omitted.
 */

/*
 * Minor device bit decoding.
 * the 8 bit minor device number for /dev/nd* is decoded:
 * 	L P UUUUUU	where:
 * L = local, P = read only public, U = user relative unit number.
 * e.g. /dev/nd0 = 0, /dev/ndp0 = 0100, /dev/ndl0 = 0200.
 *
 */
#define	NDMINLOCAL	0200	/* local device */
#define	NDMINPUBLIC	0100	/* public device */
#define	NDMINUNIT	077	/* unit number */

/*
 * ND protocol packet header format.
 */
struct ndhdr {
	u_char	np_op;		/* operation code, see below */
	u_char	np_min;		/* minor device */
	char	np_error;	/* b_error */
	char	np_ver;		/* version number */
	long	np_seq;		/* sequence number */
	long	np_blkno;	/* b_blkno, disk block number */
	long	np_bcount;	/* b_bcount, total bytes of request */
	long	np_resid;	/* b_resid, residual byte count */
	long	np_caddr;	/* current byte offset of this packet */
	long	np_ccount;	/* current byte count of this packet */
}; 				/* data follows */

/* np_op operation codes. */
#define	NDOPREAD	1	/* read */
#define	NDOPWRITE	2	/* write */
#define	NDOPERROR	3	/* error */
#define	NDOPCODE	7	/* op code mask */
#define	NDOPWAIT	010	/* waiting for DONE (from server) or next
				   request (from client) */
#define	NDOPDONE	020	/* operation done (if packet is from server,
				   bit is on, otherwise off) */
