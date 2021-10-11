/*	@(#)ipi_driver.h	1.1 7/30/92 SMI */

/*
 * Definitions for IPI-3 device drivers.
 */
#ifndef	_ipi_driver_h
#define	_ipi_driver_h

/*
 * Macros to convert IPI device addresses to channel, slave, and facility 
 * numbers, and to form addresses from those numbers.
 *
 * For convenience, the channel, slave, and facility numbers are combined
 * into a 32 bit address:   16 bit channel, 8 bit slave, 8 bit facility.
 * The slave and facility numbers may be 0xff to indicate no slave or
 * no facility is being addressed.  If the facility is 0xff, the controller
 * itself is being addressed.
 */
#define IPI_NO_ADDR		(0xff)
#define IPI_NSLAVE		8		/* slaves per channel */
#define IPI_NFAC		16		/* max facilities per slave */

#define IPI_MAKE_ADDR(c,s,f)	((c)<<16|(s)<<8|(f))
#define	IPI_CHAN(a)		((unsigned int)(a)>>16)
#define IPI_SLAVE(a)		(((int)(a)>>8)&0xff)
#define IPI_FAC(a)		((int)(a)&0xff)
#define IPI_CTLR_ADDR(a)	((int)(a)|IPI_NO_ADDR)

/*
 * IPI queue request element.
 *
 * This structure must be a multiple of 4 bytes long for intparms to work.
 *
 * Only the following fields can be modified by device drivers:
 *
 *   q_dev		driver can set anytime
 *   q_ctlr		driver can set anytime.  IPI layer inits to ctlr arg.
 *   q_dma_len		driver can reduce to actual length to be transferred.
 *   q_buf		points to buffer.  Driver may modify.
 *   q_next		driver can modify after completion or before start.
 *   q_time		time limit.  Driver may set after setup before start.
 *   q_result		result/state of request - modify only after completion
 *
 * The following fields can be referenced but not modified by device drivers:
 *   q_cmd		points to command packet which driver fills in.
 *   q_tnp		points to transfer notification parameter data
 *   q_tnp_len		length of transfer notification parameter data
 *   q_flag		flags of request
 *   q_csf		IPI address which was supplied at setup.
 *   q_resp		points to response packet if any.
 */
struct ipiq {
	/*
	 * The portion that driver is permitted to reference.
	 */
	struct ipi3header *q_cmd;	/* command packet */
	struct ipiq 	*q_next;	/* next element in queue */
	caddr_t		q_dev;		/* data for device driver */
	caddr_t		q_ctlr;		/* data for device driver */
/* 10 */
	struct buf	*q_buf;		/* block I/O buffer */
	long		q_time;		/* timeout value in seconds */
	u_short		q_filler;	/* fill for intparm alignment */
	u_short		q_refnum;	/* command reference number of ipiq */
	char		*q_tnp;		/* transfer notification parameter */
/* 20 */
	u_char		q_tnp_len;	/* length of transfer notification */
	u_char		q_retry;	/* number of times retry attempted */
	u_char		q_intclass;	/* interrupt class (no driver access) */
	u_char		q_result;	/* code summarizing response */

	/*
	 * These fields are presented to the driver at interrupt time.
	 * They may be examined but not modified by the driver.
	 */
	long		q_flag;		/* flags for request */
	struct ipi3resp *q_resp;	/* response packet if any */
	int		q_csf;		/* channel/slave/facility address */
/* 30 */

	/* 
	 * The portion that is reserved for host adaptor layer and the
	 * channel board drivers.
	 */
	void		(*q_func)();	/* interrupt handler function */
	caddr_t		q_dma_info;	/* DMA info for channel driver */
	u_long		q_dma_len;	/* byte count of transfer. */
					/* len can be reduced by dev driver */
	struct ipi_chan	*q_chan;	/* channel */ 
/* 40 */
	caddr_t		q_cmd_pkt;	/* kernel address of command packet */
	caddr_t		q_cmd_ioaddr;	/* DVMA or phys addr of cmd packet */
	int		q_cmd_len;	/* length of external packet */
	struct ipiq	*q_respq;	/* ipiq containing response */
/* 50 */
	void		(*q_chan_func)(); /* channel routine for retry */
	caddr_t		q_ch_data;	/* Channel driver data pointer */
	long		q_dev_data[2];	/* data for upper level device driver */
};

/*
 * On completion of IPI channel commands, the interrupt handling function
 * is called.  It should be declared like this:
 *
 *      ipi_handler(q)
 *	struct ipiq *q;		request causing interrupt
 *
 * The result of the operation is summarized in q->q_result by one of the
 * following values:
 */	
#define IP_INPROGRESS	0	/* issued but not done yet */
#define IP_SUCCESS	1	/* successful completion */
#define IP_ERROR	2	/* error during transfer - see response */
#define IP_COMPLETE	3	/* complete but response needs analysis */
#define IP_OFFLINE	4	/* channel or device not available */
#define IP_INVAL	5	/* host software detected invalid command */
#define IP_MISSING	7	/* time limit expired - abort recommended */
#define IP_RETRY	8	/* channel system requests retry */
#define IP_RETRY_FAILED	9	/* retry limit exceeded */
#define IP_FREE		10	/* on free list */
#define IP_SETUP	11	/* setup for driver or internal use */
#define IP_ASYNC	12	/* asynchronous interrupt */

/*
 * Flags in ipiq.
 */
#define IP_ABORTED	2	/* command aborted by driver */
#define IP_SYNC		4	/* driver will poll for interrupt */
#define IP_RESP_SUCCESS 8	/* store response on success */
#define IP_LOG_RESET	0x20	/* for reset_slave: do logical reset */
#define IP_NO_RETRY	0x40	/* do not retry this cmd on controller resets */
#define IP_NO_DMA_SETUP	0x80	/* DMA setup before ipi_setup (for dumps) */
#define IP_DD_FLAG0	0x1000	/* flags for device drivers to pass */
#define IP_DD_FLAG1	0x2000
#define IP_DD_FLAG2	0x4000
#define IP_DD_FLAG3	0x8000
#define IP_DD_FLAG4	0x10000
#define IP_DD_FLAG5	0x20000
#define IP_DD_FLAG6	0x40000
#define IP_DD_FLAG7	0x80000
#define IP_DD_FLAGS	0xff000	/* mask for driver flags */
/*
 * Mask for driver-settable flags.
 */
#define IP_DRIVER_FLAGS	\
	(IP_SYNC | IP_RESP_SUCCESS \
		| IP_NO_RETRY | IP_LOG_RESET | IP_NO_DMA_SETUP \
		| IP_DD_FLAGS)

/*
 * Special callback routine arguments for ipi_setup().
 */
#define IPI_NULL   ((int (*)()) 0)	/* return NULL if no resource free */
#define IPI_SLEEP  ((int (*)()) 1)	/* sleep until resources available */

/*
 * Table of response handling functions.
 * 	Terminated by zero entry in rt_parm_id.
 * 	These tables are passed to ipi_parse_resp() with a pointer to the
 *	ipiq.  The handling function for the parameters that are present
 *	are called with the ipiq and the parameter.  They return after
 *	setting flags in the ipiq for use by the driver.  The parameters
 *	may occur in any order.
 *	The table ends with a zero parameter ID.  If that entry has a 
 *	function pointer, that function is called to handle any parameters
 *	that aren't explicitly mentioned in the rest of the table.
 *
 * Call to ipi_parse_resp:
 *
 *	ipi_parse_resp(q, table, arg)
 *		struct ipiq *q;
 *		struct ipi_resp_table *table;
 *		caddr_t	arg;
 * 
 * Handling function call:
 *
 * 	func(q, parm_id, parm, len, arg)
 *		struct ipiq *q;	
 *		u_char	*parm;
 *		int	parm_id, len;
 *		caddr_t arg;
 */
struct ipi_resp_table {
	char	rt_parm_id;	/* parameter ID (0 if end of table) */
	char	rt_min_len;	/* minimum length of parameter */
	int	(*rt_func)();	/* handling function */
};

#define spl_ipi()		(splvm())
#define spl_ipi_device(csf)	(splvm())
	/* XXX - this isn't done yet.  spl_ipi */

/*
 * Orders for slave recovery routines.
 */
#define	IPR_RESET	0	/* slave reset required before proceding */
#define IPR_REINIT	1	/* reinitialize: logical slave reset required */

/*
 * Miscellaneous constants
 */
#define IPI_RETRY_LIMIT	3	/* Number of retries before giving up */

/*
 * Definitions for Get/Set transfer mode.
 * These must match IPI-3 and IOCB declarations.
 */
#define IP_STREAM	1	/* Use data streaming for command/response */
#define IP_STREAM_20	2	/* Use 20 MB/sec streaming (otherwise 10) */
 
#define IP_INLK_CAP	0x04	/* Can send cmd/resp in interlocked mode */
#define IP_STREAM_CAP	0x08	/* Can send cmd/resp using streaming */
#define IP_STREAM_MODE	0x10	/* Now using streaming for cmd/resp */

/*
 * Routines for device drivers.
 */
extern struct ipiq *	ipi_setup();
extern void		ipi_free();
extern int		ipi_poll();
extern int		ipi_device();
extern void		ipi_start();
extern void		ipi_test_board();
extern void		ipi_reset_board();
extern void		ipi_reset_chan();
extern void		ipi_reset_slave();
extern void		ipi_retry();
extern void		ipi_get_xfer_mode();
extern void		ipi_set_xfer_mode();
extern int		ipi_alloc();

/*
 * Routines for channel driver
 */
extern void		ipi_parse_resp();
extern struct ipiq *	ipi_lookup();
extern void		ipi_complete();
#endif /* _ipi_driver_h */
