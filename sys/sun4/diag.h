/*      @(#)diag.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _sun4_diag_h
#define _sun4_diag_h

/*
 * The diagnostic register drvies an 8-bit LED display.
 * This register is addressed in ASI_CTL space and is write only.
 * A "0" bit written will cause the corresponding LED to light up,
 * a "1" bit to be dark.
 */
#define	DIAGREG		0x70000000	/* addr of diag reg in ASI_CTL space */

#endif /*!_sun4_diag_h*/
