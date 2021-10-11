/*
 *	.seg	"data"
 *	.asciz	"@(#)stubs1.s 1.1 92/07/30"
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include <sun4/asm_linkage.h>

#define	SYS_mountroot	0x0
#define	SYS_open	0x5
#define	SYS_read	0x3
#define	SYS_close	0x6
#define	SYS_lseek	0x13
#define	NW		7
#define	REGSIZE		76
#define	SAVE_ARGS	0x1f0000
#define	SAVE_RESULTS	0x1f0100

#define	TRAP_VECTOR_BASE	0xffe810d8

#define	PSR_ET		0x00000020	/* enable traps */

#define	SIZE_TRAP	1024

	.data
	.globl	mountroot_psr
	.align	4
_save_args1:
	.skip	4*4
_save_args:
	.skip	4*4
_save_return:
	.skip	4
_save_stat:
	.skip	4
_trap_window:
	.skip	4*24
temp_trap:
	.skip	4*4096
mountroot_psr:
	.skip	4
_save_sp:
	.skip	4
_user_window:
	.skip	WINDOWSIZE


	.text
 	.seg	"text"
 	.align	4

	.globl	_mountroot
_mountroot:
	!#PROLOGUE# 0
	sethi	%hi(LF15),%g1
	add	%g1,%lo(LF15),%g1
	save	%sp,%g1,%sp
	st	%sp, [_save_sp]
	!#PROLOGUE# 1
	set	_user_window, %o3
	SAVE_WINDOW(%o3);
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_mountroot, %o0
	! This lunacy is to get around the window being lost
	! in the PROM on trap..
	set	SAVE_ARGS, %i0
	st	%o0, [%i0+0x0]
	st	%o1, [%i0+0x4]
	st	%o2, [%i0+0x8]
	st	%o3, [%i0+0xc]
	! End of lunacy
	t	0;
	nop
	set	_user_window, %o3
	RESTORE_WINDOW(%o3)
	nop
	ld	[_save_sp], %sp
	! More lunacy to take account of the PROM
	set	SAVE_RESULTS, %o1
	ld	[%o1], %o0
	mov	%o0,%i0
	! End of more lunacy
	ret
	restore
	LF15 = -64
	LP15 = 64
	LT15 = 64

	.globl _open;
_open:
	!#PROLOGUE# 0
	sethi	%hi(LF15),%g1
	add	%g1,%lo(LF15),%g1
	save	%sp,%g1,%sp
	st	%sp, [_save_sp]
	!#PROLOGUE# 1
	set	_user_window, %o3
	SAVE_WINDOW(%o3);
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_open, %o0
	! This lunacy is to get around the window being lost
	! in the PROM on trap..
	set	SAVE_ARGS, %i0
	st	%o0, [%i0+0x0]
	st	%o1, [%i0+0x4]
	st	%o2, [%i0+0x8]
	st	%o3, [%i0+0xc]
	! End of lunacy
	nop
	nop
	t	0
	nop
	nop
	set	_save_stat, %o1
	st	%o0, [%o1]
	nop
	nop
	set	_user_window, %o3
	RESTORE_WINDOW(%o3)
	nop
	nop
	ld	[_save_sp], %sp
	! More lunacy to take account of the PROM
	set	SAVE_RESULTS, %o1
	ld	[%o1], %o0
	mov	%o0,%i0
	! End of more lunacy
	ret
	restore
	nop

	.globl	_read
_read:
	!#PROLOGUE# 0
	sethi	%hi(LF15),%g1
	add	%g1,%lo(LF15),%g1
	save	%sp,%g1,%sp
	st	%sp, [_save_sp]
	!#PROLOGUE# 1
	set	_user_window, %o3
	SAVE_WINDOW(%o3);
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_read, %o0
	! This lunacy is to get around the window being lost
	! in the PROM on trap..
	set	SAVE_ARGS, %i0
	st	%o0, [%i0+0x0]
	st	%o1, [%i0+0x4]
	st	%o2, [%i0+0x8]
	st	%o3, [%i0+0xc]
	! End of lunacy
	nop
	nop
	t	0
	nop
	nop
	set	_save_stat, %o1
	st	%o0, [%o1]
	nop
	nop
	ld	[_save_sp], %sp
	! More lunacy to take account of the PROM
	set	SAVE_RESULTS, %o1
	ld	[%o1], %o0
	mov	%o0,%i0
	! End of more lunacy
	nop
	ret
	restore
	nop

	.globl	_close
_close:
	!#PROLOGUE# 0
	sethi	%hi(LF15),%g1
	add	%g1,%lo(LF15),%g1
	save	%sp,%g1,%sp
	st	%sp, [_save_sp]
	!#PROLOGUE# 1
	set	_user_window, %o3
	SAVE_WINDOW(%o3);
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_close, %o0
	! This lunacy is to get around the window being lost
	! in the PROM on trap..
	set	SAVE_ARGS, %i0
	st	%o0, [%i0+0x0]
	st	%o1, [%i0+0x4]
	st	%o2, [%i0+0x8]
	st	%o3, [%i0+0xc]
	! End of lunacy
	nop
	nop
	t	0
	nop
	nop
	set	_save_stat, %o1
	st	%o0, [%o1]
	nop
	nop
	ld	[_save_sp], %sp
	! More lunacy to take account of the PROM
	set	SAVE_RESULTS, %o1
	ld	[%o1], %o0
	mov	%o0,%i0
	! End of more lunacy
	nop
	ret
	restore
	nop

	.globl	_lseek
_lseek:
	!#PROLOGUE# 0
	sethi	%hi(LF15),%g1
	add	%g1,%lo(LF15),%g1 
	save	%sp,%g1,%sp
	st	%sp, [_save_sp]
	!#PROLOGUE# 1
	set	_user_window, %o3
	SAVE_WINDOW(%o3);
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_lseek, %o0
	! This lunacy is to get around the window being lost
	! in the PROM on trap..
	set	SAVE_ARGS, %i0
	st	%o0, [%i0+0x0]
	st	%o1, [%i0+0x4]
	st	%o2, [%i0+0x8]
	st	%o3, [%i0+0xc]
	! End of lunacy
	nop
	nop
	t	0
	nop
	nop
	set	_save_stat, %o1
	st	%o0, [%o1]
	nop
	nop
	ld	[_save_sp], %sp
	! More lunacy to take account of the PROM
	set	SAVE_RESULTS, %o1
	ld	[%o1], %o0
	mov	%o0,%i0
	! End of more lunacy
	nop
	ret
	restore
	nop

/*
 * Get vector base register
 */
	ENTRY(getvbr1)
	retl				! leaf routine return
	mov	%tbr, %o0		! read trap base register

/*
 * Make sure the vector for 'trap #0'
 */
	.globl	_set_vec
_set_vec:
	set	TRAP_VECTOR_BASE, %l0
	ld	[%l0], %l0
	set	systrap, %o0
	st	%o0, [%l0 + 0x200]	! write new vector
	nop
	nop
	retl				! done, leaf routine return
	nop

/*
 * System call handler.
 */
systrap:
	!#PROLOGUE# 0
	sethi	%hi(LF15),%g1
	add	%g1,%lo(LF15),%g1
	save	%sp,%g1,%sp
	st	%sp, [_save_sp]
	!#PROLOGUE# 1
	nop
	nop	! psr delay
	nop
	! This lunacy is to get around the window being lost
	! in the PROM on trap..
	set	SAVE_ARGS, %o0
	ld	[%o0+0x0], %i0
	ld	[%o0+0x4], %i1
	ld	[%o0+0x8], %i2
	ld	[%o0+0xc], %i3
	! End of lunacy
	set	_save_args, %o1
	st	%i1, [%o1+0]
	st	%i2, [%o1+4]
	st	%i3, [%o1+8]
	mov	%i0, %o0		! which syscall
	nop
	nop
	call	_syscall		! syscall(rp)
	! add	%sp, MINFRAME, %o0	! ptr to reg struct
	nop
	set	SAVE_RESULTS, %o1
	st	%o0, [%o1]
	mov	%o0, %i0
	ret
	restore
	nop
	/* end syscall */
