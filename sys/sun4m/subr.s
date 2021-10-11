/*      @(#)subr.s 1.1 92/07/30 SMI      */

/*
 *	Copyright (c) 1988,1990 by Sun Microsystems, Inc.
 */

/*
 * General machine architecture & implementation specific
 * assembly language routines.
 */

#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/eeprom.h>
#include <machine/param.h>
#include <machine/intreg.h>
#include <machine/async.h>
#ifdef	IOMMU
#include <machine/iommu.h>
#endif
#include <machine/devaddr.h>
#include <machine/auxio.h>
#include "assym.s"
#include <machine/psl.h>
#include <machine/cpu.h>

/*
 * known physical addresses.
 * XXX - don't we get this info from the prom's device tree?
 */
#define	PA_DIAGLED	0xF1600000	/* asi=2F: diagnostic LED */
#define PA_DIAGMESG	0x00001000	/* asi=2F: diagnostic message */
#define PA_MBUS_ARBEN	0xE0001008	/* asi=2F: mbus arbiter enable */
#define PA_SYSCTL	0xF1F00000	/* asi=2F: system control/status */
#define PA_AUXIO	0xF1800000	/* asi=2F: AUXIO and LED */


/*
 * The first set of routines are those that are common to all sun4m
 * implementations since they are either hardware independent or
 * they access hardware that is a required feature of the architecture
 */
	.seg	"text"
	.align	4

/*
 * Read the ID prom.
 * This is mapped from IDPROM_ADDR for IDPROMSIZE bytes in the
 * kernel virtual address space.
 * XXX - assumes that all implementations use 8kx8 NVRAM part
 *
 * Note that this copies the bytes in reverse order.
 */
	ENTRY(getidprom)
	set	IDPROM_ADDR, %o1
	set	IDPROMSIZE, %o2
1:	deccc	%o2			! test loop termination
	ldub	[%o1 + %o2], %o3	! read eeprom
	bne	1b			! loop until done
	stb	%o3, [%o0 + %o2]	! write destination
	retl
	nop

/*
 * Turn on or off bits in the system interrupt mask register.
 * no need to lock out interrupts, since set/clr softint is atomic.
 * === sipr as specified in 15dec89 sun4m arch spec, sec 5.7.3.2
 *
 * set_intmask(bit, flag)
 *	int bit;		bit mask in interrupt mask
 *	int flag;		0 = off, otherwise on
 */
	ALTENTRY(set_intmask)
	tst	%o1
	set	IR_SYS_CLEAR, %o3	! system clear mask
	bnz,a	1f
	add	%o3, 4, %o3		! bump to "set mask"
1:
	retl
	st	%o0, [%o3]		! set/clear interrupt

/*
 * Turn on or off bits in the pending interrupt register.
 * no need to lock out interrupts, since set/clr softint is atomic.
 * === MID as specified in 15dec89 sun4m spec, sec 5.4.3
 * === int regs as specified in 15dec89 sun4m arch spec, sec 5.7.1
 *
 * set_intreg(bit, flag)
 *	int bit;		bit mask in interrupt reg
 *	int flag;		0 = off, otherwise on
 */
	ALTENTRY(set_intreg)
	GETCPU(%o2)			! get module id 8..11
	and	%o2, 3, %o2		! mask to 0..3
	sll	%o2, 12, %o2		! shift for +0x1000 per mid
	set	V_INT_REGS+4, %o3	! softint ack for cpu0
	add	%o3,%o2,%o3		! add offset per module
	tst	%o1			! set or clear?
	bnz,a	1f			! if set,
	add	%o3, 4, %o3		!   use set reg, not clear reg
1:
	retl
	st	%o0, [%o3]		! set/clear interrupt

/*
 * Grab the interrupt target register
 */
	ALTENTRY(take_interrupt_target)
	GETCPU(%o0)			! get cpu number, 0..3
	sethi	%hi(V_INT_REGS+0x4010), %g1
	retl
	st	%o0, [%g1+%lo(V_INT_REGS+0x4010)]

/*
 * Get the processor ID.
 * === MID reg as specified in 15dec89 sun4m spec, sec 5.4.3
 */

	ALTENTRY(getprocessorid)
	GETCPU(%o0)			! get module id 8..11
	retl
	and	%o0, 3, %o0		! return 0..3

	ALTENTRY(chk_cpuid)
#ifdef	MULTIPROCESSOR
	GETCPU(%o0)			! get module id 8..11
	and	%o0, 3, %o0		! return 0..3
	sethi	%hi(_cpuid), %o1
	ld	[%o1+%lo(_cpuid)], %o1
	cmp	%o0, %o1
	beq	1f
	nop
#endif	MULTIPROCESSOR
	ALTENTRY(bad_cpuid)
	nop
1:	retl
	nop

/*
 * int	ldphys(int paddr)
 * read word of memory at physical address
 * also called "ldphys" by some codes
 * In the case of VIKING:
 * currently, ldphys only called to load the physical address of a pte
 * so, we only check MBus mode vs. CC mode.
 * for Mbus mode, page tables are not cached.  don't set AC bit.
 * for CC mode, page tables are cached. set AC bit.
 */
	ALTENTRY(ldphys)
	sethi	%hi(_cache), %g1
	ld	[%g1 + %lo(_cache)], %g1
	cmp	%g1, CACHE_PAC_E		! CC mode?
	bne,a	1f				! No, don't cache the
	lda	[%o0]ASI_MEM, %o0		! srmmu tables.

	! For viking with E$, it's necessary set the AC bit in the
	! module control register to indicate that all memory access,
	! except table walking accesses, for which the physical address
	! is not obtained through a PTE are cached in the viking internal
	! caches and the external cache.
	mov	%psr, %o5		! get psr
	andn	%o5, PSR_ET, %g1	! disable traps
	mov	%g1, %psr		! new psr
	nop; nop; nop			! PSR delay
	lda	[%g0]ASI_MOD, %o4	! get MMU CSR
	set	CPU_VIK_AC, %g1		! AC bit
	or	%o4, %g1, %g1		! or in AC bit
	sta	%g1, [%g0]ASI_MOD	! store new CSR
	lda	[%o0]ASI_MEM, %o0	! the actual ldphys
	sta	%o4, [%g0]ASI_MOD	! restore CSR;
	mov	%o5, %psr		! restore psr; enable traps again
	nop				! PSR delay
1:
	retl
	nop

/*
 * void stphys(int paddr, int data)
 * write word of memory at physical address
 * In the case of VIKING:
 * currently, stphys only called to load the physical address of a pte
 * so, we only check MBus mode vs. CC mode.
 * for Mbus mode, page tables are not cached.  don't set AC bit.
 * for CC mode, page tables are cached. set AC bit.
 */
	ALTENTRY(stphys)
	sethi	%hi(_cache), %g1
	ld	[%g1 + %lo(_cache)], %g1
	cmp	%g1, CACHE_PAC_E		! CC mode?
	bne,a	1f				! No, don't cache the
	sta	%o1, [%o0]ASI_MEM		! srmmu tables.

	! For viking with E$, it's necessary set the AC bit in the
	! module control register to indicate that all memory access,
	! except table walking accesses, for which the physical address
	! is not obtained through a PTE are cached in the viking internal
	! caches and the external cache.
	mov	%psr, %o5		! get psr
	andn	%o5, PSR_ET, %g1	! disable traps
	mov	%g1, %psr		! new psr
	nop; nop; nop			! PSR delay
	lda	[%g0]ASI_MOD, %o4	! get MMU CSR
	set	CPU_VIK_AC, %g1		! AC bit
	or	%o4, %g1, %g1		! or in AC bit
	sta	%g1, [%g0]ASI_MOD	! store new CSR
	sta	%o1, [%o0]ASI_MEM	! the actual stphys
	sta	%o4, [%g0]ASI_MOD	! restore CSR;
	mov	%o5, %psr		! restore psr; enable traps again
	nop				! PSR delay
1:
	retl
	nop

/*
 * void swphys(int data, int paddr)
 * write word of memory at physical address
 * In the case of VIKING:
 * currently, swphys only called to load the physical address of a pte
 * so, we only check MBus mode vs. CC mode.
 * for Mbus mode, page tables are not cached.  don't set AC bit.
 * for CC mode, page tables are cached. set AC bit.
 */
	ALTENTRY(swphys)
	sethi	%hi(_cache), %g1
	ld	[%g1 + %lo(_cache)], %g1
	cmp	%g1, CACHE_PAC_E		!CC mode?
	bne,a	1f				!No, don't cache 
	swapa	[%i1]ASI_MEM, %i0

	! For viking with E$, it's necessary set the AC bit in the
	! module control register to indicate that all memory access,
	! except table walking accesses, for which the physical address
	! is not obtained through a PTE are cached in the viking internal
	! caches and the external cache.
	mov	%psr, %o5		! get psr
	andn	%o5, PSR_ET, %g1	! disable traps
	mov	%g1, %psr		! new psr
	nop; nop; nop			! PSR delay
	lda	[%g0]ASI_MOD, %o4	! get MMU CSR
	set	CPU_VIK_AC, %g1		! AC bit
	or	%o4, %g1, %g1		! or in AC bit
	sta	%g1, [%g0]ASI_MOD	! store new CSR
	swapa	[%o1]ASI_MEM, %o0	! the actual swphys
	sta	%o4, [%g0]ASI_MOD	! restore CSR;
	mov	%o5, %psr		! restore psr; enable traps again
	nop; nop; nop			! PSR delay
1:	retl
	nop

#ifdef IOMMU
	/*
 	 * Set iommu ctlr reg.
 	 * iommu_set_ctl(value)
 	 */
	ALTENTRY(iommu_set_ctl)
	set	V_IOMMU_CTL_REG, %o1
	retl
	st	%o0, [%o1]

	/*
 	 * Set iommu base addr reg.
 	 * iommu_set_base(value)
 	 */
	ALTENTRY(iommu_set_base)
	set	V_IOMMU_BASE_REG, %o1
	retl
	st	%o0, [%o1]

	/*
 	 * iommu flush all TLBs 
	 * iommu_flush_all()
 	 */
	ALTENTRY(iommu_flush_all)
	set	V_IOMMU_FLUSH_ALL_REG, %o1
	retl
	st	%g0, [%o1]

	/*
 	 * iommu addr flush 
 	 * iommu_addr_flush(flush_addr_reg)
	 */
	ALTENTRY(iommu_addr_flush)
	set	V_IOMMU_ADDR_FLUSH_REG, %o1
	retl
	st	%o0, [%o1]

#endif IOMMU

#ifdef IOC
	/*      
 	 * Low-Level interface to IOC 
 	 */

	/*
 	 * Set ioc tag memory.
 	 * do_load_ioc(addr,value)
 	 */
	ALTENTRY(do_load_ioc)
	retl
	sta	%o1, [%o0]ASI_CTL

	/*
 	 * read ioc tag memory.
 	 * do_read_ioc(addr)
 	 */
	ALTENTRY(do_read_ioc)
	retl
	lda	[%o0]ASI_CTL,%o0

	/*
 	 * flush ioc
 	 * do_flush_ioc(addr)
 	 */
	ALTENTRY(do_flush_ioc)
	retl
	sta	%g0, [%o0]ASI_CTL

#endif IOC

	/*
	 * Get Synchronous Fault Status
	 */
	ENTRY(get_sfsr)
	set	RMMU_FSR_REG, %o0
	retl
	lda	[%o0]ASI_MOD, %o0

/*
 *      Code used to vector to the correct routine when the
 *      action to be performed is done differently on different
 * 	Sun-4M system implementations. Module differences are
 *	handled in the "module*" files.
 *
 *      The default action is whatever conforms to the latest 
 *   	Sun4M System Architecture Specification; if an implementation
 * 	requires things to be done differently, then the kernel must 
 * 	install the proper function pointer in the _v_ table. This
 *	happens in early_startup() once the system type has been
 *	retrieved from the PROM.
 */

#define VECT(s) .global _v_/**/s; _v_/**/s: .word _sun4m_/**/s

        .seg    "data"
        .align  4
_v_sys_base:
VECT(get_sysctl)
VECT(set_sysctl)
VECT(set_diagled)
VECT(get_diagmesg)
VECT(set_diagmesg)
VECT(enable_dvma)
VECT(disable_dvma)
VECT(l15_async_fault)
VECT(flush_writebuffers)
VECT(flush_poke_writebuffers)
VECT(init_all_fsr)

	.seg    "text"
	.align  4

/*
 * Read the System Control & Status Register
 *
 *	u_long get_sysctl()
 */
	ALTENTRY(get_sysctl)
	sethi	%hi(_v_get_sysctl), %o5
	ld	[%o5+%lo(_v_get_sysctl)], %o5
	jmp 	%o5
	nop

/*
 * Write the System Control & Status Register
 *
 *	void set_sysctl(val)
 *	u_long val;
 */
	ALTENTRY(set_sysctl)
	sethi	%hi(_v_set_sysctl), %o5
	ld	[%o5+%lo(_v_set_sysctl)], %o5
	jmp 	%o5
	nop

/* 
 * Write the Diagnostic LED Register (noop for some implementations)
 *
 *	void set_diagled(val)
 *	u_short val; XXX - 8/16/32bit access?
 */
	ALTENTRY(set_diagled)
	sethi	%hi(_v_set_diagled), %o5
	ld	[%o5+%lo(_v_set_diagled)], %o5
	jmp 	%o5
	nop

/*
 * Read the Diagnostic Message Register
 *
 *	u_long get_diagmesg()
 */
	ALTENTRY(get_diagmesg)
	sethi	%hi(_v_get_diagmesg), %o5
	ld	[%o5+%lo(_v_get_diagmesg)], %o5
	jmp 	%o5
	nop

/* 
 * Write the Diagnostic Message Register
 * XXX - should this use byte access only?
 *
 *	void set_diagmesg(val)
 *	u_long val;
 */
	ALTENTRY(set_diagmesg)
	sethi	%hi(_v_get_diagmesg), %o5
	ld	[%o5+%lo(_v_get_diagmesg)], %o5
	jmp 	%o5
	nop

/*
 * Enable DVMA Arbitration
 *
 *	void enable_dvma()
 */
	ALTENTRY(enable_dvma)
	sethi	%hi(_v_enable_dvma), %o5
	ld	[%o5+%lo(_v_enable_dvma)], %o5
	jmp 	%o5
	nop

/*
 * Disable DVMA Arbitration 
 *
 *	void disable_dvma()
 */
	ALTENTRY(disable_dvma)
	sethi	%hi(_v_disable_dvma), %o5
	ld	[%o5+%lo(_v_disable_dvma)], %o5
	jmp 	%o5
	nop

/*
 * Level-15 Asynchronous Fault Handler
 *
 *	void l15_async_fault(sipr)
 *	u_int	sipr;
 */
	ALTENTRY(l15_async_fault)
	sethi	%hi(_v_l15_async_fault), %o5
	ld	[%o5+%lo(_v_l15_async_fault)], %o5
	jmp 	%o5
	nop


/*
 * Flush Writebuffers
 *
 *	void flush_writebuffers()
 */
	ALTENTRY(flush_writebuffers)
	sethi	%hi(_v_flush_writebuffers), %o5
	ld	[%o5+%lo(_v_flush_writebuffers)], %o5
	jmp 	%o5
	nop

/*
 * Flush Writebuffers after a poke-type probe
 *
 *	void flush_poke_writebuffers()
 *	void flush_writebuffers_to(unsigned va)
 */
	ALTENTRY(flush_poke_writebuffers)
	ALTENTRY(flush_writebuffers_to)
	sethi	%hi(_v_flush_poke_writebuffers), %o5
	ld	[%o5+%lo(_v_flush_poke_writebuffers)], %o5
	jmp 	%o5
	nop

/*
 * Clear System Error Registers
 *
 *	u_long init_all_fsr()
 */
	ALTENTRY(init_all_fsr)
	sethi	%hi(_v_init_all_fsr), %o5
	ld	[%o5+%lo(_v_init_all_fsr)], %o5
	jmp 	%o5
	nop

/*
 * Sun-4M/50 Stuff
 *
 * include anything here that Sun-4M/50 needs to do differently 
 * from the generic sun4m code.
 */

#undef SVC
#define SVC(fn) set _p4m50_/**/fn, %o5 ;\
		 st %o5, [%o0+(_v_/**/fn-_v_sys_base)]

	ALTENTRY(p4m50_sys_setfunc)
	set	(_v_sys_base), %o0
	SVC(set_diagled)
	SVC(init_all_fsr)
	retl
	nop


	ALTENTRY(p4m50_set_diagled)
	set     PA_AUXIO, %o0
	ldsba   [%o0]ASI_CTL, %o1
	or      %o1, AUX_LED | AUX_MBO, %o1
	stba    %o1, [%o0]ASI_CTL
	retl
	nop
 
! read and clear all error registers
! on the Sun-4M/50 processor board.
 
	ALTENTRY(p4m50_init_all_fsr)
 
	set     0xE0001000, %o0         ! M to S Fault Status Reg.
	add     %o0, 4, %o1
	lda     [%o0]ASI_CTL, %g0       ! read m2safsr
	lda     [%o1]ASI_CTL, %g0       ! read m2safar
	sta     %g0, [%o0]ASI_CTL       ! unlock m2s
 
	set     0x00000008, %o0         ! ECC Mem Fault Status Reg.
	lda     [%o0]ASI_CTL, %g0       ! read eccmfsr
	sta     %g0, [%o0]ASI_CTL       ! unlock ecc
		
	set     0x00000000, %o0         ! ECC Memory Enable Reg
	lda     [%o0]ASI_CTL, %o1       ! yes, read old value, keep rsvd bits
	or      %o1, 1, %o1             ! enable checking
        or      %o1, 2, %o1             ! enable int on corr.err
	sta     %o1, [%o0]ASI_CTL       ! update register.
						  
 
	set     0xE0001008, %o0         ! Arbiter Enable Register
	lda     [%o0]ASI_CTL, %o1       ! read old value, keep rsvd bits
	set     0x80100000, %g1         ! enable S to M async writes
	or      %o1, %g1, %o1           ! and onboard ether/scsi dvma
	sta     %o1, [%o0]ASI_CTL       ! update register.
 
	retl
	nop

/*
 * Sun-4M/600 Stuff
 *
 * include anything here that Sun-4M/600 needs to do differently 
 * from the generic sun4m code.
 */

#undef SVC
#define SVC(fn) set _p4m690_/**/fn, %o5 ;\
		 st %o5, [%o0+(_v_/**/fn-_v_sys_base)]

	ENTRY(p4m690_sys_setfunc)
	set	(_v_sys_base), %o0
	retl
	nop

/*
 * Sun-4M Stuff
 *
 *  generic sun4m routines
 */

	ALTENTRY(sun4m_get_sysctl)
	set 	PA_SYSCTL, %o1
	retl
	lda	[%o1]ASI_CTL, %o0


	ALTENTRY(sun4m_set_sysctl)
	set 	PA_SYSCTL, %o1
	retl
	sta	%o0, [%o1]ASI_CTL


	ALTENTRY(sun4m_set_diagled)
	sub	%g0, 1, %o1
	xor	%o0, %o1, %o0		! zero => led ON
	set 	PA_DIAGLED, %o1
	stha	%o0, [%o1]ASI_CTL	! Sun-4M has 12 LEDs
	retl
	nop

	ALTENTRY(sun4m_get_diagmesg)
	set 	PA_DIAGMESG, %o1
	retl
	lda	[%o1]ASI_CTL, %o0


	ALTENTRY(sun4m_set_diagmesg)
	set 	PA_DIAGMESG, %o1
	retl
	sta	%o0, [%o1]ASI_CTL


	ALTENTRY(sun4m_enable_dvma)
	set     PA_MBUS_ARBEN, %o0
	set     EN_ARB_SBUS, %o1
	retl
	sta     %o1, [%o0]ASI_CTL

	ALTENTRY(sun4m_disable_dvma)
        set     PA_MBUS_ARBEN, %o0
        retl
        sta     %g0, [%o0]ASI_CTL

/* 
 * Flush module and M-to-S write buffers.  These are
 * the write buffers which MUST be flushed just before changing 
 * the MMU context.  Note that we don't need to flush the mem I/F
 * (ECC) write buffer because the context number at the time of the
 * fault is not needed to perform asynchronous fault handling.
 * Also note that we don't bother flushing the VME write buffer
 * because we can't even tell which CPU caused the fault.
 * The one exception where we don't need to call this
 * routine is when changing contexts temporarly to flush caches.
 */
	ALTENTRY(sun4m_flush_writebuffers)
 	/*
	 * flush module write buffer by performing a swap on some
	 * dummy memory location which is cached.  This is supposed
	 * to work because the swap is supposed to be an atomic
	 * instruction, and so the hardware is supposed to stall
	 * for the write to complete, which implies that the write
	 * buffer for the write-back cache is flushed, since this
	 * is the last write.
 	 */
/* commented out 19sep90 impala@dcpower,
 * we don't really need to do this. */
!	set	_module_wb_flush, %o0
!	swap	[%o0], %g0

	/*
	 * the hardware was designed to allow one to flush the
	 * M-to-S write buffer by simply reading the M-to-S
	 * Asynchronous Fault Status Register.
	 * According to the manual, gotta read this thrice.
	 */
	set	MTS_AFSR_VADDR, %o0
	ld	[%o0], %g0
	ld	[%o0], %g0
	ld	[%o0], %g0
	/*
	 * extra nops put in to ensure that the fault will occurr
	 * while executing instructions here (before we return).
	 * XXX -- need to chack with hardware to see if some of
	 * these nops can be removed. Also, verify whether
	 * there are enough NOPs if we do need 'em.
	 */
	nop ;	nop ;	nop
	nop ;	nop ;	nop
	retl ;	nop

/*
 * Flush write buffers which may have filled up as a result of
 * a poke operation.  This routine is used by the poke routine to
 * cause asynchronous faults to happen immediately, thus indicating
 * whether or not the probe failed.
 */
	ALTENTRY(sun4m_flush_poke_writebuffers)
	/*
	 * the hardware was designed to allow one to flush the
	 * M-to-S write buffer by simply reading the M-to-S
	 * Asynchronous Fault Status Register.
	 * According to the manual, gotta read this thrice.
	 * Actually, this should not be necessay, as the
	 * VFSR flush below should handle it, but I am
	 * getting paranoid.
	 */
	set	MTS_AFSR_VADDR, %o0
	ld	[%o0], %g0
	ld	[%o0], %g0
	ld	[%o0], %g0
#ifdef VME
#ifdef SUN4M_50
	/*
	 * Don't poke the VME Asynchronous Fault Status Register
	 * if it does not exist!
	 */
	sethi	%hi(_ioc), %o0
	ld	[%o0 + %lo(_ioc)], %o0
	tst	%o0
	bz	0f
	nop
#endif SUN4M_50

	/*
	 * Note that reading the VME Asynchronous Fault Status
	 * Register is supposed to flush the S-to-VME write buffer,
	 * the M-to-S write buffer, and any write buffer in the
	 * processor module that issued the write performed in the
	 * probe.  This should be sufficient for all uses of
	 * the poke routines. We read it several times because
	 * it could take some time for the fault to travel
	 * up to the processor, and a bunch of NOPs was not
	 * good enough. BLETCH. Anyway, three times chosen
	 * in symmatry with the M-to-S flushing above.
	 */
	set	VFSR_VADDR, %o0
	ld	[%o0], %g0
	ld	[%o0], %g0
	ld	[%o0], %g0
0:
#endif VME

	/*
	 * How many nops do we need? can we get rid of
	 * all but one of the above loads?
	 */
	nop ; 	nop ; 	nop ; 	nop
	nop ; 	nop ; 	nop ; 	nop
	nop ; 	nop ; 	nop ; 	nop
	nop ; 	nop ; 	nop ; 	nop

	retl ;	nop

! read and clear all error registers
! on the sun4m processor board.

	ALTENTRY(sun4m_init_all_fsr)

	set	0xE0001000, %o0		! M to S Fault Status Reg.
	add	%o0, 4, %o1
	lda	[%o0]ASI_CTL, %g0	! read m2safsr
	lda	[%o1]ASI_CTL, %g0	! read m2safar
	sta	%g0, [%o0]ASI_CTL	! unlock m2s

	set	0x00000008, %o0		! ECC Mem Fault Status Reg.
	add	%o0, 8, %o1
	add	%o1, 4, %o2
	lda	[%o0]ASI_CTL, %g0	! read eccmfsr
	lda	[%o1]ASI_CTL, %g0	! read eccmfar0
	lda	[%o2]ASI_CTL, %g0	! read eccmfar1
	sta	%g0, [%o0]ASI_CTL	! unlock ecc

	set	0x00000000, %o0		! ECC Memory Enable Reg
	lda	[%o0]ASI_CTL, %o1	! yes, read old value, keep rsvd bits
	or	%o1, 1, %o1		! enable checking
!	or	%o1, 2, %o1		! enable int on corr.err
	sta	%o1, [%o0]ASI_CTL	! update register.

	set	0xDF010000, %o0		! VME Interface Control Reg.
	lda	[%o0]ASI_CTL, %o1
	set	0x40000000, %o2		! SVME Enable
	or	%o1, %o2, %o1		!   gets turned on
	set	0x90000000, %o2		! IOC Enable and VME Reset
	andn	%o1, %o2, %o1		!   get turned off
	sta	%o1, [%o0]ASI_CTL

	set	0xDF010008, %o0		! VME Async Fault Status Reg.
	sub	%o0, 4, %o1
	lda	[%o0]ASI_CTL, %g0	! read vmeafsr
	lda	[%o1]ASI_CTL, %g0	! read vmeafar
	sta	%g0, [%o0]ASI_CTL	! unlock vme

	set	0xE0001008, %o0		! Arbiter Enable Register
	lda	[%o0]ASI_CTL, %o1	! read old value, keep rsvd bits
	set	0x80100000, %g1		! enable S to M async writes
	or	%o1, %g1, %o1		! and onboard ether/scsi dvma
	sta	%o1, [%o0]ASI_CTL	! update register.

	retl
	nop
