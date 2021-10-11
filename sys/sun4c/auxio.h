/*	@(#)auxio.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4c_auxio_h
#define	_sun4c_auxio_h

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Definitions and structures for the Auxiliary Input/Output register
 * on sun4c machines.
 */
#define	OBIO_AUXIO_ADDR	0xF7400000	/* addr in obio space */
#define	AUXIO_ADDR	0xFFFF6000	/* virtual addr we map to */
#define	AUXIO_REG	0xFFFF6003	/* the address of the register */

/* Bits of the auxio register -- input bits must always be written with ones */
#define	AUX_MBO		0xF0		/* Must be written with ones */
/* NOTE: whenever writing to the auxio register, or in AUX_MBO!!! */
#define	AUX_DENSITY	0x20		/* Floppy density (Input) */
					/* 1 = high, 0 = low */
#define	AUX_DISKCHG	0x10		/* Floppy diskette change (input) */
					/* 1 = new diskette inserted */
#define	AUX_DRVSELECT	0x08		/* Floppy drive select (output) */
					/* 1 = selected, 0 = deselected */
#define	AUX_TC		0x04		/* Floppy terminal count (output) */
					/* 1 = transfer over */
#define	AUX_EJECT	0x02		/* Floppy eject (output, NON inverted)*/
					/* 0 = eject the diskette */
#define	AUX_LED		0x01		/* LED (output); 1 = on, 0 = off */

#endif	/* !_sun4c_auxio_h */
