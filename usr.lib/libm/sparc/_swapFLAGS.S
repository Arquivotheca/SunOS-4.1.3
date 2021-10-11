
	.seg	"data"
	.asciz	"@(#)_swapFLAGS.S 1.1 92/07/30 Copyr 1988 Sun Micro"

#define LOCORE
#include <machine/asm_linkage.h>

! _swapRD(rd)  exchanges rd with the current rounding direction.
! _swapRP(rp)  exchanges rp with the current rounding precision.
! _swapTE(ex)  exchanges ex with the current exception trap enable bits.
! _swapEX(ex)  exchanges ex with the current accrued exception.
!		(see ../C/_swapFLAGS.c)
! Note: we use %sp+0x44 (shadow area of %o0) for temporary space

	.seg	"text"
	.global	_fp_direction, _fp_precision, _fp_accrued_exceptions
 
	ENTRY(_swapTE)
	set	0x0f800000,%o4		! mask of TEM (Trap Enable Mode bits)
	sll	%o0,23,%o1		! shift input to TEM bit location
	st	%fsr,[%sp+0x44]
	ld	[%sp+0x44],%o0		! o0 = fsr
	and	%o1,%o4,%o1
	andn	%o0,%o4,%o2		
	or	%o1,%o2,%o1		! o1 = new fsr
	st	%o1,[%sp+0x44]
	ld	[%sp+0x44],%fsr
	and	%o0,%o4,%o0
	retl
	srl	%o0,23,%o0

	ENTRY(_swapEX)
	set	0x000003e0,%o4		! mask of accrued exception bits
	set	_fp_accrued_exceptions,%o3
	st	%o0,[%o3]
	sll	%o0,5,%o1		! shift input to aexc bit location
	st	%fsr,[%sp+0x44]
	ld	[%sp+0x44],%o0		! o0 = fsr
	and	%o1,%o4,%o1
	andn	%o0,%o4,%o2		
	or	%o1,%o2,%o1		! o1 = new fsr
	st	%o1,[%sp+0x44]
	ld	[%sp+0x44],%fsr
	and	%o0,%o4,%o0
	retl
	srl	%o0,5,%o0

	ENTRY(_swapRD)
	set	0xc0000000,%o4		! mask of rounding direction bits
	set	_fp_direction,%o3
	st	%o0,[%o3]
	sll	%o0,30,%o1		! shift input to RD bit location
	st	%fsr,[%sp+0x44]
	ld	[%sp+0x44],%o0		! o0 = fsr
	and	%o1,%o4,%o1
	andn	%o0,%o4,%o2		
	or	%o1,%o2,%o1		! o1 = new fsr
	st	%o1,[%sp+0x44]
	ld	[%sp+0x44],%fsr
	and	%o0,%o4,%o0
	retl
	srl	%o0,30,%o0

	ENTRY(_swapRP)
	set	0x30000000,%o4		! mask of rounding precision bits
	set	_fp_precision,%o3
	st	%o0,[%o3]
	sll	%o0,28,%o1		! shift input to RP bit location
	st	%fsr,[%sp+0x44]
	ld	[%sp+0x44],%o0		! o0 = fsr
	and	%o1,%o4,%o1
	andn	%o0,%o4,%o2		
	or	%o1,%o2,%o1		! o1 = new fsr
	st	%o1,[%sp+0x44]
	ld	[%sp+0x44],%fsr
	and	%o0,%o4,%o0
	retl
	srl	%o0,28,%o0
