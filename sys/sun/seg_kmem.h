/*	@(#)seg_kmem.h	1.1 92/07/30	SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sun_seg_kmem_h
#define	_sun_seg_kmem_h

/*
 * VM - Kernel Segment Driver
 */

/*
 * These variables should be put in a place which
 * is guaranteed not to get paged out of memory.
 */
extern struct ctx *kctx;	/* kernel's context */
extern struct as kas;		/* kernel's address space */
extern struct seg kseg;		/* kernel's "most everything" segment */
extern struct seg kdvmaseg;	/* kernel's dvma segment */
extern struct seg bufheapseg;	/* kernel's buffer cache and heap segment */

/*
 * For segkmem_create, the argsp is actually a pointer to the
 * optional array of pte's used to map the given segment.
 */
int	segkmem_create(/* seg, argsp */);

/*
 * Special kernel segment operations. NOTE: mapin() and mapout() are
 * supported for compatibility only. Callers should be using
 * segkmem_mapin() and segkmem_mapout().
 */
int	segkmem_alloc(/* seg, addr, len, canwait */);
void	segkmem_free(/* seg, addr, len */);
void	segkmem_mapin(/* seg, addr, len, vprot, pcookie, flags */);
void	segkmem_mapout(/* seg, addr, len */);

/*
 * Usage codes.  The p_offset fields of page structs allocated through
 * segkmem_alloc() are set to one of these values to indicate what
 * the page is being used for.
 */
#define	SEGKMEM_HEAP	1
#define	SEGKMEM_STREAMS	2
#define	SEGKMEM_MBUF	3
#define	SEGKMEM_UFSBUF	4
#define	SEGKMEM_USTRUCT	5
#define	SEGKMEM_LOADMOD	6

#endif /*!_sun_seg_kmem_h*/
