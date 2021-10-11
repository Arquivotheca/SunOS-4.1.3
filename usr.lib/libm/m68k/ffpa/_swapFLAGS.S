	.data
	.asciz	"@(#)_swapFLAGS.S 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "DEFS.h"
#include "Wdefs.h"

/* 
 * _swapRD(rd) 	exchanges rd with the current rounding direction.
 * _swapRP(rp) 	exchanges rp with the current rounding precision.
 * _swapTE(ex)  exchanges ex with the current exception trap enable bits.
 * _swapEX(ex) 	exchanges ex with the current accrued exception. 
 *		Here ex is a five bit value where each bit (cf. enum 
 *		fp_exception_type in <sys/ieeefloatingpoint.h>) corresponds 
 *		to either an exception-occurred accrued status flag or an
 *		exception trap enable flag (0 off,1 on):
 * 			bit 0 :  inexact flag
 * 			bit 1 :  division by zero flag
 * 			bit 2 :  underflow flag
 * 			bit 3 :  overflow flag
 * 			bit 4 :  invalid flag
 */

|#include <floatingpoint.h>

|extern unsigned fpsr, fpcr;

|enum fp_direction_type _swapRD(rd)
|enum fp_direction_type rd;
|{
|	/* 
||	   swap rd with the current rounding direction
|	 */
|#define MASK 3
|#define SHIFT 4
|register unsigned t ;
|	fp_direction = rd ;
|	t = fpcr ;
|	fpcr = (t &~ (MASK << SHIFT)) | (((unsigned) rd) << SHIFT) ;
|	return (enum fp_direction_type) ((t >> SHIFT) & MASK) ;
|}
|
|enum fp_precision_type _swapRP(rp)
|enum fp_precision_type rp;
|{
|	/* 
|	   swap rp with the current rounding precision
|	 */
|#define MASK 3
|#define SHIFT 6
|register unsigned t ;
|	fp_precision = rp ;
|	t = fpcr ;
|	fpcr = (t &~ (MASK << SHIFT)) | (((unsigned) rp) << SHIFT) ;
|	return (enum fp_precision_type) ((t >> SHIFT) & MASK) ;
|}
|
|fp_exception_field_type _swapEX(ex)
|fp_exception_field_type ex;
|{
|	/* 
|	   swap ex with the current accrued exception mode
|	 */
|#define MASK 0x1f
|#define SHIFT 3
|register unsigned t ;
|	fp_accrued_exceptions = ex ;
|	t = fpsr ;
|	fpsr = (t &~ (MASK << SHIFT)) | (((unsigned) ex) << SHIFT) ;
|	return (fp_exception_field_type) ((t >> SHIFT) & MASK) ;
|}
|
|fp_exception_field_type _swapTE(ex)
|fp_exception_field_type ex;
|{
|	/* 
|	   swap ex with the current exception trap enable bits
|	 */
|#define MASK 0x1f
|#define SHIFT 9
|register unsigned s, t ;
|	s = ((unsigned) ex) << SHIFT;
|	if (s & (1 << SHIFT)) s |= (1 << (SHIFT-1)) ; 
|					/* if INEX2 then INEX1 */
|	if (s & (0x10 << SHIFT)) s |= 0x60 << SHIFT ; 
|					/* if OPERR then BSUN & SNAN. */
|	t = fpcr ;
|	fpcr = (t &~ (MASK << SHIFT)) | s ;
|	return (fp_exception_field_type) ((t >> SHIFT) & MASK) ;
|}
|
	.text
RTENTRY(__swapRD)
	movl	PARAM,d1		| d1 gets unshifted new RD.
	movel	d1,_fp_direction	| _fp_direction gets unshifted new RD.
	lsll	#4,d1			| d1 gets new RD<<4.
	fmovel	fpcr,d0			| d0 gets 68881 mode.
	andl	#-49,d0			| Clear RD bits.
	orl	d1,d0			| d0 gets new RD<<4.
	fmovel	d0,fpcr			| Store new fpcr.
	lsrl	#2,d1			| d1 gets new RD<<2.
	btst	#3,d1			| May need to allow for difference
					| between Motorola and Weitek.
	jeq	1f			| Branch if rn or rz.
	bchg	#2,d1			| Interchange rp and rm.
1:
	movl	d2,a0
	fpmove	fpamode,d2
	movl	d2,d0
	andl	#3,d0			| Preserve fpamode bits 0 & 1.
	orl	d1,d0
	fpmove	d0,fpamode
	movl	d2,d0
	lsrl	#2,d0
	moveq	#3,d2
	andl	d2,d0
	movl	a0,d2
	RET

RTENTRY(__swapRP)
	movel	#-1,d0			| Rounding precision not supported.
	RET

RTENTRY(__swapEX)
	movl	PARAM,d1
	movel	d1,_fp_accrued_exceptions
	movl	d2,a0
	fmovel	fpsr,d2
	movl	d2,d0
	andl	#-249,d0
	lsll	#3,d1
	orl	d1,d0
	fmovel	d0,fpsr
        btst    #3,d0
        bnes    1f                      | Branch if accrued inexact on.
        movel   #1,FPABASEADDRESS+0xf14 | Enable FPA inexact mask.
1:
	movl	d2,d0
	lsrl	#3,d0
	moveq	#31,d2
	andl	d2,d0
	movl	a0,d2
	RET

RTENTRY(__swapTE)
        movl    PARAM,d1
        movel	d6,a0
	movel	d7,a1
        moveq   #9,d7
        lsll    d7,d1
        btst    d7,d1
        jeq     1f
        orw     #256,d1
1:
        btst    #13,d1
        jeq     1f
        orw     #49152,d1
1:
        fmovel  fpcr,d6
        movl    d6,d0
        andl    #-15873,d0
        orl     d1,d0
        fmovel  d0,fpcr
        btst    #9,d0
        beqs    1f                      | Branch if inexact disabled.
        movel   #1,FPABASEADDRESS+0xf14 | Enable FPA inexact mask.
1:
        movl    d6,d0
        lsrl    d7,d0
        moveq   #31,d1
        andl    d1,d0
        movel	a0,d6
	movel	a1,d7
	RET
