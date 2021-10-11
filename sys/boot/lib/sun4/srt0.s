/*
 * srt0.s - standalone startup code
 *	.seg	"data"
 *	.asciz	"@(#)srt0.s 1.1 92/07/30"
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */
 	.seg	"text"
 	.align	4

#include <sun4/asm_linkage.h>
#include <sun4/param.h>

/* 
 * param.h defines VAC, DELAY, and CDELAY
 * if we are compiling for the bootblock undef VAC
 * to save on code size
 */

#ifdef BOOTBLOCK
#undef VAC
#endif

/*
 * This variable is used in the DELAY macro.  5 is the right value for
 * 68010's running 10MHz.  3 is the right value for 68020's running 16MHz
 * with cache on.  (4x as fast at doing the delay loop.)  Other values
 * should be empirically determined as needed.  Srt0.s sets this value
 * based on the actual runtime environment encountered.
 *
 * For a sparc machine with no cache (16Mhz) the delay should be 4
 * if the cache is on the delay can be 1
 *
 * It's critical that the value be no SMALLER than required, e.g. the
 * DELAY macro guarantees a MINIMUM delay, not a maximum.
 */
	.seg	"data"
	.global	_cpudelay
_cpudelay:
	.word	5
	.global	_vac
_vac:
	.word	1			! vac is there by default
idp:
	.word	0,0,0,0,0,0,0,0		! room for idprom
	.word	0,0,0,0,0,0,0,0
	.word	0,0,0,0,0,0,0,0
	.word	0,0,0,0,0,0,0,0

	.seg	"text"
	.align	8
	.global	_end
	.global	_edata
	.global	_main
	.global	__exit
	.global	__exitto
	.global	_start
_start:

#ifdef BOOTBLOCK
	.global _begin, _bootb, _bbsize, _bbchecksum
	b,a	_begin
_bootb:
! should be NBLKENTRIES	(64), see installboot.c
	.word	0,0,0,0,0,0,0,0		! room for boot_tab
	.word	0,0,0,0,0,0,0,0
	.word	0,0,0,0,0,0,0,0
	.word	0,0,0,0,0,0,0,0
	.word	0,0,0,0,0,0,0,0
	.word	0,0,0,0,0,0,0,0
	.word	0,0,0,0,0,0,0,0
	.word	0,0,0,0,0,0,0,0
_bbsize:	.word 0
_bbchecksum:	.word 0
_begin:
#endif BOOTBLOCK

	set	_start-MINFRAME, %sp	! start stack at _start
	save	%sp, -WINDOWSIZE, %sp	! leave room for window
acall:
	call	1f			! get the current pc into o7
					! (where _start is currently located)
	sethi	%hi(acall), %o3		! relocated address of call
1:
	sub	%o7,acall-_start,%o0	! address where _start is in memory
	or	%o3, %lo(acall), %o3	! relocated address of call
	cmp	%o3, %o7
	be	jmpstart
	.empty				! next instruction ok in delay slot
	set	_edata+4, %o2		! end of program, inclusive, except bss
	set	_start, %o1		! beginning of program
	sub	%o2, %o1, %o2		! size of program
2:
	ldd	[%o0], %o4		! relocate program
	inc	8, %o0			! bcopy
	std	%o4, [%o1]
	deccc	8, %o2
	bg	2b
	inc	8, %o1
	
	set	jmpstart, %g1		! now that it is relocated, jump to it
	jmp	%g1
	nop

jmpstart:
/* need to read idprom to figure out what we are */
	set	idp, %o1
	mov     1, %o0
	call	_idprom
	nop
	cmp	%o0, 1
	bne	1f			! default to a sunrise
	nop
	set	idp, %o1
	add	%o1, 1, %o1
	ldub	[%o1], %o1
	cmp	%o1, 0x22
	bne	1f
	mov	1, %o0
	set	_cpudelay, %o1
	st	%o0, [%o1]		! set vac to one
	set	_vac, %o1
	st	%g0, [%o1]		! set vac to zero
1:
#ifdef VAC
	set	_vac, %o0		! check vac flag
	ld	[%o0], %o0		
	tst	%o0
	bz	1f
	set	_start-0x4000, %o0	! stack grows down from start
	sethi	%hi(_end), %o1
	call	_cache_prog		! clear the NC bits for prog's pages
	or	%o1, %lo(_end), %o1

	call	_turnon_cache
	nop
1:
#endif VAC
	set	_edata, %o0		! beginning of bss
	set	_end+4, %o1		! end of bss
	call	_bzero			! zero the bss
	sub	%o1, %o0, %o1		! size of bss

! set the priority to a known value, don't assmue anything
mov  %psr, %o3
andn %o3, 0x0f00, %o1
mov  13, %o2
sll  %o2, 8, %o2
or   %o2, %o1, %o1
mov  %o1, %psr
nop

	call	_main
	nop

#ifndef VAC
	mov	0, %o0			! delay slot
__exit:
	ret				! ret to prom
	restore

__exitto:
	jmp	%o0			! ret to the specified address
	restore

#else
	call	__exit
	mov	0, %o0

__exit:
	save	%sp, -WINDOWSIZE, %sp
	set	_vac, %o0		! check vac flag
	ld	[%o0], %o0		
	tst	%o0
	bz	1f
	call	_turnoff_cache
	nop
	nop
	b	1f
	restore

__exitto:
	!
	! turn off the cache, flush the cache and then
	! disable it via system enable register
	!
	mov	%o0, %l3		! save address to exit to
	set	_vac, %o0		! check vac flag
	ld	[%o0], %o0		
	tst	%o0
	bz	2f
	nop
	call	_turnoff_cache
	nop
2:
	sub	%l3, 8, %i7		! will return to address in i7+8
1:
	ret
	restore
#endif
