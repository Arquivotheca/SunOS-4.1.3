!	.asciz "@(#)rtldlib.s 1.1 92/07/30 SMI"
!
!Copyright (c) 1987 by Sun Microsystems, Inc.
!
#include <machine/asm_linkage.h>
#include <sys/syscall.h>

#define VERS1		1
#define BASEADDR	0
#define version		%i0
#define ip		%l0
#define baseaddr	%l2
#define RELOCOFF	0xc

	.seg    "text"
	.global	_binder,_rtbinder


    ! rtl(versionnumber)
    ! for version number 1 the following argument are passed on the stack:
    !		- base address where ld.so is mapped to
    !		- file descriptor for /dev/zero
    !		- file descriptor for ld.so
    !		- user dynamic pointer
    !		- environment pointer
    !		- break address for adb/dbx
_rtl:
	save	%sp,-SA(MINFRAME),%sp
L1:
	call    1f
        nop
1:
        sethi	%hi(__GLOBAL_OFFSET_TABLE_ - (L1 - 1b)), %l7
L2:
	or	%l7, %lo(__GLOBAL_OFFSET_TABLE_ - (L1 - L2)), %l7
	add	%l7, %o7, %l7
version1:
	mov	version,%o0
	add	%fp,%i1,ip		! get interface pointer
	mov	ip,%o1			! ptr to interface structure
	ld	[ip+BASEADDR],baseaddr	! address where ld.so is mapped in
	ld	[%l7],%l1		! ptr to ld.so first entry in globtable
	add	baseaddr,%l1,%o2	! push runtime linker dynamic pointer
	ld	[%l7+_rtld],%g1		! manually fix pic reference to rtld
	add	%g1,baseaddr,%g1
	jmpl	%g1,%o7
	nop
	mov	0,%o0
	mov	%o0,%i0
	ret
	restore

_rtbinder:
	save	%sp,-SA(MINFRAME),%sp
	mov	%i7,%o0			! %o0 has the address of the second
					! instruction of the jump slot
	ld	[%o0+RELOCOFF],%o1	! relocation index in %o1
	call	_binder
	nop
	mov	%o0,%g1			! save address of routine binded
	restore				! pop frame used for rtbinder
	restore				! pop frame used for jump slot
	jmp	%g1			! jump to it
	nop

! Special system call stubs to save system call overhead

	.global	_getreuid, _getregid
_getreuid:
	save	%sp,-SA(MINFRAME),%sp	! Build a frame
	mov	SYS_getuid,%g1		! Set system call
	t	0			! Do it
	st	%o0,[%i0]		! Store real uid
	st	%o1,[%i1]		!   and store effective uid
	ret				! Pipeline the return
	restore				!   with the frame pop

_getregid:
	save	%sp,-SA(MINFRAME),%sp	! Build a frame
	mov	SYS_getgid,%g1		! Set system call
	t	0			! Do it
	st	%o0,[%i0]		! Store real gid
	st	%o1,[%i1]		!   and store effective gid
	ret				! Pipeline the return
	restore				!   with the frame pop
