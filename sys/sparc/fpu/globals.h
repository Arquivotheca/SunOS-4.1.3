/*	@(#)globals.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

	/*	Sparc floating-point simulator PRIVATE include file. */

#include <sys/types.h>
#include <vm/seg.h>

	/*	PRIVATE CONSTANTS	*/

/*	PRIVATE CONSTANTS	*/

#define	INTEGER_BIAS	   31
#define	SINGLE_BIAS	  127
#define	DOUBLE_BIAS	 1023
#define	EXTENDED_BIAS	16383

/* PRIVATE TYPES	*/

#ifdef DEBUG
#define	PRIVATE
#else
#define	PRIVATE static
#endif

#define	DOUBLE_E(n) (n & 0xfffe)	/* More significant word of double. */
#define	DOUBLE_F(n) (1+DOUBLE_E(n))	/* Less significant word of double. */
#define	EXTENDED_E(n) (n & 0xfffc) /* Sign/exponent/significand of extended. */
#define	EXTENDED_F(n) (1+EXTENDED_E(n)) /* 2nd word of extended significand. */
#define	EXTENDED_G(n) (2+EXTENDED_E(n)) /* 3rd word of extended significand. */
#define	EXTENDED_H(n) (3+EXTENDED_E(n)) /* 4th word of extended significand. */


typedef struct {
	int sign;
	enum fp_class_type fpclass;
	int	exponent;		/* Unbiased exponent	*/
	unsigned significand[4];	/* Four significand word . */
	int	rounded;		/* rounded bit */
	int	sticky;			/* stick bit */
} unpacked;

struct regs *fptraprp;			/* Users regs for syncfpu() */

/* PRIVATE FUNCTIONS */
extern void _fp_read_pfreg(/* pf, n */);
/*	FPU_REGS_TYPE *pf		Where to put current %fn. */
/*	unsigned n;			Want to read register n. */

extern void _fp_write_pfreg(/* pf, n */);
/*	FPU_REGS_TYPE *pf		Where to get new %fn. */
/*	unsigned n;			Want to read register n. */

/* vfreg routines use "virtual" FPU registers at *_fp_current_pfregs. */

extern void _fp_read_vfreg(/* pf, n, pfpsd */);
/*	FPU_REGS_TYPE *pf		Where to put current %fn. */
/*	unsigned n;			Want to read register n. */

extern void _fp_write_vfreg(/* pf, n, pfpsd */);
/*	FPU_REGS_TYPE *pf		Where to get new %fn. */
/*	unsigned n;			Want to read register n. */

extern enum ftt_type _fp_iu_simulator(/* pinst, pregs, pwindow, pfpu */);
/*	fp_inst_type	pinst;	  FPU instruction to simulate. */
/*	struct regs	*pregs;	  Pointer to PCB image of registers. */
/*	struct rwindow	*pwindow; Pointer to locals and ins. */
/*	struct fpu	*pfpu;	  Pointer to FPU register block. */

extern void _fp_unpack(/* pu, n, type */);
/*	unpacked	*pu;	unpacked result */
/*	unsigned	n;	register where data starts */
/*	fp_op_type	type;	type of datum */

extern void _fp_pack(/* pu, n, type */);
/*	unpacked	*pu;	unpacked result */
/*	unsigned	n;	register where data starts */
/*	fp_op_type	type;	type of datum */

extern void _fp_unpack_word(/* pu, n */);
/*	unsigned	*pu;	unpacked result */
/*	unsigned	n;	register where data starts */

extern void _fp_pack_word(/* pu, n, type */);
/*	unsigned	*pu;	unpacked result */
/*	unsigned	n;	register where data starts */

extern void fpu_normalize(/* pu */);
/*	unpacked	*pu;	unpacked operand and result */

extern void fpu_rightshift(/* pu, n */);
/*	unpacked *pu; unsigned n;	*/
/*	Right shift significand sticky by n bits. */

extern void fpu_set_exception(/* ex */);
/*	enum fp_exception_type ex;	exception to be set in curexcep */

extern void fpu_error_nan(/* pu */);
/*	unpacked *pu; 		Set invalid exception and error nan in *pu */

extern void unpacksingle(/* pfpsd, pu, x */);
/* 	fp_simd_type	*pfpsd;	simulator data */
/*	unpacked	*pu;	unpacked result */
/*	single_type	x;	packed single */

extern void unpackdouble(/* pfpsd, pu, x, y */);
/* 	fp_simd_type	*pfpsd;	simulator data */
/*	unpacked	*pu;	unpacked result */
/*	double_type	x;	packed double */
/*	unsigned	y;	*/


extern enum fcc_type _fp_compare(/* px, py */);

extern void _fp_add(/* pfpsd, px, py, pz */);
extern void _fp_sub(/* pfpsd, px, py, pz */);
extern void _fp_mul(/* pfpsd, px, py, pz */);
extern void _fp_div(/* pfpsd, px, py, pz */);
extern void _fp_sqrt(/* pfpsd, px, pz */);

extern enum ftt_type	_fp_write_word(/* caddr_t, value */);
extern enum ftt_type	_fp_read_word(/* caddr_t, pvalue */);
extern enum ftt_type	read_iureg(/* n, pregs, pwindow, pvalue */);


