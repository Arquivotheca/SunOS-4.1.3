/*	@(#)vmparam.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Machine dependent constants for Sun-3
 */

#ifndef _sun3_vmparam_h
#define _sun3_vmparam_h

#include <machine/mmu.h>

/*
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack.  LOWPAGES and HIGHPAGES are
 * the number of pages from the beginning of the user address space
 * to the beginning of the text and from the end of the stack to the
 * end of the user address space respectively.
 */
#define	USRTEXT		0x2000
#define	USRSTACK	0x0E000000
#define	LOWPAGES	btoc(USRTEXT)
#define	HIGHPAGES	0

/*
 * Virtual memory related constants for UNIX resource control, all in bytes
 */
#ifndef DFLDSIZ
#define	DFLDSIZ		USRSTACK	/* initial data size limit */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		USRSTACK	/* max data size limit */
#endif
#ifndef DFLSSIZ
#define	DFLSSIZ		(2*1024*1024)	/* initial stack size limit */
#endif
#ifndef MAXSSIZ
#define	MAXSSIZ		CTXSIZE/2	/* max stack size limit */
#endif

#define	SSIZE		1		/* initial stack size */
#define	SINCR		1		/* increment of stack */

/*
 * Size of the kernel segkmem system pte table.  This virtual
 * space is controlled by the resource map "kernelmap".
 */
#define	SYSPTSIZE	(0x700000 / MMU_PAGESIZE)

/*
 * Minimum allowable virtual address space to be used
 * by the seg_map segment driver for fast kernel mappings.
 */
#define	MINMAPSIZE	0x200000

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */
#define	MAXSLP 		20

/*
 * A swapped in process is given a small amount of core without being bothered
 * by the page replacement algorithm.  Basically this says that if you are
 * swapped in you deserve some resources.  We protect the last SAFERSS
 * pages against paging and will just swap you out rather than paging you.
 * Note that each process has at least UPAGES pages which are not
 * paged anyways so this number just means a swapped in process is
 * given around 32k bytes.
 */
#define	SAFERSS		3		/* nominal ``small'' resident set size
					   protected against replacement */

/*
 * DISKRPM is used to estimate the number of paging i/o operations
 * which one can expect from a single disk controller.
 */
#define	DISKRPM		60

/*
 * Paging thresholds (see vm_pageout.c).
 * Strategy of 3/17/83:
 *	lotsfree is 256k bytes, but at most 1/8 of memory
 *	desfree is 100k bytes, but at most 1/16 of memory
 *	minfree is 32k bytes, but at most 1/2 of desfree
 */
#define	LOTSFREE	(256 * 1024)
#define	LOTSFREEFRACT	8
#define	DESFREE		(100 * 1024)
#define	DESFREEFRACT	16
#define	MINFREE		(32 * 1024)
#define	MINFREEFRACT	2

/*
 * There are two clock hands, initially separated by HANDSPREAD bytes
 * (but at most all of user memory).  The amount of time to reclaim
 * a page once the pageout process examines it increases with this
 * distance and decreases as the scan rate rises.
 */
#define	HANDSPREAD	(2 * 1024 * 1024)

/*
 * Paged text files that are less than PGTHRESH bytes
 * may be "prefaulted in" instead of demand paged.
 */
#define PGTHRESH	(280 * 1024)

#endif /*!_sun3_vmparam_h*/
