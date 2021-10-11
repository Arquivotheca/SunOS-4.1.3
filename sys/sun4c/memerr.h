/*	@(#)memerr.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4c_memerr_h
#define	_sun4c_memerr_h

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * All Sun-4c implementations have memory parity error detection.
 * The memory error register consists of a control register.  If an error
 * occurs, the control register stores information relevant to the error.
 * The address of the word in error is stored in one of the error
 * address registers; the SEVAR for synchronous errors and the ASEVAR for
 * asynchronous errors.  The byte(s) in error are identified by the
 * memory error register.
 * Errors are reported either by a trap (for synchronous errors) or a
 * non-maskable level 15 interrupt (for asynchronous errors).
 * If a second error occurs before the first one has been processed,
 * the MULTI bit will be on in the memory error register, and the
 * address of the first (for asynchronous errors) or last (for
 * synchronous errors) error will be stored in the appropriate error
 * address register.
 * The interrupt is cleared by toggling the "enable all interrupts" bit
 * in the interrupt control register.
 * The information bits in the memory error register are cleared by
 * reading it.
 */

#define	OBIO_MEMERR_ADDR 0xF4000000	/* address of memerr in obio space */

#define	MEMERR_ADDR 0xFFFF9000		/* virtual address we map memerr to */
#define MEMERR_ADDR2 0xFFFF9008		/* v.a. of expansion card parity reg */

#define MEMEXP_START 0x8000000	/* start of expansion card memory */

#ifdef LOCORE
#define	MEMERR MEMERR_ADDR	/* virtual address we map memerr to be at */
#define MEMERR2 MEMERR_ADDR2
#else
struct memerr {
	u_int	me_err;		/* memory error register */
#define	me_per	me_err		/* parity error register */
};
#define	MEMERR ((struct memerr *)(MEMERR_ADDR))
#define MEMERR2 ((struct memerr *) (MEMERR_ADDR2))
#endif LOCORE

/*
 *  Bits for the memory error register when used as parity error register
 */
#define	PER_ERROR	0x80	/* r/o - 1 = parity error detected */
#define	PER_MULTI	0x40	/* r/o - 1 = second error detected */
#define	PER_TEST	0x20	/* r/w - 1 = write inverse parity */
#define	PER_CHECK	0x10	/* r/w - 1 = enable parity checking */
#define	PER_ERR00	0x08	/* r/o - 1 = parity error <0..7> */
#define	PER_ERR08	0x04	/* r/o - 1 = parity error <8..15> */
#define	PER_ERR16	0x02	/* r/o - 1 = parity error <16..23> */
#define	PER_ERR24	0x01	/* r/o - 1 = parity error <24..31> */
#define	PER_ERRS	0x0F	/* r/o - mask for specific error bits */
#define	PARERR_BITS	\
"\20\10ERROR\7MULTI\6TEST\5CHECK\4ERR00\3ERR08\2ERR16\1ERR24"

#endif	/* !_sun4c_memerr_h */
