/*
 *	.asciz	"@(#)trap.s 1.1 92/07/30 SMI"
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include "SYS.h"
#include "assym.s"

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
 * Syscall.
 */
	.globl  syscall
syscall:
        movl    sp@(8),d0                  | get the syscall code
syscont:
	pea	sp@(0x10)
        movl    d0,sp@-                 | push syscall code
        jsr     _syscall                | go to C routine
        addqw   #8,sp                   | pop arg
        rte                             | and return!

/*
 * Make sure the vector for 'trap #0'
 * points to syscall()
 */
	ENTRY(set_vec)
	movl	sp@(4),a0
	movl	#syscall,a0@(0x080)
	rts

/*
 * cerror stub
 */
	.globl	cerror
	.globl	__exit
cerror:
	jmp	__exit
