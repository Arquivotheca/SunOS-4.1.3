#ident	"@(#)scsi_confsubr.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990, Sun Microsystems, Inc.
 */
/*
 *
 *	Utility SCSI configuration routines
 *
 */

#include <scsi/scsi.h>

/*
 *
 * SCSI target config routine - provided as a service to host adapters
 *
 */

#define	DPRINTF	if (scsi_options & SCSI_DEBUG_LIB) printf

extern struct scsi_device *sd_root;
extern int scsi_spl, scsi_ncmds_per_dev;

static int scsi_make_device();

/*
 * SCSI configuration
 *
 * As each host adapter is probed out and attached, it makes
 * itself known to the SCSI subsystem by passing up a transport
 * structure and a device cookie.
 *
 * The transport structure is used to form the 'a_cookie' field
 * of a scsi_address structure.
 *
 * The device cookie is autoconfiguration dependent. For OPENPROMS
 * support, it is the dev_info structure for the host adapter, else
 * it is the mb_ctlr structure for the host adapter.
 *
 * In either case, the routine scsi_config will take this information
 * and walk through a config(8) generated table (scsi_conf.c) that
 * contains a list of scsi devices, what target, lun and device unit
 * they are, which host adapter they want to be on. For each match,
 * the routine make_scsi_device will allocate the appropriate structures
 * and call the appropriate slave and attach routines for them.
 */

#define	MAXSCSIBUS	16
struct scsi_transport *scsibuscookies[MAXSCSIBUS];

#ifdef	OPENPROMS
struct dev_info *scsibusctlrs[MAXSCSIBUS];
#else	OPENPROMS
struct mb_ctlr *scsibusctlrs[MAXSCSIBUS];
#endif	OPENPROMS

/*
 * maximum spl of all attached scsi bus host adapters
 */

int scsi_spl;

#ifdef	OPENPROMS
int
scsi_add_device(sa, tgtops, tname, tunit)
struct scsi_address *sa;
struct dev_ops *tgtops;
char *tname;
int tunit;
{
	register i, s;

	for (i = 0; i < MAXSCSIBUS; i++) {
		if (sa->a_cookie == (int) scsibuscookies[i]) {
			break;
		}
	}
	if (i == MAXSCSIBUS)
		return (0);
	s = splr(scsi_spl);
	i = scsi_make_device(scsibuscookies[i], scsibusctlrs[i],
	    tgtops, tname, tunit, (int) sa->a_target, (int) sa->a_lun);
	(void) splx(s);
	return (i);
}

void
scsi_config(tranp, dev)
struct scsi_transport *tranp;
struct	dev_info *dev;
{
	register oldspl, ndevs;
	register struct scsi_conf *scp;

	ndevs = nscsi_devices;

	/*
	 * save old priority, and raise to current host adapter
	 * priority (cannot necessarily assume that we are already
	 * at that level).
	 */

	oldspl = scsi_spl;
	scsi_spl = MAX(scsi_spl, tranp->tran_spl);

	/*
	 * initialize resources (if not already done)
	 */

	scsi_rinit();

	for (scp = scsi_conf; scp->haops; scp++) {
		if (scp->haops != dev->devi_driver) {
			continue;
		} else if (strcmp(dev->devi_name, scp->hname)) {
			continue;
		} else if (dev->devi_unit != scp->hunit) {
			continue;
		} else {
			u_char busid = scp->busid;
			if (busid >= MAXSCSIBUS || (scsibuscookies[busid] &&
			    scsibuscookies[busid] != tranp)) {
				printf("%s%d: illegal SCSI bus\n",
				    dev->devi_name, dev->devi_unit);
				break;
			}
			scsibuscookies[busid] = tranp;
			scsibusctlrs[busid] = dev;

			if (scsi_make_device(tranp, dev, scp->tgtops,
			    scp->tname, (int) scp->dunit, (int) scp->target,
			    (int) scp->lun)) {
				nscsi_devices++;
			}
		}
	}

	if (nscsi_devices == ndevs) {
		printf("%s%d: Warning- no devices found for this SCSI bus\n",
		    dev->devi_name, dev->devi_unit);
		scsi_spl = oldspl;
	}

}

static int
scsi_make_device(tranp, pdev, tgtops, tname, dunit, target, lun)
struct scsi_transport *tranp;
struct dev_info *pdev;
struct dev_ops *tgtops;
char *tname;
int dunit, target, lun;
{
	register struct scsi_device *devp, *tdevp;

	DPRINTF("make device %s%d\n", tname, dunit);
	devp = (struct scsi_device *)
		kmem_zalloc((unsigned) sizeof (struct scsi_device));

	if (devp == (struct scsi_device *) 0)
		return (0);

	devp->sd_address.a_cookie = (int) tranp;
	devp->sd_address.a_target = target;
	devp->sd_address.a_lun = lun;

	devp->sd_dev = (struct dev_info *)
		kmem_zalloc((unsigned) sizeof (struct dev_info));

	if (devp->sd_dev == (struct dev_info *) 0) {
		(void)kmem_free((caddr_t) devp,
			(unsigned) sizeof (struct scsi_device));
		return (0);
	}

	devp->sd_dev->devi_intr = (struct dev_intr *)
		kmem_zalloc((unsigned) sizeof (struct dev_intr));

	if (!devp->sd_dev->devi_intr) {
		(void) kmem_free((caddr_t) devp->sd_dev,
			(unsigned) sizeof (struct dev_info));
		(void) kmem_free((caddr_t) devp,
			(unsigned) sizeof (struct scsi_device));
		return (0);
	}

	devp->sd_dev->devi_parent = pdev;
	if (pdev->devi_slaves) {
		devp->sd_dev->devi_next = pdev->devi_slaves;
	}
	pdev->devi_slaves = devp->sd_dev;
	devp->sd_dev->devi_name = tname;
	devp->sd_dev->devi_unit = dunit;
	devp->sd_dev->devi_intr->int_pri = spltoipl(tranp->tran_spl);

	if ((*tgtops->devo_identify)(devp) < 0) {
		pdev->devi_slaves = devp->sd_dev->devi_next;
		(void) kmem_free((caddr_t) devp->sd_dev->devi_intr,
		    (unsigned) sizeof (struct dev_intr));
		(void) kmem_free((caddr_t) devp->sd_dev,
			(unsigned) sizeof (struct dev_info));
		(void) kmem_free((caddr_t) devp,
			(unsigned) sizeof (struct scsi_device));
		return (0);
	}

	if (devp->sd_present) {
		printf("%s%d at %s%d target %d lun %d\n", tname,
			dunit, pdev->devi_name, pdev->devi_unit,
			target, lun);
		(void) (*tgtops->devo_attach)(devp);
	}

	if (tdevp = sd_root) {
		while (tdevp->sd_next)
			tdevp = tdevp->sd_next;
		tdevp->sd_next = devp;
	} else {
		sd_root = devp;
	}
	/*
	 * If the standard packet allocator is used, maybe
	 * add some commands to the pool becaues we've
	 * added another target.
	 */
	if (tranp->tran_pktalloc == scsi_std_pktalloc &&
	    ((nscsi_devices + 1) * scsi_ncmds_per_dev) > scsi_ncmds) {
		scsi_addcmds(scsi_ncmds_per_dev);
	}
	return (1);
}

#else	OPENPROMS

int
scsi_add_device(sa, tgtdriver, tname, tunit)
struct scsi_address *sa;
struct mb_driver *tgtdriver;
char *tname;
int tunit;
{
	register i, s;

	for (i = 0; i < MAXSCSIBUS; i++) {
		if (sa->a_cookie == (int) scsibuscookies[i]) {
			break;
		}
	}
	if (i == MAXSCSIBUS)
		return (0);
	s = splr(scsi_spl);
	i = scsi_make_device(scsibuscookies[i], scsibusctlrs[i],
	    tgtdriver, tname, tunit, (int) sa->a_target, (int) sa->a_lun);
	(void) splx(s);
	return (i);
}

void
scsi_config(tranp, md)
struct scsi_transport *tranp;
struct mb_device *md;
{
	register oldspl, ndevs;
	register struct scsi_conf *scp;
	struct mb_ctlr *mc = md->md_mc;

	ndevs = nscsi_devices;

	/*
	 * save old priority, and raise to current host adapter
	 * priority (cannot necessarily assume that we are already
	 * at that level).
	 */

	oldspl = scsi_spl;
	scsi_spl = MAX(scsi_spl, tranp->tran_spl);

	/*
	 * initialize resources (if not already done)
	 */

	scsi_rinit();

	for (scp = scsi_conf; scp->hadriver; scp++) {
		if (scp->hadriver != mc->mc_driver) {
			continue;
		} else if (strcmp(mc->mc_driver->mdr_cname, scp->hname)) {
			continue;
		} else if (mc->mc_ctlr != scp->hunit) {
			continue;
		} else {
			u_char busid = scp->busid;
			if (busid >= MAXSCSIBUS || (scsibuscookies[busid] &&
			    scsibuscookies[busid] != tranp)) {
				printf("%s%d: illegal SCSI bus\n",
				    mc->mc_driver->mdr_cname, mc->mc_ctlr);
				break;
			}

			scsibuscookies[busid] = tranp;
			scsibusctlrs[busid] = mc;

			if (scsi_make_device(tranp, mc, scp->tgtdriver,
			    scp->tname, (int) scp->dunit, (int) scp->target,
			    (int) scp->lun)) {
				nscsi_devices++;
			}
		}
	}
	if (nscsi_devices == ndevs) {
		printf("%s%d: Warning- no devices found for this SCSI bus\n",
		    mc->mc_driver->mdr_cname, mc->mc_ctlr);
		scsi_spl = oldspl;
	}
}

static int
scsi_make_device(tranp, mc, tgtdriver, tname, dunit, target, lun)
struct scsi_transport *tranp;
struct mb_ctlr *mc;
struct mb_driver *tgtdriver;
char *tname;
int dunit, target, lun;
{
	int made_cname;
	register struct scsi_device *devp, *tdevp;
	register struct mb_device *md;

	DPRINTF("make device %s%d\n", tname, dunit);
	/*
	 * first, find the mb_device for this target
	 */

	for (md = mbdinit; md->md_driver; md++) {
		if (md->md_driver == tgtdriver && dunit == md->md_unit)
			break;
	}
	if (!md->md_driver) {
		/*
		 * XXX: here is where we would be put code to look through
		 * XXX: a constructed && kmem_alloc'd list of loaded modules
		 */
		DPRINTF("can't find mb_device\n");
		return (0);
	}

	devp = (struct scsi_device *)
		kmem_zalloc((unsigned) sizeof (struct scsi_device));

	if (devp == (struct scsi_device *) 0)
		return (0);

	devp->sd_address.a_cookie = (int) tranp;
	devp->sd_address.a_target = target;
	devp->sd_address.a_lun = lun;
	devp->sd_dev = md;
	md->md_ctlr = mc->mc_ctlr;
	md->md_intpri = mc->mc_intpri;
	md->md_mc = mc;
	md->md_hd = &mb_hd;
	if (md->md_driver && md->md_driver->mdr_cname == (char *) 0) {
		made_cname = 1;
		md->md_driver->mdr_cname = mc->mc_driver->mdr_cname;
	} else {
		made_cname = 0;
	}

	if ((*tgtdriver->mdr_slave)(devp) < 0) {
		(void) kmem_free((caddr_t) devp, sizeof (struct scsi_device));
		md->md_ctlr = -1;
		md->md_intpri = 0;
		md->md_mc = (struct mb_ctlr *) 0;
		md->md_hd = (struct mb_hd *) 0;
		if (made_cname)
			md->md_driver->mdr_cname = (char *) 0;
		return (0);
	}

	if (devp->sd_present) {
		md->md_alive = 1;	/* XXX should be on always? */
		printf("%s%d at %s%d target %d lun %d\n", tname,
			dunit, mc->mc_driver->mdr_cname, mc->mc_ctlr,
			target, lun);
		(void) (*tgtdriver->mdr_attach)(devp);
	}

	if (tdevp = sd_root) {
		while (tdevp->sd_next)
			tdevp = tdevp->sd_next;
		tdevp->sd_next = devp;
	} else {
		sd_root = devp;
	}
	/*
	 * If the standard packet allocator is used, maybe
	 * add some commands to the pool becaues we've
	 * added another target.
	 */
	if (tranp->tran_pktalloc == scsi_std_pktalloc &&
	    ((nscsi_devices + 1) * scsi_ncmds_per_dev) > scsi_ncmds) {
		scsi_addcmds(scsi_ncmds_per_dev);
	}
	return (1);
}
#endif

/*
 *
 * SCSI slave probe routine - provided as a service to target drivers
 *
 * Mostly attempts to allocate and fill devp inquiry data..
 */

#define	WRAPBUFFER(bp, addr, count, flag)	{\
		bzero((caddr_t)(bp), sizeof (struct buf)); \
		(bp)->b_un.b_addr = (caddr_t) addr; \
		(bp)->b_bcount = (count); \
		(bp)->b_flags = (flag); \
	}

#define	ROUTE	(&devp->sd_address)

int
scsi_slave(devp, canwait)
register struct scsi_device *devp;
int canwait;
{
	register struct scsi_pkt *pkt = 0, *rqpkt = 0;
	register u_long rqs = 0;
	register int rval = SCSIPROBE_NOMEM;
	register int (*f)() = (canwait != 0) ? SLEEP_FUNC: NULL_FUNC;
	auto struct buf local;

	if (devp->sd_inq == (struct scsi_inquiry *) 0) {
		devp->sd_inq = (struct scsi_inquiry *) IOPBALLOC(SUN_INQSIZE);
		if (devp->sd_inq == (struct scsi_inquiry *) 0)
			goto out;
	}

	if (!(pkt = scsi_pktalloc(ROUTE, CDB_GROUP0, 1, f))) {
		goto out;
	}
	makecom_g0(pkt, devp, FLAG_NOINTR|FLAG_NOPARITY,
	    SCMD_TEST_UNIT_READY, 0, 0);
	if (scsi_poll(pkt) < 0) {
		if (pkt->pkt_reason == CMD_INCOMPLETE)
			rval = SCSIPROBE_NORESP;
		else
			rval = SCSIPROBE_FAILURE;
		goto out;
	}

	if (!(rqs = (u_long) IOPBALLOC(SENSE_LENGTH))) {
		goto out;
	}
	WRAPBUFFER(&local, rqs, SENSE_LENGTH, B_READ);
	rqpkt = scsi_resalloc(ROUTE, CDB_GROUP0, 1, (opaque_t)&local, f);
	if (!rqpkt) {
		goto out;
	}
	makecom_g0(rqpkt, devp, FLAG_NOINTR|FLAG_NOPARITY,
	    SCMD_REQUEST_SENSE, 0, SENSE_LENGTH);

	if (((struct scsi_status *) pkt->pkt_scbp)->sts_chk) {
		/*
		 * The controller type is as yet unknown, so we
		 * have to do a throwaway non-extended request sense, and hope
		 * that that clears the check condition for that
		 * unit until we can find out what kind of drive it is.
		 *
		 * A non-extended request sense is specified by stating that
		 * the sense block has 0 length, which is taken to mean
		 * that it is four bytes in length.
		 *
		 */
		if (scsi_poll(rqpkt) < 0) {
			rval = SCSIPROBE_FAILURE;
			goto out;
		}
	}

	/*
	 * At this point, we are guaranteed that something responded
	 * to this scsi bus target id. We don't know yet what
	 * kind of device it is, or even whether there really is
	 * a logical unit attached (as some SCSI target controllers
	 * lie about a unit being ready, e.g., the Emulex MD21).
	 */

	WRAPBUFFER(&local, devp->sd_inq, SUN_INQSIZE, B_READ);
	if (scsi_dmaget(pkt, (opaque_t)&local, f) == (struct scsi_pkt *) 0) {
		goto out;
	}

	bzero((caddr_t) devp->sd_inq, SUN_INQSIZE);
	makecom_g0(pkt, devp, FLAG_NOINTR|FLAG_NOPARITY,
	    SCMD_INQUIRY, 0, SUN_INQSIZE);

	if (scsi_poll(pkt) < 0) {
		rval = SCSIPROBE_FAILURE;
		goto out;
	}

	/*
	 * Okay we sent the INQUIRY command. scsi_poll() will
	 * shield us from incomplete commands, and attempt
	 * to shield us from BUSY status returns.
	 *
	 * If enough data was transferred, we count that the
	 * Inquiry command succeeded, else we have to assume
	 * that this is a non-CCS scsi target (or a nonexistent
	 * target/lun).
	 */

	if (((struct scsi_status *) pkt->pkt_scbp)->sts_chk) {
		(void) scsi_poll(rqpkt);
	}

	/*
	 * If we got a parity error on receive of inquiry data,
	 * we're just plain out of luck because we told the host
	 * adapter to not watch for parity errors.
	 */

	if ((pkt->pkt_state & STATE_XFERRED_DATA) == 0 ||
	    ((SUN_INQSIZE - pkt->pkt_resid) < SUN_MIN_INQLEN)) {
		rval = SCSIPROBE_NONCCS;
	} else {
		rval = SCSIPROBE_EXISTS;
	}

out:
	if (rqpkt) {
		scsi_resfree(rqpkt);
	}
	if (pkt) {
		scsi_resfree(pkt);
	}
	if (rqs) {
		IOPBFREE(rqs, SENSE_LENGTH);
	}
	return (rval);
}
