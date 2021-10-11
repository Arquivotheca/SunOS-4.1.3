/*	@(#)kvm_impl.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <unistd.h>
#include <sys/types.h>
#include <vm/seg.h>
#include <machine/vm_hat.h>
#include <vm/as.h>
#include <vm/page.h>
#include <sys/dumphdr.h>

/* libkvm library debugging */
#ifdef _KVM_DEBUG
extern void _kvm_error(), _kvm_perror();
#endif _KVM_DEBUG

#define	LIVE_NAMELIST	"/vmunix"
#define	LIVE_COREFILE	"/dev/mem"
#define	LIVE_VIRTMEM	"/dev/kmem"
#define	LIVE_SWAPFILE	"/dev/drum"

#ifdef sun3
extern u_long		_kvm_gapaddr, _kvm_gapsize;
#define	kvtop(kd, x)    (((x) >= _kvm_gapaddr) ? \
				((x) - KERNELBASE + _kvm_gapsize) : \
				(x) - KERNELBASE)
#else
#define	kvtop(kd, x)	((x) - KERNELBASE)
#endif sun3
#define	klvtop(x)	((x) - SYSBASE)

struct _kvmd {		/* libkvm dynamic variables */
	int	wflag;		/* true if kernel opened for write */
	int	namefd;		/* namelist file descriptor */
	int	corefd;		/* corefile file descriptor */
	int	virtfd;		/* virtual memory file descriptor */
	int	swapfd;		/* swap file descriptor */
	char	*name;		/* saved name of namelist file */
	char	*core;		/* saved name of corefile file */
	char	*virt;		/* saved name of virtual memory file */
	char	*swap;		/* saved name of swap file */

	u_int	nproc;		/* number of process structures */
	u_long	proc;		/* address of process table */
	u_int	Syslimit;	/* max kernel virtual addr mapped by Sysmap */
	u_int	econtig;	/* end of contiguous kernel memory */
	struct pte *Sysmap;	/* kernel pte cache for kseg */
	struct pte *Heapptes;	/* kernel pte cache for bufheapseg */
	struct as Kas;		/* kernel address space */
	struct seg Ktextseg;	/* segment that covers kernel text+data */
	struct seg Kseg;	/* segment structure that covers Sysmap */
	struct seg Bufheapseg;	/* segment structure that covers Heapptes */
	struct seg_ops *segvn;	/* ops vector for segvn segments */
	struct seg_ops *segmap;	/* ops vector for segmap segments */
	struct seg_ops *segdev;	/* ops vector for segdev segments */
	struct seg_ops *segkmem; /* ops vector for segkmem segments */
	struct seg_ops *segu;   /* ops vector for segu segments */
	struct swapinfo *sip;	/* ptr to linked list of swapinfo structures */
	struct swapinfo *swapinfo; /* kernel virtual addr of 1st swapinfo */
	struct memseg memseg;   /* first memory segment */
	u_int	page_hashsz;	/* number of buckets in page hash list */
	struct page **page_hash; /* address of page hash list */

	struct proc *pbuf;	/* pointer to process table cache */
	int	pbase;		/* cache base */
	int	pnext;		/* kvmnextproc() pointer */
	char	*uarea;		/* pointer to u-area buffer */

	char	*sbuf;	/* pointer to process stack cache */
	int	stkpg;		/* page in stack cache */
	struct seg useg;	/* segment structure for user pages */

	struct as Uas;		/* default user address space */
	struct condensed {
		struct dumphdr *cd_dp;	/* dumphdr pointer */
		off_t	*cd_atoo;	/* pointer to offset array */
		int cd_atoosize;	/* number of elements in array */
		int cd_chunksize;	/* size of each chunk, in bytes */
		} *corecdp;	/* if not null, core file is condensed */
	struct condensed *swapcdp; /* if not null, swap file is condensed */
#ifdef sun4m
	int is_mp;              /* > 0 = MP ( # of modules); 0 = UP */
	int cpuid;		/* current running cpu */
	union ptpe *percpu_ptpe; /* l1 page table pointer */
	union ptpe *percpu_l3tbl; /* l3 page table */
#endif sun4m
};
