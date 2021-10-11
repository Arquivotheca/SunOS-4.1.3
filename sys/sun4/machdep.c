#ifndef	lint
static	char sccsid[] = "@(#)machdep.c 1.1 92/07/30 SMI";
#endif	lint

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
#include <machine/asm_linkage.h>
#include <machine/eeprom.h>
#include <machine/intreg.h>
#include <machine/memerr.h>
#include <machine/eccreg.h>
#include <machine/seg_kmem.h>
#include <machine/enable.h>

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
int use_vac = 1;	/* patch to 0 to not use the VAC */
#else	!VAC
#define	use_vac 0
#endif	!VAC

#ifdef	IOC
int use_ioc = 1;	/* patch to 0 to have the kernel not use IOC */
int ioc_net = 1;	/* patch to 0 to not iocache the network */
int ioc_vme = 1;	/* patch to 0 to not iocache vme */
int ioc_debug = 0;	/* patch to 1 to debug ioc code */
#else	!IOC
#define	use_ioc 0
#define	ioc_net 0
#define	ioc_vme 0
#define	ioc_debug 0
#endif	!IOC

#ifdef	BCOPY_BUF
int bcopy_res = -1;	/* block copy buffer reserved (in use) */
int use_bcopy = 1;	/* patch to 0 to not use bcopy buffer */
int bcopy_cnt = 0;	/* number of bcopys that used the buffer */
int bzero_cnt = 0;	/* number of bzeros that used the buffer */
#endif	BCOPY_BUF

/*
 * Configuration parameters set at boot time.
 */
struct	cpuinfo {
	u_int cpui_mmutype : 1; 	/* mmu type, 0=2 level, 1=3 level */
	u_int cpui_vac : 1;		/* has virtually addressed cache */
	u_int cpui_ioctype : 2;		/* I/O cache type, 0 = none */
	u_int cpui_iom : 1;		/* has I/O MMU */
	u_int cpui_clocktype : 2;	/* clock type */
	u_int cpui_bcopy_buf : 1;	/* has bcopy buffer */
	u_int cpui_sscrub : 1;		/* requires software ECC scrub */
	u_int cpui_linesize : 8;	/* cache line size (if any) */
	u_short cpui_nlines;		/* number of cache lines */
	u_short cpui_dvmasize;		/* size of DMA area (in MMU pages) */
	u_short cpui_cpudelay;		/* cpu delay factor (cache on) */
	u_short cpui_ncdelay;		/* cpu delay factor (cache off) */
	u_short cpui_nctx;		/* number of contexts */
	u_int cpui_nsme;		/* number of segment map entries */
	u_int cpui_npme;		/* number of page map entries */
} cpuinfo[] = {
/* mmu vac ioc iom clk bcp scr lsz  nln  dsz cdly ndly nctx  nsme  npme */
/* 4_260 */
{   0,  1,  0,  0,  0,  0,  0, 16, 8192, 126,  7,   2,  16, 65536, 16384 },
/* 4_110 */
{   0,  0,  0,  0,  0,  0,  0,  0,    0, 126,  7,   7,   8, 32768,  8192 },
/* 4_330 */
{   0,  1,  0,  0,  1,  0,  0, 16, 8192, 126, 11,   4,  16, 65536,  8192 },
/* 4_470 */
{   1,  1,  1,  0,  1,  1,  0, 32, 4096, 126, 16,   3,  64, 16384, 32768 },
};
struct	cpuinfo *cpuinfop;	/* misc machine info */
u_int nctxs;			/* number of implemented contexts */
#ifdef	MMU_3LEVEL
u_int nsmgrps;			/* number of smgrps in segment map */
#endif	MMU_3LEVEL
u_int npmgrps;			/* number of pmgrps in page map */
u_int segmask;			/* mask for segment numbers */
addr_t hole_start;		/* addr of start of MMU "hole" */
addr_t hole_end;		/* addr of end of MMU "hole" */
int hole_shift;			/* "hole" check shift to get high addr bits */
addr_t econtig;			/* End of first block of contiguous kernel */
addr_t vstart;			/* Start of valloc'ed region */
addr_t vvm;			/* Start of valloc'ed region we can mmap */
u_int vmsize;			/* size of vm structures */
int vac_linesize;		/* size of a cache line */
int vac_nlines;			/* number of lines in the cache */
int vac_pglines;		/* number of cache lines in a page */
int vac_size;			/* cache size in bytes */
int Cpudelay;			/* delay loop count/usec */
int cpudelay = 0;		/* for compatibility with old macros */
int dvmasize;			/* usable dvma space (in MMU pages) */
u_int shm_alignment;		/* VAC address consistency modulus */
extern int fastscan;		/* from ../os/vm_pageout.c, tuned for 470 */

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

#ifdef	CLK2
int clk2 = 0;			/* clock type, default to clock type 1 */
#endif	CLK2

extern int npmgrpssw;
extern int npmghash;

int	vac_hashwusrflush;	/* set to 1 if cache has HW user flush */
/*
 * Dynamically allocated MMU structures
 */
extern struct   ctx *ctxs, *ctxsNCTXS;
extern struct   pmgrp *pmgrps, *pmgrpsNPMGRPS;
extern struct   pment *pments, *pmentsNPMENTS;
extern struct   pte   *ptes,   *ptesNPTES;
extern struct   hwpmg *hwpmgs, *hwpmgsNHWPMGS;
extern struct   pmgrp   **pmghash, **pmghashNPMGHASH;
#ifdef  MMU_3LEVEL
extern struct   smgrp *smgrps, *smgrpsNSMGRPS;
extern struct   sment *sments, *smentsNSMENTS;
#endif  MMU_3LEVEL

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
struct	memseg memseg0;		/* phys mem segment struct for page 0 */
struct	memseg memseg;		/* phys mem segment struct for most memory */


/*
 * For implementations that have fewer than the default number of page
 * frame bits (19), startup() patch back this variable as appropriate.
 */

u_long pfnumbits = PG_PFNUM & ~PG_TYPE;

#define	TESTVAL	0xA55A		/* memory test value */

/*
 *		 Physical memory layout.
 *		 _______________________
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
 *		|  hat data structures	|
 *		|-----------------------|
 *		|    kernel data, bss	|
 *		|-----------------------|
 *		|    interrupt stack	|
 *		|-----------------------|
 *		|    kernel text (RO)	|
 *		|-----------------------|
 *		|    trap table (4k)	|
 *		|-----------------------|
 *	page 1	|	 msgbuf		|
 *		|-----------------------|
 *	page 0	| unused - reclaimed	|
 *		 -----------------------
 *
 *
 *		 Virtual memory layout.
 *
 *	The Intel 82586 ethernet chip needs to access 0xFFFFFFF4
 *	for initialization so we burn a page at the top just for it
 *	(see if_ie.c).
 *		 _______________________
 *		|      ie init page	|
 * 0xFFFFE000  -|-----------------------|
 *		|  ethernet descriptors |
 * 0xFFFFC000  -|-----------------------|
 *		|	  DVMA		|
 *		|      IOPB memory	|
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
 * 0xFF946000	|-----------------------|
 *		|  	NCARGS (1MB)	|
 * 0xFF846000  -|-----------------------|- Syslimit
 *		|	Mbmap		|
 *		|  MBPOOLMMUPAGES (2MB)	|
 * 0xFF646000  -|-----------------------|- mbutl
 *		|	mmap (1 page)	|
 * 0xFF644000  -|-----------------------|- vmmap
 *		|	CMAP2 (1 page)	|
 * 0xFF642000  -|-----------------------|- CADDR2
 *		|	CMAP1 (1 page)	|
 * 0xFF640000  -|-----------------------|- CADDR1
 *		|	kernelmap	|
 *		|    SYSPTSIZE (6.25MB)	|
 * 0xFF000000  -|-----------------------|- Sysbase
 *		|        unused		|
 * 0xFEFD2000  -|-----------------------|
 *		|     iocache tags	|
 * 0xFEFD1000  -|-----------------------|
 *		|     iocache data	|
 * 0xFEFD0000  -|-----------------------|
 *		|     iocache flush	|
 * 0xFEFCC000  -|-----------------------|
 *		|    counter - timer 	|
 * 0xFEFCA000  -|-----------------------|
 *		|    interrupt reg	|
 * 0xFEFC8000  -|-----------------------|
 *		|   memory error reg	|
 * 0xFEFC6000  -|-----------------------|
 *		|    memory ecc reg	|
 * 0xFEFC4000  -|-----------------------|
 *		|	eeprom		|
 * 0xFEFC2000  -|-----------------------|
 *		|	 clock		|
 * 0xFEFC0000  -|-----------------------|- MDEVBASE (VXMVX_LIMIT)
 *		| Monitor large frame   |
 *		|       buffer mappings |
 * 0xFE000000  -|-----------------------|- BUFLIMIT (VXMVX_BASE)
 *		|   Buffer Cache (2MB)	|
 * 0xFDE00000	|-----------------------|
 *		|   Kernel Heap (16MB)	|
 * 0xFCE00000  -|-----------------------|- HEAPBASE
 *		|	  unused	|
 *		|-----------------------|
 *		|	  segu		|
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
 *		|      (invalid)	|
 * 0xF8000000  -|-----------------------|- KERNELBASE
 *		|      user stack	|
 *		:			:
 *		:			:
 *		|      user data	|
 *		|-----------------------|
 *		|      user text	|
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
	register addr_t addr;
	u_int firstaddr;		/* first free physical page number */
	u_int mapaddr, npages, page_base;
	struct page *pp;
	struct segmap_crargs a;
	u_int map_getsgmap();
	int mon_mem;
#ifndef	SAS
	extern void v_handler();
#endif	!SAS
	extern char etext[], end[], CADDR1[], Syslimit[];
	extern trapvec kclock14_vec;
	extern struct scb scb;
	extern int clock_type;
	extern char *cpu_str;


	/*
	 * romp->v_memorybitmap contains the address of a pointer to
	 * an array of bits. If this pointer is non-zero, then we had
	 * some bad pages. Until we get smarter, we give up if we find
	 * this condition.
	 */
#ifdef	notdef
	if (romp->v_memorybitmap)
		halt("Memory bad");
#endif
	setcputype();

	cpuinfop = &cpuinfo[(cpu & CPU_MACH) - 1];

	clock_type = cpuinfop->cpui_clocktype;

	Cpudelay = cpuinfop->cpui_ncdelay;	/* cache gets turned on later */
	dvmasize = cpuinfop->cpui_dvmasize;

#ifdef	VAC
	vac = cpuinfop->cpui_vac;
#endif	VAC
#ifdef	IOC
	ioc = cpuinfop->cpui_ioctype;
#endif	IOC
#ifdef	BCOPY_BUF
	bcopy_buf = cpuinfop->cpui_bcopy_buf;
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
	mmu_3level = cpuinfop->cpui_mmutype;
#endif	MMU_3LEVEL

	nctxs = cpuinfop->cpui_nctx;
	npmgrps = cpuinfop->cpui_npme / NPMENTPERPMGRP;
	segmask = PMGRP_INVALID;

#ifdef	MMU_3LEVEL
	if (mmu_3level) {
		/* no hole in a 3 level mmu, just set it halfway */
		nsmgrps = cpuinfop->cpui_nsme / NSMENTPERSMGRP;
		hole_shift = 31;
		hole_start = (addr_t)(1 << hole_shift);
	} else
#endif	MMU_3LEVEL
	{
		register u_int npmgrpperctx;
		/* compute hole size in 2 level MMU */
		npmgrpperctx = cpuinfop->cpui_nsme / nctxs;
		hole_start = (addr_t)((npmgrpperctx / 2) * PMGRPSIZE);
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
#ifdef	MMU_3LEVEL
	if (!mmu_3level)
#endif
		ASSERT(hole_shift < 31);
#endif	VA_HOLE

	/*
	 * The default stack size of 8M allows an optimization of mmu mapping
	 * resources so that in normal use a single mmu region map entry (smeg)
	 * can be used to map both the stack and shared libraries.
	 */
	if (dflssiz == 0)
		dflssiz = (8*1024*1024);
	if (vac) {
		vac_linesize = cpuinfop->cpui_linesize;
		vac_nlines = cpuinfop->cpui_nlines;
		vac_pglines = PAGESIZE / vac_linesize;
		vac_size = vac_nlines * vac_linesize;
#ifdef  MMU_3LEVEL
		if (mmu_3level)
			vac_hashwusrflush = 1;
#endif  MMU_3LEVEL
	}

	/*
	 * Map in devices.
	 * For 3 level MMU we use the middle smeg in the segment map
	 * We always use the middle pmeg in the page map.
	 */
#ifdef	MMU_3LEVEL
/*
 * Adjustment for SunMon 4.0 large frame buffer mappings...
 *
 *  OpenBoot writes text to a frame buffer directly via a memory-
 *  mapped pointer to its display memory.  This requires an 
 *  additional virtual address space to be reserved for this use.
 *  Here we have taken a nearly 16MB (from 0xfe00.0000 to 
 *  MDEVBASE(0xfefc.0000)) that was previously unused.  The
 *  mappings are reserved ONLY if SunMon needed to use this area!
 *
 *  NOTE: SunMon versions prior to 4.x left a "ramp" of address maps
 *  set-up in this unused range.  This caused the tests for valid
 *  page-map-groups that we now use to determine if SunMon used
 *  these virtual addresses to pass even though the mapping is really
 *  invalid!  This prevents SunOS from booting reporting these errors:
 *	 assertion failed: pmg->pmg_sme == NULL,
 *	 				file: ../../sun4/vm_hat.c, line: 2974
 *	 panic: assertion failed
 *	 rebooting...
 *
 *  A test for the SunMon version fixes this problem.
 *
 *  Additional adjustments similar to this are made in "kvm_init()".
 */
	if ((romp->v_mon_id[0]=='4')) {
	    if (mmu_3level) {
		int iret = map_getrgnmap ((addr_t)MDEVBASE);
		if (iret == 0xff) map_setrgnmap((addr_t)MDEVBASE, nsmgrps / 2);
	    }
	}
	else {
	    if (mmu_3level) 
	    map_setrgnmap((addr_t)MDEVBASE, nsmgrps / 2);
	}
#endif
	map_setsgmap((addr_t)MDEVBASE, npmgrps / 2);


	/* init pmeg */
	for (addr = (addr_t)MDEVBASE; addr < (addr_t)MDEVBASE + PMGRPSIZE;
	    addr += MMU_PAGESIZE)
		map_setpgmap(addr, 0);

	map_setpgmap((addr_t)EEPROM_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_EEPROM_ADDR));

	map_setpgmap((addr_t)CLOCK0_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_CLOCK_ADDR));

	map_setpgmap((addr_t)MEMERR_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_MEMERR_ADDR));

	map_setpgmap((addr_t)INTREG_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_INTREG_ADDR));

	map_setpgmap((addr_t)ECCREG_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_ECCREG0_ADDR));


	if (cpu == CPU_SUN4_330 || cpu == CPU_SUN4_470)
		map_setpgmap((addr_t)COUNTER_ADDR,
			PG_V | PG_KW | PGT_OBIO | btop(OBIO_COUNTER_ADDR));

	if (cpu == CPU_SUN4_470)
		/* remap eccreg for Sunray */
		map_setpgmap((addr_t)ECCREG_ADDR,
			PG_V | PG_KW | PGT_OBIO | btop(OBIO_ECCREG1_ADDR));

	/*
	 * Save the kernels level 14 interrupt vector code and install
	 * the monitor's. This lets the monitor run the console until we
	 * take it over.
	 */
	kclock14_vec = scb.interrupts[14 - 1];
	start_mon_clock();
	(void)splzs();			/* we can take hi clock ints now */

	bootflags();			/* get the boot options */

	/*
	 * If the boot flags say that the debugger is there,
	 * test and see if it really is by peeking at DVEC.
	 * If is isn't, we turn off the RB_DEBUG flag else
	 * we call the debugger scbsync() routine.
	 */
#ifndef	SAS
	if ((boothowto & RB_DEBUG) != 0) {
		if (peek((short *)DVEC) == -1)
			boothowto &= ~RB_DEBUG;
		else
			(*dvec->dv_scbsync)();
	}
#endif	!SAS

	/*
	 * allow interrupts now,
	 * after memory error handling has been initialized
	 */
	*INTREG |= IR_ENA_INT;

#ifdef	SAS
	mon_mem = 0;
	physmem = btop(availmem);
	physmax = physmem - 1;
#else
	/*
	 * v_vector_cmd is the handler for the monitor vector command .
	 * We install v_handler() there for Unix.
	 */
	*romp->v_vector_cmd = v_handler;

	/*
	 * v_memorysize is the amount of physical memory while
	 * v_memoryavail is the amount of usable memory. Mon_mem
	 * is the difference which is the number of pages hidden
	 * by the monitor.
	 */
	mon_mem = btop(*romp->v_memorysize - *romp->v_memoryavail);

	/*
	 * If physmem is patched to be non-zero, use it instead of
	 * the monitor value unless physmem is larger than the total
	 * amount of memory on hand.
	 */
	if (physmem == 0 || physmem > btop(*romp->v_memorysize))
		physmem = btop(*romp->v_memorysize);
	/*
	 * Adjust physmem down for the pages stolen by the monitor.
	 */
	physmem -= mon_mem;

	/*
	 * physmax is the highest numbered page in the system,
	 * irrespective of physmem (for dumping).
	 */
	physmax = btop(*romp->v_memorysize) - 1;

	/*
	 * If debugger is in memory, subtract the pages it stole from physmem.
	 * By doing this here instead of adding to mon_mem, the physical
	 * memory message display by UNIX will show memory loss for debugger.
	 */
	if (boothowto & RB_DEBUG)
		physmem -= *dvec->dv_pages;

	/*
	 * for sunray, if fastscan has not been patched,
	 * set the fastscan to 1/4 of the size of free memory.
	 * This overrides similar code in vm_pageout.c, which
	 * sets fastscan to 200 pages if not set already --
	 * no good for Sunray.
	 * XXX This is a hack. Put a clean fix here
	 */
	if (cpu == CPU_SUN4_470)
		if (fastscan == 0)
			fastscan = physmem/4;

#endif	SAS
	maxmem = physmem - IOPBMEM;

	if (nrnode == 0) {
		nrnode = ncsize << 1;
	}

	if (npmgrpssw == 0) {
		npmgrpssw = 8 * (ptob(physmem) / PMGRPSIZE);

		/*
		 * There is no point in having less SW than HW pmegs.
		 */
		if (npmgrpssw < NPMGRPS)
			npmgrpssw = NPMGRPS;
	}

	/*
	 * Number of look aside entries for hat_pmgfind.
	 * Round up to the nearest power of two.
	 */
	npmghash = (2 * npmgrpssw) - 1;

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
#ifdef  MMU_3LEVEL
	if (mmu_3level) {
		valloclim(smgrps, struct smgrp, NSMGRPS, smgrpsNSMGRPS);
		valloclim(sments, struct sment, (NSMGRPS * NSMENTPERSMGRP),
			smentsNSMENTS);
	}
#endif  MMU_3LEVEL
	valloclim(pmgrps, struct pmgrp, NPMGRPSSW, pmgrpsNPMGRPS);
	valloclim(pments, struct pment, (NPMGRPSSW * NPMENTPERPMGRP),
		pmentsNPMENTS);
	valloclim(ptes, struct pte, (NPMGRPSSW * NPMENTPERPMGRP), ptesNPTES);
	valloclim(hwpmgs, struct hwpmg, NPMGRPS, hwpmgsNHWPMGS);
	valloclim(pmghash, struct pmgrp *, NPMGHASH, pmghashNPMGHASH);

	/*
	 * Vallocate the remaining kernel data structures.
	 * The first available real memory address is in "firstaddr".
	 * "mapaddr" is the real memory address where the tables start.
	 * It is used when remapping the tables later.
	 */
	v = vvm = (addr_t)roundup((u_int)v, PAGESIZE);
	vmsize = v - vstart;	/* size of vm structures */
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
	valloc(mb_hd.mh_map, struct map, NDVMAMAPS(dvmasize));
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
	 * Allocate space for page structures and hash table.
	 * Get enough for all of physical memory minus amount
	 * dedicated to the system.
	 */
	page_base = mapaddr;
	npages = physmem - page_base;
	npages++;			/* add one to reclaim page zero */
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
	 * Make sure that all valloc'ed structures are mapped.
	 *
	 * Note that we must map up to econtig (not just vvm)!!
	 */
	mapvalloced(econtig);

	/*
	 * Initialize the hat layer.
	 */
	bzero((caddr_t)vstart, vmsize);
	hat_init();

	/*
	 * Initialize VM system, and map kernel address space.
	 */
	kvm_init();

#if	 defined(SUN4_330) || defined(SUN4_470)
	/* fix for bug in SUNRAY IU version 1, bit 0 of ASI 1 cycle late */
	if ((getpsr()>>24) == 0x10)
		hat_chgprot(&kseg, (addr_t) &scb, PAGESIZE, PROT_USER);
#endif	defined (SUN4_330)
#if	defined(SUN4_330)
	/* Sun-4/330  dislikes cache hits on 1st cycle of a trap */
	if (cpu == CPU_SUN4_330)
		vac_dontcache ((addr_t)&scb);
#endif

	/*
	 * Determine if anything on the VME bus lives in the range of
	 * VME addresses (low 1 Mb) that correspond with system DVMA.
	 * We go through both the 16 bit and 32 bit device types.
	 */
#ifndef	SAS
	disable_dvma();
	for (i = 0; i < dvmasize; i++) {
		segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
		    PROT_READ | PROT_WRITE, (u_int)(i | PGT_VME_D16), 0);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
		segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
		    PROT_READ | PROT_WRITE, (u_int)(i | PGT_VME_D32), 0);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
	}
	if (i < dvmasize) {
		printf("CAN'T HAVE PERIPHERALS IN RANGE 0 - %dKB\n",
		    mmu_ptob(dvmasize) / 1024);
		halt("dvma collision");
	}
	enable_dvma();
#endif

#ifdef	VAC
	if (vac) {
		extern int nopagereclaim;

		/*
		 * The Sun-4/260's have a hardware bug which causes
		 * non-ready floating point numbers to not be waited
		 * for before they are stored to non-cached pages.
		 * The most common way user's get non-cached pages is
		 * when they "reclaim" a page which already mapped
		 * into DVMA space.  As a kludge effort to reduce the
		 * likelyhood of this happening, we turn off page
		 * reclaims on Sun-4/260's.  Yuck...
		 */
		vac_init();		/* initialize cache */
		if (cpu == CPU_SUN4_260)
			nopagereclaim = 1;
		if (use_vac) {
			on_enablereg(ENA_CACHE);	/* turn on cache */
			vac = 1;		/* indicate cache is on */
			Cpudelay = cpuinfop->cpui_cpudelay;
			shm_alignment = vac_size;
		} else  {
			printf("CPU CACHE IS OFF!\n");
			vac = 0;
			shm_alignment = PAGESIZE;
		}
	}
#endif	VAC

#ifdef	SUN4_110
	if (cpu == CPU_SUN4_110) {
		/*
		 * 4/110s have only 15 bits worth of valid page frame number.
		 */
		pfnumbits &= 0x7fff;
	}
#endif	SUN4_110

#ifdef	IOC
	if (ioc) {
		ioc_init();
		if (use_ioc)
			on_enablereg(ENA_IOCACHE);
		else
			printf("IO CACHE IS OFF!\n");
	}
	if (!(ioc && use_ioc)) {
		ioc = 0;
		ioc_net = 0;
		ioc_vme = 0;
		ioc_debug = 0;
	}
#endif	IOC

	/*
	 * Good {morning, afternoon, evening, night}.
	 * When printing memory, use the total including
	 * those hidden by the monitor (mon_mem).
	 */
	printf(version);
	printf("cpu = %s\n", cpu_str);

#ifdef	SAS
	printf("mem = %dK (0x%x)\n", availmem / 1024, availmem);
#else
	printf("mem = %dK (0x%x)\n", ctob(physmem + mon_mem) / 1024,
	    ctob(physmem + mon_mem));
#endif	SAS

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
	 * which would depend on disk traffic.
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
	if (unixsize >= physmem - 8*UPAGES)
		halt("no memory");

	segkmem_mapin(&kvalloc, (addr_t)vvm, (u_int)(econtig - vvm),
		PROT_READ | PROT_WRITE, mapaddr, 0);

	bzero((caddr_t)vvm, (u_int)(econtig - vvm));

	/*
	 * Initialize more of the page structures.
	 */
	page_init(pp, 1, 0, &memseg0);		/* page 0 memory segment */
	page_init(&pp[1], npages-1, page_base, &memseg); /* everything else */

#ifdef	VAC
	/*
	 * If we are running on a machine with a virtual address cache, we call
	 * mapin again for the valloc'ed memory now that we have the page
	 * structures set up so the hat routines leave the translations cached.
	 */
	if (vac) {
		segkmem_mapin(&kvalloc, (addr_t)vvm, (u_int)(econtig - vvm),
			PROT_READ | PROT_WRITE, mapaddr, 0);
	}
#endif	VAC

	/*
	 * Initialize callouts.
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i - 1].c_next = &callout[i];

	/*
	 * initialize handling of memory errors
	 * IMPORTANT: memerr_init() initializes ecc regs and turns on ecc
	 * reporting. If an ecc error recieved at this time, a timeout
	 * will be set up. Timeout expects callouts be initialized.
	 * Hence, memerr_init should be done AFTER callouts are initialized.
	 * Orelse, an ecc error will cause a panic 'timeout table overflow'.
	 */

	memerr_init();

	/*
	 * Initialize memory free list.
	 */
	memialloc(0, 1);		/* add page zero to the free list */
	memialloc(mapaddr + btoc(econtig - vvm), (u_int)maxmem);
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
	rminit(mb_hd.mh_map, (long)(dvmasize - IOPBMEM), (u_long)IOPBMEM,
	    "DVMA map space", NDVMAMAPS(dvmasize));

	/*
	 * Allocate IOPB memory space just below the end of
	 * memory and map it to the first pages of DVMA space.
	 * The memory from maxmem to physmem has page structs
	 * already allocated but which are not on the free list.
	 */
	segkmem_mapin(&kdvmaseg, DVMA, mmu_ptob(IOPBMEM),
	    PROT_READ | PROT_WRITE, (u_int)maxmem, 0);

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

#ifdef	SAS
	boothowto = RB_SINGLE | RB_NOBOOTRC | RB_WRITABLE;
#else
	bp = (*romp->v_bootparam);
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
#endif	SAS
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

#ifdef	PGINPROF
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
#endif	PGINPROF

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
		pcb->pcb_fpctxp = 0;
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
	register int i;
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
	flush_user_windows_to_stack();
	u.u_onstack = scp->sc_onstack & 01;
	u.u_procp->p_sigmask = scp->sc_mask & ~cantmask;
	regs[SP] = scp->sc_sp;
	regs[PC] = scp->sc_pc;
	regs[nPC] = scp->sc_npc;
	regs[PSR] = regs[PSR] & ~PSL_USERMASK | scp->sc_psr & PSL_USERMASK;
	regs[G1] = scp->sc_g1;	/* restore regs signal handler can't restore */
	regs[O0] = scp->sc_o0;
	if ((u_int)scp->sc_wbcnt < nwindows) { /* restore outstanding windows */
		register int j = pcb->pcb_wbcnt;
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
#ifdef	SAS
	asm("t 255");
#else
	/* extreme priority; allow clock interrupts to monitor at level 14 */
	s = splzs();
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
		*INTREG &= ~IR_ENA_INT;		/* disable all interrupts */
		if (howto & RB_STRING)
			(*romp->v_boot_me)(bootstr);
		else
			(*romp->v_boot_me)(howto & RB_SINGLE ? "-s" : "");
		/*NOTREACHED*/
	}
	(void) splx(s);
#endif	SAS
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
	lomem = memseg.pages_base;
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
#ifdef SAS
	return (addr < availmem);
#else !SAS
	return (addr < *romp->v_memorysize);
#endif !SAS
}

/*
 * Machine-dependent portion of dump-checking;
 * mark all pages for dumping.
 */
dump_allpages_machdep()
{
	u_int	lophys, hiphys;
	u_int	i;

	lophys = 0;
	hiphys = mmu_btop(*romp->v_memorysize);

	for (i = lophys; i < hiphys; i++)
		dump_addpage(i);
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
	mmu_setpte(addr, pte);
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
		pte[0] = pte[1]; /* BUG 1025311: lint doesn't like this */
#endif lint
		mmu_setpte(addr, pte[0]);
		mmu_getpte(kaddr + MMU_PAGESIZE, &tpte);
		pte[1].pg_pfnum = (tpte.pg_v && tpte.pg_type == OBMEM) ?
					tpte.pg_pfnum : 0;
		mmu_setpte(addr + MMU_PAGESIZE, pte[1]);
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
#ifdef	SAS
	if (s)
		printf("(%s) ", s);
	printf("Halted\n\n");
	asm("t 255");
#else
	start_mon_clock();
	if (s)
		(*romp->v_printf)("(%s) ", s);
	(*romp->v_printf)("Halted\n\n");
	(*romp->v_exit_to_mon)();
	stop_mon_clock();
#endif	SAS
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
	int use_segkmem, set_ioc = 0;

	/*
	 * Select where to find the pte values we need.
	 * They can either be gotten from a list of page
	 * structures hung off the bp, or are already
	 * available (in Sysmap), or must be read from
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

	if (
#ifdef VAC
	    !vac &&
#endif
	    kaddr >= DVMA) {
		/*
		 * No vac on this machine and we're setting up a DVMA mapping
		 * so we can avoid all the overhead of mapin and go directly
		 * to the hardware.
		 */
		use_segkmem = 0;
#ifdef	IOC
		if (ioc) {
			if (bp->b_flags & B_READ) {
				if ((((u_int)bp->b_un.b_addr &
				    IOC_LINEMASK) == 0) &&
				    ((bp->b_bcount & IOC_LINEMASK) == 0))
					set_ioc = 1;
			} else
				set_ioc = 1;
		}
#endif	IOC
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
		if (kaddr >= DVMA) {
			flags |= PTELD_INTREP;
#ifdef	IOC
			if (ioc) {
				if (bp->b_flags & B_READ) {
					if ((((u_int)bp->b_un.b_addr &
						IOC_LINEMASK) == 0) &&
					    ((bp->b_bcount & IOC_LINEMASK)
						== 0))
						flags |= PTELD_IOCACHE;
				} else
					flags |= PTELD_IOCACHE;
			}
#endif	IOC
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
#ifdef	IOC
			if (flags & PTELD_IOCACHE)
				check_ioctag(kaddr);
#endif	/* IOC */
			segkmem_mapin(seg, kaddr, MMU_PAGESIZE,
			    PROT_READ | PROT_WRITE, MAKE_PFNUM(pte), flags);
		} else {
			mmu_setpte(kaddr, *pte);
#ifdef	IOC
			if (set_ioc) {
				check_ioctag(kaddr);
				ioc_pteset(pte);
			}
#endif	/* IOC */
		}
		/*
		 * adjust values of interest
		 */
		kaddr += MMU_PAGESIZE;
		npf--;
	}
}

/*
 * Buscheck is called by mb_mapalloc, physio, and some device drivers
 * to check to see it the requested setup is a valid busmem type
 * (i.e, obmem, vme16, vme24, vme32).
 *
 * Returns -1 if illegal mappings, or returns 0 if all OBMEM, else
 * it returns the starting page frame number for a legal "bus request".
 *
 * The starting page frame number is defined to be mmu_btop(0xff------)
 * for a vme24 address and mmu_btop(0xffff----) for a vme16 address.  This
 * arrangement allows masters to detect whether a 24 bit or a 32 bit
 * address should be used.  However, most masters always use the same
 * modifier.  A VMEbus slave device that wants to be able to handle both
 * 24 bit and 32 bit addresses then needs to be set up to respond to a
 * 32 bit address of 0xff------ or a 24 bit address of 0x------
 * (or in reality, an address of 0x??------).
 *
 * We insist that non-OBMEM mappings must be contiguous and of the
 * same type (as determined from the starting page).
 */

#define	a16bits (mmu_btop(VME16_BASE) & pfnumbits)
#define	a24bits (mmu_btop(VME24_BASE) & pfnumbits)

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
			if (!pte_valid(pte) || pte->pg_type != pt) {
				return (-1);
			} else if (pt != OBMEM && pte->pg_pfnum != pf) {
				return (-1);
			}
		} else {
			/*
			 * (This sounds weird but) no IO to obio space.
			 * Also insure that we really have a mapping.
			 */
			pf = pte->pg_pfnum;
			pt = pte->pg_type;
			if (pt == OBIO || !pte_valid(pte)) {
				return (-1);
			}
			if (pt == OBMEM) {
				/*
				 * all Onboard memory memory so far
				 */
				res = 0;
			} else {
				/*
				 * Else save starting page frame number,
				 * masking off the bits that might not
				 * really be implemented.
				 */
				res = pf & pfnumbits;
				if (pt == VME_D16 || pt == VME_D32) {
					/*
					 * If the bus address is in the
					 * DVMA area, bounce the request.
					 * Note also that the test for
					 * a16 is *before* the test for
					 * a24 as VME16 space is stolen
					 * from the top 64k of VME24
					 * space (as VME24 space steals
					 * the top 16mb of VME32 space).
					 */
					if (res < dvmasize) {
						return (-1);
					} else if ((res & a16bits) == a16bits) {
						res |= mmu_btop(VME16_BASE);
					} else if ((res & a24bits) == a24bits) {
						res |= mmu_btop(VME24_BASE);
					}
				}
			}
		}
		pf++;
		npf--;
	}
	return (res);
}

#ifdef	VAC
/*
 * Allocate 'size' units from the given map so that
 * the vac alignment constraints for bp are maintained.
 *
 * Return 'addr' if successful, 0 if not.
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

	if (vac) {
		extern struct pment *pments;	/* from <machine/vm_hat.c> */

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
		(void)splx(s);
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
	(void)splx(s);

	if (mp->m_size == 0)
		return (0);

	/* let rmget() do the rest of the work */
	s = splhigh();
	addr = rmget(map, (long)size, (u_long)addr);
	(void)splx(s);
	return (addr);
}
#endif	VAC

/*
 * Called to convert bp for pageio/physio to a kernel addressable location.
 * We allocate virtual space from the kernelmap and then use bp_map to do
 * most of the real work.
 */
bp_mapin(bp)
	register struct buf *bp;
{
	int npf, o;
	u_long a;
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
	register addr_t addr;
	register int s;

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
	struct pte pte;
	int physaddr;

	mmu_getkpte(addr, &pte);
	physaddr = mmu_ptob(pte.pg_pfnum);
	switch (pte.pg_type) {
	case VME_D16:
	case VME_D32:
		if (physaddr > VME16_BASE) {
			/* 16 bit VMEbus address */
			physaddr -= VME16_BASE;
		} else if (physaddr > VME24_BASE) {
			/* 24 bit VMEbus address */
			physaddr -= VME24_BASE;
		}
		/*
		 * else 32 bit VMEbus address,
		 * physaddr doesn't require adjustments
		 */
		break;

	case OBMEM:
	case OBIO:
		/* physaddr doesn't require adjustments */
		break;
	}

	return (physaddr + off);
}

char mon_clock_on = 0;		/* disables profiling */
trapvec kclock14_vec;		/* kernel clock14 vector code */
trapvec mon_clock14_vec;	/* monitor clock14 vector code */
u_int mon_memerr;		/* monitor memory register setting */

start_mon_clock()
{

#ifndef	SAS
	if (!mon_clock_on) {
		mon_clock_on = 1;		/* disable profiling */
		write_scb_int(14, &mon_clock14_vec);	/* install mon vector */
#ifndef	GPROF
		set_clk_mode(IR_ENA_CLK14, 0);	/* enable level 14 clk intr */
#endif	!GPROF
#if	defined(SUN4_260) || defined(SUN4_470) || defined(SUN4_330)
		if ((cpu == CPU_SUN4_260 ||
		    cpu == CPU_SUN4_470 ||
		    cpu == CPU_SUN4_330) && mon_memerr)
			MEMERR->me_err = mon_memerr;
#endif	/* SUN4_260 || SUN4_470 || SUN4_330 */
	}
#endif	!SAS
}

stop_mon_clock()
{
	extern struct scb scb;

#ifndef	SAS
	if (mon_clock_on) {
		mon_clock_on = 0		/* enable profiling */;
#ifndef	GPROF
		set_clk_mode(0, IR_ENA_CLK14);	/* disable level 14 clk intr */
#endif	!GPROF
		write_scb_int(14,  &kclock14_vec); /* install kernel vector */
#if	defined(SUN4_260) || defined(SUN4_470)
		if (cpu == CPU_SUN4_260 || cpu == CPU_SUN4_470) {
			mon_memerr = MEMERR->me_err;
			MEMERR->me_err = EER_INTENA | EER_CE_ENA;
		}
#endif	/* SUN4_260 || SUN4_470 */
	}
#endif	!SAS
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
	extern map_setpgmap();

	sp = &scb.interrupts[level - 1];
	s = spl8();

	/* save old mapping */
	savepte = map_getpgmap((caddr_t)sp);

	/* allow writes */
	map_setpgmap((caddr_t)sp, (u_int)(PG_V | PG_KW | savepte & PG_PFNUM));

	/* write out new vector code */
	*sp = *ip;

	/* flush out the write since we are changing mappings */
	vac_flush((caddr_t)sp, sizeof (struct trapvec));

	/* restore old mapping */
	(void) map_setpgmap((caddr_t)sp, savepte);

	(void) splx(s);
}

#ifndef	SAS
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
#endif	!SAS

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
	register int c, i;
	register addr_t v, tv;
	struct pte pte;

	/*
	 * Put kernel segment in kernel address space.  Make it a
	 * "kernel memory" segment object.
	 */
	(void) seg_attach(&kas, (addr_t)KERNELBASE, (u_int)(vvm -
	    KERNELBASE), &ktextseg);
	(void) segkmem_create(&ktextseg, (caddr_t)NULL);

	(void) seg_attach(&kas, (addr_t)vvm, (u_int)econtig -
	    (u_int)vvm, &kvalloc);
	(void) segkmem_create(&kvalloc, (caddr_t)NULL);

	(void) seg_attach(&kas, (addr_t)HEAPBASE, (u_int)(BUFLIMIT - HEAPBASE),
	    &bufheapseg);
	(void) segkmem_create(&bufheapseg, (caddr_t)Heapptes);

	(void) seg_attach(&kas, (addr_t)SYSBASE, (u_int)(Syslimit - SYSBASE),
	    &kseg);
	(void) segkmem_create(&kseg, (caddr_t)Sysmap);

	/*
	 * Make sure the invalid pmeg and smeg have no valid entries
	 */
	map_setsgmap(SEGTEMP, PMGRP_INVALID);
	for (i = 0; i < PMGRPSIZE; i += PAGESIZE) {
		mmu_getpte(SEGTEMP + i, &pte);
		if (pte.pg_v) {
			pte.pg_v = 0;
			mmu_setpte(SEGTEMP + i, pte);
		}
	}
#ifdef	MMU_3LEVEL
	if (mmu_3level) {
		for (i = 0; i < SMGRPSIZE; i += PMGRPSIZE) {
			struct pmgrp *pmgrp;
			pmgrp = mmu_getpmg((addr_t)(REGTEMP + i));
			if (pmgrp->pmg_num != PMGRP_INVALID)
				mmu_pmginval((addr_t)(REGTEMP + i));
		}
	}
#endif	MMU_3LEVEL

	/*
	 * Invalidate segments before kernel.
	 */
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

	/*
	 * Duplicate kernel into every context.  From this point on,
	 * adjustments to the mmu will automatically copy kernel changes.
	 * Use the ROM to do this copy to avoid switching to unmapped
	 * context.
	 */
#ifdef	SAS
	asm("t 209");		/* make all segments the same as this one */
#else

#ifdef	MMU_3LEVEL
	if (mmu_3level) {
		for (c = 1; c < NCTXS; c++) {
			for (i= 0, v = (addr_t)0; i < NSMGRPPERCTX;
			    i++, v += SMGRPSIZE)
				(*romp->v_setcxsegmap)(c, v,
				    (mmu_getsmg(v))->smg_num);
		}
	} else
#endif	MMU_3LEVEL
	{
		for (c = 1; c < NCTXS; c++) {
			for (v = (addr_t)0; v < hole_start; v += PMGRPSIZE)
				(*romp->v_setcxsegmap)(c, v,
				    (mmu_getpmg(v))->pmg_num);
			for (v = hole_end; v != NULL;  v += PMGRPSIZE)
				(*romp->v_setcxsegmap)(c, v,
				    (mmu_getpmg(v))->pmg_num);
		}
	}
#endif	SAS

	/*
	 * Initialize the kernel page maps.
	 */
	v = (addr_t)KERNELBASE;

	/* user copy red zone */
	(void) as_fault(&kas, v, PAGESIZE, F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, PAGESIZE, 0);
	v += PAGESIZE;

	/* msgbuf */
	(void) as_fault(&kas, v, PAGESIZE, F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, PAGESIZE, PROT_READ | PROT_WRITE);
	v += PAGESIZE;

	/*
	 * (Normally) Read-only until end of text.
	 */
	(void) as_fault(&kas, v, (u_int)(etext - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(etext - v), (u_int)
	    (PROT_READ | PROT_EXEC | ((kernprot == 0)? PROT_WRITE : 0)));
	v = (addr_t)roundup((u_int)etext, PAGESIZE);

	/*
	 * Writable until end.
	 */
	(void) as_fault(&kas, v, (u_int)(vvm - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(vvm - v),
	    PROT_READ | PROT_WRITE | PROT_EXEC);
	v = (addr_t)vvm;

	/*
	 * Validate the valloc'ed structures.
	 */
	(void) as_fault(&kas, v, (u_int)(econtig - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(econtig - v),
	    PROT_READ | PROT_WRITE | PROT_EXEC);

	/*
	 * segu will reclaim some of this space when it is created.
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
	 * More adjustment for SunMon 4.0 large frame buffer mappings...
	 */
	if ((romp->v_mon_id[0]=='4')) {
	    v = (addr_t)roundup((u_int)VXMVX_BASE, PMGRPSIZE);

	    for (; v < (addr_t)VXMVX_LIMIT; v += PMGRPSIZE) {
		if (mmu_getpmg(v) != pmgrp_invalid) {
		    for (tv = v; tv < (v + PMGRPSIZE); tv += PAGESIZE) {
			mmu_getpte(tv, &pte);
			if (pte_valid(&pte))
			    break;
		    }
		    if (tv < (v + PMGRPSIZE)) {
			    hat_pmgreserve(&kseg, v);
			    hat_chgprot(&kseg, v, PMGRPSIZE, ~PROT_USER);

		    } else {
			mmu_pmginval(v);
		    }
		}
		else {
		    mmu_pmginval(v);
		}
	    }
	}
	else {
	    v = (addr_t)roundup((u_int)VXMVX_BASE, PMGRPSIZE);
	    for (; v < (addr_t)VXMVX_LIMIT; v += PMGRPSIZE)
		mmu_pmginval(v);
	}
	/*
	 * end Adjustment for SunMon 4.0
	 */

	/*
	 * Reserve device mapping segment.
	 */
	hat_pmgreserve(&kseg, (addr_t)MDEVBASE);

	/*
	 * Invalid to Syslimit.
	 */
	v = (addr_t)SYSBASE;
	(void) as_setprot(&kas, v, (u_int)(Syslimit - v), 0);
	v = (addr_t)roundup((u_int)v, PMGRPSIZE);

	/*
	 * Invalid segment mappings until start of debugger.
	 */
	for (; v < (addr_t)DEBUGSTART; v += PMGRPSIZE)
		mmu_pmginval(v);

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

	/*
	 * Invalid between debugger and monitor.
	 */
	for (; v < (addr_t)MONSTART; v += PMGRPSIZE)
		mmu_pmginval(v);

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

	/*
	 * Invalidate rest of segments up to final pmeg.
	 */
	for (; v < (addr_t)-PMGRPSIZE; v += PMGRPSIZE)
		mmu_pmginval(v);

	/*
	 * Loop through the last segment and set page protections
	 * without messing with things that lie past dvma.
	 * We get the interrupt redzone for free when the kernel is
	 * write protected as the interrupt stack is the first thing
	 * in the data area.
	 */
	hat_pmgreserve(&kseg, v);
	tv = (addr_t) &DVMA[mmu_ptob(dvmasize)];
	for (; v < tv; v += PAGESIZE)
			mmu_setpte(v, mmu_pteinvalid);

	/*
	 * Invalidate all the unlocked pmegs.
	 * and smegs, if they exist
	 */
	hat_pmginit();

	/*
	 * Now create a segment for the DVMA virtual
	 * addresses using the segkmem segment driver.
	 */
	(void) seg_attach(&kas, DVMA, (u_int)ctob(dvmasize), &kdvmaseg);
	(void) segkmem_create(&kdvmaseg, (caddr_t)NULL);

	/*
	 * Allocate pmegs for DVMA space.
	 */
	hat_reserve(&kdvmaseg, (addr_t)DVMA, (u_int)ctob(dvmasize));

	/*
	 * Wire down pmegs for temporaries
	 */
	hat_reserve(&kseg, CADDR1, MMU_PAGESIZE);
	hat_reserve(&kseg, CADDR2, MMU_PAGESIZE);
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
			a += 1;
		}
	} else {
		/*
		 * Cannot get an entry from the cache,
		 * allocate a slot for ourselves.
		 */
		s = splhigh();
		while ((a = rmalloc(kernelmap, (long)CLSIZE)) == 0) {
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
			a += 1;
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
		mmu_setpte(CADDR2, pte.u_pte);
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
		mmu_setpte(CADDR2, mmu_pteinvalid);
	}
}

#ifdef	VAC
void
vac_init()
{
	vac_tagsinit();
}
#endif	VAC

#ifndef	SAS
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

int
cngetc()
{
	register int c;

	while ((c = (*romp->v_mayget)()) == -1)
		;
	return (c);
}
#endif	!SAS

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
	newseg = (struct seguser *)segu_get();
	if (newseg == (struct seguser *)0)
		panic("can't create kernel process");

	fp = (struct frame *)KSTACK(newseg);
	fp->fr_local[6] = (u_int)startaddr;	/* l6 gets start address */
	fp->fr_local[0] = arg;			/* l0 gets the argument */
	fp->fr_local[5] = 1;			/* means fork a kernel proc */
	pp = freeproc;
	freeproc = pp->p_nxt;
	newproc(0, (struct as *)0, (int)0, (struct file *)0, newseg, pp);
}

#ifdef	IOC
/*
 * ioc_init()
 * Initialize the iocache tags to be invalid and zero data.
 * Extra ram in the iocache is used as fast memory to hold
 * the ethernet descriptors so that cache misses (which tend
 * to cause over/underruns) never occur when the ethernet is
 * reading or writing control informaion.  Map second half of
 * i/o cache data ram into a fixed address (0xFFFF8000).  The
 * tags for the descriptors should be marked valid, modified,
 * and the tag address must match the virtual address where it
 * is mapped (so the ethernet descriptor cache always hits)
 */
ioc_init()
{
	register u_long  *p, tag;

	/*
	 * map in tags and initialize them
	 * we use the flush virtual address temporarily
	 * since we normally don't touch the tags or data
	 * once the i/o cache is initialized
	 */
	map_setpgmap((addr_t)IOC_FLUSH_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCDATA_ADDR));

	/* zero both data and tag */
	for (p = (u_long *)IOC_FLUSH_ADDR;
		(u_long)p != IOC_FLUSH_ADDR+0x2000;
		p++)
		*p = 0;

	/*
	 * Initialize the ram and tags we are going to use
	 * to hold the ethernet decriptors.
	 */
	map_setpgmap((addr_t)IOC_IEDESCR_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_DESCR_DATA_ADDR));

	/* zero descriptor data */
	for (p = (u_long *)IOC_IEDESCR_ADDR;
		(u_long)p < IOC_IEDESCR_ADDR+0x1000;
		p++)
		*p = 0;

	/*
	 * Initialize descriptor tags,
	 * Bit 0 is the modified bit, bit 1 is the valid bit.
	 * Bits 2-4 aren't used and the rest are virtual address bits.
	 */
	for (p = (u_long *)(IOC_IEDESCR_ADDR+0x1000), tag = IOC_IEDESCR_ADDR+3;
		(u_long)p < IOC_IEDESCR_ADDR+0x2000;) {
		*p = tag;
		p++;
		if ((p != (u_long *)(IOC_IEDESCR_ADDR+0x1000)) &&
		    (((u_long)p%32) == 0))
			tag += IOC_LINESIZE;
	}

	/* map in flush page for i/o cache */
	map_setpgmap((addr_t)IOC_FLUSH_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCFLUSH_ADDR));

	/* XXX - for debugging only,  remove later */
	map_setpgmap((addr_t)IOC_DATA_ADDR,
		PG_V | PG_KW | PGT_OBIO | btop(OBIO_IOCDATA_ADDR));
}

ioc_mbufset(pte, addr)
struct pte *pte;
addr_t addr;
{
	if (ioc_net) {
		/*
		 * All pages that are mbufs are I/O cacheable. The
		 * only I/O that can occur to these pages is ethernet.
		 */
		if (addr >= (addr_t)mbutl &&
		    addr < ((addr_t)mbutl + MBPOOLBYTES))
			ioc_pteset(pte);
	}
}

check_ioctag(addr)
	caddr_t addr;
{
	unsigned int ioc_tag;

	ioc_tag = get_ioctag((u_int)addr);

	if (ioc_tag & MODIFIED_IOC_LINE) {
		printf("IOCACHE TAG IS MODIFIED!!!??  address  0x%x  \
ioc tag  0x%x\n",
		    addr, ioc_tag);
		panic("check_ioctag");
	}
}

get_ioctag(addr)
unsigned int addr;
{
	return (*(unsigned long *)(IOC_TAGS_ADDR + get_pagenum(addr)*32));
}

get_pagenum(addr)
unsigned int addr;
{

	return ((addr&0xFE000) >> 13);
}
#endif	IOC

/*
 * Flush a ctx that has been stolen form another process.
 * When in this situation, and if the hardware support
 * for flushing non-supervisor lines exists, we flush all
 * user context from the cache and mark the contexts as clean.
 * Further context stealing will not require any flushes for
 * up to NCXTS -1 allocations. When we run a new or existing
 * process we clear the context's clean bit.
 *
 * Note: the new vm_hat.c uses the c_clean bit on all sun4/sun4c cpu's.
 */
void
vac_flushallctx()
{
	register int i;
	u_int map_getctx();

	if (ctxs[map_getctx()].c_clean == 1) {
		return;
	}
	vac_usrflush();
	for (i = 1; i < NCTXS; i++)	/* skip kernel context */
		ctxs[i].c_clean = 1;
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
 * Make sure that the kernel addresses from KERNELBASE through the
 * end of the valloc'ed structures are mapped to the physical memory starting
 * at page frame 0. The monitor guarantees that only the first 2Mbytes are
 * mapped. Since kernels with the hat layer using many SW pmegs may be
 * larger than 2 Mbytes, we must set up the mapping before we call hat_init().
 *
 * XXX - this function will fail if the number of HW pmegs > 1024 or
 *	 the number of smegs > 256.
 * XXX - replace MAPPED_BY_MONITOR with a #define from the monitor interface
 *	 file.
 */

#define	MAXPMEGS	1024
#define	MAXSMEGS	 256
#define	MAPPED_BY_MONITOR (2 * 1024 * 1024) /* 2 MBytes */

mapvalloced(eaddr)
	addr_t		eaddr;
{
	char		resrvpmegs[MAXPMEGS];
	addr_t		v;
	int		freepmeg = 0;
	u_int		pf;
#ifdef MMU_3LEVEL
	char		resrvsmegs[MAXSMEGS];
	int		freesmeg = 0;
#endif MMU_3LEVEL

	bzero(resrvpmegs, sizeof (resrvpmegs));
#ifdef MMU_3LEVEL
	bzero(resrvsmegs, sizeof (resrvsmegs));
#endif MMU_3LEVEL

	/*
	 * Devices at MDEVBASE
	 */
	resrvpmegs[map_getsgmap((addr_t)MDEVBASE)] = 1;
#ifdef MMU_3LEVEL
	if (mmu_3level)
		resrvsmegs[map_getrgnmap((addr_t)MDEVBASE)] = 1;
#endif MMU_3LEVEL

	/*
	 * addr 0xffffc000 is reserved in locore.s
	 */
	resrvpmegs[map_getsgmap((addr_t)-PMGRPSIZE)] = 1;
#ifdef MMU_3LEVEL
	if (mmu_3level)
		resrvsmegs[map_getrgnmap((addr_t)-PMGRPSIZE)] = 1;
#endif MMU_3LEVEL

	/*
	 * Debugger
	 */
	for (v = (addr_t)DEBUGSTART; v < (addr_t)DEBUGEND; v += PMGRPSIZE) {
		resrvpmegs[map_getsgmap(v)] = 1;
#ifdef MMU_3LEVEL
		if (mmu_3level)
			resrvsmegs[map_getrgnmap(v)] = 1;
#endif MMU_3LEVEL
	}

	/*
	 * Monitor
	 */
	for (v = (addr_t)MONSTART; v < (addr_t)MONEND; v += PMGRPSIZE) {
		resrvpmegs[map_getsgmap(v)] = 1;
#ifdef MMU_3LEVEL
		if (mmu_3level)
			resrvsmegs[map_getrgnmap(v)] = 1;
#endif MMU_3LEVEL
	}

	/*
	 * Loaded kernel. We assume that the monitor maps correctly
	 * at least MAPPED_BY_MONITOR bytes of physical memory.
	 */
	for (v = (addr_t)KERNELBASE;
		v < (addr_t)KERNELBASE + MAPPED_BY_MONITOR; v += PMGRPSIZE) {
		resrvpmegs[map_getsgmap(v)] = 1;
#ifdef MMU_3LEVEL
		if (mmu_3level)
			resrvsmegs[map_getrgnmap(v)] = 1;
#endif MMU_3LEVEL
	}

#ifdef TRACE_MAPVALLOCED
	printf("mapvalloced(econtig = 0x%x)\n", eaddr);
	{
		int	i;

		printf("Reserved pmegs: ");
		for (i = 0; i < MAXPMEGS; i++)
			if (resrvpmegs[i])
				printf(" %d", i);
		printf("\n");

		printf("Reserved smegs: ");
		for (i = 0; i < MAXSMEGS; i++)
			if (resrvsmegs[i])
				printf(" %d", i);
		printf("\n");
	}
#endif TRACE_MAPVALLOCED

	/*
	 * Map <KERNELBASE + MAPPED_BY_MONITOR .. eaddr) to low memory frames.
	 */
	for (v = (addr_t)KERNELBASE + MAPPED_BY_MONITOR,
	    pf = MAPPED_BY_MONITOR / PAGESIZE;
	    v < eaddr;
	    v += PAGESIZE, pf++) {
#ifdef MMU_3LEVEL
		if (mmu_3level && ((u_int)v & SMGRPOFFSET) == 0) {
			/*
			 * Reserve region.
			 */
			while (resrvsmegs[freesmeg])
				freesmeg++;

			map_setrgnmap(v, (u_int)freesmeg);
			resrvsmegs[freesmeg++] = 1; /* mark as reserved */
		}
#endif MMU_3LEVEL
		if (((u_int)v & PMGRPOFFSET) == 0) {
			/*
			 * Reserve segment.
			 */
			while (resrvpmegs[freepmeg]) {
#ifdef TRACE_MAPVALLOCED
				printf("skipping reserved pmeg # %d\n",
					freepmeg);
#endif TRACE_MAPVALLOCED
				freepmeg++;
			}
			map_setsgmap(v, (u_int)freepmeg);
			resrvpmegs[freepmeg++] = 1; /* mark as reserved */
		}

		map_setpgmap(v, PG_V | PG_KW | PGT_OBMEM | pf);
	}
}
