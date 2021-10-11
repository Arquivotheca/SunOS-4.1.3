/*
 *	.seg    "data"
 *	.asciz  "@(#)enable.s 1.1 92/07/30"
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	system enable/disable register control
 */
	.seg    "text"
	.align  4

#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/enable.h>

ENTRY(sys_enable)
	set	ENABLEREG, %o2
	lduba 	[%o2]ASI_CTL, %o1
	or	%o1, %o0, %o0
	retl
	stba	%o0, [%o2]ASI_CTL

ENTRY(sys_disable)
	set	ENABLEREG, %o2
	lduba	[%o2]ASI_CTL, %o1
	andn	%o1, %o0, %o0
	retl
	stba	%o0, [%o2]ASI_CTL
