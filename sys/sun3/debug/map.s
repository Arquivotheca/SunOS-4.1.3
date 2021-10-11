	.data
	.asciz	"@(#)map.s 1.1 92/07/30 SMI"
	.even
	.text

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Additional memory mapping routines for use by standalone debugger,
 * setpgmap(), getpgmap(), and setsegmap() are taken from the boot code.
 */

#include <sys/param.h>
#include <debug/debug.h>
#include <sun3/asm_linkage.h>
#include <sun3/mmu.h>
#include <sun3/pte.h>

/*
 * Getpgmap() and Setpgmap() use software pte's as opposed
 * to hardware pme's.  On sun3, they are basically the same
 * except that Setpgmap() will automatically turn on the
 * ``no cache'' bit for all mappings between _start and
 * _start + DEBUGSIZE.
 */
	ENTRY(Getpgmap)
	movl	sp@(4),d0		| get virtual address
	andl	#CONTROL_MASK,d0	| clear extraneous bits
	orl	#PAGE_MAP,d0		| set to page map base offset
	movl	d0,a0
	movc	sfc,d1			| save source function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,sfc			| set source function code
	movsl	a0@,d0			| read page map entry
	movc	d1,sfc			| restore source function code
	rts				| done

	ENTRY(Setpgmap)
	movl	sp@(4),d0		| get virtual address
	movl	sp@(8),d1		| get page map entry
	cmpl	#_start,d0
	bmi	0f			| skip ahead if before debugger
	cmpl	#_start + DEBUGSIZE,d0
	bhi	0f			| skip ahead if after debugger
	orl	#PG_NC,d1		| turn on the ``no cache'' bit
0:
	andl	#CONTROL_MASK,d0	| clear extraneous bits
	orl	#PAGE_MAP,d0		| set to page map base offset
	movl	d0,a0
	movc	dfc,d0			| save dest function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,dfc			| set dest function code
	movsl	d1,a0@			| write page map entry
	movc	d0,dfc			| restore dest function code
	rts				| done

	ENTRY(getsegmap)
	movl	sp@(4),d0		| get segment number
	andl	#CONTROL_MASK,d0	| clear extraneous bits
	orl	#SEGMENT_MAP,d0		| set to segment map offset
	movl	d0,a0
	movc	sfc,d1			| save source function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,sfc			| set source function code
	moveq	#0,d0			| clear upper part of register
	movsb	a0@,d0			| read segment map entry
	movc	d1,sfc			| restore source function code
	rts				| done

	ENTRY(setkcontext)
	moveq	#KCONTEXT,d0		| get context value to set
	movc	dfc,d1			| save dest function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,dfc			| set destination function code
	movsb	d0,CONTEXT_REG		| move value into context register
	movc	d1,dfc			| restore dest function code
	rts				| done

	ENTRY(getmachinetype)
	movc	sfc,d1			| save source function code
	lea	FC_MAP,a1		| get function code in a reg
	movc	a1,sfc			| set source function code
	lea	ID_PROM+1,a0		| address for machine type
	moveq	#0,d0			| clear upper part of register
	movsb	a0@,d0			| read idprom entry
	movc	d1,sfc			| restore source function code
	rts				| done

	ENTRY(cache_pageflush)
	movc	dfc,a1			| save dest function code
	moveq	#FC_MAP,d1		| get function code in a reg
	movc	d1,dfc			| set destination function code
	movl	sp@(4),d0		| get vaddr of this page
	andl	#VAC_PAGEMASK,d0	| clear extraneous bits
	orl	#VAC_FLUSH_BASE,d0	| set to page flush base address
	movl	d0,a0			| a0 gets page flush base address
	moveq	#VAC_PAGEFLUSH_COUNT-1,d0| loop this many of times
	movb	#VAC_PAGEFLUSH,d1	| page flush "command"
0:
	movsb	d1,a0@			| do a unit of page flush
	addl	#VAC_FLUSH_INCRMNT,a0	| address of next page flush
	dbra	d0,0b			| do next flush cycle until done
	movc	a1,dfc			| restore dest function code
	rts				| done

	ENTRY(cache_init)
	movc	dfc,a1			| save dest function code
	moveq	#FC_MAP,d1		| get function code in a reg
	movc	d1,dfc			| set destination function code
	movl	#VAC_RWTAG_BASE,a0	| set base address to R/W VAC tags
	movl	#VAC_RWTAG_COUNT-1,d0	| loop through all lines
	clrl	d1			| reset "valid" bit of tags
0:
	movsl	d1,a0@			| invalid the tag of this line
	addl	#VAC_RWTAG_INCRMNT,a0	| address to write to next line
	dbra	d0,0b			| invalid next tag until done
	movc	a1,dfc			| restore dest function code
	rts				| done
