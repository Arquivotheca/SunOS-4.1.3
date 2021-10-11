/******************************************************************************
	@(#)fpa3x_def.h - Rev 1.1 - 7/30/92
	Copyright (c) 1988 by Sun Microsystems, Inc.
	Deyoung Hong

	FPA-3X macros header file.
******************************************************************************/


/*
 * FPA-3X misc definitions.
 */
#define	DEVICE_NAME	"fpa"		/* default device name */
#define	FPA3X_VERSION	2		/* FPA-3X version is 001m */
#define	FPA3X_BASE	0xE0000000	/* FPA-3X base address */
#define	FPA3X_ILLADDR	0xE0000D00	/* illegal address */
#define	FPA3X_ILLDIAG	0xE0000E40	/* illegal address (diag map only) */
#define	FPA3X_ILLCTRL	0xE0000F00	/* illegal control register */
#define	FPA_NSHORT_REGS	16		/* no. of short operational registers */

#define FPERR		-2		/* exit_code for fpa3x */


/*
 * FPA-3X general instructions.
 */
#define INS_UNIMPLEMENT	0xE000095C	/* unimplement */
#define	INS_LOADRF_M	0xE0000E80	/* write RegFile via loadptr */
#define	INS_LOADRF_L	0xE0000E84
#define	INS_LOOPCNTR	0xE0000968	/* diagnostic loop counter test */


/*
 * FPA-3X single precision short instructions.
 */
#define	SP_NOP		0xE0000000		/* nop */
#define	SP_NEGATE	0xE0000080		/* r1 = -operand */
#define	SP_ABS		0xE0000100		/* r1 = |operand| */
#define	SP_FLOAT	0xE0000180		/* r1 = float operand */
#define	SP_FIX		0xE0000200		/* r1 = fixed operand */
#define	SP_TODOUBLE	0xE0000280		/* r1 = converted operand */
#define	SP_SQUARE	0xE0000300		/* r1 = operand * operand  */
#define	SP_ADD		0xE0000380		/* r1 = r1 + operand */
#define	SP_SUB		0xE0000400		/* r1 = r1 - operand */
#define	SP_MULT		0xE0000480		/* r1 = r1 * operand */
#define	SP_DIVIDE	0xE0000500		/* r1 = r1 / operand */
#define	SP_RSUB		0xE0000580		/* r1 = operand - r1 */
#define	SP_RDIVIDE	0xE0000600		/* r1 = operand / r1 */

#define	SP_REG1(x)	(x<<3)			/* reg1 instruction field */


/*
 * FPA-3X double precision short instructions.
 */
#define	DP_NOP		0xE0000004		/* nop */
#define	DP_NEGATE	0xE0000084		/* r1 = -operand */
#define	DP_ABS		0xE0000104		/* r1 = |operand| */
#define	DP_FLOAT	0xE0000184		/* r1 = float operand */
#define	DP_FIX		0xE0000204		/* r1 = fixed operand */
#define	DP_TOSINGLE	0xE0000284		/* r1 = converted operand */
#define	DP_SQUARE	0xE0000304		/* r1 = operand * operand  */
#define	DP_ADD		0xE0000384		/* r1 = r1 + operand */
#define	DP_SUB		0xE0000404		/* r1 = r1 - operand */
#define	DP_MULT		0xE0000484		/* r1 = r1 * operand */
#define	DP_DIVIDE	0xE0000504		/* r1 = r1 / operand */
#define	DP_RSUB		0xE0000584		/* r1 = operand - r1 */
#define	DP_RDIVIDE	0xE0000604		/* r1 = operand / r1 */

#define	DP_LSW		0xE0001000		/* lsw of all dp instructions */
#define	DP_REG1(x)	(x<<3)			/* reg1 instruction field  */


/*
 * FPA-3X extended instructions.
 */
#define	XSP_R1_O2aR2xO	0xE0001000		/* sp, r1 = op2 + (r2 * op) */
#define	XSP_R1_O2sR2xO	0xE0001080		/* sp, r1 = op2 - (r2 * op) */
#define	XSP_R1_mO2aR2xO	0xE0001100		/* sp, r1 = -op2 + (r2 * op) */
#define	XSP_R1_O2xR2aO	0xE0001180		/* sp, r1 = op2 * (r2 + op) */
#define	XSP_R1_O2xR2sO	0xE0001200		/* sp, r1 = op2 * (r2 - op) */
#define	XSP_R1_O2xmR2aO	0xE0001280		/* sp, r1 = op2 * (-r2 + op) */
#define	XSP_ADD		0xE0001300		/* sp, r1 = r2 + operand */
#define	XSP_SUB		0xE0001380		/* sp, r1 = r2 - operand */
#define	XSP_MULT	0xE0001400		/* sp, r1 = r2 * operand */
#define	XSP_DIVIDE	0xE0001480		/* sp, r1 = r2 / operand */
#define	XSP_RSUB	0xE0001500		/* sp, r1 = operand - r2 */
#define	XSP_RDIVIDE	0xE0001580		/* sp, r1 = operand / r2 */

#define	XDP_ADD		0xE0001304		/* dp, r1 = r2 + operand */
#define	XDP_SUB		0xE0001384		/* dp, r1 = r2 - operand */
#define	XDP_MULT	0xE0001404		/* dp, r1 = r2 * operand */
#define	XDP_DIVIDE	0xE0001484		/* dp, r1 = r2 / operand */
#define	XDP_RSUB	0xE0001504		/* dp, r1 = operand - r2 */
#define	XDP_RDIVIDE	0xE0001584		/* dp, r1 = operand / r2 */
#define	XDP_R1_R3aR2xO	0xE0001804		/* dp, r1 = r3 + (r2 * op) */
#define	XDP_R1_R3sR2xO	0xE0001884		/* dp, r1 = r3 - (r2 * op) */
#define	XDP_R1_mR3aR2xO	0xE0001904		/* dp, r1 = -r3 + (r2 * op) */
#define	XDP_R1_R3xR2aO	0xE0001984		/* dp, r1 = r3 * (r2 + op) */
#define	XDP_R1_R3xR2sO	0xE0001A04		/* dp, r1 = r3 * (r2 - op) */
#define	XDP_R1_R3xmR2aO	0xE0001A84		/* dp, r1 = r3 * (-r2 + op) */
#define	XDP_R1_OaR3xR2	0xE0001B04		/* dp, r1 = op + (r3 * r2) */
#define	XDP_R1_OsR3xR2	0xE0001B84		/* dp, r1 = op - (r3 * r2) */
#define	XDP_R1_mOaR3xR2	0xE0001C04		/* dp, r1 = -op + (r3 * r2) */
#define	XDP_R1_OxR3aR2	0xE0001C84		/* dp, r1 = op * (r3 + r2) */
#define	XDP_R1_OxR3sR2	0xE0001D04		/* dp, r1 = op * (r3 - r2) */

#define	X_REG1(x)	(x<<3)			/* reg1 instruction field  */
#define	X_REG2(x)	(x<<3)			/* reg2 instruction field  */
#define	X_REG3(x)	(x<<7)			/* reg3 instruction field  */
#define	X_LSW		0xE0001800		/* lsw of all extended inst. */


/*
 * FPA-3X command register instructions.
 */
#define	CSP_SINE	0xE0000800	/* sp, r1 = sine(r2) */
#define	CSP_ARCTAN	0xE0000818	/* sp, r1 = arctan(r2) */
#define	CSP_R1_R2	0xE0000880	/* sp, r1 = r2 */
#define	CSP_R1_R3aR2xR4	0xE0000888	/* sp, r1 = r3 + (r2 * r4) */
#define	CSP_R1_R3sR2xR4	0xE0000890	/* sp, r1 = r3 - (r2 * r4) */
#define	CSP_R1_mR3aR2xR4 0xE0000898	/* sp, r1 = -r3 + (r2 * r4) */
#define	CSP_R1_R3xR2aR4	0xE00008A0	/* sp, r1 = r3 * (r2 + r4) */
#define	CSP_R1_R3xR2sR4	0xE00008A8	/* sp, r1 = r3 * (r2 - r4) */
#define	CSP_R1_R3xmR2aR4 0xE00008B0	/* sp, r1 = r3 * (-r2 + r4) */
#define	CSP_SINCOS	0xE0000980	/* sp, sine and cosine */
#define	CSP_TRANS4x4	0xE00009B0	/* sp, transpose 4x4 matrix */

#define	CDP_R1_R2	0xE0000884	/* dp, r1 = r2 */
#define	CDP_R1_R3aR2xR4	0xE000088C	/* dp, r1 = r3 + (r2 * r4) */
#define	CDP_R1_R3sR2xR4	0xE0000894	/* dp, r1 = r3 - (r2 * r4) */
#define	CDP_R1_mR3aR2xR4 0xE000089C	/* dp, r1 = -r3 + (r2 * r4) */
#define	CDP_R1_R3xR2aR4	0xE00008A4	/* dp, r1 = r3 * (r2 + r4) */
#define	CDP_R1_R3xR2sR4	0xE00008AC	/* dp, r1 = r3 * (r2 - r4) */
#define	CDP_R1_R3xmR2aR4 0xE00008B4	/* dp, r1 = r3 * (-r2 + r4) */
#define	CDP_SINCOS	0xE0000984	/* dp, sine and cosine */
#define	CDP_R1_eR2s1	0xE0000824	/* dp, r1 = e ^ r2 - 1 */
#define	CDP_DOTPROD	0xE00008CC	/* dp, dot product */

#define	CWSP_ADD	0xE0000A80	/* Weitek sp, r1 = r2 + r3 */
#define	CWSP_MULT	0xE0000A08	/* Weitek sp, r1 = r2 * r3 */
#define	CWSP_DIVIDE	0xE0000A30	/* Weitek sp, r1 = r2 / r3 */

#define	CWDP_ADD	0xE0000A84	/* Weitek dp, r1 = r2 + r3 */
#define	CWDP_MULT	0xE0000A0C	/* Weitek sp, r1 = r2 * r3 */
#define	CWDP_DIVIDE	0xE0000A34	/* Weitek dp, r1 = r2 / r3 */

#define	C_REG1(x)	(x)		/* reg1 data field  */
#define	C_REG2(x)	(x<<6)		/* reg2 data field  */
#define	C_REG3(x)	(x<<16)		/* reg3 data field  */
#define	C_REG4(x)	(x<<26)		/* reg4 data field  */


/*
 * FPA-3X Weitek expected status.
 */
#define	WSTAT0		0xA004		/* expected Weitek status 0 */
#define	WSTAT1		0xA119		/* expected Weitek status 1 */
#define	WSTAT2		0xA200		/* expected Weitek status 2 */
#define	WSTAT3		0x2300		/* expected Weitek status 3 */
#define	WSTAT4		0x2400		/* expected Weitek status 4 */
#define	WSTAT5		0x2500		/* expected Weitek status 5 */
#define	WSTAT6		0x2600		/* expected Weitek status 6 */
#define	WSTAT7		0x2700		/* expected Weitek status 7 */
#define	WSTAT8		0x2800		/* expected Weitek status 8 */
#define	WSTAT9		0x2900		/* expected Weitek status 9 */
#define	WSTATA		0x2A00		/* expected Weitek status 10 */
#define	WSTATB		0x2B00		/* expected Weitek status 11 */
#define	WSTATC		0x2C00		/* expected Weitek status 12 */
#define	WSTATD		0x2D00		/* expected Weitek status 13 */
#define	WSTATE		0x2E00		/* expected Weitek status 14 */
#define	WSTATF		0x2F02		/* expected Weitek status 15 */

#define	WS_COND		0x300		/* Weitek specific condition */
#define	WS_ERROR	0x8000		/* Weitek error status */


/*
 * TI status taken from Weitek.
 */
#define	TI_ZERO		0x000		/* zero result */
#define	TI_INFIN	0x100		/* infinity result */
#define	TI_FINEX	0x200		/* finite exact result */
#define	TI_FININ	0x300		/* finite inexact result */
#define	TI_UNUSED	0x400		/* not used */
#define	TI_OVFIN	0x500		/* overflow inexact result */
#define	TI_UNDFEX	0x600		/* underflow exact result */
#define	TI_UNDFIN	0x700		/* underflow inexact result */
#define	TI_ADNORM	0x800		/* op A denormalized */
#define	TI_BDNORM	0x900		/* op B denormalized */
#define	TI_ABDNORM	0xA00		/* op A and B denormalized */
#define	TI_DIVZERO	0xB00		/* divide by zero */
#define	TI_ANAN		0xC00		/* op A is NaN */
#define	TI_BNAN		0xD00		/* op B is NaN */
#define	TI_ABNAN	0xE00		/* op A and B are NaN */
#define	TI_INVAL	0xF00		/* invalid operation */


/*
 * FPA-3X register mask values
 */
#define	CONTEXT_MASK	0x1F		/* context mask 0-31 */
#define	IERR_MASK	0xFF0000	/* IERR register mask */
#define	PIPESTAT_MASK	0xFF0000	/* pipe status mask */
#define	IMASK_MASK	0xF		/* IMASK register mask */
#define	MODE_MASK	0xF		/* MODE register mask */
#define	LOADPTR_MASK	0xFFFF		/* LOAD_PTR register mask */
#define	WSTATUS_MASK	0xEF1F		/* WSTATUS register mask */
#define	IERR_SHIFT	16		/* IERR shift value */
#define	WSTATUS_SHIFT	8		/* WSTATUS shift value */
#define	TISTAT_MASK	0xF00		/* TI status mask */


/*
 * FPA-3X constant RAM values.
 */
#define	SMONE		0xBF800000	/* sp integer -1 */
#define	SZERO		0x00000000	/* sp integer 0 */
#define	SONE		0x3F800000	/* sp integer 1 */
#define	STWO		0x40000000	/* sp integer 2 */
#define	STHREE		0x40400000	/* sp integer 3 */
#define	SFOUR		0x40800000	/* sp integer 4 */
#define	SFIVE		0x40A00000	/* sp integer 6 */
#define	SSIX		0x40C00000	/* sp integer 6 */
#define	SEIGHT		0x41000000	/* sp integer 8 */
#define	SNINE		0x41100000	/* sp integer 9 */
#define	STEN		0x41200000	/* sp integer 10 */
#define	SSIXTEEN	0x41800000	/* sp integer 16 */
#define	STHIRTY2	0x42000000	/* sp integer 32 */
#define	SHALF		0x3F000000	/* sp one half */
#define	SPI		0x40490FDB	/* sp pi value */
#define	SPIO4		0x3F490FDB	/* sp pi/4 value */
#define	SINF		0x7F800000	/* sp infinity value */
#define	SSNAN		0x7FBFFFFF	/* sp signaling NaN */
#define	SMINSUB		0x00000001	/* sp min subnormal value */
#define	SMAXSUB		0x007FFFFF	/* sp max subnormal value */
#define	SMINNORM	0x00800000	/* sp min normal value */
#define	SMAXNORM	0x7F7FFFFF	/* sp max normal value */
#define	SMIN1		0x00800001	/* sp min 1 value */

#define	DMONE_M		0xBFF00000	/* dp integer -1 */
#define	DMONE_L		0x00000000
#define	DZERO_M		0x00000000	/* dp integer 0 */
#define	DZERO_L		0x00000000
#define	DONE_M		0x3FF00000	/* dp integer 1 */
#define	DONE_L		0x00000000
#define	DTWO_M		0x40000000	/* dp integer 2 */
#define	DTWO_L		0x00000000
#define	DTHREE_M	0x40080000	/* dp integer 3 */
#define	DTHREE_L	0x00000000
#define	DFOUR_M		0x40100000	/* dp integer 4 */
#define	DFOUR_L		0x00000000
#define	DFIVE_M		0x40140000	/* dp integer 5 */
#define	DFIVE_L		0x00000000
#define	DSIX_M		0x40180000	/* dp integer 6 */
#define	DSIX_L		0x00000000
#define	DSEVEN_M	0x401C0000	/* dp integer 7 */
#define	DSEVEN_L	0x00000000
#define	DEIGHT_M	0x40200000	/* dp integer 8 */
#define	DEIGHT_L	0x00000000
#define	DNINE_M		0x40220000	/* dp integer 9 */
#define	DNINE_L		0x00000000
#define	DTEN_M		0x40240000	/* dp integer 10 */
#define	DTEN_L		0x00000000
#define	DSIXTEEN_M	0x40300000	/* dp integer 16 */
#define	DSIXTEEN_L	0x00000000
#define	DTHIRTY2_M	0x40400000	/* dp integer 32 */
#define	DTHIRTY2_L	0x00000000
#define	DFORTY_M	0x40440000	/* dp integer 40 */
#define	DFORTY_L	0x00000000
#define	DHALF_M		0x3FE00000	/* sp one half */
#define	DHALF_L		0x00000000
#define	DPI_M		0x400921FB	/* dp pi value */
#define	DPI_L		0x54442D18
#define	DPIO4_M		0x3FE921FB	/* dp pi/4 value */
#define	DPIO4_L		0x54442D18
#define	DINF_M		0x7FF00000	/* dp infinity value */
#define	DINF_L		0x00000000
#define	DSNAN_M		0x7FF7FFFF	/* dp signaling NaN */
#define	DSNAN_L		0xFFFFFFFF
#define	DMINSUB_M	0x00000000	/* dp min subnormal value */
#define	DMINSUB_L	0x00000001
#define	DMAXSUB_M	0x000FFFFF	/* dp max subnormal value */
#define	DMAXSUB_L	0xFFFFFFFF
#define	DMINNORM_M	0x00100000	/* dp min normal value */
#define	DMINNORM_L	0x00000000
#define	DMAXNORM_M	0x7FEFFFFF	/* dp max normal value */
#define	DMAXNORM_L	0xFFFFFFFF
#define	DMIN1_M		0x00100001	/* dp min 1 value */
#define	DMIN1_L		0x00010001

#define	SVAL1		0x01000000	/* test value 1 */
#define	SINE_SVAL1	0x01000000	/* sine of test value 1 */
#define	SVAL2		0x81000000	/* test value 2 */
#define	SINE_SVAL2	0x81000000	/* sine of test value 2 */
#define	SVAL3		0x70000000	/* test value 3 */
#define	ATAN_SVAL3	0x3FC90FDB	/* arctan of test value 3 */
#define	SVAL4		0xF0000000	/* test value 4 */
#define	ATAN_SVAL4	0xBFC90FDB	/* arctan of test value 4 */
#define	SINE_SONE	0x3F576AA4	/* sine 1 */
#define	SINE_SMONE	0xBF576AA4	/* sine -1 */
#define	ATAN_STWO	0x3F8DB70D	/* arctan 2 */

#define	DEXP32S1_M	0x42D1F43F	/* e^32 - 1 */
#define	DEXP32S1_L	0xCC4B65EC
#define	DEXP16S1_M	0x4160F2EB	/* e^16 - 1 */
#define	DEXP16S1_L	0xB0A80020
#define	DEXP8S1_M	0x40A747EA	/* e^8 - 1 */
#define	DEXP8S1_L	0x7D470C6E
#define	DEXP4S1_M	0x404ACC90	/* e^4 - 1 */
#define	DEXP4S1_L	0x2E273A58


/*
 * IEEE floating point format value.
 */
#define	S_FBITS		23		/* f bit field in IEEE single format */
#define	D_FBITS		52		/* f bit field in IEEE double format */
#define	S_EMAX		0xFF		/* e max value in IEEE single format */
#define	D_EMAX		0x7FF		/* e max value in IEEE double format */
#define	S_SIGNBIT	31		/* s bit in IEEE single format */
#define	D_SIGNBIT	63		/* s bit in IEEE double format */


/*
 * Misc macros.
 */
#define	MAXSTRING	256		/* max string */
#define	CLEAR		0		/* clear condition */
#define	ON		1		/* on */
#define	OFF		0		/* off */
#define	TRUE		1		/* true */
#define	FALSE		0		/* false */
#define	WORD_MAX	0xFFFF		/* max of 16-bit  value */
#define	BYTE_MAX	0xFF		/* max of 8-bit  value */
#define	NIBBLE_MAX	0xF		/* max of 4-bit  value */
#define	SPMARGIN	0.0000001	/* single precision margin */
#define	DPMARGIN	0.000000000000001	/* double precision margin */
#define	X		0		/* don't care */
#define	pi		3.141592654	/* value of pi */


