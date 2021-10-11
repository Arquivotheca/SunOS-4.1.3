/*	@(#)glob.h 1.1 92/07/30 SMI; from S5R2 1.1	*/

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * A bunch of global variable declarations lie herein.
 * def.h must be included first.
 */

extern int	msgCount;		/* Count of messages read in */
extern int	mypid;			/* Current process id */
extern int	rcvmode;		/* True if receiving mail */
extern int	sawcom;			/* Set after first command */
extern int	Fflag;			/* -F option (followup) */
extern int	hflag;			/* Sequence number for network -h */
extern int	Hflag;			/* print headers and exit */
extern char	*rflag;			/* -r address for network */
extern char	*Tflag;			/* -T temp file for netnews */
extern int	exitflg;		/* -e for mail test */
extern int	newsflg;		/* -I option for netnews */
extern int	UnUUCP;			/* -U flag */
extern char	nosrc;			/* Don't source
						/usr/lib/mailx/mailx.rc */
extern char	noheader;		/* Suprress initial header listing */
extern int	selfsent;		/* User sent self something */
extern int	senderr;		/* An error while checking */
extern int	edit;			/* Indicates editing a file */
extern int	readonly;		/* Will be unable to rewrite file */
extern int	noreset;		/* String resets suspended */
extern int	sourcing;		/* Currently reading variant file */
extern int	loading;		/* Loading user definitions */
extern int	cond;			/* Current state of conditional exc. */
extern FILE	*itf;			/* Input temp file buffer */
extern FILE	*otf;			/* Output temp file buffer */
extern FILE	*pipef;			/* Pipe file we have opened */
extern int	image;			/* File descriptor for image of msg */
extern FILE	*input;			/* Current command input file */
extern char	*editfile;		/* Name of file being edited */
extern char	*sflag;			/* Subject given from non tty */
extern char	*tflag;			/* Read headers from text */
extern int	outtty;			/* True if standard output a tty */
extern int	intty;			/* True if standard input a tty */
extern int	baud;			/* Output baud rate */
extern char	homedir[];		/* Name of home directory */
extern char	mailname[];		/* Name of system mailbox */
extern int	uid;			/* The invoker's user id */
extern char	myname[];		/* My login id */
extern off_t	mailsize;		/* Size of system mailbox */
extern int	lexnumber;		/* Number of TNUMBER from scan() */
extern char	lexstring[];		/* String from TSTRING, scan() */
extern int	regretp;		/* Pointer to TOS of regret tokens */
extern int	regretstack[];		/* Stack of regretted tokens */
extern char	*stringstack[];		/* Stack of regretted strings */
extern int	numberstack[];		/* Stack of regretted numbers */
extern struct	message	*dot;		/* Pointer to current message */
extern struct	message	*message;	/* The actual message structure */
extern struct	var	*variables[];	/* Pointer to active var list */
extern struct	grouphead	*groups[];/* Pointer to active groups */
extern struct	ignore		*ignore[];/* Pointer to ignored fields */
extern struct	ignore		*retain[HSHSIZE];/* Pointer to retained fields */
extern int	nretained;		/* Number of retained fields */
extern char	**altnames;		/* List of alternate names for user */
extern int	debug;			/* Debug flag set */
extern int	rmail;			/* Being called as rmail */
extern char	*prompt;		/* prompt string */

#include <setjmp.h>

extern jmp_buf	srbuf;


/*
 * The pointers for the string allocation routines,
 * there are NSPACE independent areas.
 * The first holds STRINGSIZE bytes, the next
 * twice as much, and so on.
 */

#define	NSPACE	25			/* Total number of string spaces */
struct strings {
	char	*s_topFree;		/* Beginning of this area */
	char	*s_nextFree;		/* Next alloctable place here */
	unsigned s_nleft;		/* Number of bytes left here */
};
extern struct strings stringdope[];
