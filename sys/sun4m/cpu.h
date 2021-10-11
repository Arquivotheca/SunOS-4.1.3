/*	@(#)cpu.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#ifndef	_sun4m_cpu_h
#define	_sun4m_cpu_h

#ifndef LOCORE
#include <mon/obpdefs.h>
#endif !LOCORE


#define	CPU_ARCH	0xf0		/* mask for architecture bits */

#define SUN4M_ARCH	0x70		/* arch value for Standard referenc mmu */

#define	CPU_MACH	0x0f		/* mask for machine implementation */

#define	MACH_690	0x01		/* SPARCsystem 600 series */

/* put back old CPU number for campus2. When we have real 
 * machine number like the one for galaxy, we will put them here
 */
#define MACH_50         0x02    /* campus2 */

#define	CPU_SUN4M_690	(SUN4M_ARCH + MACH_690)

#define CPU_SUN4M_50    (SUN4M_ARCH + MACH_50)


#define CACHE_NONE      0
#define CACHE_VAC       1       /* virtual address cache type */
#define CACHE_PAC       2       /* physical address cache */
#define CACHE_PAC_E     3       /* physical address cache + External cache */

#if defined(KERNEL) && !defined(LOCORE)
extern int cpu;				/* machine type we are running on */
extern int dvmasize;			/* usable dvma size in clicks */

#ifdef	VAC
extern int vac;			/* there is a virtual address cache */
extern int cache;		/* there is a virtual/physical  address cache */
#else
#define	vac 0
#define	cache 0
#endif	VAC

#ifdef	IOC
extern int ioc;				/* there is an I/O cache */
#else
#define	ioc 0
#endif	IOC

extern int bcopy_buf;			/* there is a bcopy buffer */

struct machinfo
{
	u_int	sys_type;		/* system type */
	u_char *sys_name;		/* system name string */
	u_int	nmods;			/* number of modules */
	u_int	iommu;			/* has iommu */
	u_int	vme;			/* has vme */
};

struct iommuinfo
{
	u_int   ccoher;                 /* cache-coherence. 0=no, 1=yes */
	u_int	pagesize;		/* mapping pagesize */
};

struct vmeinfo
{
	u_int   iocache;                /* iocache. 0=no, 1=yes */
};

struct mmcinfo
{
	u_int   mc_type;                /* memory controller type */
	u_char  *mc_name;
	u_int   parity_width;
	u_int   ecc_width;
};

struct modinfo
{
	u_int	mod_type;		/* module type */
	u_char	*mod_name;		/* module name string */
	u_int	mid;			/* module id */
	u_int	dev_type;		/* 0=processor; 1=graphics */
        u_int   nmmus;                  /* number of mmus */
        u_int   splitid_mmu;            /* 0=no; 1=yes */
        u_int   ncaches;                /* number of caches */
        u_int   splitid_cache;          /* 0=no; 1=yes */
        u_int   phys_cache;             /* physical cache. 0=no; 1=yes */
        u_int   write_thru;             /* write through cache? 0=no;1=yes */
        u_int   clinesize;              /* cache linesize */
        u_int   cnlines;                /* cache number of lines */
        u_int   cassociate;             /* cache associativity */
        u_int   ccoher;                 /* cache coherence */
        u_int   mmu_nctx;               /* number of context */
	u_int	pagesize;		/* mapping pagesize */
        u_int   sparc_ver;              /* SPARC version */
        u_int   bcopy;                  /* bcopy? 0=no; 1=yes(viking) */
        u_int   bfill;                  /* bfill? 0=no; 1=yes(viking) */
        u_int   iclinesize;             /* instruction cache linesize */
        u_int   icnlines;               /* instruction cache number of lines */
        u_int   icassociate;            /* instruction cache associativity */
        u_int   dclinesize;             /* data cache linesize */
        u_int   dcnlines;               /* data cache number of lines */
        u_int   dcassociate;            /* data cache associativity */
        u_int   eclinesize;             /* external cache linesize */
        u_int   ecnlines;               /* external cache number of lines */
        u_int   ecassociate;            /* external cache associativity */
        u_int   ec_parity;              /* external cache supports parity */
};
#endif KERNEL && !LOCORE
#endif /*!_sun4m_cpu_h*/
