#ifndef lint
static	char sccsid[] = " @(#)allo.c 1.1 92/07/30	Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * The routines in this file historically reside in ../mip/pftn.c
 * They are, however, machine-dependent routines having to do with
 * allocation and initialization of variables and structure members
 * so have been seperated out and moved to the machine-dependent directory.
 */


# include "cpass1.h"

# include "messages.h"

#ifndef CXREF
extern int host, target;
/*
 *struct alignment al68k = {
 *      SZCHAR, SZINT,   SZFLOAT, SZDOUBLE, SZLONG,  SZSHORT, SZPOINT,
 *      SZCHAR, SZSHORT, SZSHORT, SZSHORT,  SZSHORT, SZSHORT, SZSHORT, SZSHORT
 *};
 *struct alignment alsunrise = {
 *      SZCHAR, SZINT,   SZFLOAT, SZDOUBLE, SZLONG, SZSHORT,  SZPOINT,
 *      SZCHAR, SZINT,   SZINT,   SZDOUBLE, SZINT,  SZSHORT,  SZINT,   SZCHAR
 *};
 *struct alignment *curalign = &alsunrise; */  /* assume 68k -> sunrise */

struct alignment altable [3] = {
    { 8,	/* size of char */
      32,	/* size of int */
      32,	/* size of float */
      64,	/* size of double */
      32,	/* size of long */
      16,	/* size of short */
      32,	/* size of pointer */
      8,	/* alignment of char */
      16,	/* alignment of int */
      16,	/* alignment of float */
      16,	/* alignment of double */
      16,	/* alignment of long */
      16,	/* alignment of short */
      16,	/* alignment of pointer */
      16	/* alignment of structure */
    },					/*  alignment for 68K  */
    { 8,	/* size of char */
      32,	/* size of int */
      32,	/* size of float */
      64,	/* size of double */
      32,	/* size of long */
      16,	/* size of short */
      32,	/* size of pointer */
      8,	/* alignment of char */
      32,	/* alignment of int */
      32,	/* alignment of float */
      64,	/* alignment of double */
      32,	/* alignment of long */
      16,	/* alignment of short */
      32,	/* alignment of pointer */
      8		/* alignment of structure */
    },					/* alignment for SPARC */
    { 8,	/* size of char */
      32,	/* size of int */
      32,	/* size of float */
      64,	/* size of double */
      32,	/* size of long */
      16,	/* size of short */
      32,	/* size of pointer */
      8,	/* alignment of char */
      32,	/* alignment of int */
      32,	/* alignment of float */
      32,	/* alignment of double */
      32,	/* alignment of long */
      16,	/* alignment of short */
      32,	/* alignment of pointer */
      8		/* alignment of structure */
    }					/* alignment for 386 */
};

/* these are appropriate for the -p flag */
/* October 1980 -
 *	It would be nice if these values could be set so as to eliminate any
 *	possibility of machine dependency.  When we tried this by making
 *	the sizes relatively prime, several obscure problems were encountered.
 *	PCC apparently makes some sort of assumption(s) that can result in a
 *	compiler error when these sizes are used.
 */
struct alignment portalign = {
      8,	/* size of char */
      16,	/* size of int */
      32,	/* size of float */
      64,	/* size of double */
      32,	/* size of long */
      16,	/* size of short */
      16,	/* size of pointer */
      8,	/* alignment of char */
      16,	/* alignment of int */
      32,	/* alignment of float */
      64,	/* alignment of double */
      32,	/* alignment of long */
      16,	/* alignment of short */
      16,	/* alignment of pointer */
      16	/* alignment of structure */
};					/* alignment for "portable" */

struct alignment *curalign;


void
swapalign(){
	if (curalign == &altable[host] )
		curalign = &altable[target];
	else
		curalign = &altable [host];
}
#endif

extern int errline,lineno;

typedef unsigned long ulong;

unsigned int offsz;

long
upoff( size, alignment, poff )
register alignment;
register long *poff;
{
	/* update the offset pointed to by poff; return the
	/* offset of a value of size `size', alignment `alignment',
	/* given that off is increasing */

	register long off;

	off = *poff;
	SETOFF( off, alignment );
	if ( ( ( (ulong) offsz ) - ( (ulong) off ) ) < (ulong) size ){
		if( instruct!=INSTRUCT )cerror("too many local variables");
		else cerror("Structure too large");
		}
	*poff = off+size;
	return( off );
	}


oalloc( p, poff ) register struct symtab *p; register OFFSZ *poff; {
	/* allocate p with offset *poff, and update *poff */
	register al, tsz;
	register long off;
	int otheral,  otheroff;
	long noff, xoff, upoff();

	al = talign( p->stype, p->sizoff );
	xoff = noff = off = *poff;
	tsz = tsize( p->stype, p->dimoff, p->sizoff );
#ifdef BACKAUTO
	if( p->sclass == AUTO ){
		if ( ( ( (ulong) offsz ) - ( (ulong) off ) ) < (ulong) tsz )
			cerror("too many local variables");
		/*
		 * longword-align long auto variables, without
		 * changing internal alignments of structures.
		 */
		if (tsz >= SZLONG && al < SZLONG) {
			al = SZLONG;
		}
		noff = off + tsz;
		SETOFF( noff, al );
		off = -noff;
	}
	else
#endif
			/* align char/short PARAM and REGISTER to ALINT */
		if( (p->sclass == PARAM || p->sclass == REGISTER) && ( tsz < SZINT ) ){
			off = upoff( SZINT, ALINT, &noff );
# ifndef RTOLBYTES
			off = noff - tsz;
#endif
		} else {
			off = upoff( tsz, al, &noff );
#ifndef CXREF
			if (!pflag) {
				swapalign();
				otheral = talign( p->stype, p->sizoff );
				otheroff = upoff( tsz, otheral, &xoff );
				swapalign();
				if ( off != otheroff )
					if (hflag) werror("changing offset for struct/union element %s", p->sname );
			}
#endif
		}

	if( p->sclass != REGISTER ){ /* in case we are allocating stack space for register arguments */
		if( p->offset == NOOFFSET ) p->offset = off;
		else if( off != p->offset ) return(1);
		}

	*poff = noff;
	return(0);
	}

falloc( p, w, new, pty )  register struct symtab *p; NODE *pty; {
	/* allocate a field of width w */
	/* new is 0 if new entry, 1 if redefinition, -1 if alignment */

	register al,sz,type;

	type = (new<0)? pty->in.type : p->stype;

	/* this must be fixed to use the current type in alignments */
	switch( new<0?pty->in.type:p->stype ){

	case ENUMTY:
		{
			int s;
			s = new<0 ? pty->fn.csiz : p->sizoff;
			al = dimtab[s+2];
			sz = dimtab[s];
			break;
			}

	case CHAR:
#ifdef UFIELDS
		/* fields always unsigned */
		type = UCHAR;
#endif
	case UCHAR:
		al = ALCHAR;
		sz = SZCHAR;
		break;

	case SHORT:
#ifdef UFIELDS
		/* fields always unsigned */
		type = USHORT;
#endif
	case USHORT:
		al = ALSHORT;
		sz = SZSHORT;
		break;

	case INT:
#ifdef UFIELDS
		/* fields always unsigned */
		type = UNSIGNED;
#endif
	case UNSIGNED:
		al = ALINT;
		sz = SZINT;
		break;
#ifdef LONGFIELDS

	case LONG:
#ifdef UFIELDS
		/* fields always unsigned */
		type = ULONG;
#endif
	case ULONG:
		al = ALLONG;
		sz = SZLONG;
		break;
#endif

	default:
		if( new < 0 ) {
			/* "illegal field type" */
			UERROR( MESSAGE( 57 ) );
			al = ALINT;
			}
		else {
			al = fldal( p->stype );
			sz =SZINT;
			}
		}

	if( w > sz ) {
		/* "field too big" */
		UERROR( MESSAGE( 39 ));
		w = sz;
		}

	if( w == 0 ){ /* align only */
		SETOFF( strucoff, al );
		/* "zero size field" */
		if( new >= 0 ) UERROR( MESSAGE( 120 ));
		return(0);
		}

	if( strucoff%al + w > sz ) SETOFF( strucoff, al );
	if( new < 0 ) {
		if( (offsz-strucoff) < w )
			cerror("structure too large");
		strucoff += w;  /* we know it will fit */
		return(0);
		}

	/* establish the field */

	if( new == 1 ) { /* previous definition */
		if( p->offset != strucoff || p->sclass != (FIELD|w) ) return(1);
		}
	p->offset = strucoff;
	if( (offsz-strucoff) < w ) cerror("structure too large");
	strucoff += w;
	p->stype = type;
	fldty( p );
	return(0);
	}

nidcl( p ) NODE *p; { /* handle uninitialized declarations */
	/* assumed to be not functions */
	register class;
	register commflag;  /* flag for labelled common declarations */

	commflag = 0;

	/* compute class */
	if( (class=curclass) == SNULL ){
		if( blevel > 1 ) class = AUTO;
		else if( blevel != 0 || instruct ) cerror( "nidcl error" );
		else { /* blevel = 0 */
			class = noinit();
			if( class == EXTERN ) commflag = 1;
			}
		}
#ifdef LCOMM
	/* hack so stab will come at as LCSYM rather than STSYM */
	if (class == STATIC) {
		extern int stabLCSYM;
		stabLCSYM = 1;
	}
#endif

	defid( p, class );

#ifndef LCOMM
	if( class==EXTDEF || class==STATIC )
#else
	if (class==STATIC) {
		register struct symtab *s = STP(p->tn.rval);
		extern int stabLCSYM;
		int sz = tsize(s->stype, s->dimoff, s->sizoff)/SZCHAR;
		
		stabLCSYM = 0;
		if (sz % sizeof (int))
			sz += sizeof (int) - (sz % sizeof (int));
		allocate_static( s, sz );
	}else if (class == EXTDEF) 
#endif
	{
		/* simulate initialization by 0 */
		beginit(p->tn.rval);
		endinit();
		}
	if( commflag ) commdec( p->tn.rval );
	}


