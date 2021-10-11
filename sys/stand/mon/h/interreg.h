/*
 * interreg.h
 *
 * @(#)interreg.h 1.1 92/07/30 SMI
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * The interrupt register provides for the generation of software
 * interrupts and controls the video and clock hardware interrupts.
 */
#if defined(sun4) || defined(sun4c) || defined(sun4x)
#define	OBIO_INTERREG 0xf5000000 /* address of interreg in obio space */
#else sun4
#define	OBIO_INTERREG 0x000A0000 /* address of interreg in obio space */
#endif sun4

#ifdef LOCORE
#define	INTERREG 0x0FFE6000 /* virtual address we map interreg to be at */
#else LOCORE
#if defined(sun4) || defined(sun4c) || defined(sun4x)
#define	INTERREG ((u_char *)(0xFFFE6000))
#else sun4
#define INTERREG ((u_char *)(0x0FFE6000))
#endif sun4
#endif LOCORE

/*
 * Bits of the interrupt register.
 */
#if defined(sun4) || defined(sun4c) || defined(sun4x)
#define	IR_ENA_CLK14	0x80	/* r/w - enable clock level 14 interrupt */
#define	IR_ENA_CLK10	0x20	/* r/w - enable clock level 10 interrupt */
#define	IR_ENA_VID8	0x10	/* r/w - enable video level 8 interrupt */
#define	IR_SOFT_INT6	0x08	/* r/w - cause software level 6 interrupt */
#define	IR_SOFT_INT4	0x04	/* r/w - cause software level 4 interrupt */
#define	IR_SOFT_INT1	0x02	/* r/w - cause software level 1 interrupt */
#define	IR_ENA_INT	0x01	/* r/w - enable (all) interrupts */
#else sun4
#define	IR_ENA_CLK7	0x80	/* r/w - enable clock level 7 interrupt */
#define	IR_ENA_CLK5	0x20	/* r/w - enable clock level 5 interrupt */
#define	IR_ENA_VID4	0x10	/* r/w - enable video level 4 interrupt */
#define	IR_SOFT_INT3	0x08	/* r/w - cause software level 3 interrupt */
#define	IR_SOFT_INT2	0x04	/* r/w - cause software level 2 interrupt */
#define	IR_SOFT_INT1	0x02	/* r/w - cause software level 1 interrupt */
#define	IR_ENA_INT	0x01	/* r/w - enable (all) interrupts */
#endif sun4
