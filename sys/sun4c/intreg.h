/*	@(#)intreg.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4c_intreg_h
#define	_sun4c_intreg_h

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * The interrupt register provides for the generation of software
 * interrupts and controls the video and clock hardware interrupts.
 */
#define	OBIO_INTREG_ADDR 0xF5000000	/* address in obio space */

#define	INTREG_ADDR	0xFFFFA000	/* virtual address we map it to */

#ifndef LOCORE
#define	INTREG	((u_char *)(INTREG_ADDR))
#endif !LOCORE

/*
 * Bits of the interrupt register.
 */
#define	IR_ENA_CLK14	0x80	/* r/w - enable clock level 14 interrupt */
#define	IR_ENA_CLK10	0x20	/* r/w - enable clock level 10 interrupt */
#define	IR_ENA_LVL8	0x10	/* r/w - enable IPL 8 (SBus IRQ 6) interrupt */
#define	IR_SOFT_INT6	0x08	/* r/w - cause software level 6 interrupt */
#define	IR_SOFT_INT4	0x04	/* r/w - cause software level 4 interrupt */
#define	IR_SOFT_INT1	0x02	/* r/w - cause software level 1 interrupt */
#define	IR_ENA_INT	0x01	/* r/w - enable (all) interrupts */

#endif	/* !_sun4c_intreg_h */
