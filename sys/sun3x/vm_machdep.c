#ifndef lint
static	char sccsid[] = "@(#)vm_machdep.c 1.1 92/07/30";
#endif
/* syncd to sun3/vm_machdep.c 1.48 */
/* syncd to sun3/vm_machdep.c 1.52 (4.1) */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * UNIX machine dependent virtual memory support.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>
#include <sys/mman.h>
#include <vm/page.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>

#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/seg_kmem.h>
#include <sys/vnode.h>

/*
 * Handle a pagefault.
 */
faultcode_t
pagefault(addr, rw, iskernel, type)
	addr_t	addr;
	enum	seg_rw rw;
	int	iskernel;
	u_int	type;
{
	register addr_t adr = (addr_t)addr;
	register struct as *as;
	register struct proc *p;
	faultcode_t res;
	addr_t	base;
	u_int	len;
	int	err;

	/*
	 * Count faults.
	 */
	cnt.v_faults++;
	if (iskernel)
		as = &kas;
	else {
		p = u.u_procp;
		as = p->p_as;
	}
	if (as == NULL)
		panic("pagefault -- no as");
	/*
	 * Stop access violations cold.
	 */
	if (type == MMUE_SUPER)
		return (FC_NOMAP);
	/*
	 * Dispatch pagefault.
	 */
	res = as_fault(as, adr, 1,
	    type == MMUE_INVALID ? F_INVAL : F_PROT, rw);
	/*
	 * If this isn't a potential unmapped hole in the user's
	 * UNIX data or stack segments, just return status info.
	 */
	if (!(res == FC_NOMAP && iskernel == 0))
		return (res);
	/*
	 * Check to see if we happened to fault on a currently unmapped
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
 * range which is just below the current stack limit.
 * addrp is a value/result parameter.
 * On input it is a hint from the user to be used in a completely
 * machine dependent fashion.  We decide to completely ignore this hint.
 *
 * On output it is NULL if no address can be found in the current
 * processes address space or else an address that is currently
 * not mapped for len bytes with a page of red zone on either side.
 * If align is true, then the selected address will obey the alignment
 * constraints of a vac machine based on the given off value; not
 * applicable here.
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
	/*
	 * Look for a large enough hole starting below the stack limit.
	 * After finding it, use the upper part.
	 */
	if (as_hole(as, len, &base, &slen, AH_HI) == A_SUCCESS)
		*addrp = (addr_t)(base + slen - len);
	else
		*addrp = NULL;		/* no more virtual space */
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
	/*
	 * If hi rolled over the top, try cutting back.
	 */
	if (hi < lo) {

printf("valid_va_range: hi < lo, lo=0x%x, hi=0x%x\n", lo, hi);

		if (0 - (u_int)lo + (u_int)hi < minlen)
			return (0);
/* ******************
		if (0 - (u_int)lo < minlen)
			return (0);
		*lenp = 0 - (u_int)lo;
************* */
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

	if (size % PAGESIZE)
		panic("pagemove -- bad size");
	vac_flush();
	for (fpte = &Sysmap[mmu_btop((u_int)from - SYSBASE)];
	    size > 0;
	    size -= PAGESIZE, from += PAGESIZE,
	    to += PAGESIZE, fpte += CLSIZE) {
		segkmem_mapin(&kseg, (addr_t)to, PAGESIZE,
		    PROT_READ | PROT_WRITE, MAKE_PFNUM(fpte), 0);
		segkmem_mapout(&kseg, (addr_t)from, PAGESIZE);
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
#ifdef SUN3X_80
/*
 * page_giveup()
 * Give up the page 'addr' is from. Remove all association with
 * vnode.
 * Do a retry to see if the parity error occurs again. If there is one
 * then remove page from further use.
 */
page_giveup(addr, pte)
addr_t addr;
struct pte *pte;
{
	register struct proc *p;
	struct segvn_data *svd;
	register struct as *as;
	register addr_t raddr;                  /* rounded addr counter */
	struct seg *seg;
	struct page  *pp;
	struct vnode *vp;
	u_int off;
	int remove_page;
      
	p = u.u_procp;
	as = p->p_as;

	raddr = (addr_t)((u_int)addr & PAGEMASK);
	seg = as_segat(as, raddr);

	svd = (struct segvn_data *)seg->s_data;
	vp = svd->vp;
	off = svd->offset + (raddr - seg->s_base);

	if (vp == (struct vnode *)NULL) {
		return (0);
	}

	pp = page_lookup(vp, off);
	if (pp == (struct page *)NULL) {
		return(0);
	}
	remove_page = retry(addr, pte);
	hat_pageunload(pp);
	page_hashout(pp);
	if (remove_page == -1) {	/* retry failed throw away the page */
		pp->p_lock = 1;
	}
	return(1);
}

#endif SUN3X_80
