#ifndef lint
static	char sccsid[] = "@(#)vm_machdep.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * UNIX machine dependent virtual memory support.
 *
 * Context, segment, and page map support for the
 * Sun-3 now shared between vm_hat.c and mmu.c.
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

/*
 * Handle a pagefault.
 */
faultcode_t
pagefault(addr, rw, iskernel)
	int addr;
	enum seg_rw rw;
	int iskernel;
{
	register addr_t adr;
	register struct as *as;
	register struct proc *p;
	faultcode_t res;
	struct pte pte;
	addr_t base;
	u_int len;
	int err;

	/*
	 * Count faults.
	 */
	cnt.v_faults++;

	if ((addr & ~ADDRESS_MASK) != 0 &&
	    (addr & ~ADDRESS_MASK) != ~ADDRESS_MASK)
		return (FC_NOMAP);

	adr = (addr_t)(addr & ADDRESS_MASK);

	if (iskernel)
		as = &kas;
	else {
		p = u.u_procp;
		as = p->p_as;
	}

	/*
	 * If in kernel context and this is a user fault, then ensure
	 * hardware is set up.  If this results in a valid page and this
	 * is not a copy-on-write fault, then return now with success.
	 */
	if (iskernel == 0 && mmu_getctx() == kctx) {
		if (as == NULL)
			panic("pagefault");
		hat_setup(as);
		mmu_getpte(adr, &pte);
		if (pte_valid(&pte) && !(rw == S_WRITE && pte.pg_prot == UR))
			return (0);
	} else
		mmu_getpte(adr, &pte);

	if (iskernel == 0 && pte_valid(&pte) &&
	    (pte.pg_prot == KW || pte.pg_prot == KR))
		return (FC_NOMAP);		/* paranoid */

	/*
	 * Dispatch pagefault.
	 */
	res = as_fault(as, adr, 1, (pte_valid(&pte) && (rw == S_WRITE)
	    && ((pte.pg_prot == UR) || pte.pg_prot == KR)) ?
	    F_PROT : F_INVAL, rw);

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
	if (adr < base || adr >= base + len) {
		len = ptob(p->p_ssize);			/* data seg? */
		base = (addr_t)USRSTACK - len;
		if (adr < base || adr >= (addr_t)USRSTACK) {	/* stack seg? */
			/* not in either UNIX data or stack segments */
			return (FC_NOMAP);
		}
	}

	if (as_hole(as, NBPG, &base, &len, AH_CONTAIN, adr) != A_SUCCESS) {
		/*
		 * Since we already got an FC_NOMAP return code from
		 * as_fault, there must be a hole at `adr'.  Therefore,
		 * as_hole should never fail here.
		 */
		panic("pagefault as_hole");
	}

	err = as_map(as, base, len, segvn_create, zfod_argsp);
	if (err)
		return (FC_MAKE_ERR(err));

	return (as_fault(as, adr, 1, F_INVAL, rw));
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
/*ARGSUSED*/
valid_va_range(basep, lenp, minlen, dir)
	addr_t *basep;
	u_int *lenp;
	register u_int minlen;
	int dir;
{
	register addr_t hi, lo;

	lo = *basep;
	hi = lo + *lenp;
	if (lo >= (addr_t)CTXSIZE)
		return (0);
	if (hi > (addr_t)CTXSIZE) {
		if ((addr_t)CTXSIZE - lo < minlen)
			return (0);
		*lenp = (addr_t)CTXSIZE - lo;
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
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of PAGESIZE.
 */
pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	register struct pte *fpte;

	if ((size % PAGESIZE) != 0)
		panic("pagemove");

	for (fpte = &Sysmap[mmu_btop((int)from - SYSBASE)];
	  size > 0;
	  size -= PAGESIZE, from += PAGESIZE, to += PAGESIZE, fpte += CLSIZE) {
		u_int tpf;

		/*
		 * We save the old page frame info and unmap
		 * the old address "from" before we set up the
		 * new mapping to new address "to" to avoid
		 * VAC conflicts
		 */
		tpf = MAKE_PFNUM(fpte);
		segkmem_mapout(&kseg, (addr_t)from, PAGESIZE);
		segkmem_mapin(&kseg, (addr_t)to, PAGESIZE,
		  PROT_READ | PROT_WRITE, tpf, 0);
	}
}

/*
 * Defines used for running (old) Sun-2 format executables.
 */
#define	ONBPG		0x800		/* old page size */
#define	ODATA_ALIGN	0x8000		/* old data alignment */
#define	OUSRTEXT	0x8000		/* old USRTEXT value */

/*
 * Routine used to check to see if an a.out can be executed
 * by the current machine/architecture.
 */
chkaout()
{

	if ((u.u_exdata.ux_mach == M_68010) ||
	    (u.u_exdata.ux_mach == M_68020) ||
	    (u.u_exdata.ux_mach == M_OLDSUN2))
		return (0);
	else
		return (ENOEXEC);
}

/*
 * The following functions return information about an a.out
 * which is used when a program is executed.
 */

/*
 * Return the size of the text segment adjusted for the type of a.out.
 * An old a.out will eventually become OMAGIC so it has no text size.
 */
int
getts()
{

	if (u.u_exdata.ux_mach == M_OLDSUN2)
		return (0);
	else
		return (clrnd(btoc(u.u_exdata.ux_tsize)));
}

/*
 * Return the size of the data segment depending on the type of a.out.
 * For the case of an old a.out we need to allow for the old data
 * alignment and the fact that the text segment starts at OUSRTEXT
 * and not USRTEXT.  To do this we calculate the end of the text
 * segment in the old system and subtract off the new starting address
 * (USRTEXT) before adding in the size of the data and bss segments.
 */
int
getds()
{

	if (u.u_exdata.ux_mach == M_OLDSUN2)
		return (clrnd(btoc(
		    roundup(OUSRTEXT + u.u_exdata.ux_tsize, ODATA_ALIGN)
		      - USRTEXT + u.u_exdata.ux_dsize + u.u_exdata.ux_bsize)));
	else
		return (clrnd(btoc(u.u_exdata.ux_dsize + u.u_exdata.ux_bsize)));
}

/*
 * Return the load memory address for the data segment.
 */
caddr_t
getdmem()
{
	u_int usrtxt;
	u_int data_align;

	if (u.u_exdata.ux_mach == M_OLDSUN2) {
		usrtxt = OUSRTEXT;
		data_align = ODATA_ALIGN;
	} else {
		usrtxt = USRTEXT;
		data_align = DATA_ALIGN;
	}

	if (u.u_exdata.ux_tsize)
		return ((caddr_t)(roundup(usrtxt + u.u_exdata.ux_tsize,
		    data_align)));
	else
		return ((caddr_t)usrtxt);
}

/*
 * Return the starting disk address for the data segment.
 */
getdfile()
{

	if (u.u_exdata.ux_mag == ZMAGIC)
		return (((u.u_exdata.ux_mach == M_OLDSUN2) ? ONBPG : 0) +
		    u.u_exdata.ux_tsize);
	else
		return (sizeof (u.u_exdata) + u.u_exdata.ux_tsize);
}

/*
 * Return the load memory address for the text segment.
 */
caddr_t
gettmem()
{

	if (u.u_exdata.ux_mach == M_OLDSUN2)
		return ((caddr_t)OUSRTEXT);
	else
		return ((caddr_t)USRTEXT);
}

/*
 * Return the file byte offset for the text segment.
 */
gettfile()
{

	if (u.u_exdata.ux_mag == ZMAGIC)
		return ((u.u_exdata.ux_mach == M_OLDSUN2) ? ONBPG : 0);
	else
		return (sizeof (u.u_exdata));
}
