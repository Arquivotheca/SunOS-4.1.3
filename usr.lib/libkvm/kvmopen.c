#ifndef lint
static	char sccsid[] = "@(#)kvmopen.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "kvm.h"
#include "kvm_impl.h"
#include <stdio.h>
#include <nlist.h>
#include <varargs.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <vm/swap.h>

static struct nlist knl[] = {
	{ "_Sysmap" },
#define X_SYSMAP	0
	{ "_Syslimit" },
#define X_SYSLIMIT	1
	{ "_proc" },
#define X_PROC		2
	{ "_nproc" },
#define X_NPROC		3
	{ "_segvn_ops" },
#define X_SEGVN		4
	{ "_segmap_ops" },
#define X_SEGMAP	5
	{ "_segdev_ops" },
#define X_SEGDEV	6
	{ "_swapinfo" },
#define X_SWAPINFO	7
	{ "_memsegs" },
#define X_MEMSEGS	8
	{ "_segu_ops" },
#define X_SEGU		9
	{ "_page_hash" },
#define X_PAGE_HASH	10
	{ "_page_hashsz" },
#define X_PAGE_HASHSZ	11
	{ "_econtig" },
#define X_ECONTIG	12
	{ "_kas" },
#define X_KAS		13
	{ "_kseg" },
#define X_KSEG		14
	{ "_segkmem_ops" },
#define X_SEGKMEM	15
	{ "_Heapptes" },
#define X_HEAPPTES	16
	{ "_bufheapseg" },
#define X_BUFHEAPSEG	17
	{ "_end" },
#define X_END		18
#ifdef sun4m
	{"_panic_cpu" },
#define X_CPUID		19
	{"_percpu_ptpe" },
#define X_PERCPU_PTPE	20
	{"_nmod" },
#define X_NMOD		21
#endif sun4m
	{ "" },
};
#define knlsize	(sizeof (knl) / sizeof (struct nlist))

#ifdef sun3
#include <machine/cpu.h>
u_long _kvm_kend;
u_long _kvm_gapaddr;
u_long _kvm_gapsize;
#endif sun3

static void openerror(), openperror();
static int getkvar();

/*
 * Open kernel namelist and core image files and initialize libkvm
 */
kvm_t *
kvm_open(namelist, corefile, swapfile, flag, err)
	char *namelist;
	char *corefile;
	char *swapfile;
	int flag;
	char *err;
{
	register int msize;
	register kvm_t *kd;
	struct nlist kn[knlsize];
	u_long memseg;
#ifdef sun4m
	u_int paddr, cix, ptp;
	u_int idx;
#endif sun4m

	kd = (kvm_t *)malloc((u_int)sizeof (struct _kvmd));
	if (kd == NULL) {
		openperror(err, "cannot allocate space for kvm_t");
		return (NULL);
	}
	kd->namefd = -1;
	kd->corefd = -1;
	kd->virtfd = -1;
	kd->swapfd = -1;
	kd->pbase = -1;
	kd->pnext = 0;
	kd->uarea = NULL;
	kd->pbuf = NULL;
	kd->sbuf = NULL;
	kd->Heapptes = NULL;
	kd->Bufheapseg.s_size = 0;
	kd->Sysmap = NULL;
	kd->econtig = NULL;
	kd->sip = NULL;
	kd->corecdp = NULL;
	kd->swapcdp = NULL;
#ifdef sun4m
	kd->is_mp = 0;
	kd->cpuid = 0;
#endif sun4m

	/* copy static array to dynamic storage for reentrancy purposes */
	bcopy((caddr_t)knl, (caddr_t)kn, sizeof (knl));

	switch (flag) {
	case O_RDWR:
		kd->wflag = 1;
		break;
	case O_RDONLY:
		kd->wflag = 0;
		break;
	default:
		openerror(err, "illegal flag 0x%x to kvm_open()", flag);
		goto error;
	}

	if (namelist == NULL)
		namelist = LIVE_NAMELIST;
	kd->name = namelist;
	if ((kd->namefd = open(kd->name, O_RDONLY, 0)) < 0) {
		openperror(err, "cannot open %s", kd->name);
		goto error;
	}

	if (corefile == NULL) {
		corefile = LIVE_COREFILE;
		kd->virt = LIVE_VIRTMEM;
		if (swapfile == NULL)
			swapfile = LIVE_SWAPFILE;
		if ((kd->virtfd = open(kd->virt, flag, 0)) < 0) {
			openperror(err, "cannot open %s", kd->virt);
			goto error;
		}
		flag = O_RDONLY;	/* open /dev/mem readonly */
	}
	kd->core = corefile;
	if ((kd->corefd = open(kd->core, flag, 0)) < 0) {
		openperror(err, "cannot open %s", kd->core);
		goto error;
	}
	if (condensed_setup(kd->corefd, &kd->corecdp, kd->core, err) == -1)
		goto error;	/* condensed_setup calls openperror */
	if (swapfile != NULL) {
		kd->swap = swapfile;
		if ((kd->swapfd = open(kd->swap, O_RDONLY, 0)) < 0) {
			openperror(err, "cannot open %s", kd->swap);
			goto error;
		}
		if (condensed_setup(kd->swapfd, &kd->swapcdp, kd->swap, err) ==
		    -1)
			goto error;
	}

	/* read kernel data for internal library use */
	if (kvm_nlist(kd, kn) == -1 ) {
		openerror(err, "%s: not a kernel namelist", kd->name);
		goto error;
	}

#ifdef sun3
/*
 * The following cruft is necessary to deal with the discontinuity
 * in the kernel virtual-to-physical mapping on Sun-3's that results
 * from the presence of the 3/50 frame buffer memory at 1MB.  Virtual
 * addresses greater than _kvm_gapaddr have their physical addresses
 * offset by _kvm_gapsize.
 */
	if (kn[X_END].n_type == N_UNDF) {
		openerror(err, "%s: _end not defined", kd->core);
		goto error;
	}
	_kvm_kend = ctob(btoc(kn[X_END].n_value - KERNELBASE)) + KERNELBASE;
	if (_kvm_kend < KERNELBASE + OBFBADDR) {
		_kvm_gapaddr = _kvm_kend;
		_kvm_gapsize = (KERNELBASE + OBFBADDR - _kvm_kend) + FBSIZE;
	} else {
		_kvm_gapaddr = KERNELBASE + OBFBADDR;
		_kvm_gapsize = FBSIZE;
	}
#endif sun3

	if (getkvar(kd, kn[X_KAS].n_value, (caddr_t)&kd->Kas, 
			sizeof (struct as)) != 0) {
		openperror(err, "%s: error on %s", kd->core, "kas");
		goto error;
	}
	if (getkvar(kd, (u_long)kd->Kas.a_segs, (caddr_t)&kd->Ktextseg, 
			sizeof (struct seg)) != 0) {
		openperror(err, "%s: error on %s", kd->core, "ktextseg");
		goto error;
	}
	if (getkvar(kd, kn[X_KSEG].n_value,(caddr_t)&kd->Kseg,
			sizeof (struct seg)) != 0) {
		openperror(err, "%s: error on %s", kd->core, "kseg");
		goto error;
	}
	if(kn[X_BUFHEAPSEG].n_value && kn[X_HEAPPTES].n_value){
	  if (getkvar(kd, kn[X_BUFHEAPSEG].n_value,(caddr_t)&kd->Bufheapseg,
		      sizeof (struct seg)) != 0) {
	    openperror(err, "%s: error on %s", kd->core, "bufheapseg");
	    goto error;
	  }
	}
	if (getkvar(kd, kn[X_ECONTIG].n_value, (caddr_t)&kd->econtig,
			sizeof (int)) != 0) {
		openperror(err, "%s: error on %s", kd->core, "econtig");
		goto error;
	}
	kd->Syslimit = (u_int)kd->Kseg.s_base + kd->Kseg.s_size;
	msize = (kd->Kseg.s_size >> MMU_PAGESHIFT) * sizeof (struct pte);
	kd->Sysmap = (struct pte *)malloc((u_int)msize);
	if (kd->Sysmap == NULL) {
		openperror(err, "cannot allocate space for Sysmap");
		goto error;
	}
	if (getkvar(kd, kn[X_SYSMAP].n_value, (caddr_t)kd->Sysmap,
			msize) != 0) {
		openperror(err, "%s: error on %s", kd->core, "Sysmap");
		goto error;
	}
	if(kn[X_BUFHEAPSEG].n_value && kn[X_HEAPPTES].n_value){
	  msize = (kd->Bufheapseg.s_size >> MMU_PAGESHIFT) * sizeof (struct pte);
	  kd->Heapptes = (struct pte *)malloc((u_int)msize);
	  if (kd->Heapptes == NULL) {
	    openperror(err, "cannot allocate space for Heapptes");
	    goto error;
	  }
	  if (getkvar(kd, kn[X_HEAPPTES].n_value, (caddr_t)kd->Heapptes,
		      msize) != 0) {
	    openperror(err, "%s: error on %s", kd->core, "Heapptes");
	    goto error;
	  }
	}

#ifdef sun4m
	if (getkvar(kd, kn[X_CPUID].n_value, (caddr_t)&kd->cpuid,
			sizeof (int)) == 0) {
		if (getkvar(kd, kn[X_NMOD].n_value,
			(caddr_t)&kd->is_mp, sizeof (int)) != 0) {
			/*
			 * if nmod is not found, i.e. old kernel
			 * assume is_mp is NCPU.
			 */
			kd->is_mp = NCPU;
		}
		msize = kd->is_mp * sizeof (union ptpe);
		kd->percpu_ptpe = (union ptpe *)malloc((u_int)msize);
		if (kd->percpu_ptpe == NULL) {
			openperror(err, "cannot allocate space for percpu_ptpe");
			goto error;
		}
		if (getkvar(kd, kn[X_PERCPU_PTPE].n_value,
			(caddr_t)kd->percpu_ptpe, msize) != 0) {
	    		openperror(err, "%s: error on %s",
				kd->core, "percpu_ptpe");
			goto error;
		}
		msize = kd->is_mp * NL3PTEPERPT * sizeof (union ptpe);
		kd->percpu_l3tbl = (union ptpe *)malloc((u_int)msize);
		if (kd->percpu_l3tbl == NULL) {
			openperror(err, "cannot allocate space for percpu_l3tbl");
			goto error;
		}
		for (cix = 0; cix < kd->is_mp; ++cix) {
			ptp = kd->percpu_ptpe[cix].ptpe_int;
			paddr = (ptp & ~0xF) << 4;
			/* read in the first entry in the level-2 page table at
			   paddr */
			if (getkvar(kd, paddr + KERNELBASE, &ptp,
				sizeof (int)) != 0) {
	    			openperror(err, "%s: error on %s",
					kd->core, "l2 page table");
				goto error;
			}
			paddr = (ptp &~ 0xF) << 4;
			/* read in level-3 page table from paddr */
			if (getkvar(kd, paddr + KERNELBASE,
				&kd->percpu_l3tbl[cix * NL3PTEPERPT].ptpe_int,
				sizeof (union ptpe) * NL3PTEPERPT) != 0) {
	    			openperror(err, "%s: error on %s",
					kd->core, "l3 page table");
				goto error;
			}
		}
	}
#endif sun4m
#define getval(o, b)	\
	if (kvm_read(kd, kn[o].n_value, 				\
		(caddr_t)&kd->b, sizeof (kd->b)) != sizeof (kd->b)) {	\
		openerror(err, "%s: can't find %s", kd->name, "b");	\
		goto error;						\
	}

	/* save segment mapping and process table info */
	kd->segvn = (struct seg_ops *)kn[X_SEGVN].n_value;
	kd->segmap = (struct seg_ops *)kn[X_SEGMAP].n_value;
	kd->segkmem = (struct seg_ops *)kn[X_SEGKMEM].n_value;
	kd->segdev = (struct seg_ops *)kn[X_SEGDEV].n_value;
	kd->segu = (struct seg_ops *)kn[X_SEGU].n_value;

	kvm_read(kd, kn[X_MEMSEGS].n_value, (caddr_t)&memseg, sizeof memseg);
	kvm_read(kd, memseg, (caddr_t)&kd->memseg, sizeof (struct memseg));
	getval(X_SWAPINFO, swapinfo);

	getval(X_PAGE_HASH, page_hash);
	getval(X_PAGE_HASHSZ, page_hashsz);
	getval(X_PROC, proc);
	getval(X_NPROC, nproc);

	return (kd);

error:
	(void) kvm_close(kd);
	return (NULL);
}

static int
getkvar(kd, kaddr, buf, size)
	kvm_t *kd;
	u_long kaddr;
	caddr_t buf;
	int size;
{
	u_int paddr;

	paddr = kvtop(kd, kaddr);
	if (_uncondense(kd, kd->corefd, &paddr))
		return (-1);
	if (lseek(kd->corefd, (off_t)paddr, L_SET) == -1)
		return (-1);
	if (read(kd->corefd, buf, size) != size)
		return (-1);
	return 0;
}

static int
condensed_setup(fd, cdpp, name, err)
	int fd;
	struct condensed **cdpp;
	char *name, *err;
{
	struct dumphdr *dp = (struct dumphdr *) -1;
	struct condensed *cdp;
	char *bitmap = (char *) -1;
	off_t *atoo = NULL;
	off_t offset;
	int bitmapsize;
	int pagesize;
	int chunksize;
	int nchunks;
	int i;
	int rc = 0;

	*cdpp = cdp = NULL;

	/* See if beginning of file is a dumphdr */
	dp = (struct dumphdr *)mmap(0, sizeof(struct dumphdr),
					PROT_READ, MAP_PRIVATE, fd, 0);
	if ((int)dp == -1) {
		/* This is okay; assume not condensed */
#ifdef _KVM_DEBUG
		_kvm_error("cannot mmap %s; assuming non_condensed", name);
#endif _KVM_DEBUG
		goto not_condensed;
	}
	if (dp->dump_magic != DUMP_MAGIC || 
	    (dp->dump_flags & DF_VALID) == 0) {
		/* not condensed */
		goto not_condensed;
	}
	pagesize = dp->dump_pagesize;

	/* get the bitmap */
	bitmapsize = dp->dump_bitmapsize;
	bitmap = mmap(0, bitmapsize, PROT_READ, MAP_PRIVATE, fd, pagesize);
	if ((int)bitmap == -1) {
		openperror(err, "cannot mmap %s's bitmap", name);
		goto error;
	}

	/* allocate the address to offset table */
	if ((atoo = (off_t *)
		calloc(bitmapsize * NBBY, sizeof(*atoo))) == NULL) {
		openperror(err, "cannot allocate %s's uncondensing table",
				name);
		goto error;
	}

	/* allocate the condensed structure */
	if ((cdp = (struct condensed *)
			malloc(sizeof(struct condensed))) == NULL) {
		openperror(err, "cannot allocate %s's struct condensed",
				name);
		goto error;
	}
	cdp->cd_atoo = atoo;
	cdp->cd_atoosize = bitmapsize * NBBY;
	cdp->cd_dp = dp;
	*cdpp = cdp;

	cdp->cd_chunksize = chunksize = pagesize * dp->dump_chunksize;
	offset = pagesize + roundup(bitmapsize, pagesize);
	nchunks = dp->dump_nchunks;

	for (i=0; nchunks > 0; i++) {
		if (isset(bitmap, i)) {
			atoo[i] = offset;
			offset += chunksize;
			nchunks--;
		}
	}

	goto done;

error:
	rc = -1;
not_condensed:
	if (cdp) {
		(void) free((char *)cdp);
		*cdpp = NULL;
	}
	if ((int)dp != -1)
		(void) munmap((caddr_t)dp, sizeof(*dp));
	if (atoo)
		(void) free((char *)atoo);
done:
	if ((int)bitmap != -1)
		(void) munmap((caddr_t)bitmap, bitmapsize);

	return (rc);
}

static void
condensed_takedown(cdp)
	struct condensed *cdp;
{
	struct dumphdr *dp;
	off_t *atoo;

	if (cdp) {
		if ((int)(dp = cdp->cd_dp) != -1) {
			(void) munmap((caddr_t)dp, sizeof(*dp));
		}
		if ((atoo = cdp->cd_atoo) != NULL) {
			(void) free((char *)atoo);
		}
		(void) free((char *)cdp);
	}
}

/*
 * Close files associated with libkvm
 */
int
kvm_close(kd)
	kvm_t *kd;
{
	register struct swapinfo *sn;

	(void) kvm_setproc(kd);	/* rewind process table state */

	if (kd->namefd != -1) {
		(void) close(kd->namefd);
	}
	if (kd->corefd != -1) {
		(void) close(kd->corefd);
	}
	if (kd->virtfd != -1) {
		(void) close(kd->virtfd);
	}
	if (kd->swapfd != -1) {
		(void) close(kd->swapfd);
	}
	if (kd->pbuf != NULL) {
		free((char *)kd->pbuf);
	}
	if (kd->uarea != NULL) {
		free((char *)kd->uarea);
	}
	if (kd->sbuf != NULL) {
		free((char *)kd->sbuf);
	}
	if (kd->Sysmap != NULL) {
		free((char *)kd->Sysmap);
	}
	if (kd->Heapptes != NULL) {
		free((char *)kd->Heapptes);
	}
	while ((sn = kd->sip) != NULL) {
		kd->sip = sn->si_next;
		free((char *)sn);
	}
	condensed_takedown(kd->corecdp);
	condensed_takedown(kd->swapcdp);
	free((char *)kd);
	return (0);
}

/*
 * Reset the position pointers for kvm_nextproc().
 * This routine is here so that kvmnextproc.o isn't always included.
 */
int
kvm_setproc(kd)
	kvm_t *kd;
{

	kd->pnext = 0;			/* rewind the position ptr */
	kd->pbase = -1;			/* invalidate the proc cache */
	return (0);
}

#ifdef sun3
/*
 * convert virtual address into physical address
 * undocumented for savecore to use to bypass handling of frame buffer hole
 *
 */
int
kvm_kdtop(kd, addr)
kvm_t *kd;
int addr;
{
        return(kvtop(kd, addr + KERNELBASE));
}
#endif

/*
 * openerror - print error messages for kvm_open() if prefix is non-NULL
 */
/*VARARGS*/
static void
openerror(va_alist)
	va_dcl
{
	va_list args;
	char *prefix, *fmt;

	va_start(args);

	prefix = va_arg(args, char *);
	if (prefix == NULL)
		goto done;

	fprintf(stderr, "%s: ", prefix);
	fmt = va_arg(args, char *);
	vfprintf(stderr, fmt, args);
	putc('\n', stderr); 

done:
	va_end(args);
}

/*VARARGS*/
static void
openperror(va_alist)
	va_dcl
{
	va_list args;
	char *prefix, *fmt;

	va_start(args);

	prefix = va_arg(args, char *);
	if (prefix == NULL)
		goto done;

	fprintf(stderr, "%s: ", prefix);
	fmt = va_arg(args, char *);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, ": ");
	perror("");

done:
	va_end(args);
}

/* debugging routines */
#ifdef _KVM_DEBUG

int _kvm_debug = _KVM_DEBUG;

/*VARARGS*/
void
_kvm_error(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	if (!_kvm_debug)
		return;

	va_start(args);

	fprintf(stderr, "libkvm: ");
	fmt = va_arg(args, char *);
	vfprintf(stderr, fmt, args);
	putc('\n', stderr); 

	va_end(args);
}

/*VARARGS*/
void
_kvm_perror(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	if (!_kvm_debug)
		return;

	va_start(args);

	fprintf(stderr, "libkvm: ");
	fmt = va_arg(args, char *);
	vfprintf(stderr, fmt, args);
	perror("");

	va_end(args);
}

#endif _KVM_DEBUG
