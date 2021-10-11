/*
/*	@(#)fd_asm.s 1.1 92/07/30 SMI	*/
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */
/*
 *	Campus 1 low level floppy hardware interrupt handler.
 *
 *	This is hardwired onto level 11 all by itself.  It is needed to handle
 * the floppy data transfers without DMA.  The basic flow of operations is:
 *	device driver calculates parameters
 * 	device driver sets up fdintr_* values for this interrupt handler
 *	device driver starts operation
 *	device driver sleeps on someplace where its software interrupt handler
 *		will wake it.
 *	this interrupt handler does the data/result xfer for read/write/formats
 *		(or sense status for recalibrate/seek)
 *	then this interrupt handler sets a flag for the floppy soft interrupt
 *		handler and sets its bit in the interrupt control register
 *	the floppy software interrupt handler does whatever it needs to,
 *		(like retries, wake of upper levels, ... )
 *		and clears the opmode value to "reset" the hardware level.
 *
 *
 * NOTES ON TRAPS
 *	This code is called directly out of the trap table and
 * RUNS WITH TRAPS DISABLED!  Therefore it must be PERFECTLY ROBUST!
 * 
 *	Some things to remember:
 * when a trap is taken, traps are disabled: SO we must be *VERY CAREFUL*
 * when a trap is taken, PC and nPC go into r17/%l1 and r18/%l2, respectively
 * the TRAP() macro is: #define TRAP(H)  b (H); mov %psr,%l0; nop; nop;
 * I have only the registers %l0-%l7 to work with.
 *  - and %l0 - %l2 are already used.
 * the two instructions to return from trap are: jmpl ; rett
 *  - ie:  jmp     %l1	! return
 *         rett    %l2
 * 
 * TODO:
 * () decide on status returned, now it is just 0 = ok, !0 = !ok
 */

#define I82077

#include <machine/auxio.h>
#include <machine/intreg.h>
#include <machine/asm_linkage.h>

#include <machine/mmu.h>

/* these defines are always the same for 82072 controller chip */
#define RQM 0x80
#define DIO 0x40
#define NDM 0x20
#define CB  0x10
#define SENSE_INTERRUPT_STATUS 0x08


#ifdef COMMENT	/* just a (longish) comment of pseudocode */
fdc_hardintr()
{
	int	timeoutcntr;	/* timeout counter NYD XXX NOT USED? TBD */
	register u_char	*fdc; 	/* pointer to fdc registers */
	register int	len;
	register char	*addr;

	fdc = fdctlr_reg;
	
	switch( fdintr_opmode ) {
	case 1:
		/* read/write/format data xfer - during data xfer phase */
		/***timeoutcntr = fdintr_timeout; /* XXX NYD TBD NOT USED? */
		len = fdintr_len;
		addr = fdintr_addr;

		/* if the fifo ready bit set, then data available */
		while( *fdc & RQM ) {
			if( (*fdc & NDM) == 0 ) {
				/* whoops! things blew up, abandon it */
				/* DON'T send terminal count, it clears status*/
				fdintr_len = len;
				fdintr_addr = addr;
				fdintr_opmode = 3;
				goto results;
			}
			/* NDM is set, that means data */
			/* if input data, then input; else output */
			if( *fdc & DIO )
				*addr++ = *fifo;
			else
				*fifo = *addr++;
			len--;
			/* if length went to 0, then send the TC */
			if( len == 0 )
				goto end_xfr;
		}
		/* no more data, stash updated len, addr */
		fdintr_len = len;
		fdintr_addr = addr;
		break;
		/*****/
end_xfr:
		/* start send terminal count pulse */
		*(char *)AUXIO_REG |= (AUX_TC|AUX_MBO);
		/* save updated len, addr (is also a DELAY for TC) */
		fdintr_len = len;
		fdintr_addr = addr;
		/* clear the terminal count pulse */
		*(char *)AUXIO_REG =
		    AUX_MBO | ((*(char *)AUXIO_REG) & ~AUX_TC);
		/* set "this is status phase" */
		fdintr_opmode = 3;
		return;
		break;
	case 2:
		/* recal,seek - no result phase, must do sense interrupt status */
sense_int_stat:
		while( *fdc & CB )  ;
		while( ! (*fdc & RQM) )   ;
		*fifo = SENSE_INTERRUPT_STATUS;
		while( ! (*fdc & RQM) )   ;
		fdintr_statbuf[0] = *fifo;
		while( ! (*fdc & RQM) )   ;
		fdintr_statbuf[1] = *fifo;
		goto notify;	/* now tell upper layer that things are done */
		break;

	case 3:
results:
		/* result phase of regular ops, gather status */
		addr = fdintr_statp;	/* get ptr to status buf */
		while( *fdc & CB ) {	/* while chip is still "busy" */
			while( (*fdc & (RQM|DIO)) != (RQM|DIO) )
				;
			*addr++ = *fifo;
		}
		fdintr_statp = addr;	/* save updated ptr */
		goto notify;	/* now tell upper layer that things are done */
		break;

	default:
		/* no drive, no operation, ... */
		fdintr_spurious++;
		if( fdc != 0 )
			goto sense_int_stat;
		/* shall we do a reset to the FDC ? XXX NYD TBD */
	}
	goto out;
	/*******/

notify:
	/* set bit in soft interrupt register */
#ifndef sun4m
	*(u_char *)INTREG_ADDR |= IR_SOFT_INT4; 
#else sun4m
	send_dirint(get_processorid(), 4);
#endif sun4m
	/* set software interrupt flag */
	fdintr_opmode = 4;
	fdintr_status = 0;

out:
	restore_psr_with_delay;
	return_from_trap;
}
#endif COMMENT


/* start of assembler source */
/*
 * private data for this interrupt handler
 *	I = input, set before starting operation
 *	O = output, available *after* this invokes the software level
 */
	.seg	"data"
	.align	4

	/* I: 1 = read/write/format,  2 = seek/recal; O: 4 = done */
	/* 3 = internal state -waiting for results */
	/* only set if operation is actually pending - KEEP CLEAR OTHERWISE! */
	.global	_fdintr_opmode
_fdintr_opmode:
	.word	0x0

	/* I/O: (current) data xfer address */
	.global	_fdintr_addr
_fdintr_addr:
	.word	0x0

	/* I: data xfer length; O: remainder */
	.global	_fdintr_len
_fdintr_len:
	.word	0x0

	/* I: n/a; O: 0 = ok, TBD XXX NYD errors */
	.global	_fdintr_status
_fdintr_status:
	.word	0x0

	/* timeout value for  ??? */
/* NOT USED YET/EVER??? XXX TBD NYD NEVER MAYBE */
	.global	_fdintr_timeout
_fdintr_timeout:
	.word	0x0

	/* count of spurious interrupts */
	.global	_fdintr_spurious
_fdintr_spurious:
	.word	0x0

	/* I: pointer to fdintr_statbuf; O: n/a */
	.global	_fdintr_statp
_fdintr_statp:
	.word	_fdintr_statbuf + 0x0

/*XXX status of number of interrupts! */
	.global	_fdintr_nints
_fdintr_nints:
	.word	0


	/*
	 * WARNING, DANGER - if there is ever an interrupt and this address is
	 * not * setup (by fdattach) then things may/will hang! (depends on
	 * what's at 0)
	 */
	/* pointer to fdc registers, set by fdattach() */
	.global	_fdctlr_reg
_fdctlr_reg:
	.word	0x0
#ifdef I82077
	.global	_fd_msr
_fd_msr:
	.word	0x0
	.global	_fd_dsr
_fd_dsr:
	.word	0x0
	.global	_fd_dor
_fd_dor:
	.word	0x0
	.global	_fd_dir
_fd_dir:
	.word	0x0
	.global	_fd_fifo
_fd_fifo:
	.word	0x0
#endif I82077

	/* I: n/a; O: 10 bytes of status buffer */
	.global	_fdintr_statbuf
_fdintr_statbuf:
	.word	0xAAAAAAAA
	.word	0xBBBBBBBB
	.word	0xCCCCCCCC
	.word	0xDDDDDDDD

/*
 * fdc_hardintr - interrupt function to do DMA-like operation for floppy
 *	Since it is the only thing on level 11 on a Campus, we can just
 * call this thing directly from trap table.
 * WE RUN WITH TRAPS DISABLED! - BE CAREFUL!
 * I have only the registers %l0-%l7 to work with! (and %l{0-2} are already used)
 */	
/*
 * CODING CONVENTIONS
 *	REGISTER USAGE - is as shown by the following defines
 *	INSTRUCTIONS following BRANCH - have a "!!" comment delimiter
 */
#define Fdc %l3 /* csr - the pointer to the chip's registers */
#define Adr %l4 /* addr - the data address pointer */
#define Len %l5 /* len - length of data */
#define Ptr %l6 /* misc ptr */
#define Tmp %l7 /* scratch */

	.seg	"text"
	.global	_fdc_hardintr
_fdc_hardintr:
#ifdef MEASURE_TIME
	/* START: set the LED */
	sethi	%hi(AUXIO_REG), Ptr
	ldub	[Ptr + %lo(AUXIO_REG)], Tmp
	or	Tmp, AUX_LED|AUX_MBO, Tmp
	stb	Tmp, [Ptr + %lo(AUXIO_REG)]
#endif MEASURE_TIME

/*XXX DEBUG ??? */
	sethi	%hi(_fdintr_nints), Ptr		! load ptr to count of ints
	ld	[Ptr + %lo(_fdintr_nints)], Tmp
	add	Tmp, 0x1, Tmp
	st	Tmp, [Ptr + %lo(_fdintr_nints)]
#ifdef I82077
	/*
	 * This routine only uses 2 8207[27] registers, the msr and fifo
	 * registers.  In the two configurations, the addresses of the msr
	 * are different, but the fifo always directly follows the msr.
	 * For this reason, we stuff the address of msr into Fdc instead
	 * of the base address of the chip's registers.
	 */
	sethi	%hi(_fd_msr), Ptr		! load ptr to msr into Fdc
	ld	[Ptr + %lo(_fd_msr)], Fdc
#else
	sethi	%hi(_fdctlr_reg), Ptr		! load ptr to csr into Fdc
	ld	[Ptr + %lo(_fdctlr_reg)], Fdc
#endif I82077

	/*
	 * switch( opmode ) {
	 */
	sethi	%hi(_fdintr_opmode), Ptr
	ld	[Ptr + %lo(_fdintr_opmode)], Tmp
	cmp	Tmp, 1			!case 1: read/write data-xfr
	be	L10
	cmp	Tmp, 2			!case 2: seek/recalibrate ?
	be	L21
	cmp	Tmp, 3			!case 3: results ?
	be	L30

! default: SPURIOUS INTERRUPT! XXX NYD hope we NEVER GET HERE!
	sethi	%hi(_fdintr_spurious), Ptr		!! spurious++
	ld	[Ptr + %lo(_fdintr_spurious)], Tmp
	add	Tmp, 0x1, Tmp
	st	Tmp, [Ptr + %lo(_fdintr_spurious)]	!!
	! pray and test that the controller is mapped in!
	tst	Fdc
	bne	L21			! yes, do a sense interrupt status
	nop
	b	L900			! no, just exit
	nop
	/**********/

	/*
	 * case 1:
	 * read/write/format data-xfer case - they have a result phase
	 */
L10:
#ifdef NEVER	/* no timeout stuff just yet */
	sethi	%hi(_fdintr_timeout), %__
	ld	[%__ + %lo(_fdintr_timeout)], %__
	st	%__, [%fp + -0x4]
#endif NEVER
	sethi	%hi(_fdintr_len), Ptr		! len into Len
	ld	[Ptr + %lo(_fdintr_len)], Len
	sethi	%hi(_fdintr_addr), Ptr		! addr into Adr
	ld	[Ptr + %lo(_fdintr_addr)], Adr

	/* while the fifo ready bit set, then data/status available */
	/* while( *csr & RQM ) { */
L11:	ldub	[Fdc], Tmp			! get msr
	andcc	Tmp, RQM, %g0
	be	L19				! branch if bit clear

	/* if NDM is set, that means data */
	andcc	Tmp, NDM, %g0			!!  if( *csr & NDM ) {
	be	L803		! if ! set, is status!

	/* check for input vs. output data */
	andcc	Tmp, DIO, %g0			!! _if( *csr & DIO ) {
	be	L13
	sub	Len, 0x1, Len			!! len--

	ldub	[Fdc + 0x1], Tmp		! DIO set, *addr = *fifo
	b	L12
	stb	Tmp, [Adr]			!!

L13:	ldsb	[Adr], Tmp		! _}else{ DIO clear, *fifo = *addr
	stb	Tmp, [Fdc + 0x1]
						! _} end if *csr & DIO
	/* addr++; len--; if(len==0) send TC */
L12:	! moved len-- up beneath test DIO branch
	tst	Len			! if( len == 0 ) send TC
	bne	L11			! branch if not, back to while( ..RQM )
	add	Adr, 0x1, Adr		!! addr++  } /* end of *csr & NDM */
	/* if ( Len == 0 ) goto end_xfr */
	b	L801
	nop

	/* } end while( *csr & RQM ) */
L19:	/* save updated len, addr */
	sethi	%hi(_fdintr_len), Ptr
	st	Len, [Ptr + %lo(_fdintr_len)]
	sethi	%hi(_fdintr_addr), Ptr
	b	L900				! not done yet, return
	st	Adr, [Ptr + %lo(_fdintr_addr)]	!!
	/**********/

	/*
	 * END OF TRANSFER - if read/write, toggle the TC bit in AUXIO_REG
	 * then save status and set state = 3.
	 */
/* end_xfr: */
L801:	sethi	%hi(AUXIO_REG), Ptr
	ldub	[Ptr + %lo(AUXIO_REG)], Tmp
	or	Tmp, AUX_MBO|AUX_TC, Tmp
	stb	Tmp, [Ptr + %lo(AUXIO_REG)]
/* begin TC delay */
	! five nops provide 100ns of delay at 10MIPS to ensure
	! TC is wide enough at slowest possible floppy clock (500ns @ 250Kbps)
	/*
	 * (do something usefull in delay)
	 * stash len and addr before they get lost
	 */
L802:	sethi	%hi(_fdintr_len), Ptr
	st	Len, [Ptr + %lo(_fdintr_len)]
	sethi	%hi(_fdintr_addr), Ptr
	st	Adr, [Ptr + %lo(_fdintr_addr)]
	nop
/* end TC delay */
	! now clear the TC bit
	sethi	%hi(AUXIO_REG), Ptr
	ldub	[Ptr + %lo(AUXIO_REG)], Tmp
	and	Tmp, ~AUX_TC, Tmp
	or	Tmp, AUX_MBO, Tmp
	stb	Tmp, [Ptr + %lo(AUXIO_REG)]

	/* opmode = 3 to indicate going into status mode */
	mov	0x3, Tmp
	sethi	%hi(_fdintr_opmode), Ptr
	b	L900
	st	Tmp, [Ptr + %lo(_fdintr_opmode)]	!!
	/***********/

	/*
	 * error status state: save old pointers, go direct to result snarfing
	 */
L803:	sethi	%hi(_fdintr_len), Ptr
	st	Len, [Ptr + %lo(_fdintr_len)]
	sethi	%hi(_fdintr_addr), Ptr
	st	Adr, [Ptr + %lo(_fdintr_addr)]
	mov	0x3, Tmp
	sethi	%hi(_fdintr_opmode), Ptr
	b	L30
	st	Tmp, [Ptr + %lo(_fdintr_opmode)]	!!
	/*******************************/

	/*
	 * case 2:
	 * recalibrate/seek - no result phase, must do sense interrupt status
	 */
/* sense_int_stat: */
L21:	ldub	[Fdc], Tmp			! Tmp = *csr
L22:	andcc	Tmp, CB, Tmp			! is CB set?
	bne	L22				! yes, keep waiting
	ldub	[Fdc], Tmp			!! Tmp = *csr

	/* wait!!! should we check rqm first???  ABSOLUTELY YES!!!! */
L23:	ldub	[Fdc], Tmp			! busy wait until RQM set
	andcc	Tmp, RQM, Tmp
	be	L23				! branch if bit clear
	nop
	mov	0x8, Tmp			! cmd for SENSE_INTERRUPT_STATUS
	stb	Tmp, [Fdc + 0x1]

	/* NOTE: we ignore DIO here, assume it is set before RQM! */
L24:	ldub	[Fdc], Tmp			! busy wait until RQM set
	andcc	Tmp, RQM, Tmp
	be	L24				! branch if bit clear
	nop

	! fdintr_statbuf[0] = *fifo;
	ldub	[Fdc + 0x1], Tmp
	sethi	%hi(_fdintr_statbuf), Ptr
	stb	Tmp, [Ptr + %lo(_fdintr_statbuf)]

L26:	ldub	[Fdc], Tmp			! busy wait until RQM set
	andcc	Tmp, RQM, Tmp
	be	L26				! branch if bit clear
	nop

	/* fdintr_statbuf[1] = *fifo; */
	ldub	[Fdc + 0x1], Tmp
	sethi	%hi(_fdintr_statbuf + 0x1), Ptr
	b	L40
	stb	Tmp, [Ptr + %lo(_fdintr_statbuf + 0x1)]	!!


	/*
	 * case 3: result mode
	 * are in result mode make sure all status bytes are read out
	 */
L30:	sethi	%hi(_fdintr_statp), Adr		! &statp in Adr		
	ld	[Adr + %lo(_fdintr_statp)], Ptr	! statp is Ptr
	sethi	%hi(_fdintr_statbuf+10), Len	! Len is limit for status bytes
	or	Len, %lo(_fdintr_statbuf+10), Len
	ldub	[Fdc], Tmp			!
L34:	andcc	Tmp, CB, %g0			! is CB set?
	be	L32				! no, jump around, must be fini
	nop
	! are *both* RQM and DIO set?
	andcc	Tmp, RQM, %g0
	be	L33
	nop
	andcc	Tmp, DIO, %g0
	be	L33
	nop
	ldub	[Fdc + 0x1], Tmp		! *fifo into Tmp
	stb	Tmp, [Ptr]
	! check to see if status buffer ptr is at/past limit
/*XXX limits? */
	cmp	Ptr, Len
	bge	L33				! if past, don't increment!
	nop
	add	Ptr, 0x1, Ptr			! statp++ in Ptr
L33:	b	L34				! back to while( *Fdc & CB )
	ldub	[Fdc], Tmp			!
	
L32:	! stash updated statp and go to notify
	b	L40
	st	Ptr, [Adr + %lo(_fdintr_statp)]	!!


/* notify: */	/* set bit in software interrupt register */
L40:	/* *(uchar *)INTREG_ADDR |= IR_SOFT_INT4 */
#ifndef sun4m
	sethi	%hi(INTREG_ADDR), Ptr
	ldub	[Ptr + %lo(INTREG_ADDR)], Tmp
	or	Tmp, IR_SOFT_INT4, Tmp
	stb	Tmp, [Ptr + %lo(INTREG_ADDR)]
#else sun4m
/*
 * set a pending level 4 software interrupt
 * we are probably going to be the master,
 * and if we are not then it will be vectored
 * to the proper master.
 */
	GETCPU(Tmp)			! get module id, 8..11
	and	Tmp, 3, Tmp		! mask to 0..3
	sll	Tmp, 12, Tmp		! shift for +0x1000 per mid
	set	V_INT_REGS, Ptr
	add	Ptr, Tmp, Ptr		! calculate location of OUR int reg
	set	IR_SOFT_INT4, Tmp
	st	Tmp, [Ptr+8]		! send the softint
#endif sun4m

	/* opmode = 4; status = 0; */
	mov	0x4, Tmp
	sethi	%hi(_fdintr_opmode), Ptr
	st	Tmp, [Ptr + %lo(_fdintr_opmode)]
	mov	0x0, Tmp
	sethi	%hi(_fdintr_status), Ptr
	st	Tmp, [Ptr + %lo(_fdintr_status)]

/* out: */
L900:
#ifdef MEASURE_TIME
	/* END: clear the LED */
	sethi	%hi(AUXIO_REG), Ptr
	ldub	[Ptr + %lo(AUXIO_REG)], Tmp
	and	Tmp, ~AUX_LED, Tmp
	or	Tmp, AUX_MBO, Tmp
	stb	Tmp, [Ptr + %lo(AUXIO_REG)]
#endif MEASURE_TIME
	mov	%l0, %psr	! restore psr - and user's ccodes
	nop
	nop
	jmp	%l1	! return from interrupt
	rett	%l2
	.empty

	.global	_fdc_intr
_fdc_intr:
	save	%sp, -96, %sp
#ifdef MEASURE_TIME
	/* START: set the LED */
	sethi	%hi(AUXIO_REG), Ptr
	ldub	[Ptr + %lo(AUXIO_REG)], Tmp
	or	Tmp, AUX_LED|AUX_MBO, Tmp
	stb	Tmp, [Ptr + %lo(AUXIO_REG)]
#endif MEASURE_TIME

! Assume interrupt is from Floppy, unless otherwise overwritten (%i0)
	mov	0x1, %i0

/*XXX DEBUG ??? */
	sethi	%hi(_fdintr_nints), Ptr		! load ptr to count of ints
	ld	[Ptr + %lo(_fdintr_nints)], Tmp
	add	Tmp, 0x1, Tmp
	st	Tmp, [Ptr + %lo(_fdintr_nints)]
#ifdef I82077
	/*
	 * This routine only uses 2 8207[27] registers, the msr and fifo
	 * registers.  In the two configurations, the addresses of the msr
	 * are different, but the fifo always directly follows the msr.
	 * For this reason, we stuff the address of msr into Fdc instead
	 * of the base address of the chip's registers.
	 */
	sethi	%hi(_fd_msr), Ptr		! load ptr to msr into Fdc
	ld	[Ptr + %lo(_fd_msr)], Fdc
#else
	sethi	%hi(_fdctlr_reg), Ptr		! load ptr to csr into Fdc
	ld	[Ptr + %lo(_fdctlr_reg)], Fdc
#endif I82077

	/*
	 * switch( opmode ) {
	 */
	sethi	%hi(_fdintr_opmode), Ptr
	ld	[Ptr + %lo(_fdintr_opmode)], Tmp
	cmp	Tmp, 1			!case 1: read/write data-xfr
	be	L100
	cmp	Tmp, 2			!case 2: seek/recalibrate ?
	be	L210
	cmp	Tmp, 3			!case 3: results ?
	be	L300
	nop


! do a sense interrupt status

	tst	Fdc
	bne	L210
	nop

! Return 0, interrupt is not from Floppy.
	b	L9000			! no, just exit with Zero return
	mov	0x0, %i0
	/**********/

	/*
	 * case 1:
	 * read/write/format data-xfer case - they have a result phase
	 */
L100:
#ifdef NEVER	/* no timeout stuff just yet */
	sethi	%hi(_fdintr_timeout), %__
	ld	[%__ + %lo(_fdintr_timeout)], %__
	st	%__, [%fp + -0x4]
#endif NEVER
	sethi	%hi(_fdintr_len), Ptr		! len into Len
	ld	[Ptr + %lo(_fdintr_len)], Len
	sethi	%hi(_fdintr_addr), Ptr		! addr into Adr
	ld	[Ptr + %lo(_fdintr_addr)], Adr

	/* while the fifo ready bit set, then data/status available */
	/* while( *csr & RQM ) { */
L110:	ldub	[Fdc], Tmp			! get msr
	andcc	Tmp, RQM, %g0
	be	L190				! branch if bit clear

	/* if NDM is set, that means data */
	andcc	Tmp, NDM, %g0			!!  if( *csr & NDM ) {
	be	L8030			! if ! set, is status!

	/* check for input vs. output data */
	andcc	Tmp, DIO, %g0			!! _if( *csr & DIO ) {
	be	L130
	sub	Len, 0x1, Len			!! len--

	ldub	[Fdc + 0x1], Tmp		! DIO set, *addr = *fifo
	b	L120
	stb	Tmp, [Adr]			!!

L130:	ldsb	[Adr], Tmp		! _}else{ DIO clear, *fifo = *addr
	stb	Tmp, [Fdc + 0x1]
						! _} end if *csr & DIO
	/* addr++; len--; if(len==0) send TC */
L120:	! moved len-- up beneath test DIO branch
	tst	Len			! if( len == 0 ) send TC
	bne	L110			! branch if not, back to while( ..RQM )
	add	Adr, 0x1, Adr		!! addr++  } /* end of *csr & NDM */
	/* if ( Len == 0 ) goto end_xfr */
	b	L8010
	nop

	/* } end while( *csr & RQM ) */
L190:	/* save updated len, addr */
	sethi	%hi(_fdintr_len), Ptr
	st	Len, [Ptr + %lo(_fdintr_len)]
	sethi	%hi(_fdintr_addr), Ptr
	b	L9000			! not done yet, return
	st	Adr, [Ptr + %lo(_fdintr_addr)]	!!
	/**********/

	/*
	 * END OF TRANSFER - if read/write, toggle the TC bit in AUXIO_REG
	 * then save status and set state = 3.
	 */
/* end_xfr: */
L8010:	sethi	%hi(AUXIO_REG), Ptr
	ldub	[Ptr + %lo(AUXIO_REG)], Tmp
	or	Tmp, AUX_MBO|AUX_TC, Tmp
	stb	Tmp, [Ptr + %lo(AUXIO_REG)]
/* begin TC delay */
	! five nops provide 100ns of delay at 10MIPS to ensure
	! TC is wide enough at slowest possible floppy clock (500ns @ 250Kbps)
	/*
	 * (do something usefull in delay)
	 * stash len and addr before they get lost
	 */
L8020:	sethi	%hi(_fdintr_len), Ptr
	st	Len, [Ptr + %lo(_fdintr_len)]
	sethi	%hi(_fdintr_addr), Ptr
	st	Adr, [Ptr + %lo(_fdintr_addr)]
	nop
/* end TC delay */
	! now clear the TC bit
	sethi	%hi(AUXIO_REG), Ptr
	ldub	[Ptr + %lo(AUXIO_REG)], Tmp
	and	Tmp, ~AUX_TC, Tmp
	or	Tmp, AUX_MBO, Tmp
	stb	Tmp, [Ptr + %lo(AUXIO_REG)]

	/* opmode = 3 to indicate going into status mode */
	mov	0x3, Tmp
	sethi	%hi(_fdintr_opmode), Ptr
	b	L9000
	st	Tmp, [Ptr + %lo(_fdintr_opmode)]	!!
	/***********/

	/*
	 * error status state: save old pointers, go direct to result snarfing
	 */
L8030:	sethi	%hi(_fdintr_len), Ptr
	st	Len, [Ptr + %lo(_fdintr_len)]
	sethi	%hi(_fdintr_addr), Ptr
	st	Adr, [Ptr + %lo(_fdintr_addr)]
	mov	0x3, Tmp
	sethi	%hi(_fdintr_opmode), Ptr
	b	L300
	st	Tmp, [Ptr + %lo(_fdintr_opmode)]	!!
	/*******************************/

	/*
	 * case 2:
	 * recalibrate/seek - no result phase, must do sense interrupt status
	 */
/* sense_int_stat: */
L210:	ldub	[Fdc], Tmp			! Tmp = *csr
L220:	andcc	Tmp, CB, Tmp			! is CB set?
	bne	L220				! yes, keep waiting
	ldub	[Fdc], Tmp			!! Tmp = *csr

	/* wait!!! should we check rqm first???  ABSOLUTELY YES!!!! */
L230:	ldub	[Fdc], Tmp			! busy wait until RQM set
	andcc	Tmp, RQM, Tmp
	be	L230				! branch if bit clear
	nop
	mov	0x8, Tmp			! cmd for SENSE_INTERRUPT_STATUS
	stb	Tmp, [Fdc + 0x1]

	/* NOTE: we ignore DIO here, assume it is set before RQM! */
L240:	ldub	[Fdc], Tmp			! busy wait until RQM set
	andcc	Tmp, RQM, Tmp
	be	L240				! branch if bit clear
	nop

	! fdintr_statbuf[0] = *fifo;
	ldub	[Fdc + 0x1], Tmp
	sethi	%hi(_fdintr_statbuf), Ptr
	stb	Tmp, [Ptr + %lo(_fdintr_statbuf)]

L260:	ldub	[Fdc], Tmp			! busy wait until RQM set
	andcc	Tmp, RQM, Tmp
	be	L260			! branch if bit clear
	nop

	/* fdintr_statbuf[1] = *fifo; */
	ldub	[Fdc + 0x1], Tmp
	sethi	%hi(_fdintr_statbuf + 0x1), Ptr
	b	L400
	stb	Tmp, [Ptr + %lo(_fdintr_statbuf + 0x1)]	!!


	/*
	 * case 3: result mode
	 * are in result mode make sure all status bytes are read out
	 */
L300:	sethi	%hi(_fdintr_statp), Adr		! &statp in Adr		
	ld	[Adr + %lo(_fdintr_statp)], Ptr	! statp is Ptr
	sethi	%hi(_fdintr_statbuf+10), Len	! Len is limit for status bytes
	or	Len, %lo(_fdintr_statbuf+10), Len
	ldub	[Fdc], Tmp			!
L340:	andcc	Tmp, CB, %g0			! is CB set?
	be	L320				! no, jump around, must be fini
	nop
	! are *both* RQM and DIO set?
	andcc	Tmp, RQM, %g0
	be	L330
	nop
	andcc	Tmp, DIO, %g0
	be	L330
	nop
	ldub	[Fdc + 0x1], Tmp		! *fifo into Tmp
	stb	Tmp, [Ptr]
	! check to see if status buffer ptr is at/past limit
/*XXX limits? */
	cmp	Ptr, Len
	bge	L330			! if past, don't increment!
	nop
	add	Ptr, 0x1, Ptr			! statp++ in Ptr
L330:	b	L340				! back to while( *Fdc & CB )
	ldub	[Fdc], Tmp			!
	
L320:	! stash updated statp and go to notify
	b	L400
	st	Ptr, [Adr + %lo(_fdintr_statp)]	!!


/* notify: */	/* set bit in software interrupt register */
L400:	/* *(uchar *)INTREG_ADDR |= IR_SOFT_INT4 */
#ifndef sun4m
	sethi	%hi(INTREG_ADDR), Ptr
	ldub	[Ptr + %lo(INTREG_ADDR)], Tmp
	or	Tmp, IR_SOFT_INT4, Tmp
	stb	Tmp, [Ptr + %lo(INTREG_ADDR)]
#else sun4m
/*
 * set a pending level 4 software interrupt
 * we are probably going to be the master,
 * and if we are not then it will be vectored
 * to the proper master.
 */
	GETCPU(Tmp)			! get module id, 8..11
	and	Tmp, 3, Tmp		! mask to 0..3
	sll	Tmp, 12, Tmp		! shift for +0x1000 per mid
	set	V_INT_REGS, Ptr
	add	Ptr, Tmp, Ptr		! calculate location of OUR int reg
	set	IR_SOFT_INT4, Tmp
	st	Tmp, [Ptr+8]		! send the softint
#endif sun4m

	/* opmode = 4; status = 0; */
	mov	0x4, Tmp
	sethi	%hi(_fdintr_opmode), Ptr
	st	Tmp, [Ptr + %lo(_fdintr_opmode)]
	mov	0x0, Tmp
	sethi	%hi(_fdintr_status), Ptr
	st	Tmp, [Ptr + %lo(_fdintr_status)]

/* out: */
L9000:
#ifdef MEASURE_TIME
	/* END: clear the LED */
	sethi	%hi(AUXIO_REG), Ptr
	ldub	[Ptr + %lo(AUXIO_REG)], Tmp
	and	Tmp, ~AUX_LED, Tmp
	or	Tmp, AUX_MBO, Tmp
	stb	Tmp, [Ptr + %lo(AUXIO_REG)]
#endif MEASURE_TIME
	ret
	restore
	.empty






#ifdef I82077
/*
 * Turn on or off bits in the digital output register.
 *
 * fd_set_dor(bit, flag)
 *	int bit;		bit mask in the dor
 *	int flag;		0 = off, otherwise on
 */
	ENTRY(fd_set_dor)

	tst	%o1
	set	_fd_dor, %o2		! get address of address of dor
	ld	[%o2], %o2		! get address of dor
	ldub	[%o2], %g1		! read dor
	bnz,a	1f
	bset	%o0, %g1		! on
	bclr	%o0, %g1		! off
1:
	stb	%g1, [%o2]		! write dor

	retl
	nop
#endif I82077
