        .data
|        .asciz  "@(#)fp_save.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

#define	SIGFPE	8

#define	COMREG	-4	/* Sky command register offset from __skybase. */
#define	STCREG	-2	/* Sky status register offset from __skybase. */
#define SKYSAVE 0x1040	/* Sky context save command. */
#define SKYRESTORE 0x1041 /* Sky context restore command. */
#define SKYNOP 0x1063 	/* Sky noop command. */

ENTER(fp_save)
	subl	#FPSAVESIZE-4,sp	| Allocate block on stack.
	movel	sp@(FPSAVESIZE-4),sp@-	| Save proper return address.
	lea	sp@(4),a0		| a0 gets address of block.
	movel	a0@(FPSAVESIZE+16),d0	| d0 gets signal number.	
	movel	d0,a0@+			| Save signal number.
	cmpw	#SIGFPE,d0	
	jeq	9f			| If SIGFPE, don't save/restore.
					| If not SIGFPE, do save/restore.
#ifdef PIC
	PIC_SETUP(a1)
	movel	a1@(_fp_switch:w),a1
	movel	a1@,d0
#else
	movel	_fp_switch,d0
#endif
	movel	d0,a0@+			| Save fp_switch in case it gets wrecked.
	cmpw	#FP_SUNFPA,d0
	jne	3f
	JBSR(ffpa_save,a1)
	bras	9f
3:
	cmpw	#FP_MC68881,d0
	bnes	2f
	JBSR(f68881_save,a1)
	bras	9f
2:
	cmpw	#FP_SKYFFP,d0
	bnes	9f
	bsrs	fsky_save
9:
	rts


ENTER(fp_restore)
	lea	sp@(4),a0		| a0 gets address of block.
	movel	a0@+,d0
	cmpw	#SIGFPE,d0
	beqs	9f			| Branch if no restore required.
	movel	a0@+,d0			| Get saved fp_switch.
	cmpw	#FP_SUNFPA,d0
	jne	3f
	JBSR(ffpa_restore,a1)
	bras	9f
3:
	cmpw	#FP_MC68881,d0
	bnes	2f
	JBSR(f68881_restore,a1)
	bras	9f
2:
	cmpw	#FP_SKYFFP,d0
	bnes	9f
	JBSR(fsky_restore,a1)
9:
	movel	sp@,sp@(FPSAVESIZE)	| Move proper return address.
	addl	#FPSAVESIZE,sp		| Deallocate block on stack.
	rts

f68881_save:
	fmovem	fp0/fp1,a0@
	fmovem	fpcr/fpsr/fpiar,a0@(24)
	rts

f68881_restore:
	fmovem	a0@+,fp0/fp1
	fmovem	a0@ ,fpcr/fpsr/fpiar
	rts

#define IOCTL	0x40020000

fsky_save:
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a1
	movl	a1@,a1
	bsr	fsky_save__
	movl	sp@+,a2
	RET
fsky_save__:
#else
	movel	__skybase,a1
#endif PIC
1:
	movew	a1@(STCREG),d0
	btst	#14,d0
	beqs	2f			| Branch if busy.
	movew	#SKYNOP,a0@(32)
	bras	8f
2:
	tstw	a1@(STCREG)
	bpls	1b			| Branch if i/o not ready.

	pea	a0@(32)			| Push address of pc save area.
	movel	#IOCTL,sp@-
#ifdef PIC
	movl	a2@(__sky_fd:w),a2	| ***No more usage of a2!
	movl	a2@,sp@-
#else
	movl	__sky_fd,sp@-		| Push sky descriptor.
#endif

skyioctl:
	JBSR(_ioctl,a2)
	addl	#12,sp			| Remove parameters.
8:
	movew	#SKYNOP,a1@(COMREG)
	movew	#SKYSAVE,a1@(COMREG)
	movel	a1@,a0@
	movel	a1@,a0@(4)
	movel	a1@,a0@(8)
	movel	a1@,a0@(12)
	movel	a1@,a0@(16)
	movel	a1@,a0@(20)
	movel	a1@,a0@(24)
	movel	a1@,a0@(28)
	rts

fsky_restore:
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a1
	movl	a1@,a1
	bsr	fsky_restore__
	movl	sp@+,a2
	RET
fsky_restore__:	
#else
	movel	__skybase,a1
#endif
1:
	movew	a1@(STCREG),d0
	btst	#14,d0
	beqs	2f			| Branch if busy.
	bras	8f
2:
	tstw	a1@(STCREG)
	bpls	1b			| Branch if i/o not ready.

	pea	a0@(36)			| Push address of scratch area.
	movel	#IOCTL,sp@-
#ifdef PIC
	movel	a2@(__sky_fd),a2	| ***No more usage of a2!
	movel	a2@,sp@-
#else
	movel	__sky_fd,sp@-		| Push sky descriptor.
#endif
rskyioctl:
	JBSR(_ioctl,a2)	
	addl	#12,sp			| Remove parameters.
8:
	movew	#SKYNOP,a1@(COMREG)
	movew	#SKYRESTORE,a1@(COMREG)
	movel	a0@,a1@
	movel	a0@(4),a1@
	movel	a0@(8),a1@
	movel	a0@(12),a1@
	movel	a0@(16),a1@
	movel	a0@(20),a1@
	movel	a0@(24),a1@
	movel	a0@(28),a1@
1:
	movew	a1@(STCREG),d0
	btst	#14,d0
	beqs	1b
	movew	a0@(32),a1@(COMREG)
	rts

ffpa_save:
	fmovem	fp0/fp1,a0@
	addl	#24,a0
	fmovem	fpcr/fpsr/fpiar,a0@+
	movel	#(FPABASEADDRESS+0xf14),a1
	movel	a1@+,a0@+		| Stable read imask.
	movel	a1@+,a0@+		| Stable read load_ptr.
	movel	a1@+,a0@+		| Stable read ierr.
	movel	a1@+,a0@+		| Active instruction.
	movel	a1@+,a0@+		| Active data 1.
	movel	a1@+,a0@+		| Active data 2.
	movel	a1@+,a0@+		| Next instruction.
	movel	a1@+,a0@+		| Next data 1.
	movel	a1@+,a0@+		| Next data 2.
	movel	a1@+,a0@+		| Stable read weitek mode.
	movel	a1@+,a0@+		| Stable read weitek status.
	movel	#0,0xe0000f84		| Clear pipe.
	movel	#(FPABASEADDRESS+0xe00),a1
	movel	a1@+,a0@+		| fpa0
	movel	a1@+,a0@+		| fpa0
	movel	a1@+,a0@+		| fpa1
	movel	a1@+,a0@+		| fpa1
	movel	a1@+,a0@+		| fpa2
	movel	a1@+,a0@+		| fpa2
	movel	a1@+,a0@+		| fpa3
	movel	a1@+,a0@+		| fpa3

|	Set up almost like a new context.

	movel	#(FPABASEADDRESS),a1
	movel	#0,a1@(0xf1c)		| Clear ierr.
	movel	#0,a1@(0xf14)		| Set imask = 0 since SIGFPE
					| may already be signalled.
	fpmove@1 #2,fpamode		| Set FPA mode to default.
	fmovel	#0,fpsr
	fmovel	#0,fpcr
	rts

ffpa_restore:
	movel	#0,0xe0000f84		|  Clear pipe.
	fmovem	a0@+,fp0/fp1
	fmovem	a0@+,fpcr/fpsr/fpiar
	addl	#44,a0
	movel	#(FPABASEADDRESS+0xc00),a1
	movel	a0@+,a1@+		| fpa0
	movel	a0@+,a1@+		| fpa0
	movel	a0@+,a1@+		| fpa1
	movel	a0@+,a1@+		| fpa1
	movel	a0@+,a1@+		| fpa2
	movel	a0@+,a1@+		| fpa2
	movel	a0@+,a1@+		| fpa3
	movel	a0@+,a1@+		| fpa3
	subl	#76,a0
	movel	#(FPABASEADDRESS+0xf14),a1
	movel	a0@+,a1@+ 		|  imask.
	movel	a0@+,a1@+		|  load_ptr.
	movel	a0@+,a1@+		|  ierr.
	movel	#(FPABASEADDRESS),a1
	movel	a0@(24),d0
	andl	#0xf,d0			| Clear all but bottom bits.
	fpmove@1	d0,fpamode
	fpmove@1	a0@(28),fpastatus
	movel	#0,a1@(0x9c4)		| Rewrite shadow registers!
	movel	a0@,d0			| Load active instruction.
	lea	a0@(8),a0		| Point to active data.
	JBSR(ffpa_rewrite,a1)		| Restore active instruction.
	movel	a0@(-4),d0		| Load next instruction.
	lea	a0@(8),a0		| Point to next data.
	JBSR(ffpa_rewrite,a1)		| Restore next instruction.
	rts

ffpa_rewrite:
	swap	d0
	btst	#15,d0
	bnes	9f			| Quit if invalid.
	andw	#0x1ffc,d0		| Clear bogus bits.
	movel	a0@,a1@(0,d0:w)
	swap	d0
	btst	#15,d0
	bnes	9f			| Quit if invalid.
	andw	#0x1ffc,d0		| Clear bogus bits.
	movel	a0@(4),a1@(0,d0:w)
9:
	rts
