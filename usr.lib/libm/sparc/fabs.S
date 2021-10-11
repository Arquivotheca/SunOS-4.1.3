	.seg	"data"
	.asciz	"@(#)fabs.S 1.1 92/07/30 SMI"
	.align	8
	.seg	"text"

!	Copyright (c) 1988 by Sun Microsystems, Inc.

#define LOCORE
#include <machine/asm_linkage.h>

	.seg	"text"
	ENTRY(fabs)
	sethi	%hi(0x80000000),%o2
	andn	%o0,%o2,%o0
	std	%o0,[%sp+0x48]
	retl
	ldd	[%sp+0x48],%f0
