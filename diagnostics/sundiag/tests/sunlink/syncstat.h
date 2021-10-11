/* @(#)syncstat.h 1.1 92/07/30 SMI; from UCB 4.34 83/06/13      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Statistics for synchronous serial lines Each structure must be kept under
 * 16 bytes to work with the awful ifreq ioctl kludge AAARRRGGGH!
 * 
 * This file is shared between the main CPU and the DCP board, so we use longs
 * instead of ints for portability.
 */

/* data stats */
struct ss_dstats {
    long            ssd_ipack;		       /* input packets */
    long            ssd_opack;		       /* output packets */
    long            ssd_ichar;		       /* input bytes */
    long            ssd_ochar;		       /* output bytes */
};

/* error stats */
struct ss_estats {
    long            sse_abort;		       /* abort received */
    long            sse_crc;		       /* CRC error */
    long            sse_overrun;	       /* receiver overrun */
    long            sse_underrun;	       /* xmitter underrun */
};
