/*	@(#)lpass2.h 1.1 92/07/30 SMI; from S5R2 1.3	*/

# if pdp11
#	define FSZ 80
# else
#	define FSZ 900		/* max # file names (used to be 80) */
# endif

typedef struct sty STYPE;
struct sty { ATYPE t; STYPE *next; };

typedef struct sym {
	char *name;
	short nargs;
	int decflag;
	int fline;
	STYPE symty;
	int fno;
	int mno;
	int use;
	short more;
	} SYMTAB;

