/* @(#)ipi.c	1.1 7/30/92 */
#ifndef lint
static  char sccsid[] = "@(#)ipi.c 1.1 92/07/30 Copyr 1991 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Host Adapter Layer for IPI Channels.
 *
 * This module links IPI device and controller drivers to 
 * IPI Channel drivers.  The IPI Channel drivers call ipi_channel
 * to "register" with this module and declare their willingness
 * to accept requests.
 *
 * The basic structure passed by this module is the ipiq, which
 * contains pointers to the IPI command and response packets.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sundev/mbvar.h>
#include <sundev/ipi3.h>
#include <sundev/ipi_driver.h>
#include <sundev/ipi_chan.h>
#include <sundev/ipi_trace.h>
#include <machine/psl.h>
#include <sys/debug.h>

#define RESP_BUF_LEN	512	/* size of response unpacking buf (on stack) */

extern caddr_t	kmem_alloc();

/*
 * Channel structure pointer anchor.
 * The number of channels is dynamically configured.
 */
static struct ipi_chan **ipi_chan;
static int	ipi_nchan;

#define	CHANSET	4		/* number of channel pointers to extend by */

#define	IPI_RETRY_TIMEOUT	21	/* timeout for command retried */
#define	IPI_DEFAULT_TIMEOUT	19	/* default timeout for command */
static int ipi_retry_limit = IPI_RETRY_LIMIT;
static int ipi_retry_timeout = IPI_RETRY_TIMEOUT;
static int ipi_default_timeout = IPI_DEFAULT_TIMEOUT;

/*
 * Request lookup table.
 *
 * This table provides lookup of requests by command reference number.
 *
 * The range of command reference numbers should be larger than the number 
 * of request packets in order to reduce the frequency re-use of 
 * command reference numbers.  It cannot be increased beyond 64K-1.
 */
#define MAX_REFNUM	1024		/* max cmd ref number (16 bits) */
int	ipi_lookup_len = MAX_REFNUM;	/* size of refnum lookup table */
static struct ipiq **ipi_ipiq_lookup;	/* lookup to IPIQ */

void	assign_refnum();
void	free_refnum();

static int	ipi_refnum = 1;		/* Next cmd reference number to use */

/*
 * Header for list of ipiq allocations.
 */
struct ipiq_chunk ipiq_chunk_list = { &ipiq_chunk_list, 0};

static int	ipi_pri;

#define	IPI_WATCH_TIME	(9*hz)		/* interval between queue checks */
static void	ipi_watch();		/* look for hung IPI requests */
static void	ipi_setup_start();	/* setup q for call to channel */


/*
 * Callback list.
 */
struct	ipi_callback {
	int	(*ic_fcn)();
};

#define	IPIQ_CBQ_LEN	10		/* one per driver should be enough */

static struct ipi_callback ipiq_callback_list[IPIQ_CBQ_LEN]; 	/* list */
static struct ipi_callback *ipiq_cb_in = ipiq_callback_list; 	/* in pointer */
static struct ipi_callback *ipiq_cb_out = &ipiq_callback_list[IPIQ_CBQ_LEN-1]; 
							/* out pointer */
static int	ipiq_free_wait;		/* processes waiting for ipiqs */
static int	ipiq_callback_len;

static struct ipiq_stats {
	int	i_runout;		/* number of times out of ipiqs */
	int	i_runout_int;		/* number of times out of internal */
	int	i_max_qlen;		/* maximum callback length */
} ipiq_stats;

static void	ipiq_callback();	/* schedule callback */
static int	ipiq_call();		/* invoke callback */

/*
 * Initialization:
 *	Called each time ipi_channel is called.  Therefore this allocation
 *	is only performed if an on-line channel board is configured.
 *  Setup IPI command reference number lookup.
 *  Start command timeout watchdog routine.
 */
static char	ipi_init_flag;		/* non-zero after initialization */

static void
ipi_init()
{
	if (ipi_init_flag)
		return;
#ifdef	IPI_TRACE_SOLO
	ipi_inittrace();
#endif

#ifndef lint
	/*
 	 * IPIQ must be aligned so that the low order bits of the intparm
	 * can be used for flags.  Lint doesn't understand why anyone would 
	 * use constant expressions in conditionals.
	 */
	if (sizeof(struct ipiq) % sizeof(long)) {
		panic("ipiq improperly sized");
	}
#endif
	/*
	 * Allocate and initialize command reference lookup table.
	 */
	ipi_ipiq_lookup = (struct ipiq **)kmem_zalloc((u_int)ipi_lookup_len 
		* sizeof(struct ipiq *));
	if (ipi_ipiq_lookup == NULL) 
		panic("ipi_init: no memory for IPI lookup");

	/*
	 * Schedule watchdog interrupt.
	 */
	timeout((int (*)())ipi_watch, (caddr_t)NULL, IPI_WATCH_TIME);

	ipi_init_flag = 1;	/* indicate initialization complete */
}

/*
 * Perform initialization for a device. 
 *
 * Returns non-zero if the address passed is invalid or the channel and
 * slave are not attached.
 */
/* ARGSUSED */
ipi_device(addr, intpri, flags, async, recover, ctlr)
	int	addr;		/* IPI device address */
	int	intpri;
	int	flags;
	void	(*async)();	/* async interrupt routine */
	void	(*recover)();	/* driver recovery routine */
	caddr_t	ctlr;		/* driver info for ipiq structures */
{	
	int	chan;
	int	slave;
	int	fac;
	struct ipi_chan *ch;
	struct ipi_slave *sl;

	if ((chan = IPI_CHAN(addr)) >= ipi_nchan || chan < 0 
		|| (ch = ipi_chan[chan]) == NULL 
		|| ((slave = IPI_SLAVE(addr)) < 0 || slave >= IPI_NSLAVE)
		|| ch->ic_slave[slave].ic_arg == 0
		|| (fac = IPI_FAC(addr)) != IPI_NO_ADDR 
			&& (fac < 0 || fac >= IPI_NFAC))
		return -1;
	sl = &ch->ic_slave[slave];
	sl->ic_intpri = intpri;
	sl->ic_async = async;
	sl->ic_recover = recover;
	sl->ic_ctlr = ctlr;	/* driver info for ipiq structures */
	return 0;
}


/*
 * Allocate request (ipiq) structure for driver and setup DMA for buffer.
 * 
 * The channel number is passed so that a compatible ipiq can be returned,
 * i.e. one with the appropriate mappings.
 * The command and response buffer lengths are passed in so that eventually
 * we can handle drivers that desire long responses.  
 *
 * Returns NULL if temporarily out of IPIQs.
 * If buffer cannot be setup, returns NULL after setting error flags in buf.
 */
struct ipiq *
ipi_setup(addr, bp, cmdlen, resplen, callback, flags)
	int		addr;		/* IPI address */
	struct buf	*bp;		/* buffer for request (if any) */
	int		cmdlen;
	int		resplen;
	int		(*callback)();	/* callback function */
	int		flags;		/* flags for request */
{
	register struct ipiq	*q;
	register struct ipi_chan *ch;
	register struct ipi_slave *sl;
	register u_int		chan;
	register u_int		slave;

	chan = IPI_CHAN(addr);
	slave = IPI_SLAVE(addr);
	if (chan >= ipi_nchan || slave >= IPI_NSLAVE
		|| (ch = ipi_chan[chan]) == NULL 
		|| ((sl = &ch->ic_slave[slave])->ic_arg) == 0)
	{
		printf("ipi_setup %x: invalid IPI address\n", addr);
		return NULL;
	}

	q = (*ch->ic_driver->id_setup)(sl->ic_arg, bp, cmdlen, resplen, 
		callback, flags);

	if (q != NULL) {
		q->q_result = IP_SETUP;
		q->q_chan = ch;
		q->q_csf = addr;
		q->q_intclass = sl->ic_intpri;
		q->q_ctlr = sl->ic_ctlr;
		q->q_time = ch->ic_slave[slave].ic_time;
	}
	return q;
}

/*
 * Release resources for request.
 */
void
ipi_free(q)
	register struct ipiq	*q;
{
	free_refnum(q);
	(*q->q_chan->ic_driver->id_relse)(q);
}		

/*
 * Send command to channel driver.
 */
void
ipi_start(q, flag, func)
	register struct ipiq *q;	/* request pointer */
	long	flag;			/* flags */
	void 	(*func)();		/* interrupt routine */
{
	struct ipi_chan	*ch;

	ch = q->q_chan;
	ipi_setup_start(q, flag, func);
	ipi_trace6(TRC_IPI_CMD, q->q_csf, q->q_refnum,
		((long *)q->q_cmd)[0], ((long *)q->q_cmd)[1],
		((long *)q->q_cmd)[2], ((long *)q->q_cmd)[3]);
	q->q_chan_func = ch->ic_driver->id_cmd;
	(*q->q_chan_func)(q);
}

static void
ipi_setup_start(q, flag, func)
	register struct ipiq *q;	/* request pointer */
	long	flag;			/* flags */
	void 	(*func)();		/* interrupt routine */
{
	register int	addr;

	q->q_flag |= flag & IP_DRIVER_FLAGS;
	q->q_func = func;
	q->q_result = IP_INPROGRESS;

	/*
	 * Set deadline for completion.
	 */
	if (q->q_time > 0) {
		q->q_time += time.tv_sec - boottime.tv_sec;
	}

	/*
	 * Set IPI layer portion of packet.
	 */
	addr = q->q_csf;
	q->q_cmd->hdr_slave = IPI_SLAVE(addr);;
	q->q_cmd->hdr_facility = IPI_FAC(addr);
	assign_refnum(q);		/* assign command reference number */
}

/*
 * Send reset slave command to channel driver.
 */
void
ipi_reset_slave(q, flag, func)
	register struct ipiq *q;	/* request pointer */
	long	flag;			/* flags */
	void 	(*func)();		/* interrupt routine */
{
	struct ipi_chan	*ch;

	ipi_setup_start(q, flag | IP_NO_RETRY, func);
	ch = q->q_chan;
	ipi_trace2(TRC_IPI_RST_SL, q->q_csf, q->q_refnum);
	q->q_chan_func = ch->ic_driver->id_reset_slave;
	(*q->q_chan_func)(q);
}

/*
 * Send reset channel command to channel driver.
 */
void
ipi_reset_chan(q, flag, func)
	register struct ipiq *q;	/* request pointer */
	long	flag;			/* flags */
	void 	(*func)();		/* interrupt routine */
{
	struct ipi_chan	*ch;

	ch = q->q_chan;

	/*
	 * If this is just a virtual channel, for addressing bus-attached
	 * controllers (like the ISP-80), don't really reset it and the
	 * other controllers.
	 */
	if (!(ch->ic_flags & IC_VIRT_CHAN)) {
		/*
		 * Tell all drivers that their controller will have to be reset.
		 * This should stop them from issuing new requests until the 
		 * controller is reset/initialized.  This routine should 
		 * not issue any I/O of it's own, since it would only get in the
		 * way of the channel reset.
		 */
		ipi_recover_chan(IPI_CHAN(q->q_csf), IPR_REINIT);
	}

	ipi_setup_start(q, flag, func);

	ipi_trace2(TRC_IPI_RST_CH, q->q_csf, q->q_refnum);
	q->q_chan_func = ch->ic_driver->id_reset_chan;
	(*q->q_chan_func)(q);
}

/*
 * Send NO-OP command to channel driver to test board function.
 */
void
ipi_test_board(q, flag, func)
	register struct ipiq *q;	/* request pointer */
	long	flag;			/* flags */
	void 	(*func)();		/* interrupt routine */
{
	struct ipi_chan	*ch;

	ch = q->q_chan;
	ipi_setup_start(q, flag, func);
	ipi_trace2(TRC_IPI_TST_BD, q->q_csf, q->q_refnum);
	q->q_chan_func = ch->ic_driver->id_test_board;
	(*q->q_chan_func)(q);
}

/*
 * Get transfer settings.
 *	Returns transfer setting octet in first byte of buffer.
 */
void
ipi_get_xfer_mode(q, flag, func)
	register struct ipiq *q;	/* request pointer */
	long	flag;			/* flags */
	void 	(*func)();		/* interrupt routine */
{
	struct ipi_chan	*ch;

	ch = q->q_chan;
	ipi_setup_start(q, flag, func);
	ipi_trace2(TRC_IPI_GET_XFR, q->q_csf, q->q_refnum);
	q->q_chan_func = ch->ic_driver->id_get_xfer_mode;
	(*q->q_chan_func)(q);
}

/*
 * Set transfer settings.
 *	Returns transfer setting octet in first byte of buffer.
 */
void
ipi_set_xfer_mode(q, flag, func, mode)
	register struct ipiq *q;	/* request pointer */
	long	flag;			/* flags */
	void 	(*func)();		/* interrupt routine */
	int	mode;			/* transfer mode */
{
	struct ipi_chan	*ch;

	ch = q->q_chan;
	ipi_setup_start(q, flag, func);
	ipi_trace2(TRC_IPI_SET_XFR, q->q_csf, q->q_refnum);
	q->q_chan_func = ch->ic_driver->id_set_xfer_mode;
	(*q->q_chan_func)(q, mode);
}

/*
 * Send reset board command to channel driver.
 */
void
ipi_reset_board(q, flag, func)
	register struct ipiq *q;	/* request pointer */
	long	flag;			/* flags */
	void 	(*func)();		/* interrupt routine */
{
	struct ipi_chan	*ch;

	ch = q->q_chan;
	ipi_setup_start(q, flag, func);
	ipi_trace2(TRC_IPI_RST_BD, q->q_csf, q->q_refnum);
	q->q_chan_func = ch->ic_driver->id_reset_board;
	(*q->q_chan_func)(q);
}

/*
 * Indicate that all slaves should be reset.
 * 	Called by a channel driver with multiple channels 
 *	after a successful reset of the board.
 *	Purpose is to notify device driver that a slave reset is required.
 */
void
ipi_recover_chan(chan, code)
	u_int	chan;
	int	code;
{
	register struct ipi_chan *ch;
	register struct ipi_slave *sl;

	if (chan >= ipi_nchan || (ch = ipi_chan[chan]) == NULL) {
		printf("ipi %x: channel not configured\n", chan);
		return;	
	}
	for (sl = ch->ic_slave; sl < &ch->ic_slave[IPI_NSLAVE]; sl++) {
		if (sl->ic_recover)
			(*sl->ic_recover)(sl->ic_ctlr, code);
	}
}

/*
 * Handle interrupt from channel driver.
 */
void
ipi_intr(q)
	register struct ipiq	*q;
{
	register struct ipi_chan *ch;
	register struct ipiq	*qq;

	ch = q->q_chan;

	/*
	 * See if there are any interrupts queued.  These are
	 * interrupts that couldn't be handled because they occured while
	 * polling for other devices.
	 *
	 * Note that this loop starts at the head of the list each time,
	 * in case the list changes during the driver's interrupt routine.
	 */
	while ((qq = ch->ic_rupt.q_head) != NULL) {
		if ((ch->ic_rupt.q_head = qq->q_next) == NULL)
			ch->ic_rupt.q_tail = NULL;
		if (qq->q_result == IP_ASYNC) {
			(*qq->q_func)(qq);	/* call device driver */
			ipi_free(qq);
		} else {
			(*qq->q_func)(qq);	/* call device driver */
		}
	}

#ifdef IPI_TRACE
	if (q->q_resp) {
		ipi_trace6(TRC_IPI_RUPT, q->q_csf, q->q_refnum,
			((int *)q->q_resp)[0], ((int *)q->q_resp)[1], 
			((int *)q->q_resp)[2], ((int *)q->q_resp)[3]);
	} else {
		ipi_trace2(TRC_IPI_RUPT, q->q_csf, q->q_refnum);
	}
#endif

	/*
	 * If the request is synchronous (using polling, not interrupts),
	 * this probably isn't really an interrupt, but an early presentation
	 * of an error or a simulated completion.  Just queue the request
	 * so ipi_poll will find it.
	 */
	if (q->q_flag & IP_SYNC) {
		q->q_next = NULL;
		ch = q->q_chan;
		if (ch->ic_rupt.q_head == NULL) {
			ch->ic_rupt.q_head = q;
		} else {
			ch->ic_rupt.q_tail->q_next = q;
		}
		ch->ic_rupt.q_tail = q;
	} else {
		if (q->q_result == IP_ASYNC) {
			(*q->q_func)(q);	/* call device driver */
			ipi_free(q);
		} else {
			(*q->q_func)(q);	/* call device driver */
		}
	}
}

/*
 * Reflect error detected by host adaptor layer to driver.
 */
void
ipi_complete(q, err)
	register struct ipiq *q;
	int	err;
{
	if (err)
		q->q_result = err;
	ipi_intr(q);
}

/*
 * Setup request for asynchronous interrupt by channel.  Channel will
 * return the request through ipi_intr or the driver's poll routine.
 * The routine returns non-zero on error.
 * The request should be eventually freed by the channel driver.
 */
int
ipi_async(chan, q)
	int		chan;	/* system channel number */
	struct ipiq	*q;	/* request containing asynchronous response */
{
	struct ipi_chan	*ch;
	struct ipi_slave *sl;
	u_int		slave;

	if (chan >= ipi_nchan || (ch = ipi_chan[chan]) == NULL) {
		printf("ipi_async: IPI channel %x not configured\n", chan);
		return -1;
	}
	q->q_chan = ch;		/* setup channel pointer so ipi_free works */
	q->q_result = IP_ASYNC;
	q->q_csf = IPI_MAKE_ADDR(chan, slave = q->q_resp->hdr_slave, 
			q->q_resp->hdr_facility);
	if (slave < IPI_NSLAVE) {
		sl = &ch->ic_slave[slave];
		q->q_ctlr = sl->ic_ctlr;
		q->q_func = sl->ic_async;
	} else {
		printf("ipi_async: ipi %x slave number out of range\n",
			q->q_csf);
		return -1;
	}
	return 0;
}

/*
 * Wait synchronously for a completion interrupt from a command.
 *	Interrupts for the same unit or controller are passed back to the
 *	driver.  All other interrupts received are held on a queue.
 * 	Always called from the process level with IPI interrupts masked.
 *
 *	Returns non-zero when the request completes.
 * 	Returns zero if the timeout elapses first.
 */
ipi_poll(qq, time_out)
	struct ipiq	*qq;		/* the request being polled */
	long		time_out;	/* approx. time limit in microseconds */
{
	register struct ipi_chan *ch;
	register struct ipiq	*q;
	register struct ipiq	*pq;
	register int	slave, fac, addr;
	register struct ipi3header *ip;

	ch = qq->q_chan;
	ip = qq->q_cmd;
	fac = IPI_MAKE_ADDR(0, ip->hdr_slave, ip->hdr_facility);
	slave = IPI_MAKE_ADDR(0, ip->hdr_slave, IPI_NO_ADDR);

	/*
	 * First see if the interrupt or any others for the controller or
	 * the same device was queued during an earlier poll or by an 
	 * error during ipi_start.
	 */
	pq = NULL;
	for (q = ch->ic_rupt.q_head; q != NULL; pq = q, q = q->q_next) {
		ip = q->q_cmd;
		addr = IPI_MAKE_ADDR(0, ip->hdr_slave, ip->hdr_facility);
		if (addr == slave || addr == fac) {
			if (pq != NULL) 
				pq->q_next = q->q_next;
			else
				ch->ic_rupt.q_head = q->q_next;
			if (ch->ic_rupt.q_tail == q)
				ch->ic_rupt.q_tail = q;
			if (q->q_result == IP_ASYNC) {
				(*q->q_func)(q);	/* call device driver */
				ipi_free(q);
			} else {
				(*q->q_func)(q);	/* call device driver */
			}
			if (qq == q) {
				return (1);
			}
		}
	}	

	time_out /= 0x1000; 	/* approx. time per loop */
	time_out++;
	while (time_out-- >= 0) {
		/*
		 * Call the channel driver's poll routine. 
		 */
		if ((q = (ch->ic_driver->id_poll)(qq)) != NULL) {
#ifdef IPI_TRACE
			if (q->q_resp) {
				ipi_trace6(TRC_IPI_RUPT, q->q_csf, q->q_refnum,
					((int *)q->q_resp)[0],
					((int *)q->q_resp)[1], 
					((int *)q->q_resp)[2],
					((int *)q->q_resp)[3]);
			} else {
				ipi_trace2(TRC_IPI_RUPT, q->q_csf, q->q_refnum);
			}
#endif
			ip = q->q_cmd;
			addr = IPI_MAKE_ADDR(0,ip->hdr_slave,ip->hdr_facility);
			if (addr == slave || addr == fac) {
				if (q->q_result == IP_ASYNC) {
					(*q->q_func)(q);	/* call device driver */
					ipi_free(q);
				} else {
					(*q->q_func)(q);	/* call device driver */
				}
				if (qq == q) 
					return (1);
			} else {
				/*
				 * The interrupt is not for the desired request,
				 * so just queue it on the channel.
				 */
				q->q_next = NULL;
				if (ch->ic_rupt.q_head)
					ch->ic_rupt.q_tail->q_next = q;
				else
					ch->ic_rupt.q_tail = 
						ch->ic_rupt.q_head = q;
			}
		}
		DELAY(0x1000);
	}
	return 0;
}


/*
 * Register channel driver.
 *
 * Called during initialization to set up the IPI layer to call channel driver
 * whenever requests come in for the channel.
 */
ipi_channel(chan, slave, dp, arg, intpri)
	u_int	chan;			/* system channel number */
	u_int	slave;			/* slave number (IPI_NO_ADDR for all) */
	struct ipi_driver *dp;		/* driver routines */
	int	arg;			/* argument to pass driver routines */
	int	intpri;			/* interrupt priority */
{
	struct ipi_chan **new_chan;
	struct ipi_chan	*ch;
	int	i;
	int 	j;

	ipi_init();
	/*
	 * If not enough channel pointers have been allocated, allocate them.
	 */
	if (ipi_nchan <= chan) {
		i = chan + CHANSET - ipi_nchan % CHANSET;
		new_chan = (struct ipi_chan **)
			kmem_zalloc((u_int)i * sizeof(struct ipi_chan *));
		if (new_chan == NULL) {
			printf("ipi_channel: No memory to add channel %d\n", 
				chan);
			return -1;
		}
		/*
		 * copy existing channel pointers.
		 */	
		for (j = 0; j < ipi_nchan; j++) 
			new_chan[j] = ipi_chan[j];		
		/* 
		 * free old pointers
		 */
		kmem_free((caddr_t)ipi_chan, 
			(u_int)ipi_nchan * sizeof(struct ipi_chan *));
		ipi_chan = new_chan;
		ipi_nchan = i;
	}

	/*
	 * Return error if slave is already defined.
	 */
	if ((ch = ipi_chan[chan]) != NULL && (slave == IPI_NO_ADDR
			|| ch->ic_slave[slave].ic_arg != 0 
			|| ch->ic_driver != dp))
	{
		printf("ipi_channel:  channel %d already defined\n", chan);
		return -1;
	}

	/*
	 * setup new channel.
	 */
	if (ch == NULL) {
		ipi_chan[chan] = ch =
			(struct ipi_chan *)kmem_zalloc(sizeof(struct ipi_chan));
		if (ch == NULL) {
			printf("ipi_channel:  no memory to alloc channel %d\n",
				chan);
			return -1;
		}
		ch->ic_chan = chan;		/* system channel number */
		ch->ic_driver = dp;
		if (slave != IPI_NO_ADDR)
			ch->ic_flags |= IC_VIRT_CHAN;
	}

	/*
	 * Setup specified slave or all slaves on the channel.
	 */
	for (i = 0; i < IPI_NSLAVE; i++) {
		if (slave == IPI_NO_ADDR || slave == i) {
			ch->ic_slave[i].ic_arg = arg;
			ch->ic_slave[i].ic_intpri = intpri;
		}
	}

	if (intpri > ipi_pri)
		ipi_pri = intpri;
	return 0;
}

#if 0
/*
 * Free channel.
 * 
 * This is used to undo the declaration of a channel after its configuration
 * routines (probe/slave/attach) have failed.
 */
ipi_free_chan(chan)
	int	chan;
{
	register struct ipi_chan *ch;

	if (chan >= ipi_nchan || (ch = ipi_chan[chan]) == NULL) {
		printf("ipi_free_chan: channel %d not allocated\n", chan);
		return 0;
	}
printf("ipi channel %x deleted\n", chan);

	kmem_free((caddr_t)ch, sizeof(struct ipi_chan));
	ipi_chan[chan] = NULL;
	return 0;
}
#endif

/*
 * Parse response.
 *
 * Call functions in table for each parameter matched.
 * Align parameters apropriately.
 */
void
ipi_parse_resp(q, table, arg)
	struct ipiq	*q;
	struct ipi_resp_table	*table;
	caddr_t		arg;		/* argument for parsing functions */
{
	register struct ipi_resp_table	*rtp;	/* response table pointer */
	register struct ipi3resp *rp;
	u_char		buf[RESP_BUF_LEN];
	register u_char	*cp;
	int		id;		/* parameter id */
	int		plen;		/* parameter length */
	int		rlen;		/* response length */

	rp = q->q_resp;
	rlen = rp->hdr_pktlen + sizeof(rp->hdr_pktlen)
		- sizeof(struct ipi3resp);
	cp = (u_char *)rp + sizeof(struct ipi3resp);

	for (; rlen > 0; cp += plen) {
		if ((plen = *cp + 1) > rlen) { 
			break;
		} 
		rlen -= plen;

		if (plen <= 1)
			continue;
		/*
		 * Find parameter ID in the table.
		 * Stop when reaching zero or matching parameter.
		 */
		id = cp[1];
		for (rtp = table; rtp->rt_parm_id != 0 
				&& rtp->rt_parm_id != id; rtp++) 
			;
		if (rtp->rt_func == NULL)
			continue;
		/*
		 * Check alignment if the parameter contains more than
	 	 * just the length and parameter ID.
		 */
		if (plen > 2 && (((int)cp + 2) % sizeof(long)) != 0) {
			if (plen - 2 > sizeof(buf)) {
panic("ipi_parse_resp: buffer too short");
			}
			bcopy((caddr_t)cp+2, (caddr_t)buf, (u_int)plen-2);
			(*rtp->rt_func)(q, id, buf, plen-2, arg);
		} else {
			(*rtp->rt_func)(q, id, cp+2, plen-2, arg);
		}
	}
}

/*
 * Print IPI parameters from response or command packet for debugging.
 */
static void
ipi_print_parms(bp, cp)
	u_char	*bp;	/* start of packet (for length and offset of parms) */
	u_char	*cp;	/* pointer to start of paramters */
{
	int	len;	/* length remaining in packet */
	int	plen;	/* length of parameter */
	int	col;	/* column number */

	/*
	 * Length is in first short, and doesn't include the 2 bytes for the
	 * length itself.  Subtract out the header length.
	 */
	len = ((struct ipi3header *)bp)->hdr_pktlen + sizeof(u_short) - (cp-bp);

	/*
         * print header for parameters
	 */
	if (len > 0)
		printf("\n  addr  len ID  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f 10\n");

	while (len > 0) {
		plen = *cp + 1;		/* plen includes length byte */
		if (plen > len) {
			printf("Parameter length error:\n");
			plen = len;
		}
		len -= plen;
		printf("  %4x : ", cp - bp);
		col = -2;		/* column number for length */
		while (plen-- > 0) {
			if (col++ >= 0x10) {
				printf("\n  %4x :       ", cp - bp);
				col = 1;
			}
			printf("%2x ", *cp++);
		}
		printf("\n");
	}
	printf("\n");
}

/*
 * Print command packet for debugging and errors.
 */
ipi_print_cmd(chan, ip, message)
	u_int	chan;
	struct ipi3header *ip;
	char	*message;
{
	if (ip == NULL) {
		printf("%s: channel %x, null command\n", message, chan);
		return;
	}
	printf("%s: channel %x  slave %x  facility %x\n", message, chan,
		ip->hdr_slave, ip->hdr_facility);
	printf("IPI command:\n  len %x  refnum %x   opcode %2x  mod %2x\n",
		ip->hdr_pktlen, ip->hdr_refnum, ip->hdr_opcode, ip->hdr_mods);
	ipi_print_parms((u_char *)ip, (u_char *)ip + sizeof(struct ipi3header));
}


/*
 * Print response for debugging and errors.
 */
ipi_print_resp(chan, rp, message)
	u_int	chan;
	struct ipi3resp *rp;
	char	*message;
{
	if (rp == NULL) {
		printf("%s: channel %x, null response\n", message, chan);
		return;
	}
	printf("%s: channel %x  slave %x  facility %x\n", message, chan,
		rp->hdr_slave, rp->hdr_facility);
	printf("IPI Response:\n  len %x  refnum %x   opcode %2x  mod %2x  stat %4x\n",
		rp->hdr_pktlen, rp->hdr_refnum, rp->hdr_opcode, rp->hdr_mods,
		rp->hdr_maj_stat);

	ipi_print_parms((u_char *)rp, (u_char *)rp + sizeof(struct ipi3resp));
}


/*
 * Find the next available command reference number and assign it to
 * this command.  Returns -1 if no free spot found.
 */
void
assign_refnum(q)
	struct ipiq	*q;
{
	register int	i;
	struct ipiq	**lp;

	if (q->q_refnum != 0) {
		printf("ipi: assign_refnum:  q %x already has refnum %d\n",
			q, q->q_refnum);
	}

	/*
	 * Find unused reference number.
	 */
	i = ipi_refnum; 
	for (lp = &ipi_ipiq_lookup[i]; *lp != NULL; ) {
		++lp;
		if (++i >= ipi_lookup_len) {
			i = 1;
			lp = &ipi_ipiq_lookup[1];
		}
	}

	*lp = q;
	q->q_refnum = i;
	q->q_cmd->hdr_refnum = i;

	/*
	 * setup next search start value.
	 */
	ipi_refnum = i + 1;		/* where to start next */
	if (ipi_refnum >= ipi_lookup_len)
		ipi_refnum = 1;
}

/*
 * Remove command reference number without freeing request.
 */
void
free_refnum(q)
	register struct ipiq	*q;
{
	register int	refnum;
	register struct ipiq	**lp;

	if ((refnum = q->q_refnum) != 0) {
		lp = &ipi_ipiq_lookup[refnum];
		if (*lp != q) {
			printf("free_refnum: q %x refnum %x lookup %x\n", 
				q, refnum, *lp);
			panic("free_refnum: freeing wrong reference number");
		} else {
	 		*lp = NULL;
		}
	}
}

/*
 * Lookup ipiq from command reference number.
 */
struct ipiq *
ipi_lookup(refnum)
	u_short	refnum;
{
	register struct ipiq	*q;

	if (refnum >= ipi_lookup_len)
		return NULL;
	q = ipi_ipiq_lookup[refnum];
	if (q != NULL && q->q_refnum != refnum) {
		printf("ipi_lookup:  found wrong command for refnum\n");
		printf("ipi_lookup:  looking for %x, found %x q %x\n",
			refnum, q->q_refnum, q);
		q = NULL;
	}
	return q;
}

/*
 * Lookup ipiq from command I/O address.
 */
struct ipiq *
ipi_lookup_cmdaddr(io_addr)
	caddr_t		io_addr;
{
	register struct ipiq	*q;
	register struct ipiq_chunk *qcp;

	for (qcp = ipiq_chunk_list.qc_next; qcp != &ipiq_chunk_list;
		qcp = qcp->qc_next)
	{
		for (q = qcp->qc_ipiqs; q < qcp->qc_end; q++)
			if (q->q_cmd_ioaddr == io_addr)
				return q;
	}
	return NULL;
}

/*
 * Set timeout value for channel (in seconds).
 *	returns old timeout value, or -1 if error.
 * 
 *	The timeout is for all facilities on a slave.
 */
ipi_timeout(csf, new_time)
	int	csf;		/* channel/slave/facility address */
	int	new_time;
{
	int	i;
	int	old_time;
	struct ipi_chan	*ch;

	if ((i = IPI_CHAN(csf)) >= ipi_nchan || (ch = ipi_chan[i]) == NULL)
		return -1;
	if ((i = IPI_SLAVE(csf)) >= IPI_NSLAVE)
		return -1;
	if (new_time < 0)
		return -1;
	old_time = ch->ic_slave[i].ic_time;
	ch->ic_slave[i].ic_time = new_time;
	return old_time;
}

/*
 * Check for hung requests.
 */
static void
ipi_watch()
{
	register struct ipiq_chunk *qcp;
	register struct ipiq	*q;
	int	time_since_boot;	/* time since boot in seconds */
	int	s;

	time_since_boot = time.tv_sec - boottime.tv_sec;

	for (qcp = ipiq_chunk_list.qc_next; qcp != &ipiq_chunk_list;
		qcp = qcp->qc_next)
	{
		for (q = qcp->qc_ipiqs; q < qcp->qc_end; q++) {
			if (q->q_time <= 0 || q->q_result != IP_INPROGRESS)
				continue;
			if (q->q_time < time_since_boot) {
				/* 
				 * It looks late.  mask and check again. 
				 */
				s = spl_ipi();
				if (q->q_time >= 0 && q->q_time < time_since_boot
					&& q->q_result == IP_INPROGRESS)
				{
					printf("ipi %x: missing interrupt.  refnum %x\n", 
						q->q_csf, q->q_refnum);
					q->q_result = IP_MISSING;
					q->q_time = ipi_retry_timeout;
					(*q->q_func)(q);
				}
				(void) splx(s);	
			}
		}
	}
	timeout((int (*)())ipi_watch, (caddr_t)NULL, IPI_WATCH_TIME);
}

/*
 * Retry.  Use original IPIQ to re-issue command.
 * 	Change the command reference number to make it possible to detect
 *	problems associated with the original request coming back.
 */
void
ipi_retry(q)
	register struct ipiq	*q;	/* request which had failed */
{
	if (q->q_retry++ >= ipi_retry_limit) {
		ipi_complete(q, IP_RETRY_FAILED);
	} else {
		/*
		 * Re-assign command reference number.
		 * XXX - channel driver should be notified of refnum change.
		 */
		free_refnum(q);
		q->q_refnum = 0;
		ipi_setup_start(q, q->q_flag, q->q_func);
		ipi_trace6(TRC_IPI_RETRY, q->q_csf, q->q_refnum,
			((long *)q->q_cmd)[0], ((long *)q->q_cmd)[1],
			((long *)q->q_cmd)[2], ((long *)q->q_cmd)[3]);
		(*q->q_chan_func)(q);
	}
}


/*
 * Ask device driver to retry all requests for a particular controller.
 * The request passed as an argument is not affected.
 * If slave is passed as IPI_NO_ADDR, all slaves on the channel are affected.
 * 
 * This is necessary during controller or channel resets.
 */
void
ipi_retry_slave(qq, slave)
	struct ipiq	*qq;
	int		slave;
{
	register struct ipi_chan	*ch;
	register struct ipiq_chunk	*qcp;
	register struct ipiq		*q;

	ch = qq->q_chan;

	for (qcp = ipiq_chunk_list.qc_next; qcp != &ipiq_chunk_list;
		qcp = qcp->qc_next)
	{
		for (q = qcp->qc_ipiqs; q < qcp->qc_end; q++) {
			if (q->q_chan == ch && q != qq &&
				(slave == IPI_NO_ADDR 
					|| IPI_SLAVE(q->q_csf) == slave)
				&& (q->q_result == IP_INPROGRESS
					|| q->q_result == IP_MISSING))
			{
				if (q->q_flag & IP_NO_RETRY)
					ipi_complete(q, IP_RETRY_FAILED);
				else
					ipi_complete(q, IP_RETRY);
			}
		}
	}
}

/*
 * Allocate resources for future requests.
 *	Called during controller initialization by device driver.
 */
int
ipi_alloc(csf, count, size) 
	int	csf;		/* IPI address of controller */
	int	count;		/* upper bound on number of structs needed */
	int	size;		/* packet size required */
{
	register struct ipi_chan *ch;
	register struct ipi_slave *sl;
	register u_int		chan;
	register u_int		slave;

	chan = IPI_CHAN(csf);
	slave = IPI_SLAVE(csf);
	if (chan >= ipi_nchan || slave >= IPI_NSLAVE
		|| (ch = ipi_chan[chan]) == NULL 
		|| ((sl = &ch->ic_slave[slave])->ic_arg) == 0)
	{
		printf("ipi_alloc %x: invalid IPI address\n", csf);
		return 0;
	}
	return (*ch->ic_driver->id_alloc)(sl->ic_arg, slave, count, size);
}


/*
 * Common Queue free list management for IPI channels.
 */
struct ipiq *
ipiq_get(free_list, size, callback)
	struct ipiq_free_list *free_list;
	int	size;		/* length of command packet desired. */
	int	(*callback)();	/* routine to call when ipiq available */
{
	struct ipiq_free_list	*flp;
	struct ipiq		*q;

	/*
	 * Find non-empty list that's big enough.  Use callback or sleep
	 * if no requests are found.
	 */
	for (;;)  {
		for (flp = free_list; flp->fl_size > 0; flp++)
			if (size <= flp->fl_size && flp->fl_list.q_head)
				break;
		if (flp->fl_size > 0)
			break;
		ipiq_stats.i_runout++;
		if (callback == IPI_SLEEP) {
			ipiq_wait();
		} else if (callback == IPI_NULL) {
			return NULL;
		} else {
			ipiq_callback(callback);
			return NULL;
		}
	}

	q = flp->fl_list.q_head;
	flp->fl_list.q_head = q->q_next;

	/*
	 * Clean up the IPIQ for debugging -- delete some later for performance.
	 */
	q->q_func = NULL;
	q->q_dev = NULL;
	q->q_ctlr = NULL;
	q->q_buf = NULL;
	q->q_flag = 0;
	q->q_next = NULL;
	q->q_chan = NULL;
	q->q_result = IP_INPROGRESS;
	q->q_resp = NULL;
	q->q_respq = NULL;
	q->q_time = ipi_default_timeout;
	q->q_retry = 0;
	q->q_cmd = (struct ipi3header *)q->q_cmd_pkt;
	q->q_cmd->hdr_pktlen = 0;
	return q;
}


/*
 * Free request
 */
void
ipiq_free(flp, q)
	struct ipiq_free_list *flp;
	register struct ipiq	*q;
{
	int	size;

	/*
	 * Find the right list.  There should be an exact match.
	 */
	size = q->q_cmd_len;
	for (; flp->fl_size < size; flp++)
		;
	ASSERT(flp->fl_size == size);

	q->q_time = 0;
	q->q_next = NULL;
	q->q_result = IP_FREE;

	/*
	 * If panicking, put free queue back on front of list so dump
	 * doesn't use all of them.  Otherwise, put queue on end of list
	 * to preserve evidence in case of crash.
	 */
	if (panicstr) {	
		q->q_next = flp->fl_list.q_head;
		if (flp->fl_list.q_head == NULL)
			flp->fl_list.q_tail = q;
		flp->fl_list.q_head = q;
	} else {
		if (flp->fl_list.q_head == NULL) {
			flp->fl_list.q_head = q;
			flp->fl_list.q_tail = q;
		} else {
			flp->fl_list.q_tail->q_next = q;
			flp->fl_list.q_tail = q;
		}
	}
	if (ipiq_free_wait > 0) {
		ipiq_free_wait--;
		wakeup((caddr_t)&ipiq_free_wait);
	}
	if (ipiq_callback_len)
		(void) ipiq_call();
}


/*
 * Add a number of ipiqs to the given free list.
 * The ipiqs in the chunk have been linked to each other front to back.
 */
void
ipiq_free_chunk(flp, qcp)
	struct ipiq_free_list	*flp;		/* free list */
	register struct ipiq_chunk *qcp;	/* pointer to "chunk" */
{
	struct ipiq	*q;

	q = qcp->qc_end  - 1;		/* point to last q in chunk */
	q->q_next = NULL;
	if (flp->fl_list.q_head == NULL) {
		flp->fl_list.q_head = qcp->qc_ipiqs;
	} else {
		flp->fl_list.q_tail->q_next = qcp->qc_ipiqs;
	}
	flp->fl_list.q_tail = q;

	/*
	 * link on to system-wide list of all IPIQs.
	 */
	qcp->qc_next = ipiq_chunk_list.qc_next;
	ipiq_chunk_list.qc_next = qcp;
}

/*
 * Wait for there to be more free queue elements.
 */
ipiq_wait()
{
	ipiq_free_wait++;
	(void) sleep((caddr_t)&ipiq_free_wait, PRIBIO);
}

/*
 * Schedule callback when ipiq is available.
 */
static void
ipiq_callback(fp)
	int	(*fp)();
{
	struct ipi_callback	*p;

	p = ipiq_cb_out;
	while (p != ipiq_cb_in) {
		if (p->ic_fcn == fp) {
			return;
		}
		if (++p >= &ipiq_callback_list[IPIQ_CBQ_LEN])
			p = ipiq_callback_list;
	}
	if (++ipiq_callback_len > ipiq_stats.i_max_qlen)
		ipiq_stats.i_max_qlen++;
	p = ipiq_cb_in;
	p->ic_fcn = fp;
	if (++p >= &ipiq_callback_list[IPIQ_CBQ_LEN])
		p = ipiq_callback_list;
	ipiq_cb_in = p;
	ASSERT(ipiq_cb_in != ipiq_cb_out);
}

/*
 * Call all callback routines currently in the list.
 *	New callbacks can be added by the ones called, but we won't call them.
 */
static int
ipiq_call()
{
	struct ipi_callback	*p;
	struct ipi_callback	*end;
	int	(*fp)();

	p = ipiq_cb_out;
	end = ipiq_cb_in;
	while (ipiq_callback_len > 0)  {
		if (++p >= &ipiq_callback_list[IPIQ_CBQ_LEN])
			p = ipiq_callback_list;
		if (p == end)
			break;
		ipiq_cb_out = p;
		ipiq_callback_len--;
		fp = p->ic_fcn;
		p->ic_fcn = NULL;
		(void) (*fp)((caddr_t) 0);
	}
}
