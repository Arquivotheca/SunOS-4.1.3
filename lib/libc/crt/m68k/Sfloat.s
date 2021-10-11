	.data
|	.asciz	"@(#)Sfloat.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Sfltd)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movel	a2@,SKYBASE
	bsr	Sfltd__
	movl	sp@+,a2
	RET
Sfltd__:
#else
	movl	__skybase,SKYBASE 
#endif PIC
	movw	#S_ITOD,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	movl	SKYBASE@,d1
	RET

RTENTRY(Sflts)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movel	a2@,SKYBASE
	bsr	Sflts__
	movl	sp@+,a2
	RET
Sflts__:
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_ITOS,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	RET

RTENTRY(Sund)
	cmpl	#0x41e00000,d0
	bges	1f
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movel	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FDTOL(d0,d1,d0)
	bras	2f
1:
	JBSR(Fund,a2)
2:
	RET

RTENTRY(Suns)
	cmpl	#0x4f000000,d0
	bges	1f
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_STOI,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	bras	2f
1:
	JBSR(Funs,a2)
2:
	RET

RTENTRY(Sintd)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FDTOL(d0,d1,d0)
	RET

RTENTRY(Sints)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_STOI,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	SKYBASE@,d0
	RET

RTENTRY(Sstod)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FSTOD(d0,d0,d1)
	RET

RTENTRY(Sdtos)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_DTOS,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET

RTENTRY(Saddd)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FADDD(d0,d1,ARG2PTR@+,ARG2PTR@,d0,d1)
	RET

RTENTRY(Sadds)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_SADD3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	SKYBASE@,d0
	RET

RTENTRY(Ssubd)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FSUBD(d0,d1,ARG2PTR@+,ARG2PTR@,d0,d1)
	RET

RTENTRY(Ssubs)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_SSUB3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	IORDY
	movl	SKYBASE@,d0
	RET

RTENTRY(Smuld)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FMULD(d0,d1,ARG2PTR@+,ARG2PTR@,d0,d1)
	RET

RTENTRY(Smuls)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FMULS(d0,d1,d0)
	RET

RTENTRY(Sdivd)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FDIVD(d0,d1,ARG2PTR@+,ARG2PTR@,d0,d1)
	RET

RTENTRY(Sdivs)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FDIVS(d0,d1,d0)
	RET

RTENTRY(Ssqrd)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FMULD(d0,d1,d0,d1,d0,d1)
	RET

RTENTRY(Ssqrs)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	FMULS(d0,d0,d0)
	RET

| 	Switch mode and status.

RTENTRY(Smode)
	movel	#ROUNDTODOUBLE,d0
	RET

RTENTRY(Sstatus)
	clrl	d0
	RET

RTENTRY(Scmpd)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_DCMP3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movl	ARG2PTR@+,SKYBASE@
	movl	ARG2PTR@ ,SKYBASE@
	movew	SKYBASE@,cc
	RET

RTENTRY(Scmps)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
	movl	sp@+,a2
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_SCMP3,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	movl	d1,SKYBASE@
	movw	SKYBASE@,cc
	RET

