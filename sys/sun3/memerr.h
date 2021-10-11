/*      @(#)memerr.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * All Sun-3 implementations have either memory parity error detection
 * or memory equipped with error correction (ECC). The memory error
 * register consists of a control and an address register.  If an error
 * occurs, the control register stores information relevant to the error.
 * The memory address error register stores the virtual address, the
 * context number, and the CPU/DVMA bit of the memory cycle at which
 * the error was detected.  Errors are reported via a non-maskable
 * level 7 interrupt.  In case of multiple (stacked) memory errors,
 * the information relation to the first error is latched in the
 * memory error register.  The interrupt is held pending and the error
 * information in the memory error register is latched (frozen) until
 * it is cleared (unfrozen) by a write to bits <31..24> of the memory
 * error address register.
 */

#ifndef _sun3_memerr_h
#define _sun3_memerr_h

#define	OBIO_MEMREG 0x80000	/* address of memreg in obio space */

#ifdef LOCORE
#define	MEMREG 0x0FFE4000	/* virtual address we map memreg to be at */
#else
struct memreg {
	u_char	mr_er;		/* memory error control register */
#define	mr_per	mr_er		/* parity error register */
#define	mr_eer	mr_er		/* ECC error register */
	u_char	mr_undef[3];
	u_int	mr_dvma	: 1;
	u_int	mr_ctx	: 3;
	u_int	mr_vaddr:28;
};
#define	MEMREG ((struct memreg *)(0x0FFE4000))
#endif LOCORE

/*
 *  Bits for the memory error register when used as parity error register
 */
#define PER_INTR	0x80	/* r/o - 1 = parity interrupt pending */
#define PER_INTENA	0x40	/* r/w - 1 = enable interrupt on parity error */
#define PER_TEST	0x20	/* r/w - 1 = write inverse parity */
#define PER_CHECK	0x10	/* r/w - 1 = enable parity checking */
#define PER_ERR24	0x08	/* r/o - 1 = parity error <24..31> */
#define PER_ERR16	0x04	/* r/o - 1 = parity error <16..23> */
#define PER_ERR08	0x02	/* r/o - 1 = parity error <8..15> */
#define PER_ERR00	0x01	/* r/o - 1 = parity error <0..7> */
#define	PER_ERR		0x0F	/* r/o - mask for some parity error occuring */
#define PARERR_BITS	"\20\10INTR\7INTENA\6TEST\5CHECK\4ERR24\3ERR16\2ERR08\1ERR00"

/*
 *  Bits for the memory error register when used as ECC error register
 */
#define EER_INTR	0x80	/* r/o - ECC memory interrupt pending */
#define EER_INTENA	0x40	/* r/w - enable interrupts on errors */
#define EER_BUSHOLD	0x20	/* r/w - hold memory bus mastership */
#define EER_CE_ENA	0x10	/* r/w - enable CE recording */
#define	EER_TIMEOUT	0x08	/* r/o - Sirius bus time out */
#define	EER_WBACKERR	0x04	/* r/o - write back error */
#define EER_UE		0x02	/* r/o - UE, uncorrectable error  */
#define EER_CE		0x01	/* r/o - CE, correctable (single bit) error */
#define	EER_ERR		0x0F	/* r/o - mask for some ECC error occuring */
#define ECCERR_BITS	"\20\10INTR\7INTENA\6BUSHOLD\5CE_ENA\4TIMEOUT\3WBACKERR\2UE\1CE"


#define	ER_INTR		0x80	/* mask for ECC/parity interrupt pending */
#define	ER_INTENA	0x40	/* mask for ECC/parity enable interrupt */

#define MEMINTVL	60	/* sixty second delay for softecc */

#endif /*!_sun3_memerr_h*/
