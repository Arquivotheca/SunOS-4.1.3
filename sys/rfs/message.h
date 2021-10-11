/*	@(#)message.h 1.1 92/07/30 SMI	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:sys/message.h	10.17" */

/*
 *	network message definitions
 */

#ifndef _rfs_message_h
#define _rfs_message_h

#define	DATASIZE	1024		/* just for testing		*/
#define MAXSNAME	20		/* mach name size in mnt_data	*/
#define	MAXSERVERS	25		/* maximum number of servers	*/
#define ANY		(-1)		/* waitfor a msg on ANY queue	*/
#define CFRD		0		/* Chow Fun receive descriptor	*/
#define SIGRDX		1		/* Special signal receive descriptor*/
#define RECOVER_RD	2		/* Recovery receive descriptor */
#define SCTRSHFT        9       	/* log2 bytes per sector -- rq_ulimit 
					   is defined in terms of these units */

#define REQ_MSG		1		/* request message type */
#define RESP_MSG	2		/* response message type */
#define NACK_MSG	3		/* RFS flow control nack message type */

/* opcodes for remote service */
#define	DUACCESS	33
#define	DUSYSACCT	51
#define	DUCHDIR		12
#define	DUCHMOD		15
#define	DUCHOWN		16
#define DUCHROOT	61
#define	DUCLOSE		6
#define	DUCREAT		8
#define	DUEXEC		11
#define DUEXECE		59
#define DUFCNTL		62
#define DUFSTAT		28
#define DUFSTATFS	38
#define	DUIOCTL		54
#define	DULINK		9
#define	DUMKNOD		14
#define	DUMOUNT		21
#define	DUOPEN		5
#define	DUREAD		3
#define	DUSEEK		19
#define	DUSTAT		18
#define DUSTATFS	35
#define	DUUMOUNT	22
#define	DUUNLINK	10
#define	DUUTIME		30
#define DUUTSSYS  	57
#define	DUWRITE		4
#define DUGETDENTS	81
#define DUMKDIR		80
#define DURMDIR		79
#define	DUADVERTISE	70
#define	DUUNADVERTISE	71
#define	DURMOUNT	72	/* nami half of rmount */
#define DURUMOUNT	73
#define	DUSRMOUNT	97	/* msg from ns to server */
#define	DUSRUMOUNT	98
#define DUCOPYIN	106
#define DUCOPYOUT	107
#define DULINK1		109	/* second half of link  */
#define	DUCOREDUMP	111
#define DUWRITEI	112
#define DUREADI		113
#define DULBMOUNT	115	/* lbin mount (second namei in smount)	*/
#define DULBUMOUNT	116	/* lbin unmount			*/
#define DURSIGNAL	119
#define DUGDPACK	120
#define DUGDPNACK	121
#define DUSYNCTIME	122	/* date synchronization */
#define DUDOTDOT	124	/* server sends this back to client */
#define DULBIN		125	/* server sends this back to client */
#define DUFUMOUNT	126	/* forced unmount */
#define DUSENDUMSG	127	/* send message to remote user-level */
#define DUGETUMSG	128	/* get message from remote user-level */
#define DUWAKEUP	129	/* copyout and wakeup (test only) */
#define DUFWFD		130	
#define DUIPUT		131
#define DUIUPDATE	132
#define DUUPDATE	133
#define DUCACHEDIS	134	/* disable cache */


/* to guarantee allignment for response and request structures, we
 * create this 
 */

struct common {
	long	opcode;			/* what to do */
	long	sysid;			/* where we came from */
	long	type;			/* message type - request/response */
	long	pid;			/* client pid */
	long	uid;			/* client uid */
	long	gid;			/* client gid */
	long	ftype;
	long	nlink;
	long	size;
	long	mntindex;		/* mount index */
};

struct	request	{
	struct	common	rq;
	long	rq_rrdir;
	daddr_t	rq_ulimit;
	long	rq_arg[4];
	long	rq_tmp[4];		/* for future use */
	char	rq_data[DATASIZE];
};

#define	rq_opcode	rq.opcode
#define	rq_sysid	rq.sysid
#define	rq_type		rq.type
#define	rq_pid		rq.pid
#define	rq_uid		rq.uid
#define	rq_gid		rq.gid
#define	rq_mntindx	rq.mntindex

#define	rq_fmode	rq_arg[3]		/* user mode */
#define	rq_mode		rq_arg[3]		/* file.h modes */
#define	rq_newuid	rq_arg[0]
#define	rq_newgid	rq_arg[1]
#define	rq_bufptr	rq_arg[0]
#define rq_len		rq_arg[1]
#define rq_fstyp	rq_arg[2]
#define	rq_cmd		rq_arg[0]
#define	rq_ioarg	rq_arg[1]
#define	rq_dev		rq_arg[1]
#define	rq_crtmode	rq_arg[1]
#define	rq_count	rq_arg[1]
#define	rq_offset	rq_arg[0]
#define	rq_whence	rq_arg[1]
#define	rq_atime	rq_arg[0]
#define	rq_mtime	rq_arg[1]
#define rq_fcntl	rq_arg[1]
#define	rq_sofar	rq_arg[2]	/* for copyin, see os/rmove.c  */
#define rq_prewrite	rq_tmp[0]	/* for copyin, see os/rmove.c  */
#define rq_base		rq_arg[2]	/* for readi/writei */
#define rq_newmntindx	rq_arg[0]	/* new mount index for lbin mount */
#define	rq_cmask	rq_arg[2]
#define	rq_flag		rq_arg[0]	/* for rmount readwrite flag	*/
#define	rq_srmntindx	rq_arg[0]	/* for forced unmount */
#define rq_fflag	rq_arg[3]
#define rq_link		rq_arg[0]
#define rq_foffset      rq_arg[2]
#define rq_synctime	rq_arg[2]
#define rq_fhandle	rq_arg[0]	/* file handle for client caching */
#define	rq_flags	rq_tmp[1]	/* request flags (see below)*/

/* request flags (rq_flags) */
#define	RQ_MNDLCK	0x1

struct	response	{
	struct common	rp;
	long		rp_errno;
	long		rp_bp;
	long		rp_rval;	/* general purpose integer	 */
	long		rp_arg[4];
	long		rp_tmp[3];	/* for future use */
	char		rp_data[DATASIZE];
};

#define	rp_opcode	rp.opcode
#define	rp_sysid	rp.sysid
#define	rp_type		rp.type
#define	rp_pid		rp.pid
#define	rp_uid		rp.uid
#define	rp_gid		rp.gid
#define	rp_mntindx	rp.mntindex
#define rp_size		rp.size
#define rp_nlink	rp.nlink
#define rp_ftype	rp.ftype

#define	rp_count	rp_arg[0]
#define rp_isize	rp_arg[1]
#define	rp_bufptr	rp_arg[2]
#define	rp_copysync	rp_arg[1]
#define rp_mode		rp_arg[2]
#define rp_sig		rp_arg[3]
#define rp_subyte	rp_tmp[0]
#define rp_offset       rp_tmp[1]
#define rp_synctime	rp_arg[2]
#define rp_fhandle	rp_tmp[2]	/* file handle for client caching */
#define rp_cache	rp_tmp[1]	/* cache flag for client caching */

#define	RP_MNDLCK	0x8000		/* mandatory lock set (included with
					   client cache flags in rp_cache*/

/* mntdata is used to store the uname and resname in rq_data
 * Later, it may also store domain name.
 */

struct mntdata {
	char	md_uname[MAXSNAME];
	char	md_resname[DATASIZE-MAXSNAME];
};

#endif /*!_rfs_message_h*/
