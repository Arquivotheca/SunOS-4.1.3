/*	@(#)ipi_chan.h	1.1 7/30/92 SMI */

#ifndef	_ipi_chan_h
#define	_ipi_chan_h

/*
 * Declarations for IPI host-adaptor (channel) drivers.
 */

#include <sundev/ipi_driver.h>

/*
 * IPI request queue header.
 */
struct ipiq_list {
	struct ipiq	*q_head;	/* pointer to first element */
	struct ipiq	*q_tail;	/* pointer to last element */
};

/*
 * There are multiple lists of free IPI queues, sorted by length of packets.
 * These are managed by the ipiq_free_list structure below.
 *
 * These could be LIFO, but make them FIFO so recently used command 
 * buffers aren't reused right away.  This may be worse for performance
 * (because of cache corruption and extra link work) but better for 
 * problem tracking.
 */
struct ipiq_free_list {
	short	fl_size;		/* size of command packet */
	short	fl_init;		/* initial number of reqs this size */
	struct ipiq_list fl_list;	/* list headers */
};

/*
 * Allocation chunk header. 
 * 
 * IPI request structures are allocated in groups linked together through
 * this header structure.  This is so that timeouts and other searches through
 * all IPIQs can work without allocating another pointer per request.
 */
struct ipiq_chunk {
	struct ipiq_chunk *qc_next;	/* next allocation */
	struct ipiq	*qc_end;	/* pointer past last ipiq in chunk */
	struct ipiq	qc_ipiqs[1];	/* start of allocation */
};
extern struct ipiq_chunk ipiq_chunk_list;	/* list header */

/*
 * Structure declaring the driver for a channel.
 *
 * There is one of these for each type of channel supported.
 * This is usually initialized at compile time in the channel board driver 
 * and passed to ipi_channel() to "register" the channel for use.
 */
struct ipi_driver {
	void		(*id_cmd)();		/* send command */
	struct ipiq *	(*id_poll)();		/* poll for interrupt */
	struct ipiq *	(*id_setup)();		/* allocate cmd and setup DMA */
	void		(*id_relse)();		/* free command packet */
	void		(*id_reset_board)();	/* board reset routine */
	void		(*id_reset_chan)();	/* reset channel */
	void		(*id_reset_slave)();	/* reset slave */
	void		(*id_test_board)();	/* test board */
	void		(*id_get_xfer_mode)();	/* get transfer settings */
	void		(*id_set_xfer_mode)();	/* set transfer settings */
	int		(*id_alloc)();		/* allocate ipiqs */
};

/*
 * Structure for keeping track of the channels.
 *
 * There is one of these for each configured channel.
 */
struct ipi_chan {
	struct ipi_driver *ic_driver;	/* driver declaration */
	int	ic_chan;		/* system channel number */
	int	ic_flags;		/* flags for channel */
	struct ipiq_list ic_rupt;	/* interrupt queue */
	struct ipi_slave {
		int	ic_arg;		/* arg for setup routine */
		void	(*ic_async)();	/* driver asynch interrupt handler */
		void	(*ic_recover)(); /* driver recovery routine */
		caddr_t	ic_ctlr;	/* driver info for ipiq structures */
		int	ic_time;	/* time limit (sec) on requests */
		u_char	ic_intpri;	/* interrupt priority */
	} ic_slave[IPI_NSLAVE];
};

/*
 * Channel flags.
 */
#define	IC_VIRT_CHAN	1		/* virtual chan - bus-attached ctlrs */

extern int	ipi_channel();		/* declare channel to system */
extern void	ipi_intr();		/* present interrupt */
extern void	ipi_complete();		/* present completion with error */
extern void	ipi_recover_chan();	/* indicate channel has been reset */
extern void	ipi_retry_slave();	/* issue retries for controllers */
extern int	ipi_async();		/* setup request for async rupt */
struct ipiq *	ipi_lookup_cmdaddr();	/* lookup ipiq by command address */

extern struct ipiq * 	ipiq_get();
extern void		ipiq_free();
extern void		ipiq_free_chunk();

#endif /* _ipi_chan_h */
