/*	@(#)mbvar.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987-1991 by Sun Microsystems, Inc.
 */

#ifndef _sundev_mbvar_h
#define _sundev_mbvar_h

/*
 * This file contains definitions related to the kernel structures
 * for dealing with the Mainbus, or definitions of kernel routines
 * that control allocation of DVMA resources.
 *
 * For information relating to OPENPROM autoconfig structures (e.g.,
 * devinfo), see <sun/openprom.h>
 */

#include <sys/types.h>
#include <sys/buf.h>
#include <machine/param.h>
#include <sun/openprom.h>

/*
 *
 * The Mainbus has an mb_hd structure.
 * Each Mainbus controller which is not a device has an mb_ctlr structure.
 * Each Mainbus device has an mb_device structure.
 *
 */

/*
 * The Mainbus structure.
 *
 * At boot time we determine the devices attached to the Mainbus.
 *
 * During normal operation, resources are allocated and returned
 * to the structures here. 
 * 
 * When Mainbus resources are needed and not available, or if a device
 * which can tolerate no other Mainbus activity gets on the bus,
 * then device drivers may have to wait to get to the bus and are
 * queued here.
 */
struct	mb_hd {
	struct	mb_ctlr *mh_actf;	/* head of queue to transfer */
	struct	mb_ctlr *mh_actl;	/* tail of queue to transfer */
	struct	map *mh_map;		/* DVMA resource map */
	short	mh_users;		/* transient use count */
	short	mh_xclu;		/* a bus hog is using the Mainbus */
};

/*
 * Per-controller structure.
 * (E.g. one for each disk and tape controller)
 *
 * If a controller has devices attached, then there are
 * cross-referenced mb_device structures.
 * This structure is the one which is queued in Mainbus resource wait,
 * and saves the information about Mainbus resources which are used.
 * The queue of devices waiting to transfer is also attached here.
 */
struct mb_ctlr {
	struct	mb_driver *mc_driver;
	short	mc_ctlr;	/* controller index in driver */
	short	mc_alive;	/* controller exists */
	caddr_t	mc_addr;	/* address of device in i/o space */
	int	mc_intpri;	/* interrupt priority level */
#ifdef	sun386
	int	mc_intr;	/* interrupt request channel */
#else
	struct	vec *mc_intr;	/* if vectored interrupts used */
#endif	sun386
	u_int	mc_space;	/* encode bits for addr device space */
#ifdef	sun386
	int mc_dmachan;		/* dma channel */
#endif	sun386
	struct	mb_hd *mc_mh;
	int	mc_mbinfo;	/* Mainbus resource allocation info */
	char	mc_rw;		/* READ or WRITE */
	struct	buf mc_tab;	/* queue of devices for this controller */
	struct	mb_ctlr	*mc_forw;	/* link in transfer queue */
};

/*
 * Per ``device'' structure.
 * (A controller has devices or uses and releases Mainbus memory).
 * (Everything else is a ``device''.)
 *
 * If a controller has many drives attached, then there will
 * be several mb_device structures associated with a single mb_ctlr
 * structure.  Some of the new drivers use this structure in conjunction
 * with the mb_ctlr structure to queue for Mainbus resources.
 *
 * This structure contains all the information necessary to run a Mainbus
 * device such as a serial line interface.  It also contains information
 * for slaves of Mainbus controllers as to which device on the slave
 * this is.  A flags field here can also be given in the system specification
 * and is used to tell which mux lines are hard wired or other device
 * specific parameters.
 */
struct mb_device {
	struct	mb_driver *md_driver;
	short	md_unit;	/* unit number on the system */
	short	md_ctlr;	/* mass ctlr number; -1 if none */
	short	md_slave;	/* slave on controller */
	caddr_t	md_addr;	/* address of device in i/o space */
	int	md_intpri;	/* interrupt priority */
	short	md_dk;		/* if init 1 set to number for iostat */
	int	md_flags;	/* parameter from system specification */
#ifdef	sun386
	int	md_intr;	/* interrupt request channel */
#else
	struct	vec *md_intr;	/* if vectored interrupts used */
#endif	sun386
	u_int	md_space;	/* encode bits for addr device space */
#ifdef	sun386
	short	md_dmachan;	/* dma channel */
#endif	sun386
	short	md_alive;	/* device exists */
	short	md_type;	/* driver specific type information */
/* if the device is connected to a controller, this is the controller */
	struct	mb_ctlr *md_mc;
	struct	mb_hd *md_hd;
	struct	diskhd md_utab;		/* queueing per unit */
	struct	mb_device *md_forw;	/* link in transfer queue */
	int 	md_mbinfo;		/* Mainbus resource allocation info */
};

/*
 * Per-driver structure.
 *
 * Each Mainbus driver defines entries for a set of routines
 * as well as an array of types which are acceptable to it.
 * These are used at boot time by the configuration program.
 */
struct mb_driver {
	int	(*mdr_probe)();		/* see if a driver is really there */
	int	(*mdr_slave)();		/* see if a slave is there */
	int	(*mdr_attach)();	/* setup driver for a slave */
	int	(*mdr_go)();		/* routine to start transfer */
	int	(*mdr_done)();		/* routine to finish transfer */
	int	(*mdr_intr)();		/* polling interrupt routine */
	int	mdr_size;		/* amount of memory space needed */
	char	*mdr_dname;		/* name of a device */
	struct	mb_device **mdr_dinfo;	/* backpointers to mbdinit structs */
	char	*mdr_cname;		/* name of a controller */
	struct	mb_ctlr **mdr_cinfo;	/* backpointers to mbcinit structs */
	short	mdr_flags;		/* Mainbus usage flags */
	struct	mb_driver *mdr_link;	/* interrupt routine linked list */
};

/* Driver flags */
#define	MDR_XCLU	0x01		/* needs exclusive use of bus */
#define	MDR_BIODMA	0x02		/* (block device) does Mainbus DMA */
#define	MDR_SWAB	0x04		/* Mainbus buffer must be swabbed */
#define	MDR_OBIO	0x08		/* device in on-board I/O space */
#define	MDR_DMA		0x10		/* (nonblock device) does Mainbus DMA */
#define MDR_PSEUDO	0x20		/* pseudo device flag */

/* 
 * the following three bits are used by bp_alloc() to allocate
 * the most efficient map for the device. If not given, the 
 * default map is vme24map, for Sun4m-690.
 * NOTE: These flags are interpreted by Sun4M's mb_XXX only. Other
 *	 machines DO NOT look at these flags at all at the present
 *	 time.
 */
#define MDR_BIGSBUS	0x40		/* S-bus device that uses big map */
#define MDR_VME32	0x80		/* VMEA32 device */
#define MDR_DVMA_PEEK	0x100		/* io mapping has to be seen by host */
#define MDR_MAP_MSK	0x1C0		/* mask to extract the above 3 bits */
#define MDR_LINE_NOIOC	0x200		/* do not cache this IOC line */
#define MDR_ALTSBUS	0x400		/* S-bus map for Sun4m-50 only */

/* Flags to mbsetup */
#define	MB_CANTWAIT	0x01		/* don't block me */
/* note that the following three bits are direct input from mdr_flags */
#define BIGSBUS_MAP	MDR_BIGSBUS	/* use big Sbus map */
#define VME32_MAP	MDR_VME32	/* VMEA32 device */
#define ALTSBUS_MAP	MDR_ALTSBUS	/* Sun4m-50 only sbus device */
#define DVMA_PEEK	MDR_DVMA_PEEK	/* io mapping has to be seen by host */

/*
 * The drivers that use the new mb interface (mb_mapalloc) directly 
 * will have a "callback routine", a pointer to which is passed to 
 * mb_mapalloc(). When invoked, this routine returns DVMA_RUNOUT if 
 * there is not enough space for its request. 
 */
#define DVMA_RUNOUT (-1)

/*
 * If mbcookie.mbi_mapreg < dvmasize, then mbi_mapreg is the
 * starting map register within DVMA space which is mapped
 * in mbsetup() and unmapped in mbrelse().  Otherwise
 * mbi_mapreg is the starting page of contigously mapped
 * region within an external "mainbus" mapping region
 * (e.g. Multibus Memory or VMEbus) as checked and
 * approved by buscheck().
 */
struct mbcookie {
#ifdef sun386
unsigned int	mbi_offset:PGSHIFT,	/* byte offset */
		mbi_mapreg:(32-PGSHIFT);/* starting map register (page) */
#else
unsigned int	mbi_mapreg:(32-PGSHIFT),/* starting map register (page) */
		mbi_offset:PGSHIFT;	/* byte offset */
#endif sun386
};

/*
 * Macros to bust return word from map allocation routines.
 */
#ifdef sun386
/* MBI_MR() and MBI_OFFSET() are valid iff i >= DVMA */
#define MBI_MR(i)	((int)(((unsigned)(i)-(unsigned)DVMA)>>PGSHIFT))
#define MBI_ADDR(i)	((int)(i))
#define MBI_OFFSET(i)	((int)(((unsigned)(i)-(unsigned)DVMA)&PGOFSET))
#else
#define	MBI_MR(i)	((int)((unsigned)(i)>>PGSHIFT))
#define	MBI_ADDR(i)	((int)(i))
#define MBI_OFFSET(i)   ((int)((unsigned)(i)&PGOFSET))
#endif	sun386

/*
 * Vectored Interrupts use this structure
 */

struct vec {
	int	(*v_func)();		/* interrupt function to call */
	int	v_vec;			/* vector number (64-255) */
	int	*v_vptr;		/* pointer to value passed */
};

/*
 * (this is in the wrong place- it should be in <machine/psl.h>)
 * Convert interrupt prio to SR 
 *
 */
#ifdef	sun386

#ifdef	SUN386
#define pritospl(n)	(n)
#define spltopri(n)	(n)
#else	SUN386
extern u_char splmasks[];
#define pritospl(n)	(splmasks[n] | ((n) << 8))
#define spltopri(n)	(((n) & 0xff00) >> 8)
#endif	SUN386

#endif	sun386

/*
 * Kernel Data and Function Declarations
 */

#ifdef KERNEL

#if defined(sun4c) 
/*
 * The sun4c architecture implementation only uses the mb_mapalloc,
 * mb_mapfree, and swab functions. Since the sun4c has only an Sbus
 * (no VME- or MULTI- bus), it has no compatibility issues for older
 * drivers (swab called from in_cksum.c).
 *
 */

int mb_mapalloc(/* map, bp, flags, waitfp, arg */);
void mb_mapfree(/* map, amr */);
void swab (/* pf, pt, n */);

#else	sun4c

int mbgo(/* mc */);
int mbugo(/* md */);
int mbsetup(/* mh, bp, flags */);
int mballoc(/* mh, addr, bcnt, flags */);
int mbuwrap(/* mc */);
int mb_mapalloc(/* map, bp, flags, waitfp, arg */);
int mb_nbmapalloc(/* map, addr, bcnt, flags, waitfp , arg */);
void swab (/* pf, pt, n */);
void mbdone(/* mc */);
void mbudone(/* md */);
void mbrelse(/* mh, amr */);
void mb_mapfree(/* map, amr */);
void mbpresetup(/* mh, bp, flags */);

#endif	sun4c

/*
 * Maximum interrupt priority used by Mainbus DMA.
 * This value is determined as each driver that uses DMA calls adddma
 * with the hardware interrupt priority level that they use.  Adddma
 * increases splvm_val as necessary, and computes SPLMB based on it.
 *
 * XXX SPLMB is for backward compatibility only.
 * XXX Drivers should call splvm() instead of splr(pritospl(SPLMB)).
 */
extern	int	SPLMB;
extern	int	splvm_val;

/*
 * Mainbus related kernel variables
 */
#if !defined(sun4c) 
extern	struct	mb_hd mb_hd;

/*
 * Mbcinit and mbdinit initialize the mass storage controller and
 * device tables specifying possible devices.
 */
extern	struct	mb_ctlr mbcinit[];
extern	struct	mb_device mbdinit[];
#endif	!sun4c

#ifdef sun2
extern	char mbio[];			/* mb device addr space */
#endif

#if defined(sun4c) 
extern	struct	map	*dvmamap;
#endif	sun4c

#if defined(sun4m)
#ifdef IOMMU
extern	struct	map	*vme24map;
extern	struct	map	*vme32map;
extern	struct	map	*altsbusmap;
extern	struct	map	*sbusmap;
extern	struct	map	*bigsbusmap;
extern	struct	map	*mbutlmap;
#endif IOMMU

extern	struct	map	*dvmamap;
#endif	sun4m

/*
 * NOTE: the some drivers (xy, xd)  drivers assume that the IOPBMAP is wholly 
 * contained in the first 64K of DVMA space- this is a constraint of the
 * hardware.
 *
 * The whole point of the iopb map is retain a couple of pages of guaranteed
 * non-cacheable (and DMA'able) pages around for small i/o tranfers (e.g.,
 * control information) where cache consistency would be expensive to maintain.
 */

struct	map *iopbmap;
#define IOPBMEM		(0x2000/NBPG)	/* 8k's worth of pages for IOPB crud */
#define IOPBMAPSIZE	64

/*
 * For coherence and safety across architectures, we'll provide a couple
 * of macros that guarantee longword aligned allocation from the iopb map.
 *
 * roundup() is defined in <sys/param.h>
 */

#define	IOPBALLOC(size)	\
	(caddr_t) rmalloc(iopbmap,(long) roundup((size),(sizeof (long))))

#define	IOPBFREE(addr,size)	(void) rmfree(iopbmap,(long) roundup((size),\
					(sizeof (long))),(u_long) (addr))

/*
 * Most of the DVMA space is controlled by a resource map.
 * We want to define the number of dvma map entries so that
 * we have enough entries to handle the largest number of
 * MAXBSIZE'd transfers at one time.  The extra page is
 * for the ``redzone'' which is set up for each mapping.
 */
#define NDVMAMAPS(ndvmapages)   ((ndvmapages)/(MAXBSIZE/MMU_PAGESIZE + 1))
extern	char DVMA[];

#endif KERNEL
#endif	_sundev_mbvar_h
