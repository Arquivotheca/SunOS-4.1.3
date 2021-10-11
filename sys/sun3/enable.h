/*      @(#)enable.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * The System Enable register controls overall
 * operation of the system.  When the system is
 * reset, the Enable register is cleared.  The
 * enable register is addressed as a byte in
 * FC_MAP space.
 */

#ifndef _sun3_enable_h
#define _sun3_enable_h

/*
 * Bits of the Enable Register
 */
#define	ENA_DIAG	0x01		/* r/o - diag switch, 1 = on */
#define	ENA_FPA		0x02		/* r/w - enable floating point accel */
#define	ENA_COPY	0x04		/* r/w - enable copy update mode */
#define	ENA_VIDEO	0x08		/* r/w - enable video memory */
#define	ENA_CACHE	0x10		/* r/w - enable external cache */
#define	ENA_SDVMA	0x20		/* r/w - enable system DVMA */
#define	ENA_FPP		0x40		/* r/w - enable floating point proc */
#define	ENA_NOTBOOT	0x80		/* r/w - non-boot state, 1 = normal */


#define	ENABLEREG	0x40000000	/* addr of enable reg in FC_MAP space */

#endif /*!_sun3_enable_h*/
