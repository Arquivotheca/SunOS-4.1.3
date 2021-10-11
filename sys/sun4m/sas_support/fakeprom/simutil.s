#ident "@(#)simutil.s	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */


	.seg	"text"
	.align	4

#include <machine/asm_linkage.h>
#include <sys/syscall.h>

/*
 * SAS uses three special traps to simulate system calls and io
 * trap 210 	simualted system call
 *		the system call number is in %g1
 * trap 252	simulated console input character, cngetc
 *		sas will generate a level12 interrupt when a input
 *		character is avaliable, at that time cngetc should be called
 *		which returns the character in %o0
 * trap 253	simulated console output character, cnoutc
 *		the character to output should be placed in %o0
 */

	.global	_errno

#define SYSCALL(x) \
	ENTRY(s_/**/x); \
	mov	SYS_/**/x, %g1; \
	t	210; \
	bcs	cerror; \
	nop; \
	retl; \
	nop

/*
 * Simulator system calls, (SAS -m), used to implement
 * simulated disk i/o from the driver level of the kernel.
 */

	SYSCALL(open)

	SYSCALL(close)

	SYSCALL(read)

	SYSCALL(write)

	SYSCALL(lseek)

	SYSCALL(ioctl)

	SYSCALL(bind)

	SYSCALL(sendto)

	SYSCALL(recv)

	SYSCALL(socket)

cerror:
	sethi	%hi(_errno), %g1;
	st	%o0, [%g1 + %lo(_errno)];
	retl;
	mov	-1, %o0;

/*
 * traps to the simulator for character i/o
 */
	.global _simcoutc
_simcoutc:
	t	253
	retl
	nop

	.global	_simcinc
_simcinc:
	t	252
	retl
	nop
