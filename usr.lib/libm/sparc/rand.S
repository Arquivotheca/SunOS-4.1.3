
#define LOCORE
#include <machine/asm_linkage.h>

! Copyright (c) 1986 by Sun Microsystems, Inc.
!
! long rand(),srand()
! Fast integer random number generators, to replace UNIX's rand()
! Code by K.C. Ng, Nov 3, 1986
!
! rand() is based on the additive number generator described in Knuth's 
! The art of computing  Vol II (second edition) section 3.2.2. It returns a 
! positive random integer. 

! srand() uses ax+c mod 2**32 to generate seeds for rand(). Here 
! a=8*(10**8-29)+5, c=10**9-63.
!

	.seg	"data"
	.asciz	"@(#)rand.S 1.1 92/07/30 Copyr 1986 Sun Micro"

	.seg	"text"
#if 0
	ENTRY(rand)
	set	base-4,%o5	! o5 = address of base Y[0]
	ld	[%o5-4],%o1	! o1 = j
	ld	[%o5],%o2	! o2 = k
	add	%o5,%o1,%o3	! o3 = address of Y[j]
	add	%o5,%o2,%o4	! o4 = address of Y[k]
	ld	[%o3],%o0	! o0 = Y[j]
	ld	[%o4],%o3	! o3 = Y[k]
	add	%o0,%o3,%o0	! o0 = Y[j]+Y[k]
	st	%o0,[%o4]	! replace Y[k] by Y[j]+Y[k]
	subcc	%o1,4,%o1
	bnz	1f
	nop
	or	%g0,220,%o1
1:	st	%o1,[%o5-4]	! j <- j-1 mod 56
	subcc	%o2,4,%o2
	bnz	1f
	sethi	%hi(0x80000000),%o1
	or	%g0,220,%o2
1:	st	%o2,[%o5]	! k <- k-1 mod 56
	retl
	andn	%o0,%o1,%o0	! make it positive

	
	.seg	"data"
	.align	4
j:	.word	96	! j
k:	.word	220	! k
base:
	.word 0x8ca0df45, 0x37334f23, 0x4a5901d2, 0xaeede075, 0xd84bd3cf
	.word 0xa1ce3350, 0x35074a8f, 0xfd4e6da0, 0xe2c22e6f, 0x045de97e
	.word 0x0e6d45b9, 0x201624a2, 0x01e10dca, 0x2810aef2, 0xea0be721
	.word 0x3a3781e4, 0xa3602009, 0xd2ffcf69, 0xff7102e9, 0x36fab972
	.word 0x5c3650ff, 0x8cd44c9c, 0x25a4a676, 0xbd6385ce, 0xcd55c306
	.word 0xec8a31f5, 0xa87b24ce, 0x1e025786, 0x53d713c9, 0xb29d308f
	.word 0x0dc6cf3f, 0xf11139c9, 0x3afb3780, 0x0ed6b24c, 0xef04c8fe
	.word 0xab53d825, 0x3ca69893, 0x35460fb1, 0x058ead73, 0x0b567c59
	.word 0xfdddca3f, 0x6317e77d, 0xaa5febe5, 0x655f73e2, 0xd42455bb
	.word 0xe845a8bb, 0x351e4a67, 0xa36a9dfb, 0x3e0ac91d, 0xbaa0de01
	.word 0xec60dc66, 0xdb29309e, 0xcfa52971, 0x1f3eddaf, 0xe14aae61

	.seg	"text"
	ENTRY(srand)
	save	%sp,-SA(MINFRAME),%sp	! Stack Align #MINFRAME
	set	a,%l0			! l0 = address of a
	set	base,%l2		! l2 = address of base in _rand
	set	zbase,%l4		! l4 = address of zbase
	ld	[%l0+4],%l1		! l1 = c
	ld	[%l0],%l0		! l0 = a
	or	%g0,96,%l3		! l3 = 4*24
	st	%l3,[%l2-8]		! store 4*24 in j
	or	%g0,220,%l3		! l3 = 4*55
	st	%l3,[%l2-4]		! store 4*55 in k
	or	%g0,55,%l3		! set l3 = 55
	mov	%i0,%o0
1:
	subcc	%l3,1,%l3		! counter
	bge	2f
	tst	%o0
	ret
	restore
2:
	bne	3f
	mov	%l0,%o1
	ld	[%l4],%o1
	st	%o1,[%l2]
	add	%l2,4,%l2
	ba	1b
	add	%l4,4,%l4
3:
	call	.mul			! a*x
	nop
	add	%o0,%l1,%o0		! ax+c
	srl	%o0,1,%o0		! (ax+c)>>1
	st	%o0,[%l2]
	ba	1b
	add	%l2,4,%l2

	.seg	"data"
	.align	4
a:	.word 0x2faf071d	! a  = 8*(10**8-29)+5
c:	.word 0x3b9ac9c1	! c  = 10**9-63		
zbase:
	.word 0x8ca0df45, 0x37334f23, 0x4a5901d2, 0xaeede075, 0xd84bd3cf
	.word 0xa1ce3350, 0x35074a8f, 0xfd4e6da0, 0xe2c22e6f, 0x045de97e
	.word 0x0e6d45b9, 0x201624a2, 0x01e10dca, 0x2810aef2, 0xea0be721
	.word 0x3a3781e4, 0xa3602009, 0xd2ffcf69, 0xff7102e9, 0x36fab972
	.word 0x5c3650ff, 0x8cd44c9c, 0x25a4a676, 0xbd6385ce, 0xcd55c306
	.word 0xec8a31f5, 0xa87b24ce, 0x1e025786, 0x53d713c9, 0xb29d308f
	.word 0x0dc6cf3f, 0xf11139c9, 0x3afb3780, 0x0ed6b24c, 0xef04c8fe
	.word 0xab53d825, 0x3ca69893, 0x35460fb1, 0x058ead73, 0x0b567c59
	.word 0xfdddca3f, 0x6317e77d, 0xaa5febe5, 0x655f73e2, 0xd42455bb
	.word 0xe845a8bb, 0x351e4a67, 0xa36a9dfb, 0x3e0ac91d, 0xbaa0de01
	.word 0xec60dc66, 0xdb29309e, 0xcfa52971, 0x1f3eddaf, 0xe14aae61

#endif
