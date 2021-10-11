/*	@(#)hetero.h 1.1 92/07/30 SMI; from S5R3.1 10.3.1.1	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Define machine attributes for heterogeneity.
 *
 *	Machine attribute consists of three components -
 *	byte ordering, alignment, and data unit size.
 *	The machine attributes are defined in a byte (8 bits),
 *	the lower 2 bits are used to define the byte ordering,
 *	the middle 3 bits are used to define the alignment,
 *	the higher 3 bits are used to define the data unit size.
 *
 *
 *	BYTE_ORDER	0x01	3B, IBM byte ordering
 *			0x02	VAX byte ordering
 *	ALIGNMENT	0x04	word aligned (4 bytes boundary)
 *			0x08	half-word aligned (2 bytes boundary)
 *			0x0c	byte aligned
 *	UNIT_SIZE	0x20	4 bytes integer, 2 bytes short, 4 bytes pointer
 *			0x40	2 bytes integer, 2 bytes short, 2 bytes pointer
 */

#ifndef _rfs_hetero_h
#define _rfs_hetero_h

/*
 *	Define masks for machine attributes
 */
#define BYTE_MASK	0x03
#define ALIGN_MASK	0x1c
#define UNIT_MASK	0xe0


/*
 *	Define what need to be converted - header or data parts
 */

#define ALL_CONV	0	/* convert both header and data parts */
#define DATA_CONV	1	/* convert data part */
#define NO_CONV		2	/* no conversion needed */



/*
 *	Define machine attributes for 3B
 */

#ifdef u3b2
#define BYTE_ORDER	0x01
#define ALIGNMENT	0x04
#define UNIT_SIZE	0x20
#endif
#ifdef mc68010
#define BYTE_ORDER	0x01
#define ALIGNMENT	0x08
#define UNIT_SIZE	0x20
#endif
#ifdef mc68020
#define BYTE_ORDER	0x01
#define ALIGNMENT	0x08
#define UNIT_SIZE	0x20
#endif
#ifdef sparc
#define BYTE_ORDER	0x01
#define ALIGNMENT	0x04
#define UNIT_SIZE	0x20
#endif

#define MACHTYPE	(BYTE_ORDER | ALIGNMENT | UNIT_SIZE)

#endif /*!_rfs_hetero_h*/
