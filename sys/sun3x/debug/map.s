	.data
	.asciz "@(#)map.s 1.1 92/07/30 SMI"
	.even
	.text

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Additional memory mapping routines for use by standalone debugger.
 */

#include <sys/param.h>
#include <debug/debug.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>

/*
 * Return the physical address of the pte for a given virtual address.
 */
	ENTRY(getpteaddr)
	clrl	d0
	movl	sp@(4),a0
	ptestr	#3,a0@,#3,a1
	/*
	pmove	psr,sp@(4)
	*/
	.long	0xf02f6200
	.word	0x4
	movw	sp@(4),d1
	andw	#7,d1
	cmpw	#3,d1
	bne	0f
	movl	a1,d0
0:
	rts

/*
 * Flush the on chip data cache.
 */
	ENTRY(vac_flush)
	movc	cacr,d0			| get current cache control register
	orl	#DCACHE_CLEAR,d0	| or in clear flag
	movc	d0,cacr			| clear cache (regardless of state)
	rts

/*
 * Flush the entire MMU TLB (or ATC if you prefer)
 */
	ENTRY(atc_flush)
	pflusha
	rts
