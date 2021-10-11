#ifndef lint
static char sccsid[] = "@(#)fbutil.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1985, 1988 by Sun Microsystems, Inc.
 */

/*
 * Frame buffer driver utilities.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/vmmac.h>
#include <machine/pte.h>
#include <sundev/mbvar.h>

/* detect new VM system */
#define	NEWVM	defined(_MAP_NEW)


/* ARGSUSED */
fbopen(dev, flag, numdevs, mb_devs)
	dev_t dev;
	int flag, numdevs;
	struct mb_device **mb_devs;
{
	register struct mb_device *md;

	if (minor(dev) >= numdevs || 
		(md = mb_devs[minor(dev)]) == 0 || 
		md->md_alive == 0)
		return ENXIO;

	return 0;
}

/*ARGSUSED*/
fbmmap(dev, off, prot, numdevs, mb_devs, size)
	dev_t dev;
	off_t off;
	int prot;
	int numdevs;
	struct mb_device **mb_devs;
	int size;
{
	if ((u_int) off >= size)
		return -1;
#ifdef lint
	(void) fbmmap(dev, off, prot, numdevs, mb_devs, size);
#endif

	return fbgetpage(mb_devs[minor(dev)]->md_addr) + btop(off);
}

/* get page frame number and type */
fbgetpage(addr)
	caddr_t addr;
{
#if NEWVM
	u_int hat_getkpfnum();

	return (int) hat_getkpfnum((addr_t) addr);

#else NEWVM

	return getkpgmap(addr) & PG_PFNUM;

#endif NEWVM
}

/* simplified mapin/mapout */
fbmapin(virt, phys, size)
	caddr_t virt;
	int phys;
	int size;
{
	mapin(&Usrptmap[btokmx((struct pte *) virt)], btop(virt),
		(u_int) phys, (int) btoc(size), PG_V | PG_KW);
}

fbmapout(virt, size)
	caddr_t virt;
	int size;
{
	mapout(&Usrptmap[btokmx((struct pte *) virt)], (int) btoc(size));
}

#ifdef sun2
/*
 * Call intclear to turn off interrupts on all configured devices.
 * If intclear returns a non-zero value, we found the interrupting
 * device.
 */
fbintr(numdevs, mb_devs, intclear)
	int numdevs;
	register struct mb_device **mb_devs;
	int (*intclear)();
{
	register struct mb_device *md;

	while (--numdevs >= 0) 
		if ((md = *mb_devs++) && 
			md->md_alive &&
			(*intclear)(md->md_addr))
			return 1;

	return 0;
}
#endif sun2

#ifndef sun2

#include <sundev/p4reg.h>
#define P4_ID_TYPE_MASK 0x7f000000

p4probe (addr, w, h)
    caddr_t         addr;
    int            *w,
                   *h;
{
    static int      bwsize[P4_ID_RESCODES][2] = {
	1600, 1280,
	1152, 900,
	1024, 1024,
	1280, 1024,
	1440, 1440,
	640, 480,
	0, 0,
    };
    static int      colorsize[P4_ID_RESCODES][2] = {
	1600, 1280,
	1152, 900,
	1024, 1024,
	1280, 1024,
	1440, 1440,
	1152, 900,		       /* 24-bit */
	0, 0,
    };

    register long  *reg = (long *) addr;
    long            id;
    int             type;

    /* peek the P4 register, then try to modify the type code */
    if (peekl (reg, &id) ||
	pokel (reg, (id &= ~P4_REG_RESET) ^ P4_ID_TYPE_MASK))
	return -1;
    /* NOTREACHED */

    /* if the "type code" changed, put the old value back and quit */
    if ((*reg ^ id) & P4_ID_TYPE_MASK) {
	*reg = id;
	return -1;
	/* NOTREACHED */
    }

    /*
     * Except for cg8, the high nibble is the type and the low nibble is the
     * resolution.
     */
    switch (type = (id = (u_long) id >> 24) & P4_ID_MASK) {
    case P4_ID_FASTCOLOR:	/* 6 */
	    *w = colorsize[5][0];	/* cgsix is 0x60 */
	    *h = colorsize[5][1];
	    return id;
	    /* NOTREACHED */
    case P4_ID_COLOR8P1:	/* 4 */
	if ((id &= ~P4_ID_MASK) < P4_ID_RESCODES) {
	    *w = colorsize[id][0];
	    *h = colorsize[id][1];
	}
	if (id < 5)
	    return type;
	/* NOTREACHED */
	return (type + id);
	/* NOTREACHED */
    case P4_ID_BW:
	if ((id &= ~P4_ID_MASK) < P4_ID_RESCODES) {
	    *w = bwsize[id][0];
	    *h = bwsize[id][1];
	}
	return type;
	/* NOTREACHED */
    default:
	return -1;
	/* NOTREACHED */
    }
    /* NOTREACHED */
}

#endif !sun2
