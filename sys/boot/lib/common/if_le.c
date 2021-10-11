#ifndef LEBOOT
#ifndef lint
static        char sccsid[] = "@(#)if_le.c 1.1 92/07/30 SMI";
#endif
#endif

#ifndef sun2

/*
 * For Sun-3/25 debugging we want DEBUG on.  For production, turn it off.
 */
/* #define DEBUG */

/* #define DEBUG1 */

#define LANCEBUG 1	/* Rev C Lance chip bug */

/* 
 * Parameters controlling TIMEBOMB action: times out if chip hangs.
 *
 */
#define	TIMEBOMB	1000000	/* One million times around is all... */

/*
 * Sun Lance Ethernet Controller interface
 */
#include <stand/saio.h>
#include <stand/param.h>
#include <sys/socket.h>
#include <sunif/if_lereg.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <mon/idprom.h>
#include <mon/cpu.map.h>
#ifdef sun3x
#include <mon/cpu.addrs.h>
#endif

/* Determine whether we are PROM or not. */
#ifdef LEBOOT
#define PROM
#endif LEBOOT

int	lancexmit(), lancepoll(), lancereset(), myetheraddr(), lancestats();

struct saif leif = {
	lancexmit,
	lancepoll,
	lancereset,
	myetheraddr,
	lancestats
};

#define	LANCERBUFSIZ	1600
#define	LANCETBUFSIZ	1600
#define	LANCERBUFFS	8	/* how many receive buffs */

struct lance_softc {
	char		es_scrat[PROTOSCRATCH];	/* work space for nd */
	struct le_device    	*es_lance;	/* Device register address */
	struct ether_addr	es_enaddr;	/* Our Ethernet address */
	struct le_init_block	es_ib;		/* Initialization block */
	struct le_md		*es_rmdp[LANCERBUFFS];	/* Receive Desc Ring
					 		ptrs */
	struct le_md		*es_tmdp;	/* Transmit Desc Ring ptr */
	u_char			es_rbuf[LANCERBUFFS][LANCERBUFSIZ]; /* Receive
							 Buffers */
#ifndef PROM
	u_char			es_tbuf[LANCETBUFSIZ];	/* Transmit Buffer */
#endif	PROM
	int			es_next_rmd;	/* Next descriptor in ring */
	/*
	 * The transmit and receive ring descriptors.  The es_{r,t}mdp
	 * entries point into these arrays.  We allocate an extra one of
	 * each so that we can guarantee 8-byte alignment.  (That is, we
	 * don't necessarily point locations an integral number of le_mds
	 * from the start of the arrays.
	 */
	struct le_md		es_rmd[LANCERBUFFS+1],
				es_tmd[2];
};


/*
 * Need to initialize the Ethernet control reg to:
 *	Reset is active
 *	Loopback is NOT active
 *	Interrupt enable is not active.
 */
#ifdef sun3x
u_long lancestd[] = {pa_AMD_ETHER};
#else
u_long lancestd[] = {VIOPG_AMD_ETHER << BYTES_PG_SHIFT};
#endif

struct devinfo lanceinfo = {
	sizeof (struct le_device),
	sizeof (struct lance_softc),
	0,				/* Local bytes (we use dma) */
	1,				/* Standard addr count */
	lancestd,			/* Standard addrs */
	MAP_OBIO,			/* Device type */
	0,				/* transfer size handled by ND */
};

int lanceprobe(), xxboot(), lanceopen(), lanceclose(), etherstrategy();
int nullsys();

struct boottab ledriver = {
	"le", lanceprobe, xxboot, lanceopen, lanceclose,
	etherstrategy, "le: Sun/Lance Ethernet", &lanceinfo,
};

extern struct ether_addr etherbroadcastaddr;

/* Statistics counters for lancestats() */
static struct {
	int	ipackets;
	int	opackets;
	int	collis;
	int	defer;
	int	crcerrs;
	int	framerrs;
	int	missed;
	int	ofloerrs;
	int	ufloerrs;
} stats;

/*
 * Probe for device.
 * Must return -1 for failure for monitor probe
 */
/*ARGSUSED*/
int
lanceprobe(sip)
	struct saioreq *sip;
{
	struct idprom id;

	if (IDFORM_1 == idprom(IDFORM_1, &id) &&
	       (id.id_machine == IDM_SUN3_M25 || 
		id.id_machine == IDM_SUN3_F ||
		id.id_machine == IDM_SUN3X_HYDRA ||
		id.id_machine == IDM_SUN4_STINGRAY))
		return (0);
	else
		return (-1);
}

/*
 * Open Lance Ethernet nd connection, return -1 for errors.
 */
lanceopen(sip)
	struct saioreq *sip;
{
	register int result;

#ifdef DEBUG1
	printf("le: lanceopen[\n");
#endif
	sip->si_sif = &leif;
	if ( lanceinit(sip) || (result = etheropen(sip)) < 0 ) {
		lanceclose(sip);		/* Make sure we kill chip */
#ifdef DEBUG1
		printf("le: lanceopen --> -1\n");
#endif
		return (-1);
	}
#ifdef DEBUG1
	printf("le: lanceopen --> %x\n", result);
#endif
	return (result);
}

/*
 * Set up memory maps and Ethernet chip.
 * Returns 1 for error (after printing message), 0 for ok.
 */
/*ARGSUSED*/
int
lanceinit(sip)
	struct saioreq *sip;
{
	register struct lance_softc *es;
	struct idprom id;
	
	/* 
	 * Our locals were obtined from DMA space, now put the locals
	 * pointer in the standard place.  This is OK since close()
	 * will only deallocate from devdata if its size in devinfo is >0.
	 */
	es = (struct lance_softc *)sip->si_dmaaddr;
	sip->si_devdata = (caddr_t)es;
	es->es_lance = (struct le_device *) sip->si_devaddr;

	if (IDFORM_1 != idprom(IDFORM_1, &id)) {
		printf("le: No ID PROM\n");
		return 1;
	}

	return lancereset(es, sip);
}

/*
 * Basic Lance initialization
 * Returns 1 for error (after printing message), 0 for ok.
 */
/*ARGSUSED*/
int
lancereset(es, sip)
	register struct lance_softc *es;
	struct saioreq *sip;
{
	register struct le_device *le = es->es_lance;
	register struct le_init_block *ib = &es->es_ib;
	struct idprom id;
	int timeout = TIMEBOMB;
	int	i;

#ifdef DEBUG1
	printf("le: lancereset(%x, %x)\n", es, sip);
#endif

	/* Reset the chip */
	le->le_rap = LE_CSR0;
	le->le_csr = LE_STOP;

	/* Perform the basic initialization */
	
	/* Construct the initialization block */
	bzero((caddr_t)&es->es_ib, sizeof (struct le_init_block));

	/* Leave the mode word 0 for normal operating mode */

	myetheraddr(&es->es_enaddr);

	/* Oh, for a consistent byte ordering among processors */
	ib->ib_padr[0] = es->es_enaddr.ether_addr_octet[1];
	ib->ib_padr[1] = es->es_enaddr.ether_addr_octet[0];
	ib->ib_padr[2] = es->es_enaddr.ether_addr_octet[3];
	ib->ib_padr[3] = es->es_enaddr.ether_addr_octet[2];
	ib->ib_padr[4] = es->es_enaddr.ether_addr_octet[5];
	ib->ib_padr[5] = es->es_enaddr.ether_addr_octet[4];

	/* Leave address filter 0 -- we don't want Multicast packets */

	/*
	 * Set up transmit and receive ring pointers,
	 * taking the 8-byte alignment requirement into account.
	 */
	es->es_rmdp[0] = (struct le_md *) ((u_long)(&es->es_rmd[0]) & ~07) + 1;
	for (i = 1; i < LANCERBUFFS; i++)
		es->es_rmdp[i] = es->es_rmdp[i-1] + 1;
	es->es_tmdp    = (struct le_md *) ((u_long)(&es->es_tmd[0]) & ~07) + 1;

	ib->ib_rdrp.drp_laddr = (long)es->es_rmdp[0];
	ib->ib_rdrp.drp_haddr = (long)es->es_rmdp[0] >> 16;
	ib->ib_rdrp.drp_len  = 3;   /* 2 to the 1 power = 2 */
	
	ib->ib_tdrp.drp_laddr = (long)es->es_tmdp;
	ib->ib_tdrp.drp_haddr = (long)es->es_tmdp >> 16;
	ib->ib_tdrp.drp_len  = 0;   /* 2 to the 0 power = 1 */

	/* Clear all the descriptors */
	bzero((caddr_t)es->es_rmdp[0], LANCERBUFFS * sizeof (struct le_md));
	bzero((caddr_t)es->es_tmdp, sizeof (struct le_md));

	/* Give the init block to the chip */
	le->le_rap = LE_CSR1;	/* select the low address register */
	le->le_rdp = (long)ib & 0xffff;

	le->le_rap = LE_CSR2;	/* select the high address register */
	le->le_rdp = ((long)ib >> 16) & 0xff;

	if (IDFORM_1 != idprom(IDFORM_1, &id)) {
		printf("le: No ID PROM\n");
		return 1;
	}

	le->le_rap = LE_CSR3;	/* Bus Master control register */
	if (id.id_machine == IDM_SUN4_STINGRAY || id.id_machine == IDM_SUN3X_HYDRA )
		le->le_rdp = LE_BSWP+LE_ACON+LE_BCON;
	else
		le->le_rdp = LE_BSWP;

	le->le_rap = LE_CSR0;	/* main control/status register */
	le->le_csr = LE_INIT;

	while( ! (le->le_csr & LE_IDON) ) {
		if (timeout-- <= 0) {
		    printf("le: cannot initialize\n");
		    return (1);
		}
	}
	le->le_csr = LE_IDON;	/* Clear the indication */

	/* Hang out the receive buffers */
	es->es_next_rmd = 0;

	for (i = 0; i < LANCERBUFFS; i++)
		install_buf_in_rmd(es->es_rbuf[i], es->es_rmdp[i]);

	le->le_csr = LE_STRT;

#ifdef DEBUG1
	printf("le: lancereset returns OK\n");
#endif
	return 0;		/* It all worked! */
}

install_buf_in_rmd(buffer, rmd)
	u_char *buffer;
	register struct le_md *rmd;
{
	rmd->lmd_ladr = (u_short)buffer;
	rmd->lmd_hadr = (long)buffer >> 16;
	rmd->lmd_bcnt = -LANCERBUFSIZ;
	rmd->lmd_mcnt = 0;
	rmd->lmd_flags = LMD_OWN;
}

/*
 * Transmit a packet.
 * If possible, just points to the packet without copying it anywhere.
 */
lancexmit(es, buf, count)
	register struct lance_softc *es;
	char *buf;
	int count;
{
	register struct le_device *le = es->es_lance;
	struct le_md *tmd = es->es_tmdp; /* Transmit Msg. Descriptor */
	caddr_t tbuf;
	int timeout = TIMEBOMB;

#ifdef DEBUG2
	printf( "xmit np_blkno %x\n",
		((struct ndpack *)(buf+14))->np_blkno);
#endif
	/*
	 * We assume the buffer is in an area accessible by the chip.
	 * The caller of xmit is currently ndxmit(), which only sends
	 * an nd structure, which we happen to know is allocated in the
	 * right area (in fact, it's part of the struct nd which
	 * is the first thing in our own es structure).  If we wish to
	 * use xmit for some other purpose, the buffer might not be
	 * accessible by the chip, so to be general we ought to copy
	 * into some accessible place.  HOWEVER, PROM space is really tight,
	 * so this generality is not free.
	 */
#ifdef PROM
	tbuf = buf;
#else  PROM
/* FIXME, constant address masks here! */
#ifdef sun3x
        if ( ((int)buf & 0xFF000000) == 0xFF000000) { /* we can point to it
*/
#else
	if ( ((int)buf & 0x0F000000) == 0x0F000000) { /* we can point to it */
#endif sun3x
	    tbuf = buf;
	} else {
	    tbuf = (caddr_t)es->es_tbuf;
	    bcopy((caddr_t)buf, tbuf, count);
	}
#endif PROM
	
	tmd->lmd_hadr = (int)tbuf >> 16;
	tmd->lmd_ladr = (u_short) tbuf;
	tmd->lmd_bcnt = -count;
	
#ifdef notdef
	if (tmd->lmd_bcnt < -MINPACKET)
	    tmd->lmd_bcnt = -MINPACKET;
#endif
	tmd->lmd_flags3 = 0;
	tmd->lmd_flags = LMD_STP | LMD_ENP | LMD_OWN;
	
	le->le_rap = LE_CSR0;
	le->le_csr = LE_TDMD;

	do {
	    if ( le->le_csr & LE_TINT ) {
		le->le_csr = LE_TINT; /* Clear interrupt */
		break;
	    }
	} while ( --timeout > 0);

	if (tmd->lmd_flags & TMD_DEF)
		stats.defer++;
	if (tmd->lmd_flags & TMD_ONE)
		stats.collis++;
	else if (tmd->lmd_flags & TMD_MORE)
		stats.collis += 2;

	if (tmd->lmd_flags3 & TMD_UFLO)
		stats.ufloerrs++;

	if (tmd->lmd_flags3 & TMD_LCAR)
		printf("le: Ethernet cable problem\n");

	if ( (tmd->lmd_flags & LMD_ERR)
	||   (tmd->lmd_flags3 & TMD_BUFF)
	||   (timeout <= 0) ) {
#ifdef DEBUG
		printf("le: xmit failed - tmd1 flags %x tmd3 %x\n",
	       		tmd->lmd_flags, tmd->lmd_flags3);
#endif
		return (1);
	}

	stats.opackets++;

	return (0);
}

int
lancepoll(es, buf)
	register struct lance_softc *es;
	char *buf;
{
	register struct le_device *le = es->es_lance;
	register struct le_md *rmd;
	register struct ether_header *header;
	int length;
#ifdef never
	register struct le_md *next_rmd;
#endif never

#ifdef DEBUG1
	printf("le: poll\n");
#endif
	le->le_rap = LE_CSR0;
	if ( ! (le->le_csr & LE_RINT)  ) {
		rmd = es->es_rmdp[es->es_next_rmd];
		if ((rmd->lmd_flags & LMD_OWN) == 0) {
#ifdef never
			printf ("le: one waiting 0x%x\n",
			    rmd->lmd_mcnt - 4);
#endif never
			goto cheat;
		}
		return (0);		/* No packet yet */
	}

cheat:
	stats.ipackets++;
	if (le->le_csr & LE_MISS) {
#ifdef DEBUG1
		printf ("le: missed packet\n");
#endif DEBUG
		stats.missed++;
		le->le_csr |= LE_MISS;
	}

#ifdef DEBUG1
	printf("le: received packet\n");
#endif

	le->le_csr = LE_RINT; 		/* Clear interrupt */

	rmd = es->es_rmdp[es->es_next_rmd];

	if (rmd->lmd_flags & RMD_OFLO)
		stats.ofloerrs++;
	else {
		if ((rmd->lmd_flags & RMD_FRAM) && (rmd->lmd_flags & LMD_ENP))
			stats.framerrs++;
		if ((rmd->lmd_flags & RMD_CRC) && (rmd->lmd_flags & LMD_ENP))
			stats.crcerrs++;
	}

	if ( (rmd->lmd_flags & ~RMD_OFLO) != (LMD_STP|LMD_ENP) ) {
#ifdef DEBUG
		printf("Receive packet error - rmd flags %x\n",rmd->lmd_flags);
#endif
		length = 0;
		goto restorebuf;
	}

	/* Get input data length and a pointer to the ethernet header */

	length = rmd->lmd_mcnt - 4;	/* don't count the 4 CRC bytes */
	header = (struct ether_header *)es->es_rbuf[es->es_next_rmd];

#ifdef LANCEBUG
	/*
	 * Check for unreported packet errors.  Rev C of the LANCE chip
	 * has a bug which can cause "random" bytes to be prepended to
	 * the start of the packet.  The work-around is to make sure that
	 * the Ethernet destination address in the packet matches our
	 * address.
	 */
#define ether_addr_not_equal(a,b)	\
	(  ( *(short *)(&a.ether_addr_octet[0]) != \
	     *(short *)(&b.ether_addr_octet[0]) )  \
	|| ( *(short *)(&a.ether_addr_octet[2]) != \
	     *(short *)(&b.ether_addr_octet[2]) )  \
	|| ( *(short *)(&a.ether_addr_octet[4]) != \
	     *(short *)(&b.ether_addr_octet[4]) )  \
	)

    	if( ether_addr_not_equal(header->ether_dhost, es->es_enaddr)
	&&  ether_addr_not_equal(header->ether_dhost, etherbroadcastaddr) ) {
		printf("le: LANCE Rev C Extra Byte(s) bug; Packet punted\n");
		length = 0;
		/* Don't return directly; restore the buffer first */
	}
#endif LANCEBUG

#ifdef DEBUG2
    	if( header->ether_dhost.ether_addr_octet[0] == 0xff )
		printf("Broadcast packet\n");
	else	printf("recv np_blkno %x\n",
	        	((struct ndpack *)(buf+14))->np_blkno);
#endif
	
	/* Copy packet to user's buffer */
	if ( length > 0 )
		bcopy((caddr_t)header, buf, length);

restorebuf:
	rmd->lmd_mcnt = 0;
	rmd->lmd_flags = LMD_OWN;

	/* Get ready to use the other buffer next time */
	/* What about errors ? */
	es->es_next_rmd =
	    (es->es_next_rmd == (LANCERBUFFS - 1)) ? 0 : es->es_next_rmd + 1;
#ifdef never
	next_rmd = es->es_rmdp[es->es_next_rmd];
	if ((next_rmd->lmd_flags & LMD_OWN) == 0)
		printf ("le: one waiting\n");
#endif never

	return (length);
}

/*
 * Close down Lance Ethernet device.
 * On the Model 25, we reset the chip and take it off the wire, since
 * it is sharing main memory with us (occasionally reading and writing),
 * and most programs don't know how to deal with that -- they just assume
 * that main memory is theirs to play with.
 */
lanceclose(sip)
	struct saioreq *sip;
{
	struct lance_softc *es = (struct lance_softc *) sip->si_devdata;
	struct le_device *le = es->es_lance;

	/* Reset the chip */
	le->le_rap = LE_CSR0;
	le->le_csr = LE_STOP;
}

lancestats()
{
	printf("le:  ipackets %d opackets %d collis %d defer %d crcerrs %d framerrs %d missed %d ofloerrs %d ufloerrs %d\n",
		stats.ipackets,
		stats.opackets,
		stats.collis,
		stats.defer,
		stats.crcerrs,
		stats.framerrs,
		stats.missed,
		stats.ofloerrs,
		stats.ufloerrs);
}

#endif !sun2
