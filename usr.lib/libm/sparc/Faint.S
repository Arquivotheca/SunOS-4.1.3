
	.seg	"data"
	.asciz	"@(#)Faint.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

!	Copyright (c) 1988 by Sun Microsystems, Inc.

	.global	_r_aint_
	.seg	"text"
	ENTRY(Faint)
	save	%sp,-SA(MINFRAME+8),%sp
	st	%i0,[%fp-8]
	call	_r_aint_
	sub	%fp,8,%o0
	ret
	restore
