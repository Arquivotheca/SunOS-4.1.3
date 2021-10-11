!	.seg	"data"
!	.asciz	"@(#)_setjmp.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

#include <sun4/asm_linkage.h>
#include <sun4/trap.h>

SC_ONSTACK	= (0*4)	! offsets in sigcontext structure
SC_MASK		= (1*4)
SC_SP		= (2*4)
SC_PC		= (3*4)
SC_NPC		= (4*4)
SC_PSR		= (5*4)
SC_G1		= (6*4)
SC_O0		= (7*4)

SS_SP		= (0*4)	! offset in sigstack structure
SS_ONSTACK	= (1*4)
SIGSTACKSIZE	= (2*4)

/*
 * _setjmp(buf_ptr)
 * buf_ptr points to a eight word array (jmp_buf)
 * which is a sigcontext structure.
 */
#ifdef S5EMUL
	ALTENTRY(setjmp)
#endif
	ENTRY(_setjmp)
	clr	[%o0 + SC_ONSTACK]	! clear onstack, sigmask
	clr	[%o0 + SC_MASK]
	st	%sp, [%o0 + SC_SP]	! save caller's sp
	add	%o7, 8, %g1		! comupte return pc
	st	%g1, [%o0 + SC_PC]	! save pc
	inc	4, %g1			! npc = pc + 4
	st	%g1, [%o0 + SC_NPC]
	clr	[%o0 + SC_PSR]		! psr (icc), g1 = 0 (paranoid)
	clr	[%o0 + SC_G1]		!   o0 filled in by longjmp
	retl
	clr	%o0			! return (0)

/*
 * _longjmp(buf_ptr, val)
 * buf_ptr points to a sigcontext which has been initialized by _setjmp.
 * val is the value we wish to return to _setjmp's caller
 *
 * We flush the register file to the stack by doing a kernel call.
 * This is necessary to ensure that the registers we want to
 * pick up are stored on the stack. A new frame is also made on
 * the current top of stack before adjusting fp in case we are
 * returning to our caller. Then, we set fp from the saved fp.
 * The final restore causes the window pointed to by the saved
 * fp to be restored.
 */
#ifdef S5EMUL
	ALTENTRY(longjmp)
#endif
	ENTRY(_longjmp)
	t	ST_FLUSH_WINDOWS	! flush all reg windows to the stack.
	sub	%sp, WINDOWSIZE, %sp	! establish new save area before
	ld	[%o0 + SC_SP], %fp	!  adjusting fp to new stack frame
	ld	[%o0 + SC_PC], %g1	! get new return pc
	tst	%o1			! is return value 0?
	bne	1f			! no - leave it alone
	sub	%g1, 8, %o7		! normalize return (for adb) (dly slot)
	mov	1, %o1			! yes - set it to one
1:
	retl
	restore	%o1, 0, %o0		! return (val)
