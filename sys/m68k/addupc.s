	.data
 	.asciz	"@(#)addupc.s 1.1 92/07/30 Copyr 1984 Sun Micro"
	.even
	.text

|	Copyright (c) 1984 by Sun Microsystems, Inc.

| 
|  Add to user profiling counters.
|
| struct uprof {		/* profile arguments */
| 	short	*pr_base;	/* buffer base */
| 	unsigned pr_size;	/* buffer size */
| 	unsigned pr_off;	/* pc offset */
| 	unsigned pr_scale;	/* pc scaling */
| } ;
| addupc( pc, pr, incr) 
|     register int pc;
|     register struct uprof *pr;
|     int incr;
| {
| 	register short *slot;
| 	short counter;
| 
| 	slot = pr->pr_base 
| 		+ (((pc - pr->pr_off) * pr->pr_scale) >> 16)/(sizeof *slot);
| 	if (slot >= pr->pr_base &&
| 	    slot < (short *)(pr->pr_size + (int)pr->pr_base)) {
| 		if ((counter=fusword(slot))<0) {
| 			pr->pr_scale = 0;		/* turn off profiling */
| 		} else {
| 			counter += incr;
| 			susword(slot, counter);
| 		}
| 	}
| }

#include <machine/asm_linkage.h>

	PR_SIZE	=	4
	PR_OFF	=	8
	PR_SCALE=	12
	.text
	ENTRY(addupc)
	link	a6,#-16
	moveml	#0x3080,sp@
	movl	a6@(8),d7
	movl	a6@(12),a5
	| compute:
	|         pc - pr->pr_off in d7
	|         +--------+--------+
	|         |   a    |   b    |
	|         +--------+--------+
	|          31    16 15     0
	|
	|	  pr->pr_scale at a5@(PR_SCALE)
	|         +--------+--------+
	|         |   c    |   d    |
	|         +--------+--------+
	|          31    16 15     0
	|
	| (pc - pr->pr_off) * pr->pr_scale >>16 
	|   is the middle bits of a sum of 4 products:
	|
	|                            +--------+--------+
	|                            |      b * d      |
	|                            +--------+--------+
	|                   +--------+--------+
	|     +             |      a * d      |
	|                   +--------+--------+
	|                   +--------+--------+
	|     +             |      c * b      |
	|                   +--------+--------+
	|          +--------+--------+
	|     +    |      a * c      |
	|          +--------+--------+
	|______________________________________________________
	|
	|                   +--------+--------+
	|                   |  result in d0   |
	|                   +--------+--------+
	|
	|

	subl	a5@(PR_OFF),d7
	movw	d7,d0
	mulu	a5@(PR_SCALE+2),d0
	clrw	d0
	swap	d0
	movw	d7,d1
	beqs	1f  | short cut if product will be zero
	mulu	a5@(PR_SCALE),d1
	addl	d1,d0
1:
	swap	d7
	movw	a5@(PR_SCALE+2),d1
	beqs	2f  | short cut if product will be zero
	mulu	d7,d1
	addl	d1,d0
2:
	movw	a5@(PR_SCALE),d1
	beqs	3f  | short cut if product will be zero
	mulu	d7,d1
	swap	d1
	clrw	d1
	addl	d1,d0
3:
	andb	#0xfe,d0	| make sure address is even.
	addl	a5@,d0
	movl	d0,a4
	cmpl	a5@,a4
	jcs	LE12
	movl	a5@(PR_SIZE),d0
	addl	a5@,d0
	cmpl	d0,a4
	jcc	LE12
	pea	a4@
	jbsr	_fusword
	addql	#4,sp
	tstl	d0
	jge	L16
	clrl	a5@(PR_SCALE)
	jra	LE12
L16:
	addw	a6@(18),d0
	movl	d0,sp@-
	pea	a4@
	jbsr	_susword
	addql	#8,sp
LE12:
	moveml	sp@,#0x3080
	unlk	a6
	rts
