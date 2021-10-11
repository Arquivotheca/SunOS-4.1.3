/*	@(#)init_processor.c 1.1 92/07/30 SMI; from UCB 4.53 83/07/01	*/

#include <sys/param.h>

#ifdef MULTIPROCESSOR

#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/vfs.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/session.h>
#include <sys/buf.h>
#include <sys/vm.h>
#include <sys/clist.h>
#include <sys/vnode.h>
#ifdef QUOTA
#include <ufs/quota.h>
#endif
#ifdef TRACE
#include <sys/trace.h>
#endif
#include <sys/systm.h>
#include <sys/bootconf.h>
#include <machine/mmu.h>
#include <machine/reg.h>
#include <machine/pte.h>
#ifdef sun386
#include <machine/cpu.h>
#endif

#include <vm/swap.h>

/*
 * Initialization code for 
 * processors other than processor 0

 * Functions:
 * Pull in a process from the runq
 * and start executing it.
 * If runq is empty fall into idle.
 * 
 */

#ifdef	VAC
extern int	vac;
extern int	cache;
extern int	use_cache;
#endif

#include "percpu.h"

#define	CCI		ccheck(__FILE__, __LINE__)

/*
 * startup code for processors other than the first. The system is
 * initialized, so we just need to initialize our private data and
 * private hardware, and inform the system that we are available.
 */
pxmain()
{
	extern int procset;
	extern int ncpu;
									CCI;
#ifdef VAC
/*
 * If the other processor is running cache-on, we need to turn
 * our cache on before we start working with any data that is
 * cached and shared between units.
 */
	/* FIXME(VIKING): The is for bringup debugging purpose.  
	 * Once it's done, viking initialization should be done by 
	 * the PROM, then we don't need this anymore.
	 */
	if (cache) {
		extern void cache_on();
		vac_init();		/* initialize cache */

	/* no need to call cache_on() if VIKING because this function is
	 * included in the vac_init() routine.
	 */
		if (use_cache && vac)
			cache_on();
	}
#endif VAC

#ifdef	TLBLOCK_ATOMS
	atom_tlblock();
#endif	TLBLOCK_ATOMS

	setcpudelay();

	klock_enter();
									CCI;
	procset |= 1<<cpuid;
	ncpu ++;

	idlework();
	/* NOTREACHED */
}
#endif MULTIPROCESSOR
