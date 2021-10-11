
/*	@(#)stdns.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:stdns.h	1.4.1.1"
/*
 *
 *	stdns.h contains formats, etc for domain name server queries,
 *	replies and resource records.  The terminology is reminiscent
 *	of some of ARPA's name server work (e.g., RFC 883), however,
 *	since this name server's functions are not at all the same
 *	as ARPA's, there is no resemblance beyond terminology.
 *
 */

#ifndef RFS_SUCCESS
#define RFS_FAILURE	0
#define RFS_SUCCESS	1
#endif
#define	SEPARATOR	'.'
#define WILDCARD	'*'	/* wildcard char for string compares */

/*
 * each record in the name server has one of the types defined below
 */

#define MASKNS	~0x7F		/* mask out ns order bits		*/
#define NSTYPE	0x80		/* bit set for all ns records		*/
#define	PNS	(NSTYPE|0x01)	/* primary name server			*/
#define SNS	(NSTYPE|0x02)	/* secondary name server 		*/
#define SOA	PNS
#define NS	SNS
/* all types, except NS types, must have the lowest 8 bits clear	*/
#define A	0x100		/* host address				*/
#define NULLREC 0x200		/* empty record				*/
#define RN	0x300		/* shared resource record		*/
#define DOM	0x400		/* "internal" type for domain		*/
#define ANYTYPE 0xFFFF		/* request for any record type		*/

/* RCODE values, response codes */

extern int ns_errno;
#define R_NOERR	0	/* no error	*/
#define R_FORMAT 1	/* format error */
#define R_NSFAIL 2	/* name server failure */
#define R_NONAME 3	/* name does not exist */
#define R_IMP	 4	/* request type not implemented (or bad type) */
#define R_PERM	 5	/* no permission for this operation	*/
#define R_DUP	 6	/* name not unique (for advertise)	*/
#define R_SYS	 7	/* a system call failed in name server  */
#define R_EPASS  8	/* error in accessing passwd file on primary  */
#define R_INVPW  9   	/* invalid password			*/
#define R_NOPW   10	/* no password entry in primary passwd file   */
#define R_SETUP  11	/* error in ns_setup()			*/
#define R_SEND   12	/* error in ns_send()			*/
#define R_RCV    13	/* error in ns_rcv()			*/
#define R_INREC	 14	/* in recovery, try again		*/
#define R_FAIL	 15	/* unknown failure			*/

/* miscellaneous defines	*/

/* values for h_qr field	*/
#define QUERY	 1	/* this request is a query	*/
#define RESPONSE 0	/* this request is a response	*/

/* values for h_aa field (or-able with h_qr)	*/
#define AUTHORITY	2
#define NOT_AUTHORITY	0

#define DBLKSIZ   512	/* default block size (start small)		*/
#define MAX_RETRY 5	/* maximum # of name servers to try on query	*/

/*
 * The place structure keeps track of progress reading or
 * writing a block in canonical format.
 */
typedef struct place {
	char	*p_start; /* beginning of block		 */
	char	*p_ptr;	  /* current place in block	 */
	char	*p_end;	  /* end of block		 */
	int	p_extra;  /* location in bits from p_ptr */
} place_t, *place_p;

#define ALIGN	4	/* canonical form alignment in bytes	*/
#define L_SIZE	4	/* size of a canonical long		*/
#if ALIGN == 4
#define	align(p)	(((unsigned int)(p)+3) & ~3)
#else
#define align(x)	(((unsigned int)(x) % ALIGN)?\
			 (unsigned int)(x) + (ALIGN - ((unsigned int)(x) % ALIGN)): (x))
#endif

#define c_sizeof(s)	align(L_SIZE + ((s)?strlen(s)+1:1))

/* miscellaneous defines	*/

/* WARNING: overbyte references the parameter p twice. Don't use expressions
   with side effects (e.g. ++s) */
#define overbyte(p,b)	(p->p_ptr+b > p->p_end)

/* functions in ind_data.c */
char	*compress();
char	*expand();
char	*reqtob();
struct request	*btoreq();
place_p	setplace();
char	*getstr();
long	getlong();
int	putlong();
