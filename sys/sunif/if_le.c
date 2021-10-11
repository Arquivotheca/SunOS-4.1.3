#ident	"@(#)if_le.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*LINTLIBRARY*/

/*
 * Am7990 (LANCE) Ethernet Device Driver.
 *
 * Copyright(c) 1990 Sun Microsystems, Inc.
 * Copyright (c) 1988 by Van Jacobson.  Adapted from an earlier
 * program of the same name by Sun Microsystems, Inc.
 */

#include "le.h"

#if NLE > 0

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/map.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/debug.h>

#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <sundev/mbvar.h>

#include <sunif/if_lereg.h>
#include <sunif/if_levar.h>

#include <machine/cpu.h>
#include <machine/psl.h>

#ifdef	sun4c
#include <sys/mman.h>
#include <vm/page.h>
#include <machine/pte.h>
#endif	sun4c

#ifdef	OPENPROMS
#include <sun/autoconf.h>
#include <vm/hat.h>
#include <machine/mmu.h>
#endif	OPENPROMS

extern	char	DVMA[];

#ifdef	OPENPROMS

int	leidentify(), leattach(), leintr();
struct	dev_ops le_ops = {
	1,
	leidentify,
	leattach,
};

#else	OPENPROMS

int	leprobe(), leattach(), leintr();
struct	mb_driver ledriver = {
	leprobe, 0, leattach, 0, 0, leintr,
	sizeof (struct le_device), "le", leinfo, 0, 0, MDR_DMA,
};

#endif	OPENPROMS

#define		MAXLE	10

int	leinit(), leioctl(), leoutput(), lereset();

struct mbuf *copy_to_mbufs();
struct mbuf *mclgetx();
caddr_t	lealloctbuf();
struct	leops	*leopsfind();
int	ledog();

/*
 * Patchable debug variable.
 * Set this to nonzero to enable *all* error messages.
 */
static	int	ledebug = 0;

/*
 * Platforms which use the S4DMA (4/60, 4/65, 4/20, 3/80,
 * and 4/3XX) may have LANCE bus latency problems which manifest
 * as frequent xmit memory underflow (uflo).
 * Waiting for the upper layer protocol to timeout and
 * retransmit the packet can mean a big performance hit.
 */
static	int	leretransmit = 1;

/*
 * Patchable "pullup" values.
 * Controls how many bytes to ensure in tmd0 and tmd[1-n].
 */
int	lepullup0 = 102;
int	lepullup1 = 20;

/*
 * Default programmed i/o copy size and alignment mask values.
 */
int	leburstsize = 8;	/* sizeof (doubleword) */
u_long	leburstmask = (~7);	/* alignment for leburstsize */

/*
 * Size of transmit buffer area used when xmit buffer is
 * not chip addressable.
 */
u_int	letxsize = 64 * 1024;

/*
 * Patchable auto-select flags.  Notice that only le0 will use them.
 */
static	int	le_tp_aui_select, le_tp_aui;

/*
 * LANCE chips support only 24bits (16M) of address space
 * and we hardwire the high order byte to 0xFF .
 */
#define	TOP16M	(caddr_t)0xff000000

/*
 * Allocate an array of "number" structures
 * of type "structure" in kernel memory.
 */
#define	getstruct(structure, number)   \
	((structure *) kernelmap_zalloc(\
		(u_int) (sizeof (structure) * (number)), KMEM_SLEEP))

/*
 * Return the address of an adjacent descriptor in the given ring.
 */
#define	next_rmd(es, rmdp)	((rmdp) == (es)->es_rdrend		\
					? (es)->es_rdrp : ((rmdp) + 1))
#define	next_tmd(es, tmdp)	((tmdp) == (es)->es_tdrend		\
					? (es)->es_tdrp : ((tmdp) + 1))
#define	prev_tmd(es, tmdp)	((tmdp) == (es)->es_tdrp		\
					? (es)->es_tdrend : ((tmdp) - 1))

/*
 * Link the buffer *bp into the list rooted at lp.
 */
#define	le_linkin(lp, bp) \
	(bp)->lb_next = (lp), \
	(lp) = (bp)

/*
 * Take a receive buffer and put it on the free list.
 */
#define	free_rbuf(es, rb)	le_linkin(es->es_rbuf_free, rb)

/*
 * Convert kernel virtual address into an
 * address appropriate for chip.
 */
#define	LEADDR(es, a) \
	(((es)->es_flags & LE_IOADDR) ? \
	((es)->es_iobase + ((a) - (es)->es_membase)) : (a))

/*
 * Sanity timer goes off every LEWATCHDOG seconds
 * and calls ledog() for chip sanity check.
 */
#define	LEWATCHDOG	60

#ifdef	OPENPROMS

/*
 * Identify device
 */

static int
leidentify(name)
	char *name;
{
	if (strcmp(name, "le") == 0)
		return (1);
	else
		return (0);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */

static int
leattach(dev)
	register struct dev_info *dev;
{
	struct le_softc *es;
	struct le_device *le;
	int unit;
	static int curle = 0;
#ifdef	sun4m
	char	*cable_select;
#endif	sun4m

	le_units++;	/* count the number of interfaces */

	/*
	 * if le_softc is null, than this is the first time thru..
	 */

	if (le_softc == NULL) {
		le_softc = getstruct(struct le_softc, MAXLE);
		leinfo = getstruct(struct le_info, MAXLE);
		if (le_softc == 0 || leinfo == 0) {
			printf("le: no space for structures\n");
			return (-1);
		}
	}

#ifdef	sun4m
	/*
	 * Bcopy hardware?
	 */
	{
		extern	struct	modinfo	mod_info[];
		if (mod_info[0].bcopy) {
			leburstsize = 32;
			leburstmask = ~31;
		}
	}
#endif	sun4m

	unit = curle++;
	leinfo[unit].le_dev = dev;
	es = &le_softc[unit];
	dev->devi_data = (caddr_t)es;
	dev->devi_unit = unit;

	/* map in the device registers */
	if (dev->devi_nreg == 1) {
		/*
		 * Check to make sure the ethernet card is not plugged
		 * into a slave-only SBus slot.
		 */
		int slot;

		slot = slaveslot(dev->devi_reg->reg_addr);
		if (slot >= 0) {
			printf("le%d: not used - SBus slot %d is slave-only\n",
				unit, slot);
			return (-1);
		}

		le = leinfo[unit].le_addr = (struct le_device *) map_regs(
		    dev->devi_reg->reg_addr,
		    dev->devi_reg->reg_size,
		    dev->devi_reg->reg_bustype);
	} else {
		printf("le%d: warning: bad register specification\n", unit);
		return (-1);
	}

	/* Reset the chip. */
	le->le_rap = LE_CSR0;
	le->le_csr = LE_STOP;

	(void) localetheraddr((struct ether_addr *)NULL, &es->es_enaddr);

	/* Do hardware-independent attach stuff. */
	ether_attach(&es->es_if, unit, "le",
		leinit, leioctl, leoutput, lereset);
	/*
	 * attach interrupts and dma vectors
	 */
	if (dev->devi_nintr) {
		es->es_ipl = dev->devi_intr->int_pri;
#ifdef	INTLEVEL_MASK
		es->es_ipl &= INTLEVEL_MASK;
#endif	INTLEVEL_MASK
		addintr(dev->devi_intr->int_pri, leintr, dev->devi_name, unit);
		adddma(dev->devi_intr->int_pri);
	}

#ifdef	sun4m
	/*
	 * Twisted-Pair/AUI selection:
	 * Look for "cable-selection" property on our parent node.
	 * It takes on three string values:  "aui", "tpe" or "unknown".
	 * If the property doesn't exist or has the value "unknown", then enable
	 * software TP/AUI selection mode.  Otherwise, set it to aui or tp.
	 */
	cable_select = getlongprop(dev->devi_parent->devi_nodeid,
		"cable-selection");
	if (cable_select) {
		if (ledebug)
			ether_error(&es->es_if, "cable-selection:  %s\n",
				cable_select);
		if (strcmp(cable_select, "tpe") == 0) {
			if (ledebug)
				ether_error(&es->es_if,
					"force twisted-pair\n");
			le_tp_aui = 1;
		} else if (strcmp(cable_select, "aui") == 0) {
			if (ledebug)
				ether_error(&es->es_if,
					"force aui\n");
			le_tp_aui = 0;
		} else {
			le_tp_aui_select = 1;
			le_tp_aui = 0;
		}
	} else {
		le_tp_aui_select = 1;
		le_tp_aui = 0;
	}
#endif	/* sun4m */

	/*
	 * tell the world we're here
	 */
	report_dev(dev);
	return (0);
}

#else	OPENPROMS

/*
 * Probe for device.
 */
leprobe(reg)
	caddr_t reg;
{
	register struct le_device *le = (struct le_device *)reg;

#if defined(sun4) || defined(sun3x)
	if (poke((short *)&le->le_rdp, 0))	/* FIXME - need better test */
#else
	if (pokec((char *)&le->le_rdp, 0))	/* FIXME - need better test */
#endif sun4 || sun3x
		return (0);
	return (sizeof (struct le_device));
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
leattach(md)
	struct mb_device *md;
{
	struct le_softc *es;

	/*
	 * if le_softc is null, than this is the first time thru..
	 */
	if (le_softc == NULL)
		if ((le_softc = getstruct(struct le_softc, le_units)) == NULL)
			printf("le:  no space for structures\n");

	es = &le_softc[md->md_unit];

	/* Reset the chip. */
	((struct le_device *)md->md_addr)->le_rap = LE_CSR0;
	((struct le_device *)md->md_addr)->le_csr = LE_STOP;

	es->es_ipl = spltoipl(pritospl(md->md_intpri));

	(void) localetheraddr((struct ether_addr *)NULL, &es->es_enaddr);

	/* Do hardware-independent attach stuff. */
	ether_attach(&es->es_if, md->md_unit, "le",
		    leinit, leioctl, leoutput, lereset);
}
#endif	OPENPROMS

/*
 * Allocate:
 *   - transmit message descriptor ring
 *   - receive message descriptor ring
 *   - chip initialization block
 *   - fixed receive buffer freelist
 *   - fixed transmit buffer area
 *	 - array of transmit mbuf pointers
 *	 - array of receive le_buf pointers
 *
 * It must be possible to call this routine repeatedly with
 * no ill effects.
 */
leallocthings(es)
register struct le_softc *es;
{
	int	nif;
	u_long	a;

	if (es->es_ib)	/* return if resources already allocated */
		return;

	/*
	 * Allocate device-specific things in host memory for
	 * DVMA-masters and in device memory for non-DVMA
	 * (slave-only) devices.
	 */
	if (LEPIO(es)) {	/* a slave-only interface */
		/* Initialize the numbers of descriptors and buffers */
		es->es_ntmdp2 = 7;	/* 2^7 == 128 */
		es->es_nrmdp2 = 5;	/* 2^5 == 32 */
		es->es_ntmds = 1 << es->es_ntmdp2;
		es->es_nrmds = 1 << es->es_nrmdp2;
		if (es->es_memsize <= (128*1024))
			es->es_nrbufs = 48;
		else
			es->es_nrbufs = 88;

		a = (u_long) es->es_membase;
		a = roundup(a, 8);

		/* Allocate the chip initialization block */
		es->es_ib = (struct le_init_block *) a;
		a += sizeof (struct le_init_block);
		a = roundup(a, 8);

		/* Allocate the message descriptor rings */
		es->es_rdrp = (struct le_md *) a;
		a += es->es_nrmds * sizeof (struct le_md);
		a = roundup(a, 8);
		es->es_tdrp = (struct le_md *) a;
		a += es->es_ntmds * sizeof (struct le_md);
		a = roundup(a, 8);

		/* Allocate receive buffers burstsize-aligned */
		es->es_rbufs = (struct le_buf *) a;
		a += es->es_nrbufs * sizeof (struct le_buf);
		a = roundup(a, leburstsize);

		/* The rest is turned into a transmit area */
		es->es_tbase = (caddr_t) a;
		es->es_tlimit = es->es_membase + es->es_memsize;
	} else {	/* a DVMA master interface */

		/* Initialize the numbers of descriptors and buffers */
		nif = ifcount();
		es->es_ntmdp2 = (nif > 1)? le_high_ntmdp2: le_low_ntmdp2;
		es->es_nrmdp2 = (nif > 1)? le_high_nrmdp2: le_low_nrmdp2;
		es->es_ntmds = 1 << es->es_ntmdp2;
		es->es_nrmds = 1 << es->es_nrmdp2;
		es->es_nrbufs = (nif > 1)? le_high_nrbufs: le_low_nrbufs;

		/* Allocate the chip initialization block */
		es->es_ib = getstruct(struct le_init_block, 1);

#ifdef	sun4c
		/*
		 * Allocate the message descriptor rings. They are
		 * allocated out of the IOPBMAP to allow transparent
		 * cache consistency to occur.
		 *
		 *	Force 8-byte alignment by allocating an extra
		 *	one and then rounding the starting address.
		 *	This code depends on message descriptors being
		 *	>= 8 bytes long.  (Bletch!)
		 */
		a = (u_long) IOPBALLOC(sizeof (struct le_md) *
			(es->es_nrmds + 1));
		if (a == NULL) {
			printf("le%d: no space for receive descriptors",
				es-le_softc);
			panic("leallocthings");
			/*NOTREACHED*/
		}
		es->es_rdrp = ((struct le_md *) (a & ~7)) + 1;
		a = (u_long) IOPBALLOC(sizeof (struct le_md) *
			(es->es_ntmds + 1));
		if (a == NULL) {
			printf("le%d: no space for transmit descriptors",
				es-le_softc);
			panic("leallocthings");
			/*NOTREACHED*/
		}
		es->es_tdrp = ((struct le_md *) (a & ~7)) + 1;
#else	sun4c
		/*
		 * Allocate the message descriptor rings.
		 * We assume 8-byte alignment, which is guaranteed
		 * by kmem_alloc() in 4.1.
		 * This code depends on message descriptors being
		 * >= 8 bytes long.  (Bletch!)
		 */
		es->es_rdrp = getstruct(struct le_md, es->es_nrmds);
		es->es_tdrp = getstruct(struct le_md, es->es_ntmds);
		ASSERT(((u_long)es->es_rdrp & 7) == 0 &&
				((u_long)es->es_tdrp & 7) == 0);
#endif	sun4c

		/* Allocate receive buffers. */
		es->es_rbufs = getstruct(struct le_buf, es->es_nrbufs);

		/*
		 * Allocate a chunk of memory in the top 16M of kernel
		 * virtual space and copy the outbound mbufs here if/when
		 * they are not chip addressible (< TOP16M).
		 */
		es->es_tbase = kernelmap_zalloc(letxsize, KMEM_SLEEP);
		if (es->es_tbase == NULL)
			panic("no kernelmap space for transmit bufs\n");
		es->es_tlimit =  es->es_tbase + letxsize;
	}

	/*
	 * Remember address of last descriptor in the ring for
	 * ease of bumping pointers around the ring.
	 */
	es->es_rdrend = &((es->es_rdrp)[es->es_nrmds-1]);
	es->es_tdrend = &((es->es_tdrp)[es->es_ntmds-1]);

	/*
	 * Allocate array of transmit mbuf chain pointers
	 * and array of receive le_buf pointers.
	 */
	es->es_tbuf = getstruct(caddr_t, es->es_ntmds);
	es->es_tmbuf = getstruct(struct mbuf*, es->es_ntmds);
	es->es_rbufc = getstruct(struct le_buf *, es->es_nrmds);

	/*
	 * Zero the data structures that need it.
	 */
	bzero((caddr_t) es->es_rbufs,
		sizeof (struct le_buf) * (u_int) es->es_nrbufs);
	bzero((caddr_t) es->es_tbuf,
		sizeof (caddr_t) * (u_int) es->es_ntmds);
	bzero((caddr_t) es->es_tmbuf,
		sizeof (struct mbuf*) * (u_int) es->es_ntmds);
	bzero((caddr_t) es->es_rbufc,
		sizeof (struct le_buf *) * (u_int) es->es_nrmds);
}

/*
 * Reset of interface after system reset.
 */
lereset(unit)
	int unit;
{
	if (unit >= le_units || (! LE_ALIVE(unit))) {
		return;
	}
	leinit(unit);
}

/*
 * Initialization of interface; clear recorded pending
 * operations.
 */
leinit(unit)
	int unit;
{
	register struct le_softc *es = &le_softc[unit];
	register struct le_device *le;
	register struct le_init_block *ib;
	register int s, mc;
	struct ifnet *ifp = &es->es_if;
#ifdef	OPENPROMS
	struct	leops	*lop;
#endif	OPENPROMS
	int	i;
	static	once = 1;

	s = splimp();

	untimeout(ledog, (caddr_t) es);

	/*
	 * Freeze the chip before doing anything else.
	 */
	le = LE_ADDR(unit);
	le->le_rap = LE_CSR0;
	le->le_csr = LE_STOP;

	if ((ifp->if_flags & IFF_UP) == 0) {
		(void) splx(s);
		return;
	}

#ifdef	OPENPROMS
	lop = (struct leops *) leopsfind(leinfo[es - le_softc].le_dev);

#ifndef	sun4c
	/*
	 * S4DMA chip based SBus LANCE cards are not supported in non-Sun4c
	 * architectures due to higher SBus latencies and insufficient
	 * local buffering.  Check for existance of lebuf or ledma.
	 */
	if (lop == NULL) {
		printf(
		"le%d:  not used - unsupported SBus card for this machine!\n",
			unit);
		return;
	}
#endif

	if (lop) {
		if (lop->lo_flags & LO_PIO)
			es->es_flags |= LE_PIO;
		if (lop->lo_flags & LO_IOADDR)
			es->es_flags |= LE_IOADDR;
		es->es_membase = lop->lo_membase;
		es->es_iobase = lop->lo_iobase;
		es->es_memsize = lop->lo_size;
		es->es_leinit = lop->lo_init;
		es->es_leintr = lop->lo_intr;
		es->es_learg = lop->lo_arg;
	}
#endif	OPENPROMS

	/*
	 * Device-specific initialization.
	 */
	if (es->es_leinit)
		(*es->es_leinit)(es->es_learg, le_tp_aui, 1);

	/*
	 * Insure that resources are available.
	 *	(The first time through, this will allocate them.)
	 */
	leallocthings(es);

	/*
	 * Reset message descriptors.
	 */
	es->es_his_rmd = es->es_rdrp;
	es->es_cur_tmd = es->es_tdrp;
	es->es_nxt_tmd = es->es_tdrp;

	/* Initialize buffer allocation information. */

	/*
	 * Set up the receive buffer free list.
	 *	All receive buffers but those that are currently
	 *	loaned out go on it.
	 */
	es->es_rbuf_free = (struct le_buf *)0;
	for (i = 0; i < es->es_nrbufs; i++) {
		register struct le_buf	*rb = &es->es_rbufs[i];

		if (rb->lb_loaned)
			continue;
		free_rbuf(es, rb);
	}

	es->es_trd = es->es_twr = es->es_tbase;

	/* Construct the initialization block */
	ib = es->es_ib;
	bzero((caddr_t)ib, (u_int) sizeof (struct le_init_block));

	/*
	 * Mode word 0 should be all zeroes except
	 * possibly for the promiscuous mode bit.
	 */
	if (ifp->if_flags & IFF_PROMISC)
		ib->ib_prom = 1;

	ib->ib_padr[0] = es->es_enaddr.ether_addr_octet[1];
	ib->ib_padr[1] = es->es_enaddr.ether_addr_octet[0];
	ib->ib_padr[2] = es->es_enaddr.ether_addr_octet[3];
	ib->ib_padr[3] = es->es_enaddr.ether_addr_octet[2];
	ib->ib_padr[4] = es->es_enaddr.ether_addr_octet[5];
	ib->ib_padr[5] = es->es_enaddr.ether_addr_octet[4];

	/*
	 * Set up multicast address filter by passing all multicast
	 * addresses through a crc generator, and then using the
	 * high order 6 bits as a index into the 64 bit logical
	 * address filter. The high order two bits select the word,
	 * while the rest of the bits select the bit within the word.
	 */
	for (mc = 0; mc < es->es_nmcaddr; mc++) {
		register u_char *cp;
		register u_long crc;
		register u_long c;
		register int len;

		cp = (unsigned char *)&(es->es_mcaddr[mc].mc_enaddr);
		c = *cp;
		crc = 0xffffffff;
		len = 6;
		while (len-- > 0) {
			c = *cp;
			for (i = 0; i < 8; i++) {
				if ((c & 0x01) ^ (crc & 0x01)) {
					crc >>= 1;
					crc = crc ^ 0xedb88320; /* polynomial */
				} else
					crc >>= 1;
				c >>= 1;
			}
			cp++;
		}
		/* Just want the 6 most significant bits. */
		crc = crc >> 26;

		/* Turn on the corresponding bit in the address filter. */
		ib->ib_ladrf[crc >> 4] |= 1 << (crc & 0xf);

	}

	ib->ib_rdrp.drp_laddr = (long)LEADDR(es, (caddr_t) es->es_rdrp);
	ib->ib_rdrp.drp_haddr = (long)LEADDR(es, (caddr_t) es->es_rdrp) >> 16;
	ib->ib_rdrp.drp_len   = (long)es->es_nrmdp2;
	ib->ib_tdrp.drp_laddr = (long)LEADDR(es, (caddr_t) es->es_tdrp);
	ib->ib_tdrp.drp_haddr = (long)LEADDR(es, (caddr_t) es->es_tdrp) >> 16;
	ib->ib_tdrp.drp_len   = (long)es->es_ntmdp2;

	/* Clear all the descriptors */
	bzero((caddr_t)es->es_rdrp,
			(u_int) (es->es_nrmds * sizeof (struct le_md)));
	bzero((caddr_t)es->es_tdrp,
			(u_int) (es->es_ntmds * sizeof (struct le_md)));

	/* Hang out the receive buffers. */
	{
		register struct le_buf *rb;

		while (rb = es->es_rbuf_free) {
			register struct le_md *rmd = es->es_his_rmd;

			es->es_rbuf_free = rb->lb_next;
			install_buf_in_rmd(es, rb, rmd);
			rmd->lmd_flags = LMD_OWN;
			es->es_his_rmd = next_rmd(es, rmd);
			if (es->es_his_rmd == es->es_rdrp)
				break;
		}
		/*
		 * Verify that all receive ring descriptor slots
		 * were filled.
		 */
		if (es->es_his_rmd != es->es_rdrp) {
			identify(&es->es_if);
			panic("leinitrbufs");
		}
	}

	/* Give the init block to the chip */
	le->le_rap = LE_CSR1;	/* select the low address register */
	le->le_rdp = (long)LEADDR(es, (caddr_t) ib) & 0xffff;

	le->le_rap = LE_CSR2;	/* select the high address register */
	le->le_rdp = ((long)LEADDR(es, (caddr_t) ib) >> 16) & 0xff;

	le->le_rap = LE_CSR3;	/* Bus Master control register */
#ifdef	OPENPROMS
	le->le_rdp = getprop(leinfo[unit].le_dev->devi_nodeid,
		"busmaster-regval", LE_BSWP | LE_ACON | LE_BCON);
#elif	defined(sun4)
	if (cpu == CPU_SUN4_330)
		le->le_rdp = LE_BSWP | LE_ACON | LE_BCON;
	else
		le->le_rdp = LE_BSWP;
#elif defined(sun3x)
	if (cpu == CPU_SUN3X_80)
		le->le_rdp = LE_BSWP | LE_ACON | LE_BCON;
	else
		le->le_rdp = LE_BSWP;
#else
	le->le_rdp = LE_BSWP;
#endif

	le->le_rap = LE_CSR0;	/* main control/status register */
	le->le_csr = LE_INIT;

	/*
	 * Allow 10 ms for the chip to complete initialization.
	 */
	CDELAY((le->le_csr & LE_IDON), 10000);	/* in us */
	if (!(le->le_csr & LE_IDON)) {
		identify(&es->es_if);
		panic("chip didn't initialize");
	}
	le->le_csr = LE_IDON;		/* Clear this bit */

	/*
	 * Clear software record of pending transmit operations.
	 */
	for (i = 0; i < es->es_ntmds; i++) {
		if (es->es_tmbuf[i])
			m_freem(es->es_tmbuf[i]);
		es->es_tmbuf[i] = NULL;
		es->es_tbuf[i] = NULL;
	}

	if (le_tp_aui_select && once && es->es_leinit) {
		leautoselect(unit);
		once = 0;
	}

	/* (Re)start the chip. */
	le->le_csr = LE_STRT | LE_INEA;

#ifdef sun4m
	if (le->le_csr)		/* readback */
		;
#endif

	ifp->if_flags |= IFF_UP|IFF_RUNNING;
	timeout(ledog, (caddr_t)es, LEWATCHDOG*hz);
	if (ifp->if_snd.ifq_head)
		lestart(unit);
	es->es_init++;

	(void) splx(s);
}

leautoselect(unit)
	int	unit;
{
	register struct le_softc *es = &le_softc[unit];
	register struct le_device *le = LE_ADDR(unit);
	struct le_md *t;
	caddr_t tbuf;
	struct ether_header *ehp;

	/*
	 * Xmit an ethernet header only.  The destination is
	 * the same as the source so that we do not bother
	 * other stations.
	 */
	tbuf = lealloctbuf(es, sizeof (struct ether_header));
	ehp = (struct ether_header *)tbuf;
	ether_copy(&es->es_enaddr, &ehp->ether_shost);
	ether_copy(&es->es_enaddr, &ehp->ether_dhost);
	ehp->ether_type = 0;

	/*
	 * Initialize the TMD.
	 */
	t = es->es_nxt_tmd;
	t->lmd_ladr = (u_short) tbuf;
	t->lmd_hadr = (int) tbuf >> 16;
	t->lmd_bcnt = 0 - sizeof (struct ether_header);
	t->lmd_flags3 = 0;
	t->lmd_flags = LMD_STP | LMD_ENP | LMD_OWN;

	/* Do not enable interrupt since we are polling. */
	le->le_csr = LE_STRT | LE_TDMD;

	while (!(le->le_csr & LE_TINT))
		;

	/* If we get loss-of-carrier, use TPE from now on. */
	identify(&es->es_if);
	if (t->lmd_flags3 & TMD_LCAR) {
		(*es->es_leinit)(es->es_learg, 1, 0);
		le_tp_aui = 1;
		printf("Twisted Pair Ethernet\n");
	} else
		printf("AUI Ethernet\n");

	es->es_trd = es->es_twr;
	t->lmd_ladr = 0;
	t->lmd_hadr = 0;
	t->lmd_bcnt = 0;
	t->lmd_flags = 0;
	t->lmd_flags3 = 0;

	le->le_csr = LE_TINT;
	if (le->le_csr)		/* readback */
		;
	le_tp_aui_select = 0;
}

/*
 * Watchdog timer routine invoked every LEWATCHDOG seconds.
 */
ledog(es)
	register struct le_softc	*es;
{
	register int unit;

	if ((es->es_if.if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		return;

	/*
	 * Reset the chip on certain error conditions.
	 */
	unit = es - le_softc;
	if (LE_ALIVE(unit)) {
		if (le_chip_error(es, LE_ADDR(unit))) {
			es->es_dogreset++;
			return;
		}
	}
	timeout(ledog, (caddr_t) es, LEWATCHDOG*hz);
}


/*
 * Set the receive descriptor rmd to refer to the buffer rb.
 *	Note that we don't give the descriptor/buffer pair
 *	back to the chip here, since our caller may not be
 *	ready to give it up yet.
 */
install_buf_in_rmd(es, rb, rmd)
	struct	le_softc	*es;
	struct le_buf *rb;
	register struct le_md *rmd;
{
	/*
	 * Careful here -- the chip considers the buffer to start
	 * with an ether header, so we really have to point it at
	 * the lb_ehdr field of our buffer structure.
	 */
	register u_char *buffer = (u_char *) &rb->lb_ehdr;

	/* convert to address appropriate for the chip */
	buffer = (u_char *) LEADDR(es, (caddr_t) buffer);

	rmd->lmd_ladr = (u_short) buffer;
	rmd->lmd_hadr = (long)buffer >> 16;
	rmd->lmd_bcnt = -MAXBUF;
	rmd->lmd_mcnt = 0;

	es->es_rbufc[rmd - es->es_rdrp] = rb;
}

/*
 * Repossess a loaned-out receive buffer.
 *	Called from within MFREE by the loanee when disposing
 *	of the cluster-type mbuf wrapped around the buffer.
 *
 *	Assumes called with lists locked.
 */
leclfree(lbp)
	register struct le_buf	*lbp;
{
	register struct le_softc	*es;

	if (!lbp)
		panic("leclfree");

	es = lbp->lb_es;
	lbp->lb_es = NULL;
	if (!es) {
		(void) printf("lbp: %x\n", lbp);
		panic("leclfree2");
	}

	lbp->lb_loaned = 0;
	free_rbuf(es, lbp);
}

static char outoftmds[] = "out of tmds - packet dropped\n";
static char outofmbufs[] = "out of mbufs - packet dropped\n";
static char	outoftbufs[] = "out of transmit buffers - packet dropped\n";

/*
 * Start or restart output on interface.
 * Pull packets off the interface queue.
 * wrap each in transmit message descriptors,
 * and give to the chip.
 * Fully parallel host and LANCE chip processing here!
 * Note:  Out of tmds's causes outgoing packets
 * to be discarded rather than requeued
 * and the snd queue is flow controlled *only*
 * in the very rare case where data has
 * to be copied into the static transmit buffer.
 */
lestart(unit)
	int	unit;
{
	register struct mbuf *m, *m0;
	register struct le_md *t, *t0;
	register struct le_softc *es = &le_softc[unit];
	caddr_t	tbuf;
	caddr_t	savetwr;
	int	len;
	extern	struct mbuf *ether_pullup();
	int	mustcopy;

	/*
	 * Loop until there are no more packets on the if_snd queue.
	 */
	while (es->es_if.if_snd.ifq_head) {
		IF_DEQUEUE(&es->es_if.if_snd, m);

		/*
		 * Set the ethernet source address.
		 */
		ether_copy(&es->es_enaddr,
			&(mtod(m, struct ether_header *)->ether_shost));

		/*
		 * Force the first buffer of each outgoing packet
		 * to be at least the minimum size the chip requires.
		 */
		if (m->m_len < lepullup0)
			if ((m = ether_pullup(m, lepullup0)) == NULL) {
				ether_error(&es->es_if, outofmbufs);
				continue;
			}

		/*
		 * If the resulting length is still small, we know that
		 * the packet is entirely contained in the pulled-up
		 * mbuf.  In this case, we must insure that the packet's
		 * length is at least the Ethernet minimum transmission
		 * unit.  We can reach in and adjust the mbuf's size
		 * with impunity because: ether_pullup has returned us
		 * an mbuf whose data leaves room for m_len to
		 * be extended to ETHER_MIN_TU.
		 */
		if (m->m_len < ETHER_MIN_TU)
			m->m_len = ETHER_MIN_TU;

		/*
		 * Ugly hack:  set "mustcopy" if any of the mbufs in the chain
		 * are not chip addressible and the entire mbuf chain
		 * must be copied.
		 */
		mustcopy = 0;
		if (LEPIO(es))
			mustcopy = 1;
		else
			for (m0 = m; m0; m0 = m0->m_next)
				if (mtod(m0, caddr_t) < TOP16M)
					mustcopy = 1;

		/*
		 * Wrap a transmit descriptor around each of the
		 * packet's mbufs.  Check for resource exhaustion
		 * and handle it by tossing the packet.
		 */
		m0 = m;
		t0 = t = es->es_nxt_tmd;
		savetwr = es->es_twr;
		do {
			/*
			 * Pull each mbuf up to a "reasonable" size
			 * to prevent UFLO.  The value of lepullup1
			 * was pulled out of a hat.
			 */
			if (m->m_next && (m->m_next->m_len < lepullup1)) {
				m->m_next = ether_pullup(m->m_next, lepullup1);
				if (!m->m_next)
					ether_error(&es->es_if, outofmbufs);
			}

			/*
			 * Set tbuf pointer and buffer length for later.
			 */
			tbuf = mtod(m, caddr_t);
			len = m->m_len;

			/*
			 * Skip any zero-length mbufs to avoid BABL .
			 */
			if (len == 0)
				continue;

			/*
			 * For programmed i/o devices, copy the mbuf.
			 * Otherwise, just point the tmd at the buffer.
			 */
			if (mustcopy) {
				caddr_t	base;
				int	nbytes;
				int	offset;

				base = (caddr_t) ((u_long) tbuf & leburstmask);
				offset = tbuf - base;

				nbytes = (tbuf + len) - base;
				nbytes = roundup(nbytes, leburstsize);

				if ((tbuf = lealloctbuf(es, nbytes)) == NULL) {
					m_freem(m0);
					IF_DROP(&es->es_if.if_snd);
					if (ledebug)
						ether_error(&es->es_if,
							outoftbufs);
					es->es_no_tbufs++;
					es->es_twr = savetwr;
					return;
				}
				es->es_outcpy++;
				bcopy(base, tbuf, (u_int) nbytes);
				tbuf += offset;
			}

			tbuf = (caddr_t) LEADDR(es, (caddr_t) tbuf);

			/*
			 * Initialize the TMD.
			 */
			t->lmd_ladr = (u_short) tbuf;
			t->lmd_hadr = (int) tbuf >> 16;
			t->lmd_bcnt = -len;
			t->lmd_flags3 = 0;
			t->lmd_flags = 0;

			/*
			 * Point to next free tmd.
			 */
			t = next_tmd(es, t);
			if (t == es->es_cur_tmd) {	/* out of tmds */
				m_freem(m0);
				IF_DROP(&es->es_if.if_snd);
				if (ledebug)
					ether_error(&es->es_if, outoftmds);
				es->es_no_tmds++;
				es->es_twr = savetwr;
				return;
			}
		} while (m = m->m_next);

		/*
		 * t points to the next free tmd.
		 */
		es->es_nxt_tmd = t;

		t = prev_tmd(es, t);

		/*
		 * Save current transmit area write pointer or mbuf chain
		 * for later reference in leintr().
		 */
		if (mustcopy)
			es->es_tbuf[t - es->es_tdrp] = es->es_twr;
		else
			es->es_tmbuf[t - es->es_tdrp] =  m0;

		/*
		 * Fire off the packet by giving each of its associated
		 * descriptors to the LANCE chip, setting the start-
		 * and end-of-packet flags as well.
		 * We give the descriptors back in reverse order
		 * to prevent race conditions with the chip.
		 */
		t0->lmd_flags = LMD_STP;
		t->lmd_flags |= LMD_ENP;
		for (; t != t0; t = prev_tmd(es, t))
			t->lmd_flags |= LMD_OWN;
		t->lmd_flags |= LMD_OWN;

		(LE_ADDR(unit))->le_csr = LE_TDMD | LE_INEA;

		/*
		 * Take the time to free the mbuf chain
		 * after getting the chip started transmitting..
		 */
		if (mustcopy)
			m_freem(m0);
	}
}

/*
 * Allocate an nbyte contiguous buffer within the "transmit area"
 * bounded by [es_tbase, es_tlimit].  Return NULL on failure.
 * Since we can treat the transmit area as a FIFO this is simplier
 * and avoids the fragmentation problems of using rmalloc().
 */
caddr_t
lealloctbuf(es, nbytes)
register	struct	le_softc	*es;
int	nbytes;
{
	register	caddr_t	base, limit, rd, wr;

	/* For compilers that don't optimize pointer references ... */
	rd = es->es_trd;
	wr = es->es_twr;
	base = es->es_tbase;
	limit = es->es_tlimit;

	if (rd <= wr) {
		if ((wr + nbytes) < limit) {
			es->es_twr = wr + nbytes;
			return (wr);
		}
		if ((base + nbytes) < rd) {	/* wrap-around case */
			es->es_twr = base + nbytes;
			return (base);
		}
	} else if ((wr + nbytes) < rd) {
		es->es_twr = wr + nbytes;
		return (wr);
	}

	return (NULL);
}

/*
 * Ethernet interface interrupt.
 */
int	leservice[16] = { 0, };

leintr()
{
	register struct le_softc *es;
	register struct le_device *le;
	register struct le_md *lmd;
	register u_short	csr0;
	int unit;
	int uflo = 0;
	int serviced;
	u_short	flags;
	int	s;
	int	intlevel;
	int	devlevel;

	s = splimp();
	intlevel = spltoipl(s);

	/*
	 * Loop through all active interfaces.
#if 0
	 * Due to devices that interrupt at different levels,
	 * it's important to return after processing at most
	 * one interface rather than going on and processing
	 * all device interrupts.
#endif
	 */
	es = &le_softc[0];
	for (unit = 0; unit < le_units; unit++, es++) {
		/*
		 * Skip to the next device if this unit
		 * is not there.
		 */
		if (!LE_ALIVE(unit))
			continue;

		devlevel = es->es_ipl;
		le = LE_ADDR(unit);

		/*
		 * Read CSR0 into a local variable to cut down the number
		 * of Lance slave accesses.  Accessing a chip register
		 * while the chip is transmitting causes it to stop filling
		 * its fifo to satisfy the slave access which plays a role
		 * in UFLO.
		 */
		csr0 = le->le_csr;

		if (!(csr0 & LE_INTR)) {
			if (es->es_leintr != NULL &&
				(*es->es_leintr)(es->es_learg)) {
				if (ledebug)
					printf("leintr:  reset\n");
				lereset(unit);
			}
			continue;	/* not interrupting */
		}

		if ((es->es_if.if_flags & (IFF_UP|IFF_RUNNING)) !=
				(IFF_UP|IFF_RUNNING)) {
			lereset(unit);
			leservice[devlevel] ++;
			continue;
		}

		/* Keep statistics for lack of heartbeat */
		if (csr0 & LE_CERR) {
			le->le_csr = LE_CERR | LE_INEA;
			es->es_noheartbeat++;
		}

		/*
		 * The chip's internal (externally invisble) pointer
		 * always points to the rmd just after the last
		 * one that the software has taken from the chip.
		 * Es_his_rmd always points to the rmd just after the
		 * last one that was given to the chip.
		 */

		/*
		 * It is possible to omit the RINT test and to
		 * just check for the OWN bit clear in the next
		 * rmd.  However, the RINT bit provides a nice
		 * consistency check.
		 */

		/* Check for receive activity */
		if (csr0 & LE_RINT) {
			/*
			 * Check for loss of descriptor synchronization
			 * with the LANCE chip.  Following a receive
			 * error the chip immediately sets RINT and then
			 * clears the OWN bit for the remaining rmds
			 * of the packet, opening a small window
			 * where it can appear we've gotten out of sync.
			 * Therefore we check again after delaying
			 * for a bit.
			 */
			lmd = es->es_his_rmd;
			if (lmd->lmd_flags & LMD_OWN) {
				CDELAY(!(lmd->lmd_flags & LMD_OWN), 1000);
				if (lmd->lmd_flags & LMD_OWN) {
					ether_error(&es->es_if,
				"RINT but buffer owned by LANCE\n");
					lereset(unit);
					leservice[devlevel] ++;
				}
			}

			/* Pull packets off interface */
			for (lmd = es->es_his_rmd;
			    !(lmd->lmd_flags & LMD_OWN);
			    es->es_his_rmd = lmd = next_rmd(es, lmd)) {
				leservice[devlevel] ++;

				/*
				 * We acknowledge the RINT inside the
				 * loop so that the own bit for the next
				 * packet will be checked *after* RINT
				 * is acknowledged.  If we were to
				 * acknowlege the RINT just once after
				 * the loop, a packet could come in
				 * between the last test of the own bit
				 * and the time we do the RINT, in
				 * which case we might not see the
				 * packet (because we cleared its RINT
				 * indication but we did not see the
				 * own bit become clear).
				 *
				 * Race prevention: since the chip uses
				 * the order <clear own bit, set RINT>,
				 * we must use the opposite order
				 * <clear RINT, set own bit>.
				 */

				le->le_csr = LE_RINT | LE_INEA;

				leread(es, lmd);

				/*
				 * Give the descriptor and associated
				 * buffer back to the chip.
				 */
				lmd->lmd_mcnt = 0;
				lmd->lmd_flags = LMD_OWN;
			}
		}

		/* Check for transmit activity */
		if (csr0 & LE_TINT) {
			register int i;

			/*
			 * Check for loss of descriptor synchronization
			 * with the LANCE chip.  Following a transmit
			 * error the chip immediately sets TINT and then
			 * clears the OWN bit for the remaining tmds
			 * of the packet, opening a small window
			 * where it can appear we've gotten out of sync.
			 * Therefore we check again after delaying
			 * for a bit.
			 */

			lmd = es->es_cur_tmd;
			flags = lmd->lmd_flags;

			if (flags & LMD_OWN) {
				CDELAY(!(lmd->lmd_flags & LMD_OWN), 1000);
				es->es_tsync++;
				if (lmd->lmd_flags & LMD_OWN) {
					ether_error(&es->es_if,
				"TINT but buffer owned by LANCE\n");
					lereset(unit);
					leservice[devlevel] ++;
				}
				flags = lmd->lmd_flags;
			}

			/*
			 * Check each of the packet's descriptors
			 * for errors.
			 */
			while (!(flags & LMD_OWN) && lmd != es->es_nxt_tmd) {

				le->le_csr = LE_TINT | LE_INEA;

				/*
				 * Keep defer/retry statistics.
				 * Note that they don't result in
				 * aborted transmissions.  Since
				 * the chip duplicates this information
				 * into each descriptor for a packet,
				 * we check only the first.
				 * We can keep only an approximate count
				 * of collisions, since the chip gives
				 * us only 0/1/more-than-1 information.
				 * We count the last case as 2.
				 */
				if (flags & LMD_STP) {
					if (flags & TMD_DEF)
						es->es_defer++;
					if (flags & (TMD_MORE | TMD_ONE)) {
						es->es_retries++;
						es->es_if.if_collisions +=
						    (flags & TMD_MORE) ?
							2 : 1;
					}
				}

				if (flags & LMD_ENP)
					es->es_if.if_opackets++;

				/*
				 * These errors cause the packet to be aborted.
				 */
				if (lmd->lmd_flags3 &
			(TMD_BUFF|TMD_UFLO|TMD_LCOL|TMD_LCAR|TMD_RTRY)) {
					(void) le_xmit_error(es, lmd);
					es->es_if.if_oerrors++;
					if (lmd->lmd_flags3 & TMD_UFLO)
						uflo = 1;
				}

				/*
				 * Either free the mbuf chain or
				 * update the transmit area reclaim pointer.
				 */
				i = lmd - es->es_tdrp;
				if (es->es_tmbuf[i] && !uflo) {
					m_freem(es->es_tmbuf[i]);
					es->es_tmbuf[i] = NULL;
				} else if (es->es_tbuf[i]) {
					es->es_trd = es->es_tbuf[i];
					es->es_tbuf[i] = NULL;
				}

				lmd = next_tmd(es, lmd);
				flags = lmd->lmd_flags;
				leservice[devlevel] ++;
			}

			es->es_cur_tmd = lmd;

			if (es->es_if.if_snd.ifq_head)
				lestart(es - le_softc);
		}

		/*
		 * Check for errors not specifically related
		 * to transmission or reception.
		 */
		if ((csr0 & (LE_BABL|LE_MERR|LE_MISS|LE_TXON|LE_RXON))
		    != (LE_RXON|LE_TXON)) {
			leservice[devlevel]++;
			(void) le_chip_error(es, le);
		}

#ifdef sun4m
		if (le->le_csr)	/* readback */
			;
#endif
	}

	serviced = leservice[intlevel];
	leservice[intlevel] = 0;
	(void) splx(s);
	return (serviced);
}

/*
 * Move info from driver toward protocol interface
 */
leread(es, rmd)
	register struct le_softc *es;
	register struct le_md *rmd;
{
	register struct ether_header *header;
	register caddr_t buffer;
	register struct mbuf *m;
	register struct le_buf *rcvbp;
	struct le_buf *rplbp;
	int length;
	int size;
	int off;

	es->es_if.if_ipackets++;

	/* Check for packet errors. */

	/*
	 * ! ENP is an error because we have allocated maximum
	 * receive buffers (we don't do receive buffer chaining).
	 */
	if ((rmd->lmd_flags & ~RMD_OFLO) != (LMD_STP|LMD_ENP)) {
		le_rcv_error(es, rmd);
		es->es_if.if_ierrors++;
		return;
	}

	/*
	 * Convert the buffer address embedded in the receive
	 * descriptor to the address of the le_buf structure
	 * containing the buffer.
	 */
	rcvbp = es->es_rbufc[rmd - es->es_rdrp];

	/*
	 * Get input data length (minus ethernet header and crc),
	 * pointer to ethernet header, and address of data buffer.
	 */
	size = rmd->lmd_mcnt - 4;
	length = size - sizeof (struct ether_header);
	header = &rcvbp->lb_ehdr;
	buffer = (caddr_t)&rcvbp->lb_buffer[0];

#ifdef	sun4c
	if (vac)
		vac_flush((char *)header, size);
#endif	/* sun4c */

	if (check_trailer(header, buffer, &length, &off)) {
		ether_error(&es->es_if, "trailer error\n");
		es->es_if.if_ierrors++;
		return;
	}

	/*
	 * Pull packet off interface.  Off is nonzero if packet has
	 * trailing header; copy_to_mbufs will then force this header
	 * information to be at the front and will drop the extra
	 * trailer type and length fields.
	 */
	/*
	 * Receive buffer loan-out:
	 *	We're willing to loan the buffer containing this
	 *	packet to the higher protocol layers provided that
	 *	the packet's big enough and doesn't have a trailer
	 *	and that we have a spare receive buffer to use as
	 *	a replacement for it.
	 */
	/*
	 * If everything's go, wrap the receive buffer up
	 * in a cluster-type mbuf and make sure it worked.
	 */
	if (length > MLEN && off == 0 && (rplbp = es->es_rbuf_free) &&
	    (m = mclgetx(leclfree, (int)rcvbp, buffer, length, M_DONTWAIT))) {
		rcvbp->lb_es = es;
		rcvbp->lb_loaned = 1;
		/*
		 * Replace the newly loaned buffer and move the
		 * replacement out of the free list.
		 */
		install_buf_in_rmd(es, rplbp, rmd);
		es->es_rbuf_free = rplbp->lb_next;
	} else {
		m = copy_to_mbufs(buffer, length, off);
		es->es_incpy++;
	}
	if (m == (struct mbuf *) 0)
		return;

	do_protocol(header, m, &es->es_ac, length);
}

leoutput(ifp, m0, dst)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
{
	return (ether_output(ifp, m0, dst, lestart));
}

/*
 * Process an ioctl request.
 */
leioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register int			unit = ifp->if_unit;
	register struct le_softc	*es = &le_softc[unit];
	struct ifreq			*ifr = (struct ifreq *)data;
	register int			error = 0;
	int				s = splimp();

	switch (cmd) {

	/*
	 * Return link-level (Ethernet) address, filling it into
	 * the data part of the sockaddr embedded in the ifreq.
	 * The address family part of the sockaddr is problematic,
	 * and we leave it untouched.
	 */
	case SIOCGIFADDR:
		ether_copy(&((struct arpcom *)ifp)->ac_enaddr,
			ifr->ifr_addr.sa_data);
		break;

	case SIOCSIFADDR:
		error = set_if_addr(ifp,
			&((struct ifaddr *)data)->ifa_addr, &es->es_enaddr);
		break;

	/*
	 * Set a multicast address for the interface.
	 */
	case SIOCADDMULTI:
		error = addmultiaddr((struct arpcom *)es, ifr);
		if (error)
			break;

		/* Initialize chip with new address. */
		leinit(unit);
		break;

	/*
	 * Remove a multicast address for the interface.
	 */
	case SIOCDELMULTI:
		error = delmultiaddr((struct arpcom *)es, ifr);
		if (error)
			break;

		/* Initialize chip with new address. */
		leinit(unit);
		break;

	case SIOCSIFFLAGS:
		/* Bring the new state into effect. */
		leinit(unit);
		break;

	default:
		error = EINVAL;
	}

	(void) splx(s);

	return (error);
}

le_rcv_error(es, lmd)
	struct le_softc *es;
	struct le_md	*lmd;
{
	u_char flags = lmd->lmd_flags;
	struct	le_buf	*bp;

	if ((flags & RMD_FRAM) && !(flags & RMD_OFLO)) {
		if (ledebug)
			ether_error(&es->es_if,
				"Receive: framming error\n");
		es->es_fram++;
	}
	if ((flags & RMD_CRC) && !(flags & RMD_OFLO)) {
		if (ledebug)
			ether_error(&es->es_if,
				"Receive: crc error\n");
		es->es_crc++;
	}
	if ((flags & RMD_OFLO) && !(flags & LMD_ENP)) {
		if (ledebug)
			ether_error(&es->es_if,
				"Receive: overflow error\n");
		es->es_oflo++;
	}
	if (flags & RMD_BUFF)
		ether_error(&es->es_if,	"Receive: BUFF set in rmd\n");
	/*
	 * If an OFLO error occurred, the chip may not set STP or ENP,
	 * so we ignore a missing ENP bit in these cases.
	 */
	if (!(flags & LMD_STP) && !(flags & RMD_OFLO))
		ether_error(&es->es_if,
			"Receive: STP in rmd cleared\n");
	else if (!(flags & LMD_ENP) && !(flags & RMD_OFLO)) {
		bp = es->es_rbufc[lmd - es->es_rdrp];
		ether_error(&es->es_if,
			"Receive: giant packet from %x:%x:%x:%x:%x:%x\n",
			bp->lb_ehdr.ether_shost.ether_addr_octet[0],
			bp->lb_ehdr.ether_shost.ether_addr_octet[1],
			bp->lb_ehdr.ether_shost.ether_addr_octet[2],
			bp->lb_ehdr.ether_shost.ether_addr_octet[3],
			bp->lb_ehdr.ether_shost.ether_addr_octet[4],
			bp->lb_ehdr.ether_shost.ether_addr_octet[5]);
	}
}

/*
 * Report on transmission errors paying very close attention
 * to the Rev B (October 1986) Am7990 programming manual.
 */
le_xmit_error(es, lmd)
	struct le_softc *es;
	struct le_md	*lmd;
{
	u_short flags = lmd->lmd_flags3;
	char *NCMSG =
	"No carrier - twisted pair cable problem or hub link test disabled?\n";

	/*
	 * BUFF is not valid if either RTRY or LCOL is set.
	 * We assume here that BUFFs are always caused by UFLO's
	 * and not driver bugs.
	 */
	if ((flags & (TMD_BUFF | TMD_RTRY | TMD_LCOL)) == TMD_BUFF) {
		if (ledebug)
			ether_error(&es->es_if, "Transmit: BUFF set in tmd\n");
		es->es_tBUFF++;
	}
	if (flags & TMD_UFLO) {
		if (ledebug)
			ether_error(&es->es_if, "Transmit underflow\n");
		es->es_uflo++;
	}
	if (flags & TMD_LCOL) {
		if (ledebug)
			ether_error(&es->es_if,
				"Transmit late collision - net problem?\n");
		es->es_tlcol++;
	}
	if (flags & TMD_LCAR) {
		int unit = es - le_softc;

		if (unit == 0 && le_tp_aui)
			ether_error(&es->es_if, NCMSG);
		else
			ether_error(&es->es_if,
				"No carrier - transceiver cable problem?\n");
		es->es_tnocar++;
	}
	if (flags & TMD_RTRY) {
		if (ledebug)
			ether_error(&es->es_if,
		"Transmit retried more than 16 times - net jammed\n");
		es->es_trtry++;
	}
}

/* Handles errors that are reported in the chip's status register */
le_chip_error(es, le)
	struct le_softc  *es;
	struct le_device *le;
{
	register u_short	csr = le->le_csr;
	int restart = 0;
	static	int	opackets;

	if (csr & LE_MISS) {
		if (ledebug)
			ether_error(&es->es_if, "missed packet\n");
		es->es_missed++;
		es->es_if.if_ierrors++;
		le->le_csr = LE_MISS | LE_INEA;
	}

	if (csr & LE_BABL) {
	    ether_error(&es->es_if,
		"Babble error - packet longer than 1518 bytes\n");
	    le->le_csr = LE_BABL | LE_INEA;
	}
	/*
	 * If a memory error has occurred, both the transmitter
	 * and the receiver will have shut down.
	 * Display the Reception Stopped message if it
	 * wasn't caused by MERR (should never happen) OR ledebug.
	 * Display the Transmission Stopped message if it
	 * wasn't caused by MERR (was caused by UFLO) AND ledebug
	 * since we will have already displayed a UFLO msg.
	 */
	if (csr & LE_MERR) {
		if (ledebug)
			ether_error(&es->es_if, "Memory error!\n");
		le->le_csr = LE_MERR | LE_INEA;
		es->es_memerr++;
		restart++;
	}
	if (!(csr & LE_RXON)) {
	    if (!(csr & LE_MERR) || ledebug)
		ether_error(&es->es_if, "Reception stopped\n");
	    restart++;
	}
	if (!(csr & LE_TXON)) {
		if (!(csr & LE_MERR) && ledebug)
			ether_error(&es->es_if, "Transmission stopped\n");
		restart++;
	}

	if (restart) {
		if (ledebug) {
			identify(&es->es_if);
			le_print_csr(csr);
		}
		/*
		 * Requeue packets only if leretransmit is turned on
		 * and there has been at least one successfully transmitted
		 * packet since last time and this is a DMA interface.
		 */
		if (leretransmit && (es->es_if.if_opackets != opackets) &&
			!LEPIO(es))
			lerequeue(es);
		opackets = es->es_if.if_opackets;
		leinit(es - le_softc);
	}
	return (restart);
}

/*
 * Requeue mbuf chains on the if_snd queue.
 */
lerequeue(es)
register	struct	le_softc	*es;
{
	register	struct	mbuf	*m;
	register	struct	le_md	*t;
	register	int	i;

	t = es->es_nxt_tmd;
	do {
		t = prev_tmd(es, t);	/* reverse order! */
		i = t - es->es_tdrp;
		if (m = es->es_tmbuf[i]) {
			if (!IF_QFULL(&es->es_if.if_snd)) {
				IF_PREPEND(&es->es_if.if_snd, m);
			} else
				m_freem(m);
			es->es_tmbuf[i] = NULL;
		}
	} while (t != es->es_cur_tmd);
}

static	struct	leops	*leops = NULL;

/*
 * Create and add an leops structure to the linked list.
 */
leopsadd(lop)
struct	leops	*lop;
{
	lop->lo_next = leops;
	leops = lop;
}

struct leops*
leopsfind(dev)
struct	dev_info	*dev;
{
	register	struct	leops	*lop;

	for (lop = leops; lop; lop = lop->lo_next)
		if (lop->lo_dev == dev)
			return (lop);
	return (NULL);
}

/*
 * Print out a csr value in a nicely formatted way.
 */
le_print_csr (csr)
	register u_short	csr;
{
#define	LE_BITS	"\20\20ERR\17BABL\16CERR\15MISS\14MERR\13RINT\12TINT\
\11IDON\10INTR\7INEA\6RXON\5TXON\4TDMD\3STOP\2STRT\1INIT"
	(void) printf("csr: %b\n", csr, LE_BITS);
}

#endif NLE > 0
