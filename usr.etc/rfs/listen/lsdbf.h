/*	@(#)lsdbf.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:lsdbf.h	1.3.1.1"

/*
 * lsdbf.h:	listener data base file defines and structs
 */

#define DBFCOMMENT	'#'		/* end of line comment char	*/
#define DBFWHITESP	" \t"		/* space, tab: white space 	*/
#define DBFTOKENS	" \t:"		/* space, tab, cmnt token seps	*/

/*
 * defines for flag characters
 */

#define	DBF_ADMIN	0x01
#define DBF_OFF		0x02		/* service is turned off	*/
#define	DBF_UNKNOWN	0x80		/* indicates unkown flag character */

/*
 * service code parameters
 */

#define	DBF_INT_CODE	"1"		/* intermediary proc svc code	*/
#define DBF_SMB_CODE	"2"		/* MS-NET server proc svc code	*/

#define N_DBF_ADMIN	2		/* no. of admin entries		*/
#define MIN_REG_CODE	101		/* min normal service code	*/

#define	SVC_CODE_SZ	16

/*
 * current database version
 */

#define VERSION	2

typedef struct {
	int	dbf_flags;		/* flags			*/
	char	*dbf_svc_code;		/* null terminated service code	*/
	char	*dbf_id;		/* user id for server to run as */
	char	*dbf_reserved;		/* place holder for future use	*/
	char	*dbf_modules;		/* optional modules to push	*/
	char	*dbf_cmd_line;		/* null terminated cmd line	*/
} dbf_t;
