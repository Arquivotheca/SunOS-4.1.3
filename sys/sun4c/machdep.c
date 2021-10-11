#ident "@(#)machdep.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#define	OPENPROMS
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
#include <sun/autoconf.h>
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
#include <machine/asm_linkage.h>
#include <machine/eeprom.h>
#include <machine/intreg.h>
#include <machine/memerr.h>
#include <machine/seg_kmem.h>
#include <machine/buserr.h>
#include <machine/enable.h>
#include <machine/auxio.h>
#include <machine/trap.h>

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

#define	MINBUF		8			/* min # of buffers (8) */
#define BUFPERCENT	(1000 / 15)		/* max pmem for buffers(1.5%) */
#define MAXBUF		(BUFBYTES/MAXBSIZE - 1)	/* max # of buffers (255) */

/*
 * Declare these as initialized data so we can patch them.
 */
int nbuf = 0;
int physmem = 0;	/* memory size in pages, patch if you want less */
int kernprot = 1;	/* write protect kernel text */
int msgbufinit = 1;	/* message buffer has been initialized, ok to printf */
int nrnode = 0;		/* maximum number of NFS rnodes */
int nopanicdebug = 0;	/* 0 = call debugger (if any) on panic, 1 = reboot */
u_int dfldsiz;		/* default data size limit */
u_int dflssiz;		/* default stack size limit */
u_int maxdsiz;		/* max data size limit */
u_int maxssiz;		/* max stack size limit */

#ifdef	VAC
int use_vac = 1;	/* variable to patch to have the kernel use the cache */
#else	!VAC
#define	use_vac 0
#endif	!VAC

#ifdef	IOC
int use_ioc = 0;	/* variable to patch to have the kernel use i/o cache */
int cachenet = 1;	/* iocache the network */
int cachevme = 1;	/* iocache the vme */
int iocdebug = 0;	/* debugging flag for ioc code */
int iocenable = 0;	/* vestigial */
#else	!IOC
#define	use_ioc 0
#define	cachenet 0
#define	cachevme 0
#define	iocdebug 0
#endif	!IOC

#ifdef	BCOPY_BUF
int bcopy_res = -1;	/* block copy buffer reserved (in use) */
int use_bcopy = 0;	/* variable to patch to have kernel use bcopy buffer */
int bcopy_cnt = 0;	/* number of bcopys that used the buffer */
int bzero_cnt = 0;	/* number of bzeros that used the buffer */
#endif	BCOPY_BUF

/*
 * Configuration parameters set at boot time.
 */
u_int nctxs = NCTXS_60;		/* number of implemented contexts */
#ifdef	MMU3_LEVEL
u_int nsmgrps;			/* number of smgrps in segment map */
#endif	MMU3_LEVEL
u_int npmgrps = NPMGRPS_60;	/* number of pmgrps in page map */
addr_t hole_start;		/* addr of start of MMU "hole" */
addr_t hole_end;		/* addr of end of MMU "hole" */
int hole_shift;			/* "hole" check shift to get high addr bits */
addr_t econtig;			/* End of first block of contiguous kernel */
addr_t vstart;			/* Start of valloc'ed region */
addr_t vvm;			/* Start of valloc'ed region we can mmap */
u_int vmsize;			/* size of vm structures */
int cpu_buserr_type = 0;	/* bus error type (0 = 4/60 style) */
int vac_size = VAC_SIZE_60;	/* cache size in bytes */
int vac_linesize = VAC_LINESIZE_60;	/* size of a cache line */
int vac_hwflush = 0;		/* cache has HW flush */
int Cpudelay = 0;		/* delay loop count/usec */
int cpudelay = 0;		/* for compatibility with old macros */
int dvmasize = 246;		/* usable dvma space */
u_int shm_alignment;		/* VAC address consistency modulus */
struct memlist *physmemory;	/* Total installed physical memory */
struct memlist *availmemory;	/* Available (unreserved) physical memory */
struct memlist *virtmemory;	/* Available (unmapped?) virtual memory */
int memexp_flag;		/* memory expansion card flag */
#ifdef	SAS
extern int availmem;		/* physical memory available in SAS */
#endif	SAS

/*
 * Most of these vairables are set to indicate whether or not a particular
 * architectural feature is implemented.  If the kernel is not configured
 * for these features they are #defined as zero. This allows the unused
 * code to be eliminated by dead code detection without messy ifdefs all
 * over the place.
 */
#ifdef	MMU_3LEVEL
int mmu_3level = 0;		/* three level MMU present */
#endif	MMU_3LEVEL

#ifdef	VAC
int vac = 0;			/* virtual address cache type (none == 0) */
#endif	VAC

#ifdef	IOC
int ioc = 0;			/* I/O cache type (none == 0) */
#endif	IOC

#ifdef	BCOPY_BUF
int bcopy_buf = 0;		/* block copy buffer present */
#endif	BCOPY_BUF

extern int	npmgrpssw;
extern int	npmghash;

/*
 * Dynamically allocated MMU structures
 */
extern struct   ctx *ctxs, *ctxsNCTXS;
extern struct   pmgrp *pmgrps, *pmgrpsNPMGRPS;
extern struct   pment *pments, *pmentsNPMENTS;
extern struct   pte   *ptes,   *ptesNPTES;
extern struct   hwpmg *hwpmgs, *hwpmgsNHWPMGS;
extern struct   pmgrp   **pmghash, **pmghashNPMGHASH;

#ifdef	MMU_3LEVEL
struct	smgrp *smgrps, *smgrpsNSMGRPS;
struct	sment *sments, *smentsNSMENTS;
#endif	MMU_3LEVEL

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

/*
 * FORTH monitor gives us romp as a variable
 */
struct sunromvec *romp = (struct sunromvec *)0xFFE81000;
struct debugvec *dvec;

/*
 * Saved beginning page frame for kernel .data and last page
 * frame for up to end[] for the kernel. These are filled in
 * by kvm_init().
 */

u_int kpfn_dataseg, kpfn_endbss;

/*
 * Monitor pages may not be where this sez they are.
 * and the debugger may not be there either.
 *
 * Somewhere (probably in the middle), IOPBMEM will steal
 * a couple of pages for the IOPB map. These will vanish from the
 * list of free pages reported to the kernel and thus will not be
 * backed with page structs. See infra.
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
 *		|	????		|
 * 0xFFFFB000  -|-----------------------|
 *		|    interrupt reg	|	NPMEG-2
 * 0xFFFFA000  -|-----------------------|
 *		|   memory error reg	|
 * 0xFFFF9000  -|-----------------------|
 *		| eeprom, idprom, clock	|
 * 0xFFFF8000  -|-----------------------|
 *		|    counter regs	|
 * 0xFFFF7000  -|-----------------------|
 *		|	auxio reg 	|
 * 0xFFFF6000  -|-----------------------|
 *		|	DVMA		|
 *		|    IOPB memory	|
 * 0xFFF00000  -|-----------------------|- DVMA
 *		|     monitor (2M)	|
 * 0xFFD00000  -|-----------------------|- MONSTART
 *		|     debugger (1M)	|
 * 0xFFC00000  -|-----------------------|- DEBUGSTART
 *		|     SEGTEMP2 (256K)	|
 * 0xFFBC0000	|-----------------------|- SEGTEMP2
 *		|     SEGTEMP (256K)	|
 * 0xFFB80000  -|-----------------------|- SEGTEMP
 *		|	 segkmap	|
 *		|  >= MINMAPSIZE (2M)	|
 * 0xFF943000	|-----------------------|
 *		|  	NCARGS (1MB)	|
 * 0xFF843000  -|-----------------------|- Syslimit
 *		|	Mbmap		|
 *		|  MBPOOLMMUPAGES (2MB)	|
 * 0xFF643000  -|-----------------------|- mbutl
 *		|	mmap (1 page)	|
 * 0xFF642000  -|-----------------------|- vmmap
 *		|	CMAP2 (1 page)	|
 * 0xFF641000  -|-----------------------|- CADDR2
 *		|	CMAP1 (1 page)	|
 * 0xFF640000  -|-----------------------|- CADDR1
 *		|	kernelmap	|
 *		|    SYSPTSIZE (6.25MB)	|
 * 0xFF000000  -|-----------------------|- Sysbase
 *		|	GX (16MB)	|
 * 0xFE000000	|-----------------------|- GX_BASE BUFLIMIT
 *		|    Buffer Cache (2MB)	|
 * 0xFDE00000	|-----------------------|- BUFBASE HEAPLIMIT
 *		|    Kernel Heap (16MB)	|
 * 0xFCE00000  -|-----------------------|- HEAPBASE
 *		|	 unused		|
 *		|-----------------------|
 *		|	 segu		|
 *		|-----------------------|- econtig
 *		|   configured tables	|
 *		|	buffers		|
 *		|-----------------------|- vvm
 *		|  hat data structures	|
 *		|-----------------------|- end
 *		|	kernel		|
 *		|-----------------------|
 *		|   trap table (4k)	|
 * 0xF8004000  -|-----------------------|- start
 *		|	 msgbuf		|
 * 0xF8002000  -|-----------------------|- msgbuf
 *		|  user copy red zone	|
 *		|	(invalid)	|
 * 0xF8000000  -|-----------------------|- KERNELBASE
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
 * Machine-dependent startup code
 */
startup()
{
	register int unixsize;
	register unsigned i;
	register caddr_t v;
	u_int firstaddr;		/* first free physical page number */
	u_int mapaddr, npages, page_base;
	struct page *pp;
	struct segmap_crargs a;
	u_int map_getsgmap();
	struct memseg *memseg_base;
/*#ifndef SAS*/
	extern void v_handler();
	extern void vx_handler();
/*#endif !SAS*/
	extern char etext[], end[], Syslimit[];
	extern trapvec kclock14_vec;
	extern struct scb scb;

	int dbug_mem, memblocks;
	struct memlist *pmem;
	u_int iopb_pages = 0;
	struct memseg *memsegp;
	u_int first_page, num_pages, index;
	static int offdelay = -1, ondelay = -1;
	extern struct memlist *getmemlist();
	extern void prom_init();

	static struct {
		char *name;
		int *var;
	} prop[] = {
		"buserr-type",	&cpu_buserr_type,
		"vac-size",	&vac_size,
		"vac-linesize",	&vac_linesize,
		"vac-hwflush",	&vac_hwflush,
		"mmu-nctx",	(int *) &nctxs,
		"mmu-npmg",	(int *) &npmgrps,
		"mips-off",	&offdelay,
		"mips-on",	&ondelay,
	};


	prom_init("Kernel");
	setcputype();

	/*
	 * Extract manifest cpu related boot PROM properties.
	 * Note that we are using memblocks as a temp var here.
	 */

	for (i = 0; i < (sizeof prop / sizeof prop[0]); i++)
		if ((memblocks = getprop(0, prop[i].name, -1)) != -1)
			*prop[i].var = memblocks;

	/*
	 * Workaround for bugid 1067719: vac-hwflush is mis-named
	 * vac_hwflush, and it has the wrong value for SS1+ anyway.
	 * (This is the bugfix for 1068462.)
	 */
	switch (cpu) {
	case CPU_SUN4C_75:	/* SS-2 */
	case CPU_SUN4C_50:	/* IPX */
	case CPU_SUN4C_25:	/* ELC */
		if (vac_hwflush == 0)
			vac_hwflush = 1;
	}

#ifdef VAC
#define	NO_OP	0x01000000
	/* 
	 * XXX - There are are several sets of instructions in locore.s
	 * to work around hardware bug #1050558. It applies only to the
	 * cache+ chip first used by SS2. For those machines that don't
	 * have this bug, we no-op the instructions here.
	 * There is a long, general sequence at sys_trap, and 4 3-instruction
	 * sequences at: go_multiply_check, go_window_overflow,
	 * go_window_underflow and (#ifdef GPROF) go_test_prof.
	 */
	if ((cpu_buserr_type != 1) || (use_vac == 0)) {
		int	*nopp = (int *)NULL;
		int	**noppp;
#ifndef lint
		extern int sys_trap[];
		extern int go_multiply_check[];
		extern int go_window_overflow[];
		extern int go_window_underflow[];
#ifdef GPROF
		extern int go_test_prof[];
#endif
#endif lint
		static int	*noppps[] = {
#ifndef lint
			go_multiply_check,
			go_window_overflow,
			go_window_underflow,
#ifdef GPROF
			go_test_prof,
#endif
#endif lint
			(int *)0 };

#ifndef lint
		nopp = sys_trap;
#endif lint
		for (i = 0; i < 7; i++)
			*nopp++ = NO_OP;

		for (noppp = noppps; *noppp; noppp++) {
			nopp = *noppp;
			for (i = 0; i < 3; i++)
				*nopp++ = NO_OP;
		}
	}
#endif VAC
	/*
	 * Establish on/off delay defaults in
	 * case PROM doesn't know what they are.
	 */

	if (offdelay == -1)
		offdelay = 0;

	if (ondelay == -1) {
		if (cpu == CPU_SUN4C_60) {
			ondelay = CPU_MAXMIPS_20MHZ;
		} else {
			ondelay = CPU_MAXMIPS_25MHZ;
		}
	}

	/*
	 * set cache-off delay constant
	 */

	setdelay(offdelay);

#ifdef notdef
	/*
	 * The following remain here only for reference purpose.
	 */
	if (cpu == CPU_SUN4C_75) {
		printf(" cpu = sun4/75\n");
		cpu_buserr_type = 1;
		vac_size = 64 * 1024;
		vac_linesize = 32;
		vac_hwflush = 1;
		nctxs = 16;
		npmgrps = 256;
	} else if (cpu == CPU_SUN4C_60) {
		printf(" cpu = sun4/60\n");
		cpu_buserr_type = 1;
		vac_size = 64 * 1024;
		vac_linesize = 16;
		vac_hwflush = 0;
		nctxs = 8;
		npmgrps = 128;
	} else
		halt(" illegal cpu type ");
#endif notdef
	/*
	 * XXX: The following options are not really supported on Sun-4c.
	 * Perhaps they should be unifdef-ed?
	 */
#ifdef	VAC
	vac = 1;
#endif	VAC
#ifdef	IOC
	ioc = 0;
#endif	IOC
#ifdef	BCOPY_BUF
	bcopy_buf = 0;
	if (bcopy_buf) {
		if (use_bcopy) {
			bcopy_res = 0;		/* allow use of hardware */
		} else {
			printf("bcopy buffer disabled\n");
			bcopy_res = -1;		/* reserve now, hold forever */
		}
	}
#endif	BCOPY_BUF
#ifdef	MMU_3LEVEL
	mmu_3level = 0;
#endif	MMU_3LEVEL

#ifdef	MMU_3LEVEL
	if (mmu_3level) {
		/* no hole in a 3 level mmu, just set it halfway */
		/* XXX no cpuinfo */
		nsmgrps = cpuinfop->cpui_nsme / NSMENTPERSMGRP;
		hole_shift = 31;
		hole_start = (addr_t)(1 << hole_shift);
	} else
#endif	MMU_3LEVEL
	{
		/* compute hole size in 2 level MMU */
		hole_start = (addr_t)((NPMGRPPERCTX / 2) * PMGRPSIZE);
		hole_shift = ffs((long)hole_start) - 1;
	}
	hole_end = (addr_t)(-(long)hole_start);
	if (maxdsiz == 0)
		maxdsiz = (int)hole_start - USRTEXT;
	if (maxssiz == 0)
		maxssiz = (int)hole_start - KERNELSIZE;
	if (dfldsiz == 0)
		dfldsiz = maxdsiz;

#ifdef	VA_HOLE
	ASSERT(hole_shift < 31);
#endif	VA_HOLE

	/*
	 * The default stack size of 8M allows an optimization of mmu mapping
	 * resources so that in normal use a single mmu region map entry (smeg)
	 * can be used to map both the stack and shared libraries.
	 */
	if (dflssiz == 0)
		dflssiz = (8*1024*1024);

	/*
	 * Save the kernel's level 14 interrupt vector code and install
	 * the monitor's. This lets the monitor run the console until we
	 * take it over.
	 */
	kclock14_vec = scb.interrupts[14 - 1];
	start_mon_clock();
	(void)splzs();			/* allow hi clock ints but not zs */

	bootflags();			/* get the boot options */

	/*
	 * If the boot flags say that the debugger is there,
	 * test and see if it really is by peeking at DVEC.
	 * If is isn't, we turn off the RB_DEBUG flag else
	 * we call the debugger scbsync() routine.
	 */
#ifndef SAS
	if ((boothowto & RB_DEBUG) != 0) {
		if (dvec == NULL || peek((short *)dvec) == -1)
			boothowto &= ~RB_DEBUG;
		else {
			extern trapvec kadb_tcode, trap_ff_tcode,
				trap_fe_tcode;

			(*dvec->dv_scbsync)();
			/*
			 * Now steal back the traps.
			 * We "know" that kadb steals trap TRAPBRKNO-1 and 
			 * TRAPBRKNO, and that it uses the same trap code for both.
			 */
			kadb_tcode = scb.user_trap[TRAPBRKNO];
			scb.user_trap[TRAPBRKNO] = trap_ff_tcode;
			scb.user_trap[TRAPBRKNO-1] = trap_fe_tcode;
		}
	}
#endif !SAS


/*#ifdef SAS*/
#ifdef notdef
	/* SAS has contigouous memory */
	npages = physmem = btop(availmem);
	physmax = physmem - 1;
	memblocks = 0; /* set this to zero so the valloc below ends up w/ 1 */
/*#else*/
#endif notdef
	/*
	 * Install PROM callback handler (give both, promlib picks the
	 * appropriate handler.
	 */
	if (prom_sethandler(v_handler, vx_handler) != 0)
		panic("No handler for PROM?");

	/*
	 * Add up how much physical memory the prom has passed us.
	 */
	if (prom_getversion() > 0) {
		/*
		 * We must initialize the memlists ourselves.
		 */
		availmemory = getmemlist("memory", "available");
		physmemory = getmemlist("memory", "reg");
		virtmemory = getmemlist("virtual-memory", "available");
	} else {
		availmemory = *romp->v_availmemory;
		physmemory = *romp->v_physmemory;
		virtmemory = *romp->v_virtmemory;
	}

	/* check for memory expansion and set flag if it's there */
	for (memexp_flag = 0, pmem = physmemory; pmem; pmem = pmem->next) {
		if (pmem->address >= MEMEXP_START) {
			memexp_flag++;
			break;
		}
	}

	/*
	 * initialize handling of memory errors
	 */
	memerr_init();


	/*
	 * allow interrupts now,
	 * after memory error handling has been initialized.
	 * Must turn on SBus IRQ 6 also.
	 */
	*INTREG |= (IR_ENA_INT | IR_ENA_LVL8);


	npages = memblocks = 0;
	for (pmem = availmemory; pmem; pmem = pmem->next) {
		memblocks++;
		/*
		 * Allocate pages for the IOPBMAP from the end of
		 * the first memblock that's big enough. (Probably
		 * the initial memblock will always be "big enough.")
		 * Reserve these pages for later by deducting them
		 * from the availmemory list.
		 * NOTE: This pages must *not* be backed by page
		 * structs, or cache consistency will fail and
		 * your day will end in misery.
		 */
		if ((iopb_pages == 0) && (mmu_btop(pmem->size) > IOPBMEM)) {
			iopb_pages =
			    pmem->address + pmem->size - mmu_ptob(IOPBMEM);
			pmem->size -= mmu_ptob(IOPBMEM);
		}
		npages += mmu_btop(pmem->size);
	}

	/*
	 * Remember what the physically available highest page is
	 * so that dumpsys works properly.
	 */
	for (physmax = 0, pmem = physmemory; pmem; pmem = pmem->next)
		if (physmax < mmu_btop(pmem->size + pmem->address))
			physmax = mmu_btop(pmem->size + pmem->address);

	/*
	 * If physmem is patched to be non-zero, use it instead of
	 * the monitor value unless physmem is larger than the total
	 * amount of memory on hand.
	 */
	if (physmem == 0 || physmem > npages)
		physmem = npages + IOPBMEM;

	/*
	 * If debugger is in memory, note the pages it stole from physmem.
	 */
	if (boothowto & RB_DEBUG) {
		dbug_mem = *dvec->dv_pages;
	} else
		dbug_mem = 0;

/*#endif SAS*/
	/*
	 * maxmem is the amount of physical memory we're playing with.
	 * We've already stolen the IOPBMAP.
	 */
	maxmem = physmem;

	/*
	 * Why is this here?
	 */

	if (nrnode == 0) {
		nrnode = ncsize << 1;
	}


	/*
	 * The number of SW pmgrps is set to map
	 * the physical memory 8 times over.
	 */
	if (npmgrpssw == 0) {
		npmgrpssw = 8 * (ptob(physmem) / PMGRPSIZE);

		/*
		 * If the calculated number of SW page tables is smaller
		 * than the number of HW pmegs, use the number
		 * of HW pmegs instead.
		 */
		if (npmgrpssw < NPMGRPS)
			npmgrpssw = NPMGRPS;
	}

	/*
	 * Number of cached entries for hat_pmgfind.
	 * Round up to the nearest power of two.
	 */
	npmghash = 2 * npmgrpssw - 1;

	for (i = (0x80 << ((sizeof (int) - 1) * NBBY)); i != 0; i >>= 1) {
		if ((npmghash & i) != 0) {
			npmghash = i << 1;
			break;
		}
	}

	/*
	 * Allocate space for system data structures.
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * It is used when remapping the tables later.
	 * Note we continue right after end (not the next page).
	 */
	v = vstart = end;

#define	valloc(name, type, num) \
	    (name) = (type *)(v); (v) = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))
	/*
	 * First the machine dependent, Hardware Address Translation (hat)
	 * data structures are allocated. We depend on the fact that the
	 * Monitor maps in more memory than required. We do this first so
	 * that the remaining data structures can be allocated from the
	 * page pool in the usual way.
	 */
	valloclim(ctxs, struct ctx, NCTXS, ctxsNCTXS);
	valloclim(pmgrps, struct pmgrp, NPMGRPSSW, pmgrpsNPMGRPS);
	valloclim(pments, struct pment, (NPMGRPSSW * NPMENTPERPMGRP),
	    pmentsNPMENTS);
	valloclim(ptes, struct pte, (NPMGRPSSW * NPMENTPERPMGRP), ptesNPTES);
	valloclim(hwpmgs, struct hwpmg, NPMGRPS, hwpmgsNHWPMGS);
	valloclim(pmghash, struct pmgrp *, NPMGHASH, pmghashNPMGHASH);
#ifdef	MMU_3LEVEL
	if (mmu_3level) {
		valloclim(smgrps, struct smgrp, NSMGRPS, smgrpsNSMGRPS);
		valloclim(sments, struct sment, (NSMGRPS * NSMENTPERSMGRP),
			smentsNSMENTS);
	}
#endif	MMU_3LEVEL

	/* check that the vm data structures are actually mapped */
	if (map_getsgmap(v) == (u_int)PMGRP_INVALID){
		panic("kernel too big");
	}

	/*
	 * Initialize the hat layer.
	 */
	vmsize = v - vstart;		/* size of vm structures */
#ifndef	SIMUL
	bzero((caddr_t)vstart, vmsize);
#else	!SIMUL
	bzero((caddr_t)vstart, 20);
#endif	!SIMUL
	hat_init();

	/*
	 * Vallocate the remaining kernel data structures.
	 * The first available real memory address is in "firstaddr".
	 * "mapaddr" is the real memory address where the tables start.
	 * It is used when remapping the tables later.
	 */
	v = vvm = (addr_t)roundup((u_int)v, PAGESIZE);
	firstaddr = btoc((int)vvm - KERNELBASE);
	mapaddr = firstaddr;

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
	valloc(dvmamap, struct map, NDVMAMAPS(dvmasize));
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

	page_base = mapaddr;

	/*
	 * use maxmem here to *not* back IOPBMEM with page structures
	 */

/*#ifndef	SAS*/
	npages = maxmem - page_base;
	npages += 2;			/* add two to reclaim pages 0, 1 */
/*#endif*/
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

	/*
	 * Initialize VM system, and map kernel address space.
	 */
	kvm_init();

#ifndef	SAS
	enable_dvma();
#endif

#ifdef VAC
	if (vac) {
		if (use_vac) {
			vac_control(1);
			setdelay(ondelay);
			shm_alignment = vac_size;
		} else  {
			printf("CACHE IS OFF!\n");
			vac_control(0);
			shm_alignment = PAGESIZE;
		}
	}
#endif VAC


#ifdef	IOC
	if (ioc) {
		ioc_init();
		if (use_ioc) {
			on_enablereg(ENA_IOCACHE);
			iocenable = 1;
		} else {
			printf("IO CACHE IS OFF!\n");
			iocenable = 0;
			ioc = 0;
		}
	}
#endif	IOC
	/*
	 * On Sun-4c machines we can increase klustsize and maxphys
	 * (statically initialized elsewhere, didn't want to #ifdef).
	 */
	{
		extern int klustsize;	/* specfs/spec_vnodeops.c */
		extern int maxphys;	/* os/vm_subr.c */

		klustsize = 124 * 1024;
		maxphys = 124 * 1024;
	}

	/*
	 * Good {morning, afternoon, evening, night}.
	 * When printing memory, show the total as physmem less
	 * that stolen by a debugger.
	 */
	printf(version);

/*#ifdef SAS
	printf("mem = %dK (0x%x)\n", availmem / 1024, availmem);
#else*/
	printf("mem = %dK (0x%x)\n",
		(calc_memsize()-mmu_ptob(dbug_mem))>>10,
		(calc_memsize()-mmu_ptob(dbug_mem)));
/*#endif SAS*/

	if (Syslimit + NCARGS + MINMAPSIZE > SEGTEMP)
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

	unixsize = btoc(v - KERNELBASE);

	/*
	 * Clear allocated space, and make r/w entries
	 * for the space in the kernel map.
	 */
/*#ifndef SAS*/
	if (unixsize >= mmu_btop(availmemory->size) - 8*UPAGES)
/*#else SAS
	if (unixsize >= physmem - 8*UPAGES)
#endif SAS*/
		halt("no memory");

	segkmem_mapin(&kvalloc, (addr_t)vvm, (u_int)(econtig - vvm),
		PROT_READ | PROT_WRITE, mapaddr, 0);

#ifndef SIMUL
	bzero((caddr_t)vvm, (u_int)(econtig - vvm));
#else SIMUL
	/* Hardware simulator has already cleared memory */
	bzero((caddr_t)vvm, 20);
#endif SIMUL

/*#ifndef SAS*/
	/*
	 * Initialize more of the page structures. We walk through the
	 * list of physical memory, initializing all the pages that are
	 * backed by page structs.
	 *
	 * We do this a little funny, because we want page 0 && 1
	 * to be reclaimed and put on the free list.
	 *
	 * npages was set above.
	 */

	/*
	 * Do pages 0 and 1...
	 */

	memsegp = memseg_base;
	page_init(&pp[0], 2, 0, memsegp++);

	/*
	 * now do the rest of the pages
	 */

	index = 2;	/* to account for pages 0 && 1 */
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
/*#else SAS*/
	/* for SAS, memory is contiguous */
/*	page_init(&pp[0], npages, page_base, memseg_base);
#endif SAS*/

#ifdef VAC
	/*
	 * If we are running on a machine with a virtual address cache, we call
	 * mapin again for the valloc'ed memory now that we have the page
	 * structures set up so the hat routines leave the translations cached.
	 */
	if (vac) {
		segkmem_mapin(&kvalloc, (addr_t)vvm, (u_int)(econtig - vvm),
			PROT_READ | PROT_WRITE, mapaddr, 0);
	}
#endif VAC

	/*
	 * Initialize callouts.
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

/*#ifdef	SAS
	first_page = memsegs->pages_base;
	if (first_page < mapaddr + btoc(econtig - vvm))
		first_page = mapaddr + btoc(econtig - vvm);
	memialloc(first_page, memsegs->pages_end);
#else	SAS*/
	/*
	 * Initialize memory free list. We free all the pages that are in
	 * our memseg list and aren't already taken by the valloc's.
	 * We also add pages 0 and 1 to the free list.
	 */
	memialloc(0, 2);
	for (memsegp = memsegs->next; memsegp; memsegp = memsegp->next) {
		first_page = memsegp->pages_base;
		if (first_page < mapaddr + btoc(econtig - vvm))
			first_page = mapaddr + btoc(econtig - vvm);
		memialloc(first_page, memsegp->pages_end);
	}
/*#endif	SAS*/

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
	rminit(dvmamap, (long)(dvmasize - IOPBMEM), (u_long)IOPBMEM,
	    "DVMA map space", NDVMAMAPS(dvmasize));

	/*
	 * Map IOPB memory space to the first pages of DVMA space.  The
	 * memory in IOPB space has no page structs allocated to it. This
	 * is necessary so those pages cannot be cacheable. This is a
	 * cornerstone of our software cache consistency algorithm.
	 * NOTE: this assumes that the physical memory for IOPBMEM has
	 * already been allocated. See above, when iopb_pages was set.
	 */

	segkmem_mapin(&kdvmaseg, DVMA, mmu_ptob(IOPBMEM),
	    PROT_READ|PROT_WRITE, mmu_btop(iopb_pages), 0);

	/*
	 * Configure the system.
	 */
	configure();		/* set up devices */
	maxmem = freemem;

	/*
	 * Initialize the u-area segment type.  We position it
	 * after the configured tables and buffers (whose end
	 * is given by econtig) and before HEAPBASE
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
	 * Now create generic mapping segment.  This mapping
	 * goes NCARGS beyond Syslimit up to the SEGTEMP area.
	 * But if the total virtual address is greater than the
	 * amount of free memory that is available, then we trim
	 * back the segment size to that amount.
	 */
	v = Syslimit + NCARGS;
	i = SEGTEMP - v;
	if (i > mmu_ptob(freemem))
		i = mmu_ptob(freemem);
	segkmap = seg_alloc(&kas, v, i);
	if (segkmap == NULL)
		panic("cannot allocate segkmap");
	a.prot = PROT_READ | PROT_WRITE;
	if (segmap_create(segkmap, (caddr_t)&a) != 0)
		panic("segmap_create segkmap");

	(void) spl0();		/* allow interrupts */
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
 * Parse the boot line to determine boot flags .
 */
bootflags()
{
	register struct bootparam *bp;
	register char *cp;
	register int i;
	extern void prom_enter_mon();
	extern char *prom_bootargs();
	extern struct bootparam *prom_bootparam();

#ifdef SAS
	boothowto = RB_SINGLE | RB_NOBOOTRC;
#else
	if (prom_getversion() > 0) {
		cp = prom_bootargs();
		while (*cp && (*cp != '-'))
			++cp;
	} else {
		bp = prom_bootparam();
		cp = bp->bp_argv[1];
	}
	if (cp && *cp && *cp++ == '-')
		do {
			for (i = 0; bootf[i].let; i++) {
				if (*cp == bootf[i].let) {
					boothowto |= bootf[i].bit;
					break;
				}
			}
			cp++;
		} while (bootf[i].let && *cp && (*cp != ' '));
#ifdef ROOT_ON_TAPE
	/* force "-sw", and NOT "-a" for MUNIX */
	boothowto |= RB_SINGLE | RB_WRITABLE;
	boothowto &= ~RB_ASKNAME;
#endif
	if (boothowto & RB_INITNAME) {
		if (prom_getversion() > 0) {
			while (*cp && (*cp == ' '))
				++cp;
			initname = cp;
		} else {
			initname = bp->bp_argv[2];
		}
	}
	if (boothowto & RB_HALT) {
		printf("halted by -h flag\n");
		prom_enter_mon();
	}
#endif SAS
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
#ifdef SAS
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
	*INTREG &= ~IR_ENA_INT;		/* disable all interrupts */
	if (howto & RB_STRING)
		prom_reboot(bootstr);
	else
		prom_reboot(howto & RB_SINGLE ? "-s" : "");
	/* should never get here */
	(void) splx(s);
#endif SAS
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
	struct memlist  *pmem;

	/*
	 * We mark all the pages below the first page (other than page 0)
	 * covered by a page structure.
	 * This gets us the message buffer and the kernel (text, data, bss),
	 * including the interrupt stack and proc[0]'s u area.
	 * (We also get page zero, which we may not need, but one page
	 * extra is no big deal.)
	 */
	pmem = physmemory;
	lophys = mmu_btop(pmem->address);
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
	struct memlist	*pmem;

	for (pmem = physmemory; pmem; pmem = pmem->next) {
		if (pmem->address <= addr &&
		    addr < (pmem->address + pmem->size))
			return (1);
	}
	return (0);
}

/*
 * Machine-dependent portion of dump-checking;
 * mark all pages for dumping.
 */
dump_allpages_machdep()
{
	u_int	i, j;
	struct memlist	*pmem;

	for (pmem = physmemory; pmem; pmem = pmem->next) {
		i = mmu_btop(pmem->address);
		j = i + mmu_btop(pmem->size);
		for (; i < j; i++)
			dump_addpage(i);
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
	struct pte pte;

	addr = &DVMA[mmu_ptob(dvmasize) - MMU_PAGESIZE];
	pte = mmu_pteinvalid;
	pte.pg_v = 1;
	pte.pg_prot = KW;
	pte.pg_pfnum = pg;
	mmu_setpte((addr_t)addr, pte);
	err = VOP_DUMP(vp, addr, bn, ctod(1));
	vac_pageflush(addr);

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
 * We assume that dvmasize is at least two pages.
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
	struct pte pte[2], tpte;
	register int offset;

	offset = (u_int)kaddr & MMU_PAGEOFFSET;
	addr = &DVMA[mmu_ptob(dvmasize) - 2 * MMU_PAGESIZE];
	pte[0] = mmu_pteinvalid;
	pte[1] = mmu_pteinvalid;
	pte[1].pg_v = 1;
	pte[1].pg_prot = KW;
	mmu_getpte(kaddr, &tpte);
	pte[1].pg_pfnum = tpte.pg_pfnum;

	while (count > 0 && !err) {
#ifndef lint
		pte[0] = pte[1];  /* BUG 1025311: lint doesn't like this */
#endif
		mmu_setpte((addr_t) addr, pte[0]);
		mmu_getpte(kaddr + MMU_PAGESIZE, &tpte);
		pte[1].pg_pfnum = (tpte.pg_v && tpte.pg_type == OBMEM) ?
					tpte.pg_pfnum : 0;
		mmu_setpte((addr_t) (addr + MMU_PAGESIZE), pte[1]);
		err = VOP_DUMP(vp, addr + offset, bn, ctod(1));
		bn += ctod(1);
		count -= ctod(1);
		vac_pageflush(addr);
		vac_pageflush(addr + MMU_PAGESIZE);
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
#ifdef SAS
	if (s)
		printf("(%s) ", s);
	printf("Halted\n\n");
	asm("t 255");
#else
	extern void prom_exit_to_mon();

	start_mon_clock();
	if (s)
		prom_printf("(%s) ", s);
	prom_printf("Halted\n\n");
	prom_exit_to_mon();
	stop_mon_clock();
#endif SAS
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
	register addr_t addr, saddr;
	struct pmgrp *pmgrp;
	struct as *as;
	int s;

	addr = bp->b_un.b_addr;
	pmgrp = (struct pmgrp *)NULL;

	if (bp->b_proc == (struct proc *)NULL ||
	    (bp->b_flags & B_REMAPPED) != 0 ||
	    (as = bp->b_proc->p_as) == (struct as *)NULL) {
		as = &kas;
		saddr = addr;
	}

	/*
	 * We spl around use of SEGTEMP2 virtual address mapping
	 * since this routine can be called at interrupt level.
	 */
	s = splvm();
	while (npf-- > 0) {
		if (as != &kas && (pmgrp == (struct pmgrp *)NULL ||
		    ((int)addr & (PMGRPSIZE - 1)) < MMU_PAGESIZE)) {
			/*
			 * Find the pmgrp in the address space.
			 * It should already be locked down,
			 * panic if we cannot actually find it.
			 */
			saddr = (addr_t)((int)addr & ~(PMGRPSIZE - 1));
			for (pmgrp = as->a_hat.hat_pmgrps; pmgrp &&
			    pmgrp->pmg_base != saddr; pmgrp = pmgrp->pmg_next)
				;
			if (pmgrp == (struct pmgrp *)NULL)
				panic("read_hwmap no pmgrp");
			mmu_settpmg(SEGTEMP2, pmgrp);
			saddr = SEGTEMP2 + ((int)addr & (PMGRPSIZE - 1));
		}

		/*
		 * Read in current pte's from hardware into pte array given.
		 */
		mmu_getpte(saddr, pte);
		addr += MMU_PAGESIZE;
		saddr += MMU_PAGESIZE;
		if (pte->pg_v == 0)
			panic("read_hwmap invalid pte");
		pte->pg_prot = KW;		/* XXX - generous */
		pte++;
	}
	mmu_settpmg(SEGTEMP2, pmgrp_invalid);
	(void) splx(s);
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
	int use_segkmem;

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
	 * Since I/O doesn't go thru the cache on sun4c, if we are mapping
	 * over DVMA[], don't use segkmem and the hat layer to mistakenly
	 * 'manage' cache consistency.
	 */
	if (kaddr >= DVMA) {
		/*
		 * The address being mapped is in the DVMA range. This means
		 * we can skip the hat layer entirely.
		 */
		use_segkmem = 0;
	} else {
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
		/*
		 * For sun4c, we allow I/O to the kernel heap directly
		 */
		if (kaddr >= Sysbase && kaddr < Syslimit) {
			flags |= PTELD_INTREP;
		}
		use_segkmem = 1;
	}

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
			hat_mempte(pp, PROT_WRITE | PROT_READ, pte);
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
		if (use_segkmem) {
			segkmem_mapin(seg, kaddr, MMU_PAGESIZE,
			    PROT_READ | PROT_WRITE, MAKE_PFNUM(pte), flags);
		} else {
			mmu_setpte((addr_t) kaddr, *pte);
		}
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
			if (!pte_valid(pte) || pt != pte->pg_type) {
				return (-1);
			} else if (pt != OBMEM && pte->pg_pfnum != pf) {
				return (-1);
			}
		} else {
			if (!pte_valid(pte)) {
				return (-1);
			}
			pf = pte->pg_pfnum;
			pt = pte->pg_type;
			if (pt == OBMEM) {
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
 *
 * For sun4c I/O doesn't go through the cache, but it
 * *may* (not proven yet) still be worthwhile to attempt
 * to allocate along shm_alignment constraints if we are
 * mapping a buffer into the kernel's address space for
 * other than DVMA reasons. Thus the test against the map
 * being dvmamap (XXX: or kncmap (later, maybe)).
 */

int
bp_alloc(map, bp, size)
	struct map *map;
	register struct buf *bp;
	int size;
{
	register struct mapent *mp;
	register u_long addr, mask;
	int align = -1;
	int s;

	if (vac && map != dvmamap) {
		extern struct pment *pments;	/* from <sun/vm_hat.c> */

		/*
		 * XXX: this needs to be reexamined for sun4c
		 */

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
			struct page *pp = bp->b_pages;

			if (pp->p_mapping != NULL) {
				struct pment *pme;

				pme = (struct pment *)pp->p_mapping;
				align = (pme - pments) &
				    mmu_btop(shm_alignment - 1);
			}
		} else if (bp->b_un.b_addr != NULL) {
			align = mmu_btop((int)bp->b_un.b_addr &
			    (shm_alignment - 1));
		}
	}

	if (align == -1) {
		s = splhigh();
		addr = rmalloc(map, (long)size);
		(void) splx(s);
		return (addr);
	}

	/*
	 * Look for a map segment containing a request that works.
	 * If none found, return failure.
	 */
	mask = mmu_btop(shm_alignment) - 1;
	s = splhigh();
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
	(void) splx(s);

	if (mp->m_size == 0)
		return (0);

	/* let rmget() do the rest of the work */
	s = splhigh();
	addr = rmget(map, (long)size, (u_long)addr);
	(void) splx(s);
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
		(void)splx(s);
		bp->b_flags &= ~B_REMAPPED;
	}
}

#ifdef	THIS_IS_NOW_USED
/*
 * Taken out 10/12/89 (mjacob). Nobody is calling this function. And anyway,
 * the notion of returning an address just as an offset from a SBUS_BASE
 * is silly.
 *
 * For this architecture, the physical reg address is very nicely stored
 * in the devinfo structure
 */

/*
 * Compute the address of an I/O device within standard address
 * ranges and return the result.  This is used by DKIOCINFO
 * ioctl to get the best guess possible for the actual address
 * the card is at.
 */
getdevaddr(addr)
	caddr_t addr;
{
	int off = (int)addr & MMU_PAGEOFFSET;
	struct pte pte;
	int physaddr;

	mmu_getkpte(addr, &pte);
	physaddr = mmu_ptob(pte.pg_pfnum);
	switch (pte.pg_type) {
	case OBMEM:
		break;
	case OBIO:
		/*
		 * If it's on the Sbus, give address relative to
		 * the beginning of the Sbus.
		 */
		if (physaddr >= SBUS_BASE)
			physaddr -= SBUS_BASE;
		break;
	}

	return (physaddr + off);
}

#endif	THIS_IS_NOW_USED

/*
 * Explicitly set these so they end up in the data segment.
 * We clear the bss *after* initializing these variables in locore.s.
 */
char mon_clock_on = 0;			/* disables profiling */
trapvec kclock14_vec = { 0 };		/* kernel clock14 vector code */
trapvec mon_clock14_vec = { 0 };	/* monitor clock14 vector code */

start_mon_clock()
{
#ifndef SAS
	if (!mon_clock_on) {
		mon_clock_on = 1;		/* disable profiling */
		write_scb_int(14, &mon_clock14_vec);	/* install mon vector */
#ifndef GPROF
		set_clk_mode(IR_ENA_CLK14, 0);	/* enable level 14 clk intr */
#endif !GPROF
	}
#endif !SAS
}

stop_mon_clock()
{
#ifndef SAS
	if (mon_clock_on) {
		mon_clock_on = 0;		/* enable profiling */
#ifndef GPROF
		set_clk_mode(0, IR_ENA_CLK14);	/* disable level 14 clk intr */
#endif !GPROF
		write_scb_int(14, &kclock14_vec); /* install kernel vector */
	}
#endif !SAS
}

/*
 * Write the scb, which is the first page of the kernel.
 * Normally it is write protected, we provide a function
 * to fiddle with the mapping temporarily.
 *	1) lock out interrupts
 *	2) save the old pte value of the scb page
 *	3) set the pte so it is writable
 *	4) write the desired vector into the scb
 *	5) restore the pte to its old value
 *	6) restore interrupts to their previous state
 */
write_scb_int(level, ip)
	register int level;
	struct trapvec *ip;
{
	register int s;
	register u_int savepte;
	register trapvec *sp;
	extern u_int map_getpgmap();
	extern void map_setpgmap();

	sp = &scb.interrupts[level - 1];
	s = spl8();

	/* save old mapping */
	savepte = map_getpgmap((caddr_t)sp);

	/* allow writes */
	map_setpgmap((caddr_t)sp, (u_int)(PG_V | PG_KW | savepte & PG_PFNUM));

	/* write out new vector code */
	*sp = *ip;

	/* flush out the write since we are changing mappings */
	vac_flush((addr_t) sp, sizeof (struct trapvec));

	/* restore old mapping */
	map_setpgmap((addr_t)sp, savepte);

	(void) splx(s);
}

/*#ifndef SAS*/
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
		prom_printf("%s\n", (char *)addr);
		break;

	case '%':		/* p'%'int anything a la printf */
		prom_printf(str, addr);
		prom_printf("\n");
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
		prom_printf("Don't understand 0x%x '%s'\n", addr, str);
	}
}
/*
 * Handler for OBP v1 monitor vector cmd -
 * Currently, the PROM only uses the sync suncommand, but an interactive
 * interface is provided to allow the user to pass in any arbitrary string.
 */
void
vx_handler(str)
	char *str;
{
	char *sargv[8];

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
		/*NOTREACHED*/

	default:
		prom_printf("Don't understand '%s'\n", str);
	}
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
/*#endif !SAS*/

/*
 * Kernel VM initialization.
 * XXX Obsolete Assumptions about kernel address space ordering:
 *	(1) gap (user space)
 *	(2) kernel text
 *	(3) kernel data/bss
 *	(4) gap
 *	(5) kernel data structures
 *	(6) gap
 *	(7) debugger (optional)
 *	(8) monitor
 *	(9) gap (possibly null)
 *	(10) dvma
 *	(11) devices, u area
 */
kvm_init()
{
	u_int map_getsgmap();
	u_int pmg_econtig, pmg_256k;
	union {
		struct pte u_pte;
		u_int u_ipte;
	} eeprom_pte, counter_pte, intreg_pte, memerr_pte, auxio_pte;
	int c;
	register addr_t v, tv;
	struct pte pte;
	void prom_setcxsegmap();
/* XXXX kvm_int debugging stuff; for debugging strange cache & mmu bugs */
#ifndef KVM_DEBUG
#define	KVM_DEBUG 0		/* 0 = no debugging, 1 = debugging */
#endif
#if KVM_DEBUG > 0
#define	KVM_HERE printf("kvm_init: checkpoint %d\n", ++kvm_here);
#define	KVM_DONE { printf("kvm_init: all done\n"); kvm_here = 0; }
	int kvm_here=0;
#else
#define	KVM_HERE
#define	KVM_DONE
#endif

	/*
	 * If the pmeg that was used (in locore) to map the last 256K of
	 * kernel virtual address space is less than the highest pmeg used
	 * to map the kernel upto econtig, the pmeg for the last 256K gets
	 * invalidated. This causes a panic when the OBP devices are 
	 * referenced. To prevent the panic from happening, the last 256K of
	 * the kernel virtual address space is re mapped using a pmeg higher
	 * than the one used to map the kernel upto econtig. This fix does
	 * not solve the case of running out of kernel virtual address space.
	 * The maximum configuration tested was MAXUSERS=248, 
	 * maximum memory = 128MB (on a sparcstation 2) using a GENERIC
	 * configuration.
	 */

	pmg_econtig = map_getsgmap(econtig); 
	pmg_256k = map_getsgmap((addr_t) (0 - (PMGRPSIZE)));

	if (pmg_256k <= pmg_econtig) {

		/*
		 * Invalidate the PTEs for the on board devices.
 		 */
		mmu_setpte((addr_t) EEPROM_ADDR, mmu_pteinvalid);
		mmu_setpte((addr_t) COUNTER_ADDR, mmu_pteinvalid);
		mmu_setpte((addr_t) MEMERR_ADDR, mmu_pteinvalid);
		mmu_setpte((addr_t) AUXIO_ADDR, mmu_pteinvalid);
		mmu_setpte((addr_t) INTREG_ADDR, mmu_pteinvalid);

		/*
		 * Remap on board devices.
		 */

		map_setsgmap((addr_t) (0-(PMGRPSIZE)), pmg_econtig + 1);

		eeprom_pte.u_pte = mmu_pteinvalid;
		eeprom_pte.u_ipte =
		PG_V | PG_KR | PGT_OBIO | PG_NC | btop(OBIO_EEPROM_ADDR);
		mmu_setpte((addr_t) EEPROM_ADDR, eeprom_pte.u_pte);

		counter_pte.u_pte = mmu_pteinvalid;
		counter_pte.u_ipte =
		PG_V | PG_KW | PGT_OBIO | PG_NC | btop(OBIO_COUNTER_ADDR);
		mmu_setpte((addr_t) COUNTER_ADDR, counter_pte.u_pte);

		memerr_pte.u_pte = mmu_pteinvalid;
		memerr_pte.u_ipte =
		PG_V | PG_KW | PGT_OBIO | PG_NC | btop(OBIO_MEMERR_ADDR);
		mmu_setpte((addr_t) MEMERR_ADDR, memerr_pte.u_pte);

		auxio_pte.u_pte = mmu_pteinvalid;
		auxio_pte.u_ipte =
		PG_V | PG_KW | PGT_OBIO | PG_NC | btop(OBIO_AUXIO_ADDR);
		mmu_setpte((addr_t) AUXIO_ADDR, auxio_pte.u_pte);

		intreg_pte.u_pte = mmu_pteinvalid;
		intreg_pte.u_ipte =
		PG_V | PG_KW | PGT_OBIO | PG_NC | btop(OBIO_INTREG_ADDR);
		mmu_setpte((addr_t) INTREG_ADDR, intreg_pte.u_pte);

	}

KVM_HERE
	/*
	 * Put kernel segment in kernel address space.  Make it a
	 * "kernel memory" segment object.
	 */
	(void) seg_attach(&kas, (addr_t)KERNELBASE, (u_int)(vvm -
	    KERNELBASE), &ktextseg);
	(void) segkmem_create(&ktextseg, (caddr_t)NULL);

KVM_HERE
	(void) seg_attach(&kas, (addr_t)vvm, (u_int)econtig -
	    (u_int)vvm, &kvalloc);
	(void) segkmem_create(&kvalloc, (caddr_t)NULL);

KVM_HERE
	(void) seg_attach(&kas, (addr_t)HEAPBASE, (u_int)(BUFLIMIT - HEAPBASE),
	    &bufheapseg);
	(void) segkmem_create(&bufheapseg, (caddr_t)Heapptes);

KVM_HERE
	(void) seg_attach(&kas, (addr_t)SYSBASE, (u_int)(Syslimit - SYSBASE),
	    &kseg);
	(void) segkmem_create(&kseg, (caddr_t)Sysmap);

KVM_HERE
#ifdef	MMU_3LEVEL
	if (mmu_3level) {
		for (v = 0; v < (addr_t)KERNELBASE; v += SMGRPSIZE)
			mmu_smginval(v);
	} else
#endif	MMU_3LEVEL
	{
		/*
		 * For 2 level MMUs there is a hole in the middle of the
		 * virtual address space.
		 */
		for (v = (addr_t)0; v < hole_start; v += PMGRPSIZE)
			mmu_pmginval(v);
		for (v = hole_end; v < (addr_t)KERNELBASE; v += PMGRPSIZE)
			mmu_pmginval(v);
	}

KVM_HERE
	/*
	 * Duplicate kernel into every context.  From this point on,
	 * adjustments to the mmu will automatically copy kernel changes.
	 * Use the ROM to do this copy to avoid switching to unmapped
	 * context.
	 */
#if defined(SAS) && !defined(SIMUL)
	asm("t 209");		/* make all segments the same as this one */
#else
#ifdef	MMU_3LEVEL
	if (mmu_3level) {
		for (c = 1; c < NCTXS; c++) {
			register int i;
			v = (addr_t) 0;
			for (i = 0; i < NSMGRPPERCTX; i++) {
				prom_setcxsegmap(c, v,
				    (mmu_getsmg(v))->smg_num);
				v += SMGRPSIZE)
			}
		}
	} else
#endif	MMU_3LEVEL
	{
		for (c = 1; c < NCTXS; c++) {
			for (v = (addr_t)0; v < hole_start; v += PMGRPSIZE)
				prom_setcxsegmap(c, v,
				    (mmu_getpmg(v))->pmg_num);
			for (v = hole_end; v != NULL;  v += PMGRPSIZE)
				prom_setcxsegmap(c, v,
				    (mmu_getpmg(v))->pmg_num);
		}
	}
#endif SAS

KVM_HERE
	/*
	 * Initialize the kernel page maps.
	 */
	v = (addr_t)KERNELBASE;

	/* user copy red zone */
	(void) as_fault(&kas, v, PAGESIZE * 2, F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, PAGESIZE * 2, 0);
	v += PAGESIZE * 2;

KVM_HERE
	/* msgbuf */
	(void) as_fault(&kas, v, PAGESIZE * 2, F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, PAGESIZE * 2, PROT_READ | PROT_WRITE);
	v += PAGESIZE * 2;

KVM_HERE
	/*
	 * (Normally) Read-only until end of text.
	 */
	(void) as_fault(&kas, v, (u_int)(etext - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(etext - v), (u_int)
	    (PROT_READ | PROT_EXEC | ((kernprot == 0)? PROT_WRITE : 0)));
	v = (addr_t)roundup((u_int)etext, PAGESIZE);

KVM_HERE
	/*
	 * Writable until end.
	 */
	(void) as_fault(&kas, v, (u_int)(vvm - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(vvm - v),
	    PROT_READ | PROT_WRITE | PROT_EXEC);
	v = (addr_t)vvm;

KVM_HERE
	/*
	 * Validate the valloc'ed structures
	 */
	(void) as_fault(&kas, v, (u_int)(econtig - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(econtig - v),
	    PROT_READ | PROT_WRITE | PROT_EXEC);


KVM_HERE

	/*
	 *
	 * segu will reclaim some of this space when it is created.
	 *
	 */

	v = (addr_t)roundup((u_int)econtig, PMGRPSIZE);

	for (; v < (addr_t)HEAPBASE; v += PMGRPSIZE)
		mmu_pmginval(v);

	/*
	 * Heap segment
	 */
	v = (addr_t)HEAPBASE;
	(void) as_setprot(&kas, v, (u_int)((addr_t)BUFLIMIT - v), 0);

	/*
	 * XXX GX KLUDGE (1060915)
	 * Instead of assuming that GX will need 16MB, we should
	 * get the info from OBP to decide how much to reserve for GX,
	 * since only certain configurations need all this space.
	 */
	for(v = (addr_t)GX_BASE; v < (addr_t)GX_LIMIT; v += PMGRPSIZE)
		if (mmu_getpmg(v) != pmgrp_invalid) {
			hat_pmgreserve(&kseg, v);
			hat_chgprot(&kseg, v, PMGRPSIZE, ~PROT_USER);
		}

KVM_HERE
	/*
	 * Invalid to Syslimit
	 */
	v = (addr_t) SYSBASE;
	(void) as_setprot(&kas, v, (u_int)(Syslimit - v), 0);
	v = (addr_t)roundup((u_int)v, PMGRPSIZE);

KVM_HERE
	/*
	 * Invalid segment mappings until start of debugger.
	 */
	for (; v < (addr_t)DEBUGSTART; v += PMGRPSIZE)
		mmu_pmginval(v);

KVM_HERE
	/*
	 * Reserve debugger pmegs if it is present.  Only reserve
	 * those segments which are not already set invalid.
	 * Then remove user access to these addresses.
	 */
	for (; v < (addr_t)DEBUGEND; v += PMGRPSIZE) {
		if ((boothowto & RB_DEBUG) == 0)
			mmu_pmginval(v);
		else if (mmu_getpmg(v) != pmgrp_invalid) {
			hat_pmgreserve(&kseg, v);
			hat_chgprot(&kseg, v, PMGRPSIZE, ~PROT_USER);
		}
	}

KVM_HERE
	/*
	 * Invalid between debugger and monitor.
	 */
	for (; v < (addr_t)MONSTART; v += PMGRPSIZE)
		mmu_pmginval(v);

KVM_HERE
	/*
	 * Reserve monitor segments.  Only reserve those segments
	 * which have at least one valid pment.  Then be sure
	 * to remove user access to these addresses.
	 */
	for (; v < (addr_t)MONEND; v += PMGRPSIZE)
		if (mmu_getpmg(v) != pmgrp_invalid) {
			for (tv = v; tv < (v + PMGRPSIZE); tv += PAGESIZE) {
				mmu_getpte(tv, &pte);
				if (pte_valid(&pte))
					break;
			}
			if (tv < (v + PMGRPSIZE)) {
				hat_pmgreserve(&kseg, v);
				hat_chgprot(&kseg, v, PMGRPSIZE, ~PROT_USER);
			} else
				mmu_pmginval(v);
		}

KVM_HERE
	/*
	 * Invalidate rest of segments up to final pmeg.
	 */
	for (; v < (addr_t)-PMGRPSIZE; v += PMGRPSIZE)
		mmu_pmginval(v);

KVM_HERE
	/*
	 * Loop through the last segment and set page protections.
	 * We want to invalidate any other pages in last segment
	 * besides the u area, EEPROM_ADDR, COUNTER_ADDR, MEMERR_ADDR,
	 * AUXIO_ADDR, and INTREG_ADDR.
	 * This sets up the kernel redzone below the u area.
	 * We get the interrupt redzone for free when the kernel is
	 * write protected as the interrupt stack is the first thing
	 * in the data area.
	 */
	hat_pmgreserve(&kseg, v);
	for (; v < (caddr_t)(0-(NBPG*UPAGES)); v += PAGESIZE) {
		if ((u_int)v != (u_int)EEPROM_ADDR &&
		    (u_int)v != (u_int)COUNTER_ADDR &&
		    (u_int)v != (u_int)MEMERR_ADDR &&
		    (u_int)v != (u_int)AUXIO_ADDR &&
		    (u_int)v != (u_int)INTREG_ADDR)
			mmu_setpte(v, mmu_pteinvalid);
	}

KVM_HERE
	/*
	 * Invalidate all the unlocked pmegs.
	 */
	hat_pmginit();

KVM_HERE
	/*
	 * Now create a segment for the DVMA virtual
	 * addresses using the segkmem segment driver.
	 */
	(void) seg_attach(&kas, DVMA, (u_int)ctob(dvmasize), &kdvmaseg);
	(void) segkmem_create(&kdvmaseg, (caddr_t)NULL);

KVM_HERE
	/*
	 * Allocate pmegs for DVMA space.
	 */
	hat_reserve(&kdvmaseg, (addr_t)DVMA, (u_int)ctob(dvmasize));

KVM_HERE
	/*
	 * Wire down pmegs for temporaries
	 */
	hat_reserve(&kseg, CADDR1, MMU_PAGESIZE);
	hat_reserve(&kseg, CADDR2, MMU_PAGESIZE);

	/*
	 * Find the beginning page frames of the kernel data
	 * segment and the ending pageframe (-1) for bss.
	 */

	mmu_getpte((addr_t) (roundup ((u_int)etext, DATA_ALIGN)), &pte);
	kpfn_dataseg = pte.pg_pfnum;
	mmu_getpte((addr_t) end, &pte);
	kpfn_endbss = pte.pg_pfnum;

KVM_DONE

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
		(void)splx(s);
		va = Sysbase + mmu_ptob(a);
		hat_pteload(&kseg, va, (struct page *)NULL, mmu_pteinvalid, 1);
	}

	if (!pp->p_mapping) {
		pte.u_ipte = PG_V | PG_KW | PGT_OBMEM | page_pptonum(pp);
		mmu_setpte(va, pte.u_pte);
		dohat = 0;
	} else {
		hat_mempte(pp, PROT_READ | PROT_WRITE, &pte.u_pte);
		hat_pteload(&kseg, va, pp, pte.u_pte,
		    PTELD_LOCK | PTELD_NOSYNC);
		dohat = 1;
	}

	(void) copyin((caddr_t)addr, va, PAGESIZE);

	if (dohat) {
		hat_unload(&kseg, va, PAGESIZE);
	} else {
		vac_pageflush(va);
		mmu_setpte(va, mmu_pteinvalid);
	}

	if ((va_cache_valid & VA_CACHE_MASK) == VA_CACHE_MASK) {
		/* cache is full, just return the virtual space */
		hat_unlock(&kseg, va);
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
	int dohat;
	union {
		struct pte u_pte;
		int u_ipte;
	} pte;

	ASSERT((int)len > 0 && (int)off >= 0 && off + len <= PAGESIZE);

	if (!pp->p_mapping) {
		pte.u_ipte = PG_V | PG_KW | PGT_OBMEM | page_pptonum(pp);
		mmu_setpte((addr_t) CADDR2, pte.u_pte);
		dohat = 0;
	} else {
		hat_mempte(pp, PROT_READ | PROT_WRITE, &pte.u_pte);
		hat_pteload(&kseg, CADDR2, pp, pte.u_pte,
		    PTELD_LOCK | PTELD_NOSYNC);
		dohat = 1;
	}

	bzero(CADDR2 + off, len);

	if (dohat) {
		hat_unload(&kseg, CADDR2, PAGESIZE);
	} else {
		vac_pageflush(CADDR2);
		mmu_setpte((addr_t) CADDR2, mmu_pteinvalid);
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
		pte.u_ipte = PG_V | PG_KW | PGT_OBMEM | page_pptonum(pp);
		mmu_setpte((addr_t) CADDR2, pte.u_pte);
		dohat = 0;
	} else {
		hat_mempte(pp, PROT_READ | PROT_WRITE, &pte.u_pte);
		hat_pteload(&kseg, CADDR2, pp, pte.u_pte,
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
		mmu_setpte((addr_t) CADDR2, mmu_pteinvalid);
	}

	return (checksum);
}
#endif VMEMDEBUG

/*#ifndef SAS*/
/*
 * Console put and get character routines.
 */
cnputc(c)
	register int c;
{
	register int s;
	void prom_putchar();

	s = spl7();
	if (c == '\n')
		prom_putchar('\r');
	prom_putchar(c);
	(void) splx(s);
}

int
cngetc()
{
	u_char prom_getchar();

	return ((int)prom_getchar());
}
/*#endif !SAS*/

#ifdef SAS
/*
 * We don't want a kernel running under SAS to spend an inordinate
 * amount of time in the idle loop, so the idle loop calls
 * fake_clockticks() to bump up to the next event in the timeout queue.
 * This is only done when there is nothing to run (whichqs is zero).
 */
fake_clockticks()
{
	register caddr_t pc;
	register int ps;
	register int s;
	register int n;

	pc = (caddr_t) fake_clockticks;
	ps = PSR_PS | PSR_S;	/* attribute to the system */
	s = splclock();
	/* We only want to dispatch one event, so capture current c_time */
	n = calltodo.c_next->c_time;
	while (n-- > 0)
		hardclock(pc, ps);
	s = splx(s);
}
#endif SAS

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
	if (newseg == (struct seguser *) 0) {
		panic("can't create kernel process");
	}
	fp = (struct frame *)KSTACK(newseg);
	fp->fr_local[6] = (u_int)startaddr;	/* l6 gets start address */
	fp->fr_local[0] = arg;			/* l0 gets the argument */
	fp->fr_local[5] = 1;			/* means fork a kernel proc */
	pp = freeproc;
	freeproc = pp->p_nxt;
	newproc(0, (struct as *)0, (int)0, (struct file *)0, newseg, pp);
}


/*
 * For things that depend on register state being on the stack,
 * copy any register windows that get saved into the window buffer
 * (in the uarea) onto the stack.  This normally gets fixed up
 * before returning to a user program.  Callers of this routine
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
 * set delay constant for usec_delay()
 * delay ~= (usecs * (Cpudelay * 2 + 3) + 8) / mips
 *  => Cpudelay = ceil((mips - 3) / 2)
 * XXX should be in sparc_subr.s with usec_delay()
 */
static
setdelay(mips)
	int mips;
{
	extern int Cpudelay;

	Cpudelay = 0;
	if (mips > 3)
		Cpudelay = (mips - 2) >> 1;
}

/*
 * Flush all user lines in VAC.
 */
void
vac_flushallctx()
{
	register int i;
	u_int map_getctx();

	if (ctxs[map_getctx()].c_clean)
		return;

	vac_usrflush();
	for (i = 1; i < NCTXS; i++)	/* skip kernel context */
		ctxs[i].c_clean = 1;
}
