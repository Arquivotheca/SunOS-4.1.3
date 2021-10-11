/*	@(#)vm_hatasm.s  1.1 92/07/30 SMI	*/

	.ident "vm_hatasm.s"

/*
 * Copyright 1988-1989 Sun Microsystems, Inc.
 */

/*
 * The assembly code functions support the new hat layer implementation in 
 * sun/vm_hat.c. This source file is shared by sun4 and sun4c architectures.
 *
 * The equivalent C definitions of the assembly code functions are in 
 * sun/vm_hatasm.c.
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/enable.h>
#include "assym.s"

	.seg	"text"
	.align	4
!
! Load a SW page table to HW pmg
!
! void
! hat_pmgloadptes(a, ppte)
!	addr_t		a;
!	struct pte	*ppte;
!
!	o0	running page address
!	o1	running ppte
!	o3	end address	
!	o4	pte
!	g1	PG_V	
!	g3	pagesize (machine dependent value 4k, or 8k)
!
	ENTRY(hat_pmgloadptes)
	set	PAGESIZE, %g3
	set	PMGRPSIZE, %o3
	set	PG_V, %g1
	add	%o0, %o3, %o3		! o3 = a + PMGRPSIZE
	
0:
	ld	[%o1], %o4		! o4 has SW pte
	andcc	%o4, %g1, %g0		! valid page?
	bnz,a	1f
	sta	%o4, [%o0]ASI_PM	! write page map entry
	
1:
	add	%o0, %g3, %o0		! a += pagesize	
	cmp	%o0, %o3		! (a < end address) ?
	blu	0b
	add	%o1, 4, %o1		! o1 += sizeof(pte)

	retl
	nop

! 
! Unload a SW page table from HW pmg.
!
! void
! hat_pmgunloadptes(a, ppte)
!	addr_t		a;
!	struct pte	*ppte;
!
!	o0	running page address
!	o1	running ppte
!	o3	end address	
!	o4	pte
!	g1	PG_V	
!	g2	pte_invalid
!	g3	pagesize (machine dependent value 4k, or 8k)
!
	ENTRY(hat_pmgunloadptes)
	sethi 	%hi(_mmu_pteinvalid), %g1  ! g2 = *mmu_pteinvalid
	ld [%g1 + %lo(_mmu_pteinvalid)], %g2
	set	PG_V, %g1		! g1 PG_V

	set	PAGESIZE, %g3		! g3 = PAGESIZE
	set	PMGRPSIZE, %o3
	add	%o0, %o3, %o3		! o3 = a + PMGRPSIZE
	
0:
	ld	[%o1], %o4		! o4 has SW pte
	andcc	%o4, %g1, %g0		! valid page?
	bnz,a	1f
	lda	[%o0]ASI_PM, %o4	! (delay slot) read page map entry

	ba	2f
	add	%o0, %g3, %o0		! (delay slot) a += pagesize	
1:
	st	%o4, [%o1]		! store pte in SW page table
	sta	%g2, [%o0]ASI_PM	! write invalid pte
	add	%o0, %g3, %o0		! a += pagesize	
2:
	cmp	%o0, %o3		! (a < end address) ?
	blu	0b
	add	%o1, 4, %o1		! o1 += sizeof(pte)

	retl
	nop

! 
! Unload old SW page table from HW pmg and load a new SW page table 
! into this HW pmg at the same time.
!
! void
! hat_pmgswapptes(a, pnew, pold)
!	addr_t		a;
!	struct pte	*pnew;
!	struct pte	*pold;
!
!	o0	running page address
!	o1	running new ppte
!	o2	running old ppte
!	o3	end address	
!	o4	pte
!	g1	PG_V	
!	g2	pte_invalid             
!	g3	pagesize (machine dependent value 4k, or 8k)
!
	ENTRY(hat_pmgswapptes)
	sethi 	%hi(_mmu_pteinvalid), %g1  ! g2 = *mmu_pteinvalid
	ld [%g1 + %lo(_mmu_pteinvalid)], %g2
	set	PG_V, %g1		! g1 PG_V

	set	PAGESIZE, %g3		! g3 = PAGESIZE
	set	PMGRPSIZE, %o3
	add	%o0, %o3, %o3		! o3 = a + PMGRPSIZE

1:
	ld	[%o2], %o4		! old SW pte in o4
	andcc	%o4, %g1, %g0		! valid page?
	be,a	2f
	ld	[%o1], %o4		! (delay slot) new SW pte in o4

	lda	[%o0]ASI_PM, %o4	! read old HW pte
	st	%o4, [%o2]		! store old pte in SW copy

	!
	! XXX - note that we could save a few cycles by always writing
	!	the o4. If its PG_V is 0, we may use it instead of g2.
	!
	ld	[%o1], %o4	
	andcc	%o4, %g1, %g0		! valid page?
	bne,a	3f
	sta	%o4, [%o0]ASI_PM	! (delay slot) write new pte into HW
	b	3f
	sta	%g2, [%o0]ASI_PM	! (delay slot) write new pte into HW

2:
	andcc	%o4, %g1, %g0		! we have new SW pte in o4
	bnz,a	3f			! valid page?
	sta	%o4, [%o0]ASI_PM

3:
	add	%o0, %g3, %o0
	cmp	%o0, %o3		! (a < end address)?
	inc	4, %o2			! o2 += sizeof(pte)
	blu	1b
	inc	4, %o1			! o1 += sizeof(pte)

	retl
	nop  

