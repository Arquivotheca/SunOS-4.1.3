
/*	@(#)pn.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)rfsetup:pn.h	1.2" */

#ifndef _rfs_pn_h
#define _rfs_pn_h

/* switch table structure */
typedef struct {
	char *sw_opcode;	/* opcode */
	int sw_idx;		/* index */
} pntab_t;

#define RFSD 105			/* for rfsdaemon */

#define NUMSWENT 3		/* the number of entries in sw_tab */

/* these are the indicies into sw_tab.
   note that the orders must match the opcodes */

#define RF_RF 0			/* remote file service */
#define RF_NS 1			/* name service */
#define RF_AK 2			/* acknowledgement */

#define NUMDUENT 1		/* the number of entries in du_tab */

/* these are the indicies into du_tab.
   note that the orders must match the opcodes */

#define MNT 0			/* mount case */

#define PASSWDLEN	20		/* length of password */
#define C_RETRY		2		/* command retry count */
#define DEVSTR		"/dev/%s"
#define LISTNMSG	"NLPS:000:001:%d"
#define CANONSTR	"c4ll"		/* canonical pntab */
#define CANON_CLEN	16		/* canonical length of pntab */
#define OPCODLEN	4		/* 3 chars + null */

/* negotiate data packect */

typedef struct {
	long n_hetero;		/* heterogeneity indication */
	char n_passwd[PASSWDLEN];	/* password */
	struct token n_token;	/* client's token */
	char n_netname[MAXDNAME];	/* netnodename */
} ndata_t;

struct n_data	{
	ndata_t nd_ndata;
	int	n_vhigh;
	int	n_vlow;
};

typedef struct {
	char pn_op[OPCODLEN];
	long pn_lo;
	long pn_hi;
} pnhdr_t;

/* these version numbers specify compatibility between
	different versions of protocol negotiations */

#define LO_VER	1		/* lo version of this library */
#define HI_VER	1		/* hi version of this library */

/* these are the flags for the negotiate routine */

#define	RFS_SERVER	0
#define	RFS_CLIENT	1

#define TIMOD	"timod"

#endif /*!_rfs_pn_h*/
