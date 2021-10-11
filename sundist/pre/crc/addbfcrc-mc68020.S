	.data
	.even
sccsid:	.asciz	"@(#)addbfcrc-mc68020.S 1.1 92/07/30"

| tcrc
| addbfcrc(buf, size, crc)
|         register char *buf;
|         register int size;
|         register tcrc crc;
| {
|         register int i;
| 
|         for (i = 0;  i < size;  ++i)
|                 crc = crctab[(int)(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
|         return(crc);
| }

#ifdef USE_ASM
	.text
	.proc
	.globl	_addbfcrc
_addbfcrc:
	link	a6,#0
	moveml	d6/d7,sp@-

	movl	a6@(16),d0		| d0 = crc
	tstl	a6@(12)			| size > 0?
	jle	out
	lea	_crctab,a0		| a0 = &crctab[]
	movl	a6@(8),d1
	movl	d1,a1			| a1 is pointer to buf
	addl	a6@(12),d1		| d1 = buf + size
	clrl	d7			| d7 is temp for crctab[d7]
again:
	movb	a1@+,d7			| buf[i]
	eorb	d0,d7			| crc ^ buf[i]
	movl	a0@(0,d7:w:4),d6	| crctab[crc ^ buf[i]]
	lsrl	#8,d0			| crc >> 8
	eorl	d6,d0			| crctab[crc ^ buf[i]] ^ (crc >> 8)
	cmpl	d1,a1
	jcs	again
out:
	moveml	sp@+,d6/d7
	unlk	a6
	rts
#endif
