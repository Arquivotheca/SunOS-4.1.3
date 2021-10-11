/*	@(#)iocache.h 1.1 92/07/30 SMI */

/*
 * Copyright (c) 1990-1991 by Sun Microsystems, Inc.
 */

#ifndef _sun4m_iocache_h
#define _sun4m_iocache_h

#define IOC_CACHE_LINES	0x400		/* Num cache lines in the iocache  */

#define IOC_ADDR_MSK    0x007FE000      /* mask to extract pa<13-22> */
#define IOC_RW_SHIFT    0x8             /* shift pa<13-22> to pa<5-14> */
#define IOC_FLUSH_PHYS_ADDR     0xDF020000
#define IOC_TAG_PHYS_ADDR       0xDF000000

#define IOC_PAGES_PER_LINE      0x2     /* one line maps 8k */
#define IOC_LINE_MAP            0x2000  /* one line maps 8k */
#define IOC_LINESIZE		32
#define IOC_LINEMASK		(IOC_LINESIZE-1)

/* some bits in ioc tag */
#define IOC_LINE_ENABLE		(0x00200000) /* IOC enabled bit in tag */
#define IOC_LINE_WRITE		(0x00100000) /* IOC writeable bit in tag */

/* tell ioc_setup() what to do */
#define IOC_LINE_INVALID	(0x1) 
#define IOC_LINE_NOIOC		(0x2)	/* do not set IC bit */ 	
#define IOC_LINE_LDIOC		(0x4) 

#define ioc_sizealign(x)	((((x) + IOC_LINESIZE - 1) / IOC_LINESIZE) \
				    * IOC_LINESIZE)
#define ioc_padalign(x)		(ioc_sizealign(x) - (x))

#ifdef IOC
#ifndef LOCORE
extern int ioc;
#endif !LOCORE
/* FIXME: we should not need this. */
/* #define ioc_pteset(pte) { pte->pg_ioc = 1; } */
#else
#define ioc_flush(line)
#define fast_ioc_flush(line)	/* does not call flush_writebuffers */
#define ioc_pteset(pte, flag)
#define ioc_mbufset(pte, addr)
#endif IOC

#endif /*!_sun4m_iocache_h*/
