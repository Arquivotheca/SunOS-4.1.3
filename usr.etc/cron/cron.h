/*	@(#)cron.h 1.1 92/07/30 SMI; from S5R3 1.4	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define FALSE		0
#define TRUE		1
#define MINUTE		60L
#define HOUR		60L*60L
#define	NQUEUE		26		/* number of queues available */
#define	ATEVENT		0
#define BATCHEVENT	1
#define CRONEVENT	2

#define ADD		'a'
#define DELETE		'd'
#define	AT		'a'
#define CRON		'c'

#define	QUE(x)		('a'+(x))
#define RCODE(x)	(((x)>>8)&0377)
#define TSIGNAL(x)	((x)&0177)
#define	COREDUMPED(x)	((x)&0200)

#define	FLEN	15
#define	LLEN	9

/* structure used for passing messages from the
   at and crontab commands to the cron			*/

struct	message {
	char	etype;
	char	action;
	char	fname[FLEN];
	char	logname[LLEN];
} msgbuf;

/* anything below here can be changed */

#define CRONDIR		"/var/spool/cron/crontabs"
#define ATDIR		"/var/spool/cron/atjobs"
#define PRIVCONFIGDIR	"/var/spool/cron"
#define COMCONFIGDIR	"/usr/lib/cron"
#define CRONALLOW	"cron.allow"
#define CRONDENY	"cron.deny"
#define ATALLOW		"at.allow"
#define ATDENY		"at.deny"
#define PROTO		".proto"
#define	QUEDEFS		"queuedefs"
#define	FIFO		"/var/spool/cron/FIFO"

#define SHELL		"/bin/sh"	/* shell to execute */

#define CTLINESIZE	1000	/* max chars in a crontab line */
#define UNAMESIZE	20	/* max chars in a user name */
