/*	@(#)fpu_simulator.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

	/*	Sparc floating-point simulator PUBLIC include file. */

#include <machine/reg.h>
#include <sys/ieeefp.h>
#include <sys/types.h>

#include <vm/seg.h>


/*	PUBLIC TYPES	*/

enum fcc_type {			/* relationships */
	fcc_equal	= 0,
	fcc_less	= 1,
	fcc_greater	= 2,
	fcc_unordered	= 3
};

/* FSR types. */

enum ftt_type {			/* types of traps */
	ftt_none	= 0,
	ftt_ieee	= 1,
	ftt_unfinished	= 2,
	ftt_unimplemented = 3,
	ftt_sequence	= 4,
	ftt_alignment	= 5,	/* defined by software convention only */
	ftt_fault	= 6,	/* defined by software convention only */
	ftt_7		= 7
};

typedef	struct {		/* Sparc FSR. */
	enum fp_direction_type		rd   :	2; /* rounding direction */
	enum fp_precision_type		rp   :	2; /* rounding precision */
	unsigned			tem  :	5; /* trap enable mask */
	unsigned			:	6;
	enum ftt_type			ftt  :	3; /* FPU trap type */
	unsigned			qne  :	1; /* FPQ not empty */
	unsigned			pr   :	1; /* partial result */
	enum fcc_type			fcc  :	2; /* FPU condition code */
	unsigned			aexc :	5; /* accumulated exception */
	unsigned			cexc :	5; /* current exception */
} fsr_type;

typedef			/* FPU register viewed as single components. */
	struct {
	unsigned sign :		1;
	unsigned exponent :	8;
	unsigned significand : 23;
} single_type;

typedef			/* FPU register viewed as double components. */
	struct {
	unsigned sign :		1;
	unsigned exponent :    11;
	unsigned significand : 20;
} double_type;

typedef			/* FPU register viewed as extended components. */
	struct {
	unsigned sign :		 1;
	unsigned exponent :	15;
	unsigned significand :	16;
} extended_type;

typedef			/* FPU register with multiple data views. */
	union {
	int		int_reg;
	long		long_reg;
	unsigned	unsigned_reg;
	float		float_reg;
	single_type	single_reg;
	double_type	double_reg;
	extended_type	extended_reg;
} freg_type;

enum fp_op_type {		/* Type specifiers in FPU instructions. */
	fp_op_integer	= 0,	/* Not in hardware, but convenient to define. */
	fp_op_single	= 1,
	fp_op_double	= 2,
	fp_op_extended	= 3
};

enum fp_opcode {	/* FPU op codes, minus precision and leading 0. */
	fmovs		= 0x0,
	fnegs		= 0x1,
	fabss		= 0x2,
	fp_op_3 = 3, fp_op_4 = 4, fp_op_5 = 5, fp_op_6 = 6, fp_op_7 = 7,
	fp_op_8		= 0x8,
	fp_op_9		= 0x9,
	fsqrt		= 0xa,
	fp_op_b = 0xb, fp_op_c = 0xc, fp_op_d = 0xd,
	fp_op_e = 0xe, fp_op_f = 0xf,
	fadd		= 0x10,
	fsub		= 0x11,
	fmul		= 0x12,
	fdiv		= 0x13,
	fcmp		= 0x14,
	fcmpe		= 0x15,
	fp_op_16 = 0x16, fp_op_17 = 0x17,
	fp_op_18	= 0x18,
	fp_op_19	= 0x19,
	fsmuld		= 0x1a,
	fdmulx		= 0x1b,
	fp_op_20	= 0x20,
	fp_op_21	= 0x21,
	fp_op_22	= 0x22,
	fp_op_23	= 0x23,
	fp_op_24 = 0x24, fp_op_25 = 0x25, fp_op_26 = 0x26, fp_op_27 = 0x27,
	fp_op_28 = 0x28, fp_op_29 = 0x29, fp_op_2a = 0x2a, fp_op_2b = 0x2b,
	fp_op_2c = 0x2c, fp_op_2d = 0x2d, fp_op_2e = 0x2e, fp_op_2f = 0x2f,
	fp_op_30	= 0x30,
	ftos		= 0x31,
	ftod		= 0x32,
	ftox		= 0x33,
	ftoi		= 0x34,
	fp_op_35 = 0x35, fp_op_36 = 0x36, fp_op_37 = 0x37,
	ft_op_38	= 0x38,
	fp_op_39 = 0x39, fp_op_3a = 0x3a, fp_op_3b = 0x3b,
	fp_op_3c	= 0x3c,
	fp_op_3d = 0x3d, fp_op_3e = 0x3e, fp_op_3f = 0x3f
};

typedef			/* FPU instruction. */
	struct {
	unsigned		hibits	: 2;	/* Top two bits. */
	unsigned		rd	: 5;	/* Destination. */
	unsigned		op3	: 6;	/* Main op code. */
	unsigned		rs1	: 5;	/* First operand. */
	unsigned		ibit	: 1;	/* I format bit. */
	enum fp_opcode		opcode	: 6;	/* Floating-point op code. */
	enum fp_op_type		prec	: 2;	/* Precision. */
	unsigned		rs2	: 5;	/* Second operand. */
} fp_inst_type;

typedef			/* FPU data used by simulator. */
	struct {
	unsigned		fp_fsrtem;
	enum fp_direction_type	fp_direction;
	enum fp_precision_type	fp_precision;
	unsigned		fp_current_exceptions;
	struct	fpu		* fp_current_pfregs;
	void			(* fp_current_read_freg) ();
	void			(* fp_current_write_freg) ();
	int			fp_trapcode;
	char			*fp_trapaddr;
	struct	regs		*fp_traprp;
	enum	seg_rw		fp_traprw;
} fp_simd_type;

/* PUBLIC FUNCTIONS */


extern enum ftt_type fpu_simulator(/* pfpsd, pinst, pfsr, instr */);
/*	fp_simd_type	*pfpsd;	 Pointer to FPU simulator data */
/*	fp_inst_type	*pinst;	 Pointer to FPU instruction to simulate. */
/*	fsr_type	*pfsr;	 Pointer to image of FSR to read and write. */
/*	int		instr;	 Instruction to emulate. */

/*
 * fpu_simulator simulates FPU instructions only; reads and writes FPU data
 * registers directly.
 */

extern enum ftt_type fp_emulator (/* pfpsd, pinst, pregs, pwindow, pfpu */);
/*	fp_simd_type	*pfpsd;	   Pointer to FPU simulator data */
/*	fp_inst_type	*pinst;    Pointer to FPU instruction to simulate. */
/*	struct regs	*pregs;    Pointer to PCB image of registers. */
/*	struct rwindow	*pwindow;  Pointer to locals and ins. */
/*	struct fpu	*pfpu;	   Pointer to FPU register block. */

/*
 * fp_emulator simulates FPU and CPU-FPU instructions; reads and writes FPU
 * data registers from image in pfpu.
 */

extern void fp_traps(/* pfpsd, ftt, rp */);
/*	fp_simd_type	*pfpsd;	 Pointer to FPU simulator data */
/*	enum ftt_type	ftt;	 Type of trap. */
/*	struct regs	*rp;	 Pointer to PCB image of registers. */
/*
 * fp_traps handles passing execption conditions to the kernel.
 * It is called after fp_simulator or fp_emulator fail (return a non-zero ftt).
 */



