/* @(#)cg6reg.h 1.7 89/01/25 SMI */

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#ifndef	cg6reg_DEFINED
#define	cg6reg_DEFINED

/*
 * CG6 frame buffer hardware definitions.
 */


/* Physical frame buffer and color map addresses */
/*
 * The base address is defined in the configuration file, e.g. GENERIC.
 * These constants are the offset from that address.
 */

#ifdef sun4c
/* Sbus offsets */
#define CG6_ADDR_ROM		0
#define CG6_ADDR_CMAP		0x200000
#define CG6_ADDR_FHC		0x300000
#define CG6_ADDR_THC		0x301000
#define CG6_ADDR_FBC		0x700000
#define	CG6_ADDR_TEC		0x701000
#define CG6_ADDR_COLOR		0x800000
#else
/* P4 offsets */
#define CG6_ADDR_CMAP		0x200000
#define CG6_ADDR_P4REG		0x300000
#define CG6_ADDR_FHC		0x300000
#define CG6_ADDR_THC		0x301000
#define CG6_ADDR_ROM		0x380000
#define CG6_ADDR_OVERLAY	0x400000	/* FAKE */
#define CG6_ADDR_ENABLE		0x600000
#define CG6_ADDR_FBC		0x700000
#define	CG6_ADDR_TEC		0x701000
#define CG6_ADDR_COLOR		0x800000
#endif

#define CG6_ADDR_FBCTEC		CG6_ADDR_FBC
#define CG6_ADDR_FHCTHC		CG6_ADDR_FHC	

#define CG6_CMAP_SZ		8192
#define CG6_FBCTEC_SZ		8192
#define CG6_FHCTHC_SZ		8192
#define CG6_ROM_SZ		(64*1024)
#define CG6_FB_SZ		(1024*1024)

/*
 * Offsets of TEC/FHC into page
 */
#define CG6_TEC_POFF		0x1000
#define CG6_THC_POFF		0x1000

/*
 * Virtual (mmap offsets) addresses
 */ 
#define CG6_VBASE		0x70000000
#define CG6_VADDR(x)		(CG6_VBASE + (x) * 8192)

/*
 * CG6 Virtual object addresses
 */
#define CG6_VADDR_FBC		CG6_VADDR(0)
#define	CG6_VADDR_TEC		(CG6_VADDR_FBC + CG6_TEC_POFF)
#define CG6_VADDR_CMAP		CG6_VADDR(1)
#define CG6_VADDR_FHC		CG6_VADDR(2)
#define CG6_VADDR_THC		(CG6_VADDR_FHC + CG6_THC_POFF)
#define CG6_VADDR_ROM		CG6_VADDR(3)
#define CG6_VADDR_COLOR		(CG6_VADDR_ROM + CG6_ROM_SZ)

#define CG6_VADDR_FBCTEC	CG6_VADDR_FBC
#define CG6_VADDR_FHCTHC	CG6_VADDR_FHC
/*
 * to map in all of lego, use mmapsize below, and offset CG6_VBASE
 */
#define MMAPSIZE(dfbsize)	(CG6_VADDR_COLOR-CG6_VBASE+dfbsize)

/*
 * convert from address returned by pr_makefromfd (eg. mmap) 
 * to CG6 register set.
 */
#define CG6VA_TO_FBC(base) \
	((struct fbc*)  (((char*)base)+(CG6_VADDR_FBC-CG6_VBASE)))
#define CG6VA_TO_TEC(base)  \
	((struct tec*)  (((char*)base)+(CG6_VADDR_TEC-CG6_VBASE)))
#define CG6VA_TO_FHC(base)  \
	((u_int*) 	(((char*)base)+(CG6_VADDR_FHC-CG6_VBASE)))
#define CG6VA_TO_THC(base)  \
	((struct thc*)  (((char*)base)+(CG6_VADDR_THC-CG6_VBASE)))
#define CG6VA_TO_DFB(base)  \
	((short*) 	(((char*)base)+(CG6_VADDR_COLOR-CG6_VBASE)))
#define CG6VA_TO_ROM(base)  \
	((u_int*)	(((char*)base)+(CG6_VADDR_ROM-CG6_VBASE)))
#define CG6VA_TO_CMAP(base) \
	((struct cg6_cmap*) (((char*)base)+(CG6_VADDR_CMAP-CG6_VBASE)))


/* (Brooktree DAC) definitions */

/* number of colormap entries */
#define CG6_CMAP_ENTRIES	256

struct cg6_cmap {
	u_int	addr;		/* address register */
	u_int	cmap;		/* color map data register */
	u_int	ctrl;		/* control register */
	u_int	omap;		/* overlay map data register */
};

#endif	!cg6reg_DEFINED
