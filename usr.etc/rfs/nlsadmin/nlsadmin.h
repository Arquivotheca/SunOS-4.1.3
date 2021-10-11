/*	@(#)nlsadmin.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nlsadmin:nlsadmin.h	1.8"

#include "sys/param.h"
/*
 * nlsadmin.h:  nlsadmin defines and structs
 */

/*
 * stuff from lsdbf.h
 */

#define DBFCOMMENT      '#'     /* end of line comment char */
#define DBFWHITESP      " \t"	/* space, tab: white space */
#define DBFTOKENS       " \t:"	/* space, tab, cmnt token seps */
#define SVC_CODE_SZ	16

/*
 * defines for flag characters
 */

#define DBF_ADMIN       0x01
#define DBF_OFF	 0x02		/* service is turned off */
#define DBF_UNKNOWN     0x80	/* indicates unknown flag character */

/*
 * service code parameters
 */

#define DBF_INT_CODE    1	/* intermediary proc svc code */
#define DBF_SMB_CODE    2	/* MS-NET server proc svc code */

#define N_DBF_ADMIN     2	/* no. of admin entries */
#define MIN_REG_CODE    101	/* min normal service code */

/*
 * stuff from lsparam.h
 */

#define DBFLINESZ       1024	   /* max line size in data base   */
#define MAXNAMESZ	15

#define PIDNAME	"pid"
#define DBFNAME	"dbf"
#define VERSION	2		/* current database version */
#define DEFAULTID	"listen"	/* default id for servers */
#define LSUIDNAME	"daemon"
#define LSGRPNAME	"daemon"
#define INITPATH	"/usr/net/nls"
#define HOMEDIR 	"/usr/net/nls" /* root directory for listener file */

/*
 * stuff from listen.h
 */

#define NAMEBUFSZ       64
#define NAMESIZE	(NAMEBUFSZ - 1)

/*
 * stuff for nlsadmin only
 */

#define NONE    0x0000
#define INIT    0x0001
#define ADD     0x0002
#define REM     0x0004
#define ENABL   0x0008
#define DISBL   0x0010
#define KILL    0x0020
#define START   0x0040
#define ADDR    0x0080
#define VERBOSE 0x0100
#define ALL	0x0200
#define ZCODE	0x0400
#define STARLAN	0x0800

#define LISTENER	"listen"
#define PIDFILE	 PIDNAME
#define LOCKFILE	"lock"
#define DBFILE	  DBFNAME
#define ADDRFILE	"addr"
#define DIRECTORY       0040000
#define FILETYPE	0170000
#define ALLPERM		0777
#define LOCKPERM	0666

#define TTY     01
#define LISTEN  02

extern  char    *optarg;
extern  int     optind;
extern  int     errno;

#ifdef S4
#define LOCK locking
#else
extern int lockf();
#define LOCK lockf
#endif

/* error codes */

#define NLSOK		0		/* no error */
#define NLSPERM		2		/* bad permissions */
#define NLSCMD		3		/* bad command line */
#define NLSMEM		4		/* could not malloc enough memory */
#define NLSOPEN		5		/* could not open file */
#define NLSCREAT	6		/* could not create file or directory */
#define NLSINIT		7		/* net_spec already initialized */
#define NLSSERV		8		/* bad service code */
#define NLSEXIST	9		/* service code already exists */
#define NLSCLOSE	10		/* error on close of file */
#define NLSNOCODE	11		/* service code does not exist */
#define NLSSTAT		12		/* could not stat file */
#define NLSFINDP	13		/* could not find pid file */
#define NLSPID		14		/* bad pid file */
#define NLSSIG		15		/* could not send signal to listener process */
#define NLSACTIV	16		/* listener already active */
#define NLSLINK		17		/* could not link file */
#define NLSULINK	18		/* could not unlink file */
#define NLSRDIR		19		/* could not read directory */
#define NLSRDFIL	20		/* could not read file */
#define NLSDBF		21		/* bad database file */
#define NLSADRF		22		/* bad address file */
#define NLSDEAD		23		/* listener not active */
#define NLSNOGRP	24		/* no group entry */
#define NLSCHOWN	25		/* could not change ownership on file */
#define NLSNOPASS	26		/* no passwd entry */
#define NLSEXEC		27		/* could not exec listener */
#define NLSBUSY		28		/* tempfile busy */
#define NLSNOTF		29		/* net_spec not found */
#define NLSNOTUNIQ	30		/* listen addresses not unique */
#define NLSVER		31		/* database file not current version */
#define NLSDBFBAD	32		/* database file is corrupt */

/* other defines */

#define MAXPATH		200		/* max. length of pathnames */
#define DBFNULL		"NULL,"		/* module place holder */
