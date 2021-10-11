#ifndef lint
static char sccsid[] = "@(#)standalloc.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * standalloc.c
 * this file belongs in /usr/src/sys/boot/os and is used by boot.c
 *
 * ROM Monitor's routines for allocating resources needed on a temporary
 * basis (eg, for initialization or for boot drivers).
 *
 * Note, all requests are rounded up to fill a page.  This is not a
 * malloc() replacement!
 */

/* This flag causes printfs */
static int alloc_debug = 0;
#define	DPRINT	if (alloc_debug) printf

#include <machine/param.h>
#include <machine/pte.h>
#include <stand/saio.h>
#include <machine/mmu.h>
/*
 * Artifice. This should be defined in <mmu.h>.
 */
#ifndef DVMABASE
# include <machine/cpu.h>
# define	DVMABASE	(0-DVMASIZE)
#endif

/*
 * Artifice so standalone code uses same variable names as monitor's
 * for debugging.  FIXME?  Or leave this way?
 */
struct globram {
	char *g_nextrawvirt;
	char *g_nextdmaaddr;
	struct pte g_nextmainmap;
} gp[1];

/*
 * Valid, supervisor-only, memory page's map entry.
 * (To be copied to a map entry and then modified.)
 */
struct pte mainmapinit =
#if defined(sun3x)
	{0, 0, 0, 1, 0, 0, 0, RW, PTE_VALID};
#define	pg_pfnum	pte_pfn
#elif defined(sun4)
	{1, KW, 0, OBMEM, 0, 0, 0, 0};
#elif sun4c
	{1, KW, 0, OBMEM, 0, 0, 0};
#elif sun4m
	{0, 0, 0, 0, MMU_STD_SRWX, MMU_ET_PTE };
#endif

int	memory_avail;
#ifdef OPENPROMS
struct eachentry {
	caddr_t		addr;
	unsigned	size;
	};
static caddr_t	allocpool = 0;
static struct eachentry	*next_free;
#endif OPENPROMS

/*
 * Say Something Here FIXME
 */
char *
resalloc(type, bytes)
	enum RESOURCES type;
	register unsigned bytes;
{
	register char *	addr;	/* Allocated address */
	register char *	raddr;	/* Running addr in loop */
#ifdef	OPENPROMS
	extern caddr_t prom_alloc();
#endif	OPENPROMS

#ifndef sun4m
	/* Initialize if needed. */
	if (gp->g_nextrawvirt == 0) {
#ifdef	OPENPROMS
		if (prom_getversion() == 0)
#endif	OPENPROMS
			reset_alloc(memory_avail);
	}
#endif	sun4m

	DPRINT("resalloc(%x, %x) %x %x %x\n", type, bytes,
	    gp->g_nextrawvirt, gp->g_nextdmaaddr, *(int*)&(gp->g_nextmainmap));

	if (bytes == 0)
		return ((char *)0);
	bytes = (bytes + (PAGESIZE - 1)) & ~(PAGESIZE - 1);

#ifdef OPENPROMS
	if (prom_getversion() > 0) {
		if (allocpool == 0) {
			allocpool = prom_alloc(0, PAGESIZE);
			next_free = (struct eachentry *)allocpool;
			bzero(allocpool, PAGESIZE);
			DPRINT("allocpool %x\n", allocpool);
		}
		switch (type) {
		case RES_MAINMEM:
		case RES_RAWVIRT:
			addr = prom_alloc(0, bytes);
			next_free->addr = (caddr_t)addr;
			next_free->size = bytes;
			DPRINT("next_free %x addr %x size %x\n", next_free,
				next_free->addr, next_free->size);
			next_free++;
			return (addr);
		case RES_DMAMEM:
		case RES_DMAVIRT:
			addr = prom_alloc(gp->g_nextdmaaddr, bytes);
			gp->g_nextdmaaddr = addr + bytes;
			next_free->addr = (caddr_t)addr;
			next_free->size = bytes;
			DPRINT("next_free %x addr %x size %x\n", next_free,
				next_free->addr, next_free->size);
			next_free++;
			return (addr);
		default:
			return ((char *)0);
		}
	}
#endif

#ifndef sun4m
	/*
	 * Sun4m only provides OBP
	 */
	switch (type) {

	case RES_RAWVIRT:
		addr = gp->g_nextrawvirt;
		gp->g_nextrawvirt += bytes;
		return (addr);

	case RES_DMAVIRT:
		addr = gp->g_nextdmaaddr;
		gp->g_nextdmaaddr += bytes;
		return (addr);

	case RES_MAINMEM:
		addr = gp->g_nextrawvirt;
		gp->g_nextrawvirt += bytes;
		break;

	case RES_DMAMEM:
		addr = gp->g_nextdmaaddr;
		gp->g_nextdmaaddr += bytes;
		break;

	default:
		return ((char *)0);
	}

	/*
	 * Now map in main memory.
	 * Note that this loop goes backwards!!
	 */
	DPRINT("mapping to %x returning %x gp %x\n",
	    *(int*)&gp->g_nextmainmap, addr, gp);
	for (raddr = addr;
	    bytes > 0;
	    raddr += PAGESIZE, bytes -= PAGESIZE,
            gp->g_nextmainmap.pg_pfnum -= 1) {
		setpgmap(raddr, *(int *)&gp->g_nextmainmap);
		bzero((caddr_t)raddr, PAGESIZE);
	}

	return (addr);
#endif
}

#ifdef sun3
struct pte devmaps[] = {
/* MAINMEM */
	{1, KW, 0, OBMEM,   0, 0, 0},
/* OBIO */
	{1, KW, 0, OBIO,    0, 0, 0},
/* MBMEM */
	{1, KW, 0, VME_D16, 0, 0, btop(0xFF000000) },
/* MBIO */
	{1, KW, 0, VME_D16, 0, 0, btop(0xFFFF0000) },
/* VME16A16D */
	{1, KW, 0, VME_D16, 0, 0, btop(0xFFFF0000) },
/* VME16A32D */
	{1, KW, 0, VME_D32, 0, 0, btop(0xFFFF0000) },
/* VME24A16D */
	{1, KW, 0, VME_D16, 0, 0, btop(0xFF000000) },
/* VME24A32D */
	{1, KW, 0, VME_D32, 0, 0, btop(0xFF000000) },
/* VME32A16D */
	{1, KW, 0, VME_D16, 0, 0, btop(0x00000000) },
/* VME32A32D */
	{1, KW, 0, VME_D32, 0, 0, btop(0x00000000) },
};
#endif sun3

#ifdef sun3x
struct pte devmaps[] = {
/* MAINMEM */
	{0,                   0, 0, 0, 0, 0, 0, RW, PTE_VALID},
/* OBIO */
	{0,                   0, 0, 1, 0, 0, 0, RW, PTE_VALID},
/* MBMEM */
	{btop(VME24D16_BASE), 0, 0, 1, 0, 0, 0, RW, PTE_VALID},
/* MBIO */
	{btop(VME16D16_BASE), 0, 0, 1, 0, 0, 0, RW, PTE_VALID},
/* VME16A16D */
	{btop(VME16D16_BASE), 0, 0, 1, 0, 0, 0, RW, PTE_VALID},
/* VME16A32D--not supported */
	{0,                   0, 0, 1, 0, 0, 0, RW, 0},
/* VME24A16D */
	{btop(VME24D16_BASE), 0, 0, 1, 0, 0, 0, RW, PTE_VALID},
/* VME24A32D */
	{btop(VME24D32_BASE), 0, 0, 1, 0, 0, 0, RW, PTE_VALID},
/* VME32A16D--not supported */
	{0,                   0, 0, 1, 0, 0, 0, RW, 0},
/* VME32A32D */
	{btop(VME32D32_BASE), 0, 0, 1, 0, 0, 0, RW, PTE_VALID},
};
#endif sun3x

#ifdef sun4
struct pte devmaps[] = {
/* MAINMEM */
	{1, KW, 0, OBMEM,   0, 0, 0, 0},
/* OBIO */
	{1, KW, 0, OBIO,    0, 0, 0, 0},
/* MBMEM */
	{1, KW, 0, VME_D16, 0, 0, 0, btop(0xFF000000) },
/* MBIO */
	{1, KW, 0, VME_D16, 0, 0, 0, btop(0xFFFF0000) },
/* VME16A16D */
	{1, KW, 0, VME_D16, 0, 0, 0, btop(0xFFFF0000) },
/* VME16A32D */
	{1, KW, 0, VME_D32, 0, 0, 0, btop(0xFFFF0000) },
/* VME24A16D */
	{1, KW, 0, VME_D16, 0, 0, 0, btop(0xFF000000) },
/* VME24A32D */
	{1, KW, 0, VME_D32, 0, 0, 0, btop(0xFF000000) },
/* VME32A16D */
	{1, KW, 0, VME_D16, 0, 0, 0, btop(0x00000000) },
/* VME32A32D */
	{1, KW, 0, VME_D32, 0, 0, 0, btop(0x00000000) },
};
#endif sun4

#ifdef sun4c
struct pte devmaps[] = {
/* MAINMEM */
	{1, KW, 0, OBMEM, 0, 0, 0},
/* OBIO */
	{1, KW, 0, OBIO,  0, 0, 0},
};
#endif sun4c

#ifdef nodef
#ifdef sun4m
struct pte devmaps[] = {
/* MAINMEM */
	{0, 0, 0, 0, MMU_STD_SRWX, MMU_ET_PTE},
/* OBIO */
	{0, 0, 0, 0, MMU_STD_SRWX, MMU_ET_PTE},
};
#endif sun4m
#endif nodef


/*
 * devalloc() allocates virtual memory and maps it to a device
 * at a specific physical address.
 *
 * It returns the virtual address of that physical device.
 */
char *
devalloc(devtype, physaddr, bytes)
	enum MAPTYPES	devtype;
	register char  *physaddr;
	register unsigned	bytes;
{
	char		*addr;
	register char	*raddr;
	register int	pages;
	struct pte	mapper;
#ifdef	OPENPROMS
	extern caddr_t	prom_map();
#endif	OPENPROMS

	DPRINT("devalloc(%x, %x, %x) ", devtype, physaddr, bytes);

	if (bytes == 0)
		return ((char *)0);
	pages = bytes + ((int)(physaddr) & (PAGESIZE-1));

#ifdef OPENPROMS
	if (prom_getversion > 0)
		return (prom_map(0, devtype, physaddr, pages));
#endif

#ifndef sun4m
	addr = resalloc(RES_RAWVIRT, pages);
	if (addr == 0)
		return ((char *)0);

	mapper = devmaps[(int)devtype];		/* Set it up first */
	mapper.pg_pfnum += btop(physaddr);

	for (raddr = addr;
	    pages > 0;
	    raddr += PAGESIZE, pages -= PAGESIZE,
	    mapper.pg_pfnum += 1) {
		DPRINT("mapping to %x ", *(int *)&mapper);
		setpgmap(raddr, *(int *)&mapper);
	}

	DPRINT("returns roughly %x\n", addr);
	return (addr + ((int)(physaddr) & (PAGESIZE-1)));
#endif
}

#ifndef sun4m
/*
 * reset_alloc() does all the setup and all the releasing for the PROMs.
 */
reset_alloc(memsize)
	unsigned memsize;
{
	int i, addr, pmeg, lopm, pm;

#ifdef sun3
	gp->g_nextrawvirt = (char *)0x200000;
	/*
	 * The monitor only allocates as many PMEGs as there is real
	 * memory so we have to set up more PMEGs for virtual memory
	 * on machines with only 2 megabytes.
	 */
	for (i=0; i < 0x100000; i += PMGRPSIZE) {	/* 1 Meg */
		addr = (int)gp->g_nextrawvirt + i;
		pmeg = addr / PMGRPSIZE;
		setsegmap(addr, pmeg);
	}
#elif defined(sun4c)
	/*
	 * The booter loads itself at 3 Meg so we add an extra half-Meg to
	 * get to a place to allocate virtual addresses and still have pmegs.
	 * This only applies to Campus proms; OBP machines use the romvec
	 * alloc/free routines below.
	 */
	gp->g_nextrawvirt = (char *)0x380000;
#else
	/*
	 * The booter loads itself at 2 Meg so we add an extra Meg to
	 * get to a place to allocate virtual addresses and still have pmegs.
	 * This assumes that the minimum memory size for "modern" Suns is 4 Meg.
	 */
	gp->g_nextrawvirt = (char *)0x300000;
#endif sun3

#if defined(sun4c) && !defined(KADB)
	if (prom_getversion() == 0) {
		/*
		 * The monitor does not allocate PMEGs for DVMA space.
		 * Look for the lowest pmeg in use in the monitor (which
		 * assumes all other pmegs are invalid).
		 */
		lopm = PMGRP_INVALID;
		for (i = MONSTART; i < MONEND; i += PMGRPSIZE)
			if (((pm = getsegmap(i)) < lopm) && pm)
				lopm = pm;
		/*
		 * The next pmeg down is unused: claim it for DVMA.
		 */
		lopm -= 1;
		DPRINT("mapping %x to segment %x\n", DVMABASE, lopm);
		setsegmap(DVMABASE, lopm);
	}
#endif

	gp->g_nextdmaaddr = (char *)DVMABASE;
	gp->g_nextmainmap = mainmapinit;
	gp->g_nextmainmap.pg_pfnum = btop(memsize) - 1;
	DPRINT("reset_alloc(%x) mainmap %x gp %x nextmain %x\n",
	    memsize, *(int *)&mainmapinit, gp, *(int*)&gp->g_nextmainmap);
}
#endif

#ifdef OPENPROMS
our_die_routine(retaddr)
	register caddr_t retaddr;
{
	register struct eachentry *nextentry;
	extern void prom_free();

	DPRINT("Boot: freeing resources ...\n");

	if (allocpool == 0)
		return;

	for (nextentry = (struct eachentry *)allocpool;
	    nextentry->addr; nextentry++)  {
#ifdef	notdef
		printf("  Free 0x%x, Size %d\n",
		    nextentry->addr, nextentry->size);
#endif	notdef
		prom_free(nextentry->addr, nextentry->size);
	}

	prom_free(allocpool, PAGESIZE);
	return;
}
#endif OPENPROMS
