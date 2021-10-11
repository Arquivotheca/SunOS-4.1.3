/*	@(#)pcb.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef _sparc_pcb_h
#define _sparc_pcb_h

/*
 * Sun software process control block
 */

#include <machine/reg.h>

#define	MAXWIN	12		/* max number of windows currently supported */

/*
 * The system actually supports one more than the above number.
 * There is always one window reserved for trap handlers that
 * never has to be saved into the pcb struct.
 */

#ifndef LOCORE
#ifdef STD_REF_MMU
#include <machine/vm_hat.h>
#endif
struct pcb {
#ifdef STD_REF_MMU
	/* this struct MUST be 1K aligned -- checked for in assym.s */
	struct  l1pt pcb_l1pt;  /* level 1 page table */
#endif
	label_t	pcb_regs;	/* saved pc and sp */
	int	pcb_psr; 	/* processor status word */
	int	pcb_uwm;	/* user window mask */
	struct	rwindow pcb_wbuf[MAXWIN]; /* user window save buffer */
	char	*pcb_spbuf[MAXWIN]; /* sp's for each wbuf */
	int	pcb_wbcnt;	/* number of saved windows in pcb_wbuf */

	int	*pcb_psrp;	/* psr pointer to en/disable coprocessors */
	struct	fpu *pcb_fpctxp;/* pointer to fpu state */
	int	pcb_fpflags;	/* fpu enable flags */
	int	*pcb_cpctxp;	/* pointer to coprocessor state */
	int	pcb_cpflags;	/* coprocessor enable flags */

	int	pcb_flags;	/* various state flags */
	int	pcb_wocnt;	/* window overflow count */
	int	pcb_wucnt;	/* window underflow count */
};

#define pcb_pc		pcb_regs.val[0]
#define pcb_sp		pcb_regs.val[1]

#define	aston()		{u.u_pcb.pcb_flags |= AST_SCHED;}

#define astoff()	{u.u_pcb.pcb_flags &= ~AST_SCHED;}

#endif !LOCORE

/* pcb_flags */
#define	AST_SCHED	0x80000000	/* force a reschedule */
#define	AST_CLR		0x80000000
#define	PME_CLR		0
#define	AST_NONE	0

/* pcb_flags */
#define CLEAN_WINDOWS	0x1		/* keep user regs clean */
#define FIX_ALIGNMENT	0x2		/* fix unaligned references */

#endif /*!_sparc_pcb_h*/
