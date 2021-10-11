#ifndef	lint
static	char sccsid[] = "@(#)machdep.c  1.1 92/07/30 SMI";
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
#include <machine/eeprom.h>
#include <machine/interreg.h>
#include <machine/memerr.h>
#include <machine/eccreg.h>
#include <machine/seg_kmem.h>
#ifdef	SUN3_260
#include <machine/enable.h>
#endif

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
#define	BUFPERCENT	(100 / 7)	/* % mem for bufs - was 10%, now 3% */

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
addr_t vstart;		/* Start of valloc'ed region */
addr_t vvm;		/* Start of valloc'ed region we can mmap */

/*
 * VM data structures
 */
int page_hashsz;		/* Size of page hash table (power of two) */
struct	page **page_hash;	/* Page hash table */
struct	seg *segkmap;		/* Kernel generic mapping segment */
struct	seg ktextseg;		/* Segment used for kernel executable image */
struct	seg kvalloc;		/* Segment used for "valloc" mapping */
struct	memseg memseg;		/* phys mem segment structure */
struct	memseg memseg0;		/* phys mem segment structure for page zero */

int (*exit_vector)() = (int (*)())0;	/* Where to go when halting UNIX */

/*
 * For implementations that have fewer than the default number of page
 * frame bits (19), startup() patch back this variable as appropriate.
 */

u_long pfnumbits = PG_PFNUM & ~PG_TYPE;

#define	TESTVAL	0xA55A		/* memory test value */

#ifdef	SUN3_260
/*
 * Since there is no implied ordering of the memory cards, we store
 * a zero terminated list of pointers to eccreg's that are active so
 * that we only look at existent memory cards during softecc() handling.
 */
struct	eccreg *ecc_alive[MAX_ECC+1];
#endif	SUN3_260

/*
 * Initialize the flag indicating the presence of a virtual address
 * cache to prevent its use during early initialization phases.
 * Same with bcopy hardware.
 */
int vac = 0;
int bcenable = 0;

/*
 * Various kernel boundaries
 */
extern char start[], etext[], end[];

/*
 * Machine-dependent startup code
 */
startup()
{
	register int unixsize, dvmapage;
	register unsigned i;
	register caddr_t v;
	u_int firstaddr;		/* next free physical page number */
	u_int mapaddr, npages, page_base;
	struct page *pp;
	struct segmap_crargs a;
	void v_handler();
	int mon_mem;

	bootflags();			/* get the boot options */

	initscb();			/* set trap vectors */
	*INTERREG |= IR_ENA_INT;	/* make sure interrupts can occur */
	firstaddr = btoc((int)end - KERNELBASE);

	setcputype();			/* sets cpu and dvmasize variables */
#ifdef	VAC
	if (cpu == CPU_SUN3_260) {
		extern int nopagereclaim;

		vac_init();		/* initialize cache */
		/*
		 * For VAC machines, we disable page reclaim to
		 * prevent having pages currently loaded for DVMA
		 * from going to cached to non-cached in the middle
		 * of a IO operation.  With nopagereclaim set to
		 * true, the system will wait for any pages currently
		 * marked as "intransit" to finish before trying
		 * to load up a translation to the page.
		 */
		nopagereclaim = 1;
	}
#endif

	/*
	 * Make sure the memory error register is
	 * set up to generate interrupts on error.
	 */
	if (cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50 ||
	    cpu == CPU_SUN3_110 || cpu == CPU_SUN3_60 || cpu == CPU_SUN3_E)
		MEMREG->mr_per = PER_INTENA | PER_CHECK;

#ifdef	SUN3_260
	if (cpu == CPU_SUN3_260) {
		register struct eccreg **ecc_nxt = ecc_alive;
		register struct eccreg *ecc;

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
			ecc->syndrome |= SY_CE_MASK; /* clear syndrom fields */
			ecc->eccena |= ENA_SCRUB_MASK;
			ecc->eccena |= ENA_BUSENA_MASK;
			*ecc_nxt++ = ecc;
		}
		*ecc_nxt = (struct eccreg *)0;		/* terminate list */
	}
#endif	SUN3_260

	switch (cpu) {
		case CPU_SUN3_E:
			/*
			 * 16 bit page number
			 */
			pfnumbits &= 0xffff;
			break;

		case CPU_SUN3_50:
		case CPU_SUN3_60:
			/*
			 * 11 bit page number
			 */
			pfnumbits &= 0x7ff;
			break;

		case CPU_SUN3_160:
		case CPU_SUN3_260:
		case CPU_SUN3_110:
		default:
			/*
			 * All page frame number bits implemented,
			 * pfnumbits already set correctly.
			 */
			break;
	}

	/*
	 * v_memorysize is the amount of physical memory while
	 * v_memoryavail is the amount of usable memory in versions
	 * equal or greater to 1.  Mon_mem is the difference which
	 * is the number of pages hidden by the monitor.
	 */
	if (romp->v_romvec_version >= 1)
		mon_mem = btop(*romp->v_memorysize - *romp->v_memoryavail);
	else
		mon_mem = 0;

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
	maxmem = physmem - IOPBMEM;

	/*
	 * v_vector_cmd is the handler for new monitor vector
	 * command in versions equal or greater to 2.
	 * We install v_handler() there for Unix.
	 */
	if (romp->v_romvec_version >= 2)
		*romp->v_vector_cmd = v_handler;

	/*
	 * Decide whether or not to leave hole for video copy memory.
	 */
#include "bwtwo.h"
#if	NBWTWO > 0
	if (physmem > btop(OBFBADDR + FBSIZE))
		fbobmemavail = 1;
	else
		fbobmemavail = 0;
#else
	fbobmemavail = 1;
#endif

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * "mapaddr" is the real memory address where the tables start.
	 * It is used when remapping the tables later.
	 * In order to support the frame buffer which might appear in
	 * the middle of contiguous memory we adjust the map address to
	 * start after the end of the frame buffer.  Later we will adjust
	 * the core map to take this hole into account.  The reason for
	 * this is to keep all the kernel tables contiguous in virtual space.
	 * In addition we need to allow for the hole left by the frame
	 * buffer on big kernels.
	 */

	if (((u_int)end - (u_int)KERNELBASE) > OBFBADDR)
		mapaddr = btoc((end - KERNELBASE) + FBSIZE);
	else
		mapaddr = btoc(OBFBADDR + FBSIZE);
	vvm = vstart = v = (caddr_t)(ctob(firstaddr) + KERNELBASE);

	if (nrnode == 0) {
		nrnode = ncsize << 1;
	}

#define	valloc(name, type, num) \
	    (name) = (type *)(v); (v) = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))
#ifdef	UFS
#ifdef	QUOTA
	valloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
#endif	QUOTA
#endif	UFS
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
	if (firstaddr <= btoc(OBFBADDR))
		page_base = firstaddr;
	else
		page_base = btoc(OBFBADDR);
	npages = physmem - page_base;
	npages++;			/* add one, we reclaim page zero */
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
	hat_init();
	kvm_init();

	/*
	 * Determine if anything lives in DVMA bus space.
	 * We're paranoid and go through both the 16 bit
	 * and 32 bit device types.
	 */
	disable_dvma();
	for (dvmapage = 0; dvmapage < dvmasize; dvmapage++) {
		segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
		    PROT_READ | PROT_WRITE, (u_int)(dvmapage | PGT_VME_D16), 0);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
		segkmem_mapin(&kseg, CADDR1, MMU_PAGESIZE,
		    PROT_READ | PROT_WRITE, (u_int)(dvmapage | PGT_VME_D32), 0);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
	}
	enable_dvma();

#ifdef	VAC
	if (cpu == CPU_SUN3_260) {
		on_enablereg((u_char)ENA_CACHE);	/* turn on cache */
		vac = 1;
#ifdef	BCOPY
		bcenable = 1;
#endif	BCOPY
	}
#endif	VAC

	/*
	 * Good {morning, afternoon, evening, night}.
	 * When printing memory, use the total including
	 * those hidden by the monitor (mon_mem).
	 */
	printf(version);
	printf("mem = %dK (0x%x)\n", ctob(physmem + mon_mem) / 1024,
	    ctob(physmem + mon_mem));

	if (dvmapage < dvmasize) {
		printf("CAN'T HAVE PERIPHERALS IN RANGE 0 - %dKB\n",
		    ctob(dvmasize) / 1024);
		panic("dvma collision");
	}

	if (Syslimit + NCARGS + MINMAPSIZE > SEGTEMP)
		panic("system map tables too large");

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

		availpgs = MIN((SYSPTSIZE - btoc(v - KERNELBASE)), maxmem);
		nbuf = MAX(((availpgs * NBPG) / BUFPERCENT / MAXBSIZE), MINBUF);
	}

	unixsize = btoc(v - KERNELBASE);

	if ((int)unixsize > SYSPTSIZE)
		panic("sys pt too small");

	/*
	 * Clear allocated space, and make r/w entries
	 * for the space in the kernel map.
	 */
	if (unixsize >= physmem - 8*UPAGES)
		panic("no memory");

	segkmem_mapin(&kvalloc, (addr_t)vvm, (u_int)(econtig - vvm),
		PROT_READ | PROT_WRITE, mapaddr, 0);
	bzero((caddr_t)vvm, (u_int)(econtig - vvm));

	/*
	 * Initialize more of the page structures.
	 */
	page_init(pp, 1, 0, &memseg0);
	page_init(&pp[1], npages-1, page_base, &memseg);

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
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize memory free list.
	 */
	if (fbobmemavail && firstaddr < btoc(OBFBADDR))
		memialloc(firstaddr, btop(OBFBADDR));
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
	    "kernel map", 4 * nproc);
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
	if (fbobmemavail) {
		/*
		 * Onboard frame buffer memory still
		 * available, put back onto the free list.
		 */
		memialloc((int)btop(OBFBADDR), (int)btop(OBFBADDR + FBSIZE));
		fbobmemavail = 0;
	}
	maxmem = freemem;

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
		} else {
			(void) strcpy(pathbuf, initname);
		}

		/*
		 * Move out the file name (also arg 0).
		 */
		i = 0;
		while (pathbuf[i]) {
			i += 1;
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
#ifdef	FPU
	fpu_setregs();
#endif	FPU
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
		/* NOTREACHED */
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
	return (addr < *romp->v_memorysize);
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

	addr = &DVMA[ctob(dvmasize-1)];
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
 * We assume that DVMASIZE is at least two pages.
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
	addr = &DVMA[ctob(dvmasize-2)];
	pte[0] = mmu_pteinvalid;
	pte[1] = mmu_pteinvalid;
	pte[1].pg_v = 1;
	pte[1].pg_prot = KW;
	mmu_getpte(kaddr, &tpte);
	pte[1].pg_pfnum = tpte.pg_pfnum;

	while (count > 0 && !err) {
		/*
		 * lint doesn't like this ... lint bug? (yes)
		 * In order to keep pushing on lint getting
		 * fixed, keep this visible.
		 */
		pte[0] = pte[1];
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
	asm("trap #14");
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
tracedump()
{
	label_t	l;

	(void)syscall_setjmp(&l);
	traceback((long)l.val[11], (long)l.val[12]);
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
		 * Since sun3s have ptes for KERNELBASE on up,
		 * check for having to go read the hardware
		 * for the address in question if it between
		 * KERNELBASE and SYSBASE or between Syslimit
		 * and DEBUGSTART is not necessary.
		 */
		spte = &Sysmap[btop((u_int)bp->b_un.b_addr - SYSBASE)];
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
			mmu_setpte(kaddr, *pte);
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
		spte = &Sysmap[btop((u_int)bp->b_un.b_addr - SYSBASE)];
	}

	if (spte == (struct pte *) NULL) {
		bpcopy = *bp;	/* structure copy */
		cidx = PTECHUNKSIZE;
	}

	/*
	 * Set the result, page frame, and page type to impossible values
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
					if (res < btoc(DVMASIZE)) {
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
 * If VAC is not defined, bp_alloc is just a #define
 * for (int)rmalloc((map), (long)(size)).
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
		extern struct pment pments[];	/* from <sun/vm_hat.c> */

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
	int a;
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
	addr_t addr;
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

static int (*mon_nmi)();		/* monitor's level 7 nmi routine */
static u_char mon_mem;			/* monitor memory register setting */
extern int level7();			/* Unix's level 7 nmi routine */

stop_mon_clock()
{
	struct scb *vbr, *getvbr();

	vbr = getvbr();
	if (vbr->scb_autovec[7 - 1] != level7) {
#ifndef	GPROF
		set_clk_mode(0, IR_ENA_CLK7);	/* disable level 7 clk intr */
#endif	!GPROF
		mon_nmi = vbr->scb_autovec[7 - 1];	/* save mon vec */
		vbr->scb_autovec[7 - 1] = level7;	/* install Unix vec */
		/*
		 * It got too cumbersome to have #ifdef SUN3_260 followed
		 * by #if defined(EVERY OTHER SUN3) machine. If there is
		 * ever another sun3, change it back.
		 */
#ifdef	SUN3_260
		if (cpu == CPU_SUN3_260) {
			mon_mem = MEMREG->mr_eer;
			MEMREG->mr_eer = EER_INTENA | EER_CE_ENA;
		}
#else	/* SUN3_260 */
		if (cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50 ||
		    cpu == CPU_SUN3_110 || cpu == CPU_SUN3_60 ||
		    cpu == CPU_SUN3_E)
			mon_mem = MEMREG->mr_per;
#endif	/* SUN3_260 */
	}
}

start_mon_clock()
{
	struct scb *getvbr();

	if (mon_nmi) {
		getvbr()->scb_autovec[7 - 1] = mon_nmi;	/* install mon vec */
#ifndef	GPROF
		set_clk_mode(IR_ENA_CLK7, 0);	/* enable level 7 clk intr */
#endif	!GPROF
		/*
		 * It got too cumbersome to have #ifdef SUN3_260 followed
		 * by #if defined(EVERY OTHER SUN3) machine. If there is
		 * ever another sun3, change it back.
		 */
#ifdef	SUN3_260
		if (cpu == CPU_SUN3_260)
			MEMREG->mr_eer = mon_mem;
#else	/* SUN3_260 */
		if (cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50 ||
		    cpu == CPU_SUN3_110 || cpu == CPU_SUN3_60 ||
		    cpu == CPU_SUN3_E)
			MEMREG->mr_per = mon_mem;
#endif	/* SUN3_260 */
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
	case '\0':
		/*
		 * No (non-hex) letter was specified on
		 * command line, use only the number given
		 */
		switch (addr) {
		case 0:		/* old g0 */
		case 0xd:	/* 'd'ump short hand */
			panic("zero");
			/* NOTREACHED */

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
 * Handle parity/ECC memory errors.  XXX - use something like
 * vax to only look for soft ecc errors periodically?
 */
memerr()
{
	u_int per, eer;
	char *mess = 0;
	struct ctx *ctx;
	extern struct ctx ctxs[];
	struct pte pme;

	eer = per = MEMREG->mr_er;
#ifdef	SUN3_260
	if (cpu == CPU_SUN3_260 && (((eer & EER_ERR) == EER_CE) &&
			((eer & EER_ERR) != EER_UE))) {
		MEMREG->mr_eer = ~EER_CE_ENA;
		softecc();
		MEMREG->mr_dvma = 1;	/* clear latching */
		MEMREG->mr_eer = EER_CE_ENA;
		return;
	}
#endif	SUN3_260

	/*
	 * Since we are going down in flames, disable further
	 * memory error interrupts to prevent confusion.
	 */
	MEMREG->mr_er &= ~ER_INTENA;

#ifndef	SUN3_260
	if ((cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50 ||
	    cpu == CPU_SUN3_110 || cpu == CPU_SUN3_60 || cpu == CPU_SUN3_E) &&
	    (per & PER_ERR) != 0) {
		printf("Parity Error Register %b\n", per, PARERR_BITS);
		mess = "parity error";
	}
#endif	/* !SUN3_260 */

#ifdef	SUN3_260
	if ((cpu == CPU_SUN3_260) && (eer & EER_ERR) != 0) {
		printf("ECC Error Register %b\n", eer, ECCERR_BITS);
		if (eer & EER_TIMEOUT)
			mess = "memory timeout error";
		if (eer & EER_UE)
			mess = "uncorrectable ECC error";
		if (eer & EER_WBACKERR)
			mess = "writeback error";
	}
#endif	SUN3_260

	if (!mess) {
		printf("Memory Error Register %b %b\n",
		    per, PARERR_BITS, eer, ECCERR_BITS);
		mess = "unknown memory error";
	}

	printf("DVMA = %x, context = %x, virtual address = %x\n",
		MEMREG->mr_dvma, MEMREG->mr_ctx, MEMREG->mr_vaddr);

	ctx = mmu_getctx();
	mmu_setctx(&ctxs[MEMREG->mr_ctx]);
	mmu_getpte((caddr_t)MEMREG->mr_vaddr, &pme);
	printf("pme = %x, physical address = %x\n", pme,
	    ptob(pme.pg_pfnum) + (MEMREG->mr_vaddr & PAGEOFFSET));
	mmu_setctx(ctx);

	/*
	 * Clear the latching by writing to the top
	 * nibble of the memory address register
	 */
	MEMREG->mr_dvma = 1;

	panic(mess);
	/* NOTREACHED */
}

#ifdef	SUN3_260
int prtsoftecc = 1;
extern int noprintf;
int memintvl = MEMINTVL;

struct	{
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
#endif	SUN3_260

/*
 * Kernel VM initialization.
 * Assumptions about kernel address space ordering:
 *	(1) gap (user space)
 *	(2) kernel text
 *	(4) gap
 *	(3) kernel data/bss
 *	(4) gap
 *	(5) debugger (optional)
 *	(6) monitor
 *	(7) gap (possibly null)
 *	(8) dvma
 *	(9) MONSTARTPAGE (monitor globals, u area, devices)
 */
kvm_init()
{
	int c;
	register addr_t v, tv;
	struct pte pte;

	/*
	 * Put kernel segment in kernel address space.  Make it a
	 * "kernel memory" segment object.
	 */
	(void) seg_attach(&kas, (addr_t)KERNELBASE, (u_int)(vvm - KERNELBASE),
	    &ktextseg);
	(void) segkmem_create(&ktextseg, (caddr_t)NULL);

	(void) seg_attach(&kas, (addr_t)vvm, (u_int)econtig - (u_int)vvm,
	    &kvalloc);
	(void) segkmem_create(&kvalloc, (caddr_t)NULL);

	(void) seg_attach(&kas, (addr_t)SYSBASE, (u_int)(Syslimit - SYSBASE),
	    &kseg);
	(void) segkmem_create(&kseg, (caddr_t)Sysmap);

	/*
	 * This KLUDGE is because earlier proms will blow up during
	 * reboot if the pmgrp which maps the last 2 physical pages
	 * of memory is modified!  Because of this, we have to reserve
	 * this pmgrp if we are on a system which potentially has a
	 * buggy prom.  This is thoroughly disgusting!
	 */
	if (cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50)
		hat_pmgreserve(&kseg, (addr_t)*romp->v_memoryavail);

	/*
	 * Invalidate segments before kernel.
	 */
	for (v = (addr_t)0; v < (addr_t)KERNELBASE; v += PMGRPSIZE)
		mmu_pmginval(v);

	/*
	 * Duplicate kernel into every context.  From this point on,
	 * adjustments to the mmu will automatically copy kernel changes.
	 * Use the ROM to do this copy to avoid switching to unmapped
	 * context.
	 */
	for (c = 0; c < NCTXS; c++)
		if (c != KCONTEXT)
			for (v = (addr_t)0; v < (addr_t)CTXSIZE;
			    v += PMGRPSIZE)
				(*romp->v_setcxsegmap)(c, v,
				    (mmu_getpmg(v))->pmg_num);

	/*
	 * Invalid until start except msgbuf/scb page which is KW
	 */
	for (v = (addr_t)KERNELBASE; v < (addr_t)start; v += PAGESIZE) {
		(void) as_fault(&kas, v, PAGESIZE, F_SOFTLOCK, S_OTHER);
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
	(void) as_fault(&kas, v, (u_int)(etext - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(etext - v), (u_int)
	    (PROT_READ | PROT_EXEC | ((kernprot == 0)? PROT_WRITE : 0)));
	v = (addr_t)roundup((u_int)etext, PAGESIZE);

	/*
	 * Writable until end.
	 */
	(void) as_fault(&kas, v, (u_int)(end - v), F_SOFTLOCK, S_OTHER);
	(void) as_setprot(&kas, v, (u_int)(end - v),
	    PROT_READ | PROT_WRITE | PROT_EXEC);
	v = (addr_t)roundup((u_int)end, PAGESIZE);

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
	v = (addr_t)roundup((u_int)econtig, PMGRPSIZE);
	for (; v < (addr_t)SYSBASE; v += PMGRPSIZE)
		mmu_pmginval(v);

	/*
	 * Invalid to Syslimit.
	 */
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
	for (; v < (addr_t)(CTXSIZE - PMGRPSIZE); v += PMGRPSIZE)
		mmu_pmginval(v);

	/*
	 * Loop through the last segment and set page protections.
	 * We want to invalidate any other pages in last segment
	 * besides EEPROM_ADDR, CLKADDR, MEMREG, INTERREG,
	 * ECCREG and MONSHORTPAGE.
	 * We get the interrupt redzone
	 * for free when the kernel is write protected as the
	 * interrupt stack is the first thing in the data area.
	 * Since MONSHORTPAGE is defined as 32 bit virtual
	 * addresses (to get short references to work), we must mask
	 * with ADDRESS_MASK to get only the bits we really want to look at.
	 * Be sure to remove user access to these pages.
	 */
	hat_pmgreserve(&kseg, v);
	hat_chgprot(&kseg, v, PMGRPSIZE, ~PROT_USER);
	for (; v < (caddr_t)CTXSIZE; v += PAGESIZE) {
		if ((u_int)v != (MONSHORTPAGE & ADDRESS_MASK) &&
		    (u_int)v != (u_int)EEPROM_ADDR &&
		    (u_int)v != (u_int)CLKADDR &&
		    (u_int)v != (u_int)MEMREG &&
		    (u_int)v != (u_int)INTERREG &&
		    (u_int)v != (u_int)ECCREG)
			mmu_setpte(v, mmu_pteinvalid);
	}

	/*
	 * Invalidate all the unlocked pmegs.
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
