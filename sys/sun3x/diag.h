/*       @(#)diag.h 1.1 92/07/30 SMI    */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * The diagnostic register drvies an 8-bit LED display.
 * A "0" bit written will cause the corresponding LED to light up,
 * a "1" bit to be dark.
 */

#ifndef _sun3x_diag_h
#define _sun3x_diag_h

#include <machine/devaddr.h>

#define	DIAGREG		(V_DIAGREG)

#endif /*!_sun3x_diag_h*/
