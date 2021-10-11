/*      @(#)diag.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _sun3_diag_h
#define _sun3_diag_h

/*
 * The diagnostic register drvies an 8-bit LED display.
 * This register is addressed in FC_MAP space and is write only.
 * A "0" bit written will cause the corresponding LED to light up,
 * a "1" bit to be dark.
 */
#define	DIAGREG		0x70000000	/* addr of diag reg in FC_MAP space */

#endif /*!_sun3_diag_h*/
