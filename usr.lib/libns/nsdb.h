
/*	@(#)nsdb.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:nsdb.h	1.4" */
/*****************************************************************
 *
 *	nsdb.h contains structure and function definitions
 *	for the name server database.  It should be included
 *	by any program that uses the database functions.
 *
 ****************************************************************/

#ifndef RFS_SUCCESS
#define RFS_SUCCESS 1
#define RFS_FAILURE 0
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/*****	Defines *****/

#define NAMSIZ	15	/* directory name size + 1		*/
#define MAXREC	100	/* maximum # of records in a domain	*/
#define NREC	10	/* number of records allocated at a time	*/
#define MAXNS	10	/* maximum # of ns records in a domain	*/
#define WHITESP	"\t \n"

/***** Macros *****/

#define isauth(d,t)	((d->d_auth)&t) /* d is domain ptr, t is NS or SOA */
#define setauth(d,t)	(d->d_auth=t)	/* d is domain ptr, t is NS, SOA or 0 */
#define readdb(f)	readfile(f,TRUE,FALSE)
#define merge(f,o)	readfile(f,FALSE,o)
#define copystr(s)	((s)?(strcpy(malloc(strlen(s)+1),s)):NULL)

/*****  Externs *****/

extern char	Dname[];	/* domain name of this machine	*/
extern int	Primary; 	/* TRUE or FALSE, is this ns primary	*/
extern struct address	Addbuf; /* buffer for addr of current request	*/
extern struct address	*Caddress; /* ptr to address of the current 	*/
				   /* request, NULL if request is LOCAL	*/

/*****  Structures *****/

struct rn {
	char	*rn_owner;	/* full domain name of owner	*/
	char	*rn_desc;	/* description of resource	*/
	char	*rn_path;	/* pathname of resource		*/
	long	rn_flag;	/* "permission" flag for res.	*/
};

struct domain {
	long	d_auth;	/* TRUE if NS is authority for domain	*/
	long	d_size;	/* # of records in d_rec		*/
	struct res_rec	**d_rec;	/* resource records	*/
};

struct res_rec {
	char	rr_name[NAMSIZ]; /* resource, machine or domain name	*/
	long	rr_type;	 /* NS, SOA, RN, or DOM			*/
	union rdata {
		char	*rd_data;	/* data field		*/
		struct rn *rd_rn;	/* RN  - resource data	*/
		struct domain *rd_dom;	/* DOM - domain		*/
	} rr_rdata;
};

#define rr_data  rr_rdata.rd_data
#define rr_ns	 rr_rdata.rd_data
#define rr_soa	 rr_rdata.rd_data
#define rr_a	 rr_rdata.rd_data
#define rr_dom	 rr_rdata.rd_dom
#define rr_dauth rr_rdata.rd_dom->d_auth
#define rr_dsize rr_rdata.rd_dom->d_size
#define rr_drec	 rr_rdata.rd_dom->d_rec
#define rr_rn	 rr_rdata.rd_rn
#define rr_owner rr_rdata.rd_rn->rn_owner
#define rr_desc  rr_rdata.rd_rn->rn_desc
#define rr_path  rr_rdata.rd_rn->rn_path
#define rr_flag	 rr_rdata.rd_rn->rn_flag

struct question {	/* general query format in request */
	char	*q_name;
	long	q_type;
};

struct header {
	long	h_version;
	long	h_flags;
	long	h_opcode;
	long	h_rcode;
	long	h_qdcnt;
	long	h_ancnt;
	long	h_nscnt;
	long	h_arcnt;
	char	*h_dname;
};

struct request {
	struct header	*rq_head;
	struct question **rq_qd;	/* query records		*/
	struct res_rec	**rq_an;	/* answer and add records	*/
	struct res_rec	**rq_ns;	/* ns records for forwarding	*/
	struct res_rec	**rq_ar;	/* additional records		*/
};

/***** Access Functions ************************************************
 *
 * char *filename;		file name
 * char	*name;			fully qualified name
 * struct res_rec *res_rec;  	resource record 
 * struct res_rec **reslist; 	resource record list (NULL Terminated)
 * char *domain;		fully qualified domain name
 * int	type;			record type (NS, SOA, ...)
 *
 ***********************************************************************/

int	writedb(/* filename */); 	   /* write db as Master file	*/
int	readfile(/* filename, clear */);   /* read db in Master format	*/
struct res_rec **findrr(/* name, type */); /* find rrs that match name	*/
int	addrr(/* name, res_rec */); 	   /* add rr to db, name is key	*/
int	remrr(/* name, res_rec */); 	   /* remove rr, name is key	*/
struct res_rec **iquery(/* domain, type, name */); /* inverse query	*/
struct res_rec **findind(/* name */); 	   /* find remote NS or SOA	*/
char	*getctype(/* type */);	    	   /* translate type to string	*/
int	gettype(/* type_name */);   	   /* trans. type name to int	*/
