/*
 * @(#)iocache.h 1.1 92/07/30 Copyr 1990 Sun Micro
 */

#ifndef _sun3x_iocache_h
#define _sun3x_iocache_h

#define	IOC_CACHE_LINES		0x80		/* # of lines in I/O cache */

#define IOC_LINESIZE		16
#define IOC_LINEMASK		(IOC_LINESIZE-1)

#define ioc_sizealign(x)	((((x) + IOC_LINESIZE - 1) / IOC_LINESIZE) \
				    * IOC_LINESIZE)
#define ioc_padalign(x)		(ioc_sizealign(x) - (x))

#define	IOC_ETHER_IN		0x7e		/* line for ethernet input */
#define	IOC_ETHER_OUT		0x7f		/* line for ethernet output */

#ifdef IOC
#ifndef LOCORE
extern int ioc;			/* ioc exists */
extern int ioc_net;		/* use ioc for network */
extern int ioc_vme;		/* use ioc for vme */
extern int ioc_debug;		/* debug ioc (sometimes useful) */
void	ioc_flush(/* line */);
#endif !LOCORE
#define ioc_pteset(pte) { (pte)->pte_iocache = 1; }
#else
#define ioc_pteset(pte)
#define ioc_mbufset(pte, addr)
#endif IOC

#endif /*!_sun3x_iocache_h*/
