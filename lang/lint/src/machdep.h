/*	@(#)machdep.h 1.1 92/07/30 SMI; from S5R2 macdefs 1.2	*/

/*
 * "Machine" parameters for "lint"
 */

# ifndef _machdep_
# define _machdep_

#if CXREF
# define LINT
extern char infile[];
#else
# define EXIT(x)	lerror( "", 06 )
#endif

/*	register cookie for stack pointer */

# define STKREG 9

#if CXREF
# include <stdio.h>
extern FILE *outfp;	/* CXREF */
# define efcode()	/* CXREF */
# define bfcode(x,y)	retlab = 1	/* CXREF */
# define defnam(p)	/* CXREF */
# define commdec(x)	/* CXREF */
# define aocode(x)	/* CXREF */
# define ejobcode(x)	/* CXREF */
# define ecode(p)
# define swcomp(p) 
# define fldty(p)
#endif
#ifdef RTOLBYTES
# define makecc(val,i)  lastcon |= val<<(8*i);  /* pdp-11 womp next char  */
#else
# define makecc(val,i)	lastcon = i ? (val<<8)|lastcon : val
#endif

# define  ARGINIT 288 /* initial offset for arguments */
# define  AUTOINIT 0   /* initial automatic offset */

#ifdef CXREF
#define	SZCHAR		8
#define	SZINT		16
#define	SZFLOAT		32
#define	SZDOUBLE	64
#define	SZLONG		32
#define	SZSHORT		16
#define	SZPOINT		16
#define	ALCHAR		8
#define	ALINT		16
#define	ALFLOAT		32
#define	ALDOUBLE	64
#define	ALLONG		32
#define	ALSHORT		16
#define	ALPOINT		16
#define	ALSTRUCT	16
#else
extern struct alignment {
	int	szchar,
		szint,
		szfloat,
		szdouble,
		szlong,
		szshort,
		szpoint;
	int	alchar,
		alint,
		alfloat,
		aldouble,
		allong,
		alshort,
		alpoint,
		alstruct;
} * curalign;

/*
 * Sizes and alignment rules are target-dependent.
 */
# define SZCHAR		(curalign->szchar)
# define SZINT		(curalign->szint)
# define SZFLOAT	(curalign->szfloat)
# define SZDOUBLE	(curalign->szdouble)
# define SZLONG		(curalign->szlong)
# define SZSHORT	(curalign->szshort)
# define SZPOINT	(curalign->szpoint)
# define ALCHAR		(curalign->alchar)
# define ALINT		(curalign->alint)
# define ALFLOAT	(curalign->alfloat)
# define ALDOUBLE	(curalign->aldouble)
# define ALLONG		(curalign->allong)
# define ALSHORT	(curalign->alshort)
# define ALPOINT	(curalign->alpoint)
# define ALSTRUCT	(curalign->alstruct)

# define mc68020AL 0
# define sparcAL 1
# define i386AL 2
#endif

/*	size in which constants are converted */
/*	should be long if feasible */

# define CONSZ long
# define CONFMT "%ld"
# define CONOFMT "%lo"

/* size in which offsets are kept */
/* should be large enough to cover address space in bits */

# define OFFSZ long

/* 	character set macro */

# define  CCTRANS(x) x

/* many macro definitions for functions irrelevant to lint */

# define locctr(n) 0
# define getlab() 10
# define bccode()
# define cendarg()
# define incode(a,s) (inoff += (s))
# define fincode(a,s) (inoff += (s) )
# define vfdzero(n) (inoff += (n))
# define aobeg()
# define aoend()
# define branch(n)	(n)
# define defalign(n)	(n)
# define deflab(n)	(n)
# define SUTYPE int
# define asmout()
# define fileargs(name)
# define strcode(s, slen)

#define PUSH_ALLOCATION( sp ) *sp++ = autooff; *sp++ = regvar
#define POP_ALLOCATION( sp )  regvar = *--sp; autooff = *--sp

#ifndef CXREF
#    define	WERROR	lwerror
#    define	UERROR	luerror
#    define	MESSAGE(x)	(x)
#endif

# define NOFORTRAN { WERROR( MESSAGE( 42 ) ); }

# define ENUMSIZE(low,high) INT

#endif _machdep_

