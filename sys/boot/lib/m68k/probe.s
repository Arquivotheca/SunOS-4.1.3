|	@(#)probe.s 1.1 92/07/30 Copyr 1983 Sun Micro

|       Copyright (c) 1983 by Sun Microsystems, Inc.

| 
| probe.s
| 
| Determine if an interface is present on the bus. 
|

|
| peek(addr)
|
| Temporarily re-routes Bus Errors, and then tries to
| read a short from the specified address.  If a Bus Error occurs,
| we return -1; otherwise, we return the unsigned short that we read.
|

	.text
	.globl	_peek
_peek:
	movl	a7@(4),a0	| Get address to probe
	movc	vbr,a1		| get vbr
	movl	a1@(8),d1	| save bus error handler
	movl	#BEhand,a1@(8)	| set up our own handler
	movl	sp,a1		| save current stack pointer
	movl	a0,d0
	btst	#0,d0		| See if odd address
	bne	BEhand		| Yes, the probe fails.
	moveq	#0,d0		| Clear top half
	movw	a0@,d0		| Read a shortword.
PAexit:
	movc	vbr,a1		| get vbr
	movl	d1,a1@(8)	| restore bus error handler
	rts
BEhand:
	movl	a1,sp		| Restore stack after bus error
	moveq	#-1,d0		| Set result of -1, indicating fault.
	bra	PAexit

|
| peekc(addr)
|
| Temporarily re-routes Bus Errors, and then tries to
| read a char from the specified address.  If a Bus Error occurs,
| we return -1; otherwise, we return the unsigned char that we read.
|

	.text
	.globl	_peekc
_peekc:
	movl	a7@(4),a0	| Get address to probe
	movc	vbr,a1		| get vbr
	movl	a1@(8),d1	| save bus error handler
	movl	#BEhand,a1@(8)	| set up our own handler
	movl	sp,a1		| save current stack pointer
	movl	a0,d0
	moveq	#0,d0		| Clear top half
	movb	a0@,d0		| Read a byte.
	bra	PAexit

|
| pokec(a,c)
|  
| This routine is the same, but uses a store instead of a read, due to
| stupid I/O devices which do not respond to reads.
|
| if (pokec (charpointer, bytevalue)) itfailed;
|

	.globl	_pokec
_pokec:
	movl	a7@(4),a0	| Get address to probe
	movc	vbr,a1		| get vbr
	movl	a1@(8),d1	| save bus error handler
	movl	#BEhand,a1@(8)	| set up our own handler
	movl	sp,a1		| save current stack pointer
	movb	a7@(11),a0@	| Write a byte
| A fault in the movb will vector us to BEhand above.
	moveq	#0,d0		| It worked; return 0 as result.
	bra	PAexit		| restores bus error handler and returns - above

|
| poke(a,c)
|  
| if (poke(pointer, value)) itfailed;
|

	.globl	_poke
_poke:
	movl	a7@(4),a0	| Get address to probe
	movc	vbr,a1		| get vbr
	movl	a1@(8),d1	| save bus error handler
	movl	#BEhand,a1@(8)	| set up our own handler
	movl	sp,a1		| save current stack pointer
	movw	a7@(10),a0@	| Write a word
| A fault in the movb will vector us to BEhand above.
	moveq	#0,d0		| It worked; return 0 as result.
	bra	PAexit		| restores bus error handler and returns - above
