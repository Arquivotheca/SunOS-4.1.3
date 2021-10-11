!	stret.sa (special version for boot)
! 	Copyright (c) 1986 by Sun Microsystems, Inc.
!	.seg	"data"
!	.asciz	"@(#)stret.s 1.1 92/07/30"

 	.seg	"text"
 	.align	4


#include <machine/asm_linkage.h>

#define FROM	%o0
#define SIZE	%o1
#define TO	%i0
#define MEMWORD	%o3	/* use o-registers as temps */
#define TEMP	%o4
#define TEMP2	%o5
#define UNIMP	0
#define MASK	0x00000fff
#define STRUCT_VAL_OFF	(16*4)


/*#if STRET4 */
#    define BYTES_PER_MOVE	4
#    define LD	ld
#    define ST	st
	RTENTRY(.stret4)
	RTENTRY(.stret8)
/*#endif*/
/*
#if STRET2
#    define BYTES_PER_MOVE	2
#    define LD	lduh
#    define ST	sth
	RTENTRY(.stret2)
#endif
#if STRET1
#    define BYTES_PER_MOVE	1
#    define LD	ldub
#    define ST	stb
	RTENTRY(.stret1)
#endif
*/

	/*
	 * see if key matches: if not,
	 * structure value not expected,
	 * so just return
	 */
	ld	[%i7+8],MEMWORD
	and	SIZE,MASK,TEMP
	sethi	%hi(UNIMP),TEMP2
	or      TEMP,TEMP2,TEMP2
	cmp     TEMP2,MEMWORD
	bne	LE12
	.empty
	/*
	 * set expected return value
	 */
L14:
	ld	[%fp+STRUCT_VAL_OFF],TO		/* (DELAY SLOT) */
	/* 
	 * copy the struct using the same loop the compiler does
	 * for large structure assignment
	 */
L15:
	subcc	SIZE,BYTES_PER_MOVE,SIZE
	LD	[FROM+SIZE],TEMP
	bg	L15
	ST	TEMP,[TO+SIZE]			/* (DELAY SLOT) */
	/* bump return address */
	add	%i7,0x4,%i7
LE12:
	jmp	%i7+8
	restore
       LF12 = -92
