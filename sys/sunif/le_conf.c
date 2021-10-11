#ident	"@(#)le_conf.c	1.1"
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * Sun AMD Ethernet Controller configuration file
 */

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/map.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sundev/mbvar.h>
#include <machine/mmu.h>
#include <sunif/if_lereg.h>
#include <sunif/if_levar.h>
 
#include "le.h"
#include "ie.h"

#if NLE > 0
/*
 * Ethernet software status per interface.
 */
struct le_softc *le_softc;
#ifdef	OPENPROMS
struct le_info *leinfo;
int	le_units = 0;
#else	OPENPROMS
struct	mb_device *leinfo[NLE];

int     le_units = NLE;
#endif	OPENPROMS
#endif	NLE > 0

/*
 * Resource amounts.
 *	For now, at least, these never change, and resources
 *	are allocated once and for all at boot time.
 *
 * NFS servers and gateways get generous amounts, otherwise we cut
 * them down to save memory. Sun-4s are fast enough to almost never
 * require more than a few receive buffers.
 *
 * Some systems dynamically autoconfigure (OPENPROMS systems,
 * e.g. 4/60 (Sparcstation 1)) therefore we can't tell at
 * compile-time whether the system will have multiple network
 * interfaces and can be a gateway or not.
 */

/*
 * These "high" values are only used when multiple interfaces are present.
 */
int	le_high_ntmdp2 = 7;	/* power of 2 Transmit Ring Descriptor */
int	le_high_nrmdp2 = 5;	/* power of 2 Receive Ring Descriptor */
int	le_high_nrbufs = 40;	/* Receive Buffer (~1600 bytes each) */

/*
 * These "low" values are only used when a single interface is present.
 */
#if defined(NFSSERVER)
int	le_low_ntmdp2 = 7;
int	le_low_nrmdp2 = 5;
int	le_low_nrbufs = 40;

#else
#if defined(sparc)
int	le_low_ntmdp2 = 7;
int	le_low_nrmdp2 = 3;
int	le_low_nrbufs = 8;
#else
int	le_low_ntmdp2 = 7;
int	le_low_nrmdp2 = 3;
int	le_low_nrbufs = 10;
#endif defined(sparc)
#endif defined(NFSSERVER)
