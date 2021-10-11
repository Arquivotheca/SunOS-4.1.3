/*	@(#)pcb.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sun3_pcb_h
#define _sun3_pcb_h

#include <machine/reg.h>
#include <machine/buserr.h>

/*
 * Sun software process control block
 */

#ifndef LOCORE
struct pcb {
	label_t	pcb_regs;	/* saved registers */
	int	pcb_sr; 	/* program status word */
	int	pcb_flags;	/* various state flags */
	struct buserr_state {
		struct user_buserr_stack *u_buserr_stack;
		int u_buserr_pc;
	} u_buserr_state;
#define u_berr_stack	u_buserr_state.u_buserr_stack
#define u_berr_pc	u_buserr_state.u_buserr_pc
	struct fp_status u_fp_status;	/* user visible fpp state */
/*
 * Struct for the internal state of the MC68881
 * This is the maximum allowed by the coprocessor interface for internal state.
 */
#define	FPIS_BUFSIZ	0x100
#define	FPP_EXC_PEND	0x8	/* exception pending bit in buiflags msbyte */
	struct fp_istate {
		unsigned char	fpis_vers;	/* version number */
		unsigned char	fpis_bufsiz;	/* size of info in fpis_buf */
		unsigned short	fpis_reserved;	/* reserved word */
		unsigned char	fpis_buf[FPIS_BUFSIZ];	/* fpp internal state buffer */
	} u_fp_istate;	/* internal fpp state */
	/* fpa only */
	short	u_fpa_flags; /* if zero, u_fpa_*'s are meaningless */
	struct fpa_status u_fpa_status; /* saved/restored on ctx switches */
	/* end fpa only */
};

/*
 * On user bus errors that result in a signal, the following stack frame
 * is saved so that we can restart after return from the signal handler.
 */
struct user_buserr_stack {
	u_int	uberr_save_size;		/* Size of saved frame */
	long	uberr_ast;			/* AST bits */
	struct regs uberr_regs;			/* processor registers */
	short	uberr_stkfmt;			/* Exception frame format */
	union bei{
		struct bei_longb  bei_longb;	/* Bus error exception frame */
		struct bei_medium  bei_medium;	/* Bus error exception frame */
		struct bei_coproc bei_coproc;	/* Mid-Instruction frame */
	} u_bei;
#define uberr_longb	u_bei.bei_longb
#define uberr_medium	u_bei.bei_medium
#define uberr_coproc	u_bei.bei_coproc
	struct fp_istate fp_istate;		/* 68882 internals */
};
#endif

#define	AST_SCHED	0x80000000	/* force a reschedule */
#define	AST_STEP	0x40000000	/* force a single step */
#define	TRACE_USER	0x20000000	/* user has requested tracing */
#define	TRACE_AST	0x10000000	/* AST has requested tracing */
#define	TRACE_PENDING	0x08000000	/* trace caught in supervisor mode */
#define	AST_CLR		0xf8000000
#define	PME_CLR		0
#define	AST_NONE	0

#define	aston()		{u.u_pcb.pcb_flags |= AST_SCHED;}

#endif /*!_sun3_pcb_h*/
