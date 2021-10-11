!
!	"@(#)SYS.h 1.1 92/07/30"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"
	.align	4

#include <sys/syscall.h>
#include "PIC.h"

#define WINDOWSIZE	(16*4)

#ifdef PROF
	.global mcount

#define ENTRY(x) \
	.global	_/**/x; \
_/**/x: ; \
	save	%sp, -WINDOWSIZE, %sp; \
	set	_/**/x/**/1, %o0; \
	call	mcount; \
	nop; \
	restore; \
	.reserve	_/**/x/**/1, 4, "data", 4
	
#else PROF
#define ENTRY(x) \
	.global	_/**/x; \
_/**/x:
#endif PROF

	.global	cerror

#define SYSCALL(x) \
	ENTRY(x); \
	mov	SYS_/**/x, %g1; \
	t	0; \
	CERROR(o5)

#define BSDSYSCALL(x) \
	ENTRY(_/**/x); \
	mov	SYS_/**/x, %g1; \
	t	0; \
	CERROR(o5)

#define PSEUDO(x, y) \
	ENTRY(x); \
	mov	SYS_/**/y, %g1; \
	t	0;

#define RET	retl; nop;

#ifdef PIC
#define CERROR(free_reg) \
	bcc	noerr; \
	PIC_SETUP(free_reg); \
	.empty;	\
	ld	[%free_reg+ cerror],%free_reg; \
	jmp	%free_reg; \
	.empty;	\
noerr:	nop;	
#else
#define CERROR(free_reg) \
	bcc 	noerr; \
	.empty; \
	sethi	%hi(cerror), %free_reg;\
	or	%free_reg, %lo(cerror), %free_reg;\
	jmp	%free_reg;\
	.empty;\
noerr: nop;
#endif
