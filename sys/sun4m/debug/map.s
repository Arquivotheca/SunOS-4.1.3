	.ident  "@(#)map.s 1.1 92/07/30 SMI"

!
!	Copyright (c) 1986 by Sun Microsystems, Inc.
!

/*
 * Additional memory mapping routines for use by standalone debugger,
 * setpgmap(), getpgmap() are taken from the boot code.
 */

#include "assym.s"
#include <sys/param.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/enable.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/eeprom.h>
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <debug/debug.h>

	.seg	"text"
	.align	4


#define CACHE_LINESHFT  5
#define CACHE_LINESZ	(1<<CACHE_LINESHFT)

/*
 * Flush a page from the cache.
 *
 * vac_pageflush(vaddr)
 * caddr_t vaddr;
 */
	ENTRY(vac_pageflush)

	set	PAGESIZE/8, %g1
	add	%o0, %g1, %o1
	add	%o1, %g1, %o2
	add	%o2, %g1, %o3
	add	%o3, %g1, %o4
	add	%o4, %g1, %o5
	add	%o5, %g1, %g2
	add	%g2, %g1, %g3

1:	deccc	CACHE_LINESZ, %g1
	sta	%g0, [%o0 + %g1]ASI_FCP
	sta	%g0, [%o1 + %g1]ASI_FCP
	sta	%g0, [%o2 + %g1]ASI_FCP
	sta	%g0, [%o3 + %g1]ASI_FCP
	sta	%g0, [%o4 + %g1]ASI_FCP
	sta	%g0, [%o5 + %g1]ASI_FCP
	sta	%g0, [%g2 + %g1]ASI_FCP
	bne	1b
	sta	%g0, [%g3 + %g1]ASI_FCP

	retl
	nop


/*
 * int  ldphys(int paddr)
 * read word of memory at physical address
 * also called "ldphys" by some codes
 */
        ENTRY(ldphys)
	sethi	%hi(_cache), %o1
	ld	[%o1 + %lo(_cache)], %o1
	cmp	%o1, CACHE_PAC_E	! Viking/E$
	bnz,a	0f			! No,
	lda	[%o0]ASI_MEM, %o0	!    dont need AC bit set

	mov	%psr, %o1		! Save %psr in %o1
	andn    %o1, PSR_ET, %g1        ! disable traps
        mov     %g1, %psr
        nop; nop; nop                   ! PSR delay
        lda     [%g0]ASI_MOD, %o2       ! get MMU CSR, %o2 keeps the saved CSR
        set     CPU_VIK_AC, %o3         ! AC bit
        or      %o2, %o3, %o3           ! or in AC bit
        sta     %o3, [%g0]ASI_MOD       ! store new CSR
 
        lda     [%o0]ASI_MEM, %o0

	sta     %o2, [%g0]ASI_MOD       ! restore CSR;
        mov     %o1, %psr               ! restore psr; enable traps again
        nop				! PSR delay
0:
        retl
	nop
/*
 * void stphys(int paddr, int data)
 * write word of memory at physical address
 */
        ENTRY(stphys)
	sethi	%hi(_cache), %o4
	ld	[%o4 + %lo(_cache)], %o4
	cmp	%o4, CACHE_PAC_E	! Viking/E$?
	bnz,a	0f			! No, dont need AC bit set
        sta     %o1, [%o0]ASI_MEM

	mov	%psr, %o4		! Save %psr in %o4
	andn    %o4, PSR_ET, %g1        ! disable traps
        mov     %g1, %psr
        nop; nop; nop                   ! PSR delay
        lda     [%g0]ASI_MOD, %o2       ! get MMU CSR, %o2 keeps the saved CSR
        set     CPU_VIK_AC, %o3         ! AC bit
        or      %o2, %o3, %o3           ! or in AC bit
        sta     %o3, [%g0]ASI_MOD       ! store new CSR
 
        sta     %o1, [%o0]ASI_MEM

	sta     %o2, [%g0]ASI_MOD       ! restore CSR;
        mov     %o4, %psr               ! restore psr; enable traps again
        nop				! PSR delay
0:
        retl
	nop
/*
 * void mmu_flushall()
 * flush all entries from TLB
 * XXX - for the moment this will only work with GENERIC SRMMU
 * modules, also MP is not supported.
 */
	ENTRY(mmu_flushall)
	or	%g0, FT_ALL << 8, %o0
	sta	%g0, [%o0]ASI_FLPR
	retl
	nop		! let mmu settle ??

/*
 * int mmu_getctp(void)
 * return current context table pointer
 */
	ENTRY(mmu_getctp)			! int	mmu_getctp(void);
	set	RMMU_CTP_REG, %o1		! get srmmu context table ptr
	retl
	lda	[%o1]ASI_MOD, %o0
