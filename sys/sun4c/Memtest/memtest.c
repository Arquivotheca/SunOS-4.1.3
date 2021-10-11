/* memtest.c */

/*
 * This is a loadable driver (oh boy!) that allows you to check the
 * functioning of the parity and memory error handling code in the kernel.
 *
 * Mmap'ing it gives non-existent storage (references will cause
 * timeouts).
 *
 * The following ioctls are understood:
 * MEMTESTBADPAR	rewrites the byte at (user) addr with bad parity:
 * MEMTESTBADPAREXP	same as MEMTESTBADPAR but does it in expansion memory:	
 * MEMTESTBADFETCH	does a kernel-mode ifetch of a bad-parity word:
 * MEMTESTBADLOAD	does a kernel-mode load of a bad-parity word:
 * MEMTESTBADSTORE	does a kernel-mode store to user address*
 * MEMTESTBADLOADT	does a kernel-mode load of a user address*
 * MEMTESTBADFETCHT	does a kernel-mode ifetch of a user address*
 * MEMTESTPEEKPOKE	does kernel-mode peeks and pokes of a user address*
 *	*preferably one that has been mmap'd to non-existent memory;
 * Usage:
 * int fd = open("/dev/memtest", O_RDWR);
 * caddr_t paddr = mmap(addr, len, prot, flags, fd, off);
 * typically,
 * caddr_t paddr = mmap(0, getpagesize(), PROT_ALL, MAP_SHARED, fd, 0);
 *	*paddr = 0; 	/* cause a memory timeout */ /*
 *
 * char *addr = 0;
 * ioctl(fd, MEMTESTBADPAR, addr);
 * int c = *addr;	/* cause a user parity fault */ /*
 *
 * ioctl(fd, MEMTESTBADFETCH, 0);	/* should panic kernel */ /*
 * ioctl(fd, MEMTESTBADLOAD, 0);	/* should panic kernel */ /*
 * ioctl(fd, MEMTESTBADSTORE, paddr);	/* should panic kernel */ /*
 * ioctl(fd, MEMTESTBADLOADT, paddr);	/* should panic kernel */ /*
 * ioctl(fd, MEMTESTBADFETCHT, paddr);	/* should panic kernel */ /*
 *
 * struct memtest_peekpoke peekpoke;
 * peekpoke.paddr = paddr;
 * ioctl(fd, MEMTESTPEEKPOKE, &peekpoke);/* should return 6 -1's /*
 */

#include <sun/vddrv.h>
#include <memtestio.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sundev/mbvar.h>
#include <vm/seg.h>
#include <machine/pte.h>
#include <machine/memerr.h>
#include <mon/sunromvec.h>
#include <machine/cpu.h>
#include <machine/enable.h>

struct dev_ops	memtestops;
struct cdevsw	memtestcdevsw;

struct vdldrv	memtestdrv = {
	VDMAGIC_PSEUDO,
	"memtest pseudo-device",
	&memtestops,
	NULL,			/* bdevsw */
	&memtestcdevsw,
};

struct dev_ops	memtestops = {
	1,			/* rev */
};

int memtest_open(), memtest_close(), memtest_ioctl(), memtest_mmap();
extern int nodev(), nulldev(), seltrue();

struct cdevsw	memtestcdevsw = {
	memtest_open,
	nulldev,		/* close */
	nodev,			/* read */
	nodev,			/* write */
	memtest_ioctl,
	nodev,			/* reset */
	seltrue,
	memtest_mmap,
	0,			/* str */
};

memtest_init(fcn, vdp, vdi, vds)
	int			fcn;
	register struct vddrv	*vdp;
	caddr_t			vdi;
	struct vdstat		*vds;	/* only meaningful for VDSTAT */
{
	int	status = 0;

	switch (fcn) {

	case VDLOAD:
		vdp->vdd_vdtab = (struct vdlinkage *)&memtestdrv;
		break;

	case VDUNLOAD:
		break;

	case VDSTAT:
		break;

	default:
		printf("memtest_init: unknown function 0x%x\n", fcn);
		status = EINVAL;
	}

	return status;
}

memtest_open(dev)
	dev_t	dev;
{
	if (minor(dev) != 0)
		return (ENXIO);

	return 0;
}

int
memtest_ioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	caddr_t uaddr = *(caddr_t *)data;

	switch (cmd) {

	case MEMTESTBADPAR:
		return memtest_badpar_u(uaddr, 0);

	case MEMTESTBADPAREXP:
		return memtest_badpar_u(uaddr, 1);

	case MEMTESTBADFETCH:
		return memtest_badfetch();

	case MEMTESTBADLOAD:
		return memtest_badload();

	case MEMTESTBADSTORE:
		return memtest_badstore(uaddr);

	case MEMTESTBADLOADT:
		return memtest_badloadt(uaddr);

	case MEMTESTBADFETCHT:
		return memtest_badfetcht(uaddr);

	case  MEMTESTPEEKPOKE:
		return memtest_peekpoke((struct memtest_peekpoke *) data);

	default:
		return (EINVAL);
	}
}

/*ARGSUSED*/
memtest_mmap(dev, off, prot)
	dev_t dev;
	off_t off;
{
	struct pte	pte;
	extern int	physpages;

	*(unsigned int *)&pte = 0;
	pte.pg_type = PGT_OBMEM;
	pte.pg_pfnum = ~0;
	return (*(unsigned int *)&pte);
}

/*
 * make sure uaddr is valid.
 * fault it in, if necessary.
 */
int
memtest_badpar_u(uaddr, exp_flag)
	caddr_t	uaddr;
	int	exp_flag;	/* if exp_flag=1 use expansion parity reg */
{
	register int	c;

	if ((c = fubyte_nc(uaddr)) == -1)	/* Is page accessable? */
		return (EFAULT);	/* (this also faults it in) */

	return (memtest_badpar(uaddr, c, exp_flag));
}

/*
 * fetch a byte from user space with the cache off
 */
int
fubyte_nc(addr)
	caddr_t addr;
{
	int	oldvac = vac;
	int	s, x;
	struct pte	pte;

	mmu_getpte(addr, &pte);

	if (pte_valid(&pte)) {
		if (oldvac) {
			s = spl8();
			off_enablereg(ENA_CACHE);
			vac = 0;
			splx(s);
		}
		x = fubyte(addr);
		if (oldvac) {
			s = spl8();
			vac_flushall();	/* clear the cache */
			on_enablereg(ENA_CACHE);
			vac = 1;
			splx(s);
		}
	} else
		x = fubyte(addr);

	return (x);
}

/* static data used by memtest_badload() */
int	memtest_data = 0;

int
memtest_badload()
{
	register int	c, d;
	caddr_t addr;
	int exp_flag=0;

	c = memtest_data;
	addr = (caddr_t)&memtest_data;
	memtest_badpar(addr, c, exp_flag);
	memtest_badpar(addr+2, c, exp_flag);
	printf("memtest: wrote bad parity to 0x%x and 0x%x\n", addr, addr+2);
	printf("memtest: about to load from 0x%x\n", &memtest_data);
	d = memtest_data;		/* should panic here */

	return (c ^ d);
}

/* function used by memtest_badfetch */
extern int memtest_retl();

int
memtest_badfetch()
{
	register int	c, d;
	int	exp_flag=0;

	c = *(u_char *)memtest_retl;
	memtest_badpar(memtest_retl, c, exp_flag);
	printf("memtest: about to fetch from 0x%x\n", memtest_retl);
	d = memtest_retl(c);		/* should panic here */
	*(char *)memtest_retl = c;
	return (d ^ c);

}

int
memtest_badstore(uaddr)
	caddr_t uaddr;
{

	printf("memtest: about to store to 0x%x\n", uaddr);
	if (subyte(uaddr, 0) == -1)
		return (EFAULT);
	
	return 0;
}

int
memtest_badloadt(uaddr)
	caddr_t uaddr;
{

	printf("memtest: about to load from 0x%x\n", uaddr);
	if (fubyte(uaddr) == -1)
		return (EFAULT);
	
	return 0;
}

int
memtest_badfetcht(uaddr)
	caddr_t uaddr;
{


	/*
	 * Make sure we have a mapping; can't use on_fault() becuase
	 * that only works for kernel data faults, not kernel text faults.
	 */
	if (pagefault(uaddr, F_INVAL, S_READ, 0) != 0)
		return (EFAULT);
	printf("memtest: about to fetch from 0x%x\n", uaddr);
	((int (*)())uaddr)();

	return 0;
}

extern int peek(), peekc(), peekl(), poke(), pokec(), pokel();

/*
 * Invoke peek* and poke* routines with the user's bad-map address.
 */
int
memtest_peekpoke(ppp)
	struct memtest_peekpoke *ppp;
{
	caddr_t uaddr = ppp->paddr;
	long dummy;

	/*
	 * Make sure we have a mapping; peek and poke use nofault, not
	 * lofault.
	 */
	if (pagefault(uaddr, F_INVAL, S_WRITE, 0) != 0)
		return (EFAULT);
	printf("memtest: testing peeks from 0x%x\n", uaddr);
	ppp->error[0] = peekc(uaddr);
	ppp->error[1] = peek(uaddr);
	ppp->error[2] = peekl(uaddr, &dummy);
	printf("memtest: testing pokes to 0x%x\n", uaddr);
	ppp->error[3] = pokec(uaddr, (char) 0);
	ppp->error[4] = poke(uaddr, (short) 0);
	ppp->error[5] = pokel(uaddr, (long) 0);

	return 0;
}

/*
 * Write a byte with bad parity to an address.
 * This even works if the page isn't writeable, as we modify the pte.
 * (We modify the pte because we want the write to be transparent; we
 * want to be able to generate parity errors without otherwise setting
 * the modify bit, so we can test parity recovery.)
 */
int
memtest_badpar(addr, c, exp_flag)
	caddr_t	addr;
	register int	c;
	int 	exp_flag;	/* if exp_flag=1 use expansion par reg */
{
	int	s;
	register u_int	saveper;
	struct pte	pte;
	register int	err = 0;

	printf("memtest: about to store 0x%x with bad parity to 0x%x\n",
		c, addr);

	s = spl8();
	mmu_getpte(addr, &pte);		/* Save pte */
	if (pte.pg_v) {
		struct pte	 tpte;

		tpte = pte;
		tpte.pg_prot = KW;
		mmu_setpte(addr, tpte);		/* Writeable for us */
		vac_pageflush(addr);		/* keep cache consistent */
		if (exp_flag == 0) 
			saveper = MEMERR->me_per;
		else 
			saveper = MEMERR2->me_per;
		disable_dvma();		/* Don't want dvma w/ bad parity! */
		if (exp_flag == 0) 
			MEMERR->me_per = PER_TEST;
		else 
			MEMERR2->me_per = PER_TEST;
		*(addr) = c;			/* write bad parity */
		if (exp_flag == 0) 
			MEMERR->me_per = saveper;
		else 
			MEMERR2->me_per = saveper;
		enable_dvma();		/* Safe to turn dvma back on */
		mmu_setpte(addr, pte);
		vac_pageflush(addr);		/* keep cache consistent */
	} else
		err = EFAULT;
	splx(s);
	vac_flush(addr, 1);		/* flush it from the cache */

	return err;
}

/*
 * Note: loadable device drivers are type 407; the text is read-write.
 * Otherwise, we would have had to place this in a separate assembler
 * file.
 */
int
memtest_retl(c)
	int	c;
{
	return c;
}
