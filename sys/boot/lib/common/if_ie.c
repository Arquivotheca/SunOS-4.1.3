#ifndef lint
static        char sccsid[] = "@(#)if_ie.c 1.1 92/07/30 SMI";
#endif


/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef sun4c	/* to end of file */

/* 
 * Parameters controlling TIMEBOMB action: times out if chip hangs.
 *
 */
#define	TIMEBOMB	1000000		/* One million times around is all... */
#define	TIMEOUT		-1		/* if (function()) return (TIMEOUT); */
#define	OK		0		/* Traditional OK value */


/*
 * Sun Intel Ethernet Controller interface
 */
#include <stand/saio.h>
#include <stand/param.h>
#include <sys/socket.h>
#include <boot/if.h>
#include <netinet/in.h>
#include <boot/if_ether.h>
#include <sunif/if_iereg.h>
#include <sunif/if_mie.h>
#include <sunif/if_obie.h>
#include <sunif/if_tie.h>
#include <mon/idprom.h>
#ifdef sun4
#include <machine/mmu.h>
#include <sun4/cpu.h>
#include <sun4/iocache.h>
#include <sun4/pte.h>
#include <sun4/enable.h>
#endif sun4
#include <mon/cpu.map.h>

#ifdef sun3x
#include <sun3x/devaddr.h>
#endif sun3x

/*
 * Never did get IO Caching the rbufs to work -- somehow the flush
 * didn't seem to work.  But since we boot fine with an IO Cached
 * xmit buf, and multiple non-IO-Cached rbufs, I left it that way.
 */
#ifdef IOC_RBUF
#undef IOC_RBUF
#endif IOC_RBUF

int	iexmit(), iepoll(), iereset(), myetheraddr(), iestats();

unsigned long iestd[] = { 0x88000, 0x8C000 };
#define NSTD 2

struct saif ieif = {
	iexmit,
	iepoll,
	iereset,
	myetheraddr,
	iestats
};

#define	IEVVSIZ		1024		/* # of pages in page map */
#define IEPHYMEMSIZ	(16*1024)
#define IEVIRMEMSIZ	(16*1024)
#define IEPAGSIZ	1024
#define	IERBUFSIZ	1600	/* Multiple of 32 bytes, for 4/470 IOC */
#define	IEXBUFSIZ	1600	/* Multiple of 32 bytes, for 4/470 IOC */

#ifdef sun2
#define	IEDMABASE	0x000000	/* Can access all of memory */
#endif sun2
#ifdef sun3
#define	IEDMABASE	0x0F000000	/* Can access top 16MB of memory */
#endif sun3
#ifdef sun3x
#define	IEDMABASE	0xFF000000	/* Can access top 16MB of memory */
#endif sun3x
#ifdef sun4
#define	IEDMABASE	0xFF000000	/* Can access top 16MB of memory */
#endif sun4

/* Location in virtual memory of the SCP (System Configuration Pointer) */
#define SCP_LOC (char *)(IEDMABASE+IESCPADDR)

/* controller types */
#define IE_MB	1	/* Multibus */
#define	IE_OB	2	/* onboard */
#define	IE_TE	3	/* 3E */

/* 2nd ethernet multibus ie1 defines */
#define IE_MB_PA_MEM		0xe40000
#define IE_MB_PA_REG		0xe88000

/* 3E releated defines */
#define IE_TE_BOARD		0x300000
#define IE_TE_CSR		(IE_TE_BOARD + 0x1ff02)
#define IE_TE_BUFFER		(IE_TE_BOARD + 0x20000)
#define IE_TE_SCP_OFFSET	0x1fff6

#define	IENRBUF	8	/* # of receive rfd's, rbd's, and rbuf's */

struct ie_softc {
	/*
	 * This first item does two things.  First, it reserves space for
	 * all the various protocols that need to put their locals somewhere
	 * (it was decided that HERE is convenient!).  Second, it fills
	 * out the structure to align es_scp to the wierd-in address
	 * that Intel (thanks guys) fetches it from.
	 */
	char	es_fill[PROTOSCRATCH - sizeof(struct iescp)]; /* Align */
	struct	iescp	es_scp;		/* System config pointer (used once) */
	/* page boundary should be here */
	struct	ieiscp	es_iscp;	/* Intermediate sys config ptr (once) */
	struct	iescb	es_scb;		/* Sys Control Block, the real mama */
	struct	ierbd	es_rbdtab[IENRBUF];
	struct	ierfd	es_rfdtab[IENRBUF];
	struct	ietfd	es_tfd;
	struct	ietbd	es_tbd;
	struct	mie_device *es_mie;
	struct	obie_device *es_obie;
	struct	tie_device *es_tie;
	short	es_type;
	struct	ieiaddr	es_iaddr;	/* Cmd to set up our Ethernet addr */
	struct	ieconf	es_ic;		/* Cmd to configure the chip */
	/*
	 * The Sun-3/E VME board can address buffers only if they
	 * are on-board.  For this reason, Sun-3 declares es_rubfs[]
	 * and es_xbuf statically inside ie_softc which lives on-board.
	 * The Sun-4/490 needs to put ie_softc inside special ethernet
	 * descriptor RAM, but these static buffers would not fit.
	 * For this reason, and the fact that the 490 needs to I/O Cache
	 * the xbuf to prevent underruns, Sun-4 allocates es_rbufs[] and
	 * es_xbuf dynamically.
	 */
#ifdef sun4
	char	*es_rbufs[IENRBUF];
	char	*es_xbuf;	/* only one, must be NULL till allocated */
#else
	char	es_rbufs[IENRBUF][IERBUFSIZ];
	char	es_xbuf[IEXBUFSIZ];
#endif
	struct	ierfd	*es_rfdhd;	/* points to first rfd */
};

/* Statistics counters for iestats() */
static struct {
	int	ipackets;
	int	opackets;
	int	collis;
	int	defer;
	int	crcerrs;
	int	alnerrs;
	int	rscerrs;
	int	orunerrs;
	int	urunerrs;
} stats;

/*
 * Convert big endian address to Intel 24-bit address.
 */
#define put_ieaddr(addr, where) { \
	register char *cp; \
	union { \
		int	n; \
		char	c[4]; \
	} a; \
 \
	a.n = (int)(addr); \
	cp = (char *)(where); \
	cp[0] = a.c[3]; \
	cp[1] = a.c[2]; \
	cp[2] = a.c[1]; \
}
 
/*
 * This initializes the onboard Ethernet control reg to:
 *	Reset is active
 *	Loopback is NOT active
 *	Channel Attn is not active
 *	Interrupt enable is not active.
 * Loopback is deactivated due to a bug in the Intel serial interface
 * chip.  This chip powers-up in a locked up state if Loopback is active.
 * It "unlocks" itself if you release Loopback; then it's OK to reassert it.
 */
struct obie_device obie_reset = {0, 1, 0, 0, 0, 0, 0};

ieoff_t to_ieoff();
ieaddr_t to_ieaddr();
ieint_t	from_ieint();
#ifdef sun4
void ieallocbufs();
#endif sun4
extern int verbosemode;

struct	ierfd	*prevrfd(), *nextrfd();

struct devinfo ieinfo = {
	sizeof (struct mie_device),
	sizeof(struct ie_softc),
	0,
	NSTD,
	iestd,
	MAP_MBMEM,
	0				/* transfer size handled by ND */
};

int xxprobe(), xxboot(), ieopen(), ieclose(), etherstrategy(), nullsys();

struct boottab iedriver = {
	"ie",	xxprobe,	xxboot,	ieopen,		ieclose,
	etherstrategy,	"ie: Sun/Intel Ethernet",	&ieinfo,
};

struct idprom id;

/*
 * Open Intel Ethernet nd connection, return -1 for errors.
 */
ieopen(sip)
	struct saioreq *sip;
{
	register int result;

	sip->si_sif = &ieif;
	if ( ieinit(sip) || (result = etheropen(sip)) < 0 ) {
		ieclose(sip);		/* Make sure we kill chip */
		return (-1);
	}
	return (result);
}

/*
 * Set up memory maps and Ethernet chip.
 * Returns 1 for error, 0 for ok.
 */
int
ieinit(sip)
	struct saioreq *sip;
{
	register struct ie_softc *es = (struct ie_softc *)0;
	int paddr;
	int i;
	register struct mie_device *mie;
	struct miepg *pg;
	short *ap;

	if (idprom(IDFORM_1, &id) != IDFORM_1) {
		printf("not supported\n");
		return (1);
	}
	switch (id.id_machine) {
	case IDM_SUN2_MULTI:
		mie = (struct mie_device *) sip->si_devaddr;
		mie->mie_peack = 1;
		mie->mie_noloop = 0;
		mie->mie_ie = 0;
		mie->mie_pie = 0;
		paddr = mie->mie_mbmhi << 16;
		ap = (short *)mie->mie_pgmap;
		for (i=0; i<IEVVSIZ; i++)	/* note: sets p2mem -> 0 */
			*ap++ = 0;
		for (i=0; i<IEPHYMEMSIZ/IEPAGSIZ; i++) {
			mie->mie_pgmap[0].mp_pfnum = i;
		}
		pg = &mie->mie_pgmap[0];
		for (i=0; i<IEVIRMEMSIZ/IEPAGSIZ; i++) {
			pg->mp_swab = 1;
			pg->mp_pfnum = i;
			pg++;
		}
		/* last page for chip init */
		mie->mie_pgmap[IEVVSIZ-1].mp_pfnum = (PROTOSCRATCH/IEPAGSIZ)-1;
		es = (struct ie_softc *)
			devalloc(MAP_MBMEM, paddr, sizeof (struct ie_softc));
		bzero((char *)es, sizeof *es);
		es->es_type = IE_MB;
		es->es_mie = mie;
		break;

	case IDM_SUN3_E:
		/* 3E ethernet */
		es = (struct ie_softc *)
			devalloc(MAP_VME24A16D, IE_TE_BUFFER, IE_TE_MEMSIZE);
		bzero((char *)es, IE_TE_MEMSIZE);
		es->es_tie = (struct tie_device *)devalloc(MAP_VME24A16D,
			IE_TE_CSR, sizeof (struct tie_device));
		es->es_type = IE_TE;
		break;

#if defined(sun4)
	case IDM_SUN4_SUNRAY:
		/* if this is a sunray ie0, the iocache has to be used */
		if (sip->si_ctlr == 0 || sip->si_ctlr == 0x88000) {
			struct pte pte;
			char *temp;
			register u_int *p;

			/*
			 * for sunray IOC_IEDESCR_ADDR is the magic address
			 * where the 4K sunray descriptor cache sits.
			 * everything in ie_softc will fit; we double
			 * map the page before it so that the scp is at
			 * the end of the page at the "hard-weird" address.
			 */
			sunray_ioc_init();

			/* grab a page for the scp */
			p = (u_int *)&pte;
			temp = resalloc(RES_MAINMEM, PAGESIZE);
			*p = getpgmap(temp);
			pte.pg_nc = 1;
			setpgmap(IOC_IEDESCR_ADDR - PAGESIZE, *p);

			*p = getpgmap(IOC_IEDESCR_ADDR);
			pte.pg_ioc = 1;
			setpgmap(IOC_IEDESCR_ADDR, *p);

			es = (struct ie_softc *)(IOC_IEDESCR_ADDR - PAGESIZE);
			es->es_type = IE_OB;
			es->es_obie = (struct obie_device *)
				devalloc(MAP_OBIO, VIOPG_ETHER<<BYTES_PG_SHIFT,
					sizeof (struct obie_device));
			break;
		}
		/* if not onboard, fall through to default */
#endif /* sun4 */
	default:
		if (sip->si_ctlr == 0 || ((sip->si_ctlr == 0x88000) &&
					(id.id_machine != IDM_SUN4_SUNRAY))) {
			/* onboard Ethernet */
			es = (struct ie_softc *)sip->si_dmaaddr;
			bzero((char *)es, sizeof *es);
			es->es_type = IE_OB;
#ifdef sun3x
			es->es_obie = (struct obie_device *)
				devalloc(MAP_OBIO, OBIO_INTEL_ETHER,
					sizeof (struct obie_device));
#else sun3x
			es->es_obie = (struct obie_device *)
				devalloc(MAP_OBIO, VIOPG_ETHER<<BYTES_PG_SHIFT,
					sizeof (struct obie_device));
#endif sun3x
		} else if ((sip->si_ctlr == 1) || (sip->si_ctlr == 0x8c000)) {
			/* ie1 multibus in vme */
			mie = (struct mie_device *)
				devalloc(MAP_VME24A16D, IE_MB_PA_REG,
					sizeof (struct mie_device));
			sip->si_devaddr = (char *)mie;

			/* probably need only a page */
			es = (struct ie_softc *)
				devalloc(MAP_VME24A16D, IE_MB_PA_MEM,
					IEPHYMEMSIZ);
			sip->si_devdata = (char *)es;

			ie_putval(mie, &es->es_mie);

			mie->mie_reset = 1;
			DELAY(200);
			mie->mie_peack = 1;
			mie->mie_noloop = 0;
			DELAY(40);
			mie->mie_ie = 0;
			mie->mie_pie = 0;
			mie->mie_reset = 0;
			DELAY(200);

			ap = (short *)&mie->mie_pgmap[0];
			for (i=0; i<IEVVSIZ; i++) /* note: sets p2mem -> 0 */
				*ap++ = 0;
			for (i=0; i<IEPHYMEMSIZ/IEPAGSIZ; i++) {
				mie->mie_pgmap[0].mp_pfnum = i;
				bzero((char *)es + (IEPAGSIZ*i), IEPAGSIZ);
			}
			pg = &mie->mie_pgmap[0];
			for (i=0; i<IEVIRMEMSIZ/IEPAGSIZ; i++) {
				pg->mp_swab = 1;
				pg->mp_pfnum = i;
				pg++;
			}
			/* last page for chip init */
			mie->mie_pgmap[IEVVSIZ-1].mp_pfnum =
					(PROTOSCRATCH/IEPAGSIZ)-1;
			es->es_type = IE_MB;
		} else if (sip->si_ctlr == 2) {
			/* 3E ethernet */
			es = (struct ie_softc *) devalloc(MAP_VME24A32D,
				IE_TE_BUFFER, IE_TE_MEMSIZE);
			bzero((char *)es, IE_TE_MEMSIZE);
			es->es_tie = (struct tie_device *)
				devalloc(MAP_VME24A16D, IE_TE_CSR,
					sizeof (struct tie_device));
			es->es_type = IE_TE;
		}
		break;
	}
	/* FIXME, release multibus resources ifdef. */
	sip->si_devdata = (caddr_t)es;
	return iereset(es, sip);
}

/*
 * Basic 82586 initialization
 * Returns 1 for error, 0 for ok.
 */
/*ARGSUSED*/
int
iereset(es, sip)
	register struct ie_softc *es;
	struct saioreq *sip;
{
	struct ieiscp *iscp = &es->es_iscp;
	struct iescb *scb = &es->es_scb;
	struct ieiaddr *iad = &es->es_iaddr;
	struct ieconf *ic = &es->es_ic;
	int j;
	int savepmap;
	register struct obie_device *obie;
	register struct tie_device *tie;
	register struct mie_device *mie;
	struct iescp *te_scp;
#ifdef sun4
	register char *addr;
	register short savesegno;
#endif sun4

	for (j = 0; j < 10; j++) {
		/* Set up the control blocks for initializing the chip */
		bzero((caddr_t)&es->es_scp, sizeof (struct iescp));
		bzero((caddr_t)iscp, sizeof (struct ieiscp));
		bzero((caddr_t)scb, sizeof (struct iescb));
		iscp->ieis_busy = 1;
		iscp->ieis_scb = to_ieoff(es, (caddr_t)scb);
 
		if (es->es_type != IE_MB) {
			es->es_scp.ies_iscp = to_ieaddr(es, (caddr_t)iscp);
			iscp->ieis_cbbase = to_ieaddr(es, (caddr_t)es);
		} else {
			put_ieaddr((caddr_t)iscp - (caddr_t)es,
				&es->es_scp.ies_iscp);
			put_ieaddr(0, &iscp->ieis_cbbase);
		}
 
		switch (es->es_type) {
		case IE_OB:
			obie = es->es_obie;
			/*
			 * The 82586 has bugs that require us to be in 
			 * loopback mode while we initialize it, to avoid
			 * transitions on CRS (carrier sense).
			 * 
			 * However, the Intel serial interface chip used
			 * on Carrera also has bugs: if it powers-up in
			 * loopback, you have to release loopback at least
			 * once to make it behave -- it powers up with
			 * CRS and CDT permanently active, which rapes the 586.
			 *
			 * How do you spell "broken"?  I-N-T-E-L...
			 */
			*obie = obie_reset;	/* Reset chip & interface */
			DELAY(40);
			obie->obie_noloop = 0;	/* Put it in loopback now */
			DELAY(40);
			obie->obie_noreset = 1;	/* Release Reset on 82586 */
			DELAY(300);
			/*
			 * Now set up to let the Ethernet chip read the SCP.
			 * Intel wired in the address of the SCP.  It happens
			 * to be 0xFFFFF6.
			 */
#ifdef sun4
			if (id.id_machine != IDM_SUN4_SUNRAY) {
				addr = resalloc(RES_RAWVIRT, 2*BYTESPERSEG);
				savesegno = getsegmap((int)SCP_LOC);
				setsegmap(SCP_LOC, getsegmap((int)(addr +
					BYTESPERSEG - 4)));
			}
#endif sun4
			savepmap = getpgmap(SCP_LOC);
			setpgmap(SCP_LOC, getpgmap((char *)&es->es_scp));
			break;
		case IE_MB:
			mie = (struct mie_device *)sip->si_devaddr;
			mie->mie_reset = 1;
			DELAY(20);
			mie->mie_noloop = 0;
			DELAY(20);
			mie->mie_reset = 0;
			DELAY(200);
			break;
		case IE_TE:
			tie = es->es_tie;
			te_scp = (struct iescp *)
#ifdef sun4
			    (((caddr_t) es) + IE_TE_SCP_OFFSET - 2);
#else
			    (((caddr_t) es) + IE_TE_SCP_OFFSET);
#endif sun4
			bcopy((caddr_t) &es->es_scp,(caddr_t)te_scp,
			    sizeof(struct iescp));
			tie->tie_reset = 1;
			DELAY(20);
			*(char *)&tie->tie_status = 0;
			DELAY(200);
			break;
		default:
			printf("iereset: unknown interface\n");
			break;
		}

		/*
		 * We are set up.  Give the chip a zap, then wait up to
		 * 1 sec, or until chip comes ready.
		 */
		ieca(es);
		CDELAY(iscp->ieis_busy != 1, 1000);

		/* Whether or not it worked, we have to clean up. */
		switch (es->es_type) {
		case IE_OB:
			/* If it didn't init, reset chip again. */
			if (iscp->ieis_busy == 1)
				obie->obie_noreset = 0;
			setpgmap(SCP_LOC, savepmap);
#ifdef sun4
			if (id.id_machine != IDM_SUN4_SUNRAY)
				setsegmap(SCP_LOC, savesegno);
#endif sun4
			break;
		case IE_MB:
		case IE_TE:
			break;
		}
		if (iscp->ieis_busy == 1)
			continue;	/* Continue loop until we get it */

		/*
		 * Now try to run a few simple commands before we say "OK".
		 */
		bzero((caddr_t)iad, sizeof (struct ieiaddr));
		iad->ieia_cb.iec_cmd = IE_IADDR;
		myetheraddr((struct ether_addr *)iad->ieia_addr);
		if (iesimple(es, &iad->ieia_cb)) {
			printf("ie: hang while setting Ethernet address.\n");
			continue;
		}

		iedefaultconf(ic);
		if (iesimple(es, &ic->ieconf_cb)) {
			printf("ie: hang while setting chip config.\n");
			continue;
		}

		/*
		 * Take the Ethernet interface chip out of loopback mode, i.e.
		 * put us on the wire.  We can't do this before having
		 * initialized the Ethernet chip because the chip does random
		 * things if its 'wire' is active between the time it's reset
		 * and the first CA.  Also, the IA-setup and configure commands
		 * will hang under some conditions unless the interface is
		 * very quiet and still.
		 */
		switch (es->es_type) {
		case IE_OB:
			obie->obie_noloop = 1;
			break;
		case IE_MB:
			mie->mie_noloop = 1; 
			break;
		case IE_TE:
			tie->tie_noloop = 1;
			break;
		default:
			break;
		}

#ifdef sun4
		ieallocbufs(es);
#endif sun4
		ierfdinit(es);
		if (ierustart(es)) {
			printf("ie: hang while starting receiver.\n");
			continue;
		}
			
		return 0;		/* It all worked! */
	}

	/* We tried a bunch of times, no luck. */
	printf("ie: cannot initialize\n");
	return 1;
}

/*
 * Return pointer to previous rfd in rfd ring.
 */
struct ierfd*
prevrfd(es, rfd)
struct ie_softc *es;
struct ierfd *rfd;
{
	if (--rfd >= es->es_rfdtab)
		return (rfd);
	else
		return (&es->es_rfdtab[IENRBUF-1]);
}

/*
 * Return pointer to next rfd in rfd ring.
 */
struct ierfd*
nextrfd(es, rfd)
struct ie_softc	*es;
struct ierfd *rfd;
{
	if (++rfd < &es->es_rfdtab[IENRBUF])
		return (rfd);
	else
		return (es->es_rfdtab);
}

/*
 *	Initialize the Receive Unit data structures.
 *	Chain the es_rfdtab[] entries into a ring
 *	with each rfd getting one rbd and each rbd getting one rbuf.
 *	es->es_rfdhd always points to the first rfd in the ring
 *	and the "tail" of the rfd ring is always prevrfd(es, es->es_rfdhd)
 *	which makes things pretty simple.
 */
ierfdinit(es)
register struct ie_softc *es;
{
	register	int	i;
	struct	ierfd	*rfd;
	struct	ierbd	*rbd;
	char	*rbuf;

	/*
	 * Zero the rfd and rbd tables.
	 */
	bzero(es->es_rfdtab, (IENRBUF * sizeof (struct ierfd)));
	bzero(es->es_rbdtab, (IENRBUF * sizeof (struct ierbd)));

	/*
	 * Walk through the rfd ring and rbd table initializing each.
	 */
	for (i = 0; i < IENRBUF; i++) {
		rfd = &es->es_rfdtab[i];
		rbd = &es->es_rbdtab[i];
		rbuf = es->es_rbufs[i];

		rfd->ierfd_next = to_ieoff(es, (caddr_t) nextrfd(es, rfd));
		rfd->ierfd_rbd = to_ieoff(es, (caddr_t) rbd);

		rbd->ierbd_next = -1;	/* not used since el is 1 */
		rbd->ierbd_el = 1;
		rbd->ierbd_buf = to_ieaddr(es, rbuf);
		rbd->ierbd_sizehi = IERBUFSIZ >> 8;
		rbd->ierbd_sizelo = IERBUFSIZ & 0xFF;
	}
	rfd = &es->es_rfdtab[IENRBUF-1];
	rfd->ierfd_next = to_ieoff(es, (caddr_t) &es->es_rfdtab[0]);
	rfd->ierfd_el = 1;

	/*
	 *	Set the head rfd pointer.
	 */
	es->es_rfdhd = &es->es_rfdtab[0];
}

/*
 * Initialize and start the Receive Unit
 */
int
ierustart(es)
	register struct ie_softc *es;
{
	register struct iescb *scb = &es->es_scb;
	int timebomb = TIMEBOMB;

	if (scb->ie_rus == IERUS_READY)
	{
		return OK;
	}

	while (scb->ie_cmd != 0)	/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;

	/*
	 * Start the RU.
	 */
	scb->ie_rfa = to_ieoff(es, (caddr_t) es->es_rfdhd);
	scb->ie_cmd = IECMD_RU_START;
	ieca(es);
	return OK;
}

ieca(es)
	register struct ie_softc *es;
{
	switch (es->es_type) {
	case IE_OB:
		es->es_obie->obie_ca = 1;
		es->es_obie->obie_ca = 0;
		break;
	case IE_MB:
		{
		register struct mie_device *mie;
		mie = (struct mie_device *)ie_getval(&es->es_mie);
		mie->mie_ca = 1;
		mie->mie_ca = 0;
		}
		break;
	case IE_TE:
		es->es_tie->tie_ca = 1;
		es->es_tie->tie_ca = 0;
		break;
	default:
		break;
	}
}

int
iesimple(es, cb)
	register struct ie_softc *es;
	register struct iecb *cb;
{
	register struct iescb *scb = &es->es_scb;
	register timebomb = TIMEBOMB;

#if defined(sun4)
	/*
	 * Sunray must use IO Cache.  Flush the outgoing ethernet line.
	 * This insures that the buffers we are about to add to the queue
	 * are not already in the I/O cache.
	 * 
	 * XXX -- not positive that this is necessary.
	 */
	if ((id.id_machine == IDM_SUN4_SUNRAY) && (es->es_type == IE_OB)){
		ioc_flush(IOC_ETHER_OUT);
	}
#endif /* sun4 */

	*(short *)cb = 0;	/* clear status bits */
	cb->iec_el = 1;
	cb->iec_next = 0;

	/* start CU */
	while (scb->ie_cmd != 0)	/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;
	scb->ie_cbl = to_ieoff(es, (caddr_t)cb);
	scb->ie_cmd = IECMD_CU_START;
	if (scb->ie_cx)
		scb->ie_cmd |= IECMD_ACK_CX;
	if (scb->ie_cnr)
		scb->ie_cmd |= IECMD_ACK_CNR;
	ieca(es);
	while (!cb->iec_done)		/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;
	while (scb->ie_cmd != 0)	/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;
	if (scb->ie_cx)
		scb->ie_cmd |= IECMD_ACK_CX;
	if (scb->ie_cnr)
		scb->ie_cmd |= IECMD_ACK_CNR;
	ieca(es);
	return OK;
}
 
/*
 * Transmit a packet.
 * Always copy the packet for the sake of Multibus
 * boards and Sun3's.  This is not a performance critical situation
 */
iexmit(es, buf, count)
	register struct ie_softc *es;
	char *buf;
	int count;
{
	register struct ietbd *tbd = &es->es_tbd;
	register struct ietfd *td = &es->es_tfd;

	bzero((caddr_t)tbd, sizeof *tbd);
	tbd->ietbd_eof = 1;
	tbd->ietbd_cntlo = count & 0xFF;
	tbd->ietbd_cnthi = count >> 8;
	bcopy(buf, es->es_xbuf, count);
	if (es->es_type == IE_MB){	/* this is a macro */
		put_ieaddr((es->es_xbuf - (caddr_t)es), &(tbd->ietbd_buf));
	}else
		tbd->ietbd_buf = to_ieaddr(es, es->es_xbuf);

	td->ietfd_tbd = to_ieoff(es, (caddr_t)tbd);
	td->ietfd_cmd = IE_TRANSMIT;
	if (iesimple(es, (struct iecb *)td)) {
		printf("ie: xmit hang\n");
		return -1;
	}
	stats.collis += td->ietfd_ncoll;
	if (td->ietfd_ok) {
		stats.opackets++;
		stats.defer += td->ietfd_defer;
		return (0);
	}
	if (td->ietfd_xcoll || td->ietfd_nocarr || td->ietfd_nocts)
		printf("ie: Ethernet cable problem\n");
	if (td->ietfd_underrun)
		stats.urunerrs++;
	return (-1);
}

int
iepoll(es, buf)
	register struct ie_softc *es;
	char *buf;
{
	register struct ierfd *rfd;
	register struct ierbd *rbd;
	register char	*rbuf;
	register struct iescb *scb = &es->es_scb;
	int	i;
	int len;
	int timebomb = TIMEBOMB;

	/*
	 * Cheat.  Depend on the fact that rfd-->rbd-->rbuf use same indices.
	 */
	rfd = es->es_rfdhd;
	i = (int) (rfd - es->es_rfdtab);
	rbd = &es->es_rbdtab[i];
	rbuf = es->es_rbufs[i];
	
	/* update cumulative statistics */
	stats.crcerrs = from_ieint(scb->ie_crcerrs);
	stats.alnerrs = from_ieint(scb->ie_alnerrs);
	stats.rscerrs = from_ieint(scb->ie_rscerrs);
	stats.orunerrs = from_ieint(scb->ie_ovrnerrs);

	if (!rfd->ierfd_done)
		return (0);		/* no packet yet */

	/*
	 * We got a packet.
	 */
	stats.ipackets++;
	if (rbd->ierbd_eof) {	/* good packet */
#ifdef sun4
		/*
		 * For sunray IO Cache, flush the incoming ethernet
		 * line. This insures that the buffer we are about to
		 * look at is not still in the I/O cache.
		 */
#if defined(IOC_RBUF)
		if ((id.id_machine == IDM_SUN4_SUNRAY) &&
			(es->es_type == IE_OB))
		{
			ioc_flush(IOC_ETHER_IN);
		}
#endif IOC_RBUF
#endif sun4
		len = (rbd->ierbd_cnthi << 8) + rbd->ierbd_cntlo;
		bcopy(rbuf, buf, len);
	}

	/*
	 * Reinitialize rfd and increment rfd ring head pointer.
	 */
	rbd->ierbd_sizehi = IERBUFSIZ >> 8;
	rbd->ierbd_sizelo = IERBUFSIZ & 0xFF;
	rfd->ierfd_el = 1;
	prevrfd(es, es->es_rfdhd)->ierfd_el = 0;
	es->es_rfdhd = nextrfd(es, es->es_rfdhd);
	
	/*
	 * Acknowledge frame reception and prod the chip for more.
	 */
	while (scb->ie_cmd != 0)	/* XXX */
		if (timebomb-- <= 0) return TIMEOUT;
	if (scb->ie_fr)
		scb->ie_cmd |= IECMD_ACK_FR;
	if (scb->ie_rnr)
		scb->ie_cmd |= IECMD_ACK_RNR;
	ieca(es);
	ierustart(es);
	return (len);
}

/*
 * Convert a CPU virtual address into an Ethernet virtual address.
 *
 * For Multibus, we assume it's in the Ethernet's memory space, and we
 * just subtract off the start of the memory space (==es).  For Model 50,
 * the Ethernet chip has full access to supervisor virtual memory.
 * For Carrera, the chip can access the top 16MB of virtual memory,
 * but we never give it addresses outside that range, so the high
 * order bits can be ignored.
 */
ieaddr_t
to_ieaddr(es, cp)
	struct ie_softc *es;
	caddr_t cp;
{
	union {
		int	n;
		char	c[4];
	} a, b;

#ifdef sun3
#ifdef DEBUG
	if ((es->es_type != IE_TE) &&
	    (cp < (char *)0x0F000000 || cp >= (char *)0x10000000)) {
		/* printf("Bad ptr to_ieaddr(%x)\n", cp); */
		;  asm(" .word 0xFFFF") ; ;
	}
#endif DEBUG
#endif sun3
	switch (es->es_type) {
	case IE_MB:
	case IE_TE:
		a.n = cp - (caddr_t)es;
		break;
	case IE_OB:
		a.n = (int)cp;
		break;
	default:
		break;
	}
	b.c[0] = a.c[3];
	b.c[1] = a.c[2];
	b.c[2] = a.c[1];
	b.c[3] = 0;
	return (b.n);
}

/*
 * Convert a CPU virtual address into a 16-bit offset for the Ethernet
 * chip.
 *
 * This is the same for Onboard and Multibus, since the offset is based
 * on the absolute address supplied in the initial system configuration
 * block -- which we customize for Multibus or Onboard.
 */
ieoff_t
to_ieoff(es, addr)
	register struct ie_softc *es;
	caddr_t addr;
{
	union {
		short	s;
		char	c[2];
	} a, b;

	a.s = (short)(addr - (caddr_t)es);
	b.c[0] = a.c[1];
	b.c[1] = a.c[0];
	return (b.s);
}

ieint_t
from_ieint(n)
	short n;
{
	union {
		short	s;
		char	c[2];
	} a, b;

	a.s = n;
	b.c[0] = a.c[1];
	b.c[1] = a.c[0];
	return (b.s);
}

/*
 * Set default configuration parameters
 * As spec'd by Intel, except acloc == 1 for header in data
 */
iedefaultconf(ic)
	register struct ieconf *ic;
{
	bzero((caddr_t)ic, sizeof (struct ieconf));
	ic->ieconf_cb.iec_cmd = IE_CONFIG;
	ic->ieconf_bytes = 12;
	ic->ieconf_fifolim = 8;
	ic->ieconf_pream = 2;		/* 8 byte preamble */
	ic->ieconf_alen = 6;
	ic->ieconf_acloc = 1;
	ic->ieconf_space = 96;
	ic->ieconf_slttmh = 512 >> 8;
	ic->ieconf_minfrm = 64;
	ic->ieconf_retry = 15;
}

/*
 * Close down intel ethernet device.
 * On the Model 50, we reset the chip and take it off the wire, since
 * it is sharing main memory with us (occasionally reading and writing),
 * and most programs don't know how to deal with that -- they just assume
 * that main memory is theirs to play with.
 */
ieclose(sip)
	struct saioreq *sip;
{
	register struct ie_softc *es = (struct ie_softc *) sip->si_devdata;

	if (es->es_type == IE_OB) {
		*es->es_obie = obie_reset;
	}
}

iestats()
{
	printf("ie:  ipackets %d opackets %d collis %d defer %d crcerrs %d alnerrs %d rscerrs %d orunerrs %d urunerrs %d\n",
		stats.ipackets,
		stats.opackets,
		stats.collis,
		stats.defer,
		stats.crcerrs,
		stats.alnerrs,
		stats.rscerrs,
		stats.orunerrs,
		stats.urunerrs);
}

/*
 * used to store controller address so we only have to pass around
 * a pointer to ie_softc
 */
ie_putval(val, where)
{
	register u_short *cp;
	union {
		u_int 	n;
		u_short	s[2];
	} a;

	a.n = (u_int)val;
	cp = (u_short *)(where);
	cp[0] = a.s[0];
	cp[1] = a.s[1];
}

ie_getval(where)
{
	register u_short *cp;
	union {
		u_int 	n;
		u_short	s[2];
	} a;
	register int x;

	cp = (u_short *)(where);
	a.s[0] = cp[0];
	a.s[1] = cp[1];
	x = a.n;
	return (x);
}

#if defined(sun4)
/*
 * map sunray static RAM to IOC_IEDESCR_ADDR.
 * set the tags and zero out the descriptor RAM.
 */
sunray_ioc_init()
{
	register u_long *p;
	register u_long tag;

	/*
	 * map in tags and initialize them
	 * we use the flush virtual address temporarily
	 * since we normally don't touch the tags or data
	 * once the i/o cache is initialized
	 */
	setpgmap((addr_t)IOC_FLUSH_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCDATA_ADDR));

	/* zero both data and tag */
        for (p = (u_long *)IOC_FLUSH_ADDR;
	     (u_long)p != IOC_FLUSH_ADDR+0x2000;
	     p++)
                *p = 0;

	/*
	 * Initialize the ram and tags we are going to use
	 * to hold the ethernet decriptors.
	 */
	setpgmap((addr_t)IOC_IEDESCR_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_DESCR_DATA_ADDR));

	/* zero descriptor data */
	for (	p = (u_long *)IOC_IEDESCR_ADDR;
		(u_long)p < IOC_IEDESCR_ADDR + 0x1000;
		p++ )
			*p = 0;

	/*
	 * Initialize descriptor tags,
	 * Bit 0 is the modified bit, bit 1 is the valid bit.
	 * Bits 2-4 aren't used and the rest are virtual address bits.
	 */
	for ( p = (u_long *)(IOC_IEDESCR_ADDR+0x1000), tag = IOC_IEDESCR_ADDR+3;
		(u_long)p < IOC_IEDESCR_ADDR + PAGESIZE; ) {
			*p = tag;
			p++;
			if ((p != (u_long *)(IOC_IEDESCR_ADDR+0x1000)) &&
			    (((u_long)p%32) == 0))
				tag += IOC_LINESIZE;
	}

	/* map in flush page for i/o cache */
	setpgmap((addr_t)IOC_FLUSH_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCFLUSH_ADDR));
	
	/* XXX - for debugging only,  remove later */
	setpgmap((addr_t)IOC_DATA_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCDATA_ADDR));

	/* turn on the i/o cache */
	sys_enable(ENA_IOCACHE);
}
#define SUNRAY_IE_DMA	(1<<23 | 1<<15)		/* 0x00808000 */
/*
 * On a Sunray, if the 82586 issues a VA with both these
 * bits set, and the pg_ioc bit is set in the pte, then
 * the access will go to the on board descriptor RAM, not
 * the IO Cached buffer in main memory.
 * Since rbufs and xbuf don't fit in descriptor RAM
 * and they must be IO Cached on Sunray,
 * we must put them in memory address where both
 * these bits are not simultaneously set.
 */

char *
alloc_sunray_page()
{
        char *addr;
	int max;
#if defined(IOC_RBUF)
	register u_int *p;
	struct pte pte;

	p = (u_int *)&pte;
#endif IOC_RBUF

#define MAX_WASTE 4	/* 4 consecutive pages have VA[15] set */

	for(max = 0; max <= MAX_WASTE; ++max)
	{
		addr = resalloc(RES_DMAMEM, PAGESIZE);
		if(((u_int)addr & SUNRAY_IE_DMA) != SUNRAY_IE_DMA)
			break;	/* address ok */
	}
	if(max > MAX_WASTE)
		panic("ie: can't allocate legal sunray ethernet buffer.");
#if defined(IOC_RBUF)
	*p = getpgmap(addr);
	pte.pg_ioc = 1;
	setpgmap(addr, *p);
#endif IOC_RBUF
	return(addr);
}

#ifdef DEBUG
/*
 * alloc_sanity()
 *
 * Sanity check addresses of xbuf and rbufs
 * to make sure that 586 can reach them,
 * and in the case of Sunray, that the VA's are legal
 */
alloc_sanity(es)
register struct ie_softc *es;
{
	u_int death = 0;
	int i;

	if(id.id_machine == IDM_SUN4_SUNRAY)
	{

		/* Sanity check Sunray addresses */
		if(((u_int)es->es_xbuf & SUNRAY_IE_DMA) == SUNRAY_IE_DMA)
		{
			printf("alloc_sanity: xbuf = 0x%x\n", es->es_xbuf);
			death++;
		}
		for (i = 0; i < IENRBUF ; ++i)
		{
			if(((u_int)es->es_rbufs[i] & SUNRAY_IE_DMA) == SUNRAY_IE_DMA)
			{
				printf("alloc_sanity: es_rbufs[%d]=0x%x\n",
							i, es->es_rbufs[i]);
				death++;
			}
		}
		if(death)
			panic("alloc_sanity: illegal Sunray 85286 buffers");
	}
	/* sanity check that the 82586 can actually reach the buffers */
	if(((u_int)(es->es_xbuf) & IEDMABASE) != IEDMABASE)
	{
		printf("alloc_sanity: es_xbuf=0x%x\n", es->es_xbuf);
		death++;
	}
	for (i = 0; i < IENRBUF ; ++i)
	{
		if(((u_int)(es->es_rbufs[i]) & IEDMABASE) != IEDMABASE)
		{
			printf("alloc_sanity: rbufs[%d]=0x%x\n",
					i, es->es_rbufs[i]);
			death++;
		}
	}
	if(death)
		panic("alloc_sanity: buffers can't be addressed by 82586.");
}
#endif  DEBUG

/*
 * ieallocbufs()
 * Allocates xbuf and rbufs, and initializes
 * es->es_xbuf and es->es_rbufs[] appropriately.
 *
 * Knows about Sunray IO Cache requirements.
 * 
 * no return value.
 */

void
ieallocbufs(es)
register struct ie_softc *es;
{
	int rbuf;
	char *addr;

	if(es->es_xbuf) /* been here already */
	{
		return;
	}

	/* Receive Buffers */

#define RBUFPERPAGE	(PAGESIZE / IERBUFSIZ)

	for (rbuf = 0;  rbuf < IENRBUF; rbuf += RBUFPERPAGE)
	{
		int i;

		if(id.id_machine == IDM_SUN4_SUNRAY)
			addr = alloc_sunray_page();
		else
			addr = resalloc(RES_DMAMEM, PAGESIZE);
		for(i = 0; (i < RBUFPERPAGE) && ((rbuf + i) < IENRBUF); ++i)
		{
			es->es_rbufs[rbuf + i] = addr + (i * IERBUFSIZ);
			if(verbosemode)
				printf("  ieallocbufs() es_rbufs[%d]=0x%x\n",
					rbuf + i, es->es_rbufs[rbuf + i]);
		}
	}

	/* Transmit Buffer */
	if(id.id_machine == IDM_SUN4_SUNRAY)
	{
#if !defined(IOC_RBUF)
		register u_int *p;
		struct pte pte;

		p = (u_int *)&pte;
#endif !IOC_RBUF
		es->es_xbuf =  alloc_sunray_page();

#if !defined(IOC_RBUF)
		*p = getpgmap(es->es_xbuf);
		pte.pg_ioc = 1;
		setpgmap(es->es_xbuf, *p);
#endif !IOC_RBUF
	}
	else
		es->es_xbuf =  resalloc(RES_DMAMEM, PAGESIZE);

#if defined(DEBUG)
	alloc_sanity(es);
#endif DEBUG
}
#endif sun4
#endif !sun4c	/* top of file */
