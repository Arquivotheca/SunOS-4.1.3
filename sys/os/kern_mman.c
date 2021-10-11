/*	@(#)kern_mman.c 1.1 92/07/30 SMI; from UCB 1.12 83/07/01	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vnode.h>
#include <sys/vm.h>
#include <sys/file.h>
#include <sys/vadvise.h>
#include <sys/mman.h>

#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_dev.h>
#include <vm/seg_vn.h>
#include <machine/seg_kmem.h>

/*
 * Adjust UNIX break by incr, returning the old break value.
 */
sbrk()
{
	register struct a {
		int	incr;
	} *uap = (struct a *)u.u_ap;
	struct proc *p = u.u_procp;

	u.u_r.r_val1 = ctob(dptov(p, p->p_dsize));
	sbrk1(uap->incr / PAGESIZE);
}

/*
 * Common routine for sbrk() and brk().  Sets error in
 * u.u_error on error and adjusts p->p_dsize if successful.
 */
static
sbrk1(d)
	register int d;
{
	register struct proc *p = u.u_procp;

	if (d == 0)
		return;

	if (d < 0 && -d > p->p_dsize)
		d = -p->p_dsize;

	if ((u_int)ctob(p->p_dsize + d) >
	    (u_int)u.u_rlimit[RLIMIT_DATA].rlim_cur) {
		u.u_error = ENOMEM;
		return;
	}

	if (chksize((u_int)p->p_tsize, (u_int)p->p_dsize + d,
	    (u_int)p->p_ssize))
		return;

	if (d > 0) {
		/*
		 * Add new zfod mapping to extend UNIX data segment
		 */
		u.u_error = as_map(p->p_as, (addr_t)ctob(dptov(p, p->p_dsize)),
		    (u_int)ctob(d), segvn_create, zfod_argsp);
	} else if (d < 0) {
		/*
		 * Release mapping to shrink UNIX data segment
		 */
		(void) as_unmap(p->p_as, (addr_t)ctob(dptov(p, p->p_dsize + d)),
		    (u_int)ctob(-d));
	}

	if (u.u_error == 0)
		p->p_dsize += d;
}

sstk()
{
	register struct a {
		int	incr;
	} *uap = (struct a *)u.u_ap;

	if (sstk1(uap->incr / PAGESIZE) == 0)
		u.u_error = ENOMEM;
	u.u_r.r_val1 = USRSTACK - ctob(u.u_procp->p_ssize);
}

/*
 * Common routine for sstk() and grow().  Returns 0 on
 * failure and 1 on success after updating p->p_ssize.
 */
static int
sstk1(d)
	register int d;
{
	register struct proc *p = u.u_procp;

	if (d == 0)
		return (1);

	if (d < 0 && -d > p->p_ssize)
		d = -p->p_ssize;

	if (ctob(p->p_ssize + d) > u.u_rlimit[RLIMIT_STACK].rlim_cur)
		return (0);

	if (chksize((u_int)p->p_tsize, (u_int)p->p_dsize,
	    (u_int)p->p_ssize + d))
		return (0);

	if (d > 0) {
		if (as_map(p->p_as, (addr_t)(USRSTACK - ctob(p->p_ssize + d)),
		    (u_int)ctob(d), segvn_create, zfod_argsp) != 0)
			return (0);
	} else {
		/*
		 * Release mapping to shrink UNIX stack segment
		 */
		(void) as_unmap(p->p_as, (addr_t)(USRSTACK -
		    ctob(p->p_ssize)), (u_int)ctob(-d));
	}

	p->p_ssize += d;
	return (1);
}

getpagesize()
{

	u.u_r.r_val1 = PAGESIZE;
}

smmap()
{
	register struct a {
		caddr_t	addr;
		int	len;
		int	prot;
		int	flags;
		int	fd;
		off_t	pos;
	} *uap = (struct a *)u.u_ap;
	register struct vnode *vp;
	addr_t addr;
	register u_int len;
	struct as *as = u.u_procp->p_as;
	struct file *fp;
	u_int prot, maxprot;
	u_int type;
	int old_mmap;
	register u_int flags;

	flags = uap->flags;
	type = flags & MAP_TYPE;

	if ((flags & ~(MAP_SHARED | MAP_PRIVATE | MAP_FIXED | _MAP_NEW
	    | MAP_NORESERVE	/* not implemented, but don't fail here */
	    /* | MAP_RENAME */	/* not yet implemented, let user know */
	    )) != 0) {
		u.u_error = EINVAL;
		return;
	}

	if (type != MAP_PRIVATE && type != MAP_SHARED) {
		u.u_error = EINVAL;
		return;
	}

	len = uap->len;
	addr = uap->addr;

	/*
	 * Check for bad lengths and file position.
	 * We let the VOP_MAP routine check for negative offsets
	 * since on some vnode types this might be appropriate.
	 */
	if ((int)len <= 0 || (uap->pos & PAGEOFFSET) != 0) {
		u.u_error = EINVAL;
		return;
	}

	u.u_error = getvnodefp(uap->fd, &fp);
	if (u.u_error)
		return;

	vp = (struct vnode *)fp->f_data;

	maxprot = PROT_ALL;		/* start out allowing all accesses */
	prot = uap->prot | PROT_USER;

	if (type == MAP_SHARED && (fp->f_flag & FWRITE) == 0) {
		/* no write access allowed */
		maxprot &= ~PROT_WRITE;
	}

	/*
	 * XXX - Do we also adjust maxprot based on protections
	 * of the vnode?  E.g. if no execute permission is given
	 * on the vnode for the current user, maxprot probably
	 * should disallow PROT_EXEC also?  This is different
	 * from the write access as this would be a per vnode
	 * test as opposed to a per fd test for writability.
	 */

	/*
	 * Verify that the specified protections are not greater than
	 * the maximum allowable protections.  Also test to make sure
	 * that the file descriptor does allows for read access since
	 * "write only" mappings are hard to do since normally we do
	 * the read from the file before the page can be written.
	 */
	if (((maxprot & prot) != prot) || (fp->f_flag & FREAD) == 0) {
		u.u_error = EACCES;
		return;
	}

	/*
	 * See if this is an "old mmap call".  If so, remember this
	 * fact and convert the flags value given to mmap to indicate
	 * the specified address in the system call must be used.
	 * _MAP_NEW is turned set by all new uses of mmap.
	 */
	old_mmap = (flags & _MAP_NEW) == 0;
	if (old_mmap)
		flags |= MAP_FIXED;
	flags &= ~_MAP_NEW;

	/*
	 * If the user specified an address, do some simple checks here
	 */
	if ((flags & MAP_FIXED) != 0) {
		/*
		 * Use the user address.  First verify that
		 * the address to be used is page aligned.
		 * Then make some simple bounds checks.
		 */
		if (((int)addr & PAGEOFFSET) != 0) {
			u.u_error = EINVAL;
			return;
		}
		if (addr >= (caddr_t)USRSTACK ||
		    addr + len > (caddr_t)USRSTACK) {
			u.u_error = EINVAL;
			return;
		}
	}

	/*
	 * Ok, now let the vnode map routine do its thing to set things up.
	 */
	u.u_error = VOP_MAP(vp, uap->pos, as, &addr, len, prot, maxprot, flags,
	    fp->f_cred);
	if (u.u_error)
		return;

	/*
	 * Some old users of mmap simply check if mmap returned 0
	 * for success and thus think that mmap failed if it didn't
	 * return 0.  Thus we define mmap to only return the starting
	 * address if this is a "new" mmap usage.  Thus this hack
	 * is binary compatible, but not source compatible.  Users
	 * of mmap should now expect the return value of mmap to be the
	 * starting mapping address on success and (caddr_t)-1 on error.
	 */
	if (!old_mmap)
		u.u_r.r_val1 = (int)addr;	/* return starting address */
}

munmap()
{
	struct a {
		caddr_t	addr;
		int	len;
	} *uap = (struct a *)u.u_ap;
	register addr_t addr = uap->addr;
	register u_int len = uap->len;
	register addr_t eaddr = addr + len;
	struct proc *p = u.u_procp;
	addr_t ds, eds;
	addr_t mapend, mapstart;

	if (((int)addr & PAGEOFFSET) || (int)len <= 0) {
		u.u_error = EINVAL;
		return;
	}

	if (addr >= (caddr_t)USRSTACK || eaddr > (caddr_t)USRSTACK) {
		u.u_error = EINVAL;
		return;
	}

	if (as_unmap(u.u_procp->p_as, addr, len) != A_SUCCESS) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Now we have to check if we unmapped a region in the Unix
	 * "data" segment and if so we can remap to some anonymous
	 * memory now to try and reserve the swap space here.  If
	 * this fails because we are currently out of swap space, we
	 * don't return this error back to the user.  If the user
	 * later faults on this unmapped region in the Unix data
	 * segment, we will try again to set up the mapping at that time.
	 */
	ds = (addr_t)ctob(dptov(p, 0));
	eds = (addr_t)ctob(dptov(p, p->p_dsize));

	if (addr < eds) {
		mapstart = MAX(ds, addr);
		mapend = MIN(eaddr, eds);
		(void) as_map(p->p_as, mapstart,
		    (u_int)(mapend - mapstart), segvn_create, zfod_argsp);
	}
}

/*
 * "Memory-control" operations: things that affect the "VM cache".
 */
mctl()
{
	struct a {
		addr_t	addr;
		u_int	len;
		int	function;
		caddr_t	arg;
	} *uap = (struct a *)u.u_ap;
	register addr_t addr = uap->addr;
	register u_int len = uap->len;
	register caddr_t arg = uap->arg;
	enum as_res as_res = A_SUCCESS;
	faultcode_t fc;

	if ((uap->function == MC_LOCKAS) || (uap->function == MC_UNLOCKAS)) {
		if ((addr != 0) || (len != 0)) {
			u.u_error = EINVAL;
			return;
		}
	} else if (((int)addr & PAGEOFFSET) || ((int)len <= 0)) {
		u.u_error = EINVAL;
		return;
	}
	switch (uap->function) {
	case MC_SYNC:
		if ((int)arg & ~(MS_ASYNC|MS_INVALIDATE))
			u.u_error = EINVAL;
		else {
			as_res = as_ctl(u.u_procp->p_as, addr, len,
			    uap->function, arg);
			if (as_res == A_OPFAIL)
				u.u_error = EIO;
			if (as_res == A_RESOURCE)
				u.u_error = EPERM;
		}
		break;
	case MC_LOCK:
	case MC_UNLOCK:
	case MC_LOCKAS:
	case MC_UNLOCKAS:
		if (!suser())
			u.u_error = EPERM;
		else {
			as_res = as_ctl(u.u_procp->p_as, addr, len,
			    uap->function, arg);
			if (as_res == A_RESOURCE)
				u.u_error = EAGAIN;
			if (as_res == A_OPFAIL)
				u.u_error = EIO;
		}
		break;
	case MC_ADVISE:
		switch (arg) {
		case MADV_WILLNEED:
			fc = as_faulta(u.u_procp->p_as, addr, len);
			if (fc) {
				if (FC_CODE(fc) == FC_OBJERR)
					u.u_error = FC_ERRNO(fc);
				else
					u.u_error = EINVAL;
			}
			break;

		case MADV_DONTNEED:
			/*
			 * For now, don't need is turned into an as_ctl(MC_SYNC)
			 * operation flagged for async invalidate.
			 */
			as_res = as_ctl(u.u_procp->p_as, addr, len, MC_SYNC,
				(caddr_t)(MS_ASYNC | MS_INVALIDATE));
			if (as_res == A_OPFAIL)
				u.u_error = EIO;
			break;

		default:
			as_res = as_ctl(u.u_procp->p_as, addr, len,
				uap->function, arg);
			if (as_res == A_OPFAIL)
				u.u_error = EIO;
		}
		break;
	default:
		u.u_error = EINVAL;
		return;
	}
	if (as_res == A_BADADDR)
		u.u_error = ENOMEM;
}

omsync()
{
	struct a {
		caddr_t addr;
		int	len;
		int	flags;
	} *uap = (struct a *)u.u_ap;
	register addr_t addr = uap->addr;
	register u_int len = uap->len;
	register u_int flags = uap->flags;
	enum as_res as_res;

	if (((int)addr & PAGEOFFSET) || ((int)len <= 0) ||
	    ((int)flags & ~(MS_ASYNC|MS_INVALIDATE))) {
		u.u_error = EINVAL;
		return;
	}
	as_res = as_ctl(u.u_procp->p_as, addr, len, MC_SYNC, (caddr_t)flags);
	if (as_res == A_OPFAIL)
		u.u_error = EIO;
	if (as_res == A_RESOURCE)
		u.u_error = EPERM;
}

mprotect()
{
	struct a {
		caddr_t	addr;
		int	len;
		int	prot;
	} *uap = (struct a *)u.u_ap;
	register addr_t addr = uap->addr;
	register u_int len = uap->len;
	register u_int prot = uap->prot | PROT_USER;
	enum as_res as_res;

	if (((int)addr & PAGEOFFSET) || (int)len <= 0) {
		u.u_error = EINVAL;
		return;
	}

	as_res = as_setprot(u.u_procp->p_as, addr, len, prot);

	if (as_res == A_BADADDR)
		u.u_error = ENOMEM;
	if (as_res == A_OPFAIL)
		u.u_error = EACCES;
	if (as_res == A_RESOURCE)
		u.u_error = EAGAIN;
}

omadvise()
{
	register struct a {
		caddr_t	addr;
		int len;
		int behav;
	} *uap = (struct a *)u.u_ap;
	enum as_res as_res;
	faultcode_t fc;

	if (uap->len <= 0 || uap->addr >= (caddr_t)USRSTACK ||
	    uap->addr + uap->len > (caddr_t)USRSTACK) {
		u.u_error = EINVAL;
		return;
	}

	switch (uap->behav) {

	case MADV_NORMAL:
		/*
		 * Should inform segment driver to go back to read-ahead
		 * and no free behind the current address on faults.
		 */
		break;

	case MADV_RANDOM:
		/*
		 * Should inform segment driver to have no read-ahead
		 * and no free behind the current address on faults.
		 */
		break;

	case MADV_SEQUENTIAL:
		/*
		 * Should inform segment driver to do read-ahead and
		 * to free behind the current address on faults.
		 */
		break;

	case MADV_WILLNEED:
		fc = as_faulta(u.u_procp->p_as, uap->addr, (u_int)uap->len);
		if (fc) {
			if (FC_CODE(fc) == FC_OBJERR)
				u.u_error = FC_ERRNO(fc);
			else
				u.u_error = EINVAL;
		}
		break;

	case MADV_DONTNEED:
		/*
		 * For now, don't need is turned into an as_ctl(MC_SYNC)
		 * operation flagged for async invalidate.
		 */
		as_res = as_ctl(u.u_procp->p_as, uap->addr, (u_int)uap->len,
		    MC_SYNC, (caddr_t)(MS_ASYNC | MS_INVALIDATE));
		if (as_res == A_OPFAIL)
			u.u_error = EIO;
		break;

	default:
		/* unknown behavior value */
		u.u_error = EINVAL;
		break;

	}
}

mincore()
{
#define	MC_CACHE	128			/* internal result buffer */
#define	MC_QUANTUM	(MC_CACHE * PAGESIZE)	/* addresses covered in loop */
	struct a {
		caddr_t addr;
		int	len;
		char	*vec;
	} *uap = (struct a *)u.u_ap;
	register addr_t addr = uap->addr;
	register addr_t ea;			/* end address of loop */
	u_int rl;				/* inner result length */
	char vec[MC_CACHE];			/* local vector cache */
	enum as_res as_res;

	/*
	 * Validate form of address parameters.
	 */
	if (((int)addr & PAGEOFFSET) || (uap->len <= 0)) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Loop over subranges of interval [addr : addr + len), recovering
	 * results internally and then copying them out to caller.  Subrange
	 * is based on the size of MC_CACHE, defined above.
	 */
	for (ea = addr + uap->len; addr < ea; addr += MC_QUANTUM) {
		as_res = as_incore(u.u_procp->p_as, addr,
		    (u_int)MIN(MC_QUANTUM, ea - addr), vec, &rl);
		if (rl != 0) {
			rl = (rl + PAGESIZE - 1) / PAGESIZE;
			if ((u.u_error = copyout(vec, uap->vec, rl)) != 0)
				return;
			uap->vec += rl;
		}
		if (as_res != A_SUCCESS) {
			u.u_error = ENOMEM;
			return;
		}
	}
}

/*
 * Adjust UNIX break to nsiz.
 */
brk()
{
	struct a {
		char	*nsiz;
	} *uap = (struct a *)u.u_ap;
	register int n, d;

	n = btoc(uap->nsiz) - dptov(u.u_procp, 0);
	if (n < 0)
		n = 0;
	d = clrnd(n - u.u_procp->p_dsize);
	sbrk1(d);
}

/*
 * Grow the stack to include the sp returning true if successful.
 * Used by trap to extend the stack segment on data fault.
 */
grow(sp)
	register int sp;
{
	register struct proc *p = u.u_procp;

	if (sp >= USRSTACK - ctob(p->p_ssize))
		return (0);
	return (sstk1(((USRSTACK - sp) / PAGESIZE) - p->p_ssize + SINCR));
}

/* BEGIN DEFUNCT */
ovadvise()
{
	register struct a {
		int	anom;
	} *uap = (struct a *)u.u_ap;
	register struct proc *rp = u.u_procp;

	rp->p_flag &= ~(SSEQL|SUANOM);

	/* N.B. - the ANOM and SEQL flags do not do anything anymore */
	switch (uap->anom) {
	case VA_ANOM:
		rp->p_flag |= SUANOM;
		break;

	case VA_SEQL:
		rp->p_flag |= SSEQL;
		break;

	case VA_FLUSH:
		if (rp->p_as)
			hat_free(rp->p_as);
		break;
	}
}
/* END DEFUNCT */
