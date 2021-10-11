#if !defined(lint) && defined(SCCSIDS)
static char     sccsid[] = "@(#)fpa_recompute.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc. 
 */

#include <signal.h>
#include <stdio.h>
#include "fpa_support.h"

extern double
                fabs();		/* Inline expansion actually takes care of
				 * this. */

STATIC
extended_precision()
/* Forces extended precision expression evaluation	 */

{
	int             oldmode, newmode;

#ifdef F68881
	newmode = 0;
	oldmode = Wmode(newmode);
	newmode = oldmode & ~0xc0;	/* Force extended */
	Wmode(newmode);
#endif
}

STATIC long
read_msw(n)
	int             n;
{				/* read most significant 32 bits of FPA reg n */
	long           *fpa_pointer;
	fpa_pointer = (long *) (FPABASE + 0xc00 + 8 * n);
	return (*fpa_pointer);
}

STATIC long
read_lsw(n)
	int             n;
{				/* read least significant 32 bits of FPA reg
				 * n */
	long           *fpa_pointer;
	fpa_pointer = (long *) (FPABASE + 0xc04 + 8 * n);
	return (*fpa_pointer);
}

STATIC void
write_msw(n, data)
	int             n;
	long            data;
{				/* write most significant 32 bits of FPA reg
				 * n */
	long           *fpa_pointer;
	fpa_pointer = (long *) (FPABASE + 0xc00 + 8 * n);
	*fpa_pointer = data;
}

STATIC void
write_lsw(n, data)
	int             n;
	long            data;
{				/* write most significant 32 bits of FPA reg
				 * n */
	long           *fpa_pointer;
	fpa_pointer = (long *) (FPABASE + 0xc04 + 8 * n);
	*fpa_pointer = data;
}

STATIC double
read_single(n)
	int             n;
{				/* read FPA register n as single */
	union singlekluge k;
	union doublekluge dk;

	k.i[0] = read_msw(n);
	dk.x = (double) k.x;
	/*
	 * Conversion of single to double causes no exception except possibly
	 * converting a signalling NaN to quiet.  The following code undoes
	 * that. 
	 */
	if (Wstatus(0) & 0x80) {/* Wstatus(0) clears fpsr. */
		dk.i[0] = dk.i[0] & ~0x00080000;	/* Convert quiet back to
							 * signalling. */
	}
	return (dk.x);
}

STATIC double
read_long(n)
	int             n;
{				/* read FPA register n as long */
	union singlekluge k;

	k.x = read_msw(n);
	return (k.x);
}

STATIC double
read_double(n)
	int             n;
{				/* read FPA register n as double */
	union doublekluge k;

	k.i[0] = read_msw(n);
	k.i[1] = read_lsw(n);
	return (k.x);
}

STATIC void
write_long(n, data)
	int             n;
	double          data;
{				/* write FPA register n as long */
	union singlekluge k;

	k.i[0] = data;
	write_msw(n, k.i[0]);
}

STATIC void
write_single(n, data)
	int             n;
	double          data;
{				/* write FPA register n as single */
	union singlekluge k;

	k.x = data;
	write_msw(n, k.i[0]);
}

STATIC void
write_double(n, data)
	int             n;
	double          data;
{				/* write FPA register n as double */
	union doublekluge k;

	k.x = data;
	write_msw(n, k.i[0]);
	write_lsw(n, k.i[1]);
}

STATIC double
read_2_single(data)
	long            data;
/*
 * returns FPA register defined in the reg_2 field of data from command
 * format. 
 */
{
	long           *fpa_pointer;
	int             reg_1;
	double          reg_1_op, reg_2_op;

	reg_1 = data & 0x1f;
	reg_1_op = read_single(reg_1);
	fpa_pointer = (long *) (FPABASE + 0x880);
	*fpa_pointer = data;	/* reg_1 gets reg_2 contents */
	reg_2_op = read_single(reg_1);
	write_single(reg_1, reg_1_op);
	return (reg_2_op);
}

STATIC double
read_2_double(data)
	long            data;
/*
 * returns FPA register defined in the reg_2 field of data from command
 * format. 
 */
{
	long           *fpa_pointer;
	int             reg_1;
	double          reg_1_op, reg_2_op;

	reg_1 = data & 0x1f;
	reg_1_op = read_double(reg_1);
	fpa_pointer = (long *) (FPABASE + 0x884);
	*fpa_pointer = data;	/* reg_1 gets reg_2 contents */
	reg_2_op = read_double(reg_1);
	write_double(reg_1, reg_1_op);
	return (reg_2_op);
}

STATIC double
read_3_single(data)
	long            data;
/*
 * returns FPA register defined in the reg_3 field of data from command
 * format, wiping out reg_1 in the process. 
 */
{
	long           *fpa_pointer, reg_1;
	double          reg_1_op, reg_2_op;

	reg_1 = data & 0x1f;
	reg_1_op = read_single(reg_1);
	fpa_pointer = (long *) (FPABASE + 0x880);
	*fpa_pointer = ((data >> 10) & ~0x1f) | reg_1;	/* reg_1 gets reg_3
							 * contents */
	reg_2_op = read_single(reg_1);
	write_single(reg_1, reg_1_op);
	return (reg_2_op);
}

STATIC double
read_3_double(data)
	long            data;
/*
 * returns FPA register defined in the reg_3 field of data from command
 * format. 
 */
{
	long           *fpa_pointer, reg_1;
	double          reg_1_op, reg_2_op;

	reg_1 = data & 0x1f;
	reg_1_op = read_double(reg_1);
	fpa_pointer = (long *) (FPABASE + 0x884);
	*fpa_pointer = ((data >> 10) & ~0x1f) | reg_1;	/* reg_1 gets reg_3
							 * contents */
	reg_2_op = read_double(reg_1);
	write_double(reg_1, reg_1_op);
	return (reg_2_op);
}

STATIC double
read_4_single(data)
	long            data;
/*
 * returns FPA register defined in the reg_4 field of data from command
 * format. 
 */
{
	long           *fpa_pointer, reg_1;
	double          reg_1_op, reg_2_op;

	reg_1 = data & 0x1f;
	reg_1_op = read_single(reg_1);
	fpa_pointer = (long *) (FPABASE + 0x880);
	*fpa_pointer = ((data >> 20) & 0x7c0) | reg_1;	/* reg_1 gets reg_4
							 * contents */
	reg_2_op = read_single(reg_1);
	write_single(reg_1, reg_1_op);
	return (reg_2_op);
}

STATIC double
read_4_double(data)
	long            data;
/*
 * returns FPA register defined in the reg_4 field of data from command
 * format. 
 */
{
	long           *fpa_pointer, reg_1;
	double          reg_1_op, reg_2_op;

	reg_1 = data & 0x1f;
	reg_1_op = read_double(reg_1);
	fpa_pointer = (long *) (FPABASE + 0x884);
	*fpa_pointer = ((data >> 20) & 0x7c0) | reg_1;	/* reg_1 gets reg_4
							 * contents */
	reg_2_op = read_double(reg_1);
	write_double(reg_1, reg_1_op);
	return (reg_2_op);
}


STATIC void
fpa_compare(x, y, pc)
	double          x, y;
	unsigned short *pc;

/* Procedure to recompute Weitek status for x compare y - always unord. */

{
	long           *fpa_pointer;
	int             i, cc;

	/*
	 * Following code attempts to determine if compare was for == != or
	 * for an ordered comparison < <= > >=.  The way it works is the
	 * instruction stream is followed from pc+6 to find a bcc, dbcc, or
	 * scc instruction: 
	 *
	 *
	 	fpcmps@1  a0@,fpa2 
pc->		fpmove@1  fpastatus,d0 
pc+4->         	movw    d0,cc 
pc+6->         	seq     d0 
	 *
	 * A bcc looks like 6cXX. A dbcc or scc looks like 5cXX with bits 6 and 7
	 * on. 
	 *
	 * Obviously this code can be fooled so we allow at most 100
	 * instructions. 
	 *
	 * This only matters if... 1) neither operand is a signaling nan 2) the
	 * comparison is == or !=. When both conditions hold, IEEE invalid
	 * should not be signaled. 
	 */

	pc = (unsigned short *) ((int) pc + 6);
	i = 0;
	while (((*pc & 0xf000) != 0x6000) && ((*pc & 0xf0c0) != 0x50c0) && (i < 100)) {
		i++;
		pc++;
	}
	if (((*pc & 0xf000) != 0x6000)
	    || ((*pc & 0xf0c0) != 0x50c0)) {	/* cc op code found */
		cc = (*pc & 0x0f00) >> 8;
		if ((cc == 6) || (cc == 7))
			cc = (x == y);	/* == or !=, don't signal. */
		else
			cc = (x >= y);	/* Unordered, always signal. */
	} else {		/* no cc op code found */
		printf(" no cc op code found \n") ;
		cc = x >= y;	/* Assume unordered, always signal. */
	}
	if (!cc) {		/* cc is always false! But we need to fool
				 * the optimizer! */
		fpa_pointer = (long *) (FPABASE + 0x958);
		*fpa_pointer = 0xf00;	/* Set unordered. */
	}
}

STATIC int
short_single(a1, pc)
	struct fpa_access *a1;
	unsigned short *pc;

/* Procedure to recompute short single format instructions. 	 */

{
	struct fpa_access ap0;
	int             opcode, src, dest, found;
	double          reg, operand, result;
	union singlekluge sk;

	found = 1;
	ap0 = *a1;
	opcode = ap0.kluge.kis.op;
	src = ap0.kluge.kis.reg1;
	dest = ap0.kluge.kis.reg1;
	reg = read_single(src);
	sk.i[0] = ap0.data;
	operand = sk.x;
	switch (opcode) {
	case 1:
		result = -(operand);
		break;
	case 2:
		result = fabs(operand);
		break;
	case 3:
		result = sk.i[0];
		break;
	case 4:
		result = fpa_tolong(operand);
		write_long(dest, result);
		goto pastwrites;
	case 5:
		result = operand;
		write_double(dest, result);
		goto pastwrites;
	case 6:
		result = (float) (operand * operand);
		break;
	case 7:
		result = (float) (reg + operand);
		break;
	case 8:
		result = (float) (reg - operand);
		break;
	case 9:
		result = (float) (reg * operand);
		break;
	case 10:
		result = (float) (reg / operand);
		break;
	case 11:
		result = (float) (operand - reg);
		break;
	case 12:
		result = (float) (operand / reg);
		break;
	case 13:
		fpa_compare(operand, 0.0, pc);
		goto pastwrites;
	case 14:
		fpa_compare(operand, reg, pc);
		goto pastwrites;
	case 15:
		fpa_compare(fabs(operand), fabs(reg), pc);
		goto pastwrites;
	default:
		found = 0;
		goto pastwrites;
	}
	write_single(dest, result);
pastwrites:;
	if (!found)
		fprintf(stderr, "\n Invalid result from failed FPA short single format instruction:\n operand %g reg_1 %g \n", operand, reg);
	return (found);
}

STATIC int
short_double(a1, a2, pc)
	struct fpa_access *a1, *a2;
	unsigned short *pc;

/* Procedure to recompute short double format instructions. 	 */

{
	struct fpa_access ap0, ap1;
	int             opcode, src, dest, found;
	double          reg, operand, result;
	union doublekluge dk;

	found = 1;
	ap0 = *a1;
	ap1 = *a2;
	opcode = ap0.kluge.kis.op;
	src = ap0.kluge.kis.reg1;
	dest = ap0.kluge.kis.reg1;
	reg = read_double(src);
	dk.i[0] = ap0.data;
	dk.i[1] = ap1.data;
	operand = dk.x;
	switch (opcode) {
	case 1:
		result = -(operand);
		break;
	case 2:
		result = fabs(operand);
		break;
	case 3:
		result = dk.i[0];
		break;
	case 4:
		result = fpa_tolong(operand);
		write_long(dest, result);
		goto pastwrited;
	case 5:
		result = (double) (float) operand;
		write_single(dest, result);
		goto pastwrited;
	case 6:
		result = operand * operand;
		break;
	case 7:
		result = reg + operand;
		break;
	case 8:
		result = reg - operand;
		break;
	case 9:
		result = reg * operand;
		break;
	case 10:
		result = reg / operand;
		break;
	case 11:
		result = operand - reg;
		break;
	case 12:
		result = operand / reg;
		break;
	case 13:
		fpa_compare(operand, 0.0, pc);
		goto pastwrited;
	case 14:
		fpa_compare(operand, reg, pc);
		goto pastwrited;
	case 15:
		fpa_compare(fabs(operand), fabs(reg), pc);
		goto pastwrited;
	default:
		found = 0;
		goto pastwrited;
	}
	write_double(dest, result);
pastwrited:
	if (!found)
		fprintf(stderr, "\n Invalid result from failed FPA short double format instruction:\n operand %g reg_1 %g \n", operand, reg);
	return (found);
}

STATIC int
extended_semantics(opcode, reg_1, prec, mem_op, mem_2_op, reg_2_op, reg_3_op)
	int             opcode, prec, reg_1;
	double          mem_op, mem_2_op, reg_2_op, reg_3_op;
{
	int             found;
	double          result;
	complexunion	complex1, complex2, complex3;
		double d1, d2;
		pdoublekluge pdk;

	found = 1;
	switch (opcode) {
	case 0:
		extended_precision();
		result = mem_2_op + (reg_2_op * mem_op);
		break;
	case 1:
		extended_precision();
		result = mem_2_op - (reg_2_op * mem_op);
		break;
	case 2:
		extended_precision();
		result = (reg_2_op * mem_op) - mem_2_op;
		break;
	case 3:
		extended_precision();
		result = mem_2_op * (reg_2_op + mem_op);
		break;
	case 4:
		extended_precision();
		result = mem_2_op * (reg_2_op - mem_op);
		break;
	case 5:
		extended_precision();
		result = mem_2_op * (mem_op - reg_2_op);
		break;
	case 6:
		result = reg_2_op + mem_op;
		break;
	case 7:
		result = reg_2_op - mem_op;
		break;
	case 8:
		result = reg_2_op * mem_op;
		break;
	case 9:
		result = reg_2_op / mem_op;
		break;
	case 10:
		result = mem_op - reg_2_op;
		break;
	case 11:
		result = mem_op / reg_2_op;
		break;
        case 12: /* sqrt */
                result = fpa_transcendental(8, mem_op);
                break;
        case 13: /* hypot */
		extended_precision();
                pdk.pd[0] = (double *)&mem_op;
		pdk.pd[1] = (double *)&reg_2_op;
		result = fpa_transcendental(9, pdk.d);
                break;
        case 14: /* complex negation */
                complex3.d = mem_op;
                complex1.f[0] = -complex3.f[0];
                complex1.f[1] = -complex3.f[1];
                result = complex1.d;
                break;
        case 15: /* complex absolute value */
		extended_precision();
                complex3.d = mem_op;
                d1=complex3.f[0];
		d2=complex3.f[1];
                pdk.pd[0] = (double *)&d1;
		pdk.pd[1] = (double *)&d2;
		result = (double) (float) fpa_transcendental(9, pdk.d);
                write_single(reg_1, result);
                goto pastwrite;
                break;
	case 16:
		extended_precision();
		result = reg_3_op + (reg_2_op * mem_op);
		break;
	case 17:
		extended_precision();
		result = reg_3_op - (reg_2_op * mem_op);
		break;
	case 18:
		extended_precision();
		result = (reg_2_op * mem_op) - reg_3_op;
		break;
	case 19:
		extended_precision();
		result = reg_3_op * (reg_2_op + mem_op);
		break;
	case 20:
		extended_precision();
		result = reg_3_op * (reg_2_op - mem_op);
		break;
	case 21:
		extended_precision();
		result = reg_3_op * (mem_op - reg_2_op);
		break;
	case 22:
		extended_precision();
		result = mem_op + (reg_2_op * reg_3_op);
		break;
	case 23:
		extended_precision();
		result = mem_op - (reg_2_op * reg_3_op);
		break;
	case 24:
		extended_precision();
		result = (reg_2_op * reg_3_op) - mem_op;
		break;
	case 25:
		extended_precision();
		result = mem_op * (reg_2_op + reg_3_op);
		break;
	case 26:
		extended_precision();
		result = mem_op * (reg_3_op - reg_2_op);
		break;
        case 28: /* complex addition */
                complex2.d = reg_2_op;
                complex3.d = mem_op;
                complex1.f[0] = complex2.f[0]+complex3.f[0];
                complex1.f[1] = complex2.f[1]+complex3.f[1];
                result = complex1.d;
                break;
        case 29: /* complex subtraction */
                complex2.d = reg_2_op;
                complex3.d = mem_op;
                complex1.f[0] = complex2.f[0]-complex3.f[0];
                complex1.f[1] = complex2.f[1]-complex3.f[1];
                result = complex1.d;
                break;
        case 30: /* complex multiplication */
                extended_precision();
                complex2.d = reg_2_op;
                complex3.d = mem_op;
                complex1.f[0] = complex2.f[0]*complex3.f[0]-complex2.f[1]*complex3.f[1];
                complex1.f[1] = complex2.f[0]*complex3.f[1]+complex2.f[1]*complex3.f[0];
                result = complex1.d;
                break;
        case 31: /* complex division */
                extended_precision();
                complex2.d = reg_2_op;
                complex3.d = mem_op;
                if (complex3.f[1] == 0)
			{ /* Special case division by real. */
			complex1.f[0]=complex2.f[0]/complex3.f[0];
			complex1.f[1]=complex2.f[1]/complex3.f[0];
			}
		else
			{ /* General case. */
			register double denominator = complex3.f[0] * complex3.f[0] + complex3.f[1] * complex3.f[1] ;
                complex1.f[0] = (complex2.f[0]*complex3.f[0]+complex2.f[1]*complex3.f[1])/denominator;
                complex1.f[1] = (complex2.f[1]*complex3.f[0]-complex2.f[0]*complex3.f[1])/denominator;
			}
                result = complex1.d;
                break;
	default:
		found = 0;
	}
	if (found) {
		if (prec)
			write_double(reg_1, result);
		else
			write_single(reg_1, result);
	}
pastwrite:
	if (!found)
		fprintf(stderr, "\n Invalid result from failed FPA extended format instruction:\n operand %X reg_2 %g reg_3 %g \n", mem_op, reg_2_op, reg_3_op);
	return (found);
}

STATIC int
extended_single(a1, a2)
	struct fpa_access *a1, *a2;

/* Procedure to recompute extended single format instructions. 	 */

{
	int             found, opcode;
	struct fpa_access ac1, ac2;
	short           reg_2, reg_3;
	double          reg_2_op, reg_3_op, mem_op, mem_2_op;
	union singlekluge sk;

	ac1 = *a1;
	ac2 = *a2;
	reg_2 = ac1.kluge.kis.reg1;
	reg_2_op = read_single(reg_2);
	reg_3 = ac2.kluge.kis.op & 0xf;
	reg_3_op = read_single(reg_3);
	sk.i[0] = ac1.data;
	mem_op = sk.x;
	sk.i[0] = ac2.data;
	mem_2_op = sk.x;

	found = extended_semantics(ac1.kluge.kis.op & 0x1f, ac2.kluge.kis.reg1, ac1.kluge.kis.prec, mem_op, mem_2_op, reg_2_op, reg_3_op);

	return (found);
}

STATIC int
extended_double(a1, a2)
	struct fpa_access *a1, *a2;

/* Procedure to recompute extended double format instructions. 	 */

{
	int             found, opcode;
	struct fpa_access ac1, ac2;
	short           reg_2, reg_3;
	double          reg_2_op, reg_3_op, mem_op;
	union doublekluge dk;

	ac1 = *a1;
	ac2 = *a2;
	reg_2 = ac1.kluge.kis.reg1;
	reg_2_op = read_double(reg_2);
	reg_3 = ac2.kluge.kis.op & 0xf;
	reg_3_op = read_double(reg_3);
	dk.i[0] = ac1.data;
	dk.i[1] = ac2.data;
	mem_op = dk.x;

	found = extended_semantics(ac1.kluge.kis.op & 0x1f, ac2.kluge.kis.reg1, ac1.kluge.kis.prec, mem_op, 0.0, reg_2_op, reg_3_op);

	return (found);
}

STATIC int
direct_semantics(opcode, prec, data, pc)
	int             opcode, prec, data;
{
	int             found;
	double          reg_2_op, reg_3_op, result;
	union singlekluge sk;

	found = 1;
	if (prec) {		/* double */
		reg_2_op = read_2_double(data);
		reg_3_op = read_3_double(data);
	} else {		/* single */
		reg_2_op = read_2_single(data);
		reg_3_op = read_3_single(data);
	}
	if ((opcode % 2) == 1)
		switch (opcode >> 4) {	/* multiplier */
		case 0:
			result = (reg_2_op) * (reg_3_op);
			break;
		case 1:
			result = fabs(reg_2_op) * (reg_3_op);
			break;
		case 2:
			result = (reg_2_op) * fabs(reg_3_op);
			break;
		case 3:
			result = fabs(reg_2_op) * fabs(reg_3_op);
			break;
		case 4:
			result = -(reg_2_op) * (reg_3_op);
			break;
		case 5:
			result = -fabs(reg_2_op) * (reg_3_op);
			break;
		case 6:
			result = -(reg_2_op) * fabs(reg_3_op);
			break;
		case 7:
			result = -fabs(reg_2_op) * fabs(reg_3_op);
			break;
		}
	else
		switch (opcode >> 1) {	/* alu */
		case 0:
			result = reg_2_op - reg_3_op;
			break;
		case 1:
			result = fabs(reg_2_op - reg_3_op);
			break;
		case 2:
			result = fabs(reg_2_op) - fabs(reg_3_op);
			break;
		case 3:
			result = reg_2_op / reg_3_op;
			break;
		case 4:
			result = -reg_2_op;
			break;
		case 8:
			result = reg_2_op + reg_3_op;
			break;
		case 9:
			result = fabs(reg_2_op + reg_3_op);
			break;
		case 10:
			result = fabs(reg_2_op) + fabs(reg_3_op);
			break;
		case 12:
			result = reg_2_op;
			break;
		case 14:
			result = fabs(reg_2_op);
			break;
		case 16:
			fpa_compare(reg_2_op, reg_3_op, pc);
			goto pastwrite;
		case 18:
			fpa_compare(fabs(reg_2_op), fabs(reg_3_op), pc);
			goto pastwrite;
		case 20:
			fpa_compare(reg_2_op, 0.0, pc);
			goto pastwrite;
		case 28:
			result = fpa_tolong(reg_2_op);
			write_long(data & 0x1f, result);
			goto pastwrite;
		case 29:
			sk.x = (float) read_2_single(data);
			result = sk.i[0];
			break;
		case 30:
			if (prec) {	/* double result */
				reg_2_op = read_2_single(data);
			} else {/* single result */
				reg_2_op = read_2_double(data);
			}
			result = (float) reg_2_op;
			break;
		default:
			found = 0;
			break;
		}
	if (found) {
		if (prec)
			write_double(data & 0x1f, result);
		else
			write_single(data & 0x1f, result);
	}
pastwrite:
	if (!found)
		fprintf(stderr, "\n Invalid result from failed FPA direct format instruction:\n data %X reg_2 %g reg_3 %g \n", data, reg_2_op, reg_3_op);
	return (found);
}

STATIC int
direct_single(a1, pc)
	struct fpa_access *a1;
	unsigned short *pc;

/* Procedure to recompute direct single format instructions. 	 */

{
	return (direct_double(a1, pc));
}

STATIC int
direct_double(a1, pc)
	struct fpa_access *a1;
	unsigned short *pc;

/* Procedure to recompute direct double format instructions. 	 */

{
	int             found, opcode, prec;
	struct fpa_access ac1;
	long            data;

	ac1 = *a1;
	opcode = (ac1.kluge.kint >> 3) & 0x3f;
	prec = ac1.kluge.kis.prec;
	data = ac1.data;

	found = direct_semantics(opcode, prec, data, pc);
	return (found);
}

STATIC int
command_semantics(opcode, prec, data)
	int             opcode, prec, data;
{
	complexunion	complex1, complex2, complex3;
	double          reg_2_op, reg_3_op, reg_4_op, result;
	double          r2_op[4], r3_op[4];
	int             found, rmax, ddata, i;
	double d1, d2;
	pdoublekluge pdk;

	found = 1;
	if (prec) {		/* double */
		reg_2_op = read_2_double(data);
		reg_3_op = read_3_double(data);
		reg_4_op = read_4_double(data);
	} else {		/* single */
		reg_2_op = read_2_single(data);
		reg_3_op = read_3_single(data);
		reg_4_op = read_4_single(data);
	}
	switch (opcode) {
	case 0:
	case 1:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		extended_precision();
		result = fpa_transcendental(opcode, reg_2_op);
		break;
	case 8: /* sqrt */
		result = fpa_transcendental(opcode, reg_2_op);
		break;
        case 9: /* hypot */
		extended_precision();
                pdk.pd[0] = (double *)&reg_3_op;
		pdk.pd[1] = (double *)&reg_2_op;
		result = fpa_transcendental(9, pdk.d);
                break;
        case 10: /* complex negation */
                complex3.d = reg_2_op;
                complex1.f[0] = -complex3.f[0];
                complex1.f[1] = -complex3.f[1];
                result = complex1.d;
                break;
        case 11: /* complex absolute value */
                extended_precision();
                complex3.d = reg_2_op;
                d1=complex3.f[0];
                d2=complex3.f[1];
                pdk.pd[0] = (double *)&d1;
                pdk.pd[1] = (double *)&d2;
		result = (double) (float) fpa_transcendental(9, pdk.d);
                write_single(data & 0x1f, result);
                goto pastwrite;
                break;
        case 12: /* complex addition */
                complex2.d = reg_2_op;
                complex3.d = reg_3_op;
                complex1.f[0] = complex2.f[0]+complex3.f[0];
                complex1.f[1] = complex2.f[1]+complex3.f[1];
                result = complex1.d;
                break;
        case 13: /* complex subtraction */
                complex2.d = reg_2_op;
                complex3.d = reg_3_op;
                complex1.f[0] = complex2.f[0]-complex3.f[0];
                complex1.f[1] = complex2.f[1]-complex3.f[1];
                result = complex1.d;
                break;
        case 14: /* complex multiplication */
                extended_precision();
                complex2.d = reg_2_op;
                complex3.d = reg_3_op;
                complex1.f[0] = complex2.f[0]*complex3.f[0]-complex2.f[1]*complex3.f[1];
                complex1.f[1] = complex2.f[0]*complex3.f[1]+complex2.f[1]*complex3.f[0];
                result = complex1.d;
                break;
        case 15: /* complex division */
                extended_precision();
                complex2.d = reg_2_op;
                complex3.d = reg_3_op;
                if (complex3.f[1] == 0)
			{ /* Special case division by real. */
			complex1.f[0]=complex2.f[0]/complex3.f[0];
			complex1.f[1]=complex2.f[1]/complex3.f[0];
			}
		else
			{ /* General case. */
			register double denominator = complex3.f[0] * complex3.f[0] + complex3.f[1] * complex3.f[1] ;
                complex1.f[0] = (complex2.f[0]*complex3.f[0]+complex2.f[1]*complex3.f[1])/denominator;
                complex1.f[1] = (complex2.f[1]*complex3.f[0]-complex2.f[0]*complex3.f[1])/denominator;
			}
                result = complex1.d;
                break;
	case 48:		/* sincos */
		extended_precision();
		result = fpa_transcendental(1, reg_2_op);	/* cos */
		if (prec)
			write_double((data >> 26) & 0x1f, result);
		else
			write_single((data >> 26) & 0x1f, result);
		result = fpa_transcendental(0, reg_2_op);	/* sin */
		break;
	case 17:
		extended_precision();
		result = reg_3_op + (reg_2_op * reg_4_op);
		break;
	case 18:
		extended_precision();
		result = reg_3_op - (reg_2_op * reg_4_op);
		break;
	case 19:
		extended_precision();
		result = (reg_2_op * reg_4_op) - reg_3_op;
		break;
	case 20:
		extended_precision();
		result = reg_3_op * (reg_2_op + reg_4_op);
		break;
	case 21:
		extended_precision();
		result = reg_3_op * (reg_2_op - reg_4_op);
		break;
	case 22:
		extended_precision();
		result = reg_3_op * (reg_4_op - reg_2_op);
		break;
	case 23:
		rmax = 1;
		goto vector;
	case 24:
		rmax = 2;
		goto vector;
	case 25:
		rmax = 3;
vector:
		extended_precision();
		r2_op[0] = reg_2_op;
		r3_op[0] = reg_3_op;
		ddata = data;
		for (i = 1; i <= rmax; i++) {
			ddata = data + 0x10040;
			/* increment reg_2 and reg_3 */
			if (prec) {	/* double */
				r2_op[i] = read_2_double(ddata);
				r3_op[i] = read_3_double(ddata);
			} else {/* single */
				r2_op[i] = read_2_single(ddata);
				r3_op[i] = read_3_single(ddata);
			}
		}
		switch (rmax) {	/* write out in extenso to get maximum
				 * benefit from extended precision evaluation */
		case 1:
			result = r2_op[0] * r3_op[0] +
				r2_op[1] * r3_op[1];
			break;
		case 2:
			result = r2_op[0] * r3_op[0] +
				r2_op[1] * r3_op[1] +
				r2_op[2] * r3_op[2];
			break;
		case 3:
			result = r2_op[0] * r3_op[0] +
				r2_op[1] * r3_op[1] +
				r2_op[2] * r3_op[2] +
				r2_op[3] * r3_op[3];
			break;
		}
		break;
	default:
		found = 0;
		break;
	}
	if (found) {
		if (prec)
			write_double(data & 0x1f, result);
		else
			write_single(data & 0x1f, result);
	}
pastwrite:
	if (!found)
		fprintf(stderr, "\n Invalid result from failed FPA command format instruction:\n data %X reg_2 %g reg_3 %g \n", data, reg_2_op, reg_3_op);
	return (found);
}

STATIC int
command_single(a1)
	struct fpa_access *a1;

/* Procedure to recompute command single format instructions. 	 */

{
	return (command_double(a1));
}

STATIC int
command_double(a1)
	struct fpa_access *a1;

/* Procedure to recompute command double format instructions. 	 */

{
	int             found, opcode, prec;
	struct fpa_access ac1;
	long            data;

	ac1 = *a1;
	opcode = (ac1.kluge.kint >> 3) & 0x3f;
	prec = ac1.kluge.kis.prec;
	data = ac1.data;

	found = command_semantics(opcode, prec, data);
	return (found);
}

#ifdef F68881
int
fpa_81comp(x, pc)
#endif
#ifdef FSOFT
	int             fpa_softcomp(x, pc)
#endif

	struct fpa_inst *x;
	unsigned short *pc;

/* Procedure to recompute an FPA instruction. */

{
	struct fpa_access *ap, ap0, ap1;
	int             i, opcode, found;

	ap0 = x->a[0];
	ap1 = x->a[1];
	opcode = ap0.kluge.kis.op;
	if (opcode >= 0x20) {	/* extended */
		if (ap0.kluge.kis.prec == 1)
			found = extended_double(&ap0, &ap1);
		else
			found = extended_single(&ap0, &ap1);
	} else if (opcode >= 0x14) {	/* direct */
		if (ap0.kluge.kis.prec == 1)
			found = direct_double(&ap0, pc);
		else
			found = direct_single(&ap0, pc);
	} else if (opcode >= 0x10) {	/* command */
		if (ap0.kluge.kis.prec == 1)
			found = command_double(&ap0);
		else
			found = command_single(&ap0);
	} else {
		if (ap0.kluge.kis.prec == 1)
			found = short_double(&ap0, &ap1, pc);
		else
			found = short_single(&ap0, pc);
	}
	if (!found)
		for (i = 0; i < 2;) {
			ap = (struct fpa_access *) & (x->a[i++]);
			if (ap->kluge.kis.valid == 0) {
				fprintf(stderr, " access %1d op %2x reg %2d prec %1d data %8X \n",
					i, ap->kluge.kis.op, ap->kluge.kis.reg1, ap->kluge.kis.prec,
					ap->data);
			}
		}
	return (found);
}
