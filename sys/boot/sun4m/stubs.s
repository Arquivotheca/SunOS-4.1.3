/*
 *	.seg	"data"
 *	.asciz	"@(#)stubs.s	1.1 7/30/92 SMI"
 *	Copyright (c) 1988,1990 by Sun Microsystems, Inc.
 */
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <machine/psl.h>

#define	SYS_mountroot	0x0
#define SYS_exitto	0x1
#define	SYS_read	0x3
#define	SYS_open	0x5
#define	SYS_close	0x6
#define	SYS_reopen	0x7
#define	SYS_lseek	0x13

	.text
 	.seg	"text"
 	.align	4

	ENTRY(mountroot)
	save	%sp, -SA(MINFRAME), %sp
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_mountroot, %o0
	t	0;
	mov	%o0, %i0
	ret
	restore

	ENTRY(exitto)
	save	%sp, -SA(MINFRAME), %sp
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_exitto, %o0
	t	0;
	call	__exitto		! __exitto(go2) must be
	mov	%i0, %o0		!    on this side of the trap
	ret
	restore

	ENTRY(open)
	save	%sp, -SA(MINFRAME), %sp
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_open, %o0
	t	0
	mov	%o0, %i0
	ret
	restore

	ENTRY(read)
	save	%sp, -SA(MINFRAME), %sp
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_read, %o0
	t	0
	mov	%o0, %i0
	ret
	restore

	ENTRY(close)
	save	%sp, -SA(MINFRAME), %sp
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_close, %o0
	t	0
	mov	%o0, %i0
	ret
	restore

	ENTRY(reopen)
	save	%sp, -SA(MINFRAME), %sp
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_reopen, %o0
	t	0
	mov	%o0, %i0
	ret
	restore

	ENTRY(lseek)
	save	%sp, -SA(MINFRAME), %sp
	mov	%i2, %o3
	mov	%i1, %o2
	mov	%i0, %o1
	mov	SYS_lseek, %o0
	t	0
	mov	%o0,%i0
	ret
	restore

