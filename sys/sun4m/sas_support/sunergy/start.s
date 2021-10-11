! @(#)start.s	1.1 7/30/92 SMI
	.seg	"data"
	.seg	"text"

	.global	_do_boot, _do_enter, _do_exit, _do_startcpu, _start
	.global _do_cstart, _do_cexit
	.global _romp
!	
! start: get here from a reset trap.
!

_start:
	set	0x78000, %g4		! initial context tbl ptr reg
	set	0x100, %g5	! RMMU_CTP_REG
	sta	%g4, [%g5]0x4		! locate context table
	set	0x200, %g5	! RMMU_CTX_REG
	sta	%g0, [%g5]0x4		! set context number
	set	1, %g4
	set	0x000, %g5	! RMMU_CTL_REG

	set	_do_boot, %g1
	sta	%g4, [%g5]0x4		! turn on mmu
	jmp	%g1			! use prom in high memory
	nop

!
! do_boot: vector here to initiate system boot.
!	processor wanders into kernel at 0x4000.
!

_do_boot:
	set	0xFA0, %g1		! pil=15, s=1, et=1
	mov	%g1, %psr		! initialize psr
	nop ; nop
	set	0x2, %g1
	mov	%g1, %wim		! initialize wim
	set	address_0, %g1
	mov	%g1, %tbr		! initialize trap table base

	set	0xFFD7EFF8, %g3		! set up stack (double aligned)
	mov	%g3, %sp
	mov	%g0, %fp		! terminate frame link chain
	set 	_romvec, %o0
	set 	_romp, %o1
	call	_do_cstart		! do what's required (if anything)
	st	%o0, [%o1]		! initialize romp 

	set	_romvec, %o0		! romp
	mov	%g0, %o1		! dvec

	set	0x4000, %g3
	jmp	%g3
	nop

_do_enter:
	b,a	_do_enter

_do_exit:
	call	_do_cexit
	nop
	b,a	_do_enter

_do_startcpu:		! (cpuid, pc, ctx)
	b,a	_do_startcpu
