#ident	"@(#)ie_conf.c	1.1"
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * Sun Intel Ethernet Controller configuration file
 */

#include	<sys/param.h>
#include	<sys/mbuf.h>
#include	<sys/buf.h>
#include	<sys/map.h>
#include	<sys/socket.h>
#include	<sys/errno.h>
#include	<sys/time.h>
#include	<sys/kernel.h>
#include	<sys/ioctl.h>
#include	<net/if.h>
#include	<net/if_arp.h>
#include	<netinet/in.h>
#include	<netinet/if_ether.h>
#include	<sundev/mbvar.h>
#include	<sunif/if_lereg.h>
#include	<sunif/if_levar.h>
#include	<sunif/if_mie.h>
#include	<sunif/if_obie.h>
#include	<sunif/if_tie.h>
#include	<sunif/if_iereg.h>
#include	<sunif/if_ievar.h>
#include	"ie.h"

#if NIE > 0
/*
 * Ethernet software status per interface.
 */
struct	ie_softc	ie_softc[NIE];
#define	IERMAPSIZE	5
struct	map	iermap[NIE][IERMAPSIZE];
struct	mb_device	*ieinfo[NIE];

int	ie_units = NIE;
#endif

/*
 * Resource amounts.
 *	For now, at least, these never change, and resources
 *	are allocated once and for all at boot time.
 *
 * NFS servers and gateways get generous amounts, otherwise we cut
 * them down to save memory. Sun-4s are fast enough to almost never
 * require more than a few receive buffers.
 */


/*
 * These "high" values are only used when multiple interfaces are present
 */
int	ie_high_tbds  = 24;	/* # transmit buffer descriptors */
int	ie_high_tfds  = 12;	/* # transmit frame descriptors */
int	ie_high_tbufs =  3;	/* # static transmit buffers */
int	ie_high_rbds  = 20;	/* # receive buffer descriptors */
int	ie_high_rfds  = 19;	/* # receive frame descriptors (< ie_rbds!) */
int	ie_high_rbufs = 40;	/* # static receive buffers (> ie_rbds !) */

/*
 * These "low" values are only used when a single interface is present.
 */
int	ie_low_tbds  = 24;
int	ie_low_tfds  = 12;
int	ie_low_tbufs =  3;
int	ie_low_rbds  = 20;
int	ie_low_rfds  = 19;
int	ie_low_rbufs = 40;
