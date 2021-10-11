/* @(#)SYS.h 1.1 92/07/30 SMI; from UCB 4.1 83/05/10 */

#include <syscall.h>
#include "PIC.h"

	.globl	cerror
#ifdef PROF
	.globl  mcount
#define	ENTRY(x)	.globl _/**/x;\
		 _/**/x: link a6,#0;\
			 lea x/**/1,a0;\
		 .data; x/**/1: .long 0; .text;\
			 JBSR(mcount, a1);\
			 unlk a6
#else
#define	ENTRY(x)	.globl _/**/x; _/**/x:
#endif

#define PARAM		sp@(4)
#define PARAM2		sp@(8)
#define	PSEUDO(x,y)	ENTRY(x); pea SYS_/**/y; trap #0
#define	SYSCALL(x)    	err: CERROR(a0); ENTRY(x); pea SYS_/**/x; trap #0; jcs err
#define	BSDSYSCALL(x)	err: CERROR(a0); ENTRY(_/**/x); pea SYS_/**/x; trap #0; jcs err
#define RET		rts

#ifdef PIC
#define CERROR(r) \
			PIC_SETUP(r); \
			movl	r@(cerror:w),r; \
			jmp	r@;
#else 
#define CERROR(r)	jmp cerror;
#endif
