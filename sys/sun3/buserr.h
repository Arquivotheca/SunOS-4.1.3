/*	@(#)buserr.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Definitions and structures related to 68020 bus error handling for Sun-3.
 */

#ifndef _sun3_buserr_h
#define _sun3_buserr_h

#include <m68k/excframe.h>

/*
 * The System Bus Error Register captures the state of the
 * system as of the last bus error.  The Bus Error Register
 * always latches the cause of the most recent bus error.
 * Thus, in the case of stacked bus errors, the information
 * relating to the earlier bus errors is lost.  The Bus Error
 * register is read-only and is addressed in FC_MAP space.
 * The Bus Error Register is clocked for bus errors for CPU Cycles
 * line.  The Bus Error Register is not clocked for DVMA cycles
 * or for CPU accesses to the floating point chip with the
 * chip not enabled.
 *
 * The be_proterr bit is set only if the valid bit was set AND the
 * page protection does not allow the kind of operation attempted.
 * The rest of the bits are 1 if they caused the error.
 */
#ifndef LOCORE
struct	buserrorreg {
	u_int	be_invalid	:1;	/* Page map Valid bit was off */
	u_int	be_proterr	:1;	/* Protection error */
	u_int	be_timeout	:1;	/* Bus access timed out */
	u_int	be_vmeberr	:1;	/* VMEbus bus error */
	u_int	be_fpaberr	:1;	/* FPA bus error */
	u_int	be_fpaena	:1;	/* FPA enable error */
	u_int			:1;
	u_int	be_watchdog	:1;	/* Watchdog reset */
};
#endif !LOCORE

/*
 * Equivalent bits.
 */
#define	BE_INVALID	0x80		/* page map was invalid */
#define	BE_PROTERR	0x40		/* protection error */
#define	BE_TIMEOUT	0x20		/* bus access timed out */
#define	BE_VMEBERR	0x10		/* VMEbus bus error */
#define	BE_FPABERR	0x08		/* FPA bus error */
#define	BE_FPAENA	0x04		/* FPA enable error */
#define	BE_WATCHDOG	0x01		/* Watchdog reset */

#define	BUSERRREG	0x60000000	/* addr of buserr reg in FC_MAP space */

#define BUSERR_BITS	"\20\10INVALID\7PROTERR\6TIMEOUT\5VMEBERR\4FPABERR\3FPAENA\1WATCHDOG"

#endif /*!_sun3_buserr_h*/
