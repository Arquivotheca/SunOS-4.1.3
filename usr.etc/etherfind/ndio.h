/*	@(#)ndio.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * External definitions for network disk driver.
 */

#ifndef NDIOCSON

/*
 * ioctl's
 */
#define	NDIOCSON	_IO(n, 0)	/* server on */
#define	NDIOCSOFF	_IO(n, 1)	/* server off */
#define	NDIOCUSER	_IOW(n, 3, struct ndiocuser) /* set user parameters */
#define	NDIOCSAT	_IOW(n, 4, int) /* server at ipaddress */
#define	NDIOCCLEAR	_IO(n, 5)	/* clear table of reg. users */
#define	NDIOCVER	_IOW(n, 7, int)	/* version number */
#define	NDIOCETHER	_IOW(n, 8, struct ndiocether) /* set ether address */

/*
 * NDIOCUSER request structure
 */
struct ndiocuser {
	struct in_addr nu_addr;	/* net address of client */
	int	nu_hisunit;	/* his minor device unit number */
	int	nu_mydev;	/* fd of my serving device (block device) */
	int	nu_myoff;	/* offset of subpartition */
	int	nu_mysize;	/* size of subpartition */
	int	nu_mylunit;	/* my local unit number */
};

/*
 * NDIOCETHER request structure
 */
struct ndiocether {
	struct	in_addr ne_iaddr;	/* inet address of client */
	struct	ether_addr ne_eaddr;	/* ether address of client */
	int	ne_maxpacks;		/* max packs to send between acks */
};

/*
 * kernel driver statistics area
 */
struct ndstat {
	int	ns_rpacks;	/* total received packets */
	int	ns_xpacks;	/* total transmitted packets */
	int	ns_notuser;	/* request received from an invalid user */
	int	ns_noumatch;	/* DONE or ERROR doesnt match any request */
	int	ns_rexmits;	/* retransmits */
	int	ns_nobufs;	/* no buffer available */
	int	ns_lbusy;	/* "local" buffer busy */
	int	ns_operrors;	/* NDOPERRORs */
	int	ns_rseq;	/* bad read or write sequencing */
	int	ns_wseq;
	int	ns_badreq;	/* bad request */
	int	ns_stimo;	/* server timeouts */
	int	ns_utimo;	/* user timeouts */
	int	ns_iseq;	/* bad initial sequence number */
};

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

#if NND > 0
/*
 * "nd" protocol packet format.
 */
struct ndpack {
	struct ip np_ip;	/* ip header, proto IPPROTO_ND */
	u_char	np_op;		/* operation code, see below */
	u_char	np_min;		/* minor device */
	char	np_error;	/* b_error */
	char	np_ver;		/* version number */
	long	np_seq;		/* sequence number */
	long	np_blkno;	/* b_blkno, disk block number */
	long	np_bcount;	/* b_bcount, byte count */
	long	np_resid;	/* b_resid, residual byte count */
	long	np_caddr;	/* current byte offset of this packet */
	long	np_ccount;	/* current byte count of this packet */
}; 				/* data follows */
#endif NND

/* np_op operation codes. */
#define	NDOPREAD	1	/* read */
#define	NDOPWRITE	2	/* write */
#define	NDOPERROR	3	/* error */
#define	NDOPCODE	7	/* op code mask */
#define	NDOPWAIT	010	/* waiting for DONE or next request */
#define	NDOPDONE	020	/* operation done */

/* misc protocol defines. */
#define	NDMAXDATA	1024	/* max data per packet - don't try to change */
#define	NDMAXIO		63*1024	/* max np_bcount - at least minphys */
#define	NDXTIMER	(2*2)	/* rexmit interval (secs*2 for slowtimo) */

#endif
