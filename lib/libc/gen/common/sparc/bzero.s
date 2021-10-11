/*
 * @(#)bzero.s 1.1 92/07/30 Copyr 1987 Sun Micro
 */

#include <sun4/asm_linkage.h>

	ENTRY(bzero)
	cmp	%o1, 7			! if small counts, just zero bytes
	ble	zchar
	andcc	%o0, 3, %o3		! if bigger, align to 4 bytes	
	bz	zword

	mov	4, %o4			! align to a four byte boundary
	sub	%o4, %o3, %o3
	sub	%o1, %o3, %o1
1:	stb	%g0, [%o0]
	deccc	%o3	
	bnz	1b
	inc	%o0

zword:	andn	%o1, 3, %o2		! create word sized count in %o2
	mov	%o2, %o4		! size for this loop
1:	subcc	%o2, 4, %o2		! word clearing loop
	bnz	1b
	st	%g0, [%o0 + %o2]
	add	%o4, %o0, %o0		! add in the zeroed amount

	and	%o1, 3, %o1		! leftover count, if any
zchar:	tst	%o1
	bz	zout
	.empty				! following label ok in delay slot
1:	deccc	%o1			! byte clearing loop
	bnz	1b
	stb	%g0, [%o0 + %o1]	

zout:	retl
	mov	0, %o0
