/*	@(#)cirmgr.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:sys/cirmgr.h	10.14" */

#ifndef _rfs_cirmgr_h
#define _rfs_cirmgr_h

#define MAXTOKLEN sizeof(struct token)		/* maximum token length in bytes */

struct gdpmisc {
	int hetero;
	int version;
};

struct gdp {
	struct queue *queue;	/* pointer to associated stream head	*/
	struct file *file;	/* file pointer to stream head we stole */
	short mntcnt;		/* number of mounts on this stream	*/
	short sysid;
	short flag;		/* connection info */		
	char istate;		/* input state machine			*/
	char oneshot;		/* 1 if incoming msg is in a single block */
	int hetero;		/* need to canonicalize messages	*/
	int version;		/* DU version at the other end of circuit */
	long time;		/* time delta */
	struct token token;	/* circuit identification		*/
	char	*idmap[2];	/* 0=uid=UID_DEV, 1=gid=GID_DEV		*/
	struct msgb *hdr;	/* message header collected so far	*/
	struct msgb *idata;	/* request/response collected so far	*/
	int hlen;		/* header length needs to be collected	*/
	int dlen;		/* data length needs to be collected	*/
	long maxpsz;		/* maximum TIDU size of the provider	*/
	int rbufcid;		/* id of pending read-side bufcall	*/
};

#ifdef KERNEL
extern int maxgdp;
extern struct gdp *gdp;
#endif
#define get_sysid(x)       ((struct gdp *)(x)->sd_queue->q_ptr)->sysid
#define	GDP(x)		((struct gdp *)(x)->q_ptr)


/* GDP circuit state flags */
#define GDPRECOVER	0x004
#define GDPDISCONN	0x002
#define GDPCONNECT	0x001
#define GDPFREE		0x000

/* GDP istate */
#define	GDPST0		0x0	/* gathering header	*/
#define	GDPST1		0x1	/* processing header	*/
#define	GDPST2		0x2	/* gathering data	*/
#define	GDPST3		0x3	/* processing data	*/

#endif /*!_rfs_cirmgr_h*/
