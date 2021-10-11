	.title	"D cache and interrupt disabling and enabling utilities"
	.file	"D_cache.s"


#include	<i860.h>
#include	<genasm.h>

        .data
        .align  8

_sccsid:
        .string "@(#)i_D_cache.s 1.1 92/07/30 SMI"


	.text
        .align  8

	.sbttl	"D cache and interrupt disabling routines."
	.page


// ***********************************************************************
// ***********************************************************************
// **									**
// **	_D_cache_disable						**
// **	_interrupt_disable						**
// **									**
// **									**
// **	This routine will disable the D caching and all interrupts	**
// **									**
// **									**
// **				CAUTION:				**
// **	     This routine should not use the stack in any way.		**
// **	     It is not validity checked.				**
// **									**
// **									**
// **	unsigned int  D_cache_disable();				**
// **	unsigned int  interrupt_disable();				**
// **									**
// **									**
// **	Input:	 NONE							**
// **									**
// **	Output:	 NONE							**
// **									**
// **	Returns: original interrupt mask (psr)				**
// **	Returns: original dirbase					**
// **									**
// **									**
// **	Registers clobbered:	r16, r17				**
// **									**
// **									**
// ***********************************************************************
// ***********************************************************************



_interrupt_disable::

	
	ld.c	psr,r16			// get/save current contents of psr
	andnot	PSR_IM,r16,r17		// turn off interrupts, keep old
	st.c	r17,psr			// disable interrupts

	bri	r1			// return to caller
	 nop				// return value = old interrupt mask


_D_cache_disable::


	ld.c    dirbase,r16             // get current dirbase
	or	DIR_RC_FREEZE,r16,r17	// disable D caching
	st.c	r17,dirbase

	bri	r1			// return to caller
	 nop				// return value = old dirbase



	.sbttl	"Enable interrupts and D caching"
	.page


// ***********************************************************************
// ***********************************************************************
// **									**
// **	_D_cache_enable							**
// **	_interrupt_enable						**
// **									**
// **									**
// **	This subroutines restore the D caching and interrupts which	**
// **	disabled before.						**
// **									**
// **				CAUTION:				**
// **	     This routine should not use the stack in any way.		**
// **	     It is not validity checked.				**
// **									**
// **									**
// **	void  D_cache_enable(old_mask);					**
// **	void  interrupt_enable(old_dirbase);				**
// **									**
// **									**
// **	Input:	 original interrupt mask.				**
// **	Input:	 original dirbase.					**
// **									**
// **	Output:	 NONE							**
// **									**
// **	Returns: NONE							**
// **									**
// **									**
// **	Registers clobbered: NONE					**
// **									**
// **									**
// ***********************************************************************
// ***********************************************************************

_D_cache_enable::

	st.c    r16,dirbase	// restore D caching

	bri	r1
	 nop



_interrupt_enable::

	st.c	r16,psr		// restore interrupt mask

	bri	r1
	 nop
