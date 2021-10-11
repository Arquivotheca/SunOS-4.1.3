	.seg	"data"
	.asciz	"@(#)mcount.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.align	4
	.seg	"text"

#include <machine/asm_linkage.h>

!	Copyright (c) 1987 by Sun Microsystems, Inc.
!
!	Cast (in order of appearance)
!
#define	CELL_PP		%o0
#define SCRATCH		%o1
#define	PROF_P		%o5
#define CELL_P		%o4
#define CNTB_P		%o4
#define CNTB		%o3
#define CNTRS_P		%o2
#define CNTRS		%o5
	.seg	"data"
	.align	4
_profiling:
	.word	3
_countbase:
	.word	0
_numctrs:
	.word	0
_cntrs:
	.word	0
	.seg	"text"
	.proc	0
	.global mcount
mcount:
!	No save or restore.  We know that the previous routine just
!	did that.  We just don't use any of the i-registers.
!
!	The address of the pointer to the struct cnt for the calling
!	routine was left for us in %o0.
!
!	We have to be real careful with register usage.  Notice that
!	several of the register definitions above overlap.
!
	sethi	%hi(_profiling),PROF_P
	ld	[PROF_P+%lo(_profiling)],SCRATCH
	tst	SCRATCH
	bne	$1				!check for recursion
	mov	1,SCRATCH
	st	SCRATCH,[PROF_P+%lo(_profiling)]	!set profiling = 1
	ld	[CELL_PP],CELL_P
	tst	CELL_P				!indirect cell ptr initialized?
	bne,a	$2
	ld	[CELL_P],SCRATCH		!load cell if we branch
!
!	if we follow this path, we don't need the contents of
!	CELL_P anymore.
!
	sethi	%hi(_countbase),CNTB_P		!cell ptr not initialized
	ld	[CNTB_P+%lo(_countbase)],CNTB
	tst	CNTB				!did we run monstartup?
	be,a	$1				!exit
	st	%g0,[PROF_P+%lo(_profiling)]	!reset profiling if we branch

	sethi	%hi(_cntrs),CNTRS_P
	ld	[CNTRS_P+%lo(_cntrs)],CNTRS	!get _cntrs
!
!	we've just trashed our pointer to _profiling.  But we only follow
!	this path once per routine.
!
	sethi	%hi(_numctrs),SCRATCH		!get _numctrs
	ld	[SCRATCH+%lo(_numctrs)],SCRATCH
	inc	CNTRS				!increment _cntrs
	cmp	CNTRS,SCRATCH			!do we have any more space
	bg	$4
	st	CNTRS,[CNTRS_P+%lo(_cntrs)]	!store whether or not we branch
	st	%o7,[CNTB]			!store return address
	inc	4,CNTB
	st	CNTB,[CELL_PP]			!initalize cell pointer
	mov	1,SCRATCH
	st	SCRATCH,[CNTB]			!put 1 into cell
	inc	4,CNTB
	st	CNTB,[CNTB_P+%lo(_countbase)]	!increment countbase
	sethi	%hi(_profiling),PROF_P
	st	%g0,[PROF_P+%lo(_profiling)]	!reset profiling
	retl
	nop
	

$2:	
	inc	SCRATCH				!cell ptr was initialized
	st	SCRATCH,[CELL_P]		!increment cell
	st	%g0,[PROF_P+%lo(_profiling)]	!reset profiling	
$1:
	retl
	nop

$4:
	.seg	"data1"
$5:
	.ascii	"mcount\72 counter overflow\12\0"
	.seg	"text"
	save	%sp,-SA(MINFRAME),%sp
	set	$5,%o1
	mov	26,%o2
	call	_write,3
	mov	2,%o0
	ret
	restore
