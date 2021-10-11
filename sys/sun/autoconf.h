/* @(#)autoconf.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _sun_autoconf_h
#define	_sun_autoconf_h

/*
 * Defines for bus types.  These are magic cookies passed between config
 * and the kernel to tell what bus each device is on.
 */
#define	SP_BUSMASK	0x0000FFFF	/* mask for bus type */
#define	SP_VIRTUAL	0x0001		/* virtual address */
#define	SP_OBMEM	0x0002		/* on board memory */
#define	SP_OBIO		0x0004		/* on board i/o */
#define	SP_MBMEM	0x0010		/* multibus memory (sun2 only) */
#define	SP_MBIO		0x0020		/* multibus i/o (sun2 only) */
#define	SP_ATMEM	0x0040		/* atbus memory (sun386 only) */
#define	SP_ATIO		0x0080		/* atbus i/o (sun386 only) */
#define	SP_VME16D16	0x0100		/* vme 16/16 */
#define	SP_VME24D16	0x0200		/* vme 24/16 */
#define	SP_VME32D16	0x0400		/* vme 32/16 (sun3 only) */
#define	SP_VME16D32	0x1000		/* vme 16/32 (sun3 only) */
#define	SP_VME24D32	0x2000		/* vme 24/32 (sun3 only) */
#define	SP_VME32D32	0x4000		/* vme 32/32 (sun3 only) */
#define	SP_IPI		0x8000		/* IPI channel address */

#ifdef	sun4m
/*
 * when adding interrupts, you will get better and faster
 * service if you can specify what the interrupt source is
 * when you call "addintr". Normal levels are 1..15; use
 * INTLEVEL_SOFT+n to express interest in soft interrupts
 * on level n (1..15); INTLEVEL_ONBOARD+n to express interest
 * in interrupts from onboard devices on level n (1..15);
 * INTLEVEL_SBUS+n to express interest in SBUS interrupts
 * at SPARC level n (1..15); INTLEVEL_VME to express interest
 * in VME interrupts at SPARC level n (1..15). Note that
 * the translation from VME or SBUS to SPARC must be done
 * before addintr is called.
 *
 * Order of evaulation for HARD interrupts:
 * (processing stops when anyone claims the interrupt)
 *	1. OnBoard interrupts at this level	INTLEVEL_ONBOARD+n
 *	2. SBus interrupts at this level	INTLEVEL_SBUS+n
 *	3. VME interrupts at this level		INTLEVEL_VME+n
 *	4. Other interrupts at this level	n
 *
 * Order of evaluation for SOFT interrupts:
 * (all routines are called)
 *	1. Software interrupts at this level	INTLEVEL_SOFT+n
 *	2. Other interrupts at this level	n
 *
 * In other words, if you register your interrupt handler according
 * to whether it is onboard, sbus, vme, or soft, you (a) will get
 * lower latency because we can get to you sooner, and (b) you will
 * not be called in many cases when the system can tell that the
 * interrupt is not for you anyway.
 *
 * Routines that continue to map themselves to levels 1..15 will still
 * get called on unclaimed hard interrupts and on soft interrupts, just
 * like before.
 *
 * I highly suggest using code similar to below, for backward and forward
 * compatibility.
 *
 *		addintr(
 *	#ifdef	INTLEVEL_SOFT
 *			INTLEVEL_SOFT +
 *	#endif
 *			mylevel, myhandler, "myname", myunit);
 *
 */
#define	INTLEVEL_MASK		15 /* mask to apply to get SPARC int level */

#define	INTLEVEL_SOFT		16
#define	INTLEVEL_ONBOARD	32
#define	INTLEVEL_SBUS		48
#define	INTLEVEL_VME		64

#endif	sun4m

/*
 * Defines for encoding the machine type in the space field of
 * each device.
 */
#define	SP_MACHMASK	0xFFFF0000	/* space mask for machine type */
#define	MAKE_MACH(m)	((m)<<16)
#define	SP_MACH_ALL	MAKE_MACH(0)	/* 0 implies all machines */

#ifdef OPENPROMS

/*
 * OPENPROM autoconfiguration goodies
 * These structures are NOT exported to the world!
 * They are defined here ONLY as a service to device driver writers
 * who would prefer to create (at least some of) the
 * PROM internal structures in C.
 */

/*
 * As far as the external world (clients of the ROMvec) is concerned,
 * device nodes are "magic cookies" of type "int".  We assume here that
 * an int is capable of representing an opaque pointer.
 */
typedef struct dev_element {
	struct dev_element *next;
	struct dev_element *slaves;
	struct property *props;
} devnode_t;

struct property {
	char *name;
	int size;
	caddr_t value;
};

#define	SPO_VIRTUAL	-1	/* virtual address (not used?) */

/*
 * Other legal values are OBMEM and OBIO from machine/pte.h, because
 * the space identifier is identical to the "type" bits from the MMU.
 * The space identifier should be thought of as simply some extra
 * physical address bits.
 */
/*
 * These are service routines in openprom_util.c
 */
struct dev_info *path_to_devi(/* char *path */);
int get_part_from_path(/* char *path */);
u_int atou(/* char *s */);
int (*path_getdecodefunc(/* struct devinfo *dip */))();
char *(*path_getencodefunc(/* struct devinfo *dip */))();
int (*path_getmatchfunc(/* struct devinfo *dip */))();
 
/*
 * Structures and service routines from machine/openprom_xxx.c.
 */
struct dev_path_ops {
        char *devp_name;                /* driver name */
        int (*devp_decode_regprop)();   /* decode reg property to reg struct */
        char *(*devp_encode_reg)();     /* encode reg struct to address */
        int (*devp_match_addr)();       /* does this node match this address? */};
 
extern struct dev_path_ops dev_path_opstab[];
char *obio_encode_reg(/* struct dev_info *dip, char *addrspec */);
int obio_match(/* struct dev_info *dip, char *addrspec */);

#endif OPENPROMS

#ifdef i386

/* maximum number of interrupt request channels (irq's) */
#ifdef SUN386
#define	NVECT	24
#else  SUN386
#define	NVECT	72
#endif SUN386

#else  i386

/* maximum number of autovectored interrupts at a given priority */
#define	NVECT	10

#endif i386

/*
 * An array of these structures will replace the three "levelN_*" arrays
 * used by older architectures.
 */
struct autovec {
	int (*av_vector)();	/* interrupt handler */
	int av_intcnt;		/* interrupt counter */
	char *av_name;		/* pointer to driver name */
	int av_devcnt;		/* number of devices driver handles */
};

#ifdef	KERNEL
#if	defined(sun4c) || defined(sun4m)
/*
 * These are service routines in autoconf.c that device drivers
 * can call.
 */

addr_t map_regs(/* (addr_t)addr, (u_int)size, (int)space */);
void unmap_regs(/* (addr_t)addr, (u_int)size */);
int walk_devs(/* (struct dev_info *)dev_info, (int (*)())f, (caddr_t)arg */);
void report_dev(/* (struct dev_info) dev */);

#endif	/*defined(sun4c) || defined(sun4m) */
#endif	KERNEL

#endif /*!_sun_autoconf_h*/
