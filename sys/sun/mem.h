/*	@(#)mem.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Memory Device Minor Numbers
 */

#ifndef _sun_mem_h
#define	_sun_mem_h

#define	M_MEM		0	/* /dev/mem - physical main memory */
#define	M_KMEM		1	/* /dev/kmem - virtual kernel memory & I/O */
#define	M_NULL		2	/* /dev/null - EOF & Rathole */
#define	M_MBMEM		3	/* /dev/mbmem - Multibus memory space (1 Meg) */
#define	M_MBIO		4	/* /dev/mbio - Multibus I/O space (64k) */
#define	M_VME16D16	5	/* /dev/vme16d16 - VME 16bit addr/16bit data */
#define	M_VME24D16	6	/* /dev/vme24d16 - VME 24bit addr/16bit data */
#define	M_VME32D16	7	/* /dev/vme32d16 - VME 32bit addr/16bit data */
#define	M_VME16D32	8	/* /dev/vme16d32 - VME 16bit addr/32bit data */
#define	M_VME24D32	9	/* /dev/vme24d32 - VME 24bit addr/32bit data */
#define	M_VME32D32	10	/* /dev/vme32d32 - VME 32bit addr/32bit data */
#define	M_EEPROM	11	/* /dev/eeprom - on board eeprom device */
#define	M_ZERO		12	/* /dev/zero - source of private memory */
#define	M_ATBUS		13	/* /dev/at - AT bus space */
#define	M_WEITEK	14	/* /dev/weitek - weitek floating point proc */
#define M_METER		15	/* /dev/meter - Sunray performance meters */
/*
 * Move M_SBUS to 32 (0x20)- that way the slot number can be in the
 * minor number from 0 to 0x1f
 */
#define	M_SBUS		32	/* /dev/sbus - Sbus address space */
#define	M_SBUS_SLOTMASK	0x1f	/* mask for sbus slot number in minor number */
/*
 * information to group vme spaces together
 */
#define	M_VME		5	/* base of vme devices */
#define	M_VME_SPACES	6	/* number of vme devices */

/* Structure for virtual to physical translation ioctl */

struct v2p {
	caddr_t vaddr;  /* argument to ioctl */
	caddr_t paddr;  /* filled in on return */
};

/*
 * Ioctl values
 */
#define	 _SUNDIAG_V2P	_IOWR(M, 1, struct v2p)	/* Get physical address */

/* for /dev/meter */
#define MM_HRCNT	_IO(M, 2)
#define MM_CCRW		_IOW(M, 3, int)
#define MM_PCNT0	_IOW(M, 4, int)
#define MM_PCNT1	_IOW(M, 5, int)

/* for processor affinity masks, via any mem device (pref. /dev/null) */
#define	MIOCSPAM	_IOWR(M, 2, u_int) /* set processor affinity mask */
#define	MIOCGPAM	_IOWR(M, 3, u_int) /* get processor affinity mask */

#endif /*!_sun_mem_h*/
