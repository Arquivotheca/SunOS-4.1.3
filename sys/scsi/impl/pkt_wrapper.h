/*	@(#)pkt_wrapper.h 1.1 92/07/30 SMI	*/

#ifndef	_scsi_impl_pktwrapper_h
#define	_scsi_impl_pktwrapper_h


/*
 * Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 * Implementation specific SCSI definitions for wrapping around
 * the generic scsi command packet.
 */

#include	<scsi/scsi_types.h>

struct dataseg {
	u_long	d_base;		/* base of current segment */
	u_long	d_count;	/* length of current segment */
	struct dataseg *d_next;	/* pointer to next segment */
};

struct scsi_cmd {
	struct scsi_pkt		cmd_pkt;	/* the generic packet itself */
	caddr_t			cmd_cdbp;	/* active command pointer */
	caddr_t			cmd_scbp;	/* active status pointer */
	u_long			cmd_data;	/* active data 'pointer' */
	u_long			cmd_saved_data;	/* saved data 'pointer' */
	struct dataseg		cmd_mapseg;	/* mapped in data segment */
	struct dataseg		cmd_subseg;	/* initial data sub-segment */
	struct dataseg		*cmd_cursubseg;	/* current data sub-segment */
	u_long			cmd_timeout;	/* command timeout */
	u_short			cmd_flags;	/* private flags */
	u_char			cmd_scblen;	/* length of scb */
	u_char			cmd_scb[STATUS_SIZE];	/* 3 byte scb */
	u_char			cmd_cdblen;	/* length of cdb */
	u_char			cmd_cdb[CDB_SIZE];	/* 'generic' Sun cdb */
};

/*
 * compatibility defines- if only one data segment is mapped in (default case)
 * this will suffice.
 */

#define	cmd_mapping	cmd_mapseg.d_base
#define	cmd_dmacount	cmd_mapseg.d_count

/*
 * A note about dataseg structures:
 *
 *	The cmd_mapseg is the beginning of a list of mapped in memory
 *	(mapped in by dma allocation routines). This mapping is what
 *	is appropriate for I/O devices on a given architecture. For
 *	most current Sun platforms, this will be a single mapping
 *	established in DVMA space.
 *
 *	The cmd_subseg and cmd_cursubseg tags are for use by the host
 *	adapter to keep track of data transfers to and from the memory
 *	mappings described in cmd_mapseg. If a SCSI target uses the
 *	MODIFY DATA POINTER message, or some combination of the SAVE
 *	DATA POINTER/RESTORE POINTERS messages, this mechanism allows
 *	the host adapter to keep track of whether data was retransmitted
 *	by the target, or whether the host adapter picked up or sent only
 *	bits and pieces of the data.
 *	
 * A note about the data 'pointers':
 *
 *	In this implementation, these pointers are actually 
 *	DVMA mapping cookies- that is, they specify the mapping
 *	register and offset from the beginning of DVMA. On many
 *	baseline Sun machines DVMA hardware refers to specific set
 *	of mapping registers that translate Multibus or VME dma
 *	requests (in the range of 0 to 256kb or 0 to 1 mb resp.)
 *	into physical OBMEM references. On those machines, the
 *	SunOS kernel maintains alias addressing of the same
 *	physical locations by wiring down the same amount of space
 *	that can then be referenced through the kernel addresses
 *	DVMA[0..mmu_ptob(ndvmapages)]. On some other Sun machines,
 *	the actual hardware implementation may be different (i.e.,
 *	there is no actual specific extra hardware- the dma
 *	requests may actually use the same MMU mapping as the
 *	kernel), but for all Sun machines to date, there is
 *	always some meaning to DVMA. Each specific host adapter,
 *	if it assumes dma information (i.e., data 'pointers'
 *	here) is a DVMA mapping cookie, can provide the appropriate
 *	and specific translation to that required by its own
 *	dma engine.
 *
 *
 * A note about the cmd_cdb && cmd_scb structures:
 *
 * 	If the command allocation requested exceeds the size of c_cdb,
 *	the cdb will be allocated outside this structure (via kmem_alloc)
 *	The same applies to cmd_scb.
 *
 */

/*
 * These are the defined flags for this structure
 */

#define	CFLAG_DMAVALID	0x01		/* dma mapping valid */
#define	CFLAG_DMASEND	0x02		/* data is going 'out' */
#define	CFLAG_DMAKEEP	0x04		/* don't unmap on dma free */
#define	CFLAG_CMDDISC	0x10		/* cmd currently disconnected */
#define	CFLAG_WATCH	0x20		/* watchdog time for this command */
#define	CFLAG_CDBEXTERN	0x40		/* cdb kmem_alloc'd */
#define	CFLAG_SCBEXTERN	0x80		/* scb kmem_alloc'd */
#define	CFLAG_CMDPROXY	0x100		/*
					 * cmd is a 'proxy' command -
					 * i.e., run by the host adapter
					 * for specific message reasons
					 * (e.g., ABORT or RESET operations)
					 */
#define	CFLAG_NEEDSEG	0x1000		/*
					 * Need a new dma segment on next
					 * data phase.
					 */

#ifdef	KERNEL
extern int scsi_chkdma();
#endif

#endif	_scsi_impl_pktwrapper_h
