/* @(#)syncmode.h 1.1 92/07/30 SMI; from UCB 4.34 83/06/13      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Definitions for modes of operations of synchronous serial lines, both
 * RS-232 and RS-449 This structure must be kept under 16 bytes in order to
 * work with the ifreq ioctl kludge.
 */
struct syncmode {
    char            sm_txclock;		       /* enum - transmit clock
					        * sources */
    char            sm_rxclock;		       /* enum - receive clock
					        * sources */
    char            sm_loopback;	       /* boolean - do internal
					        * loopback */
    char            sm_nrzi;		       /* boolean - use NRZI */
    long            sm_baudrate;	       /* baud rate - bits per second */
};
/* defines for txclock */
#define	TXC_IS_TXC	0		       /* use incoming transmit clock */
#define	TXC_IS_RXC	1		       /* use incoming receive clock */
#define	TXC_IS_BAUD	2		       /* use baud rate generator */
#define	TXC_IS_PLL	3		       /* use phase-lock loop output */

/* defines for rxclock */
#define	RXC_IS_RXC	0		       /* use incoming receive clock */
#define	RXC_IS_TXC	1		       /* use incoming transmit clock */
#define	RXC_IS_BAUD	2		       /* use baud rate - only good
					        * for loopback */
#define	RXC_IS_PLL	3		       /* use phase-lock loop */
