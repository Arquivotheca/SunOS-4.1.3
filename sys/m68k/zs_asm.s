/*
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "assym.s"
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/psl.h>
#include <sundev/zsreg.h>

	.data
	.asciz	"@(#)zs_asm.s 1.1 92/07/30"
	.even
	.text
/*
 * We come here for auto-vectored interrupts
 * (All Sun-2s and the Sun-3 Model 25)
 * We assume that the most recently interrupting
 * chip is interrupting again and read the vector out of the
 * chip and branch to the right routine. The "special receive"
 * interrupt routine gets control if the assumption was wrong
 * and performs the longer task of figuring out who is really interrupting
 * and why.
 */
	.globl	zslevel6, _zscurr
zslevel6:
	movl	a1,sp@-			| save a1
	movl	a0,sp@-			| save a0
	movl	_zscurr,a0		| get most active channel desc
	movl	a0@(ZS_ADDR),a1		| get hardware channel addr
	movb	#2,a1@			| read from reg 2 in channel B
/* we need at least 1.6 usec recovery time between here ... */
	movl	d1,sp@-			| recovery time - save d1, d0
	movl	d0,sp@-			| recovery time - save d1, d0
#ifdef sun3
        nop; nop; nop; nop; nop; nop    | additional recovery time
#endif sun3
#ifdef sun3x
	nop; nop; nop; nop; nop; nop	| additional recovery time
	nop; nop; nop;
#endif sun3x
/* ... and here */
	movb	a1@,d0
	btst    #3,d0			| channel A?
	beq	1f			| no, skip
	subw	#ZSSIZE,a0		| point to channel A
1:	andw	#6,d0			| get vector bits
	addw	d0,d0			| get multiple of 4
	pea	a0@			| push argument
	movl	a0@(0,d0:w),a0		| get routine address
	jsr	a0@			| call routine
	movl	sp@+,a0			| pop argument
	movl	a0@(ZS_ADDR),a1		| get hardware channel addr
	movb	#ZSWR0_CLR_INTR,a1@	| reset interrupt
	moveml	sp@+,#0x0303		| restore all saved regs
	addql	#1,_cnt+V_INTR		| increment io interrupt count
	rte				| and return!

#if defined(sun3) || defined(sun3x)
/*
 * Vector stubs for SCCs
 * Each SCC requires 8 vectors - 4 for each channel, channel B before channel A
 */
	.globl	_zscom
#define	ZSSTUB(chip,chan,src)				\
zsv/**/chip/**/chan/**/src:				\
zs/**/chip/**/chan/**/src = ZSSIZE*(chip*2+chan);	\
zv/**/chip/**/chan/**/src = ZSSIZE*(chip*2+chan)+(4*src); \
	moveml	#0xC0C0,sp@-;				\
	pea	_zscom+zs/**/chip/**/chan/**/src;	\
	movl	_zscom+zv/**/chip/**/chan/**/src,a0;	\
	jsr	a0@;					\
	addql	#4,sp;					\
	movl	_zscom+zs/**/chip/**/chan/**/src+ZS_ADDR,a1; \
	movb	#ZSWR0_CLR_INTR,a1@;			\
	moveml  sp@+,#0x0303;				\
	addql	#1,_cnt+V_INTR;				\
	rte;						\
	.data;						\
	.long	zsv/**/chip/**/chan/**/src;		\
	.text

.data
	.globl	_zsvectab
_zsvectab:
.text
#include "zs.h"
#if NZS > 0
	ZSSTUB(0,1,0); ZSSTUB(0,1,1); ZSSTUB(0,1,2); ZSSTUB(0,1,3);
	ZSSTUB(0,0,0); ZSSTUB(0,0,1); ZSSTUB(0,0,2); ZSSTUB(0,0,3);
#endif
#if NZS > 1
	ZSSTUB(1,1,0); ZSSTUB(1,1,1); ZSSTUB(1,1,2); ZSSTUB(1,1,3);
	ZSSTUB(1,0,0); ZSSTUB(1,0,1); ZSSTUB(1,0,2); ZSSTUB(1,0,3);
#endif
#if NZS > 2
	ZSSTUB(2,1,0); ZSSTUB(2,1,1); ZSSTUB(2,1,2); ZSSTUB(2,1,3);
	ZSSTUB(2,0,0); ZSSTUB(2,0,1); ZSSTUB(2,0,2); ZSSTUB(2,0,3);
#endif
#if NZS > 3
ERROR - too many ZSs
#endif
#endif sun3 || sun3x

#if defined(sun3) || defined(sun3x)
#include <machine/interreg.h>
/*
 * Turn on a zs soft interrupt - Sun-3/Sun-3x
 */
	ENTRY(setzssoft)
	bset	#IR_SOFT_INT3_BIT,INTERREG
	rts

/*
 * Test and clear a zs soft interrupt - Sun-3
 */
	ENTRY(clrzssoft)
	movl	#0,d0
	bclr	#IR_SOFT_INT3_BIT,INTERREG
	jeq	1f
	movl	#1,d0
1:	rts
#endif sun3 || sun3x

#ifdef sun2
#include <sun2/enable.h>
ZSSOFTMASK = ENA_SOFT_INT_3
/*
 * Turn on a zs soft interrupt - Sun-2
 */
	ENTRY(setzssoft)
	movc	dfc,a1
	movl	#FC_MAP,d0		| set to address FC_MAP space
	movc	d0,dfc
1:
	movl	#ZSSOFTMASK,d0
	orw	d0,enablereg		| enable the interrupt
	movw	enablereg,d0		| get it in a reg
	movsw	d0,ENABLEREG		| put enable register back
	cmpw	enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	movc	a1,dfc			| restore dfc
	rts

/*
 * Test and clear a zs soft interrupt - Sun-2
 */
	ENTRY(clrzssoft)
	movw	enablereg,d0
	andl	#ZSSOFTMASK,d0		| clear d0 if no intr
	beq	2f
	movc	dfc,a1
	movl	#FC_MAP,d0		| set to address FC_MAP space
	movc	d0,dfc
1:
	movl	#~ZSSOFTMASK,d0
	andw	d0,enablereg		| enable the interrupt
	movw	enablereg,d0		| get it in a reg
	movsw	d0,ENABLEREG		| put enable register back
	cmpw	enablereg,d0		| see if someone higher changed it
	bne	1b			| if so, try again
	movc	a1,dfc			| restore dfc
	movl	#1,d0			| return true
2:
	rts
#endif sun2

/*
 * Read a control register in the chip
 */
	ENTRY(zszread)
	movl	sp@(4),a1		| chip address
	movb	sp@(0xb),a1@		| write register number
#if defined(sun3) || defined(sun3x)
	moveq   #20,d0			| DELAY(2)
	movl    _cpudelay,d1
	asrl    d1,d0
1:	subql   #1,d0
	bgts    1b
#else
/* need 1.6 usec recovery here */
	clrl	d0
	nop; nop; nop
/* that was actually 1.8  */
#endif sun3 || sun3x
	movb	a1@,d0			| get returned register
	rts

/*
 * Write a control register in the chip
 */
	ENTRY(zszwrite)
	movl	sp@(4),a1		| chip address
	movb	sp@(0xb),a1@		| write register number
#if defined(sun3) || defined(sun3x)
	moveq   #20,d0			| DELAY(2)
	movl    _cpudelay,d1
	asrl    d1,d0
1:	subql   #1,d0
	bgts    1b
#else
/* 1.6 usec recovery plus slop for buffered video writes */
	nop; nop; nop; nop; nop
#endif sun3 || sun3x
	movb	sp@(0xf),a1@		| write data
	rts
