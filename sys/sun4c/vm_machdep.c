#ident "@(#)vm_machdep.c 1.1 92/07/30 SMI"

/*
 * Copyright 1988-1989 Sun Microsystems, Inc.
 */

/*
 * UNIX machine dependent virtual memory support.
 *
 * Context, segment, and page map support for the
 * Sun-4 now shared between vm_hat.c and mmu.c.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/mman.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>

#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/seg_kmem.h>
#include <vm/page.h>

/*
 * Handle a pagefault.
 */
faultcode_t
pagefault(addr, type, rw, iskernel)
	register addr_t addr;
	register enum fault_type type;
	register enum seg_rw rw;
	register int iskernel;
{
	register struct as *as;
	register struct proc *p;
	register faultcode_t res;
	struct pte pte;
	addr_t base;
	u_int len;
	int err;

	/*
	 * Count faults.
	 */
	cnt.v_faults++;

	if (!good_addr(addr))
		return (FC_NOMAP);

	if (iskernel) {
		as = &kas;
	} else {
		p = u.u_procp;
		as = p->p_as;
	}

	/*
	 * If in kernel context and this is a user fault, then ensure
	 * hardware is set up. If this results in a valid page and this
	 * is not a copy-on-write fault, then return now with success.
	 */
	if (iskernel == 0 && mmu_getctx() == kctx) {
		if (as == NULL || type == F_PROT)
			panic("pagefault");
		hat_setup(as);
		mmu_getpte(addr, &pte);
		if (pte_valid(&pte) && !(rw == S_WRITE && pte.pg_prot == UR))
			return (0);
	} else {
		mmu_getpte(addr, &pte);
	}

        /*
         * Check if the HAT layer can resolve the fault by loading
         * a SW page table to MMU.
         */
        if (type == F_INVAL) {
                if (hat_fault(addr) == 0)
                        return (0);
                else
                        mmu_getpte(addr, &pte);
        }

	if (iskernel == 0 && pte_valid(&pte) &&
	    (pte.pg_prot == KW || pte.pg_prot == KR))
		return (FC_NOMAP);		/* paranoid */

	/*
	 * Dispatch pagefault.
	 */
	res = as_fault(as, addr, 1, type, rw);

	/*
	 * If this isn't a potential unmapped hole in the user's
	 * UNIX data or stack segments, just return status info.
	 */
	if (!(res == FC_NOMAP && iskernel == 0))
		return (res);

	/*
	 * Check to see if we happened to faulted on a currently unmapped
	 * part of the UNIX data or stack segments.  If so, create a zfod
	 * mapping there and then try calling the fault routine again.
	 */
	base = (addr_t)ptob(dptov(p, 0));
	len = ptob(p->p_dsize);
	if (addr < base || addr >= base + len) {		/* data seg? */
		len = ptob(p->p_ssize);
		base = (addr_t)USRSTACK - len;
		if (addr < base || addr >= (addr_t)USRSTACK) {	/* stack seg? */
			/* not in either UNIX data or stack segments */
			return (FC_NOMAP);
		}
	}

	if (as_hole(as, NBPG, &base, &len, AH_CONTAIN, addr) != A_SUCCESS) {
		/*
		 * Since we already got an FC_NOMAP return code from
		 * as_fault, there must be a hole at `addr'.  Therefore,
		 * as_hole should never fail here.
		 */
		panic("pagefault as_hole");
	}

	err = as_map(as, base, len, segvn_create, zfod_argsp);
	if (err)
		return (FC_MAKE_ERR(err));

	return (as_fault(as, addr, 1, F_INVAL, rw));
}

/*
 * map_addr() is the routine called when the system is to
 * chose an address for the user.  We will pick an address
 * range which is just below the current stack limit.  The
 * algorithm used for cache consistency on machines with virtual
 * address caches is such that offset 0 in the vnode is always
 * on a shm_alignment'ed aligned address.  Unfortunately, this
 * means that vnodes which are demand paged will not be mapped
 * cache consistently with the executable images.  When the
 * cache alignment for a given object is inconsistent, the
 * lower level code must manage the translations so that this
 * is not seen here (at the cost of efficiency, of course).
 *
 * addrp is a value/result parameter.
 *	On input it is a hint from the user to be used in a completely
 *	machine dependent fashion.  We decide to completely ignore this hint.
 *
 *	On output it is NULL if no address can be found in the current
 *	processes address space or else an address that is currently
 *	not mapped for len bytes with a page of red zone on either side.
 *	If align is true, then the selected address will obey the alignment
 *	constraints of a vac machine based on the given off value.
 */
/*ARGSUSED*/
map_addr(addrp, len, off, align)
	addr_t *addrp;
	register u_int len;
	off_t off;
	int align;
{
	register struct proc *p = u.u_procp;
	register struct as *as = p->p_as;
	addr_t base;
	u_int slen;

	base = (addr_t)ctob(dptov(p, u.u_dsize));
	slen = (addr_t)USRSTACK - u.u_rlimit[RLIMIT_STACK].rlim_cur - base;
	len = (len + PAGEOFFSET) & PAGEMASK;
	len += 2 * PAGESIZE;			/* redzone for each side */

#ifdef VAC
	if (vac && align)
		len += 2 * shm_alignment;
#endif VAC

	/*
	 * Look for a large enough hole starting below the stack limit.
	 * After finding it, use the upper part.
	 */
	if (as_hole(as, len, &base, &slen, AH_HI) == A_SUCCESS) {
		register u_int addr;

		addr = (u_int)base + slen - len + PAGESIZE;
#ifdef VAC
		if (vac && align) {
			/*
			 * Adjust up to the next shm_alignment boundary, then
			 * up by the offset in shm_alignment from there.
			 */
			addr = roundup(addr, shm_alignment);
			addr += off & (shm_alignment - 1);
		}
#endif VAC
		*addrp = (addr_t)addr;		/* set result parameter */
	} else {
		*addrp = NULL;			/* no more virtual space */
	}
}

/*
 * Determine whether [base, base+len) contains a mapable range of
 * addresses at least minlen long. base and len are adjusted if
 * required to provide a mapable range.
 */
int
valid_va_range(basep, lenp, minlen, dir)
	register addr_t *basep;
	register u_int *lenp;
	register u_int minlen;
	register int dir;
{
	register addr_t hi, lo;

	lo = *basep;
	hi = lo + *lenp;
	if (lo < hole_start && hi >= hole_end) {
		if (dir == AH_LO) {
			if (hole_start - lo >= minlen)
				hi = hole_start;
			else if (hi - hole_end >= minlen)
				lo = hole_end;
			else
				return (0);
		} else {
			if (hi - hole_end >= minlen)
				lo = hole_end;
			else if (hole_start - lo >= minlen)
				hi = hole_start;
			else
				return (0);
		}
		*basep = lo;
		*lenp = hi - lo;
	} else if (hi - lo < minlen)
		return (0);
	return (1);
}

/*
 * Check for valid program size
 */
/*ARGSUSED*/
chksize(ts, ds, ss)
	unsigned ts, ds, ss;
{


	/*
	 * Most of the checking is done by the as layer routines, we
	 * simply check the resource limits for data and stack here.
	 */
	if (ctob(ds) > u.u_rlimit[RLIMIT_DATA].rlim_cur ||
	    ctob(ss) > u.u_rlimit[RLIMIT_STACK].rlim_cur) {
		u.u_error = ENOMEM;
		return (1);
	}
	return (0);
}

/*
 * Move pages from one kernel virtual address to another.
 * As these are buffer addresses, they are both assumed to
 * live in the Bufptes, and size must be a multiple of PAGESIZE.
 */

pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	register struct pte *fpte;

	if ((size % PAGESIZE) != 0)
		panic("pagemove");

	if (!((from >= (caddr_t)HEAPBASE) && (from < (caddr_t)BUFLIMIT) &&
	    (to >= (caddr_t)HEAPBASE) && (to < (caddr_t)BUFLIMIT)))
	  panic("pagemove: bad buffer address");

	for (fpte = &Bufptes[mmu_btop((int)from - BUFBASE)]; size > 0;
			size -= PAGESIZE, from += PAGESIZE,
			to += PAGESIZE, fpte += CLSIZE) {
		u_int tpf;

		/*
		 * We save the old page frame info and unmap
		 * the old address "from" before we set up the
		 * new mapping to new address "to" to avoid
		 * VAC conflicts
		 */
		tpf = MAKE_PFNUM(fpte);
		segkmem_mapout(&bufheapseg, (addr_t)from, PAGESIZE);
		segkmem_mapin(&bufheapseg, (addr_t)to, PAGESIZE,
			PROT_READ | PROT_WRITE, tpf, 0);
	}
}

#define	ONBPG 		2048	/* old page size */
#define	ONBSG 		32768	/* old segment size */

/*
 * Routine used to check to see if an a.out can be executed
 * by the current machine/architecture.
 */
chkaout()
{

	if (u.u_exdata.ux_mach == M_SPARC)
		return (0);
	else
		return (ENOEXEC);
}

/*
 * The following functions return information about an a.out
 * which is used when a program is executed.
 */

/*
 * Return the size of the text segment.
 */
int
getts()
{

	return (clrnd(btoc(u.u_exdata.ux_tsize)));
}

/*
 * Return the size of the data segment.
 */
int
getds()
{

	return (clrnd(btoc(u.u_exdata.ux_dsize + u.u_exdata.ux_bsize)));
}

/*
 * Return the load memory address for the data segment.
 */
caddr_t
getdmem()
{

	if (u.u_exdata.ux_tsize)
		return ((caddr_t)(roundup(USRTEXT+u.u_exdata.ux_tsize,
			DATA_ALIGN)));
	else
		return ((caddr_t)USRTEXT);
}

/*
 * Return the starting disk address for the data segment.
 */
getdfile()
{

	if (u.u_exdata.ux_mag == ZMAGIC)
		return (u.u_exdata.ux_tsize);
	else
		return (sizeof (u.u_exdata) + u.u_exdata.ux_tsize);
}

/*
 * Return the load memory address for the text segment.
 */
caddr_t
gettmem()
{

	return ((caddr_t)USRTEXT);
}

/*
 * Return the file byte offset for the text segment.
 */
gettfile()
{

	if (u.u_exdata.ux_mag == ZMAGIC)
		return (0);
	else
		return (sizeof (u.u_exdata));
}

/*
 * Give up this page, which contains the address that caused the
 * parity error.  While we are at it, see if the parity error is permanent.
 * If it is, then lock the page so that it will never be used again (at
 * least until we reboot).
 */
page_giveup(addr, ppte, pp)
	addr_t		 addr;
	struct pte	*ppte;
	struct page	*pp;
{
	int remove_page;

	/*
	 * Check to see if we get another parity error when we access
	 * this address again.
	 */
	remove_page = parerr_reset(addr, ppte);

	hat_pageunload(pp);
	page_abort(pp);

	/*
	 * If retry failed (returned -1), then lock the page to keep it
	 * from being used again.  This will let the machine stay up
	 * for now, without the parity error affecting future processes.
	 * XXX do we need to splvm() between the abort and the lock?
	 */
	if (remove_page == -1) {
		if (pp->p_free)
			page_unfree(pp);
		pp->p_lock = 1;
	}
	printf("page %x %s service.\n",
		ptob(ppte->pg_pfnum),
		remove_page == -1 ? "marked out of" :
			"back in");
}

static struct page *page_freelist;

struct page **
get_page_freelist()
{
	return(&page_freelist);
}

page_freelist_sub(pp)
	register struct page *pp;
{
	page_sub(&page_freelist, pp);
}

page_freelist_add(pp)
	register struct page *pp;
{
	page_add(&page_freelist, pp);
}
