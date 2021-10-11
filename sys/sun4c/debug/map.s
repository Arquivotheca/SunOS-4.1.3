/*	@(#)map.s 1.1 92/07/30	*/

!
!	Copyright (c) 1986 by Sun Microsystems, Inc.
!

/*
 * Additional memory mapping routines for use by standalone debugger,
 * setpgmap(), getpgmap() are taken from the boot code.
 */

#include "assym.s"
#include <sys/param.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/enable.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <debug/debug.h>

	.seg	"text"
	.align	4

/*
 * get the machine type in the ID prom.
 *
 * u_char
 * getmachinetype()
 */
	ENTRY(getmachinetype)
	set	IDPROM_ADDR+1, %o0
	retl
	ldub	[%o0], %o0

/*
 * Flush a page from the cache.
 *
 * vac_pageflush(vaddr)
 * caddr_t vaddr;
 */
	ENTRY(vac_pageflush)
	srl	%o0, PGSHIFT, %o0	! mask off low bits
	sll	%o0, PGSHIFT, %o0
	set	MMU_PAGESIZE, %o1	! compute NLINES=PAGESIZE/LINESIZE
	set	_vac_linesize, %o2	!  ...number of lines to flush
	ld	[%o2], %o2
	sll	%o2, 2, %o5		! VAC_LINESIZE * 4
0:
	srl	%o2, 1, %o2		! we "know" LINESIZE is a power of 2
	tst	%o2			!  so just shift until LINESIZE
	bnz,a	0b			!  goes to zero
	srl	%o1, 1, %o1
	mov	16, %o2			! offsets for by 4 loop unroll
	mov	32, %o3
	mov	48, %o4
1:					! flush a 4 line chunk of the cache
	sta	%g0, [%o0]ASI_FCP	! offset 0
	sta	%g0, [%o0 + %o2]ASI_FCP	! offset 16
	sta	%g0, [%o0 + %o3]ASI_FCP	! offset 32
	sta	%g0, [%o0 + %o4]ASI_FCP	! offset 48
	subcc	%o1, 4, %o1		! decrement count
	bg	1b			! done yet?
	add	%o0, %o5, %o0		! generate next match address
2:
	retl
	nop

/*
 * Initialize the cache, write all tags to zero
 *
 * vac_init()
 */
	ENTRY(vac_init)
	set	CACHE_TAGS, %o0		! address of cache tags in CTL space
	set	_vac_size, %o1		! compute NLINES=SIZE/LINESIZE
	ld	[%o1], %o1		!  ...number of lines to initialize
	set	_vac_linesize, %o2
	ld	[%o2], %o2
	sll	%o2, 2, %o5		! VAC_LINESIZE * 4
0:
	srl	%o2, 1, %o2		! we "know" LINESIZE is a power of 2
	tst	%o2			!  so just shift until LINESIZE
	bnz,a	0b			!  goes to zero
	srl	%o1, 1, %o1
	mov	16, %o2			! offsets for by 4 loop unroll
	mov	32, %o3
	mov	48, %o4
1:
	sta	%g0, [%o0]ASI_CTL	! write tags to zero
	sta	%g0, [%o0 + %o2]ASI_CTL	! offset 16
	sta	%g0, [%o0 + %o3]ASI_CTL	! offset 32
	sta	%g0, [%o0 + %o4]ASI_CTL	! offset 48
	subcc	%o1, 4, %o1		! done yet?
	bg	1b
	add	%o0, %o5, %o0		! next cache tags address

	retl
	nop
