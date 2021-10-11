/*      @(#)nsports.h 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nserve:nsports.h	1.7"
/********************************************************************
 *
 *	nsports.h contains the defines necessary for programs
 *	that use the functions in nsports.c
 *
 *******************************************************************/
/* sizes	*/
#define NPORTS	10	/* maximum # of ports per process	*/
#define PK_MAXSIZ 1024	/* maximum packet size for name server	*/

/* nswait return codes	*/
#define FATAL	-1	/* FATAL error, can't proceed			*/
#define NON_FATAL 1	/* error occurred, but not serious, continue	*/
#define LOC_REQ 2	/* local request				*/
#define REM_REQ 3	/* remote request				*/
#define REC_IN	4	/* input from recovery stream			*/
#define REC_CON	5	/* connect indication from recovery stream	*/
#define REC_ACC	6	/* connect accept  from recovery stream		*/
#define REC_HUP	7	/* hangup on recovery stream			*/

/* port mode	*/
#define UNUSED	0
#define LOCAL	1
#define REMOTE	2
#define RECOVER	3
#define RESERVED 4
#define PENDING 5
#define PENDING2 6

/* local defines	*/
#define VER_HI		NSVERSION
#define VER_LO		NSVERSION
#define TIRDWR_MOD	"tirdwr"
#define	FIRST_FMT	"lc8c504"		/* tracks struct first_msg	*/
#define FMSGTYPE	8	/* tracks size of first c field in FIRST_FMT	*/
#define FMSGADDR	504	/* tracks size of second c field in FIRST_FMT	*/
#define LOC_MSG		"LOCAL"
#define REM_MSG		"REMOTE"
#define REC_MSG		"RECOVER"
#define OK_MSG		"OK"
#define NOK_MSG		"NOT_OK"
#define BAD_VERSION	"BAD_VERSION"
#define UNK_MODE	"UNK_MODE"
#define FMSGSIZ		sizeof(struct first_msg)
#define SEL_TIME	-1		/* wait indefinitely		*/

/* structures	*/
struct pkt_hd {	/* packet header, same for all	*/
	long	h_fill;	 /* filler to distinguish data from control	*/
	long	h_id;	 /* packet id	*/
	long	h_index; /* this packet's index in stream	*/
	long	h_total; /* total # of packets in stream	*/
	long	h_size;	 /* size of data in this packet		*/
};
#define HDR_FMT	"lllll"

struct pkt { /* packet itself, pkt_hd is needed mostly for size	*/
	struct pkt_hd	pk_hd;
	char		pk_data[1]; /* just a place holder	*/
};

/* defines to hide pkt_hd	*/
#define pk_fill		pk_hd.h_fill
#define pk_id		pk_hd.h_id
#define pk_index	pk_hd.h_index
#define pk_total	pk_hd.h_total
#define pk_size		pk_hd.h_size

struct nsport {
	int	p_mode;			/* CONNECT or LISTEN		*/
	int	p_fd;			/* file descriptor		*/
	struct pkt  *p_wpkt;		/* space for write packet	*/
	struct pkt  *p_rpkt;		/* space for read packet	*/
};

struct first_msg {
	long	version;
	char	mode[FMSGTYPE];		/* type of request	*/
	char	addr[FMSGADDR];		/* address of requestor	*/
	long	pad[2];			/* padding to account   */
					/* for canon len header */
};

/* functions	*/
int	nsconnect(/* struct address *addr, int mode */);
int	nslisten();
int	nswrite(/* int pd, char *buf, int size */);
int	nsread(/* int pd, char **buf, int size */);
int	nswait(/* int ppd, int spd */);
int	nsclose(/* int pd */);
int	nsgetpd();
int	ptrtopd(/* struct nsport *pptr */);
struct nsport *pdtoptr(/* int pd */);
