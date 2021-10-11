/*       @(#)ex.h 1.1 92/07/30 SMI; from S5R3.1 1.24    */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

/*
 * This file contains most of the declarations common to a large number
 * of routines.  The file ex_vis.h contains declarations
 * which are used only inside the screen editor.
 * The file ex_tune.h contains parameters which can be diddled per installation.
 *
 * The declarations relating to the argument list, regular expressions,
 * the temporary file data structure used by the editor
 * and the data describing terminals are each fairly substantial and
 * are kept in the files ex_{argv,re,temp,tty}.h which
 * we #include separately.
 *
 * If you are going to dig into ex, you should look at the outline of the
 * distribution of the code into files at the beginning of ex.c and ex_v.c.
 * Code which is similar to that of ed is lightly or undocumented in spots
 * (e.g. the regular expression code).  Newer code (e.g. open and visual)
 * is much more carefully documented, and still rough in spots.
 *
 */
#ifdef UCBV7
#include <whoami.h>
#endif
#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/stat.h>

#define FTYPE(A)	(A.st_mode)
#define FMODE(A)	(A.st_mode)
#define	IDENTICAL(A,B)	(A.st_dev==B.st_dev && A.st_ino==B.st_ino)
#define ISBLK(A)	((A.st_mode & S_IFMT) == S_IFBLK)
#define ISCHR(A)	((A.st_mode & S_IFMT) == S_IFCHR)
#define ISDIR(A)	((A.st_mode & S_IFMT) == S_IFDIR)
#define ISFIFO(A)	((A.st_mode & S_IFMT) == S_IFIFO)
#define ISREG(A)	((A.st_mode & S_IFMT) == S_IFREG)

#ifdef USG
#include <termio.h>
#ifdef TCGETS
typedef struct termios SGTTY;
#else
typedef struct termio SGTTY;
#endif
#else
#include <sgtty.h>
typedef struct sgttyb SGTTY;
#endif

#ifdef PAVEL
#define SGTTY struct sgttyb	/* trick Pavel curses to not include <curses.h> */
#endif
typedef char bool;
typedef unsigned long chtype;
#include <term.h>
#define bool vi_bool
#ifdef PAVEL
#undef SGTTY
#endif
#ifndef var
#define var	extern
#endif
var char *exit_bold;		/* string to exit standout mode */

/*
 *	The following little dance copes with the new USG tty handling.
 *	This stuff has the advantage of considerable flexibility, and
 *	the disadvantage of being incompatible with anything else.
 *	The presence of the symbol USG will indicate the new code:
 *	in this case, we define CBREAK (because we can simulate it exactly),
 *	but we won't actually use it, so we set it to a value that will
 *	probably blow the compilation if we goof up.
 */
#ifdef USG
#define CBREAK xxxxx
#endif

extern	int errno;

#ifndef VMUNIX
typedef	short	line;
#else
typedef	int	line;
#endif
typedef	short	bool;

#include "ex_tune.h"
#include "ex_vars.h"
/*
 * Options in the editor are referred to usually by "value(vi_name)" where
 * name is all uppercase, i.e. "value(vi_PROMPT)".  This is actually a macro
 * which expands to a fixed field in a static structure and so generates
 * very little code.  The offsets for the option names in the structure
 * are generated automagically from the structure initializing them in
 * ex_data.c... see the shell script "makeoptions".
 */
struct	option {
	char	*oname;
	char	*oabbrev;
	short	otype;		/* Types -- see below */
	short	odefault;	/* Default value */
	short	ovalue;		/* Current value */
	char	*osvalue;
};

#define	ONOFF	0
#define	NUMERIC	1
#define	STRING	2		/* SHELL or DIRECTORY */
#define	OTERM	3

#define	value(a)	options[a].ovalue
#define	svalue(a)	options[a].osvalue

extern	 struct	option options[vi_NOPTS + 1];


/*
 * The editor does not normally use the standard i/o library.  Because
 * we expect the editor to be a heavily used program and because it
 * does a substantial amount of input/output processing it is appropriate
 * for it to call low level read/write primitives directly.  In fact,
 * when debugging the editor we use the standard i/o library.  In any
 * case the editor needs a printf which prints through "putchar" ala the
 * old version 6 printf.  Thus we normally steal a copy of the "printf.c"
 * and "strout" code from the standard i/o library and mung it for our
 * purposes to avoid dragging in the stdio library headers, etc if we
 * are not debugging.  Such a modified printf exists in "printf.c" here.
 */
#ifdef TRACE
#include <stdio.h>
	var	FILE	*trace;
	var	bool	trubble;
	var	bool	techoin;
	var	char	tracbuf[BUFSIZ];
#undef	putchar
#undef	getchar
#else
/*
 * Warning: do not change BUFSIZ without also changing LBSIZE in ex_tune.h
 * Running with BUFSIZ set to anything besides what is in <stdio.h> is
 * not recommended, if you use stdio.
 */
#ifdef u370
#define	BUFSIZ	4096
#else
#define	BUFSIZ	1024
#endif
#undef	NULL
#define	NULL	0
#undef	EOF
#define	EOF	-1
#endif

/*
 * Character constants and bits
 *
 * The editor uses the QUOTE bit as a flag to pass on with characters
 * e.g. to the putchar routine.  The editor never uses a simple char variable.
 * Only arrays of and pointers to characters are used and parameters and
 * registers are never declared character.
 */
#define	QUOTE	0400
#define	TRIM	0377
#define	NL	CTRL(j)
#define	CR	CTRL(m)
#define	DELETE	0177		/* See also ATTN, QUIT in ex_tune.h */
#define	ESCAPE	033
#undef	CTRL
#define	CTRL(c)	('c' & 037)

/*
 * Miscellaneous random variables used in more than one place
 */
var	bool	aiflag;		/* Append/change/insert with autoindent */
var	bool	anymarks;	/* We have used '[a-z] */
var	int	chng;		/* Warn "No write" */
var	char	*Command;
var	short	defwind;	/* -w# change default window size */
var	int	dirtcnt;	/* When >= MAXDIRT, should sync temporary */
#ifdef SIGTSTP
var	bool	dosusp;		/* Do SIGTSTP in visual when ^Z typed */
#endif
var	bool	edited;		/* Current file is [Edited] */
var	line	*endcore;	/* Last available core location */
extern	 bool	endline;	/* Last cmd mode command ended with \n */
#ifndef VMUNIX
var	short	erfile;		/* Error message file unit */
#endif
var	line	*fendcore;	/* First address in line pointer space */
var	char	file[FNSIZE];	/* Working file name */
var	char	genbuf[MAXBSIZE]; /* Working buffer when manipulating linebuf */
var	bool	hush;		/* Command line option - was given, hush up! */
var	char	*globp;		/* (Untyped) input string to command mode */
var	bool	holdcm;		/* Don't cursor address */
var	bool	inappend;	/* in ex command append mode */
var	bool	inglobal;	/* Inside g//... or v//... */
var	char	*initev;	/* Initial : escape for visual */
var	bool	inopen;		/* Inside open or visual */
var	char	*input;		/* Current position in cmd line input buffer */
var	bool	intty;		/* Input is a tty */
var	short	io;		/* General i/o unit (auto-closed on error!) */
extern	 short	lastc;		/* Last character ret'd from cmd input */
var	bool	laste;		/* Last command was an "e" (or "rec") */
var	unsigned char	lastmac;	/* Last macro called for ** */
var	char	lasttag[TAGSIZE];	/* Last argument to a tag command */
var	char	*linebp;	/* Used in substituting in \n */
var	char	linebuf[LBSIZE];	/* The primary line buffer */
var	bool	listf;		/* Command should run in list mode */
var	char	*loc1;		/* Where re began to match (in linebuf) */
var	char	*loc2;		/* First char after re match (") */
var	line	names['z'-'a'+2];	/* Mark registers a-z,' */
var	int	notecnt;	/* Count for notify (to visual from cmd) */
var	bool	numberf;	/* Command should run in number mode */
var	char	obuf[BUFSIZ];	/* Buffer for tty output */
var	short	oprompt;	/* Saved during source */
var	short	ospeed;		/* Output speed (from gtty) */
var	int	otchng;		/* Backup tchng to find changes in macros */
var	short	peekc;		/* Peek ahead character (cmd mode input) */
var	char	*pkill[2];	/* Trim for put with ragged (LISP) delete */
var	bool	pfast;		/* Have stty -nl'ed to go faster */
var	int	pid;		/* Process id of child */
var	int	ppid;		/* Process id of parent (e.g. main ex proc) */
var	jmp_buf	resetlab;	/* For error throws to top level (cmd mode) */
var	int	rpid;		/* Pid returned from wait() */
var	bool	ruptible;	/* Interruptible is normal state */
var	bool	seenprompt;	/* 1 if have gotten user input */
var	bool	shudclob;	/* Have a prompt to clobber (e.g. on ^D) */
var	int	status;		/* Status returned from wait() */
var	int	tchng;		/* If nonzero, then [Modified] */
extern	short	tfile;		/* Temporary file unit */
var	bool	vcatch;		/* Want to catch an error (open/visual) */
var	jmp_buf	vreslab;	/* For error throws to a visual catch */
var	bool	writing;	/* 1 if in middle of a file write */
var	int	xchng;		/* Suppresses multiple "No writes" in !cmd */
var	long	bsize;		/* Block size for disk i/o */

/*
 * Macros
 */
#define	CP(a, b)	(ignore(strcpy(a, b)))
			/*
			 * FIXUNDO: do we want to mung undo vars?
			 * Usually yes unless in a macro or global.
			 */
#define FIXUNDO		(inopen >= 0 && (inopen || !inglobal))
#define ckaw()		{if (chng && value(vi_AUTOWRITE) && !value(vi_READONLY)) \
				wop(0);\
			}
#define	copy(a,b,c)	Copy((char *) (a), (char *) (b), (c))
#define	eq(a, b)	((a) && (b) && strcmp(a, b) == 0)
#define	getexit(a)	copy(a, resetlab, sizeof (jmp_buf))
#define	lastchar()	lastc
#define	min(a,b)	(a > b ? b : a)
#define	outchar(c)	(*Outchar)(c)
#define	pastwh()	(ignore(skipwh()))
#define	pline(no)	(*Pline)(no)
#define	reset()		longjmp(resetlab,1)
#define	resexit(a)	copy(resetlab, a, sizeof (jmp_buf))
#define	setexit()	setjmp(resetlab)
#define	setlastchar(c)	lastc = c
#define	ungetchar(c)	peekc = c

#define	CATCH		vcatch = 1; if (setjmp(vreslab) == 0) {
#define	ONERR		} else { vcatch = 0;
#define	ENDCATCH	} vcatch = 0;

/*
 * Environment like memory
 */
var	char	altfile[FNSIZE];	/* Alternate file name */
extern	char	direct[ONMSZ];		/* Temp file goes here */
extern	char	shell[ONMSZ];		/* Copied to be settable */
var	char	uxb[UXBSIZE + 2];	/* Last !command for !! */

/*
 * The editor data structure for accessing the current file consists
 * of an incore array of pointers into the temporary file tfile.
 * Each pointer is 15 bits (the low bit is used by global) and is
 * padded with zeroes to make an index into the temp file where the
 * actual text of the line is stored.
 *
 * To effect undo, copies of affected lines are saved after the last
 * line considered to be in the buffer, between dol and unddol.
 * During an open or visual, which uses the command mode undo between
 * dol and unddol, a copy of the entire, pre-command buffer state
 * is saved between unddol and truedol.
 */
var	line	*addr1;			/* First addressed line in a command */
var	line	*addr2;			/* Second addressed line */
var	line	*dol;			/* Last line in buffer */
var	line	*dot;			/* Current line */
var	line	*one;			/* First line */
var	line	*truedol;		/* End of all lines, including saves */
var	line	*unddol;		/* End of undo saved lines */
var	line	*zero;			/* Points to empty slot before one */

/*
 * Undo information
 *
 * For most commands we save lines changed by salting them away between
 * dol and unddol before they are changed (i.e. we save the descriptors
 * into the temp file tfile which is never garbage collected).  The
 * lines put here go back after unddel, and to complete the undo
 * we delete the lines [undap1,undap2).
 *
 * Undoing a move is much easier and we treat this as a special case.
 * Similarly undoing a "put" is a special case for although there
 * are lines saved between dol and unddol we don't stick these back
 * into the buffer.
 */
var	short	undkind;

var	line	*unddel;	/* Saved deleted lines go after here */
var	line	*undap1;	/* Beginning of new lines */
var	line	*undap2;	/* New lines end before undap2 */
var	line	*undadot;	/* If we saved all lines, dot reverts here */

#define	UNDCHANGE	0
#define	UNDMOVE		1
#define	UNDALL		2
#define	UNDNONE		3
#define	UNDPUT		4

#ifdef CRYPT
/*
 * Various miscellaneous flags and buffers needed by the encryption routines.
 */
#define	KSIZE   9       /* key size for encryption */
var	int	xflag;		/* True if we are in encryption mode */
var	int	xtflag;		/* True if the temp file is being encrypted */
var	int	kflag;		/* True if the key has been accepted */
var	int	crflag;		/* True if the key has been accepted and the 
				 *  file being read is ciphertext 
				 */

#define SIZECBLOCK	768		 
var	char	perm[SIZECBLOCK];
var	char	tperm[SIZECBLOCK];
var	char	*key;
var	char	crbuf[CRSIZE];
char	*getpass();
#endif

var	bool	write_quit;	/* True if executing a 'wq' command */
var	int	errcnt;		/* number of error/warning messages in */
				/*	editing session (global flag)  */ 
/*
 * Function type definitions
 */
#define	NOSTR	(char *) 0
#define	NOLINE	(line *) 0

extern	int	(*Outchar)();
extern	int	(*Pline)();
extern	int	(*Putchar)();
var	void	(*oldhup)();
int	(*setlist())();
int	(*setnorm())();
int	(*setnorm())();
int	(*setnumb())();
char	*sbrk();
line	*address();
char	*cgoto();
char	*genindent();
char	*getblock();
char	*getenv();
line	*getmark();
char	*mesg();
char	*place();
char	*plural();
line	*scanfor();
line	*setin();
char	*strcat();
char	*strcpy();
char	*strend();
char	*tailpath();
char	*tgetstr();
char	*tgoto();
char	*ttyname();
line	*vback();
char	*vfindcol();
char	*vgetline();
char	*vinit();
char	*vpastwh();
char	*vskipwh();
int	put();
int	putreg();
int	YANKreg();
int	delete();
int	execlp();
int	vi_filter();
int	getfile();
int	getsub();
int	gettty();
int	join();
int	listchar();
off_t	lseek();
int	normchar();
int	normline();
int	numbline();
var	void	(*oldquit)();
void	onhup();
void	onintr();
void	onsusp();
int	putch();
int	shift();
int	termchar();
int	vfilter();
#ifdef CBREAK
void	vintr();
#endif
int	vputch();
int	vshftop();
int	yank();

/*
 * C doesn't have a (void) cast, so we have to fake it for lint's sake.
 */
#ifdef lint
#define	ignore(a)	Ignore((char *) (a))
#define	ignorf(a)	Ignorf((int (*) ()) (a))
#else
#define	ignore(a)	a
#define	ignorf(a)	a
#endif
