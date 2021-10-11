#ifndef lint
static  char sccsid[] = "@(#)kvmrdwr.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "kvm.h"
#include "kvm_impl.h"
#include <sys/param.h>
#include <sys/vmmac.h>
#include <sys/file.h>
#include <machine/pte.h>
#include <machine/mmu.h>

static int kvm_rdwr();
extern int read();
extern int write();

int
kvm_read(kd, addr, buf, nbytes)
	kvm_t *kd;
	u_long addr;
	char *buf;
	u_int nbytes;
{

	return (kvm_rdwr(kd, addr, buf, nbytes, read));
}

int
kvm_write(kd, addr, buf, nbytes)
	kvm_t *kd;
	u_long addr;
	char *buf;
	u_int nbytes;
{

	if (!kd->wflag)		/* opened for write? */
		return (-1);
	return (kvm_rdwr(kd, addr, buf, nbytes, write));
}

static int
kvm_rdwr(kd, addr, buf, nbytes, rdwr)
	kvm_t *kd;
	u_long addr;
	char *buf;
	u_int nbytes;
	int (*rdwr)();
{

	return(kvm_as_rdwr(kd, addr >= KERNELBASE ? &kd->Kas : &kd->Uas,
		addr, buf, nbytes, rdwr));
}

kvm_as_read(kd, as, addr, buf, nbytes)
	kvm_t *kd;
	struct as *as;
	u_long addr;
	char *buf;
	u_int nbytes;
{

	return (kvm_as_rdwr(kd, as, addr, buf, nbytes, read));
}

kvm_as_write(kd, as, addr, buf, nbytes)
	kvm_t *kd;
	struct as *as;
	u_long addr;
	char *buf;
	u_int nbytes;
{

	if (!kd->wflag)		/* opened for write? */
		return (-1);
	return (kvm_as_rdwr(kd, as, addr, buf, nbytes, write));
}

kvm_as_rdwr(kd, as, addr, buf, nbytes, rdwr)
	kvm_t *kd;
	struct as *as;
	u_long addr;
	char *buf;
	u_int nbytes;
	int (*rdwr)();
{
	register u_long a;
	register u_long end;
	register int n;
	register int cnt;
	register char *bp;
	register int i;
	int fd;
	u_int paddr;

	if (addr == 0) {
#ifdef _KVM_DEBUG
		_kvm_error("kvm_as_rdwr: address == 0");
#endif _KVM_DEBUG
		return (-1);
	}

	if (kd->corefd == -1) {
#ifdef _KVM_DEBUG
		_kvm_error("kvm_as_rdwr: no corefile descriptor");
#endif _KVM_DEBUG
		return (-1);
	}

	/* if virtual addresses may be used, by all means use them */
	if ((as->a_segs == kd->Kas.a_segs) && (kd->virtfd != -1)) {
		if (lseek(kd->virtfd, (off_t)addr, L_SET) == -1) {
#ifdef _KVM_DEBUG
			_kvm_perror("%s: seek error", kd->virt);
#endif _KVM_DEBUG
			return (-1);
		}
		cnt = (*rdwr)(kd->virtfd, buf, nbytes);
#ifdef _KVM_DEBUG
		if (cnt != nbytes) {
			_kvm_error("%s: i/o error", kd->virt);
		}
#endif _KVM_DEBUG
		return (cnt);
	}
	cnt = 0;
	end = addr + nbytes;
	for (a = addr, bp = buf; a < end; a += n, bp += n) {
		n = PAGESIZE - (addr & PAGEOFFSET);
		if ((a + n) > end)
			n = end - a;
		_kvm_physaddr(kd, as, (u_int)a, &fd, &paddr);
		if (fd == -1) {
#ifdef _KVM_DEBUG
			_kvm_error("kvm_rdwr: _kvm_physaddr failed");
#endif _KVM_DEBUG
			return (-1);
		}
		/* see if file is of condensed type */
		switch (_uncondense(kd, fd, &paddr)) {
		case -1:
#ifdef _KVM_DEBUG
			_kvm_error("%s: uncondense error", kd->core);
#endif _KVM_DEBUG
			return (-1);
		case -2:
			/* paddr corresponds to a hole */
			if (rdwr == read) {
				bzero(bp, n);
			}
			/* writes are ignored */
			cnt += n;
			continue;
		}
		/* could be physical memory or swap */
		if (lseek(fd, (off_t)paddr, L_SET) == -1) {
#ifdef _KVM_DEBUG
			_kvm_perror("%s: seek error", kd->core);
#endif _KVM_DEBUG
			return (-1);
		}
		i = (*rdwr)(fd, bp, n);
		cnt += i;
		if (i != n) {
#ifdef _KVM_DEBUG
			_kvm_perror("%s: i/o error", kd->core);
#endif _KVM_DEBUG
			break;
		}
	}
	return (cnt);
}

int
_uncondense(kd, fd, paddrp)
	kvm_t *kd;
	int fd;
	u_int *paddrp;
{
	struct condensed *cdp;

	/* There ought to be a better way! */
	if (fd == kd->corefd)
		cdp = kd->corecdp;
	else if (fd == kd->swapfd)
		cdp = kd->swapcdp;
	else {
#ifdef _KVM_DEBUG
			_kvm_error("uncondense: unknown fd");
#endif _KVM_DEBUG
		return (-1);
	}

	if (cdp) {
		u_int paddr = *paddrp;
		off_t *atoo = cdp->cd_atoo;
		off_t offset;
		int chunksize = cdp->cd_chunksize;
		int i;

		i = paddr / chunksize;
		if (i < cdp->cd_atoosize) {
			if ((offset = atoo[i]) == NULL)
				return (-2);	/* Hole */
		} else {
			/*
			 * An attempt to read/write beyond the end of
			 * the logical file; convert to the equivalent
			 * offset, and let a read hit EOF and a write do
			 * an extension.
			 */
			offset = (i - cdp->cd_atoosize) * chunksize +
				cdp->cd_dp->dump_dumpsize;
		}

		*paddrp = (u_int)(offset + (paddr % chunksize));
	}
	return (0);
}
