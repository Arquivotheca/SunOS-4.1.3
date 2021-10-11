/*	@(#)defs 1.7 86/03/30 SMI; from S5R2 1.2 03/28/83	*/

#include <stdio.h>
#include <sys/param.h>
#include <sys/dir.h>

/*
 *	Flags
 */

typedef struct chain		*CHAIN;
typedef struct nameblock	*NAMEBLOCK;
typedef struct shblock		*SHBLOCK;
typedef struct stateblock	*STATEBLOCK;
typedef struct varblock		*VARBLOCK;

#define IS_OFF(a)	(!(IS_ON(a)))
#define IS_ON(a)	(Mflags&(a))
#define TURNOFF(a)	(Mflags &= (~(a)))
#define TURNON(a)	(Mflags |= (a))

#define DBUG	0000001		/* debug flag */
#define ENVOVER	0000002		/* environ overides file defines */
#define EXPORT	0000004		/* put current variable in environ */
#define PRTR	0000010		/* set `-p' flag */
#define SIL	0000020		/* set `-s' flag */
#define NOEX	0000040		/* set `-n' flag */
#define INTRULE	0000100		/* use internal rules */
#define QUEST	0001000		/* set `-q' flag */
#define INARGS	0002000		/* currently reading cmd args */
#define IGNERR	0004000		/* set `-i' flag */
#define KEEPGO	0010000		/* set `-k' flag */
#define MH_DEP	0040000		/* use old question about whether cmd exists */

extern int	Mflags;

#define YES 1
#define NO 0

#define any(a,b)	strchr( (a), (b) )
#define equal(a,b)	(a[0] == b[0] ? !strcmp((a),(b)) : NO )
#define max(a,b)	((a)>(b)?(a):(b))
#define ALLOC(x) (struct x *) intalloc(sizeof(struct x))

struct stateblock {
	STATEBLOCK	next,
			prev;
	char		*name;
	NAMEBLOCK	othercommands;
	SHBLOCK		commands;
};

struct shblock
{
	SHBLOCK nextsh;
	short	flags;
	char	*shbp;		/* shell command */
};
#define SH_NOECHO	1	/* do not echo this command */
#define SH_NOSTOP	2	/* do not stop if command prduces error */

struct varblock
{
	VARBLOCK nextvar;
	char	*varname,
		*varval;

	int	noreset:1,
		used:1,
		envflg:1,
		v_aflg:1;
};

struct chain
{
	CHAIN nextchain;
	char	*datap;
};


struct nameblock
{
	NAMEBLOCK nextname;	/* pointer to next nameblock */
	NAMEBLOCK backname;	/* pointer to predecessor */
	char	*namep;		/* ASCII name string */
	short	 namelen;	/* name length to speed lookup */
};

int	*intalloc();
NAMEBLOCK
	srchname(),
	makename();
VARBLOCK
	getfirstvar(),
	srchvar(),
	varptr();

char	*calloc(),
	*colontrans(),
	*concat(),
	*copstr(),
	*copys(),
	*dftrans(),
	*findfl(),
	*mkqlist(),
	*straightrans(),
	*strchr(),
	*strrchr();
STATEBLOCK	findstate();

extern int	(*sigivalue)();
extern int	(*sigqvalue)();

void	addstars(),
	strshift(),
	setvar();

extern int wait_pid;
extern int ndocoms;
extern int okdel;
extern NAMEBLOCK firstname;
extern STATEBLOCK	firststate;

#define OUTMAX 10240

#define Smergeflags "SMERGEFLAGS"
