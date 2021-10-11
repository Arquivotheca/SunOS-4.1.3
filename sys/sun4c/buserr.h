/*	@(#)buserr.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4c_buserr_h
#define	_sun4c_buserr_h

/*
 * Copyright 1987-1989 Sun Microsystems, Inc.
 */

/*
 * Definitions related to exception handling on the Sun4c.
 */

/*
 * The Synchronous Error register captures the cause(s) of synchronous
 * exceptions given to the CPU. The CPU is notified by assertion
 * of the MEX signal, causing a memory exception trap. Because of
 * prefetch the SE register accumulates errors. The software must verify
 * the type of error manually if multiple bits are set in this register.
 * The SE register is technically read/write for diagnostic purposes, but
 * is treated as read-only by the kernel. It is cleared automatically
 * each time it is read. The SE_RW bit is synchronized with the SVA reg,
 * so it always reflects data pertaining to the cycle latched in the SVA reg.
 * NOTE: for early hardware, certain errors reported in the SE reg will
 * also cause a level 15 interrupt to be generated. The kernel must
 * know to ignore these interrupts.
 *
 * The Synchronous Virtual Address register captures the address of
 * synchronous data faults. It is used in conjunction with the SE reg.
 * It is guaranteed to hold the address of the last data fault to have
 * occurred since the SE reg was read.  This is necessary because an
 * instruction pre-fetch to an invalid address can occur before the
 * memory cycle of a load/store instruction to an invalid address, and we
 * need to capture the data associated with the load/store instruction
 * (which is faulting), and not the pre-fetched instruction (whose
 * execution hasn't been attempted yet).
 *
 * The Asynchronous Error register caputures the cause(s) of asynchronous
 * exceptions given to the CPU. The CPU is notified with a level 15 interrupt.
 * Because of the possibility of multiple errors, the ASE register
 * accumulates errors until it is read. It is cleared automatically when
 * it is read. The ASE register is technically read/write for diagnostic
 * purposes, but is treated as read-only by the kernel.
 *
 * The Asynchronous Virtual Address register captures the address of
 * asynchronous data faults. It is used in conjunction with the ASE reg.
 * It is guaranteed to latch the address of the first data fault since
 * it was last cleared.  It is cleared automatically each time it is
 * read.
 *
 * [ the following registers are not implemented on all Sun-4Cs ]
 *
 * The Asynchronous Data #1 Register captures the first (or only) data
 * word associated with the first asynchronous write buffer error to
 * have occurred since it was last cleared.  It is cleared
 * automatically each time it is read.
 *
 * The Asynchronous Data #2 Register captures the second data word
 * associated with the first asynchronous write buffer error to have
 * occurred since it was last cleared.  It is cleared automatically
 * each time it is read.  It is only valid when ASE_DBLE is set in the
 * ASE register.
 */
#ifndef LOCORE
struct error_regs {
	u_int	sync_error_reg;		/* synchronous error reg */
	u_int	sync_va_reg;		/* synchronous address reg */
	u_int	async_error_reg;	/* asynchronous error reg */
	u_int	async_va_reg;		/* asynchronous address reg */
	u_int	async_data1_reg;	/* asynchronous data 1 reg */
	u_int	async_data2_reg;	/* asynchronous data 2 reg */
};
#endif !LOCORE

/*
 * Synchronous Error bits.
 */
#define	SE_RW		0x8000		/* direction of bad cycle (0=read) */
#define	SE_INVALID	0x0080		/* page map was invalid */
#define	SE_PROTERR	0x0040		/* protection error */
#define	SE_TIMEOUT	0x0020		/* bus access timed out */
#define	SE_SBBERR	0x0010		/* Sbus bus error */
#define	SE_MEMERR	0x0008		/* Synchronous memory error */
#define	SE_SIZERR	0x0002		/* size error */
#define	SE_WATCHDOG	0x0001		/* Watchdog reset */

#define	SYNCERR_BITS	\
"\20\20WRITE\10INVALID\7PROTERR\6TIMEOUT\5SBBERR\4MEMERR\2SIZERR\1WATCHDOG"

/*
 * Asynchronous Error bits.
 */
#define ASE_SIZ		0x0300		/* IU transaction size */
#define ASE_SIZ_SHIFT	8		/* amount to shift by */
#define	ASE_WBACKERR	0x0080		/* buffered write invalid error */
#define	ASE_INVALID	0x0080		/* page map was invalid */
#define	ASE_PROTERR	0x0040		/* protection error */
#define	ASE_TIMEOUT	0x0020		/* buffered write timeout */
#define	ASE_DVMAERR	0x0010		/* dvma cycle error */
#define	ASE_MEMERR	0x0008		/* Asynchronous memory error */
#define	ASE_SBERR	0x0002		/* Sbus bus error */
#define	ASE_MULTIPLE	0x0001		/* Multiple SBus errors detected */


#define	ASE_ERRMASK	0x00b0		/* valid error mask */

#define	ASYNCERR_BITS	"\20\10WBACKERR\6TIMEOUT\5DVMAERR"

/*
 * XXX Due to a bug in the 4/60 cache chip, the ASEVAR isn't properly
 * XXX sign-extended on DVMA asynchronous errors.  It is properly
 * XXX extended on asynchronous IU errors.
 */
#define	ASEVAR_SIGNBIT	0x20000000
#define	ASEVAR_SIGNEXTEND(x)	(((x) ^ ASEVAR_SIGNBIT) - ASEVAR_SIGNBIT)

#define	ASE_ERRMASK_70	0x00fb		/* valid error mask */

#define	ASYNCERR_BITS_70 \
"\20\10INVALID\7PROTERR\6TIMEOUT\5DVMAERR\4MEMERR\2SBERR\1MULTIPLE"

/*
 * Flags to define type of memory error.
 */
#define	MERR_SYNC	0x0
#define	MERR_ASYNC	0x1

#endif	_sun4c_buserr_h
