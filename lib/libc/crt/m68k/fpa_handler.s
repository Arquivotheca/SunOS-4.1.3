	.data
|	.asciz	"@(#)fpa_handler.s 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

/*
	int fpa_handler(sig,code,scp) ;
	int sig,code;
	struct sig_context *scp ;

*/

RTENTRY(_fpa_handler)
	movel	PARAM3,d0		| d0 gets scp.
	fmovemx	fp0/fp1,sp@-		| Save 68881 scratch registers.
	fmovel	fpcr,sp@-
	movel	d0,sp@-			| Push scp.
	JBSR(_fpa_handle,a0)
	addql	#4,sp			| Remove parameter from stack.
	fmovel	sp@+,fpcr
	fmovemx	sp@+,fp0/fp1
	tstl	d0
	jne	1f
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(fpa_error:w),a1
	pea	a1@
	movel	a0@(__iob:w),a0
	pea	a0@(40)
	JBSR(_fprintf,a0)
	addqw	#8,sp
	pea	a0@(40)
#else
	pea	fpa_error
	pea	__iob+40
	JBSR(_fprintf,a0)
	addqw	#8,sp
	pea	__iob+40
#endif
	JBSR(_fflush,a0)
	addqw	#4,sp
	JBSR(_abort,a0)
1:
	RET

	.data
fpa_error:
	.ascii	" Unknown FPA recomputation request\73 probably invalid FPA op code"
	.ascii	" \12\0"
