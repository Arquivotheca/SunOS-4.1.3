#ifdef sccsid
static char     sccsid[] = "@(#)iu_simulator.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Integer Unit simulator for Sparc FPU simulator. */

#include <machine/fpu/fpu_simulator.h>
#include <machine/fpu/globals.h>

#define FPU_REG_FIELD unsigned_reg	/* Coordinate with FPU_REGS_TYPE. */
#define FPU_FSR_FIELD unsigned_reg	/* Coordinate with FPU_FSR_TYPE. */

PRIVATE enum ftt_type
fbcc(pinst, pregs, pfpu)
	fp_inst_type    pinst;	/* FPU instruction to simulate. */
	struct regs	*pregs;	/* Pointer to PCB image of registers. */
	struct fpu	*pfpu;	/* Pointer to FPU register block. */

{
	union {
		fsr_type	fsr;
		long		i;
	} klugefsr;
	union {
		fp_inst_type	fi;
		int		i;
	} kluge;
	enum fcc_type	fcc;
	enum icc_type {
		fbn, fbne, fblg, fbul, fbl, fbug, fbg, fbu,
		fba, fbe, fbue, fbge, fbuge, fble, fbule, fbo
	} icc;

	unsigned	annul, takeit;

	klugefsr.i = pfpu->fpu_fsr;
	fcc = klugefsr.fsr.fcc;
	icc = (enum icc_type) (pinst.rd & 0xf);
	annul = pinst.rd & 0x10;

	switch (icc) {
	case fbn:
		takeit = 0;
		break;
	case fbl:
		takeit = fcc == fcc_less;
		break;
	case fbg:
		takeit = fcc == fcc_greater;
		break;
	case fbu:
		takeit = fcc == fcc_unordered;
		break;
	case fbe:
		takeit = fcc == fcc_equal;
		break;
	case fblg:
		takeit = (fcc == fcc_less) || (fcc == fcc_greater);
		break;
	case fbul:
		takeit = (fcc == fcc_unordered) || (fcc == fcc_less);
		break;
	case fbug:
		takeit = (fcc == fcc_unordered) || (fcc == fcc_greater);
		break;
	case fbue:
		takeit = (fcc == fcc_unordered) || (fcc == fcc_equal);
		break;
	case fbge:
		takeit = (fcc == fcc_greater) || (fcc == fcc_equal);
		break;
	case fble:
		takeit = (fcc == fcc_less) || (fcc == fcc_equal);
		break;
	case fbne:
		takeit = fcc != fcc_equal;
		break;
	case fbuge:
		takeit = fcc != fcc_less;
		break;
	case fbule:
		takeit = fcc != fcc_greater;
		break;
	case fbo:
		takeit = fcc != fcc_unordered;
		break;
	case fba:
		takeit = 1;
		break;
	}
	if (takeit) {		/* Branch taken. */
		int		tpc;

		kluge.fi = pinst;
		tpc = pregs->r_pc;
		if (annul && (icc == fba)) {	/* fba,a is wierd */
			pregs->r_pc = tpc + (int) ((kluge.i << 10) >> 8);
			pregs->r_npc = pregs->r_pc + 4;
		} else {
			pregs->r_pc = pregs->r_npc;
			pregs->r_npc = tpc + (int) ((kluge.i << 10) >> 8);
		}
	} else {		/* Branch not taken. */
		if (annul) {	/* Annul next instruction. */
			pregs->r_pc = pregs->r_npc + 4;
			pregs->r_npc += 8;
		} else {	/* Execute next instruction. */
			pregs->r_pc = pregs->r_npc;
			pregs->r_npc += 4;
		}
	}
	return (ftt_none);
}

/*
 * Simulator for loads and stores between floating-point unit and memory.
 */
PRIVATE enum ftt_type
fldst(pfpsd, pinst, pregs, pwindow, pfpu)
	fp_simd_type	*pfpsd;	/* FPU simulator data. */
	fp_inst_type	pinst;	/* FPU instruction to simulate. */
	struct regs	*pregs;	/* Pointer to PCB image of registers. */
	struct rwindow	*pwindow; /* Pointer to locals and ins. */
	struct fpu	*pfpu;	/* Pointer to FPU register block. */
{
	unsigned	nrs1, nrs2, nrd;	/* Register number fields. */
	freg_type	f;
	int		ea;
	int		i;
	union {
		fp_inst_type	fi;
		int		i;
	} kluge;
	enum ftt_type   ftt;

	nrs1 = pinst.rs1;
	nrs2 = pinst.rs2;
	nrd = pinst.rd;
	if (pinst.ibit == 0) {	/* effective address = rs1 + rs2 */
		if (read_iureg(nrs1, pregs, 
			       (struct rwindow *)pwindow, &ea, pfpsd))
			return (ftt_fault);
		if (read_iureg(nrs2, pregs, 
			       (struct rwindow *)pwindow, &i, pfpsd))
			return (ftt_fault);
		ea += i;
	} else {		/* effective address = rs1 + imm13 */
		kluge.fi = pinst;
		ea = (kluge.i << 19) >> 19;	/* Extract simm13 field. */
		if (read_iureg(nrs1, pregs, 
			       (struct rwindow *)pwindow,  &i, pfpsd))
			return (ftt_fault);
		ea += i;
	}

	pfpsd->fp_trapaddr = (char *) ea; /* setup bad addr in case we trap */
	switch (pinst.op3 & 7) {
	case 0:		/* LDF */
		ftt = _fp_read_word((caddr_t) ea, &(f.int_reg), pfpsd);
		if (ftt != ftt_none)
			return (ftt);
		pfpu->fpu_regs[nrd] = f.FPU_REG_FIELD;
		break;
	case 1:		/* LDFSR */
		ftt = _fp_read_word((caddr_t) ea, &(f.int_reg), pfpsd);
		if (ftt != ftt_none)
			return (ftt);
		pfpu->fpu_fsr = f.FPU_FSR_FIELD;
		break;
	case 3:		/* LDDF */
		if ((ea & 0x7) != 0)
			return (ftt_alignment);	/* Require double-alignment. */
		ftt = _fp_read_word((caddr_t) ea, &(f.int_reg), pfpsd);
		if (ftt != ftt_none)
			return (ftt);
		pfpu->fpu_regs[DOUBLE_E(nrd)] = f.FPU_REG_FIELD;
		ftt = _fp_read_word((caddr_t) (ea + 4), &(f.int_reg), pfpsd);
		if (ftt != ftt_none)
			return (ftt);
		pfpu->fpu_regs[DOUBLE_F(nrd)] = f.FPU_REG_FIELD;
		break;
	case 4:		/* STF */
		f.FPU_REG_FIELD = pfpu->fpu_regs[nrd];
		ftt = _fp_write_word((caddr_t) ea, f.int_reg, pfpsd);
		if (ftt != ftt_none)
			return (ftt);
		break;
	case 5:		/* STFSR */
		f.FPU_FSR_FIELD = pfpu->fpu_fsr;
		f.FPU_FSR_FIELD &= ~0x301000;	/* Clear reserved bits. */
		f.FPU_FSR_FIELD |= 0x0E0000;	/* Set version number=7 . */
		ftt = _fp_write_word((caddr_t) ea, f.int_reg, pfpsd);
		if (ftt != ftt_none)
			return (ftt);
		break;
	case 7:		/* STDF */
		if ((ea & 0x7) != 0)
			return (ftt_alignment);	/* Require double-alignment. */
		f.FPU_REG_FIELD = pfpu->fpu_regs[DOUBLE_E(nrd)];
		ftt = _fp_write_word((caddr_t) ea, f.int_reg, pfpsd);
		if (ftt != ftt_none)
			return (ftt);
		f.FPU_REG_FIELD = pfpu->fpu_regs[DOUBLE_F(nrd)];
		ftt = _fp_write_word((caddr_t) (ea + 4), f.int_reg, pfpsd);
		if (ftt != ftt_none)
			return (ftt);
		break;
	default:
		/* addr of unimp inst */
		pfpsd->fp_trapaddr = (char *) pregs->r_pc;
		return (ftt_unimplemented);
	}

	pregs->r_pc = pregs->r_npc;	/* Do not retry emulated instruction. */
	pregs->r_npc += 4;
	return (ftt_none);
}

/* PUBLIC FUNCTIONS */

enum ftt_type
_fp_iu_simulator(pfpsd, pinst, pregs, pwindow, pfpu)
	fp_simd_type	*pfpsd;	/* FPU simulator data. */
	fp_inst_type	pinst;	/* FPU instruction to simulate. */
	struct regs	*pregs;	/* Pointer to PCB image of registers. */
	struct rwindow	*pwindow; /* Pointer to locals and ins. */
	struct fpu	*pfpu;	/* Pointer to FPU register block. */
{
	switch (pinst.hibits) {
	case 0:
		return (fbcc(pinst, pregs, pfpu));
	case 3:
		return (fldst(pfpsd, pinst, pregs, pwindow, pfpu));
	default:
		return (ftt_unimplemented);
	}
}
