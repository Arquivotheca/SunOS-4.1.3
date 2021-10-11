#ifdef sccsid
static char	sccsid[] = "@(#)fpu_simulator.c 1.1 92/07/30 SMI;
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/* Main procedures for Sparc FPU simulator. */

#include <machine/fpu/fpu_simulator.h>
#include <machine/fpu/globals.h>
#include <sys/proc.h>
#include <sys/signal.h>

/* PUBLIC FUNCTIONS */

PRIVATE enum ftt_type
_fp_fpu_simulator(pfpsd, inst, pfsr)
	fp_simd_type	*pfpsd;	/* Pointer to fpu simulotor data */
	fp_inst_type	inst;	/* FPU instruction to simulate. */
	fsr_type	*pfsr;	/* Pointer to image of FSR to read and write. */
{
	unpacked	us1, us2, ud;	/* Unpacked operands and result. */
	unsigned	nrs1, nrs2, nrd;	/* Register number fields. */
	fsr_type	fsr;
	unsigned	usr;
	unsigned	andexcep;
	enum fcc_type	cc;

	nrs1 = inst.rs1;
	nrs2 = inst.rs2;
	nrd = inst.rd;
	fsr = *pfsr;
	pfpsd->fp_current_exceptions = 0;	/* Init current exceptions. */
	pfpsd->fp_fsrtem    = fsr.tem;		/* Obtain fsr's tem */
	pfpsd->fp_direction = fsr.rd;		/* Obtain rounding direction. */
	pfpsd->fp_precision = fsr.rp;		/* Obtain rounding precision. */
	switch (inst.opcode) {
	case fmovs:
		_fp_unpack_word(pfpsd, &usr, nrs2);
		_fp_pack_word(pfpsd, &usr, nrd);
		break;
	case fabss:
		_fp_unpack_word(pfpsd, &usr, nrs2);
		usr &= 0x7fffffff;
		_fp_pack_word(pfpsd, &usr, nrd);
		break;
	case fnegs:
		_fp_unpack_word(pfpsd, &usr, nrs2);
		usr ^= 0x80000000;
		_fp_pack_word(pfpsd, &usr, nrd);
		break;
	case fadd:
		_fp_unpack(pfpsd, &us1, nrs1, inst.prec);
		_fp_unpack(pfpsd, &us2, nrs2, inst.prec);
		_fp_add(pfpsd, &us1, &us2, &ud);
		_fp_pack(pfpsd, &ud, nrd, inst.prec);
		break;
	case fsub:
		_fp_unpack(pfpsd, &us1, nrs1, inst.prec);
		_fp_unpack(pfpsd, &us2, nrs2, inst.prec);
		_fp_sub(pfpsd, &us1, &us2, &ud);
		_fp_pack(pfpsd, &ud, nrd, inst.prec);
		break;
	case fmul:
		_fp_unpack(pfpsd, &us1, nrs1, inst.prec);
		_fp_unpack(pfpsd, &us2, nrs2, inst.prec);
		_fp_mul(pfpsd, &us1, &us2, &ud);
		_fp_pack(pfpsd, &ud, nrd, inst.prec);
		break;
	case fsmuld:
	case fdmulx:
		_fp_unpack(pfpsd, &us1, nrs1, inst.prec);
		_fp_unpack(pfpsd, &us2, nrs2, inst.prec);
		_fp_mul(pfpsd, &us1, &us2, &ud);
		_fp_pack(pfpsd, &ud, nrd, (enum fp_op_type) ((int)inst.prec+1));
		break;
	case fdiv:
		_fp_unpack(pfpsd, &us1, nrs1, inst.prec);
		_fp_unpack(pfpsd, &us2, nrs2, inst.prec);
		_fp_div(pfpsd, &us1, &us2, &ud);
		_fp_pack(pfpsd, &ud, nrd, inst.prec);
		break;
	case fcmp:
		_fp_unpack(pfpsd, &us1, nrs1, inst.prec);
		_fp_unpack(pfpsd, &us2, nrs2, inst.prec);
		cc = _fp_compare(pfpsd, &us1, &us2, 0);
		if (!(pfpsd->fp_current_exceptions & pfpsd->fp_fsrtem))
			fsr.fcc = cc;
		break;
	case fcmpe:
		_fp_unpack(pfpsd, &us1, nrs1, inst.prec);
		_fp_unpack(pfpsd, &us2, nrs2, inst.prec);
		cc = _fp_compare(pfpsd, &us1, &us2, 1);
		if (!(pfpsd->fp_current_exceptions & pfpsd->fp_fsrtem))
			fsr.fcc = cc;
		break;
	case fsqrt:
		_fp_unpack(pfpsd, &us1, nrs2, inst.prec);
		_fp_sqrt(pfpsd, &us1, &ud);
		_fp_pack(pfpsd, &ud, nrd, inst.prec);
		break;
	case ftoi:
		_fp_unpack(pfpsd, &us1, nrs2, inst.prec);
		pfpsd->fp_direction = fp_tozero;
		/* Force rounding toward zero. */
		_fp_pack(pfpsd, &us1, nrd, fp_op_integer);
		break;
	case ftos:
		_fp_unpack(pfpsd, &us1, nrs2, inst.prec);
		_fp_pack(pfpsd, &us1, nrd, fp_op_single);
		break;
	case ftod:
		_fp_unpack(pfpsd, &us1, nrs2, inst.prec);
		_fp_pack(pfpsd, &us1, nrd, fp_op_double);
		break;
	case ftox:
		_fp_unpack(pfpsd, &us1, nrs2, inst.prec);
		_fp_pack(pfpsd, &us1, nrd, fp_op_extended);
		break;
	default:
		return (ftt_unimplemented);
	}

	fsr.cexc = pfpsd->fp_current_exceptions;
	if (pfpsd->fp_current_exceptions) {	/* Exception(s) occurred. */
		andexcep = pfpsd->fp_current_exceptions & fsr.tem;
		if (andexcep != 0) {	/* Signal an IEEE SIGFPE here. */
			if (andexcep & (1 << fp_invalid))
				pfpsd->fp_trapcode = FPE_FLTOPERR_TRAP;
			else if (andexcep & (1 << fp_overflow))
				pfpsd->fp_trapcode = FPE_FLTOVF_TRAP;
			else if (andexcep & (1 << fp_underflow))
				pfpsd->fp_trapcode = FPE_FLTUND_TRAP;
			else if (andexcep & (1 << fp_division))
				pfpsd->fp_trapcode = FPE_FLTDIV_TRAP;
			else if (andexcep & (1 << fp_inexact))
				pfpsd->fp_trapcode = FPE_FLTINEX_TRAP;
			else
				pfpsd->fp_trapcode = 0;
			*pfsr = fsr;
			return (ftt_ieee);
		} else {	/* Just set accrued exception field. */
			fsr.aexc |= pfpsd->fp_current_exceptions;
		}
	}
	*pfsr = fsr;
	return (ftt_none);
}

/*
 * fpu_simulator simulates FPU instructions only;
 * reads and writes FPU data registers directly.
 */
enum ftt_type
fpu_simulator(pfpsd, pinst, pfsr, inst)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	fp_inst_type	*pinst;	/* Address of FPU instruction to simulate. */
	fsr_type	*pfsr;	/* Pointer to image of FSR to read and write.*/
	unsigned long	 inst;	/* The FPU instruction to simulate */
{
	union {
		int		i;
		fp_inst_type	inst;
	}		kluge;

	kluge.i = inst;
	pfpsd->fp_trapaddr = (char *) pinst;
	pfpsd->fp_current_read_freg = _fp_read_pfreg;
	pfpsd->fp_current_write_freg = _fp_write_pfreg;
	return (_fp_fpu_simulator(pfpsd, kluge.inst, pfsr));
}

/*
 * fp_emulator simulates FPU and CPU-FPU instructions; reads and writes FPU
 * data registers from image in pfpu.
 */
enum ftt_type
fp_emulator(pfpsd, pinst, pregs, pwindow, pfpu)
	fp_simd_type	*pfpsd;	/* Pointer to simulator data */
	fp_inst_type	*pinst;	/* Pointer to FPU instruction to simulate. */
	struct regs	*pregs;	/* Pointer to PCB image of registers. */
	struct rwindow	*pwindow; /* Pointer to locals and ins. */
	struct fpu	*pfpu;	/* Pointer to FPU register block. */
{
	union {
		int		i;
		fp_inst_type	inst;
	}		kluge;
	enum ftt_type	ftt;
	u_int		tfsr;

	tfsr = pfpu->fpu_fsr;
	pfpsd->fp_current_pfregs = pfpu;
	pfpsd->fp_current_read_freg = _fp_read_vfreg;
	pfpsd->fp_current_write_freg = _fp_write_vfreg;

	pfpsd->fp_trapaddr = (char *) pinst; /* bad inst addr in case we trap */
	ftt = _fp_read_word((caddr_t) pinst, &(kluge.i), pfpsd);
	if (ftt != ftt_none)
		return (ftt);
	if ((kluge.inst.hibits == 2) &&
	    ((kluge.inst.op3 == 0x34) || (kluge.inst.op3 == 0x35))) {
		ftt = _fp_fpu_simulator(pfpsd, kluge.inst, (fsr_type *)&tfsr);
		pfpu->fpu_fsr = tfsr;
		/* Do not retry emulated instruction. */
		pregs->r_pc = pregs->r_npc;
		pregs->r_npc += 4;
	} else
		ftt = _fp_iu_simulator(pfpsd, kluge.inst, pregs, pwindow, pfpu);

	if (ftt != ftt_none)
		return (ftt);

again:
	/*
	 * now read next instruction and see if it can be emulated
	 */
	pinst = (fp_inst_type *)pregs->r_pc;
	pfpsd->fp_trapaddr = (char *) pinst; /* bad inst addr in case we trap */
	ftt = _fp_read_word((caddr_t) pinst, &(kluge.i), pfpsd);
	if (ftt != ftt_none)
		return (ftt);


	if ((kluge.inst.hibits == 2) &&		/* fpops */
	    ((kluge.inst.op3 == 0x34) || (kluge.inst.op3 == 0x35))) {
		ftt = _fp_fpu_simulator(pfpsd, kluge.inst, (fsr_type *)&tfsr);
		/* Do not retry emulated instruction. */
		pfpu->fpu_fsr = tfsr;
		pregs->r_pc = pregs->r_npc;
		pregs->r_npc += 4;
	} else if (
						/* fldst */
	    ((kluge.inst.hibits == 3) && ((kluge.inst.op3 & 0x38) == 0x20)) ||
						/* fbcc */
	    ((kluge.inst.hibits == 0) && (((kluge.i>>21) & 0x7) == 6))) {
		ftt = _fp_iu_simulator(pfpsd, kluge.inst, pregs, pwindow, pfpu);
	} else
		return (ftt);

	if (ftt != ftt_none)
		return (ftt);
	else
		goto again;
}
