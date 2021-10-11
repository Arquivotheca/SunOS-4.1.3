/*	@(#)v7.local.h 1.1 92/07/30 SMI; from S5R2 1.1	*/

/*
 * Declarations and constants specific to an installation.
 */
 
/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 */

#define	GETHOST				/* Have gethostname syscall */
/* #define	UNAME			/* System has uname syscall */
#define	LOCAL		EMPTYID		/* Dynamically determined local host */

#define MYDOMAIN	".uucp"		/* Appended to local host name */

#define	MAIL		"/bin/mail"	/* Name of mail sender */
#define SENDMAIL	"/usr/lib/sendmail"
					/* Name of classy mail deliverer */
#define	EDITOR		"/usr/ucb/ex"	/* Name of text editor */
#define	VISUAL		"/usr/ucb/vi"	/* Name of display editor */
#define	MORE		(value("PAGER") ? value("PAGER") : "/usr/ucb/more")
					/* Standard output pager */
#define	LS		(value("LISTER") ? value("LISTER") : "/bin/ls")
					/* Name of directory listing prog*/
#define	SHELL		"/bin/sh"	/* Standard shell */
#define	HELPFILE	libpath("Mail.help")
					/* Name of casual help file */
#define	THELPFILE	libpath("Mail.help.~")
					/* Name of casual tilde help */
#define	UIDMASK		0177777		/* Significant uid bits */
#define	MASTER		libpath("Mail.rc")
#define	APPEND				/* New mail goes to end of mailbox */
#define CANLOCK				/* Locking protocol actually works */
#define	UTIME				/* System implements utime(2) */

#ifndef VMUNIX
#include "sigretro.h"			/* Retrofit signal defs */
#endif VMUNIX
