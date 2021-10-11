#if !defined(lint) && !defined(BOOTBLOCK)
static char	sccsid[] = "@(#)boot_if.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 *
 * SAIO network (if) device driver using OPENPROMs ROMvec calls
 */

#include <sys/types.h>
#include <stand/saio.h>
#include "boot/conf.h"

extern int	romdev, rompclose(), rompopen(), nullprobe(), nullboot();

int	ifopen(), ifstrategy(), ifclose();
int	ifxmit(), ifpoll(), ifreset(), ifmacaddr();

static void ifsetmacaddr(), ifclearmacaddr();

struct boottab ifdriver = {
	"if", nullprobe, nullboot, ifopen, ifclose, ifstrategy,
	0, 	/* b_desc  descriptive string */
	0, 	/* b_devinfo memory allocation stuff */
};


struct saif ifsaif = {
	ifxmit, ifpoll, ifreset, ifmacaddr
};


int
ifopen(sip)
struct saioreq *sip;
{
	int i;
	sip->si_sif = &ifsaif;

	if ((i = rompopen(sip)) != 0)  {
		printf("ifopen: rompopen failed\n");
		return (i);
	}
	/* Set global mac address here for use by ifmacaddr */
	ifsetmacaddr(romdev);
	return (i);
}

int
ifclose(sip)
struct saioreq *sip;
{
	ifclearmacaddr();
	return (rompclose(sip));
}


/*ARGSUSED*/
int
ifstrategy(sip, rw)
struct saioreq *sip;
int	rw;
{
	return (-1);
}


/*ARGSUSED*/
int
ifxmit(es, buf, count)
{
	register struct binfo *bd;
#ifdef DEBUG
	printf("ifxmit() count = %x buf = %x\n", count, buf);
#endif DEBUG

	if (prom_getversion() > 0) {
		/* get file descriptor */
		bd = (struct binfo *)es;
		romdev = bd->ihandle;
		if ((*romp->op2_write)(romdev, buf, count) == count)
			return (0);
		else
			return (1);
	}
#ifndef sun4m
	if ((*romp->v_xmit_packet)(romdev, count, buf) == count)
		return (0);
	return (1);
#endif !sun4m
}


/*ARGSUSED*/
int
ifpoll(es, buf)
{
	register struct binfo *bd;

	if (prom_getversion() > 0) {
		/* get file descriptor */
		bd = (struct binfo *)es;
		romdev = bd->ihandle;
		return ((*romp->op2_read)(romdev, buf, 1500));
	}
#ifndef sun4m
	return ((*romp->v_poll_packet)(romdev, 1500, buf));
#endif !sun4m
}


/*ARGSUSED*/
int
ifreset(es)
{
	return (0);
}


/*
 * ifsetmacaddr:	Called at ifopen() time to buffer the net
 *			address used during booting.  If later than V0
 *			PROMS, can use the generic property interface to
 *			get the prop "mac-address" from the devinfo node
 *			of the boot device.  If V0, then we have to fall back
 *			to first trying v_mac_address() PROM function and then
 *			reading NVRAM.  In both of these latter cases, only
 *			ethernet boot is supported, thus a fixed size, 6 byte
 *			buffer may be reserved for this purpose.
 */

static char *ifmacaddrp;
static char  ifethernetaddr[6];
static int  ifmacaddrlen = 0;

static void
ifsetmacaddr(dev)
{
	dnode_t devi;
	char *p, *kmem_alloc();

	if (ifmacaddrlen != 0)
		ifclearmacaddr();

	if (prom_getversion() > 0) {
		devi = (dnode_t)(prom_getphandle(dev));
		if ((devi == OBP_BADNODE) || (devi == OBP_NONODE))  {
#ifdef	DEBUG
			printf("ifsetmacaddr: Can't get nodeid of boot dev.\n");
#endif	DEBUG
			;
		}  else  {
			ifmacaddrlen = prom_getproplen(devi, OBP_MAC_ADDR);
			if (ifmacaddrlen > 0)  {
				if (ifmacaddrlen <= sizeof (ifethernetaddr))
					ifmacaddrp = (char *)(ifethernetaddr);
				else
					ifmacaddrp = kmem_alloc(ifmacaddrlen);
				p = (char *)(getlongprop(devi, OBP_MAC_ADDR));
				if ((p != 0) && (ifmacaddrp != 0))  {
					bcopy(p, ifmacaddrp, ifmacaddrlen);
					return;
				}
			}
		}
#ifdef	DEBUG
		printf("ifsetmacaddr: Can't get mac-address property.\n");
#endif	DEBUG
	}

	if ((int)romp->v_mac_address)  {
		ifmacaddrp = (char *)(ifethernetaddr);
		ifmacaddrlen = sizeof (ifethernetaddr);
		(void)(*romp->v_mac_address)(dev, ifmacaddrp);
		return;
	}  else  {
		ifmacaddrp = (char *)(ifethernetaddr);
		ifmacaddrlen = sizeof (ifethernetaddr);
#ifdef	DEBUG
		printf("ifsetmacaddr: ");
#endif	DEBUG
		(void)myetheraddr(ifethernetaddr);
	}
}

static void
ifclearmacaddr()
{
	if (ifmacaddrlen)  {
#ifdef	DEBUG
		printf("ifclearmacaddr: Clearing buffered if address.\n");
#endif	DEBUG
		if (ifmacaddrp != (char *)(ifethernetaddr))
			kmem_free(ifmacaddrp, ifmacaddrlen);
	}
	ifmacaddrlen = 0;
}

/*ARGSUSED*/
int
ifmacaddr(es, buf)
caddr_t buf;	/* should be struct ether_addr, but... */
{
	register struct binfo *bd;
	register dnode_t devi;

	/*
	 * If ifmacaddrlen is set (assume the same interface)
	 * return bufferred ethernet address.
	 */

	if (ifmacaddrlen)  {
		bcopy(ifmacaddrp, buf, ifmacaddrlen);
		return (0);
	}

	/*
	 * What to do -- asking for the address of an unopened device?
	 * XXX: Probably, the rest of this can go away.
	 */


	if (prom_getversion() > 0) {
		/* get file descriptor */
		bd = (struct binfo *)es;
		romdev = bd->ihandle;

		devi = (dnode_t)(prom_getphandle(romdev));
		if ((devi == OBP_BADNODE) || (devi == OBP_NONODE))  {
#ifdef	DEBUG
			printf("ifmacaddr: Can't get nodeid of boot dev.\n");
#endif	DEBUG
			;
		}  else  {

			register int len;
			register char *p;

			len = prom_getproplen(devi, OBP_MAC_ADDR);
			p = (char *)(getlongprop(devi, OBP_MAC_ADDR));
			if ((len > 0) && (p != 0))  {
				bcopy(p, buf, len);
				return (0);
			}
#ifdef	DEBUG
			printf("ifmacaddr: Can't get mac-address property!\n");
#endif	DEBUG
		}
	}

	if ((int)romp->v_mac_address)  {
#ifdef	DEBUG
		printf("ifmacaddr: no buffered address: Calling v_mac_addr!\n");
#endif	DEBUG
		return ((*romp->v_mac_address)(romdev, buf));
	}  else  {
#ifdef	DEBUG
		printf("ifmacaddr (unbuffered): ");
#endif	DEBUG
		return (myetheraddr(buf));
	}
}
