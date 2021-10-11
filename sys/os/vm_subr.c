#ident	"@(#)vm_subr.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mman.h>
#include <sys/vnode.h>
#include <sys/vm.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/page.h>

#include <machine/pte.h>
#include <machine/seg_kmem.h>

/*
 * rout is the name of the routine where we ran into problems.
 */
swkill(p, rout)
	struct proc *p;
	char *rout;
{

	printf("pid %d: killed due to swap problems in %s\n", p->p_pid, rout);
	uprintf("sorry, pid %d was killed due to swap problems in %s\n",
	    p->p_pid, rout);
	/*
	 * To be sure no looping (e.g. in vmsched trying to
	 * swap out) mark process locked in core (as though
	 * done by user) after killing it so noone will try
	 * to swap it out.
	 */
	psignal(p, SIGKILL);
	p->p_flag |= SULOCK;
}

/*
 * Don't increase beyond NDMAXIO (this)
 * for machines that cannot guarantee the
 * integrity of dma xfers this size.
 *
 * Also- check <machine/machdep.c> to
 * keep MBMAXPHYS in sync w/this.
 *
 */

#define	MAXPHYS	(63 * 1024)

int	maxphys = MAXPHYS;
void
minphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > maxphys)
		bp->b_bcount = maxphys;
}

static caddr_t physio_bufs;

/*
 * Raw I/O. The arguments are
 *	The strategy routine for the device
 *	A buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose
 *	The device number
 *	Read/write flag
 * Essentially all the work is computing physical addresses and
 * validating them.
 * If the user has the proper access privileges, the process is
 * marked 'delayed unlock' and the pages involved in the I/O are
 * faulted and locked. After the completion of the I/O, the above pages
 * are unlocked.
 */
/*ARGSUSED*/
physio(strat, bp, dev, rw, mincnt, uio)
	int (*strat)();
	struct buf *bp;
	dev_t dev;
	int rw;
	void (*mincnt)();
	struct uio *uio;
{
	register struct iovec *iov;
	register int c;
	faultcode_t fault_err;
	char *a;
	int s, error = 0;
	int allocbuf = 0;

	if (bp == NULL) {
		/*
		 * Allocate buffer header.
		 * XXX Should use common function (getvbuf?) here and
		 * for swap buf headers.
		 */
		bp = (struct buf *)new_kmem_fast_zalloc(
			&physio_bufs, sizeof (struct buf), 1, KMEM_SLEEP);
		bp->b_flags = 0;
		allocbuf = 1;
	}
nextiov:
	if (uio->uio_iovcnt == 0)
		goto out;
	iov = uio->uio_iov;
	if (useracc(iov->iov_base, (u_int)iov->iov_len,
	    rw == B_READ ? B_WRITE : B_READ) == NULL) {
		error = EFAULT;
		goto out;
	}
	s = spl6();
	while (bp->b_flags & B_BUSY) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)bp, PRIBIO+1);
	}
	(void) splx(s);
	bp->b_error = 0;
	bp->b_proc = u.u_procp;
	bp->b_un.b_addr = iov->iov_base;
	while (iov->iov_len > 0) {
		bp->b_flags = B_BUSY | B_PHYS | rw;
		bp->b_dev = dev;
		bp->b_blkno = (uio->uio_fmode & FSETBLK) ?
		    uio->uio_offset : btodb(uio->uio_offset);
		/*
		 * Don't count on b_addr remaining untouched by the
		 * code below (it may be reset because someone does
		 * a bp_mapin on the buffer) -- reset from the iov
		 * each time through, updating the iov's base address
		 * instead.
		 */
		bp->b_un.b_addr = iov->iov_base;
		bp->b_bcount = iov->iov_len;
		(*mincnt)(bp);
		c = bp->b_bcount;
		u.u_procp->p_flag |= SPHYSIO;
		a = bp->b_un.b_addr;
		fault_err = as_fault(u.u_procp->p_as, a, (u_int)c, F_SOFTLOCK,
		    rw == B_READ? S_WRITE : S_READ);
		if (fault_err != 0) {
			/*
			 * Even though the range of addresses were valid
			 * and had the correct permissions, we couldn't
			 * lock down all the pages for the access we needed.
			 * (e.g. we needed to allocate filesystem blocks
			 * for rw == B_READ but the file system was full).
			 */
			if (FC_CODE(fault_err) == FC_OBJERR)
				error = FC_ERRNO(fault_err);
			else
				error = EFAULT;
			bp->b_flags |= B_ERROR;
			bp->b_error = error;
			(void) spl6();
			if (u.u_procp->p_threadcnt <= 1)
				u.u_procp->p_flag &= ~SPHYSIO;
			if (bp->b_flags & B_WANTED)
				wakeup((caddr_t)bp);
			(void) splx(s);
			bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
			goto out;
		}
		if (buscheck(bp) < 0) {
			/*
			 * The io was not requested across legal pages.
			 */
			bp->b_flags |= B_ERROR;
			bp->b_error = error = EFAULT;
		} else {
			(void) (*strat)(bp);
			error = biowait(bp);
		}
		if (as_fault(u.u_procp->p_as, a, (u_int)c, F_SOFTUNLOCK,
		    rw == B_READ? S_WRITE : S_READ) != 0)
			panic("physio unlock");
		(void) spl6();
		if (u.u_procp->p_threadcnt <= 1)
			u.u_procp->p_flag &= ~SPHYSIO;
		if (bp->b_flags & B_WANTED)
			wakeup((caddr_t)bp);
		(void) splx(s);
		c -= bp->b_resid;
		iov->iov_base += c;
		iov->iov_len -= c;
		uio->uio_resid -= c;
		uio->uio_offset += (uio->uio_fmode & FSETBLK) ? btodb(c) : c;
		/* temp kludge for tape drives */
		if (bp->b_resid || error)
			break;
	}
	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
	/* temp kludge for tape drives */
	if (bp->b_resid || error)
		goto out;
	uio->uio_iov++;
	uio->uio_iovcnt--;
	goto nextiov;
out:
	if (allocbuf)
		kmem_fast_free(&physio_bufs, (caddr_t)bp);
	return (error);
}

int
useracc(addr, count, access)
	caddr_t addr;
	u_int count;
	int access;
{
	u_int prot;

	prot = PROT_USER | ((access == B_READ) ? PROT_READ : PROT_WRITE);
	if (as_checkprot(u.u_procp->p_as, (addr_t)addr, count, prot) ==
	    A_SUCCESS)
		return (1);
	/*
	 * We may have failed above due to accessing a part of the stack
	 * that has not been expanded yet. This may happen when doing
	 * a physio() to a buffer on the stack for instance.
	 */
	if (grow((int)addr) == 0)
		return (0);	/* addr not part of stack expansion */
	return (as_checkprot(u.u_procp->p_as, (addr_t)addr, count, prot) ==
		A_SUCCESS);
}

int
kernacc(addr, count, access)
	caddr_t addr;
	u_int count;
	int access;
{
	u_int prot;

	prot = ((access == B_READ) ? PROT_READ : PROT_WRITE);
	return (as_checkprot(&kas, (addr_t)addr, count, prot) == A_SUCCESS);
}

/*
 * Add pages [first, last) to page free list.
 * Assumes that memory is allocated with no
 * overlapping allocation ranges, and that increasing physical addresses
 * imply increasing page struct addresses.
 */
memialloc(first, last)
	register u_int first, last;
{
	register struct page *p, *q;

	p = page_numtopp(first);
	q = page_numtopp(last - 1);
	if (first >= last ||
	    p == (struct page *)NULL ||
	    q == (struct page *)NULL)
		panic("memialloc");
	for (; p <= q; p++)
		page_free(p, 1);
}

/*
 * Common code to core dump the UNIX data or stack segment.
 */
int
core_seg(vp, offset, addr, size, cred)
	struct vnode *vp;
	int offset;
	register caddr_t addr;
	int size;
	struct ucred *cred;
{
	register addr_t eaddr;
	addr_t base;
	u_int len;
	register int err;

	eaddr = (addr_t)(addr + size);
	for (base = addr; base < eaddr; base += len) {
		len = eaddr - base;
		if (as_memory(u.u_procp->p_as, &base, &len) != A_SUCCESS)
			return (0);
		err = corefile_rdwr(UIO_WRITE, vp, base, (int)len,
		    offset + (base - (addr_t)addr),
		    UIO_USERSPACE, IO_UNIT, (int *)0, cred);
		if (err)
			return (err);
	}
	return (0);
}
