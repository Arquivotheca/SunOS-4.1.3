#ifndef lint
static	char sccsid[] = "@(#)machdep.c 1.1 92/07/30 SMI";
#endif
/* syncd to sun3/machdep.c 1.150 */
/* syncd to sun3/machdep.c 1.177 (4.1)*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/mman.h>
#include <sys/vm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/dnlc.h>
#include <sys/dumphdr.h>
#include <sys/debug.h>
#include <debug/debug.h>
#include "fpa.h"

#if	defined(IPCSEMAPHORE) || defined(IPCMESSAGE) || defined(IPCSHMEM)
#include <sys/ipc.h>
#endif	IPCSEMAPHORE || IPCMESSAGE || IPCSHMEM

#ifdef	IPCSEMAPHORE
#include <sys/sem.h>
#endif	IPCSEMAPHORE

#ifdef	IPCMESSAGE
#include <sys/msg.h>
#endif	IPCMESSAGE

#ifdef	IPCSHMEM
#include <sys/shm.h>
#endif	IPCSHMEM

#include <net/if.h>
#ifdef	UFS
#include <ufs/inode.h>
#ifdef	QUOTA
#include <ufs/quota.h>
#endif	QUOTA
#endif	UFS
#include <sun/consdev.h>
#include <machine/frame.h>
#include <sundev/mbvar.h>

#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/clock.h>
#include <machine/pte.h>
#include <machine/scb.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/interreg.h>
#include <machine/memerr.h>
#include <machine/eccreg.h>
#include <machine/seg_kmem.h>
#include <machine/enable.h>
#include <machine/devaddr.h>

#ifdef	IOC
#include <machine/iocache.h>
#endif	IOC

#include <vm/hat.h>
#include <vm/anon.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/seg_u.h>
#include <vm/swap.h>

#define	MINBUF		6		/* min # of buffers - was 16 */
#define	BUFPERCENT	(100 / 7)	/* % mem for bufs - was 10%, now 7% */

/*
 * Declare these as initialized data so we can patch them.
 */
int nbuf = 0;
int physmem = 0;	/* memory size in pages, patch if you want less */
int kernprot = 1;	/* write protect kernel text */
int msgbufinit = 1;	/* message buffer has been initialized, ok to printf */
int nopanicdebug = 0;
int nrnode = 0;		/* maximum number of NFS rnodes */
addr_t econtig;		/* End of first block of contiguous kernel */
addr_t eecontig;	/* end of segu, which is after econtig */
addr_t vstart;		/* Start of valloc'ed region */
addr_t vvm;		/* Start of valloc'ed region we can mmap */
int npts = 0;		/* number of page tables */
int dvmasize;		/* usable dvma space (in MMU pages) */
int softenablereg = (S_CPUCACHE + S_BCOPY + S_ECC); /* 0x23 */

/*
 * VM data structures
 */
int page_hashsz;		/* Size of page hash table (power of two) */
struct page **page_hash;	/* Page hash table */
struct seg *segkmap;		/* Kernel generic mapping segment */
struct seg ktextseg;		/* Segment used for kernel executable image */
struct seg kvalloc;		/* Segment used for "valloc" mapping */
struct l1pt *kl1pt = (struct l1pt *) NULL;	/*
						 * pointer to level 1 page
						 * table for system
						 */
struct l2pt *kl2pts = (struct l2pt *) NULL;	/*
						 * pointer to array of level 2
						 * page tables
						 */

struct pte *iomap_ptes;	/* beginning of I/O map shadow ptes */

int (*exit_vector)() = (int (*)())0;	/* Where to go when halting UNIX */

#define	TESTVAL	0xA55A		/* memory test value */

#ifdef SUN3X_470
/*
 * Since there is no implied ordering of the memory cards, we store
 * a zero terminated list of pointers to eccreg's that are active so
 * that we only look at existent memory cards during softecc() handling.
 * We also have a flag for the parity memory daughterboard.
 */
struct eccreg *ecc_alive[MAX_ECC+1];
int par_alive;
#endif SUN3X_470

/*
 * Initialize the flag indicating bcopy hardware is present to prevent
 * its use during early initialization phases.
 */
int bcenable = 0;

#ifdef	IOC
int use_ioc = 1;	/* patch to 0 to have the kernel not use the IOC */
int ioc_net = 1;	/* patch to 0 to not iocache the network */
int ioc_vme = 1;	/* patch to 0 to not iocache vme */
int ioc_debug = 0;	/* patch to 1 to debug ioc code */
#else	!IOC
#define	use_ioc 0
#define	ioc_net 0
#define	ioc_vme 0
#define ioc_debug	0
#endif	!IOC

#ifdef	IOC
int ioc = 0;			/* I/O cache type (none == 0) */
#endif	IOC
int iocenable = 0;		/* 4.1 driver compatibility */

/*
 * Various kernel boundaries
 */
extern char start[], etext[], edata[], end[];

/*
 * Define for start of dvma segment that monitor maps in.
 */
#define	MON_DVMA_ADDR		(0 - DVMA_MAP_SIZE)

/*
 * This routine sets up a temporary mapping for the kernel so it can run
 * at the right addresses quickly. It uses the built-in level 1 and level
 * 2 page table structs, and borrows the end of physical memory for the
 * level 3 tables. It is designed to run at the wrong addresses. When it
 * is done, the first MAINMEM_MAP_SIZE of memory will be mapped straight
 * through with a virtual offset of KERNELBASE. Also, the monitor and
 * debugger ptes are preserved as is, and the dvma segment that the monitor
 * set up is also preserved.  The routine returns the
 * physical address of the level 1 page table.
 */
u_int
load_tmpptes()
{
	register struct l1pt *kl1ptr;
	register struct sptp *kl2ptps;
	register struct pte *kpteptr;
	struct l3pt *kl3ptr;
	struct l2pt *kl2ptr;
	register u_int i, j;
	int *tmp;
	int ptpfn;
	int *pmem;
	extern int kptstart;

	/*
	 * Calculate actual location for kernel level 1 & 2 page tables.
	 * These must be 16 byte aligned. The storage kptstart is 16
	 * bytes longer than the sum of the sizes of the page tables.
	 */
	i = ((u_int)&kptstart + 0x10) & ~0xf;
	tmp = (int *)((u_int)&kl1pt - KERNELBASE);
	*tmp = i;
	kl1ptr = (struct l1pt *)(i - KERNELBASE);

	i += sizeof (struct l1pt);
	tmp = (int *)((u_int)&kl2pts - KERNELBASE);
	*tmp = i;
	kl2ptps = (struct sptp *)(i - KERNELBASE);
	kl2ptr = (struct l2pt *)kl2ptps;

	/*
	 * Calculate pointer to the statically allocated memory size variable,
	 * then calculate how much memory we have.
	 * NOTE: we check to see if physmem was patched, but it had to have
	 * been patched while at the wrong address!
	 */
	pmem = (int *)((u_int)&physmem - KERNELBASE);
	/*
	 * Start with the memory size the monitor claims.
	 */
	if (*pmem == 0 || *pmem > btop(*romp->v_memorysize))
		*pmem = btop(*romp->v_memorysize);
	/*
	 * Subtract off any memory stolen by the debugger.
	 */
	if (pte_valid((struct pte *)*romp->v_monptaddr))
		*pmem -= *dvec->dv_pages;
	/*
	 * Subtract off any memory stolen by the monitor.
	 */
	*pmem -= btop(*romp->v_memorysize - *romp->v_memoryavail);
	/*
	 * Calculate the page number to use for the temp ptes and the
	 * physical address of the ptes.
	 */
	ptpfn = *pmem - NKTMPPTPGS;
	kl3ptr = (struct l3pt *)mmu_ptob(ptpfn);
	/*
	 * Map the temp ptes into the beginning of the prom's dvma segment.
	 */
	kpteptr = (struct pte *)*romp->v_shadowpteaddr;
	for (i = 0; i < NKTMPPTPGS; i++) {
		*(u_int *)&kpteptr[i] = MMU_INVALIDPTE;
		kpteptr[i].pte_vld = PTE_VALID;
		kpteptr[i].pte_pfn = ptpfn + i;
	}
	kpteptr = (struct pte *)MON_DVMA_ADDR;
	/*
	 * Invalidate all the level 1 entries.
	 */
	for (i = 0; i < NL1PTEPERPT; i++) {
		*(u_int *)&kl1ptr->lptp[i] = MMU_INVALIDLPTP1;
		*((u_int *)&kl1ptr->lptp[i] + 1) = MMU_INVALIDLPTP2;
	}
	/*
	 * Validate the last level 1 entries to point to the level 2 tables.
	 */
	for (j = 0, i = NL1PTEPERPT - NKL2PTS; i < NL1PTEPERPT; j++, i++) {
		kl1ptr->lptp[i].lptp_vld = PTPTR_VALID;
		kl1ptr->lptp[i].lptp_svisor = 1;
		kl1ptr->lptp[i].lptp_taddr = (u_int)&kl2ptr[j] >>
		    PGPT_TADDRSHIFT;
	}
	/*
	 * Validate the level 2 entries to point to the level 3 tables.
	 * For the level 3 tables mapping the debugger and monitor, we
	 * make the level 2 entries point to the tables already set up
	 * by the monitor. This is necessary so the debugger & monitor
	 * can keep track of where their ptes are.
	 */
	for (i = 0, j = KERNELBASE; i < NKL2PTS * NL2PTEPERPT;
	    i++, j += L3PTSIZE) {
		*(u_int *)&kl2ptps[i] = MMU_INVALIDSPTP;
		kl2ptps[i].sptp_vld = PTPTR_VALID;
		if (j >= DEBUGSTART && j < MONEND) {
			kl2ptps[i].sptp_taddr =
			    ((u_int)((struct pte *)*romp->v_monptphysaddr +
				mmu_btop(j - DEBUGSTART))) >> PGPT_TADDRSHIFT;
		} else
			kl2ptps[i].sptp_taddr = (u_int)&kl3ptr[i] >>
			    PGPT_TADDRSHIFT;
	}
	/*
	 * Invalidate the level 3 entries.
	 */
	for (i = 0; i < NL3PTEPERPT * NKL3PTS; i++)
		*(u_int *)&kpteptr[i] = MMU_INVALIDPTE;
	/*
	 * Validate the first MAINMEM_MAP_SIZE to map through.
	 */
	for (i = 0; i < mmu_btop(MAINMEM_MAP_SIZE); i++) {
		kpteptr[i].pte_vld = PTE_VALID;
		kpteptr[i].pte_pfn = i;
	}
	/*
	 * Copy the dvma segment that the monitor set up. This temporary
	 * copy is not really useful, since it doesn't reside in the I/O
	 * mapper. However, it holds the values of the translations until
	 * we get the I/O mapper set up correctly.
	 */
	j = mmu_btop(MON_DVMA_ADDR - KERNELBASE);
	for (i = 0; i < mmu_btop(DVMA_MAP_SIZE); i++)
		kpteptr[j + i] = ((struct pte *)*romp->v_shadowpteaddr)[i];
	return ((u_int)kl1ptr);
}

/*
 * Early startup code that no longer needs to be assembler. Note that the
 * ptes being acted on here are the temporary ones. All changes made here
 * will be propagated to the real ptes when they are initialized.
 */
void
early_startup()
{
	register struct pte *tmpptes;
	register int index;

	/*
	 * Check to see if memory was all ok. If v_memorybitmap is non-zero,
	 * we had some bad pages. Until we get smarter, we give up
	 * if we find this condition.
	 */
	if (*romp->v_memorybitmap != NULL)
		halt("Memory bad");
	/*
	 * Map in the nonconfigurable devices to standard places. When done,
	 * the I/O Mapper, I/O cache, enable reg, buserror reg, diag reg,
	 * idprom, memerr reg, interrupt reg, eeprom, clock, and ecc/parity
	 * regs will all be mapped in.
	 */
	tmpptes = (struct pte *)MON_DVMA_ADDR;
	index = mmu_btop(V_IOMAP_ADDR - KERNELBASE);
	tmpptes[index].pte_vld = PTE_VALID;
	tmpptes[index].pte_nocache = 1;
	tmpptes[index].pte_pfn = mmu_btop(OBIO_IOMAP_ADDR);
	index = mmu_btop(V_IOC_FLUSH - KERNELBASE);
	tmpptes[index].pte_vld = PTE_VALID;
	tmpptes[index].pte_nocache = 1;
	tmpptes[index].pte_pfn = mmu_btop(OBIO_IOC_FLUSH);
	index = mmu_btop(V_ENABLEREG - KERNELBASE);
	tmpptes[index].pte_vld = PTE_VALID;
	tmpptes[index].pte_nocache = 1;
	tmpptes[index].pte_pfn = mmu_btop(OBIO_ENABLEREG);
	index = mmu_btop(V_EEPROM_ADDR - KERNELBASE);
	tmpptes[index].pte_vld = PTE_VALID;
	tmpptes[index].pte_nocache = 1;
	tmpptes[index].pte_pfn = mmu_btop(OBIO_EEPROM_ADDR);
	index = mmu_btop(V_CLKADDR - KERNELBASE);
	tmpptes[index].pte_vld = PTE_VALID;
	tmpptes[index].pte_nocache = 1;
	tmpptes[index].pte_pfn = mmu_btop(OBIO_CLKADDR);
	index = mmu_btop(V_ECCPARREG - KERNELBASE);
	tmpptes[index].pte_vld = PTE_VALID;
	tmpptes[index].pte_nocache = 1;
	tmpptes[index].pte_pfn = mmu_btop(OBIO_ECCPARREG);
	/*
	 * Set up an initial mapping for kernel's u-area, using
	 * physical page 0.
	 */
	index = mmu_btop(UADDR - KERNELBASE);
	tmpptes[index].pte_vld = PTE_VALID;
	tmpptes[index].pte_pfn = 0;

	/*
	 * Conservatively wipe out the ATC in case the above mappings were
	 * somehow in there.
	 */

	atc_flush_all();

	/*
	 * Zero out the u page, the scb, and the bss. NOTE - the scb part
	 * makes assumptions about its layout with the msgbuf.
	 */
	bzero((caddr_t)uunix, sizeof (*uunix));
	bzero((caddr_t)&scb, MMU_PAGESIZE - sizeof (struct msgbuf));
	bzero(edata, (u_int)(end - edata));
}

/*
 * Machine-dependent startup code.
 */
startup()
{
	register u_int unixsize;
#ifdef SUN3X_470
	register u_int dvmapage;
#endif SUN3X_470
	register u_int i;
	register caddr_t v;
	u_int firstpage;		/* next free physical page number */
	u_int npages, page_base;
	struct page *pp;
	struct segmap_crargs a;
	void v_handler();
	int mon_mem, memblocks;
	struct physmemory *pmem;
	struct memseg *memseg;
	u_int first_page, num_pages, index;
	u_int bytes_avail, bytes_used;
	struct pte *tmpptes = (struct pte *)MON_DVMA_ADDR;
	register struct ptbl *ptbl;
	register struct pte *pte;
	struct sptp *kptp, *kptp_min, *kptp_max;
	struct lptp *klptp;
	extern struct memseg *memsegs;

	/*
	 * Get the boot options.
	 */
	bootflags();
	/*
	 * Initialize the trap vectors.
	 */
	initscb();
	/*
	 * Make sure interrupts can occur.
	 */
	*INTERREG |= IR_ENA_INT;
	/*
	 * Initialize the current end of used memory.
	 */
	firstpage = mmu_btopr((int)end - KERNELBASE);
	/*
	 * Set cpu and dvmasize variables.
	 */
	setcputype();
#ifdef SUN3X_80
	if (cpu == CPU_SUN3X_80) {
		extern u_char ledpat;

		ledpat = -1;
	}
#endif SUN3X_80

#ifdef SUN3X_470
	/*
	 * Initialize and enable the physical cache & I/O cache. Use a page of
	 * kernel space that isn't used till later.
	 */
	if (cpu == CPU_SUN3X_470) {
		extern int nopagereclaim;

		pac_init(&tmpptes[mmu_btop(V_L1PT_ADDR - KERNELBASE)],
		    (addr_t)V_L1PT_ADDR);
		/*
		 * For pegasus, we disable page reclaim to
		 * prevent having pages currently loaded for DVMA
		 * from going to cached to non-cached in the middle
		 * of a IO operation.  With nopagereclaim set to
		 * true, the system will wait for any pages currently
		 * marked as "intransit" to finish before trying
		 * to load up a translation to the page.
		 */
		nopagereclaim = 1;
	}
#endif SUN3X_470
#ifdef IOC
	if (ioc) {
		ioc_init(&tmpptes[mmu_btop(V_L1PT_ADDR - KERNELBASE)],
			(addr_t)V_L1PT_ADDR);
		if (use_ioc) {
			on_enablereg((u_char)ENA_IOCACHE);
			iocenable = 1;	/* 4.1 driver compatibility */
		}
		else
			printf("IO CACHE IS OFF!\n");
	}
	if (!(ioc && use_ioc)) {
		ioc = 0;
		ioc_net = 0;
		ioc_vme = 0;
		ioc_debug = 0;
	}
#endif IOC
	/*
	 * Make sure memory error register is set up to generate
	 * interrupts on error.
	 */
#ifdef SUN3X_470
	if (cpu == CPU_SUN3X_470 && (softenablereg & S_ECC)) {
		struct eccreg **ecc_nxt = ecc_alive;
		struct eccreg *ecc;

		/*
		 * Go probe for all memory cards and perform initialization.
		 * The address of the cards found is stashed in ecc_alive[].
		 * We assume that the cards are already enabled and the
		 * base addresses have been set correctly by the monitor.
		 */
		for (ecc = ECCREG; ecc < &ECCREG[MAX_ECC]; ecc++) {
			if (peekc((char *)ecc) == -1)
				continue;
			MEMREG->mr_dvma = 1; /* clear intr from mem register */
			ecc->syndrome |= SY_CE_MASK; /* clear syndrome fields */
			ecc->eccena |= (ENA_SCRUB_MASK | ENA_BUSENA_MASK);
			*ecc_nxt++ = ecc;
		}
		*ecc_nxt = (struct eccreg *) 0;	/* terminate list */
		/*
		 * Now probe for the parity board. We set the par_alive flag
		 * if it is found.
		 */
		if (peekc((char *)&PARREG->parena) != -1) {
			MEMREG->mr_dvma = 1; /* clear intr from mem register */
			PARREG->parena |= PENA_BUSENA_MASK;
			par_alive = 1;
		} else
			par_alive = 0;
	}
#endif SUN3X_470

#ifdef SUN3X_80
	if (cpu == CPU_SUN3X_80) {
		/* enable parity checking */
		MEMREG->mr_per = PER_CHECK | PER_INTENA;
	}
#endif SUN3X_80

	/*
	 * The size of usable physical memory was already calculated in an
	 * earlier routine. However, here we calculate how much memory
	 * the monitor took so we can tell how much real memory there was.
	 */
	mon_mem = btop(*romp->v_memorysize - *romp->v_memoryavail);

	/*
	 * Add up how much physical memory the prom has passed us.
	 */
	for (pmem = romp->v_physmemory, npages = memblocks = 0;
	    pmem != (struct physmemory *)NULL; pmem = pmem->next) {
		npages += mmu_btop(pmem->size);
		memblocks++;
		if (pmem->next == NULL)
			physmax = mmu_btop(pmem->size + pmem->address) - 1;
	}

	/*
	 * Allocate IOPB memory space just below the end of memory. We
	 * don't map it in until later, since we are not running out of
	 * real ptes yet. However, we need to reserve the space now.
	 */
	maxmem = physmem;
	maxmem -= IOPBMEM;
	/*
	 * v_vector_cmd is the handler for the monitor vector command.
	 * We install v_handler() there for Unix.
	 */
	*romp->v_vector_cmd = v_handler;

	if (nrnode == 0) {
		nrnode = ncsize << 1;
	}


	/*
	 * Allocate space for system data structures.
	 * The first available real memory is in firstpage.
	 * The first available kernel virtual address is in v.
	 * As pages of kernel virtual memory are allocated, v is incremented.
	 */
	v = (caddr_t)(mmu_ptob(firstpage) + KERNELBASE);
	vvm = vstart = v = (caddr_t)(mmu_ptob(firstpage) + KERNELBASE);
#define	valloc(name, type, num) \
	(name) = (type *)(v); (v) = (caddr_t)((name) + (num))
#define	valloclim(name, type, num, lim) \
	(name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))
#ifdef UFS
#ifdef QUOTA
	valloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
#endif QUOTA
#endif
	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloc(cfree, struct cblock, nclist);
	valloc(callout, struct callout, ncallout);
	valloc(kernelmap, struct map, 4 * nproc);
	valloc(iopbmap, struct map, IOPBMAPSIZE);
	valloc(mb_hd.mh_map, struct map, NDVMAMAPS(dvmasize));
	valloc(ncache, struct ncache, ncsize);

/* define macro to round up to integer size */
#define	INTSZ(X)	howmany((X), sizeof (int))
/* define macro to round up to nearest int boundary */
#define	INTRND(X)	roundup((X), sizeof (int))

#ifdef IPCSEMAPHORE
	valloc(sem, struct sem, seminfo.semmns);
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(semmap, struct map, seminfo.semmap);
	valloc(sem_undo, struct sem_undo *, nproc);
	valloc(semu, int, INTSZ(seminfo.semusz * seminfo.semmnu));
#endif IPCSEMAPHORE

#ifdef IPCMESSAGE
	valloc(msg, char, INTRND(msginfo.msgseg * msginfo.msgssz));
	valloc(msgmap, struct map, msginfo.msgmap);
	valloc(msgh, struct msg, msginfo.msgtql);
	valloc(msgque, struct msqid_ds, msginfo.msgmni);
#endif IPCMESSAGE

#ifdef IPCSHMEM
	valloc(shmem, struct shmid_ds, shminfo.shmmni);
#endif IPCSHMEM

	/*
	 * Allocate memory segment structures to describe physical memory.
	 */
	valloc(memseg, struct memseg, memblocks);

	/*
	 * Calculate how many page tables we will have.
	 * The algorithm is to have enough ptes to map all
	 * of memory 4 times over (due to fragmentation). We
	 * add in the size of the I/O Mapper, since it is always there.
	 * We also add in enough tables to map the kernel text/data once.
	 * There is an upper limit, however, since we use indices
	 * in page table lists.
	 */
	if (npts == 0)
		npts = IOMAP_SIZE / sizeof (struct l3pt);
		npts += (firstpage + NL3PTEPERPT - 1) / NL3PTEPERPT;
		npts += (maxmem - firstpage + NL3PTEPERPT - 1)
		    * 4 / NL3PTEPERPT;
	if (npts < minpts) npts = minpts;
	if (npts > MAX_PTBLS)
		npts = MAX_PTBLS;
	/*
	 * Allocate the ptbl structs, the pte_nexts array, and the page
	 * tables. Note that page tables have a 16 byte alignment
	 * requirement.
	 */
	valloclim(ptbls, struct ptbl, npts, eptbls);
	valloc(pte_nexts, struct pte *, npts * NL3PTEPERPT);
	v = (caddr_t)roundup((u_int)v, 16);
	pts_addr = (u_int)v - KERNELBASE;
	valloclim(ptes, struct pte, npts * NL3PTEPERPT, eptes);
	/*
	 * Calculate how many page structs we need and allocate them. Also
	 * allocate the hash tables for the page structs. There will be
	 * some extra page stucts here because we don't take every last
	 * detail into account (like the hash size), but the waste
	 * should be minimal.
	 */
	bytes_used = (u_int)v - KERNELBASE;
	bytes_avail = mmu_ptob(maxmem) - bytes_used;
	npages = bytes_avail / (PAGESIZE + sizeof (struct page));
	npages++;			/* add one, we reclaim page zero */
	page_base = maxmem - npages;
	valloc(pp, struct page, npages);
#ifdef SUN3X_470
	/* io cache counts needed for cache consistency bug workaround */
	valloc(iocs, char, npages);
#endif
	/*
	 * Compute hash size and round up to the nearest power of two.
	 */
	page_hashsz = npages / PAGE_HASHAVELEN;
	for (i = (0x80 << ((sizeof (int) - 1) * NBBY)); i != 0; i >>= 1) {
		if ((page_hashsz & i) != 0) {
			page_hashsz = i << 1;
			break;
		}
	}
	valloc(page_hash, struct page *, page_hashsz);
	/*
	 * Note the end of the contiguous part of the kernel.
	 */
	econtig = (addr_t)roundup((u_int)v, PAGESIZE);

	/*
	 * For the purpose of setting up the kernel's page tables, which
	 * is done below, after hat_init, we have to add in the segu segment,
	 * even though is hasn't been created yet.  Note that this assumes
	 * that seg_alloc will place the segu segment starting at econtig.
	 * This happens because there is no cache alignment that has to
	 * be dealt with.
	 */
	eecontig = (addr_t)((u_int)econtig + nproc * ptob (SEGU_PAGES));

	/*
	 * Make sure we didn't run over anything.
	 */
	if ((u_int)v > KERNEL_LIMIT)
		halt("out of virtual memory");
	unixsize = mmu_btopr(v - KERNELBASE);
	if (unixsize >= physmem - 8 * UPAGES)
		halt("out of physical memory");
	/*
	 * Map in the valloc'd structures.
	 */
	for (i = firstpage; i < mmu_btop((u_int)econtig - KERNELBASE); i++) {
		tmpptes[i].pte_vld = PTE_VALID;
		tmpptes[i].pte_pfn = i;
	}

	/*
	 * Conservatively wipe out the ATC in case the above mappings were
	 * somehow in there.
	 */
	atc_flush_all();
	/*
	 * Zero out the entire valloc'd space.
	 */
	bzero((caddr_t)vvm, (u_int)(econtig - vvm));
	/*
	 * Note the new first free physical page.
	 */
	firstpage = i;
	/*
	 * Now that we are mapped in and zeroed, we can
	 * initialize the VM system.
	 */
	hat_init();
	/*
	 * Ok, the real fun begins. We now have to set up the real ptes
	 * for the kernel. The kernel consists of 2 pieces. From KERNEL_LIMIT
	 * to the end, there are always ptes allocated, even if they will
	 * never be used. From KERNELBASE to eecontig, there
	 * is really stuff there, so we also need ptes allocated. Between
	 * those two is a hole that will have no level 2 or level 3
	 * page tables. We allocate the last segment first, and in reverse
	 * order, because there are embedded assumptions about these ptes
	 * all being contiguous and in order. The only exception
	 * are the ptes for the debugger and monitor. Those were allocated
	 * by the monitor, so we don't reallocate them.
	 * The rest we can allocate in any order.
	 * The ptes are copied as the tables are allocated, and the
	 * level 2 entries must be updated also.
	 */
	for (i = 0 - L3PTSIZE,
	    kptp = &kl2pts[NKL2PTS - 1].sptp[NL2PTEPERPT - 1];
	    i >= KERNEL_LIMIT;
	    i -= L3PTSIZE, kptp--) {
		/*
		 * Skip the tables for the debugger and monitor, they were
		 * set up already. We know the level 2 entries already
		 * point to the correct tables.
		 */
		if (i >= DEBUGSTART && i < MONEND)
			continue;
		ptbl = hat_ptblreserve((addr_t)i, 3);
		pte = ptbltopte(ptbl);
		hat_ptecopy(&tmpptes[mmu_btop(i - KERNELBASE)], pte);
		kptp->sptp_taddr = ptetota(pte);
	}
	kptp_max = kptp;
	for (i = KERNELBASE, kptp = &kl2pts[0].sptp[0];
	    i < (u_int)eecontig;
	    i += L3PTSIZE, kptp++) {
		ptbl = hat_ptblreserve((addr_t)i, 3);
		pte = ptbltopte(ptbl);
		hat_ptecopy(&tmpptes[mmu_btop(i - KERNELBASE)], pte);
		kptp->sptp_taddr = ptetota(pte);
	}
	kptp_min = kptp;
	/*
	 * Next we need to copy the level 2 page tables into our array.
	 * We need to make sure that all the entries we didn't fill in
	 * above get marked invalid. Also, we may leave out
	 * some of the tables, since there is a hole in the address space.
	 */
	for (kptp = kptp_min; kptp <= kptp_max; kptp++)
		kptp->sptp_vld = PTE_INVALID;
	/*
	 * Walk through the level 2 tables, except the last one,
	 * copying ones with real pages.
	 */
	for (i = 0; i < NKL2PTS - 1; i++) {
		klptp = &kl1pt->lptp[NL1PTEPERPT - NKL2PTS + i];
		bytes_used = KERNELBASE + i * L2PTSIZE;
		if (bytes_used >= (u_int)eecontig) {
			klptp->lptp_vld = PTE_INVALID;
			continue;
		}
		ptbl = hat_ptblreserve((addr_t)bytes_used, 2);
		pte = ptbltopte(ptbl);
		hat_ptecopy((struct pte *)&kl2pts[i].sptp[0], pte);
		klptp->lptp_taddr = ptetota(pte);
	}
	/*
	 * We know the last level 2 table is real, and that the
	 * hole will be right below it, so copy it by hand.
	 */
	klptp = &kl1pt->lptp[NL1PTEPERPT - 1];
	ptbl = hat_ptblreserve((addr_t)(0 - L2PTSIZE), 2);
	pte = ptbltopte(ptbl);
	hat_ptecopy((struct pte *)&kl2pts[NKL2PTS - 1].sptp[0], pte);
	klptp->lptp_taddr = ptetota(pte);

#if NFPA > 0
	/*
	 * If there's an fpa in the system, we need to map it into all
	 * address spaces. The easiest way to do this is to put it in
	 * the prototype table (kl1pt), so it gets copied to the others
	 * automatically. Thus, we simply create the translation in our
	 * current table, and the rest is done for us. We use the
	 * early termination feature of the MMU to avoid allocating
	 * any actual tables. This will map the whole 32Mb straight
	 * through, but the user will get a timeout error if he tries
	 * to touch outside the FPA itself. This code assumes that
	 * V_FPA_ADDR is on a 32Mb boundary.
	 */
	klptp = &kl1pt->lptp[getl1in(V_FPA_ADDR)];
	klptp->lptp_vld = PTE_VALID;
	klptp->lptp_nocache = 1;
	klptp->lptp_taddr = OBIO_FPA_ADDR >> PGPT_TADDRSHIFT;
#endif NFPA > 0
	/*
	 * The u page for proc 0 is currently mapped in. This call will
	 * initialize it's level 1 page table, then use it for all kas
	 * needs. This will also cause us to begin running out of that level
	 * 1 table instead of the proto one. This will also flush out the
	 * ATC, so we are now truly running in the new ptes. Note that
	 * before we can call hat_setup(), we need to initialize the
	 * L1PT mapping to valid. This is because the hat layer assumes it
	 * is always valid, to save the time of constantly checking
	 * and setting the valid bit.
	 */
	L1PT_PTE->pte_vld = PTE_VALID;
	hat_setup(&kas);
	/*
	 * We made it! We are now running for real. Now go back and
	 * map in the kernel more correctly.
	 */
	kvm_init();
	/*
	 * Now we go through and initialize the I/O Mapper to look like
	 * the top of the kernel address space. After this, the hat layer
	 * will keep them in sync.
	 */
	iomap_ptes = eptes - (IOMAP_SIZE / sizeof (struct pte));
	for (i = 0, pte = (struct pte *)V_IOMAP_ADDR;
	    i < IOMAP_SIZE / sizeof (struct pte); i++)
		pte[i] = iomap_ptes[i];
	/*
	 * Determine if anything lives in DVMA bus space.
	 */
	disable_dvma();
#ifdef SUN3X_470
	if (cpu == CPU_SUN3X_470) {
		for (dvmapage = 0; dvmapage < dvmasize; dvmapage++) {
			segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
			    PROT_READ | PROT_WRITE,
			    dvmapage + btop(VME24D16_BASE), 0);
			if (poke((short *)CADDR1, TESTVAL) == 0) {
/* XXX */			printf("bad dev at %x",
				    dvmapage + btop(VME24D16_BASE));
				break;
			}
			segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
			    PROT_READ | PROT_WRITE,
			    dvmapage + btop(VME24D32_BASE), 0);
			if (poke((short *)CADDR1, TESTVAL) == 0) {
/* XXX */			printf("bad dev at %x",
				    dvmapage + btop(VME24D32_BASE));
				break;
			}
			segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
			    PROT_READ | PROT_WRITE,
			    dvmapage + btop(VME32D32_BASE), 0);
			if (poke((short *)CADDR1, TESTVAL) == 0) {
/* XXX */			printf("bad dev at %x",
				    dvmapage + btop(VME32D32_BASE));
				break;
			}
		}
	}
#endif SUN3X_470


	enable_dvma();
	/*
	 * Good {morning, afternoon, evening, night}.
	 * When printing memory, use the total including
	 * those hidden by the monitor (mon_mem).
	 */
	printf(version);
	printf("mem = %dK (0x%x)\n", mmu_ptob(physmem + mon_mem) / 1024,
	    mmu_ptob(physmem + mon_mem));
	/*
	 * If something was found in DVMA space, quit now.
	 */
/*
	if (dvmapage < dvmasize) {
		printf("CAN'T HAVE PERIPHERALS IN RANGE 0 - %dKB\n",
		    mmu_ptob(dvmasize) / 1024);
		halt("dvma collision");
	}
*/
	/*
	 * Now we are using mapping for rdwr type routines and buffers
	 * are needed only for file system control information (i.e.
	 * cylinder groups, inodes, etc). Thus MINBUF and BUFPERCENT
	 * are lower than they used to be.
	 * nbuf is now the maximum number of MAXBSIZE buffers that
	 * can be in the cache if it (dynamically) grows to that size.
	 * It should never be less than 2 since deadlock may result.
	 * We check for <2 since some people automatically patch
	 * their kernels to have nbuf=1 before dynamic control buffer
	 * allocation was implemented.
	 * Currently, we calculate nbuf as a percentage of the available
	 * space in the kernel map (or maxmem, if that is smaller).
	 */
	if (nbuf < 2) {
		int availpgs;

		availpgs = MIN(SYSPTSIZE, maxmem);
		nbuf = MAX(((availpgs * NBPG) / BUFPERCENT / MAXBSIZE), MINBUF);
	}

	/*
	 * Initialize more of the page structures.
	 */
	for (i = 0, index = 0, pmem = romp->v_physmemory;
	    pmem != (struct physmemory *)NULL;
	    pmem = pmem->next, i++) {
		first_page = mmu_btop(pmem->address);
		if (page_base > first_page)
			first_page = page_base;
		num_pages = mmu_btop(pmem->address + pmem->size) - first_page;
		if (num_pages > npages - index)
			num_pages = npages - index;
		page_init(&pp[index], num_pages, first_page, &memseg[i]);
		index += num_pages;
		if (index >= npages)
			break;
	}

	/*
	 * Initialize callouts.
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
	/*
	 * Initialize memory free list.
	 */
	for (memseg = memsegs; memseg != (struct memseg *)NULL;
	    memseg = memseg->next) {
		first_page = memseg->pages_base;
		if (first_page < unixsize)
			first_page = unixsize;
		memialloc(first_page, memseg->pages_end);
	}

	/*
	 * Map in the IOPB memory we allocated earlier to the first pages
	 * of DVMA space. This mapping is permanent, and will be marked
	 * as non-cacheable by the lower levels.
	 */
	segkmem_mapin(&kdvmaseg, DVMA, mmu_ptob(IOPBMEM),
	    PROT_READ | PROT_WRITE, btop(physpos(ptob((int)maxmem))), 0);

	maxmem = freemem;
	printf("avail mem = %d\n", mmu_ptob(maxmem));
	/*
	 * Initialize kernel memory allocator.
	 */
	kmem_init();
	/*
	 * Initialize resource maps. Note that we must start kernelmap
	 * one page in since the number 0 is a delimiter to the map
	 * routines. We have to throw away the first page of virtual space.
	 */
	rminit(kernelmap, (long)SYSPTSIZE - 1, (u_long)1,
	    "kernel map", 4 * nproc);
	rminit(iopbmap, (long)ctob(IOPBMEM), (u_long)DVMA,
	    "IOPB space", IOPBMAPSIZE);
	rminit(mb_hd.mh_map, (long)(dvmasize - IOPBMEM), (u_long)IOPBMEM,
	    "DVMA map space", NDVMAMAPS(dvmasize));

	/*
	 * Configure the system.
	 */
	configure();
	/*
	 * Initialize the u area.
	 */

	/*
	 * Initialize the u-area segment type.  We position it
	 * after the configured tables and buffers (whose end
	 * is given by econtig) and before Sysbase.
	 */
	v = econtig;
	i = nproc * ptob(SEGU_PAGES);
	if (v + i > Sysbase)
		panic("insufficient virtual space for segu: nproc too big?");
	segu = seg_alloc(&kas, v, i);
	if (segu == NULL)
		panic("cannot allocate segu\n");
	if (segu_create(segu, (caddr_t)NULL) != 0)
		panic("segu_create segu");
	/*
	 * Advance v to immediately past the end of segu.  Insure that the
	 * region between the end of segu and Sysbase is unmapped.
	 */
	v += i;

	/*
	 * Now create generic mapping segment.  This mapping
	 * goes from NCARGS beyond Syslimit up to the dvma area.
	 * But if the total virtual address is greater than the
	 * amount of free memory that is available, then we trim
	 * back the segment size to that amount.
	 */
	v = Syslimit + NCARGS;
	i = DVMA - v;
	if (i > mmu_ptob(freemem))
		i = mmu_ptob(freemem);
	segkmap = seg_alloc(&kas, v, i);
	if (segkmap == NULL)
		halt("cannot allocate kas");
	a.prot = PROT_READ | PROT_WRITE;
	if (segmap_create(segkmap, (caddr_t)&a) != 0)
		halt("segmap_create");

	/*
	 * Allow interrupts.
	 */
	(void) spl0();
}

struct	bootf {
	char	let;
	short	bit;
} bootf[] = {
	'a',	RB_ASKNAME,
	's',	RB_SINGLE,
	'i',	RB_INITNAME,
	'h',	RB_HALT,
	'b',	RB_NOBOOTRC,
	'd',	RB_DEBUG,
	'w',	RB_WRITABLE,
	0,	0,
};
char *initpath[] = {
	"/sbin/", "/single/", "/etc/", "/bin/", "/usr/etc/", "/usr/bin/", 0
};
char *initname = "init";

/*
 * Parse the boot line to determine boot flags .
 */
bootflags()
{
	register struct bootparam *bp = (*romp->v_bootparam);
	register char *cp;
	register int i;

	cp = bp->bp_argv[1];
	if (cp && *cp++ == '-')
		do {
			for (i = 0; bootf[i].let; i++) {
				if (*cp == bootf[i].let) {
					boothowto |= bootf[i].bit;
					break;
				}
			}
			cp++;
		} while (bootf[i].let && *cp);
#ifdef ROOT_ON_TAPE
	/* force "-sw", and NOT "-a" for MUNIX */
	boothowto |= RB_SINGLE | RB_WRITABLE;
	boothowto &= ~RB_ASKNAME;
#endif
	if (boothowto & RB_INITNAME && bp->bp_argv[2])
		initname = bp->bp_argv[2];
	if (boothowto & RB_HALT)
		halt("bootflags");
}

/*
 * Start the initial user process.
 * The program [initname] is invoked with one argument
 * containing the boot flags.
 */
icode(regs)
	struct regs regs;
{
	struct execa {
		char	*fname;
		char	**argp;
		char	**envp;
	} *ap;
	char *ucp, **uap, *arg0, *arg1;
	char *str;
	char **path;
	static char pathbuf[128];
	int i;
	extern char *strcpy();
	struct proc *p = u.u_procp;
	int errors = 0;

	/*
	 * Wait for pageout to wake us up.  This is to insure that pageout
	 * is created before init starts running.  I use &proc[1] as a convient
	 * address because nobody else uses it so there won't be a conflict.
	 */
	(void) sleep((caddr_t)&proc[1], PZERO);

	u.u_error = 0;					/* paranoid */

	/*
	 * We need to set u.u_ar0 since icode acts like a system call
	 * and references the regs to set the entry point (see setregs)
	 * when it tries to exec.
	 * On regular fork, the ar0 of the child is undefined
	 * since it is set on each system call.
	 */
	u.u_ar0 = &regs.r_r0;

	/*
	 * Allocate user address space.
	 */
	p->p_as = as_alloc();

	/*
	 * Make a (1 page) user stack.
	 */
	if (u.u_error = as_map(p->p_as, (addr_t)(USRSTACK - PAGESIZE),
		PAGESIZE, segvn_create, zfod_argsp)) {
		printf("Can't invoke %s, error %d\n", initname, u.u_error);
		panic("icode");
	}

	for (path = initpath; *path; path++) {
		u.u_error = 0;				/* paranoid */
		/*
		 * Move out the boot flag argument.
		 */
		ucp = (char *)USRSTACK;
		(void) subyte(--ucp, 0);		/* trailing zero */
		for (i = 0; bootf[i].let; i++) {
			if (boothowto & bootf[i].bit)
				(void) subyte(--ucp, bootf[i].let);
		}
		(void) subyte(--ucp, '-');		/* leading hyphen */
		arg1 = ucp;


		/*
		 * Build a pathname unless initname starts with /.
		 */
		if (*initname != '/') {
			(void) strcpy(pathbuf, *path);
			for (str = pathbuf; *str; str++)
				;
			(void) strcpy(str, initname);
		} else
			(void) strcpy(pathbuf, initname);

		/*
		 * Move out the file name (also arg 0).
		 */
		i = 0;
		while (pathbuf[i]) {
			i += 1;	/* size the name */
		}
		while (i >= 0) {
			(void) subyte(--ucp, pathbuf[i]);
			i -= 1;
		}
		arg0 = ucp;

		/*
		 * Move out the arg pointers.
		 */
		uap = (char **)((int)ucp & ~(NBPW-1));
		(void) suword((caddr_t)--uap, 0);	/* terminator */
		(void) suword((caddr_t)--uap, (int)arg1);
		(void) suword((caddr_t)--uap, (int)arg0);

		/*
		 * Point at the arguments.
		 */
		u.u_ap = u.u_arg;
		ap = (struct execa *)u.u_ap;
		ap->fname = arg0;
		ap->argp = uap;
		ap->envp = 0;

		/*
		 * Now let exec do the hard work.
		 */
		execve();
		if (!u.u_error)
			break;
		else {
			errors++;
			printf("Can't invoke %s, error %d\n", pathbuf,
				u.u_error);
		}
		/*
		 * The user passed in the full path name for init so don't
		 * bother trying all of the paths.
		 */
		if (*initname == '/')
			break;
	}
	if (u.u_error)
		panic("icode");
	if (errors)
		printf("init is %s\n", pathbuf);
}

#ifdef PGINPROF
/*
 * Return the difference (in microseconds)
 * between the current time and a previous
 * time as represented by the arguments.
 */
vmtime(otime, olbolt)
	register int otime, olbolt;
{

	return (((time-otime)*HZ + lbolt-olbolt)*(1000000/HZ));
}
#endif PGINPROF

/*
 * Clear registers on exec
 */
setregs(entry)
	u_long entry;
{
	register int i;
	register struct regs *r = (struct regs *)u.u_ar0;

	for (i = 0; i < 8; i++) {
		r->r_dreg[i] = 0;
		if (&r->r_areg[i] != &r->r_sp)
			r->r_areg[i] = 0;
	}
	r->r_ps = PSL_USER;
	r->r_pc = entry;
	u.u_eosys = JUSTRETURN;
#ifdef FPU
	fpu_setregs();
#endif FPU
}

/*
 * Send an interrupt to process.
 *
 * When using new signals user code must do a
 * sys #139 to return from the signal, which
 * calls sigcleanup below, which resets the
 * signal mask and the notion of onsigstack,
 * and returns from the signal handler.
 */
sendsig(p, sig, mask)
	void (*p)();
	int sig, mask;
{
	register int usp, *regs, scp;
	struct nframe {
		int	sig;
		int	code;
		int	scp;
		char	*addr;
	} frame;
	struct sigcontext sc;
	int oonstack;

	regs = u.u_ar0;
	oonstack = u.u_onstack;
	/*
	 * Allocate and validate space for the signal handler
	 * context.  Note that if the stack is in P0 space, the
	 * call to grow() is a nop, and the useracc() check
	 * will fail if the process has not already allocated
	 * the space with a `brk'.
	 */
	if (!u.u_onstack && (u.u_sigonstack & sigmask(sig))) {
		usp = (int)u.u_sigsp;
		u.u_onstack = 1;
	} else
		usp = regs[SP];
	usp -= sizeof (struct sigcontext);
	scp = usp;
	usp -= sizeof (frame);
	if (!u.u_onstack && usp <= USRSTACK - ctob(u.u_ssize))
		(void) grow(usp);
	if (useracc((caddr_t)usp, sizeof (frame) + sizeof (sc), B_WRITE) == 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
printf("sendsig: bad user stack pid=%d, sig=%d\n", u.u_procp->p_pid, sig);
printf("usp is %x, action is %x, upc is %x\n", usp, p, regs[PC]);
		u.u_signal[SIGILL] = SIG_DFL;
		sig = sigmask(SIGILL);
		u.u_procp->p_sigignore &= ~sig;
		u.u_procp->p_sigcatch &= ~sig;
		u.u_procp->p_sigmask &= ~sig;
		psignal(u.u_procp, SIGILL);
		return;
	}
	/*
	 * Since save_fpe_state() will correct the value in regs[PC] if
	 * it is wrong, we must call it before accessing regs[PC].
	 */
	if (sig == SIGFPE)
		save_fpe_state();
	/*
	 * push sigcontext structure.
	 */
	sc.sc_onstack = oonstack;
	sc.sc_mask = mask;
	sc.sc_sp = regs[SP];
	sc.sc_pc = regs[PC];
	sc.sc_ps = regs[PS];
	/*
	 * If trace mode was on for the user process
	 * when we came in here, it may have been because
	 * of an ast-induced trace on a trap instruction,
	 * in which case we do not want to restore the
	 * trace bit in the status register later on
	 * in sigcleanup().  If we were to restore it
	 * and another ast trap had been posted, we would
	 * end up marking the trace trap as a user-requested
	 * real trace trap and send a bogus "Trace/BPT" signal.
	 */
	if ((sc.sc_ps & PSL_T) && (u.u_pcb.pcb_flags & TRACE_AST))
		sc.sc_ps &= ~PSL_T;
	(void) copyout((caddr_t)&sc, (caddr_t)scp, sizeof (sc));
	/*
	 * push call frame.
	 */
	frame.sig = sig;
	switch (sig) {

	case SIGILL:
	case SIGFPE:
	case SIGEMT:
	case SIGBUS:
	case SIGSEGV:
		/*
		 * These signals get a code and an addr.
		 */
		frame.addr = u.u_addr;
		frame.code = u.u_code;
		u.u_code = 0;
		break;

	default:
		frame.addr = (char *)0;
		frame.code = 0;
		break;
	}
	frame.scp = scp;
	(void) copyout((caddr_t)&frame, (caddr_t)usp, sizeof (frame));
	regs[SP] = usp;
	regs[PC] = (int)p;
}

/*
 * Routine to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * notion of on signal stack from context
 * left there by sendsig (above).  Pop these
 * values and perform rti.
 */
sigcleanup()
{
	struct sigcontext *scp, sc;

	scp = (struct sigcontext *)fuword((caddr_t)u.u_ar0[SP] + sizeof (int));
	if ((int)scp == -1)
		return;
	if (copyin((caddr_t)scp, (caddr_t)&sc, sizeof (sc)))
		return;
	u.u_onstack = sc.sc_onstack & 01;
	u.u_procp->p_sigmask = sc.sc_mask & ~cantmask;
	u.u_ar0[SP] = sc.sc_sp;
	u.u_ar0[PC] = sc.sc_pc;
	u.u_ar0[PS] = sc.sc_ps;
	u.u_ar0[PS] &= PSL_USERMASK;
	u.u_ar0[PS] |= PSL_USER;
	u.u_eosys = JUSTRETURN;

#ifdef FPU
	/* this frestore goes along with the stack restore in syscont: */
	if (u.u_pcb.u_berr_pc && (u.u_pcb.u_berr_pc == sc.sc_pc))
		frestore(&u.u_pcb.u_berr_stack->fp_istate);
#endif FPU
}

int	waittime = -1;

boot(howto)
	int howto;
{
	static short prevflag = 0;
	static short bufprevflag = 0;
	extern void vfs_syncall();
	int s;
	extern char *bootstr;

	consdev = rconsdev;
	consvp = rconsvp;
	start_mon_clock();
	if ((howto & RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw &&
	    bufprevflag == 0) {
		bufprevflag = 1;		/* prevent panic recursion */
		waittime = 0;
		s = spl0();
		vfs_syncall();
		(void) splx(s);
	}
	s = spl7();				/* extreme priority */
	if (howto & RB_HALT) {
		halt((char *)NULL);
		/* MAYBE REACHED */
	} else {
		if (howto & RB_DUMP && prevflag == 0) {
			if ((boothowto & RB_DEBUG) != 0 && nopanicdebug == 0) {
				CALL_DEBUG();
			}
			prevflag = 1;		/* prevent panic recursion */
			dumpsys();
		}
		printf("rebooting...\n");
		*INTERREG &= ~IR_ENA_INT;	/* disable all interrupts */
		if (howto & RB_STRING)
			(*romp->v_boot_me)(bootstr);
		else
			(*romp->v_boot_me)(howto & RB_SINGLE ? "-s" : "");
		/*NOTREACHED*/
	}
	(void) splx(s);
}

/*
 * Machine-dependent part of dump header initialization;
 * mark the static pages used by the kernel.
 */
/* ARGSUSED */
dumphdr_machinit(dp)
	struct dumphdr	*dp;
{
	u_int	lomem;
	u_int	lophys;
	u_int	i;

	/*
	 * We mark all the pages below the first page (other than page 0)
	 * covered by a page structure.
	 * This gets us the message buffer and the kernel (text, data, bss),
	 * including the interrupt stack and proc[0]'s u area.
	 * (We also get page zero, which we may not need, but one page
	 * extra is no big deal.)
	 */

	lophys = 0;
	lomem = memsegs->pages_base;
	for (i = lophys; i < lomem; i++)
		dump_addpage(i);
}

/*
 * Machine-dependent portion of dump-checking;
 * verify that a physical address is valid.
 */
int
dump_checksetbit_machdep(addr)
	u_int	addr;
{
	struct physmemory *pmem;

	for (pmem = romp->v_physmemory; pmem != (struct physmemory *)NULL;
	    pmem = pmem->next) {
		if ((addr >= pmem->address) &&
		    (addr < pmem->address + pmem->size)) {
			return (1);
		}
	}
	return (0);
}

/*
 * Machine-dependent portion of dump-checking;
 * mark all pages for dumping.
 */
dump_allpages_machdep()
{
	u_int	i;
	struct physmemory *pmem;

	pmem = romp->v_physmemory;
	while (pmem != (struct physmemory *) NULL) {
		i = mmu_btop(pmem->address);
		while (i <= mmu_btop(pmem->address + pmem->size)) {
			dump_addpage(i++);
		}
		pmem = pmem->next;
	}
}

/*
 * Dump a page frame.
 */
int
dump_page(vp, pg, bn)
	struct vnode *vp;
	int pg;
	int bn;
{
	register caddr_t addr;
	register int err = 0;
	struct pte *pte;

	addr = &DVMA[mmu_ptob(dvmasize-1)];
	pte = hat_ptefind(&kas, addr);
	*(u_int *)pte = MMU_INVALIDPTE;
	pte->pte_vld = 1;
	pte->pte_readonly = PG_KW;
	pte->pte_iocache = 1;
	pte->pte_pfn = pg;
	atc_flush_entry(addr);
	IOMAP_SYNC(addr, pte);
	err = VOP_DUMP(vp, addr, bn, ctod(1));
	vac_flush();

	return (err);
}

/*
 * XXX This is grotty, but for historical reasons, xxdump() routines
 * XXX (called by spec_dump) expect to be called with an object already
 * XXX in DVMA space, and break if it isn't.  This doesn't matter for
 * XXX nfs_dump, but to be general we do it for everyone.
 *
 * Dump an arbitrary kernel-space object.
 * We do this by mapping this object into DVMA space a page-worth's
 * of bytes at a time, since this object may not be on a page boundary
 * and may span multiple pages.  We must be careful because the object
 * (or trailing portion thereof) may not span a page boundary and the
 * next virtual address may map to i/o space, which could cause
 * heartache.
 * We assume that DVMASIZE is at least two pages.
 */
int
dump_kaddr(vp, kaddr, bn, count)
	struct vnode *vp;
	caddr_t kaddr;
	int bn;
	int count;
{
	register caddr_t addr, addr2;
	register int err = 0;
	struct pte *pte0, *pte1, *tpte;
	register int offset;

	offset = (u_int)kaddr & MMU_PAGEOFFSET;
	addr = &DVMA[mmu_ptob(dvmasize-2)];
	pte0 = hat_ptefind(&kas, addr);
	*(u_int *)pte0 = MMU_INVALIDPTE;
	addr2 = &DVMA[mmu_ptob(dvmasize-1)];
	pte1 = hat_ptefind(&kas, addr2);
	*(u_int *)pte1 = MMU_INVALIDPTE;
	pte1->pte_vld = 1;
	pte1->pte_readonly = PG_KW;
	tpte = hat_ptefind(&kas, kaddr);
	pte1->pte_pfn = tpte->pte_pfn;

	while (count > 0 && !err) {

	/* lint doesn't like this ... lint bug? */
#ifndef lint
		*(u_int *)pte0 = *(u_int *)pte1;
#endif !lint

		tpte = hat_ptefind(&kas, kaddr + MMU_PAGESIZE);
		pte1->pte_pfn =
		    (tpte->pte_vld && bustype(tpte->pte_pfn) == BT_OBMEM) ?
			tpte->pte_pfn : 0;
		atc_flush_entry(addr);
		atc_flush_entry(addr + MMU_PAGESIZE);
		IOMAP_SYNC(addr, pte0);
		IOMAP_SYNC(addr2, pte1);
		err = VOP_DUMP(vp, addr + offset, bn, ctod(1));
		bn += ctod(1);
		count -= ctod(1);
		vac_flush();
		kaddr += MMU_PAGESIZE;
	}

	return (err);
}

/*
 * Initialize UNIX's vector table:
 * Vectors are copied from protoscb unless
 * they are zero; zero means preserve whatever the
 * monitor put there.  If the protoscb is zero,
 * then the original contents are copied into
 * the scb we are setting up.
 */
initscb()
{
	register int *s, *p, *f;
	register int n;
	struct scb *orig, *getvbr();

	orig = getvbr();
	exit_vector = orig->scb_trap[14];
	s = (int *)&scb;
	p = (int *)&protoscb;
	f = (int *)orig;
	for (n = sizeof (struct scb)/sizeof (int); n--; s++, p++, f++) {
		if (*p)
			*s = *p;
		else
			*s = *f;
	}
	setvbr(&scb);
	/*
	 * If the boot flags say that the debugger is there,
	 * test and see if it really is by peeking at DVEC.
	 * If is isn't, we turn off the RB_DEBUG flag else
	 * we call the debugger scbsync() routine.
	 */
	if ((boothowto & RB_DEBUG) != 0) {
		if (peek((short *)DVEC) == -1)
			boothowto &= ~RB_DEBUG;
		else
			(*dvec->dv_scbsync)();
	}
}

/*
 * Halt the machine and return to the monitor
 */
halt(s)
	char *s;
{
	extern struct scb *getvbr();

	if (s)
		(*romp->v_printf)("(%s) ", s);
	(*romp->v_printf)("Halted\n\n");
	start_mon_clock();
	if (exit_vector)
		getvbr()->scb_trap[14] = exit_vector;
	_halt();
	if (exit_vector)
		getvbr()->scb_trap[14] = protoscb.scb_trap[14];
	stop_mon_clock();
}

/*
 * Print out a traceback for the caller - can be called anywhere
 * within the kernel or from the monitor by typing "g4" (for sun-2
 * compatibility) or "w trace".  This causes the monitor to call
 * the v_handler() routine which will call tracedump() for these cases.
 */
/*VARARGS0*/
tracedump(x1)
	caddr_t x1;
{
	struct frame *fp = (struct frame *)(&x1 - 2);
	u_int tospage = btoc(fp);

	(*romp->v_printf)("Begin traceback...fp = %x\n", fp);
	while (btoc(fp) == tospage) {
		if (fp == fp->fr_savfp) {
			(*romp->v_printf)("FP loop at %x", fp);
			break;
		}
		(*romp->v_printf)("Called from %x, fp=%x, args=%x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	}
	(*romp->v_printf)("End traceback...\n");
}

/*
 * Common routine for bp_map and buscheck which will read the
 * pte's from the page tables into the pte array given.
 */
static
read_hwmap(bp, npf, pte)
	struct buf *bp;
	int npf;
	register struct pte *pte;
{
	register addr_t addr;
	struct as *as;
	struct	pte *hpte;

	if (bp->b_proc == (struct proc *)NULL ||
	    (bp->b_flags & B_REMAPPED) != 0 ||
	    (as = bp->b_proc->p_as) == (struct as *)NULL)
		as = &kas;
	addr = bp->b_un.b_addr;
	hpte = (struct pte *)NULL;
	while (npf-- > 0) {
		if (hpte == NULL || ((u_int)addr & (L3PTSIZE - 1))
		    < MMU_PAGESIZE)
			hpte = hat_ptefind(as, addr);
		else
			hpte++;
		/*
		 * The hardware ptes should be in place. Panic if we can't
		 * find them.
		 */
		if (hpte == NULL)
			panic("read_hwmap no pte");
		/*
		 * Read current ptes from the tables into pte array given.
		 * It should be valid, we panic if it's not.
		 */
		*pte = *hpte;
		if (!pte_valid(pte))
			panic("read_hwmap invalid pte");
		/*
		 * We make the translation writable, even if the current
		 * mapping is read only. This is necessary because the
		 * new pte is blindly used in other places where it needs
		 * to be writable.
		 */
		pte->pte_readonly = 0;
		pte++;
		addr += MMU_PAGESIZE;
	}
}

/*
 * Map the data referred to by the buffer bp into the kernel
 * at kernel virtual address kaddr.  Used to map in data for
 * DVMA, among other things.
 */

/*
 * PTECHUNKSIZE is just a convenient number of pages to deal with at a time.
 * No way does it reflect anything but that.
 */

#define	PTECHUNKSIZE	16

bp_map(bp, kaddr)
	register struct buf *bp;
	caddr_t kaddr;
{
	auto struct buf bpcopy;
	auto struct pte ptecache[PTECHUNKSIZE];
	register struct pte *pte = &ptecache[0];
	register struct pte *spte = (struct pte *) NULL;
	register struct page *pp = (struct page *) NULL;
	int npf, flags, cidx;
	struct seg *seg;

	/*
	 * Select where to find the pte values we need.
	 * They can either be gotten from a list of page
	 * structures hung off the bp, or are already
	 * available (in Sysmap), or can must be read from
	 * the hardware.
	 */

	if (bp->b_flags & B_PAGEIO) {
		/*
		 * The way to get the pte's is to traverse
		 * the page structures and convert them to ptes.
		 *
		 * The original code commented against 'having
		 * to protect against interrupts messing up
		 * the array'. I don't think that that applies.
		 */
		pp = bp->b_pages;
	} else if ((bp->b_flags & B_PHYS) == 0) {
		u_int vaddr = (u_int) bp->b_un.b_addr;
		if (!((vaddr < DEBUGSTART && vaddr > ((u_int) Syslimit)) ||
		    (vaddr < SYSBASE && vaddr > KERNELBASE))) {
			spte = &Sysmap[btop(vaddr - SYSBASE)];
		}
	}

	/*
	 * If the pte's aren't in Sysmap, or can't be gotten from page
	 * structures, then we have to read them from the hardware.
	 */

	if ((spte == (struct pte *) NULL) && (pp == (struct page *) NULL)) {
		bpcopy = *bp;	/* structure copy */
		cidx = PTECHUNKSIZE;
	}

	/*
	 * We want to use mapin to set up the mappings now since some
	 * users of kernelmap aren't nice enough to unmap things
	 * when they are done and mapin handles this as a special case.
	 * If kaddr is in the kernelmap space, we use kseg so the
	 * software ptes will get updated. Otherwise we use kdvmaseg.
	 * We should probably check to make sure it is really in
	 * one of those two segments, but it's not worth the effort.
	 */
	if (kaddr < Syslimit) {
		seg = &kseg;
	} else {
		seg = &kdvmaseg;
	}

	flags = PTELD_NOSYNC;
#ifdef IOC
	if (ioc) {
		if (bp->b_flags & B_READ) {
		  if ((((u_int)bp->b_un.b_addr & IOC_LINEMASK) == 0) &&
		      ((bp->b_bcount & IOC_LINEMASK) == 0))
			flags |= PTELD_IOCACHE;
		} else
			flags |= PTELD_IOCACHE;
	}
#endif IOC

	/*
	 * Loop through the number of page frames.
	 * Don't use a predecrement because we use
	 * npf in the loop.
	 */

	npf = btoc(bp->b_bcount + ((int)bp->b_un.b_addr & PAGEOFFSET));

	while (npf > 0) {
		/*
		 * First, fetch the pte we're interested in.
		 */
		if (spte) {
			pte = spte++;
		} else if (pp) {
			hat_mempte(pp, PROT_WRITE | PROT_READ, pte,
			    (addr_t) KERNELBASE);
			pp = pp->p_next;
		} else {
			if (cidx == PTECHUNKSIZE) {
				int np = MIN(npf, PTECHUNKSIZE);
				read_hwmap(&bpcopy, np, ptecache);
				bpcopy.b_un.b_addr += ctob(np);
				cidx = 0;
			}
			pte = &ptecache[cidx++];
		}

		/*
		 * Now map it in
		 */

		segkmem_mapin(seg, kaddr, MMU_PAGESIZE,
		    PROT_READ | PROT_WRITE, MAKE_PFNUM(pte), flags);

		/*
		 * adjust values of interest
		 */
		kaddr += MMU_PAGESIZE;
		npf--;
	}
}

/*
 * Buscheck is called by mb_mapalloc and physio to check to see it the
 * requested setup is a valid busmem type (i.e, vme16, vme24, vme32).
 * Returns -1 if illegal mappings, returns 0 if all OBMEM and returns
 * the starting page frame number for a legal "bus request".
 *
 * We insist that non-OBMEM mappings must be contiguous and of the
 * same type (as determined from the starting page).
 */

int
buscheck(bp)
	struct buf *bp;
{
	auto struct buf bpcopy;
	auto struct pte ptecache[PTECHUNKSIZE];
	register struct pte *pte, *spte;
	register int npf, cidx, res;
	u_int pt, pf;

	if (bp->b_flags & B_PAGEIO)
		return (0);		/* all OBMEM */

	spte = (struct pte *) NULL;

	/*
	 * If it's not a B_PHYS mapping, we assume that it's somewhere
	 * in the kernel's already mapped in address space. Some of these
	 * places already have a handy list of ptes available (Sysmap),
	 * else we have to read the hardware.
	 *
	 * For anything else, we have to read the hardware.
	 */

	if ((bp->b_flags & B_PHYS) == 0) {
		u_int vaddr = (u_int) bp->b_un.b_addr;
		/*
		 * If the address is between Syslimnit && DEBUGSTART,
		 * or between KERNELBASE && SYSBASE, we have to read
		 * the hardware for the page types, else the ptes
		 * are in Sysmap.
		 */
		if (!((vaddr < DEBUGSTART && vaddr > ((u_int) Syslimit)) ||
		    (vaddr < SYSBASE && vaddr > KERNELBASE))) {
			spte = &Sysmap[btop(vaddr - SYSBASE)];
		}
	}

	if (spte == (struct pte *) NULL) {
		bpcopy = *bp;	/* structure copy */
		cidx = PTECHUNKSIZE;
	}

	/*
	 * Set the result and page type to impossible values
	 */

	res = -1;
	pt = pf = (u_int) -1;

	/*
	 * ...and calculate the page frame count (allowing for rounding)
	 */

	npf = btoc(bp->b_bcount + ((int)bp->b_un.b_addr & PAGEOFFSET));

	while (npf > 0) {
		if (spte) {
			pte = spte++;
		} else {
			if (cidx == PTECHUNKSIZE) {
				int np = MIN(npf, PTECHUNKSIZE);
				read_hwmap(&bpcopy, np, ptecache);
				bpcopy.b_un.b_addr += ctob(np);
				cidx = 0;
			}
			pte = &ptecache[cidx++];
		}

		if (pt != -1) {
			/*
			 * Invalid or wrong page type (i.e., page type
			 * changed from first page)- error.
			 * If this isn't OBMEM, and the page frame numbers
			 * aren't contiguous, error also.
			 */

			if (!pte_valid(pte)) {
				return (-1);
			} else if (pt == BT_VME && pte->pte_pfn != pf) {
				return (-1);
			} else if (bustype(pte->pte_pfn) != pt) {
				return (-1);
			}
		} else {
			/*
			 * (This sounds weird but) no IO to obio space.
			 * Also insure that we really have a mapping.
			 */
			pf = pte->pte_pfn;
			pt = bustype(pf);
			if (!pte_valid(pte) || pt == BT_OBIO) {
				return (-1);
			}

			if (pt == BT_OBMEM) {
				/*
				 * all Onboard memory memory so far
				 */
				res = 0;
			} else {
				/*
				 * Else save starting page frame number
				 */
				res = pf;
#ifdef	DONT_WORRY_BE_HAPPY
				if (pt == BT_VME) {
					/*
					 * XXX
					 * In the case of VME, we should check
					 * to make sure that it is not in DVMA
					 * space. However, this is complicated,
					 * so we don't worry about it...
					 * XXX
					 */
				}
#endif	/* DONT_WORRY_BE_HAPPY */
			}
		}
		pf++;
		npf--;
	}

	if (res == 0 || res == -1)
		return (res);

	/* "res" holds the starting pte pfnum */
	if (res >= mmu_btop(VME32D32_BASE))
		return (res - mmu_btop(VME32D32_BASE));

	if (res >= mmu_btop(VME24D32_BASE) &&
	    res < mmu_btop(VME24D32_BASE + VME24D32_SIZE))
		return (res - mmu_btop(VME24D32_BASE) +
		    mmu_btop(~0 - VME24D32_MASK));

	if (res >= mmu_btop(VME24D16_BASE) &&
	    res < mmu_btop(VME24D16_BASE + VME24D16_SIZE))
		return (res - mmu_btop(VME24D16_BASE) +
		    mmu_btop(~0 - VME24D16_MASK));

	if (res >= mmu_btop(VME16D32_BASE) &&
	    res < mmu_btop(VME16D32_BASE + VME16D32_SIZE))
		return (res - mmu_btop(VME16D32_BASE) +
		    mmu_btop(~0 - VME16D32_MASK));

	if (res >= mmu_btop(VME16D16_BASE) &&
	    res < mmu_btop(VME16D16_BASE + VME16D16_SIZE))
		return (res - mmu_btop(VME16D16_BASE) +
		    mmu_btop(~0 - VME16D16_MASK));

	/* shouldn't happen - print an error message? */
	return (-1);
}


/*
 * Allocate 'size' units from the given map
 * Return 'addr' if successful, 0 if not.
 * as there is no vac, this code just calls rmalloc
 */
/*ARGSUSED*/
int
bp_alloc(map, bp, size)
	struct map *map;
	struct buf *bp;
	int size;
{
	register u_long addr;
	register int s;

	s = splhigh();
	addr = rmalloc(map, (long)size);
	(void)splx(s);
	return (addr);
}

/*
 * Called to convert bp for pageio/physio to a kernel addressable location.
 * We allocate virtual space from the kernelmap and then use bp_map to do
 * most of the real work.
 */
bp_mapin(bp)
	register struct buf *bp;
{
	int npf, o;
	long a;
	caddr_t kaddr;

	if ((bp->b_flags & (B_PAGEIO | B_PHYS)) == 0 ||
	    (bp->b_flags & B_REMAPPED) != 0)
		return;		/* no pageio/physio or already mapped in */

	if ((bp->b_flags & (B_PAGEIO | B_PHYS)) == (B_PAGEIO | B_PHYS))
		panic("bp_mapin");

	o = (int)bp->b_un.b_addr & PAGEOFFSET;
	npf = btoc(bp->b_bcount + o);

	/*
	 * Allocate kernel virtual space for remapping.
	 */
	while ((a = bp_alloc(kernelmap, bp, npf)) == 0) {
		mapwant(kernelmap)++;
		(void) sleep((caddr_t)kernelmap, PSWP);
	}
	kaddr = Sysbase + mmu_ptob(a);

	/* map the bp into the virtual space we just allocated */
	bp_map(bp, kaddr);
	bp->b_flags |= B_REMAPPED;
	bp->b_un.b_addr = kaddr + o;
}

/*
 * bp_mapout will release all the resources associated with a bp_mapin call.
 * We call hat_unload to release the work done by bp_map which will insure
 * that the reference and modified bits from this mapping are not OR'ed in.
 */


bp_mapout(bp)
	register struct buf *bp;
{
	int npf, cntt;
	u_long a;
	addr_t addr;
	int s;

	if (bp->b_flags & B_REMAPPED) {
		addr = (addr_t) ((int) bp->b_un.b_addr & (PAGEMASK));
		npf = btoc(bp->b_bcount + ((int) bp->b_un.b_addr & PAGEOFFSET));
		cntt = npf;
		while (--cntt >= 0) {
			hat_unload(&kseg, addr, MMU_PAGESIZE);
			addr += MMU_PAGESIZE;
		}
		a = mmu_btop(bp->b_un.b_addr - Sysbase);
		bzero((caddr_t) & Usrptmap[a],
		    (u_int) (sizeof (Usrptmap[0]) * npf));
		s = splhigh();
		rmfree(kernelmap, (long) npf, a);
		(void) splx(s);
		bp->b_un.b_addr =
		    (caddr_t) ((int) bp->b_un.b_addr & PAGEOFFSET);
		bp->b_flags &= ~B_REMAPPED;
	}
}

/*
 * Compute the address of an I/O device within standard address
 * ranges and return the result.  This is used by DKIOCINFO
 * ioctl to get the best guess possible for the actual address
 * set on the card.
 */
getdevaddr(addr)
	caddr_t addr;
{
	int off = (int)addr & MMU_PAGEOFFSET;
	struct pte *pte;
	u_int physaddr;

	pte = hat_ptefind(&kas, addr);
	if (pte == NULL || !pte_valid(pte))
		panic("getdevaddr no pte");
	physaddr = mmu_ptob(pte->pte_pfn);
	/*
	 * These tests are ordered in decreasing physical address. This allows
	 * us to only check one bound.
	 */
	if (physaddr >= VME32D32_BASE)
		physaddr -= VME32D32_BASE;
	else if (physaddr >= VME24D32_BASE)
		physaddr -= VME24D32_BASE;
	else if (physaddr >= VME24D16_BASE)
		physaddr -= VME24D16_BASE;
	else if (physaddr >= VME16D32_BASE)
		physaddr -= VME16D32_BASE;
	else if (physaddr >= VME16D16_BASE)
		physaddr -= VME16D16_BASE;
	return (physaddr + off);
}

static int (*mon_nmi)();		/* monitor's level 7 nmi routine */
static u_char mon_memreg;		/* monitor memory register setting */
extern int level7();			/* Unix's level 7 nmi routine */

stop_mon_clock()
{
	struct scb *vbr, *getvbr();

	vbr = getvbr();
	if (vbr->scb_autovec[7 - 1] != level7) {
#ifndef GPROF
		set_clk_mode(0, IR_ENA_CLK7);	/* disable level 7 clk intr */
#endif !GPROF
		mon_nmi = vbr->scb_autovec[7 - 1];	/* save mon vec */
		vbr->scb_autovec[7 - 1] = level7;	/* install Unix vec */
#ifdef SUN3X_470
		if (cpu == CPU_SUN3X_470) {
			mon_memreg = MEMREG->mr_eer;
			MEMREG->mr_eer = EER_INTENA | EER_CE_ENA;
		}
#endif SUN3X_470
	}
}

start_mon_clock()
{
	struct scb *getvbr();

	if (mon_nmi) {
		getvbr()->scb_autovec[7 - 1] = mon_nmi;	/* install mon vec */
#ifndef GPROF
		set_clk_mode(IR_ENA_CLK7, 0);	/* enable level 7 clk intr */
#endif !GPROF
#ifdef SUN3X_470
		if (cpu == CPU_SUN3X_470)
			MEMREG->mr_eer = mon_memreg;
#endif SUN3X_470
	}
}

/*
 * Handler for monitor vector cmd -
 * For now we just implement the old "g0" and "g4"
 * commands and a printf hack.
 */
void
v_handler(addr, str)
	int addr;
	char *str;
{

	switch (*str) {
	case 'w':
	case 'W':
		{
		struct lptp *lptp;
		struct sptp *sptp;
		struct pte *pte;
		u_int phys;

		/*
		 * Walk the page table and give info on it. Assumes that
		 * the level one table currently in use in the current
		 * uarea.
		 */
		printf("ROOT PTR PA = %x\n", mmu_getrootptr());
		lptp = uunix->u_pcb.pcb_l1pt.lptp;
		lptp += getl1in(addr);
		printf("LEVEL ONE ENTRY: VA = %x, PA = %x\n", lptp);
		printf("\tTADDR = %x, NOC = %x, WP = %x, VLD = %x, SVSR = %x\n",
		    lptp->lptp_taddr, lptp->lptp_vld, lptp->lptp_nocache,
		    lptp->lptp_readonly, lptp->lptp_svisor);
		if (lptp->lptp_vld != PTPTR_VALID)
			return;
		phys = lptp->lptp_taddr << PGPT_TADDRSHIFT;
		sptp = (struct sptp *)tatopte(lptp->lptp_taddr);
		sptp += getl2in(addr);
		phys += getl2in(addr) * sizeof (struct sptp);
		printf("LEVEL TWO ENTRY: VA = %x, PA = %x\n", sptp, phys);
		printf("\tTADDR = %x, WP = %x, VLD = %x\n", sptp->sptp_taddr,
		    sptp->sptp_readonly, sptp->sptp_vld);
		if (sptp->sptp_vld != PTPTR_VALID)
			return;
		phys = sptp->sptp_taddr << PGPT_TADDRSHIFT;
		pte = tatopte(sptp->sptp_taddr);
		pte += getl3in(addr);
		phys += getl3in(addr) * sizeof (struct pte);
		printf("LEVEL THREE ENTRY: VA = %x, PA = %x\n", pte, phys);
		printf("\tPFN = %x, ADDR = %x\n",
		    pte->pte_pfn, pte->pte_pfn * PAGESIZE);
		printf("\tLD = %x, IOC = %x, NOC = %x, NSYNC = %x, ",
		    pte->pte_loaded, pte->pte_iocache, pte->pte_nocache,
		    pte->pte_nosync);
		printf("MOD = %x, REF = %x, WP = %x, VALID = %x\n",
		    pte->pte_modified, pte->pte_referenced,
		    pte->pte_readonly, pte->pte_vld);
		return;
		}
	case '\0':
		/*
		 * No (non-hex) letter was specified on
		 * command line, use only the number given
		 */
		switch (addr) {
		case 0:		/* old g0 */
		case 0xd:	/* 'd'ump short hand */
			panic("zero");
			/*NOTREACHED*/

		case 4:		/* old g4 */
			tracedump();
			break;

		default:
			goto err;
		}
		break;

	case 'p':		/* 'p'rint string command */
	case 'P':
		(*romp->v_printf)("%s\n", (char *)addr);
		break;

	case '%':		/* p'%'int anything a la printf */
		(*romp->v_printf)(str, addr);
		(*romp->v_printf)("\n");
		break;

	case 't':		/* 't'race kernel stack */
	case 'T':
		tracedump();
		break;

	case 'u':		/* d'u'mp hack ('d' look like hex) */
	case 'U':
		if (addr == 0xd) {
			panic("zero");
		} else
			goto err;
		break;

	default:
	err:
		(*romp->v_printf)("Don't understand 0x%x '%s'\n", addr, str);
	}
}

/*
 * Handle parity/ECC memory errors.
 */
memerr()
{
	u_int per, eer;
	char *mess = 0;

	eer = per = MEMREG->mr_er;
#ifdef SUN3X_470
	if (cpu == CPU_SUN3X_470 && (((eer & EER_ERR) == EER_CE) &&
			((eer & EER_ERR) != EER_UE))) {
		MEMREG->mr_eer = ~EER_INTENA;
		softecc();
		MEMREG->mr_dvma = 1;	/* clear latching */
		MEMREG->mr_eer = (EER_CE_ENA|EER_INTENA);
		return;
	}
#endif SUN3X_470

	/*
	 * Since we are going down in flames, disable further
	 * memory error interrupts to prevent confusion.
	 */
	MEMREG->mr_er &= ~ER_INTENA;

#ifdef notdef
	    if ((per & PER_ERR) != 0) {
		printf("Parity Error Register %b\n", per, PARERR_BITS);
		mess = "parity error";
	}
#endif notdef

#ifdef SUN3X_470
	if ((cpu == CPU_SUN3X_470) && (eer & EER_ERR) != 0) {
		printf("PAR/ECC Error Register %b\n", eer, ECCERR_BITS);
		if (eer & EER_TIMEOUT)
			mess = "memory timeout error";
		if (eer & EER_UE) {
			if (par_alive &&
			    ((PARREG->parerr & PERR_ERR_MASK) != 0)) {
				mess = "parity error";
				printf("Parity board register %x\n",
				    PARREG->parerr);
			} else
				mess = "uncorrectable ECC error";
		}
	}
#endif SUN3X_470

	if (!mess) {
		printf("Memory Error Register %b %b\n",
		    per, PARERR_BITS, eer, ECCERR_BITS);
		mess = "unknown memory error";
	}

	printf("DVMA = %x, physical address = %x\n",
		MEMREG->mr_dvma, MEMREG->mr_paddr);

	/*
	 * Clear the latching by writing to the top
	 * nibble of the memory address register
	 */
	MEMREG->mr_dvma = 1;

	panic(mess);
	/*NOTREACHED*/
}

#ifdef SUN3X_470
int prtsoftecc = 1;
extern int noprintf;
int memintvl = MEMINTVL;

struct {
	u_char	m_syndrome;
	char	m_bit[3];
} memlogtab[] = {
	0x01, "64", 0x02, "65", 0x04, "66", 0x08, "67", 0x0B, "30", 0x0E, "31",
	0x10, "68", 0x13, "29", 0x15, "28", 0x16, "27", 0x19, "26", 0x1A, "25",
	0x1C, "24", 0x20, "69", 0x23, "07", 0x25, "06", 0x26, "05", 0x29, "04",
	0x2A, "03", 0x2C, "02", 0x31, "01", 0x34, "00", 0x40, "70", 0x4A, "46",
	0x4F, "47", 0x52, "45", 0x54, "44", 0x57, "43", 0x58, "42", 0x5B, "41",
	0x5D, "40", 0x62, "55", 0x64, "54", 0x67, "53", 0x68, "52", 0x6B, "51",
	0x6D, "50", 0x70, "49", 0x75, "48", 0x80, "71", 0x8A, "62", 0x8F, "63",
	0x92, "61", 0x94, "60", 0x97, "59", 0x98, "58", 0x9B, "57", 0x9D, "56",
	0xA2, "39", 0xA4, "38", 0xA7, "37", 0xA8, "36", 0xAB, "35", 0xAD, "34",
	0xB0, "33", 0xB5, "32", 0xCB, "14", 0xCE, "15", 0xD3, "13", 0xD5, "12",
	0xD6, "11", 0xD9, "10", 0xDA, "09", 0xDC, "08", 0xE3, "23", 0xE5, "22",
	0xE6, "21", 0xE9, "20", 0xEA, "19", 0xEC, "18", 0xF1, "17", 0xF4, "16",
};

/*
 * Routine to turn on correctable error reporting.
 */
ce_enable(ecc)
	struct eccreg *ecc;
{

	ecc->eccena |= ENA_BUSENA_MASK;
}

/*
 * Probe memory cards to find which one(s) had ecc error(s).
 * If prtsoftecc is non-zero, log messages regarding the failing
 * syndrome.  Then clear the latching on the memory card.
 */
softecc()
{
	register struct eccreg **ecc_nxt, *ecc;

	for (ecc_nxt = ecc_alive; *ecc_nxt != (struct eccreg *)0; ecc_nxt++) {
		ecc = *ecc_nxt;
		if (ecc->syndrome & SY_CE_MASK) {
			if (prtsoftecc) {
				noprintf = 1;	/* (not on the console) */
				memlog(ecc);		/* log the error */
				noprintf = 0;
			}
			ecc->syndrome |= SY_CE_MASK;	/* clear latching */
			timeout(ce_enable, (caddr_t)ecc, memintvl*hz);
		}
	}
}

memlog(ecc)
	register struct eccreg *ecc;
{
	register int i;
	register u_char syn;
	register u_int err_addr;
	register u_int base_addr;
	int unum;

	syn = (ecc->syndrome & SY_SYND_MASK) >>	SY_SYND_SHIFT;
	err_addr = ((ecc->syndrome & SY_ADDR_MASK) << SY_ADDR_SHIFT) &
		PHYSADDR;
	if ((ecc->eccena & ENA_TYPE_MASK) != TYPE0)	/* big board */
		base_addr = 0;
	else
		base_addr = (ecc->eccena&ENA_ADDR_MASK)<<ENA_ADDR_SHIFT;

	switch (ecc->eccena & ENA_BDSIZE_MASK) {
	case ENA_BDSIZE_4MB:
		i = MEG4;
		break;
	case ENA_BDSIZE_8MB:
		i = MEG8;
		break;
	case ENA_BDSIZE_16MB:
		i = MEG16;
		break;
	case ENA_BDSIZE_32MB:
		i = MEG32;
		break;
	}
	err_addr = (err_addr|base_addr) & i;	/* make full address */

	printf("mem%d: soft ecc addr %x syn %b ",
	    ecc - ECCREG, err_addr, syn, SYNDERR_BITS);
	for (i = 0; i < (sizeof (memlogtab) / sizeof (memlogtab[0])); i++)
		if (memlogtab[i].m_syndrome == syn)
			break;
	if (i < (sizeof (memlogtab) / sizeof (memlogtab[0]))) {
		printf("%s ", memlogtab[i].m_bit);
		/*
		 * Compute U number on board, first
		 * figure out which half is it.
		 */
		if (atoi(memlogtab[i].m_bit) >= LOWER) {
			if ((ecc->eccena & ENA_TYPE_MASK) == TYPE0)
			switch (err_addr & ECC_BITS) {
			case U14XX:
				unum = 14;
				break;
			case U16XX:
				unum = 16;
				break;
			case U18XX:
				unum = 18;
				break;
			case U20XX:
				unum = 20;
				break;
			} else switch (err_addr & PEG_ECC_BITS) {
			case U14XX:
				unum = 14;
				break;
			case U16XX:
				unum = 16;
				break;
			case PEG_U18XX:
				unum = 18;
				break;
			case PEG_U20XX:
				unum = 20;
				break;
			}
		} else {
			if ((ecc->eccena & ENA_TYPE_MASK) == TYPE0)
			switch (err_addr & ECC_BITS) {
			case U15XX:
				unum = 15;
				break;
			case U17XX:
				unum = 17;
				break;
			case U19XX:
				unum = 19;
				break;
			case U21XX:
				unum = 21;
				break;
			} else switch (err_addr & PEG_ECC_BITS) {
			case U15XX:
				unum = 15;
				break;
			case U17XX:
				unum = 17;
				break;
			case PEG_U19XX:
				unum = 19;
				break;
			case PEG_U21XX:
				unum = 21;
				break;
			}
		}
		printf("U%d%s\n", unum, memlogtab[i].m_bit);
	} else
		printf("No bit information\n");
}

atoi(num_str)
	char *num_str;
{
	register int num;

	num = (*num_str++ & 0xf) * 10;
	num = num + (*num_str & 0xf);
	return (num);
}
#endif SUN3X_470

/*
 * Kernel VM initialization.
 * Assumptions about kernel address space ordering:
 *	(1) gap (user space)
 *	(2) kernel text
 *	(3) kernel data/bss
 *	(4) gap (possibly null)
 *	(5) fixed location devices
 *	(6) debugger
 *	(7) monitor
 *	(8) kernel allocation segments
 *	(9) gap (possibly null)
 *	(10) dvma
 *	(11) MONSTARTPAGE (monitor globals, u area)
 */
kvm_init()
{
	register addr_t v;
	register struct pte *pte;

	/*
	 * Put the kernel segments in kernel address space. Make them
	 * "kernel memory" segment objects. We make the "text" segment
	 * include all the fixed location devices because there are no
	 * soft ptes backing up those mappings.
	 */
	(void) seg_attach(&kas, (addr_t)KERNELBASE,
		(u_int)(vvm - KERNELBASE), &ktextseg);
	(void) segkmem_create(&ktextseg, (caddr_t)NULL);
	(void) seg_attach(&kas, (addr_t)vvm,
	    (u_int)econtig - (u_int)vvm, &kvalloc);
	(void) segkmem_create(&kvalloc, (caddr_t)NULL);
	(void) seg_attach(&kas, (addr_t)SYSBASE,
		(u_int)(Syslimit - SYSBASE), &kseg);
	(void) segkmem_create(&kseg, (caddr_t)Sysmap);
	/*
	 * All level 1 entries other than the kernel were set invalid
	 * when our prototype level 1 table was created. Thus, we only need
	 * to deal with addresses above KERNELBASE here. Also, all ptes
	 * for this region have been allocated and locked, or they are not
	 * used. Thus, all we need to do is set protections. Invalid until
	 * start except for msgbuf/scb page which is KW.
	 */
	for (v = (addr_t)KERNELBASE; v < (addr_t)start; v += PAGESIZE) {
		if ((u_int)v == (((u_int)&scb) & PAGEMASK) ||
		    (u_int)v == (((u_int)&msgbuf) & PAGEMASK))
			(void) as_setprot(&kas, v, PAGESIZE,
			    PROT_READ | PROT_WRITE);
		else
			(void) as_setprot(&kas, v, PAGESIZE, 0);
	}
	/*
	 * (Normally) Read-only until end of text.
	 */
	(void) as_setprot(&kas, v, (u_int)(etext - v),
	    (u_int)(PROT_READ | PROT_EXEC | ((kernprot == 0)? PROT_WRITE : 0)));
	v = (addr_t)roundup((u_int)etext, PAGESIZE);
	/*
	 * Writable to end of allocated region.
	 */
	(void) as_setprot(&kas, v, (u_int)(econtig - v),
	    PROT_READ | PROT_WRITE | PROT_EXEC);
	v = (addr_t)roundup((u_int)econtig, PAGESIZE);

	v = (addr_t)vvm;

	/*
	 * Validate the valloc'ed structures
	 */
	(void) as_fault(&kas, v, (u_int)(econtig - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(econtig - v),
		PROT_READ | PROT_WRITE | PROT_EXEC);
	v = (addr_t)roundup((u_int)econtig, PAGESIZE);

	/*
	 * Invalid to SYSBASE; segu will reclaim some of this space when it
	 * is created.
	 */
	(void) as_setprot(&kas, v, (u_int)((u_int)SYSBASE - (u_int)v), 0);

	/*
	 * Invalid to KERNEL_LIMIT.
	 */
	(void) as_setprot(&kas, v, KERNEL_LIMIT - (u_int)v, 0);
	/*
	 * We leave the fixed address device, debugger, and monitor
	 * alone. They were all set up nicely earlier.
	 */
	v = (addr_t)SYSBASE;
	/*
	 * Invalid to Syslimit.
	 */
	(void) as_setprot(&kas, v, (u_int)(Syslimit - v), 0);
	v = (addr_t)Syslimit;
	/*
	 * We rely on the fact that all the kernel ptes in the upper segment
	 * are in order, so we only need to locate them once.
	 */
	pte = hat_ptefind(&kas, v);
	/*
	 * Invalid until the monitor page.
	 */
	for (; (u_int)v < (u_int)(0 - MMU_PAGESIZE); v += MMU_PAGESIZE, pte++)
		pte->pte_vld = PTE_INVALID;
	/*
	 * Now create a segment for the DVMA virtual
	 * addresses using the segkmem segment driver.
	 */
	(void) seg_attach(&kas, DVMA, (u_int)mmu_ptob(dvmasize), &kdvmaseg);
	(void) segkmem_create(&kdvmaseg, (caddr_t)NULL);
	/*
	 * Flush the ATC of any old mappings.
	 */
	atc_flush_all();
}

/*
 * pagecopy() and pagezero() assume that they will not be called at
 * interrupt level.  But pagecopy can sleep during its operations so
 * we use a cache of addresses here to manage the destination mappings.
 */
#define	VA_CACHE_SIZE 2			/* must be between 1 and 31 */
#define	VA_CACHE_MASK ((1 << VA_CACHE_SIZE) - 1)

static int va_cache_valid = 0;
static addr_t va_cache[VA_CACHE_SIZE];

/*
 * Copy the data from `addr' to physical page represented by `pp'.
 * `addr' is a (user) virtual address which we might fault on.
 */
pagecopy(addr, pp)
	addr_t addr;
	struct page *pp;
{
	register addr_t va;
	register u_long a;
	register struct pte *pte;
	register int s;

	if (va_cache_valid) {
		/* pull address out of cache */
		a = 0;
		for (;;) {
			if ((va_cache_valid & (1 << a)) != 0) {
				va_cache_valid &= ~(1 << a);
				va = va_cache[a];
				break;
			}
			a++;
		}
	} else {
		/*
		 * Cannot get an entry from the cache,
		 * allocate a slot for ourselves.
		 */
		s = splhigh();
		while ((a = rmalloc(kernelmap, (long)CLSIZE)) == NULL) {
			mapwant(kernelmap)++;
			(void) sleep((caddr_t)kernelmap, PZERO - 1);
		}
		(void)splx(s);
		va = Sysbase + mmu_ptob(a);
	}
	/*
	 * We cheat like hell here to make the performance better. We should
	 * have locked down the virtual space, and should call the hat layer
	 * to load/unload the translations neatly. However, since we know
	 * we are just going to undo the mapping, and the whole kernel is
	 * always locked anyway, we fudge it.
	 */
	pte = hat_ptefind(&kas, va);
	*(u_int *)pte = MMU_INVALIDPTE;
	pte->pte_vld = PTE_VALID;
	pte->pte_pfn = page_pptonum(pp);
	atc_flush_entry(va);

	(void) copyin((caddr_t)addr, va, PAGESIZE);

	vac_flush();
	pte->pte_vld = PTE_INVALID;
	atc_flush_entry(va);

	if ((va_cache_valid & VA_CACHE_MASK) == VA_CACHE_MASK) {
		/* cache is full, just return the virtual space */
		s = splhigh();
		rmfree(kernelmap, (long)CLSIZE, (u_long)btokmx(va));
		(void)splx(s);
	} else {
		/* find the first non-valid slot and store va there  */
		a = 0;
		for (;;) {
			if ((va_cache_valid & (1 << a)) == 0) {
				va_cache[a] = va;
				va_cache_valid |= (1 << a);
				break;
			}
			a++;
		}
	}
}

/*
 * Zero the physical page from off to off + len given by `pp'
 * without changing the reference and modified bits of page.
 * pagezero uses global CADDR2 and assumes that no one uses this
 * map at interrupt level and no one sleeps with an active mapping there.
 */
pagezero(pp, off, len)
	struct page *pp;
	u_int off, len;
{
	register struct pte *pte;

	ASSERT((int)len > 0 && (int)off >= 0 && off + len <= PAGESIZE);

	pte = hat_ptefind(&kas, CADDR2);
	*(u_int *)pte = MMU_INVALIDPTE;
	pte->pte_vld = PTE_VALID;
	pte->pte_pfn = page_pptonum(pp);
	atc_flush_entry(CADDR2);

	bzero(CADDR2 + off, len);

	vac_flush();
	pte->pte_vld = PTE_INVALID;
	atc_flush_entry(CADDR2);
}

/*
 * Console put and get character routines.
 */
cnputc(c)
	register int c;
{
	register int s;

	s = spl7();
	if (c == '\n')
		(*romp->v_putchar)('\r');
	(*romp->v_putchar)(c);
	(void) splx(s);
}

/*
 * fork a kernel process at address "start".
 * Used by main to create init and pageoutd.
 * used by rfs to create kernel "mediumweight" processes which have
 * their own u area but no address space.
 */
kern_proc(startaddr, arg)
	void (*startaddr)();
	int arg;
{
	register struct seguser *newseg;
	struct proc *pp;
	struct s {
		long s_sr;
		long s_startaddr;
		short s_pad;
		long s_arg;
		long s_dummy;
	} *s;

	/* This segment may not be needed for some kernel processes. */
	newseg = (struct seguser *) segu_get();
	if (newseg == (struct seguser *)0)
		panic("can't create kernel process");

	/*
	 * Forge kernel stack.
	 * We are careful to push a 0 at the top of the kernel
	 * stack since yield_child uses this to tell if it's a kernel
	 * or user process.  yield_child will make it appear that
	 * start(arg) was called from exit1 (exit1 just calls exit(0)).
	 */
	s = (struct s *)KSTACK(newseg) - 1;
	s->s_dummy = 0;				/* dummy -- must be 0 */
	s->s_arg = arg;				/* argument */
	s->s_pad = 0;				/* pad */
	s->s_startaddr = (long)startaddr;	/* new pc */
	s->s_sr = SR_LOW;			/* kernel ps */

	pp = freeproc;
	freeproc = pp->p_nxt;
	newproc(0, (struct as *)0, 0, (struct file *)0, newseg, pp);
}

/*
 * Set up child's kstack for rte by pushing <10000(long), pc(long), sr(short)>.
 * Note that this leaves an odd number of shorts on the stack, but when
 * the system is entered from user state, an odd number of shorts are pushed
 * (when saving registers due to the padding the sr to a long) in addition
 * to the information pushed by the chip and thus we will have long word
 * stack alignment thoughout the kernel.
 * Important that pad is non-zero; this is how yield_child tells if
 * kernel or user process (non-zero is user process).
 */
void
userstackset(seg)
	struct seguser *seg;
{
	struct s {
		long s_sr;
		long s_pc;
		int s_pad;
	} *s;

	int sr = u.u_ar0[PS] & ~SR_CC;	/* clear error from fork system call */
	int pc = u.u_ar0[PC];

	s = (struct s *)KSTACK(seg) - 1;
	s->s_pad  = 0x10000;	/* non-zero pad + fmt + vor */
	s->s_pc = pc;
	s->s_sr = (short)sr;
}

int
cngetc()
{
	register int c;

	while ((c = (*romp->v_mayget)()) == -1)
		;
	return (c);
}

#ifdef SUN3X_470
/*
 * This routine initializes and enables the physical system cache.
 */
pac_init(pte, addr)
	register struct pte *pte;
	addr_t addr;
{
	register int i;

	/*
	 * Loop through each page of the cache tags, using the pte that
	 * is passed to us to map each page. Zeroing out the
	 * tags initializes the cache to invalid.
	 * (implicit assumption -> cache tags size is multiple of page size)
	 */
	*(u_int *)pte = MMU_INVALIDPTE;
	pte->pte_vld = PTE_VALID;
	pte->pte_nocache = 1;
	for (i = 0; i < mmu_btop(PCACHETAGS_SIZE); i++) {
		pte->pte_pfn = mmu_btop(OBIO_PCACHE_TAGS) + i;
		atc_flush_entry(addr);
		bzero(addr, MMU_PAGESIZE);
	}
	*(u_int *)pte = MMU_INVALIDPTE;
	/*
	 * Enable the cache. Also mark bcopy hardware usable.
	 */
	if (softenablereg & S_CPUCACHE) on_enablereg((u_char)ENA_CACHE);
	if (softenablereg & S_DBGCACHE) on_enablereg((u_char)ENA_DBGCACHE);
	if (softenablereg & S_BCOPY) bcenable = 1;
}

#ifdef IOC
/*
 * This routine initializes the I/O Cache.
 */
ioc_init(pte, addr)
	register struct pte *pte;
	addr_t addr;
{
	register int i;

	/*
	 * Map in the I/O cache tags and zero them out.
	 * Assumes the tags are no larger than a page.
	 */
	*(u_int *)pte = MMU_INVALIDPTE;
	pte->pte_vld = PTE_VALID;
	pte->pte_nocache = 1;
	pte->pte_pfn = mmu_btop(OBIO_IOC_TAGS);
	atc_flush_entry(addr);
	for (i = 0; i < (IOC_CACHE_LINES * 4); i += 4)
		*((int *)addr + i) = 0;
	*(u_int *)pte = MMU_INVALIDPTE;
}

/*
 * This routine flushes the specified line of the I/O cache.
 */
void
ioc_flush(line)
	int line;
{
	int *ptr = (int *)V_IOC_FLUSH;

	ptr[line * 4] = 0;
}
#endif IOC
#endif SUN3X_470


/*
 * We need to check the first time to see if we have an idprom, or if
 * the id information is in the eeprom.  Sun3x/80 saves the id information
 * in the eeprom.
 */

#define	FIND_IDPROM	0
#define	IN_IDPROM	1
#define	IN_EEPROM	2
static  int idprom_info = FIND_IDPROM;

getidinfo(id)
char *id;
{
	if (idprom_info == FIND_IDPROM) {
		if (peekc((char *) V_IDPROM_ADDR) != -1) {
			idprom_info = IN_IDPROM;
		} else {
			idprom_info = IN_EEPROM;
		}
	}
	if (idprom_info == IN_IDPROM)
		getidprom(id);
	else
		getideeprom(id);
}

int
physpos(pos)
int pos;
{
	struct physmemory *pmem;
	int position;

	for (pmem = romp->v_physmemory, position = pos;
	    pmem != (struct physmemory *) NULL; pmem = pmem->next) {
		position += pmem->address;
		if ((position >= pmem->address) &&
		    (position < (pmem->address + pmem->size)))
			return (position);
		else
			position -= (pmem->address + pmem->size);
	}
	return (-1);
}


#ifdef SUN3X_80
int kill_on_parity = 1;
int parity_errcnt = 0;
int parity_hard_error = 0;
#ifdef PARITY
int debug_parity = 1;
#define	PPRINTF(x)	if (debug_parity) printf (x)
#else
#define	PPRINTF(x)
#endif PARITY

user_parity_recover(addr, rw)
	register addr_t addr;
	register enum seg_rw rw;
{
	static char *ukilled =
	    "PARITY ERROR: CANNOT RECOVER - User process %d killed\n";
	struct pte *pte;
	register int res;
	long *x;
	register struct proc *p;
	register struct as *as;

	parity_errcnt++;
	PPRINTF("user_parity_recover \n");

	p = u.u_procp;
	as = p->p_as;

	pte = hat_ptefind(as, addr);
	if (pte == (struct pte *)NULL) {
		panic("Parity error: Cannot find pte for address");
		/* NOT REACHED */
	}

	if (!(pte->pte_vld == PTE_VALID)) {	/* paranoid checks */
		x = (long *)pte;
		printf("Parity error:invalid page addr 0x%x pte 0x%x\n",
		    addr, *x);
		panic("synchronous parity error - user invalid page");
	}

	if ((u.u_exdata.ux_mag == ZMAGIC) || !(pte->pte_modified)) {
		PPRINTF(" ZMAGIC type object\n");
		if (page_giveup(addr, pte)) {
			/* fault the pagein */
			res = pagefault(addr, rw, 0, MMUE_INVALID);
			PPRINTF(" RECOVERED FROM PARITY ERROR \n");
		} else {
			panic(" Parity error: Cannot locate page ");
			/* NOT REACHED */
		}
		if (parity_hard_error == 0x10) {
			panic(" Too Many Hard parity errors, REPLACE SIMMS");
			/* NOT REACHED */
		}
	} else if (kill_on_parity) {
		printf(ukilled, u.u_procp->p_pid);
		psignal(u.u_procp, SIGBUS);
		res = 1;
	} else {
		panic("synchronous parity error - user mode");
		/* NOT REACHED */
	}
	PPRINTF(" user parity recover done\n");
	return (res);
}

#define	BANK_SIZE	0x1000000

parity_report(addr, erreg)
addr_t	addr;
u_char	erreg;
{
	int	whichbyte;
	int	dvma, bank, simmbyte;
	int	paddr;

	dvma = MEMREG->mr_dvma;
	paddr = MEMREG->mr_paddr;
	printf("Parity error:\n\tvirtual address 0x%x\n\terrreg 0x%x\
	    \n\tphysical address 0x%x\n", addr, erreg, paddr);
	whichbyte = erreg&PER_ERR;
	if (dvma)
		panic("DVMA cycle Parity error");
	switch (whichbyte) {
	    case PER_ERR24:
		simmbyte = 3;
		break;
	    case PER_ERR16:
		simmbyte = 2;
		break;
	    case PER_ERR08:
		simmbyte = 1;
		break;
	    case PER_ERR00:
		simmbyte = 0;
		break;
	    default:
		printf(" Parity error Byte unknown\n");
		return;
	}

	/*
	 * Very machine dependent simm calculation.  This one finds the
	 * correct simm location for the 3X/80.
	 */
	for (bank = 0; paddr > BANK_SIZE; bank++)
		paddr -= BANK_SIZE;
	printf("Parity error at SIMM U0%c0%c\n",
	    '6'+bank, simmbyte+(3*(1-(bank%2)))+'0');
}




/*
 * retry()
 * See if we get another parity error at the same location
 */

int
retry(addr, pte)
addr_t addr;
struct pte *pte;
{
	unsigned long put = 0xaaaaaaaa;
	long retval, vaddr;
	long taddr= 0;
	/*
	 * There may have been more than one byte parity error,
	 * test out a long word peek. Adjust the address to do that.
	 */

	addr++;
	vaddr = (long)addr;
	while (vaddr&0x3) {
		vaddr--;
		addr--;
	}
	/*
	 * Change prot, don't need to put
	 * it back. We have already given
	 * up the page.
	 */

	pte->pte_readonly = PG_KW;

	*(long *)addr = put;

	retval = peekl((long *) addr, &taddr);
	if (retval == -1)  {
		printf("\tHARD Parity error\n");
		parity_hard_error++;
	}
	return (retval);
}
#endif SUN3X_80
