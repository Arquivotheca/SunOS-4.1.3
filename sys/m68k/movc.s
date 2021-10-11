	.data
	.asciz	"@(#)movc.s 1.1 92/07/30 SMI"
	.even
	.text

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <machine/asm_linkage.h>
#include <machine/param.h>
#include "assym.s"

#define	UNROLLB			/* unroll byte copy loop */
#define	UNROLLL			/* unroll long copy loop */

#ifdef BCOPY
#include <machine/mmu.h>
#include <machine/pte.h>
	.data
bcopys:	.byte	0			| semaphore for bcopy
ctx_save: .byte 0
	.even
	.long 	0, 0, 0, 0
blockz:
	.long 	0, 0, 0, 0
	.text
#endif BCOPY

/*
 * Stack offsets for {k/b}copy.
 */
#define	count	a6@(0x10)
#define	to	a6@(0xc)
#define	from	a6@(0x8)
#define	slf	a6@(-0x4)		/* Saved u_lofault value */
#define	lfv	a6@(-0x8)		/* Lofault vectors */
#define	svd	a6@(-0xc)		/* Saved D2 */

kclfv:	.long 	copyerr
#ifdef BCOPY
	.long	copysem
	.long	copyfault
#endif BCOPY
bclfv:	.long	0
#ifdef BCOPY
	.long	0
	.long	copyfault
#endif BCOPY

/***********************************************************************
 *  The "dbra" Comment:		(16mar88 limes)
 *
 *  The mc68k "dbra" instruction only decrements the last 16 bits of
 *  the counter; this has bitten assembly programmers around here
 *  several times. The problem is that, when the last sixteen bits are
 *  all zero, the dbra sets them to FFFF but does not borrow from the
 *  top. The simple and clear way to recover is to clear the bottom
 *  half and do the decrement again; if the counter is negative after
 *  this, then we are done. Remember, the count is signed ...
 *
 *  Thus, a loop to move <d0> bytes looks like:
 *
 *		jra	2f		| enter loop
 *      1:      movb    a0@+,a1@+	| 68010 loop mode
 *      2:      dbra    d0,1b		| up to 65536 loops
 *              clrw    d0		| undo last decrement
 *              subql   #1,d0		| 32 bit decrement
 *              jpl     1b		| branch if not all done yet
 */

/*
 * Simple tight loop for copying longs.
 * Guarantees long word bus cycles for long word addresses.
 * Useful when you can't safely use bcopy due to destination
 * size/alignment constraints. (eg. FPA)
 * 
 * Usage: lcopy (from, to, count)
 * caddr_t from, to; u_int count;
 * count is the number of longs to transfer.
 */
	ENTRY(lcopy)
	link	a6,#0
	movl	count,d0
	movl	from,a0
	movl	to,a1
	jra	2f
1:	movl	a0@+,a1@+
2:	dbra	d0,1b
	clrw	d0			| see "dbra comment" above
	subql	#1,d0			|
	jpl	1b			|
	unlk	a6
	rts



/*
 * Copy a block of storage, returning an error code if `from' or
 * `to' takes a kernel pagefault which cannot be resolved.
 *
 * Usage: int kcopy(from, to, count);
 * caddr_t from, to; u_int count;
 * returns Exxx on pagefault error, 0 if all ok
 */
	ENTRY(kcopy)
	link	a6,#-0x20		| Build stack frame
	movl	#kclfv,lfv		| Save pointer to lofault vector
	jra	bcopy_getaddrs		| Join common code

/*
 * Copy a block of storage - must not overlap (from + len <= to)
 * Usage: bcopy(from, to, count)
 * caddr_t from, to; u_int count;
 */
	ENTRY(bcopy)
	link	a6,#-0x20		| Build stack frame
	movl	#bclfv,lfv		| Save pointer to lofault vector
bcopy_getaddrs:
	movl	d2,svd			| %%% save d2
	movl	_uunix, a1
	movl	a1@(U_LOFAULT),slf	| Save u_lofault
	movl	count,d0		| Get count
	jle	ret			| Return if not positive
	movl	lfv,a0			| Initialize u_lofault from
	movl	a0@,a1@(U_LOFAULT)	|   appropriate lofault vector
	movl	from,a0			| Get other parameters
	movl	to,a1
#if !defined(mc68020) || defined(SUN3_E)
/*
 * If from address odd, move one byte to try to make things even
 */
	movw	a0,d1			| from
	btst	#0,d1			| test for odd bit in from
	jeq	0f			| even, skip
	movb	a0@+,a1@+		| move a byte
	subql	#1,d0			| adjust count
/* If to address is odd now, we have to do byte moves */
0:	movl	a1,d1			| low bit one if mutual oddness
	btst	#0,d1			| test low bit
	jne	bytes			| do it slow and easy
#else	mc68020 && !SUN3_E
/*
 * The mc68020 knows how to handle misaligned transfer. We still align
 * the source to a longword for efficiency, but writing a misaligned
 * long is faster than doing it byte-by-byte.
 *
 * However, as noted in bugid 1016954, doing this on a 3/E causes
 * incoming "rlogin" sessions to wedge when full packets start
 * getting sent over the net. Until we have time to track this
 * problem down, require 68010-style alignment if we are supporting 
 * the Sun-3/E.
 */
0:	movw	a0,d1			| source addr
	andl	#3,d1			| check long alignment
	jeq	1f			| align OK, continue
	movb	a0@+,a1@+		| move a byte
	subql	#1,d0			| update count
	jeq	ret			| copy may be complete
	jmp	0b			| check align again
1:
#endif	mc68020 && !SUN3_E
/*
 * All aligned - now move longs
 */
	movl	d0,d1			| save count
	lsrl	#2,d0			| get # longs
#ifdef BCOPY
/*
 * WARNING - THIS CODE IS NOT REENTRANT
 * Fast copy routine uses the Block Copy commands of a BCOPY machine.
 * The Block Copy Read and Write commands copy BC_LINESIZE (16) bytes
 * at a time and avoid the displacement of any valid data from the cache.
 * We use Block Copy Read and Write until there are less than
 * BC_LINESIZE bytes to move.  Here, a0 = from, a1 = to, (a0 and a1 are
 * in word boundary. d0 = count in long words, and d1 = count in bytes.
 * Isn't worth doing Block Copy Read/Write if 2 * BC_LINESIZE >= byte count
 * If we're running on the interrupt stack we can't use the bcopy hardware
 * because we might've interrupted trap() while processing a bus error but
 * before it had read the bus error register, and we might take a bus error
 * fault while doing the bcopy, thus destroying the bus error register before
 * the trap() routine that we interrupted had a chance to read it.
 */
	tstl	_bcenable		| test if there is BCOPY
	jeq	outnr			| bit is off, skip fast copy
	cmpl	#BC_MINBCOPY,d1
	jlt	outnr			| 2*BC_LINESIZE >= byte count
	cmpl	#eintstack,sp		| on interrupt stack?
	jls	outnr			| yes, skip

	moveml	d2-d4/a2/a4,sp@		| save d2, d3, d4, and a2, a4
	movl	_uunix, a4		| a4 has current u
	tas	bcopys			| only one person at a time
	jne	out			| busy, skip fast copy
	movl	lfv,a2			| Update lofault
	movl	a2@(4),a4@(U_LOFAULT)	|   from common vector

#ifdef sun3x
	movl	#ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,d2
	movc	d2,cacr			| inline vac_flush
#endif

/* Use normal bcopy if a0 and a1 are not aligned according to line size */
	movl	a0,d2
	andl	#0xf,d2			| get byte residual of line size of a0
	movl	a1,d3
	andl	#0xf,d3			| get byte residual of line size of a1
	cmpl	d2,d3
	jne	done			| not line size aligned
	cmpl	#0,d2			| skip word move if in 16 byte boundary
	jeq	3f
/*
 * a0 (from) and a1 (to) are aligned to each other in terms of line size (16).
 * Move a word (2 bytes) at a time until byte residual of "from" is zero.
 * (a0 and a1 are in word boundary when "fast" bcopy is entered.)
 */
	movl	#BC_LINESIZE,d2		| figure out number of words to move
	subl	d3,d2			| d2 = byte to move to align to 16
	subl	d2,d1			| update byte count
	movl	d1,d0			| updated long word count
	lsrl	#2,d0
	lsrl	#1,d2			| # of words to move in the residual
	jra	2f			| enter word move loop
1:	movw	a0@+,a1@+		| move a word in the residual part
2:	dbra	d2,1b			| decr, br if >= 0
3:
	movl	lfv,a2			| install retry fault handler
	movl	a2@(8),a4@(U_LOFAULT)
	movl	d0,d2			| save long word count
	movl	a0,d3			| save to & from
	movl	a1,a2

	lsrl	#BC_LINE_SHIFT,d0	| # of lines to move
#ifndef sun3x
/* Set up Block Copy R/W commands to a0 (from) and a1 (to). */
	movl	a0,d4			| to set up Block Copy R/W command
	andl	#BC_BLOCK_OFF,d4	| mask off cmd bits
	orl	#BC_BLOCK_CPCMD,d4	| set up "from"
	movl	d4,a0
	movl	a1,d4			| to set up Block Copy R/W command
	andl	#BC_BLOCK_OFF,d4	| mask off cmd bits
	orl	#BC_BLOCK_CPCMD,d4 	| set up "to"
	movl	d4,a1
#endif sun3x
/* Block Copy R/W loop, one line at a time. */
	jra	5f			| enter block copy loop
4:	movsl	a0@,d4			| Block Copy Read, d4 is dummy
	movsl	d4,a1@			| Block Copy Write, d4 is dummy
	addl	#BC_LINESIZE,a0		| update "from"
	addl	#BC_LINESIZE,a1		| update "to"
5:	dbra	d0,4b			| decr, br if >= 0
	clrw	d0			| clear low 
	subql	#1,d0			| borrow from high half if != 0
	jpl	4b			| loop if high half was not 0
#ifndef sun3x
/* Mask off Block Copy R/W commands in a0 (from) and a1 (to). */
	movl	a0,d4			| to mask off Block Copy R/W command
	andl	#BC_BLOCK_OFF,d4	| mask off "from"
	movl	d4,a0
	movl	a1,d4			| to mask off Block Copy R/W command
	andl	#BC_BLOCK_OFF,d4	| mask off "to"
	movl	d4,a1
#endif sun3x
/*
 * Update long count in d0, since only the last two bits of d1 (byte count)
 * are to be used and the last two bits of d1 are not affected by the Block
 * Copy R/W, we don't have to update d1.
 */
	movl	d2,d0
	andl	#BC_LINE_RESIDU,d0	| update long word count
	jra	done

copyfault:				| got here by faulting
	movl	d2,d0			| restore to,from & count
	movl	d3,a0
	movl	a2,a1
done:
	clrb	bcopys			| reset semaphore
out:
	movl	lfv,a2			| Update lofault
	movl	a2@,a4@(U_LOFAULT)	|   one more time
	moveml	sp@,a2/a4/d2-d4		| restore a2,a4 and d2, d3 & d4
outnr:
#endif BCOPY
/* MOVE A BUNCH OF LONGS: d0 is number of longs, d1 is bytes;
 * a0 is source, a1 is destination. only the last two bits of d1
 * are useful.
 */
longs:
#ifdef	UNROLLL
	movl	d0,d2
	lsrl	#4,d0			| loop once per 16 longs
	andl	#15,d2			| do the leftovers first
	lsll	#1,d2			| "movl a0@+,a1@+" is two bytes long
	negl	d2			| make it negative offset
	addl	#34,d2			| relative to end of loop
	jmp	pc@(0,d2)		| dive into unrolled loop

6:	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+
	movl	a0@+,a1@+

#else	UNROLLL
	jra	7f			| enter long move loop
6:	movl	a0@+,a1@+		| move a long
#endif	UNROLLL
7:	dbra	d0,6b			| decr, br if >= 0
	clrw	d0			| clear low 
	subql	#1,d0			| borrow from hi half, get F's in lower
	jpl	6b			| loop if top non-negative, still more
/* Now up to 3 bytes remain to be moved */
	movl	d1,d0			| restore count
	andl	#3,d0			| mod sizeof long

/* We have some bytes left over that could not be
 * moved as longs; this could be due to alignment
 * restrictions, or it could be up to three bytes
 * of data remaining after a longword transfer.
 */
bytes:
#ifdef	UNROLLB
	movl	d0,d1
	lsrl	#4,d0			| loop once per 16 bytes
	andl	#15,d1			| short bunch in first loop
	lsll	#1,d1			| "movb a0@+,a1@+" is two bytes long
	negl	d1			| make it negative offset
	addl	#0x22,d1		| from end of loop
	jmp	pc@(0,d1)		| dive into unrolled loop

0:	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+
	movb	a0@+,a1@+	

#else	UNROLLB
	jra	1f
0:	movb	a0@+,a1@+		| loop mode byte moves
#endif	UNROLLB
1:	dbra	d0,0b
	clrw	d0
	subql	#1,d0			| see "dbra" comment above
	jpl	0b

ret:	clrl	d0			| ret code (not used for bcopy)
#ifdef BCOPY
	jra	copyerr			| Join common code

/*
 * We got here because of a fault during the middle of the block
 * copy using the block copy hardware.  Clear the semaphore and
 * restore preserved registers we've appropriated.
 */
copysem:
	clrb	bcopys
	moveml	sp@,a2/a4/d2-d4
#endif BCOPY

/*
 * If we got here because of fault failure (kcopy only) then
 * an errno value should have already been loaded into d0 by
 * trap code that returns here.
 */
copyerr:
	movl	_uunix, a1
	movl	slf,a1@(U_LOFAULT)	| restore old lofault value
	movl	svd,d2			| restore D2 register
	unlk	a6
	rts				| all done

#undef	count
#undef	to
#undef	from
#undef	slf
#undef	lfv

/* Block copy with possibly overlapped operands */
	ENTRY(ovbcopy)
	movl	sp@(4),a0		| from
	movl	sp@(8),a1		| to
	movl	sp@(12),d0		| get count
	jle	3f			| return if not positive
	cmpl	a0,a1			| check direction of copy
	jgt	bwd			| do it backwards
/* Here if from > to - copy bytes forward */
	jra	2f
/* Loop mode byte copy */
1:	movb	a0@+,a1@+
2:	dbra	d0,1b
	clrw	d0
	subql	#1,d0			| see "dbra" comment above
	jpl	1b
3:
	rts

/* Here if from < to - copy bytes backwards */
bwd:	addl	d0,a0			| get end of from area
	addl	d0,a1			| get end of to area
	jra	2f			| enter loop
/* Loop mode byte copy */
1:	movb	a0@-,a1@-
2:	dbra	d0,1b
	clrw	d0
	subql	#1,d0			| see "dbra" comment above
	jpl	1b
	rts				| all done

/*
 * Stack offsets for {k/b}zero.
 */
#define	count	a6@(0xc)
#define	base	a6@(0x8)
#define	oab	a6@(0xb)
#define	slf	a6@(-0x4)
#define	lfv	a6@(-0x8)

kzlfv:	.long	zeroerr
#ifdef BCOPY
	.long	zerosem
	.long	zerofault
#endif BCOPY
bzlfv:	.long	0
#ifdef BCOPY
	.long	0
	.long	zerofault
#endif BCOPY

/*
 * Zero a block of storage, returning an error code if we
 * take a kernel pagefault which cannot be resolved.
 *
 * Usage: int kzero(base, count);
 * caddr_t base; u_int count;
 * returns Exxx on pagefault error, 0 if all ok
 */
	ENTRY(kzero)
	link	a6,#-0x1c		| Build stack frame
	movl	#kzlfv,lfv		| Initialize lofault vector
	jra	zerostart		| Join common code

/*
 * Zero block of storage
 * Usage: bzero(base, count);
 * caddr_t base; u_int count;
 */
	ENTRY2(bzero,blkclr)
	link	a6,#-0x1c		| Build stack frame
	movl	#bzlfv,lfv		| Initialize lofault vector
zerostart:
	movl	d2,svd			| save working register
	movl	_uunix, a1
	movl	a1@(U_LOFAULT),slf	| Save lofault
	movl	lfv,a0			| Get vector
	movl	a0@,a1@(U_LOFAULT)	|   and set initial value
	movl	base,a1			| address
	movl	count,d0		| length
	clrl	d1			| use zero register to avoid clr fetches
	btst	#0,oab			| odd address?
	jeq	1f			| no, skip
	movb	d1,a1@+			| do one byte
	subql	#1,d0			| to adjust to even address
1:	movl	d0,a0			| save possibly adjusted count
	lsrl	#2,d0			| get count of longs
#ifdef BCOPY
/*
 * WARNING - THIS CODE IS NOT REENTRANT
 * Fast zero routine uses the Block Copy commands of a BCOPY machine.
 * The Block Copy Read and Write commands copy BC_LINESIZE bytes
 * at a time and avoid the displacement of any valid data from the cache.
 * We use Block Copy Read and Write until there are less than
 * BC_LINESIZE bytes to zero.
 * d0 = count in long words, d1 = 0, a0 = saved count in bytes, a1 = to.
 * Doesn't worth doing Block Copy Read/Write if 2 * BC_LINESIZE >= byte count
 */ 
	tstl	_bcenable		| test if there is BCOPY
	jeq	zoutnr			| bit is off, skip fast zero
	cmpl	#BC_MINBCOPY,a0
	jlt	zoutnr			| 2*BC_LINESIZE >= byte count

	moveml	d2/d3/a2/a4,sp@		| save d2-d3 and a2, a4
	movl	_uunix, a4
	tas	bcopys			| only one person at a time
	jne	zout			| busy, skip fast copy
	movl	lfv,a2			| Update lofault to
	movl	a2@(4),a4@(U_LOFAULT)	|   clear semaphore

#ifdef sun3x
	movl	#ICACHE_ENABLE+DCACHE_CLEAR+DCACHE_ENABLE,d2
	movc	d2,cacr			| inline vac_flush
#endif

/* Align a1 to line size if not already there; a1 is in word boundary */
	movl	a1,d2
	andl	#0xf,d2			| get byte residual of line size of a1
	jeq	2f			| aligned to line size (16) already
	movl	#BC_LINESIZE,d3		| figure out number of words to move
	subl	d2,d3			| d3 = bytes to clear to align a1 to 16
	subl	d3,a0			| update byte count
	movl	a0,d0			| update long count
	lsrl	#2,d0
	lsrl	#1,d3			| # of words to clear to align a1 to 16
	jra	1f			| enter word clear loop
0:	movw	d1,a1@+			| clear a word
1:	dbra	d3,0b			| decr, br if >= 0
2:

	movl	lfv,a2			| install retry fault handler
	movl	a2@(8),a4@(U_LOFAULT)
	movl	d0,d2			| save long word count
	movl	a1,d1			| save to

	lsrl	#BC_LINE_SHIFT,d0	| # of lines to move
/*
 * Set up Block Copy R/W commands. blockz points to the middle of 32 bytes
 * of zeros. "from" (a2) is in 16 byte boundary and points to 16 bytes of
 * zero's.
 */
	lea	blockz,a2		| get address of zero block
	movl	a2,d3			| set up "from"
#ifdef sun3x
	andl	#0xFFFFFFF0,d3		| mask off lower 16 bytes
#else
	andl	#0x0FFFFFF0,d3		| mask off lower 16 bytes and <31,28>
	orl	#BC_BLOCK_CPCMD,d3	
#endif sun3x
	movl	d3,a2
	movsl	a2@,d3			| Block Copy Read, d3 is dummy
#ifndef sun3x
	movl	a1,d3			| to set up Block Copy R/W command
	andl	#BC_BLOCK_OFF,d3	| mask off cmd bits
	orl	#BC_BLOCK_CPCMD,d3 	| set up "to"
	movl	d3,a1
#endif sun3x
/* Block Copy R/W loop to zero BC_LINESIZE bytes at a time. */
	jra	7f			| enter block copy loop
6:	movsl	d3,a1@			| Block Copy Write, d3 is dummy
	addl	#BC_LINESIZE,a1		| update "to"
7:	dbra	d0,6b			| decr, br if >= 0
	clrw	d0			| clear low 
	subql	#1,d0			| borrow from high half if != 0
	jpl	6b			| loop if high half was not 0
#ifndef sun3x
/* Mask off Block Copy Write command in a1 (to). */
	movl	a1,d3			| to mask off Block Copy R/W command
	andl	#BC_BLOCK_OFF,d3	| mask off "to"
	movl	d3,a1
#endif sun3x
/*
 * Update long count in d0, since only the last two bits of a0 (byte count)
 * are to be used and the last two bits of a0 are not affected by the Block
 * Copy R/W, we don't have to update a0.
 */
	movl	d2,d0			| update long word count
	andl	#BC_LINE_RESIDU,d0
	jra	zdone
zerofault:				| got here cause of a fault
	movl	d2,d0			| restore to & count
	movl	d1,a1
zdone:
	clrb	bcopys			| reset semaphore
zout:
	clrl	d1			| reclear, used to save to
	movl	lfv,a2			| Reset lofault to
	movl	a2@,a4@(U_LOFAULT)	|   simple error return
	moveml	sp@,d2/d3/a2/a4		| restore d2, d3, and a2, a4
zoutnr:
#endif BCOPY

#ifdef	UNROLLL
	movl	d0,d2
	lsrl	#4,d0		| loop once per 16 longs
	andl	#15,d2		| short bunch in first loop
	lsll	#1,d2		| "movl a0@+,a2@+" is two bytes long
	negl	d2		| make it negative offset
	addl	#0x22,d2	| from end of loop
	jmp	pc@(0,d2)	| dive into unrolled loop

8:	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
	movl	d1,a1@+
#else	UNROLLL
	jra	9f			| go to loop test
/* Here is the fast inner loop - loop mode on 68010 */
8:	movl	d1,a1@+			| store long
#endif	UNROLLL
9:	dbra	d0,8b			| decr count; br until done
	clrw	d0			| clear low 
	subql	#1,d0			| borrow from hi half, get F's in lower
	jpl	8b			| loop if top non-negative, still more
/* Now up to 3 bytes remain to be cleared */
	movl	a0,d0			| restore count
	btst	#1,d0			| need a short word?
	jeq	0f			| no, skip
	movw	d1,a1@+			| do a short
0:	btst	#0,d0			| need another byte
	jeq	1f			| no, skip
	movb	d1,a1@+			| do a byte
1:
	clrl	d0			| ret code (used for kzero only)
#ifdef BCOPY
	jra	zeroerr			| Join common code

/*
 * We got here because of a fault failed.  Clear semaphore and 
 * restore preserved registers.
 */
zerosem:
	clrb	bcopys
	moveml	sp@,d2/d3/a2/a4
#endif BCOPY

/*
 * If we got here because of fault failure (kzero), the errno value
 * should have already been loaded into d0 by trap code that returns here.
 */
zeroerr:
	movl	_uunix, a1
	movl	slf,a1@(U_LOFAULT)	| restore old U_LOFAULT value
	movl	svd,d2			| restore working register
	unlk	a6
	rts

#undef	count
#undef	base
#undef	oac
#undef	slf
#undef	lfv
