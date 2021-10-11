#ident "@(#)machdep.c	1.1 7/30/92"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/vmmac.h>
#include <sun/openprom.h>
#include <machine/buserr.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/enable.h>
#include <machine/scb.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/clock.h>
#include <machine/intreg.h>
#include <machine/eeprom.h>
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include <machine/vm_hat.h>
#include "allregs.h"
#include "../../debug/debug.h"
#include <debug/debugger.h>

extern int errno;


int istrap = 0;
int scbsyncdone = 0;

/*
 * The next group of variables and routines handle the
 * Open Boot Prom devinfo or property information.
 *
 * These machine-dependent quantities are set from the prom properties.
 * For the time being, set these to "large, safe" values.
 * XXX - does this all want to be packaged in a header file?
 */
extern void fiximp();
extern	int print_cpu_done;
/*
 * properties tchotchkes
 */
#define GETPROPLEN	prom_getproplen
#define GETPROP		prom_getprop
#define NEXT		prom_nextnode
#define CHILD		prom_childnode
extern int getprop();
int debug_props = 0;		/* Turn on to enable debugging message */

#ifdef	MULTIPROCESSOR
#define	CPUTOMID(x)	((x) | 0x8)
#define	MIDTOCPU(x)	((x) & 0x3)
int cpus_enabled[NCPU];
#ifdef	PROM_IDLECPUS_WORKS
extern int prom_idlecpu();
extern int prom_resumecpu();
unsigned cpus_nodeid[NCPU];
#endif	/* PROM_IDLECPUS_WORKS */
#endif	MULTIPROCESSOR

/*
 * Open proms give us romp as a variable
 */
struct sunromvec *romp = (struct sunromvec *)0xFFE81000;

int fake_bpt;			/* place for a fake breakpoint at startup */
jmp_buf debugregs;		/* context for debugger */
jmp_buf mainregs;		/* context for debuggee */
jmp_buf_ptr curregs;		/* pointer to saved context for each process */
struct allregs regsave; 	/* temp save area--align to double */
struct scb *mon_tbr, *our_tbr;	/* storage for %tbr's */

int use_kern_tbr = 0;

extern char start[], estack[], etext[], edata[], end[];
extern int exit();
extern struct scb *gettbr();

extern int cache;

/*
 * Definitions for registers in jmp_buf
 */
#define	JB_PC	0
#define	JB_SP	1

#define	CALL(func)	(*(int (*)())((int)(func) - (int)start + (int)real))
#define	RELOC(adr)	((adr) - (char *)start + real)
extern setcontext(), getsegmap(), setsegmap(), getpgmap(), setpgmap();
void Setpgmap();

/*
 * get_rom_l2_ptr:
 *  return the physical address in the current context of the pte 
 *  that would map virtual address 'vaddr' for 'size' bytes. 
 *  We are only setup to use level-2 (256KB) mappings. If a level-0
 *  or level-1 mapping already exists then do not need to do anything.
 */

#define UPTPE_NULL (union ptpe *)NULL

union ptpe *
get_l2_ptr(vaddr)
u_int vaddr;
{
	union ptpe ct, rp, l1;
	
	ct.ptpe_int = mmu_getctp();
	rp.ptpe_int = ldphys(ct.ptp.PageTablePointer << MMU_STD_PTPSHIFT);
	
	switch (rp.ptp.EntryType) {
		
	      case MMU_ET_PTE: /* pte in context table */
		return (UPTPE_NULL);
		
	      case MMU_ET_PTP: /* ptp in context table */
		l1.ptpe_int = 
			ldphys((rp.ptp.PageTablePointer << MMU_STD_PTPSHIFT) |
				     MMU_L1_INDEX(vaddr) << 2);
		
		switch (l1.ptp.EntryType) {
			
		      case MMU_ET_PTE: /* pte in level-1 table */
			return (UPTPE_NULL);
			
		      case MMU_ET_PTP: /* ptp in level-1 table */
			return ((union ptpe *)((l1.ptp.PageTablePointer <<
				MMU_STD_PTPSHIFT) |
				MMU_L2_INDEX(vaddr) << 2));
		}
	}
}

/*
 * This routine is called before we are relocated to the correct address,
 * so everything is done via funny macros when dealing w/ globals.
 * Also, we have a candidate for "worst abuse of preprocessor" award:
 * MONSTART and MONEND are actually defined using romp, but _romp
 * isn't set yet.  Call our sunromvec argument "romp" so that the global
 * variable is hidden in this function and the correct value is used.
 */
early_startup(real, romp)
	register char *real;
	register struct sunromvec *romp;
{
	register u_int cnt, i;
	register int *from, *to;
	register int lastpmsav, lastpgsav;
	caddr_t	 vstart;
	int too_big = 0;

	/* number of level-3 tables required by the debugger */
#define NDBGL3PTS (mmu_btop(DEBUGSIZE)/MMU_NPTE_THREE)

	/*(void) CALL(setcontext)(0); - remove? */

	/*
	 * Find the top of physical memory and take as many pages as we need.
	 * We assume the physmem chunks are in ascending order and the last
	 * chunk is big enough to hold us. (It actually doesn't matter if
	 * the pages are in order; we handle that gracefully.) We reserve
	 * an extra page for our own level-3 page tables. We can't use level-2
	 * page tables since that would require us to be 256KB aligned which
	 * could be more wasteful of memory than just grabbing a page here.
	 *
	 * It seems that in allocating the extra page for the level-3 
	 * page tables we run into a problem with the PROM virtual address
	 * allocation with sbrk.  Do we need this extra page here???
	 * Right now have worked around in sbrk.
	 * Cant call sbrk here, so if need page, need workaround.
	 */
	cnt = mmu_btopr(end - start) + 1;	/* XXX */
	vstart = (caddr_t)(*romp->op2_alloc)((int)start, mmu_ptob(cnt));
	if ((u_int)vstart != (u_int)start)
		too_big = 1;

	/*
	 * Copy program up to correct address
	 */
	for (to = (int *)start, from = (int *)real; to < (int *)edata; )
		*to++ = *from++;
	for (to = (int *)edata; to < (int *)end; ++to)
		*to = 0;

	/*
	 * Now we can reference global variables,
	 * save page count and monitor's nmi routine address.
	 * Dont need this information on OBP.
	 */
	lastpg = 0;
	pagesused = 0;
	mon_tbr = gettbr();
	if (too_big) {
		printf("kadb: size %x exceeded available memory\n",
			mmu_ptob(cnt));
	}
}

/*
 * Startup code after relocation.
 */
startup()
{
	register int i;
	register int pg;

	/*
	 * Set our implementation parameters from prom properties.
	 */
	fiximp();

	/*
	 * Fix up old scb.
	 */
	kadbscbsync();
	spl13();		/* we can take nmi's now */

	/*
	 * Now make text (and dvec) read only,
	 * this also sets a stack redzone
	 */
#ifdef later
	for (i = (int)start; i < (int)etext; i += MMU_PAGESIZE) {
		pg = getpgmap(i);
		Setpgmap(i, (pg & ~PG_PROT) | PG_KR);
	}
#endif
	mmu_flushall();

}

scbsync()
{
	kadbscbsync();
	scbsyncdone = 1;
}

kadbscbsync()
{
	register struct scb *tbr;
	register int otbr_pg;
	extern trapvec tcode;

	tbr = gettbr();
	otbr_pg = getpgmap(tbr);
	Setpgmap(tbr, (otbr_pg & ~PG_PROT) | PG_KW);

	tbr->user_trap[TRAPBRKNO-1] = tcode;
	tbr->user_trap[TRAPBRKNO] = tcode;
	Setpgmap(tbr, otbr_pg);
	if (scbstop) {
		/*
		 * We're running interactively. Trap into the debugger
		 * so the user can look around before continuing.
		 * We use trap TRAPBRKNO-1: "enter debugger"
		 */
		scbstop = 0;
		asm_trap(TRAPBRKNO-1);
	}
}

/*
 * Sys_trap trap handlers.
 */

/*
 * level15 (memory error) interrupt.
 */
level15()
{
	/*
	 * For now, the memory error regs are not mapped into the debugger,
	 * so we just print a message.
	 */
	printf("memory error\n");
}

/*
 * Miscellanous fault error handler
 */
fault(trap, trappc, trapnpc)
	register int trap;
	register int trappc;
	register int trapnpc;
{
	register int ondebug_stack;
	register u_int *pc;
	register u_int realpc;

	ondebug_stack = (getsp() > (int)etext && getsp() < (int)estack);
	if (trap == T_DATA_FAULT && nofault && ondebug_stack) {
		jmp_buf_ptr sav = nofault;

		nofault = NULL;
		_longjmp(sav, 1);
		/*NOTREACHED*/
	}

	traceback(getsp());
	/*
	 * If we are on the debugger stack and
	 * abort_jmp is set, do a longjmp to it.
	 */
	if (abort_jmp && ondebug_stack) {
		printf("abort jump: trap %x sp %x pc %x npc %x\n",
			trap, getsp(), trappc, trapnpc);
		printf("etext %x estack %x edata %x nofault %x\n",
			etext, estack, edata, nofault);
		_longjmp(abort_jmp, 1);
		/*NOTREACHED*/
	}

	/*
	 * Ok, the user faulted while not in the
	 * debugger. Enter the main cmd loop
	 * so that the user can look around...
	 */
	/*
	 * There is a problem here since we really need to tell cmd()
	 * the current registers.  We would like to call cmd() in locore
	 * but the interface is not really set up to handle this (yet?)
	 */

	printf("fault and calling cmd: trap %x sp %x pc %x npc %x\n",
	    trap, getsp(), trappc, trapnpc);
	cmd();	/* error not resolved, enter debugger */
}
long trap_window[25];

static jmp_buf_ptr saved_jb;
static jmp_buf jb;
extern int debugging;


/*
 * Peekc is so named to avoid a naming conflict
 * with adb which has a variable named peekc
 */
int
Peekc(addr)
	char *addr;
{
	u_char val;

	saved_jb = nofault;
	nofault = jb;
	errno = 0;
	if (!_setjmp(jb)) {
		val = *addr;
		/* if we get here, it worked */
		nofault = saved_jb;
		return ((int)val);
	}
	/* a fault occured */
	nofault = saved_jb;
	errno = EFAULT;
	return (-1);
}

short
peek(addr)
	short *addr;
{
	short val;

	saved_jb = nofault;
	nofault = jb;
	errno = 0;
	if (!_setjmp(jb)) {
		val = *addr;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (val);
	}
	/* a fault occured */
	nofault = saved_jb;
	errno = EFAULT;
	return (-1);
}
long
peekl(addr)
	long *addr;
{
	long val;

	saved_jb = nofault;
	nofault = jb;
	errno = 0;
	if (!_setjmp(jb)) {
		val = *addr;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (val);
	}
	/* a fault occured */
	nofault = saved_jb;
	errno = EFAULT;
	return (-1);
}

int
pokec(addr, val)
	char *addr;
	char val;
{

	saved_jb = nofault;
	nofault = jb;
	errno = 0;
	if (!_setjmp(jb)) {
		*addr = val;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (0);
	}
	/* a fault occured */
	nofault = saved_jb;
	errno = EFAULT;
	return (-1);
}

int
pokel(addr, val)
	long *addr;
	long val;
{

	saved_jb = nofault;
	nofault = jb;
	errno = 0;
	if (!_setjmp(jb)) {
		*addr = val;
		/* if we get here, it worked */
		nofault = saved_jb;
		return (0);
	}
	/* a fault occured */
	nofault = saved_jb;
	errno = EFAULT;
	return (-1);
}

poketext(addr, val)
	int *addr;
	int val;
{
	int pg = 0;

	pg = getpgmap((int)addr);
	if (PTE_ETYPE(pg) != MMU_ET_PTE) {
		if (debugging > 2)
			printf("poketext: invalid page map %X at %X\n",
			    pg, addr);
		goto err;
	}

	if ((pg & PTE_CE_MASK) && 
		!((cache == CACHE_PAC) || (cache == CACHE_PAC_E))) {
		vac_pageflush(addr);
		if (btop(addr + sizeof (int) - 1) != btop(addr))
			vac_pageflush(addr + sizeof (int) - 1);
	}

	if ((pg & PG_PROT) == PG_KR)
		Setpgmap(addr, (pg & ~PG_PROT) | PG_KW);
	else if ((pg & PG_PROT) == PG_URKR)
		Setpgmap(addr, (pg & ~PG_PROT) | PG_UW);
	/* otherwise it is already writeable */
	*addr = val;		/* should be prepared to catch a fault here? */
	/*
	 * Reset to page map to previous entry,
	 * but mark as modified
	 */
	if ((pg & PTE_CE_MASK) && 
		!((cache == CACHE_PAC) || (cache == CACHE_PAC_E))) {
		vac_pageflush(addr);
		if (btop(addr + sizeof (int) - 1) != btop(addr))
			vac_pageflush(addr + sizeof (int) - 1);
	}

	/* XXX - why not referenced also? */
	Setpgmap(addr, pg | PTE_MOD(1));
	mmu_flushall();
	errno = 0;
	return (0);

err:
	errno = EFAULT;
	return (-1);
}

scopy(from, to, count)
	register char *from;
	register char *to;
	register int count;
{
	register int val;

	for (; count > 0; count--) {
		if ((val = Peekc(from++)) == -1)
			goto err;
		if (pokec(to++, val) == -1)
			goto err;
	}
	return (0);
err:
	errno = EFAULT;
	return (-1);
}

/*
 * Setup a new context to run at routine using stack whose
 * top (end) is at sp.  Assumes that the current context
 * is to be initialized for mainregs and new context is
 * to be set up in debugregs.
 */
spawn(sp, routine)
	char *sp;
	func_t routine;
{
	char *fp;
	int res;

	if (curregs != 0) {
		printf("bad call to spawn\n");
		exit(1);
	}
	if ((res = _setjmp(mainregs)) == 0) {
		/*
		 * Setup top (null) window.
		 */
		sp -= WINDOWSIZE;
		((struct rwindow *)sp)->rw_rtn = 0;
		((struct rwindow *)sp)->rw_fp = 0;
		/*
		 * Setup window for routine with return to exit.
		 */
		fp = sp;
		sp -= WINDOWSIZE;
		((struct rwindow *)sp)->rw_rtn = (int)exit - 8;
		((struct rwindow *)sp)->rw_fp = (int)fp;
		/*
		 * Setup new return window with routine return value.
		 */
		fp = sp;
		sp -= WINDOWSIZE;
		((struct rwindow *)sp)->rw_rtn = (int)routine - 8;
		((struct rwindow *)sp)->rw_fp = (int)fp;
		/* copy entire jump buffer to debugregs */
		bcopy((caddr_t)mainregs, (caddr_t)debugregs, sizeof (jmp_buf));
		debugregs[JB_SP] = (int)sp;	/* set sp */
		curregs = debugregs;
		regsave.r_npc = (int)&fake_bpt;
		_longjmp(debugregs, 1);		/* jump to new context */
		/*NOTREACHED*/
	}

}

doswitch()
{
	int res;

	if ((res = _setjmp(curregs)) == 0) {
		/*
		 * Switch curregs to other descriptor
		 */
		if (curregs == mainregs) {
			curregs = debugregs;
		} else /* curregs == debugregs */ {
			curregs = mainregs;
		}
		_longjmp(curregs, 1);
		/*NOTREACHED*/
	}
	/*
	 * else continue on in new context
	 */
}

/*
 * Main interpreter command loop.
 */
cmd()
{
	int addr;

	dorun = 0;

	/*
	 * See if the sp says that we are already on the debugger stack
	 */
	reg = (struct regs *)&regsave;
	addr = getsp();
	if (addr > (int)etext && addr < (int)estack) {
		printf("Already in debugger!\n");
		return;
	}

	do {
		doswitch();
		if (dorun == 0)
			printf("cmd: nothing to do\n");
	} while (dorun == 0);
	/* we don't need to splx since we are returning to the caller */
	/* and will reset his/her state */
}

/*
 * Call into the monitor (hopefully)
 */
montrap(funcptr)
	int (*funcptr)();
{
	our_tbr = gettbr();
	if (use_kern_tbr) {
		settbr(regsave.r_tbr);	
	} else
		settbr(mon_tbr);
	/*
	 * Must do the following because libprom calls
	 * montrap to "enter" and "exit_to" the prom.
	 * Since kadb has this montrap function we
	 * are calling through here.  Thus, if an argument
	 * is passed do as libprom's routine does, else
	 * do as before.
	 */
	if (funcptr)
	    (*funcptr)();
	else
	    (*romp->op_exit)();

	settbr(our_tbr);
}

/*
 * set delay constant for usec_delay()
 * NOTE: we use the fact that the per-
 * processor clocks are available and
 * mapped properly at "utimers".
 */
static
setcpudelay()
{
	/* If need this call grab the latest from kernels machdep.c */
}
 
/*
 * Set the pte (level-3) for address v using the software pte given.
 * Setpgmap() automatically turns off the `cacheable' bit
 * for all mappings between DEBUGSTART and DEBUGEND.
 */
void
Setpgmap(v, pte)
	caddr_t v;
	u_int pte;
{
	setpgmap(v, pte);
}

traceback(sp)
	caddr_t sp;
{
	register u_int tospage;
	register struct frame *fp;
	static int done = 0;

#ifdef PARTIAL_ALIGN
	if (partial_align? ((int)sp & 0x3): ((int)sp & 0x7)) {
#else
	if ((int)sp & (STACK_ALIGN-1)) {
#endif PARTIAL_ALIGN
		printf("traceback: misaligned sp = %x\n", sp);
		return;
	}
	flush_windows();
	tospage = (u_int)btoc(sp);
	fp = (struct frame *)sp;
	printf("Begin traceback... sp = %x\n", sp);
	while (btoc((u_int)fp) == tospage) {
		if (fp == fp->fr_savfp) {
			printf("FP loop at %x", fp);
			break;
		}
		printf("Called from %x, fp=%x, args=%x %x %x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2],
		    fp->fr_arg[3], fp->fr_arg[4], fp->fr_arg[5]);
#ifdef notdef
		printf("\tl0-l7: %x, %x, %x, %x, %x, %x, %x, %x\n",
		    fp->fr_local[0], fp->fr_local[1],
		    fp->fr_local[2], fp->fr_local[3],
		    fp->fr_local[4], fp->fr_local[5],
		    fp->fr_local[6], fp->fr_local[7]);
#endif
		fp = fp->fr_savfp;
		if (fp == 0)
			break;
	}
	printf("End traceback...\n");
}

our_die_routine(retaddr)
        register caddr_t retaddr;
{
#ifdef NOTYET
        (*romp->op2_chain)(0, 0, retaddr+8);
        /* NOTREACHED */
#endif
        ;
}

struct memlist *
getmemlist(name, prop)
        char *name, *prop;
{
        int nodeid;
        int i, j, k, chunks;
        u_int propsize;
        struct dev_reg *rp;
        struct memlist *pmem;
        static caddr_t bufp = (caddr_t)0;
        static u_int left;

        if (bufp == (caddr_t) 0) {
                bufp = (caddr_t)(*romp->op2_alloc)((caddr_t)0, PAGESIZE);
                left = PAGESIZE;
        }

        /*
         * First find the right node.
         */
        nodeid = search(NEXT(0), name, bufp);
        if (nodeid == 0)
                return ((struct memlist *)0);
 
        /*
         * Allocate memlist elements, one per dev_reg struct.
         */
        propsize = (u_int)GETPROPLEN(nodeid, prop);
        chunks = propsize / sizeof (struct dev_reg);
        if (chunks == 0)
                return ((struct memlist *)0);
        if (left < (chunks * sizeof (struct memlist))) {
                printf("memlists too big");
                return ((struct memlist *)0);
        }
        pmem = (struct memlist *)bufp;
        bzero((caddr_t)pmem, (u_int) chunks * sizeof (struct memlist));
        left -= (u_int) chunks * sizeof (struct memlist);
        bufp += (u_int) chunks * sizeof (struct memlist);
        if (left < propsize) {
                printf("buffer too small");
                return ((struct memlist *)0);
        }
        /*
         * Fill in memlist chunks.
         */
        rp = (struct dev_reg *)bufp;
        bzero((caddr_t)rp, propsize);
        left -= propsize;
        bufp += propsize;
 
        (void)GETPROP(nodeid, prop, rp);
        for (i = 0; i < chunks; ++i) {
	/*
	 * Cant test against BT_ or SP_ because using the 32-bit space
	 * as bustype.  So since no define for OBMEM space, hardcoding
	 * in the value of 0x0.
	 */
                if (rp[i].reg_bustype != 0x0)
                        continue;
 
                /*
                 * Insert the new element into the array of list nodes
                 * so that the array remains sorted in ascending order.
                 */
 
                /* Find the first element with a larger address */
                for (j = 0; j < i; j++)
                        if (pmem[j].address > (u_int)rp[i].reg_addr)
                                break;
 
                /* Copy following elements up to make room for the new one */
                for (k = i; k > j; --k)
                        pmem[k] = pmem[k-1];
 
                /* Insert the new element */
                pmem[j].address = (u_int)rp[i].reg_addr;
                pmem[j].size = rp[i].reg_size;
        }
        for (i = 1; i < chunks; ++i) {
                if (pmem[i].address)
                        pmem[i-1].next = &pmem[i];
        }
        return (pmem);
}

search(node, name, buf)
        int node;
        char *name;
        char *buf;
{
        int id;
 
        *buf = '\0';
        (void)GETPROP(node, "name", buf);
        if (strcmp(buf, name) == 0) {
                return (node);
        }
        id = NEXT(node);
        if (id && (id = search(id, name, buf)))
                return (id);
        id = CHILD(node);
        if (id && (id = search(id, name, buf)))
                return (id);
        return (0);
}

/*
 * setpgmap:
 *  set the pte that maps virtual address `v' to `pte'.
 *  for success `v' must be mapped by a level-3 pte
 *  XXX - fix this so that we can look at monitors pages.
 */
setpgmap(v, pte)
caddr_t v;
u_int pte;
{
	union ptpe *l2ptr;
	union ptpe l2ptpe;
	union ptes *l3ptr;
	union ptes l3ptes;

	l2ptr = get_l2_ptr(v);
	l2ptpe.ptpe_int = ldphys(l2ptr);
	l3ptr = ((union ptes *)(l2ptpe.ptp.PageTablePointer << 
			       MMU_STD_PTPSHIFT)) + MMU_L3_INDEX((u_int)v);
	l3ptes.pte_int = ldphys((u_int *)l3ptr);


	stphys((u_int *)l3ptr, pte);

	/* Flush the TLB after fiddling with PTE */
	srmmu_mmu_flushpage(v);

	return (0);
}

/*
 * getpgmap:
 *  return the pte that maps virtual address `v'.
 *  for success `v' must be mapped by a level-3 pte
 */
getpgmap(v)
caddr_t v;
{
	union ptpe *l2ptr;
	union ptpe l2ptpe;
	union ptes *l3ptr;
	union ptes l3ptes;

	l2ptr = get_l2_ptr(v);
	l2ptpe.ptpe_int = ldphys(l2ptr);
	l3ptr = ((union ptes *)(l2ptpe.ptp.PageTablePointer << 
			       MMU_STD_PTPSHIFT)) + MMU_L3_INDEX((u_int)v);
	l3ptes.pte_int = ldphys((u_int *)l3ptr);

	return (l3ptes.pte_int);
}

#ifdef	MULTIPROCESSOR
/*
 * XXX-These functions should eventually implement the cross-call
 * mechanism, or someother such method.
 */
idle_cpus(mid)
dnode_t	mid;
{
int	cur_cpuid, i;

	cur_cpuid = MIDTOCPU(mid);
	/*
	 * Only print which cpu we are on, if we have more
	 * than one cpu.  Else, its unnecessary.
	 */
	for (i=1; i < NCPU; i++) {
		if (cpus_enabled[i]) {
			if (!print_cpu_done)
				printf("currently on cpu%d\n", cur_cpuid);
			break;
		}
	}
#ifdef	PROM_IDLECPU_WORKS
	for (i=0; i < NCPU; i++) {
		if (cpus_enabled[i] && (i != cur_cpuid)) {
		/*
		 * prom_idlecpu returns 0 on success, -1 on failure
		 */
		    if (prom_idlecpu(cpus_nodeid[i]))
			printf("-->failed to idle cpu%d\n", i);
		}
	}
	/* Now back to the regularly scheduled program */
#endif	/* PROM_IDLECPU_WORKS */
	cmd();
}

#ifdef	PROM_IDLECPUS_WORKS
/*
 * On way out of kadb have to get other cpu's back up.
 */
resume_cpus(mid)
dnode_t	mid;
{
int	cur_cpuid, i;

	cur_cpuid = MIDTOCPU(mid);
	for (i=0; i < NCPU; i++) {
		if (cpus_enabled[i] && (i != cur_cpuid)) {
		/*
		 * prom_resumecpu returns 0 on success, -1 on failure
		 */
		    if (prom_resumecpu(cpus_nodeid[i]))
			printf("-->failed to resume cpu%d\n", i);
		}
	}
}
#endif	/* PROM_IDLECPUS_WORKS */
#endif	MULTIPROCESSOR
