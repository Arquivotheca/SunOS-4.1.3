/*
 *	.seg	"data"
 *	.asciz	"@(#)memset.s 1.1 92/07/30"
 *	Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include <sun4/asm_linkage.h>

	.seg	"text"
	.align	4

/*
 * char *memset(sp, c, n)
 * Set an array of n chars starting at sp to the character c.
 * Return sp.
 */
	ENTRY(memset)
	mov	%o0, %o5		! copy sp before using it
	cmp	%o2, 7			! if small counts, just write bytes
	bl	wrchar
	.empty				! following lable is ok in delay slot

walign:	btst	3, %o5			! if bigger, align to 4 bytes
	bz	wrword
	andn	%o2, 3, %o3		! create word sized count in %o3
	dec	%o2			! decrement count
	stb	%o1, [%o5]		! clear a byte
	b	walign
	inc	%o5			! next byte

wrword:	and	%o1, 0xff, %o1		! generate a word filled with c
	sll	%o1, 8, %o4
	or 	%o1, %o4, %o1
	sll	%o1, 16, %o4
	or	%o1, %o4, %o1
1:	st	%o1, [%o5]		! word writing loop
	subcc	%o3, 4, %o3
	bnz	1b
	inc	4, %o5

	and	%o2, 3, %o2		! leftover count, if any
wrchar:	deccc	%o2			! byte clearing loop
	inc	%o5
	bge,a	wrchar
	stb	%o1, [%o5 + -1]		! we've already incremented the address

	retl
	nop
