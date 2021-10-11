/*      @(#)udvma.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * On implementations that allow user DVMA, the User DVMA enable
 * register controls which contexts have DVMA access. The user DVMA
 * enable register is addressed as a byte in FC_MAP space.
 */

#ifndef _sun3_udvma_h
#define _sun3_udvma_h

/*
 * Bits for the User DVMA enable register
 */
#define	UDVMA_CX7	0x80		/* User DVMA for context 7 */
#define	UDVMA_CX6	0x40		/* User DVMA for context 6 */
#define	UDVMA_CX5	0x20		/* User DVMA for context 5 */
#define	UDVMA_CX4	0x10		/* User DVMA for context 4 */
#define	UDVMA_CX3	0x08		/* User DVMA for context 3 */
#define	UDVMA_CX2	0x04		/* User DVMA for context 2 */
#define	UDVMA_CX1	0x02		/* User DVMA for context 1 */
#define	UDVMA_CX0	0x01		/* User DVMA for context 0 */

#define	UDVMA_NORMAL	0x00		/* Normal - all user DVMA disabled */

#define	UDVMA_REG	0x50000000	/* addr of udvma reg in FC_MAP space */

#endif /*!_sun3_udvma_h*/
