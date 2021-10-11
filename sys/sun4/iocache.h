/*
 * @(#)iocache.h 1.1 92/07/30 Copyr 1989 Sun Micro
 */

#ifndef _sun4_iocache_h
#define _sun4_iocache_h

#define IOC_CACHE_LINES	0x80		/* Num cache lines in the iocache  */

#define IOC_FLUSH_ADDR	(MDEVBASE + 0xC000)	/* virt addr for ioc flush */
#define IOC_DATA_ADDR	(MDEVBASE + 0x10000)	/* virt addr for ioc data */
#define IOC_TAGS_ADDR	(MDEVBASE + 0x11000)	/* virt addr for ioc tag  */

#define	OBIO_IOCFLUSH_ADDR 	0xEE004000	/* phys address of ioc flush */
#define	OBIO_IOCDATA_ADDR 	0xEE006000	/* phys address of ioc data */
#define	OBIO_IOCTAG_ADDR 	0xEE007000	/* phys address of ioc tag */

/*
 * The second half of the Sunray ioc ram is used as fast memory to
 * hold the the ethernet descriptors.  We initialize the tags
 * as valid, modified, and set the virutal address to where
 * the ethernet will reference it.  Once initialized, it is
 * never flushed and keeps all the ethernet control blocks and
 * descriptors.  Since the ie chip can only access above
 * 0xFF000000 the virtual address for the descriptors can't be
 * in the MDEVBASE segment.
 */
#define IOC_IEDESCR_ADDR	0xFFFFC000	/* virt addr of ie descr */

#define OBIO_DESCR_DATA_ADDR	0xEE00E000	/* address of ioc data	*/
#define OBIO_DESCR_TAGS_ADDR	0xEE00F000	/* address of ioc tag	*/

#define IOC_LINESIZE		32
#define IOC_LINEMASK		(IOC_LINESIZE-1)

#define ioc_sizealign(x)	((((x) + IOC_LINESIZE - 1) / IOC_LINESIZE) \
				    * IOC_LINESIZE)
#define ioc_padalign(x)		(ioc_sizealign(x) - (x))

#define IOC_ETHER_IN		0x7e		/* line for ethernet input */
#define IOC_ETHER_OUT		0x7f		/* line for ethernet output */

#define MODIFIED_IOC_LINE	0x1		/* IOC tag modified bit */
#define VALID_IOC_LINE		0x2		/* IOC tag valid bit */

#ifdef IOC
#ifndef LOCORE
extern int ioc;			/* ioc exists */
extern int ioc_net;		/* use ioc for network */
extern int ioc_vme;		/* use ioc for vme */
extern int ioc_debug;		/* debug ioc (sometimes useful) */
#endif !LOCORE
#define ioc_pteset(pte) { pte->pg_ioc = 1; }
#define ioc_flush(line) { *(unsigned char *)(IOC_FLUSH_ADDR + (line)) = 0; }
#else
#define ioc_flush(line)
#define ioc_pteset(pte)
#define ioc_mbufset(pte, addr)
#endif IOC

#endif /*!_sun4_iocache_h*/
