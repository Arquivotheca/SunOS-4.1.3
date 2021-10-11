#ifndef lint
static	char sccsid[] = "@(#)mem.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
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
#include <machine/async.h>
#include <sys/ioctl.h>
#include <sun/mem.h>
#include <os/atom.h>

extern struct memlist *availmemory;

/*
 * SBUS space layout information
 * array index is slot number
 * %%% NOTE: Sun4M/690's onboard stuff
 * is in slot "4", not slot "15".
 */
extern unsigned	sbus_numslots;
extern unsigned	sbus_basepage[];
extern unsigned	sbus_slotsize[];

#ifdef	VME
/*
 * VME space layout information
 * so far, all sun4m vme interfaces
 * are supposed to match this.
 * Note that mmaping vmewhatever
 * gets you a vme "user" access,
 * but the kernel gets "supv"
 * when the device gets mapped
 * into kernel space.
 */

#define	VME16D16_PAGE	0xCFFFF0
#define	VME16D32_PAGE	0xDFFFF0
#define	VME24D16_PAGE	0xCFF000
#define	VME24D32_PAGE	0xDFF000
#define	VME32D16_PAGE	0xC00000
#define	VME32D32_PAGE	0xD00000

#define	VME_A16_MASK	0x0000FFFF
#define	VME_A24_MASK	0x00FFFFFF
#define	VME_A32_MASK	0xFFFFFFFF
#endif	VME

/*
 * Other definitions have been moved to <sun/mem.h>
 */

/* transfer sizes */
#define	SHORT	1
#define	LONG	2

/*
 * SLOTOFF is a workaround for the prom since it is passing back the wrong info
 * in the sbus_basepage array, and the prom has currently frozen its FCS version.
 * Slot 0 is correct, but the other slot base
 * pages are incorrect.  Fix it here since this is the only place it is
 * used.  Also make it backwards compatible so it won't break if the prom
 * gets fixed.
 */
/*sbus slot offset for sun4m */
#define SLOTOFF	0x10000

#ifdef NOPROM
extern int availmem;	/* physical memory available in SAS */
#endif

int vmmap_flag = 0;     /* See comments below */
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
		/* all Sun-4c machines must have EEPROM */
		break;

#ifdef	VME
	case M_VME16D16:
	case M_VME24D16:
	case M_VME32D16:
	case M_VME16D32:
	case M_VME24D32:
	case M_VME32D32:
		if (cpu == CPU_SUN4M_690)
			break;
		/*FALLTHRU*/
#endif	VME
	case M_MBMEM:
	case M_MBIO:
		/* Unsupported type */
		return (EINVAL);

	default:
		/*
		 * First see if it's one of the SBUS minor numbers
		 */
		if ((minor(dev) & ~M_SBUS_SLOTMASK) == M_SBUS) {
			if ((minor(dev) & M_SBUS_SLOTMASK) >= sbus_numslots) {
				/* Slot number too high */
				return (ENXIO);
			} else {
				/* all Sun-4m machines have an SBUS */
				return (0);
			}
		} else {
			/* Unsupported or unknown type */
			return (EINVAL);
		}
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
	register int o;
	register u_int c, v;
	register struct iovec *iov;
	int error = 0;
	int size;
	struct memlist *pmem;

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
#ifndef NOPROM
			for (pmem = availmemory;
			    pmem != (struct memlist *)NULL;
			    pmem = pmem->next) {
				if (v >= btop(pmem->address) &&
				    v < btop(pmem->address + pmem->size))
					break;
			}
			if (pmem == (struct memlist *)NULL)
				goto fault;
#else NOPROM
			if (v >= btop(availmem))
				goto fault;
#endif
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
                                (void) sleep(vmmap, PRIBIO);
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

#ifdef	VME
		case M_VME16D16:
			if (uio->uio_offset & ~VME_A16_MASK)
				goto fault;
			v = VME16D16_PAGE;
			size = SHORT;
			goto vme;

		case M_VME16D32:
			if (uio->uio_offset & ~VME_A16_MASK)
				goto fault;
			v = VME16D32_PAGE;
			size = LONG;
			goto vme;

		case M_VME24D16:
			if (uio->uio_offset & ~VME_A24_MASK)
				goto fault;
			v = VME24D16_PAGE;
			size = SHORT;
			goto vme;

		case M_VME24D32:
			if (uio->uio_offset & ~VME_A24_MASK)
				goto fault;
			v = VME24D32_PAGE;
			size = LONG;
			goto vme;

		case M_VME32D16:
		/* %%% knows that VME_A32_MASK is all ones */
			v = VME32D16_PAGE;
			size = SHORT;
			goto vme;

		case M_VME32D32:
		/* %%% knows that VME_A32_MASK is all ones */
			v = VME32D32_PAGE;
			size = LONG;
			/* FALL THROUGH */

		vme:
			v |= btop(uio->uio_offset);

			/* Mapin for VME, no cache operation is involved. */

			/* See comments above for the case MEM */
                        while (vmmap_flag & VMMAP_BUSY){
                                vmmap_flag |= VMMAP_WANT;
                                (void) sleep(vmmap, PRIBIO);
                        }
                        vmmap_flag |= VMMAP_BUSY;

			segkmem_mapin(&kseg, vmmap, MMU_PAGESIZE,
			    (u_int)(rw == UIO_READ ? PROT_READ :
			    PROT_READ | PROT_WRITE), v, 0);
			o = (int)uio->uio_offset & PGOFSET;
			c = min((u_int)(NBPG - o), (u_int)iov->iov_len);
			error = mmpeekio(uio, rw, &vmmap[o], (int)c, size);

			segkmem_mapout(&kseg, vmmap, MMU_PAGESIZE);
			vmmap_flag &= ~VMMAP_BUSY;
                        if (vmmap_flag & VMMAP_WANT) {
                                vmmap_flag &= ~VMMAP_WANT;
                                wakeup(vmmap);
                        }
			break;
#endif	VME

		default:
			if ((minor(dev) & ~M_SBUS_SLOTMASK) == M_SBUS) {
				/* case M_SBUS: */
				int slot;

				slot = minor(dev) & M_SBUS_SLOTMASK;
				if (slot >= sbus_numslots) {
					/*
					 * If we get here, dev is bad
					 */
					panic("mmrw: invalid minor(dev)\n");
				}
				if (uio->uio_offset >= sbus_slotsize[slot]) {
					goto fault;
				}
				v = btop(uio->uio_offset);
				/*
				 * Mapin for Sbus, no cache operation
				 * is involved.
				 */
				
                                while (vmmap_flag & VMMAP_BUSY){
                                        vmmap_flag |= VMMAP_WANT;
                                        (void) sleep(vmmap, PRIBIO);
                                }
                                vmmap_flag |= VMMAP_BUSY;
				segkmem_mapin(&kseg, vmmap, MMU_PAGESIZE,
					(u_int)(rw == UIO_READ ? PROT_READ :
					PROT_READ | PROT_WRITE),
					sbus_basepage[0] + (slot*SLOTOFF) + v,
					0);
				o = (int)uio->uio_offset & PGOFSET;
				c = min((u_int)(NBPG - o), (u_int)iov->iov_len);
				error = mmpeekio(uio, rw, &vmmap[o],
								(int)c, LONG);
				segkmem_mapout(&kseg, vmmap, MMU_PAGESIZE);
				vmmap_flag &= ~VMMAP_BUSY;
                                if (vmmap_flag & VMMAP_WANT) {
                                        vmmap_flag &= ~VMMAP_WANT;
                                        wakeup(vmmap);
                                }
			} else {
				/*
				 * If we get here, either dev is bad,
				 * or someone has created a mem
				 * device which doesn't support read and write
				 */
				panic("mmrw: invalid minor(dev)\n");
			}
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
					if (peekl((long *)addr, (long *)&o)
						== -1) {
						return (EFAULT);
					}
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
 * Read/write the eeprom. On Sun4c machines, the eeprom is really static
 * memory, so there are no delays or write limits. We simply do byte
 * writes like it is memory.
 * However, we do keep the eeprom write-protected when not actively
 * using it, since it does contain the idprom (cpuid and ethernet address)
 * and we don't want them to get accidentally scrogged.
 * For that reason, mmap'ing the EEPROM is not allowed.
 * Since reading the clock also requires writing the EEPROM, we
 * splclock() to prevent interference.
 */
static
mmeeprom(uio, rw, addr, len)
	struct uio *uio;
	enum uio_rw rw;
	caddr_t addr;
	int len;
{
	int o;

	if ((int)addr > EEPROM_SIZE)
		return (EFAULT);

	while (len > 0) {
		if ((int)addr == EEPROM_SIZE)
			return (-1);			/* EOF */

		if (rw == UIO_WRITE) {
			int s;
			struct pte pte;
			unsigned int saveprot;
			int err = 0;
			caddr_t eepromaddr = EEPROM_ADDR + addr;

			if ((o = uwritec(uio)) == -1)
				return (EFAULT);
			/* write-enable the eeprom; we "know" it isn't cached */
			s = splclock();
			mmu_getpte(eepromaddr, &pte);
			saveprot = pte.AccessPermissions;
			pte.AccessPermissions = MMU_STD_SRWX;
			mmu_setpte(eepromaddr, pte);
			if (pokec(eepromaddr, (char)o))
				err = EFAULT;
			mmu_getpte(eepromaddr, &pte);
			pte.AccessPermissions = saveprot;
			mmu_setpte(eepromaddr, pte);
			(void) splx(s);
			if (err)
				return (err);
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

/*ARGSUSED*/
mmioctl(dev, cmd, arg)
	dev_t dev;
	int cmd;
	caddr_t arg;
{
	int error = 0;
#ifdef MULTIPROCESSOR
	int npam;
	extern int cpuid;
	extern int procset;
	extern struct proc *masterprocp;
#endif	MULTIPROCESSOR

	switch (cmd) {
	case MIOCSPAM:
#ifndef	MULTIPROCESSOR
		*(u_int *)arg = 1;
#else	MULTIPROCESSOR

		npam = *(u_int *)arg;
		/*
		 * if mask would disallow execution on all CPUs, then
		 * enable execution on the current one.
		 */
		if ((npam & procset) == 0)
			npam = 1<<cpuid;
		masterprocp->p_pam = npam;
		/*
		 * if mask disallows execution on the current CPU, then
		 * set runrun so we switch processes.
		 */
		if (!(npam & 1<<cpuid))
			runrun = 1;
		/*
		 * return the new mask value, only including the active
		 * processors.
		 */
		*(u_int *)arg = npam & procset;
#endif	MULTIPROCESSOR
		break;

	case MIOCGPAM:
#ifndef	MULTIPROCESSOR
		*(u_int *)arg = 1;
#else	MULTIPROCESSOR
		*(u_int *)arg = masterprocp->p_pam & procset;
#endif	MULTIPROCESSOR
		break;
	case _SUNDIAG_V2P: {
		struct pte pte;
		struct v2p *vtop;
		addr_t adr;

		vtop = (struct v2p *)arg;
		adr = (addr_t)((int) vtop->vaddr & MMU_PAGEMASK);
		mmu_getpte(adr, &pte);
		if (pte_valid(&pte))
			vtop->paddr = 
				(caddr_t) (pte.PhysicalPageNumber << MMU_PAGESHIFT)
					+ ((int) vtop->vaddr & MMU_PAGEOFFSET);
		else
			error = EINVAL;
		break;
	}
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
	register struct memlist *pmem;

	switch (minor(dev)) {

	case M_MEM:
		pf = btop(off);
#ifndef NOPROM
		for (pmem = availmemory;
		    pmem != (struct memlist *)NULL; pmem = pmem->next) {
			if (pf >= btop(pmem->address) &&
			    pf < btop(pmem->address + pmem->size))
				return (pf);
		}
#else
		if (pf < btop(availmem))
			return (pf);
#endif
		break;

	case M_KMEM:
		mmu_getkpte((addr_t)off, &pte);
		if (pte_valid(&pte))
			return (MAKE_PFNUM(&pte));
		break;

	case M_ZERO:
		/*
		 * We shouldn't be mmap'ing to /dev/zero here as mmsegmap
		 * should have already converted a mapping request for
		 * this device to a mapping using seg_vn.
		 */
		break;

#ifdef	VME
	case M_VME16D16:
		if (off & ~VME_A16_MASK)
			break;
		return (VME16D16_PAGE | btop(off));

	case M_VME16D32:
		if (off & ~VME_A16_MASK)
			break;
		return (VME16D32_PAGE | btop(off));

	case M_VME24D16:
		if (off & ~VME_A24_MASK)
			break;
		return (VME24D16_PAGE | btop(off));

	case M_VME24D32:
		if (off & ~VME_A24_MASK)
			break;
		return (VME24D32_PAGE | btop(off));

	case M_VME32D16:
		/* %%% knows that VME_A32_MASK is all ones */
		return (VME32D16_PAGE | btop(off));

	case M_VME32D32:
		/* %%% knows that VME_A32_MASK is all ones */
		return (VME32D32_PAGE | btop(off));
#endif	VME

	default:
		if ((minor(dev) & ~M_SBUS_SLOTMASK) == M_SBUS) {
			/* case M_SBUS */
			int slot;
			slot = minor(dev) & M_SBUS_SLOTMASK;
			if (slot >= sbus_numslots) {
				/*
				 * If we get here, dev is bad
				 */
				panic("mmmmap: invalid minor(dev)\n");
			}


			if (off >= sbus_slotsize[slot]) {
				break;
			}
			return (sbus_basepage[0] + (slot*SLOTOFF) +  btop(off));
		}
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
	int ret;

	/*
	 * If we are not mapping /dev/zero, then use spec_segmap()
	 * to set up the mapping which resolves to using mmmap().
	 */
	if (minor(dev) != M_ZERO) {
		ret = spec_segmap(dev, off, as, addrp, len, prot, maxprot,
		    flags, cred);
#ifdef	MULTIPROCESSOR
		if (ret != -1) {
		    	if (((minor(dev)) & ~M_SBUS_SLOTMASK) == M_SBUS) {
				register u_int slot;

				slot = minor(dev) & M_SBUS_SLOTMASK;
				off |= (slot << 28);
				mm_add(uunix->u_procp, MM_SBUS_DEVS, off, len);
#ifdef	VME
			} else {
				switch (minor(dev)) {
				case M_VME16D16:
				case M_VME16D32:
					off |= 0xFFFF0000;
		       	        	mm_add(uunix->u_procp, MM_VME_DEVS,
						off, len);
					break;
				case M_VME24D16:
				case M_VME24D32:
					off |= 0xFF000000;
		       	        	mm_add(uunix->u_procp, MM_VME_DEVS,
						off, len);
					break;
				case M_VME32D16:
				case M_VME32D32:
		       	        	mm_add(uunix->u_procp, MM_VME_DEVS,
						off, len);
					break;
				default:
					break;
				}
#endif	VME
			}
		}
#endif	MULTIPROCESSOR
		return(ret);
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

#ifdef	MULTIPROCESSOR
struct sbus_vme_mm *mm_base = NULL;
extern atom_t mm_flag;

mm_add(p, type, off, len)
	struct proc *p;
	u_int type;
	u_int off;
	u_int len;
{
	register struct sbus_vme_mm *mmp, *nmmp, *smmp;
	extern struct sbus_vme_mm *mm_find_proc();
	struct user *pu = p->p_uarea;

	smmp = mm_base;
	while ((mmp = mm_find_proc(p, smmp)) != NULL) {
		if ((mmp->mm_type == type) &&
		    ((off >= mmp->mm_paddr) &&
		     (off+len <= mmp->mm_paddr+mmp->mm_len)))
			return;
		smmp = mmp->mm_next;
	}
	mmp = (struct sbus_vme_mm *) new_kmem_alloc((sizeof(struct
		sbus_vme_mm)), KMEM_SLEEP);
	mmp->mm_next = NULL;
	mmp->mm_p = p;
	mmp->mm_paddr = (u_int) off;
	mmp->mm_len = len;
	mmp->mm_type = type;
#ifdef	VME
	if (mmp->mm_type == MM_VME_DEVS)
		pu->u_pcb.pcb_flags |= MM_VME_DEVS;
#endif	VME
	if (mmp->mm_type == MM_SBUS_DEVS)
		pu->u_pcb.pcb_flags |= MM_SBUS_DEVS;

	if (!atom_tas(mm_flag))
		atom_wcs(mm_flag);
	if (mm_base == NULL) {
		mm_base = mmp;
	} else {
		for (nmmp = mm_base; nmmp->mm_next != NULL;
			nmmp = nmmp->mm_next);
		nmmp->mm_next = mmp;
	}
	atom_clr(mm_flag);
}

mm_delete_proc(p)
	struct proc *p;
{
	register struct sbus_vme_mm *nmmp, *pmmp = NULL;

	if (mm_base != NULL) {
		if (!atom_tas(mm_flag))
			atom_wcs(mm_flag);
		for (nmmp = mm_base; nmmp; nmmp = nmmp->mm_next) {
			if (nmmp->mm_p == p) {
				if (nmmp == mm_base)
					mm_base = NULL;
				else if (nmmp->mm_next == NULL)
					pmmp->mm_next = NULL;
				else
					pmmp->mm_next = nmmp->mm_next;
				kmem_free((caddr_t)nmmp, sizeof(struct sbus_vme_mm));
			}
			pmmp = nmmp;
		}
		atom_clr(mm_flag);
	}
}

struct sbus_vme_mm *
mm_find_proc(p, smmp)
	struct proc *p;
	struct sbus_vme_mm *smmp;
{
	register struct sbus_vme_mm *nmmp;

	if (smmp == NULL) {
		return (NULL);
	} else {
		for (nmmp = smmp; nmmp; nmmp = nmmp->mm_next)
			if (nmmp->mm_p == p)
				return (nmmp);
	}
	return (NULL);
}

struct sbus_vme_mm *
mm_find_paddr(type, paddr, smmp)
	u_int type;
	u_int paddr;
	struct sbus_vme_mm *smmp;
{
	register struct sbus_vme_mm *nmmp;

	if (smmp == NULL) {
		return (NULL);
	} else {
		for (nmmp = smmp; nmmp; nmmp = nmmp->mm_next) {
			if ((nmmp->mm_type == type) &&
			    (((u_int)paddr >= nmmp->mm_paddr) &&
			    ((u_int)paddr <= nmmp->mm_paddr+nmmp->mm_len))) {
				return (nmmp);
			}
		}
	}
	return (NULL);
}
#endif	MULTIPROCESSOR
