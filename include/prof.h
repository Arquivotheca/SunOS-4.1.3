/*	@(#)prof.h 1.1 92/07/30 SMI; from S5R2 1.3	*/

#ifndef _prof_h
#define _prof_h

#ifndef MARK
#define MARK(L)	{}
#else
#undef MARK
#ifdef vax
#define MARK(L)	{\
		asm("	.data");\
		asm("	.align	2");\
		asm(".L.:");\
		asm("	.long	0");\
		asm("	.text");\
		asm("M.L:");\
		asm("	nop;nop");\
		asm("	movab	.L.,r0");\
		asm("	jsb	mcount");\
		}
#endif
#ifdef u3b
#define MARK(L)	{\
		asm("	.data");\
		asm("	.align	4");\
		asm(".L.:");\
		asm("	.word	0");\
		asm("	.text");\
		asm("M.L:");\
		asm("	movw	&.L.,%r0");\
		asm("	jsb	_mcount");\
		}
#endif
#ifdef pdp11
#define MARK(L)	{\
		asm("	.bss");\
		asm(".L.:");\
		asm("	.=.+2");\
		asm("	.text");\
		asm("M.L:");\
		asm("	mov	$.L.,r0");\
		asm("	jsr	pc,mcount");\
		}
#endif
#ifdef mc68000
#define MARK(L)	{\
		asm("	.bss");\
		asm("	.even");\
		asm(".L.:");\
		asm("	.skip	4");\
		asm("	.text");\
		asm("M.L:");\
		asm("	movl	#.L.,a0");\
		asm("	jsr	mcount");\
		}
#endif
#ifdef sparc
#define MARK(L) {\
		asm("	.reserve	.L., 4, \"data\", 4");\
		asm("M.L:");\
		asm("	sethi	%hi(.L.), %o0");\
		asm("	call	mcount");\
		asm("	or	%o0, %lo(.L.), %o0");\
		}
#endif
#endif

#endif /*!_prof_h*/
