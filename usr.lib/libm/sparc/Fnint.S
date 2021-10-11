
	.seg	"data"
	.asciz	"@(#)Fnint.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

!	Copyright (c) 1988 by Sun Microsystems, Inc.

	.global	_ir_nint_
	.seg	"text"
	ENTRY(Fnint)
	save	%sp,-SA(MINFRAME+8),%sp
	st	%i0,[%fp-8]
	call	_ir_nint_
	sub	%fp,8,%o0
	ret
	restore
