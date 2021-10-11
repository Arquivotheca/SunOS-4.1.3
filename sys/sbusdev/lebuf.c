#ident	"@(#)lebuf.c	1.1"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include "le.h"
#if	NLE > 0

/*
 * LANCE Ethernet Programmable I/O Buffer 'driver'.
 *
 * This driver identifies "lebuffer", maps in the memory associated
 * with the device, attaches child nodes ("le"!) and calls the
 * le driver routine leopsadd() to add an entry to the le driver
 * ops linked list describing the lebuffer-specifics.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/systm.h>

#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/debug.h>
#include <sundev/mbvar.h>

#include <sun/autoconf.h>
#include <machine/mmu.h>

#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sunif/if_lereg.h>
#include <sunif/if_levar.h>

static int lebufidentify(), lebufattach();
struct	dev_ops lebuf_ops = {
	1,
	lebufidentify,
	lebufattach
};

static int
lebufidentify(name)
char *name;
{
	if (strcmp(name, "lebuffer") == 0)
		return (1);
	else
		return (0);
}

static int
lebufattach(dev)
register struct dev_info *dev;
{
	static	int	unit = 0;
	addr_t	base;
	struct	leops	*lop;

	dev->devi_unit = unit++;

	if (dev->devi_nreg > 1) {
		printf("lebuf%d: bad regs specification\n", dev->devi_unit);
		return (-1);
	}

	/* map in the buffer */
	base = map_regs(dev->devi_reg->reg_addr, dev->devi_reg->reg_size,
		dev->devi_reg->reg_bustype);
	if (base == NULL) {
		printf("lebuf%d: unable to map registers\n", dev->devi_unit);
		return (-1);
	}

	report_dev(dev);
	attach_devs(dev);	/* attach children */

	/* add entry to le driver ops linked list */
	if ((lop = (struct leops *) kmem_alloc(sizeof (struct leops))) == NULL) {
		printf("lebuf%d:  kmem_alloc failed!\n", dev->devi_unit);
		return (-1);
	}
	lop->lo_dev = dev->devi_slaves;
	lop->lo_flags = LO_PIO | LO_IOADDR;
	lop->lo_membase = base;
	lop->lo_iobase = (caddr_t)0;
	lop->lo_size = (int) dev->devi_reg->reg_size;
	lop->lo_init = NULL;
	lop->lo_intr = NULL;
	lop->lo_arg = 0;
	leopsadd(lop);

	return (0);
}

#endif	NLE > 0
