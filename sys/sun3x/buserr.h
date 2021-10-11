/*       @(#)buserr.h 1.1 92/07/30 SMI    */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef _sun3x_buserr_h
#define _sun3x_buserr_h

#include <machine/devaddr.h>
#include <m68k/excframe.h>
/*
 * The System Bus Error Register captures the state of the
 * system as of the last bus error.  The Bus Error Register
 * always latches the cause of the most recent bus error.
 * Thus, in the case of stacked bus errors, the information
 * relating to the earlier bus errors is lost.  The Bus Error
 * register is read-only.
 * The Bus Error Register is clocked for bus errors for CPU Cycles.
 * The Bus Error Register is not clocked for DVMA cycles
 * or for CPU accesses to the floating point chip with the
 * chip not enabled.
 *
 * The rest of the bits are 1 if they caused the error.
 */
#ifdef LOCORE
#define	BUSERRREG	(V_BUSERRREG)
#else
struct	buserrorreg {
	u_int	be_parityerr	:1;	/* Parity error */
	u_int			:1;
	u_int	be_timeout	:1;	/* Bus access timed out */
	u_int	be_vmeberr	:1;	/* VMEbus bus error */
	u_int	be_fpaberr	:1;	/* FPA bus error */
	u_int	be_fpaena	:1;	/* FPA enable error */
	u_int			:1;
	u_int	be_watchdog	:1;	/* Watchdog reset */
};
#define	BUSERRREG	((struct buserrorreg *)V_BUSERRREG)
#endif

/*
 * Equivalent bits.
 */
#define	BE_TIMEOUT	0x20		/* bus access timed out */
#define	BE_VMEBERR	0x10		/* VMEbus bus error */
#define	BE_FPABERR	0x08		/* FPA bus error */
#define	BE_FPAENA	0x04		/* FPA enable error */
#define	BE_WATCHDOG	0x01		/* Watchdog reset */

#define BUSERR_BITS	"\20\6TIMEOUT\5VMEBERR\4FPABERR\3FPAENA\1WATCHDOG"

#endif /*!_sun3x_buserr_h*/

