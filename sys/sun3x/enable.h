/*      @(#)enable.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * The System Enable register controls overall
 * operation of the system.  When the system is
 * reset, the Enable register is cleared.
 */

#ifndef _sun3x_enable_h
#define _sun3x_enable_h

#include <machine/devaddr.h>

#define	ENABLEREG	(V_ENABLEREG)

/*
 * Bits of the Enable Register
 */
#define	ENA_DBGCACHE	0x0008		/* r/w - debug mode for system cache */
#define	ENA_LOOPBACK	0x0010		/* r/w - vme loopback mode */
#define	ENA_IOCACHE	0x0020		/* r/w - enable I/O cache */
#define	ENA_CACHE	0x0040		/* r/w - enable system cache */
#define	ENA_DIAG	0x0100		/* r/w - diag switch/programmable */
#define	ENA_FPA		0x0200		/* r/w - enable floating point accel */
#define	ENA_RES		0x0400		/* r/o - monitor resolution */
#define	ENA_VIDEO	0x0800		/* r/w - enable video memory */
#define	ENA_SDVMA	0x2000		/* r/w - enable system DVMA */
#define	ENA_FPP		0x4000		/* r/w - enable floating point proc */
#define	ENA_NOTBOOT	0x8000		/* r/w - non-boot state, 1 = normal */

#endif /*!_sun3x_enable_h*/
