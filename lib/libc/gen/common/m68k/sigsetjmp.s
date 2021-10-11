	.data
/*	.asciz	"@(#)sigsetjmp.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"
#include "PIC.h"
#include "fpcrtdefs.h"
#include "Wdefs.h"
#include <syscall.h>

|setjmp, longjmp
|
|	longjmp(a, v)
|causes a "return(v)" from the
|last call to
|
|	setjmp(v)
|by restoring d2-d7,a2-a7 (all the register variables) and
|adjusting the stack.  The signal mask is restored as well.
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
|
|sigsetjmp, siglongjmp
|
|	siglongjmp(a, v)
|causes a "return(v)" from the
|last call to
|
|	sigsetjmp(v, savemask)
|by restoring d2-d7,a2-a7 (all the register variables) and
|adjusting the stack.  If "savemask" was set in that call, the signal mask
|saved in that call is restored as well.
|
|sigjmp_buf is the same as jmp_buf, but with an extra longword at the
|beginning; the value of "savemask" is stored there.

	.text
#ifndef S5EMUL
ENTRY(setjmp)
#endif
	movl	PARAM,a0	/* pointer to jmp_buf */
	jra	setjmpsav	/* always save signal mask */

ENTRY(sigsetjmp)
	movl	PARAM,a0	/* pointer to sigjmp_buf */
	movl	PARAM2,a0@+	/* store "savemask" */
	beq	setjmpnosav	/* if "savemask" 0, don't save signal mask */

setjmpsav:
	pea	0
	pea	0		/* fake PC making system call interface happy */
	pea	SYS_sigblock	/* get signal mask */
	trap	#0
	addql	#8,sp
	movl	d0,a0@(4)	/* save signal mask of caller */

setjmpnosav:
	subql	#8,sp		/* space for current struct sigstack */
	pea	sp@		/* get current values */
	pea	0		/* no new values */
	pea	0		/* fake PC making system call interface happy */
	pea	SYS_sigstack	/* get onsigstack status of caller */
	trap	#0
	lea	sp@(16),sp	/* pop args plus signal stack value */
	movl	sp@+,a0@	/* save onsigstack status of caller */
	movl	PARAM0,a0@(12)	/* save pc of caller */
#ifdef PROF
	unlk	a6		/* restore value of A6 and SP on entry */
#endif PROF
	lea	sp@(4),a1	/* sp before we were called */
	movl	a1,a0@(8)	/* save sp of caller */
	moveml	d2-d7/a2-a6,a0@(20) /* rest of the registers */
#ifdef PIC
	PIC_SETUP(a1)
	movel 	a1@(_fp_switch:w),a1
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

#ifndef S5EMUL
ENTRY(longjmp)
#endif
	movl	PARAM,a0	/* pointer to jmp_buf */
	moveq	#1,d1		/* always restore signal mask */
	jra	longjmpcmn

ENTRY(siglongjmp)
	movl	PARAM,a0	/* pointer to sigjmp_buf */
	movl	a0@+,d1		/* "savemask" flag */

longjmpcmn:
#ifdef PIC
	PIC_SETUP(a1)
	movel	a1@(_fp_switch:w),a1
	movel	a1@,d0
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
	tstl	d1		/* restore signal mask? */
	bne	1f		/* yes */
	movl	a0@(8),sp	/* restore sp */
	movl	a0@(12),sp@-	/* restore pc of call to setjmp to stack */
	rts			/* and jump without restoring signal mask */

1:
	movl	a0,sp@-		/* address of jmp_buf is address of sigcontext */
	pea	139		/* sigcleanup call */
	trap	#0		/* restore rest - including pc, so no return */
