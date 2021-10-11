/*      @(#)audio_79C30_intr.s 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <machine/intreg.h>
#include <machine/asm_linkage.h>
#include <machine/auxio.h>

#include "assym.s"

#ifdef	sun4m
#define	AUDIO_C_TRAP
#endif	sun4m
/*
 * Normally, AUDIO_C_TRAP is not defined.
 * If handling interrupts from C (or no audio device configured),
 * don't compile anything in this file.
 */
#include "audioamd.h"
#if NAUDIOAMD > 0 && !defined (AUDIO_C_TRAP)


/* Return from interrupt code */
#define	RETI		mov %l0,%psr; nop; nop; jmp %l1; rett %l2
#define	Zero		%g0		/* Global zero register */


/* %l3-%l7 are scratch registers */
#define	Int_Active	%l7
#define	Chip		%l6
#define	Devp		%l5
#define	Cmdp		%l4
#define	Temp1		%l3
#define	Temp2		%l2

/* Int_Active register bits: schedule level 4 intr, stop device when inactive */
#define	Interrupt	1
#define	Active		2

/*
 * %l2 must be saved and restored within the Interrupt Service Routine.
 * It is ok to save it in a global because the ISR is non-reentrant.
 */
	.seg	"data"
	.align	4
save_temp2:.word	0


	.seg	"text"
	.proc	4

/* XXX - 4.0.3c compatibility */
	.global	audio_trap
audio_trap:

	.global	_audio_79C30_intr
_audio_79C30_intr:

	set	save_temp2,Temp1;
	st	Temp2,[Temp1]			/* save one extra register */

	/*
	 * Get the address of the aud_dev_t array.
	 * XXX - assume unit 0 for now.
	 */
	sethi	%hi(_audio_79C30_devices),Devp
	ld	[Devp+%lo(_audio_79C30_devices)],Devp

	ld	[Devp+AUD_DEV_CHIP],Chip	/* get the chip address */
	ldsb	[Chip+AUD_CHIP_IR],Temp1	/* clear interrupt condition */
	ldsb	[Devp+AUD_REC_ACTIVE],Temp1
	tst	Temp1				/* is record active? */
	be	play
	mov	Zero,Int_Active			/* clear the status flag */

	ld	[Devp+AUD_REC_CMDPTR],Cmdp
	tst	Cmdp				/* NULL command list? */
	be	recnull
	mov	1,Temp1

recskip:
	lduh	[Cmdp+AUD_CMD_SKIP],Temp2	/* check skip & done flags */
	tst	Temp2
	be,a	recactive
	or	Int_Active,Active,Int_Active	/* record is active */

	stb	Temp1,[Cmdp+AUD_CMD_DONE]	/* mark this command done */
	ld	[Cmdp+AUD_CMD_NEXT],Cmdp	/* update to next command */
	st	Cmdp,[Devp+AUD_REC_CMDPTR]
	tst	Cmdp				/* end of list? */
	bne	recskip				/* if not, check skip flag */
	or	Int_Active,Interrupt,Int_Active	/* if so, data overflow */

recnull:
	stb	Temp1,[Devp+AUD_REC_ERROR]	/* set error flag */
	b	recintr
	stb	Zero,[Devp+AUD_REC_ACTIVE]	/* disable recording */

	or	Int_Active,Active,Int_Active	/* record is active */
recactive:
	ld	[Devp+AUD_REC_SAMPLES],Temp1
	inc	Temp1				/* increment sample count */
	st	Temp1,[Devp+AUD_REC_SAMPLES]
	ldub	[Chip+AUD_CHIP_BBRB],Temp2	/* get data from device */
	ld	[Cmdp+AUD_CMD_DATA],Temp1
	stb	Temp2,[Temp1]			/* store data in buffer */
	inc	Temp1				/* increment buffer pointer */
	ld	[Cmdp+AUD_CMD_ENDDATA],Temp2
	cmp	Temp1,Temp2			/* buffer complete? */
	bcs	play				/* branch if not */
	st	Temp1,[Cmdp+AUD_CMD_DATA]	/* update buffer pointer */

	mov	1,Temp1
	stb	Temp1,[Cmdp+AUD_CMD_DONE]	/* mark command done */
	ld	[Cmdp+AUD_CMD_NEXT],Cmdp	/* update to next command */
	st	Cmdp,[Devp+AUD_REC_CMDPTR]

recintr:
	or	Int_Active,Interrupt,Int_Active	/* schedule level 4 intr */

play:
	ldsb	[Devp+AUD_PLAY_ACTIVE],Temp1
	tst	Temp1				/* is play active? */
	be,a	checkactive
	andcc	Int_Active,Active,Zero		/* if not, any activity? */

	ld	[Devp+AUD_PLAY_CMDPTR],Cmdp
	tst	Cmdp				/* NULL command list? */
	be	playnull
	mov	1,Temp1

playskip:
	lduh	[Cmdp+AUD_CMD_SKIP],Temp2	/* check skip & done flags */
	tst	Temp2
	be,a	playactive
	or	Int_Active,Active,Int_Active	/* play is active */

	stb	Temp1,[Cmdp+AUD_CMD_DONE]	/* mark this command done */
	ld	[Cmdp+AUD_CMD_NEXT],Cmdp	/* update to next command */
	st	Cmdp,[Devp+AUD_PLAY_CMDPTR]
	tst	Cmdp				/* end of list? */
	bne	playskip			/* if not, check skip flag */
	or	Int_Active,Interrupt,Int_Active	/* if so, data underflow */

playnull:
	stb	Temp1,[Devp+AUD_PLAY_ERROR]	/* set error flag */
	b	playintr
	stb	Zero,[Devp+AUD_PLAY_ACTIVE]	/* disable play */

	or	Int_Active,Active,Int_Active	/* play is active */
playactive:
	ld	[Devp+AUD_PLAY_SAMPLES],Temp1
	inc	Temp1				/* increment sample count */
	st	Temp1,[Devp+AUD_PLAY_SAMPLES]
	ld	[Cmdp+AUD_CMD_DATA],Temp1
	ldsb	[Temp1],Temp2			/* get sample from buffer */
	stb	Temp2,[Chip+AUD_CHIP_BBRB]	/* write it to the device */
	inc	Temp1				/* increment buffer pointer */
	ld	[Cmdp+AUD_CMD_ENDDATA],Temp2
	cmp	Temp1,Temp2			/* buffer complete? */
	bcs	done				/* branch if not */
	st	Temp1,[Cmdp+AUD_CMD_DATA]	/* update buffer pointer */

	mov	1,Temp1
	stb	Temp1,[Cmdp+AUD_CMD_DONE]	/* mark command done */
	ld	[Cmdp+AUD_CMD_NEXT],Cmdp	/* update to next command */
	st	Cmdp,[Devp+AUD_PLAY_CMDPTR]
playintr:
	or	Int_Active,Interrupt,Int_Active	/* schedule level 4 intr */

done:
	andcc	Int_Active,Active,Zero		/* anything active? */
checkactive:
	bne	checkintr			/* branch if so */
	sethi	%hi(save_temp2),Temp2		/* prepare to restore reg */

	mov	AUD_CHIP_INIT_REG,Temp1
	stb	Temp1,[Chip+AUD_CHIP_CR]	/* set init register */
	mov	AUD_CHIP_DISABLE,Temp1
	stb	Temp1,[Chip+AUD_CHIP_DR]	/* disable device interrupts */

checkintr:
	andcc	Int_Active,Interrupt,Zero	/* schedule level 4 intr? */
	be	reti				/* branch if not */
	or	Temp2,%lo(save_temp2),Temp2	/* get saved reg address */

#ifdef	MULTIPROCESSOR
	GETCPU(Int_Active)
	and	Int_Active,0x3,Int_Active
#else
	or	%g0,%g0,Int_Active
#endif	MULTIPROCESSOR
#define	CPUINT_OFF(x)	sll	x,12,x

	CPUINT_OFF(Int_Active)
	sethi	%hi(V_INT_REGS),Temp1
	or	Temp1,%lo(V_INT_REGS),Temp1	/* schedule level 4 intr */
	or	Temp1,Int_Active,Temp1
	set	IR_SOFT_INT4,Int_Active
	st	Int_Active,[Temp1+8]

reti:
	ld	[Temp2],Temp2			/* restore saved register */

	sethi	%hi(_cnt+V_INTR),Temp1		/* increment interrupt count */
	ld	[Temp1+%lo(_cnt+V_INTR)],Cmdp
	inc	Cmdp
	st	Cmdp,[Temp1+%lo(_cnt+V_INTR)]

	RETI					/* return from interrupt */

#endif /*!AUDIO_C_TRAP*/
