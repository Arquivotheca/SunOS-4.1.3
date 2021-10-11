#ifndef lint
static char sccsid[] = "@(#)is.c	1.1 7/30/92 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * IPI Channel driver for the Sun IPI VME String Controller
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/debug.h>
#include <sys/map.h>
#include <sundev/mbvar.h>
#include <sundev/ipi_driver.h>
#include <sundev/ipi_chan.h>
#include <sundev/ipi3.h>
#include <sundev/isdev.h>
#include <sundev/isvar.h>
#include <sundev/ipi_trace.h>
#include <machine/psl.h>
#if defined(IOC) && defined(sun3x)
#include <machine/mmu.h>		/* XXX - necesary for IOC_LINESIZE */
#endif IOC && sun3x

#define	RESET_DELAY	45000000	/* time limit for reset (microsecs) */
#define IS_POST_RESET_WAIT	20	/* delay after reset for drive init */
#ifndef	lint
#define is_syncw(r) \
	{ u_long synch = (r); }
#else	!lint
#define	is_syncw(r)
#endif	!lint

static int	is_tot_iopb;		/* packet alloc from IOPB (bytes) */
static caddr_t	is_pkt;			/* pointer to next part of cmd area */
static int	is_pkt_len;		/* remaining bytes available in pkt */
static int	is_pkt_mbinfo;		/* DVMA address for is_pkt */

static int	is_reset_enable = 1;	/* set zero to disable recovery reset */

extern int	nisc;
extern struct mb_ctlr	*iscinfo[];
extern struct mb_device *isdinfo[];
extern struct is_ctlr	is_ctlr[];

/*
 * Main Bus variables.
 */
static int	isprobe(), isslave(), isattach();

/*ARGSUSED*/
static int
isnone(arg)
	caddr_t arg;
{
	return 0;		/* empty callback routine */
}

struct mb_driver iscdriver = {
	isprobe, isslave, isattach, isnone, isnone, 0,
	sizeof(struct is_reg), "is", isdinfo, "isc", iscinfo,
        MDR_BIODMA
};

/*
 * IPI Host Adaptor Layer interface variables.
 */
static void		is_cmd();
static struct ipiq *	is_poll();
static struct ipiq *	is_setup();
static void		is_relse();
static void		is_nop();
static void		is_reset_slave();
static void		is_get_xfer_mode();
static void		is_set_xfer_mode();
static int		is_alloc();

static struct ipi_driver isc_ipi_driver = {
	is_cmd, is_poll, is_setup, is_relse,
	is_nop, is_nop, is_reset_slave, is_nop,
	is_get_xfer_mode, is_set_xfer_mode,
	is_alloc,
};

/*
 * Local functions.
 */
static struct ipiq *	is_poll_intr();
static struct ipiq *	is_get_resp();
static struct ipiq *	is_fault();
static int		is_asyn_ready();
static void		is_reset_test();

/*
 * Free lists.
 * Extra one at end terminates list with it's zero size.
 */
static struct ipiq_free_list	is_free_list[IS_NLIST+1];

/*
 * Determine whether String Controller board is present.
 */
static int
isprobe(reg, ctlr)
	caddr_t	reg;
	int	ctlr;
{
	register struct is_reg *rp;
	register struct is_ctlr *c;
	long	id;

	if (ctlr >= nisc) 
		return 0;

	rp = (struct is_reg *)reg;

	c = &is_ctlr[ctlr];
	c->ic_reg = rp;

	/*
	 * Attempt to read board ID register.
	 */
	if (peekl((long *)&rp->dev_bir, &id))
		return 0;
	if (id == PM_ID)
		return 0;		/* panther- */
	/*
	 * Wait for the reset which was started by the boot to
	 * complete.
	 */
	CDELAY((rp->dev_csr & CSR_RESET) == 0, RESET_DELAY);
	if (rp->dev_csr & CSR_RESET) {
		printf("isc%x: channel reset timed out\n", ctlr);
		return 0;
	}
	id = rp->dev_resp;		/* clear response register valid */
	c->ic_flags |= IS_PRESENT;	/* indicate probe worked */

	return sizeof(struct is_reg);
}	


/*
 * Initialize channel board device structures.
 *
 * The controller and device structures should be parallel since
 * there is only one channel per Panther board.
 */
static int
isslave(md, reg)
	register struct mb_device *md;
	caddr_t	reg;
{
	register struct ipiq_free_list *flp;
	register struct is_ctlr *c;
	register struct mb_ctlr	*mc;
	u_int	slave;
	int	pri;
	int	i;

	if (md->md_unit >= nisc)
		return 0; 
	c = &is_ctlr[md->md_unit];
	mc = iscinfo[md->md_unit];
	c->ic_ctlr = mc;
	c->ic_dev = md;
	pri = c->ic_ctlr->mc_intpri;

	/*
	 * The flags field from config gives the channel and slave number that
	 * the IPI drivers will use to access this channel.
	 */
	c->ic_chan = IPI_CHAN(md->md_flags);
	if ((slave = IPI_SLAVE(md->md_flags)) > IPI_NSLAVE) {
		printf("isc%d: bad IPI address %x\n", 
			md->md_unit, md->md_flags);
		return 0;
	}

	/*
	 * Initialize interrupt vector.
	 */
	if (mc->mc_intr == NULL) {
		printf("isc%x: interrupt vector not specified in config\n",
			md->md_unit);
		return 0;
	}
	*(mc->mc_intr->v_vptr) = (int)c;
	((struct is_reg *)reg)->dev_vector = mc->mc_intr->v_vec;
	((struct is_reg *)reg)->dev_csr |= CSR_EIRRV;	/* en rupt on resp */
	is_syncw(((struct is_reg *)reg)->dev_csr);	/* synch the write */

	/*
	 * If the free list hasn't been initialized, do it now.
	 * Allocate a small number of requests for probing.
 	 */
	if ((flp = is_free_list)->fl_size == 0) {
		for (i = 0; i < IS_NLIST; i++, flp++)
			flp->fl_size = IS_SIZE(i);
		(void) is_alloc(c, slave, IS_INIT_ALLOC, IS_INIT_SIZE);
	}

	/*
 	 * Setup to be called by IPI Host Adapter layer for requests
	 * to the channel number supported by this channel board.
	 *
	 * On return, the driver is given a pointer to the ipi_chan
	 * structure if the channel was correctly set up.  The only
	 * possible error is that the channel number was out of range
	 * or previously used by another driver.
	 */
	if (ipi_channel(c->ic_chan, slave, &isc_ipi_driver, (int)c, pri))
		return 0;
	return 1;
}

/*
 * Attach device.  
 */
/* ARGSUSED */
static int
isattach(md)
	struct mb_device *md;
{
	return 1;
}


/*
 * Send a command to the IPI channel.
 */
static void
is_cmd(q)
	register struct ipiq	*q;
{
	register struct is_ctlr *c;
	register int	s;
	u_long		status;

	c = (struct is_ctlr *)q->q_ch_data;
	q->q_next = NULL;
	s = splr(pritospl(c->ic_ctlr->mc_intpri));

	/*
	 * If the command register is busy, schedule an interrupt
	 * when it isn't.  Queue the command for later.
	 */
	if ((status = c->ic_reg->dev_csr) & CSR_CRBUSY) {
		if ((status & CSR_EICRNB) == 0) {
			c->ic_reg->dev_csr |= CSR_EICRNB;
			is_syncw(c->ic_reg->dev_csr);
		}
		if (c->ic_wait.q_head) {
			c->ic_wait.q_tail->q_next = q;
			c->ic_wait.q_tail = q;
		} else {
			c->ic_wait.q_head = c->ic_wait.q_tail = q;
		}
	} else {
		/*
		 * Ready to send a command.  If one was waiting
		 * before, queue the new one and send the old one.
		 */
		if (c->ic_wait.q_head) {
			c->ic_wait.q_tail->q_next = q;
			c->ic_wait.q_tail = q;
			q = c->ic_wait.q_head;	/* dequeue command */
			c->ic_wait.q_head = q->q_next;
		}
		/*
		 * Send the command by putting its DVMA address in
		 * the command register.
		 */
		ipi_trace4(TRC_IS_CMD, c->ic_ctlr->mc_ctlr, q->q_refnum,
			(int)q->q_cmd_ioaddr, q->q_cmd->hdr_opcode);
		c->ic_reg->dev_cmdreg = (u_long)q->q_cmd_ioaddr;
		is_syncw(c->ic_reg->dev_cmdreg);
	}
	(void) splx(s);
}

/*
 * Interrupt handling routine.
 */
isintr(c)
	struct is_ctlr	*c;
{
	struct ipiq	*q;

	if ((q = is_poll_intr(c)) != NULL)
		ipi_intr(q);
}

/*
 * Poll routine called by IPI layer.
 */
static struct ipiq *
is_poll(q)
	struct ipiq	*q;
{
	return is_poll_intr((struct is_ctlr *)q->q_ch_data);
}

/*
 * Internal version of poll.
 */
static struct ipiq *
is_poll_intr(c)
	struct is_ctlr	*c;
{
	struct ipiq	*q;		/* request associated with interrupt */
	struct ipiq	*rq;		/* request for pending commands */
	u_long		status;
	u_long		response;
	int		ctlr;

	q = NULL;
	response = 0;
	ctlr = c->ic_ctlr->mc_ctlr;	/* controller number for messages */
	status = c->ic_reg->dev_csr;	/* read status register */

	if (status & CSR_RRVLID) {
		if ((status & (CSR_ERROR | CSR_MRINFO)) == 0) {
			/*
			 * Successful command completion.
			 * The response register has the command reference 
			 * number.
			 */
			response = c->ic_reg->dev_resp;
			ipi_trace3(TRC_IS_RUPT_OK, ctlr, status, response);
			if ((q = ipi_lookup((u_short)response)) == NULL) {
printf("is%d: refnum lookup on success failed.  refnum %x\n", ctlr, response);
			} else {
				q->q_result = IP_SUCCESS;
			}
		} else if (status & CSR_MRINFO) {
			q = is_get_resp(c, status);	/* handle response */
		} else {
			q = is_fault(c, status);	/* handle ERROR */
		}
	}

	/*
	 * See if there are any more commands queued that can now be sent.
	 */
	if ((status & (CSR_EICRNB | CSR_CRBUSY)) == CSR_EICRNB) {
		if ((rq = c->ic_wait.q_head) != NULL) {
			if ((c->ic_wait.q_head = rq->q_next) == NULL)
				c->ic_wait.q_tail = NULL;
			c->ic_reg->dev_cmdreg = (u_long)rq->q_cmd_ioaddr;
			is_syncw(c->ic_reg->dev_cmdreg);
		}
		if (c->ic_wait.q_head == NULL) {
			c->ic_reg->dev_csr &= ~CSR_EICRNB;
			is_syncw(c->ic_reg->dev_csr);
		}
	}
	return q;
}

/*
 * Get response.
 *	This rarely happens: robustness is more important than speed here.
 */
/*ARGSUSED*/
static struct ipiq *
is_get_resp(c, status)
	struct is_ctlr	*c;
	u_long		status;
{
	struct ipiq	*q;		/* request associated with interrupt */
	u_long		response;
	u_long		*lp;
	u_long		*rlp;
	union {				/* local copy of response */
		struct ipi3resp hdr;		/* to access header fields */
		u_long	l[1];			/* copy a word at a time */
		u_char	c[ISC_MAX_RESP];	/* maximum response size */
	} resp;
	int		ctlr;
	int		resp_len;	/* length of response packet */
	int		cmd_len;	/* length of command packet */
	char		async;		/* response is asynchronous */
	int		i;

	q = NULL;
	response = 0;
	ctlr = c->ic_ctlr->mc_ctlr;	/* controller number for messages */

	/*
	 * There is a response packet.  It must be read before the response
	 * register is read.  Get first word, containing refnum and length.
	 */
	rlp = c->ic_reg->dev_resp_pkt;
	lp = resp.l;
	*lp++ = *rlp++;
	resp_len = i = resp.hdr.hdr_pktlen + sizeof(resp.hdr.hdr_pktlen);

	/*
	 * Check for short (or no) response.
	 */
	if (resp_len < sizeof(struct ipi3resp)) {
		response = c->ic_reg->dev_resp;		/* clear rupt */
		printf("is%d: response too short.  len %d min %d response %x\n",
			ctlr, resp_len, sizeof(struct ipi3resp), response);
		rlp--;
		ipi_trace6(TRC_IS_RUPT_RESP, ctlr, status,
			rlp[0], rlp[1], rlp[2], rlp[3]);
		return NULL;
	}

	/*
	 * Check for response too long.
	 */
	if (resp_len > sizeof(resp)) {
		printf("is%d: response too long. max %d len %d truncating\n",
			ctlr, sizeof(resp), resp_len);
		resp_len = i = sizeof(resp);	
		resp.hdr.hdr_pktlen = resp_len - sizeof(u_short);
		while ((i -= sizeof(u_long)) > 0)
			*lp++ = *rlp++;
		ipi_print_resp(c->ic_chan, &resp.hdr, "truncated response");
	}

	/*
	 * Copy rest of response.  Use only word fetches.
	 * Response buffer should be word aligned.
	 */
	while ((i -= sizeof(u_long)) > 0)
		*lp++ = *rlp++;

	/*
	 * If asynchronous, get ipiq in which to place response.
	 * Otherwise find command from the reference number.
	 */
	async = (IP_RTYP(resp.hdr.hdr_maj_stat) == IP_RT_ASYNC);
	if (async) {
		if ((q = ipiq_get(is_free_list, resp_len, is_asyn_ready))
			== NULL)
		{
			/*
			 * No place to put asynchronous response.
			 * Don't read the response register, leaving the
			 * interrupt pending.  Disable the rupt, will re-enable
			 * when called back indicating an ipiq is free.
			 */
			c->ic_reg->dev_csr &= ~CSR_EIRRV;	/* disable */
			is_syncw(c->ic_reg->dev_csr);
			return NULL;
		}
	} else if ((q = ipi_lookup(resp.hdr.hdr_refnum)) == NULL) {
		ipi_print_resp(c->ic_chan, &resp.hdr,
			"is_poll_intr: resp for unknown cmd");
		ipi_trace6(TRC_IS_RUPT_RESP, ctlr, status,
			resp.l[0], resp.l[1], resp.l[2], resp.l[3]);
		response = c->ic_reg->dev_resp;		/* clear rupt */
		return NULL;
	}

	/*
	 * Find place for response after command area.
	 * Command areas are allocated large enough for this.
	 */
	cmd_len = (q->q_cmd->hdr_pktlen
		+ sizeof(q->q_cmd->hdr_pktlen) + sizeof(long) - 1)
		& ~(sizeof(long) - 1);
	if (async)
		cmd_len = 0;
	lp = (u_long *) ((int)q->q_cmd + cmd_len);
	q->q_resp = (struct ipi3resp *)lp;
	for (rlp = resp.l; resp_len > 0; resp_len -= sizeof(u_long))
		*lp++ = *rlp++;

	/*
	 * Now that response packet has been read, it is safe to read the
	 * response register to clear the pending interrupt.  It should
	 * contain the command reference number.
	 */
	response = c->ic_reg->dev_resp;
	if (async) {
		q->q_resp->hdr_slave = IPI_SLAVE(c->ic_dev->md_flags);
		(void) ipi_async((int)c->ic_chan, q);
	} else {
		if (response != resp.hdr.hdr_refnum) {
   printf("isc%d: response reg %x not same as packet refnum %x\n",
				ctlr, response, resp.hdr.hdr_refnum);
		}

		/*
		 * If major status was simple success (0x18), set result
		 * accordingly, otherwise, just complete (probably error).
		 */
		q->q_result = 
			(resp.hdr.hdr_maj_stat == (IP_MS_SUCCESS | IP_RT_CCR)) 
			? IP_SUCCESS : IP_COMPLETE;
	} 

	ipi_trace6(TRC_IS_RUPT_RESP, ctlr, status,
		resp.l[0], resp.l[1], resp.l[2], resp.l[3]);
	return q;
}

/*
 * Callback from ipiq_get when a request area is available.
 * Enable all controllers for interrupt when response register valid.
 * The resulting interrupt will grab the ipiq (if really available) or
 * will just disable again.
 */
static int
is_asyn_ready()
{
	struct is_ctlr	*c;

	for (c = is_ctlr; c < &is_ctlr[nisc]; c++) {
		if (c->ic_flags & IS_PRESENT) {
			c->ic_reg->dev_csr |= CSR_EIRRV;	/* enable */
			is_syncw(c->ic_reg->dev_csr);
		}
	}
}

/*
 * Handle error from firmware.  
 *
 * This type of error is indicated by status of RRVLID, ERROR, but not MRINFO.
 * The response register contains the value that was written into the command
 * register, not the reference number, since the error occurred in fetching
 * the command packet, the firmware doesn't know the reference number.
 *
 * Find the request and return it's pointer.
 */
static struct ipiq *
is_fault(c, status)
	struct is_ctlr	*c;
	u_long	status;
{
	int		ctlr;
	char		*msg;
	long		refnum;
	int		response;
	struct ipiq	*q;

	ASSERT((status & CSR_ERROR) != 0);

	ctlr = c->ic_ctlr->mc_ctlr;	/* controller number for messages */

	/*
	 * First word in response is the DVMA address of the command packet.
	 */
	response = c->ic_reg->dev_resp;

	printf("isc%d: error status %x response %x\n", ctlr, status, response);

	switch ((status & CSR_FAULT) >> CSR_FAULT_SHIFT) {
	case CSR_FAULT_VME_B:
		msg = "bus error";
		break;
	case CSR_FAULT_VME_T:
		msg = "timeout";
		break;
	case CSR_INVAL_CMD:
		msg = "invalid command reg write";
		break;
	default:
		msg = "unknown fault code";
		break;
	}

	/*
	 * Search for command packet with this I/O address.
	 */
	if ((q = ipi_lookup_cmdaddr((caddr_t)response)) != NULL) {
		q->q_result = IP_ERROR;
		refnum = q->q_refnum;
	} else {
		refnum = 0;
	}

	printf("isc%d: ctlr fault %x - %s on cmd refnum %x\n",
		ctlr, status, msg, refnum);

	ipi_trace3(TRC_IS_RUPT_ERR, ctlr, status, response);

	return q;
}

/*
 * Allocate command packet and do DMA setup.  Called even if no data transfer.
 */
/* ARGSUSED */
static struct ipiq *  
is_setup(c, bp, cmdlen, resplen, callback, qflags)
	register struct is_ctlr *c;
	register struct buf	*bp;
	int			cmdlen;
	int			resplen;
	int			(*callback)();
	int			qflags;
{
	register struct ipiq	*q;
	int	flags = MDR_VME32;	/* flags for mb_mapalloc */

	/*
	 * If there's a buffer, add length of TNP to requested packet length.
	 * Also check alignment restrictions.
	 */
	if (bp) {
		cmdlen += 2 * sizeof(caddr_t);	/* 2 words for TNP and pad */
		if ((bp->b_flags & B_PAGEIO) == 0 
			&& ((int)bp->b_un.b_addr % sizeof(short)) != 0)
		{
			bp->b_flags |= B_ERROR;
			bp->b_error = EINVAL;
			return NULL;
		}
	}


	/*
	 * Allocate a command packet if possible.
	 */
	cmdlen += ISC_MAX_RESP + sizeof(caddr_t);	/* room for response */
	if ((q = ipiq_get(is_free_list, cmdlen, callback)) == NULL) 
		return NULL;

	if (bp == NULL) {
		q->q_dma_len = 0;		/* no data transfer */
		q->q_tnp_len = 0;
		q->q_dma_info = NULL;
		q->q_tnp = NULL;
		q->q_buf = NULL;
	} else {
		if (callback == IPI_NULL) {
			callback = isnone;	/* does nothing when called */
			flags |= MB_CANTWAIT;
		} else if (callback != IPI_SLEEP) {
			flags |= MB_CANTWAIT;
		}
#if defined(IOC) && defined(sun3x)
		/*
		 * If the buffer address is aligned properly, mark the
		 * transfer I/O cacheable.
		 */
		if (((int)bp->b_un.b_addr & (IOC_LINESIZE - 1)) == 0)
			bp->b_flags |= B_IOCACHE;
#endif IOC && sun3x
		/*
		 * Dumps have DMA already setup.
		 */
		if ((q->q_flag |= qflags) & IP_NO_DMA_SETUP) {
			q->q_dma_info = (caddr_t)(bp->b_un.b_addr - DVMA);
		} else if ((q->q_dma_info = 
			(caddr_t)mb_mapalloc(c->ic_dev->md_hd->mh_map, 
				bp, flags, callback, (caddr_t) 0)) == NULL)
		{
			ipiq_free(is_free_list, q);
			return NULL;
		}
		q->q_buf = bp;
		q->q_dma_len = bp->b_bcount;
		q->q_tnp = (char *) &q->q_dma_info;
		q->q_tnp_len = sizeof(caddr_t);
	}
	q->q_ch_data = (caddr_t)c;
	return q;
}

static void
is_relse(q)
	register struct ipiq	*q;
{
	register struct is_ctlr *c;
	register struct ipiq	*rq;

	c = (struct is_ctlr *)q->q_ch_data;
	q->q_refnum = 0;
	if (q->q_dma_info) {
		if (!(q->q_flag & IP_NO_DMA_SETUP))
			mb_mapfree(c->ic_dev->md_hd->mh_map,
				(int *)&q->q_dma_info);
		q->q_buf = NULL;
		q->q_dma_len = 0;
		q->q_tnp = NULL;
		q->q_tnp_len = 0;
	}
	if ((rq = q->q_respq) != NULL) {
		ipiq_free(is_free_list, rq);
		q->q_resp = NULL;
		q->q_respq = NULL;
	}
	ipiq_free(is_free_list, q);
}

/*
 * No-operation routine used for test_board, reset_board, reset_channel.
 */
/* ARGSUSED */
static void
is_nop(q)
	register struct ipiq	*q;
{
	ipi_complete(q, IP_SUCCESS);
}

/*
 * Reset slave.
 *	This resets the Panther controller board.  
 *	This routine is a no-op the first time after boot since the board
 *	was presumably reset by the boot.  This saves lot of time.
 */
static void
is_reset_slave(q)
	register struct ipiq	*q;
{
	register struct is_ctlr	*c;

	c = (struct is_ctlr *)q->q_ch_data;
	if ((c->ic_flags & IS_ENABLE_RESET) == 0) {
		c->ic_flags |= IS_ENABLE_RESET;
		ipi_complete(q, IP_SUCCESS);
		return;
	}

	if (!is_reset_enable) {
		printf("is%d: is_reset_slave: slave reset not enabled.\n",
			c->ic_ctlr->mc_ctlr);
		ipi_complete(q, IP_ERROR);
		return;
	}

	printf("is%d: resetting slave\n", c->ic_ctlr->mc_ctlr);

	/*
	 * Clear queue pointers.  Requests will be taken away by the IPI layer.
	 */
	c->ic_wait.q_head = c->ic_wait.q_tail = NULL;	/* drop queues */
	c->ic_reg->dev_csr = CSR_RESET;	/* reset controller board */
	is_syncw(c->ic_reg->dev_csr);
	ipi_retry_slave(q, IPI_SLAVE(q->q_csf));	/* retry all requests */
	is_reset_test(q);	/* test for completion */
}

/*
 * Callout-driven routine to respond to reset slave request.
 */
static void
is_reset_test(q)
	register struct ipiq	*q;
{
	register struct is_ctlr	*c;
	struct is_reg	*rp;
	int		i;

	c = (struct is_ctlr *)q->q_ch_data;
	rp = c->ic_reg;
	if (q->q_flag & IP_SYNC) {
		CDELAY((rp->dev_csr & CSR_RESET) == 0, RESET_DELAY);
	} else if (rp->dev_csr & CSR_RESET) {
		timeout((int(*)())is_reset_test, (caddr_t)q, hz);
		return;
	} else if ((c->ic_flags & IS_RESET_WAIT) == 0) { 
		/*
		 * Additional delay to get drives initialized.
		 */
		c->ic_flags |= IS_RESET_WAIT;
		timeout((int(*)())is_reset_test, (caddr_t)q,
			hz * IS_POST_RESET_WAIT);
		return;
	}
	c->ic_flags &= ~IS_RESET_WAIT;
	if (rp->dev_csr & CSR_RESET) {
		printf("isc%x: ctlr reset timed out\n", c->ic_ctlr->mc_ctlr);
		ipi_complete(q, IP_ERROR);
		return;
	}
	i = rp->dev_resp;		/* clear response register valid */
#ifdef	lint
	i = i;
#endif
	rp->dev_vector = c->ic_ctlr->mc_intr->v_vec;
	rp->dev_csr |= CSR_EIRRV;	/* enable rupt on response */
	is_syncw(c->ic_reg->dev_csr);

	ipi_complete(q, IP_SUCCESS);
}

static void
is_get_xfer_mode(q)
	struct ipiq	*q;
{
	/*
	 * Stuff current mode in buffer.  Relies on device driver not
	 * detaching buffer.
	 */
	q->q_buf->b_un.b_addr[0] = IP_STREAM_CAP | IP_STREAM_MODE;
	ipi_complete(q, IP_SUCCESS);
}

/*
 * Set transfer mode.
 * 	The mode argument is ignored since the channel only supports
 *	streaming 10 MB/sec transfers.
 */
/* ARGSUSED */
static void
is_set_xfer_mode(q, mode)
	struct ipiq	*q;
	int	mode;
{
	if ((mode & (IP_STREAM | IP_STREAM_20)) == IP_STREAM)
		ipi_complete(q, IP_SUCCESS);
	else
		ipi_complete(q, IP_INVAL);
}

/*
 * Allocate resources for future requests.
 *	Controller driver is declaring it's maximum needs.  It isn't 
 *	necessary to allocate all of the structures mentioned, since not
 *	all controllers will be completely busy at any one time, but it
 *	must be possible for the controller to setup a request of the
 *	largest declared size.
 *
 *	This may be called multiple times by the same controller if it
 *	requires differently sized packets.
 *
 *	If this is called more than once for the same sized packet for the
 *	same controller, the allocations don't add.  This is so the controller
 *	can go through re-initialization.
 */
/* ARGSUSED */
static int
is_alloc(c, slave, count, size)
	struct is_ctlr	*c;
	u_int		slave;	/* slave address - not needed here */
	int		count;	/* upper bound on number of structs needed */
	int		size;	/* packet size required */
{
	struct ipiq_free_list	*flp;		/* free list */
	struct ipiq_free_list	*fflp;		/* found free list */
	struct ipiq_chunk	*qcp;		/* New area of IPIQs */
	struct ipiq		*q;		/* IPI queues */
	caddr_t			cmd;		/* command packets */
	caddr_t			cmd_start;	/* start of command area */
	int			mbinfo;		/* DMA address of packets */
	int			len;		/* length of packet area */
	int			chunk_len;	/* length of area for chunk */
	int			i;

	/*
	 * Add room for response.
	 */
	size += ISC_MAX_RESP;
	if (size > IS_MAX_SIZE)
		size = IS_MAX_SIZE;

	/*
	 * Find free list for this size.
	 */
	fflp = NULL;
	i = 0;
	for (flp = is_free_list; flp->fl_size > 0; flp++, i++) {
		if (size <= flp->fl_size) {
			size = flp->fl_size;
			fflp = flp;
			break;
		}
	}

	if (fflp == NULL)
		return 0;

	count -= c->ic_alloc[i];	/* subtract part already allocated */
	if (count <= 0)
		return 1;

	/*
	 * calculate lengths of areas needed.
	 */
	len = count * size;
	if (is_tot_iopb + len < IS_MAX_IOPB) {
		cmd_start = cmd = (caddr_t)rmalloc(iopbmap, (long)len);
		if (cmd_start == NULL) 
			return 0;		/* shouldn't happen */
		mbinfo = cmd_start - DVMA;
		is_tot_iopb += len;
	} else {
		/*
		 * Allocate more ``IOPBs'' for IPI use only.
		 */
		if (is_pkt == NULL) {
			is_pkt = (caddr_t)kmem_zalloc(IS_MAX_ALLOC);
			if (is_pkt == NULL)
				return 0;	/* out of memory */
			mbinfo = mballoc(c->ic_ctlr->mc_mh, is_pkt,
				IS_MAX_ALLOC, MDR_LINE_NOIOC);
			if (mbinfo == NULL)
				return 0;	/* out of DVMA */
			is_pkt_mbinfo = mbinfo;
			is_pkt_len = IS_MAX_ALLOC;
		}
		/*
		 * If there's no more in the "extra" IOPBs, don't alloc.
		 */
		if (is_pkt_len <= 0)
			return (1);
		/*
		 * If there isn't enough to allocate what is needed, 
		 * allocate as much as possible.
		 */
		if (len > is_pkt_len) {
			count = is_pkt_len / size;
			len = count * size;
		}
		cmd = is_pkt;
		mbinfo = is_pkt_mbinfo;
		is_pkt += len;
		is_pkt_mbinfo += len;
		is_pkt_len -= len;
	}

	/*
	 * Allocate ipiqs in a "chunk", so IPI layer can find all ipiqs.
	 */
	chunk_len = sizeof(struct ipiq_chunk) + (count-1) * sizeof(struct ipiq);
	qcp = (struct ipiq_chunk *)kmem_zalloc((u_int)chunk_len);
	if (qcp == NULL)
		return 0;		/* wastes mem - shouldn't happen */

	c->ic_alloc[i] += count;	/* add to current allocation */

	/*
	 * Setup chunk.	Link all the queues together for the free list.
	 */
	for (q = qcp->qc_ipiqs; count-- > 0; q++) {
		q->q_cmd_pkt = cmd;
		q->q_cmd_len = size;
		q->q_cmd_ioaddr = (caddr_t)mbinfo;
                if (count == 0) q->q_next = NULL;
		else q->q_next = q + 1;
		cmd += size;
		mbinfo += size;
	}
	qcp->qc_end = q;

	/*
	 * Add the whole lot to the free list.
	 */
	ipiq_free_chunk(fflp, qcp);
	return 1;
}
