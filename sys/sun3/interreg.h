/*      @(#)interreg.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * The interrupt register provides for the generation of software
 * interrupts and controls the video and clock hardware interrupts.
 */

#ifndef _sun3_interreg_h
#define _sun3_interreg_h

#define	OBIO_INTERREG 0xA0000	/* address of interreg in obio space */

#ifdef LOCORE
#define	INTERREG 0x0FFE6000	/* virtual address we map interreg to be at */
#else
#define	INTERREG ((u_char *)(0x0FFE6000))
#endif

/*
 * Bits of the interrupt register.
 */
#define	IR_ENA_CLK7	0x80	/* r/w - enable clock level 7 interrupt */
#define	IR_ENA_CLK5	0x20	/* r/w - enable clock level 5 interrupt */
#define	IR_ENA_VID4	0x10	/* r/w - enable video level 4 interrupt */
#define	IR_SOFT_INT3	0x08	/* r/w - cause software level 3 interrupt */
#define	IR_SOFT_INT2	0x04	/* r/w - cause software level 2 interrupt */
#define	IR_SOFT_INT1	0x02	/* r/w - cause software level 1 interrupt */
#define	IR_ENA_INT	0x01	/* r/w - enable (all) interrupts */

#endif /*!_sun3_interreg_h*/
