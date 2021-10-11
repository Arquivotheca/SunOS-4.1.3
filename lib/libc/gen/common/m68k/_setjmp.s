	.data
/*	.asciz	"@(#)_setjmp.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"
#include "fpcrtdefs.h"
#include "Wdefs.h"

|_setjmp, _longjmp
|
|	_longjmp(a, v)
|causes a "return(v)" from the
|last call to
|
|	_setjmp(v)
|by restoring d2-d7,a2-a7 (all the register variables) and
|adjusting the stack
|
|jmp_buf is set up as:
|
|	_________________
|0	|   onsigstack	|
|	-----------------
|4	|    sigmask	|
|	-----------------
|8	|       sp	|
|	-----------------
|12	|       pc	|
|	-----------------
|16	|       ps	|
|	-----------------
|20	|	d2	|
|	-----------------
|	|	...	|
|	-----------------
|40	|	d7	|
|	-----------------
|44	|	a2	|
|	-----------------
|	|	...	|
|	-----------------
|60	|	a6	|
|	-----------------
|64	|	fp2a	|
|	-----------------
|	|	fp2b	|
|	-----------------
|	|	fp2c	|
|	-----------------
|	|	...	|
|	-----------------
|	|	fp7a	|
|	-----------------
|	|	fp7b	|
|	-----------------
|	|	fp7c	|
|	-----------------
|136	|	fpa4a	|
|	-----------------
|	|	fpa4b	|
|	-----------------
|	|	...	|
|	-----------------
|	|	fpa15a	|
|	-----------------
|	|	fpa15b	|
|	-----------------
|232

	.text

#ifdef S5EMUL
ALTENTRY(setjmp)
#endif
ENTRY(_setjmp)
	movl	PARAM,a0	/* pointer to jmp_buf */
	movl	PARAM0,a0@(12)	/* pc */
#ifdef PROF
	unlk	a6		/* restore A6 */
#endif
	movl	sp,a0@(8)	/* CURRENT sp */
	moveml	d2-d7/a2-a6,a0@(20) /* rest of the registers */
#ifdef PIC
	PIC_SETUP(a1)
	movel	a1@(_fp_switch:w),a1
	movel	a1@,d0
#else
	movel   _fp_switch,d0
#endif
        cmpw    #FP_MC68881,d0
        jne     3f
	fmovemx	fp2-fp7,a0@(64)
	jra	1f
3:	
	cmpw	#FP_SUNFPA,d0
	jne	1f
	fmovemx	fp2-fp7,a0@(64)
	lea	a0@(136),a0
        movel   #(FPABASEADDRESS+0xc00+32),a1
        movel   a1@+,a0@+               | fpa4
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa5
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa6
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa7
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa8
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa9
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa10
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa11
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa12
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa13
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa14
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa15
        movel   a1@+,a0@+             
1: 
	clrl	d0		/* return 0 */
	rts

#ifdef S5EMUL
ALTENTRY(longjmp)
#endif
ENTRY(_longjmp)
	movl	PARAM,a0	/* pointer to jmp_buf */
#ifdef PIC
	PIC_SETUP(a1)
	movel	a1@(_fp_switch:w),a1
	movel	a1@, d0
#else
	movel   _fp_switch,d0
#endif
        cmpw    #FP_MC68881,d0
        jne     3f
	fmovemx	a0@(64),fp2-fp7
	jra	1f
3:	
	cmpw	#FP_SUNFPA,d0
	jne	1f
	fmovemx	a0@(64),fp2-fp7
	lea	a0@(136),a1
        movel   #(FPABASEADDRESS+0xc00+32),a0
        movel   a1@+,a0@+               | fpa4
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa5
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa6
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa7
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa8
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa9
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa10
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa11
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa12
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa13
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa14
        movel   a1@+,a0@+             
        movel   a1@+,a0@+               | fpa15
        movel   a1@+,a0@+             
	lea	a1@(-232),a0
1: 
	movl	PARAM2,d0	/* value returned */
	bne	1$
	moveq	#1,d0
1$:
	moveml	a0@(20),d2-d7/a2-a6	/* restore most of the registers */
	movl	a0@(8),sp	/* restore sp */
	movl	a0@(12),sp@	/* restore pc of call to setjmp to stack */
	rts
