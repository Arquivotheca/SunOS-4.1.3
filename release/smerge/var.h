typedef struct varblock *VARBLOCK;

struct varblock
{
	VARBLOCK nextvar;
	char	*varname,
			*varval;
	int noreset:1;
	int used:1;
	int envflg:1;
	int v_aflg:1;
};

VARBLOCK getfirstvar();
VARBLOCK varptr();
VARBLOCK srchvar();
extern char Nullstr[];

#define YES	1
#define NO	0
#define ALLOC(x) (struct x *) intalloc(sizeof(struct x))


extern int	Mflags;

#define TURNON(a)	(Mflags |= (a))
#define TURNOFF(a)	(Mflags &= (~(a)))
#define IS_ON(a)	(Mflags&(a))
#define IS_OFF(a)	(!(IS_ON(a)))

#define DBUG	0000001		/* debug flag */
#define ENVOVER	0000002		/* environ overides file defines */
#define EXPORT	0000004		/* put current variable in environ */
/* #define PRTR	0000010		/* set `-p' flag */
#define SIL	0000020		/* set `-s' flag */
#define NOEX	0000040		/* set `-n' flag */
#define INTRULE	0000100		/* use internal rules */
/* #define TOUCH	0000200		/* set `-t' flag */
/* #define GET	0000400		/* do a $(GET) if file not found */
#define QUEST	0001000		/* set `-q' flag */
#define INARGS	0002000		/* currently reading cmd args */
#define IGNERR	0004000		/* set `-i' flag */
#define KEEPGO	0010000		/* set `-k' flag */
/* #define GF_KEEP	0020000		/* keep auto get files */
/* #define MH_DEP	0040000		/* use old question about whether cmd exists */
/* #define MEMMAP	0100000		/* print memory map */

typedef struct chain *CHAIN;

struct chain
{
	CHAIN nextchain;
	char	*datap;
};

typedef struct nameblock *NAMEBLOCK;

struct nameblock
{
	NAMEBLOCK nextname;		/* pointer to next nameblock */
	NAMEBLOCK backname;		/* pointer to predecessor */
	char	*namep;		/* ASCII name string */
	short	 namelen;	/* name length to speed lookup */
	char	*alias;		/* ASCII alias (when namep translates to another
				 * pathstring.
				 */
/*	LINEBLOCK linep; /* pointer to dependents */
/*	int done:3;		/* flag used to tell when finished */
/*	int septype:3;		/* distinguishes between single and double : */
/*	int rundep:1;		/* flag indicating runtime translation done */
/*	TIMETYPE modtime;	/* set by exists() */
};


#define equal(a,b)	(a[0] == b[0] ? !strcmp((a),(b)) : NO )
#define any(a,b)	strchr( (a), (b) )
