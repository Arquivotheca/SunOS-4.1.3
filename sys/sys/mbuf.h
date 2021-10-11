/*	@(#)mbuf.h 1.1 92/07/30 SMI; from UCB 7.10 2/8/88	*/

/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef _sys_mbuf_h
#define _sys_mbuf_h

/*
 * Constants related to memory allocator.
 *
 */
#define	MSIZE		128			/* size of an mbuf */

#if CLBYTES > 1024
#define	MCLBYTES	1024
#define	MCLSHIFT	10
#define	MCLOFSET	(MCLBYTES - 1)
#else
#define	MCLBYTES	CLBYTES
#define	MCLSHIFT	CLSHIFT
#define	MCLOFSET	CLOFSET
#endif

#define	MMINOFF		12			/* mbuf header length */
#define	MTAIL		4
#define	MMAXOFF		(MSIZE-MTAIL)		/* offset where data ends */
#define	MLEN		(MSIZE-MMINOFF-MTAIL)	/* mbuf data length */
#define	NMBPCL		(CLBYTES/MSIZE)		/* # mbufs per cluster */

/*
 * Macros for type conversion
 */

/* bytes to mclusters, rounding up, and back */
#define	btomcl(x)	(((x) + MCLBYTES - 1) / MCLBYTES)
#define	mcltob(x)	((x) * MCLBYTES)

/* network cluster number to virtual address, and back */
#define	cltom(x) ((struct mbuf *)((int)mbutl + ((x) << MCLSHIFT)))
#define	mtocl(x) (((int)x - (int)mbutl) >> MCLSHIFT)

/* address in mbuf to mbuf head */
#define	dtom(x)		((struct mbuf *)((int)x & ~(MSIZE-1)))

/* mbuf head, to typed data */
#define	mtod(x,t)	((t)((int)(x) + (x)->m_off))

/*
 * Standard 4.3 supports two kinds of mbufs: normal mbufs, with data
 * residing in the mbuf itself; and cluster mbufs with data stored
 * elsewhere (at a higher address than the corresponding mbuf).
 * The data area for cluster mbufs is allocated as a pool and is
 * managed by the mbuf system itself.  We add another kind of cluster
 * mbuf, whose data area is not part of the mbuf system.  Instead,
 * a routine wishing to use this kind of mbuf must provide a buffer
 * for the storage associated with the mbuf and a pointer to a routine
 * to free the buffer when the mbuf is freed.  The fields in mun_cl
 * below record the necessary bookkeeping information.
 */
struct mbuf {
	struct	mbuf *m_next;		/* next buffer in chain */
	u_long	m_off;			/* offset of data */
	short	m_len;			/* amount of data in this mbuf */
	short	m_type;			/* mbuf type (0 == free) */
	union {
		u_char	mun_dat[MLEN];	/* data storage */
		struct {
			short	mun_cltype;	/* "cluster" type */
			int	(*mun_clfun)();
			int	mun_clarg;
			int	(*mun_clswp)();
		} mun_cl;
	} m_un;
	struct	mbuf *m_act;		/* link in higher-level mbuf list */
};
#define	m_dat		m_un.mun_dat
#define	m_cltype	m_un.mun_cl.mun_cltype
#define	m_clfun		m_un.mun_cl.mun_clfun
#define	m_clarg		m_un.mun_cl.mun_clarg
#define	m_clswp		m_un.mun_cl.mun_clswp

/* mbuf types */
#define	MT_FREE		0	/* should be on free list */
#define	MT_DATA		1	/* dynamic (data) allocation */
#define	MT_HEADER	2	/* packet header */
#define	MT_SOCKET	3	/* socket structure */
#define	MT_PCB		4	/* protocol control block */
#define	MT_RTABLE	5	/* routing tables */
#define	MT_HTABLE	6	/* IMP host tables */
#define	MT_ATABLE	7	/* address resolution tables */
#define	MT_SONAME	8	/* socket name */
#define	MT_ZOMBIE	9	/* zombie proc status */
#define	MT_SOOPTS	10	/* socket options */
#define	MT_FTABLE	11	/* fragment reassembly header */
#define	MT_RIGHTS	12	/* access rights */
#define	MT_IFADDR	13	/* interface address */

/*
 * Values for m_cltype: applicable only for cluster mbufs
 */
#define	MCL_STATIC	1	/* data in mbuf cluster pool */
#define	MCL_LOANED	2	/* data allocated elsewhere and "loaned"
				   to the mbuf */

/* flags to m_get */
#define	M_DONTWAIT	0
#define	M_WAIT		1

/* flags to m_pgalloc */
#define	MPG_MBUFS	0		/* put new mbufs on free list */
#define	MPG_CLUSTERS	1		/* put new clusters on free list */
#define	MPG_SPACE	2		/* don't free; caller wants space */

/* length to m_copy to copy all */
#define	M_COPYALL	1000000000

/*
 * m_pullup will pull up additional length if convenient;
 * should be enough to hold headers of second-level and higher protocols. 
 */
#define	MPULL_EXTRA	32

/*
 * Macros for mbuf manipulation.
 *
 * Standard 4.3 defines some additional macros not given here
 * (MTOCL, MCLFREE), as they are useful only within uipc_mbuf.c
 * or can't cope with the existence of the MCL_LOANED cluster
 * mbuf type.
 *
 */

#define	MGET(m, i, t) \
	{ int ms = splimp(); \
	  if ((m)=mfree) \
		{ if ((m)->m_type != MT_FREE) panic("mget"); (m)->m_type = t; \
		  mbstat.m_mtypes[MT_FREE]--; mbstat.m_mtypes[t]++; \
		  mfree = (m)->m_next; (m)->m_next = 0; \
		  (m)->m_off = MMINOFF; } \
	  else \
		(m) = m_more(i, t); \
	  (void) splx(ms); }

/*
 * Mbuf page cluster macros.
 * MCLALLOC allocates mbuf page clusters.
 * Note that it works only with a count of 1 at the moment.
 * MCLGET adds such clusters to a normal mbuf.
 * m->m_len is set to MCLBYTES upon success, and to MLEN on failure.
 */
#define	MCLALLOC(m, i) \
	{ int ms = splimp(); \
	  if (mclfree == 0) \
		(void)m_clalloc((i), MPG_CLUSTERS, M_DONTWAIT); \
	  if ((m)=mclfree) \
	     {++mclrefcnt[mtocl(m)];mbstat.m_clfree--;mclfree = (m)->m_next;} \
	  (void) splx(ms); }

#define	M_HASCL(m)	((m)->m_off >= MSIZE)

/*
 * N.B.: success/failure indicated by m_len == MCLBYTES
 */
#define	MCLGET(m) \
	{ struct mbuf *p; \
	  MCLALLOC(p, 1); \
	  if (p) { \
		(m)->m_off = (int)p - (int)(m); \
		(m)->m_len = MCLBYTES; \
		(m)->m_cltype = MCL_STATIC; \
	  } else \
		(m)->m_len = MLEN; \
	}

#define	MFREE(m, n) \
	{ int ms = splimp(); \
	  if ((m)->m_type == MT_FREE) panic("mfree"); \
	  mbstat.m_mtypes[(m)->m_type]--; mbstat.m_mtypes[MT_FREE]++; \
	  (m)->m_type = MT_FREE; \
	  if (M_HASCL(m)) \
		mclput(m); \
	  (n) = (m)->m_next; (m)->m_next = mfree; \
	  (m)->m_off = 0; (m)->m_act = 0; mfree = (m); \
	  (void) splx(ms); \
	  if (m_want) { \
		  m_want = 0; \
		  wakeup((caddr_t)&mfree); \
	  } \
	}

/*
 * Mbuf statistics.
 */
struct mbstat {
	short	m_mbufs;	/* mbufs obtained from page pool */
	short	m_clusters;	/* clusters obtained from page pool */
	short	m_clfree;	/* free clusters */
	short	m_drops;	/* times failed to find space */
	short	m_mtypes[256];	/* type specific mbuf allocations */
/*
 * Added at the end to preserve binary compatibility
 */
	u_long	m_space;	/* interface pages obtained from page pool */
	u_long	m_wait;		/* times waited for space */
	u_long	m_drain;	/* times drained protocols for space */
};

#ifdef	KERNEL
extern	struct mbuf mbutl[];		/* virtual address of net free mem */
extern	struct pte Mbmap[];		/* page tables to map mbutl */
struct	mbstat mbstat;
struct	mbuf *mfree, *mclfree;
char	mclrefcnt[];
int	m_want;
struct	mbuf *m_get(),*m_getclr(),*m_free(),*m_more(),*m_copy(),*m_pullup();
struct	mbuf *mclgetx();
caddr_t	m_clalloc();
#endif

#endif /*!_sys_mbuf_h*/
