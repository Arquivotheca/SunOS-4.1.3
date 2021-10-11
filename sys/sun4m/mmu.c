#ident "@(#)mmu.c	1.1 92/07/30	SMI"

/*
 * Copyright 1987-1990 Sun Microsystems, Inc.
 */

/*
 * VM - Sun-4 low-level routines.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/vm.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>

#include <vm/page.h>
#include <vm/seg.h>
#include <vm/as.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/vm_hat.h>
#include <machine/seg_kmem.h>

#include <sundev/mbvar.h>

extern	struct ctx *ctxs;

extern unsigned	pte_read();

struct pte mmu_pteinvalid =  { 0 /* page number */, 0 /* cacheable */,
	0 /* modified */, 0 /* referenced */, 0 /* Access permissions */,
	MMU_ET_INVALID /* pte type */ };

/* Only here for compatibility */
void
mmu_setpte(base, pte)
	addr_t base;
	struct pte pte;
{
	/*
	 * Make sure user windows are flushed before
	 * possibly taking a mapping away.
	 */
	if (!pte_valid(&pte))
		flush_user_windows();
	hat_pteload(&kseg, base, (struct page *)NULL,
		&pte, PTELD_LOCK | PTELD_NOSYNC);
	mmu_flushpage(base);	/* flush translation from all MMUs */
}

/* Only for compatibility, but
 * Called during panic operations.
 *
 * XXX - assumes PTEs and unsigned ints are same size
 */
void
mmu_getpte(base, ppte)
	addr_t base;
	struct pte *ppte;
{
	struct proc *p;
	struct as *as;
	struct ctx *ctx;
	unsigned cno, scn;
	extern int noproc;

	/*
	 * If the address is in kernel space
         * pr if there is no process
         * or if the process has no address space
         * then just use a mmu probe.
	 */
	if ((base >= (addr_t)KERNELBASE) ||
            (noproc) ||
            ((p = u.u_procp) == NULL) ||
            ((as = p->p_as) == NULL)) {
		*(u_int *)ppte = mmu_probe(base);
		return;
	}

	if ((ctx = as->a_hat.hat_ctx) != NULL) {
		cno = ctx->c_num;
		scn = mmu_getctx();
		if (cno == scn) {
			/*
                         * If the current context is
                         * assigned to this process,
                         * we can just probe it.
                         */
			*(unsigned *)ppte = mmu_probe(base);

			return;
		}
#ifndef	MULTIPROCESSOR
 		/*
                 * If the process has a context,
                 * we can borrow the context
                 * and probe the mmu.
                 */
		mmu_setctx(cno);
		*(unsigned *)ppte = mmu_probe(base);
		mmu_setctx(scn);
		return;		
#endif	MULTIPROCESSOR
	}

	/*
         * No shortcuts, darn it.
         * Dive into the hat layer
	 * so software will have to do the table-walk.
	 */

	*(unsigned *)ppte = pte_read(hat_ptefind(p->p_as, base));
}

void
mmu_getkpte(base, ppte)
	addr_t base;
	struct pte *ppte;
{

	/*
 	 * XXX - assumes PTEs and unsigned ints are same size
	 */

	*(unsigned *)ppte = mmu_probe(base); 
}
