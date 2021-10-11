#ifndef lint
static	char sccsid[] = "@(#)mem.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Memory special file
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/uio.h>
#include <sys/mman.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/seg_kmem.h>
#include <sys/ioctl.h>
#include <sun/mem.h>

/* for /dev/meter */
#include <machine/memerr.h>

/* transfer sizes */
#define	SHORT	1
#define	LONG	2

int vmmap_flag = 0;      /* See comments below in mmrw() */
#define VMMAP_IDLE 0x0
#define VMMAP_BUSY 0x1
#define VMMAP_WANT 0x2


/*
 * Check bus type memory spaces for accessibility on this machine
 */
mmopen(dev)
	dev_t dev;
{
	switch (minor(dev)) {
	case M_MEM:
	case M_KMEM:
	case M_NULL:
	case M_ZERO:
		/* standard devices */
		break;

	case M_EEPROM:
		/* all Sun-4 machines must have EEPROM */
		break;

	case M_VME16D16:
	case M_VME24D16:
	case M_VME32D16:
	case M_VME16D32:
	case M_VME24D32:
	case M_VME32D32:
		break;

	case M_METER:
		if (cpu == CPU_SUN4_470)	/* only works for Sunray */
			break;
		/* otherwise fall through to unsupported/unknown */

	case M_MBMEM:
	case M_MBIO:
	default:
		/* Unsupported or unknown type */
		return (EINVAL);
	}
	return (0);
}

mmread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (mmrw(dev, uio, UIO_READ));
}

mmwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (mmrw(dev, uio, UIO_WRITE));
}

/*
 * When reading the M_ZERO device, we simply copyout the zeroes
 * array in NZEROES sized chunks to the user's address.
 *
 * XXX - this is not very elegant and should be redone.
 */
#define	NZEROES		0x100
static char zeroes[NZEROES];

mmrw(dev, uio, rw)
	dev_t dev;
	struct uio *uio;
	enum uio_rw rw;
{
	register int i, o, xfersize;
	register u_int c, v;
	register struct iovec *iov;
	int error = 0;
	int pgsp;
	register u_int *p;
	u_int meters[5];

	while (uio->uio_resid > 0 && error == 0) {
		iov = uio->uio_iov;
		if (iov->iov_len == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			if (uio->uio_iovcnt < 0)
				panic("mmrw");
			continue;
		}
		switch (minor(dev)) {

		case M_MEM:
			v = btop(uio->uio_offset);
			if (v >= physmem)
				goto fault;
			/* There is a race condition in the original code */
                        /* Process A could do the segkmem_mapin, and then
                           sleep inside the uiomove. Process B could then
                           come in and either changed the mapping when it
                           calls segkmem_mapin, or remove the mapping
                           compeletly if it reaches the segkmem_mapout
                           call below.  Thus, we need to put sleep/wakeup
                           pairs to bracket the code between segkmem_mapin
                           and segkmem_mapout, to guarantee mutual
                           exclusion. Bugid 1079876 */

                        /* If someone is in the critical section, don't
                           enter it */
                        while (vmmap_flag & VMMAP_BUSY){
                                vmmap_flag |= VMMAP_WANT;
                                sleep(vmmap, PRIBIO);
                        }
                        vmmap_flag |= VMMAP_BUSY;
			segkmem_mapin(&kseg, vmmap, MMU_PAGESIZE,
			    (u_int)(rw == UIO_READ ? PROT_READ :
			    PROT_READ | PROT_WRITE), v, 0);
			o = (int)uio->uio_offset & PGOFSET;
			c = MIN((u_int)(NBPG - o), (u_int)iov->iov_len);
			/*
			 * I don't know why we restrict ourselves to
			 * no more than the rest of the target page.
			 */
			c = MIN(c, (u_int)(NBPG -
				((int)iov->iov_base & PGOFSET)));
			error = uiomove((caddr_t)&vmmap[o], (int)c, rw, uio);
			segkmem_mapout(&kseg, vmmap, MMU_PAGESIZE);
			/* Done. Wakeup anyone sleeping */
                        vmmap_flag &= ~VMMAP_BUSY;
                        if (vmmap_flag & VMMAP_WANT) {
                                vmmap_flag &= ~VMMAP_WANT;
                                wakeup(vmmap);
                        }
			break;

		case M_KMEM:
			c = iov->iov_len;
			if (kernacc((caddr_t)uio->uio_offset, c,
			    rw == UIO_READ ? B_READ : B_WRITE)) {
				error = uiomove((caddr_t)uio->uio_offset,
					(int)c, rw, uio);
				continue;
			}
			error = mmpeekio(uio, rw, (caddr_t)uio->uio_offset,
			    (int)c, SHORT);
			break;

		case M_ZERO:
			if (rw == UIO_READ) {
				c = MIN(iov->iov_len, sizeof (zeroes));
				error = uiomove(zeroes, (int)c, rw, uio);
				break;
			}
			/* else it's a write, fall through to NULL case */

		case M_NULL:
			if (rw == UIO_READ)
				return (0);
			c = iov->iov_len;
			iov->iov_base += c;
			iov->iov_len -= c;
			uio->uio_offset += c;
			uio->uio_resid -= c;
			break;

		case M_EEPROM:
			error = mmeeprom(uio, rw, (caddr_t)uio->uio_offset,
			    iov->iov_len);
			if (error == -1)
				return (0);		/* EOF */
			break;

		case M_VME16D16:
			if (uio->uio_offset >= VME16_SIZE)
				goto fault;
			v = uio->uio_offset + VME16_BASE;
			pgsp = PGT_VME_D16;
			xfersize = SHORT;
			goto vme;

		case M_VME16D32:
			if (uio->uio_offset >= VME16_SIZE)
				goto fault;
			v = uio->uio_offset + VME16_BASE;
			pgsp = PGT_VME_D32;
			xfersize = LONG;
			goto vme;

		case M_VME24D16:
			if (uio->uio_offset >= VME24_SIZE)
				goto fault;
			v = uio->uio_offset + VME24_BASE;
			pgsp = PGT_VME_D16;
			xfersize = SHORT;
			goto vme;

		case M_VME24D32:
			if (uio->uio_offset >= VME24_SIZE)
				goto fault;
			v = uio->uio_offset + VME24_BASE;
			pgsp = PGT_VME_D32;
			xfersize = LONG;
			goto vme;

		case M_VME32D16:
			pgsp = PGT_VME_D16;
			v = uio->uio_offset;
			xfersize = SHORT;
			goto vme;

		case M_VME32D32:
			pgsp = PGT_VME_D32;
			v = uio->uio_offset;
			xfersize = LONG;
			/* FALL THROUGH */

		vme:
			v = btop(v);

			/* Mapin for VME, no cache operation is involved. */
			while (vmmap_flag & VMMAP_BUSY){
                                        vmmap_flag |= VMMAP_WANT;
                                        sleep(vmmap, PRIBIO);
                        }
                        vmmap_flag |= VMMAP_BUSY;
			segkmem_mapin(&kseg, vmmap, MMU_PAGESIZE,
			    (u_int)(rw == UIO_READ ? PROT_READ :
			    PROT_READ | PROT_WRITE), pgsp | v, 0);
			o = (int)uio->uio_offset & PGOFSET;
			c = min((u_int)(NBPG - o), (u_int)iov->iov_len);
			error = mmpeekio(uio, rw, &vmmap[o], (int)c, xfersize);
			segkmem_mapout(&kseg, vmmap, MMU_PAGESIZE);
			vmmap_flag &= ~VMMAP_BUSY;
                        if (vmmap_flag & VMMAP_WANT) {
                                        vmmap_flag &= ~VMMAP_WANT;
                                        wakeup(vmmap);
                        }
			break;

		case M_METER:
			/*
			 * fill the destination with the meter contents
			 * large sizes are ignored as this device has
			 * only four words, smaller ones will limit it.
			 * writes are not allowed.
			 */
		 
			if (rw == UIO_WRITE)
				return (EPERM);
		 
			p = (u_int *)(MEMERR_ADDR + 0x1c);
			for (i = 0; i < 5; i++) {
				meters[i] = *p++;
			}
			error = uiomove((caddr_t)meters,
				(iov->iov_len > 20) ? 20 : iov->iov_len,
				UIO_READ, uio);
			break;

		}
	}
	return (error);
fault:
	return (EFAULT);
}

static
mmpeekio(uio, rw, addr, len, xfersize)
	struct uio *uio;
	enum uio_rw rw;
	caddr_t addr;
	int len;
	int xfersize;
{
	register int c;
	int o;
	short sh;
	long lsh;

	while (len > 0) {
		if ((len|(int)addr) & 1) {
			c = sizeof (char);
			if (rw == UIO_WRITE) {
				if ((o = uwritec(uio)) == -1)
					return (EFAULT);
				if (pokec(addr, (char)o))
					return (EFAULT);
			} else {
				if ((o = peekc(addr)) == -1)
					return (EFAULT);
				if (ureadc((char)o, uio))
					return (EFAULT);
			}
		} else {
			if ((xfersize == LONG) &&
			    (((int)addr % sizeof (long)) == 0) &&
			    (len % sizeof (long)) == 0) {
				c = sizeof (long);
				if (rw == UIO_READ) {
					if (peekl((long *)addr, (long *)&o) == -1)
						return (EFAULT);
					lsh = o;
				}
				if (uiomove((caddr_t)&lsh, c, rw, uio))
					return (EFAULT);
				if (rw == UIO_WRITE) {
					if (pokel((long *)addr, lsh))
						return (EFAULT);
				}
			} else {
				c = sizeof (short);
				if (rw == UIO_READ) {
					if ((o = peek((short *)addr)) == -1)
						return (EFAULT);
					sh = o;
				}
				if (uiomove((caddr_t)&sh, c, rw, uio))
					return (EFAULT);
				if (rw == UIO_WRITE) {
					if (poke((short *)addr, sh))
						return (EFAULT);
				}
			}
		}
		addr += c;
		len -= c;
	}
	return (0);
}

/*
 * If eeprombusy is true, then the eeprom has just
 * been written to and cannot be read or written
 * until the required 10 MS has passed. It is
 * assumed that the only way the EEPROM is written
 * is thru the mmeeprom routine.
 */
static int eeprombusy = 0;

static
mmeepromclear()
{

	eeprombusy = 0;
	wakeup((caddr_t)&eeprombusy);
}

static
mmeeprom(uio, rw, addr, len)
	struct uio *uio;
	enum uio_rw rw;
	caddr_t addr;
	int len;
{
	int o, oo;
	int s;

	if ((int)addr > EEPROM_SIZE)
		return (EFAULT);

	while (len > 0) {
		if ((int)addr == EEPROM_SIZE)
			return (-1);			/* EOF */

		if (cpu != CPU_SUN4_330) {
			s = splclock();
			while (eeprombusy)
				(void) sleep((caddr_t)&eeprombusy, PUSER);
			(void) splx(s);
		}

		if (rw == UIO_WRITE) {
			if ((o = uwritec(uio)) == -1)
				return (EFAULT);
			if ((oo = peekc(EEPROM_ADDR + addr)) == -1)
				return (EFAULT);
			/*
			 * Check to make sure that the data is actually
			 * changing before committing to doing the write.
			 * This avoids the unneeded eeprom lock out
			 * and reduces the number of times the eeprom
			 * is actually written to.
			 */
			if (o != oo) {
				if (pokec(EEPROM_ADDR + addr, (char)o))
					return (EFAULT);
				if (cpu != CPU_SUN4_330) {
					/*
					 * Block out access to the eeprom for
					 * two clock ticks
					 * (longer than > 10 MS).
					 */
					eeprombusy = 1;
					timeout(mmeepromclear, (caddr_t)0, 2);
				}
			}
		} else {
			if ((o = peekc(EEPROM_ADDR + addr)) == -1)
				return (EFAULT);
			if (ureadc((char)o, uio))
				return (EFAULT);
		}
		addr += sizeof (char);
		len -= sizeof (char);
	}
	return (0);
}

mmioctl(dev, cmd, arg)
	dev_t dev;
	int cmd;
	caddr_t arg;
{
	int error = 0;
	register u_int *p;

	if (minor(dev) != M_METER && (cmd == MM_CCRW || cmd == MM_HRCNT ||
			cmd == MM_PCNT0 || cmd == MM_PCNT1))
		return(EINVAL);

	switch (cmd) {
	case _SUNDIAG_V2P: {
		struct pte pte;
		struct v2p *vtop;
		addr_t adr;

		vtop = (struct v2p *)arg;
		adr = (addr_t)((int) vtop->vaddr & MMU_PAGEMASK);
		mmu_getpte(adr, &pte);
		if (pte_valid(&pte))
			vtop->paddr = (caddr_t) (pte.pg_pfnum << MMU_PAGESHIFT)
			    + ((int) vtop->vaddr & MMU_PAGEOFFSET);
		else
			error = EINVAL;
		break;
	    }

        case MM_CCRW:
		p = (u_int *)arg;
                *(u_int *)(MEMERR_ADDR + 0x1c) = *p;
                break;
        case MM_HRCNT:
                *(u_int *)(MEMERR_ADDR + 0x20) = 0;
                *(u_int *)(MEMERR_ADDR + 0x24) = 0;
                break;
        case MM_PCNT0:
                *(u_int *)(MEMERR_ADDR + 0x28) = 0;
                break;
        case MM_PCNT1:
                *(u_int *)(MEMERR_ADDR + 0x2c) = 0;
                break;

	default:
		error = EINVAL;
	}
	return (error);
}

/*ARGSUSED*/
mmmmap(dev, off, prot)
	dev_t dev;
	off_t off;
{
	int pf;
	struct pte pte;

	switch (minor(dev)) {

	case M_MEM:
		pf = btop(off);
		if (pf < physmem)
			return (PGT_OBMEM | pf);
		break;

	case M_KMEM:
		mmu_getkpte((addr_t)off, &pte);
		if (pte_valid(&pte))
			return (MAKE_PFNUM(&pte));
		break;

	case M_VME16D16:
		if (off >= VME16_SIZE)
			break;
		return (PGT_VME_D16 | btop(off + VME16_BASE));

	case M_VME16D32:
		if (off >= VME16_SIZE)
			break;
		return (PGT_VME_D32 | btop(off + VME16_BASE));

	case M_VME24D16:
		if (off >= VME24_SIZE)
			break;
		return (PGT_VME_D16 | btop(off + VME24_BASE));

	case M_VME24D32:
		if (off >= VME24_SIZE)
			break;
		return (PGT_VME_D32 | btop(off + VME24_BASE));

	case M_VME32D16:
		return (PGT_VME_D16 | btop(off));

	case M_VME32D32:
		return (PGT_VME_D32 | btop(off));

	/* ZZZ */
	case M_METER:
		break;

	case M_ZERO:
		/*
		 * We shouldn't be mmap'ing to /dev/zero here as this
		 * mmsegmap should have already converted the mapping
		 * request for this device to a mapping using seg_vn.
		 */
	default:
		break;
	}
	return (-1);
}

/*
 * This function is called when a memory device is mmap'ed.
 * Set up the mapping to the correct device driver.
 */
int
mmsegmap(dev, off, as, addrp, len, prot, maxprot, flags, cred)
	dev_t dev;
	u_int off;
	struct as *as;
	addr_t *addrp;
	u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{
	struct segvn_crargs vn_a;

	/*
	 * If we are not mapping /dev/zero, then use spec_segmap()
	 * to set up the mapping which resolves to using mmmap().
	 */
	if (minor(dev) != M_ZERO) {
		return (spec_segmap(dev, off, as, addrp, len, prot, maxprot,
		    flags, cred));
	}

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * No need to worry about vac alignment since this
		 * is a "clone" object that doesn't yet exist.
		 */
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address -
		 * Blow away any previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	/*
	 * Use seg_vn segment driver for /dev/zero mapping.
	 * Passing in a NULL amp gives us the "cloning" effect.
	 */
	vn_a.vp = NULL;
	vn_a.offset = 0;
	vn_a.type = (flags & MAP_TYPE);
	vn_a.prot = prot;
	vn_a.maxprot = maxprot;
	vn_a.cred = cred;
	vn_a.amp = NULL;

	return (as_map(as, *addrp, len, segvn_create, (caddr_t)&vn_a));
}
