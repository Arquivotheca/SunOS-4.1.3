	.ident "@(#)map.s 1.1 92/07/30 SMI"

/*
 * Copyright 1988-1989 Sun Microsystems, Inc.
 */

/*
 * Sun-4c MMU and Virtual Address Cache routines.
 *
 * Notes:
 *
 * - Supports write-through caches only.
 * - Hardware cache flush must work in page size chunks.
 * - Cache size <= 1 MB, line size <= 256 bytes.
 * - Assumes vac_flush() performance is not critical.
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
! Read the page map entry for the given address v
! and return it in a form suitable for software use.
!
! u_int
! map_getpgmap(v)
!	addr_t v;
!
	ENTRY(map_getpgmap)
	andn	%o0, 0x3, %o1		! align to word boundary
	retl
	lda	[%o1]ASI_PM, %o0	! read page map entry

!
! Set the pme for address v using the software pte given.
!
! map_setpgmap(v, pte)
!	addr_t v;
!	u_int pte;
!
	ENTRY(map_setpgmap)
	andn	%o0, 0x3, %o2		! align to word boundary
	retl
	sta	%o1, [%o2]ASI_PM	! write page map entry

!
! Return the 16 bit segment map entry for the given segment number.
!
! u_int
! map_getsgmap(v)
!	addr_t v;
!
	ENTRY(map_getsgmap)
	lduba	[%o0]ASI_SM, %o0	! read segment number
	retl
	and     %o0, 0xff, %o0		! mask off top bit

!
! Set the segment map entry for segno to pm.
!
! map_setsgmap(v, pm)
!	addr_t v;
!	u_int pm;
!
	ENTRY(map_setsgmap)
	retl
	stba	%o1, [%o0]ASI_SM	! write segment entry

!
! Return the current context number.
!
! u_int
! map_getctx()
!
	ENTRY(map_getctx)
	set	CONTEXT_REG, %o1
	retl
	lduba	[%o1]ASI_CTL, %o0	! read the context register

!
! Set the current context number.
!
! map_setctx(c)
!	u_int c;
!
	ENTRY(map_setctx)
	set	CONTEXT_REG, %o1
	retl
	stba	%o0, [%o1]ASI_CTL	! write the context register

/*
 * cache config word: _vac_info
 */
#if 0
struct {
	vi_size : 21;		/* cache size, bytes */
	vi_vac : 1;		/* vac enabled */
	vi_hw : 1;		/* HW flush */
	vi_linesize : 9;	/* line size, bytes */
} vac_info;
#endif 0

	.reserve vac_info, 4, "bss", 4

#define	VAC_INFO_VSS		11		/* VAC size shift (32 - 21) */
#define	VAC_INFO_VAC		(0x400)		/* VAC bit */
#define	VAC_INFO_HW		(0x200)		/* HW flush bit */
#define	VAC_INFO_LSM		(0x1ff)		/* line size mask */

/* note: safe in .empty slot */

#define	GET(val,p,d) \
	sethi %hi(val), p; \
	ld [p + %lo(val)], d ;

#define	GET_INFO(p,d)	GET(vac_info,p,d)

!
! Set up VAC config word from imported variables, set vac, clear tags,
! enable/disable cache.
!
! void
! vac_control(on)
!
	ENTRY(vac_control)

! encode VAC params
	GET(_vac_size, %g1, %o1)		! get VAC size
	GET(_vac_hwflush, %g2, %o2)		! get HW flush flag
	GET(_vac_linesize, %g1, %o3)		! get line size
	sll	%o1, VAC_INFO_VSS, %o1		! encode VAC size
	sethi	%hi(_vac), %g2			! prepare to write "vac"
	tst	%o2				! HW flush?
	bz	1f
	st	%o0, [%g2 + %lo(_vac)]		! write "vac"
	bset	VAC_INFO_HW, %o1		! encode HW flush
1:	tst	%o0				! enabling VAC?
	bz	2f
	bset	%o3, %o1			! merge line size
	bset	VAC_INFO_VAC, %o1		! encode VAC enabled
2:	sethi	%hi(vac_info), %g1		! prepare to write vac_info
	set	_off_enablereg, %g2		! prepare to disable
	bz	3f
	st	%o1, [%g1 + %lo(vac_info)]	! write vac_info

! enabling cache
	mov	%o7, %g1			! save ret addr
	call	_vac_flushall			! flush cache
	.empty
	set	_on_enablereg, %g2		! prepare to enable
	mov	%g1, %o7			! restore ret addr

! do it
3:	jmp	%g2
	mov	ENA_CACHE, %o0

!
! Initialize the cache by invalidating all the cache tags.
! DOES NOT turn on cache enable bit in the enable register.
!
! void
! vac_flushall()
!
	ENTRY(vac_flushall)
	GET_INFO(%o5, %o2)		! get VAC info
	set	CACHE_TAGS, %o3		! address of cache tags in CTL space
	srl	%o2, VAC_INFO_VSS, %o1	! cache size
	and	%o2, VAC_INFO_LSM, %o2	! line size

#ifdef SIMUL
	/*
	 * Don't clear entire cache in the hardware simulator,
	 * it takes too long and the simulator has already cleared it
	 * for us.
	 */
	set	256, %o1		! initialize only 256 bytes worth
#endif SIMUL

1:	subcc	%o1, %o2, %o1		! done yet?
	bg	1b
	sta	%g0, [%o3+%o1]ASI_CTL	! clear tag

	retl
	.empty

!
! Flush a context from the cache.
! To flush a context we must cycle through all lines of the
! cache issuing a store into alternate space command for each
! line whilst the context register remains constant.
!
! void
! vac_ctxflush()
!
	ENTRY(vac_ctxflush)
	GET_INFO(%o5, %o2)		! get VAC info
	sethi	%hi(_flush_cnt+FM_CTX), %g1 ! get flush count
	btst	VAC_INFO_VAC, %o2	! check if cache is turned on
	bz	9f			! cache off, return
	srl	%o2, VAC_INFO_VSS, %o1	! cache size
	ld	[%g1 + %lo(_flush_cnt+FM_CTX)], %g2 ! get flush count
	btst	VAC_INFO_HW, %o2	! HW flush?
	inc	%g2			! increment flush count
	bz	2f			! use SW flush
	st	%g2, [%g1 + %lo(_flush_cnt+FM_CTX)] ! store flush count

! hardware flush

	set	NBPG, %o2

1:	subcc	%o1, %o2, %o1
	bg	1b
	sta	%g0, [%o1]ASI_FCC_HW

	retl
	.empty

! software flush

2:	and	%o2, VAC_INFO_LSM, %o2	! line size
	add	%o2, %o2, %o3		! LS * 2
	add	%o2, %o3, %o4		! LS * 3
	add	%o2, %o4, %o5		! LS * 4
	add	%o2, %o5, %g1		! LS * 5
	add	%o2, %g1, %g2		! LS * 6
	add	%o2, %g2, %g3		! LS * 7
	add	%o2, %g3, %g4		! LS * 8

3:	subcc	%o1, %g4, %o1
	sta	%g0, [%o1 + %g0]ASI_FCC
	sta	%g0, [%o1 + %o2]ASI_FCC
	sta	%g0, [%o1 + %o3]ASI_FCC
	sta	%g0, [%o1 + %o4]ASI_FCC
	sta	%g0, [%o1 + %o5]ASI_FCC
	sta	%g0, [%o1 + %g1]ASI_FCC
	sta	%g0, [%o1 + %g2]ASI_FCC
	bg	3b
	sta	%g0, [%o1 + %g3]ASI_FCC

9:	retl
	.empty

!
! Flush a segment from the cache.
! To flush the argument segment from the cache we hold the bits that
! specify the segment in the address constant and issue a store into
! alternate space command for each line.
!
! vac_segflush(v)
!	addr_t v;
!
	ENTRY(vac_segflush)
	GET_INFO(%o5, %o2)		! get VAC info
	sethi	%hi(_flush_cnt+FM_SEGMENT), %g1 ! get flush count
	btst	VAC_INFO_VAC, %o2	! check if cache is turned on
	bz	9f			! cache off, return
	srl	%o2, VAC_INFO_VSS, %o1	! cache size
	ld	[%g1 + %lo(_flush_cnt+FM_SEGMENT)], %g2 ! get flush count
	btst	VAC_INFO_HW, %o2	! HW flush?
	inc	%g2			! increment flush count
	srl	%o0, PMGRPSHIFT, %o0	! get segment part of address
	sll	%o0, PMGRPSHIFT, %o0
	bz	2f			! use SW flush
	st	%g2, [%g1 + %lo(_flush_cnt+FM_SEGMENT)] ! store flush count

! hardware flush

	set	NBPG, %o2

1:	subcc	%o1, %o2, %o1
	bg	1b
	sta	%g0, [%o0 + %o1]ASI_FCS_HW

	retl
	.empty

! software flush

2:	and	%o2, VAC_INFO_LSM, %o2	! line size
	add	%o2, %o2, %o3		! LS * 2
	add	%o2, %o3, %o4		! LS * 3
	add	%o2, %o4, %o5		! LS * 4
	add	%o2, %o5, %g1		! LS * 5
	add	%o2, %g1, %g2		! LS * 6
	add	%o2, %g2, %g3		! LS * 7
	add	%o2, %g3, %g4		! LS * 8

3:	subcc	%o1, %g4, %o1
	sta	%g0, [%o0 + %g0]ASI_FCS
	sta	%g0, [%o0 + %o2]ASI_FCS
	sta	%g0, [%o0 + %o3]ASI_FCS
	sta	%g0, [%o0 + %o4]ASI_FCS
	sta	%g0, [%o0 + %o5]ASI_FCS
	sta	%g0, [%o0 + %g1]ASI_FCS
	sta	%g0, [%o0 + %g2]ASI_FCS
	sta	%g0, [%o0 + %g3]ASI_FCS
	bg	3b
	add	%o0, %g4, %o0

9:	retl
	.empty

!
! Flush a page from the cache.
! To flush the page containing the argument virtual address from
! the cache we hold the bits that specify the page constant and
! issue a store into alternate space command for each line.
!
! vac_pageflush(v)
!	addr_t v;
!
	ENTRY(vac_pageflush)
	GET_INFO(%o5, %o2)		! get VAC info
	sethi	%hi(_flush_cnt+FM_PAGE), %g1 ! get flush count
	btst	VAC_INFO_VAC, %o2	! check if cache is turned on
	bz	9f			! cache off, return
	set	NBPG, %o1		! page size
	ld	[%g1 + %lo(_flush_cnt+FM_PAGE)], %g2 ! get flush count
	btst	VAC_INFO_HW, %o2	! HW flush?
	inc	%g2			! increment flush count
	bz	2f			! use SW flush
	st	%g2, [%g1 + %lo(_flush_cnt+FM_PAGE)] ! store flush count

! hardware flush

	bclr	3,%o0				! force word alignment
	retl
	sta	%g0, [%o0]ASI_FCP_HW

! software flush

2:
#if PGSHIFT <= 13
	bclr	(NBPG - 1), %o0		! get page part of address
#else
	srl	%o0, PGSHIFT, %o0	! get page part of address
	sll	%o0, PGSHIFT, %o0
#endif
	and	%o2, VAC_INFO_LSM, %o2	! line size
	add	%o2, %o2, %o3		! LS * 2
	add	%o2, %o3, %o4		! LS * 3
	add	%o2, %o4, %o5		! LS * 4
	add	%o2, %o5, %g1		! LS * 5
	add	%o2, %g1, %g2		! LS * 6
	add	%o2, %g2, %g3		! LS * 7
	add	%o2, %g3, %g4		! LS * 8

3:	subcc	%o1, %g4, %o1
	sta	%g0, [%o0 + %g0]ASI_FCP
	sta	%g0, [%o0 + %o2]ASI_FCP
	sta	%g0, [%o0 + %o3]ASI_FCP
	sta	%g0, [%o0 + %o4]ASI_FCP
	sta	%g0, [%o0 + %o5]ASI_FCP
	sta	%g0, [%o0 + %g1]ASI_FCP
	sta	%g0, [%o0 + %g2]ASI_FCP
	sta	%g0, [%o0 + %g3]ASI_FCP
	bg	3b
	add	%o0, %g4, %o0

9:	retl
	.empty

!
! Flush a range of addresses.
!
! vac_flush(v, nbytes)
!	addr_t v;
!	u_int nbytes;
!
	ENTRY(vac_flush)
	GET_INFO(%o5, %o2)		! get VAC info
	sethi	%hi(_flush_cnt+FM_PARTIAL), %g1 ! get flush count
	btst	VAC_INFO_VAC, %o2	! check if cache is turned on
	bz	9f			! cache off, return
	srl	%o2, VAC_INFO_VSS, %o3	! cache size
	ld	[%g1 + %lo(_flush_cnt+FM_PARTIAL)], %g2 ! get flush count
	and	%o2, VAC_INFO_LSM, %o2	! line size
	sub	%o2, 1, %o4		! convert to mask (assumes power of 2 )
	inc	%g2			! increment flush count
	st	%g2, [%g1 + %lo(_flush_cnt+FM_PARTIAL)] ! store flush count
	add	%o0, %o1, %o1		! add start to length
	andn	%o0, %o4, %o0		! round down start
	add	%o4, %o1, %o1		! round up end
	andn	%o1, %o4, %o1		! and mask off
	sub	%o1, %o0, %o1		! and subtract start
	cmp	%o1, %o3		! if (nbytes > vac_size)
	bgu,a	1f			! ...
	mov	%o3, %o1		!	nbytes = vac_size
1:	! nop

2:	subcc	%o1, %o2, %o1
	bg	2b
	sta	%g0, [%o0 + %o1]ASI_FCP

9:	retl
	nop

!
! Mark a page as noncachable.
!
! void
! vac_dontcache(p)
!	addr_t p;
!
	ENTRY(vac_dontcache)
	andn	%o0, 0x3, %o1		! align to word boundary
	lda	[%o1]ASI_PM, %o0	! read old page map entry
	set	PG_NC, %o2
	or	%o0, %o2, %o0		! turn on NOCACHE bit
	retl
	sta	%o0, [%o1]ASI_PM	! and write it back out

!
! Flush all user lines from the cache.  We accomplish it by reading a portion
! of the kernel text starting at sys_trap. The size of the portion is
! equal to the VAC size. We read a word from each line. sys_trap was chosen
! as the start address because it is the start of the locore code 
! that we assume will be very likely executed in near future.
!
! XXX - use a HW feature if the cache supports it (e.g. SunRay).
!
	ENTRY(vac_usrflush)
	GET_INFO(%o5, %o2)		! get VAC info
	btst	VAC_INFO_VAC, %o2	! check if cache is turned on
	bnz,a	1f			! cache on
	save	%sp, -SA(MINFRAME), %sp
	retl
	nop

1:
	call	_flush_user_windows	! make sure no windows are hanging out
	nop

	sethi	%hi(_flush_cnt+FM_USR), %g1 ! get flush count
	ld	[%g1 + %lo(_flush_cnt+FM_USR)], %g2 ! get flush count
	!
	! Due to a bug in HW, some processor must map the trap vectors
	! non cacheable. Software (locore.s) must guarantee that the
	! code that follows the trap vectors starts in next page.
	! We are paranoid about it and check that sys_trap is actually
	! in a cacheable page. We panic otherwise.
	!
	tst	%g2
	set	sys_trap, %i0		! start reading text seg. from sys_trap
	bnz	2f
	inc	%g2			! increment flush count

	! check pte only the first time we vac_usrflush is called
	lda	[%i0]ASI_PM, %l0	! read page map entry
	set	PG_NC, %l1
	btst	%l1, %l0
	bz	2f
	nop

	sethi	%hi(6f), %o0
	call	_panic		
	or	%o0, %lo(6f), %o0

	.seg	"data"
6:	.asciz	"vac_usrflush: sys_trap is not in cacheable page"
	.seg	"text"

2:
	st	%g2, [%g1 + %lo(_flush_cnt+FM_USR)] ! store flush count
	GET(_vac_size, %l1, %i1)	! cache size
	and	%i2, VAC_INFO_LSM, %i2	! line size

	!
	! A flush that causes a writeback will happen in parallel
	! with other instructions.  Back to back flushes which cause
	! writebacks cause the processor to wait until the first writeback
	! is finished and the second is initiated before proceeding.
	! Avoid going through the cache sequentially by flushing
	! 16 lines spread evenly through the cache.
	!
	!  i0 start address 
	!  i1 vac_size
	!  i2 linesize

	srl	%i1, 4, %l0		! vac_size / 16
	add	%l0, %l0, %l1		! 2 * (vac_size / 16)
	add	%l1, %l0, %l2	! ...
	add	%l2, %l0, %l3
	add	%l3, %l0, %l4
	add	%l4, %l0, %l5
	add	%l5, %l0, %l6
	add	%l6, %l0, %l7
	add	%l7, %l0, %o0
	add	%o0, %l0, %o1
	add	%o1, %l0, %o2
	add	%o2, %l0, %o3
	add	%o3, %l0, %o4
	add	%o4, %l0, %o5
	add	%o5, %l0, %i4		! 15 * (vac_size / 16)
	
	mov	%l0, %i3		! loop counter: vac_size / 16

3:
	ld	[%i0      ], %i1
	ld	[%i0 + %l0], %i1
	ld	[%i0 + %l1], %i1
	ld	[%i0 + %l2], %i1
	ld	[%i0 + %l3], %i1
	ld	[%i0 + %l4], %i1
	ld	[%i0 + %l5], %i1
	ld	[%i0 + %l6], %i1
	ld	[%i0 + %l7], %i1
	ld	[%i0 + %o0], %i1
	ld	[%i0 + %o1], %i1
	ld	[%i0 + %o2], %i1
	ld	[%i0 + %o3], %i1
	ld	[%i0 + %o4], %i1
	ld	[%i0 + %o5], %i1
	ld	[%i0 + %i4], %i1

	subcc	%i3, %i2, %i3		! decrement loop count
	bg	3b			! are we done yet?
	add	%i0, %i2, %i0		! generate next addr
	ret
	restore
