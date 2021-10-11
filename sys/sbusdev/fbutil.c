#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)fbutil.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1988, Sun Microsystems, Inc.
 */

/*
 * Sbus Frame buffer driver utilities.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/map.h>
#include <sys/mman.h>
#include <sys/vmmac.h>
#include <machine/pte.h>
#include <sundev/mbvar.h>
#include <vm/hat.h>


/* ARGSUSED */
fbopen(dev, flag, numdevs)
	dev_t dev;
	int flag, numdevs;
{
	/* XXX is this the right test? */
	if (minor(dev) >= numdevs)
		return ENXIO;

	return 0;
}

/* get page frame number and type */
fbgetpage(addr)
	addr_t addr;
{
	return (int) hat_getkpfnum(addr);
}

/* XXX not sure about this yet */
#ifdef FBALLOC
/* allocate virtual space */
addr_t
fballoc(size)
	int size;
{
	u_long kmx, rmalloc();

	return (kmx = rmalloc(kernelmap, btoc(size))) ?
		(addr_t) kmxtob(kmx) : 0;
}
#endif FBALLOC

/* simplified mapin/mapout */
fbmapin(virt, phys, size)
	addr_t virt;
	int phys;
	int size;
{
	mapin(&Sysmap[btokmx((struct pte *) virt)], btop(virt),
		(u_int) phys, (int) btoc(size), PG_V | PG_KW);
}

#ifdef FBMAPOUT
fbmapout(virt, size)
	addr_t virt;
	int size;
{
	mapout(&Sysmap[btokmx((struct pte *) virt)], (int) btoc(size));
}
#endif FBMAPOUT
