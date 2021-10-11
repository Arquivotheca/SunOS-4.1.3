/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)conf.h 1.1 92/07/30 SMI; from UCB 5.10 4/1/88
 *
 *  Sendmail
 *  Copyright (c) 1983  Eric P. Allman
 *  Berkeley, California
 *
 */

/*
**  CONF.H -- All user-configurable parameters for sendmail
*/

/*
 * Reject messages to large mailing lists that have no body.
 */
# define REJECT_MIN		10		/* minimum bytes in body */

/*
**  Table sizes, etc....
**	There shouldn't be much need to change these....
*/

# define MAXLINE	1024		/* max line length */
# define MAXNAME	256		/* max length of a name */
# define MAXFIELD	2500		/* max total length of a hdr field */
# define MAXPV		40		/* max # of parms to mailers */
# define MAXHOP		30		/* max value of HopCount */
# define MAXATOM	100		/* max atoms per address */
# define MAXMAILERS	25		/* maximum mailers known to system */
# define MAXRWSETS	50		/* max # of sets of rewriting rules */
# define MAXPRIORITIES	25		/* max values for Precedence: field */
# define MAXTRUST	30		/* maximum number of trusted users */
# define MAXUSERENVIRON	40		/* max # of items in user environ */
# define QUEUESIZE	600		/* max # of jobs per queue run */
# define MAXMXHOSTS	10		/* max # of MX records */

/*
**  Compilation options.
**
**	#define these if they are available; comment them out otherwise.
*/

# define DBM		1	/* use DBM library (requires -ldbm) */
# define NDBM		1	/* new DBM library available (requires DBM) */
# define DEBUG		1	/* enable debugging */
# define LOG		1	/* enable logging */
# define SMTP		1	/* enable user and server SMTP */
# define QUEUE		1	/* enable queueing */
# define UGLYUUCP	1	/* output ugly UUCP From lines */
# define DAEMON		1	/* include the daemon (requires IPC & SMTP) */
# define FLOCK		1	/* use flock file locking */
# define SETPROCTITLE	1	/* munge argv to display current status */
/* # define WIZ		1	/* allow wizard mode */
#define NEWTZCODE	1	/* use new timezone code */
/*#define USG		1	/* building for USG (S3, S5) system */
# define SCANF		1	/* enable scanf format in F lines */

# define YELLOW		1	/* Call NIS for aliases */
# define ALIAS_MAP	"mail.aliases"	/* default NIS map for aliases */
# define FreezeMode 	0644	/* creation mode for Freeze file: */
				/* Must be public read if using NFS */
