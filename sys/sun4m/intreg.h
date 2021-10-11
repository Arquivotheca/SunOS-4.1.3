/*      @(#)intreg.h 1.1 92/07/30 SMI      */

#ifndef	_sun4m_intreg_h
#define	_sun4m_intreg_h


#include <machine/devaddr.h>

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef LOCORE

struct cpu_intreg {
	u_int	pend;
	u_int	clr_pend;
	u_int	set_pend;
	u_char	filler[0x1000 - 0xc];
};
	
struct int_regs {
	struct	cpu_intreg	cpu[4];
	u_int	sys_pend;
	u_int	sys_m;
	u_int	sys_mclear;
	u_int	sys_mset;
	u_int	itr;
};
#define	INT_REGS	((struct int_regs *) (V_INT_REGS))

#endif !LOCORE

/*
 * Bits of the interrupt registers.
 */
#define	IR_SOFT_SHIFT	0x10		/* soft interrupt in set reg */
#define	IR_CPU_CLEAR	0x4		/* clear pending register for cpu */
#define	IR_CPU_SOFTINT	0x8		/* set soft interrupt for cpu */
#define	IR_SYS_CLEAR	(V_INT_REGS + 0x4008)
#define	IR_SYS_SET	(V_INT_REGS + 0x400C)

#define	SIPR_VADDR	(V_INT_REGS + 0x4000)

#define	SIR_MASKALL	0x80000000	/* mask all interrupts */
#define	SIR_MODERROR	0x40000000	/* module error */
#define	SIR_M2SWRITE	0x20000000	/* m-to-s write buffer error */
#define	SIR_ECCERROR	0x10000000	/* ecc memory error */
#define	SIR_VMEERROR	0x08000000	/* vme async error */
#define	SIR_FLOPPY	0x00400000	/* floppy disk */
#define	SIR_MODULEINT	0x00200000	/* module interrupt */
#define	SIR_VIDEO	0x00100000	/* onboard video */
#define	SIR_REALTIME	0x00080000	/* system timer */
#define	SIR_SCSI	0x00040000	/* onboard scsi */
#define	SIR_AUDIO	0x00020000	/* audio/isdn */
#define	SIR_ETHERNET	0x00010000	/* onboard ethernet */
#define	SIR_SERIAL	0x00008000	/* serial ports */
#define	SIR_KBDMS	0x00004000	/* keyboard/mouse */
#define	SIR_SBUSBITS	0x00003F80	/* sbus int bits */
#define	SIR_VMEBITS	0x0000007F	/* vme int bits */
/* asynchronous fault */
#define SIR_ASYNCFLT	(SIR_VMEERROR+SIR_ECCERROR+SIR_M2SWRITE+SIR_MODERROR)

#define	IR_ENA_CLK14	0x80		/* r/w - clock level 14 interrupt */
#define	IR_ENA_CLK10	0x80000		/* r/w - clock level 10 interrupt */
#define	IR_ENA_INT	0x80000000	/* r/w - enable (all) interrupts */

#define	IR_HARD_INT(n)	(0x000000001 << (n))
#define	IR_SOFT_INT(n)	(0x000010000 << (n))

#define	IR_SOFT_INT6	IR_SOFT_INT(6)		/* r/w - software level 6 interrupt */
#define	IR_SOFT_INT4	IR_SOFT_INT(4)		/* r/w - software level 4 interrupt */
#define	IR_SOFT_INT1	IR_SOFT_INT(1)		/* r/w - software level 1 interrupt */
#define IR_HARD_INT15	IR_HARD_INT(15)		/* r/w - hardware level 15 interrupt */

#endif	/* !_sun4m_intreg_h */
