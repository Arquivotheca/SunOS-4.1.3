#ifndef lint
static	char sccsid[] = "@(#)lint.c 1.1 92/07/30 SMI"; /* from S5R2 1.7 */
#endif

#ident	"@(#)RELEASE  lint1 4.1"

/* lint.c
 *	This file contains the major functions for the first pass of lint
 *
 *	There are basically two types of functions in this file:
 *	functions called directly by the portable C compiler and other
 *	(lint) functions.
 *
 *	lint functions:
 *	===============
 *		astype	hashes a struct/union line number to ensure uniqueness
 *		ctargs	count arguments of a function
 *		fldcon	check assignment of a constant to a field
 *		fsave	write out a new filename to the intermediate file
 *		lmerge
 *		lprt
 *		lpta
 *		main	driver routine for the first pass (lint1)
 *		outdef	write info out to the intermediate file
 *		strip	strip a filename down to its basename
 *		where	print location of an error
 *
 *	pcc interface routines:
 *	=======================
 *		andable		returns 1 if a node can accept the & operator
 *		aocode		called when an automatic is removed from symbol table
 *		asmout		called to put an "asm" into the output
 *		bfcode		emit code for beginning of function
 *		branch		dummy, unused by lint
 *		cinit		
 *		clocal		do local transformations on the tree
 *		commdec		put out a common declaration
 *		ctype		dummy for lint
 *		defalign	dummy, unused by lint
 *		deflab		dummy, unused by lint
 *		defname		
 *		ecode		emit code for tree (lint processing for whole tree)
 *		efcode		emit code for end of function
 *		ejobcode	end of job processing
 *		exname		create external name
 *		fldal		check field alignment
 *		fldty		check field type
 *		isitfloat
 *		noinit		return storage class for uninitialized objects
 *		offcon		make structure offset node
 *		prtdcon
 *		zecode		create arg integer words of zero
 */

# include "cpass1.h"

# include "messages.h"
#ifndef CXREF
# include "lerror.h"
#endif
# include "lmanifest.h"

# include <ctype.h>
# include <signal.h>

# define VAL 0
# define EFF 1

#if CXREF
extern int ddebug, idebug, bdebug, tdebug, edebug, xdebug;

int blocknos[20];	/* CXREF */
int blockptr = 0;	/* CXREF */
int nextblock = 1;	/* CXREF */
#endif

int vflag = 1;  /* tell about unused argments */
#if CXREF
int xflag = 0;  /* tell about unused externals */
#else
/* 28 feb 80  reverse sense of xflag */
int xflag = 1;  /* tell about unused externals */
#endif
int argflag = 0;  /* used to turn off complaints about arguments */
int libflag = 0;  /* used to generate library descriptions */
int vaflag = -1;  /* used to signal functions with a variable number of args */
#if CXREF
int aflag = 0;  /* used to check precision of assignments */
#else
/* 28 feb 80  reverse sense of aflag */
int aflag = 1;  /* used to check precision of assignments */
int zflag = 0;  /* no 'structure never defined' error */
int Cflag = 0;  /* filter out certain output, for generating libraries */
#endif

#if CXREF
char *flabel = "xxx";
int filecount = 0;
#endif

#ifndef CXREF
# ifdef sparc
	int host = sparcAL;
	int target = sparcAL;
# endif
# ifdef mc68000
	int host = mc68020AL;
	int target = mc68020AL;
# endif
# ifdef i386
	int host = i386AL;
	int target = i386AL;
# endif

	char sourcename[ BUFSIZ + 1 ] = "";

	char *libname = 0;  /* name of the library we're generating */
	char *builtin_va_alist_name;

extern struct alignment altable[];
extern struct alignment portalign;

	/* flags for the "outdef" function */
# define USUAL (-101)
# define DECTY (-102)
# define NOFILE (-103) /* somewhat of a misnomer */
# define SVLINE (-104)

# define LNAMES 250
#else	/* CXREF */
# define LNAMES 100
#endif

struct lnm {
	int lid;
	short flgs;
	}  lnames[LNAMES], *lnp;

/* contx - check context of node
 *	contx is called for each node during tree walk (fwalk);
 *	it complains about nodes that have null effect.
 *	VAL is passed to a child if that child's value is used
 *	EFF is passed to a child if that child is used in an effects context
 *
 *	arguments:
 *		p - node pointer
 *		down - value passed down from ancestor
 *		pl, pr - pointers to values to be passed down to descendants
 */
contx( p, down, pl, pr ) register NODE *p; register *pl, *pr;
{
	*pl = *pr = VAL;
	switch( p->in.op ){

		/* left side of ANDAND, OROR, and QUEST always evaluated for value
	 	   (value determines if right side is to be evaluated) */
		case ANDAND:
		case OROR:
		case QUEST:
			*pr = down; break;

		/* left side and right side treated identically */
		case SCONV:
		case PCONV:
		case COLON:
			*pr = *pl = down; break;

		/* comma operator uses left side for effect */
		case COMOP:
			*pl = EFF;
			*pr = down;

		case FORCE:
		case INIT:
		case UNARY CALL:
		case STCALL:
		case UNARY STCALL:
		case CALL:
		case UNARY FORTCALL:
		case FORTCALL:
		case CBRANCH:
			break;

		default:
			/* assignment ops are OK */
			if( asgop(p->in.op) ) break;

			/* struct x f( );  main( ) {  (void) f( ); }
		 	 *  the cast call appears as U* TVOID
			 */
			if( p->in.op == UNARY MUL &&
				( p->in.type == STRTY
				|| p->in.type == UNIONTY
				|| p->in.type == TVOID) )
				break;  /* the compiler does this... */

			/* found a null effect ... */
                        if( down == EFF && hflag && p->in.type != TVOID ) 
                                WERROR( MESSAGE( 86 ) );
	}
}

#ifndef CXREF
/* ecode - compile code for node */
ecode( p ) NODE *p;
{
	fwalk( p, contx, EFF );	/* do a preorder tree walk */
	lnp = lnames;		/* initialize pointer to start of array */
	lprt( p, EFF, 0, 0 );
}

ejobcode( flag )
{	/* called after processing each job */
	/* flag is nonzero if errors were detected */
	register k;
	register struct symtab *p;
	int i;
	extern		hdrclose( ),
			unbuffer( );

        for (i=0; i<SYMTSZ; ++i) {
                for (p=stab[i]; p!=NULL; p=p->next) {

			if( p->stype == STRTY || p->stype == UNIONTY || p->stype == ENUMTY){
				if( !zflag && dimtab[p->sizoff+1] < 0 ){
					if( hflag ){
						if (p->stype == ENUMTY)
							/* "enum %s never defined" */
							WERROR( MESSAGE( 104 ), p->sname );
						else
							/* "struct/union %s never defined" */
							WERROR( MESSAGE( 102 ), p->sname );
						}
					}
				}

			switch( p->sclass ){
			
			case STATIC:
				if( p->suse > 0 ){
					k = lineno;
					lineno = p->suse;
					if (ISFTN(p->stype))
						/* "static function %s unused" */
						UERROR( MESSAGE( 141 ),
						    p->sname );
					else
						/* "static variable %s unused" */
						UERROR( MESSAGE( 101 ),
						    p->sname );
					lineno = k;
					break;
					}
				/* no statics in libraries */
				if( Cflag ) break;

			case EXTERN:
			case USTATIC:
				/* with the xflag, worry about externs not used */
				/* the filename may be wrong here... */
				if( xflag && p->suse >= 0 && !libflag )
					outdef( p, LDX, NOFILE );
			
			case EXTDEF:
				if( p->suse < 0 )  /* used */
					outdef( p, LUM, SVLINE );
				break;
			}
		}
	}
	hdrclose( );
	unbuffer( );
	exit( 0 );
}
/* astype - hash a struct/union/enum line number to ensure uniqueness */
astype( t, i ) ATYPE *t;
{ TWORD tt;
	struct symtab *s;
	int k=0, l=0;

	if( (tt=BTYPE(t->aty))==STRTY || tt==UNIONTY || tt==ENUMTY ) {
		if( i<0 || i>= curdim-3 )
			uerror( "lint's little mind is blown" );
		else {
			s = STP(dimtab[i+3]);
			if( s == NULL ) {
				k = dimtab[i];
				l = X_NONAME;
				}
			else {
				if( s->suse <= 0 )
					uerror( "no line number for %s",
						s->sname );
				else {
					k = dimtab[i];
					l = hashstr(s->sname);
					/* XXX - -p, 8 chars? */
					}
				}
			}
		
		t->extra = k;
		t->extra1 = l;
		return( 1 );
	}
	else return( 0 );
}
/* bfcode - handle the beginning of a function */
bfcode( a, n ) int a[];
{	/* code for the beginning of a function; a is an array of
		indices in stab for the arguments; n is the number */
	/* this must also set retlab */
	register i;
	register struct symtab *cfp;
	static ATYPE t;

	retlab = 1;

	cfp = STP(curftn);
	if (cfp == NULL) return;

	/* if creating library, don't do static functions */
	if( Cflag && cfp->sclass == STATIC ) return;

	/* if variable number of arguments,
	  only print the ones which will be checked */
	if( vaflag >= 0 ) {
		if( n < vaflag )
		  uerror( "declare the VARARGS arguments you want checked!" );
		else n = vaflag;
	} else {
		/* look for varargs cookie ourselves */
		for ( i=0; i<n; ++i) {
			if (STP(a[i])->sname == builtin_va_alist_name){
				/* make sure it looks legal */
				if ( i != n - 1 )
					uerror("there are parameters after va_alist");
				vaflag = n = i;
				break;
			}
		}
	}

	fsave( ftitle );
	outdef( cfp, cfp->sclass==STATIC? LDS : (libflag?LIB:LDI),
		vaflag >= 0 ? -(n + 1) : n );
	vaflag = -1;

	/* output the arguments */
	if( n )
		for( i=0; i<n; ++i ) {
			register struct symtab *s;
			s = STP(a[i]);
			if (s == NULL) {
				bzero((char *)&t, sizeof(t));
				}
			else {
				t.aty = s->stype;
				t.extra = 0;
				t.extra1 = 0;
				if( !astype( &t, s->sizoff ) ) 
					switch( t.aty ){

					case ULONG:
						break;

					case CHAR:
					case SHORT:
						t.aty = INT;
						break;

					case UCHAR:
					case USHORT:
					case UNSIGNED:
						t.aty = UNSIGNED;
						break;

					}
				}
			fwrite( (char *)&t, sizeof(ATYPE), 1, stdout );
			}
}
/* ctargs - count arguments; p points to at least one */
ctargs( p ) NODE *p;
{
	/* the arguments are a tower of commas to the left */
	register c;
	c = 1; /* count the rhs */
	while( p->in.op == CM ){
		++c;
		p = p->in.left;
		}
	return( c );
}
/* lpta */
lpta( p ) NODE *p;
{
	static ATYPE t;

	if( p->in.op == CM ){
		lpta( p->in.left );
		p = p->in.right;
		}

	t.aty = p->in.type;
	if (p->in.op==ICON) {
		t.extra = 1;
		t.extra1 = p->tn.lval;
	} else {
		t.extra = t.extra1 = 0;
	}

	if( !astype( &t, p->fn.csiz ) )
		switch( t.aty ){

			case CHAR:
			case SHORT:
				t.aty = INT;
			case LONG:
			case ULONG:
			case INT:
			case UNSIGNED:
				break;

			case UCHAR:
			case USHORT:
				t.aty = UNSIGNED;
				break;

			case FLOAT:
				t.aty = DOUBLE;
				t.extra = 0;
				break;

			default:
				t.extra = 0;
				break;
		}
	fwrite( (char *)&t, sizeof(ATYPE), 1, stdout );
}

# define VALSET 1
# define VALUSED 2
# define VALASGOP 4
# define VALADDR 8

#define STASG_LEFT  1
#define STASG_RIGHT 2

lprt( p, down, uses, stasg ) register NODE *p;
{
	register struct symtab *q;
	register acount;
	register down1, down2;
	register use1, use2;
	register struct lnm *np1, *np2;
	extern char *htmpname;

	/* first, set variables which are set... */

	use1 = use2 = VALUSED;
	if( p->in.op == ASSIGN ) use1 = VALSET;
	else if( p->in.op == UNARY AND ) use1 = VALADDR;
	else if( asgop( p->in.op ) && p->in.op != STASG ){ /* =ops */
		use1 = VALUSED|VALSET;
		if( down == EFF ) use1 |= VALASGOP;
		}

	/* print the lines for lint */

	down2 = down1 = VAL;
	acount = 0;

	switch( p->in.op ){

	case EQ:
	case NE:
		if( ISUNSIGNED(p->in.left->in.type) &&
		    p->in.right->in.op == ICON && p->in.right->tn.lval < 0 &&
		    p->in.right->tn.rval == NONAME &&
		    !ISUNSIGNED(p->in.right->in.type) )
			/* "comparison of unsigned with negative constant" */
			WERROR( MESSAGE( 21 ) );
		goto charchk;

	case GT:
	case GE:
	case LT:
	case LE:
		if (p->in.left->in.type == CHAR && p->in.right->in.op == ICON &&
		    p->in.right->tn.lval == 0)
			/* "nonportable character comparison" */
			WERROR(MESSAGE(82));
	charchk:
		if (p->in.left->in.type == CHAR && p->in.right->in.op==ICON &&
		   p->in.right->tn.lval < 0)
			/* "nonportable character comparison" */
			WERROR( MESSAGE( 82 ) );

		break;

	case UGE:
	case ULT:
		if( p->in.right->in.op == ICON && p->in.right->tn.lval == 0
		    && p->in.right->tn.rval == NONAME ) {
			/* "degenerate unsigned comparison" */
			WERROR( MESSAGE( 30 ) );
			break;
			}

	case UGT:
	case ULE:
	    if ( ISUNSIGNED(p->in.left->in.type) &&
		 p->in.right->in.op == ICON &&
		 p->in.right->tn.rval == NONAME ) {
			/* given the "usual arithmetic conversions", which */
			/* apply to relational operators, can this ever */
			/* happen? */
			if (!ISUNSIGNED( p->in.right->in.type )
			    && p->in.right->tn.lval < 0 ) 
				/* "comparison of unsigned with negative constant" */
				WERROR( MESSAGE( 21 ) );

			if ( htmpname != NULL && p->in.right->tn.lval == 0 )
				/* "unsigned comparison with 0?" */
				WERROR( MESSAGE( 115 ) );
	    }
	    break;

	case COMOP:
		down1 = EFF;

	case ANDAND:
	case OROR:
	case QUEST:
		down2 = down;
		/* go recursively left, then right  */
		np1 = lnp;
		lprt( p->in.left, down1, use1, 0 );
		np2 = lnp;
		lprt( p->in.right, down2, use2, 0 );
		lmerge( np1, np2, 0 );
		return;

	case SCONV:
	case PCONV:
	case COLON:
		down1 = down2 = down;
		break;

	case CALL:
	case STCALL:
	case FORTCALL:
		acount = ctargs( p->in.right );
	case UNARY CALL:
	case UNARY STCALL:
	case UNARY FORTCALL:
		if( p->in.left->in.op == ICON
		    && (q = STP(p->in.left->tn.rval)) != NULL ){
			/* used to be &name */
			int lty;

			fsave( ftitle );
			/*
			 * if we're generating a library -C then
			 * we don't want to output references to functions
			 */
			if( Cflag ) break;
			/*  if a function used in an effects context is
			 *  cast to type  void  then consider its value
			 *  to have been disposed of properly
			 *  thus a call of type  void  in an effects
			 *  context is construed to be used in a value
			 *  context
			 */
			if (down == EFF) lty = (p->in.type == TVOID) ? LUV | LUE : LUE;
			else lty = LUV;

			outdef(q, lty, acount);
			if( acount )
				lpta( p->in.right );
			}
		break;

	case ICON:
		/* look for &name case */
		if( (q = STP(p->tn.rval)) != NULL ) {
			q->sflags |= (SREF|SSET);
		}
		return;

	case NAME:
		if( (q = STP(p->tn.rval)) != NULL ) {
			if( (uses&VALUSED) && !(q->sflags&SSET) ){
				if( q->sclass == AUTO || q->sclass == REGISTER ){
					if( !ISARY(q->stype ) && !ISFTN(q->stype) &&
					    (q->stype != STRTY || stasg) && q->stype!=UNIONTY ){
						/* "%s may be used before set" */
						WERROR( MESSAGE( 1 ), q->sname );
						q->sflags |= SSET;
					}
				}
			}
			if( uses & VALASGOP ) break;  /* not a real use */
			if( uses & VALSET ) q->sflags |= SSET;
			if( uses & VALUSED ) q->sflags |= SREF;
			if( uses & VALADDR ) q->sflags |= (SREF|SSET);
			if( p->tn.lval == 0 ){
				lnp->lid = p->tn.rval;
				lnp->flgs = (uses&VALADDR)?0:((uses&VALSET)?VALSET:VALUSED);
				if( ++lnp >= &lnames[LNAMES] ) --lnp;
			}
		}
		return;
	}

	if (stasg && (p->in.left->in.type == STRTY)){
		if (stasg == STASG_RIGHT) use1 = VALUSED;
		else if (stasg == STASG_LEFT) use1 = VALSET;
	}
	
	/* recurse, going down the right side first if we can */

	switch( optype(p->in.op) ){

	case BITYPE:
		np1 = lnp;
		lprt( p->in.right, down2, use2,
		      (p->in.op == STASG) ? STASG_RIGHT : 0 );
	case UTYPE:
		np2 = lnp;
		lprt( p->in.left, down1, use1,
		      (p->in.op == STASG | stasg) ? STASG_LEFT : 0 );
	}

	if( optype(p->in.op) == BITYPE ){
		if( p->in.op == ASSIGN && p->in.left->in.op == NAME )
		  /* special case for a =  .. a .. */
			lmerge( np1, np2, 0 );

		else lmerge( np1, np2, p->in.op != COLON );
		/* look for assignments to fields, and complain */
		if( p->in.op == ASSIGN && p->in.left->in.op == FLD
		  && p->in.right->in.op == ICON ) fldcon( p );
		}
}
/* lmerge */
lmerge( np1, np2, flag ) struct lnm *np1, *np2;
{
	/* np1 and np2 point to lists of lnm members, for the two sides
	 * of a binary operator
	 * flag is 1 if commutation is possible, 0 otherwise
	 * lmerge returns a merged list, starting at np1, resetting lnp
	 * it also complains, if appropriate, about side effects
	 */

	register struct lnm *npx, *npy;

	for( npx = np2; npx < lnp; ++npx ){

		/* is it already there? */
		for( npy = np1; npy < np2; ++npy ){
			if( npx->lid == npy->lid ){ /* yes */
				if( npx->flgs == 0 || npx->flgs == (VALSET|VALUSED) )
					;  /* do nothing */
				else
					if( (npx->flgs|npy->flgs)== (VALSET|VALUSED)
					  || (npx->flgs&npy->flgs&VALSET) ) {
						struct symtab *q;
						char *s;
						q = STP(npy->lid);
						s = (q == NULL ? "(null)" : q->sname);
						/* "evaluation order for ``%s'' undefined" */
						if( flag ) WERROR( MESSAGE( 0 ), s );
					  	}

				if( npy->flgs == 0 ) npx->flgs = 0;
				else npy->flgs |= npx->flgs;
				goto foundit;
			}
		}

		/* not there: update entry */
		np2->lid = npx->lid;
		np2->flgs = npx->flgs;
		++np2;

		foundit: ;
	}

	/* all finished: merged list is at np1 */
	lnp = np2;
}
/* efcode - handle end of a function */
efcode()
{
	/* code for the end of a function */
	register struct symtab *cfp;

	cfp = STP(curftn);
	if (cfp == NULL) return;
	if( retstat & RETVAL && !(Cflag && cfp->sclass==STATIC) )
		outdef( cfp, LRV, DECTY );
	if( !vflag ){
		vflag = argflag;
		argflag = 0;
	}
	if( retstat == RETVAL+NRETVAL )
		/* "function %s has return(e); and return;" */
		WERROR( MESSAGE( 43 ), cfp->sname);
	/*
	* See if main() falls off its end or has just a return;
	*/
	if (!strcmp(cfp->sname, "main") && (reached || (retstat & NRETVAL)))
		/* "main() returns random value to invocation environment" */
		WERROR(MESSAGE(127));
}
/* aocode - called when automatic p is removed from stab */
aocode(p) struct symtab *p;
{
	register struct symtab *cfs;
	cfs = STP(curftn);
	if (cfs == NULL) return;
	if(p->suse>0 && !(p->sflags&(SMOS|STAG)) ){
		if( p->sclass == PARAM
		    || p->sclass == REGISTER && p->slevel == 1 ) {
			/* "argument %s unused in function %s" */
			if( vflag ) WERROR( MESSAGE( 13 ), p->sname, cfs->sname );
		}
		else
			/* "%s unused in function %s" */
			if( p->sclass != TYPEDEF ) WERROR( MESSAGE( 6 ),
			  p->sname, cfs->sname );
	}

	if( p->suse < 0 && (p->sflags & (SSET|SREF|SMOS)) == SSET
	  && !ISARY(p->stype) && !ISFTN(p->stype) )
		/* "%s set but not used in function %s" */
		WERROR( MESSAGE( 3 ), p->sname, cfs->sname );

	if( p->stype == STRTY || p->stype == UNIONTY || p->stype == ENUMTY )
		if( !zflag && dimtab[p->sizoff+1] < 0 ){
			if (p->stype == ENUMTY)
				/* "enum %s never defined" */
				WERROR( MESSAGE( 104 ), p->sname );
			else
				/* "struct/union %s never defined" */
				WERROR( MESSAGE( 102 ), p->sname );
			}
}

/* defname - define the current location as the name p->sname */
defnam( p ) register struct symtab *p;
{
	if( p->sclass == STATIC && (p->slevel>1 || Cflag) ) return;

	if( !ISFTN( p->stype ) )
		outdef( p, p->sclass==STATIC? LDS : (libflag?LIB:LDI), USUAL );
}
#endif	/* ifndef CXREF */

/* zecode - n integer words of zeros */
zecode( n )
{
	OFFSZ temp;
	temp = n;
	inoff += temp*SZINT;
}

/*ARGSUSED*/
mklval( p ) NODE *p; {
	/* can we make this operand into a legal lvalue, if it
	   isn't one already?  One version of pcc does so if it is
	   a cast on the left hand side of an assignment.  Since
	   that is highly nonportable, we won't bother. */
	return 0;
	}

/* andable - can the NAME node p accept &  ? */
andable( p ) NODE *p;
{
	register struct symtab *s;

	if( p->in.op != NAME ) cerror( "andable error" );

	if( p->tn.rval < 0 ) return(1);  /* labels are andable */
	if( (s = STP(p->tn.rval)) == NULL ) return(0);

	if( s->sclass == AUTO || s->sclass == PARAM ) return(0); 
	/* "cannot take address of %s" */
	if( s->sclass == REGISTER ) UERROR( MESSAGE( 18 ), s->sname );
	return(1);
}
/* clocal - do local checking on tree
 *	(pcc uses this routine to do local tree rewriting)
 */
NODE *
clocal(p) NODE *p;
{
	register o;
	register TWORD t, tl;
	int s;

	switch( o = p->in.op ){

	case SCONV:
	case PCONV:
		if( p->in.left->in.type==ENUMTY )
			p->in.left = pconvert( p->in.left );

		/* assume conversion takes place; type is inherited */
		t = p->in.type;
		tl = p->in.left->in.type;
#ifndef CXREF
/* for the future: put aflag in a place where NAME is available; that is, check
 * assignment and arithmetic operators for leftchild NAME and rightchild SCONV
 * so that when message is printed it is possible to name the offending lval
 * note that the lval may be a temporary (NONAME)
 */
		if( aflag
		   && ( tl == LONG || tl == ULONG )
		   && t!=LONG && t!=ULONG && t!=TVOID
		   && t!=FLOAT && t!=DOUBLE ) 
			/* "conversion from long may lose accuracy" */
			WERROR( MESSAGE( 26 ) );

		if( aflag && pflag && tl != LONG && tl != ULONG 
		  && tl != FLOAT && tl != DOUBLE
		  && ( t == LONG || t == ULONG )
		  && p->in.left->in.op != ICON )
			/* conversion to long may sign-extend incorrectly */
			WERROR( MESSAGE( 27 ) );

		if( ISPTR(tl) && ISPTR(t) ){
			tl = DECREF(tl);
			t = DECREF(t);
			switch( ISFTN(t) + ISFTN(tl) ){

			case 0:  /* neither is a function pointer */
				if( talign(t,p->fn.csiz) > talign(tl,p->in.left->fn.csiz) ) {
					/* "possible pointer alignment problem" */
					if( hflag || pflag || (target == sparcAL))
						WERROR( MESSAGE( 91 ) );
				}
				break;

			case 1:
				/* "questionable conversion of function pointer" */
				WERROR( MESSAGE( 95 ) );

			case 2:
				;
			}
		}
#endif
		if (o == SCONV) {
			/*
			 * Must do integer <-> floating conversions here, so
			 * that if e.g. a floating-point expression cast to
			 * "int" is being used as an array dimension, we check
			 * it properly.
			 */
			if( p->in.left->in.op == ICON ) {
				if( p->in.type == FLOAT
				    || p->in.type == DOUBLE ) {
					if (ISUNSIGNED(tl)) {
						p->in.left->fpn.dval =
						    (unsigned)p->in.left->tn.lval;
					} else {
						p->in.left->fpn.dval =
						    p->in.left->tn.lval;
					}
					p->in.left->in.op = FCON;
				    }
			} else if( p->in.left->in.op == FCON ) {
				if( p->in.type != FLOAT
				    && p->in.type != DOUBLE ) {
					p->in.left->in.op = ICON;
					p->in.left->tn.lval =
					    (CONSZ)p->in.left->fpn.dval;
					p->in.left->tn.rval = NONAME;
					p->in.left->tn.name = (char*)0;
				}
			}
		}
		p->in.left->in.type = p->in.type;
		p->in.left->fn.cdim = p->fn.cdim;
		p->in.left->fn.csiz = p->fn.csiz;
		p->in.op = FREE;
		return( p->in.left );

	case PVCONV:
	case PMCONV:
		if( p->in.right->in.op != ICON ) cerror( "bad conversion");
		p->in.op = FREE;
		return( buildtree( o==PMCONV?MUL:DIV, p->in.left, p->in.right ) );

	case RS:
	case LS:
	case ASG RS:
	case ASG LS:
		if( p->in.right->in.op != ICON )
			break;
		s = p->in.right->tn.lval;
		if( s < 0 )
			/* "negative shift" */
			WERROR( MESSAGE( 136 ) );
		else
		if( s >= dimtab[ p->fn.csiz ] )
			/* "shift greater than size of object" */
			WERROR( MESSAGE( 137 ) );
		break;
	}
	return(p);
}
/* offcon - make structure offset node */
NODE *
offcon( off, t, d, s ) OFFSZ off; TWORD t;
{
	register NODE *p;
	p = bcon(0);
	p->tn.lval = off/SZCHAR;
	return(p);
}

/* noinit - return storage class for such as "int a;" */
noinit()
{
	return( pflag ? EXTDEF : EXTERN );
}

/* cinit - initialize p into size sz */
cinit( p, sz ) NODE *p;
{
	inoff += sz;
	if( p->in.op == INIT ){
		if( p->in.left->in.op == ICON ) return;
		if( p->in.left->in.op == FCON ) return;
		if( p->in.left->in.op == NAME && p->in.left->in.type == MOE ) return;
	}
	/* "illegal initialization" */
	UERROR( MESSAGE( 61 ) );
}

#ifndef CXREF
/* exname - make a name look like an external name
 *		if checking for portability or if running on gcos or ibm,
 *		truncate name to six characters
 */
char *
exname( p ) char *p;
{
	static char aa[8];
	register int i;

#if !( ibm | gcos )
	if( !pflag ) return(p);
#endif
	for( i=0; i<6; ++i ){
		if( isupper(*p ) ) aa[i] = tolower( *p );
		else aa[i] = *p;
		if( *p ) ++p;
	}
	aa[6] = '\0';
	return( aa );
}
/* strip - strip full name to get basename */
char *
strip(s) register char *s;
{
	static char x[BUFSIZ+1];
	register char *p;
	register char c;

	/*
	 * Trim off leading quote, if any.
	 */
	if (*s == '"')
		s++;
	/*
	 * Trim off leading "./"; "cpp" tends to insert it before include
	 * file names.
	 */
	if (strncmp(s, "./", 2) == 0)
		s += 2;
	/*
	 * Copy until the end of the string or until a trailing quote.
	 */
	p = x;
	while( (c = *s++) != '\0' && c != '"' ){
		if ( p > &x[BUFSIZ] ) {
			/* 5 feb 80:  simulate a call to cerror( ) */
			fprintf(stderr, "%s: compiler error: filename too long\n", x);
			exit(1);
			/* cannot call cerror( )
			 * because cerror( ) calls where( ) and where( ) calls
			 * strip( ) and this is strip
			 */
		}
		*p++ = c;
	}
	*p = '\0';
	return( x );
}

/* fsave - save file name on intermediate file */
fsave( s ) char *s; {
	static union rec fsname;
	static char buf[BUFSIZ+1];

	s = strip( s );
	if( strcmp( s, buf ) ){
		/* new one */
		strcpy( buf, s );
		fsname.f.decflag = LFN;
		fsname.f.mno = getpid();
		fwrite( (char *)&fsname, sizeof(fsname), 1, stdout );
		fwrite( buf, strlen( buf ) + 1, 1, stdout );
	}
}
#endif	/* ifndef CXREF */

/* where  - print the location of an error
 *  if the filename is a C file (the source file) then just print the lineno
 *    the filename is taken care of in a title
 *  if the file is a header file ( unlikely but possible)
 *    then the filename is printed with the line number of the error
 *    where is called by cerror, uerror and werror
 *    (it is not called by luerror or lwerror)
 */
where( f ) char	f;
{
    extern		fprintf( );
#ifndef CXREF
    extern enum boolean	iscfile( );
    extern char		*strip( );
    char		*filename;
    extern char		*htmpname;
#endif

    if( f == 'u' && nerrors > 1 ) --nerrors; /* don't get "too many errors" */

#if CXREF
    fprintf( stderr, "%s, line %d: ", ftitle, lineno );
#else
    filename = strip( ftitle );
    if ( htmpname != NULL && iscfile( filename ) == true )
		fprintf( stderr, "(%d)  ", lineno );
    else
		fprintf( stderr, "%s(%d): ", filename, lineno );
#endif
}
/* beg_file() and fpe_catch()
 * Used to do stuff to handle floating point exceptions
 * generated during floating point constant folding
 */
beg_file()
{
	extern int fpe_catch();

	/*
	 * Catch floating exceptions generated by the constant
	 * folding code.
	 */
	(void) signal( SIGFPE, fpe_catch );
}
fpe_catch()
{
	/*
	 * Floating exception generated by constant folding.
	 */
	/* "floating point constant folding causes exception" */
	UERROR( MESSAGE( 125 ) );
#if CXREF
	exit( 2 );
#else
	lerror( "", CCLOSE | HCLOSE | FATAL );
	/* note that no error message is printed */
#endif
}
/* a number of dummy routines, unneeded by lint */

#ifndef CXREF
/* fldty - check field type (called from pcc) */
fldty(p) struct symtab *p;
{
	/* for a field to be portable, it must be unsigned int */
	/* "the only portable field type is unsigned int" */
	if( pflag && BTYPE(p->stype) != UNSIGNED ) UERROR( MESSAGE( 83 ) );
}
#endif

/* fldal - field alignment (called from pcc) 
 *		called for alignment of funny types (e.g. arrays, pointers) 
 */
fldal(t) TWORD t;
{
	if( t == ENUMTY )	/* this should be thought through better... */
		return( ALINT );/* jwf try this */
#ifdef gcos
	if( !ISPTR(t) )	
#endif
		/* "illegal field type" */
		UERROR( MESSAGE( 57 ) );
	return(ALINT);
}
/* main - driver for the first pass */
main( argc, argv ) int	argc; char	*argv[ ];
{
#if CXREF
	/* definitions for CXREF */
	char *cp;
	int n = 1;
#endif
    char	 *p;
    int		i;
#ifndef CXREF
    extern 	tmpopen( );
    extern char *htmpname;
#endif
	char stdbuf[BUFSIZ];	/*stderr output buffer*/

	setbuf(stderr, stdbuf);
    /* handle options */

#ifndef CXREF
    /* 28 feb 80  reverse the sense of hflag and cflag */
    hflag = 1;
	/*
    cflag = 1;
		9/25/80	undo the (28 feb 80) change to cflag */
    /* 31 mar 80  reverse the sense of brkflag */
    brkflag = 1;
#endif

	for( i = 1; i < argc && argv[i][0] == '-'; i++ )
		for( p=argv[i]; *p; ++p ){

			switch( *p ){

			case '-':
				continue;

			case '\0':
				break;

#if CXREF
			case 'd':
				++ddebug;
				continue;

			case 'I':
				++idebug;
				continue;

			case 'b':
				++bdebug;
				continue;

			case 't':
				++tdebug;
				continue;

			case 'e':
				++edebug;
				continue;

			case 'x':
				++xdebug;
				continue;

			case 'f':	/* filename sent to cpp */
				p++;
				if (*p == '\0') {
					i++;
					if (i >= argc) {
						fprintf(stderr, "Missing argument to \"-f\"\n");
						exit(2);
					}
					p = argv[i];
				}
				infile[0] = '"';	/* put quotes around name */
				cp = &infile[1];
				while (*cp++ = *p++) ;	/* copy filename */
				*--cp = '"';
				*++cp = '\0';
				break;

			case 'i':	/* actual input filename */
				p++;
				if (*p == '\0') {
					i++;
					if (i >= argc) {
						fprintf(stderr, "Missing argument to \"-i\"\n");
						exit(2);
					}
					p = argv[i];
				}
				if (freopen(p,"r",stdin) == NULL) {
					fprintf(stderr, "Can't open %s\n",p);
					exit(1);
				}
				break;

			case 'o':
				p++;
				if (*p == '\0') {
					i++;
					if (i >= argc) {
						fprintf(stderr, "Missing argument to \"-o\"\n");
						exit(2);
					}
					p = argv[i];
				}
				if ((outfp = fopen(p,"a")) == NULL) {
					fprintf(stderr, "Can't open %s\n",p);
					exit(2);
				}
				break;
#else	/* ifndef CXREF */
			case 'b':
				brkflag = 0;
				continue;

			case 'p':
				pflag = 1;
				cflag = 1;	/* added 9/25/80 */
				continue;

			case 'q':
				qflag = 1;
				continue;

			case 'c':
				cflag = 1;
				continue;

			case 'L':		/*undocumented; make input look like library*/
				libflag = 1;

			case 'v':
				vflag = 0;
				continue;

			case 'x':
				xflag = 0;
				continue;

			case 'a':
				aflag = 0;
			case 'u':		/* second pass option */
			case 'n':		/* done in shell script */
				continue;

			case 'T':		/* second pass option */
			case 'X':		/* second pass option */
				break;

			case 'z':
				zflag = 1;
				continue;

			case 'P':	/* debugging, done in second pass */
				continue;

			case 'C':
				Cflag = 1;
				if( p[1] ) libname = p + 1;
				while( p[1] ) p++;
				continue;

			case 's':
				/* for the moment, -s triggers -h */

			case 'h':
				if (strncmp(p, "host=", 5) == 0) {
					if (strcmp(p+5, "sparc") == 0 ||
					    strcmp(p+5, "sun4") == 0) 
						host = sparcAL;
					if (strcmp(p+5, "mc68020") == 0 ||
					    strcmp(p+5, "sun3") == 0    ||
					    strcmp(p+5, "mc68010") == 0 ||
					    strcmp(p+5, "sun2") == 0) 
						host = mc68020AL;
					if (strcmp(p+5, "i386") == 0 ||
					    strcmp(p+5, "sun386") == 0) 
						host = i386AL;
					while( p[1] ) p++;
					continue;
				} else {
					hflag = 0;
					continue;
				}

			case 't':
				if (strncmp(p, "target=", 7) == 0) {
					if (strcmp(p+7, "sparc") == 0  ||
					    strcmp(p+7, "sun4") == 0) 
						target = sparcAL;
					if (strcmp(p+7, "mc68020") == 0 ||
					    strcmp(p+7, "sun3") == 0    ||
					    strcmp(p+7, "mc68010") == 0 ||
					    strcmp(p+7, "sun2") == 0) 
						target = mc68020AL;
					while( p[1] ) p++;
					continue;
				} else {
					werror( "option %c now default: see `man 1 lint'", *p );
					continue;
				}

			case 'H':
				p++;
				if (*p == '\0') {
					i++;
					if (i >= argc)
						uerror("Missing argument to \"-H\"\n");
					p = argv[i];
				}
				htmpname = p;
				break;

			default:
				uerror( "illegal option: %c", *p );
				continue;

#endif	/* ifndef CXREF */
				};
			break;
			}

#ifndef CXREF
	/*
	 * Decide whether you're S5 or pre-S3, and patch up if pre-S3.
	 * "Patching up" involves the following:
	 * - toggling the values of aflag, brkflag, hflag, and xflag
	 * - allowing "-c" to set cflag
	 * - forcing msgs to come out one per line, instead of
	 *	being nicely formatted as is the S5 default
	 */
	if( htmpname == NULL ) {
		aflag = !aflag;
		brkflag = !brkflag;
		hflag = !hflag;
		xflag = !xflag;
	} else {
		/*
		 * "cflag" true and "pflag" false means the "-c" option
		 * was specified.  This option disappeared in S5, so we
		 * complain about it.
		 */
		if( cflag && !pflag )
			fprintf(stderr,"lint: -c option ignored - no longer available\n");
	}
	/* process file name */
	if( argv[i] ) {
		p = strip( argv[i] );
		strncpy( sourcename, p, LFNM );
	}
    tmpopen( );
#endif

#if CXREF
	n = mainp1(argc, argv);
	if (feof(stdout) || ferror(stdout))
		perror("cxref.xpass");
	return( n );
#else
	/* set sizes and alignment */
	if( !pflag ){
		/* set sizes and alignments to those of target machine */
		curalign = &altable[target];
	} else {
		/* set sizes and alignments to "portable" values */
		curalign = &portalign;
	}

	/* recognize funny name when I see it */
	builtin_va_alist_name = hash( "__builtin_va_alist" );
	
	return( mainp1( argc, argv ) );
#endif
}
/* ctype - are there any funny types? */
ctype( type ) TWORD type;
{
  /* this is appropriate for sunrise/68000 mapping, since "we know"
   * that long == int
   */
	if (qflag){
		switch( BTYPE(type) ){

		case ENUMTY:	/* no such thing */
			MODTYPE(type,UNSIGNED);
			break;

		case LONG:	/* don't waste my time */
			MODTYPE(type,INT);
			break;

		case ULONG:	/* don't waste my time */
			MODTYPE(type,UNSIGNED);
			break;	
			}
	}
	return( type );
}

#ifndef CXREF
/*ARGSUSED*/
allocate_static( id, sz ){
	register struct symtab *s;
	/* put out a static variable declaration */
	s = STP(id);
	if (s == NULL) return;
	outdef( s, libflag?LIB:LDC, USUAL );
	}

/* commdec - common declaration */
commdec( id )
{
	/* 10/14/80 - the compiler complains, and so should lint */
/* this check is not quite right, e.g.,		void (*f)();
 *		-- tsize() performs a similar check, but will return
 *		before so if we've got a pointer.
 *	so, let tsize figure it out.
 *
 *	if( dimtab[ STP(id)->sizoff ] == 0 )
 */
	register struct symtab *q = STP(id);

	if (q == NULL) return;
	(void) tsize(q->stype, q->dimoff, q->sizoff);
	/* put out a common declaration */
	outdef( q, libflag?LIB:LDC, USUAL );
}
#endif	/* #ifndef CXREF */

isitfloat ( s ) char *s;
{	/* s is a character string;
	   if floating point is implemented, set dcon to the value of s */
	/* lint version */
	dcon = atof( s );
	return( FCON );
}

#ifndef CXREF
/* fldcon - check assignment of a constant to a field */
fldcon( p ) register NODE *p;
{
	/* check to see if the assignment is going to overflow,
	  or otherwise cause trouble */
	register s;
	CONSZ v;

	if( !hflag && !pflag ) return;

	s = UPKFSZ(p->in.left->tn.rval);
	v = p->in.right->tn.lval;

	switch( p->in.left->in.type ){

	case CHAR:
	case INT:
	case SHORT:
	case LONG:
	case ENUMTY:
		if( v>=0 && (v>>(s-1))==0 ) return;
		/* "precision lost in assignment to (possibly sign-extended) field" */
		WERROR( MESSAGE( 93 ) );
	default:
		return;

	case UNSIGNED:
	case UCHAR:
	case USHORT:
	case ULONG:
		/* "precision lost in field assignment" */
		if( v<0 || (v>>s)!=0 ) WERROR( MESSAGE( 94 ) );
	}
}
/* outdef - output a definition for the second pass */
outdef( p, lty, mode ) struct symtab *p;
{
	/* if mode is > USUAL, it is the number of args */
	extern char	*strncat( );
	char *fname;
	TWORD t;
	int line;
	static union rec rc;

	if( mode == NOFILE ){
		/*  fname = "???"; */
		fname = p->sfile;
		line = p->suse;
	}
	else if( mode == SVLINE ){
		fname = ftitle;
		line = -p->suse;
		}
	else {
		fname = ftitle;
		line = lineno;
		}
	fsave( fname );
	rc.l.decflag = lty;
	t = p->stype;
	if( mode == DECTY ) t = DECREF(t);
	rc.l.type.aty = t;
	rc.l.type.extra = 0;
	rc.l.type.extra1 = 0;
	astype( &rc.l.type, p->sizoff );
	rc.l.nargs = (mode>USUAL) ? mode : 0;
	rc.l.fline = line;
	fwrite( (char *)&rc, sizeof(rc), 1, stdout );
	rc.l.name = ( p->sclass == STATIC ) ? p->sname : exname( p->sname );
	fwrite( rc.l.name, strlen( rc.l.name ) + 1, 1, stdout );
}
#else	/* if CXREF */

bbcode()	/* CXREF */
{
	/* code for beginning a new block */
	blocknos[blockptr] = nextblock++;
	fprintf(outfp, "B%d\t%05d\n", blocknos[blockptr], lineno);
	blockptr++;
}

becode()	/* CXREF */
{
	/* code for ending a block */
	if (--blockptr < 0)
		uerror("bad nesting");
	else
		fprintf( outfp, "E%d\t%05d\n", blocknos[blockptr], lineno);
}
#endif	/* ifndef CXREF */

/* some useless functions */
int
prtdcon(){;}

/*ARGSUSED*/
void
do_pragma(pragma_id, args)
	int pragma_id;
	NODE *args;
{
	if (args != NIL) tfree(args);
}

int
lookup_pragma(id)
	struct symtab *id;
{
	return 1;
}

void init_pragmas() {;}

