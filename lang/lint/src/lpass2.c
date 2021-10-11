#ifndef lint
static	char sccsid[] = "@(#)lpass2.c 1.1 92/07/30 SMI"; /* from S5R2 1.8 */
#endif

#ident "@(#)RELEASE lint2 4.1"

/* lpass2.c
 *	This file includes the main functions for the second lint pass.
 *
 *	Functions:
 *	==========
 *		chkcompat	check for various sorts of compatibility
 *		chktype		check two types to see if they are compatible
 *		cleanup		do wrapup checking
 *		find		find name in symbol table
 *		lastone		check set/use in a last pass over the symbol table
 *		lread		read in a line from the intermediate file
 *		main		the driver for the second pass (lint2)
 *		mloop		the main loop of the second pass
 *		setfno		set file number (keep track of source file names)
 *		setuse		fix set/use and other information for objects
 *		tget
 */

# include <stdio.h>
# include "types.h"

extern char *strncpy();

/*
 * local aliases
 */

# define TWORD PCC_TWORD

# include "lerror.h"
# include "lmanifest.h"
# include "lpass2.h"

# define USED 01
# define VUSED 02
# define EUSED 04
# define RVAL 010
# define VARARGS 0100

# define NSZ 8192
# if pdp11
#	define TYSZ 2500
# else
#	define TYSZ 3500	/* should be 2500 temp fix for mjs */
# endif

# define NTY 50

SYMTAB stab[NSZ];			/* second pass symbol table */
SYMTAB *find();

STYPE *tptr = NULL;
STYPE *tget();

typedef char *filename;

filename *fnm = NULL;	/* varying-length array of filenames */
int fnmlimit = 0;	/* current upper bound of fnm table */
static void init_fnm();
static void new_fnm();
extern char *realloc();

char *getstr();

char *hash();

int tfree;  /* used to allocate types */
int ffree;  /* used to save filenames */

struct ty atyp[NTY];
union rec r;			/* where all the input ends up */

TWORD ctype();

int hflag = 1;			/* 28 feb 80  reverse sense of hflag */
int pflag = 0;
int qflag = 0;
int xflag = 1;			/* 28 feb 80  reverse sense of xflag */
int uflag = 1;
int zflag = 0;
int sys5flag = 1;

static int idebug = 0;
static int odebug = 0;

int cfno;				/* current file number */
int cmno;				/* current module number */

char		*ifilename = NULL;
/* main program for second pass */

main( argc, argv ) char *argv[]; 
{
	int c;
	extern char	*htmpname;
	extern char *optarg;
	int Hflag = 0, Tflag = 0;

	/*
	 * If our first argument isn't an option, we're being invoked
	 * as the pre-S3 "lint".  Use that argument as "ifilename",
	 * delete it from the middle of the argument list, and turn
	 * off the S5 flag.
	 */
	if( argc > 1 && argv[1][0] != '-' )
	{
		ifilename = argv[1];
		Tflag = 1;
		argv[1] = argv[0];
		argv++;
		argc--;
		sys5flag = 0;
	}
	while((c=getopt(argc,argv,"abchnpquvxzH:LT:X:")) != EOF)
		switch(c) {
		case 'h':
			hflag = 0;
			break;

		case 'p':
			pflag = 1;
			break;

		case 'q':
			qflag = 1;
			break;

		case 'x':
			xflag = 0;
			break;

		case 'u':
			uflag = 0;
			break;

		case 'z':
			zflag = 1;
			break;

		case 'H':
			htmpname = optarg;
			Hflag = 1;
			break;

		case 'T':
			ifilename = optarg;
			Tflag = 1;
			break;

			/* debugging options */
		case 'X':
			for(; *optarg; optarg++)
				switch(*optarg) {
				case 'i':
					idebug = 1;
					break;
				case 'o':
					odebug = 1;
					break;
				} /*end switch(*optarg), for*/
			break;

		case 'a':		/* first pass option */
		case 'b':		/* first pass option */
		case 'c':		/* first pass option */
		case 'n':		/* done in shell script */
		case 'v':		/* first pass option */
		case 'L':		/* first pass option */
			break;
		} /*end switch for processing args*/

	/*
	 * Decide whether you're S5 or pre-S3, and patch up if pre-S3.
	 * "Patching up" involves the following:
	 * - toggling the values of hflag and xflag
	 * - forcing msgs to come out one per line, instead of
	 *	being nicely formatted as is the S5 default
	 */
	if( !sys5flag )
	{
		hflag = !hflag;
		xflag = !xflag;
	}
	tmpopen( );
	if ( Hflag )
		unbuffer( );
	if ( !Tflag )
	{
		lerror( "", CCLOSE | HCLOSE );	/* all done */
		return (0);
	}
	if ( !freopen( ifilename, "r", stdin ) )
		lerror( "cannot open intermediate file", FATAL | CCLOSE | ERRMSG );

	if ( idebug ) pif();
	init_fnm();
	mloop( LDI|LDS|LIB );
	rewind( stdin );
	mloop( LDC|LDX );
	rewind( stdin );
	mloop( LRV|LUV|LUE|LUM );
	if ( odebug ) pst();
	cleanup();
	un2buffer( );
	return(0);
} /*end main*/
/* mloop - main loop
 *	each pass of the main loop reads in names from the intermediate 
 *	file that have characteristics which overlap with the
 *	characteristics specified as the parameter
 */
mloop( m )
{
	register SYMTAB *q;

	while( lread(m) ){
		q = find();
		if( q->decflag ) chkcompat(q);
		else setuse(q);
	}
}
/* lread - read a line from intermediate file into r.l */
lread(m)
{
	register n;

	for(;;) {
		/* read in line from intermediate file; exit if at end */
		if( fread( (char *)&r, sizeof(r), 1, stdin ) <= 0 ) return(0);
		if( r.l.decflag & LFN ){
			/* new filename and module number */
			r.f.fn = getstr();
			setfno( r.f.fn );
			/* the purpose of the module number is to handle static scoping
			 *	correctly.  A module is a file with all its include files.
			 *	From a scoping point of view, there is no difference between
			 *	a variable in a file and a variable in an included file.
			 *	The module number itself is not meaningful; it must only
			 *	be unique to ensure that modules are distinguishable
			 */
			cmno = r.f.mno;
			continue;
		}
		/* number of arguments is negative for VARARGS (plus one) */
		r.l.name = getstr();
		n = r.l.nargs;
		if( n<0 ) n = -n - 1;
		/* collect type info for all args */
		if( n ) {
			if ( n > sizeof atyp / sizeof (ATYPE) ) {
				lerror( "invalid number of arguments", 
				    CCLOSE | FATAL | ERRMSG );
			}
			fread( (char *)atyp, sizeof(ATYPE), n, stdin );
		}
		/* return with entry only if it has correct characteristics */
		if( ( r.l.decflag & m ) ) return( 1 );
	}
}
/* setfno - set file number */
setfno( s ) char *s;
{
  int i;
	/* look up filename */
	for( i=0; i<ffree; ++i )
		if( fnm[i] == s ){
			cfno = i;
			return;
		}
	/* make a new entry */
	new_fnm();
	fnm[ffree] = s;
	cfno = ffree++;
}

static void
init_fnm()
{
	fnm = (filename*)malloc((unsigned)FSZ*sizeof(filename));
	if (fnm == NULL) {
		lerror("cannot allocate storage for file name table");
	}
	fnmlimit = FSZ;
}

static void
new_fnm()
{
	if (ffree >= fnmlimit) {
		fnmlimit += FSZ;
		fnm = (filename*)realloc(fnm, fnmlimit*sizeof(filename));
		if (fnm == NULL) {
			lerror("cannot allocate storage for file name table");
		}
	}
}
/* find - find object in the symbol table */
SYMTAB *
find()
{
  register h=0;
	/* hashing scheme */
	/* for this to work, NSZ should be a power of 2 */
	h = ( (int) r.l.name ) % NSZ;
	/* symbol table search */
	/* 21 July 1980 - scoping added to symbol table search */
	{  register SYMTAB *p, *q, *maybe = 0;
		for( p=q= &stab[h]; q->decflag; ){
			/* find a name that matches */
			if ( r.l.name == q->name )  {
				/* match if module id-s are the same */
				if (q->mno == cmno) return(q);
				/* otherwise, possible match if neither is static */
				if( !( q->decflag&LDS || r.l.decflag&LDS ) ) maybe = q;
				/* else there must be another stab entry with same name
				 * (if there is NOT, then we will add an entry) */
				else if (!maybe) q->more = 1;
				/* at end of chain - so use global name */
				if( !(q->more) && maybe ) return(maybe);
			}
			/* no match, so continue looking */
			if( ++q >= &stab[NSZ] ) q = stab;
			if( q == p )
				lerror( "too many names defined", CCLOSE | FATAL | ERRMSG );
		}
		q->name = r.l.name;
		/* if we are inserting, the new entry is the last in the "chain" */
		q->more = 0;
		return( q );
	}
}
/* tget */
STYPE *
tget()
{
	if( tfree == 0 )
	{
		/*
		 * Allocate a fresh batch of TYSZ type entries.
		 */
		if (( tptr = (STYPE *)malloc(TYSZ*sizeof (STYPE))) == NULL )
			lerror( "can't allocate memory for type table", CCLOSE | FATAL | ERRMSG );
		tfree = TYSZ;
	}
	tfree--;
	return( tptr++ );
}
/* chkcompat - check for compatibility
 *	compare item in symbol table (stab) with item just read in
 *	The work of this routine is not well defined; there are many
 *	checks that might be added or changes
 */

chkcompat(q) SYMTAB *q; 
{
	register int i;
	STYPE *qq;

	setuse(q);

	/* do an argument check only if item is a function; if it is
	*  both a function and a variable, it will get caught later on
	*/

	if( ISFTN(r.l.type.aty) && ISFTN(q->symty.t.aty) )

		/* argument check */

		if( q->decflag & (LDI|LDS|LIB|LUV|LUE) )
			if( r.l.decflag & (LUV|LIB|LUE) ){
				if( q->nargs != r.l.nargs ){
					if( !(q->use&VARARGS) )
						/* "%.8s: variable # of args." */
						/* "%s: variable # of args." */
						buffer( 7, q );
					if( r.l.nargs > q->nargs ) r.l.nargs = q->nargs;
					if( !(q->decflag & (LDI|LDS|LIB) ) ) {
						q->nargs = r.l.nargs;
						q->use |= VARARGS;
					}
				}
				for( i=0,qq=q->symty.next; i<r.l.nargs; ++i,qq=qq->next)
					if( chktype( &qq->t, &atyp[i] ) )
						/* "%.8s, arg. %d used inconsistently" */
						/* "%s, arg. %d used inconsistently" */
						buffer( 6, q, i+1 );
			}

	if( (q->decflag&(LDI|LDS|LIB|LUV)) && r.l.decflag==LUV )
		if( chktype( &r.l.type, &q->symty.t ) )
			/* "%.8s value used inconsistently" */
			/* "%s value used inconsistently" */
			buffer( 4, q );

	/* check for multiple declaration */

	if( (q->decflag & (LDI|LDS) ) && (r.l.decflag&(LDI|LDS|LIB)) )
		/* "%.8s multiply declared" */
		/* "%s multiply declared" */
		buffer( 3, q );

	/* do a bit of checking of definitions and uses... */

	if( (q->decflag & (LDI|LDS|LIB|LDX|LDC|LUM))
	    && (r.l.decflag & (LDX|LDC|LUM))
	    && ctype(q->symty.t.aty) != ctype(r.l.type.aty) )
		/* "%.8s value type declared inconsistently" */
		/* "%s value type declared inconsistently" */
		buffer( 5, q );

	/* better not call functions which are declared to be
		structure or union returning */

	if( (q->decflag & (LDI|LDS|LIB|LDX|LDC))
	    && (r.l.decflag & LUE)
	    && q->symty.t.aty != r.l.type.aty ){
		/* only matters if the function returns union or structure */
		TWORD ty;
		ty = q->symty.t.aty;
		if( ISFTN(ty) && ((ty = DECREF(ty))==STRTY || ty==UNIONTY ) )
			/* "%.8s function value type must be declared before use" */
			/* "%s function value type must be declared before use" */
			buffer( 8, q );
	}
} /*end chkcompat*/

/* messages for defintion/use */
int	mess[2][2] = {
	NUM2MSGS,
	0,
	1,
	2
};

lastone(q) SYMTAB *q; 
{
	register nu, nd, uses;
	nu = nd = 0;
	uses = q->use;

	if( !(uses&USED) && q->decflag != LIB )
		if( strcmp(q->name,"main") ) nu = 1;

	/* treat functions and variables the same */
	switch( q->decflag ){

	case LIB:
		nu = nd = 0;  /* don't complain about uses on libraries */
		break;
	case LDX:
		if( !xflag ) break;
	case LUV:
	case LUE:
/* 01/04/80 */	case LUV | LUE:
/* or'ed together for void */
	case LUM:
		nd = 1;
	}
	if( uflag && ( nu || nd ) ) buffer( mess[nu][nd], q );

	if( (uses&(RVAL+EUSED)) == (RVAL+EUSED) ){
		if ( uses & VUSED ) 
			/* "%.8s returns value which is sometimes ignored\n" */
			/* "%s returns value which is sometimes ignored\n" */
			buffer( 11, q );
		else 
			/* "%.8s returns value which is always ignored\n" */
			/* "%s returns value which is always ignored\n" */
			buffer( 10, q );
	}

	if( (uses&(RVAL+VUSED)) == (VUSED) && (q->decflag&(LDI|LDS|LIB)) )
		/* "%.8s value is used, but none returned\n" */
		/* "%s value is used, but none returned\n" */
		buffer( 9, q );
}
/* cleanup - call lastone and die gracefully */
cleanup()
{
	SYMTAB *q;
	for( q=stab; q < &stab[NSZ]; ++q )
		if( q->decflag ) lastone(q);
}
/* setuse - check new type to ensure that it is used */
setuse(q) SYMTAB *q;
{
	if( !q->decflag ){ /* new one */
		q->decflag = r.l.decflag;
		q->symty.t = r.l.type;
		if( r.l.nargs < 0 ){
			q->nargs = -r.l.nargs - 1;
			q->use = VARARGS;
		}
		else {
			q->nargs = r.l.nargs;
			q->use = 0;
		}
		q->fline = r.l.fline;
		q->fno = cfno;
		q->mno = cmno;
		if( q->nargs ){
			int i;
			STYPE *qq;
			for( i=0,qq= &q->symty; i<q->nargs; ++i,qq=qq->next ){
				qq->next = tget();
				qq->next->t = atyp[i];
			}
		}
	}

	switch( r.l.decflag ){

	case LRV:
		q->use |= RVAL;
		return;
	case LUV:
		q->use |= VUSED+USED;
		return;
	case LUE:
		q->use |= EUSED+USED;
		return;
	/* 01/04/80 */	case LUV | LUE:
	case LUM:
		q->use |= USED;
		return;

	}
	/* 04/06/80 */
	if ( (q->decflag & LDX) && (r.l.decflag & LDC) ) q->decflag = LDC;
}
/* chktype - check the two type words to see if they are compatible */

chktype( pt1, pt2 ) register ATYPE *pt1, *pt2; 
{
	register TWORD t,t2;

	t = ctype(pt1->aty);
	t2= ctype(pt2->aty);
	switch(BTYPE(t)){
	case ENUMTY:
	case STRTY:
	case UNIONTY:
	case MOETY:
		if( t != t2){
			/*
			 * if qflag: permit const 0 and (void *) as actual
			 * parameters when formal parameter has pointer
			 * type.
			 */
			if (qflag && ISPTR(t)) {
				if (t2==INT && pt2->extra && pt2->extra1==0 ) {
					/* formal is ptr, actual is const 0 */
					return 0;
				}
				if (TVOIDPTR(t2)) {
					/* formal is ptr, actual is (void*) */
					return 0;
				}
			}
			return 1;
		}else if( pt1->extra1 != pt2->extra1 )
			return 1;
		/* if -z then don't worry about undefined structures,
		   as long as the names match */
		if( zflag && (pt1->extra == 0 || pt2->extra == 0) ) return 0;
		return pt1->extra != pt2->extra;
	default:
		break;
	}
	if( pt2->extra ){ /* constant passed in */
		/*
		 * integer constant acceptable as unsigned.
		 * long constant acceptable as unsigned long.
		 * zero acceptable as pointer if qflag.
		 */
		if( t == UNSIGNED && t2 == INT ) return( 0 );
		else if( t == ULONG && t2 == LONG ) return( 0 );
		else if (ISPTR(t) && (pt2->extra1==0) ) return(!qflag);
	}

	else if( pt1->extra ){ /* for symmetry */
		if( t2 == UNSIGNED && t == INT ) return( 0 );
		else if( t2 == ULONG && t == LONG ) return( 0 );
	}

	if (qflag) {
		/*
		 * (void*) is compatible with any other pointer type
		 */
		if (ISPTR(t) && TVOIDPTR(t2) || ISPTR(t2) && TVOIDPTR(t)) {
			return ( 0 );
		}
	}
	return( t != t2 );
}

/* snarffed from lint.c */
TWORD
ctype( type )
{ /* map types which are not defined on the local machine */
  /* this is appropriate for sunrise/68000 mapping, since "we know"
   * that long == int
   */
	if (qflag){
		switch( BTYPE(type) ){

		case ENUMTY:    /* no such thing */
			MODTYPE(type,UNSIGNED);
			break;

		case LONG:      /* don't waste my time */
			MODTYPE(type,INT);
			break;

		case ULONG:     /* don't waste my time */
			MODTYPE(type,UNSIGNED);
			break;
			}
	}
        return( type );
}

# ifdef DEBUG
/* diagnostic output tools
 *	the following programs are used to print out internal information
 *	for diagnostic purposes
 *	Current options:
 *	-Xi
 *		turns on printing of intermediate file on entry to lint2
 *			(input information)
 *	-Xo
 *		turns on printing of lint symbol table on last pass of lint2
 *			(output information)
 */

struct tb { 
	int m; 
	char * nm };
static struct tb dfs[] = {
	LDI, "LDI",
	LIB, "LIB",
	LDC, "LDC",
	LDX, "LDX",
	LRV, "LRV",
	LUV, "LUV",
	LUE, "LUE",
	LUM, "LUM",
	LDS, "LDS",
	0, "" };

static struct tb us[] = {
	USED, "USED",
	VUSED, "VUSED",
	EUSED, "EUSED",
	RVAL, "RVAL",
	VARARGS, "VARARGS",
	0,	0,
};


/* ptb - print a value from the table */
ptb( v, tp ) struct tb *tp; 
{
	int flag = 0;
	for( ; tp->m; ++tp )
		if( v&tp->m ){
			if( flag++ ) putchar( '|' );
			printf( "%s", tp->nm );
		}
}


/* pif - print intermediate file
 *	prints file written out by first pass of lint
 *  printing turned on by the debug option -Xi
 */
pif()
{
	printf("\n\tintermediate file printout:\n");
	printf("\t===========================\n");
	while( 0 < fread( (char *)&r, sizeof(r), 1, stdin) ) {
		if ( r.l.decflag & LFN )
		{
			r.f.fn = getstr();
			printf( "\nFILE NAME: %-15s\n", r.f.fn );
		}
		else {
			r.l.name = getstr();
			printf( "\t%-8s  (", r.l.name );
			ptb(r.l.decflag, dfs);
			printf(")\t line= %d", r.l.fline);
			if ( ISFTN(r.l.type.aty) ) printf("\tnargs= %d", r.l.nargs);
			else printf("\t\t\t");
			printf("\t type= ");
			tprint(r.l.type.aty);
			printf("\n");
			if ( r.l.nargs ) {
				int n = 0;
				if ( r.l.nargs < 0 ) r.l.nargs= -r.l.nargs - 1;
				fread( (char *)atyp, sizeof(ATYPE), r.l.nargs, stdin);
				while ( ++n <= r.l.nargs ) {
					printf("\t\t arg(%d)= ",n);
					tprint(atyp[n-1].aty); 
					printf("\n");
				}
			}
		}
	} /*while*/

	rewind( stdin );
} /*end pif*/


/* pst - print lint symbol table
 *	prints symbol table created from intermediate file
 *  printing turned on by the debug option -Xo
 */
pst()
{
  int count = 0;
  SYMTAB *q;
	printf("\n\tsymbol table printout:\n");
	printf("\t======================\n");
	for( q=stab; q < &stab[NSZ]; ++q) {
		if( q->decflag ) {
			count++;
			printf( "\t%8s  (", q->name );
			ptb(q->decflag, dfs);
			printf(")\t line= %d", q->fline);
			if ( ISFTN(q->symty.t.aty) ) printf("\tnargs= %d", q->nargs);
			else printf("\t\t\t");
			printf("\t type= ");
			tprint(q->symty.t.aty);
			printf("\t use= ");
			ptb(q->use, us);
			printf("\n");
			if ( q->nargs ) {
				int n = 1;
				STYPE *qq;
				for( qq=q->symty.next; n <= q->nargs; qq=qq->next) {
					printf("\t\t arg(%d)= ",n++);
					tprint(qq->t.aty); 
					printf("\n");
				}
			}
		} /*end if(q->decflag)*/
	} /*end for*/
	printf("\n%d symbol table entries\n",count);
}


/* tprint - print a nice description of a type
 *	borrowed from PCC
 */
tprint( t )  TWORD t; 
{
	static char * tnames[] = {
		"void",
		"farg",
		"char",
		"short",
		"int",
		"long",
		"float",
		"double",
		"strty",
		"unionty",
		"enumty",
		"moety",
		"uchar",
		"ushort",
		"unsigned",
		"ulong",
		"?", "?"
	};

	for(;; t = DECREF(t) ){

		if( ISPTR(t) ) printf( "PTR " );
		else if( ISFTN(t) ) printf( "FTN " );
		else if( ISARY(t) ) printf( "ARY " );
		else if((unsigned)t > ULONG) {
			printf( "t=0x%x ", t );
			return;
			}
		else {
			printf( "%s", tnames[t] );
			return;
		}
	}
}
# else
pif()
{ printf("intermediate file dump not available\n"); }
pst()
{ printf("symbol table dump not available\n"); }
# endif

#include <ctype.h>

char *
getstr()	/* read arb. len. name string from input & return ptr to it */
{
	char buf[BUFSIZ];
	register char *cp = buf;
	register int c;

	while ( ( c = getchar() ) != EOF )
	{
		*cp++ = c;
		if ( c == '\0' || !isascii( c ) )
			break;
	}
	if ( feof( stdin ) ) {
		sprintf( buf, "Premature end of file %s", ifilename ? ifilename : "<stdin>" );
		lerror( buf, CCLOSE | FATAL | ERRMSG );
	} else if ( ferror( stdin ) ) {
		sprintf( buf, "Read error on %s", ifilename ? ifilename : "<stdin>" );
		lerror( buf, CCLOSE | FATAL | ERRMSG );
	} else if ( c != '\0' ) {
		sprintf( buf, "Name string format error in intermediate file %s", ifilename ? ifilename : "<stdin>" );
		lerror( buf, CCLOSE | FATAL | ERRMSG );
	}
	return ( hash( buf ) );
}

#define NSAVETAB	4096
char	*savetab;
int	saveleft;

char *
savestr( cp )		/* copy string into permanent string storage */
	register char *cp;
{
	register int len;

	len = strlen( cp ) + 1;
	if ( len > saveleft )
	{
		saveleft = NSAVETAB;
		if ( len > saveleft )
			saveleft = len;
		savetab = (char *) malloc( (unsigned) saveleft );
		if ( savetab == 0 )
			lerror( "Ran out of memory [savestr()]",
				CCLOSE | FATAL | ERRMSG );
	}
	(void) strncpy( savetab, cp, len );
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return ( cp );
}

/*
* The segmented hash tables
*/
#define MAXHASH	20
#define HASHINC	1013

struct ht
{
	char	**ht_low;
	char	**ht_high;
	int	ht_used;
};

struct ht htab[MAXHASH];	/* non-filenames */

char *
hash( s )	/* hash and store name in permanent storage area */
	char *s;
{
	register char **h;
	register int i;
	register char *cp;
	struct ht *htp;
	int sh;

	/*
	* Hash on all the characters.
	*/
	sh = hashstr(s) % HASHINC;
	cp = s;
	/*
	* Look through each table for name.  If not found in the current
	* table, skip to the next one.
	*/
	for ( htp = htab; htp < &htab[MAXHASH]; htp++ )
	{
		if ( htp->ht_low == 0 )
		{
			register char **hp = (char **) calloc(
				sizeof (char **), HASHINC );

			if ( hp == 0 )
				lerror( "Ran out of memory [fhash()]",
					CCLOSE | FATAL | ERRMSG );
			htp->ht_low = hp;
			htp->ht_high = hp + HASHINC;
		}
		h = htp->ht_low + sh;
		/*
		* Use quadratic re-hash
		*/
		i = 1;
		do
		{
			if ( *h == 0 )
			{
				if ( htp->ht_used > ( HASHINC * 3 ) / 4 )
					break;
				htp->ht_used++;
				*h = savestr( cp );
				return ( *h );
			}
			if ( **h == *cp && strcmp( *h, cp ) == 0 )
				return ( *h );
			h += i;
			i += 2;
			if ( h >= htp->ht_high )
				h -= HASHINC;
		} while ( i < HASHINC );
	}
	lerror( "Ran out of hash tables", CCLOSE | FATAL | ERRMSG );
	/*NOTREACHED*/
}

