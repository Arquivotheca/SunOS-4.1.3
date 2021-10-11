/*	@(#)setjmp.h 1.1 92/07/30 SMI; from UCB 4.1 83/05/03	*/

#ifndef _sun3x_setjmp_h
#define _sun3x_setjmp_h

/*
 * onsstack,sigmask,sp,pc,psl,d2-d7,a2-a6,
 * fp2-fp7, 	for 68881
 * fpa4-fpa15	for FPA
 */
#define	_JBLEN	58

typedef	int jmp_buf[_JBLEN];

/*
 * One extra word for the "signal mask saved here" flag.
 */
typedef	int sigjmp_buf[_JBLEN+1];

int	setjmp(/* jmp_buf env */);
int	_setjmp(/* jmp_buf env */);
int	sigsetjmp(/* sigjmp_buf env, int savemask */);
void	longjmp(/* jmp_buf env, int val */);
void	_longjmp(/* jmp_buf env, int val */);
void	siglongjmp(/* sigjmp_buf env, int val */);

/*
 * Routines that call setjmp have strange control flow graphs,
 * since a call to a routine that calls resume/longjmp will eventually
 * return at the setjmp site, not the original call site.  This
 * utterly wrecks control flow analysis.
 */
#pragma unknown_control_flow(sigsetjmp, setjmp, _setjmp)

/*
 * On the 68000, there is the additional problem that registers
 * are restored to their values at the time of the call to setjmp;
 * they should be set to their values at the time of the call to
 * whoever eventually called longjmp.  Thus, on routines that call
 * setjmp, automatic register allocation must be suppressed.
 */
#pragma makes_regs_inconsistent(sigsetjmp, setjmp, _setjmp)

#endif	/* !_sun3x_setjmp_h */
