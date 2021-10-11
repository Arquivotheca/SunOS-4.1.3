	.data
/*	.asciz	"@(#)memset.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"
| Set block of storage to a particular byte value
| Usage: memset(addr, value, length)

ENTRY(memset)
	movl	PARAM2,d1	| value
	jeq	1$		| if zero, no need to extend
	movb	d1,d0		| copy byte value into all 4 bytes of longword
	lsll	#8,d1
	movb	d0,d1
	lsll	#8,d1
	movb	d0,d1
	lsll	#8,d1
	movb	d0,d1
1$:	movl	PARAM,a1	| address
	movl	PARAM3,d0	| length
	jle     6$		| return if not positive
	btst	#0,PARAMX(3)	| odd address?
	jeq	2$		| no, skip
	movb	d1,a1@+		| do one byte
	subql	#1,d0		| to adjust to even address
2$:	movl	d0,a0		| save possibly adjusted count
	lsrl	#2,d0		| get count of longs
	jra	4$		| go to loop test
| Here is the fast inner loop - loop mode on 68010
3$:	movl	d1,a1@+		| store long
4$:	dbra	d0,3$		| decr count; br until done
	clrw	d0
	subql	#1,d0
	bccs	3$
| Now up to 3 bytes remain to be set
	movl	a0,d0		| restore count
	btst	#1,d0		| need a short word?
	jeq	5$		| no, skip
	movw	d1,a1@+		| do a short
5$:	btst	#0,d0		| need another byte
	jeq	6$		| no, skip
	movb	d1,a1@+		| do a byte
6$:	movl	PARAM,d0	| return address
	RET			| all done
