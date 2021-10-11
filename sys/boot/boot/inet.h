/*
 * @(#)inet.h 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Following defn stolen from nd structure.
 */

#define NO_NDBASE  (struct no_nd *)(sip->si_devdata)


/* standalone RAM variables */
struct no_nd {
        int     no_nd_seq;                 /* current sequence number */
        struct ether_header no_nd_xh;      /* xmit header and packet */
        int     no_nd_block;               /* block number of data in "cache" */
        char    no_nd_buf[1600];           /* temp buf for packets */
        struct  sainet no_nd_inet;         /* INET state */
};

