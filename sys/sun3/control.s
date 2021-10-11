/*      @(#)control.s 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Control space operations for Sun-3 MMU and Virtual Address Cache
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include "assym.s"

	.text

| Get/Set control space data

| u_int
| sun_getcontrol_{byte,long}(space, addr)
|	u_int space;		/* Control space entry desired */
|	addr_t addr;		/* Address in that space to get */

#define	space	sp@(4)
#define	addr	sp@(8)

ENTRY(sun_getcontrol_byte)
	movl	addr,d0			| Obtain address
	andl	#CONTROL_MASK,d0	| Mask off bits
	orl	space,d0		| Merge control space target
	movl	d0,a0			| Use as address
	moveq	#0,d0			| Zero return register
	movsb	a0@,d0			|   to get return value
	rts

ENTRY(sun_getcontrol_long)
	movl	addr,d0			| Obtain address
	andl	#CONTROL_MASK,d0	| Mask off bits/Get long-word alignment
	orl	space,d0		| Merge control space target
	movl	d0,a0			| Use as address
	movsl	a0@,d0			|   to get return value
	rts

#undef	space
#undef	addr

| void
| sun_setcontrol_{byte,long}(space, addr, value)
|	u_int space;		/* Control space area to set */
|	addr_t addr;		/* Address within area to set */
|	u_int value;		/* Value to set */

#define	space	sp@(4)
#define	addr	sp@(8)
#define	value	sp@(12)
	
ENTRY(sun_setcontrol_byte)
	movl	addr,d0			| Get address to set
	andl	#CONTROL_MASK,d0	| Mask off bits
	orl	space,d0		| Merge control space
	movl	d0,a0			| Set up for use as address
	movl	value,d0		| Obtain value
	movsb	d0,a0@			| Set byte in control space
	rts

ENTRY(sun_setcontrol_long)
	movl	addr,d0			| Get address to set
	andl	#CONTROL_MASK,d0	| Mask off bits/Get long-word alignment
	orl	space,d0		| Merge control space
	movl	d0,a0			| Set up for use as address
	movl	value,d0		| Obtain value
	movsl	d0,a0@			| Set longword in control space
	rts

#undef	space
#undef	addr
#undef	value
	
#ifdef VAC

| Init the VAC by invaliding all cache tags.
| We loop through all 64KB to reset the valid bit of each line.
| It DOESN'T turn on cache enable bit in the enable register.
|
| void
| vac_init()

ENTRY(vac_init)
	movl	#VAC_RWTAG_BASE,a0	| Init base address to R/W VAC tags
	movl	#VAC_RWTAG_COUNT-1,d0	| Init loop counter to line count
	clrl	d1			| Value of zero will reset valid bit
0:	movsl	d1,a0@			| Invalidate this line's tag
	addl	#VAC_RWTAG_INCRMNT,a0	| Step address and
	dbra	d0,0b			|   continue over loop
	rts

| VAC (Virtual Address Cache) Flush by Context Match:
| We issue the context flush command VAC_CTXFLUSH_COUNT times.
| Each time we increment flush address by VAC_FLUSH_INCRMNT(2^9).
| Context no. is in the context register.
|
| void
| vac_ctxflush()

ENTRY(vac_ctxflush)
	tstl	_vac			| VAC?
	jeq	1f			|   No
	addql	#1,_flush_cnt+FM_CTX	| Increment statistics
	movl	#VAC_FLUSH_BASE,a0	| Get flush base initializer
	movl	#VAC_CTXFLUSH_COUNT-1,d0 |  and count of flushes to perform
	movb	#VAC_CTXFLUSH,d1	|   and command to do it
0:	movsb	d1,a0@			| Do a unit of context flush
	addl	#VAC_FLUSH_INCRMNT,a0	| Add loop increment
	dbra	d0,0b			| Continue over loop
1:	rts


| VAC (Virtual Address Cache) Flush by Segment Match.
| We issue the segment flush command VAC_SEGFLUSH_COUNT times.
| Each time we increment flush address by VAC_FLUSH_INCRMNT(2^9).
|
| void
| vac_segflush(v)
| 	addr_t v;		/* Segment number */

#define	v	sp@(4)

ENTRY(vac_segflush)
	tstl	_vac			| VAC?
	jeq	1f			|   No
	addql	#1,_flush_cnt+FM_SEGMENT | Take statistics
	movl	v,d0			| Get segment virtual address
	andl	#VAC_SEGMASK,d0		| Mask off segment bits
	orl	#VAC_FLUSH_BASE,d0	| Turn into control space address
	movl	d0,a0			| Use value as address
	movl	#VAC_SEGFLUSH_COUNT-1,d0 | Loop this many of times
	movb	#VAC_SEGFLUSH,d1	| Get segment flush "command"
0:
	movsb	d1,a0@			| Do a unit of segment flush
	addl	#VAC_FLUSH_INCRMNT,a0	| The address of next segment flush
	dbra	d0,0b			| Continue over loop
1:
	rts

#undef v

| VAC (Virtual Address Cache) Flush by Page Match
| We issue the page flush command VAC_PAGEFLUSH_COUNT times,
| Each time we increment flush address by VAC_FLUSH_INCRMNT(2^9).
|
| void
| vac_pageflush(vaddr)
| 	caddr_t v;		/* Address of page to be flushed */

#define	v	sp@(4)

ENTRY(vac_pageflush)
	tstl	_vac			| VAC?
	jeq	1f			|   No
	addql	#1,_flush_cnt+FM_PAGE	| Increment statistics
	movl	v,d0			| Get flush address
	andl	#VAC_PAGEMASK,d0	| Mask off page bits
	orl	#VAC_FLUSH_BASE,d0	| And set base address in control space
	movl	d0,a0			| Use as an address
	moveq	#VAC_PAGEFLUSH_COUNT-1,d0| Initialize loop count
	movb	#VAC_PAGEFLUSH,d1	| Initialize flush "command"
0:
	movsb	d1,a0@			| Do a unit of page flush
	addl	#VAC_FLUSH_INCRMNT,a0	| Step to next address
	dbra	d0,0b			|   and continue over loop
1:
	rts

#undef	v

| Flush-by-Page-Match nbytes of bytes starting from vaddr.
| We issue Page Match Flush commands until all nbytes
| are flushed.  In Sun-3, each time VAC_FLUSH_INCRMNT (512)
| bytes are flushed.
|
| void
| vac_flush(v, nbytes)
| 	addr_t v;		/* Address at which to start flush */
| 	int nbytes;		/* Number of bytes to flush */

#define	v	sp@(4)
#define	nbytes	sp@(8)

ENTRY(vac_flush)
	tstl	_vac			| VAC?
	jeq	1f			|   No
	addql	#1,_flush_cnt+FM_PARTIAL | Bump statistics
	movl	v,d1			| Obtain base address
	andl	#VAC_UNIT_MASK,d1	|   and find offset into flush unit
	movl	nbytes,d0		| Get requested byte count
	addl	d1,d0			| Actual count is unit + request
	addl	#VAC_FLUSH_INCRMNT-1,d0	| Initialize loop counter
	movl	#VAC_FLUSH_LOWBIT,d1	| Get (log2) multiplier
	lsrl	d1,d0			| Compute VAC units to flush
	movl	v,d1			| Obtain start address again
	andl	#VAC_UNIT_ADDRBITS,d1	| Mask to start at unit boundary
	orl	#VAC_FLUSH_BASE,d1	| Set control space address bits
	movl	d1,a0			| Prepare to use as address
	movb	#VAC_PAGEFLUSH,d1	| Initialize page flush "command"
	jra	3f			| enter into dbra loop
2:
	movsb	d1,a0@			| do a unit of Page Match Flush
	addl	#VAC_FLUSH_INCRMNT,a0	| address of next Page Match Flush
3:
	dbra	d0,2b			| decr d0, br if d0 >= 0
1:
	rts

#undef	v
#undef	nbytes

#endif VAC
