/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acctdef.h 1.1 92/07/30 SMI" /* from S5R3.1 acct:acctdef.h 1.10 */
/*
 *	defines, typedefs, etc. used by acct programs
 */


/*
 *	acct only typedefs
 */
#if u3b || u3b15 || u3b2
#define HZ	100
#else
#define HZ	60
#endif

#ifdef SYSTEMV
#define LSZ	12	/* sizeof line name */
#else
#define LSZ	8	/* sizeof line name */
#endif
#define NSZ	8	/* sizeof login name */
#define P	0	/* prime time */
#define NP	1	/* nonprime time */

/*
 *	limits which may have to be increased if systems get larger
 */
#define A_SSIZE	1000	/* max number of sessions in 1 acct run */
#define A_TSIZE	128	/* max number of line names in 1 acct run */
#define A_USIZE	500	/* max number of distinct login names in 1 acct run */

#define EQN(s1, s2)	(strncmp(s1, s2, sizeof(s1)) == 0)
#define CPYN(s1, s2)	strncpy(s1, s2, sizeof(s1))

#define SECSINDAY	86400L
#define SECS(tics)	((double) tics)/HZ
#define MINS(secs)	((double) secs)/60
#define MINT(tics)	((double) tics)/(60*HZ)

#ifndef SYSTEMV
/*	Definitions for ut_type						*/

#define	EMPTY		0
#define	RUN_LVL		1
#define	BOOT_TIME	2
#define	OLD_TIME	3
#define	NEW_TIME	4
#define	INIT_PROCESS	5	/* Process spawned by "init" */
#define	LOGIN_PROCESS	6	/* A "getty" process waiting for login */
#define	USER_PROCESS	7	/* A user process */
#define	DEAD_PROCESS	8
#define	ACCOUNTING	9

#define	UTMAXTYPE	ACCOUNTING	/* Largest legal value of ut_type */

/*
 * special name for ut_line & ut_name
 */
#define BOOT_NAME	"reboot"
#define SHUT_NAME	"shutdown"
#define OLD_NAME	"|"
#define NEW_NAME	"{"
#define ACCT_NAME	"@@acct"
#endif
