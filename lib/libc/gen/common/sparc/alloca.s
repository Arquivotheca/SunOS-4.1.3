/*
 *	.seg	"data"
 *	.asciz	"@(#)alloca.s 1.1 92/07/30 Copyr 1987 Sun Micro" 
 */
	.seg	"text"

#include <sun4/asm_linkage.h>

	!
	! o0: # bytes of space to allocate, already rounded to 0 mod 8
	! o1: %sp-relative offset of tmp area
	! o2: %sp-relative offset of end of tmp area
	!
	! we want to bump %sp by the requested size
	! then copy the tmp area to its new home
	! this is necessasy as we could theoretically
	! be in the middle of a compilicated expression.
	!
	ENTRY(__builtin_alloca)
	mov	%sp, %o3		! save current sp
	sub	%sp, %o0, %sp		! bump to new value
	! copy loop: should do nothing gracefully
	b	2f
	subcc	%o2, %o1, %o5		! number of bytes to move
1:	
	ld	[%o3 + %o1], %o4	! load from old temp area
	st	%o4, [%sp + %o1]	! store to new temp area
	add	%o1, 4, %o1
2:	bg	1b
	subcc	%o5, 4, %o5
	! now return new %sp + end-of-temp
	retl
	add	%sp, %o2, %o0
