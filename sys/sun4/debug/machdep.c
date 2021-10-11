#ifndef lint
static  char sccsid[] = "@(#)machdep.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/vmmac.h>
#include <sun4/buserr.h>
#include <sun4/enable.h>
#include <sun4/mmu.h>
#include <sun4/cpu.h>
#include <sun4/pte.h>
#include <sun4/scb.h>
#include <sun4/psl.h>
#include <sun4/trap.h>
#include <sun4/clock.h>
#include <sun4/intreg.h>
#include "reg.h"
#include "../../debug/debug.h"
#include "../../debug/debugger.h"
#include "asm_linkage.h"

extern int errno;
#define	DVMA	0xfff00000

int istrap = 0;
int scbsyncdone = 0;

int cpudelay = 0;	/* old macros: shift right amount to scale (n << 4) */
int Cpudelay = 11;	/* new macros: count down counter initial value */
int cpu;

int vac_linesize;		/* number of bytes in a cache line */
int vac_pglines;		/* number of cache lines in a page */
int vac_nlines;			/* number of cache lines in cache */

int fake_bpt;			/* place for a fake breakpoint at startup */
jmp_buf debugregs;		/* context for debugger */
jmp_buf mainregs;		/* context for debuggee */
jmp_buf_ptr curregs;		/* pointer to saved context for each process */
int cache_on;			/* cache is being used */
struct allregs regsave;		/* temp save area */

int clock_type;
static int clock_peek = 0;
unsigned long old_clock0_pte, old_counter_pte; /* save old ptes */
extern char estack[], etext[];

/*
 * Definitions for registers in jmp_buf
 */
#define	JB_PC	0
#define	JB_SP	1

static jmp_buf state;			/* used for temporary games */

/*
 * Startup code after relocation.
 */
startup()
{
	register int i;
	register int pg;
	u_char intreg;
	extern Setpgmap();
	extern char start[], end[], etext[];

#ifndef SAS
	set_clock_ptes();
#endif !SAS
	Setpgmap((caddr_t)INTREG_ADDR,
	    PG_V|PG_KW|PGT_OBIO|btop(OBIO_INTREG_ADDR));

	set_clock_type(); 
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
	for (i = (int)start; i < (int)etext; i += NBPG) {
		pg = Getpgmap(i);
		Setpgmap(i, (pg & ~PG_PROT) | PG_KR);
	}

	cpu = getmachinetype();
	switch (cpu) {
		case SUN4_ARCH:			/* proto 1 */
		case CPU_SUN4_260:
		case CPU_SUN4_110:
	        case CPU_SUN4_330:
		case CPU_SUN4_470:
			break;
		default:
			printf("%s: UNKNOWN MACHINE TYPE IN ID PROM\n", myname);
			printf("%s: DEFAULTING MACHINE TYPE TO SUN4_260\n", 
				myname);
			cpu = CPU_SUN4_260;
			break;
	}
	if (cpu != CPU_SUN4_110)
		cache_on = getenablereg() &  ENA_CACHE;
	if (cache_on || (cpu == CPU_SUN4_110)) {
		cpudelay = 1;
	} else {
		cpudelay = 4;
		vac_init();			/* invalidate entire cache */
	}
}

asm("	.align  4			");
asm("_tcode:				");
asm("	sethi	%hi(_trap), %l3		");
asm("	or	%l3, %lo(_trap), %l3	");
asm("	jmp	%l3			");
asm("	mov	%psr, %l0		");

scbsync()
{
	kadbscbsync();
	scbsyncdone = 1;
}

kadbscbsync()
{
	register struct scb *tbr;
	register int otbr_pg;
	extern trapvec tcodee;
	extern trapvec tcode;
	extern struct scb *gettbr();

	tbr = gettbr();
	otbr_pg = Getpgmap(tbr);
	Setpgmap(tbr, (otbr_pg & ~PG_PROT) | PG_KW);

	tbr->user_trap[TRAPBRKNO-1] = tcode;
	tbr->user_trap[TRAPBRKNO] = tcode;
	Setpgmap(tbr, otbr_pg);
	if (scbstop) {
		/*
		 * Set things up so that we call the debugger.
		 * Use _setjmp()/_longjmp() with some adjustments
		 * to pull this non-local goto off correctly.
		 */
		scbstop = 0;
#ifdef notdef
		(void) _setjmp(state);
		state[JB_PC] = DVEC-8;			/* new pc value */
		state[JB_SP] = *(int *)state[JB_SP];	/* new sp value */
		_longjmp(state, 1);
		/*NOTREACHED*/
#endif
		asm("	t	126 /*TRAPBRKNO*/ -1	");
	}
}

/*
 * Set and/or clear the desired clock bits in the interrupt
 * register.  We have to be extremely careful that we do it
 * in such a manner that we don't get ourselves lost.
 */
set_clk_mode(on, off)
	u_char on, off;
{
	register u_char intreg, dummy;
	register int s;

/* XXX - need SUNRAY support */

#ifndef SAS
	/*
	 * make sure that we are only playing w/ 
	 * clock interrupt register bits
	 */
	on &= (IR_ENA_CLK14 | IR_ENA_CLK10);
	off &= (IR_ENA_CLK14 | IR_ENA_CLK10);

	s = splhi();
	/*
	 * Get a copy of current interrupt register,
	 * turning off any undesired bits (aka `off')
	 */
	intreg = *INTREG & ~(off | IR_ENA_INT);
	*INTREG &= ~IR_ENA_INT;

	/*
	 * Next we turns off the CLK10 and CLK14 bits to clear
	 * the flip-flops, then we disable clock interrupts.
	 * Now we can read the clock's interrupt register
	 * to clear any pending signals there.
	 */
	*INTREG &= ~(IR_ENA_CLK14 | IR_ENA_CLK10);
	if (clock_type == INTERSIL7170) {
	     CLOCK0->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_INTRENA);
	     dummy = CLOCK0->clk_intrreg;			/* clear clock */
	}
	else {
	     dummy = COUNTER->limit10;
	     dummy = COUNTER->limit14;
	}
		  
#ifdef lint
	dummy = dummy;
#endif

	/*
	 * Now we set all the desired bits
	 * in the interrupt register, then
	 * we turn the clock back on and
	 * finally we can enable all interrupts.
	 */
	*INTREG |= (intreg | on);			/* enable flip-flops */
	if (clock_type == INTERSIL7170) {
	     CLOCK0->clk_cmd = CLK_CMD_NORMAL;		/* enable clock intr */
	}
	*INTREG |= IR_ENA_INT;				/* enable interrupts */
	(void)splx(s);
#endif !SAS
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

/*
	showregs(ondebug_stack ? "unexpected fault" : "unexpected exception",
	    getsp());
*/

	/*
	 * If we are on the debugger stack and
	 * abort_jmp is set, do a longjmp to it.
	 */
	if (abort_jmp && ondebug_stack) {
		printf("abort jump\n");
		_longjmp(abort_jmp, 1);
		/*NOTREACHED*/
	}

	if (clock_peek) { /* we were peeking for clock type */
		jmp_buf_ptr sav = nofault;

		nofault = NULL;
		_longjmp(sav, 1);
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

	 printf("fault and call cmd %x %x %x %x\n",
		 trap,getsp(),trappc,trapnpc);
	 cmd();	/* error not resolved, ether debugger */
}
long trap_window[25];

showregs(str, locregs)
	register char *str;
	register struct allregs *locregs;
{
/* 	printf("g1-g7: %x, %x, %x, %x, %x, %x, %x\n",
	     rp->r_g1, rp->r_g2, rp->r_g3,
	     rp->r_g4, rp->r_g5, rp->r_g6, rp->r_g7);
printf("trap_window: wim=%x\n", trap_window[24]);
printf("o0-o7: %x, %x, %x, %x, %x, %x, %x, %x\n",
trap_window[0], trap_window[1], trap_window[2], trap_window[3],
trap_window[4], trap_window[5], trap_window[6], trap_window[7]);
printf("l0-l7: %x, %x, %x, %x, %x, %x, %x, %x\n",
trap_window[8], trap_window[9], trap_window[10], trap_window[11],
trap_window[12], trap_window[13], trap_window[14], trap_window[15]);
printf("i0-i7: %x, %x, %x, %x, %x, %x, %x, %x\n",
trap_window[16], trap_window[17], trap_window[18], trap_window[19],
trap_window[20], trap_window[21], trap_window[22], trap_window[23]);

#ifndef SAS
	DELAY(2000000);
#endif
	(void) splx(s);
 */
}

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
	int pg = 0, i;
	
	pg = Getpgmap((int)addr);
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

	if ((cpu != CPU_SUN4_110) && !(pg&PG_NC)){
	        vac_pageflush(addr);
	        if (btop(addr + sizeof (int) - 1) != btop(addr))
	                vac_pageflush(addr + sizeof (int) - 1);
	}

	if ((pg & PG_PROT) == PG_KR)
	        Setpgmap(addr, (pg & ~PG_PROT) | PG_KW);
	else if ((pg & PG_PROT) == PG_URKR)
	        Setpgmap(addr, (pg & ~PG_PROT) | PG_UW);
	/* otherwise it is already writeable */
	*addr = val;            /* should be prepared to catch a fault here? */
	/*
	 * Reset to page map to previous entry,
	 * but mark as modified
	 */
	if ((cpu != CPU_SUN4_110) && !(pg&PG_NC)){
	        vac_pageflush(addr);
	        if (btop(addr + sizeof (int) - 1) != btop(addr))
	                vac_pageflush(addr + sizeof (int) - 1);
	}
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

	if (curregs != 0) {
		printf("bad call to spawn\n");
		exit(1);
	}
	if (!_setjmp(mainregs)) {
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
		_longjmp(debugregs, 1);/* jump to new context */
		/*NOTREACHED*/
	}
}


doswitch()
{

	if (!_setjmp(curregs)) {
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

	/*
	 * See if the sp says that we are already on the debugger stack
	 */
	reg = (struct regs *)&regsave;
	addr = getsp();
	if (addr > (int)etext && addr < (int)estack) {
		printf("Already in debugger!\n");
		return;
	}
	if (cpu == CPU_SUN4_260)
		cache_on = getenablereg() &  ENA_CACHE;
	if (cache_on || (cpu == CPU_SUN4_110))
		cpudelay = 1;
	else
		cpudelay = 4;
	/*
	 * Set up for monitor's input routines to work.  This
	 * requires that we call the monitor's nmi handler on level14
	 * clock interrupts and that interrupts and level14 clock
	 * interrupt are enabled.
	 */
	if ((*INTREG & (IR_ENA_INT | IR_ENA_CLK14)) !=
	    (IR_ENA_INT | IR_ENA_CLK14)) {
		resetclk = 1;
		interreg = *INTREG;
		set_clk_mode(IR_ENA_CLK14, 0);
	}
	s = spl13();
	do {
		doswitch();
		if (dorun == 0)
			printf("cmd: nothing to do\n");
	} while (dorun == 0);
        if (resetclk) {
		set_clk_mode(0, IR_ENA_CLK14);
		*INTREG = interreg;
	}
	/* we don't need to splx since we are returning to the caller */
	/* and will reset his/her state */
}

void
vac_init()
{
	if (cpu == CPU_SUN4_470) {
		vac_linesize = VAC_LINESIZE_SUNRAY;
	} else {
		vac_linesize = VAC_LINESIZE_SUNRISE;
	}
	vac_nlines = VAC_SIZE / vac_linesize;
	vac_pglines = PAGESIZE / vac_linesize;
	vac_tagsinit();
}



set_clock_type() { 

	clock_peek = 1;
	if (Peekc((char *)&CLOCK0->clk_hsec) != -1) {
		clock_type = INTERSIL7170;	/* put back other  old pte */
		Setpgmap((caddr_t)COUNTER_ADDR, old_counter_pte);
	}
	else {
		clock_type = MOSTEK48T02;	/* put back other old pte */
		Setpgmap((caddr_t)CLOCK0_ADDR, old_clock0_pte);
	}
	clock_peek = 0;
}

/*
 * Return an available physical page number
 */
int
getapage()
{
	extern int lastpg;

	return (--lastpg);
}
 
#ifndef SAS
set_clock_ptes() {

     caddr_t i;
     int segm;

     /* copy old ptes, put the unused page pte back to what it was */

     old_clock0_pte = Getpgmap((caddr_t)CLOCK0_ADDR);
     old_counter_pte = Getpgmap((caddr_t)COUNTER_ADDR);
	
     i = (caddr_t) CLOCK0_ADDR;
     
     /* KLUDGE
	The last seg may not be mapped in by eporom. Only the sun4s
	with intel ethernet chip have the last smeg mapped in.
	For example, sun4_260 has the last smg mapped in while
	sun4_330 does not (because 4_330 uses LANCE ethernet chip).
	*/
     if ((segm = Getsegmap(i)) == (npmgrps-1)) {
	  segm = --lastpm;
	  Setsegmap((caddr_t)CLOCK0_ADDR, segm);
     }

     Setpgmap((caddr_t)CLOCK0_ADDR,			/* intersil clock */
	      PG_V|PG_KW|PGT_OBIO|btop(OBIO_CLOCK_ADDR));
     
     Setpgmap((caddr_t)COUNTER_ADDR,			/* mostek clock */
	      PG_V|PG_KW|PGT_OBIO|btop(OBIO_COUNTER_ADDR));
   }
#endif

/*
 * Call into the monitor (hopefully)
 */
montrap()
{
	(*romp->v_exit_to_mon)();
}
