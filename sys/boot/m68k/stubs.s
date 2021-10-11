/*
 *	.asciz	"@(#)stubs.s 1.1 92/07/30 SMI"
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#define	SYS_mountroot	0x0
#define	SYS_exitto	0x1
#define	SYS_open	0x5
#define	SYS_read	0x3
#define	SYS_close	0x6
#define SYS_reopen	0x7
#define	SYS_lseek	0x13

	.globl	_mountroot
_mountroot:
	pea	SYS_mountroot
	trap	#0
	addql	#4,sp
	rts

	.globl	_exitto
	.globl	__exitto
_exitto:
	pea	SYS_exitto
	trap	#0
	addql	#4,sp
	jmp	__exitto
|	rts			| __exitto returns...

	.globl _open;
_open:
	pea	SYS_open
	trap	#0
	addql	#4,sp
	rts

	.globl	_read
_read:
	pea	SYS_read
	trap	#0
	addql	#4,sp
	rts

	.globl	_close
_close:
	pea	SYS_close
	trap	#0
	addql	#4,sp
	rts

	.globl	_reopen
_reopen:
	pea	SYS_reopen
	trap	#0
	addql	#4,sp
	rts

	.globl	_lseek
_lseek:
	pea	SYS_lseek
	trap	#0
	addql	#4,sp
	rts
