/*      @(#)memerr.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * All Sun-3X implementations have daughterboard parity memory, parity
 * memory boards, or memory equipped with error correction (ECC). The
 * memory error register consists of a control and an address register.
 * If an error occurs, the control register stores information relevant to
 * the error. The memory address error register stores the physical address
 * and the CPU/DVMA bit of the memory cycle at which
 * the error was detected.  Errors are reported via a non-maskable
 * level 7 interrupt.  In case of multiple (stacked) memory errors,
 * the information relating to the first error is latched in the
 * memory error register.  The interrupt is held pending and the error
 * information in the memory error register is latched (frozen) until
 * it is cleared (unfrozen) by a write to bits <31..24> of the memory
 * error address register.
 */

#ifndef _sun3x_memerr_h
#define _sun3x_memerr_h

#include <machine/devaddr.h>

#ifdef LOCORE
#define	MEMREG		(V_MEMREG)
#else
struct memreg {
	u_char	mr_er;		/* memory error control register */
#define	mr_eer	mr_er		/* ecc & mixed error register */
#define	mr_per	mr_er		/* parity only error register */
	u_char	mr_undef[3];
	u_int	mr_dvma	: 1;	/* DVMA vs CPU access */
	u_int		: 1;
	u_int	mr_paddr:30;	/* physical address of error */
};
#define	MEMREG ((struct memreg *)(V_MEMREG))
#endif LOCORE

/*
 * Bits for the memory error register when used as PARITY ONLY error register.
 */
#define	PER_INTR	0x80	/* r/o - parity interrupt pending */
#define	PER_INTENA	0x40	/* r/w - enable interrupts on errors */
#define	PER_TEST	0x20	/* r/w - write inverse parity */
#define	PER_CHECK	0x10	/* r/w - enable parity checking */
#define	PER_ERR24	0x08	/* r/o - parity error <31..24> */
#define	PER_ERR16	0x04	/* r/o - parity error <23..16> */
#define	PER_ERR08	0x02	/* r/o - parity error <15..8> */
#define	PER_ERR00	0x01	/* r/o - parity error <7..0> */
#define	PER_ERR		0x0F	/* r/o - mask for some parity error occuring */
#define	PARERR_BITS	"\20\10INTR\7INTENA\6TEST\5CHECK\4ERR24\3ERR16\2ERR08\1ERR00"

/*
 *  Bits for the memory error register when used as ECC/MIXED error register.
 */
#define EER_INTR	0x80	/* r/o - ECC memory interrupt pending */
#define EER_INTENA	0x40	/* r/w - enable interrupts on errors */
				/* 0x20 unused */
#define EER_CE_ENA	0x10	/* r/w - enable CE recording */
#define	EER_TIMEOUT	0x08	/* r/o - bus time out */
				/* 0x04 unused */
#define EER_UE		0x02	/* r/o - uncorrectable error (parity/ecc)  */
#define EER_CE		0x01	/* r/o - correctable (single bit) error */
#define	EER_ERR		0x0B	/* r/o - mask for some error occuring */
#define ECCERR_BITS	"\20\10INTR\7INTENA\5CE_ENA\4TIMEOUT\2UE\1CE"

#define	ER_INTR		0x80	/* mask for ECC/parity interrupt pending */
#define	ER_INTENA	0x40	/* mask for ECC/parity enable interrupt */

#define MEMINTVL	60	/* sixty second delay for softecc */

#endif /*!_sun3x_memerr_h*/
