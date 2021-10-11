#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fpa_handle.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <signal.h>
#include <stdio.h>
#include "fpa_support.h"

STATIC long read_msw(n)
int n ;
{ /* read most significant 32 bits of FPA reg n */
long *fpa_pointer ;
fpa_pointer = (long *) (FPABASE + 0xc00 + 8 * n) ;
return( *fpa_pointer ) ;
}

STATIC long read_lsw(n)
int n ;
{ /* read least significant 32 bits of FPA reg n */
long *fpa_pointer ;
fpa_pointer = (long *) (FPABASE + 0xc04 + 8 * n) ;
return( *fpa_pointer ) ;
}

STATIC void write_msw(n, data)
int n ;
long data ;
{ /* write most significant 32 bits of FPA reg n */
long *fpa_pointer ;
fpa_pointer = (long *) (FPABASE + 0xc00 + 8 * n) ;
*fpa_pointer = data ;
}
 
STATIC void write_lsw(n, data)
int n ;
long data ;
{ /* write most significant 32 bits of FPA reg n */
long *fpa_pointer ;
fpa_pointer = (long *) (FPABASE + 0xc04 + 8 * n) ;
*fpa_pointer = data ;
}

STATIC double read_double(n)
int n ;
{ /* read FPA register n as double */
union doublekluge k ;

k.i[0] = read_msw(n) ;

k.i[1] = read_lsw(n) ;
return(k.x) ;
}

STATIC void write_double(n, data)
int n ; double data ;
{ /* write FPA register n as double */
union doublekluge k ;

k.x = data ;
write_msw(n, k.i[0]) ;
write_lsw(n, k.i[1]) ;
}

static void fpa_readpipe ( active, next )
struct fpa_inst *active, *next ;

        /* Procedure to read the active and next FPA instructions from pipe. */

{
long *fpa_pointer ;
long datum ;

fpa_pointer = (long *) (FPABASE + 0xf20) ;
datum = *(fpa_pointer++) ;
active->a[0].kluge.kint = (datum >> 16) &~ 3 ;
active->a[1].kluge.kint = datum &~ 3 ;
datum = *(fpa_pointer++) ;
  next->a[0].kluge.kint = (datum >> 16) &~ 3 ;
  next->a[1].kluge.kint = datum &~ 3 ;
active->a[0].data = *(fpa_pointer++) ;
active->a[1].data = *(fpa_pointer++) ;
  next->a[0].data = *(fpa_pointer++) ;
  next->a[1].data = *(fpa_pointer++) ;
}

static void fpa_rewrite ( x )
struct fpa_inst *x ;

        /* Procedure to rewrite an FPA instruction into the FPA pipe. */

{
int i ;
struct fpa_access *ap ;
long *fpa_pointer ;

for ( i = 0 ; i < 2 ; )
        {
        ap = (struct fpa_access *) &(x->a[i++]) ;
        if (ap->kluge.kis.valid != 0) return ;
                {
                fpa_pointer = (long *) (FPABASE + (ap->kluge.kint & 0x1ffc)) ;
                *fpa_pointer = ap->data ;
                }
        }
}

static void print_inst(x)
struct fpa_inst *x ;

        /* Procedure to display an FPA instruction for debugging. */

{
int i ;
struct fpa_access *ap ;

for ( i = 0 ; i < 2 ; )
        {
        ap = (struct fpa_access *) &(x->a[i++]) ;
                {
                fprintf(stderr," access %1d: %8x = ",i,ap->kluge.kint) ;
		if (ap->kluge.kis.valid)
			fprintf(stderr," invalid ") ;
		else
			fprintf(stderr,"   valid ") ;
		fprintf(stderr," op %2x reg %2d prec %1d data %8X \n",
                ap->kluge.kis.op,ap->kluge.kis.reg1,ap->kluge.kis.prec,
                ap->data) ;
                fflush(stderr) ;
		}
        }
}

int fpa_handle( scp)
struct sigcontext *scp ;

{
int *fpa_pointer, ierr, wstatus, mode, inexactmask ;
int oldstatus81, newstatus81, oldmode81, newmode81 ;
struct fpa_inst active, next ;
int found ;

fpa_pointer = (int *) 0xe0000f1c ;
ierr = (*fpa_pointer >> 16) & 0xff ; 	/* Immediate Read Ierr. */
fpa_pointer = (int *) 0xe0000f3c ;
wstatus = *fpa_pointer & 0xffff ;	/* Stable Read Weitek Status. */
fpa_pointer = (int *) 0xe0000f38 ;
mode = *fpa_pointer & 0xf ;		/* Stable Read Weitek Mode. */
fpa_pointer = (int *) 0xe0000f14 ;
inexactmask = *fpa_pointer & 0x1 ;	/* Immediate Read Inexact Mode. */
fpa_readpipe( &active, &next ) ;
fpa_pointer = (int *) 0xe0000f84 ;
*fpa_pointer = 0 ;			/* Stable Clear Pipe. */
if (ierr != 0x20)
	{ /* Anything other than hung pipe. */
	found = 0 ;
	fprintf(stderr,"\n pc %X ",scp->sc_pc ) ;
	fprintf(stderr," ierr %x ",ierr) ;
	fprintf(stderr," fpastatus %x ",wstatus) ;
	fprintf(stderr," fpamode %x ",mode) ;
	fprintf(stderr," inexactmask %x ",inexactmask) ;
	fprintf(stderr,"\n active pipe instruction: \n") ; print_inst(&active) ;
	fprintf(stderr,"   next pipe instruction: \n") ; print_inst(&next) ;
	fflush(stderr) ;
	return(found) ;
	}
newmode81 = 0 ;
oldmode81 = Wmode(newmode81) ; /* Save old 81 mode and write new. */
if (active.a[0].kluge.kis.prec)
	newmode81 = 128 ; /* double */
else
	newmode81 = 64 ; /* single */
newmode81 = newmode81 + (oldmode81 & 0xff30 ) ; /* New rounding precision. */
Wmode(newmode81) ; /* Save old 81 mode and write new. */
newstatus81 = 0 ;
oldstatus81 = Wstatus(newstatus81) ; /* Clear 81 status. */
if (fpa_81comp( &active, scp->sc_pc ))
        {
        found = 1 ;
	newstatus81 = Wstatus(oldstatus81) ;
	oldstatus81 = newstatus81 | (oldstatus81 & 0xff) ; 
		/* Preserve old accrued exception bits. */
        if (((oldstatus81 & 0x8) != 0) && ((oldmode81 & 0x200) == 0))
		{ /* Turn off FPA inexact detection - not needed
			for accrued status or for trap enable. */
		fpa_pointer = (int *) 0xe0000f14 ;
		*fpa_pointer = 0 ;	
		}
	}
else
	{
	found = 0 ;
	fprintf(stderr," pc %X ",scp->sc_pc ) ;
	fprintf(stderr," ierr %x ",ierr) ;
	fprintf(stderr," fpastatus %x ",wstatus) ;
	fprintf(stderr," fpamode %x ",mode) ;
	fprintf(stderr," inexactmask %x ",inexactmask) ;
	fprintf(stderr,"\n active pipe instruction: \n") ; print_inst(&active) ;
	fprintf(stderr,"   next pipe instruction: \n") ; print_inst(&next) ;
	fflush(stderr) ;
	}
Wstatus(oldstatus81) ;	/* Restore status as recomputed. */
fpa_rewrite ( &next ) ;
return(found) ;
}
