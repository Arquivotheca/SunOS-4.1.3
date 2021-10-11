#ident	"@(#)module_vik.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <machine/module.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/async.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <vm/seg.h>

/*
 * Support for modules based on the
 * TI VIKING memory management unit
 *
 * To enable this module, declare this near the top of module_conf.c:
 *
 *	extern void	vik_module_setup();
 *
 * and add this line to the module_info table in module_conf.c:
 *
 *	{ 0xF0000000, 0x00000000, vik_module_setup },
 */

extern u_int	swapl();

#ifndef	lint
extern void	vik_mmu_getasyncflt();
extern void	vik_mmu_chk_wdreset();
extern u_int	probe_pte_0();
extern void	vik_mmu_flushctx();
extern void	vik_mmu_flushrgn();
extern void	vik_mmu_flushseg();
extern void	vik_mmu_flushpage();
extern void	vik_mmu_flushpagectx();
extern u_int	vik_pte_rmw();
extern u_int	vik_pte_rmw_vc();
#ifdef	MULTIPROCESSOR
extern void	vik_pte_rmw_slave();
extern void	vik_pte_rmw_slave_vc();
#endif	MULTIPROCESSOR
#else	lint
void	vik_mmu_getasyncflt(){}
void	vik_mmu_chk_wdreset(){}
u_int	probe_pte_0(){}
void	vik_mmu_flushctx(){}
void	vik_mmu_flushrgn(){}
void	vik_mmu_flushseg(){}
void	vik_mmu_flushpage(){}
void	vik_mmu_flushpagectx(){}
u_int	vik_pte_rmw(p,a,o) u_int *p,a,o;
{ return swapl((*p&~a)|o,p);}
#endif
extern void	vik_mmu_log_module_err();
extern void	mxcc_mmu_log_module_err();
extern void	vik_mmu_print_sfsr();

extern u_int	vik_pte_offon();
extern void	vik_module_wkaround();
#ifdef	MULTIPROCESSOR
extern u_int  (*mp_pte_offon)();
#endif	MULTIPROCESSOR

#ifndef	MULTIPROCESSOR
#define	xc_attention()
#define	xc_dismissed()
#else	MULTIPROCESSOR
extern	xc_attention();
extern	xc_dismissed();
#endif	MULTIPROCESSOR

#ifdef	VAC
extern void	vik_vac_init();
#ifndef	lint
extern void	vik_cache_on();
extern int	mxcc_vac_parity_chk_dis();
#else	lint
void		vik_cache_on(){}
int		mxcc_vac_parity_chk_dis(){return 0;}
#endif	lint
#endif

extern int	do_pg_coloring;
extern int	use_cache;
extern int 	use_page_coloring;
extern struct modinfo	mod_info[];

int vik_rev_level = 0;		/*  Viking rev level, 0 is unused */

void
vik_vac_init()
{
	extern char *vac_mode;
	static u_int setbits = 0;
	static u_int clrbits = 0;

/* the following extern int are used for debugging purpose.
 * they can be removed later (defined in machep.c)
 */
	extern int use_vik_prefetch;
	extern int use_mxcc_prefetch;
	extern int use_table_walk;
	extern int use_store_buffer;
	extern int use_ic;
	extern int use_dc;
	extern int use_ec;
	extern int use_multiple_cmd;
	extern int use_rdref_only;
	extern int nmod;
#ifdef	MULTIPROCESSOR
	extern int okprocset;
#endif	MULTIPROCESSOR

/* initialize mxcc first
 * E$ should be enabled before D$ and I$
 */
	if (cache == CACHE_PAC_E) {
		if (use_cache && use_ec)
			setbits |= MXCC_CE;

		if (use_mxcc_prefetch)
			setbits |= MXCC_PF;
		else
			clrbits |= MXCC_PF;
/*
 * XXX- Temporary hack to get around the fact that we want to enable
 * MC for single processor systems, but cant enable it for all MP
 * systems because of a bug in the rev 1.4 MXCCs and below.
 *
 * If use_multiple_cmd set to zero, we wont enable it.  If its set
 * to one, and you are UP, then enable it.  If its set to one and
 * you are MP, dont set it.  For testing purposes, you can set the
 * value to two for MP and enable MC.
 */
		if (use_multiple_cmd) {
			if ((nmod == 1) ||
#ifdef	MULTIPROCESSOR
				(okprocset == 1) || 
#endif	MULTIPROCESSOR
				(use_multiple_cmd == 2)) {
					setbits |= MXCC_MC;
			} else {
				use_multiple_cmd = 0; /* Print the right mesg */
				clrbits |= MXCC_MC;
			}
		} else {
			clrbits |= MXCC_MC;
		}
		if (use_rdref_only)
			setbits |= MXCC_RC;
		else
			clrbits |= MXCC_RC;
		vik_mxcc_init_asm(clrbits, setbits);
		vac_mode = "SuperSPARC/SuperCache";
	} else
		vac_mode = "SuperSPARC";


	/* initialize viking */
	clrbits = 0;
	setbits = CPU_VIK_SE;

	if (use_cache) {
		if (use_ic)
			setbits |= CPU_VIK_IE;
		if (use_dc)
			setbits |= CPU_VIK_DE;
	}
	/* 3.0 vikings are not here yet. We know 1.2 and 2.X should not
	 * data prefetch turned on. Therefore turn off for these.
	 */
	if (use_vik_prefetch && (vik_rev_level != VIK_REV_1DOT2 && 
				(vik_rev_level != VIK_REV_2DOTX)))
		setbits |= CPU_VIK_PF;
	else {
		/* 
		 * clear the use_vik_prefetch flag in case this system is a
	 	 * 1.2 or 2.X with use_vik_prefetch turned on. The user might 
		 * have turned on debug_msg in which the message will be false.
		 */
		clrbits |= CPU_VIK_PF;
		use_vik_prefetch = 0;
	}
	if (use_table_walk && (use_ec) && (cache == CACHE_PAC_E))
		setbits |= CPU_VIK_TC;
	else
		clrbits |= CPU_VIK_TC;
	if (use_store_buffer)
		setbits |= CPU_VIK_SB;
	else
		clrbits |= CPU_VIK_SB;
	vik_vac_init_asm(clrbits, setbits);

}

void		/*ARGSUSED*/
vik_module_setup(mcr)
	int	mcr;
{
        /*
	 * The prom should have initialized mcr correctly.
	 * We should have:
	 * EN   = 1
	 * NF   = 0
	 * PSO  = 0
	 * DE   = 1
	 * IE   = 1
	 * SB   = 1
	 * MB (ro) = 0 if E-$ exists. Otherwise 1 (MBus mode).
	 * PE   = 1 all E-$ srams support parity.
	 * BT   = 0
	 * SE   = 1
	 * AC   = 0
	 * TC   = !MB
	 * PF   = 1
	*/

	if (mcr & CPU_VIK_MB) { 		/* MBus mode */
		cache = CACHE_PAC;
		mod_info[0].mod_type = CPU_VIKING;
		v_mmu_log_module_err = vik_mmu_log_module_err;
	} else {				/* CC mode */
		cache = CACHE_PAC_E;
		mod_info[0].mod_type = CPU_VIKING_MXCC;
		v_mmu_log_module_err = mxcc_mmu_log_module_err;
		v_vac_parity_chk_dis = mxcc_vac_parity_chk_dis;
	}


	v_vac_init = vik_vac_init;
	v_cache_on = vik_cache_on;
	v_mmu_getasyncflt = vik_mmu_getasyncflt;
	v_mmu_chk_wdreset = vik_mmu_chk_wdreset;
	v_mmu_print_sfsr = vik_mmu_print_sfsr;
	v_mmu_probe = (int (*)())probe_pte_0;

	v_pte_offon = vik_pte_offon;
	v_module_wkaround = vik_module_wkaround;
#ifdef	MULTIPROCESSOR
	mp_pte_offon = vik_pte_offon;
#endif	MULTIPROCESSOR

	if (cache == CACHE_PAC_E) {		/* only for E$ */
		v_mmu_flushctx = vik_mmu_flushctx;
		v_mmu_flushrgn = vik_mmu_flushrgn;
		v_mmu_flushseg = vik_mmu_flushseg;
		v_mmu_flushpage = vik_mmu_flushpage;
		v_mmu_flushpagectx = vik_mmu_flushpagectx;
		if (use_page_coloring)
			do_pg_coloring = 1;/* have Ecache, will color pages */
	}
	vik_rev_level = get_vik_rev_level(mcr);
}

void
vik_mmu_log_module_err(afsr, afar0)
	u_int afsr;	/* MFSR */
	u_int afar0;	/* MFAR */
{
	if (afsr & MMU_SFSR_SB)
		printf("Store Buffer Error");
	printf("mfsr 0x%x\n", afsr);
	if (afsr & MMU_SFSR_FAV)
		printf("\tFault Virtual Address = 0x%x\n", afar0);
}

char	*mod_err_type[] = {
	"[err=0] ",
	"Uncorrectable Error ",
	"Timeout Error ",
	"Bus Error ",
	"Undefined Error ",
	"[err=5] ",
	"[err=6] ",
	"[err=7] ",
};

void
mxcc_mmu_log_module_err(afsr, afar0, aerr0, aerr1)
	u_int afsr;	/* MFSR */
	u_int afar0;	/* MFAR */
	u_int aerr0;	/* MXCC error register <63:32> */
	u_int aerr1;	/* MXCC error register <31:0> */
{

	(void) vik_mmu_print_sfsr(afsr);
	if (afsr & MMU_SFSR_FAV)
		printf("\tFault Virtual Address = 0x%x\n", afar0);

	printf("MXCC Error Register:\n");
	if (aerr0 & MXCC_ERR_ME)
		printf("\tMultiple Errors\n");
	if (aerr0 & MXCC_ERR_AE)
		printf("\tAsynchronous Error\n");
	if (aerr0 & MXCC_ERR_CC)
		printf("\tCache Consistency Error\n");
	/*
	 * XXXXXX: deal with pairty error later, just log it now
	 *	   ignore ERROR<7:0> on parity error
	 */
	if (aerr0 & MXCC_ERR_CP)
		printf("\tE$ Parity Error\n");

	if (aerr0 & MXCC_ERR_EV) {

#ifdef notdef
	printf("\tRequested transaction: %s%s at %x:%x\n",
		aerr0&MXCC_ERR_S ? "supv " : "user ",
		ccop_trans_type[(aerr0&MXCC_ERR_CCOP) >> MXCC_ERR_CCOP_SHFT],
			aerr0 & MXCC_ERR_PA, aerr1);
#endif notdef
		printf("\tRequested transaction: %s CCOP %x at %x:%x\n",
			aerr0&MXCC_ERR_S ? "supv " : "user ",
			aerr0&MXCC_ERR_CCOP, aerr0 & MXCC_ERR_PA, aerr1);
		printf("\tError type: %s\n",
		    mod_err_type[(aerr0 & MXCC_ERR_ERR) >> MXCC_ERR_ERR_SHFT]);
	}
}

void
vik_mmu_print_sfsr(sfsr)
	u_int	sfsr;
{
	printf("MMU sfsr=%x:", sfsr);
	switch (sfsr & MMU_SFSR_FT_MASK) {
	case MMU_SFSR_FT_NO: printf (" No Error"); break;
	case MMU_SFSR_FT_INV: printf (" Invalid Address"); break;
	case MMU_SFSR_FT_PROT: printf (" Protection Error"); break;
	case MMU_SFSR_FT_PRIV: printf (" Privilege Violation"); break;
	case MMU_SFSR_FT_TRAN: printf (" Translation Error"); break;
	case MMU_SFSR_FT_BUS: printf (" Bus Access Error"); break;
	case MMU_SFSR_FT_INT: printf (" Internal Error"); break;
	case MMU_SFSR_FT_RESV: printf (" Reserved Error"); break;
	default: printf (" Unknown Error"); break;
	}
	if (sfsr) {
		printf (" on %s %s %s at level %d",
			    sfsr & MMU_SFSR_AT_SUPV ? "supv" : "user",
			    sfsr & MMU_SFSR_AT_INSTR ? "instr" : "data",
			    sfsr & MMU_SFSR_AT_STORE ? "store" : "fetch",
			    (sfsr & MMU_SFSR_LEVEL) >> MMU_SFSR_LEVEL_SHIFT);
		if (sfsr & MMU_SFSR_BE)
			printf ("\n\tM-Bus Bus Error");
		if (sfsr & MMU_SFSR_TO)
			printf ("\n\tM-Bus Timeout Error");
		if (sfsr & MMU_SFSR_UC)
			printf ("\n\tM-Bus Uncorrectable Error");
		if (sfsr & MMU_SFSR_UD)
			printf ("\n\tM-Bus Undefined Error");
		if (sfsr & MMU_SFSR_P)
			printf ("\n\tParity Error");
		if (sfsr & MMU_SFSR_CS)
			printf ("\n\tControl Space Sccess Error");
		if (sfsr & MMU_SFSR_SB)
			printf ("\n\tStore Buffer Error");
	}
	printf("\n");
}

#include <machine/vm_hat.h>
#include <vm/as.h>

/*
 * vik_pte_offon: change some bits in a specified pte,
 * keeping the TLB consistant.
 *
 * This is the bottom level of manipulation for PTEs,
 * and it knows all the right things to do to flush
 * all the right caches. Further, it is safe in the face
 * of an MMU doing a read-modify-write cycle to set
 * the REF or MOD bits.
 */
unsigned
vik_pte_offon(ptpe, aval, oval)
	union ptpe	   *ptpe;
	unsigned	    aval;
	unsigned	    oval;
{
	unsigned	    rpte;
	unsigned	    wpte;
	unsigned	   *ppte;
	struct pte	   *pte;
	addr_t		    vaddr;
	struct ptbl	   *ptbl;
	struct as	   *as;
	struct ctx	   *ctx;
	u_int		    cno;
	extern	void mmu_flushpagectx();

	/*
	 * If the ptpe address is NULL, we have nothing to do.
	 */
	if (!ptpe)
		return 0;

	ppte = &ptpe->ptpe_int;
	rpte = *ppte;
	wpte = (rpte & ~aval) | oval;

	/*
	 * Early Exit Options: If no change, just return.
	 */
	if (wpte == rpte)
		return rpte;

	/*
	 * Fast Update Options: If the old PTE was not valid,
	 * then no flush is needed: update and return.
	 */
	if ((rpte & PTE_ETYPEMASK) != MMU_ET_PTE)
		return vik_pte_rmw(ppte, aval, oval);

	pte = &ptpe->pte;
	ptbl = ptetoptbl(pte);
	vaddr = ptetovaddr(pte);

	/*
	 * Decide which context this mapping
	 * is active inside. If the vaddr is a
	 * kernel address, force context zero;
	 * otherwise, peel the address space and
	 * context used by the ptbl.
	 *
	 * Fast Update Option:
	 *	If there is no address space or
	 *	there is no context, then this
	 *	mapping must not be active, so
	 *	we just update and return.
	 */
	if ((unsigned)vaddr >= (unsigned)KERNELBASE)
		cno = 0;
	else if (((as = ptbl->ptbl_as) == NULL) ||
		 ((ctx = as->a_hat.hat_ctx) == NULL))
		return vik_pte_rmw(ppte, aval, oval);
	else
		cno = ctx->c_num;

	/*
	 * It might be possible to write an RMW
	 * routine that does not require capture,
	 * but since SunOS 4.1.3/Viking/MP is not
	 * a product, and this is provably OK, we
	 * do the simple thing.
	 */

	xc_attention();
	rpte = vik_pte_rmw(ppte, aval, oval);
	mmu_flushpagectx(vaddr, cno);
	xc_dismissed();

	return rpte;
}

/*
 * viking module specific workarounds. 
 * Does SW workaround for known Viking bugs.
 * The bugs are described below.
 */
void
vik_module_wkaround(adr, rp, rw, mfsr)
addr_t *adr;
struct regs *rp;
register enum seg_rw rw;
register u_int mfsr;
{ 
     
     
    extern addr_t fix_addr();
    
     /*
      * viking HW bug workaround.
      * viking sets Modified on a write to a write protected page. This is 
      * seen on rev 1.X Viking only. Since there are a lot of 1.2 vikings
      * we carry the workaround here.
      * This confuses swapper etc..
      * Fix the pte right here
      */

     if ((vik_rev_level == VIK_REV_1DOT2) && (rw == S_WRITE)) {
	struct pte *ppte;
	u_int ipte;

 	ipte = mmu_probe(*adr);
        ppte = (struct pte *) & ipte;
        if (!(ppte->AccessPermissions&1) && (ppte->Modified))  {
		clr_modified_bit(*adr);
        }
     }

     /* Viking HW bug workaround 
      * Viking gets confused about the mfar contents (fault address) when
      * the following conditions are met (this is not a complete description
      * of the behaviour of the bug). 
      *	A sequence like below appears at the end of the page
      *
      *	0xXXFF8		{call}{ba}{jmpl}		dest
      *	0xXXFFC		{st} {swap} {ldstub} {ld}	addr
      *
      *	The call to dest causes a table walk (aka TLB miss)
      * The mem-op to addr get a protection violation or privilege violation
      * with a TLB hit (ld can produce bad addr on load from execute only
      * page).
      *
      * Under these conditions the MFAR (fault address register) could latch
      * the address of dest instead of addr. This could fool the kernel into
      * thinking that the user was trying to write to his own text and cause
      * a core dump. Therefore, recompute the faulting address.
      * See bugid 1095941 for more details.
      */

     if (((rp->r_pc & MMU_PAGEOFFSET) == (int)0xFFC) && 
	 ((int )*adr == rp->r_npc)){
	  addr_t tmp;

	  tmp = fix_addr(rp, mfsr);
	  if (tmp != (addr_t)-1) /* else leave the fault address alone */
	       *adr = tmp;
     }
}

/*
 * get viking rev level
 * Here is how it is determined
 * 
 *   Viking Rev    PSR Version    MMU Version    JTAG Version
 *   ----------    -----------    -----------    ------------
 *   1.2           0x40           0              0
 *   2.x           0x41           0              0
 *   3.0           0x40           1              >0
 */

int get_vik_rev_level(mcr) {

     u_int psr_version, mcr_version, version;

     psr_version = (getpsr() >> 24) & 0xF;
     mcr_version = (mcr >> 24) & 0xF;

     if (psr_version)
	  version = VIK_REV_2DOTX;
     else {
	  if (mcr_version) 
	       version = VIK_REV_3DOTO;
	  else
	       version = VIK_REV_1DOT2;
     }

     return(version);
}

