| 	stubs1.s 	1.1	92/07/30

#include "SYS.h"
#include "assym.s"

	.globl  cerror
err: jmp cerror;

	.globl	_mountroot
_mountroot:
	pea	0x0
	trap	#0
	addql	#4,sp
	rts

	.globl _open;
_open:
	pea	0x5
	trap	#0
	addql	#4,sp
	rts

	.globl	_read
_read:
	pea	0x3
	trap	#0
	addql	#4,sp
	rts

	.globl	_close
_close:
	pea	0x6
	trap	#0
	addql	#4,sp
	rts

	.globl	_lseek
_lseek:
	pea	0x13
	trap	#0
	addql	#4,sp
	rts

	.globl  syscall
/*
 * Get/Set vector base register
 */
        ENTRY(getvbr1)
        movc    vbr,d0
        rts

        ENTRY(setvbr)
        movl    sp@(4),d0
        movc    d0,vbr
        rts

/*
 * Make sure the vector for 'trap #0'
 * points to syscall()
 */
	ENTRY(set_vec)
	movl	sp@(4),a0
	movl	#syscall,a0@(0x080)
	rts

/*
 * Syscall.
 */
syscall:
        movl    sp@(8),d0                  | get the syscall code
syscont:
	pea	sp@(0x10)
        movl    d0,sp@-                 | push syscall code
        jsr     _syscall                | go to C routine
        addqw   #8,sp                   | pop arg

1:
        rte                             | and return!

	.globl __exit
cerror:
	jmp __exit
