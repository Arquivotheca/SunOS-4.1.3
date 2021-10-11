/*	@(#)sh.h 1.1 92/07/30 SMI; from UCB 5.3 3/29/86	*/

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <errno.h>
#include <setjmp.h>
#include <vfork.h>
#include <stdlib.h>	/* MB_xxx, mbxxx(), wcxxx() etc. */
#include <malloc.h>
#include "sh.local.h"
#include "sh.char.h"

#ifdef	MBCHAR
# if !defined(MB_LEN_MAX) || !defined(MB_CUR_MAX)
	Error: I need both ANSI macros!
# endif
#else
# if !defined(MB_LEN_MAX)
#  define MB_LEN_MAX	1
# endif
# if !defined(MB_CUR_MAX)
#  define MB_CUR_MAX	1
# endif
#endif

#ifndef	MBCHAR /* Let's replace the ANSI functions with our own macro
		* for efficiency!
		*/
#define	mbtowc(pwc, pmb, n_is_ignored)	((*(pwc)=*(pmb)), 1)
#define	wctomb(pmb, wc)			((*(pmb)=((char)wc)), 1)
#endif/*!MBCHAR*/

/*
 * C shell
 *
 * Bill Joy, UC Berkeley
 * October, 1978; May 1980
 *
 * Jim Kulp, IIASA, Laxenburg Austria
 * April, 1980
 */

#define	isdir(d)	((d.st_mode & S_IFMT) == S_IFDIR)

typedef	char	bool;

/* tchar (Tagged CHARacter) is a place holder to keep a QUOTE bit and
 * a character.
 * For European language handling, lower 8 bits of tchar is used
 * to store a character.  For other languages, especially Asian, 16 bits
 * are used to store a character.
 * Following typedef's assume short int is a 16-bit entity and long int is
 * a 32-bit entity.
 * The QUOTE bit tells whether the character is subject to further
 * interpretation such as history substitution, file mathing, command
 * subsitution.  TRIM is a mask to strip off the QUOTE bit.
 */
#ifdef	MBCHAR		/* For multibyte character handling. */
typedef	long int	tchar;
#define	QUOTE	0x80000000
#define	TRIM	0x0000ffff
#else/*!MBCHAR*/	/* European language requires only 8 bits. */
typedef	unsigned short int	tchar;
#define	QUOTE	0x8000
#define	TRIM	0x00ff
#endif/*!MBCHAR*/
#define	eq(a, b)	(strcmp_(a, b) == 0)

/*
 * Global flags
 */
bool	chkstop;		/* Warned of stopped jobs... allow exit */
bool	didfds;			/* Have setup i/o fd's for child */
bool	doneinp;		/* EOF indicator after reset from readc */
bool	exiterr;		/* Exit if error or non-zero exit status */
bool	child;			/* Child shell ... errors cause exit */
bool	haderr;			/* Reset was because of an error */
bool	intty;			/* Input is a tty */
bool	intact;			/* We are interactive... therefore prompt */
bool	justpr;			/* Just print because of :p hist mod */
bool	loginsh;		/* We are a loginsh -> .login/.logout */
bool	neednote;		/* Need to pnotify() */
bool	noexec;			/* Don't execute, just syntax check */
bool	pjobs;			/* want to print jobs if interrupted */
bool	setintr;		/* Set interrupts on/off -> Wait intr... */
bool	timflg;			/* Time the next waited for command */
bool	havhash;		/* path hashing is available */
#ifdef	FILEC
bool	filec;			/* doing filename expansion */
#endif

/*
 * Global i/o info
 */
tchar	*arginp;		/* Argument input for sh -c and internal `xx` */
int	onelflg;		/* 2 -> need line for -t, 1 -> exit on read */
tchar	*file;			/* Name of shell file for $0 */

char	*err;			/* Error message from scanner/parser */
int	errno;			/* Error from C library routines */
tchar	*shtemp;		/* Temp name for << shell files in /tmp */
struct	timeval time0;		/* Time at which the shell started */
struct	rusage ru0;

/*
 * Miscellany
 */
tchar	*doldol;		/* Character pid for $$ */
int	uid;			/* Invokers uid */
time_t	chktim;			/* Time mail last checked */
int	shpgrp;			/* Pgrp of shell */
int	tpgrp;			/* Terminal process group */
/* If tpgrp is -1, leave tty alone! */
int	opgrp;			/* Initial pgrp and tty pgrp */
int	oldisc;			/* Initial line discipline or -1 */

/*
 * These are declared here because they want to be
 * initialized in sh.init.c (to allow them to be made readonly)
 */

extern struct	biltins {
	tchar	*bname;
	int	(*bfunct)();
	short	minargs, maxargs;
} bfunc[];
extern int nbfunc;

extern struct srch {
	tchar	*s_name;
	short	s_value;
} srchn[];
extern int nsrchn;

/*
 * To be able to redirect i/o for builtins easily, the shell moves the i/o
 * descriptors it uses away from 0,1,2.
 * Ideally these should be in units which are closed across exec's
 * (this saves work) but for version 6, this is not usually possible.
 * The desired initial values for these descriptors are defined in
 * sh.local.h.
 */
short	SHIN;			/* Current shell input (script) */
short	SHOUT;			/* Shell output */
short	SHDIAG;			/* Diagnostic output... shell errs go here */
short	OLDSTD;			/* Old standard input (def for cmds) */

/*
 * Error control
 *
 * Errors in scanning and parsing set up an error message to be printed
 * at the end and complete.  Other errors always cause a reset.
 * Because of source commands and .cshrc we need nested error catches.
 */

jmp_buf	reslab;

#define	setexit()	((void) setjmp(reslab))
#define	reset()		longjmp(reslab, 0)
	/* Should use structure assignment here */
#define	getexit(a)	copy((void *)(a), (void *)reslab, sizeof reslab)
#define	resexit(a)	copy((void *)reslab, ((void *)(a)), sizeof reslab)

tchar	*gointr;		/* Label for an onintr transfer */
void	(*parintr)();		/* Parents interrupt catch */
void	(*parterm)();		/* Parents terminate catch */


/*
 * Each level of input has a buffered input structure.
 * There are one or more blocks of buffered input for each level,
 * exactly one if the input is seekable and tell is available.
 * In other cases, the shell buffers enough blocks to keep all loops
 * in the buffer.
 */
struct	Bin {
	off_t	Bfseekp;		/* Seek pointer */
	off_t	Bfbobp;			/* Seekp of beginning of buffers */
	off_t	Bfeobp;			/* Seekp of end of buffers */
	short	Bfblocks;		/* Number of buffer blocks */
	tchar	**Bfbuf;		/* The array of buffer blocks */
} B;

#define	fseekp	B.Bfseekp
#define	fbobp	B.Bfbobp
#define	feobp	B.Bfeobp
#define	fblocks	B.Bfblocks
#define	fbuf	B.Bfbuf

#define	btell()	fseekp

#ifndef	btell
off_t	btell();
#endif

/*
 * The shell finds commands in loops by reseeking the input
 * For whiles, in particular, it reseeks to the beginning of the
 * line the while was on; hence the while placement restrictions.
 */
off_t	lineloc;

#ifdef	TELL
bool	cantell;			/* Is current source tellable ? */
#endif

/*
 * Input lines are parsed into doubly linked circular
 * lists of words of the following form.
 */
struct	wordent {
	tchar	*word;
	struct	wordent *prev;
	struct	wordent *next;
};

/*
 * During word building, both in the initial lexical phase and
 * when expanding $ variable substitutions, expansion by `!' and `$'
 * must be inhibited when reading ahead in routines which are themselves
 * processing `!' and `$' expansion or after characters such as `\' or in
 * quotations.  The following flags are passed to the getC routines
 * telling them which of these substitutions are appropriate for the
 * next character to be returned.
 */
#define	DODOL	1
#define	DOEXCL	2
#define	DOALL	DODOL|DOEXCL

/*
 * Labuf implements a general buffer for lookahead during lexical operations.
 * Text which is to be placed in the input stream can be stuck here.
 * We stick parsed ahead $ constructs during initial input,
 * process id's from `$$', and modified variable values (from qualifiers
 * during expansion in sh.dol.c) here.
 */
tchar	labuf[BUFSIZ];

tchar	*lap;

/*
 * Parser structure
 *
 * Each command is parsed to a tree of command structures and
 * flags are set bottom up during this process, to be propagated down
 * as needed during the semantics/exeuction pass (sh.sem.c).
 */
struct	command {
	short	t_dtyp;				/* Type of node */
	short	t_dflg;				/* Flags, e.g. FAND|... */
	union {
		tchar	*T_dlef;		/* Input redirect word */
		struct	command *T_dcar;	/* Left part of list/pipe */
	} L;
	union {
		tchar	*T_drit;		/* Output redirect word */
		struct	command *T_dcdr;	/* Right part of list/pipe */
	} R;
#define	t_dlef	L.T_dlef
#define	t_dcar	L.T_dcar
#define	t_drit	R.T_drit
#define	t_dcdr	R.T_dcdr
	tchar	**t_dcom;			/* Command/argument vector */
	char	*cfname;			/* char pathname for execv */
	char	**cargs;			/* char arg vec  for execv */
	struct	command *t_dspr;		/* Pointer to ()'d subtree */
	short	t_nice;
};

#define	TCOM	1		/* t_dcom <t_dlef >t_drit	*/
#define	TPAR	2		/* ( t_dspr ) <t_dlef >t_drit	*/
#define	TFIL	3		/* t_dlef | t_drit		*/
#define	TLST	4		/* t_dlef ; t_drit		*/
#define	TOR	5		/* t_dlef || t_drit		*/
#define	TAND	6		/* t_dlef && t_drit		*/

#define	FSAVE	(FNICE|FTIME|FNOHUP)	/* save these when re-doing */

#define	FAND	(1<<0)		/* executes in background	*/
#define	FCAT	(1<<1)		/* output is redirected >>	*/
#define	FPIN	(1<<2)		/* input is a pipe		*/
#define	FPOU	(1<<3)		/* output is a pipe		*/
#define	FPAR	(1<<4)		/* don't fork, last ()ized cmd	*/
#define	FINT	(1<<5)		/* should be immune from intr's */
/* spare */
#define	FDIAG	(1<<7)		/* redirect unit 2 with unit 1	*/
#define	FANY	(1<<8)		/* output was !			*/
#define	FHERE	(1<<9)		/* input redirection is <<	*/
#define	FREDO	(1<<10)		/* reexec aft if, repeat,...	*/
#define	FNICE	(1<<11)		/* t_nice is meaningful */
#define	FNOHUP	(1<<12)		/* nohup this command */
#define	FTIME	(1<<13)		/* time this command */

/*
 * The keywords for the parser
 */
#define	ZBREAK		0
#define	ZBRKSW		1
#define	ZCASE		2
#define	ZDEFAULT 	3
#define	ZELSE		4
#define	ZEND		5
#define	ZENDIF		6
#define	ZENDSW		7
#define	ZEXIT		8
#define	ZFOREACH	9
#define	ZGOTO		10
#define	ZIF		11
#define	ZLABEL		12
#define	ZLET		13
#define	ZSET		14
#define	ZSWITCH		15
#define	ZTEST		16
#define	ZTHEN		17
#define	ZWHILE		18

/*
 * Structure defining the existing while/foreach loops at this
 * source level.  Loops are implemented by seeking back in the
 * input.  For foreach (fe), the word list is attached here.
 */
struct	whyle {
	off_t	w_start;		/* Point to restart loop */
	off_t	w_end;			/* End of loop (0 if unknown) */
	tchar	**w_fe, **w_fe0;	/* Current/initial wordlist for fe */
	tchar	*w_fename;		/* Name for fe */
	struct	whyle *w_next;		/* Next (more outer) loop */
} *whyles;

/*
 * Variable structure
 *
 * Aliases and variables are stored in AVL balanced binary trees.
 */
struct	varent {
	tchar	**vec;		/* Array of words which is the value */
	tchar	*v_name;	/* Name of variable/alias */
	struct	varent *v_link[3];	/* The links, see below */
	int	v_bal;		/* Balance factor */
} shvhed, aliases;
#define	v_left		v_link[0]
#define	v_right		v_link[1]
#define	v_parent	v_link[2]

struct	varent *adrof1();
#define	adrof(v)	adrof1(v, &shvhed)
#define	value(v)	value1(v, &shvhed)

/*
 * The following are for interfacing redo substitution in
 * aliases to the lexical routines.
 */
struct	wordent *alhistp;		/* Argument list (first) */
struct	wordent *alhistt;		/* Node after last in arg list */
tchar	**alvec;			/* The (remnants of) alias vector */

/*
 * Filename/command name expansion variables
 */
short	gflag;				/* After tglob -> is globbing needed? */

/*
 * A reasonable limit on number of arguments would seem to be
 * the maximum number of characters in an arg list / 6.
 *
 * XXX:	With the new VM system, NCARGS has become enormous, making
 *	it impractical to allocate arrays with NCARGS / 6 entries on
 *	the stack.  The proper fix is to revamp code elsewhere (in
 *	sh.dol.c and sh.glob.c) to use a different technique for handling
 *	command line arguments.  In the meantime, we simply fall back
 *	on using the old value of NCARGS.
 */
#ifdef	notyet
#define	GAVSIZ	(NCARGS / 6)
#else	notyet
#define	GAVSIZ	(10240 / 6)
#endif	notyet

/*
 * Variables for filename expansion
 */
tchar	**gargv;			/* Pointer to the (stack) arglist */
long	gargc;				/* Number args in gargv */
long	gnleft;

/*
 * Variables for command expansion.
 */
tchar	**pargv;			/* Pointer to the argv list space */
tchar	*pargs;				/* Pointer to start current word */
long	pargc;				/* Count of arguments in pargv */
long	pnleft;				/* Number of chars left in pargs */
tchar	*pargcp;			/* Current index into pargs */

/*
 * History list
 *
 * Each history list entry contains an embedded wordlist
 * from the scanner, a number for the event, and a reference count
 * to aid in discarding old entries.
 *
 * Essentially "invisible" entries are put on the history list
 * when history substitution includes modifiers, and thrown away
 * at the next discarding since their event numbers are very negative.
 */
struct	Hist {
	struct	wordent Hlex;
	int	Hnum;
	int	Href;
	struct	Hist *Hnext;
} Histlist;

struct	wordent	paraml;			/* Current lexical word list */
int	eventno;			/* Next events number */
int	lastev;				/* Last event reference (default) */

tchar	HIST;				/* history invocation character */
tchar	HISTSUB;			/* auto-substitute character */

/*
 * In lines for frequently called functions
 */
#define	XFREE(cp) { \
	extern char end[]; \
	char stack; \
/*??*/	if (((char *)(cp)) >= end && ((char *)(cp)) < &stack) \
/*??*/		free((void *)cp); \
}
void	*alloctmp;
#define	xalloc(i) ((alloctmp = (void *)malloc(i)) ? alloctmp : (void *)nomem(i))/*??*/

tchar	*Dfix1();
tchar	**blkcat();
tchar	**blkcpy();
tchar	**blkend();
tchar	**blkspl();
char	**blkspl_();
tchar	*cname();
tchar	**copyblk();
tchar	**dobackp();
tchar	*domod();
struct	wordent *dosub();
tchar	*exp3();
tchar	*exp3a();
tchar	*exp4();
tchar	*exp5();
tchar	*exp6();
struct	Hist *enthist();
struct	Hist *findev();
struct	wordent *freenod();
char	*getenv();
tchar	*getenv_(/* tchar * */);
tchar	*getenvs_(/* char * */);
tchar	*getinx();
struct	varent *getvx();
struct	passwd *getpwnam();
struct	wordent *gethent();
struct	wordent *getsub();
char	*getwd();
tchar	*getwd_();
tchar	**glob();
tchar	*globone();
char	*index();
tchar	*index_();
struct	biltins *isbfunc();
off_t	lseek();
tchar	*operate();
void	phup();
void	pintr();
void	pchild();
tchar	*putn();
char	*rindex();
tchar	*rindex_();
tchar	**saveblk();
tchar	*savestr();
char	*strcat();
tchar	*strcat_();
char	*strcpy();
tchar	*strcpy_();
tchar	*strend();
tchar	*strip();
tchar	*strspl();
tchar	*subword();
struct	command *syntax();
struct	command *syn0();
struct	command *syn1();
struct	command *syn1a();
struct	command *syn1b();
struct	command *syn2();
struct	command *syn3();
tchar	*value1();
tchar	*xhome();
tchar	*xname();
tchar	*xset();

#define	NOSTR	((tchar *) 0)

/*
 * setname is a macro to save space (see sh.err.c)
 */
tchar	*bname;
#define	setname(a)	(bname = (a))

#ifdef	VFORK
tchar	*Vsav;
tchar	**Vav;
tchar	*Vdp;
#endif

tchar	**evalvec;
tchar	*evalp;

struct	mesg {
	tchar	*iname;		/* signal name from /usr/include */
	char	*pname;		/* print name */
} mesg[];

/* Conversion functions between char and tchar strings. */
tchar	*strtots(/* tchar * , char * */);
char	*tstostr(/* char *  , tchar * */);

#ifndef	NULL
#define	NULL	0
#endif
