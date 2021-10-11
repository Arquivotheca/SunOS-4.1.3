/*	@(#)error.h 1.1 92/07/30 SMI; from UCB 5.1 5/31/85	*/

typedef	int	boolean;
#define	reg	register

#define	TRUE	1
#define	FALSE	0

#define	true	1
#define	false	0
/*
 *	Descriptors for the various languages we know about.
 *	If you touch these, also touch lang_table
 */
#define	INUNKNOWN	0
#define	INCPP	1
#define	INCC	2
#define	INAS	3
#define	INLD	4
#define	INLINT	5
#define	INF77	6
#define	INPI	7
#define	INPC	8
#define	INFRANZ	9
#define	INLISP	10
#define	INVAXIMA	11
#define	INRATFOR	12
#define	INLEX	13
#define	INYACC	14
#define	INAPL	15
#define	INMAKE	16
#define	INRI	17
#define	INTROFF	18
#define	INMOD2	19

extern	int	language;
/*
 *	We analyze each line in the error message file, and
 *	attempt to categorize it by type, as well as language.
 *	Here are the type descriptors.
 */
typedef	int	Errorclass;

#define	C_FIRST	0		/* first error category */
#define	C_UNKNOWN	0	/* must be zero */
#define	C_IGNORE	1	/* ignore the message; used for pi */
#define	C_SYNC		2	/* synchronization errors */
#define	C_DISCARD	3	/* touches dangerous files, so discard */
#define	C_NONSPEC	4	/* not specific to any file */
#define	C_THISFILE	5	/* specific to this file, but at no line */
#define	C_NULLED	6	/* refers to special func; so null */
#define	C_TRUE		7	/* fits into true error format */
#define	C_DUPL		8	/* sub class only; duplicated error message */
#define	C_LAST	9		/* last error category */

#define	SORTABLE(x)	(!(NOTSORTABLE(x)))
#define	NOTSORTABLE(x)	(x <= C_NONSPEC)
/*
 *	Resources to count and print out the error categories
 */
extern	char		*class_table[];
extern	int		class_count[];

#define	nunknown	class_count[C_UNKNOWN]
#define	nignore		class_count[C_IGNORE]
#define	nsyncerrors	class_count[C_SYNC]
#define	ndiscard	class_count[C_DISCARD]
#define	nnonspec	class_count[C_NONSPEC]
#define	nthisfile	class_count[C_THISFILE]
#define	nnulled		class_count[C_NULLED]
#define	ntrue		class_count[C_TRUE]
#define	ndupl		class_count[C_DUPL]

/* places to put the error complaints */

#define	TOTHEFILE	1	/* touch the file */
#define	TOSTDOUT	2	/* just print them out (ho-hum) */

FILE	*errorfile;	/* where error file comes from */
FILE	*queryfile;	/* where the query responses from the user come from*/

extern	char	*currentfilename;
extern	char	*processname;
extern	char	*scriptname;

extern	boolean	query;
extern	boolean	terse;
int	inquire();			/* inquire for yes/no */
/* 
 *	codes for inquire() to return
 */
#define	Q_NO	1			/* 'N' */
#define	Q_no	2			/* 'n' */
#define	Q_YES	3			/* 'Y' */
#define	Q_yes	4			/* 'y' */

int	probethisfile();
/*
 *	codes for probethisfile to return
 */
#define	F_NOTEXIST	1
#define	F_NOTREAD	2
#define	F_NOTWRITE	3
#define	F_TOUCHIT	4

/*
 *	Describes attributes about a language
 */
struct lang_desc{
	char	*lang_name;
	char	*lang_incomment;	/* one of the following defines */
	char	*lang_outcomment;	/* one of the following defines */
};
extern struct lang_desc lang_table[];

#define	CINCOMMENT	"/*###"
#define	COUTCOMMENT	"%%%*/\n"
#define	FINCOMMENT	"C###"
#define	FOUTCOMMENT	"%%%\n"
#define	NEWLINE		"%%%\n"
#define	PIINCOMMENT	"(*###"
#define	PIOUTCOMMENT	"%%%*)\n"
#define	LISPINCOMMENT	";###"
#define	ASINCOMMENT	"####"
#define	RIINCOMMENT	CINCOMMENT
#define	RIOUTCOMMENT	COUTCOMMENT
#define	TROFFINCOMMENT	".\\\"###"
#define	TROFFOUTCOMMENT	NEWLINE
#define	MOD2INCOMMENT	"(*###"
#define	MOD2OUTCOMMENT	"%%%*)\n"
/*
 *	Defines and resources for determing if a given line
 *	is to be discarded because it refers to a file not to
 *	be touched, or if the function reference is to a
 *	function the user doesn't want recorded.
 */
#define	IG_FILE1	"llib-lc"
#define	IG_FILE2	"llib-port"
#define	IG_FILE3	"/usr/lib/llib-lc"
#define	IG_FILE4	"/usr/lib/llib-port"

#define	ERRORNAME	"/.errorrc"
int	nignored;
char	**names_ignored;
/* 
 *	Structure definition for a full error
 */
typedef struct edesc	Edesc;
typedef	Edesc	*Eptr;

struct edesc{
	Eptr	error_next;		/*linked together*/
	int	error_lgtext;		/* how many on the right hand side*/
	char	**error_text;		/* the right hand side proper*/
	Errorclass	error_e_class;	/* error category of this error*/
	Errorclass	error_s_class;	/* sub descriptor of error_e_class*/
	int	error_language;		/* the language for this error*/
	int	error_position;		/* oridinal position */
	int	error_line;		/* discovered line number*/
	int	error_no;		/* sequence number on input */
};
/*
 *	Resources for the true errors
 */
extern	int	nerrors;
extern	Eptr	er_head;
extern	Eptr	*errors;	
/*
 *	Resources for each of the files mentioned
 */
extern	int	nfiles;
extern	Eptr	**files;	/* array of pointers into errors*/
boolean	*touchedfiles;			/* which files we touched */
/*
 *	The langauge the compilation is in, as intuited from
 *	the flavor of error messages analyzed.
 */
extern	int	langauge;
extern	char	*currentfilename;
/*
 *	Functional forwards
 */
char	*Calloc();
char	*strsave();
char	*clobberfirst();
char	lastchar();
char	firstchar();
char	next_lastchar();
char	**wordvsplice();
int	wordvcmp();
boolean	persperdexplode();
/*
 *	Printing hacks
 */
char	*plural(), *verbform();
