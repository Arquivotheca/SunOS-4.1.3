!	@(#)simutil.s 1.3 88/02/08 SMI
!	Copyright (c) 1987 by Sun Microsystems, Inc.

	.seg	"text"
	.align	4

#include <machine/asm_linkage.h>
#include <sys/syscall.h>
#include "assym.s"
#include <machine/mmu.h>

/*
 * This is for the behavioral model; the syscall traps will really cause
 * us to panic, but the printf stuff will work.
 */

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
/*
 * simcoutc(c) char c;
 */
#define UART_1N	0	/* each character followed by a new-line */
#define UART_4N	4	/* 4 characters, then a new-line */
#define UART_4	8	/* 4 characters, no new line */
	.global _simcoutc
_simcoutc:
	set	UART_BYPASS, %g1
	mov	UART_4N, %g2
	retl
	sta	%o0, [%g1 + %g2] ASI_CTL

	.global	_simcinc
_simcinc:
	t	252
	retl
	nop
