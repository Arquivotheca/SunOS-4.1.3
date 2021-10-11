#ident "@(#)memerr.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/vnode.h>

#include <sun/fault.h>

#include <machine/buserr.h>
#include <machine/intreg.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/memerr.h>
#include <machine/enable.h>
#include <machine/trap.h>

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/seg.h>
#include <vm/as.h>

extern struct seg * as_segat();
extern struct memlist *physmemory;
extern int memexp_flag; 

/* round address to VAC linesize */
#define	ROUND_ADDR(a, size)	((addr_t) ((int) (a) & -(size)))

/*
 * Memory error handling for various sun4cs
 */

memerr_init()
{
#ifndef	SAS
	/*
	 * All Sun4c's have parity memory
	 * (And if they don't, then base it on a property, not on cpuid
	 */
	MEMERR->me_per = PER_CHECK;

	/*
	 * turn on parity reporting for memory expansion if it's there
	 */
	if (memexp_flag) {
		MEMERR2->me_per = PER_CHECK;	
	}	
#endif	!SAS
}

/*
 * Handle synchronous memory errors and Campus-1 type asynchronous errors.
 * Mostly these should be parity errors, possibly during
 * dvma. However, there are some possible errors during cache write
 * cycles, due to the way writes are buffered.
 * The type parameter tell us whether the error was synchronous or not.
 * The reg field is the contents of the sync or async error reg, depending
 * on the type. Addr is the address from the appropriate va reg.
 * This routine applies to all sun4c implementations.
 * XXX perhaps should put async stuff in a different function?
 */
memerr(type, reg, addr, fault, rp)
	u_int type, reg;
	addr_t addr;
	unsigned fault;		/* only meaningful if type == MERR_SYNC */
	struct regs *rp;	/* ditto */
{
	u_int per;
	char *mess = 0;
	struct ctx *ctx;
	struct pte pme;

	ctx = mmu_getctx();
	mmu_getpte((caddr_t)addr, &pme);

	if (memexp_flag) {
		/* use phys addr of affected loc to key to correct parity reg */
		if ( (ptob(((struct pte *)&pme)->pg_pfnum) + 
			((u_int)addr & PGOFSET)) >= MEMEXP_START ) 
			per = MEMERR2->me_per; 
		else 
			per = MEMERR->me_per;
	} else 
		per = MEMERR->me_per;

	if (type == MERR_SYNC) {
		if ((per & PER_ERROR) != 0) {
			extern addr_t	parerr_addr();

			printf("Parity error reported at 0x%x, ", addr);
			addr = parerr_addr(addr);
			printf("actual address is 0x%x.\n", addr);
			if (addr == NULL)
				panic("parity error: addr NULL??");
			printf("Parity Error, ctx = 0x%x, virt addr = 0x%x\n",
			    ctx->c_num, addr);
			printf("pme = %x, phys addr = %x\n",
			    *(int *)&pme, ptob(((struct pte *)&pme)->pg_pfnum)
			    + ((u_int)addr & PGOFSET));
			printf("Parity Error Register %b\n", per, PARERR_BITS);
			pr_u_num(addr, per);
			if (parerr_recover(addr, per, &pme, fault, rp) == 0) {
				printf("System operation can continue\n");
				return;
			}
			printf("System operation cannot continue, ");
			printf("will test location anyway.\n");
			(void)parerr_reset(addr, &pme);
			mess = "parity error";
		} else {
			/* This is a "Shouldn't happen"!!! */
			printf("Non-parity Synchronous memory error, ");
			printf("ctx = 0x%x, virt addr = 0x%x\n",
			    ctx->c_num, addr);
			printf("pme = %x, phys addr = %x\n",
			    *(int *)&pme, ptob(((struct pte *)&pme)->pg_pfnum)
			    + ((u_int)addr & PGOFSET));
			printf("Sync Error Register %b\n", reg, SYNCERR_BITS);
			mess = "sync memory error";
		}
	} else {
		if (((reg & ASE_DVMAERR) != 0) && ((per & PER_ERROR) != 0)) {
			/*
			 * XXX Due to a bug in the cache chip, not
			 * XXX expected to be fixed anytime soon, the
			 * XXX address isn't sign-extended on DVMA
			 * XXX errors.  It is sign-extended on
			 * XXX asynchrounous IU errors.
			 */
			addr = (addr_t)ASEVAR_SIGNEXTEND((int)addr); /* XXX */

			/*
			 * Assume error addr is in all contexts!
			 */
			printf(
	"DVMA Parity Error, ctx = 0x%x, virt addr = 0x%x\n", 0, addr);
			printf("pme = %x, phys addr = %x\n",
			    *(int *)&pme, ptob(((struct pte *)&pme)->pg_pfnum)
			    + ((u_int)addr & PGOFSET));
			printf("Parity Error Register %b\n", per, PARERR_BITS);
			pr_u_num(addr, per);
			printf("System operation cannot continue, ");
			printf("will test location anyway.\n");
			(void)parerr_reset(addr, &pme);
			mess = "dvma parity error";
		} else {
			if (async_recover(addr, reg) == 0)
				return;
			printf(
	"Asynchronous memory error, ctx = 0x%x, virt addr = 0x%x\n",
			    ctx->c_num, addr);
			printf("pme = %x, phys addr = %x\n",
			    *(int *)&pme, ptob(((struct pte *)&pme)->pg_pfnum)
			    + ((u_int)addr & PGOFSET));
			printf("Async Error Register %b\n", reg, ASYNCERR_BITS);
			printf("Parity Error Register %b\n", per, PARERR_BITS);
			pr_u_num(addr, per);
			mess = "async memory error";
		}
		/*
		 * On Campus, locore.s has disabled interrupts before
		 * calling us.  We must re-enable before panic'ing in case we
		 * are going to dump across the ethernet, as nfs_dump requires
		 * that interrupts be enabled.
		 */
		set_intreg(IR_ENA_INT, 1);
	}
	panic(mess);
	/*NOTREACHED*/
}

/*
 * Handle asynchronous errors from Calvin-type machines.
 * These should be dvma parity errors.
 * However, there are some possible errors during cache write
 * cycles, due to the way writes are buffered.
 * This routine applies only to Calvin-type sun4c implementations.
 */
memerr_70(type, reg, addr, data1, data2)
	u_int type, reg;
	addr_t addr;
	u_int data1, data2;
{
	u_int per;
	char *mess = 0;
	struct ctx *ctx;
	struct pte pme;

	ctx = mmu_getctx();
	mmu_getpte((caddr_t)addr, &pme);

	if (memexp_flag) {
		/* use phys addr of affected loc to key to correct parity reg */
		if ( (ptob(((struct pte *)&pme)->pg_pfnum) + 
			((u_int)addr & PGOFSET)) >= MEMEXP_START ) 
			per = MEMERR2->me_per; 
		else 
			per = MEMERR->me_per;
	} else 
		per = MEMERR->me_per;

	if (type == MERR_SYNC) {
		panic("memerr_70: MERR_SYNC");
	} else {
		if (((reg & ASE_DVMAERR) != 0) && ((per & PER_ERROR) != 0)) {
			/*
			 * Assume error addr is in all contexts!
			 */
			printf(
	"DVMA Parity Error, ctx = 0x%x, virt addr = 0x%x\n", 0, addr);
			printf("pme = %x, phys addr = %x\n",
			    *(int *)&pme, ptob(((struct pte *)&pme)->pg_pfnum)
			    + ((u_int)addr & PGOFSET));
			printf("Async Error Register %b size = %d\n",
				reg, ASYNCERR_BITS_70,
				(reg & ASE_SIZ) >> ASE_SIZ_SHIFT);
			printf("Parity Error Register %b\n", per, PARERR_BITS);
			pr_u_num(addr, per);
			printf("System operation cannot continue, ");
			printf("will test location anyway.\n");
			(void)parerr_reset(addr, &pme);
			mess = "dvma parity error";
		} else {
			if (async_recover_70(addr, reg) == 0)
				return;
			printf(
	"Asynchronous memory error, ctx = 0x%x, virt addr = 0x%x\n",
			    ctx->c_num, addr);
			printf("pme = %x, phys addr = %x\n",
			    *(int *)&pme, ptob(((struct pte *)&pme)->pg_pfnum)
			    + ((u_int)addr & PGOFSET));
			printf("Async Error Register %b size = %d\n",
				reg, ASYNCERR_BITS_70,
				(reg & ASE_SIZ) >> ASE_SIZ_SHIFT);
			printf("Parity Error Register %b\n", per, PARERR_BITS);
			pr_u_num(addr, per);
			printf("Async Data Registers 0x%x 0x%x\n",
				data1, data2);
			mess = "async memory error";
		}
		/*
		 * On Campus, locore.s has disabled interrupts before
		 * calling us.  We must re-enable before panic'ing in case we
		 * are going to dump across the ethernet, as nfs_dump requires
		 * that interrupts be enabled.
		 */
		set_intreg(IR_ENA_INT, 1);
	}
	panic(mess);
	/*NOTREACHED*/
}

/*
 * Recover, if possible, from an asynchronous memory error.
 * If the address is in user's space, then sigbus the current user
 * process.
 */
async_recover_70(addr, aser)
	addr_t	addr;
	u_int	aser;
{
	register struct proc	*p;
	register struct as	*as;

	/* XXX If nofault is set do a nofault */

	/* XXX If lofault is set do a lofault */

	/* if pokefault set, do a pokefault */
	if (pokefault) {
		pokefault = 1;
		return (0);
	}

	/*
	 * If it's not a multiple error, and not a DVMA error,
	 * and the address is in the current user's space,
	 * then sigbus the user.
	 */
	if ((aser & ASE_DVMAERR) ||
	    ((aser & ASE_MULTIPLE) &&
	    (aser & ~(ASE_MULTIPLE | ASE_TIMEOUT | ASE_SBERR | ASE_SIZ))))
		return (-1);

	if ((p = u.u_procp) && (as = p->p_as) && as_segat(as, addr)) {
		psignal(p, SIGBUS);
		u.u_code = FC_HWERR;
		u.u_addr = addr;
		return (0);
	}

	return (-1);
}

/* This routine is specific to Campus-1 type sun4c's. */

int asyncerrdebug = 0;		/* Set to 1 to debug the hardware */
/*
 * Decipher Asynchronous errors
 * Due to bugs in the cache chip, we sometimes get an asynchronous error
 * with no bits on in the ASER.  This routine tries to construct an ASER
 * based on the SER.
 */
asyncerr(ser, sevar, aser, asevar)
	u_int	ser, aser;
	addr_t	sevar, asevar;
{
	u_int	fake_aser;
	int	die = 0;

	if (asyncerrdebug) {
		printf("asyncerr: ser=%b sevar=0x%x aser=%b asevar=0x%x\n",
			ser, SYNCERR_BITS, sevar, aser, ASYNCERR_BITS, asevar);
	}

	if (aser & ASE_ERRMASK) {
		/* ASER valid, so use it */
		memerr(MERR_ASYNC, aser, asevar, 0, (struct regs *) 0);
		return;
	}

	/*
	 * ASER is zero, so try to construct one from SER.
	 */
	fake_aser = 0;
	if (ser & SE_SBBERR)
		fake_aser |= ASE_WBACKERR;
	if (ser & SE_TIMEOUT)
		fake_aser |= ASE_TIMEOUT;
	if (!(ser & SE_MEMERR) ||
	    (ser & ~(SE_RW | SE_SBBERR | SE_TIMEOUT | SE_MEMERR))) {
		printf("asyncerr: unusual ser\n");
		die++;
	}
	if (asevar == sevar) {
		/*
		 * If these addresses are the same, then the cache chip
		 * thought that this was a synchronous error, and we should
		 * never have gotten here.
		 */
		printf("asyncerr: asevar == sevar\n");
		die++;
	}
	if (die) {
		printf(" ser=%b sevar=0x%x aser=%b asevar=0x%x\n",
			ser, SYNCERR_BITS, sevar, aser, ASYNCERR_BITS, asevar);
		/* Should we panic here? */
	}
	memerr(MERR_ASYNC, fake_aser, asevar, 0, (struct regs *) 0);
}

/* These routines are common to all sun4c implementations. */

/*
 * Determine the actual address of the parity error.
 * When the cache is on, and we take a parity error during a cache fill
 * operation, the address in the SEVAR is the address that triggered the
 * fill, and not necessarily the address of the bad word.
 */
addr_t
parerr_addr(addr)
	addr_t	addr;
{
	if (vac) {
		int s, i;

		s = spl8();	/* Don't want anyone else to run! */
		vac_control(0);

		addr = ROUND_ADDR(addr, vac_linesize);
		for (i = 0; i < vac_linesize; i++, addr++) {
			if (peekc(addr) == -1)
				goto found;
		}
		/* not found */
		addr = NULL;
found:
		/*
		 * Peek may have triggered another parity error, so we
		 * need to clear the register by reading it.
		 */
		if (memexp_flag) {
			i = MEMERR2->me_per;	
		}	
		i = MEMERR->me_per;

		vac_control(1);
		(void) splx(s);
	}

	return (addr);
}

/*
 * Patterns to use to determine if a location has a hard or soft parity
 * error.
 * The zero is also an end-of-list marker, as well as a pattern.
 */
static long parerr_patterns[] = {
	0xAAAAAAAA,	/* Alternating ones and zeroes */
	0x55555555,	/* Alternate the other way */
	0x01010101,	/* Walking ones ... */
	0x02020202,	/* ... four bytes at once ... */
	0x04040404,	/* ... from right to left */
	0x08080808,
	0x10101010,
	0x20202020,
	0x40404040,
	0x80808080,
	0x7f7f7f7f,	/* And now walking zeros, from left to right */
	0xbfbfbfbf,
	0xdfdfdfdf,
	0xefefefef,
	0xf7f7f7f7,
	0xfbfbfbfb,
	0xfdfdfdfd,
	0xfefefefe,
	0xffffffff,	/* All ones */
	0x00000000,	/* All zeroes -- must be last! */
};

/*
 * Reset a parity error so that we can continue operation.
 * Also, see if we get another parity error at the same location.
 * Return 0 if error reset, -1 if not.
 * We need to test all the words in a cache line, if cache is on.
 */
int
parerr_reset(addr, ppte)
	addr_t		 addr;
	struct pte	*ppte;
{
	int	retval;
	long	*bad_addr, *paddr;
	long	i, j;
	int	k;
	int	expflag;

	expflag = 0;
	if (memexp_flag) {
		if ( (ptob(ppte->pg_pfnum) + ((u_int)addr & PGOFSET))
			>= MEMEXP_START ) 
			expflag = 1;
	}

	/*
	 * We need to set the protections to make sure that we can write
	 * to this page.  Since we are giving up the page anyway, we
	 * don't worry about restoring the protections.
	 */
	ppte->pg_prot = KW;
	mmu_setpte(addr, *ppte);

	vac_flush(addr, 1);

	/*
	 * If the cache is on, make sure we reset all bad words in this
	 * cache-line image.
	 * XXX Unfortunately, we can't use parerr_addr because there's
	 * XXX no guarantee that resetting a bad word will fix it.  We have to
	 * XXX reset the entire cache-line image.
	 */
	if (vac) {
#ifdef notdef
		addr_t caddr = addr;

		while (caddr = parerr_addr(caddr))
			*(long *)((int)caddr & ~3) = 0;
#else notdef
		long *laddr;

		laddr = (long *) ROUND_ADDR(addr, vac_linesize);
		for (i = 0; i < vac_linesize; i += sizeof (*laddr))
			*laddr++ = 0;
#endif notdef
	}

	/*
	 * Test the word with successive patterns, to see if the parity
	 * error was an ephemeral event or a permanent problem.
	 */
	bad_addr =
	    (long *) ROUND_ADDR(addr, vac ? vac_linesize : sizeof (long));
	k = 0;
	retval = 0;
	do {
		paddr = parerr_patterns;
		do {
			i = *paddr++;
			*bad_addr = i;		/* store pattern */
			if ((peekl(bad_addr, &j) != 0) || (i != j)) {
				printf("parity error at %x with pattern %x.\n",
					bad_addr, i);
				/* clear parity register */
				if (expflag) 
					j = MEMERR2->me_per; 
				else
					j = MEMERR->me_per;
				retval++;
			}
			vac_flush((addr_t) bad_addr, 4);
		} while (i);
		bad_addr++;
		k += sizeof (*bad_addr);
	} while (vac && k < vac_linesize);

	/* Convert return value to what caller expects */
	if (retval)
		retval = -1;
	printf("parity error at %x is %s.\n",
		ptob(ppte->pg_pfnum) + ((u_int)addr & PGOFSET),
		retval ? "permanent" :
			"transient");
	return (retval);
}

/*
 * Recover from a parity error.  Returns 0 if successful, -1 if not.
 */
parerr_recover(vaddr, per, pte, type, rp)
	addr_t	vaddr;
	u_int per;
	struct	pte	*pte;
	unsigned	 type;
	struct regs	*rp;
{
	struct	page	*pp;

	/*
	 * If multiple parity errors, then can't do anything.
	 */
	if (per & PER_MULTI) {
		vac_flushall();	/* must flush cache! */
		printf("parity recovery: more than one error\n");
		return (-1);
	}

	/*
	 * Pages with no page structures or vnode are kernel pages.
	 * We cannot regenerate them, since there is no copy of
	 * the data on disk.
	 * And if the page was being used for i/o, we can't recover?
	 */
	pp = page_numtopp(pte->pg_pfnum);
	if (pp == (struct page *) 0) {
		printf("parity recovery: no page structure\n");
		return (-1);
	}
	if (pp->p_vnode == 0) {
		printf("parity recovery: no vnode\n");
		return (-1);
	}
	if (pp->p_intrans) {
		printf("parity recovery: i/o in progress\n");
		return (-1);
	}

	/*
	 * Sync all the page structures for this page, so that we get
	 * the bits for all the page structures, not just this one.
	 */
	hat_pagesync(pp);

	/*
	 * If the page was modified, then the disk version is out
	 * of date.  This means we have to kill all the processes
	 * that use the page.  We may find a kernel use of the page,
	 * which means we have to return an error indication so that
	 * the caller can panic.
	 *
	 * If the page wasn't modified, then check for a data fault from
	 * a load double instruction.  Some of those aren't recoverable.
	 */
	if (pp->p_mod) {
		if (hat_kill_procs(pp, vaddr) != 0) {
			return (-1);
		}
	} else if (type == T_DATA_FAULT) {
		u_int	inst;
		int	isuser = USERMODE(rp->r_ps);

		inst = isuser ? fuword((caddr_t)rp->r_pc) : *(u_int *)rp->r_pc;
		if (check_ldd(inst, vaddr) != 0) {
			if (isuser) {
				printf("pid %d killed: parity error on ldd\n",
					u.u_procp->p_pid);
				uprintf("pid %d killed: parity error on ldd\n",
					u.u_procp->p_pid);
				u.u_code = FC_HWERR;
				u.u_addr = vaddr;
				psignal(u.u_procp, SIGBUS);
			} else {
				printf("parity recovery: unrecoverable ldd\n");
				return (-1);
			}
		}
	}

	/*
	 * Give up the page.  When the fetch is retried, the page
	 * will be automatically faulted in.  Note that we have to
	 * kill the processes before we give up the page, or else
	 * the page will no longer exist in the processes' address
	 * space.
	 */
	page_giveup(vaddr, pte, pp);
	return (0);
}

/*
 * Check if this is a load double (alternate) instruction.
 * If we faulted on the second full word and the destination register
 * was also one of the source registers, we can't recover.
 */
check_ldd(inst, vaddr)
	register u_int inst;
	addr_t	vaddr;
{
	register u_int rd, rs1, rs2;
	register int immflg;

	if ((inst & 0xc1780000) != 0xc0180000)
		return (0);			/* not LDD(A) */

	rd =  (inst >> 25) & 0x1e;		/* ignore low order bit */
	rs1 = (inst >> 14) & 0x1f;
	rs2 =  inst & 0x1f;
	immflg = (inst >> 13) & 1;

	if (rd == rs1 || (immflg == 0 && rd == rs2)) {
		/*
		 * We faulted on a load double and the first destination
		 * register was also one of the source registers.
		 * If the address is on a double-word boundary, then the
		 * fault was on the first load and we haven't clobbered
		 * the register yet.
		 * If the address is 4 mod 8, then we have already
		 * loaded the first word, we clobbered the register, and
		 * we can't recover.
		 * And if it is anything else, something is wrong,
		 * anyway.
		 */

		if (((int)vaddr & 7) != 0)
			return (-1);	/* can't recover */
	}

	return (0);
}

/*
 * Recover, if possible, from an asynchronous memory error.
 * If the address is in user's space, then sigbus the current user
 * process.
 */
async_recover(addr, aser)
	addr_t	addr;
	u_int	aser;
{
	register struct proc	*p;
	register struct as	*as;

	/* XXX If nofault is set do a nofault */

	/* XXX If lofault is set do a lofault */

	/* if pokefault set, do a pokefault */
	if (pokefault) {
		pokefault = 1;
		return (0);
	}

	/*
	 * If it's a writeback error or a timeout, and the address is in
	 * the current user's space, then sigbus the user.
	 */
	if ((aser & (ASE_TIMEOUT | ASE_WBACKERR)) && (p = u.u_procp) &&
	    (as = p->p_as) && as_segat(as, addr)) {
		psignal(p, SIGBUS);
		u.u_code = FC_HWERR;
		u.u_addr = addr;
		return (0);
	}

	return (-1);
}

static char *decode60();

/*
 * print the U-number(s) of the failing memory location(s).
 */
static
pr_u_num(virt_addr, per)
	caddr_t virt_addr;
	u_int per;
{
	char * (*decode)();
	struct pte pte;
	int pfn = -1;
	int bit, bits;

#ifndef SAS
	decode = (char * (*)()) getprop(0, "get-unum", (int) decode60);
#else
	decode = decode60;
#endif !SAS

	mmu_getpte(virt_addr, &pte);
	if (!pte_valid(&pte)) {
		printf("No U-number can be calculated for this memory address");
		return;
	}
	pfn = pte.pg_pfnum;

	for (bits = per & PER_ERRS, bit = 0; bits; bits >>= 1, bit++) {
		if (bits & 1)
			printf(" bad module/chip at: %s\n",
			    (*decode)((pfn << PAGESHIFT) | ((int)virt_addr & (PAGESIZE - 4)) + bit, pte.pg_type));
	}
}

/*
 * Decode physical address into U-number (4/60 or 4/65 only).
 * ASSUMES only OBMEM.
 */
/*ARGSUSED*/
static char *
decode60(addrs, space)
	int addrs, space;
{
	int bank;

	static char *u_num[][4] = {
/*   address		byte 0	byte 1	byte 2	byte 3	*/
/* 0x00nnnnnn */	"U588",	"U587",	"U586",	"U585",
/* 0x01nnnnnn */	"U584",	"U591",	"U590",	"U589",
/* 0x02nnnnnn */	"U678",	"U676",	"U683",	"U677",
/* 0x03nnnnnn */	"U682",	"U681",	"U680",	"U679",
	};

	bank = 99;
	if ((cpu == CPU_SUN4C_60) || (cpu == CPU_SUN4C_65))
		bank = addrs >> (12 + PAGESHIFT);

	if (bank > 5)	/* invalid on 4/60 */
		return ("?");

	if (bank > 3)	/* expansion memory */
		return ("Memory expansion card");

	return (u_num[bank][addrs & 3]);
}

int
calc_memsize()
{
	struct memlist *memp;
	register int found;
#ifdef SAS
	extern int availmem;

	found = availmem;
#else SAS
	for (found = 0, memp = physmemory; memp; memp = memp->next)
		found += memp->size;
#endif SAS
	return (found);
}
