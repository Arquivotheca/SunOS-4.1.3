#ifndef lint
static	char sccsid[] = "@(#)memerr.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/user.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/memerr.h>
#include <machine/eccreg.h>
#include <machine/enable.h>
#include <machine/intreg.h>

#include <vm/hat.h>
#include <vm/page.h>
#include <vm/seg.h>

parity_hard_error = 0;
/*
 * Memory error handling for various sun4s
 */
memerr_init()
{
	switch (cpu) {
	case CPU_SUN4_110:
		memerr_init_parity();
		break;
	case CPU_SUN4_330:
		memerr_init330();
		break;
	case CPU_SUN4_260:
		memerr_init260();
		break;
	case CPU_SUN4_470:
		memerr_init470();
		break;
	}
}

/*
 * Handle parity/ECC memory errors.
 * XXX - Fix ECC handling
 */
memerr()
{
	u_int		    err, vaddr;

	err = MEMERR->me_err;	/* read the error bits */
	switch (cpu) {
	case CPU_SUN4_110:
		parity_error(err);
		break;

	case CPU_SUN4_330:
		vaddr = MEMERR->me_vaddr;
		memlog_330((int) vaddr, err);
		clear_parity_err((addr_t) vaddr);
		panic("Aynchronous parity error - DVMA operation");
		/* NOTREACHED */

	case CPU_SUN4_260:
	case CPU_SUN4_470:
		ecc_error(err);
		break;
	default:
		printf("memory error handler: unknown CPU\n");
		break;
	}
}


#if defined(SUN4_110)
parity_error(per)
	u_int		    per;
{
	char		   *mess = 0;
	struct ctx	   *ctx;
	struct pte	    pme;
	extern struct ctx  *ctxs;

	per = MEMERR->me_err;

	/*
	 * Since we are going down in
	 * flames, disable further
	 * memory error interrupts to
	 * prevent confusion.
	 */
	MEMERR->me_err &= ~ER_INTENA;

	if ((per & PER_ERR) != 0) {
		printf("Parity Error Register %b\n", per, PARERR_BITS);
		mess = "parity error";
	}
	if (!mess) {
		printf("Memory Error Register %b\n", per, ECCERR_BITS);
		mess = "unknown memory error";
	}
	printf("DVMA = %x, context = %x, virtual address = %x\n",
	    (per & PER_DVMA) >> PER_DVMA_SHIFT,
	    (per & PER_CTX) >> PER_CTX_SHIFT,
	    MEMERR->me_vaddr);

	ctx = mmu_getctx();

	mmu_setctx(&ctxs[(per & PER_CTX) >> PER_CTX_SHIFT]);
	mmu_getpte((caddr_t) MEMERR->me_vaddr, &pme);
	printf("pme = %x, physical address = %x\n", *(int *) &pme,
	    ptob(((struct pte *) & pme)->pg_pfnum)
	    + (MEMERR->me_vaddr & PGOFSET));

	/*
	 * print the U-number of the
	 * failure
	 */
	if (cpu == CPU_SUN4_110)
		pr_u_num110((caddr_t) MEMERR->me_vaddr, per);

	mmu_setctx(ctx);

	/*
	 * Clear the latching by
	 * writing to the top nibble of
	 * the memory address register
	 */
	MEMERR->me_vaddr = 0;

	panic(mess);
	/* NOTREACHED */
}

memerr_init_parity()
{
	MEMERR->me_per = PER_INTENA | PER_CHECK;
}

#else
memerr_init_parity()
{
}

parity_error(per)
	u_int		    per;
{
}

#endif					/* SUN4_100 */


#ifdef SUN4_110
/*
 * print the U-number(s) of the failing memory location(s).
 */
pr_u_num110(virt_addr, per)
	caddr_t		    virt_addr;
	u_int		    per;
{
	int		    bank;
	int		    bits;
	int		    u_num_offs = 0;

	static char	   *u_num[] = {
		"U1503", "U1502", "U1501", "U1500",
		"U1507", "U1506", "U1505", "U1504",
		"U1511", "U1510", "U1509", "U1508",
		"U1515", "U1514", "U1513", "U1512",
		"U1603", "U1602", "U1601", "U1600",
		"U1607", "U1606", "U1605", "U1604",
		"U1611", "U1610", "U1609", "U1608",
		"U1615", "U1614", "U1613", "U1612"
	};

	bank = bank_num(virt_addr);
	if (bank == -1) {
		printf("\nNo U-number can be calculated for this ");
		printf("memory configuration.\n\n");
		return;
	}
	if ((*romp->v_memorysize == 4 * 1024 * 1024) ||
	    (*romp->v_memorysize == 16 * 1024 * 1024)) {
		u_num_offs = 16;
	}
	if ((bits = ((per & PER_ERR) >> 1)) > 2)
		bits = 3;
	printf(" simm = %s\n", u_num[4 * bank + bits + u_num_offs]);
}

bank_num(virt_addr)
	caddr_t		    virt_addr;
{
	u_long		    phys_addr;
	int		    bank = -1;
	int		    temp;
	int		    memsize = *romp->v_memorysize;
	u_int		    map_getpgmap();

	memsize = memsize >> 20;
	phys_addr = map_getpgmap((addr_t) virt_addr);

	switch (memsize) {
	case 16:
		bank = (phys_addr & 0x3) ^ 0x3;
		break;
	case 8:
	case 32:
		bank = (phys_addr & 0x7) ^ 0x7;
		break;
	case 20:
		temp = (phys_addr & 0x3);	/* need bits 14:13 and 24 */
		bank = (((phys_addr >> 9) & 0x4) | temp) ^ 0x7;
		break;
	default:
		/*
		 * no U-number can be
		 * calculated for this
		 * memory size.
		 */
		bank = -1;
		break;
	}

	return (bank);
}

#else
/*ARGSUSED*/
pr_u_num110(virt_addr, per)
	caddr_t		    virt_addr;
	u_int		    per;
{
}

#endif					/* SUN4_110 */

#if defined(SUN4_260) || defined(SUN4_470)
/*
 * Since there is no implied ordering of the memory cards, we store
 * a zero terminated list of pointers to eccreg's that are active so
 * that we only look at existent memory cards during softecc() handling.
 */
union eccreg	   *ecc_alive[MAX_ECC + 1];

int		    prtsoftecc = 1;
extern int	    noprintf;
int		    memintvl = MEMINTVL;

ecc_error(err)
	u_int		    err;
{
	char		   *mess = 0;
	struct ctx	   *ctx;
	struct pte	    pme;
	extern struct ctx  *ctxs;

	if ((err & EER_ERR) == EER_CE) {
		MEMERR->me_err = ~EER_CE_ENA & 0xff;
		switch (cpu) {
		case CPU_SUN4_260:
			softecc_260();
			break;
		case CPU_SUN4_470:
			softecc_470();
			break;
		default:
			printf("ecc handler: unknown CPU\n");
		}
		return;
	}
	/*
	 * Since we are going down in
	 * flames, disable further
	 * memory error interrupts to
	 * prevent confusion.
	 */
	MEMERR->me_err &= ~ER_INTENA;

	if ((err & EER_ERR) != 0) {
		printf("Memory Error Register %b\n", err, ECCERR_BITS);
		if (err & EER_TIMEOUT)
			mess = "memory timeout error";
		if (err & EER_UE)
			mess = "uncorrectable ECC error";
		if (err & EER_WBACKERR)
			mess = "writeback error";
	}
	if (!mess) {
		printf("Memory Error Register %b\n", err, ECCERR_BITS);
		mess = "unknown memory error";
	}
	printf("DVMA = %x, context = %x, virtual address = %x\n",
	    (MEMERR->me_err & EER_DVMA) >> EER_DVMA_SHIFT,
	    (MEMERR->me_err & EER_CTX) >> EER_CTX_SHIFT,
	    MEMERR->me_vaddr);

	ctx = mmu_getctx();
	mmu_setctx(&ctxs[(MEMERR->me_err & EER_CTX) >> EER_CTX_SHIFT]);
	mmu_getpte((caddr_t) MEMERR->me_vaddr, &pme);
	printf("pme = %x, physical address = %x\n", *(int *) &pme,
	    ptob(((struct pte *) & pme)->pg_pfnum)
	    + (MEMERR->me_vaddr & PGOFSET));
	mmu_setctx(ctx);

	/*
	 * Clear the latching by
	 * writing to the top nibble of
	 * the memory address register
	 */
	MEMERR->me_vaddr = 0;

	/*
	 * turn on interrupts, else sync will not go through
	 */
	set_intreg(IR_ENA_INT, 1);

	panic(mess);
	/* NOTREACHED */
}

#else
/*ARGSUSED*/
ecc_error(err)
	u_int		    err;
{
}

#endif					/* SUN4_260 || SUN4_470 */


#if defined(SUN4_260)
struct {
	u_char		    m_syndrome;
	char		    m_bit[3];
}		    memlogtab[] = {

	0x01, "64", 0x02, "65", 0x04, "66", 0x08, "67", 0x0B, "30", 0x0E, "31",
	0x10, "68", 0x13, "29", 0x15, "28", 0x16, "27", 0x19, "26", 0x1A, "25",
	0x1C, "24", 0x20, "69", 0x23, "07", 0x25, "06", 0x26, "05", 0x29, "04",
	0x2A, "03", 0x2C, "02", 0x31, "01", 0x34, "00", 0x40, "70", 0x4A, "46",
	0x4F, "47", 0x52, "45", 0x54, "44", 0x57, "43", 0x58, "42", 0x5B, "41",
	0x5D, "40", 0x62, "55", 0x64, "54", 0x67, "53", 0x68, "52", 0x6B, "51",
	0x6D, "50", 0x70, "49", 0x75, "48", 0x80, "71", 0x8A, "62", 0x8F, "63",
	0x92, "61", 0x94, "60", 0x97, "59", 0x98, "58", 0x9B, "57", 0x9D, "56",
	0xA2, "39", 0xA4, "38", 0xA7, "37", 0xA8, "36", 0xAB, "35", 0xAD, "34",
	0xB0, "33", 0xB5, "32", 0xCB, "14", 0xCE, "15", 0xD3, "13", 0xD5, "12",
	0xD6, "11", 0xD9, "10", 0xDA, "09", 0xDC, "08", 0xE3, "23", 0xE5, "22",
	0xE6, "21", 0xE9, "20", 0xEA, "19", 0xEC, "18", 0xF1, "17", 0xF4, "16",
};

memerr_init260()
{
	register union eccreg **ecc_nxt;
	register union eccreg *ecc;

	/*
	 * Map in ECC error registers.
	 */
	map_setpgmap((addr_t) ECCREG,
	    PG_V | PG_KW | PGT_OBIO | btop(OBIO_ECCREG0_ADDR));

	/*
	 * Go probe for all memory
	 * cards and perform
	 * initialization. The address
	 * of the cards found is
	 * stashed in ecc_alive[]. We
	 * assume that the cards are
	 * already enabled and the base
	 * addresses have been set
	 * correctly by the monitor.
	 * Memory error interrupts will
	 * not be enabled until we take
	 * over the console (consconfig
	 * -> stop_mon_clock).
	 */
	ecc_nxt = ecc_alive;
	for (ecc = ECCREG; ecc < &ECCREG[MAX_ECC]; ecc++) {
		if (peekc((char *) ecc) == -1)
			continue;
		MEMERR->me_err = 0;	/* clear intr from mem register */
		ecc->ecc_s.syn |= SY_CE_MASK;	/* clear syndrom fields */
		ecc->ecc_s.ena |= ENA_SCRUB_MASK;
		ecc->ecc_s.ena |= ENA_BUSENA_MASK;
		*ecc_nxt++ = ecc;
	}
	*ecc_nxt = (union eccreg *) 0;	/* terminate list */
}

/*
 * Routine to turn on correctable error reporting.
 */
ce_enable(ecc)
	register union eccreg *ecc;
{
	ecc->ecc_s.ena |= ENA_BUSENA_MASK;
}

/*
 * Probe memory cards to find which one(s) had ecc error(s).
 * If prtsoftecc is non-zero, log messages regarding the failing
 * syndrome.  Then clear the latching on the memory card.
 */
softecc_260()
{
	register union eccreg **ecc_nxt, *ecc;

	for (ecc_nxt = ecc_alive; *ecc_nxt != (union eccreg *) 0; ecc_nxt++) {
		ecc = *ecc_nxt;
		if (ecc->ecc_s.syn & SY_CE_MASK) {
			if (prtsoftecc) {
				noprintf = 1;	/* (not on the console) */
				memlog_260(ecc);	/* log the error */
				noprintf = 0;
			}
			ecc->ecc_s.syn |= SY_CE_MASK;	/* clear latching */
			/*
			 * disable
			 * board
			 */
			ecc->ecc_s.ena &= ~ENA_BUSENA_MASK;
			timeout(ce_enable, (caddr_t) ecc, memintvl * hz);
		}
	}
}

memlog_260(ecc)
	register union eccreg *ecc;
{
	register int	    i;
	register u_char	    syn;
	register u_int	    err_addr;
	int		    unum;

	syn = (ecc->ecc_s.syn & SY_SYND_MASK) >> SY_SYND_SHIFT;
	/*
	 * Compute board offset of
	 * error by subtracting the
	 * board base address from the
	 * physical error address and
	 * then masking off the extra
	 * bits.
	 */
	err_addr = ((ecc->ecc_s.syn & SY_ADDR_MASK) << SY_ADDR_SHIFT);
	if (ecc->ecc_s.ena & ENA_TYPE_MASK)
		err_addr -=
		    (ecc->ecc_s.ena & ENA_ADDR_MASKL) << ENA_ADDR_SHIFTL;
	else
		err_addr -= (ecc->ecc_s.ena & ENA_ADDR_MASK) << ENA_ADDR_SHIFT;
	if ((ecc->ecc_s.ena & ENA_BDSIZE_MASK) == ENA_BDSIZE_8MB)
		err_addr &= MEG8;	/* mask to full address */
	if ((ecc->ecc_s.ena & ENA_BDSIZE_MASK) == ENA_BDSIZE_16MB)
		err_addr &= MEG16;	/* mask to full address */
	if ((ecc->ecc_s.ena & ENA_BDSIZE_MASK) == ENA_BDSIZE_32MB)
		err_addr &= MEG32;	/* mask to full address */
	printf("mem%d: soft ecc addr %x syn %b ",
	    ecc - ECCREG, err_addr, syn, SYNDERR_BITS);
	for (i = 0; i < (sizeof memlogtab / sizeof memlogtab[0]); i++)
		if (memlogtab[i].m_syndrome == syn)
			break;
	if (i < (sizeof memlogtab / sizeof memlogtab[0])) {
		printf("%s ", memlogtab[i].m_bit);
		/*
		 * Compute U number on
		 * board, first figure
		 * out which half is
		 * it.
		 */
		if (atoi(memlogtab[i].m_bit) >= LOWER) {
			if ((ecc->ecc_s.ena & ENA_TYPE_MASK) == TYPE0) {
				switch (err_addr & ECC_BITS) {
				case U14XX:
					unum = 14;
					break;
				case U16XX:
					unum = 16;
					break;
				case U18XX:
					unum = 18;
					break;
				case U20XX:
					unum = 20;
					break;
				}
			} else {
				switch (err_addr & PEG_ECC_BITS) {
				case U14XX:
					unum = 14;
					break;
				case U16XX:
					unum = 16;
					break;
				case PEG_U18XX:
					unum = 18;
					break;
				case PEG_U20XX:
					unum = 20;
					break;
				}
			}
		} else {
			if ((ecc->ecc_s.ena & ENA_TYPE_MASK) == TYPE0) {
				switch (err_addr & ECC_BITS) {
				case U15XX:
					unum = 15;
					break;
				case U17XX:
					unum = 17;
					break;
				case U19XX:
					unum = 19;
					break;
				case U21XX:
					unum = 21;
					break;
				}
			} else {
				switch (err_addr & PEG_ECC_BITS) {
				case U15XX:
					unum = 15;
					break;
				case U17XX:
					unum = 17;
					break;
				case PEG_U19XX:
					unum = 19;
					break;
				case PEG_U21XX:
					unum = 21;
					break;
				}
			}
		}
		printf("U%d%s\n", unum, memlogtab[i].m_bit);
	} else
		printf("No bit information\n");
}

atoi(num_str)
	register char	   *num_str;
{
	register int	    num;

	num = (*num_str++ & 0xf) * 10;
	num = num + (*num_str & 0xf);
	return (num);
}

#else
/*ARGSUSED*/
softecc_260(err)
	u_int		    err;
{
}

memerr_init260()
{
}

#endif					/* SUN4_260 */


#ifdef SUN4_470
/*
 * Moonshine memory boards have 4 banks of 72 bits per line,
 * what follows is the mapping of the syndrome bits to U numbers
 * for bank 0.	The 64 data bits d[0:63] are followed by the 8
 * syndrome/check bits s[0:7] or SX, S0, S1, S2, S4, S8, S16, S32.
 *
 * The U numbers for other banks can be calculated by multiplying the bank
 * number by 200 and adding to the bank zero U numbers given below:
 */

/* bit number to u number mapping */
unsigned char u470[72] = {
	4, 5, 6, 7, 8, 9, 10, 11,			/* bits 0-7 */
	12, 13, 14, 15, 16, 17, 18, 19,			/* bits 8-15 */
	104, 105, 106, 107, 108, 109, 110, 111,		/* bits 16-23 */
	112, 113, 114, 115, 116, 117, 118, 119,		/* bits 24-31 */
	20, 21, 22, 23, 24, 25, 26, 27,			/* bits 32-39 */
	28, 29, 30, 31, 32, 33, 34, 35,			/* bits 40-47 */
	120, 121, 122, 123, 124, 125, 126, 127,		/* bits 48-55 */
	128, 129, 130, 131, 132, 133, 134, 135,		/* bits 56-63 */
	0, 1, 2, 3, 100, 101, 102, 103			/* check bits */
};

/* syndrome to bit number mapping */
unsigned char syntab[] = {
	0xce, 0xcb, 0xd3, 0xd5, 0xd6, 0xd9, 0xda, 0xdc,	/* bits 0-7 */
	0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x31, 0x34,	/* bits 8-15 */
	0x0e, 0x0b, 0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c, /* bits 16-23 */
	0xe3, 0xe5, 0xe6, 0x69, 0xea, 0xec, 0xf1, 0xf4,	/* bits 24-31 */
	0x4f, 0x4a, 0x52, 0x54, 0x57, 0x58, 0x5b, 0x5d, /* bits 32-39 */
	0xa2, 0xa4, 0xa7, 0xa8, 0xab, 0x45, 0xb0, 0xb5, /* bits 40-47 */
	0x8f, 0x8a, 0x92, 0x94, 0x97, 0x98, 0x9b, 0x9d,	/* bits 48-55 */
	0x62, 0x64, 0x67, 0x68, 0x6b, 0x6d, 0x70, 0x75,	/* bits 56-63 */
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80	/* check bits */
};

memerr_init470()
{
	register union eccreg **ecc_nxt;
	register union eccreg *ecc;

	/*
	 * Map in ECC error registers.
	 */
	map_setpgmap((addr_t) ECCREG,
	    PG_V | PG_KW | PGT_OBIO | btop(OBIO_ECCREG1_ADDR));

	ecc_nxt = ecc_alive;
	for (ecc = ECCREG; ecc < &ECCREG[MAX_ECC]; ecc++) {
		MEMERR->me_err = 0;
		if (peekc((char *)ecc) == -1)
			continue;
		ecc->ecc_m.err = 0;		 /* clear error latching */
		ecc->ecc_m.err = 1;		 /* clear CE led */
		ecc->ecc_m.err = 2;		 /* clear UE led */
		ecc->ecc_m.syn = 0;		 /* clear syndrome fields */
		/*
		 * this is correct ordering for first time, the
		 * prom has already done this, paranoia
		 */
		ecc->ecc_m.ena |= MMB_ENA_ECC|MMB_ENA_BOARD|MMB_ENA_SCRUB;
		*ecc_nxt++ = ecc;
	}
	*ecc_nxt = (union eccreg *) 0;	/* terminate list */
	MEMERR->me_err = 0;
	MEMERR->me_err = EER_INTENA | EER_CE_ENA;
}

/*
 * turn on reporting of correctable ecc errors
 */
enable_ce_470(ecc)
	register union eccreg *ecc;
{
	if (prtsoftecc) {
		noprintf = 1;
		printf("resetting ecc handling\n");
		noprintf = 0;
	}
	ecc->ecc_m.err = 1;		/* reset latching, turn off CE led */
	MEMERR->me_err = EER_INTENA|EER_CE_ENA;	/* reenable ecc reporting */
}

softecc_470()
{
	register union eccreg **ecc_nxt, *ecc;

	for (ecc_nxt = ecc_alive; *ecc_nxt != (union eccreg *) 0; ecc_nxt++) {
		ecc = *ecc_nxt;
		if (ecc->ecc_m.err & MMB_CE_ERROR) {
			if (prtsoftecc) {
				noprintf = 1; /* not on the console */
				memlog_470(ecc);	/* log the error */
				noprintf = 0;
			}

			/*
			 * reset ecc errors in memory board
			 * 0 - enables latching of next ecc error
			 * 1 - enables latching of next ecc error and
			 *		clears CE led
			 * 2 - enables latching of next ecc error and
			 * 		clears UE led
			 */
			ecc->ecc_m.err = 0;

			/* disable ecc error reporting temporarily */
			MEMERR->me_err &= ~(EER_CE_ENA);

			/* turn ecc error reporting at a later time */
			timeout(enable_ce_470, (caddr_t)ecc, memintvl*hz);
		}
	}
}

static
memlog_470(ecc)
	register union eccreg *ecc;
{
	register int	    bit;
	register u_char	    syn;
	register u_int	    err_addr;
	register int	    bank;
	register int	    mask;

	bank = ffs((long)ecc->ecc_m.err & MMB_BANK) - 1;
	syn = (ecc->ecc_m.syn & (0xff << (8*bank))) >> (8*bank);
	switch ((ecc->ecc_m.ena & MMB_BDSIZ) >> MMB_BDSIZ_SHIFT) {
		case 1:
			mask = MMB_PA_128;
			break;	
		default:
		case 0:
			mask = MMB_PA_32;
			break;
	}
	err_addr = (ecc->ecc_m.err & mask) >> MMB_PA_SHIFT;
	printf("mem%d: soft ecc addr %x syn %b ",
		ecc - ECCREG, err_addr, syn, SYNDERR_BITS);
	for (bit = 0; bit < sizeof syntab; bit++)
		if (syntab[bit] == syn)
			break;
	if (bit < sizeof syntab) {
		printf("bit %d ", bit);
		printf("U%d\n", 2000 + u470[bit] + bank*200);
	} else
		printf("No bit information\n");
}

#else
memerr_init470()
{
}

softecc_470()
{
}

#endif					/* SUN4_470 */

memerr_init330()
{
	MEMERR->me_per = PER_INTENA | PER_CHECK;
}

#ifdef SUN4_330
int		    kill_on_parity = 1;

user_parity_recover(addr, errreg, rw)
	register addr_t	    addr;
	register unsigned   errreg;
	register enum seg_rw rw;
{
	struct pte	    pte;

	memlog_330((int) addr, errreg);
	mmu_getpte(addr, &pte);

	if (retry(addr, pte, 1) != -1) {	/* re-read succeeded */
		printf(" soft error, recovered \n");	/* continue */
		return (1);
	}
	/*
	 * parity error still exists,
	 * try recovery
	 */
	if (!(pte.pg_v)) {	/* paranoia checks */
		panic("synchrnous parity error - user invalid page ");
	}
	if (!(pte.pg_m) && (u.u_exdata.ux_mag == ZMAGIC)) {
		if (page_giveup(addr, pte)) {
			/*
			 * fault in the
			 * page right
			 * away
			 */
			if (pagefault(addr, F_INVAL, rw, 0) == 0)
				printf(" RECOVERED FROM PARITY ERROR \n");
			else
				panic("parity error: lost page");
		} else {
			panic("parity error: no page");
			/* NOT REACHED */
		}
		if (parity_hard_error == 2)
			panic("Many Hard parity errors, REPLACE SIMMS");
		/* NOT REACHED */
	} else if (kill_on_parity) {
		printf("CANNOT RECOVER: User process %d killed\n",
		    u.u_procp->p_pid);
		psignal(u.u_procp, SIGBUS);
	} else {
		panic("synchrnous parity error - user mode");
		/* NOT REACHED */
	}
	MEMERR->me_vaddr = 0;	/* clear the me_vaddr and cntl again */
	return (1);
}

/* zero is not really a valid index into the next two arrays */
static int	    xb2b[] = {
	8 << 20, 96 << 20, 64 << 20, 48 << 20,
	72 << 20, 40 << 20, 24 << 20, 16 << 20};
static int	    xb1s[] = {
	22, 24, 24, 22, 24, 24, 22, 22};

static char	    simerrfmt[] =
	"Memory error in SIMM U%d on %s card\n";
static char	    simerrsfmt[] =
	"Memory error somewhere in SIMMs U%d through U%d on %s card\n";
static char	    c_cpu[] = "CPU";
static char	    c_9u[] = "9U Memory";
static char	    c_3u1[] = "First 3U Memory";
static char	    c_3u2[] = "Second 3U Memory";

#define	SIMERR(s, u)		printf(simerrfmt, u, s)
#define	SIMERRS(s, u, v)	printf(simerrsfmt, u, v, s)

static void
simlog_330(errreg, board, simno)
	unsigned	    errreg;
	char		   *board;
	int		    simno;
{
	/*
	 * If PA0 and PA1 are zeros,
	 * access goes to the third
	 * simm in the row, i.e. U703
	 * and so on.
	 */
	if (!(errreg & PER_ERR))
		SIMERRS(board, simno, simno + 3);
	if (errreg & PER_ERR00)
		SIMERR(board, simno);
	if (errreg & PER_ERR08)
		SIMERR(board, simno + 1);
	if (errreg & PER_ERR16)
		SIMERR(board, simno + 2);
	if (errreg & PER_ERR24)
		SIMERR(board, simno + 3);
}

memlog_330(vaddr, errreg)
	int		    vaddr;
	unsigned	    errreg;
{
	int		    paddr;	/* 32-bit physical address of error */
	int		    conf;	/* memory configuration code */
	int		    obmsize;	/* bytes of onboard memory */
	int		    obmshft;	/* shift factor for for cpu card */
	int		    x9ushft;	/* shift factor for 9U mem card */
	int		    shft;	/* shift factor for 3U mem board */
	int		    base;	/* base address of "this" board */
	char		   *c_3u;	/* which 3U card is involved */

	paddr = ((map_getpgmap((addr_t) vaddr) & 0x7FFFF) << 13) |
	    (vaddr & 0x1FFF);

	printf("Parity Error: Physical Address 0x%x (Virtal Address 0x%x) Error Register %b\n",
	    paddr, vaddr, errreg,
	    "\020\001byte0\002byte1\003byte2\004byte3\005check\006test\007intena\010intr\011dvma");

	conf = read_confreg();
	obmshft = (conf & 1) ? 22 : 24; /* shift distance per bank */
	obmsize = 2 << obmshft; /* always 2 sets of 4 simms */
	conf = (conf & CONF) >> CONF_POS;	/* leave only xb1 config */

	if (paddr < obmsize) {
		/*
		 * Error is somewhere
		 * on the CPU board.
		 */
		simlog_330(errreg, c_cpu,
		    1300 + 4 * (paddr >> obmshft));
	} else {
		/*
		 * If we have a 9U
		 * card, the error is
		 * somewhere on it.
		 */
		if ((*romp->v_memorysize - obmsize) > (48 << 20))
			x9ushft = 24;
		else
			x9ushft = 22;

		if ((*romp->v_memorysize - obmsize) == (32 << 20)) {
			printf("If you have a 9U memory card populated with 1Meg SIMMs:\n");
			simlog_330(errreg, c_9u,
			    1600 + 100 * ((paddr - obmsize) >> (x9ushft + 1)) +
			    4 * ((paddr >> x9ushft) & 1));

			x9ushft = 24;
			printf("If you have a 9U memory card populated with 4Meg SIMMs:\n");
		} else
			printf("If you have a 9U memory card:\n");

		simlog_330(errreg, c_9u,
		    1600 + 100 * ((paddr - obmsize) >> (x9ushft + 1)) +
		    4 * ((paddr >> x9ushft) & 1));

		printf("If you DO NOT have a 9U memory card:\n");

		if (paddr < xb2b[conf]) {
			/*
			 * Error is on the first 3U card;
			 * set xb1s[2] to the "other" simm size
			 */
			xb1s[2] = (22 + 24) - obmshft;
			base = obmsize;
			shft = xb1s[conf];
			c_3u = c_3u1;
		} else {
			/*
			 * Error is on
			 * the second
			 * 3U card
			 */
			base = xb2b[conf];
			if ((*romp->v_memorysize - xb2b[conf]) > (16 << 20))
				shft = 24;
			else
				shft = 22;
			c_3u = c_3u2;
		}
		/*
		 * First 8(32)meg is
		 * low half of 3U card,
		 * Second 8(32)meg is
		 * high half of 3U
		 * card. Selection of
		 * the 4(16)meg row is
		 * done by PA22(24).
		 */
		printf("If you DO NOT have a 9U memory card:\n");
		simlog_330(errreg, c_3u,
		    700 + 100 * ((paddr - base) >> (shft + 1)) +
		    ((paddr >> shft) & 1));
	}
}

/*
 * retry()
 * See if we get another parity error at the same location
 * if soft is zero, write to the location and then read it back.
 * if soft is non zero, just read the location and check for an error
 */

int
retry(addr, pte, soft)
	addr_t		    addr;
	struct pte	    pte;
	int		    soft;
{
	unsigned long	    put = 0xaaaaaaaa;
	long		    retval = 0, vaddr;
	long		    taddr = 0;

	/*
	 * there may have been more
	 * than one byte parity error,
	 * test out a long word peek.
	 * Adjust the address to do
	 * that
	 */

	vaddr = (long) addr;
	while (vaddr & 0x3) {
		vaddr--;
		addr--;
	}
	if (vac)
		vac_flushone(addr);	/* always flush the line before use */

	if (!soft) {		/* if !soft, write to location then read */
		pte.pg_prot = KW;
		/*
		 * change prot, don't
		 * need to put it back.
		 * We have already
		 * given up the page.
		 */
		mmu_setpte(addr, pte);
		*(long *) addr = put;	/* stingray has a write through cache */
	}
	/* read and check for error */
	retval = peekl((long *) addr, (long *) &taddr);

	if (vac)
		vac_flushone(addr);

	if ((retval == -1) && !soft) {
		printf(" HARD Parity error \n");
		parity_hard_error++;
	}
	return (retval);

}

/*
 * handle kernel parity error
 * do a soft retry, i.e., just read again. If we get another parity
 * error go and panic. Else just continue executing. If we get another
 * parity error, clear the error before panic.
 */
kernel_parity_error(addr, errreg)
	register addr_t	    addr;
	register unsigned   errreg;
{
	struct pte	    pte;

	memlog_330((int) addr, errreg);	/* report error */
	mmu_getpte(addr, &pte);

	if (retry(addr, pte, 1) != -1)
		return;
	/*
	 * clear parity err else we
	 * might find ourselves with
	 * another parity error while
	 * dumping
	 */
	clear_parity_err(addr);

	panic(" synchronous parity error - kernel");	/* GIVE UP */
	/* NOT REACHED */
}

/*
 * clear parity error
 * Clear a kernel parity error if it is from kernel data space. Else during dump
 * we get one more.
 */
clear_parity_err(addr)
	register addr_t	    addr;
{
	struct pte	    pte;

	mmu_getpte(addr, &pte);
	vac_flushone(addr);
	if (pte.pg_prot == KW)	/* if writable */
		*addr = 0;	/* clear the parity error */
	vac_flushone(addr);
}

#endif					/* SUN4_330 */
