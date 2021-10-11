#ident "@(#)machdep.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1989-1991 by Sun Microsystems, Inc.
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
#include <sys/debug.h>
#include <debug/debug.h>
#include <sys/dumphdr.h>
#include <sys/bootconf.h>
#include <machine/async.h>
#include <mon/sunromvec.h>

#ifndef	NOPROM
union ptpe 	*tmpptes = 0;
u_int		PA_TMPPTES;	/* Use as location for mapping */
#endif	NOPROM

#include <os/dlyprf.h>
typedef void (*voidfunc_t)();

/*
 * no telling what kind of pointers we are going
 * to want to convert, so route the translation
 * through this macro. It used to cast to unsigned
 * and mask off some top bits, but now it uses
 * a real routine over in vm_hat that does the
 * mmu probing and figures out what is going on.
 */
#define	VA2PA(va)	va2pa((addr_t)(va))
extern unsigned va2pa();

/*
 * If we know that we are translating
 * a virtual address that is in the contiguous
 * kernel mapping area, we can use CA2PA. This
 * has the benefit that we can use it when the
 * address is not yet mapped in, and econtig
 * has not been extended to include it.
 */
#define	CA2PA(va)	(((u_int)(va)) - KERNELBASE)

#include "percpu.h"

#ifndef	MULTIPROCESSOR
extern struct user p0uarea;
#else	MULTIPROCESSOR
extern struct PerCPU cpu0[];	/* locore.s */
#endif	MULTIPROCESSOR

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
#include <sun/fault.h>
#include <machine/frame.h>
#include <sundev/mbvar.h>

#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/clock.h>
#include <machine/pte.h>
#include <machine/scb.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/asm_linkage.h>
#include <machine/eeprom.h>
#include <machine/intreg.h>
#include <machine/seg_kmem.h>
#include <machine/buserr.h>
#include <machine/auxio.h>
#include <machine/trap.h>
#include <machine/module.h>

#include <mon/openprom.h>

#include <vm/hat.h>
#include <vm/anon.h>
#include <vm/as.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/seg_u.h>
#include <vm/swap.h>

#ifdef IOMMU
#include <machine/iommu.h>
#endif

#ifdef IOC
#include <machine/iocache.h>
#endif

#define	MINBUF		8			/* min # of buffers (8) */
#define	BUFPERCENT	(1000 / 15)		/* max pmem for buffers(1.5%) */
#define	MAXBUF		(BUFBYTES/MAXBSIZE - 1)	/* max # of buffers (255) */

#define MINPTSA (((mmu_btop(0x2000000) + NL3PTEPERPT - 1) * 16) / NL3PTEPERPT)
#define MINPTS(a)       (a > MINPTSA? a : MINPTSA)

/*
 * Declare these as initialized data so we can patch them.
 */
int nbuf = 0;
extern struct memlist *physmemory;	/* set up in fillsysinfo */
extern struct memlist *availmemory;	/* set up in fillsysinfo */
extern struct memlist *virtmemory;	/* set up in fillsysinfo */
int physmem = 0;	/* memory size in pages, patch if you want less */
int kernprot = 1;	/* write protect kernel text */
int msgbufinit = 1;	/* message buffer has been initialized, ok to printf */
int nrnode = 0;		/* maximum number of NFS rnodes */
int nopanicdebug = 0;	/* 0 = call debugger (if any) on panic, 1 = reboot */
#if defined(ROOT_ON_TAPE)
int boothowto = RB_SINGLE | RB_WRITABLE; /* MUNIX gets "-sw" */
#else
int boothowto = 0;	/* reboot flags (see sys/reboot.h) */
#endif

extern int	initing;
extern int	cpu;

extern char end[], edata[];

#if	defined(VAC)

int use_cache = 1;

int use_page_coloring = 1;
int use_mix = 1;
int use_vik_prefetch = 0;
int use_mxcc_prefetch = 1;
int use_table_walk = 1;
int use_store_buffer = 1;
int use_ic = 1;
int use_dc = 1;
int use_ec = 1;
int use_multiple_cmd = 1;
int use_rdref_only = 0;
int vac_copyback = 1;		/* default to CopyBack */

int do_pg_coloring = 0;	/* page coloring for viking/-Ecache case */

char   *vac_mode;
addr_t	kern_nc_top_va = end;
u_int	kern_nc_top_pp = 0;
addr_t	kern_nc_end_va = end;
u_int	kern_nc_end_pp = 0;
#else
int use_cache = 0;
#endif	VAC && !MINIROOT

#ifdef	IOC
int use_ioc = 1;	/* variable to patch to have the kernel use i/o cache */
int iocenable = 0;	/* tells all that the IOC is turned on. */
#else	!IOC
#define	use_ioc 0
#endif	!IOC

#ifndef	MULTIPROCESSOR
int bcopy_res = -1;	/* block copy buffer reserved (in use) */
#else
extern int bcopy_res;
#endif	!MULTIPROCESSOR
int use_bcopy = 1;	/* variable to patch to have kernel use bcopy buffer */
int bcopy_cnt = 0;	/* number of bcopys that used the buffer */
int bzero_cnt = 0;	/* number of bzeros that used the buffer */

extern int		uniprocessor; /* default to MultiProcessor */
#ifdef NOPROM
/*
 * Configuration parameters set at boot time.
 */
struct	cpuinfo {
	u_int cpui_mmutype : 1;		/* mmu type, 0=2 level, 1=3 level */
	u_int cpui_vac : 1;		/* has virtually addressed cache */
	u_int cpui_ioctype : 2;		/* I/O cache type, 0 = none */
	u_int cpui_iom : 1;		/* has I/O MMU */
	u_int cpui_clocktype : 2;	/* clock type */
	u_int cpui_bcopy_buf : 1;	/* has bcopy buffer */
	u_int cpui_sscrub : 1;		/* requires software ECC scrub */
	u_int cpui_linesize : 8;	/* cache line size (if any) */
	u_short cpui_nlines;		/* number of cache lines */
	u_short cpui_dvmasize;		/* size of DMA area */
	u_short cpui_nctx;		/* number of contexts */
	u_int cpui_nsme;		/* number of segment map entries */
	u_int cpui_npme;		/* number of page map entries */
} cpuinfo[] = {
/* mmu vac ioc iom clk bcp scr	lsz   nln  dsz nctx   nsme   npme */
/* 4C_60 */
{   0,	1,  0,	0,  0,	0,  0,	16, 4096, 192,	 8, 32768,  8192 },
};
struct	cpuinfo *cpuinfop;	/* misc machine info */
/*
 * ===========================================
 * FIXME: will be gone when true cpuinfo is done.
 * ===========================================
 */
u_int module_type = -1;		/* some impossible value */

#else NOPROM
extern struct machinfo mach_info;
#ifdef	VME
extern struct vmeinfo  vme_info;
#endif	VME
extern struct modinfo  mod_info[];
#endif
#define	MAX_NCTXS	(1<<12) /* the most we will ever want */
u_int	nctxs = 0;		/* for srmmu table size and alignment */
u_int	nswctxs = 0;		/* how many we really use */
addr_t econtig = end;		/* End of first block of contiguous kernel */
addr_t eecontig = end;		/* end of segu, which is after econtig */
addr_t vstart;			/* Start of valloc'ed region */
addr_t vvm;			/* Start of valloc'ed region we can mmap */
int vac_linesize;		/* size of a cache line */
int vac_nlines;			/* number of lines in the cache */
int vac_pglines;		/* number of cache lines in a page */
int vac_size = 0;		/* cache size in bytes */
#ifndef	MULTIPROCESSOR
int Cpudelay = 0;		/* delay loop count/usec */
#endif	MULTIPROCESSOR
int cpudelay = 0;		/* for compatibility with old macros */
#ifdef NOPROM
int dvmasize;			/* usable dvma space */
#else /* XXX - how should this be setup */
int dvmasize = 0xff;		/* usable dvma space, in pages (1M) */
#endif NOPROM
u_int shm_alignment = 0;	/* VAC address consistency modulus */

u_int	nl1pts = 0;		/* how many l1pts we have */
extern struct l1pt *l1pts, *el1pts;
extern struct l1pt *hat_l1alloc();

int	nmod	= 1;		/* number of modules */

#ifdef	MULTIPROCESSOR

int	ncpu = 1;		/* active cpu count */
int	procset = 1;		/* bit map of active CPUs */

/*
 * points to the level-2 page table structure (shared by all CPUs)
 * used to map in the area where all the CPU's per-CPU areas
 * are mapped: VA_PERCPU.
 */
struct ptbl *percpu_ptbl;
/*
 * the level-1 page table entry for each CPU's per-CPU area
 * which is at virtual address VA_PERCPUME.
 * used by hat_map_percpu to map in the per-CPU area for a
 * user process.
 */
union ptpe percpu_ptpe[NCPU];	/* for hat_map_percpu */
#endif	MULTIPROCESSOR

#ifdef	NOPROM
extern int availmem;		/* physical memory available in SAS */
#endif	NOPROM

extern u_int	ldphys();

/*
 * Most of these vairables are set to indicate whether or not a particular
 * architectural feature is implemented.  If the kernel is not configured
 * for these features they are #defined as zero. This allows the unused
 * code to be eliminated by dead code detection without messy ifdefs all
 * over the place.
 */

#ifdef	VAC
int cache = 0;			/* address cache type. (none = 0) */
int vac = 0;			/* virtual address cache type (none == 0) */
#endif	VAC

#ifdef	IOMMU
int iom = 0;
union iommu_pte *ioptes, *eioptes; /* virtual addr of ioptes */
union iommu_pte *phys_iopte;	   /* phys addr of ioptes */

#endif	IOMMU

#ifdef	IOC
int ioc = 1;			/* I/O cache type (none == 0) */
#endif	IOC

int bcopy_buf = 0;		/* block copy buffer present */

/*
 * Dynamically allocated MMU structures
 */
extern struct	ctx *ctxs, *ectxs;	/* declared in vm_hat.c */
/* Pointer to (root) context Table */
/* extern */ struct ctx_table *ctxtabptr;  /* ctx_table is not declared in
					    * vm_hat.c !! */
/* pointer to level 1 page table for system */
struct l1pt *kl1pt = (struct l1pt *)NULL;
/* pointer to array of level 2 page tables */
struct l2pt *kl2pts = (struct l2pt *)NULL;

/*
 * VM data structures
 */
int page_hashsz;		/* Size of page hash table (power of two) */
struct	page **page_hash;	/* Page hash table */
struct	seg *segkmap;		/* Kernel generic mapping segment */
struct	map *kernelmap;		/* for Sysbase -> Syslimit */
struct	map *bufmap;		/* for Disk Buffer Cache */
struct	map *heapmap;		/* for Kernel Heap */
struct	seg ktextseg;		/* Segment used for kernel executable image */
struct	seg kvalloc;		/* Segment used for "valloc" mapping */
#ifdef	MULTIPROCESSOR
struct	seg kpercpume;
struct	seg kpercpu;
#endif	MULTIPROCESSOR

int npts = 0; /* number of page tables */

#define	TESTVAL 0xA55A		/* memory test value */

/*
 * FORTH monitor gives us romp as a variable.
 */
#ifdef	notdef
struct sunromvec *romp = (struct sunromvec *)0xffe00010;
struct debugvec *dvec = (struct debugvec *)0;
#else
extern struct sunromvec *romp;
extern struct debugvec *dvec;
#endif
#ifdef	OPENPROMS
extern dnode_t	prom_nextnode();
extern char *prom_bootargs();
#ifndef	MULTIPROCESSOR
extern void	prom_exit_to_mon();
extern void	prom_enter_mon();
extern void	prom_printf();
#define	prom_eprintf		prom_printf
extern void	prom_writestr();
extern void	prom_putchar();
extern void	prom_mayput();
#else	MULTIPROCESSOR
#define	prom_exit_to_mon	mpprom_exit
#define	prom_enter_mon		mpprom_enter
#define	prom_printf		mpprom_printf
#define	prom_eprintf		mpprom_eprintf
#define	prom_writestr		mpprom_writestr
#define	prom_putchar		mpprom_putchar
#define	prom_mayput		mpprom_mayput
#endif	MULTIPROCESSOR
#endif	OPENPROMS

/*
 * Globals for asynchronous fault handling
 */
atom_t	module_error;
/*
 * When the first system-fatal asynchronous fault is detected,
 * this variable is atomically set.
 */
atom_t	system_fatal;

#ifdef	MULTIPROCESSOR

atom_t	mm_flag;

#else	MULTIPROCESSOR
/*
 * ... and info specifying the fault is stored here:
 */
struct async_flt sys_fatal_flt;
/*
 * The following data is stored in the PERCPU area for MP systems,
 * but for single-processor systems it is stored here:
 */
/*
 * Used to store a queue of non-fatal asynchronous faults to be
 * processed.
 */
struct	async_flt a_flts[NCPU][MAX_AFLTS];
/*
 * Incremented by 1 (modulo MAX_AFLTS) by the level-15 asynchronous
 * interrupt thread before info describing the next non-fatal fault
 * in a_flts[a_head].
 */
u_int	a_head[NCPU];
/*
 * Incremented by 1 (modulo MAX_AFLTS) by the level-12 asynchronous
 * fault handler thread before processing the next non-fatal fault
 * described by info in a_flts[a_tail].
 */
u_int	a_tail[NCPU];
#endif	MULTIPROCESSOR
/*
 * When nofault is set, if an asynchronous fault occurrs, this
 * flag will get set, so that the probe routine knows something
 * went wrong.	It is up to the probe routine to reset this
 * once it's done messing around.
 *
 * values:
 *	0	no fault expected
 *	-1	fault may happen soon
 *	1	fault happened.
 */
int	pokefault = 0;

#ifndef	MULTIPROCESSOR
/*
 * for single processor which doesn't have a PERCPU area
 * which contains cpuid, define cpuid here to be always set
 * to 0.
 */
u_int	cpuid = 0;
#endif	MULTIPROCESSOR

/*
 * Monitor pages may not be where this sez they are.
 * and the debugger may not be there either.
 *
 * The point is, is the IOPBMEM will set itself up
 * as the last couple of pages of physical memory
 * that we see.
 *
 * Also, note that 'pages' here are *physical* pages,
 * which are 4k on sun4c. Thus, for us the first 4 pages
 * of physical memory are equivalent of the first two
 * 8k pages on the sun4.
 *
 *		  Physical memory layout
 *		(not necessarily contiguous)
 *
 *		_________________________
 *		|    monitor pages	|
 *    availmem -|-----------------------|
 *		|	IOPBMEM		|
 *		|-----------------------|
 *		|			|
 *		|	page pool	|
 *		|			|
 *		|-----------------------|
 *		|   configured tables	|
 *		|	buffers		|
 *   firstaddr -|-----------------------|
 *		|   hat data structures |
 *		|-----------------------|
 *		|    kernel data, bss	|
 *		|-----------------------|
 *		|    interrupt stack	|
 *		|-----------------------|
 *		|    kernel text (RO)	|
 *		|-----------------------|
 *		|    trap table (4k)	|
 *		|-----------------------|
 *	page 2-3|	 msgbuf		|
 *		|-----------------------|
 *	page 0-1|  unused- reclaimed	|
 *		|_______________________|
 *
 *
 *		  Virtual memory layout.
 *		/-----------------------\
 * 0xFFFFFFFF  -|-----------------------|
 *		|	DVMA		|
 *		|    IOPB memory	|
 * 0xFFF00000  -|-----------------------|- DVMA
 *		|	monitor (2M)	|
 * 0xFFD00000  -|-----------------------|- MONSTART
 *		|	debugger (1M)	|
 * 0xFFC00000  -|-----------------------|- DEBUGSTART
 *		|	segkmap (2.75M)	|
 *		|  >= MINMAPSIZE (2M)	|
 * 0xFF943000	|-----------------------|
 *		|	NCARGS (1MB)	|
 * 0xFF843000  -|-----------------------|- Syslimit
 *		|	mmap (1 page)	|
 * 0xFF842000  -|-----------------------|- vmmap
 *		|	CMAP2 (1 page)	|
 * 0xFF841000  -|-----------------------|- CADDR2
 *		|	CMAP1 (1 page)	|
 * 0xFF840000  -|-----------------------|- CADDR1
 *		|	Mbmap		|
 *		|  MBPOOLMMUPAGES (2MB)	|
 * 0xFF640000  -|-----------------------|- mbutl
 *		|	kernelmap	|
 *		|    SYSPTSIZE (6.25MB)	|
 * 0xFF000000  -|-----------------------|- Sysbase
 *		|	clock		|
 * 0xFEFFFFF8  -|-----------------------|
 *		|	nvram		|
 * 0xFEFFE000  -|-----------------------|
 *		|    counter/timers	|
 * 0xFEFF9000  -|-----------------------|
 *		|	interrupts	|
 * 0xFEFF4000  -|-----------------------|
 *		|    system control	|
 * 0xFEFF3000  -|-----------------------|
 *		|    vme control	|
 * 0xFEFF2000  -|-----------------------|
 *		|    vme int vector	|
 * 0xFEFF1000  -|-----------------------|
 *		|    auxilliary i/o	|
 * 0xFEFF0000  -|-----------------------|
 *		|   memory error	|
 * 0xFEFEF000  -|-----------------------|
 *		|	unused		|
 * 0xFEFEB000  -|-----------------------|
 *		|    sbus control	|
 * 0xFEFEA000  -|-----------------------|
 *		|    module id		|
 * 0xFEFE9000  -|-----------------------|
 *		|	unused		|
 * 0xFEFE1000  -|-----------------------|
 *		|	iommu		|
 * 0xFEFE0000  -|-----------------------|
 *		|	 unused		|
 * 0xFEF00000  -|-----------------------|	given to boot prom
 *		|	BOOTPROM	|	11+meg free here
 * 0xFE400000  -|-----------------------|	given to boot prom
 *		|	 Percpu		|
 *		|	Data Areas	|
 * 0xFE000000  -|-----------------------| VA_PERCPU
 *		|	reserved	|	<<<unstable mappings>>>
 * 0xFD100000  -|-----------------------|
 *		|	 PercpuMe	|
 *		|	Data Area	|
 * 0xFD000000  -|-----------------------| VA_PERCPUME (BUFLIMIT)
 *		|      Buffer Cache	|	BUFBYTES
 * 0xFCC00000	|-----------------------|- BUFBASE
 *		|      Kernel Heap	|	HEAPBYTES
 * 0xFB000000	|-----------------------|- HEAPBASE
 *		|	 TMPPTES	|
 * 0xFAFBE000	|-----------------------|- VA_TMPPTES
 *
 *		|-----------------------|- eecontig
 *		|	 segu		|
 *		|-----------------------|- econtig
 *		|   configured tables	|
 *		|	buffers		|
 *		|  hat data structures	|
 *		|-----------------------|- vvm
 *		|-----------------------|- end
 *		|	kernel		|
 *		|-----------------------|
 *		|   trap table (4k)	|
 * 0xF0004000  -|-----------------------|- start
 *		|	 msgbuf		|
 * 0xF0002000  -|-----------------------|- msgbuf
 *		|  user copy red zone	|
 *		|	(invalid)	|
 * 0xF0000000  -|-----------------------|- KERNELBASE
 *		|	user stack	|
 *		:			:
 *		:			:
 *		|	user data	|
 *		|-----------------------|
 *		|	user text	|
 * 0x00002000  -|-----------------------|
 *		|	invalid		|
 * 0x00000000  _|_______________________|
 */

/*
 * Define for start of dvma segment that monitor maps in.
 */
#define	MON_DVMA_ADDR		(0 - DVMA_MAP_SIZE)

#ifndef NOPROM
/*
 * return the level-2 ptp or pte for the specified address in the
 * current translation map. if the translation uses a level-1 pte,
 * fake up a l2 pte.
 * If no translation exists return 'dflt'.
 */

u_int
get_rom_l2_ptpe(vaddr, rv)
u_int vaddr;
u_int rv;
{
	union ptpe	ct, rp, l1, l2;
	unsigned pa, po;

	ct.ptpe_int = mmu_getctp();
	pa = ct.ptp.PageTablePointer << MMU_STD_PTPSHIFT;
	rp.ptpe_int = ldphys(pa);
	switch (rp.ptp.EntryType) {
	    case MMU_ET_PTE:	/* pte in context table: fake a level-2 pte. */
		po = ((vaddr >> MMU_STD_PAGESHIFT) &
			(MMU_STD_ROOTMASK &~ MMU_STD_SECONDMASK));
		rp.pte.PhysicalPageNumber += po;
		rv = rp.ptpe_int;
		break;

	    case MMU_ET_PTP:
		pa = (rp.ptp.PageTablePointer << MMU_STD_PTPSHIFT) |
			MMU_L1_INDEX(vaddr) << 2;
		l1.ptpe_int = ldphys(pa);
		switch (l1.ptp.EntryType) {
		    case MMU_ET_PTE: /* pte in ctx table: fake a level-2 pte. */
			po = ((vaddr >> MMU_STD_PAGESHIFT) &
				(MMU_STD_FIRSTMASK &~ MMU_STD_SECONDMASK));
			l1.pte.PhysicalPageNumber += po;
			rv = l1.ptpe_int;
			break;

		    case MMU_ET_PTP:
			pa = ((l1.ptp.PageTablePointer << MMU_STD_PTPSHIFT) |
				MMU_L2_INDEX(vaddr) << 2);
			l2.ptpe_int = ldphys(pa);
			switch (l2.ptp.EntryType) {
			    case MMU_ET_PTE:
			    case MMU_ET_PTP:
				rv = l2.ptpe_int;
				break;
			}
		}
	}
	return (rv);
}
#endif	NOPROM

/*
 * This routine sets up a temporary mapping for the kernel so it can
 * run at the right addresses quickly. It uses the built-in level 1
 * and level 2 page table structs, and borrows the end of physical
 * memory for the level 3 tables. When it is done, the first
 * MAINMEM_MAP_SIZE of memory will be mapped straight through with a
 * virtual offset of KERNELBASE. Also, the monitor and debugger ptes
 * are preserved as is, and the dvma segment that the monitor set up
 * is also preserved.  The routine returns the value to be loaded into
 * the context table pointer.
 *
 * NOTE: this routine now runs at the proper virtual address, since
 * we can kludge up a mapping that works.
 */

#define	ALIGN(v, m)	((((int)v) + m - 1) &~ (m - 1))

u_int
load_tmpptes()
{
	register struct l1pt *kl1ptr;
	register union ptpe *kl2ptps;
	register union ptpe *kpteptr;
	struct l3pt *kl3ptr;
	struct l2pt *kl2ptr;
	register u_int i, j;
	int npages;
	struct memlist *pmemptr;
#ifndef NOPROM
	struct memlist *first_membank;
	extern union ptpe *tmpptes;
	extern void build_memlists();
	extern caddr_t prom_map();
#endif

#ifndef NOPROM
	u_int get_rom_l2_ptpe();
#endif
	extern int kptstart[];

#ifndef NOPROM
	build_memlists();	/* build {phys,avail,virt}memory lists */

	/*
	 * Start with the memory size the monitor claims.
	 * "availmemory" is set up in fillsysinfo
	 */
	npages = 0;
	for (pmemptr = availmemory; pmemptr; pmemptr = pmemptr->next) {
		npages += mmu_btop(pmemptr->size);
		if (!(pmemptr->address))
			first_membank = pmemptr;
	}
	physmem = npages;
	if ((first_membank->size) > TMPPTES) {
		PA_TMPPTES = ((first_membank->size) - TMPPTES);
	} else {
		panic("no physical space for tmpptes");
	}
#endif	NOPROM
	/* locate start of reserved area [virtual address] */
	i = (int)kptstart;

	/*
	 * Calculate actual location for kernel level 1 & 2 page tables.
	 * aligned to multiple of size of l1 page table.
	 */
	i = ALIGN(i, sizeof (struct l1pt));

#ifdef	MULTIPROCESSOR
	/* further, make sure it is page aligned. */
	i = ALIGN(i, PAGESIZE);
#endif	MULTIPROCESSOR
	kl1pt = kl1ptr = (struct l1pt *)i;
#ifndef	MULTIPROCESSOR
	i += sizeof (struct l1pt);
#else	MULTIPROCESSOR
	/* one PAGE per cpu for level-1 tables ??? */
	i += PAGESIZE * NCPU;
#endif	MULTIPROCESSOR

	/*
	 * Calculate the page number to use for the temp ptes and the
	 * physical address of the ptes.
	 */
/* ### If running with sas, be sure to use at least 2 megabytes of memory */
	/*
	 * We grab the memory just under the top of the memory (+ slop) to put
	 * the temporary 3rd level page tables. We will copy these out during
	 * startup to the real tables
	 */
#ifndef	NOPROM
	/*
	 * Call to prom to get space for tmpptes
	 */
	if (!(tmpptes = (union ptpe *)prom_map((caddr_t)VA_TMPPTES, OBMEM,
		PA_TMPPTES, TMPPTES))) {
		panic("can't map space for tmpptes");
	}
	kpteptr = tmpptes;	/* THIS IS A VA ABOVE KERNELBASE */

#else	NOPROM
	kpteptr = TMPPTES;	/* THIS IS A VA ABOVE KERNELBASE */
#endif	NOPROM
	kl3ptr = (struct l3pt *) kpteptr;

	kl2pts = (struct l2pt *)&kpteptr[NL3PTEPERPT*NKL3PTS];
	kl2ptps = (union ptpe *)(kl3ptr + NKL3PTS);
	kl2ptr = (struct l2pt *)kl2ptps;

	/*
	 * Have the first level 0 entry map the bottom part of memory so
	 * that we can continue to run down there until we relocate ourselves
	 * to high memory. This will map the low 16 Meg of memory to itself.
	 */

	kl1ptr->ptpe[0].ptpe_int = PTEOF(0, 0, MMU_STD_SRWX, 0);

	/*
	 * Invalidate all the level 1 entries.
	 */
	for (i = 1; i < NL1PTEPERPT; i++)
		kl1ptr->ptpe[i].ptpe_int = MMU_STD_INVALIDPTP;

	/*
	 * Validate the last level 1 entries to point to the level 2 tables.
	 */
	for (j = 0, i = NL1PTEPERPT - NKL2PTS; i < NL1PTEPERPT; j++, i++)
		kl1ptr->ptpe[i].ptpe_int = PTPOF(VA2PA(kl2ptr+j));
	/*
	 * Validate the level 2 entries to point to the level 3 tables.
	 * For the level 3 tables mapping the debugger and monitor, we
	 * make the level 2 entries point to the tables already set up
	 * by the monitor. This is necessary so the debugger & monitor
	 * can keep track of where their ptes are.
	 */
	for (i = 0, j = 0; i < NKL2PTS * NL2PTEPERPT; i++, j += NL3PTEPERPT) {
#ifdef	NOPROM
		kl2ptps[i].ptpe_int = PTPOF(VA2PA(kpteptr+j));
#else	NOPROM
		/*
		 * If the rom provides a level-2 page table, use it.
		 * this also picks up root, level-1 and level-2 PTEs,
		 * merging part of the VA into the root and level-1
		 * PTEs so the same mapping occurs.
		 */
		kl2ptps[i].ptpe_int =
			get_rom_l2_ptpe(i * L3PTSIZE + KERNELBASE,
					PTPOF(VA2PA(kpteptr+j)));
#endif	NOPROM
	}
	/*
	 * Invalidate the level 3 entries.
	 */
	for (i = 0; i < NL3PTEPERPT * NKL3PTS; i++)
		kpteptr[i].ptpe_int = MMU_STD_INVALIDPTP;
	/*
	 * Validate the first MAINMEM_MAP_SIZE to map through.
	 * (see sunromvec.h for details).
	 * The default for kernel text is to be cacheable.
	 * Any exceptions should be taken care of here.
	 * Maybe someday we can use short termination to
	 * reduce how hard we hit the TLB.
	 */
	i = mmu_btop(MAINMEM_MAP_SIZE);

	while (i-->0)
		kpteptr[i].ptpe_int = PTEOF(0, i, MMU_STD_SRWX, 1);
	/*
	 * convert the level1 page table ptr into something
	 * that can be used as a context entry value,
	 * and return it for further use.
	 */

	return (PTPOF(VA2PA(kl1ptr)));
}

int	vscb_cacheable = 1;
#ifdef	MULTIPROCESSOR
int	percpu_cacheable = 1;	/* XXX-for debug */
#endif	MULTIPROCESSOR

/*
 * Early startup code that no longer needs to be assembler. Note that the
 * ptes being acted on here are the temporary ones. All changes made here
 * will be propagated to the real ptes when they are initialized.
 */
void
early_startup()
{
#ifdef	NOPROM
	register union ptpe *tmpptes;
#endif	NOPROM
	register int index, i, j;
	extern void mmu_flushall();
#ifndef NOPROM
	extern union ptpe *tmpptes;
	extern void system_setup();
	extern void prom_init();

	prom_init("Kernel");
	map_wellknown_devices();
	fill_machinfo();
	setcputype();
	/*
	 * setup system implementation dependent routines
	 */
	system_setup((int) mach_info.sys_type);

#else	NOPROM

	/*
	 * setup system implementation dependent routines
	 */
	system_setup(CPU_SUN4M_690);

	/*
	 * Map in the nonconfigurable devices to standard places.
	 *
	 * XXX - most of this should go away
	 */
	tmpptes = TMPPTES;

	/*
	 * NVRAM/TOD (EEPROM) Space
	 */
	index = mmu_btop(EEPROM_ADDR - KERNELBASE);
	for (i = 0; i < EEPROM_PGS; i++)
		tmpptes[index++].ptpe_int = PTEOF(0, OBIO_EEPROM_ADDR + i,
			MMU_STD_SRWX, 0);

	/*
	 * Counter/Timer registers
	 */
	index = mmu_btop(COUNTER_ADDR - KERNELBASE);
	for (i = 0; i < COUNTER_PGS; i++)
		tmpptes[index++].ptpe_int = PTEOF(0, OBIO_COUNTER_ADDR + i,
			MMU_STD_SRWX, 0);

/* system counter is really at +0x10, not +0x04 */
	tmpptes[index-1].pte.PhysicalPageNumber = OBIO_COUNTER_ADDR + 0x10;

	/*
	 * Interrupt Registers
	 */
	index = mmu_btop(V_INT_REGS - KERNELBASE);
	for (i = 0; i < INT_REGS_PGS; i++)
		tmpptes[index++].ptpe_int = PTEOF(0, OBIO_INT_REGS + i,
							MMU_STD_SRWX, 0);

/* system interrupt is really at +0x10, not +0x04 */
	tmpptes[index-1].pte.PhysicalPageNumber = OBIO_INT_REGS + 0x10;

	/*
	 * System Control & Status Register
	 */
	index = mmu_btop(V_SYSCTL_REG - KERNELBASE);
	tmpptes[index].ptpe_int = PTEOF(0, OBIO_SYSCTL_REG, MMU_STD_SRWX, 0);

	/*
	 * VME/IOC Control Space
	 */
	index = mmu_btop(V_VMEBUS_ICR - KERNELBASE);
	tmpptes[index].ptpe_int = PTEOF(0, OBIO_VMEBUS_ICR, MMU_STD_SRWX, 0);

	/*
	 * Memory Control Space
	 */
	index = mmu_btop(MEMERR_ADDR - KERNELBASE);
	tmpptes[index].ptpe_int = PTEOF(0, OBIO_MEMERR_ADDR, MMU_STD_SRWX, 0);

	/*
	 * SBus Space
	 */
	index = mmu_btop(V_SBUS_ADDR - KERNELBASE);
	for (i = 0; i < SBUS_PGS; i++)
		tmpptes[index++].ptpe_int = PTEOF(0, OBIO_SBUS_ADDR + i,
						MMU_STD_SRWX, 0);
#endif NOPROM

#ifdef  MULTIPROCESSOR

	/*
	 * we used to reserve NCPU * space for multiprocessor mode,
	 * it waists a lot of space if we only have one or two
	 * modules on the system.  temporarily, we assign only one
	 * CPU (ctx table, PERCPU area, ptes) space if there is only
	 * one cpu on the system.  for system that have more than 1
	 * and less than 4 cpus, we have to find a way to save some
	 * space later.  Note that a system with two viking modules
	 * plugged in, the cpuid will be 8 and a which means cpuid
	 * 0 and 2 (not 0 and 1).
	 */
	if (mach_info.nmods != 1) {
		nmod = NCPU;
	} else {
		uniprocessor = 1;
	}
#endif MULTIPROCESSOR

	/*
	 * Establish per-cpu mappings
	 */
	j = nmod;

	while (j-- > 0) {
#ifdef	MULTIPROCESSOR
		/*
		 * Per-CPU data area only applies to
		 * the multiprocessor case.
		 */
		index = mmu_btop(VA_PERCPU + j * PERCPUSIZE  - KERNELBASE);
		for (i = 0; i < PERCPU_PAGES; i++)
			tmpptes[index++].ptpe_int =
				PTEOF(0, va2pp((addr_t)cpu0) +
					(j * PERCPU_PAGES) + i,
					MMU_STD_SRWX, percpu_cacheable);
		/* XXX- for debug make PERCPU area cacheability tunable */
#endif	MULTIPROCESSOR
		/*
		 * Counter/Timer registers for each possible MID
		 * Make 'em readable by the user for fast profiling
		 * we want permissions to be kernel rw, user ro;
		 * it should be safe to let the user monkey with
		 * the data fields. kernel MUST be able to write.
		 */
		index = mmu_btop(VA_PERCPU + PC_utimers +
				    (j << PERCPU_SHIFT) - KERNELBASE);
		tmpptes[index] =
			tmpptes[mmu_btop(V_COUNTER_ADDR - KERNELBASE) + j];

/* %%% fun hack %%% make the user timers readable by the user! */
		tmpptes[index].pte.AccessPermissions = MMU_STD_SRWUR;

		/*
		 * Interrupt registers for each possible MID
		 */
		index = mmu_btop(VA_PERCPU + PC_intregs +
				    (j << PERCPU_SHIFT) - KERNELBASE);
		tmpptes[index] =
			tmpptes[mmu_btop(V_INTERRUPT_ADDR - KERNELBASE) + j];

#ifdef	PC_vscb
		index = mmu_btop(VA_PERCPU + PC_vscb +
				    (j << PERCPU_SHIFT) - KERNELBASE);
		tmpptes[index].ptpe_int =
			PTEOF(0, va2pp((addr_t)&scb), MMU_STD_SRWX,
				vscb_cacheable);
#endif	PC_vscb
	}

	/*
	 * Establish my-cpu mappings
	 * by duplicating the right per-cpu area.
	 */
	i = mmu_btop(VA_PERCPU - KERNELBASE);
	index = mmu_btop(VA_PERCPUME - KERNELBASE);
	j = PERCPU_MAPS;		/* number of mappings to copy */
	while (j-- > 0)
		tmpptes[index++] = tmpptes[i++];
	/*
	 * Conservatively wipe out the ATC in case the above mappings were
	 * somehow in there.
	 */
	mmu_flushall();

#ifdef	MULTIPROCESSOR
	/*
	 * initialize U-area pointer.	MULTIPROCESSOR kernels put
	 * this variable in the percpu area, so we can't staticly
	 * initialize it.
	 */
	uunix = &idleuarea;
	/*
	 * Initialize bcopy HW lock to "locked".
	 * Needed to minimize checks done in bcopy routine
	 * and to only try to use when we should.
	 * Can't initialize PERCPU variable to non-zero
	 * value, and need to lock until determine we have HW.
	 */
	bcopy_res = -1;
#endif	MULTIPROCESSOR
	/*
	 * Zero out the u page, and the bss. NOTE - the scb part
	 * makes assumptions about its layout with the msgbuf.
	 */
	bzero((caddr_t)uunix, sizeof (*uunix));
	bzero(edata, (u_int)(end - edata));
}

/* Flag for testing directed interrupts */
int	test_dirint = 0;

extern struct dev_reg	obpctx; /* obp's context table info, reg struc */

int debug_msg = 0;      /* variable to patch to get debugging messages */

/*
 * Print out ENABLED and DISABLED debugging messages
 */
void
print_debug_msg()
{
#ifdef IOC
        if (ioc) {
                if (use_ioc)
                        printf("IOC ENABLED\n");
                else
                        printf("IOC DISABLED\n");
        }
#endif  IOC

#ifndef MULTIPROCESSOR
        if (bcopy_buf) {
                if (use_bcopy)
                        printf("BCOPY BUFFER ENABLED!\n");
                else
                        printf("BCOPY BUFFER DISABLED!\n");
        }
#else
        if (bcopy_buf) {
                if (use_bcopy)
                        printf("BCOPY BUFFERS ENABLED!\n");
                else
                        printf("BCOPY BUFFERS DISABLED!\n");
        }
#endif  !MULTIPROCESSOR
 
#ifdef VAC
        if (!vac) {
                if (use_cache) {
                        if (use_ic)
                                printf("          Instruction Cache ENABLED\n");
			else
				printf("          Instruction Cache DISABLED\n");
                        if (use_dc)
                                printf("          Data Cache ENABLED\n");
			else
				printf("          Data Cache DISABLED\n");
                        if (use_mix) /* superscalar lives in breakpoint reg */
                                printf("          Superscalar ENABLED\n");
			else
				printf("          Superscalar DISABLED\n");
                }
                if (use_vik_prefetch)
                        printf("          Viking Data Prefetcher ENABLED\n");
		else
			printf("          Viking Data Prefetcher DISABLED\n");	
                if (use_store_buffer)
                        printf("          Store Buffer ENABLED\n");
		else
			printf("          Store Buffer DISABLED\n");
                if (cache == CACHE_PAC_E) {
                        if (use_ec) {
                                if (use_table_walk) {
                                        printf("          Table Walk ");
                                        printf("Cacheable ENABLED\n");
                                }
				else {
					printf("          Table Walk ");
					printf("Cacheable DISABLED\n");
				}
					
                                printf("          External Cache ENABLED\n");
                        }
			else
				printf("          External Cache DISABLED\n");
                        if (use_mxcc_prefetch)
                                printf("          MXCC Prefetch ENABLED\n");
			else
				printf("          MXCC Prefetch DISABLED\n");
                        if (use_multiple_cmd)
                                printf("          Multiple Command ENABLED\n");
			else
				printf("          Multiple Command DISABLED\n");
                }
        }
#endif VAC
}

/*
 * Machine-dependent startup code
 */
startup()
{
	register int unixsize;
#if	!defined(SAS) && defined(VMEFIXED)
	register int dvmapage;
#endif
	register unsigned i, j;
	register caddr_t v;
	u_int firstpage;		/* first free physical page number */
	u_int mapaddr, npages, page_base;
	struct page *pp;
	struct segmap_crargs a;
	struct memseg *memseg_base;
	extern void v_handler();
	extern void hat_setup_kas();
	extern char etext[], end[], Syslimit[];
	extern trapvec kclock14_vec;
	extern struct scb scb;
#ifdef NOPROM
	extern char *cpu_str;
	extern char *mod_str;
#endif
#ifdef VAC
	extern void cache_on();
#endif
	extern void mmu_flushall();

	int dbug_mem, memblocks;
	struct memlist *pmem, *lastpmem;
	struct memseg *memsegp;
	u_int first_page, num_pages, index;
	u_int bytes_used, bytes_avail;
	struct ptbl *ptbl;
	struct pte *pte;
	union ptpe *kptp, *kptp_min, *kptp_max;
	union ptpe *kptp1;
	struct modinfo *modinfop;
	int minimum_npts;
#ifdef	NOPROM
	union ptpe *tmpptes;

	tmpptes = TMPPTES;
#else	NOPROM
	extern union ptpe *tmpptes;
#endif	NOPROM
	noproc = 1;

	if (test_dirint)
		send_dirint(0, 6);
/*
 * The following statements setup various flags to indicate the presence
 * of a particular hardware feature on the machine which we are running on.
 * Traditionally this information has been hardcoded based on the cpu types
 * for which the kernel was configured. Now much of the configuration
 * information is supplied by the PROM.
 */

/* Old method */
#ifdef NOPROM
	cpuinfop = &cpuinfo[(cpu & CPU_MACH) - 1];
	setcpudelay();
	dvmasize = cpuinfop->cpui_dvmasize;

#ifdef	VAC
	vac = cpuinfop->cpui_vac;
#endif

#ifdef IOMMU
	iom = cpuinfop->cpui_iom;
#endif

#ifdef IOC
	ioc = cpuinfop->cpui_ioctype;
#endif	IOC

	bcopy_buf = cpuinfop->cpui_bcopy_buf;

#ifdef VAC
	if (vac) {
		vac_linesize = cpuinfop->cpui_linesize;
		vac_nlines = cpuinfop->cpui_nlines;
		vac_pglines = PAGESIZE / vac_linesize;
		vac_size = vac_nlines * vac_linesize;
	}
#endif /* VAC */

/* New method */
#else /* NOPROM */
	cpu = mach_info.sys_type;
	modinfop = &mod_info[0];
	setcpudelay();
#ifdef VAC
	if (modinfop->ncaches) {
		if (modinfop->phys_cache)
			if (modinfop->eclinesize)
				cache = CACHE_PAC_E;
			else
				cache = CACHE_PAC;
		else {
			cache = CACHE_VAC;
			vac = 1;
		}
	}
#endif

#ifdef IOMMU
	iom = mach_info.iommu;
#endif

#ifdef IOC
	ioc = vme_info.iocache;
#endif

	/*
	 * Dont need this to be percpu for MP since all the
	 * modules should be the same.  So if one is capable
	 * of HW bcopy, all are.
	 */
	bcopy_buf = modinfop->bcopy;

	if (nctxs == 0) {
		nctxs = modinfop->mmu_nctx;

		if (nctxs > MAX_NCTXS)
			nctxs = MAX_NCTXS;
	}

	nl1pts = nproc + 10;

	if (nswctxs == 0)
		nswctxs = nproc + 10;

	if (nswctxs > nctxs)
		nswctxs = nctxs;

#ifdef VAC
	if (vac) {
		vac_linesize = modinfop->clinesize;
		vac_nlines = modinfop->cnlines;
		vac_pglines = PAGESIZE / vac_linesize;
		vac_size = vac_nlines * vac_linesize;
	}
#endif /* VAC */

#endif /* NOPROM */

#ifdef IOC
	if (ioc) {
		if (use_ioc) {
			ioc_init();
			iocache_on();
			iocenable = 1;
		} else {
			iocenable = 0;
			ioc = 0;
		}
	}
#endif	IOC

#ifndef	MULTIPROCESSOR
	if (bcopy_buf) {
		if (use_bcopy) {
			bcopy_res = 0;		/* allow use of hardware */
		} else {
			bcopy_res = -1;		/* reserve now, hold forever */
		}
	}
#else
	if (bcopy_buf) {
		if (use_bcopy) {
			for (i = 0; i < nmod; ++i)
				PerCPU[i].bcopy_res = 0;
		} else {
			for (i = 0; i < nmod; ++i)
				PerCPU[i].bcopy_res = -1;
		}
	} else {
		for (i = 0; i < nmod; ++i)
			PerCPU[i].bcopy_res = -1;
	}
#endif	!MULTIPROCESSOR

	/*
	 * Save the kernel's level 14 interrupt vector code and install
	 * the monitor's. This lets the monitor run the console until we
	 * take it over.
	 */

	init_mon_clock();

	(void) splzs();			/* allow hi clock ints but not zs */

	bootflags();			/* get the boot options */

	/*
	 * If the boot flags say that the debugger is there,
	 * test and see if it really is by peeking at DVEC.
	 * If is isn't, we turn off the RB_DEBUG flag else
	 * we call the debugger scbsync() routine.
	 */
#ifndef NOPROM
	if ((boothowto & RB_DEBUG) != 0) {
		if (dvec == NULL || peek((short *)dvec) == -1)
			boothowto &= ~RB_DEBUG;
		else {
			extern trapvec kadb_tcode, trap_ff_tcode,
				trap_fe_tcode;

			(*dvec->dv_scbsync)();
			/*
			 * Now steal back the traps.
			 */
			kadb_tcode = scb.user_trap[TRAPBRKNO];
			scb.user_trap[TRAPBRKNO-1] = trap_fe_tcode;
			scb.user_trap[TRAPBRKNO] = trap_fe_tcode;
		}
	}
#endif NOPROM

#ifdef NOPROM
	/* SAS has contigouous memory */
	npages = physmem = btop(availmem);
	physmax = physmem - 1;
	memblocks = 0; /* set this to zero so the valloc below ends up w/ 1 */
#else
	/*
	 * v_vector_cmd is the handler for the monitor vector command .
	 * We install v_handler() there for Unix.
	 */
	if (prom_sethandler((voidfunc_t) 0, v_handler) != 0)
		panic("No handler for PROM?");

	/*
	 * Dont need OBP context table anymore.
	 * Keep the first page of context table for OBP to start other CPUs.
	 * Put them back to the availmemory pool
	 * The availmemory pool was preassigned to a 4K page size
	 * We only search for the case that ctx table space merged with
	 * the higher address.  It doesn't care about if the space
	 * can be merged into the end of a contiguous space because
	 * it will never happen (we reserved the first 4K of the space for 
	 * OBP)
	 */
	for (i = 0, pmem = availmemory; (pmem[i].next && obpctx.reg_size); i++) {
		if (pmem[i].address == 
			(u_int) (obpctx.reg_addr + obpctx.reg_size)) {
			pmem[i].address = (u_int)obpctx.reg_addr + PAGESIZE;
			pmem[i].size += (obpctx.reg_size - PAGESIZE);
			physmem += mmu_btop(pmem[i].size);
			obpctx.reg_size = 0;
		}
	}
	if (obpctx.reg_size) {
		pmem[i].next = &pmem[i+1];
		pmem[i+1].next = 0;
		for (; pmem[i].address > (u_int)obpctx.reg_addr; i--) {
			pmem[i+1].address = pmem[i].address;
			pmem[i+1].size = pmem[i].size;
		}
		pmem[++i].address = (u_int)obpctx.reg_addr + PAGESIZE;
		pmem[i].size = obpctx.reg_size - PAGESIZE;
		physmem += mmu_btop(pmem[i].size);
	}

	/*
	 * Add up how much physical memory the prom has passed us.
	 * Remember what the physically available highest page is
	 * so that dumpsys works properly.
	 */
	memblocks = 0;
	for (pmem = availmemory; pmem; pmem = pmem->next) {

		memblocks++;
		/*
		 * Remember the last pmem segment so that we can
		 * shove our IOPBMAP just below the debugger.
		 * N.B.: This assumes that the debugger (and maybe the
		 * monitor) will leave enough room in this physical memory
		 * segment for the pages that IOPBMAP requires.
		 *
		 */
		if (pmem->next == (struct memlist *) NULL) {
			lastpmem = pmem;
			physmax = mmu_btop(pmem->size + pmem->address);
		}
	}
	/*
	 * Now add up how much physical memory is really present (so
	 * dumpsys will work).
	 */
	npages = 0;
	for (pmem = availmemory; pmem; pmem = pmem->next) {
		npages += mmu_btop(pmem->size);
		if (pmem->next == NULL)
			physmax = mmu_btop(pmem->size + pmem->address) - 1;
	}

	/*
	 * If debugger is in memory, note the pages it stole from physmem.
	 */
	dbug_mem = (boothowto & RB_DEBUG) ? *dvec->dv_pages : 0;

#endif NOPROM
	/*
	 * maxmem is the amount of physical memory we're playing with,
	 * less the amount we'll give to the IOPBMAP
	 */
	maxmem = physmem - IOPBMEM;

	/*
	 * Why is this here?
	 */
	if (nrnode == 0)
		nrnode = ncsize << 1;

	/*
	 * Allocate space for system data structures.
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * It is used when remapping the tables later.
	 */
	/*
	 * First the machine dependent, Hardware Address Translation (hat)
	 * data structures are allocated. We depend on the fact that the
	 * Monitor maps in more memory than required. We do this first so
	 * that the remaining data structures can be allocated from the
	 * page pool in the usual way.
	 */

	/* the 2 in statement below is due to 4k page size */
	firstpage = mmu_btopr((int)end - KERNELBASE) + (UPAGES - 2);

	vvm = vstart = v = (caddr_t)(mmu_ptob(firstpage) + KERNELBASE);
	mapaddr = btoc((int)vvm - KERNELBASE);

#define	valloc(name, type, num) \
	(name) = (type *)(v); \
	econtig = (v) = (caddr_t)((name)+(num));

#define	valloclim(name, type, num, lim) \
	(name) = (type *)(v); \
	econtig = (v) = (caddr_t)((lim) = ((name)+(num)));

#ifdef DLYPRF
	valloc(dpring, struct dpr, dpsize);
#endif

	v = (addr_t)roundup((u_int)v, PAGESIZE);
	kern_nc_top_va = v;
	kern_nc_top_pp = va2pp(v);

#ifdef IOMMU

	if (iom) {
		/* definition: align iopte tbl to 'iopte table size' boundary */
		v = (addr_t)roundup((u_int)v, IOMMU_PTE_TBL_SIZE);
		valloclim(ioptes, union iommu_pte, IOMMU_N_PTES, eioptes);
		phys_iopte = (union iommu_pte *) CA2PA(ioptes);
	}
#endif

	/*
	 * NOTE: context table has to be aligned at
	 * its size boundary which differs for different modules.
	 *
	 * XXX-context table size should be (nctxs * sizeof(union ptpe))
	 * since roundup operates on u_ints.
	 */
	v = (addr_t)roundup((u_int)v, (nctxs * sizeof (union ptpe)));
	if ((cache == CACHE_PAC_E) && (use_cache)) {
		kern_nc_end_va = v;
		kern_nc_end_pp = va2pp(v);
	}

	valloclim(contexts, union ptpe, nmod*nctxs, econtexts);
	ctxtabptr = (struct ctx_table *) CA2PA(contexts);

	/*
	 * Calculate how many page tables we will have.
	 * The algorithm is to have enough ptes to map all
	 * of memory 16 times over (due to fragmentation). We
	 * add in the size of the I/O Mapper, since it is always there.
	 * We also add in enough tables to map the kernel text/data once.
	 * There is an upper limit, however, since we use indices
	 * in page table lists.
	 */

	/* XXX - patch as required */
	if (npts == 0) {
		npts = 64;
		npts += (firstpage + NL3PTEPERPT - 1) / NL3PTEPERPT;
		npts += (maxmem - firstpage + NL3PTEPERPT - 1) * 16 /
			NL3PTEPERPT;

		minimum_npts = MINPTS(minpts);
		if (npts < minimum_npts)
			npts = minimum_npts;

		if (npts > MAX_PTBLS)
			npts = MAX_PTBLS;
	}

	/*
	 * Allocate the ptbl structs, and the page tables.
	 * Note that page tables have a 256 byte alignment
	 * requirement.
	 */

	/* level 2 and 3 page tables need to be on 256 bytes boundary */
	v = (caddr_t) roundup((u_int)v, sizeof (struct l3pt));
	valloclim(ptes, struct pte, npts * NL3PTEPERPT, eptes);
	pts_addr = CA2PA(ptes);

	v = (addr_t)roundup((u_int)v, PAGESIZE);

	valloclim(l1pts, struct l1pt, nswctxs, el1pts);

	v = (addr_t)roundup((u_int)v, PAGESIZE);
	if ((cache != CACHE_PAC_E) || (!use_cache)) {
		kern_nc_end_va = v;
		kern_nc_end_pp = va2pp(v);
	}

	/* allocate software ctx, ptbl, pte after real h/w ones */
	valloclim(ctxs, struct ctx, nctxs, ectxs);
	valloclim(ptbls, struct ptbl, npts, eptbls);
	valloclim(sptes, struct spte, npts * NL3PTEPERPT, esptes);

	v = (caddr_t)roundup((u_int)v, PAGESIZE);

	/*
	 * Vallocate the remaining kernel data structures.
	 * "mapaddr" is the real memory address where the tables start.
	 * It is used when remapping the tables later.
	 */

#ifdef	UFS
#ifdef	QUOTA
	valloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
#endif	QUOTA
#endif	UFS

	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloc(cfree, struct cblock, nclist);
	valloc(callout, struct callout, ncallout);
	valloc(kernelmap, struct map, MIN(4 * nproc, SYSPTSIZE/2));
	valloc(bufmap, struct map, BUFPAGES/2);
	valloc(heapmap, struct map, HEAPPAGES/2);
	valloc(iopbmap, struct map, IOPBMAPSIZE);

#ifdef IOMMU
	if (iom) {
		if (cpu == CPU_SUN4M_690) {
			valloc(vme24map, struct map, NDVMAMAPS(VME24MAP_SIZE));
			valloc(vme32map, struct map, NDVMAMAPS(VME32MAP_SIZE));
		} else {
			valloc(altsbusmap, struct map,
				NDVMAMAPS(ALTSBUSMAP_SIZE));
		}
		valloc(sbusmap, struct map, NDVMAMAPS(SBUSMAP_SIZE));
		valloc(bigsbusmap, struct map, NDVMAMAPS(BIGSBUSMAP_SIZE));
		valloc(mbutlmap, struct map, NDVMAMAPS(MBUTLMAP_SIZE));
	} else {
		valloc(dvmamap, struct map, NDVMAMAPS(dvmasize));
	}

#else IOMMU
	valloc(dvmamap, struct map, NDVMAMAPS(dvmasize));
#endif IOMMU

	valloc(ncache, struct ncache, ncsize);

/* define macro to round up to integer size */
#define	INTSZ(X)	howmany((X), sizeof (int))
/* define macro to round up to nearest int boundary */
#define	INTRND(X)	roundup((X), sizeof (int))

#ifdef	IPCSEMAPHORE
	valloc(sem, struct sem, seminfo.semmns);
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(semmap, struct map, seminfo.semmap);
	valloc(sem_undo, struct sem_undo *, nproc);
	valloc(semu, int, INTSZ(seminfo.semusz * seminfo.semmnu));
#endif	IPCSEMAPHORE

#ifdef	IPCMESSAGE
	valloc(msg, char, INTRND(msginfo.msgseg * msginfo.msgssz));
	valloc(msgmap, struct map, msginfo.msgmap);
	valloc(msgh, struct msg, msginfo.msgtql);
	valloc(msgque, struct msqid_ds, msginfo.msgmni);
#endif	IPCMESSAGE

#ifdef	IPCSHMEM
	valloc(shmem, struct shmid_ds, shminfo.shmmni);
#endif	IPCSHMEM

	/*
	 * Allocate memory segment structures to describe physical memory.
	 * Allocate one more than the number of actual memory blocks we've
	 * been told about by the monitor, because we want to reclaim
	 * physical pages 0 and 1.
	 *
	 */
	valloc(memseg_base, struct memseg, memblocks + 1);

	/*
	 * Allocate space for page structures and hash table.
	 * Get enough for all of physical memory minus amount
	 * dedicated to the system minus amount used for IOPBMAP.
	 *
	 * Leaving off the pages for the IOPBMAP is necessary so
	 * those pages can't ever be made be cacheable.
	 *
	 */

	/*
	 * use maxmem here to *not* back IOPBMEM with page structures
	 */

	bytes_used = (u_int)v - KERNELBASE;
	bytes_avail = mmu_ptob(maxmem) - bytes_used;
	npages = bytes_avail / (PAGESIZE + sizeof (struct page));
	page_base = maxmem - npages;

	valloc(pp, struct page, npages);

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

	econtig = (addr_t)roundup((u_int)v, PAGESIZE);
	/* panic if valloc'd over tmpptes */
	if ((unsigned)econtig > (unsigned)tmpptes) {
		panic("tmpptes too low");
	}
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
	 * Non-recoverable conditions.  Need to check for
	 * this situation before checking eecontig below.
	 * Else may suspect the wrong problem.
	 */

	if ((u_int)v > HEAPBASE)
		halt("out of virtual memory");
	unixsize = mmu_btopr(v - KERNELBASE);
	if (unixsize >= physmem - 8 * UPAGES)
		halt("out of physical memory");

	/*
	 * Need to check for the problem of nproc being to large here
	 * because we could step on tmpptes and die ungracefully later
	 * during hat_ptecopy().
	 * This check obsoletes the check later in startup just before
	 * the segu segment is actually created.
	 * XXX--Can get more space for segu by relocating VA_TMPPTES to
	 *	just below HEAPBASE (8meg of kvm can be recovered).
	 *	Too late for integration of that fix.
	 * XXX--Also should recalculate nproc if eecontig too large.
	 *	Then print message so user knows about the adjustment.
	 */
	if (eecontig >= (addr_t)(roundup((u_int)tmpptes,
				(u_int)L3PTSIZE)-L3PTSIZE))
		panic("insufficient kvm for segu: nproc(MAXUSERS) too big?");

	/*
	 * Set up the kernel text mappings, again.
	 * %%% maybe we can actually use a L1 PTE
	 * do map in kernel text, data, bss?
	 */
	for (i = 0; i < firstpage; i++)
		tmpptes[i].ptpe_int = PTEOF(0, i, MMU_STD_SRWX, 1);
	/*
	 * Map in the valloc'd structures.
	 * Note that the vac has not been turned on; we are just
	 * setting the cachaeble bit right now, so that when the
	 * vac is later turned on, these data structures will be
	 * cacheable.
	 */
	j = mmu_btop(CA2PA(econtig-1));
	for (i = firstpage; i <= j; i++) {
#ifdef	VAC
		if (cache && use_cache &&
		    ((i < kern_nc_top_pp) || (i >= kern_nc_end_pp)))
			tmpptes[i].ptpe_int = PTEOF(0, i,  MMU_STD_SRWX, 1);
		else
#endif	VAC
			tmpptes[i].ptpe_int = PTEOF(0, i,  MMU_STD_SRWX, 0);
	}
	/* 
	 * unmap the rest of the pages in the L3 past eecontig
	 * Else they remain mapped in forever.
	 */
	i = mmu_btop(CA2PA(eecontig-1)) + 1;
	j = mmu_btop(CA2PA((addr_t)((u_int)eecontig | (L3PTSIZE - 1))));
	for (; i <= j ; i ++) 
	     tmpptes[i].ptpe_int = 0;

	mmu_flushall();

#ifndef SAS_FAST
	bzero((caddr_t)vvm, (u_int)(econtig - vvm));
#else SAS_FAST
	/* Hardware simulator has already cleared memory */
	bzero((caddr_t)vvm, 20);
#endif SAS_FAST
	/*
	 * Initialize VM system, and map kernel address space.
	 */
	hat_init();
	/* Fun begins, just as in pegasus */
	for (i = 0 - L3PTSIZE,
		kptp = &kl2pts[NKL2PTS - 1].ptpe[NL2PTEPERPT - 1];
		/* Remember, we want to go below the lowest address */
		i >= HEAPBASE;
		i -= L3PTSIZE, kptp--)
	{
#ifndef NOPROM
		if (avail_at_vaddr(i) < L3PTSIZE)
			continue;
#endif
		ptbl = hat_ptblreserve((addr_t)i, 3);
		pte = ptbltopte(ptbl);

		hat_ptecopy(&(tmpptes[mmu_btop(i - KERNELBASE)].pte), pte);
		kptp->ptp.PageTablePointer = ptetota(pte);
		kptp->ptp.EntryType = MMU_ET_PTP;
		mmu_flushall();
	}
	kptp_max = kptp;
	for (i = KERNELBASE, kptp = &kl2pts[0].ptpe[0];
		i < (u_int)eecontig;
		i += L3PTSIZE, kptp++)
	{
		ptbl = hat_ptblreserve((addr_t)i, 3);
		pte = ptbltopte(ptbl);
		hat_ptecopy(&(tmpptes[mmu_btop(i - KERNELBASE)].pte), pte);
		kptp->ptp.PageTablePointer = ptetota(pte);
		kptp->ptp.EntryType = MMU_ET_PTP;
		mmu_flushall();
	}
	for (j = 0; j < nmod; j++) {
		i = VA_PERCPU + (j * PERCPUSIZE);
		kptp = &kl2pts[(i - KERNELBASE) >> 24].ptpe[(j * PERCPUSIZE)
									>> 18];
		ptbl = hat_ptblreserve((addr_t)i, 3);
		pte = ptbltopte(ptbl);
		hat_ptecopy(&(tmpptes[mmu_btop(i - KERNELBASE)].pte), pte);
		kptp->ptp.PageTablePointer = ptetota(pte);
		kptp->ptp.EntryType = MMU_ET_PTP;

		mmu_flushall();
	}
	/*
	 * Map our PerCPU area down to PerCPU_me
	 */
	kptp = kl2pts[(VA_PERCPUME - KERNELBASE) >> 24].ptpe;
	kptp1 = kl2pts[(VA_PERCPU - KERNELBASE) >> 24].ptpe;
	kptp->ptp =  kptp1->ptp;

	mmu_flushall();

	kptp_min = kptp;
	/*
	 * Next we need to copy the level 2 page tables into our array.
	 * We need to make sure that all the entries we didn't fill in
	 * above get marked invalid. Also, we may leave out
	 * some of the tables, since there is a hole in the address space.
	 */

	for (kptp = kptp_min; kptp <= kptp_max; kptp++) {
		if (kptp->ptp.EntryType != MMU_ET_PTP)
			kptp->ptp.EntryType = MMU_ET_INVALID;

		mmu_flushall();
	}
	/*
	 * Walk through the level 2 tables, except the last one,
	 * copying ones with real pages.
	 */

	for (i = 0; i < NKL2PTS; i++) {
		kptp = &kl1pt->ptpe[NL1PTEPERPT - NKL2PTS + i];
		bytes_used = KERNELBASE + i * L2PTSIZE;
		/*
		 * There is a hole between eecontig and V_WKBASE_ADDR,
		 * which is the last piece of the kernel address.
		 * We invalidate the hole, but map in the rest.
		 */
		if (bytes_used >= (u_int)eecontig &&
		    bytes_used < (V_WKBASE_ADDR & ~(L2PTSIZE-1)) &&
		    bytes_used != ((u_int)tmpptes & ~(L2PTSIZE-1)) &&
		    bytes_used != VA_PERCPUME &&
		    bytes_used != VA_PERCPU &&
		    !(bytes_used >= (HEAPBASE & ~(L2PTSIZE-1)) &&
		    bytes_used < roundup(BUFLIMIT, L2PTSIZE))) {
			kptp->ptp.EntryType = MMU_ET_INVALID;
			continue;
		}
		ptbl = hat_ptblreserve((addr_t)bytes_used, 2);
		pte = ptbltopte(ptbl);
		hat_ptecopy((struct pte *)&kl2pts[i].ptpe[0], pte);
		kptp->ptp.PageTablePointer = ptetota(pte);
		kptp->ptp.EntryType = MMU_ET_PTP;
#ifdef	MULTIPROCESSOR
		/*
		 * keep track of the level-2 page table for the
		 * common per-CPU area, so that vm_hat.c can correctly
		 * initialize the percpu-area for all the CPUs.
		 */
		if (bytes_used == VA_PERCPU)
			percpu_ptbl = ptbl;

		/*
		 * keep track of the page table pointer to CPU 0's
		 * level-2 page table for its per-CPU area, so that
		 * hat_map_percpu can map in the appropriate CPU's
		 * per-CPU area when a process migrates to a different
		 * CPU.	 presumably, we are bringing up CPU 0 here!
		 * this same record keeping is also done in vm_hat.c
		 * (hat_percpu_setup) for the other CPUs.
		 */
		if (bytes_used == VA_PERCPUME)
			percpu_ptpe[0].ptp = kptp->ptp;
#endif	MULTIPROCESSOR

		mmu_flushall();
	}

	/* we just copied level2 entries for 256K worht of space for 
	 * eecontig. Which means we copied extra stuff that points to
	 * l3 maps in PA_TMPPTES. This is bad. unmap it right now
	 * Make sure all maps point to within our ptes-eptes range
	 * or they are unmapped at a level within the kernels l1 and
	 * pte-eptes range.
	 * Else bad programs can jump here and cause translation errors
	 * and panic. We need to avoid panics.
	 */
	
	unmap_to_end_rgn((((u_int)eecontig + L3PTSIZE) & ~(L3PTSIZE-1)));
    
	mmu_flushall();

	/*
	 * Finish setting up the kernel address space,
	 * and activate it.
	 */
	hat_setup_kas();
	/*
	 * Dont need tmpptes anymore, so lets free the pages.
	 * Prom will think it has a hole since this wont cause
	 * it to free the pages it used for pte's for our mapping
	 * but we wont see this as a hole.
	 * %%% Need a promlib call from unmapping.
	 */
	(void) OBP_V2_UNMAP((caddr_t)tmpptes, TMPPTES);

       /*
	* We have copied the L1 for the tmppte space from our old mapping.
	* After that we unmapped the tmpptes mapping by doing OBP_V2_UNMAP.
	* At this time we have L1-L2 mapping  pointing to some physical pages 
	* at PA_TMPPTES that belongs to us in our freelist. Therefore depending
	* on  the use of these pages (PA_TMPPTES to PA_TMPPTES+TMPPTES), the
	* mappings may be transient.

        */

	i = ((u_int)tmpptes & ~(L2PTSIZE-1))/L2PTSIZE;
          
	kl1pt->ptpe[i].ptpe_int  = 0;

	/*
	 * Initialize VM system, and map kernel address space.
	 */
	kvm_init();
/* officially start the monitor rom clock */
	start_mon_clock();

#if	!defined(SAS) && defined(VMEFIXED)
	/* FIXME: these might be wrong, we'll know how VME works later */
	disable_dvma();
	for (dvmapage = 0; dvmapage < btoc(DVMASIZE); dvmapage++) {
		segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
		    PROT_READ | PROT_WRITE,
			(u_int)(dvmapage + btop(VME24D16_BASE)), 0);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
		segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
		    PROT_READ | PROT_WRITE,
			(u_int)(dvmapage + btop(VME24D32_BASE)), 0);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
		segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
		    PROT_READ | PROT_WRITE,
			(u_int)(dvmapage + btop(VME32D32_BASE)), 0);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
	}

	if (dvmapage < btoc(DVMASIZE)) {
		printf("CAN'T HAVE PERIPHERALS IN RANGE 0 - %dKB\n",
		    DVMASIZE / 1024);
		halt("dvma collision");
	}
	enable_dvma();
#endif

#ifdef	TLBLOCK_ATOMS
	atom_tlblock();
#endif	TLBLOCK_ATOMS

#ifdef VAC
	if (vac) {
		if (use_cache) {
			if (boothowto & RB_WRITETHRU)
				vac_copyback = 0;
			else if (boothowto & RB_COPYBACK)
				vac_copyback = 1;
			else if (boothowto & RB_ASKMORE) {
				char	cmi[64];
				printf("Cache mode (WriteThru, CopyBack)? ");
				gets(cmi);
				if ((cmi[0] == 'w') || (cmi[0] == 'W'))
					vac_copyback = 0;
				if ((cmi[0] == 'c') || (cmi[0] == 'C'))
					vac_copyback = 1;
			}
			vac_mode = (char *)0;
			vac_init();
			cache_on();
			setcpudelay();
			shm_alignment = vac_size;
                        if (vac_mode && vac_mode[0])
                                printf("VAC ENABLED in %s mode\n", vac_mode);          
                        else
                                printf("VAC ENABLED\n");
		} else	{
			cache = 0;		/* indicate cache is off */
			vac = 0;		/* indicate cache is off */
			setcpudelay();		/* overkill? */
			shm_alignment = PAGESIZE;
			printf("VAC DISABLED\n");
		}
	} else {
		if (use_cache) {
/*
 * FIXME: this is only for bringup debugging purpose.  They can be removed once
 * we finish up the debugging since PROM will do the init and cache-on.
 */
			extern void bpt_reg();

			vac_init();	/*
					 * this routine will also turn on cahces
					 * if necessary
					 */
			setcpudelay();
                        if (vac_mode && vac_mode[0])
                                printf("%s: PAC ENABLED\n", vac_mode);
                        else
                                printf("PAC ENABLED\n");
			if (use_mix) {
				/* superscalar lives in breakpoint reg */
				bpt_reg(MBAR_MIX, 0);
			} else { /* don't want superscalar, shut it off */
				bpt_reg(0, MBAR_MIX);
			}
		} else {
			vac = 0;		/* indicate cache is off */
			cache = 0;		/* indicate cache is off */
		        if (vac_mode && vac_mode[0])
		                printf("%s: PAC DISABLED\n", vac_mode);
		        else
		                printf("PAC DISABLED\n");
		}
		shm_alignment = PAGESIZE;
	}
#endif VAC

	if (debug_msg)  /* Print out debugging messages. */
		print_debug_msg();

#ifdef	MULTIPROCESSOR
	for (i = 0; i < nmod; ++i)
		PerCPU[i].cpuid = i;
#endif	MULTIPROCESSOR
	/*
	 * miscellaneous asynchronous fault initialization.
	 */
	for (i = 0; i < nmod; ++i) {
#ifdef	MULTIPROCESSOR
		atom_init(PerCPU[i].aflt_sync);
		PerCPU[i].a_head = MAX_AFLTS - 1;
		PerCPU[i].a_tail = MAX_AFLTS - 1;
	}
	atom_init(mm_flag);
#else	MULTIPROCESSOR
		a_head[i] = MAX_AFLTS - 1;
		a_tail[i] = MAX_AFLTS - 1;
	}
#endif	MULTIPROCESSOR
	atom_init(system_fatal);
	atom_init(module_error);

	/*
	 * Good {morning, afternoon, evening, night}.
	 * When printing memory, show the total as physmem less
	 * that stolen by a debugger.
	 */
	printf(version);
#ifdef NOPROM
	if (cpu_str != (char *)0) printf("cpu = %s\n", cpu_str);
	if (mod_str != (char *)0) printf("mod = %s\n", mod_str);
#else
	printf("cpu = %s\n", mach_info.sys_name);
	for (i = 0, modinfop = &mod_info[0]; i < mach_info.nmods;
		++i, ++modinfop)
		if (modinfop->mod_name)
			printf("mod%d = %s (mid = %d)\n", i,
				modinfop->mod_name,
				modinfop->mid);
#endif

#ifdef NOPROM
	printf("mem = %dK (0x%x)\n", availmem>>10, availmem);
#else
	printf("mem = %dK (0x%x)\n", calc_memsize()>>10, calc_memsize());
#endif NOPROM

	if (Syslimit + NCARGS + MINMAPSIZE > (addr_t)DEBUGSTART)
		halt("system map tables too large");

	/*
	 * Now we are using mapping for rdwr type routines and buffers
	 * are needed only for file system control information
	 * (i.e. cylinder groups, inodes, etc).
	 *
	 * nbuf is the maximum number of MAXBSIZE buffers that
	 * can be in the cache if it (dynamically) grows to that size.
	 * It should never be less than 2 since deadlock may result.
	 * We check for <2 since some people automatically patch
	 * their kernels to have nbuf=1 before dynamic control buffer
	 * allocation was implemented.
	 *
	 * Currently, we calculate nbuf as a percentage of maxmem.
	 * with MINBUF and MAXBUF as lower and upper bounds.
	 * Note this sets the cache size according to the system's ability
	 * to provide a cache -- not the systems need for a cache --
	 * which will depend on I/O load.
	 */

	if (nbuf < 2){
		nbuf = MAX(((maxmem * NBPG) / BUFPERCENT / MAXBSIZE), MINBUF);
	}
	nbuf = MIN(nbuf, MAXBUF);

#ifndef NOPROM
	/*
	 * Initialize more of the page structures. We walk through the
	 * list of physical memory, initializing all the pages that are
	 * backed by page structs.
	 *
	 * npages was set above.
	 */

	memsegp = memseg_base;
	index = 0;
	for (pmem = availmemory; pmem != 0; pmem = pmem->next) {
		first_page = mmu_btop(pmem->address);
		if (page_base > first_page)
			first_page = page_base;
		num_pages = mmu_btop(pmem->address + pmem->size) - first_page;
		if (num_pages > npages - index)
			num_pages = npages - index;
		page_init(&pp[index], num_pages, first_page, memsegp++);
		index += num_pages;
		if (index >= npages)
			break;
	}
#else NOPROM
	/* for SAS, memory is contiguous */
	page_init(&pp[0], npages, page_base, memseg_base);
#endif NOPROM

#ifdef VAC
	/*
	 * If we are running on a machine with a virtual address cache, we call
	 * mapin again for the valloc'ed memory now that we have the page
	 * structures set up so the hat routines leave the translations cached.
	 */
	if (vac)
		segkmem_mapin(&kvalloc, (addr_t)vvm, (u_int)(econtig - vvm),
			PROT_READ | PROT_WRITE, mapaddr, 0);
#endif VAC

	/*
	 * Initialize callouts.
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];
#ifdef NOPROM
	first_page = memsegs->pages_base;
	if (first_page < mapaddr + btoc(econtig - vvm))
		first_page = mapaddr + btoc(econtig - vvm);
	memialloc(first_page, memsegs->pages_end);
#else NOPROM
	/*
	 * Initialize memory free list. We free all the pages that are in
	 * our memseg list and aren't already taken by the valloc's.
	 */
	for (memsegp = memsegs; memsegp; memsegp = memsegp->next) {
		first_page = memsegp->pages_base;
		if (first_page < mapaddr + btoc(econtig - vvm))
			first_page = mapaddr + btoc(econtig - vvm);
		memialloc(first_page, memsegp->pages_end);
	}
#endif	NOPROM

	printf("avail mem = %d\n", ctob(freemem));

	/*
	 * Initialize kernel memory allocator.
	 */
	kmem_init();

	/*
	 * Initialize resource maps.
	 */
	rminit(kernelmap, (long)(SYSPTSIZE - 1), (u_long)1,
	    "kernel map", MIN(4 * nproc, SYSPTSIZE/2));
	rminit(bufmap, (long)(BUFPAGES - 1), (u_long)1,
	    "buffer map", BUFPAGES/2);
	rminit(heapmap, (long)(HEAPPAGES - 1), (u_long)1,
	    "heap map", HEAPPAGES/2);
	rminit(iopbmap, (long)ctob(IOPBMEM), (u_long)DVMA,
	    "IOPB space", IOPBMAPSIZE);
#ifdef IOMMU
	if (iom) {
		if (cpu == CPU_SUN4M_690) {
			rminit(vme24map, (long) VME24MAP_SIZE,
				(u_long) VME24MAP_BASE, "vme24 map space",
				(int) NDVMAMAPS(VME24MAP_SIZE));

			rminit(vme32map, (long) VME32MAP_SIZE,
				(u_long) VME32MAP_BASE, "vme32 map space",
				(int) NDVMAMAPS(VME32MAP_SIZE));
		} else {
			rminit(altsbusmap, (long) ALTSBUSMAP_SIZE,
				(u_long) ALTSBUSMAP_BASE, "alt.sbus map space",
				(int) NDVMAMAPS(ALTSBUSMAP_SIZE));
		}

		rminit(sbusmap, (long) SBUSMAP_SIZE, (u_long) SBUSMAP_BASE,
			"sbus map space", (int) NDVMAMAPS(SBUSMAP_SIZE));

		rminit(bigsbusmap, (long) BIGSBUSMAP_SIZE,
			(u_long) BIGSBUSMAP_BASE, "bigsbus map space",
			(int) NDVMAMAPS(BIGSBUSMAP_SIZE));

		rminit(mbutlmap, (long) MBUTLMAP_SIZE, (u_long) MBUTLMAP_BASE,
			"mbutl map space", (int) NDVMAMAPS(MBUTLMAP_SIZE));

		/*
		 * make sure dvmamap and sbus are the same one
		 * NOTE: this assumes that dvmamap is used by SBUS
		 *	 drivers, and mb_hd.mh_map is used by
		 *	 VME driver.
		 */

		dvmamap = sbusmap;

		if (cpu == CPU_SUN4M_690) {
			/* defaults vme I/O to vme24map */
			mb_hd.mh_map = vme24map;
		} else {
			mb_hd.mh_map = 0;
		}
	} else {
		rminit(dvmamap, (long)(btoc(DVMASIZE) - IOPBMEM),
			(u_long)IOPBMEM, "DVMA map space", NDVMAMAPS(dvmasize));
	}
#else
	rminit(dvmamap, (long)(dvmasize - IOPBMEM), (u_long)IOPBMEM,
	    "DVMA map space", NDVMAMAPS(dvmasize));
#endif

	/*
	 * Allocate IOPB memory space just below the end of
	 * memory and map it to the first pages of DVMA space.
	 * The memory from maxmem to physmem has no page structs
	 * allocated to it. This is necessary so those pages cannot
	 * be cacheable. This is a cornerstone of our software cache
	 * consistency algorithm.
	 * NOTE: this assumes that the memory for IOPBMEM is physically
	 * contiguous.
	 */
#ifndef NOPROM
	/*
	 * Debuggers and monitor have already stolen their memory
	 * from the physmem structure.
	 */
	first_page = mmu_btop(lastpmem->address+lastpmem->size) - IOPBMEM;
	if (first_page < mmu_btop(lastpmem->address)) {
		if (dbug_mem) {
			printf("Debugger Size: %d Too Big!\n",
				mmu_ptob(dbug_mem));
			panic("too many debugger pages\n");
			/*NOTREACHED*/
		} else {
			printf("No room for IOPBMAP: last mem %d bytes\n",
				mmu_ptob(lastpmem->size));
			panic("no room for IOPBMAP");
			/*NOTREACHED*/
		}
	}
	segkmem_mapin(&kdvmaseg, DVMA, mmu_ptob(IOPBMEM),
		PROT_READ | PROT_WRITE, first_page, 0);
#else NOPROM
	segkmem_mapin(&kdvmaseg, DVMA, mmu_ptob(IOPBMEM),
		PROT_READ | PROT_WRITE, maxmem, 0);
#endif NOPROM

#ifdef IOMMU
	/* load tranlations for IOPBMEM in ioptes */
	if (iom) {
		/* init/turn on iommu for kernel */
		iom_init();
		for (i = 0; i < iommu_btop(mmu_ptob(IOPBMEM)); i++) {
#ifndef NOPROM
			if (cpu == CPU_SUN4M_690) {
				/* map iopbmem in vme24 range */
				iom_dvma_pteload((int) (first_page + i),
					IOMMU_IOPBMEM_BASE + iommu_ptob(i),
					IOM_WRITE);
			}

			/* also map iopbmem in sbus range */
			iom_dvma_pteload((int) (first_page + i),
				IOMMU_SBUS_IOPBMEM_BASE + iommu_ptob(i),
				IOM_WRITE);
#ifdef IOC
			/*
			 * need to setup 'w' bit in IOC even though it's
			 * notused
			 */

			if (cpu == CPU_SUN4M_690) {
				ioc_setup((int) iommu_ptob(i),
					IOC_LINE_NOIOC | IOC_LINE_WRITE);
			}
#endif IOC

#else
			if (cpu == CPU_SUN4M_690) {
				/* map iopbmem in vme24 range */
				iom_dvma_pteload(maxmem + i,
					IOMMU_IOPBMEM_BASE + iommu_ptob(i),
					IOM_WRITE);
			}

			/* also map iopbmem in sbus range */
			iom_dvma_pteload(maxmem + i,
				IOMMU_SBUS_IOPBMEM_BASE + iommu_ptob(i),
				IOM_WRITE);

			/*
			 * need to setup 'w' bit in IOC even though it's
			 * notused
			 */

			if (cpu == CPU_SUN4M_690) {
				ioc_setup(iommu_ptob(i),
					IOC_LINE_NOIOC | IOC_LINE_WRITE);
			}

#endif NOPROM
		}	/* end of for */
	}
#endif IOMMU

	/*
	 * Configure the system.
	 */
	startup_cpus();
	configure();		/* set up devices */
	memerr_init();		/* Make sure ECC mem err correction is on */
	maxmem = freemem;

	/*
	 * Initialize the u-area segment type.	We position it
	 * after the configured tables and buffers (whose end
	 * is given by econtig) and before HEAPBASE.
	 */
	v = econtig;
	i = nproc * ptob(SEGU_PAGES);
	if (v + i > (caddr_t) HEAPBASE)
		panic("insufficient virtual space for segu: nproc too big?");
	segu = seg_alloc(&kas, v, i);
	if (segu == NULL)
		panic("cannot allocate segu\n");
	if (segu_create(segu, (caddr_t)NULL) != 0)
		panic("segu_create segu");

	/*
	 * Now create generic mapping segment.	This mapping
	 * goes NCARGS beyond Syslimit up to DEBUGSTART.
	 * But if the total virtual address is greater than the
	 * amount of free memory that is available, then we trim
	 * back the segment size to that amount.
	 */
	v = Syslimit + NCARGS;
	i = (addr_t)DEBUGSTART - v;
	if (i > mmu_ptob(freemem))
		i = mmu_ptob(freemem);
	segkmap = seg_alloc(&kas, v, i);
	if (segkmap == NULL)
		panic("cannot allocate segkmap");
	a.prot = PROT_READ | PROT_WRITE;
	if (segmap_create(segkmap, (caddr_t)&a) != 0)
		panic("segmap_create segkmap");

	ledping((int *)0);	/* start LED light show */

	(void) spl0();		/* allow interrupts */
}

#ifdef	MULTIPROCESSOR
extern	struct	proc	idleproc;

#define	MID2CPU(mid)	((mid)^8)
int		okprocset = 0xF;
int		uniprocessor = 0; /* default to MultiProcessor */
unsigned	snid[NCPU];	/* saved node id's for each CPU */
struct dev_reg	ctxpr[NCPU];
#endif	MULTIPROCESSOR

startup_cpus()
{
#ifdef	MULTIPROCESSOR
	extern unsigned cpuX_startup[];
	unsigned	nodeid;
	unsigned	mid;
	unsigned	who;
	unsigned	ctxpa[NCPU];
	unsigned	multi;
	int		delays;
	extern	addr_t	map_regs();
	extern	dnode_t prom_childnode();

	/*
	 * Kick in other CPUs
	 *
	 * While the general mechanism here is pretty good, the specifics
	 * of how we figure which context table to use -- ugh, should allocate
	 * and construct them here, not earlier -- and physical addresses
	 * and such, are all very ugly and Sun-4M specific. Porting folks
	 * please take note ...
	 *
	 * It would be even nicer if we could make this look MORE like
	 * the cpus are just "normal devices" instead of doing all
	 * this horrible special-casing.
	 *
	 * okprocset sanitization:
	 *	force boot processor on,
	 *	only allow bits zero thru NCPU-1 to be on.
	 */
	okprocset &= (1<<nmod) - 1;	/* limit indicies to sane values */

	multi = 0;
/*
 * Locate all supported processors.
 */		
	nodeid = prom_nextnode(0); /* root node */
	for (nodeid = (unsigned) prom_childnode((dnode_t) nodeid);
	     (nodeid != 0) && (nodeid != -1);
	     nodeid = (unsigned) prom_nextnode((dnode_t) nodeid)) {
		if ((prom_getproplen(nodeid, "mid") == sizeof mid) &&
		    (prom_getprop(nodeid, "mid", (caddr_t) &mid) != -1) &&
		    (okprocset & (1 << (who = MID2CPU(mid))))) {
#ifdef	PC_prom_mailbox
			struct dev_reg reg;
			if ((prom_getproplen(nodeid, "mailbox") == sizeof reg) &&
			    (prom_getprop(nodeid, "mailbox", (caddr_t) &reg) != -1))
				PerCPU[who].prom_mailbox =
					(u_char *) map_regs(reg.reg_addr, reg.reg_size, reg.reg_bustype);
#endif	PC_prom_mailbox
			ctxpa[who] = CA2PA(&contexts[who*nctxs].ptpe_int);
			snid[who] = nodeid;
			printf("cpu%d at Mbus 0x%x 0x%x\n",
			       who, mid, ctxpa[who]);

			if (who != cpuid)
				multi |= 1<<who;
		}
	}
/*
 * If we have the potential of becoming a multiprocessor,
 * examine the boot flags and maybe override the uniprocessor
 * flag; note that the "-a" flag makes us ask whether the user
 * wants to be uniprocessor or multiprocessor, and if the kernel
 * had its uniprocessor flag patched on, we override that here.
 */
	if (multi) {
		if (boothowto & RB_MULTIPROCESSOR)
			uniprocessor = 0;
		else if (boothowto & RB_UNIPROCESSOR)
			uniprocessor = 1;
		else if (boothowto & RB_ASKMORE) {
			char	pmi[64];
			printf("processor mode (Uniprocessor, Multiprocessor)? ");
			gets(pmi);
			if ((pmi[0] == 'u') || (pmi[0] == 'U'))
				uniprocessor = 1;
			if ((pmi[0] == 'm') || (pmi[0] == 'M'))
				uniprocessor = 0;
		}
	}
/*
 * If we are supposed to be uniprocessor -- possibly modified
 * by boot flags or asking -- forget about all non-boot processors.
 */
	if (uniprocessor)
		multi = 0;	/* do not multiprocess. */
/*
 * If we still know of other processors, become a multiprocessor.
 */
	if (multi) {
		/*
		 * set up all PerCPU areas.
		 */ 
		for (who=0; who<NCPU; ++who)
			if (multi & (1<<who)) {
				struct user *up;
				struct proc *pp;

				up = &PerCPU[who].idleuarea;
				pp = &PerCPU[who].idleproc;
				*up = idleuarea;
				*pp = idleproc;
				PerCPU[who].cpuid = who;
				pp->p_stat = SRUN;
				pp->p_cpuid = who;
				pp->p_pam = 1<<who;
				PerCPU[who].uunix = &idleuarea;
				PerCPU[who].masterprocp = &idleproc;
			}

		/*
		 * Initialize XC and MP services
		 */
		xc_init();
		mp_init();

#ifdef	VAC
		/*
		 * Flush data from cache to memory so the other CPUs
		 * can see what we just wrote before they have their
		 * VACs turned on.
		 */
		if (vac)
			vac_flushall();
#endif	VAC
		/*
		 * Call the other processors out of their holding patterns.
		 */
		for (who=0; who<NCPU; ++who)
			if (multi & (1<<who)) {
				ctxpr[who].reg_bustype = 0;
				ctxpr[who].reg_addr = (addr_t)ctxpa[who];
				ctxpr[who].reg_size = 0;
				PerCPU[who].cpu_exists = 0;
				delays = 0;
				(void)prom_startcpu(snid[who], &ctxpr[who], 0, (addr_t)cpuX_startup);
				klock_exit();
				while (PerCPU[who].cpu_exists != 1) {
					DELAY(0x10000);
					delays++;
					if (delays > 20)
						break;
				}
				klock_enter();
				if ((delays > 20) && (debug_msg))
					printf("cpu %d won't startup\n", who);
			}
		printf("entering multiprocessor mode\n");
		uniprocessor = 0;
	} else {
		printf("entering uniprocessor mode\n");
		uniprocessor = 1;
	}

#endif	MULTIPROCESSOR
}

struct	bootf {
	char	let;
	short	bit;
} bootf[] = {
	'a',	RB_ASKNAME,
	'b',	RB_NOBOOTRC,
	'c',	RB_COPYBACK,
	'd',	RB_DEBUG,
	'h',	RB_HALT,
	'i',	RB_INITNAME,
	'm',	RB_MULTIPROCESSOR,
	'q',	RB_ASKMORE,
	's',	RB_SINGLE,
	't',	RB_WRITETHRU,
	'u',	RB_UNIPROCESSOR,
	'w',	RB_WRITABLE,
	0,	0,
};

char *initpath[] = {
	"/sbin/",
	"/single/",
	"/etc/",
	"/bin/",
	"/usr/etc/",
	"/usr/bin/",
	0
};
char *initname = "init";

/*
 * Parse the boot line to determine boot flags.
 */
bootflags()
{
	register char *cp;
	register int i;

	cp = prom_bootargs();
	if (cp) {
		while (*cp && (*cp != '-'))
			++cp;
		if (*cp++ == '-')
		do {
			for (i = 0; bootf[i].let; i++) {
				if (*cp == bootf[i].let) {
					boothowto |= bootf[i].bit;
					break;
				}
			}
			cp++;
		} while (bootf[i].let && *cp && (*cp != ' '));

		if (boothowto & RB_INITNAME) {
			while (*cp && (*cp == ' '))
				++cp;
			initname = cp;
		}
	}
	if (boothowto & RB_HALT) {
		printf("halted by -h flag\n");
		prom_enter_mon();
	}
}

/*
 * Start the initial user process.
 * The program [initname] is invoked with one argument
 * containing the boot flags.
 */
icode(rp)
	struct regs *rp;
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
	u.u_ar0 = (int *)rp;

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
		 * Build a pathname.
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
			i += 1; /* size the name */
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
	register u_long entry;
{
	register struct regs *rp;
	register struct pcb *pcb = &u.u_pcb;

	/*
	 * Initialize user registers.
	 */
	rp = (struct regs *)u.u_ar0;
	rp->r_g1 = rp->r_g2 = rp->r_g3 = rp->r_g4 = rp->r_g5 =
	    rp->r_g6 = rp->r_g7 = rp->r_o0 = rp->r_o1 = rp->r_o2 =
	    rp->r_o3 = rp->r_o4 = rp->r_o5 = rp->r_o7 = 0;

	rp->r_psr = PSL_USER;
	rp->r_pc = entry;
	rp->r_npc = entry + 4;
	u.u_eosys = JUSTRETURN;

	/*
	 * Throw out old user windows, init window buf.
	 */
	trash_user_windows();

	/*
	 * If the process's parent used the fpu, the child
	 * inherited a copy of the fpu context, but if we're
	 * going to exec, we should toss this context in
	 * order to avoid the grok of, say, init (for some
	 * reason) getting a floating point context and
	 * forcing the overhead of same on all its children.
	 * If the child then wants to use the fpu, then let it
	 * pay for it (by taking a FPU trap, which will cause
	 * a context to be allocated).
	 */

	if (pcb->pcb_fpctxp) {
		kmem_free((caddr_t) pcb->pcb_fpctxp, sizeof (struct fpu));
		pcb->pcb_fpctxp = (struct fpu *) 0;
		/*
		 * This flag is mostly historical
		 */
		pcb->pcb_fpflags = FP_UNINITIALIZED;
	}

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
	register int *regs;
	struct sigframe {
		struct rwindow rwin;		/* save area for new sp */
		struct sigargs {		/* arguments for handler */
			int	sig;
			int	code;
			struct	sigcontext *scp;
			char	*addr;
		} args;
		struct sigcontext sc;		/* sigcontext for signal */
	};
	register struct sigframe *fp;
	int oonstack;
	register int i;
	register struct pcb *pcb = &u.u_pcb;

	/*
	 * Make sure the current last user window has been flushed to
	 * the stack save area before we change the sp.
	 */
	flush_user_windows_to_stack();
	regs = u.u_ar0;
	oonstack = u.u_onstack;
	if (!u.u_onstack && (u.u_sigonstack & sigmask(sig))) {
		fp = (struct sigframe *)
		    ((int)u.u_sigsp - SA(sizeof (struct sigframe)));
		u.u_onstack = 1;
	} else {
		fp = (struct sigframe *)
		    ((int)regs[SP] - SA(sizeof (struct sigframe)));
	}
	/*
	 * Allocate and validate space for the signal handler
	 * context.  on_fault will catch any faults.
	 */
	if (((int)fp & (STACK_ALIGN-1)) != 0 ||
	    (caddr_t)fp >= (caddr_t)KERNELBASE || on_fault()) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		printf("sendsig: bad signal stack pid=%d, sig=%d\n",
		    u.u_procp->p_pid, sig);
		printf("sigsp = 0x%x, action = 0x%x, upc = 0x%x\n",
		    fp, p, regs[PC]);
		u.u_signal[SIGILL] = SIG_DFL;
		sig = sigmask(SIGILL);
		u.u_procp->p_sigignore &= ~sig;
		u.u_procp->p_sigcatch &= ~sig;
		u.u_procp->p_sigmask &= ~sig;
		psignal(u.u_procp, SIGILL);
		return;
	}
	/*
	 * Put signal and return info on signal stack.
	 */
	fp->sc.sc_onstack = oonstack;
	fp->sc.sc_mask = mask;
	fp->sc.sc_sp = regs[SP];
	fp->sc.sc_pc = regs[PC];
	fp->sc.sc_npc = regs[nPC];
	fp->sc.sc_psr = regs[PSR];
	fp->sc.sc_g1 = regs[G1];	/* save for sigcleanup syscall number */
	fp->sc.sc_o0 = regs[O0];	/* save for sigcleanup scp */
	fp->sc.sc_wbcnt = pcb->pcb_wbcnt; /* save outstanding windows */
	for (i = 0; i < pcb->pcb_wbcnt; i++) {
		fp->sc.sc_spbuf[i] = pcb->pcb_spbuf[i];
		bcopy((caddr_t)&pcb->pcb_wbuf[i],
		    (caddr_t)fp->sc.sc_wbuf[i],
		    sizeof (struct rwindow));
	}
	fp->args.sig = sig;
	/*
	 * Since we flushed the user's windows and we are changing his
	 * stack pointer, the window that the user will return to will
	 * be restored from the save area in the frame we are setting up.
	 * We copy in save area for old stack pointer so that
	 * debuggers can do a proper stack backtrace from the signal handler.
	 */
	if (pcb->pcb_wbcnt == 0) {
		bcopy((caddr_t)regs[SP], (caddr_t)&fp->rwin,
		    sizeof (struct rwindow));
	}
	switch (sig) {

	case SIGILL:
	case SIGFPE:
	case SIGEMT:
	case SIGBUS:
	case SIGSEGV:
		/*
		 * These signals get a code and an addr.
		 */
		fp->args.addr = u.u_addr;
		fp->args.code = u.u_code;
		u.u_addr = (char *)0;
		u.u_code = 0;
		break;

	default:
		fp->args.addr = (char *)0;
		fp->args.code = 0;
		break;
	}
	fp->args.scp = &fp->sc;
	no_fault();
	pcb->pcb_wbcnt = 0;		/* let user go on */
	regs[SP] = (int)fp;
	regs[PC] = (int)p;
	regs[nPC] = (int)p + 4;
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
	register struct sigcontext *scp;
	register int *regs;
	register int i, j;
	register struct pcb *pcb = &u.u_pcb;
	extern int nwindows;

	regs = u.u_ar0;
	scp = (struct sigcontext *)((caddr_t)regs[O0]);
	if ((caddr_t)scp >= (caddr_t)KERNELBASE || ((int)scp & 0x3))
		return;
	if (on_fault())
		return;
	/* make sure pc's are aligned */
	if ((scp->sc_pc & 0x3) || (scp->sc_npc & 0x3))
		return;
	/*
	 * Make sure we return to the old window we saved in sendsig.
	 */
	flush_user_windows();
	u.u_onstack = scp->sc_onstack & 01;
	u.u_procp->p_sigmask = scp->sc_mask & ~cantmask;
	regs[SP] = scp->sc_sp;
	regs[PC] = scp->sc_pc;
	regs[nPC] = scp->sc_npc;
	regs[PSR] = regs[PSR] & ~PSL_USERMASK | scp->sc_psr & PSL_USERMASK;
	regs[G1] = scp->sc_g1;	/* restore regs signal handler can't restore */
	regs[O0] = scp->sc_o0;
	if ((u_int)scp->sc_wbcnt < nwindows) { /* restore outstanding windows */
		j = pcb->pcb_wbcnt;
		for (i = 0; i < scp->sc_wbcnt; i++) {
			pcb->pcb_spbuf[j + i] = scp->sc_spbuf[i];
			bcopy((caddr_t)scp->sc_wbuf[i],
			    (caddr_t)&pcb->pcb_wbuf[j + i],
			    sizeof (struct rwindow));
		}
		pcb->pcb_wbcnt += scp->sc_wbcnt;
	} else {
		pcb->pcb_wbcnt = 0;
	}
	no_fault();
	/*
	 * Avoid mucking with carry flag and return regs on return to user.
	 */
	u.u_eosys = JUSTRETURN;
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
	extern void prom_reboot();

#ifdef MULTIPROCESSOR
	xc_attention();		/* simplify the world */
#endif MULTIPROCESSOR
	consdev = rconsdev;
	consvp = rconsvp;
	start_mon_clock();
#ifdef	PROM_PARANOIA
prom_eprintf("PROM_PARANOIA: inside boot()\n");
#endif	PROM_PARANOIA
	if ((howto & RB_NOSYNC) == 0 && waittime < 0 && bfreelist[0].b_forw &&
	    bufprevflag == 0) {
		bufprevflag = 1;		/* prevent panic recursion */
		waittime = 0;
		s = spl0();
		vfs_syncall();
		(void) splx(s);
	}
#ifdef NOPROM
	asm("t 255");
#else
	/* extreme priority; allow clock interrupts to monitor at level 14 */
	/* but don't allow UART interrupts */
	s = splzs();
	if (howto & RB_HALT) {
		halt((char *)NULL);
		howto &= ~RB_HALT;
	} else {
		if (howto & RB_DUMP && prevflag == 0) {
			if ((boothowto & RB_DEBUG) != 0 && nopanicdebug == 0) {
				CALL_DEBUG();
			}
			prevflag = 1;		/* prevent panic recursion */
			dumpsys();
		}
	}
	printf("rebooting...\n");

	INT_REGS->sys_mset = IR_ENA_INT;	/* disable all interrupts */

	start_mon_clock();
	if (howto & RB_STRING)
		prom_reboot(bootstr);
	else
		prom_reboot(howto & RB_SINGLE ? "-s" : "");
	/* should never get here */
	(void) splx(s);
#endif NOPROM
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
	struct memlist	*pmem;

	/*
	 * We mark all the pages below the first page (other than page 0)
	 * covered by a page structure.
	 * This gets us the message buffer and the kernel (text, data, bss),
	 * including the interrupt stack and proc[0]'s u area.
	 * (We also get page zero, which we may not need, but one page
	 * extra is no big deal.)
	 */
#ifndef NOPROM
	pmem = availmemory;
	lophys = mmu_btop(pmem->address);
#else
	/* for SAS, always start from pfn 0, all in one seg. */
	lophys = 0;
#endif
	lomem = memsegs->pages_base;
	for (i = lophys; i < lomem; i++)
		dump_addpage(i);
}

extern	struct dev_reg	obp_mailbox;

/*
 * Machine-dependent portion of dump-checking;
 * verify that a physical address is valid.
 */
int
dump_checksetbit_machdep(addr)
	u_int	addr;
{
	struct memlist	*pmem;

#ifndef NOPROM
	for (pmem = availmemory; pmem; pmem = pmem->next) {
		if (pmem->address <= addr &&
		    addr < (pmem->address + pmem->size))
			return (1);
	}
	if ((addr & MMU_PAGEMASK) ==
		((u_int)obp_mailbox.reg_addr & MMU_PAGEMASK))
		return (1);

	return (0);
#else
	/* all SAS memory are contiguous, from 0 to availmem */
	return ((addr >= 0) && (addr < availmem));
#endif
}

/*
 * Machine-dependent portion of dump-checking;
 * mark all pages for dumping.
 */
dump_allpages_machdep()
{
	u_int	i, j;
	struct memlist	*pmem;

#ifndef NOPROM
	for (pmem = availmemory; pmem; pmem = pmem->next) {
		i = mmu_btop(pmem->address);
		j = i + mmu_btop(pmem->size);
		for (; i < j; i++)
			dump_addpage(i);
	}
#else
	/* all SAS memory are contiguous, from 0 to availmem */
	for (i = 0; i < mmu_btop(availmem); i++)
		dump_addpage(i);
#endif
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
	struct pte pte;
#ifdef	IOMMU
	union iommu_pte *psbuspte, *pvmepte;

	union iommu_pte* iom_ptefind();
	extern void hat_vacsync();
#endif	IOMMU

	addr = &DVMA[mmu_ptob(dvmasize) - MMU_PAGESIZE];

	*(u_int *)&pte = MMU_STD_INVALIDPTE;
	pte.EntryType = MMU_ET_PTE;
	pte.AccessPermissions = MMU_STD_SRWX;
	pte.PhysicalPageNumber = pg;

	/* this loads up host SRMMU */
	mmu_setpte(addr, pte);

#ifdef	IOMMU
	psbuspte = iom_ptefind(dvmasize - 1, sbusmap);
	if (cpu == CPU_SUN4M_690)
		/* duplicate this mapping on IOMMU on both Sbus & VME */
		pvmepte = iom_ptefind(dvmasize - 1, vme24map);

	psbuspte->iopte_uint = *(u_int*) (&pte);
	if (cpu == CPU_SUN4M_690) {
		pvmepte->iopte_uint = *(u_int*) (&pte);

#ifdef IOC
		ioc_setup(dvmasize - 1, IOC_LINE_NOIOC | IOC_LINE_WRITE);
#endif IOC
	}

	/*
	 * psbuspte = pvmepte, so only need to check for cache
	 * bit in one.
	 */
	if (cache && psbuspte->iopte.cache && vac) {
		hat_vacsync((u_int) MAKE_PFNUM(&pte));
		psbuspte->iopte.cache = 0;
		if (cpu == CPU_SUN4M_690)
			pvmepte->iopte.cache = 0;
	}

	/*
	 * now flush out the old TLBs in IOMMU.
	 */
	iommu_addr_flush((iopte_to_dvma(psbuspte)) & IOMMU_FLUSH_MSK);
	if (cpu == CPU_SUN4M_690)
		iommu_addr_flush((iopte_to_dvma(pvmepte)) & IOMMU_FLUSH_MSK);

#endif	IOMMU

	err = VOP_DUMP(vp, addr, bn, ctod(1));

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
 * and may span multiple pages.	 We must be careful because the object
 * (or trailing portion thereof) may not span a page boundary and the
 * next virtual address may map to i/o space, which could cause
 * heartache.
 * We assume that dvmasize is at least two pages.
 * And that MMU_PAGESIZE == IOMMU_PAGE_SIZE.
 */
int
dump_kaddr(vp, kaddr, bn, count)
	struct vnode *vp;
	caddr_t kaddr;
	int bn;
	int count;
{
	register caddr_t addr;
	register int err = 0;
	struct pte ptea, pteb, tpte;
	register int offset;
#ifdef	IOMMU
	union iommu_pte *psbuspte, *pvmepte;

	union iommu_pte* iom_ptefind();
	extern void hat_vacsync();
#endif	IOMMU

	offset = (u_int)kaddr & MMU_PAGEOFFSET;
	addr = &DVMA[mmu_ptob(dvmasize) - 2 * MMU_PAGESIZE];

	*(u_int *)&ptea = MMU_STD_INVALIDPTE;
	*(u_int *)&pteb = MMU_STD_INVALIDPTE;
	pteb.EntryType = MMU_ET_PTE;
	pteb.AccessPermissions = MMU_STD_SRWX;

	mmu_getpte(kaddr, &tpte);

#ifdef	IOMMU
	psbuspte = iom_ptefind(dvmasize - 2, sbusmap);
	if (cpu == CPU_SUN4M_690)
		pvmepte = iom_ptefind(dvmasize - 2, vme24map);
#endif	IOMMU

	pteb.PhysicalPageNumber = tpte.PhysicalPageNumber;

	while (count > 0 && !err) {
		ptea = pteb;
		/* this loads up host MMU */
		mmu_setpte(addr, ptea);
#ifdef	IOMMU
		/*
		 * now deal with IOMMU.  but since the dump device
		 * could be either on Sbus or VME bus, we just setup
		 * DVMA mappings on both buses.
		 */
		psbuspte->iopte_uint = *(u_int*) (&tpte);
		if (cpu == CPU_SUN4M_690) {
			pvmepte->iopte_uint = *(u_int*) (&tpte);

#ifdef IOC
			/*
			 * don't use IOC, so that we don't have to worry
			 * about invalidating the IOC before each I/O.
			 */
			ioc_setup(dvmasize - 2,
				IOC_LINE_NOIOC | IOC_LINE_WRITE);
#endif IOC
		}

		/*
		 * psbuspte = pvmepte, so only need to check for cache
		 * bit in one.
		 */
		if (cache && psbuspte->iopte.cache && vac) {
			hat_vacsync((u_int) MAKE_PFNUM(&tpte));
			psbuspte->iopte.cache = 0;
			if (cpu == CPU_SUN4M_690)
				pvmepte->iopte.cache = 0;
		}

		/*
		 * now flush out the old TLBs in IOMMU.
		 */
		iommu_addr_flush((iopte_to_dvma(psbuspte)) & IOMMU_FLUSH_MSK);
		if (cpu == CPU_SUN4M_690)
			iommu_addr_flush((iopte_to_dvma(pvmepte)) &
				IOMMU_FLUSH_MSK);

#endif	IOMMU
		mmu_getpte(kaddr + MMU_PAGESIZE, &tpte);
		pteb.PhysicalPageNumber = (tpte.EntryType &&
			bustype((int) tpte.PhysicalPageNumber) == BT_OBMEM) ?
			tpte.PhysicalPageNumber : 0;
		mmu_setpte(addr + MMU_PAGESIZE, pteb);

#ifdef	IOMMU
		psbuspte[1].iopte_uint = *(u_int*) (&tpte);
		if (cpu == CPU_SUN4M_690) {
			pvmepte[1].iopte_uint = *(u_int*) (&tpte);

#ifdef IOC
			ioc_setup(dvmasize - 1,
				IOC_LINE_NOIOC | IOC_LINE_WRITE);
#endif IOC
		}

		if (cache && psbuspte[1].iopte.cache && vac) {
			hat_vacsync((u_int) MAKE_PFNUM(&tpte));
			psbuspte[1].iopte.cache = 0;
			if (cpu == CPU_SUN4M_690)
				pvmepte[1].iopte.cache = 0;
		}

		/*
		 * now flush out the old TLBs in IOMMU.
		 */
		iommu_addr_flush((iopte_to_dvma(&psbuspte[1])) &
			IOMMU_FLUSH_MSK);
		if (cpu == CPU_SUN4M_690)
			iommu_addr_flush((iopte_to_dvma(&pvmepte[1])) &
				IOMMU_FLUSH_MSK);

#endif	IOMMU

		err = VOP_DUMP(vp, addr + offset, bn, ctod(1));
		bn += ctod(1);
		count -= ctod(1);

		kaddr += MMU_PAGESIZE;
	}

	return (err);
}

/*
 * Halt the machine and return to the monitor
 */
halt(s)
	char *s;
{
#ifdef NOPROM
	if (s)
		printf("(%s) ", s);
	printf("Halted\n\n");
	asm("t 255");
#else
	trigger_logan();
#ifdef MULTIPROCESSOR
	xc_attention();		/* simplify the world */
#endif MULTIPROCESSOR
		(void) spl7();
		if (s)
			prom_printf("(%s) ", s);
		prom_printf("Halted\n\n");
		prom_exit_to_mon();
#endif NOPROM
}

/*
 * Common routine for bp_map and buscheck which will read in the
 * pte's from the hardware mapping registers into the pte array given.
 */
static
read_hwmap(bp, npf, pte)
	struct buf *bp;
	int npf;
	register struct pte *pte;
{
	register addr_t addr;
	struct as *as;
	union ptpe *hpte;

	if (bp->b_proc == (struct proc *)NULL ||
	    (bp->b_flags & B_REMAPPED) != 0 ||
	    (as = bp->b_proc->p_as) == (struct as *)NULL)
		as = &kas;
	addr = bp->b_un.b_addr;
	hpte = NULL;
	while (npf-- > 0) {
		if ((hpte == NULL) ||
		    (((u_int)addr & (L3PTSIZE - 1)) < MMU_PAGESIZE))
			hpte = hat_ptefind(as, addr);
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
		*pte = hpte->pte;
		if (!pte_valid(pte))
			panic("read_hwmap invalid pte");
		/*
		 * We make the translation writable, even if the current
		 * mapping is read only. This is necessary because the
		 * new pte is blindly used in other places where it needs
		 * to be writable.
		 */
		pte->AccessPermissions = MMU_STD_SRWX; /* XX generous?? */
		pte++;
		addr += MMU_PAGESIZE;
		hpte++;
	}
}

/*
 * This number is derived from maxphys && klustsize
 * are set above in startup() to limit the maximum
 * transfer size to 124k.  We add a pad here to account
 * for non-page aligned transfers.
 */
#define	MAXMBPHYS	((124 * 1024) + PAGESIZE)

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

	/* if kaddr >= DVMA, that's bp_iom_map()'s job */
	if (kaddr >= DVMA)
		panic("bp_map: bad kaddr\n");

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
		/*
		 * If the address is between HEAPBASE and HEAPLIMIT
		 * the ptes are in Heapptes.
		 * If the address is between SYSBASE and Syslimit
		 * the ptes are in Sysmap.
		 * else we have to read the hardware for the page tables.
		 */
		u_int vaddr = (u_int) bp->b_un.b_addr;
		if (vaddr >= HEAPBASE && vaddr < HEAPLIMIT)
			spte = &Heapptes[btop(vaddr - HEAPBASE)];
		else if ((vaddr >= SYSBASE) && (vaddr < (u_int) Syslimit)){
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
	flags = PTELD_NOSYNC;

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
			/*
			 * FIXME: probably should get rid of last arg.
			 * to hat_mempte() in vm_hat.c so that it will
			 * be the same with other machines.
			 *
			 * We stuff in KERNELBASE for now since we know
			 * PAGEIO only goes to KERNEL space.
			 */
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
		segkmem_mapin(&kseg, kaddr, MMU_PAGESIZE,
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
		 * If the address is between HEAPBASE and HEAPLIMIT
		 * the ptes are in Heapptes.
		 * If the address is between SYSBASE and Syslimit
		 * the ptes are in Sysmap.
		 * else we have to read the hardware for the page tables.
		 */
		if (vaddr >= HEAPBASE && vaddr < HEAPLIMIT)
			spte = &Heapptes[btop(vaddr - HEAPBASE)];
		else if ((vaddr >= SYSBASE) && (vaddr < (u_int) Syslimit)){
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
	pf = pt = (u_int) -1;

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
			if (!pte_valid(pte) ||
			    pt != bustype((int) pte->PhysicalPageNumber)) {
				return (-1);
			} else if (pt != BT_OBMEM &&
				    pte->PhysicalPageNumber != pf) {
				return (-1);
			}
		} else {
			if (!pte_valid(pte)) {
				return (-1);
			}
			pf = pte->PhysicalPageNumber;
			pt = bustype((int) pte->PhysicalPageNumber);
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
			}
		}
		pf++;
		npf--;
	}
	return (res);
}

/*
 * Allocate 'size' units from the given map so that
 * the vac alignment constraints for bp are maintained.
 *
 * Return 'addr' if successful, 0 if not.
 */

bp_alloc(map, bp, size)
	struct map *map;
	struct buf *bp;
	int size;
{
	register struct mapent *mp;
	register int addr, mask, s;
	int align = -1;
	struct page *pp = NULL;

#ifdef IOC
	int new_align;
	int start_padding = 0;
#endif IOC

#ifdef VAC
	if (vac) {
		if ((bp->b_flags & B_PAGEIO) != 0) {
			/*
			 * Peek at the first page's alignment.
			 * We could work harder and check the alignment
			 * of all the pages.  If a conflict is found
			 * and the page is not kept (more than once
			 * if intrans), then try to do hat_pageunload()'s
			 * to allow the IO to be cached.  However,
			 * this is very unlikely and not worth the
			 * extra work (for now at least).
			 */

			pp = bp->b_pages;

			if (pp->p_mapping != NULL) {
				align = mmu_btop((int)
						sptetovaddr((struct spte *)
						pp->p_mapping) &
						(shm_alignment - 1));
			}
		} else if (bp->b_un.b_addr != NULL) {
			align = mmu_btop((int)bp->b_un.b_addr &
					    (shm_alignment - 1));
			}
	}
#endif VAC

#ifdef IOC
	/*
	 * due to 8k mapping to each IOC line, we need to align to 8K
	 * boundary.
	 */
	if (ioc) {
		/*
		 * see if it can be IOC'ed.
		 */

		if (ioc_able(bp, map) != IOC_LINE_INVALID) {
			if (align == -1) { /* vac must be off */
				if ((bp->b_flags & B_PAGEIO) != 0) {

					pp = bp->b_pages;
					if (pp->p_mapping != NULL) {
						align = mmu_btop((int)
						    sptetovaddr((struct spte *)
						    pp-> p_mapping) &
						    (IOC_LINE_MAP - 1));
					} else {
						align = 0;
					}
				} else if (bp->b_un.b_addr != NULL) {
					align = mmu_btop((int)bp->b_un.b_addr
						& (IOC_LINE_MAP - 1));
					}
			}

			/* see if we need a padding at the beginning */
			if ((new_align = ioc_adj_start(align)) != align) {
				start_padding = align - new_align;
				size += start_padding;
				align = new_align;
			}

			/* see if we need a padding at the end */
			if ((new_align = ioc_adj_end(align+size-1))
				!= (align+size-1)) {
				size += (new_align - (align+size-1));
			}
		}
	}
#endif IOC

	if (align == -1) {
		s = splhigh();
		align = (int)rmalloc(map, (long)size);
		(void) splx(s);
		return (align);
	}

	/*
	 * Look for a map segment containing a request that works.
	 * If none found, return failure.
	 * Since VAC has a much stronger alignment requirement,
	 * we'll use shm_alignment even ioc is on too.
	 */

	if (vac)
		mask = mmu_btop(shm_alignment) - 1;
#ifdef IOC
	else	/* vac is off */
		mask = IOC_PAGES_PER_LINE - 1;
#endif

	for (mp = mapstart(map); mp->m_size; mp++) {
		if (mp->m_size < size)
			continue;

		/*
		 * Find first addr >= mp->m_addr that
		 * fits the alignment constraints.
		 */
		addr = (mp->m_addr & ~mask) + align;
		if (addr < mp->m_addr)
			addr += mask + 1;

		/*
		 * See if it fit within the map.
		 */
		if (addr + size <= mp->m_addr + mp->m_size)
			break;
	}

	if (mp->m_size == 0)
		return (0);

	s = splhigh();
	align = rmget(map, (long)size, (u_long)addr);

#ifdef IOC
	if (align && start_padding)
		align += start_padding;
#endif IOC

	(void) splx(s);
	return (align);

}

#ifdef IOMMU
int	dvma_incoherent = 0;
/*
 * NOTE: this routine assumes that IOMMU and SRMMU has the same page sizes.
 */
bp_iom_map(bp, iopfn, iom_flags, io_map)
	register struct buf *bp;
	int iopfn;
	int iom_flags;
	struct map *io_map;
{
	auto struct buf bpcopy;
	auto struct pte ptecache[PTECHUNKSIZE];
	register struct pte *pte = &ptecache[0];
	register struct pte *spte = (struct pte *) NULL;
	register struct page *pp = (struct page *) NULL;
	int npf, cidx;
	int srmmu_flags, iosize;
	union iommu_pte *p_iom_pte, *p_iom_start;
	caddr_t kaddr;
	/* struct modinfo *modinfop; */ /* apparently not used */
#ifdef VAC
	extern void hat_vacsync();
#endif

#ifdef IOC
	int ioc_pfn, ioc_set, aligned_pfn, do_ioc;
#endif

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
		/*
		 * If the address is between HEAPBASE and HEAPLIMIT
		 * the ptes are in Heapptes.
		 * If the address is between SYSBASE and Syslimit
		 * the ptes are in Sysmap.
		 * else we have to read the hardware for the page tables.
		 */
		u_int vaddr = (u_int) bp->b_un.b_addr;
		if (vaddr >= HEAPBASE && vaddr < HEAPLIMIT)
			spte = &Heapptes[btop(vaddr - HEAPBASE)];
		else if ((vaddr >= SYSBASE) && (vaddr < (u_int) Syslimit)){
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

#ifdef IOC
	if (iom_flags & MDR_LINE_NOIOC)
		do_ioc = IOC_LINE_NOIOC;
	else
		do_ioc = ioc_able(bp, io_map);
#endif IOC

	/*
	 * We want to use mapin to set up the mappings now since some
	 * users of kernelmap aren't nice enough to unmap things
	 * when they are done and mapin handles this as a special case.
	 * If kaddr is in the kernelmap space, we use kseg so the
	 * software ptes will get updated. Otherwise we use kdvmaseg.
	 * We should probably check to make sure it is really in
	 * one of those two segments, but it's not worth the effort.
	 */
	if (iom_flags & DVMA_PEEK) {

		if (io_map == sbusmap || io_map == bigsbusmap ||
		    io_map == altsbusmap) {
			/* SBUS && PEEK */
			panic("bp_iom_map: SBUS && DVMA_PEEK!!\n");
		}

		srmmu_flags = PTELD_NOSYNC | PTELD_INTREP;
#ifdef IOC
		/*
		 * FIXME: not sure if we need to notify hosts hat layer
		 * about IOC or not.
		 */
		if (do_ioc == IOC_LINE_LDIOC)
			srmmu_flags |= PTELD_IOCACHE;
#endif
	}
	kaddr = &DVMA[iommu_ptob(iopfn)];

	if ((p_iom_pte = iom_ptefind(iopfn, io_map)) == NULL)
		panic("bp_iom_map: bad iopfn");

#ifdef IOC
	ioc_pfn = iopfn;
	ioc_set = 1;
#endif
	iosize = npf = btoc(bp->b_bcount + ((int)bp->b_un.b_addr & PAGEOFFSET));

	while (npf > 0) {
		/*
		 * First, fetch the pte we're interested in.
		 */
		if (spte) {
			pte = spte++;
		} else if (pp) {
			/*
			 * FIXME: probably should get rid of last arg.
			 * to hat_mempte() in vm_hat.c so that it will
			 * be the same with other machines.
			 *
			 * We stuff in KERNELBASE for now since we know
			 * PAGEIO only goes to KERNEL space.
			 */
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
		 * load iommu pte. We did not use iom_pteload()
		 * because iopte and srmmu pte fits each other
		 * just fine. This is much faster too.
		 */
		p_iom_pte-> iopte_uint = *(u_int*) pte;

		/*
		 * If the kernel is not using the cache, then don't
		 * bother playing cache coherency games.
		 */
		if (!cache)
			p_iom_pte-> iopte.cache = 0;
		/* Is this code really necessary on ROSS machine */
#ifdef  VAC
		/*
		 * If cache coherency does not work on this configuration,
		 * or if we are disabling it, then synchronize the cache
		 * with memory and don't play coherency games.
		 *
		 * For debugging, we can prevent turning off the
		 * coherent transactions, and we can individually
		 */
		if (vac &&
#ifndef	NOPROM
		    ((mod_info->ccoher == 0) || (dvma_incoherent) ||
			(bp-> b_flags & B_PAGEIO && iosize > 1)) &&
#else	NOPROM
		    (module_type == ROSS604) &&
#endif	NOPROM
		    p_iom_pte->iopte.cache) {
			hat_vacsync((u_int) MAKE_PFNUM(pte));
			p_iom_pte-> iopte.cache = 0;
		}
#endif VAC

#ifdef IOC
		/* setup IOC, we cheat here by setting every other page */
		if (do_ioc != IOC_LINE_INVALID) {
			if (ioc_set & 1)
				ioc_setup(ioc_pfn, do_ioc |
					((p_iom_pte-> iopte.write)?
					IOC_LINE_WRITE : 0));
			ioc_set++;
		}
#endif IOC
		/*
		 * Note we also load SRMMU pte here, we could use
		 * bp_map(), which would be too slow since it has to
		 * read maps all over again, and spin through each one of them
		 * like what we just did here.
		 */
		if (iom_flags & DVMA_PEEK) {
			segkmem_mapin(&kdvmaseg, kaddr, MMU_PAGESIZE,
				PROT_READ | PROT_WRITE, MAKE_PFNUM(pte),
					srmmu_flags);
		}
		kaddr += MMU_PAGESIZE;
		npf--;
		p_iom_pte++; /* we know ioptes on IOMMU are contiguous */
#ifdef IOC
		ioc_pfn++;
#endif IOC
	}

#ifdef IOC
	/*
	 * now we handle IOMMU's mappings to "paddings" due to IOC/IOMMU's
	 * different page sizes.
	 */
	if (do_ioc != IOC_LINE_INVALID) {
		if ((aligned_pfn = ioc_adj_start(iopfn)) != iopfn) {
			p_iom_start = iom_ptefind(aligned_pfn, io_map);
			p_iom_start-> iopte_uint = p_iom_start[1].iopte_uint;
		}

		/*
		 * NOTE: it should really read:
		 *	ioc_adj_end(iopfn+iosize+1-1) != iopfn+iosize+1-1.
		 *	+1: for red zone, -1: for pfn = start + lng -1.
		 */

		if ((aligned_pfn = ioc_adj_end(iopfn + iosize)) !=
			(iopfn + iosize)) {
			p_iom_pte->iopte_uint = (p_iom_pte - 1)->iopte_uint;

			/*
			 * if DVMA starts with odd pages, and size is even
			 * pages, due to the alignment/padding, the last IOC
			 * line is NOT set up yet.
			 */
			if (!(iosize & 0x1))
				ioc_setup(ioc_pfn,
					do_ioc | ((p_iom_pte-> iopte.write)?
					IOC_LINE_WRITE : 0));
			p_iom_pte++;
		}

	}
#endif IOC

	/* red zone on IOMMU */
	p_iom_pte-> iopte_uint = 0;

}
#endif IOMMU

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
	int npf;
	u_long a;
	addr_t	addr;
	int s;

	if (bp->b_flags & B_REMAPPED) {
		addr = (addr_t)((int)bp->b_un.b_addr & PAGEMASK);
		bp->b_un.b_addr = (caddr_t)((int)bp->b_un.b_addr & PAGEOFFSET);
		npf = mmu_btopr(bp->b_bcount + (int)bp->b_un.b_addr);
		hat_unload(&kseg, addr, (u_int)mmu_ptob(npf));
		a = mmu_btop(addr - Sysbase);
		bzero((caddr_t)&Usrptmap[a], (u_int)(sizeof (Usrptmap[0])*npf));
		s = splhigh();
		rmfree(kernelmap, (long)npf, a);
		(void) splx(s);
		bp->b_flags &= ~B_REMAPPED;
	}
}

/*
 * Explicitly set these so they end up in the data segment.
 * We clear the bss *after* initializing these variables in locore.s.
 */
char mon_clock_on = 0;		/* disables profiling */
extern trapvec kclock14_vec;	/* kernel clock14 vector code (see locore.s) */
extern trapvec mon_clock14_vec; /* monitor clock14 vector code (see locore.s) */

/*
 * at startup, we have the kernel vector installed in
 * the scb, but the hardware is like when the monitor
 * clock is going, so we need to make it all consistant.
 * when we are done, things are turned off, because we
 * can't change the mappings on the scb (yet).
 */

init_mon_clock()
{
	kclock14_vec = scb.interrupts[14 - 1];
#ifndef NOPROM
	mon_clock_on = 0;	/* enable profiling */
	set_clk_mode((u_long) 0,
		(u_long) IR_ENA_CLK14); /* disable level 14 clk intr */
#endif NOPROM
}

start_mon_clock()
{
#ifndef NOPROM
	if (!mon_clock_on) {
		mon_clock_on = 1; /* disable profiling */
		write_scb_int(14, &mon_clock14_vec); /* install mon vector */
		/* enable level 14 clk intr */
		set_clk_mode((u_long) IR_ENA_CLK14, (u_long) 0);
	}
#endif NOPROM
}

stop_mon_clock()
{
#ifndef NOPROM
	if (mon_clock_on) {
		mon_clock_on = 0; /* enable profiling */
		/* disable level 14 clk intr */
		set_clk_mode((u_long) 0, (u_long) IR_ENA_CLK14);
		write_scb_int(14, &kclock14_vec); /* install kernel vector */
	}
#endif NOPROM
}

/*
 * Write the scb, which is the first page of the kernel.
#ifndef	PC_vscb
 * Normally it is write protected, we provide a function
 * to fiddle with the mapping temporarily.
 *	1) lock out interrupts
 *	2) save the old pte value of the scb page
 *	3) set the pte so it is writable
 *	4) write the desired vector into the scb
 *	5) restore the pte to its old value
 *	6) restore interrupts to their previous state
#else	PC_vscb
 * Since we are using the scb mapping in the PerCPU area,
 * it is writable, so no PTE mods are required.
 * %%% do we need to spl8 or not? if so, do we need to
 * worry about the other cpu taking this interrupt while
 * we are modifying the vector?
#endif	PC_vscb
 */
write_scb_int(level, ip)
	register int level;
	struct trapvec *ip;
{
	/* disable ints while mucking with SCB entries. */
	register int s = spl8();
#ifndef	PC_vscb
	register union ptpe *ptpe;
	register int sv_perm;
	register trapvec *sp;

	sp = &scb.interrupts[level - 1];

	/* save old mapping */
	ptpe = hat_ptefind(&kas, (addr_t)sp);
	sv_perm = ptpe->pte.AccessPermissions;

	/* allow writes */
	pte_setprot(ptpe, MMU_STD_SRWX);

	/* write out new vector code */
	*sp = *ip;

	/* restore old mapping */
	pte_setprot(ptpe, sv_perm);

#else	PC_vscb
	vscb.interrupts[level-1] = *ip;
#endif	PC_vscb

	(void) splx(s);
}

/*
 * Handler for monitor vector cmd -
 * For now we just implement the old "g0" and "g4"
 * commands and a printf hack.
 *
 * NOTE: everyone is in the boot prom, so we can snatch
 * the klock (as long as we restore its value when done).
 */
void
v_handler(str)
	char *str;
{
	char *sargv[8];
#ifdef	MULTIPROCESSOR
	extern int	klock;
	extern int	xc_ready;
	int	klock_save;
	int	xc_ready_save;
#endif	MULTIPROCESSOR

	struct cmd_info {
		char	*cmd;
		int	func;
	};
#define	ENDADDR(a)	&a[sizeof (a) / sizeof (a[0])]
	static struct cmd_info vx_cmd[] = {
		"sync", 0,
	};
#define	vx_cmd_end	ENDADDR(vx_cmd)

	register struct cmd_info *cp;
	register int	func;

#ifdef	MULTIPROCESSOR
	klock_save = klock_steal();
	xc_ready_save = xc_ready;
	xc_ready = 0;		/* avoid using xc mechanism */
#endif	MULTIPROCESSOR

	parse_str(str, sargv);

	func = -1;

	for (cp = (struct cmd_info *)vx_cmd;
	    cp < (struct cmd_info*)vx_cmd_end;
	    cp++) {
		if (strcmp(sargv[0], cp->cmd) == 0) {
			func = cp->func;
			break;
		}
	}

	switch (func) {
	case 0:		/* sync */
		panic("zero");
		/* NOTREACHED */

	default:
		prom_printf("Don't understand '%s'\n", str);
	}
#ifdef	MULTIPROCESSOR
	xc_ready = xc_ready_save;
	klock = klock_save;
#endif	MULTIPROCESSOR
}

#define	isspace(c)	((c) == ' ' || (c) == '\t' || (c) == '\n' || \
			    (c) == '\r' || (c) == '\f' || (c) == '\013')

parse_str(str, args)
	register char *str;
	register char *args[];
{
	register int i;

	while (*str && isspace(*str))
		str++;
	for (i = 0; (i < 8) && (*str); /* empty */) {
		args[i++] = str;
		while (*str && (!isspace(*str)))
			str++;
		if (*str)
			*str++ = '\0';
		while (*str && isspace(*str))
			str++;
	}
}

/*
 * a macro used by async() to
 * atomically set system_fatal and
 * log info about the fault,
 * to be printed later when we panic.
 */
#ifdef MULTIPROCESSOR

#define	SYS_FATAL_FLT(TYPE) \
	if (atom_tas(system_fatal)) { \
		PerCPU[cpuid].sys_fatal_flt.aflt_type = TYPE; \
		PerCPU[cpuid].sys_fatal_flt.aflt_subtype = subtype; \
		PerCPU[cpuid].sys_fatal_flt.aflt_stat = afsr; \
		PerCPU[cpuid].sys_fatal_flt.aflt_addr0 = afar0; \
		PerCPU[cpuid].sys_fatal_flt.aflt_addr1 = afar1; \
		PerCPU[cpuid].sys_fatal_flt.aflt_err0 = aerr0; \
		PerCPU[cpuid].sys_fatal_flt.aflt_err1 = aerr1; \
	}
#else MULTIPROCESSOR

#define	SYS_FATAL_FLT(TYPE) \
	if (atom_tas(system_fatal)) { \
		sys_fatal_flt.aflt_type = TYPE; \
		sys_fatal_flt.aflt_subtype = subtype; \
		sys_fatal_flt.aflt_stat = afsr; \
		sys_fatal_flt.aflt_addr0 = afar0; \
		sys_fatal_flt.aflt_addr1 = afar1; \
		sys_fatal_flt.aflt_err0 = aerr0; \
		sys_fatal_flt.aflt_err1 = aerr1; \
	}
#endif MULTIPROCESSOR

extern	void	mmu_log_module_err();
u_int	report_ce_log = 0;
u_int	log_ce_error = 0;

/*
 * Asynchronous fault handler.
 * This routine is called to handle a hard level 15 interrupt,
 * which is broadcasted to all CPUs.
 * All fatal failures are completely handled by this routine
 * (by panicing).  Since handling non-fatal failures would access
 * data structures which are not consistent at the time of this
 * interrupt, these non-fatal failures are handled later in a
 * soft interrupt at a lower level.
 */

sun4m_l15_async_fault(sipr)
	u_int sipr;	/* System Interrupt Pending Register */
{
	u_int afsr;		/* an Asynchronous Fault Status Register */
	u_int afar0;		/* first Asynchronous Fault Address Reg. */
	u_int afar1 = 0;	/* second Asynchronous Fault Address Reg. */
	u_int aerr0;		/* MXCC error register <63:32> */
	u_int aerr1;		/* MXCC error register <31:0> */
	u_int ftype;		/* fault type */

	extern void prom_reboot();

#ifdef	MULTIPROCESSOR
	register in_lock;

	in_lock = klock_knock();
#endif	MULTIPROCESSOR
	/*
	 * Handle the nofault case by setting pokefault so
	 * that the probe routine knows that something went
	 * wrong.  It is the responsibility of the probe routine
	 * to reset this variable.
	 * Yes, it is OK for all CPUs to do this.
	 */
	if (nofault)
		pokefault = 1;
	/*
	 * Don't let any of those sneaky other processors clear the sipr,
	 * atleast until we get to here, grumble!
	 */
#ifdef	MULTIPROCESSOR
	if (cpuid == 0) {
		register int ps = procset;
		register int i;
		for (i= 1; i<nmod; i++)
			if (ps & (1<<i))
				atom_wfs(PerCPU[i].aflt_sync);
		for (i=1; i<nmod; i++)
			if (ps & (1<<i))
				atom_clr(PerCPU[i].aflt_sync);
	} else {
		atom_set(PerCPU[cpuid].aflt_sync);
		atom_wfc(PerCPU[cpuid].aflt_sync);
	}
#endif	MULTIPROCESSOR

	/*
	 * If NONE of the interesting bits is set, then someone else
	 * took a watchdog and managed to clear his error. Dive back
	 * into the boot prom.
	 */
	if ((sipr & SIR_ASYNCFLT) == 0) {
#ifdef	MULTIPROCESSOR
		if (in_lock)
#endif	MULTIPROCESSOR
			printf("Level 15 Error: Watchdog Reset\n");
		(void) prom_stopcpu(0);
	}

	/*
	 * Heisenberg's uncertainty principle as applied to
	 * mem I/F asynchronous faults:	 Since the vac flush
	 * code that maintains consistency across various CPUs
	 * must be fast, it does not flush write buffers whenever
	 * it changes contexts temporarly.  A side effect is that
	 * it is not possible to tell which context is responsible
	 * for mem I/F faults.	Thus, if we ever wanted to do
	 * something other than panicing for timeout or bus errors,
	 * (for user accesses), we would still be forced to panic
	 * when the m-bus address does not have a page struct
	 * associated with it.
	 */
#ifdef	MULTIPROCESSOR
	if (in_lock) {
#endif 	MULTIPROCESSOR
	/*
	 * Handle ECC asynchronous faults first, because there is
	 * a minute possibility that this handler could cause
	 * further ECC faults (when storing non-fatal fault info
	 * in memory) before the first ECC fault is handled.
	 */
		if (sipr & SIR_ECCERROR)
			l15_ecc_async_flt();
		if (sipr & SIR_M2SWRITE)
			l15_mts_async_flt();
#ifdef VME
		if (sipr & SIR_VMEERROR)
			l15_vme_async_flt();
#endif VME
		if (sipr & SIR_MODERROR)
			atom_set(module_error);
#ifdef 	MULTIPROCESSOR
	}

	if (cpuid == 0) {
		register int ps = procset;
		register int i;
		for (i = 1; i < nmod; i++)
			if (ps & (1<<i))
				atom_wfs(PerCPU[i].aflt_sync);
		for (i = 1; i < nmod; i++)
			if (ps & (1<<i))
				atom_clr(PerCPU[i].aflt_sync);
	} else {
		atom_set(PerCPU[cpuid].aflt_sync);
		atom_wfc(PerCPU[cpuid].aflt_sync);
	}
#endif 	MULTIPROCESSOR

	if (atom_setp(module_error))
		l15_mod_async_flt();

#ifdef 	MULTIPROCESSOR
	if (cpuid == 0) {
		register int ps = procset;
		register int i;
		for (i = 1; i < nmod; i++)
			if (ps & (1<<i))
				atom_wfs(PerCPU[i].aflt_sync);
		atom_clr(module_error);
		for (i = 1; i < nmod; i++)
			if (ps & (1<<i))
				atom_clr(PerCPU[i].aflt_sync);
	} else {
		atom_set(PerCPU[cpuid].aflt_sync);
		atom_wfc(PerCPU[cpuid].aflt_sync);
	}
#endif	MULTIPROCESSOR

	/*
	 * if faults are expected, no further processing
	 * is necessary.
	 */
	if (nofault)
		return;

	/*
	 * If system_fatal is set, then cpu with klock takes care of
	 * panicing.  All other CPUs return to the boot prom
	 * for re-start recovery.
	 */
#ifdef	MULTIPROCESSOR
	if ((atom_setp(system_fatal)) && (in_lock)) {
		afsr = PerCPU[cpuid].sys_fatal_flt.aflt_stat;
		afar0 = PerCPU[cpuid].sys_fatal_flt.aflt_addr0;
		afar1 = PerCPU[cpuid].sys_fatal_flt.aflt_addr1;
		aerr0 = PerCPU[cpuid].sys_fatal_flt.aflt_err0;
		aerr1 = PerCPU[cpuid].sys_fatal_flt.aflt_err1;
		ftype = PerCPU[cpuid].sys_fatal_flt.aflt_type;

#else	MULTIPROCESSOR
	if (atom_setp(system_fatal)) {
		afsr = sys_fatal_flt.aflt_stat;
		afar0 = sys_fatal_flt.aflt_addr0;
		afar1 = sys_fatal_flt.aflt_addr1;
		aerr0 = sys_fatal_flt.aflt_err0;
		aerr1 = sys_fatal_flt.aflt_err1;
		ftype = sys_fatal_flt.aflt_type;

#endif	MULTIPROCESSOR
		printf("fatal system fault: sipr=%x\n", sipr);

		switch (ftype) {
		case AFLT_MODULE:
			printf("Async Fault from module: ");
			printf("afsr=%x afar=%x\n", afsr, afar0);
			mmu_log_module_err(afsr, afar0, aerr0, aerr1);
			break;

		case AFLT_MEM_IF:
			log_ce_error = 1;
			log_mem_err(afsr, afar0, afar1, (u_int) 0);
			printf("Control Registers:\n");
			printf("\tefsr = 0x%x, efar0 = 0x%x ", afsr, afar0);
			printf("efar1 = 0x%x\n", afar1);
			panic("memory error");
			/* NOTREACHED */

		case AFLT_M_TO_S:
			printf("Async Fault from M-to-S: ");
			printf("afsr=%x afar=%x\n", afsr, afar0);
			log_mtos_err(afsr, afar0);
			break;

#ifdef VME
		case AFLT_S_TO_VME:
			printf("Async Fault from S-to-VME: ");
			printf("afsr=%x afar=%x\n", afsr, afar0);
			log_stovme_err(afsr, afar0);
			break;
#endif VME

		default:
			printf("Unknown Fault type %d; ", ftype);
			printf("afsr=%x afar=%x,%x\n", afsr, afar0, afar1);
			break;
		}
		panic("Fatal Asynchronous Fault\n");
		/*
		 * never get here from panic
		 */
		atom_clr(system_fatal);
		/* NOTREACHED */
#ifdef	MULTIPROCESSOR
	} else if ((atom_setp(system_fatal)) && (!(in_lock))) {
		/*
		 * synchronization point: let the cpu w/kernel lock panic
		 */
		atom_wfc(system_fatal);
		(void) prom_stopcpu(0);
#endif	MULTIPROCESSOR
	}
}

l15_ecc_async_flt()
{
	u_int afsr;		/* an Asynchronous Fault Status Register */
	u_int afar0;		/* first Asynchronous Fault Address Reg. */
	u_int afar1;		/* second Asynchronous Fault Address Reg. */
	u_int subtype = 0;	/* module error fault subtype */
	u_int aerr0 = 0;	/* MXCC error register <63:32> */
	u_int aerr1 = 0;	/* MXCC error register <31:0> */

	/* gather ECC error information */
	afsr = *(u_int *)EFSR_VADDR;
	afar0 = *(u_int *)EFAR0_VADDR;
	afar1 = *(u_int *)EFAR1_VADDR;

	/* unlock these registers */
	*(u_int *)EFSR_VADDR = 0;

	/*
	 * Multiple errors, uncorrectable errors,
	 * and timeout errors are fatal to the system.
	 * One could argue that uncorrectable errors
	 * could simply be handled by killing off
	 * all processes which have mappings to the
	 * page affected, but this doesn't make
	 * much sence for ECC.	Besides, that
	 * approach might cause flakey behaviour
	 * when certain essential system daemons
	 * are killed off.
	 */
	if (nofault) {
		;
#ifdef SUN4M_690
	} else if ((cpu == CPU_SUN4M_690) && (afsr & EFSR_TO)) {
		SYS_FATAL_FLT(AFLT_MEM_IF);
#endif SUN4M_690
#ifdef SUN4M_50
	} else if ((cpu == CPU_SUN4M_50) &&
		(afsr & (EFSR_SE | EFSR_GE))) {
		SYS_FATAL_FLT(AFLT_MEM_IF);
#endif SUN4M_50
	} else if (afsr & EFSR_UE) {
		if (afar0 & EFAR0_S) {
			SYS_FATAL_FLT(AFLT_MEM_IF);
		} else {
			handle_aflt(cpuid, AFLT_MEM_IF, afsr, afar0, afar1);
		}
	} else if (afsr & EFSR_CE) {
		handle_aflt(cpuid, AFLT_MEM_IF, afsr, afar0, afar1);
	} else {
		SYS_FATAL_FLT(AFLT_MEM_IF);
	}
}

l15_mts_async_flt()
{
	u_int afsr;		/* an Asynchronous Fault Status Register */
	u_int afar0;		/* first Asynchronous Fault Address Reg. */
	u_int afar1 = 0;	/* second Asynchronous Fault Address Reg. */
	u_int subtype = 0;	/* module error fault subtype */
	u_int aerr0 = 0;	/* MXCC error register <63:32> */
	u_int aerr1 = 0;	/* MXCC error register <31:0> */

#ifdef	MULTIPROCESSOR
	extern struct sbus_vme_mm *mm_find_paddr();
	extern struct sbus_vme_mm *mm_base;
#endif	MULTIPROCESSOR

	/* gather MtoS error information */
	afsr = *(u_int *)MTS_AFSR_VADDR;
	afar0 = *(u_int *)MTS_AFAR_VADDR;

	/* unlock these registers */
	*(u_int *)MTS_AFSR_VADDR = 0;

	if (nofault) {
		;
#ifdef	MULTIPROCESSOR
	} else if (afsr & MTSAFSR_S) {
		if (((afsr & MTSAFSR_TO) || (afsr & MTSAFSR_BERR)) &&
		    (mm_find_paddr(MM_SBUS_DEVS, afar0, mm_base) != NULL)) {
			handle_aflt(cpuid, AFLT_M_TO_S, afsr, afar0, 0);
		} else {
			SYS_FATAL_FLT(AFLT_M_TO_S);
		}
#endif	MULTIPROCESSOR
	} else if (afsr & (MTSAFSR_ME | MTSAFSR_S)) {
		SYS_FATAL_FLT(AFLT_M_TO_S);
	} else {
		handle_aflt(cpuid, AFLT_M_TO_S, afsr, afar0, 0);
	}
}

#ifdef VME
l15_vme_async_flt()
{
	u_int afsr;		/* an Asynchronous Fault Status Register */
	u_int afar0;		/* first Asynchronous Fault Address Reg. */
	u_int afar1 = 0;	/* second Asynchronous Fault Address Reg. */
	u_int subtype = 0;	/* module error fault subtype */
	u_int aerr0 = 0;	/* MXCC error register <63:32> */
	u_int aerr1 = 0;	/* MXCC error register <31:0> */

#ifdef	MULTIPROCESSOR
	extern struct sbus_vme_mm *mm_find_paddr();
	extern struct sbus_vme_mm *mm_base;
#endif	MULTIPROCESSOR

	/* gather VME error information */
	afsr = *(u_int *)VFSR_VADDR;
	afar0 = *(u_int *)VFAR_VADDR;

	/* unlock these registers */
	*(u_int *)VFSR_VADDR = 0;

	if (nofault) {
		;
#ifdef	MULTIPROCESSOR
	} else if ((afsr & VFSR_S) && (!(afsr & VFSR_WB))) {
		if (mm_find_paddr(MM_VME_DEVS, afar0, mm_base) != NULL) {
			handle_aflt(cpuid, AFLT_S_TO_VME, afsr, afar0, afar1);
		} else {
			SYS_FATAL_FLT(AFLT_S_TO_VME)
		}
#endif	MULTIPROCESSOR
	} else if (afsr & (VFSR_ME | VFSR_S | VFSR_WB)) {
		SYS_FATAL_FLT(AFLT_S_TO_VME)
	} else {
		handle_aflt(cpuid, AFLT_S_TO_VME, afsr, afar0, afar1);
	}
}
#endif VME

extern	int	vac_parity_chk_dis();

l15_mod_async_flt()
{
	u_int afsr;		/* an Asynchronous Fault Status Register */
	u_int afar0;		/* first Asynchronous Fault Address Reg. */
	u_int afar1 = 0;	/* second Asynchronous Fault Address Reg. */
	u_int subtype = 0;	/* module error fault subtype */
	u_int aerr0 = 0;	/* MXCC error register <63:32> */
	u_int aerr1 = 0;	/* MXCC error register <31:0> */
	u_int aflt_regs[4];
	int is_watchdog;
	int parity_err;

	/*
	 * If the mmu's "watchdog reset" bit is set,
	 * dive back into the prom.
	 */
	if (mmu_chk_wdreset() != 0) {
		(void) prom_stopcpu(0);
	}

	/*
	 * read the asynchronous fault registers for all
	 * the MMU/cache modules of this cpu.  note that
	 * this code assumes there is a maximum of two
	 * such modules, because this is a hardware limit
	 * for Sun-4M.
	 * ROSS:
	 *	mod0:	afsr --> aflt_regs[0]
	 *		afar --> aflt_regs[1]
	 *	mod1:	afsr --> aflt_regs[2] (-1 if mod1 info not exist)
	 *		afar --> aflt_regs[3]
	 * VIKING only:
	 *	mod0:	mfsr --> aflt_regs[0]
	 *		mfar --> aflt_regs[1]
	 * VIKING/MXCC:
	 *	mod0:	mfsr --> aflt_regs[0]
	 *		mfar --> aflt_regs[1]
	 *		error register (<63:32>) --> aflt_regs[2]
	 *		error register (<31:0>) -->  aflt_regs[3]
	 */
	(void) mmu_getasyncflt((u_int *)aflt_regs);

	is_watchdog = 1;

	switch (mod_info[0].mod_type) {
	    case CPU_VIKING:
		/*
		 * If there is a viking only module, the async fault (l15)
		 * will occur in the case of watchdog reset (which has
		 * been taken care of) or SB error.
		 * In the case of viking only system, a l15 mod error
		 * will be broadcated for SB error after SB trap
		 * currently, we panic the system when trap occurs
		 * ignore SB case for now.
		 */
		is_watchdog = 0;
		break;

	    case CPU_VIKING_MXCC:
		/*
		 * a store buffer error (SB; bit 15 of MFSR) will
		 * cause both a trap 0x2b and a broadcast l15
		 * (module error) interrupt.  The trap will be taken
		 * first, but afterwards there will be a pending
		 * l15 interrupt waiting for this module.
		 * We may have to do something for SB error here.
		 * But right now, we just assume that trap handler
		 * will take care of the SB error.  ignore SB
		 * error for now.
		 */

		/*
		 * We check the AFSR for this module to see if it
		 * has a valid asynchronous fault.
		 * The Async fault information is kept in MXCC error register
		 */
		if (((afsr = aflt_regs[2]) != -1) && (afsr & MXCC_ERR_EV)) {
			afar0 = aflt_regs[3];
			parity_err = vac_parity_chk_dis(aflt_regs[0], afsr);
			if (nofault && !parity_err) {
				;
			} else {
				SYS_FATAL_FLT(AFLT_MODULE)
			}
			is_watchdog = 0;
		}
		break;

		/*
		 * check second module here
		 */

	    default: /* ROSS case */
	  	/*
		 * Check the first (and possibly only) module for
		 * this CPU.  This module has the lowest MID
		 */
		if ((afsr = aflt_regs[0]) & AFSREG_AFV) {
			afar0 = aflt_regs[1];
			subtype = AFLT_LOMID;

			/*
			 * MODULE asynchronous faults are always
			 * fatal, unless we have nofault set,
			 * because we can't tell whether it was
			 * due to a supervisor access or not, and
			 * because even a user access might mean
			 * that the cache consistency/snooping
			 * could have been compromised.
			 */
			if (nofault) {
				;
			} else {
				SYS_FATAL_FLT(AFLT_MODULE)
			}
			is_watchdog = 0;
		}

		/*
		 * If there is only one module for this CPU,
		 * mmu_getasyncflt will store a -1 in aflt_regs[2].
		 * If not -1 then we check the AFSR for this second
		 * module to see if it has a valid asynchronous
		 * fault.
		 */
		if (((afsr = aflt_regs[2]) != -1) && (afsr & AFSREG_AFV)) {
			afar0 = aflt_regs[3];

			subtype = AFLT_HIMID;

			if (nofault) {
				;
			} else {
				SYS_FATAL_FLT(AFLT_MODULE)
			}
			is_watchdog = 0;
		}
		break;
	}

	/*
	 * If we had a "MODERR" bit set, and did not
	 * see our watchdog bit, and did not see any
	 * async fault valid stuff, then assume someone
	 * else got a watchdog. Bletch.
	 */
	if (is_watchdog) {
		(void) prom_stopcpu(0);
	}
}

/*
 * This routine is called by autovectoring a soft level 12
 * interrupt, which was sent by handle_aflt to process a
 * non-fatal asynchronous fault.
 */
process_aflt()
{
	u_int tail;
	u_int afsr;
	u_int afar0;
	u_int afar1;
	int ce;
	extern int report_ce_console;
	extern int noprintf;

#ifdef	MULTIPROCESSOR
	struct sbus_vme_mm *mmp, *smmp;
	extern struct sbus_vme_mm *mm_find_paddr();
	extern struct sbus_vme_mm *mm_base;
#endif	MULTIPROCESSOR

	ce = 0;
	/*
	 * While there are asynchronous faults which have
	 * accumulated, process them.
	 * There shouldn't be a problem with the race condition
	 * where this loop has just encountered the exit
	 * condition where a_head == a_tail, but before process_aflt
	 * returns it is interrupted by a level 15 interrupt
	 * which stores another asynchronous fault to process.
	 * In this (rare) case, we will just take another level 13
	 * interrupt after returning from the current one.
	 */
#ifdef MULTIPROCESSOR
	while (PerCPU[cpuid].a_head != PerCPU[cpuid].a_tail) {
		/*
		 * Point to the next asynchronous fault to process.
		 */
		tail = ++(PerCPU[cpuid].a_tail) % MAX_AFLTS;
		PerCPU[cpuid].a_tail = tail;

		afsr = PerCPU[cpuid].a_flts[tail].aflt_stat;
		afar0 = PerCPU[cpuid].a_flts[tail].aflt_addr0;
		afar1 = PerCPU[cpuid].a_flts[tail].aflt_addr1;

		/*
		 * Process according to the type of write buffer
		 * fault.
		 */
		switch (PerCPU[cpuid].a_flts[tail].aflt_type) {
#else  MULTIPROCESSOR
	while (a_head[cpuid] != a_tail[cpuid]) {
		/*
		 * Point to the next asynchronous fault to process.
		 */
		tail = ++(a_tail[cpuid]) % MAX_AFLTS;
		a_tail[cpuid] = tail;

		afsr = a_flts[cpuid][tail].aflt_stat;
		afar0 = a_flts[cpuid][tail].aflt_addr0;
		afar1 = a_flts[cpuid][tail].aflt_addr1;

		/*
		 * Process according to the type of write buffer
		 * fault.
		 */
		switch (a_flts[cpuid][tail].aflt_type) {
#endif MULTIPROCESSOR

		case	AFLT_MEM_IF:
			if (afsr & EFSR_CE) {
				ce = 1;
				if (report_ce_log || report_ce_console)
					log_ce_error = 1;
			}
			log_mem_err(afsr, afar0, afar1, (u_int) 0);
			if (afsr & EFSR_UE){
				if (fix_nc_ecc(afsr, afar0, afar1) == -1) {
					printf("AFSR = 0x%x, ", afsr);
					printf("AFAR0 = 0x%x, AFAR1 = 0x%x\n",
						afar0, afar1);
					panic("Asynchronous fault");

				}
			}

			break;
		case	AFLT_M_TO_S:
			printf("M-to-S Asynchronous fault ");
			printf("due to user write - non-fatal\n");
			log_mtos_err(afsr, afar0);
			/*
			 * We want to kill off the process that
			 * performed the write that caused this
			 * asynchronous fault.	Since all asynchronous
			 * faults are handled here before we switch
			 * processes, we know which process is
			 * responsible.	 The case where the cache
			 * flush code causes write buffers to fill
			 * (and async. fault later occurrs in the
			 * orig. context) is not a problem, because
			 * only the AFLT_MEM_IF case is affected
			 * since only main memory is cached in sun4m
			 */
#ifdef MULTIPROCESSOR
			/*
			 * XXX - check hat_oncpu to see if proc was running
			 */
			smmp = mm_base;
			while ((mmp = mm_find_paddr(MM_SBUS_DEVS, afar0, smmp))
			    != NULL) {
				psignal(mmp->mm_p, SIGBUS);
				uunix->u_code = FC_HWERR;
				uunix->u_addr = (char *)afar0;
				smmp = mmp->mm_next;
			}
#else  MULTIPROCESSOR
			psignal(uunix->u_procp, SIGBUS);
			uunix->u_code = FC_HWERR;
			uunix->u_addr = (char *)afar0;
#endif MULTIPROCESSOR
			break;
#ifdef VME
		case	AFLT_S_TO_VME:
			/*
			 * There's not much we can do in this case,
			 * since we can't even tell which CPU issued
			 * the write which caused the asynchronous
			 * fault.
			 */
			printf("VME Asynchronous fault ");
			printf("due to user write - non-fatal\n");
			log_stovme_err(afsr, afar0);
#ifdef MULTIPROCESSOR
			/*
			 * XXX - check hat_oncpu to see if proc was running
			 */
			smmp = mm_base;
			while ((mmp = mm_find_paddr(MM_VME_DEVS, afar0, smmp))
			    != NULL) {
				psignal(mmp->mm_p, SIGBUS);
				uunix->u_code = FC_HWERR;
				uunix->u_addr = (char *)afar0;
				smmp = mmp->mm_next;
			}
#else  MULTIPROCESSOR
			psignal(uunix->u_procp, SIGBUS);
			uunix->u_code = FC_HWERR;
			uunix->u_addr = (char *)afar0;
#endif MULTIPROCESSOR
			break;
#endif VME

		default:
			/*
			 * Since these are supposed to be non-fatal
			 * asynchronous faults, just ignore any
			 * bogus ones.
			 */
			break;
		}
		/*
		 * Nitty-Gritty details can be obtained from the
		 * asynchronous faults registers, and may be useful
		 * by an "expert."
		 */
		if (!ce || report_ce_console){	/* Do not report CE ecc error */
			printf("AFSR = 0x%x, AFAR0 = 0x%x, AFAR1 = 0x%x\n",
				afsr, afar0, afar1);
		} else {
			if (log_ce_error) {
				noprintf = 1;
				printf("AFSR = 0x%x, AFAR0 = 0x%x, AFAR1 = 0x%x\n",
					afsr, afar0, afar1);
				noprintf = 0;
			}
		}
		ce = 0;
		log_ce_error = 0;
	}
	return (1);
}

/*
 * This routine is called by autovectoring a soft level 12
 * interrupt, which was sent by handle_aflt to process a
 * non-fatal asynchronous fault.
 */

/*
 * async() calls this routine to store info. for a particular
 * non-fatal asynchronous fault in this CPU's queue of
 * asynchronous faults to be handled, and to invoke a level-12
 * interrupt thread to process this fault.
 */
handle_aflt(cpu_id, type, afsr, afar0, afar1)
	int cpu_id;	/* CPU responsible for this fault */
	int type;	/* type of asynchronous fault */
	u_int afsr;	/* Asynchronous Fault Status Register */
	u_int afar0;	/* Asynchronous Fault Address Register 0 */
	u_int afar1;	/* Asynchronous Fault Address Register 1 */
{
	int head;

	/*
	 * Increment the head of the circular queue of fault
	 * structures by 1 (modulo MAX_AFLTS).
	 */
#ifdef MULTIPROCESSOR
	head = ++(PerCPU[cpu_id].a_head) % MAX_AFLTS;
	PerCPU[cpu_id].a_head = head;
	if (head == PerCPU[cpu_id].a_tail)
		panic("Overflow of asynchronous faults.\n");
		/* NOTREACHED */

	/*
	 * Store the asynchronous fault information supplied.
	 */
	PerCPU[cpu_id].a_flts[head].aflt_type = type;
	PerCPU[cpu_id].a_flts[head].aflt_stat = afsr;
	PerCPU[cpu_id].a_flts[head].aflt_addr0 = afar0;
	PerCPU[cpu_id].a_flts[head].aflt_addr1 = afar1;
#else  MULTIPROCESSOR
	head = ++(a_head[cpu_id]) % MAX_AFLTS;
	a_head[cpu_id] = head;
	if (head == a_tail[cpu_id])
		panic("Overflow of asynchronous faults.\n");
		/* NOTREACHED */

	/*
	 * Store the asynchronous fault information supplied.
	 */
	a_flts[cpu_id][head].aflt_type = type;
	a_flts[cpu_id][head].aflt_stat = afsr;
	a_flts[cpu_id][head].aflt_addr0 = afar0;
	a_flts[cpu_id][head].aflt_addr1 = afar1;
#endif MULTIPROCESSOR

	/*
	 * Send a soft directed interrupt at level AFLT_HANDLER_LEVEL
	 * to onesself to process this non-fatal asynchronous
	 * fault. This soft int will be forwarded to the holder
	 * of the kernel lock, if necessary.
	 */
	send_dirint(cpu_id, AFLT_HANDLER_LEVEL);
}

char *nameof_siz[] = {
	"byte", "short", "word", "double", "quad", "line", "[siz=6]", "[siz=7]",
};

char *nameof_ssiz[] = {
	"word", "byte", "short", "[siz=3]", "quad", "line", "[siz=6]", "double",
};

log_mtos_err(afsr, afar0)
	u_int afsr;	/* Asynchronous Fault Status Register */
	u_int afar0;	/* Asynchronous Fault Address Register */
{
	if (afsr & MTSAFSR_ME)
		printf("\tMultiple Errors\n");
	if (afsr & MTSAFSR_S)
		printf("\tError during Supv mode cycle\n");
	else
		printf("\tError during User mode cycle\n");
	if (afsr & MTSAFSR_LE)
		printf("\tLate Error\n");
	if (afsr & MTSAFSR_TO)
		printf("\tTimeout Error\n");
	if (afsr & MTSAFSR_BERR)
		printf("\tBus Error\n");
	printf("\tRequested transaction: %s%s at %x:%x for mid %x\n",
		afsr&MTSAFSR_S ? "supv " : "user ",
		nameof_siz[(afsr & MTSAFSR_SIZ) >> MTSAFSR_SIZ_SHFT],
		afsr & MTSAFSR_PA, afar0,
		(afsr&MTSAFSR_MID) >> MTSAFSR_MID_SHFT);
	printf("\tSpecific cycle: %s at %x:%x\n",
		nameof_ssiz[(afsr & MTSAFSR_SSIZ) >> MTSAFSR_SSIZ_SHFT],
		afsr & MTSAFSR_PA,
		((afar0 & ~0x1F) | ((afsr & MTSAFSR_SA) >> MTSAFSR_SA_SHFT)));
}

#ifdef VME
log_stovme_err(afsr, afar0)
	u_int afsr;	/* Asynchronous Fault Status Register */
	u_int afar0;	/* Asynchronous Fault Address Register */
{
	if (afsr & VFSR_TO) {
		printf("\tVME address %x Timed Out\n", afar0);
	} else
	if (afsr & VFSR_BERR) {
		printf("\tVME address %x received a BERR.\n", afar0);
	} else
	if (afsr & VFSR_WB) {
		printf("\tIO Cache Write-Back Error at address %x\n", afar0);
	} else
		printf("\tStrange S-to-VME error at %x\n", afar0);
}
#endif VME

/*
 * This table used to determine which bit(s) is(are) bad when an ECC
 * error occurrs.  The array is indexed by the 8-bit syndrome which
 * comes from the ECC Memory Fault Status Register.  The entries
 * of this array have the following semantics:
 *
 *	00-63	The number of the bad bit, when only one bit is bad.
 *	64	ECC bit C0 is bad.
 *	65	ECC bit C1 is bad.
 *	66	ECC bit C2 is bad.
 *	67	ECC bit C3 is bad.
 *	68	ECC bit C4 is bad.
 *	69	ECC bit C5 is bad.
 *	70	ECC bit C6 is bad.
 *	71	ECC bit C7 is bad.
 *	72	Two bits are bad.
 *	73	Three bits are bad.
 *	74	Four bits are bad.
 *	75	More than Four bits are bad.
 *	76	NO bits are bad.
 * Based on "Galaxy Memory Subsystem SPECIFICATION" rev 0.6, pg. 28.
 */
char ecc_syndrome_tab[] =
{
76, 64, 65, 72, 66, 72, 72, 73, 67, 72, 72, 73, 72, 73, 73, 74,
68, 72, 72, 32, 72, 57, 75, 72, 72, 37, 49, 72, 40, 72, 72, 44,
69, 72, 72, 33, 72, 61,	 4, 72, 72, 75, 53, 72, 45, 72, 72, 41,
72,  0,	 1, 72, 10, 72, 72, 75, 15, 72, 72, 75, 72, 73, 73, 72,
70, 72, 72, 42, 72, 59, 39, 72, 72, 75, 51, 72, 34, 72, 72, 46,
72, 25, 29, 72, 27, 74, 72, 75, 31, 72, 74, 75, 72, 75, 75, 72,
72, 75, 36, 72,	 7, 72, 72, 54, 75, 72, 72, 62, 72, 48, 56, 72,
73, 72, 72, 75, 72, 75, 22, 72, 72, 18, 75, 72, 73, 72, 72, 75,
71, 72, 72, 47, 72, 63, 75, 72, 72,  6, 55, 72, 35, 72, 72, 43,
72,  5, 75, 72, 75, 72, 72, 50, 38, 72, 72, 58, 72, 52, 60, 72,
72, 17, 21, 72, 19, 74, 72, 75, 23, 72, 74, 75, 72, 75, 75, 72,
73, 72, 72, 75, 72, 75, 30, 72, 72, 26, 75, 72, 73, 72, 72, 75,
72,  8, 13, 72,	 2, 72, 72, 73,	 3, 72, 72, 73, 72, 75, 75, 72,
73, 72, 72, 73, 72, 75, 16, 72, 72, 20, 75, 72, 75, 72, 72, 75,
73, 72, 72, 73, 72, 75, 24, 72, 72, 28, 75, 72, 75, 72, 72, 75,
74, 12,	 9, 72, 14, 72, 72, 75, 11, 72, 72, 75, 72, 75, 75, 74,
};


extern int noprintf;

#define	MAX_CE_ERR	255

struct	ce_info {
	char	name[8];
	u_int	cnt;
};

#define	MAX_SIMM	256
struct	ce_info	mem_ce_simm[MAX_SIMM] = 0;


log_ce_mem_err(afsr, afar0, afar1, get_unum)
	u_int afsr;	/* ECC Memory Fault Status Register */
	u_int afar0;	/* ECC Memory Fault Address Register 0 */
	u_int afar1;	/* ECC Memory Fault Address Register 1 */
	char *(*get_unum) ();
{
	int i, size, block, offset;
	u_int aligned_afar1;
	char *unum;

	u_int syn_code; /* Code from ecc_syndrome_tab */

        /*
	 * Use the 8-bit syndrome in the EFSR to index
	 * ecc_syndrome_tab to get code indicating which bit(s)
	 * is(are) bad.
	 */
	syn_code = ecc_syndrome_tab[((afsr & EFSR_SYND) >> EFSR_SYND_SHFT)];

	/*
	 * The EFAR contains the address of an 8-byte group. For 16 and 32
	 * bytes brust, the address is the start of the brust group. So
	 * added the proper offset according to the info given in the
	 * EFSR and the syndrome table.
	 */

	offset = 0;
	/*
	 * Which of the 8 byte group during 16 bytes and 32 bytes
	 * transfer
	 */
	block = (afsr & EFSR_DW) >> 4;

	size = (afar0 & EFAR0_SIZ) >> 8;
	switch (size){
		case 4:
		case 5:   offset = block*8;
			break;
		default:  offset = 0; break;
	}

	/*
	 * For some reason, the EFAR sometimes contains a non-8 byte aligned
	 * address. We should ignore the lowest 3 bits
	 */
	aligned_afar1 = afar1 & 0xFFFFFFF8;

        if (syn_code < 72) {
	/*
	 * Syn_code contains the bit no. of the group that is bad.
	 */
		if (syn_code < 64)	
			offset = offset + (7 - syn_code/8);
		else offset = offset + (7 - syn_code%8);

		if ((unum = (*get_unum)(aligned_afar1 + (u_int) offset,
			afar0 & EFAR0_PA)) == (char *)0)
			printf("simm decode function failed\n");
	} else {
		for (i = 0; i < 8; i++){
			if ((unum = (*get_unum)(aligned_afar1 + offset +
				(u_int) i, afar0 & EFAR0_PA)) == (char *)0)
				printf("simm decode function failed\n");
		}
	}
	for (i = 0; i < MAX_SIMM; i++) {
		if (mem_ce_simm[i].name[0] == NULL) {
			(void) strcpy(mem_ce_simm[i].name, unum);
			mem_ce_simm[i].cnt++;
			break;
		} else if (!strcmp(unum, mem_ce_simm[i].name)) {
			if (++mem_ce_simm[i].cnt > MAX_CE_ERR) {
				printf("Multiple softerrors: ");
				printf("Seen %x Corrected Softerrors ",
					mem_ce_simm[i].cnt);
				printf("from SIMM %s\n", unum);
				printf("Consider replacing the SIMM.\n");
				mem_ce_simm[i].cnt = 0;
				log_ce_error = 1;
			}
			break;
		}
	}
	if (i >= MAX_SIMM)
		printf("Softerror: mem_ce_simm[] out of space.\n");
	if (log_ce_error) {
        	if (syn_code < 72)
			printf("\tCorrected SIMM at: %s\n", unum);
		else
			printf("\tPossible Corrected SIMM at: %s\n", unum);
		printf("\toffset is %d\n", offset);
		if (syn_code < 64)
			printf("\tBit %2d was corrected\n", syn_code);
		else if (syn_code < 72)
			printf("\tECC Bit %2d was corrected\n", syn_code - 64);
		else{
	    	    switch (syn_code) {
			case 72:
				printf("\tTwo bits were corrected\n");
				break;
			case 73:
				printf("\tThree bits were corrected\n");
				break;
			case 74:
				printf("\tFour bits were corrected\n");
				break;
			case 75:
				printf("\tMore than Four bits were corrected\n");
				break;
			default:
				break;
	    	    }
		}
	}
}

log_ue_mem_err(afar0, afar1, get_unum)
	u_int afar0;	/* ECC Memory Fault Address Register 0 */
	u_int afar1;	/* ECC Memory Fault Address Register 1 */
	char *(*get_unum) ();
{
	char *unum;

	if ((unum = (*get_unum)(afar1, afar0 & EFAR0_PA)) == (char *)0)
		printf("simm decode function failed\n");
	else
		printf("\tUncorrected SIMM at: %s\n", unum);
}

log_mem_err(afsr, afar0, afar1, ue_on_read)
	u_int afsr;	/* ECC Memory Fault Status Register */
	u_int afar0;	/* ECC Memory Fault Address Register 0 */
	u_int afar1;	/* ECC Memory Fault Address Register 1 */
	u_int ue_on_read;
{
	char *(*get_unum) ();

	if (!ue_on_read) {
		/*
	 	 * don't printf any corrected ecc info on the console
	 	 * only log to the log file
	 	 * report_ce_console is default to 0
	 	 * print on the console when report_ce_console is set
	 	 */
		if ((afsr & EFSR_CE) && log_ce_error) {
			if (report_ce_console)
				noprintf = 0;
			else
				noprintf = 1;
			printf("Softerror: ECC Memory Error Corrected.\n");
		}

		if (afsr & EFSR_ME)
			printf("\tMultiple errors.\n");


		switch (cpu) {
#ifdef SUN4M_50
		case CPU_SUN4M_50:
			if (afsr & EFSR_GE)
				printf("Graphics Error.\n");
			if (afsr & EFSR_SE)
				printf("Misreferenced Slot Error.\n");
			break;
#else SUN4M_690
		case CPU_SUN4M_690:
			if (afsr & EFSR_TO) {
				printf("Timeout on Write access to ");
				printf("expansion memory.\n");
			}
			break;
#endif SUN4M_50
		}
		if (afsr & EFSR_UE)
			printf("Uncorrectable ECC Memory Error.\n");
	} else
		printf("Uncorrectable ECC Memory Error.\n");

	(void) prom_getprop(prom_nextnode(0), "get-unum", (caddr_t)&get_unum);
	if (get_unum == (char *(*)())0)
		printf("no simm decode function available\n");
	else if ((!ue_on_read) && (afsr & EFSR_CE))
		log_ce_mem_err(afsr, afar0, afar1, get_unum);
	else if ((ue_on_read) || (afsr & EFSR_UE))
		log_ue_mem_err(afar0, afar1, get_unum);

	if ((ue_on_read) || (!(afsr & EFSR_CE)) || log_ce_error)
		printf("\tPhysical Address = %x:%x\n", afar0 & EFAR0_PA, afar1);

	if (noprintf) noprintf = 0;
}

/*
 * Kernel VM initialization.
 * Assumptions about kernel address space ordering:
 * (yes, there are some gaps)
 *   o	user space
 *   o	kernel text
 *   o	kernel data/bss
 *   o	kernel data structures
 *   o	Per-Cpu area for "this" processor	[0xFD0xxxxx]
 *   o	Per-Cpu areas for all processors	[0xFExxxxxx]
 *   o	debugger (optional)
 *   o	monitor
 *   o	dvma
 *   o	devices, u area
 */
kvm_init()
{
	register addr_t v;
	union ptpe *ptpe;
	extern char start[], etext[];

	/*
	 * Put the kernel segments in kernel address space. Make them
	 * "kernel memory" segment objects. We make the "text" segment
	 * include all the fixed location devices because there are no
	 * soft ptes backing up those mappings.
	 */
	(void) seg_attach(&kas, (addr_t)KERNELBASE, (u_int)(vvm - KERNELBASE),
			&ktextseg);
	(void) segkmem_create(&ktextseg, (caddr_t)NULL);
	(void) seg_attach(&kas, (addr_t)vvm, (u_int)
			((u_int)econtig - (u_int)vvm), &kvalloc);
	(void) segkmem_create(&kvalloc, (caddr_t)NULL);
	(void) seg_attach(&kas, (addr_t)HEAPBASE, (u_int)(BUFLIMIT - HEAPBASE),
			&bufheapseg);
	(void) segkmem_create(&bufheapseg, (caddr_t)Heapptes);
#ifdef	MULTIPROCESSOR
	(void) seg_attach(&kas, (addr_t)VA_PERCPUME, 0x1000000, &kpercpume);
	(void) segkmem_create(&kpercpume, (caddr_t)NULL);
	(void) seg_attach(&kas, (addr_t)VA_PERCPU, (u_int) (nmod*PERCPUSIZE),
		&kpercpu);
	(void) segkmem_create(&kpercpu, (caddr_t)NULL);
#endif	MULTIPROCESSOR
	(void) seg_attach(&kas, (addr_t)SYSBASE,
			(u_int)Syslimit - SYSBASE, &kseg);
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
		if ((u_int)v >= (((u_int)&msgbuf) & PAGEMASK) &&
			(u_int)v <= (((u_int)&msgbuf + sizeof (struct msgbuf)) &
				PAGEMASK))
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
	/*
	 * The following invalidate segu, and bufheapseg
	 */
#ifdef	MULTIPROCESSOR
	/*
	 * Invalid to VA_PERCPUME
	 * skip VA_PERCPUME -> (VA_PERCPUME + nmod*PERCPUSIZE)
	 */
	(void) as_setprot(&kas, v, VA_PERCPUME - (u_int)v, 0);
	v = (addr_t)roundup((u_int)(VA_PERCPU + nmod*PERCPUSIZE), PAGESIZE);
#endif	MULTIPROCESSOR

	/*
	 * Invalid to Syslimit.
	 */
	(void) as_setprot(&kas, v, (u_int)(Syslimit - v), 0);
	v = (addr_t)Syslimit;
	/*
	 * We rely on the fact that all the kernel ptes in the upper segment
	 * are in order, so we only need to locate them once.
	 */
	ptpe = hat_ptefind(&kas, v);
	/*
	 * Invalid until the Debug and monitor area
	 * Note: running this up to the end was a bad idea,
	 * it caused us to walk off the end of "ptes" and into
	 * the "ctxs" array. This may be due to the fact that
	 * we are giving the last of eight megs to the fakeprom,
	 * but who knows.
	 */
	for (; (u_int)v < (u_int)DEBUGSTART; v += MMU_PAGESIZE, ptpe++)
		ptpe->pte.EntryType = MMU_ET_INVALID;
	/*
	 * Now create a segment for the DVMA virtual
	 * addresses using the segkmem segment driver.
	 */
	(void) seg_attach(&kas, DVMA, (u_int)mmu_ptob(dvmasize), &kdvmaseg);
	(void) segkmem_create(&kdvmaseg, (caddr_t)NULL);
	/*
	 * Flush the PDC of any old mappings.
	 */
	mmu_flushall();
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
	union {
		struct pte u_pte;
		int u_ipte;
	} pte;
	int dohat;
	int s;

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
		(void) splx(s);
		va = Sysbase + mmu_ptob(a);
		hat_pteload(&kseg, va, (struct page *)NULL, &mmu_pteinvalid, 1);
	}

	if (!pp->p_mapping) {
		pte.u_pte.PhysicalPageNumber = page_pptonum(pp);
		pte.u_pte.AccessPermissions = MMU_STD_SRWX;
		pte.u_pte.EntryType = MMU_ET_PTE;
		hat_pteload(&kseg, va, (struct page *)NULL,
			&pte.u_pte, PTELD_LOCK | PTELD_NOSYNC);
		dohat = 0;
	} else {
		hat_mempte(pp, PROT_READ | PROT_WRITE, &pte.u_pte, va);
		hat_pteload(&kseg, va, pp, &pte.u_pte,
		    PTELD_LOCK | PTELD_NOSYNC);
		dohat = 1;
	}

	(void) copyin((caddr_t)addr, va, PAGESIZE);

	if (dohat) {
		hat_unload(&kseg, va, PAGESIZE);
	} else {
		vac_pageflush(va);
		hat_pteload(&kseg, va, (struct page *)NULL,
			&mmu_pteinvalid, PTELD_LOCK | PTELD_NOSYNC);
	}

	if ((va_cache_valid & VA_CACHE_MASK) == VA_CACHE_MASK) {
		/* cache is full, just return the virtual space */
		hat_unlock(&kseg, va);
		s = splhigh();
		rmfree(kernelmap, (long)CLSIZE, (u_long)btokmx(va));
		(void) splx(s);
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
	int dohat;
	union {
		struct pte u_pte;
		int u_ipte;
	} pte;

	ASSERT((int)len > 0 && (int)off >= 0 && off + len <= PAGESIZE);

	if (!pp->p_mapping) {
		pte.u_pte.PhysicalPageNumber = page_pptonum(pp);
		pte.u_pte.EntryType = MMU_ET_PTE;
		pte.u_pte.AccessPermissions = MMU_STD_SRWX;
		hat_pteload(&kseg, CADDR2, (struct page *)NULL,
			&pte.u_pte, PTELD_LOCK | PTELD_NOSYNC);
		dohat = 0;
	} else {
		hat_mempte(pp, PROT_READ | PROT_WRITE, &pte.u_pte,
			(addr_t)CADDR2);
		hat_pteload(&kseg, CADDR2, pp, &pte.u_pte,
		    PTELD_LOCK | PTELD_NOSYNC);
		dohat = 1;
	}

	bzero(CADDR2 + off, len);

	if (dohat) {
		hat_unload(&kseg, CADDR2, PAGESIZE);
	} else {
		vac_pageflush(CADDR2);
		hat_pteload(&kseg, CADDR2, (struct page *)NULL,
			&mmu_pteinvalid, PTELD_LOCK | PTELD_NOSYNC);
	}
}

#ifdef VMEMDEBUG
/*
 * Calculate the checksum for the physical page given by `pp'
 * without changing the reference and modified bits of page.
 * pagesum uses global CADDR2 and assumes that no one uses this
 * map at interrupt level and no one sleeps with an active mapping there.
 */
int
pagesum(pp)
	struct page *pp;
{
	int dohat;
	union {
		struct pte u_pte;
		int u_ipte;
	} pte;
	int checksum = 0;
	int *first, *last, *ip;

	if (!pp->p_mapping) {
		pte.u_pte.PhysicalPageNumber = page_pptonum(pp);
		pte.u_pte.EntryType = MMU_ET_PTE;
		pte.u_pte.AccessPermissions = MMU_STD_SRWX;
		hat_pteload(&kseg, CADDR2, (struct page *)NULL,
			&pte.u_pte, PTELD_LOCK | PTELD_NOSYNC);
		dohat = 0;
	} else {
		hat_mempte(pp, PROT_READ | PROT_WRITE, &pte.u_pte,
			(addr_t)CADDR2);
		hat_pteload(&kseg, CADDR2, pp, &pte.u_pte,
		    PTELD_LOCK | PTELD_NOSYNC);
		dohat = 1;
	}

	first = (int *)CADDR2;
	last = (int *)(CADDR2 + PAGESIZE);

	for (ip = first; ip < last; ip++)
		checksum += *ip;

	if (dohat) {
		hat_unload(&kseg, CADDR2, PAGESIZE);
	} else {
		vac_pageflush(CADDR2);
		hat_pteload(&kseg, CADDR2, (struct page *)NULL,
			&mmu_pteinvalid, PTELD_LOCK | PTELD_NOSYNC);
	}

	return (checksum);
}
#endif VMEMDEBUG

/*
 * Console put and get character routines.
 */
cnputc(c)
	register int c;
{
	register int s;

	s = spl7();
	if (c == '\n')
		prom_putchar('\r');
	prom_putchar(c);
	(void) splx(s);
}

int
cngetc()
{
	extern u_char prom_getchar();

	return ((int)prom_getchar());
}

caddr_t
kstackinit(stackbase)
	register caddr_t stackbase;
{
	register u_int sp;

	sp = KERNSTACK - SA(MINFRAME + sizeof (struct regs));
	sp = (u_int)stackbase + sp;
	if (sp & 0x7)
		sp -= 4;
	return ((caddr_t)sp);
}

/*
 * fork a kernel process at address "startaddr".
 * Used by main to create init and pageout.
 */
/* ARGSUSED */
kern_proc(startaddr, arg)
	void (*startaddr)();
	int arg;
{
	register struct seguser *newseg;
	register struct frame *fp;
	struct proc *pp;

	/* This segment may not be needed for some kernel processes. */
	newseg = (struct seguser *) segu_get();
	if (newseg == (struct seguser *) 0)
		panic("can't create kernel process");
	fp = (struct frame *)KSTACK(newseg);
	fp->fr_local[6] = (u_int)startaddr;	/* l6 gets start address */
	fp->fr_local[5] = 1;			/* means fork a kernel proc */
	fp->fr_local[0] = arg;			/* l0 gets the argument */
	pp = freeproc;
	freeproc = pp->p_nxt;
	newproc(0, (struct as *)0, (int)0, (struct file *)0, newseg, pp);
}

/*
 * For things that depend on register state being on the stack,
 * copy any register windows that get saved into the window buffer
 * (in the uarea) onto the stack.  This normally gets fixed up
 * before returning to a user program.	Callers of this routine
 * require this to happen immediately because a later kernel
 * operation depends on window state (like instruction simulation).
 */
flush_user_windows_to_stack()
{
	register int j, k;
	register char *sp;
	register struct pcb *pcb = &u.u_pcb;

	flush_user_windows();

	j = pcb->pcb_wbcnt;
	while (j > 0) {
		sp = pcb->pcb_spbuf[--j];
		if (((int)sp & (STACK_ALIGN - 1)) ||
		    copyout((caddr_t)&pcb->pcb_wbuf[j],
					sp, sizeof (struct rwindow))) {
			/* failed to copy out window, don't do anything */
		} else {
			/* succeded; move other windows down */
			pcb->pcb_wbcnt--;
			for (k = j; k < pcb->pcb_wbcnt; k++) {
				pcb->pcb_spbuf[k] = pcb->pcb_spbuf[k+1];
				bcopy((caddr_t)&pcb->pcb_wbuf[k+1],
					(caddr_t)&pcb->pcb_wbuf[k],
					sizeof (struct rwindow));
			}
		}
	}
}

/*
 * called by assembler files that want to CALL_DEBUG
 */
call_debug_from_asm()
{
	int	s;

	s = splzs();		/* protect against re-entrancy */
	(void) setjmp(&u.u_pcb.pcb_regs); /* for kadb */
	CALL_DEBUG();
	(void) splx(s);
}

/*
 * Send a directed interrupt of specified level to a cpu.
 */

send_dirint(cpuix, int_level)
int	cpuix;			/* cpu to be interrupted */
int	int_level;		/* interrupt level */
{
	INT_REGS->cpu[cpuix].set_pend = 1 << (int_level + IR_SOFT_SHIFT);
}

/*
 * Clear a directed interrupt of specified level on a cpu.
 */

clr_dirint(cpuix, int_level)
int	cpuix;			/* cpu to be interrupted */
int	int_level;		/* interrupt level */
{
	INT_REGS->cpu[cpuix].clr_pend = 1 << (int_level + IR_SOFT_SHIFT);
}

/*
 * Set Interrupt Target Register to the specified target.
 */

set_itr(cpuix)
int	cpuix;			/* cpu to receive the undirected interrupts */
{
	INT_REGS->itr = cpuix;
}

/*
 * Get Interrupt Target Register target.
 */

get_itr()
{
	return INT_REGS->itr;
}

#ifdef	MULTIPROCESSOR

struct {
	int		(*fn) ();
	int		a[5];
}		xc;

int		xc_ready = 0;	/* true when xc init is done */
int		xc_atnlvl = 0;	/* attention nest level */
int		xc_callmap = 0;	/* map of cpus at attention */

unsigned	xc_atnat = 0;	/* timestamp of last atn entry */

unsigned	xc_attns = 0;	/* number of attention interrupts */
unsigned	xc_servs = 0;	/* number of xc services requested */
unsigned	xc_attnt = 0;	/* microseconds inside xc attention */

xc_init()
{
	int	cix;

	for (cix = 0; cix < nmod; ++cix) {
		atom_init(PerCPU[cix].xc_doit);
		atom_clr(PerCPU[cix].xc_doit);
		atom_init(PerCPU[cix].xc_busy);
		atom_clr(PerCPU[cix].xc_busy);
	}
	xc_ready = 1;
}

xc_loop()
{
	register int    (*fn) ();
	register int    a0, a1, a2, a3, a4;

	extern int	klock;
/*
 * We may be here a while.
 * If the ITR points to us, then
 * forward it to the lock holder so
 * he has the option of allowing
 * interrupts to occur.
 */
	if (get_itr() == cpuid)
		set_itr(klock & 3);

/*
 * we are going to be here
 * for a while. swing the ITR
 * over to the master so the
 * system can take interrupts
 * if it wants to.
 */
	if (get_itr() == cpuid)
		set_itr(klock & 3);

	while (1) {
		atom_clr(xc_busy);
		atom_wfs(xc_doit);
		fn = xc.fn;
		a0 = xc.a[0];
		a1 = xc.a[1];
		a2 = xc.a[2];
		a3 = xc.a[3];
		a4 = xc.a[4];
		atom_clr(xc_doit);
		if (!fn)
			break;
		(*fn) (a0, a1, a2, a3, a4);
	}
}

xc_serv()
{
	register int    (*fn) ();
	register int    a0, a1, a2, a3, a4;
	register int	css;

	if (!xc_ready || !cpu_exists || !atom_setp(xc_doit))
		return;

	css = cpu_supv;
	cpu_supv = 3;

	fn = xc.fn;
	a0 = xc.a[0];
	a1 = xc.a[1];
	a2 = xc.a[2];
	a3 = xc.a[3];
	a4 = xc.a[4];
	atom_clr(xc_doit);
	if (fn)
		(*fn) (a0, a1, a2, a3, a4);
	atom_clr(xc_busy);
	cpu_supv = css;
}

xc_callsync()
{
	register int    map, bit;
	register int    cix;

	if (!xc_ready || !cpu_exists)
		return;

	map = xc_callmap;
	for (cix = 0, bit = 1; cix < nmod; ++cix, bit <<= 1)
		if (map & bit)
			atom_wfc(PerCPU[cix].xc_busy);
}

xc_callstart(o0, o1, o2, o3, o4, func)
	int	o0, o1, o2, o3, o4;
	int	(*func) ();
{
	register int    s;
	register int    map, bit;
	register int    cix;

	if (!xc_ready || !cpu_exists)
		return;

	s = spl8();

	if (xc_atnlvl)
		map = xc_callmap;
	else
		xc_callmap = map = procset & ~(1 << cpuid);

	for (cix = 0, bit = 1; cix < nmod; ++cix, bit <<= 1)
		if (map & bit)
			atom_wfc(PerCPU[cix].xc_busy);

	xc.a[0] = o0;
	xc.a[1] = o1;
	xc.a[2] = o2;
	xc.a[3] = o3;
	xc.a[4] = o4;
	xc.fn = func;

	for (cix = 0, bit = 1; cix < nmod; ++cix, bit <<= 1)
		if (map & bit) {
			atom_set(PerCPU[cix].xc_busy);
			atom_set(PerCPU[cix].xc_doit);
			if (xc_atnlvl == 0)
				send_dirint(cix, 15);
		}
	xc_servs++;
	for (cix = 0, bit = 1; cix < nmod; ++cix, bit <<= 1)
		if (map & bit)
			atom_wfc(PerCPU[cix].xc_doit);
	(void) splx(s);
}

xc_attention()
{
	register int s = spl8();

	if (xc_atnlvl == 0) {
		xc_attns++;
		xc_atnat = utimers.timer_lsw;
		xc_callstart(0, 0, 0, 0, 0, xc_loop);
	}
	xc_atnlvl++;
	(void) splx(s);

}

xc_dismissed()
{
	register int    s = spl8();

	if (xc_atnlvl == 1) {
		xc_callstart(0, 0, 0, 0, 0, (int (*)())0);
		xc_attnt += (utimers.timer_lsw - xc_atnat) >> 10;
	}
	xc_atnlvl--;
	(void) splx(s);
}

xc_noop()
{
}

int
xc_sync(o0, o1, o2, o3, o4, func)
	int	o0, o1, o2, o3, o4;
	int	(*func) ();
{
	register int    rv;

	if (!func)
		func = xc_noop;

	if (!xc_ready || !cpu_exists)
		return (*func) (o0, o1, o2, o3, o4);

	xc_callstart(o0, o1, o2, o3, o4, func);
	rv = (*func) (o0, o1, o2, o3, o4);
	xc_callsync();

	return (rv);
}

xc_oncpu(o0, o1, o2, o3, cix, func)
	int	o0, o1, o2, o3;
	int	cix;
	int	(*func) ();
{
	register int    s;

	if (cix == cpuid) {
		(*func) (o0, o1, o2, o3);
		return;
	}

	if ((cix < 0) || (cix >= NCPU) ||
	    !xc_ready || !cpu_exists ||
	    !(procset & (1<<cix)) ||
	    !PerCPU[cix].cpu_exists)
		return;

	s = spl8();
	atom_wfc(PerCPU[cix].xc_busy);
	xc.a[0] = o0;
	xc.a[1] = o1;
	xc.a[2] = o2;
	xc.a[3] = o3;
	xc.fn = func;
	atom_set(PerCPU[cix].xc_busy);
	atom_set(PerCPU[cix].xc_doit);
	if (xc_atnlvl == 0)
		send_dirint(cix, 15);
	xc_servs++;
	atom_wfc(PerCPU[cix].xc_busy);
	(void) splx(s);
}
#endif	MULTIPROCESSOR

#ifdef IOMMU
iom_init()
{
	union iommu_ctl_reg ioctl_reg;
	union iommu_base_reg iobase_reg;

	/* load iommu base addr */
	iobase_reg.base_uint = 0;
	iobase_reg.base_reg.base = ((u_int) phys_iopte) >> IOMMU_PTE_BASE_SHIFT;
	iommu_set_base(iobase_reg.base_uint);

	/* set control reg and turn it on */
	ioctl_reg.ctl_uint = 0;
	ioctl_reg.ctl_reg.enable = 1;
	ioctl_reg.ctl_reg.range = IOMMU_CTL_RANGE;
	iommu_set_ctl(ioctl_reg.ctl_uint);

	/* kindly flush all tlbs for any leftovers */
	iommu_flush_all();
}

/*
 * NOTE: this routine takes DVMA_ADDR as seen from IOMMU:
 *		dvma >= IOMMU_DVMA_BASE && dvma_addr <= 0xFFFFFFFF
 */
iom_dvma_pteload(mempfn, dvma_addr, flag)
int mempfn;
u_int dvma_addr;
int flag;

{
	union iommu_pte *piopte, tmp_pte;

	piopte = dvma_pfn_to_iopte(iommu_btop(dvma_addr - IOMMU_DVMA_BASE));

	/* load iopte */
	tmp_pte.iopte_uint = 0;

	tmp_pte.iopte.pfn = mempfn;
	tmp_pte.iopte.valid = 1;
	if (flag & IOM_WRITE)
		tmp_pte.iopte.write = 1;

	if ((cache == CACHE_PAC) || (cache == CACHE_PAC_E) ||
	    (vac && (flag & IOM_CACHE)))
		tmp_pte.iopte.cache = 1;

	*piopte = tmp_pte;

}

/*
 * NOTE: this routine takes map_addr returned by rmget().
 */
iom_pteload(mempfn, map_addr, flag, io_map)
int mempfn;
int map_addr;
int flag;
struct map *io_map;

{
	union iommu_pte *piopte, tmp_pte;

	if ((piopte = iom_ptefind(map_addr, io_map)) == NULL)
		panic("iom_pteload: bad map addr");

	/* load iopte */
	tmp_pte.iopte_uint = 0;

	tmp_pte.iopte.pfn = mempfn;
	tmp_pte.iopte.valid = 1;
	if (flag & IOM_WRITE)
		tmp_pte.iopte.write = 1;

	if ((cache == CACHE_PAC) || (cache == CACHE_PAC_E) ||
	    (vac && (flag & IOM_CACHE)))
		tmp_pte.iopte.cache = 1;

	*piopte = tmp_pte;

}

/*
 * NOTE: this routines take DVMA_ADDR as seen from IOMMU:
 *		dvma >= IOMMU_DVMA_BASE && dvma_addr <= 0xFFFFFFFF
 */
iom_dvma_unload(dvma_addr)
u_int dvma_addr;

{
	union iommu_pte *piopte;

	piopte = dvma_pfn_to_iopte(iommu_btop(dvma_addr - IOMMU_DVMA_BASE));

	/* in-line pteunload. invalid iopte */
	piopte-> iopte_uint = 0;

	/* flush old TLB */
	iommu_addr_flush((int) dvma_addr & IOMMU_FLUSH_MSK);

}

iom_pteunload(piopte)
union iommu_pte *piopte;

{
	/* invalid iopte */
	piopte-> iopte_uint = 0;

	/* flush old TLB */
	iommu_addr_flush((iopte_to_dvma(piopte)) & IOMMU_FLUSH_MSK);
}

iom_pagesync(piopte)
union iommu_pte *piopte;

{
	struct page *pp;

	pp = page_numtopp(piopte-> iopte.pfn);

	if (pp) {
		pp->p_ref = 1;
		if (piopte->iopte.write)
			pp->p_mod = 1;
	}
}

union iommu_pte*
iom_ptefind(map_addr, io_map)
int map_addr;
struct map *io_map;

{
	int dvma_pfn;

	dvma_pfn = map_addr_to_dvma_pfn(map_addr, io_map);
	return (dvma_pfn_to_iopte(dvma_pfn));
}

/*
 * this routine coverts address returned by rmget() to PFN RELATIVE
 * to the starting of IOMMU_DVMA_BASE.
 * NOTE: the map passed in is used as only a "hint", we
 *	 look at the map_addr to find out the exact map.
 */
map_addr_to_dvma_pfn(map_addr, io_map)
int map_addr;
struct map *io_map;

{

	/*
	 * map_addr is the address driver required from mb_xxx
	 * routines. It is not necessarily right address for IOMMU.
	 */


	if (io_map == sbusmap || io_map == bigsbusmap || io_map == mbutlmap) {
		if (map_addr >= SBUSMAP_BASE && map_addr <
		    (SBUSMAP_BASE+SBUSMAP_SIZE))
			/* sbusmap */
			return (iommu_btop(IOMMU_SBUS_IOPBMEM_BASE)
				+ map_addr - IOMMU_DVMA_DVFN);

		if (map_addr >= BIGSBUSMAP_BASE && map_addr <
		    (BIGSBUSMAP_BASE+BIGSBUSMAP_SIZE))
			return (map_addr - IOMMU_DVMA_DVFN);

		if (map_addr >= MBUTLMAP_BASE && map_addr <
		    (MBUTLMAP_BASE+MBUTLMAP_SIZE))
			return (map_addr - IOMMU_DVMA_DVFN);

		/* error */
		panic("map_addr_to_dvma_pfn: bad sbus map_addr");
		/*NOTREACHED*/
	}
	if (cpu == CPU_SUN4M_690) {
		if (io_map == vme24map || io_map == vme32map)
			return (map_addr + IOMMU_IOPBMEM_DVFN
				- IOMMU_DVMA_DVFN);
	} else if (io_map == altsbusmap) {
		if (map_addr >= ALTSBUSMAP_BASE && map_addr <
		    (ALTSBUSMAP_BASE+ALTSBUSMAP_SIZE))
			return (map_addr - IOMMU_DVMA_DVFN);
	}
	/* error!! */
	panic("map_addr_to_dvma_pfn: unknown map");
	/*NOTREACHED*/
}

/*
 * find the corresponding map for a given map_addr.
 * NOTE: the map passed in is as a "hint" only, we need to
 *	 look at the map_addr to find out the exact map.
 */
struct map *
map_addr_to_map(map_addr, map)
int map_addr;
struct map *map;

{

	if (map == sbusmap || map == bigsbusmap || map == mbutlmap) {
		if (map_addr >= SBUSMAP_BASE && map_addr <
				(SBUSMAP_BASE+SBUSMAP_SIZE))
			return (sbusmap);

		if (map_addr >= BIGSBUSMAP_BASE && map_addr <
				(BIGSBUSMAP_BASE+BIGSBUSMAP_SIZE))
			return (bigsbusmap);

		if (map_addr >= MBUTLMAP_BASE && map_addr <
				(MBUTLMAP_BASE+MBUTLMAP_SIZE))
			return (mbutlmap);

	} else { /* VME and alt.sbus maps */
		if (cpu == CPU_SUN4M_690) {
			if (map_addr >= VME24MAP_BASE && map_addr <
					(VME24MAP_BASE+VME24MAP_SIZE))
				return (vme24map);

			if (map_addr >= VME32MAP_BASE && map_addr <
					(VME32MAP_BASE+VME32MAP_SIZE))
				return (vme32map);
		} else {
			if (map_addr >= ALTSBUSMAP_BASE && map_addr <
					(ALTSBUSMAP_BASE+ALTSBUSMAP_SIZE))
				return (altsbusmap);
		}
	}

	return (NULL);
}

#endif IOMMU

#ifdef IOC
/*
 * NOTE: this routine is really a kludge now, since it assumes
 *	 each ioc line maps 2 pages of IOMMU and SRMMU. Should
 *	 be rewritten in a more general way.
 */

ioc_adj_start(pfn)
int pfn;

{
	if (ioc)
		return ((pfn & 0x1)? pfn - 1 : pfn);
	else
		return (pfn);
}

/*
 * NOTE: pfn passed in should INCLUDE the extra page of red-zone.
 *	 a kludge routine.
 */
ioc_adj_end(pfn)
int pfn;

{
	if (ioc)
		return ((pfn & 0x1)? pfn + 1 : pfn);
	else
		return (pfn);
}

static
map_addr_to_ioc_sel(map_addr)
int map_addr;

{
	int dvma_addr;

	dvma_addr = iommu_ptob(map_addr_to_dvma_pfn(map_addr, mb_hd.mh_map));
	return ((dvma_addr & IOC_ADDR_MSK) >> IOC_RW_SHIFT);
}

ioc_setup(map_addr, flags)
int map_addr;
int flags;

{
	int ioc_tag = 0;
	int ioc_addr;

	if (ioc) {
		ioc_addr = map_addr_to_ioc_sel(map_addr) | IOC_TAG_PHYS_ADDR;

		if (flags & IOC_LINE_LDIOC)
			ioc_tag = IOC_LINE_ENABLE;

		if (flags & IOC_LINE_WRITE)
			ioc_tag |= IOC_LINE_WRITE;

		do_load_ioc(ioc_addr, ioc_tag);
	}

}

ioc_flush(map_addr)
int map_addr;

{

	extern void flush_writebuffers();

	if (ioc) {
		fast_ioc_flush(map_addr);
		flush_writebuffers();	/* push incoming data to memory */
	}
}

fast_ioc_flush(map_addr)
int map_addr;

{

	int ioc_addr;

	if (ioc) {
		ioc_addr = map_addr_to_ioc_sel(map_addr) | IOC_FLUSH_PHYS_ADDR;
		do_flush_ioc(ioc_addr);

		/*
		 * FIXME: This read is to sync. with h/w flush to
		 * make sure it's really flushed.
		 */
		(void) ioc_read_tag(map_addr); /* sync with VIC */
	}
}

ioc_read_tag(map_addr)
int map_addr;

{
	int ioc_addr;
	int tv = 0;

	if (ioc) {
		ioc_addr = map_addr_to_ioc_sel(map_addr) | IOC_TAG_PHYS_ADDR;
		tv = do_read_ioc(ioc_addr);
	}
	return (tv);
}

ioc_init()
{
	int ioc_addr, i;

	/* invalidate all ioc lines */
	if (ioc) {
		for (i = 0; i < IOC_CACHE_LINES; i++) {
			ioc_addr = i * IOC_LINE_MAP;
			ioc_addr &= IOC_ADDR_MSK;
			ioc_addr >>= IOC_RW_SHIFT;
			ioc_addr |= IOC_TAG_PHYS_ADDR;
			do_load_ioc(ioc_addr, 0);
		}
	}
}

/*
 * Turn on the io cache during startup.
 */

iocache_on()
{
	(*VMEBUS_ICR) |= VMEICP_IOC;
}

/*
 * check to see if this transaction can be IOC'ed.
 * NOTE: this is the new scheme used in 4.1.1. B_IOCACHE is now obsolete.
 *
 * returns: IOC_LINE_INVALID: sbus/IOC is not on.
 *	    IOC_LINE_LDIOC: vme & IOCable
 *	    IOC_LINE_NOIOC: vme & NOT IOCable
 */
ioc_able(bp, io_map)
struct buf *bp;
struct map *io_map;

{
/* if ioc not present or disabled, don't think about it. */
	if (!ioc) {
		return (IOC_LINE_INVALID);
	}
/* if not from vme24map or vme32map, ignore it. */
	if ((io_map != vme24map) && (io_map != vme32map)) {
		return (IOC_LINE_INVALID);
	}

/* B_WRITE does not require ioc_flush, always cache!! */
	if (!(bp->b_flags & B_READ)) {
		return (IOC_LINE_LDIOC);
	}

/* if address badly aligned, dont cache it. */
	if ((u_int)bp->b_un.b_addr & IOC_LINEMASK) {
		return (IOC_LINE_NOIOC);
	}

/* if count badly aligned, dont cache it. */
	if (bp->b_bcount & IOC_LINEMASK) {
		return (IOC_LINE_NOIOC);
	}

/* properly aligned read: iocache it. */
	return (IOC_LINE_LDIOC);
}

#endif IOC

avail_at_vaddr(vaddr)
u_int vaddr;
{
	struct memlist *pmemptr;
	int rv = 0;

	for (pmemptr = virtmemory; pmemptr; pmemptr = pmemptr->next)
		if ((vaddr >= pmemptr-> address) &&
		    ((vaddr - pmemptr-> address) < pmemptr-> size)) {
			rv = pmemptr->size - (vaddr - pmemptr->address);
			break;
		}
	return (rv);
}

int
calc_memsize()
{
	struct memlist *memp;
	register int found;

	for (found = 0, memp = availmemory; memp; memp = memp->next)
		found += memp->size;
	return (found);
}

#define	OLDDELAY(n) { register int N = ((unsigned)((n) << 4) >> cpudelay); \
		while (--N > 0); }

/*
 * set delay constant for usec_delay()
 * NOTE: we use the fact that the per-
 * processor clocks are available and
 * mapped properly at "utimers".
 */
setcpudelay()
{
	extern struct count14 utimers;
	unsigned	r;		/* timer resolution, ~ns */
	unsigned	e;		/* delay time, us */
	unsigned	es;		/* delay time, ~ns */
	unsigned	t, f;		/* for time measurement */
	int		s;		/* saved PSR for inhibiting ints */

	r = 512;		 /* worst supported timer resolution, ~ns */
	es = r * 100;		 /* target delay in ~ns */
	e = ((es + 1023) >> 10); /* request delay in us, round up */
	es = e << 10;		 /* adjusted target delay in ~ns */
	Cpudelay = 1;		 /* initial guess */
	DELAY(1);		 /* first time may be weird */
	do {
		Cpudelay <<= 1;	/* double until big enough */
		do {
			s = spl8();
			t = utimers.timer_lsw;
			DELAY(e);
			f = utimers.timer_lsw;
			(void) splx(s);
		} while (f < t);
		t = f - t;
	} while (t < es);
	Cpudelay = (Cpudelay * es + t) / t;
	if (Cpudelay < 0)
		Cpudelay = 0;

	do {
		s = spl8();
		t = utimers.timer_lsw;
		DELAY(e);
		f = utimers.timer_lsw;
		(void) splx(s);
	} while (f < t);
	t = f - t;

	/*
	 * set up old delay stuff, so grotty old binary drivers compiled
	 * under SunOS 4.0 still have half a chance of working.
	 */
	cpudelay = 11;
	do {
		cpudelay--;
		do {
			s = spl8();
			t = utimers.timer_lsw;
			OLDDELAY(e);
			f = utimers.timer_lsw;
			(void) splx(s);
		} while (f < t);
		t = f - t;
	} while ((t < es) && (cpudelay > 0));
}

cpanic(src, line, str, exp, act)
	char   *src;
	int	line;
	char   *str;
	int	exp;
	int	act;
{
#ifdef	prom_eprintf
	prom_eprintf("%s, line %d: %s; exp=%x, act=%x\n",
		src, line, str, exp, act);
#endif	prom_eprintf
	panic(str);
}

char *cpuidmsgs[] = {
	"cpuid all ok",
	"cpuid vac err",
	"cpuid tlb err",
	"cpuid tbr err",
	"cpuid mid err",
};

ccheck(src, line)
	char   *src;
	int	line;
{
	int	code;
	int	exp;
	int	act;

	code = check_cpuid(&exp, &act);
	if (code)
		cpanic(src, line, cpuidmsgs[code], exp, act);
}

/*
 * Setup system implementation dependent function vectors
 */
void
system_setup(systype)
	int	systype;
{
	switch (systype) {
#ifdef	SUN4M_690
	case CPU_SUN4M_690:
		p4m690_sys_setfunc();
		break;
#endif
#ifdef	SUN4M_50
	case CPU_SUN4M_50:
		p4m50_sys_setfunc();
		break;
#endif
	default:
		/* assume it conforms to "generic" sun4m */
		break;
	}
	init_all_fsr();
}

#ifdef	MULTIPROCESSOR

/*
 * topproc:
 * return a pointer to the proc
 * at the front of the run queue,
 * or NULL if there is none.
 */
struct proc *
topproc()
{
	register unsigned wqs;
	register int ix;

	wqs = whichqs;
	/*
	 * If no processes are waiting on the
	 * run queue, we are all done.
	 */
	if (wqs == 0)
		return (struct proc *)0;
	/*
	 * Locate the top process in the run queue.
	 */
	ix = 0;
	while ((wqs&1)==0) {
		wqs >>= 1;
		ix ++;
	}
	return qs[ix].ph_link;
}

/*
 * bestcpu:
 * return the "best" cpu to
 * run this process, or -1
 * if nobody should run it.
 */
int
bestcpu(p)
	struct proc *p;
{
	register unsigned pam;
	static int prevcpu = 0;
	extern int procset;
	int cpmax, cpwho, ix, who;
	unsigned bit;

	/*
	 * The null process should not be staged.
	 */
	if (p == (struct proc *)0)
		return -1;

	pam = p->p_pam & procset;

	/*
	 * If it can't run anywhere,
	 * don't run it anywhere.
	 */
	if (pam == 0)
		return -1;

	/*
	 * Scan the CPUs that can
	 * run this process, to find
	 * the cpu with highest "curpri".
	 * Scan starting with the CPU
	 * following the one most recently
	 * returned, so ties get spread
	 * around the list.
	 *
	 * Seed the "max" value and index
	 * with the processor that most
	 * recently ran this process to
	 * give it some favor.
	 *
	 * If someone that can run this
	 * process is idle, we are done.
	 */
	cpwho = p->p_cpuid & 0xFF;
	cpmax = PerCPU[cpwho].curpri;
	who = prevcpu;
	for (ix=0; ix<NCPU; ++ix) {
		if (++who >= NCPU)
			who = 0;
		bit = 1<<who;
		if (pam & bit) {
			if (PerCPU[who].cpu_idle)
				return who + NCPU;
			if (PerCPU[who].curpri > cpmax) {
				cpmax = PerCPU[who].curpri;
				cpwho = who;
			}
		}
	}
	/*
	 * If we did not find a CPU that
	 * would want to run this process
	 * over what it is now doing,
	 * don't run it anywhere.
	 */
	if (cpmax < p->p_pri)
		return -1;
	/*
	 * cpwho is the index of the "best" processor
	 * to run the process on the front of the queue.
	 * save it for next time, too.
	 */
	prevcpu = cpwho;
	return cpwho;
}

#ifdef	OPENPROMS
/*
 * un-hide the promlib services we are intercepting
 */
#undef	prom_exit_to_mon
#undef	prom_enter_mon
#undef	prom_printf
#undef	prom_eprintf
#undef	prom_writestr
#undef	prom_putchar
#undef	prom_mayput

extern void	prom_exit_to_mon();
extern void	prom_enter_mon();
extern int	prom_printf();
extern void	prom_writestr();
extern void	prom_putchar();
extern int	prom_mayput();

unsigned	inprom = 0;

mpprom_enter_slave()
{
	extern label_t panic_label;
	flush_windows();
	(void) setjmp(&panic_label);

	if (klock_knock()) {
		inprom = 1;
		prom_enter_mon();
	} else {
		(void) prom_idlecpu(0);
	}

	if (swapl(0, &inprom)) { /* first one out resumes the others */
		extern unsigned snid[NCPU];
		register unsigned nid;
		register int ix;

		for (ix = 0; ix < nmod; ++ix)
			if (ix != cpuid)
				if (nid = snid[ix])
					(void) prom_resumecpu(nid);
	}
}

mpprom_enter()
{
	(void) xc_sync(0, 0, 0, 0, 0, mpprom_enter_slave);
}

mpprom_exit_slave()
{
	if (klock_knock())
		prom_exit_to_mon();
	else
		(void) prom_stopcpu(0);
}

mpprom_exit()
{
	(void) xc_sync(0, 0, 0, 0, 0, mpprom_exit_slave);
}

/* VARARGS1 */
mpprom_printf(fmt, a, b, c, d, e)
char *fmt;
{
#ifdef	MULTIPROCESSOR
	klock_require(__FILE__, __LINE__, "mpprom_printf");
	xc_attention();
#endif	MULTIPROCESSOR
	prom_printf(fmt, a, b, c, d, e);
#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
}

/* VARARGS1 */
mpprom_eprintf(fmt, a, b, c, d, e)
char *fmt;
{
#ifdef	MULTIPROCESSOR
	klock_require(__FILE__, __LINE__, "mpprom_printf");
	xc_attention();
#endif	MULTIPROCESSOR
	if (fmt) prom_printf(fmt, a, b, c, d, e);
#ifdef	PROM_PARANOIA
	prom_printf("stack may be at %x or %x\n",
		    framepointer(), stackpointer());
	prom_enter_mon();
#endif	PROM_PARANOIA
#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
}

mpprom_write_nl7(buf, size)
char   *buf;
int	size;
{
	extern void prom_putchar();
	int	c;

#ifdef	MULTIPROCESSOR
	klock_require(__FILE__, __LINE__, "mpprom_printf");
	xc_attention();
#endif	MULTIPROCESSOR
	while (size-->0)
		if (c = 0x7F & *buf++) {
			if (c == '\n')
				prom_putchar('\r');
			prom_putchar(c);
		}
#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
}

mpprom_write(buf, size)
char   *buf;
u_int	size;
{
	extern void prom_putchar();

#ifdef	MULTIPROCESSOR
	klock_require(__FILE__, __LINE__, "mpprom_printf");
	xc_attention();
#endif	MULTIPROCESSOR
	prom_writestr(buf, size);
#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
}

mpprom_putchar(c)
{
#ifdef	MULTIPROCESSOR
	klock_require(__FILE__, __LINE__, "mpprom_printf");
	xc_attention();
#endif	MULTIPROCESSOR
	prom_putchar(c);
#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
}

mpprom_mayput(c)
{
	register int rv;
#ifdef	MULTIPROCESSOR
	klock_require(__FILE__, __LINE__, "mpprom_printf");
	xc_attention();
#endif	MULTIPROCESSOR
	rv = prom_mayput(c);
#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
	return (rv);
}
#endif	OPENPROMS

/*
 * klock_reqfail: report klock requirement failure
 */
klock_reqfail(fn, ln, where, act, exp, mid)
	char *fn;
	int ln;
	char *where;
	int act;
	int exp;
	int mid;
{
	trigger_logan();
	if (!fn) fn = "";
	if (!where) where = "";
/*
 * this is really ugly, but
 * what choice do we have?
 * we can't call mpprom_printf, as
 * it calls klock_require, which will
 * just branch right back here.
 */
	halt_and_catch_fire();

	prom_printf("%s(%d): klock not held%s\n",
		fn, ln, where);
	prom_printf("\tmid=%x exp=%x act=%x\n", mid, exp, act);
	prom_printf("\tstack is at %x or %x\n",
		    stackpointer(), framepointer());
	prom_enter_mon();
}
#endif	MULTIPROCESSOR

/*
 * unmap this region at L2 ptes starting at addr to end of the region
 * Assumes kl1pt as the pointer to level 1 table
 * Used at startup by the kernel to cleanup unused maps.
 */
static 
unmap_to_end_rgn(addr)
u_int addr;
{
	register union ptpe *lptp;
	register union ptpe *sptp;
	union ptpe a_ptpe;
	u_int endaddr;

	lptp = &a_ptpe;

	 lptp->ptpe_int = 
	      *(u_int *)((addr_t)kl1pt + getl1in(addr) * sizeof (struct ptp));

	if (lptp->ptp.EntryType == MMU_ET_INVALID) {
	       /* nothing to do */
		return;
	}
	
	endaddr = (((u_int)addr + (L2PTSIZE - L3PTSIZE)) & ~ (L2PTSIZE-1));
	
	for (; addr < endaddr; addr += L3PTSIZE) { 
	      /* Get the level 2 entry and make it invalid */

	     sptp = &(((struct l2pt *)ptptopte(lptp->ptp.PageTablePointer))->ptpe[getl2in(addr)]);
	     sptp->ptpe_int = 0;
	}
   }

