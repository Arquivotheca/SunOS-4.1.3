/*	@(#)auxio.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4m_auxio_h
#define	_sun4m_auxio_h

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Definitions and structures for the Auxiliary Input/Output register
 * on sun4m machines.
 */
#define AUXIO_REG       0xFEFF0000      /* the virtual address of the 
					 * register
					 */

/*
 * Bits of the auxio register -- input bits must always be written with ones
 */
#define	AUX_MBO		0xC0		/* Must be written with ones */
/*
 * NOTE: whenever writing to the auxio register, or in AUX_MBO!!!
 */
#define	AUX_LED		0x01		/* LED (output); 1 = on, 0 = off */
#define	AUX_TC		0x02		/* Floppy terminal count (output) */
					/* 1 = transfer over */
#define AUX_IMUX	0x08		/* SCCB-IMUX (Midi/Serial Mux) */
					/* 0 = Serial Port, 1 = Midi port */
#define AUX_EDGEON	0x10		/* Test edge connector jumper block */
					/* in. 1 = jumper block is in */
#define	AUX_DENSITY	0x20		/* Floppy density (Input) */
					/* 1 = high, 0 = low */

/*
 * The following defines are left in so that old code will compile.
 * These do not apply to the current sun4m auxio0 register!!!
 */

#define	AUX_DISKCHG	0x10		/* Floppy diskette change (input) */
					/* 1 = new diskette inserted */
#define	AUX_DRVSELECT	0x08		/* Floppy drive select (output) */
					/* 1 = selected, 0 = deselected */
#define	AUX_EJECT	0x02		/* Floppy eject (output, NON inverted)*/
					/* 0 = eject the diskette */

#endif	/* !_sun4m_auxio_h */
