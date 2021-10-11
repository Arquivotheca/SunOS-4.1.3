	.data
/*	.asciz	"@(#)bzero.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"
| Zero block of storage
| Usage: bzero(addr, length)

ENTRY(bzero)
	movl	PARAM,a1	| address
	movl	PARAM2,d0	| length
	jle     5$		| return if not positive
	clrl	d1		| use zero register to avoid clr fetches
	btst	#0,PARAMX(3)	| odd address?
	jeq	1$		| no, skip
	movb	d1,a1@+		| do one byte
	subql	#1,d0		| to adjust to even address
1$:	movl	d0,a0		| save possibly adjusted count
	lsrl	#2,d0		| get count of longs
	jra	3$		| go to loop test
| Here is the fast inner loop - loop mode on 68010
2$:	movl	d1,a1@+		| store long
3$:	dbra	d0,2$		| decr count; br until done
	clrw	d0
	subql	#1,d0
	bccs	2$
| Now up to 3 bytes remain to be cleared
	movl	a0,d0		| restore count
	btst	#1,d0		| need a short word?
	jeq	4$		| no, skip
	movw	d1,a1@+		| do a short
4$:	btst	#0,d0		| need another byte
	jeq	5$		| no, skip
	movb	d1,a1@+		| do a byte
5$:	RET			| all done
