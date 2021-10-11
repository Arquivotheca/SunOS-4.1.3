#ident	"@(#)scsi_resource.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 Sun Microsystems, Inc.
 */

#define	DPRINTF	if (scsi_options & SCSI_DEBUG_LIB) printf

/*
 *	Generic Resource Allocation Routines
 */

#include <scsi/scsi.h>


struct scsi_pkt *
scsi_resalloc(ap, cmdlen, statuslen, dmatoken, callback)
struct scsi_address *ap;
int cmdlen, statuslen;
opaque_t dmatoken;
int (*callback)();
{
	register struct scsi_pkt *pktp;
	register struct scsi_transport *tranp;

	/*
	 * The first part of the address points to
	 * an array of transport function points
	 */

	tranp = (struct scsi_transport *) ap->a_cookie;

	pktp = (*tranp->tran_pktalloc)(ap, cmdlen, statuslen, callback);

	if (pktp == (struct scsi_pkt *) 0) {
		if (callback == SLEEP_FUNC) {
			panic("scsi_resalloc: No packet after sleep");
			/*NOTREACHED*/
		}
	} else if (dmatoken != (opaque_t) 0) {
		if ((*tranp->tran_dmaget)(pktp, dmatoken, callback) == NULL) {
			if (callback == SLEEP_FUNC) {
				panic("scsi_resalloc: No dma after sleep");
				/*NOTREACHED*/
			}
			/*
			 * if we didn't get dma resources in this function,
			 * free up the packet resources.
			 */
			(*tranp->tran_pktfree)(pktp);
			pktp = (struct scsi_pkt *) 0;
		}
	}
	return (pktp);
}

struct scsi_pkt *
scsi_pktalloc(ap, cmdlen, statuslen, callback)
struct scsi_address *ap;
int cmdlen, statuslen;
int (*callback)();
{
	struct scsi_transport *tranp = (struct scsi_transport *) ap->a_cookie;
	return ((*tranp->tran_pktalloc)(ap, cmdlen, statuslen, callback));
}

struct scsi_pkt *
scsi_dmaget(pkt, dmatoken, callback)
struct scsi_pkt *pkt;
opaque_t dmatoken;
int (*callback)();
{
	struct scsi_transport *tranp;

	if (dmatoken == (opaque_t) NULL) {
		return ((struct scsi_pkt *) NULL);
	}
	tranp = (struct scsi_transport *) pkt->pkt_address.a_cookie;
	return ((*tranp->tran_dmaget)(pkt, dmatoken, callback));
}


/*
 *	Generic Resource Deallocation Routines
 */

void
scsi_dmafree(pkt)
struct scsi_pkt *pkt;
{
	struct scsi_transport *tranp =
		(struct scsi_transport *) pkt->pkt_address.a_cookie;
	(*tranp->tran_dmafree)(pkt);
}

void
scsi_resfree(pkt)
struct scsi_pkt *pkt;
{
	struct scsi_transport *tranp =
		(struct scsi_transport *) pkt->pkt_address.a_cookie;
	/*
	 * Free DMA resources if any need to be freed.
	 */

	(*tranp->tran_dmafree)(pkt);

	/*
	 * free packet.
	 */
	(*tranp->tran_pktfree)(pkt);
}

/*
 *	Standard Resource Allocation/Deallocation Routines
 */

/*
 * When Host Adapters don't want to supply their own resource allocation
 * routines, and they can live with certain assumptions about DVMA,
 * these routines are stuffed into their scsi_transport structures
 * which they then export to the library.
 */

/*
 * Local resource management data && defines
 */

#if	defined(sun4c) || defined(sun4m)
#define	DMA	dvmamap
#else	defined(sun4c) || defined(sun4m)
#define	DMA	mb_hd.mh_map
#endif	defined(sun4c) || defined(sun4m)

int scsi_ncmds, scsi_spl;
static int sfield();
static int scsi_cmdwake = 0;
static struct scsi_cmd *scsibase;

/*
 * This code shamelessly stolen from mb_machdep.c.
 * Full attribution and credit to John Pope.
 *
 * Drivers queue function pointers here when they have to wait
 * for resources. Subsequent calls from the same driver that are
 * forced to wait need not queue up again, as the function pointed
 * to will run the driver's internal queues until done or space runs
 * out again.
 *
 * The difficulty is that we may block on two different things here:
 * packet allocation itself, and DVMA mapping resources associated with
 * the packet. If we cannot get a packet, no problem. If we can get
 * or have already gotten a packet, but cannot get DVMA mapping resources,
 * things get a bit sticky. The DVMA mapping callback routine expects
 * us to return DVMA_RUNOUT if DVMA allocation fails again- but *our*
 * callback to the target driver may return failure not due to DVMA
 * allocation failure (again) but packet allocation failure instead.
 * In either case, we assume that *we* have appropriately re-queued
 * the caller, but we have to know what kind of allocation failure
 * it was in order to appropriately notify the sentinel in the DVMA
 * map allocation callback code.
 *
 * In other words, the SCSA specification screwed this up. Oh well.
 *
 * The magic number for SCQLEN was picked because:
 *
 *	a) That seems a reasonable limit for the number of separate and
 *	   distinct target drivers that can latch up waiting for resources.
 *
 *	b) it is a power of two, to make the cycle easier.
 *
 */

#define	SCQLEN		0x8
#define	SCQLENMASK	0x7

struct scq {
	u_int	qlen;
	u_int	qstore;
	u_int	qretrv;
	u_int	ncalls;
	u_int	incallback;
	func_t funcp[SCQLEN];
};

static struct scq scpq, scdq;
static void scq_store();		/* store element */
static func_t scq_retrieve();		/* retrieve element */


/*
 * resource initializer
 */

void
scsi_rinit()
{
	if (scsibase != (struct scsi_cmd *) 0) {
		return;
	}

	/*
	 * start with a minimum of twice the per_dev requirement
	 */
	scsi_addcmds(scsi_ncmds_per_dev << 1);

	if (scsibase == (struct scsi_cmd *) 0) {
		panic("No space for scsi command structures");
		/*NOTREACHED*/
	}
	scpq.qretrv = scdq.qretrv = SCQLEN-1;
	scpq.incallback = scdq.incallback = 0;
}

void
scsi_addcmds(ncmds)
int ncmds;
{
	register s, i;
	register struct scsi_cmd *sp;

	sp = (struct scsi_cmd *)
	    kmem_zalloc((u_int) sizeof (struct scsi_cmd) * ncmds);

	if (sp == (struct scsi_cmd *) 0) {
		return;
	}
	scsi_ncmds += ncmds;
	s = splr(scsi_spl);
	for (i = 0; i < ncmds-1; i++)
		sp[i].cmd_pkt.pkt_ha_private = (opaque_t) &sp[i+1];
	sp[ncmds-1].cmd_pkt.pkt_ha_private = (opaque_t) scsibase;
	scsibase = sp;
	(void) splx(s);
}

/*
 * pktalloc
 */

struct scsi_pkt *
scsi_std_pktalloc(ap, cmdlen, statuslen, callback)
struct scsi_address *ap;
int cmdlen, statuslen;
int (*callback)();
{
	register struct scsi_cmd *cmd;
	register s;
	register caddr_t cdbp, scbp;

	cdbp = scbp = (caddr_t) 0;

	s = splr(scsi_spl);
	cmd = scsibase;
	for (;;) {
		if (cmd == (struct scsi_cmd *) 0) {
			if (callback == SLEEP_FUNC) {
				if (servicing_interrupt()) {
					panic("scsi_std_pktalloc");
					/*NOTREACHED*/
				}
				scsi_cmdwake++;
				(void) sleep((caddr_t)&scsibase, PRIBIO);
				cmd = scsibase;

				/*
				 * around the top again...
				 */
				continue;

			} else if (callback != NULL_FUNC) {
				scq_store(&scpq, callback);
			}
			break;
		}

		if (cmdlen > CDB_SIZE) {
			cdbp = kmem_zalloc((unsigned) cmdlen);
			if (cdbp == (caddr_t) 0) {
				cmd = (struct scsi_cmd *) 0;
				continue;
			}
		}

		if (statuslen > STATUS_SIZE) {
			scbp = kmem_zalloc((unsigned) statuslen);
			if (scbp == (caddr_t) 0) {
				if (cdbp != (caddr_t) 0) {
					(void) kmem_free_intr (cdbp,
						(unsigned) cmdlen);
				}
				cmd = (struct scsi_cmd *) 0;
				continue;
			}
		}

		scsibase = (struct scsi_cmd *) cmd->cmd_pkt.pkt_ha_private;
		break;
	}

	/*
	 * We can safely drop priority now
	 */
	(void) splx(s);

	if (cmd != (struct scsi_cmd *) 0) {

		bzero ((caddr_t)cmd, sizeof (struct scsi_cmd));

		if (cdbp != (caddr_t) 0) {
			cmd->cmd_pkt.pkt_cdbp = (opaque_t) cdbp;
			cmd->cmd_flags |= CFLAG_CDBEXTERN;
		} else {
			cmd->cmd_pkt.pkt_cdbp = (opaque_t) &cmd->cmd_cdb[0];
		}

		if (scbp != (caddr_t) 0) {
			cmd->cmd_pkt.pkt_scbp = (opaque_t) scbp;
			cmd->cmd_flags |= CFLAG_SCBEXTERN;
		} else {
			cmd->cmd_pkt.pkt_scbp = (opaque_t) &cmd->cmd_scb[0];
		}

		cmd->cmd_cdblen = cmdlen;
		cmd->cmd_scblen = statuslen;
		cmd->cmd_pkt.pkt_address = *ap;
	}

	return ((struct scsi_pkt *) cmd);
}

/*
 * packet free
 */

void
scsi_std_pktfree(pkt)
struct scsi_pkt *pkt;
{
	register s = splr(scsi_spl);
	register struct scsi_cmd *sp = (struct scsi_cmd *) pkt;

	if (sp->cmd_flags & CFLAG_CDBEXTERN) {
		(void) kmem_free_intr((caddr_t) sp->cmd_pkt.pkt_cdbp,
				(unsigned int) sp->cmd_cdblen);
	}
	if (sp->cmd_flags & CFLAG_SCBEXTERN) {
		(void) kmem_free_intr((caddr_t) sp->cmd_pkt.pkt_scbp,
				(unsigned int) sp->cmd_scblen);
	}
	/*
	 * free the packet.
	 */
	sp->cmd_pkt.pkt_ha_private = (opaque_t) scsibase;
	scsibase = (struct scsi_cmd *) pkt;
	if (scsi_cmdwake) {
		scsi_cmdwake = 0;
		wakeup((caddr_t)&scsibase);
	}
	while (scpq.qlen != 0) {
		register func_t funcp;
		funcp = scq_retrieve(&scpq);
		if ((*funcp)() == 0)
			break;
	}
	(void) splx(s);
}


/*
 *
 * Dma resource allocation
 *
 */
#define	BDVMA	((u_long) &DVMA[0])
#define	EDVMA	((u_long) &DVMA[ctob(dvmasize)])
#define	DVMA_ADDR(addr, count) \
	(((u_long)addr) >= BDVMA && ((u_long)addr) < EDVMA && \
	(((u_long)addr)+count) >= BDVMA && (((u_long)addr)+count-1) < EDVMA)

#ifdef	sun4c

#define	SYS_VRANGE(addr)	((u_int)addr >= (u_int) Sysbase && \
				    (u_int)addr < (u_int) Syslimit)

#define	SYS_VADDR(bp)		(((bp->b_flags & (B_PAGEIO|B_PHYS)) == 0) && \
					SYS_VRANGE(bp->b_un.b_addr))

#endif	sun4c

struct scsi_pkt *
scsi_std_dmaget(pkt, dmatoken, callback)
struct scsi_pkt *pkt;
opaque_t dmatoken;
int (*callback)();
{
	struct buf *bp = (struct buf *) dmatoken;
	struct scsi_cmd *cmd = (struct scsi_cmd *) pkt;

	/*
	 * clear any stale flags
	 */

	cmd->cmd_flags &= ~(CFLAG_DMAKEEP|CFLAG_DMASEND|CFLAG_DMAVALID);

	/*
	 * We assume that if the address is already in the range of
	 * kernel address DVMA..DVMA+ctob(dvmasize) that the mapping has
	 * already been established by someone (so we don't have to).
	 *
	 * If this is the case it is also true that we don't have
	 * release the mapping when we're done (i.e., when scsi_std_dmafree
	 * is called), so we'll mark this mapping to not be released.
	 *
	 * Also, if this is a sun4c, and the I/O is too/from the kernel
	 * heap, we can just use that (on a sun4c, I/O is valid for what-
	 * ever is in context 0 (kernel context)).
	 *
	 */


	if (DVMA_ADDR(bp->b_un.b_addr, bp->b_bcount)) {
		cmd->cmd_mapping = (((u_long)bp->b_un.b_addr)-((u_long)DVMA));
		cmd->cmd_flags |= CFLAG_DMAKEEP;
#ifdef	sun4c
	} else if (SYS_VADDR(bp)) {
		/*
		 * I don't believe that I need to lock the address range
		 * down if it's in the kernel heap.
		 */
		cmd->cmd_mapping = (u_long) bp->b_un.b_addr;
		cmd->cmd_flags |= CFLAG_DMAKEEP;
#endif	sun4c
	} else if (callback == SLEEP_FUNC) {
		cmd->cmd_mapping =
		   mb_mapalloc(DMA, bp, MDR_BIGSBUS, (int (*)())0, (caddr_t)0);
	} else {
#ifdef	TEST_SPLS
		register s, ipl;
		static int last_spl = -1;

		if (last_spl == -1) {
			last_spl = scsi_spl;
		}
		s = splr(last_spl);
		ipl = spltoipl(last_spl) + 1;
		if (ipl > spltoipl(splvm_val)) {
			last_spl = scsi_spl;
		} else
			last_spl = ipltospl(ipl);
#else	TEST_SPLS
		register int s = splr(scsi_spl);
#endif	TEST_SPLS
		/*
		 * If the DVMA wait queue is empty, or we're in the middle
		 * of our own callback (sfield()), call mb_mapalloc for
		 * a mapping. If that fails, store up our caller to be
		 * called back later when DVMA becomes available.
		 *
		 * If the DVMA wait queue is non-empty already, store
		 * up our caller so it can be called later back when DVMA
		 * becomes available.
		 *
		 * Now if our caller had specified NULL_FUNC, we do it
		 * slightly differently- if we don't get resources,
		 * then we arrange to field a dummy callback (that
		 * goes nowhere).
		 */
		if (scdq.incallback || scdq.qlen == 0) {
			cmd->cmd_mapping = mb_mapalloc(DMA, bp,
			    (MB_CANTWAIT | MDR_BIGSBUS),
			    (callback == NULL_FUNC) ? NULL_FUNC : sfield,
			    (caddr_t) 0);
			if (cmd->cmd_mapping == 0) {
				if (callback != NULL_FUNC)
					scq_store(&scdq, (func_t)callback);
				(void) splx(s);
				return ((struct scsi_pkt *) 0);
			}
		} else {
			if (callback != NULL_FUNC)
				scq_store(&scdq, (func_t)callback);
			(void) splx(s);
			return ((struct scsi_pkt *) 0);
		}
		(void) splx(s);
	}
	cmd->cmd_dmacount = bp->b_bcount;
	if ((bp->b_flags & B_READ) == 0)
		cmd->cmd_flags |= CFLAG_DMASEND;
	cmd->cmd_flags |= CFLAG_DMAVALID;
	return ((struct scsi_pkt *) cmd);
}


/*ARGSUSED*/
static int
sfield (arg)
caddr_t arg;
{
	register s = splr(scsi_spl);

	scdq.incallback = 1;

	while (scdq.qlen != 0) {
		register func_t funcp;
		register u_int lastlen;

		/*
		 * Latch up the current queue length,
		 * because scq_retrieve will decrement it.
		 */
		lastlen = scdq.qlen;
		funcp = scq_retrieve(&scdq);
		if ((*funcp)() == 0) {
			/*
			 * The target driver's allocation failed. Why?
			 * If it failed due to packet allocation failure,
			 * we can continue on. If it failed due to DVMA
			 * allocation failure, we have to quit now and
			 * let the mb code know that DVMA has run out
			 * again. If the last dma queue length is less
			 * than or equal to the now current dma queue
			 * length, then the allocation failure was
			 * due to DVMA running out again.
			 */
			if (lastlen > scdq.qlen) {
				continue;
			}
			scdq.incallback = 0;
			(void) splx(s);
			return (DVMA_RUNOUT);
		}
	}
	scdq.incallback = 0;
	(void) splx(s);
	return (0);
}


void
scsi_std_dmafree(pkt)
struct scsi_pkt *pkt;
{
	struct scsi_cmd *cmd = (struct scsi_cmd *) pkt;

	/*
	 * we don't need an spl here because mb_mapfree does that for us.
	 */

	if ((cmd->cmd_flags & CFLAG_DMAVALID) == 0) {
		return;
	}

	if ((cmd->cmd_flags & CFLAG_DMAKEEP) == 0) {
#ifdef	TEST_SPLS
		register s, ipl;
		static int last_spl = -1;

		if (last_spl == -1) {
			last_spl = scsi_spl;
		}
		s = splr(last_spl);
		ipl = spltoipl(last_spl) + 1;
		if (ipl > spltoipl(splvm_val)) {
			last_spl = scsi_spl;
		} else
			last_spl = ipltospl(ipl);
		mb_mapfree(DMA, (int *)&cmd->cmd_mapping);
		(void) splx(s);
#else	TEST_SPLS
		mb_mapfree(DMA, (int *)&cmd->cmd_mapping);
#endif	TEST_SPLS
#if defined(sun4c) && defined(VAC)
	} else if (vac && (cmd->cmd_flags & CFLAG_DMASEND) == 0 &&
	    SYS_VRANGE(cmd->cmd_mapping)) {
		extern u_int hat_getkpfnum();
		extern void hat_vacsync();
		register addr_t vacaddr;
		vacaddr = (addr_t) (cmd->cmd_mapping & MMU_PAGEMASK);
		while (vacaddr < (addr_t)
		    (cmd->cmd_mapping + cmd->cmd_dmacount)) {
			hat_vacsync(hat_getkpfnum(vacaddr));
			vacaddr += MMU_PAGESIZE;
		}
#endif	defined(sun4c) && defined(VAC)
	}

	cmd->cmd_flags &= ~CFLAG_DMAVALID;
	cmd->cmd_mapping = cmd->cmd_dmacount = 0;
}

/*
 * Store a queue element, returning if the funcp is already queued.
 * Always called at splvm().
 */

static void
scq_store(scq, funcp)
register struct scq *scq;
func_t funcp;
{
	register int i;

	for (i = 0; i < SCQLEN; i++) {
		if (scq->funcp[i] == funcp) {
			return;
		}
	}
	scq->ncalls++;
	scq->qlen++;
	scq->funcp[scq->qstore] = funcp;
	scq->qstore = (scq->qstore + 1) & SCQLENMASK;
	ASSERT(scq->qstore != scq->qretrv);
}

/*
 * Retrieve the queue element at the head of the wait queue.
 * Always called at splvm().
 */

static func_t
scq_retrieve(scq)
register struct scq *scq;
{
	register func_t funcp;

	scq->qlen--;
	scq->qretrv = (scq->qretrv + 1) & SCQLENMASK;
	ASSERT(scq->qretrv != scq->qstore);
	funcp = scq->funcp[scq->qretrv];
	scq->funcp[scq->qretrv] = NULL;

	return (funcp);
}


/*
 * Check a passed active data pointer for being within range
 */

int
scsi_chkdma(sp, max_xfer)
register struct scsi_cmd *sp;
register int max_xfer;
{
	register u_long maxv = max_xfer;
	if (sp->cmd_data < sp->cmd_mapping)
		return (0);
	else if (sp->cmd_data >= (sp->cmd_mapping + sp->cmd_dmacount))
		return (0);
	else if ((sp->cmd_data + maxv) >= sp->cmd_mapping+sp->cmd_dmacount) {
		return (sp->cmd_mapping + sp->cmd_dmacount - sp->cmd_data);
	} else {
		return ((int)maxv);
	}
}
