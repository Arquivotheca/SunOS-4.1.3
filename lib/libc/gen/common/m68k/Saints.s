	.data
|	.asciz	"@(#)Saints.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

#ifdef PIC
#define DUMMY(f) \
	.globl S/**/f ; \
S/**/f:	movl	a2,sp@- ;\
	JBSR(F/**/f,a2) ;\
	movl	sp@+,a2 ;\
	RET
#else
#define DUMMY(f) .globl S/**/f ; S/**/f: jmp F/**/f
#endif
				| Things the Sky board can't do better than software.
	DUMMY(aints)
	DUMMY(anints)
	DUMMY(arints)
	DUMMY(floors)
	DUMMY(ceils)

