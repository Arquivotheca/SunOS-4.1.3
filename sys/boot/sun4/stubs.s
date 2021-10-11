/*
 *	.seg	"data"
 *	.asciz	"@(#)stubs.s 1.1 92/07/30 SMI"
 *	Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <sun4/asm_linkage.h>

#define	SYS_mountroot	0x0
#define	SYS_exitto	0x1
#define	SYS_open	0x5
#define	SYS_read	0x3
#define	SYS_close	0x6
#define SYS_reopen	0x7
#define	SYS_lseek	0x13
#define	NW		7
#define	REGSIZE		76
#define	SAVE_ARGS	BRELOC-ARGPUSHSIZE
#define	SAVE_RESULTS	BRELOC-8

#define	TRAP_VECTOR_BASE	0xffe810d8

#define	PSR_ET		0x00000020	/* enable traps */

#define	SIZE_TRAP	1024

	.data
	.align	4
_save_stat:
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
	set	_save_sp, %g1
	st	%sp, [%g1]
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
	set	_save_sp, %g1
	ld	[%g1], %sp
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
	set	_save_sp, %g1
	st	%sp, [%g1]
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
	set	_save_sp, %g1
	ld	[%g1], %sp
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
	set	_save_sp, %g1
	st	%sp, [%g1]
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
	set	_save_sp, %g1
	ld	[%g1], %sp
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
	set	_save_sp, %g1
	st	%sp, [%g1]
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
	set	_save_sp, %g1
	ld	[%g1], %sp
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
	set	_save_sp, %g1
	st	%sp, [%g1]
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
	set	_save_sp, %g1
	ld	[%g1], %sp
	! More lunacy to take account of the PROM
	set	SAVE_RESULTS, %o1
	ld	[%o1], %o0
	mov	%o0,%i0
	! End of more lunacy
	nop
	ret
	restore
	nop
	
	.globl	_exitto
_exitto:
	!#PROLOGUE# 0
	sethi	%hi(LF15),%g1
	add	%g1,%lo(LF15),%g1 
	save	%sp,%g1,%sp
	set	_save_sp, %g1
	st	%sp, [%g1]
	!#PROLOGUE# 1
	set	_user_window, %o3
	SAVE_WINDOW(%o3);
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_exitto, %o0
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
	set	_save_sp, %g1
	ld	[%g1], %sp
	! More lunacy to take account of the PROM
	set	SAVE_RESULTS, %o1
	ld	[%o1], %o0
	mov	%o0,%i0
	! End of more lunacy
	nop
	call	__exitto		! __exitto(go2) must be on this side
	mov	%i0, %o0		!  of trap
	ret
	restore
	nop

	.globl _reopen;
_reopen:
	!#PROLOGUE# 0
	sethi	%hi(LF15),%g1
	add	%g1,%lo(LF15),%g1
	save	%sp,%g1,%sp
	set	_save_sp, %g1
	st	%sp, [%g1]
	!#PROLOGUE# 1
	set	_user_window, %o3
	SAVE_WINDOW(%o3);
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_reopen, %o0
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
	set	_save_sp, %g1
	ld	[%g1], %sp
	! More lunacy to take account of the PROM
	set	SAVE_RESULTS, %o1
	ld	[%o1], %o0
	mov	%o0,%i0
	! End of more lunacy
	ret
	restore
	nop
