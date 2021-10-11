/*	@(#)enable.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4c_enable_h
#define	_sun4c_enable_h

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * The System Enable register controls overall
 * operation of the system.  When the system is
 * reset, the Enable register is cleared.  The
 * enable register is addressed as a byte in
 * ASI_CTL space.
 */

/*
 * Bits of the Enable Register
 */
#define	ENA_SWRESET	0x04		/* r/w - software reset */
#define	ENA_CACHE	0x10		/* r/w - enable external cache */
#define	ENA_SDVMA	0x20		/* r/w - enable system DVMA */
#define	ENA_NOTBOOT	0x80		/* r/w - non-boot state, 1 = normal */


#define	ENABLEREG	0x40000000	/* addr in ASI_CTL space */

#endif	/* !_sun4c_enable_h */
