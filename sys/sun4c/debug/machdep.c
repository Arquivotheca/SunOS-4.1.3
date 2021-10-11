#ident "@(#)machdep.c 1.1 92/07/30"

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/vmmac.h>
#include <sun/openprom.h>
#include <machine/buserr.h>
#include <machine/enable.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/scb.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/clock.h>
#include <machine/intreg.h>
#include <machine/eeprom.h>
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <machine/frame.h>
#include "allregs.h"
#include <debug/debug.h>
#include <debug/debugger.h>

extern int errno;
extern char *malloc();

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
extern int Cpudelay;
extern int nwindows;
extern u_int npmgrps;
extern int vac_size;
extern int vac_linesize;
extern int segmask;
extern int mips_off, mips_on;		/* approx. MIPS with cache off/on */
/*
 * properties tchotchkes
 */
#define	GETPROPLEN	prom_getproplen
#define	GETPROP		prom_getprop
#define	NEXT		prom_nextnode
#define	CHILD		prom_childnode
extern int getprop();

/*
 * Open proms give us romp as a variable
 */
struct sunromvec *romp = (struct sunromvec *)0xFFE81000;

int fake_bpt;			/* place for a fake breakpoint at startup */
jmp_buf debugregs;		/* context for debugger */
jmp_buf mainregs;		/* context for debuggee */
jmp_buf_ptr curregs;		/* pointer to saved context for each process */
int cache_on;			/* cache is being used */
struct allregs regsave; 	/* temp save area--align to double */
struct scb *mon_tbr, *our_tbr;	/* storage for %tbr's */

int use_kern_tbr = 0;

extern char start[], estack[], etext[], edata[], end[];
extern int exit();
extern struct scb *gettbr();

/*
 * Definitions for registers in jmp_buf
 */
#define	JB_PC	0
#define	JB_SP	1

#define	CALL(func)	(*(int (*)())((int)(func) - (int)start + (int)real))
#define	RELOC(adr)	((adr) - (char *)start + real)
extern setcontext(), getsegmap(), setsegmap(), getpgmap(), setpgmap();
extern printf();
void Setpgmap();

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
	register u_int cnt, pg, i;
	register int *from, *to;
	register int pm, lopm;
	register int lastpmsav, lastpgsav;
	register struct memlist *pmem;
	caddr_t	 vstart;
	int too_big = 0;
	u_int npmgrps;		/* yes, this hides the global too */

	(void) CALL(setcontext)(0);
	/*
	 * We need to know npmgrps so we can compute PMGRP_INVALID.
	 * We'll do this again later for the real globals.
	 * Alas! We can't just call getprop(), because the global romp
	 * isn't even mapped in yet. We want to do this:
	 *	if ((int)CALL(getprop)(0, "mmu-npmg", &npmgrps) <= 0)
	 *		npmgrps = NPMGRPS_60;
	 * but we have to do this:
	 */
	if ((*romp->op_config_ops->devr_getproplen) 
	    (0, RELOC("mmu-npmg")) == sizeof (int))
	{
		(*romp->op_config_ops->devr_getprop) 
		 (0, RELOC("mmu-npmg"), &npmgrps);
	} else
		npmgrps = NPMGRPS_60;

	/*
	 * Find the top of physical memory and take as many pages as we need.
	 * We assume the physmem chunks are in ascending order and the last
	 * chunk is big enough to hold us. (It actually doesn't matter if
	 * the pages are in order; we handle that gracefully.)
	 */
	cnt = mmu_btopr(end - start);
	if (romp->op_romvec_version > 0) {
		vstart = (caddr_t)(*romp->op2_alloc)((int)start,
			mmu_ptob(cnt));
		if ((u_int)vstart != (u_int)start)
			too_big = 1;
	} else {
		pmem = *romp->v_availmemory;
		while (pmem->next != (struct memlist *)NULL)
			pmem = pmem->next;
		pg = mmu_btopr(pmem->address + pmem->size) - cnt;
		if (mmu_ptob(cnt) > pmem->size) {
			too_big = 1;
		} else {
			too_big = 0;
			pmem->size -= mmu_ptob(cnt);
		}

		/*
		 * Look for the lowest pmeg in use in the monitor
		 * and "DVMA" space
		 */
		lopm = PMGRP_INVALID;
		for (i = MONSTART; i != 0; i += PMGRPSIZE)
			if (((pm = CALL(getsegmap)(i)) < lopm) && pm)
				lopm = pm;

		/*
		 * Adjust down from there to get our starting pmeg
		 * and save copies of last pmeg and page used.
		 */
		lopm -= ((unsigned)(end - start) + PMGRPOFFSET) >> PMGRPSHIFT;
		lastpmsav = lopm;
		lastpgsav = pg;

		for (i = (int)start; i < (int)end; i += MMU_PAGESIZE) {
			if (CALL(getsegmap)(i) == PMGRP_INVALID) {
				register u_int j = i & ~PMGRPOFFSET;
				u_int last = j + NPMENTPERPMGRP * MMU_PAGESIZE;

				(void) CALL(setsegmap)(i, lopm++);
				while (j < last) {
					(void) CALL(setpgmap)(j, 0);
					j += MMU_PAGESIZE;
				}
			}
			(void) CALL(setpgmap)(i, PG_V | PG_KW | PG_NC | pg++);
		}
	}

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
	 * Don't need this information on OBP.
	 */
	if (romp->op_romvec_version > 0) {
		lastpg = 0;
		lastpm = 0;
		pagesused = 0;
		mon_tbr = gettbr();
		if (too_big) {
			CALL(printf)(
			    RELOC("kadb: size %x exceeded available memory\n"),
			    mmu_ptob(cnt));
			_exit(1);
		}
	} else {
		lastpg = lastpgsav;
		lastpm = lastpmsav;
		pagesused = cnt;
		mon_tbr = gettbr();
		if (too_big) {
			CALL(printf)(
			    RELOC(
				"kadb: size %x exceeded available memory %x\n"),
			    mmu_ptob(cnt), pmem->size);
			_exit(1);
		}
	}
}

/*
 * Startup code after relocation.
 */
startup()
{
	register int i;
	register int pg;
	u_char intreg;
	register int vaddr, pmeg;
	register int cpu;
	extern void prom_init();

	
	prom_init("Kadb");
	
	/*
	 * Set our implementation parameters from the prom properties.
	 */
	fiximp();

	if (prom_getversion() > 0) {
		vaddr = (int)(*romp->op2_map)(COUNTER_ADDR, OBIO,
			OBIO_COUNTER_ADDR, MMU_PAGESIZE);
		vaddr = (int)(*romp->op2_map)(INTREG_ADDR, OBIO,
			OBIO_INTREG_ADDR, MMU_PAGESIZE);
	} else {
		/*
		 * We need to get a pmeg for the devices we are mapping in.
		 * Assume the devices lie within the same segment.
		 */
		vaddr = EEPROM_ADDR & COUNTER_ADDR & INTREG_ADDR;
		pmeg = getsegmap(vaddr);
		if (pmeg == PMGRP_INVALID) {
			setsegmap(vaddr, --lastpm);
		}
		/*
		 * Now map! Remember: on a sun4c, the clock is in the eeprom.
		 */
		Setpgmap((caddr_t)EEPROM_ADDR,
		    PG_V|PG_KW|PGT_OBIO|btop(OBIO_EEPROM_ADDR));
		Setpgmap((caddr_t)COUNTER_ADDR,
		    PG_V|PG_KW|PGT_OBIO|btop(OBIO_COUNTER_ADDR));
		Setpgmap((caddr_t)INTREG_ADDR,
		    PG_V|PG_KW|PGT_OBIO|btop(OBIO_INTREG_ADDR));
	}
	set_clk_mode(IR_ENA_CLK14, 0);

	/*
	 * Fix up old scb.
	 */
	kadbscbsync();
	spl13();		/* we can take nmi's now */

	/*
	 * Now make text (and dvec) read only,
	 * this also sets a stack redzone
	 */
	for (i = (int)start; i < (int)etext; i += MMU_PAGESIZE) {
		pg = getpgmap(i);
		Setpgmap(i, (pg & ~PG_PROT) | PG_KR);
	}

	cache_on = getenablereg() &  ENA_CACHE;
	if (cache_on) {
		setdelay(mips_on);
	} else {
		setdelay(mips_off);
		vac_init();			/* invalidate entire cache */
	}
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
	if ((pg & PG_V) == 0) {
		if (debugging > 2)
			printf("poketext: invalid page map %X at %X\n",
			    pg, addr);
		goto err;
	}
	if ((pg & PGT_MASK) != PGT_OBMEM) {
		if (debugging > 2)
			printf("poketext: incorrect page type %X at %X\n",
			    pg, addr);
		goto err;
	}

	vac_pageflush(addr);
	if (btop(addr + sizeof (int) - 1) != btop(addr))
		vac_pageflush(addr + sizeof (int) - 1);

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
	vac_pageflush(addr);
	if (btop(addr + sizeof (int) - 1) != btop(addr))
		vac_pageflush(addr + sizeof (int) - 1);
	Setpgmap(addr, pg | PG_M);
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
	int resetclk = 0;
	u_char intreg;
	int addr, t;
	int i;
	u_char interreg;
	int s;

	dorun = 0;
	i = 0;

	/*
	 * See if the sp says that we are already on the debugger stack
	 */
	reg = (struct regs *)&regsave;
	addr = getsp();
	if (addr > (int)etext && addr < (int)estack) {
		printf("Already in debugger!\n");
		return;
	}
	cache_on = getenablereg() &  ENA_CACHE;
	if (cache_on)
		setdelay(mips_on);
	else
		setdelay(mips_off);
	do {
		doswitch();
		if (dorun == 0)
			printf("cmd: nothing to do\n");
	} while (dorun == 0);
	/* we don't need to splx since we are returning to the caller */
	/* and will reset his/her state */
}

/*
 * Return an available physical page number
 */
int
getapage()
{
	register struct memlist *pmem;
	register caddr_t	addr;

	/*
	 * Find the top of physical memory and take the top pages.
	 * We assume the physmem chunks are in ascending order and
	 * the last chunk is non-zero.
	 */
	pmem = *romp->v_availmemory;

	for( ; pmem->next != (struct memlist *)NULL; pmem = pmem->next)
		;
	if (pmem->size < MMU_PAGESIZE) {
		printf("panic: getapage: last physmem chunk too small\n");
		_exit();
	}
	pmem->size -= MMU_PAGESIZE;
	return (mmu_btop(pmem->address + pmem->size));
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
 * delay ~= (usecs * (Cpudelay * 2 + 3) + 8) / mips
 *  => Cpudelay = ceil((mips - 3) / 2)
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
 * Set the pme for address v using the software pte given.
 * Setpgmap() automatically turns on the ``no cache'' bit
 * for all mappings between DEBUGSTART and DEBUGEND.
 */
void
Setpgmap(v, pte)
	caddr_t v;
	int pte;
{
	if (v >= (caddr_t)DEBUGSTART && v <= (caddr_t)DEBUGEND)
		pte |= PG_NC;
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
	(*romp->op_chain)(0, 0, retaddr+8);
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
		if (rp[i].reg_bustype != OBMEM)
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
