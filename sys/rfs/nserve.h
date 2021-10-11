/*	@(#)nserve.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:sys/nserve.h	10.6" */
/*
 * nserve.h contains defines needed both in the kernel
 * and in user programs for adv, mount, and name service fns
 */

#ifndef _rfs_nserve_h
#define _rfs_nserve_h

#define A_RDWR		0	/* read/write flag			*/
#define A_RDONLY	1	/* read only flag			*/
#define A_CLIST		2	/* client list flag			*/
#define A_MODIFY	4	/* modify (really replace) clist flag	*/
#define A_INUSE		8	/* advertise table entry in use		*/
#define A_FREE		0	/* advertise table entry free		*/
#define A_MINTER	16	/* unadv -- but not free yet		*/
#define SEPARATOR	'.'
#define MAXDNAME	64

/* the following will migrate to /usr/include/nserve.h in load N7	*/

#define NS_REL		15

#define R_NOERR	0	/* no error				*/
#define R_FORMAT 1	/* format error				*/
#define R_NSFAIL 2	/* name server failure			*/
#define R_NONAME 3	/* name does not exist			*/
#define R_IMP	 4	/* request type not implemented or bad	*/
#define R_PERM	 5	/* no permission for this operation	*/
#define R_DUP	 6	/* name not unique (for advertise)	*/
#define R_SYS	 7	/* a system call failed in name server  */
#define R_EPASS  8	/* error accessing primary passwd file	*/
#define R_INVPW  9   	/* invalid password			*/
#define R_NOPW   10	/* no passwd in primary passwd file	*/
#define R_SETUP  11	/* error in ns_setup()			*/
#define R_SEND   12	/* error in ns_send()			*/
#define R_RCV    13	/* error in ns_rcv()			*/
#define R_INREC	 14	/* in recovery, try again		*/
#define R_FAIL	 15	/* unknown failure			*/


/* #ident	"@(#)head:nserve.h	1.4.2.1" */
/* User portion of name server header file -- merged with sys/nserve.h */

#define NSVERSION	1	/* coincides with load n7 SVR3		*/

/*
 * Pathname defines:
 * NSDIRQ and NSDEV are set up to be combined with other #defines
 * The missing trailing quote (") on these defines is correct, as are the
 * missing leading quotes on the other defines in this group.
 * NOTE: don't put a comment after the NSDIRQ or NSDEV defines.
 */
#define NSDEV	  "/usr/nserve
#define NSDIRQ	  "/usr/nserve
#define NSPID	  NSDEV/nspid"	     /* lock file for ns, also has pid	*/
#define NS_PIPE   NSDEV/nspip"	     /* stream pipe for ns		*/
#define NSDIR	  NSDIRQ"	     /* name server working directory.	*/
#define NETMASTER NSDIRQ/rfmaster"   /* master file for nudnix network	*/
#define DOMMASTER NSDIRQ/dom.master" /* file for outside domains 	*/
#define SAVEDB	  NSDIRQ/save.db"    /* saves database when ns exits	*/
#define PASSFILE  NSDIRQ/loc.passwd" /* location of local passwd	*/
#define VERPASSWD NSDIRQ/verify/%s/passwd"  /* passwd for verification  */
#define DOMPASSWD NSDIRQ/auth.info/%s/passwd"  /* passwd for dom  "%s"  */
				     /* contains the domain name.	*/
#define NSDOM	  NSDIRQ/domain"     /* place to save domain name	*/

/* other defines	*/
#define PRIMENAME "PRIMARY"	/* default name for unknown primary	*/
#define CORRECT	  "correct"	/* message stating password is correct	*/
#define INCORRECT "sorry"	/* message stating password is wrong	*/
#define ADVFILE	  "/etc/advtab"
#define TEMPADV	  "/etc/tmpadv"
#define REC_TIME  10

/* sizes	*/

#define	SZ_RES	14	/* maximum size of resource name 		     */
#define SZ_MACH	65	/* maximum size of machine name.		     */
#define SZ_PATH 64	/* size of "pathname" of resource		     */
#define SZ_DESC	32	/* size of description				     */
#define SZ_DELEMENT 14	/* max size of one element of a domain name	     */

/* types	*/
#define FILE_TREE 1	/* sharable file tree	*/

/* flags	*/
#define READONLY  1
#define READWRITE 2

/* command types	*/
#define NS_SNDBACK	0	/* actually a "null" type	*/
#define	NS_ADV		1	/* advertise a resource		*/
#define NS_UNADV	2	/* unadvertise a resource	*/
#define NS_GET		3	/* unused query			*/
#define NS_QUERY	4	/* query by name and type.	*/
#define NS_INIT		5	/* called to setup name service */
#define NS_SENDPASS	7	/* register passwd w/ primary	*/
#define NS_VERIFY	8	/* verify validity of your pwd	*/
#define NS_MODADV	9	/* modify an advertisement	*/
#define NS_BYMACHINE	10	/* inv query by owner and type	*/
#define NS_IQUERY	11	/* general inverse query	*/
#define NS_IM_P		12	/* I am primary poll		*/
#define NS_IM_NP	13	/* I am secondary poll		*/
#define NS_FINDP	14	/* find which machine is prime	*/
#define NS_REL		15	/* relinquish being primary	*/

/* return codes	*/
#ifndef RFS_SUCCESS
#define RFS_FAILURE		0
#define RFS_SUCCESS		1
#endif
#define MORE_DATA 	2	/* or'ed into return when more coming */

/* name server request structure	*/

struct nssend {
	short	ns_code;	/* request code (e.g., NS_ADV)	*/
	short	ns_type;	/* type of data, unused for now	*/
	short	ns_flag;	/* read/write flag		*/
	char	*ns_name;	/* name of resource		*/
	char	*ns_desc;	/* description of resource	*/
	char	*ns_path;	/* local pathname of resource	*/
	struct address	*ns_addr; /* address of resource's owner*/
	char	**ns_mach;	/* list of client machines	*/
};

/* Structure-related definitions */
#define RO_BIT          1	/* for ns_flag field */


#ifndef KERNEL
/* function declarations	*/

int	ns_setup();
int	ns_close();
int	ns_send();
struct nssend	*ns_rcv();
#endif KERNEL

/* macros	*/
#define	SET_NODELAY(f)	fcntl(f,F_SETFL,fcntl(f,F_GETFL,0)|O_NDELAY)
#define	CLR_NODELAY(f)	fcntl(f,F_SETFL,fcntl(f,F_GETFL,0)&~O_NDELAY)

#endif /*!_rfs_nserve_h*/
