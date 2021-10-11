
	.seg	"data"
	.asciz	"@(#)Fanint.S 1.1 92/07/30 SMI"

#define LOCORE
#include <machine/asm_linkage.h>

!	Copyright (c) 1988 by Sun Microsystems, Inc.

	.global	_r_anint_
	.seg	"text"
	ENTRY(Fanint)
	save	%sp,-SA(MINFRAME+8),%sp
	st	%i0,[%fp-8]
	call	_r_anint_
	sub	%fp,8,%o0
	ret
	restore
