/*	@(#)fpatest.c 1.1 92/07/30 SMI	*/
#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "fpa.h"
#include "values.h"
/*	History:
	1/24/86		Mark Moyer	Test Written
	9/21/87		Mark Moyer	sincos testing added
 */
/*    The main purpose of this test is to verify the microcode.
	Other tests were added so that this test may be a more complete
	general FPA test, but the emphasis is definitely on 1) verifying
	the microcode design and implementation, and 2) providing a
	regression test for all microcode modifications.
 */
/*	some more constants */
#define sonezp1 0x3f800001
#define donezp1 0x3ff00000,0x00000001
#define sonezm1 0x3f7fffff
#define donezm1 0x3fefffff,0xffffffff
#define sonezp2 0x3f800002
#define donezp2 0x3ff00000,0x00000002
#define sonezm2 0x3f7ffffe
#define donezm2 0x3fefffff,0xfffffffe
/*	second half of extended operation */
#define X_2ND 0xE0001800
/*	write weitek status */
#define WRITE_WEITEK_STATUS 0xE0000E40
/*	initialize (load the Weitek mode registers */
#define INIT 0xE0000978
/*	test user registers, destructive & non-destructive */
#define REG_TEST_D 0xE00009C8
#define REG_TEST_N 0xE00009CC
/*	register operations */
#define REG_CPY_DP 0xE0000884
#define REG_CPY_SP 0xE0000880
#define PUT_REG_SP 0xE0000C00
#define PUT_REG_MS 0xE0000C00
#define PUT_REG_LS 0xE0000C04
#define GET_REG_SP 0xE0000C00
#define GET_REG_MS 0xE0000C00
#define GET_REG_LS 0xE0000C04
/*	load pointer register RAM operations */
#define LDP_MS 0xE0000E80
#define LDP_LS 0xE0000E84
/*	single precision operations */
#define SP_NOP 0xE0000000
#define SP_NEG 0xE0000080
#define SP_ABS 0xE0000100
#define SP_FLT 0xE0000180
#define SP_FIX 0xE0000200
#define SP_CNV 0xE0000280
#define SP_SQR 0xE0000300
#define SP_ADD 0xE0000380
#define SP_SUB 0xE0000400
#define SP_MUL 0xE0000480
#define SP_DIV 0xE0000500
#define SP_BSUB 0xE0000580
#define SP_BDIV 0xE0000600
#define SP_CMP0 0xE0000680
#define SP_CMPR 0xE0000700
#define SP_CMPM 0xE0000780
#define DP_NOP 0xE0000004,0xE0001000
#define DP_NEG 0xE0000084,0xE0001000
#define DP_ABS 0xE0000104,0xE0001000
#define DP_FLT 0xE0000184,0xE0001000
#define DP_FIX 0xE0000204,0xE0001000
#define DP_CNV 0xE0000284,0xE0001000
#define DP_SQR 0xE0000304,0xE0001000
#define DP_ADD 0xE0000384,0xE0001000
#define DP_SUB 0xE0000404,0xE0001000
#define DP_MUL 0xE0000484,0xE0001000
#define DP_DIV 0xE0000504,0xE0001000
#define DP_BSUB 0xE0000584,0xE0001000
#define DP_BDIV 0xE0000604,0xE0001000
#define DP_CMP0 0xE0000684,0xE0001000
#define DP_CMPR 0xE0000704,0xE0001000
#define DP_CMPM 0xE0000784,0xE0001000
#define XS_SIN 0xE0001000
#define XD_SIN 0xE0001004
#define XS_COS 0xE0001080
#define XD_COS 0xE0001084
#define XS_TAN 0xE0001100
#define XD_TAN 0xE0001104
#define XS_ATAN 0xE0001180
#define XD_ATAN 0xE0001184
#define XS_EXPM 0xE0001200
#define XD_EXPM 0xE0001204
#define XS_LNP 0xE0001280
#define XD_LNP 0xE0001284
#define XS_EXP 0xE0001300
#define XD_EXP 0xE0001304
#define XS_LN 0xE0001380
#define XD_LN 0xE0001384
#define XS_ADD 0xE0001300
#define XD_ADD 0xE0001304
#define XS_SUB 0xE0001380
#define XD_SUB 0xE0001384
#define XS_MUL 0xE0001400
#define XD_MUL 0xE0001404
#define XS_DIV 0xE0001480
#define XD_DIV 0xE0001484
#define XS_BSUB 0xE0001500
#define XD_BSUB 0xE0001504
#define XS_BDIV 0xE0001580
#define XD_BDIV 0xE0001584
#define XS_PIV0 0xE0001800
#define XD_PIV0 0xE0001804
#define XS_PIV1 0xE0001880
#define XD_PIV1 0xE0001884
#define XS_PIV2 0xE0001900
#define XD_PIV2 0xE0001904
#define XS_PIV3 0xE0001980
#define XD_PIV3 0xE0001984
#define XS_PIV4 0xE0001A00
#define XD_PIV4 0xE0001A04
#define XS_PIV5 0xE0001A80
#define XD_PIV5 0xE0001A84
#define XS_PIV6 0xE0001B00
#define XD_PIV6 0xE0001B04
#define XS_PIV7 0xE0001B80
#define XD_PIV7 0xE0001B84
#define XS_PIV8 0xE0001C00
#define XD_PIV8 0xE0001C04
#define XS_PIV9 0xE0001C80
#define XD_PIV9 0xE0001C84
#define XS_PIVA 0xE0001D00
#define XD_PIVA 0xE0001D04
#define XS_LIN0 0xE0001000
#define XS_LIN1 0xE0001080
#define XS_LIN2 0xE0001100
#define XS_LIN3 0xE0001180
#define XS_LIN4 0xE0001200
#define XS_LIN5 0xE0001280
#define CS_SIN 0xE0000800
#define CD_SIN 0xE0000804
#define CS_COS 0xE0000808
#define CD_COS 0xE000080C
#define CS_TAN 0xE0000810
#define CD_TAN 0xE0000814
#define CS_ATAN 0xE0000818
#define CD_ATAN 0xE000081C
#define CS_EXP1 0xE0000820
#define CD_EXP1 0xE0000824
#define CS_LN1 0xE0000828
#define CD_LN1 0xE000082C
#define CS_EXP 0xE0000830
#define CD_EXP 0xE0000834
#define CS_LN 0xE0000838
#define CD_LN 0xE000083C
#define CS_SQRT 0xE0000840
#define CD_SQRT 0xE0000844
#define CS_SINCOS 0xE0000980
#define CD_SINCOS 0xE0000984
#define CS_CPY 0xE0000880
#define CD_CPY 0xE0000884
#define CS_PIV1 0xE0000888
#define CD_PIV1 0xE000088C
#define CS_PIV2 0xE0000890
#define CD_PIV2 0xE0000894
#define CS_PIV3 0xE0000898
#define CD_PIV3 0xE000089C
#define CS_PIV4 0xE00008A0
#define CD_PIV4 0xE00008A4
#define CS_PIV5 0xE00008A8
#define CD_PIV5 0xE00008AC
#define CS_PIV6 0xE00008B0
#define CD_PIV6 0xE00008B4
#define CS_ALU00 0xE0000A00
#define CD_ALU00 0xE0000A04
#define CS_ALU01 0xE0000A10
#define CD_ALU01 0xE0000A14
#define CS_ALU02 0xE0000A20
#define CD_ALU02 0xE0000A24
#define CS_ALU03 0xE0000A30
#define CD_ALU03 0xE0000A34
#define CS_ALU04 0xE0000A40
#define CD_ALU04 0xE0000A44
#define CS_ALU05 0xE0000A50
#define CD_ALU05 0xE0000A54
#define CS_ALU06 0xE0000A60
#define CD_ALU06 0xE0000A64
#define CS_ALU07 0xE0000A70
#define CD_ALU07 0xE0000A74
#define CS_ALU08 0xE0000A80
#define CD_ALU08 0xE0000A84
#define CS_ALU09 0xE0000A90
#define CD_ALU09 0xE0000A94
#define CS_ALU0A 0xE0000AA0
#define CD_ALU0A 0xE0000AA4
#define CS_ALU0B 0xE0000AB0
#define CD_ALU0B 0xE0000AB4
#define CS_ALU0C 0xE0000AC0
#define CD_ALU0C 0xE0000AC4
#define CS_ALU0D 0xE0000AD0
#define CD_ALU0D 0xE0000AD4
#define CS_ALU0E 0xE0000AE0
#define CD_ALU0E 0xE0000AE4
#define CS_ALU0F 0xE0000AF0
#define CD_ALU0F 0xE0000AF4
#define CS_ALU10 0xE0000B00
#define CD_ALU10 0xE0000B04
#define CS_ALU11 0xE0000B10
#define CD_ALU11 0xE0000B14
#define CS_ALU12 0xE0000B20
#define CD_ALU12 0xE0000B24
#define CS_ALU13 0xE0000B30
#define CD_ALU13 0xE0000B34
#define CS_ALU14 0xE0000B40
#define CD_ALU14 0xE0000B44
#define CS_ALU15 0xE0000B50
#define CD_ALU15 0xE0000B54
#define CS_ALU16 0xE0000B60
#define CD_ALU16 0xE0000B64
#define CS_ALU17 0xE0000B70
#define CD_ALU17 0xE0000B74
#define CS_ALU18 0xE0000B80
#define CD_ALU18 0xE0000B84
#define CS_ALU19 0xE0000B90
#define CD_ALU19 0xE0000B94
#define CS_ALU1A 0xE0000BA0
#define CD_ALU1A 0xE0000BA4
#define CS_ALU1B 0xE0000BB0
#define CD_ALU1B 0xE0000BB4
#define CS_ALU1C 0xE0000BC0
#define CD_ALU1C 0xE0000BC4
#define CS_ALU1D 0xE0000BD0
#define CD_ALU1D 0xE0000BD4
#define CS_ALU1E 0xE0000BE0
#define CD_ALU1E 0xE0000BE4
#define CS_ALU1F 0xE0000BF0
#define CD_ALU1F 0xE0000BF4
#define CS_2DOT 0xE00008B8
#define CD_2DOT 0xE00008BC
#define CS_3DOT 0xE00008C0
#define CD_3DOT 0xE00008C4
#define CS_4DOT 0xE00008C8
#define CD_4DOT 0xE00008CC
#define CS_2MOV 0xE0000988
#define CD_2MOV 0xE000098C
#define CS_3MOV 0xE0000990
#define CD_3MOV 0xE0000994
#define CS_4MOV 0xE0000998
#define CD_4MOV 0xE000099C
#define CS_2TRN 0xE00009A0
#define CD_2TRN 0xE00009A4
#define CS_3TRN 0xE00009A8
#define CD_3TRN 0xE00009AC
#define CS_4TRN 0xE00009B0
#define CD_4TRN 0xE00009B4
/*	status codes */
#define ST_DONTCARE -1
#define ST_EQUAL (0x0 << FPA_STATUS_SHIFT)
#define ST_LESSTHAN (0x1 << FPA_STATUS_SHIFT)
#define ST_GREATERTHAN (0x2 << FPA_STATUS_SHIFT)
#define ST_UNORDERED (0x3 << FPA_STATUS_SHIFT)
#define ST_ZERO (0x0 << FPA_STATUS_SHIFT)
#define ST_INFINITY (0x1 << FPA_STATUS_SHIFT)
#define ST_FINITEEXACT (0x2 << FPA_STATUS_SHIFT)
#define ST_FINITEINEXACT (0x3 << FPA_STATUS_SHIFT)
#define ST_OUTOFBOUNDS (0x4 << FPA_STATUS_SHIFT)
#define ST_OVERFLOWINEXACT (0x5 << FPA_STATUS_SHIFT)
#define ST_UNDERFLOW (0x6 << FPA_STATUS_SHIFT)
#define ST_UNDERFLOWINEXACT (0x7 << FPA_STATUS_SHIFT)
#define ST_ADNRM (0x8 << FPA_STATUS_SHIFT)
#define ST_BDNRM (0x9 << FPA_STATUS_SHIFT)
#define ST_ABDNRM (0xA << FPA_STATUS_SHIFT)
#define ST_DIVBY0 (0xB << FPA_STATUS_SHIFT)
#define ST_ANAN (0xC << FPA_STATUS_SHIFT)
#define ST_BNAN (0xD << FPA_STATUS_SHIFT)
#define ST_ABNAN (0xE << FPA_STATUS_SHIFT)
#define ST_INVALIDOP (0xF << FPA_STATUS_SHIFT)
struct op1_table
{
	char *name;
	long r1;
	long data;
	long result;
	int op;
	long status;
};
struct op1_table sp_table[] =
{
	"nop", 0x0, 0x0, 0x0, SP_NOP, ST_DONTCARE,
	"nop", ssnan, 0x0, ssnan, SP_NOP, ST_DONTCARE,
	"negate", 0x0, sone, smone, SP_NEG, ST_FINITEEXACT,
	"negate", 0x0, smone, sone, SP_NEG, ST_FINITEEXACT,
	"negate", 0x0, ssnan, sone, SP_NEG, ST_ANAN,
	"absolute value", 0x0, smone, sone, SP_ABS, ST_FINITEEXACT,
	"absolute value", 0x0, sone, sone, SP_ABS, ST_FINITEEXACT,
	"absolute value", 0x0, ssnan, sone, SP_ABS, ST_ANAN,
	"float", 0x0, 1, sone, SP_FLT, ST_FINITEEXACT,
	"float", 0x0, -1, smone, SP_FLT, ST_FINITEEXACT,
	"float", 0x0, 4, sfour, SP_FLT, ST_FINITEEXACT,
	"fix", 0x0, sone, 1, SP_FIX, ST_FINITEEXACT,
	"fix", 0x0, smone, -1, SP_FIX, ST_FINITEEXACT,
	"fix", 0x0, seight, 8, SP_FIX, ST_FINITEEXACT,
	"fix", 0x0, spio2, 1, SP_FIX, ST_FINITEINEXACT,
	"fix", 0x0, ssnan, 2, SP_FIX, ST_ANAN,
	"square", 0x0, smone, sone, SP_SQR, ST_FINITEEXACT,
	"square", 0x0, sone, sone, SP_SQR, ST_FINITEEXACT,
	"square", 0x0, stwo, sfour, SP_SQR, ST_FINITEEXACT,
	"square", 0x0, s1e2, s1e4, SP_SQR, ST_FINITEEXACT,
	"square", 0x0, ssqrt2, 0x3fffffff, SP_SQR, ST_FINITEINEXACT,
	"square", 0x0, ssnan, stwo, SP_SQR, ST_ABNAN,
	"add", shalf, shalf, sone, SP_ADD, ST_FINITEEXACT,
	"add", sone, sone, stwo, SP_ADD, ST_FINITEEXACT,
	"add", sone, smone, szero, SP_ADD, ST_ZERO,
	"add", smone, stwo, sone, SP_ADD, ST_FINITEEXACT,
	"add", spio2, spio2, spi, SP_ADD, ST_FINITEEXACT,
	"add", sone, sminnorm, sone, SP_ADD, ST_FINITEINEXACT,
	"add", ssnan, spio2, spi, SP_ADD, ST_ANAN,
	"add", spio2, ssnan, spi, SP_ADD, ST_BNAN,
	"add", ssnan, ssnan, spi, SP_ADD, ST_ABNAN,
	"subtract", sone, shalf, shalf, SP_SUB, ST_FINITEEXACT,
	"subtract", ssnan, shalf, shalf, SP_SUB, ST_ANAN,
	"multiply", sone, shalf, shalf, SP_MUL, ST_FINITEEXACT,
	"multiply", 0x3FD3B200, 0xBD5BB555, 0xBDB5AF39, SP_MUL, ST_FINITEINEXACT,
	"multiply", 0xBD5BB555, 0x3FD3B200, 0xBDB5AF39, SP_MUL, ST_FINITEINEXACT,
	"multiply", sone, szero, szero, SP_MUL, ST_ZERO,
	"multiply", sfour, shalf, stwo, SP_MUL, ST_FINITEEXACT,
	"multiply", ssqrt2, ssqrt2, 0x3fffffff, SP_MUL, ST_FINITEINEXACT,
	"multiply", ssnan, stwo, spi, SP_MUL, ST_BNAN,
	"divide", shalf, sone, shalf, SP_DIV, ST_FINITEEXACT,
	"divide", shalf, sfour, s1o8, SP_DIV, ST_FINITEEXACT,
	"divide", spi, stwo, spio2, SP_DIV, ST_FINITEEXACT,
	"divide", stwo, ssqrt2, ssqrt2, SP_DIV, ST_FINITEINEXACT,
	"divide", ssnan, stwo, spio2, SP_DIV, ST_ANAN,
	"backwards subtract", shalf, sone, shalf, SP_BSUB, ST_FINITEEXACT,
	"backwards subtract", smone, smone, szero, SP_BSUB, ST_ZERO,
	"backwards subtract", smone, ssnan, szero, SP_BSUB, ST_ANAN,
	"backwards divide", stwo, spi, spio2, SP_BDIV, ST_FINITEEXACT,
	"backwards divide", ssqrt2, stwo, ssqrt2, SP_BDIV, ST_FINITEINEXACT,
	"backwards divide", stwo, ssnan, spio2, SP_BDIV, ST_ANAN,
	"compare with 0", sone, shalf, sone, SP_CMP0, ST_GREATERTHAN,
	"compare with 0", ssnan, ssnan, ssnan, SP_CMP0, ST_INVALIDOP,
	"compare with R", sone, shalf, sone, SP_CMPR, ST_GREATERTHAN,
	"compare with R", sone, stwo, sone, SP_CMPR, ST_LESSTHAN,
	"compare with R", stwo, sone, stwo, SP_CMPR, ST_GREATERTHAN,
	"compare with R", ssnan, sone, stwo, SP_CMPR, ST_INVALIDOP,
	"compare with Mag R", sone, shalf, sone, SP_CMPM, ST_GREATERTHAN,
	"compare with Mag R", ssnan, shalf, sone, SP_CMPM, ST_INVALIDOP,
	0, 0, 0, 0, 0, 0
};
struct op2_table
{
	char *name;
	long r1_ms, r1_ls;
	long data_ms, data_ls;
	long result_ms, result_ls;
	int op1, op2;
	long status;
};
/*	Note that some entries in the following structure are actually
	two entries!!! */
struct op2_table dp_table[] =
{
	"nop", dzero, dzero, dzero, DP_NOP, ST_DONTCARE,
	"negate", dzero, done, dmone, DP_NEG, ST_FINITEEXACT,
	"negate", dzero, dmone, done, DP_NEG, ST_FINITEEXACT,
	"abs value", dzero, dmone, done, DP_ABS, ST_FINITEEXACT,
	"abs value", dzero, done, done, DP_ABS, ST_FINITEEXACT,
	"float", dzero, 2, 0, dtwo, DP_FLT, ST_FINITEEXACT,
	"float", dzero, -1, 0, dmone, DP_FLT, ST_FINITEEXACT,
	"fix", 0x33333333, 0xaaaaaaaa, dfour, 4, 0xaaaaaaaa, DP_FIX, ST_FINITEEXACT,
	"fix", 0x33333333, 0xaaaaaaaa, done, 1, 0xaaaaaaaa, DP_FIX, ST_FINITEEXACT,
	"fix", 0x33333333, 0xaaaaaaaa, dmone, -1, 0xaaaaaaaa, DP_FIX, ST_FINITEEXACT,
	"convert", 0, 0x55555555, deight, seight, 0x55555555, DP_CNV, ST_FINITEEXACT,
	"square", dzero, dtwo, dfour, DP_SQR, ST_FINITEEXACT,
	"square", dzero, d1e2, d1e4, DP_SQR, ST_FINITEEXACT,
	"square", dzero, dsqrt2, 0x40000000, 0x00000001, DP_SQR, ST_FINITEINEXACT,
	"square", dzero, dsnan, dtwo, DP_SQR, ST_ABNAN,
	"add", dhalf, dhalf, done, DP_ADD, ST_FINITEEXACT,
/* an example of where the Weitek chips didn't meet IEEE !!!
 */
	"add", 0x3ff3fdcd, 0xc41c154c, 0xbf71a36a, 0x1b0a4aa0, 0x3ff3ec2a, 0x5a010b01, DP_ADD, ST_FINITEINEXACT,
	"add", 0xbf71a36a, 0x1b0a4aa0, 0x3ff3fdcd, 0xc41c154c, 0x3ff3ec2a, 0x5a010b01, DP_ADD, ST_FINITEINEXACT,
/* an example of where the Weitek chips didn't meet IEEE !!!
 */
	"add", done, dmone, dzero, DP_ADD, ST_ZERO,
	"subtract", done, dhalf, dhalf, DP_SUB, ST_FINITEEXACT,
	"multiply", 0x3fe02061, 0x02c794b9, 0xbfdd3b00, 0x0, 0xbfcd7627, 0x3ac3fd84, DP_MUL, ST_FINITEINEXACT,
	"multiply", done, dhalf, dhalf, DP_MUL, ST_FINITEEXACT,
	"multiply", dfour, dhalf, dtwo, DP_MUL, ST_FINITEEXACT,
	"divide", dhalf, done, dhalf, DP_DIV, ST_FINITEEXACT,
	"divide", dhalf, dfour, d1o8, DP_DIV, ST_FINITEEXACT,
	"bw subtract", dhalf, done, dhalf, DP_BSUB, ST_FINITEEXACT,
	"bw divide", done, dhalf, dhalf, DP_BDIV, ST_FINITEEXACT,
	"compare with 0", done, dhalf, done, DP_CMP0, ST_GREATERTHAN,
	"compare with R", done, dhalf, done, DP_CMPR, ST_GREATERTHAN,
	"compare with R", done, dtwo, done, DP_CMPR, ST_LESSTHAN,
	"compare with R", dtwo, done, dtwo, DP_CMPR, ST_GREATERTHAN,
	"compare with Mag R", done, dhalf, done, DP_CMPM, ST_GREATERTHAN,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
struct op1_2_table
{
	char *name;
	long r1_ms, r1_ls;
	long data;
	long result_ms, result_ls;
	int op;
	long status;
};
struct op1_2_table sd_table[] =
{
	"convert", 0, 0, seight, deight, SP_CNV, ST_FINITEEXACT,
	"convert", 0, 0, smone, dmone, SP_CNV, ST_FINITEEXACT,
	0, 0, 0, 0, 0, 0, 0, 0
};
struct opx_table
{
	char *name;
	long r1_sp, r1_ms, r1_ls;
	long r2_sp, r2_ms, r2_ls;
	long r3_sp, r3_ms, r3_ls;
	long data_sp, data_ms, data_ls;
	long result_sp, result_ms, result_ls;
	long op_sp, op_dp;
	long status;
};
struct opx_table x_table[] =
{
	"add", ssnan, dsnan, sone, done, ssnan, dsnan, sone, done, stwo, dtwo, XS_ADD, XD_ADD, ST_FINITEEXACT,
	"add", ssnan, dsnan, stwo, dtwo, ssnan, dsnan, sminnorm, dminnorm, stwo, dtwo, XS_ADD, XD_ADD, ST_FINITEINEXACT,
	"add", ssnan, dsnan, stwo, dtwo, ssnan, dsnan, smone, dmone, sone, done, XS_ADD, XD_ADD, ST_FINITEEXACT,
	"add", ssnan, dsnan, sone, done, ssnan, dsnan, ssnan, dsnan, stwo, dtwo, XS_ADD, XD_ADD, ST_BNAN,
	"add", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, sone, done, stwo, dtwo, XS_ADD, XD_ADD, ST_ANAN,
	"subtract", ssnan, dsnan, sone, done, ssnan, dsnan, stwo, dtwo, smone, dmone, XS_SUB, XD_SUB, ST_FINITEEXACT,
	"subtract", ssnan, dsnan, sone, done, ssnan, dsnan, sminnorm, dminnorm, sone, done, XS_SUB, XD_SUB, ST_FINITEINEXACT,
	"subtract", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, sone, done, szero, dzero, XS_SUB, XD_SUB, ST_ANAN,
	"multiply", ssnan, dsnan, sone, done, ssnan, dsnan, sone, done, sone, done, XS_MUL, XD_MUL, ST_FINITEEXACT,
	"multiply", ssnan, dsnan, ssqrt2, 0xbfdd3b00, 0x0, ssnan, dsnan, ssqrt2, 0x3fe02061, 0x02c794b9, 0x3fffffff, 0xbfcd7627, 0x3ac3fd84, XS_MUL, XD_MUL, ST_FINITEINEXACT,
	"multiply", ssnan, dsnan, ssnan, done, ssnan, dsnan, sone, dsnan, smone, dmone, XS_MUL, XD_MUL, ST_BNAN,
	"divide", ssnan, dsnan, smone, dmone, ssnan, dsnan, sone, done, smone, dmone, XS_DIV, XD_DIV, ST_FINITEEXACT,
	"divide", ssnan, dsnan, seight, deight, ssnan, dsnan, stwo, dtwo, sfour, dfour, XS_DIV, XD_DIV, ST_FINITEEXACT,
	"divide", ssnan, dsnan, sone, done, ssnan, dsnan, sonezp1, donezp1, sonezm2, donezm2, XS_DIV, XD_DIV, ST_FINITEINEXACT,
	"divide", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, stwo, dtwo, sfour, dfour, XS_DIV, XD_DIV, ST_ANAN,
	"bw sub", ssnan, dsnan, stwo, dtwo, ssnan, dsnan, sone, done, smone, dmone, XS_BSUB, XD_BSUB, ST_FINITEEXACT,
	"bw sub", ssnan, dsnan, sminnorm, dminnorm, ssnan, dsnan, stwo, dtwo, stwo, dtwo, XS_BSUB, XD_BSUB, ST_FINITEINEXACT,
	"bw sub", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, sone, done, smone, dmone, XS_BSUB, XD_BSUB, ST_BNAN,
	"bw div", ssnan, dsnan, stwo, dtwo, ssnan, dsnan, sone, done, shalf, dhalf, XS_BDIV, XD_BDIV, ST_FINITEEXACT,
	"bw div", ssnan, dsnan, sonezp1, donezp1, ssnan, dsnan, sone, done, sonezm2, donezm2, XS_BDIV, XD_BDIV, ST_FINITEINEXACT,
	"bw div", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, sone, done, shalf, dhalf, XS_BDIV, XD_BDIV, ST_BNAN,
	"3+(2*d)", ssnan, dsnan, shalf, dhalf, stwo, dtwo, sfour, dfour, sfour, dfour, XS_PIV0, XD_PIV0, ST_FINITEEXACT,
	"3+(2*d)", ssnan, dsnan, shalf, dhalf, ssnan, dsnan, sfour, dfour, sfour, dfour, XS_PIV0, XD_PIV0, ST_ANAN,
	"3+(2*d)", ssnan, dsnan, ssnan, dfour, stwo, dtwo, sfour, dsnan, sfour, dfour, XS_PIV0, XD_PIV0, ST_BNAN,
	"3-(2*d)", ssnan, dsnan, shalf, dhalf, stwo, dtwo, sfour, dfour, szero, dzero, XS_PIV1, XD_PIV1, ST_ZERO,
	"3-(2*d)", ssnan, dsnan, ssnan, dzero, stwo, dtwo, sfour, dsnan, szero, dfour, XS_PIV1, XD_PIV1, ST_BNAN,
	"3-(2*d)", ssnan, dsnan, shalf, dhalf, ssnan, dsnan, sfour, dfour, szero, dzero, XS_PIV1, XD_PIV1, ST_ANAN,
	"-3+(2*d)", ssnan, dsnan, sone, done, shalf, dhalf, shalf, dhalf, szero, dzero, XS_PIV2, XD_PIV2, ST_ZERO,
	"-3+(2*d)", ssnan, dsnan, ssnan, dzero, shalf, dhalf, shalf, dsnan, szero, dhalf, XS_PIV2, XD_PIV2, ST_BNAN,
	"-3+(2*d)", ssnan, dsnan, sone, done, ssnan, dsnan, shalf, dhalf, szero, dzero, XS_PIV2, XD_PIV2, ST_BNAN,
	"3*(2+d)", ssnan, dsnan, shalf, dhalf, sfour, dfour, shalf, dhalf, sfour, dfour, XS_PIV3, XD_PIV3, ST_FINITEEXACT,
	"3*(2+d)", ssnan, dsnan, shalf, dhalf, ssnan, dfour, shalf, dsnan, sfour, dhalf, XS_PIV3, XD_PIV3, ST_BNAN,
	"3*(2+d)", ssnan, dsnan, ssnan, dsnan, shalf, dhalf, shalf, dhalf, sfour, dfour, XS_PIV3, XD_PIV3, ST_ANAN,
	"3*(2-d)", ssnan, dsnan, shalf, dhalf, smone, dmone, sone, done, shalf, dhalf, XS_PIV4, XD_PIV4, ST_FINITEEXACT,
	"3*(2-d)", ssnan, dsnan, ssnan, dsnan, smone, dmone, sone, done, shalf, dhalf, XS_PIV4, XD_PIV4, ST_ANAN,
	"3*(2-d)", ssnan, dsnan, shalf, dhalf, ssnan, dhalf, sone, dsnan, shalf, done, XS_PIV4, XD_PIV4, ST_BNAN,
	"3*(-2+d)", ssnan, dsnan, shalf, dhalf, seight, deight, sone, done, sfour, dfour, XS_PIV5, XD_PIV5, ST_FINITEEXACT,
	"3*(-2+d)", ssnan, dsnan, ssnan, dsnan, seight, deight, sone, done, sfour, dfour, XS_PIV5, XD_PIV5, ST_BNAN,
	"3*(-2+d)", ssnan, dsnan, shalf, dsnan, ssnan, dhalf, sone, done, sfour, dfour, XS_PIV5, XD_PIV5, ST_BNAN,
	"d+(3*2)", ssnan, dsnan, shalf, dhalf, seight, deight, sfour, dfour, seight, deight, XS_PIV6, XD_PIV6, ST_FINITEEXACT,
	"d+(3*2)", ssnan, dsnan, ssnan, dsnan, seight, deight, sfour, dfour, seight, deight, XS_PIV6, XD_PIV6, ST_ANAN,
	"d+(3*2)", ssnan, dsnan, shalf, dhalf, ssnan, dsnan, sfour, dfour, seight, deight, XS_PIV6, XD_PIV6, ST_BNAN,
	"d-(3*2)", ssnan, dsnan, shalf, dhalf, seight, deight, sfour, dfour, szero, dzero, XS_PIV7, XD_PIV7, ST_ZERO,
	"d-(3*2)", ssnan, dsnan, ssnan, dsnan, seight, deight, sfour, dfour, szero, dzero, XS_PIV7, XD_PIV7, ST_ANAN,
	"d-(3*2)", ssnan, dsnan, shalf, dhalf, ssnan, dsnan, sfour, dfour, szero, dzero, XS_PIV7, XD_PIV7, ST_BNAN,
	"-d+(3*2)", ssnan, dsnan, shalf, dhalf, seight, deight, stwo, dtwo, stwo, dtwo, XS_PIV8, XD_PIV8, ST_FINITEEXACT,
	"-d+(3*2)", ssnan, dsnan, ssnan, dsnan, seight, deight, stwo, dtwo, stwo, dtwo, XS_PIV8, XD_PIV8, ST_ANAN,
	"-d+(3*2)", ssnan, dsnan, shalf, dhalf, ssnan, dsnan, stwo, dtwo, stwo, dtwo, XS_PIV8, XD_PIV8, ST_BNAN,
	"d*(3+2)", ssnan, dsnan, shalf, dhalf, smone, dmone, stwo, dtwo, smone, dmone, XS_PIV9, XD_PIV9, ST_FINITEEXACT,
	"d*(3+2)", ssnan, dsnan, ssnan, dsnan, smone, dmone, stwo, dtwo, smone, dmone, XS_PIV9, XD_PIV9, ST_BNAN,
	"d*(3+2)", ssnan, dsnan, shalf, dhalf, ssnan, dsnan, stwo, dtwo, smone, dmone, XS_PIV9, XD_PIV9, ST_ANAN,
	"d*(3-2)", ssnan, dsnan, shalf, dhalf, sone, done, stwo, dtwo, sone, done, XS_PIVA, XD_PIVA, ST_FINITEEXACT,
	"d*(3-2)", ssnan, dsnan, ssnan, dsnan, sone, done, stwo, dtwo, sone, done, XS_PIVA, XD_PIVA, ST_BNAN,
	"d*(3-2)", ssnan, dsnan, shalf, dhalf, ssnan, dsnan, stwo, dtwo, sone, done, XS_PIVA, XD_PIVA, ST_ANAN,
	0, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, 0, 0, 0
};
struct opl_table
{
	char *name;
	long r2_sp;
	long data1_sp;
	long data2_sp;
	long result_sp;
	long op_sp;
	long status;
};
struct opl_table l_table[] =
{
	"op2+(r2*op1)", sone, smone, sone, szero, XS_LIN0, ST_ZERO,
	"op2+(r2*op1)", shalf, sfour, smone, sone, XS_LIN0, ST_FINITEEXACT,
	"op2+(r2*op1)", shalf, stwo, sminnorm, sone, XS_LIN0, ST_FINITEINEXACT,
	"op2+(r2*op1)", ssnan, sfour, smone, sone, XS_LIN0, ST_BNAN,
	"op2+(r2*op1)", sone, ssnan, smone, sone, XS_LIN0, ST_ANAN,
	"op2+(r2*op1)", sone, smone, ssnan, sone, XS_LIN0, ST_ANAN,
	"op2-(r2*op1)", sone, smone, sone, stwo, XS_LIN1, ST_FINITEEXACT,
	"op2-(r2*op1)", shalf, sfour, sone, smone, XS_LIN1, ST_FINITEEXACT,
	"op2-(r2*op1)", sone, sone, sminnorm, smone, XS_LIN1, ST_FINITEINEXACT,
	"op2-(r2*op1)", ssnan, sfour, sone, smone, XS_LIN1, ST_BNAN,
	"op2-(r2*op1)", sfour, ssnan, sone, smone, XS_LIN1, ST_ANAN,
	"op2-(r2*op1)", sfour, sone, ssnan, smone, XS_LIN1, ST_ANAN,
	"-op2+(r2*op1)", sone, sone, stwo, smone, XS_LIN2, ST_FINITEEXACT,
	"-op2+(r2*op1)", sfour, sone, sfour, szero, XS_LIN2, ST_ZERO,
	"-op2+(r2*op1)", sone, sone, sminnorm, sone, XS_LIN2, ST_FINITEINEXACT,
	"-op2+(r2*op1)", ssnan, sone, sfour, szero, XS_LIN2, ST_BNAN,
	"-op2+(r2*op1)", sone, ssnan, sfour, szero, XS_LIN2, ST_ANAN,
	"-op2+(r2*op1)", sone, sfour, ssnan, szero, XS_LIN2, ST_BNAN,
	"op2*(r2+op1)", sone, smone, sone, szero, XS_LIN3, ST_ZERO,
	"op2*(r2+op1)", stwo, smone, stwo, stwo, XS_LIN3, ST_FINITEEXACT,
	"op2*(r2+op1)", szero, sonezm1, sonezp1, sone, XS_LIN3, ST_FINITEINEXACT,
	"op2*(r2+op1)", ssnan, smone, stwo, stwo, XS_LIN3, ST_BNAN,
	"op2*(r2+op1)", smone, ssnan, stwo, stwo, XS_LIN3, ST_ANAN,
	"op2*(r2+op1)", smone, stwo, ssnan, stwo, XS_LIN3, ST_ANAN,
	"op2*(r2-op1)", sone, smone, sone, stwo, XS_LIN4, ST_FINITEEXACT,
	"op2*(r2-op1)", shalf, sone, smone, shalf, XS_LIN4, ST_FINITEEXACT,
	"op2*(r2-op1)", sonezm1, szero, sonezp1, sone, XS_LIN4, ST_FINITEINEXACT,
	"op2*(r2-op1)", ssnan, sone, smone, shalf, XS_LIN4, ST_ANAN,
	"op2*(r2-op1)", sone, ssnan, smone, shalf, XS_LIN4, ST_BNAN,
	"op2*(r2-op1)", sone, smone, ssnan, shalf, XS_LIN4, ST_ANAN,
	"op2*(-r2+op1)", sone, stwo, stwo, stwo, XS_LIN5, ST_FINITEEXACT,
	"op2*(-r2+op1)", sfour, seight, shalf, stwo, XS_LIN5, ST_FINITEEXACT,
	"op2*(-r2+op1)", szero, sonezm1, sonezp1, sone, XS_LIN5, ST_FINITEINEXACT,
	"op2*(-r2+op1)", ssnan, seight, shalf, stwo, XS_LIN5, ST_BNAN,
	"op2*(-r2+op1)", seight, ssnan, shalf, stwo, XS_LIN5, ST_ANAN,
	"op2*(-r2+op1)", seight, shalf, ssnan, stwo, XS_LIN5, ST_ANAN,
	0, ssnan, ssnan, ssnan, ssnan, 0, 0
};
struct opc_table
{
	char *name;
	long r2_sp, r2_ms, r2_ls;
	long r3_sp, r3_ms, r3_ls;
	long r4_sp, r4_ms, r4_ls;
	long result_sp, result_ms, result_ls;
	long op_sp, op_dp;
	long status;
};
struct opc_table c_table[] =
{
/*	"sqrt", szero, dzero, ssnan, dsnan, ssnan, dsnan, szero, dzero, CS_SQRT, 0, ST_ZERO,
	"sqrt", sfour, dfour, ssnan, dsnan, ssnan, dsnan, stwo, dtwo, CS_SQRT, 0, ST_FINITEEXACT,
	"sqrt", sone, done, ssnan, dsnan, ssnan, dsnan, sone, dtwo, CS_SQRT, 0, ST_FINITEEXACT,
 */
	"sin", szero, dzero, ssnan, dsnan, ssnan, dsnan, szero, dzero, CS_SIN, CD_SIN, ST_ZERO,
	"sin", smhalf, dmhalf, ssnan, dsnan, ssnan, dsnan, 0xbef57744, 0xbfdeaee8, 0x744b05f0, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", shalf, dhalf, ssnan, dsnan, ssnan, dsnan, 0x3ef57744, 0x3fdeaee8, 0x744b05f0, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", sone, done, ssnan, dsnan, ssnan, dsnan, 0x3f576aa4, 0x3feaed54, 0x8f090cee, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", smone, dmone, ssnan, dsnan, ssnan, dsnan, 0xbf576aa4, 0xbfeaed54, 0x8f090cee, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", stwo, dtwo, ssnan, dsnan, ssnan, dsnan, 0x3f68c7b7, 0x3fed18f6, 0xead1b446, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", sthree, dthree, ssnan, dsnan, ssnan, dsnan, 0x3e1081c3, 0x3fc21038, 0x6db6d55b, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", 0xc0400000, 0xc0080000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbe1081c3, 0xbfc21038, 0x6db6d55b, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", 0x41000000, 0x40200000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0x3f7d4695, 0x3fefa8d2, 0xa028cf7b, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", 0xc1000000, 0xc0200000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbf7d4695, 0xbfefa8d2, 0xa028cf7b, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", 0x42000000, 0x40400000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0x3f0d2a4c, 0x3fe1a549, 0x91426566, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", 0x49c90fdb, 0x413921fb, 0x54442d18, ssnan, dsnan, ssnan, dsnan, 0x3d3bac5b, 0xbdd1a600, 0x00000000, CS_SIN, CD_SIN, ST_FINITEINEXACT,
	"sin", 0x49c90fdc, 0x413921fb, 0x54442d19, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_SIN, CD_SIN, ST_OUTOFBOUNDS,
	"sin", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_SIN, CD_SIN, ST_OUTOFBOUNDS,
	"sin", sqnan, dqnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_SIN, CD_SIN, ST_OUTOFBOUNDS,
	"sin", smaxnorm, dmaxnorm, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_SIN, CD_SIN, ST_OUTOFBOUNDS,
	"cos", szero, dzero, ssnan, dsnan, ssnan, dsnan, sone, done, CS_COS, CD_COS, ST_FINITEEXACT,
	"cos", smhalf, dmhalf, ssnan, dsnan, ssnan, dsnan, 0x3f60a940, 0x3fec1528, 0x065b7d50, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", shalf, dhalf, ssnan, dsnan, ssnan, dsnan, 0x3f60a940, 0x3fec1528, 0x065b7d50, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", smone, dmone, ssnan, dsnan, ssnan, dsnan, 0x3f0a5140, 0x3fe14a28, 0x0fb5068c, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", sone, done, ssnan, dsnan, ssnan, dsnan, 0x3f0a5140, 0x3fe14a28, 0x0fb5068c, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", stwo, dtwo, ssnan, dsnan, ssnan, dsnan, 0xbed51133, 0xbfdaa226, 0x57537205, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", 0x40400000, 0x40080000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbf7d7026, 0xbfefae04, 0xbe85e5d2, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", sthree, dthree, ssnan, dsnan, ssnan, dsnan, 0xbf7d7026, 0xbfefae04, 0xbe85e5d2, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", 0xc1000000, 0xc0200000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbe14fdf6, 0xbfc29fbe, 0xbf632f94, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", 0x41000000, 0x40200000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbe14fdf6, 0xbfc29fbe, 0xbf632f94, CS_COS, CD_COS, ST_FINITEINEXACT,
	"cos", 0x49c90fdc, 0x413921fb, 0x54442d19, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_COS, CD_COS, ST_OUTOFBOUNDS,
	"cos", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_COS, CD_COS, ST_OUTOFBOUNDS,
	"cos", sqnan, dqnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_COS, CD_COS, ST_OUTOFBOUNDS,
	"cos", smaxnorm, dmaxnorm, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_COS, CD_COS, ST_OUTOFBOUNDS,
	"atan", szero, dzero, ssnan, dsnan, ssnan, dsnan, szero, dzero, CS_ATAN, CD_ATAN, ST_ZERO,
	"atan", c_stwopm26, c_dtwopm54, ssnan, dsnan, ssnan, dsnan, c_stwopm26, c_dtwopm54, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", sone, done, ssnan, dsnan, ssnan, dsnan, 0x3f490fdb, 0x3fe921fb, 0x54442d18, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", smone, dmone, ssnan, dsnan, ssnan, dsnan, 0xbf490fdb, 0xbfe921fb, 0x54442d18, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", shalf, dhalf, ssnan, dsnan, ssnan, dsnan, 0x3eed6338, 0x3fddac67, 0x0561bb4f, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", smhalf, dmhalf, ssnan, dsnan, ssnan, dsnan, 0xbeed6338, 0xbfddac67, 0x0561bb4f, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
/*  9/16 */
	"atan", 0x3f100000, 0x3fe20000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0x3f032bf5, 0x3fe0657e, 0x94db30d0, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", 0xbf100000, 0xbfe20000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbf032bf5, 0xbfe0657e, 0x94db30d0, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
/*  13/16 */
	"atan", 0x3f500000, 0x3fea0000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0x3f2eac4c, 0x3fe5d589, 0x87169b18, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", 0xbf500000, 0xbfea0000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbf2eac4c, 0xbfe5d589, 0x87169b18, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", s1o4, d1o4, ssnan, dsnan, ssnan, dsnan, 0x3e7adbb0, 0x3fcf5b75, 0xf92c80dd, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", 0xbe800000, 0xbfd00000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbe7adbb0, 0xbfcf5b75, 0xf92c80dd, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", c_stwop26, c_dtwop54, ssnan, dsnan, ssnan, dsnan, 0x3fc90fdb, 0x3ff921fb, 0x54442d18, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", stwo, dtwo, ssnan, dsnan, ssnan, dsnan, 0x3f8db70d, 0x3ff1b6e1, 0x92ebbe44, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", 0xc0000000, 0xc0000000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbf8db70d, 0xbff1b6e1, 0x92ebbe44, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", sfour, dfour, ssnan, dsnan, ssnan, dsnan, 0x3fa9b465, 0x3ff5368c, 0x951e9cfd, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", 0xc0800000, 0xc0100000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbfa9b465, 0xbff5368c, 0x951e9cfd, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", sinf, dinf, ssnan, dsnan, ssnan, dsnan, 0x3fc90fdb, 0x3ff921fb, 0x54442d18, CS_ATAN, CD_ATAN, ST_FINITEINEXACT,
	"atan", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_ATAN, CD_ATAN, ST_OUTOFBOUNDS,
	"ln", sone, done, ssnan, dsnan, ssnan, dsnan, szero, dzero, CS_LN, CD_LN, ST_ZERO,
	"ln", stwo, dtwo, ssnan, dsnan, ssnan, dsnan, 0x3f317218, 0x3fe62e42, 0xfefa39ef, CS_LN, CD_LN, ST_FINITEINEXACT,
	"ln", sthree, dthree, ssnan, dsnan, ssnan, dsnan, 0x3f8c9f54, 0x3ff193ea, 0x7aad030a, CS_LN, CD_LN, ST_FINITEINEXACT,
	"ln", sminnorm, dminnorm, ssnan, dsnan, ssnan, dsnan, 0xc2aeac50, 0xc086232b, 0xdd7abcd2, CS_LN, CD_LN, ST_FINITEINEXACT,
	"ln", smaxsub, dmaxsub, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_LN, CD_LN, ST_OUTOFBOUNDS,
	"ln", c_smaxln, c_dmaxln, ssnan, dsnan, ssnan, dsnan, 0x42b00f33, 0x408628b7, 0x5e3a6b61, CS_LN, CD_LN, ST_FINITEINEXACT,
	"ln", 0x7f000000, 0x7fe00000, 0x00000000, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_LN, CD_LN, ST_OUTOFBOUNDS,
	"ln", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_LN, CD_LN, ST_OUTOFBOUNDS,
	"ln1", szero, dzero, ssnan, dsnan, ssnan, dsnan, szero, dzero, CS_LN1, CD_LN1, ST_ZERO,
	"ln1", c_stwopm26, c_dtwopm54, ssnan, dsnan, ssnan, dsnan, c_stwopm26, c_dtwopm54, CS_LN1, CD_LN1, ST_FINITEINEXACT,
	"ln1", 0xbf400000, 0xbfe80000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbfb17218, 0xbff62e42, 0xfefa39ef, CS_LN1, CD_LN1, ST_FINITEINEXACT,
	"ln1", 0xbe800000, 0xbfd00000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbe934b11, 0xbfd26962, 0x1134db92, CS_LN1, CD_LN1, ST_FINITEINEXACT,
	"ln1", 0xbf000000, 0xbfe00000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbf317218, 0xbfe62e42, 0xfefa39ef, CS_LN1, CD_LN1, ST_FINITEINEXACT,
	"ln1", 0x3e800000, 0x3fd00000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0x3e647fbe, 0x3fcc8ff7, 0xc79a9a22, CS_LN1, CD_LN1, ST_FINITEINEXACT,
	"ln1", sone, done, ssnan, dsnan, ssnan, dsnan, 0x3f317218, 0x3fe62e42, 0xfefa39ef, CS_LN1, CD_LN1, ST_FINITEINEXACT,
	"ln1", smone, dmone, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_LN1, CD_LN1, ST_OUTOFBOUNDS,
	"ln1", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_LN1, CD_LN1, ST_OUTOFBOUNDS,
	"ln1", stwo, dtwo, ssnan, dsnan, ssnan, dsnan, 0x3f8c9f54, 0x3ff193ea, 0x7aad030a, CS_LN1, CD_LN1, ST_FINITEINEXACT,
	"ln1", c_smaxln, c_dmaxln, ssnan, dsnan, ssnan, dsnan, 0x42b00f33, 0x408628b7, 0x5e3a6b61, CS_LN1, CD_LN1, ST_FINITEINEXACT,
	"ln1", 0x7f000000, 0x7fe00000, 0x00000000, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_LN1, CD_LN1, ST_OUTOFBOUNDS,
	"exp", szero, dzero, ssnan, dsnan, ssnan, dsnan, sone, done, CS_EXP, CD_EXP, ST_FINITEEXACT,
	"exp", c_stwopm26, c_dtwopm54, ssnan, dsnan, ssnan, dsnan, sone, done, CS_EXP, CD_EXP, ST_FINITEINEXACT,
	"exp", sone, done, ssnan, dsnan, ssnan, dsnan, 0x402df854, 0x4005bf0a, 0x8b14576a, CS_EXP, CD_EXP, ST_FINITEINEXACT,
	"exp", stwo, dtwo, ssnan, dsnan, ssnan, dsnan, 0x40ec7326, 0x401d8e64, 0xb8d4ddae, CS_EXP, CD_EXP, ST_FINITEINEXACT,
	"exp", c_s87, c_d708, ssnan, dsnan, ssnan, dsnan, 0x7e36d809, 0x7fc586f6, 0xbf260cf1, CS_EXP, CD_EXP, ST_FINITEINEXACT,
	"exp", 0x42ae0001, 0x40862000, 0x00000001, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_EXP, CD_EXP, ST_OUTOFBOUNDS,
	"exp", 0xc2ae0001, 0xc0862000, 0x00000001, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_EXP, CD_EXP, ST_OUTOFBOUNDS,
	"exp", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_EXP, CD_EXP, ST_OUTOFBOUNDS,
	"exp1", szero, dzero, ssnan, dsnan, ssnan, dsnan, szero, dzero, CS_EXP1, CD_EXP1, ST_ZERO,
	"exp1", c_stwopm26, c_dtwopm54, ssnan, dsnan, ssnan, dsnan, c_stwopm26, c_dtwopm54, CS_EXP1, CD_EXP1, ST_FINITEINEXACT,
	"exp1", 0x42800000, 0x40500000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0x6da12cc1, 0x45b42598, 0x2cf597cd, CS_EXP1, CD_EXP1, ST_FINITEINEXACT,
	"exp1", smone, dmone, ssnan, dsnan, ssnan, dsnan, 0xbf21d2a7, 0xbfe43a54, 0xe4e98864, CS_EXP1, CD_EXP1, ST_FINITEINEXACT,
	"exp1", sone, done, ssnan, dsnan, ssnan, dsnan, 0x3fdbf0a9, 0x3ffb7e15, 0x1628aed3, CS_EXP1, CD_EXP1, ST_FINITEINEXACT,
	"exp1", stwo, dtwo, ssnan, dsnan, ssnan, dsnan, 0x40cc7326, 0x40198e64, 0xb8d4ddae, CS_EXP1, CD_EXP1, ST_FINITEINEXACT,
	"exp1", 0xc0000000, 0xc0000000, 0x00000000, ssnan, dsnan, ssnan, dsnan, 0xbf5d5aab, 0xbfebab55, 0x57101f8d, CS_EXP1, CD_EXP1, ST_FINITEINEXACT,
	"exp1", c_s87, c_d708, ssnan, dsnan, ssnan, dsnan, 0x7e36d809, 0x7fc586f6, 0xbf260cf1, CS_EXP1, CD_EXP1, ST_FINITEINEXACT,
	"exp1", 0x42ae0001, 0x40862000, 0x00000001, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_EXP1, CD_EXP1, ST_OUTOFBOUNDS,
	"exp1", 0xc2ae0001, 0xc0862000, 0x00000001, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_EXP1, CD_EXP1, ST_OUTOFBOUNDS,
	"exp1", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_EXP1, CD_EXP1, ST_OUTOFBOUNDS,
	"copy", 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, ssnan, dsnan, ssnan, dsnan, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, CS_CPY, CD_CPY, ST_DONTCARE,
	"copy", 0x55555555, 0xaaaaaaaa, 0x55555555, ssnan, dsnan, ssnan, dsnan, 0x55555555, 0xaaaaaaaa, 0x55555555, CS_CPY, CD_CPY, ST_DONTCARE,
	"3+2*4", smone, dmone, sone, done, stwo, dtwo, smone, dmone, CS_PIV1, CD_PIV1, ST_FINITEEXACT,
	"3+2*4", smone, dmone, sminnorm, dminnorm, sone, done, smone, dmone, CS_PIV1, CD_PIV1, ST_FINITEINEXACT,
	"3+2*4", ssnan, dsnan, sone, done, stwo, dtwo, smone, dmone, CS_PIV1, CD_PIV1, ST_ANAN,
	"3+2*4", sone, done, ssnan, dsnan, stwo, dtwo, smone, dmone, CS_PIV1, CD_PIV1, ST_ANAN,
	"3+2*4", sone, done, stwo, dtwo, ssnan, dsnan, smone, dmone, CS_PIV1, CD_PIV1, ST_BNAN,
	"3-2*4", smone, dmone, stwo, dtwo, stwo, dtwo, sfour, dfour, CS_PIV2, CD_PIV2, ST_FINITEEXACT,
	"3-2*4", smone, dmone, sminnorm, dminnorm, stwo, dtwo, stwo, dtwo, CS_PIV2, CD_PIV2, ST_FINITEINEXACT,
	"3-2*4", ssnan, dsnan, stwo, dtwo, stwo, dtwo, sfour, dfour, CS_PIV2, CD_PIV2, ST_ANAN,
	"3-2*4", stwo, dtwo, ssnan, dsnan, stwo, dtwo, sfour, dfour, CS_PIV2, CD_PIV2, ST_ANAN,
	"3-2*4", stwo, dtwo, stwo, dtwo, ssnan, dsnan, sfour, dfour, CS_PIV2, CD_PIV2, ST_BNAN,
	"-3+2*4", sone, done, stwo, dtwo, sfour, dfour, stwo, dtwo, CS_PIV3, CD_PIV3, ST_FINITEEXACT,
	"-3+2*4", sone, done, sminnorm, dminnorm, sfour, dfour, sfour, dfour, CS_PIV3, CD_PIV3, ST_FINITEINEXACT,
	"-3+2*4", ssnan, dsnan, stwo, dtwo, sfour, dfour, stwo, dtwo, CS_PIV3, CD_PIV3, ST_ANAN,
	"-3+2*4", stwo, dtwo, ssnan, dsnan, sfour, dfour, stwo, dtwo, CS_PIV3, CD_PIV3, ST_BNAN,
	"-3+2*4", stwo, dtwo, sfour, dfour, ssnan, dsnan, stwo, dtwo, CS_PIV3, CD_PIV3, ST_BNAN,
	"3*2+4", smone, dmone, smone, dmone, shalf, dhalf, shalf, dhalf, CS_PIV4, CD_PIV4, ST_FINITEEXACT,
	"3*2+4", sonezp1, donezp1, sonezm1, donezm1, szero, dzero, sone, done, CS_PIV4, CD_PIV4, ST_FINITEINEXACT,
	"3*2+4", ssnan, dsnan, smone, dmone, shalf, dhalf, shalf, dhalf, CS_PIV4, CD_PIV4, ST_ANAN,
	"3*2+4", smone, dmone, ssnan, dsnan, shalf, dhalf, shalf, dhalf, CS_PIV4, CD_PIV4, ST_ANAN,
	"3*2+4", smone, dmone, shalf, dhalf, ssnan, dsnan, shalf, dhalf, CS_PIV4, CD_PIV4, ST_BNAN,
	"3*2-4", sone, done, sone, done, shalf, dhalf, shalf, dhalf, CS_PIV5, CD_PIV5, ST_FINITEEXACT,
	"3*2-4", sonezp1, donezp1, sonezm1, donezm1, szero, dzero, sone, done, CS_PIV5, CD_PIV5, ST_FINITEINEXACT,
	"3*2-4", ssnan, dsnan, sone, done, shalf, dhalf, shalf, dhalf, CS_PIV5, CD_PIV5, ST_ANAN,
	"3*2-4", sone, done, ssnan, dsnan, shalf, dhalf, shalf, dhalf, CS_PIV5, CD_PIV5, ST_ANAN,
	"3*2-4", sone, done, shalf, dhalf, ssnan, dsnan, shalf, dhalf, CS_PIV5, CD_PIV5, ST_BNAN,
	"3*-2+4", sone, done, smone, dmone, shalf, dhalf, shalf, dhalf, CS_PIV6, CD_PIV6, ST_FINITEEXACT,
	"3*-2+4", szero, dzero, sonezm1, donezm1, sonezp1, donezp1, sone, done, CS_PIV6, CD_PIV6, ST_FINITEINEXACT,
	"3*-2+4", ssnan, dsnan, smone, dmone, shalf, dhalf, shalf, dhalf, CS_PIV6, CD_PIV6, ST_BNAN,
	"3*-2+4", smone, dmone, ssnan, dsnan, shalf, dhalf, shalf, dhalf, CS_PIV6, CD_PIV6, ST_ANAN,
	"3*-2+4", smone, dmone, shalf, dhalf, ssnan, dsnan, shalf, dhalf, CS_PIV6, CD_PIV6, ST_ANAN,
	"ALU op = 00", sone, done, smone, dmone, ssnan, dsnan, stwo, dtwo, CS_ALU00, CD_ALU00, ST_FINITEEXACT,
	"ALU op = 01", smone, dmone, sone, done, ssnan, dsnan, stwo, dtwo, CS_ALU01, CD_ALU01, ST_FINITEEXACT,
	"ALU op = 02", smone, dmone, sone, done, ssnan, dsnan, szero, dzero, CS_ALU02, CD_ALU02, ST_ZERO,
	"ALU op = 03", seight, deight, sfour, dfour, ssnan, dsnan, stwo, dtwo, CS_ALU03, CD_ALU03, ST_FINITEEXACT,
	"ALU op = 04", smone, dmone, ssnan, dsnan, ssnan, dsnan, sone, done, CS_ALU04, CD_ALU04, ST_FINITEEXACT,
/* have to figure out how to do wrapped op's!!
	"ALU op = 07", seight, deight, sfour, dfour, ssnan, dsnan, stwo, dtwo, CS_ALU07, CD_ALU07, ST_FINITEEXACT,
 */
	0, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, 0, 0, 0
};
struct opc_table sincos_table[] =
{
	"sincos", szero, dzero, ssnan, dsnan, sone, done, szero, dzero, CS_SINCOS, CD_SINCOS, ST_ZERO,
	"sincos", smhalf, dmhalf, ssnan, dsnan, 0x3f60a940, 0x3fec1528, 0x065b7d50, 0xbef57744, 0xbfdeaee8, 0x744b05f0, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", shalf, dhalf, ssnan, dsnan, 0x3f60a940, 0x3fec1528, 0x065b7d50, 0x3ef57744, 0x3fdeaee8, 0x744b05f0, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", sone, done, ssnan, dsnan, 0x3f0a5140, 0x3fe14a28, 0x0fb5068c, 0x3f576aa4, 0x3feaed54, 0x8f090cee, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", smone, dmone, ssnan, dsnan, 0x3f0a5140, 0x3fe14a28, 0x0fb5068c, 0xbf576aa4, 0xbfeaed54, 0x8f090cee, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", stwo, dtwo, ssnan, dsnan, 0xbed51133, 0xbfdaa226, 0x57537205, 0x3f68c7b7, 0x3fed18f6, 0xead1b446, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", sthree, dthree, ssnan, dsnan, 0xbf7d7026, 0xbfefae04, 0xbe85e5d2, 0x3e1081c3, 0x3fc21038, 0x6db6d55b, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", 0xc0400000, 0xc0080000, 0x00000000, ssnan, dsnan, 0xbf7d7026, 0xbfefae04, 0xbe85e5d2, 0xbe1081c3, 0xbfc21038, 0x6db6d55b, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", 0x41000000, 0x40200000, 0x00000000, ssnan, dsnan, 0xbe14fdf6, 0xbfc29fbe, 0xbf632f94, 0x3f7d4695, 0x3fefa8d2, 0xa028cf7b, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", 0xc1000000, 0xc0200000, 0x00000000, ssnan, dsnan, 0xbe14fdf6, 0xbfc29fbe, 0xbf632f94, 0xbf7d4695, 0xbfefa8d2, 0xa028cf7b, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", 0x42000000, 0x40400000, 0x00000000, ssnan, dsnan, 0x3f558faa, 0x3feab1f5, 0x305de8e5, 0x3f0d2a4c, 0x3fe1a549, 0x91426566, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", 0x49c90fdb, 0x413921fb, 0x54442d18, ssnan, dsnan, 0x3f7fbb2c, 0x3ff00000, 0x00000000, 0x3d3bac5b, 0xbdd1a600, 0x00000000, CS_SINCOS, CD_SINCOS, ST_FINITEINEXACT,
	"sincos", 0x49c90fdc, 0x413921fb, 0x54442d19, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_SINCOS, CD_SINCOS, ST_OUTOFBOUNDS,
	"sincos", ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_SINCOS, CD_SINCOS, ST_OUTOFBOUNDS,
	"sincos", sqnan, dqnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_SINCOS, CD_SINCOS, ST_OUTOFBOUNDS,
	"sincos", smaxnorm, dmaxnorm, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, CS_SINCOS, CD_SINCOS, ST_OUTOFBOUNDS,
	0, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, ssnan, dsnan, 0, 0, 0
};
struct opm_table
{
	char *name;
	long *start;
	long *result_sp;
	long *result_dp;
	long op_sp, op_dp;
	long status;
};
long start1[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x010, 0x110, 0x011, 0x111, 0x012, 0x112, 0x013, 0x113,
	0x014, 0x114, 0x015, 0x115, 0x016, 0x116, 0x017, 0x117,
	0x018, 0x118, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_sp1[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x000, 0x110, 0x001, 0x111, 0x002, 0x112, 0x003, 0x113,
	0x014, 0x114, 0x015, 0x115, 0x016, 0x116, 0x017, 0x117,
	0x018, 0x118, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_dp1[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x014, 0x114, 0x015, 0x115, 0x016, 0x116, 0x017, 0x117,
	0x018, 0x118, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_sp2[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x000, 0x110, 0x001, 0x111, 0x002, 0x112, 0x003, 0x113,
	0x004, 0x114, 0x005, 0x115, 0x006, 0x116, 0x007, 0x117,
	0x008, 0x118, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_dp2[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_sp3[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x000, 0x110, 0x001, 0x111, 0x002, 0x112, 0x003, 0x113,
	0x004, 0x114, 0x005, 0x115, 0x006, 0x116, 0x007, 0x117,
	0x008, 0x118, 0x009, 0x119, 0x00A, 0x11A, 0x00B, 0x11B,
	0x00C, 0x11C, 0x00D, 0x11D, 0x00E, 0x11E, 0x00F, 0x11F
};
long matrix_dp3[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F
};
long matrix_sp4[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x010, 0x110, 0x012, 0x111, 0x011, 0x112, 0x013, 0x113,
	0x014, 0x114, 0x015, 0x115, 0x016, 0x116, 0x017, 0x117,
	0x018, 0x118, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_dp4[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x010, 0x110, 0x012, 0x112, 0x011, 0x111, 0x013, 0x113,
	0x014, 0x114, 0x015, 0x115, 0x016, 0x116, 0x017, 0x117,
	0x018, 0x118, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_sp5[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x010, 0x110, 0x013, 0x111, 0x016, 0x112, 0x011, 0x113,
	0x014, 0x114, 0x017, 0x115, 0x012, 0x116, 0x015, 0x117,
	0x018, 0x118, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_dp5[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x010, 0x110, 0x013, 0x113, 0x016, 0x116, 0x011, 0x111,
	0x014, 0x114, 0x017, 0x117, 0x012, 0x112, 0x015, 0x115,
	0x018, 0x118, 0x019, 0x119, 0x01A, 0x11A, 0x01B, 0x11B,
	0x01C, 0x11C, 0x01D, 0x11D, 0x01E, 0x11E, 0x01F, 0x11F
};
long matrix_sp6[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x010, 0x110, 0x014, 0x111, 0x018, 0x112, 0x01C, 0x113,
	0x011, 0x114, 0x015, 0x115, 0x019, 0x116, 0x01D, 0x117,
	0x012, 0x118, 0x016, 0x119, 0x01A, 0x11A, 0x01E, 0x11B,
	0x013, 0x11C, 0x017, 0x11D, 0x01B, 0x11E, 0x01F, 0x11F
};
long matrix_dp6[FPA_NDATA_REGS][2] = 
{
	0x000, 0x100, 0x001, 0x101, 0x002, 0x102, 0x003, 0x103,
	0x004, 0x104, 0x005, 0x105, 0x006, 0x106, 0x007, 0x107,
	0x008, 0x108, 0x009, 0x109, 0x00A, 0x10A, 0x00B, 0x10B,
	0x00C, 0x10C, 0x00D, 0x10D, 0x00E, 0x10E, 0x00F, 0x10F,
	0x010, 0x110, 0x014, 0x114, 0x018, 0x118, 0x01C, 0x11C,
	0x011, 0x111, 0x015, 0x115, 0x019, 0x119, 0x01D, 0x11D,
	0x012, 0x112, 0x016, 0x116, 0x01A, 0x11A, 0x01E, 0x11E,
	0x013, 0x113, 0x017, 0x117, 0x01B, 0x11B, 0x01F, 0x11F
};
long start_dot_sp[FPA_NDATA_REGS][2] = 
{
	sfour, 0x1100, smone, 0x1101, stwo, 0x1102, shalf, 0x1103,
	sone, 0x1104, stwo, 0x1105, smone, 0x1106, sfour, 0x1107,
	0x1008, 0x1108, 0x1009, 0x1109, 0x100A, 0x110A, 0x100B, 0x110B,
	0x100C, 0x110C, 0x100D, 0x110D, 0x100E, 0x110E, 0x100F, 0x110F,
	0x1010, 0x1110, 0x1011, 0x1111, 0x1012, 0x1112, 0x1013, 0x1113,
	0x1014, 0x1114, 0x1015, 0x1115, 0x1016, 0x1116, 0x1017, 0x1117,
	0x1018, 0x1118, 0x1019, 0x1119, 0x101A, 0x111A, 0x101B, 0x111B,
	0x101C, 0x111C, 0x101D, 0x111D, 0x101E, 0x111E, 0x101F, 0x111F
};
long start_dot_sp1[FPA_NDATA_REGS][2] = 
{
	sfour, 0x1100, smone, 0x1101, stwo, 0x1102, shalf, 0x1103,
	sone, 0x1104, stwo, 0x1105, smone, 0x1106, sfour, 0x1107,
	0x1008, 0x1108, 0x1009, 0x1109, 0x100A, 0x110A, 0x100B, 0x110B,
	0x100C, 0x110C, 0x100D, 0x110D, 0x100E, 0x110E, 0x100F, 0x110F,
	stwo, 0x1110, 0x1011, 0x1111, 0x1012, 0x1112, 0x1013, 0x1113,
	0x1014, 0x1114, 0x1015, 0x1115, 0x1016, 0x1116, 0x1017, 0x1117,
	0x1018, 0x1118, 0x1019, 0x1119, 0x101A, 0x111A, 0x101B, 0x111B,
	0x101C, 0x111C, 0x101D, 0x111D, 0x101E, 0x111E, 0x101F, 0x111F
};
long start_dot_sp2[FPA_NDATA_REGS][2] = 
{
	sfour, 0x1100, smone, 0x1101, stwo, 0x1102, shalf, 0x1103,
	sone, 0x1104, stwo, 0x1105, smone, 0x1106, sfour, 0x1107,
	0x1008, 0x1108, 0x1009, 0x1109, 0x100A, 0x110A, 0x100B, 0x110B,
	0x100C, 0x110C, 0x100D, 0x110D, 0x100E, 0x110E, 0x100F, 0x110F,
	szero, 0x1110, 0x1011, 0x1111, 0x1012, 0x1112, 0x1013, 0x1113,
	0x1014, 0x1114, 0x1015, 0x1115, 0x1016, 0x1116, 0x1017, 0x1117,
	0x1018, 0x1118, 0x1019, 0x1119, 0x101A, 0x111A, 0x101B, 0x111B,
	0x101C, 0x111C, 0x101D, 0x111D, 0x101E, 0x111E, 0x101F, 0x111F
};
long start_dot_sp3[FPA_NDATA_REGS][2] = 
{
	sfour, 0x1100, smone, 0x1101, stwo, 0x1102, shalf, 0x1103,
	sone, 0x1104, stwo, 0x1105, smone, 0x1106, sfour, 0x1107,
	0x1008, 0x1108, 0x1009, 0x1109, 0x100A, 0x110A, 0x100B, 0x110B,
	0x100C, 0x110C, 0x100D, 0x110D, 0x100E, 0x110E, 0x100F, 0x110F,
	stwo, 0x1110, 0x1011, 0x1111, 0x1012, 0x1112, 0x1013, 0x1113,
	0x1014, 0x1114, 0x1015, 0x1115, 0x1016, 0x1116, 0x1017, 0x1117,
	0x1018, 0x1118, 0x1019, 0x1119, 0x101A, 0x111A, 0x101B, 0x111B,
	0x101C, 0x111C, 0x101D, 0x111D, 0x101E, 0x111E, 0x101F, 0x111F
};
long start_dot_dp[FPA_NDATA_REGS][2] = 
{
	dfour, dmone, dtwo, dhalf,
	done, dtwo, dmone, dfour,
	0x1008, 0x1108, 0x1009, 0x1109, 0x100A, 0x110A, 0x100B, 0x110B,
	0x100C, 0x110C, 0x100D, 0x110D, 0x100E, 0x110E, 0x100F, 0x110F,
	0x1010, 0x1110, 0x1011, 0x1111, 0x1012, 0x1112, 0x1013, 0x1113,
	0x1014, 0x1114, 0x1015, 0x1115, 0x1016, 0x1116, 0x1017, 0x1117,
	0x1018, 0x1118, 0x1019, 0x1119, 0x101A, 0x111A, 0x101B, 0x111B,
	0x101C, 0x111C, 0x101D, 0x111D, 0x101E, 0x111E, 0x101F, 0x111F
};
long start_dot_dp1[FPA_NDATA_REGS][2] = 
{
	dfour, dmone, dtwo, dhalf,
	done, dtwo, dmone, dfour,
	0x1008, 0x1108, 0x1009, 0x1109, 0x100A, 0x110A, 0x100B, 0x110B,
	0x100C, 0x110C, 0x100D, 0x110D, 0x100E, 0x110E, 0x100F, 0x110F,
	dtwo, 0x1011, 0x1111, 0x1012, 0x1112, 0x1013, 0x1113,
	0x1014, 0x1114, 0x1015, 0x1115, 0x1016, 0x1116, 0x1017, 0x1117,
	0x1018, 0x1118, 0x1019, 0x1119, 0x101A, 0x111A, 0x101B, 0x111B,
	0x101C, 0x111C, 0x101D, 0x111D, 0x101E, 0x111E, 0x101F, 0x111F
};
long start_dot_dp2[FPA_NDATA_REGS][2] = 
{
	dfour, dmone, dtwo, dhalf,
	done, dtwo, dmone, dfour,
	0x1008, 0x1108, 0x1009, 0x1109, 0x100A, 0x110A, 0x100B, 0x110B,
	0x100C, 0x110C, 0x100D, 0x110D, 0x100E, 0x110E, 0x100F, 0x110F,
	dzero, 0x1011, 0x1111, 0x1012, 0x1112, 0x1013, 0x1113,
	0x1014, 0x1114, 0x1015, 0x1115, 0x1016, 0x1116, 0x1017, 0x1117,
	0x1018, 0x1118, 0x1019, 0x1119, 0x101A, 0x111A, 0x101B, 0x111B,
	0x101C, 0x111C, 0x101D, 0x111D, 0x101E, 0x111E, 0x101F, 0x111F
};
long start_dot_dp3[FPA_NDATA_REGS][2] = 
{
	dfour, dmone, dtwo, dhalf,
	done, dtwo, dmone, dfour,
	0x1008, 0x1108, 0x1009, 0x1109, 0x100A, 0x110A, 0x100B, 0x110B,
	0x100C, 0x110C, 0x100D, 0x110D, 0x100E, 0x110E, 0x100F, 0x110F,
	dtwo, 0x1011, 0x1111, 0x1012, 0x1112, 0x1013, 0x1113,
	0x1014, 0x1114, 0x1015, 0x1115, 0x1016, 0x1116, 0x1017, 0x1117,
	0x1018, 0x1118, 0x1019, 0x1119, 0x101A, 0x111A, 0x101B, 0x111B,
	0x101C, 0x111C, 0x101D, 0x111D, 0x101E, 0x111E, 0x101F, 0x111F
};
struct opm_table m_table[] =
{
	"2x2 Move", &start1[0][0], &matrix_sp1[0][0], &matrix_dp1[0][0], CS_2MOV, CD_2MOV, ST_DONTCARE,
	"3x3 Move", &start1[0][0], &matrix_sp2[0][0], &matrix_dp2[0][0], CS_3MOV, CD_3MOV, ST_DONTCARE,
	"4x4 Move", &start1[0][0], &matrix_sp3[0][0], &matrix_dp3[0][0], CS_4MOV, CD_4MOV, ST_DONTCARE,
	"2x2 Transpose", &start1[0][0], &matrix_sp4[0][0], &matrix_dp4[0][0], CS_2TRN, CD_2TRN, ST_DONTCARE,
	"3x3 Transpose", &start1[0][0], &matrix_sp5[0][0], &matrix_dp5[0][0], CS_3TRN, CD_3TRN, ST_DONTCARE,
	"4x4 Transpose", &start1[0][0], &matrix_sp6[0][0], &matrix_dp6[0][0], CS_4TRN, CD_4TRN, ST_DONTCARE,
	"2x2 Dot", &start_dot_sp[0][0], &start_dot_sp1[0][0], 0, CS_2DOT, 0, ST_FINITEEXACT,
	"3x3 Dot", &start_dot_sp[0][0], &start_dot_sp2[0][0], 0, CS_3DOT, 0, ST_FINITEEXACT,
	"4x4 Dot", &start_dot_sp[0][0], &start_dot_sp3[0][0], 0, CS_4DOT, 0, ST_FINITEEXACT,
	"2x2 Dot", &start_dot_dp[0][0], 0, &start_dot_dp1[0][0], 0, CD_2DOT, ST_FINITEEXACT,
	"3x3 Dot", &start_dot_dp[0][0], 0, &start_dot_dp2[0][0], 0, CD_3DOT, ST_FINITEEXACT,
	"4x4 Dot", &start_dot_dp[0][0], 0, &start_dot_dp3[0][0], 0, CD_4DOT, ST_FINITEEXACT,
	0, 0, 0, 0, 0, 0, 0
};
int ignore_fpe_error = 0;
int got_fpe_error = 0;
struct fpa_device *fpa = (struct fpa_device *)0xE0000000;
int stop_on_error = 1;
int loop_flag = 0;
int quiet_flag = 0;
int verbose_flag = 0;
int got_error = 0;
int fpa_fd;

main(argc, argv)
int argc;
char *argv[];
{
int fpe_error();
int bus_error();
int segv_error();
int dummy;
int i, j;
int do_conditional_flag = 0;
int sp_flag = 0;
int dp_flag = 0;
int x_flag = 0;
int c_flag = 0;
int ck_flag = 0;
int pipe_flag = 0;
int reg_flag = 0;
int shadow_flag = 0;
int status_flag = 0;
int mode_flag = 0;
int imask_flag = 0;
int debug_flag = 0;
int version_flag = 0;
int matrix_flag = 0;
int interactive_flag = 0;
int pass_count = 0;

	for(i = 1; i < argc; i++)
	{
		switch(argv[i][0])
		{
		case '-':
			switch(argv[i][1])
			{
			case 'c':
				stop_on_error = 0;
				break;
			case 'q':
				quiet_flag++;
				break;
			case 'v':
				verbose_flag++;
				break;
			case 'l':
				loop_flag++;	/* currently does nothing */
				break;
			case 't':
				do_conditional_flag++;
				for(j = 2; argv[i][j] != '\0'; j++)
				{
					switch(argv[i][j])
					{
					case 's':
						sp_flag++;
						break;
					case 'd':
						dp_flag++;
						break;
					case 'r':
						reg_flag++;
						break;
					case 'x':
						x_flag++;
						break;
					case 'c':
						c_flag++;
						break;
					case 't':
						status_flag++;
						break;
					case 'M':
						matrix_flag++;
						break;
					case 'm':
						mode_flag++;
						break;
					case 'i':
						imask_flag++;
						break;
					case 'S':
						shadow_flag++;
						break;
					case 'p':
						pipe_flag++;
						break;
					case 'C':
						ck_flag++;
						break;
					case 'u':
						debug_flag++;
						break;
					case 'v':
						version_flag++;
						break;
					case 'I':
						interactive_flag++;
						break;
					case 'h':
					default:
						printf("Bad test specifier: %c\n", argv[i][j]);
						printf("s - single precision\n");
						printf("d - double precision\n");
						printf("r - registers\n");
						printf("x - extended format\n");
						printf("c - command format\n");
						printf("t - status\n");
						printf("m - mode\n");
						printf("M - matrix\n");
						printf("i - imask\n");
						printf("u - debug\n");
						printf("v - version\n");
						printf("C - checksum\n");
						printf("p - pipe test\n");
						printf("S - shadow registers\n");
						printf("I - interactive\n");
						exit(-1);
					}
				}
				break;
			default:
				goto usage;
			}
			break;
		case 'h':
		default:
			goto usage;
		}
	}
	if(!do_conditional_flag)
	{
		if(!verbose_flag)
			quiet_flag++;
		version_flag++;
		sp_flag = dp_flag = x_flag = c_flag = 1;
		imask_flag = status_flag = mode_flag = 1;
		matrix_flag = pipe_flag = ck_flag = 1;
	}
	signal(SIGBUS, bus_error);
	signal(SIGFPE, fpe_error);
	signal(SIGSEGV, segv_error);
	if((fpa_fd = open("/dev/fpa", O_RDWR, 0)) == -1)
	{
		perror("can't open /dev/fpa");
		return(2);
	}
	fpa->fp_clear_pipe = 0;		/* just in case */
	init_fpa();
	if(!(fpa->fp_pipe_status & FPA_STABLE))
	{
		printf("couldn't get the pipe stable\n");
		exit(-1);
	}
	if(fpa->fp_pipe_status & FPA_FIRST_V_ACT)
	{
		printf("couldn't get the pipe idle\n");
		exit(-1);
	}
	do
	{
		if(reg_flag)
			reg_test();
		if(sp_flag)
			sp_test();
		if(dp_flag)
			dp_test();
		if(sp_flag)
			sd_test();
		if(x_flag)
			x_test();
		if(c_flag)
			c_test();
		if(ck_flag)
			ck_test();
		if(status_flag)
			status_test();
		if(imask_flag)
			imask_test();
		if(pipe_flag)
			pipe_test();
		if(mode_flag)
			mode_test();
		if(shadow_flag)
			shadow_test();
		if(debug_flag)
			debug_test();
		if(version_flag)
			version_test();
		if(matrix_flag)
			matrix_test();
		if(interactive_flag)
			interactive_test();
		if(!quiet_flag && loop_flag)
			printf("Pass number %d\n", ++pass_count);
	}
	while(loop_flag);
	exit(0);
usage:
	printf("Usage: test [-q] [-v] [-l] [-c] [-t[sdxcrhtimpuCIMS]] [-h]\n");
	printf("	-q: quiet (no output for successful completion)\n");
	printf("	-v: verbose\n");
	printf("	-l: loop (infinitely)\n");
	printf("	-c: continue (after error)\n");
	printf("	-t: specify tests\n");
	printf("	-h or -th: help\n");
	exit(-1);
	/* NOTREACHED */
}
init_fpa()
{
	c_wrt_fp_op(INIT, 0, 0, 0, 0);
	fpa->fp_restore_mode3_0 = 0x2;
	fpa->fp_imask = 0;
}
/*
 *	Test Single Precision Format Instructions
 */
sp_test()
{
int i, j;
long temp;
long stat;

	if(!quiet_flag)
		printf("Starting  Test of Single Precision Operations\n");
	init_fpa();
	for(fpa->fp_imask &= ~FPA_INEXACT, j = 0; j < 2; j++ , fpa->fp_imask |= FPA_INEXACT)
	{
		for(i = 0; sp_table[i].name; i++)
		{
			if(verbose_flag)
				printf("  Testing single precision %s\n", sp_table[i].name);
			wrt_fp_op(PUT_REG_SP, 0, sp_table[i].r1);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(sp_table[i].op, 0, sp_table[i].data);
			stat = ck_stat(sp_table[i].status);
			if(stat != sp_table[i].status)
			{
				printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", sp_table[i].name, sp_table[i].status, stat);
				if(stop_on_error)
					exit(-1);
			}
			rd_fp_op(GET_REG_SP, 0, &temp);
			ck_stat(ST_DONTCARE);
			if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
			{
				if(temp != sp_table[i].r1)
				{
					printf("SP Inexact Restore Failure: %s\n", sp_table[i].name);
					printf("    Expected: %08lx, Returned: %08lx\n", sp_table[i].r1, temp);
				}
			} else
			{
				if(temp != sp_table[i].result)
				{
					printf("SP Comparison Failure: %s\n", sp_table[i].name);
					printf("    Expected: %08lx, Returned: %08lx\n", sp_table[i].result, temp);
					if(stop_on_error)
						exit(-1);
				}
			}
		}
	}
	if(!quiet_flag)
		printf("Finishing Test of Single Precision Operations\n");
}
/*
 *	Test Double Precision Format Instructions
 */
dp_test()
{
int i, j;
long temp1, temp2;
long stat;

	if(!quiet_flag)
		printf("Starting  Test of Double Precision Operations\n");
	init_fpa();
	for(fpa->fp_imask &= ~FPA_INEXACT, j = 0; j < 2; j++ , fpa->fp_imask |= FPA_INEXACT)
	{
		for(i = 0; dp_table[i].name; i++)
		{
			if(verbose_flag)
				printf("  Testing double precision %s\n", dp_table[i].name);
			wrt_fp_op(PUT_REG_MS, 0, dp_table[i].r1_ms);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 0, dp_table[i].r1_ls);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(dp_table[i].op1, 0, dp_table[i].data_ms);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(dp_table[i].op2, 0, dp_table[i].data_ls);
			stat = ck_stat(dp_table[i].status);
			if(stat != dp_table[i].status)
			{
				printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", dp_table[i].name, dp_table[i].status, stat);
				if(stop_on_error)
					exit(-1);
			}
			rd_fp_op(GET_REG_MS, 0, &temp1);
			rd_fp_op(GET_REG_LS, 0, &temp2);
			ck_stat(ST_DONTCARE);
			if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
			{
				if((temp1 != dp_table[i].r1_ms) || (temp2 != dp_table[i].r1_ls))
				{
got_error++;
					printf("DP Inexact Restore Failure: %s\n", dp_table[i].name);
					printf("    Expected: %08lx(ms), %08lx(ls)\n", dp_table[i].r1_ms, dp_table[i].r1_ls);
					printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
					if(stop_on_error)
						exit(-1);
				}
			} else
			{
				if((temp1 != dp_table[i].result_ms) || (temp2 != dp_table[i].result_ls))
				{
got_error++;
					printf("DP Comparison Failure: %s\n", dp_table[i].name);
					printf("    Expected: %08lx(ms), %08lx(ls)\n", dp_table[i].result_ms, dp_table[i].result_ls);
					printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
					if(stop_on_error)
						exit(-1);
				}
			}
		}
	}
	if(!quiet_flag)
		printf("Finishing Test of Double Precision Operations\n");
}
/*
 *	Test Single Precision Format Convert Instructions
 */
sd_test()
{
int i;
long temp1, temp2;
long stat;

	if(!quiet_flag)
		printf("Starting  Test of Single Precision Convert Operations\n");
	init_fpa();
	for(i = 0; sd_table[i].name; i++)
	{
		if(verbose_flag)
			printf("  Testing single precision %s\n", sd_table[i].name);
		wrt_fp_op(PUT_REG_MS, 0, sd_table[i].r1_ms);
		ck_stat(ST_DONTCARE);
		wrt_fp_op(PUT_REG_LS, 0, sd_table[i].r1_ls);
		ck_stat(ST_DONTCARE);
		wrt_fp_op(sd_table[i].op, 0, sd_table[i].data);
		stat = ck_stat(sd_table[i].status);
		if(stat != sd_table[i].status)
		{
			printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", sd_table[i].name, sd_table[i].status, stat);
			if(stop_on_error)
				exit(-1);
		}
		rd_fp_op(GET_REG_MS, 0, &temp1);
		rd_fp_op(GET_REG_LS, 0, &temp2);
		if((temp1 != sd_table[i].result_ms) || (temp2 != sd_table[i].result_ls))
		{
			printf("SP Comparison Failure: %s\n", sd_table[i].name);
			printf("    Expected: %08lx(ms), %08lx(ls)\n", sd_table[i].result_ms, sd_table[i].result_ls);
			printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
			if(stop_on_error)
				exit(-1);
		}
	}
	if(!quiet_flag)
		printf("Finishing Test of Single Precision Convert Operations\n");
}
/*
 *	Test Extended Format Instructions
 */
x_test()
{
int i, j;
long temp, temp1, temp2;
long stat;

	if(!quiet_flag)
		printf("Starting  Test of Single Precision Extended Operations\n");
	init_fpa();
	for(fpa->fp_imask &= ~FPA_INEXACT, j = 0; j < 2; j++ , fpa->fp_imask |= FPA_INEXACT)
	{
		for(i = 0; x_table[i].name; i++)
		{
			if(x_table[i].op_sp == 0)
				continue;
			if(verbose_flag)
				printf("  Testing single precision %s\n", x_table[i].name);
			wrt_fp_op(PUT_REG_MS, 1, x_table[i].r1_sp);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 1, 0x55555555);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_MS, 2, x_table[i].r2_sp);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 2, 0xaaaaaaaa);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_MS, 3, x_table[i].r3_sp);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 3, 0xaaaaaaaa);
			ck_stat(ST_DONTCARE);
			x_wrt_fp_op(x_table[i].op_sp, 1, 2, 3, x_table[i].data_sp, 0xeeeeeeee);
			stat = ck_stat(x_table[i].status);
			if(stat != x_table[i].status)
			{
				printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", x_table[i].name, x_table[i].status, stat);
				if(stop_on_error)
					exit(-1);
			}
			rd_fp_op(GET_REG_MS, 1, &temp1);
			rd_fp_op(GET_REG_LS, 1, &temp2);
			if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
			{
				if((temp1 != x_table[i].r1_sp) || (temp2 != 0x55555555))
				{
					printf("SP Comparison Failure: %s\n", x_table[i].name);
					printf("    Expected: %08lx(ms), %08lx(ls)\n", x_table[i].r1_sp, 0x55555555);
					printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
					if(stop_on_error)
						exit(-1);
				}
			} else
			{
				if((temp1 != x_table[i].result_sp) || (temp2 != 0x55555555))
				{
					printf("SP Comparison Failure: %s\n", x_table[i].name);
					printf("    Expected: %08lx(ms), %08lx(ls)\n", x_table[i].result_sp, 0x55555555);
					printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
					if(stop_on_error)
						exit(-1);
				}
			}
		}
	}
	if(!quiet_flag)
		printf("Finishing Test of Single Precision Extended Operations\n");
	if(!quiet_flag)
		printf("Starting  Test of Double Precision Extended Operations\n");
	for(fpa->fp_imask &= ~FPA_INEXACT, j = 0; j < 2; j++ , fpa->fp_imask |= FPA_INEXACT)
	{
		for(i = 0; x_table[i].name; i++)
		{
			if(x_table[i].op_dp == 0)
				continue;
			if(verbose_flag)
				printf("  Testing double precision %s\n", x_table[i].name);
			wrt_fp_op(PUT_REG_MS, 1, x_table[i].r1_ms);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 1, x_table[i].r1_ls);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_MS, 2, x_table[i].r2_ms);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 2, x_table[i].r2_ls);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_MS, 3, x_table[i].r3_ms);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 3, x_table[i].r3_ls);
			ck_stat(ST_DONTCARE);
			x_wrt_fp_op(x_table[i].op_dp, 1, 2, 3, x_table[i].data_ms, x_table[i].data_ls);
			stat = ck_stat(x_table[i].status);
			if(stat != x_table[i].status)
			{
				printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", x_table[i].name, x_table[i].status, stat);
				if(stop_on_error)
					exit(-1);
			}
			rd_fp_op(GET_REG_MS, 1, &temp1);
			rd_fp_op(GET_REG_LS, 1, &temp2);
			if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
			{
				if((temp1 != x_table[i].r1_ms) || (temp2 != x_table[i].r1_ls))
				{
					printf("DP Comparison Failure: %s\n", x_table[i].name);
					printf("    Expected: %08lx(ms), %08lx(ls)\n", x_table[i].r1_ms, x_table[i].r1_ls);
					printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
					if(stop_on_error)
						exit(-1);
				}
			} else
			{
				if((temp1 != x_table[i].result_ms) || (temp2 != x_table[i].result_ls))
				{
					printf("DP Comparison Failure: %s\n", x_table[i].name);
					printf("    Expected: %08lx(ms), %08lx(ls)\n", x_table[i].result_ms, x_table[i].result_ls);
					printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
					if(stop_on_error)
						exit(-1);
				}
			}
		}
	}
	if(!quiet_flag)
		printf("Finishing Test of Double Precision Extended Operations\n");
	if(!quiet_flag)
		printf("Starting  Test of Extended Format Linpack Operations\n");
	init_fpa();
	for(fpa->fp_imask &= ~FPA_INEXACT, j = 0; j < 2; j++ , fpa->fp_imask |= FPA_INEXACT)
	{
		for(i = 0; l_table[i].name; i++)
		{
			if(l_table[i].op_sp == 0)
				continue;
			if(verbose_flag)
				printf("  Testing linpack %s\n", l_table[i].name);
			wrt_fp_op(PUT_REG_MS, 1, 0x33333333);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 1, 0x44444444);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_MS, 2, l_table[i].r2_sp);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 2, 0xaaaaaaaa);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_MS, 3, 0x55555555);
			ck_stat(ST_DONTCARE);
			wrt_fp_op(PUT_REG_LS, 3, 0xaaaaaaaa);
			ck_stat(ST_DONTCARE);
			x_wrt_fp_op(l_table[i].op_sp, 1, 2, 3, l_table[i].data1_sp, l_table[i].data2_sp);
			stat = ck_stat(l_table[i].status);
			if(stat != l_table[i].status)
			{
				printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", l_table[i].name, l_table[i].status, stat);
				if(stop_on_error)
					exit(-1);
			}
			rd_fp_op(GET_REG_MS, 1, &temp1);
			rd_fp_op(GET_REG_LS, 1, &temp2);
			if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
			{
				if((temp1 != 0x33333333) || (temp2 != 0x44444444))
				{
					printf("SP Comparison Failure: %s\n", l_table[i].name);
					printf("    Expected: %08lx(ms), %08lx(ls)\n", 0x33333333, 0x44444444);
					printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
					if(stop_on_error)
						exit(-1);
				}
			} else
			{
				if((temp1 != l_table[i].result_sp) || (temp2 != 0x44444444))
				{
					printf("SP Comparison Failure: %s\n", l_table[i].name);
					printf("    Expected: %08lx(ms), %08lx(ls)\n", l_table[i].result_sp, 0x44444444);
					printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
					if(stop_on_error)
						exit(-1);
				}
			}
		}
	}
	if(!quiet_flag)
		printf("Finishing Test of Extended Format Linpack Operations\n");
}
/*
 *	Test Command Register Format Instructions
 */
c_test()
{
int i, j, k;
long temp, temp1, temp2;
long stat;

	if(!quiet_flag)
		printf("Starting  Test of Single Precision Command Operations\n");
	init_fpa();
	for(k = 0; k < 2; k++)
	{
		for(fpa->fp_imask &= ~FPA_INEXACT, j = 0; j < 2; j++ , fpa->fp_imask |= FPA_INEXACT)
		{
			for(i = 0; c_table[i].name; i++)
			{
				if(c_table[i].op_sp == 0)
					continue;
				if(verbose_flag)
					printf("  Testing single precision %s\n", c_table[i].name);
				wrt_fp_op(PUT_REG_MS, 1, 0xbbbbbbbb);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 1, 0x66666666);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 2, c_table[i].r2_sp);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 2, 0x66666666);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 3, c_table[i].r3_sp);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 3, 0xaaaaaaaa);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 4, c_table[i].r4_sp);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 4, 0xaaaaaaaa);
				ck_stat(ST_DONTCARE);
					/* for inexact restore, use the same reg for
					   operand and result */
				c_wrt_fp_op(c_table[i].op_sp, 1+k, 2, 3, 4);
				stat = ck_stat(c_table[i].status);
				if(stat != c_table[i].status)
				{
					printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", c_table[i].name, c_table[i].status, stat);
					if(stop_on_error)
						exit(-1);
				}
				rd_fp_op(GET_REG_MS, 1+k, &temp1);
				rd_fp_op(GET_REG_LS, 1+k, &temp2);
				if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
				{
					if((temp1 != (k?c_table[i].r2_sp:0xbbbbbbbb)) || (temp2 != 0x66666666))
					{
						printf("SP Comparison Failure: %s\n", c_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", (k?c_table[i].r2_sp:0xbbbbbbbb), 0x66666666);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				} else
				{
					if((temp1 != c_table[i].result_sp) || (temp2 != 0x66666666))
					{
						printf("SP Comparison Failure: %s\n", c_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", c_table[i].result_sp, 0x66666666);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				}
			}
			for(i = 0; sincos_table[i].name; i++)	/* special loop for sincos instructions */
			{
				if(sincos_table[i].op_sp == 0)
					continue;
				if(verbose_flag)
					printf("  Testing single precision %s\n", sincos_table[i].name);
				wrt_fp_op(PUT_REG_MS, 1, 0xbbbbbbbb);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 1, 0x66666666);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 2, sincos_table[i].r2_sp);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 2, 0x66666666);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 3, sincos_table[i].r3_sp);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 3, 0xaaaaaaaa);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 4, 0xdddddddd);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 4, 0xaaaaaaaa);
				ck_stat(ST_DONTCARE);
					/* for inexact restore, use the same reg for
					   operand and result */
				c_wrt_fp_op(sincos_table[i].op_sp, 1+k, 2, 3, 4);
				stat = ck_stat(sincos_table[i].status);
				if(stat != sincos_table[i].status)
				{
					printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", sincos_table[i].name, sincos_table[i].status, stat);
					if(stop_on_error)
						exit(-1);
				}
				rd_fp_op(GET_REG_MS, 1+k, &temp1);
				rd_fp_op(GET_REG_LS, 1+k, &temp2);
				if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
				{
					if((temp1 != (k?sincos_table[i].r2_sp:0xbbbbbbbb)) || (temp2 != 0x66666666))
					{
						printf("SP Comparison Failure: %s: sin result\n", sincos_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", (k?sincos_table[i].r2_sp:0xbbbbbbbb), 0x66666666);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				} else
				{
					if((temp1 != sincos_table[i].result_sp) || (temp2 != 0x66666666))
					{
						printf("SP Comparison Failure: %s: sin result\n", sincos_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", sincos_table[i].result_sp, 0x66666666);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				}
				rd_fp_op(GET_REG_MS, 4, &temp1);
				rd_fp_op(GET_REG_LS, 4, &temp2);
				if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
				{
					if((temp1 != 0xdddddddd) || (temp2 != 0xaaaaaaaa))
					{
						printf("SP Comparison Failure: %s: cosin result\n", sincos_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", 0xdddddddd, 0xaaaaaaaa);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				} else
				{
					if((temp1 != sincos_table[i].r4_sp) || (temp2 != 0xaaaaaaaa))
					{
						printf("SP Comparison Failure: %s: cosin result\n", sincos_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", sincos_table[i].r4_sp, 0xaaaaaaaa);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				}
			}
		}
	}
	if(!quiet_flag)
		printf("Finishing Test of Single Precision Command Operations\n");
	if(!quiet_flag)
		printf("Starting  Test of Double Precision Command Operations\n");
	for(k = 0; k < 2; k++)
	{
		for(fpa->fp_imask &= ~FPA_INEXACT, j = 0; j < 2; j++ , fpa->fp_imask |= FPA_INEXACT)
		{
			for(i = 0; c_table[i].name; i++)
			{
				if(c_table[i].op_dp == 0)
					continue;
				if(verbose_flag)
					printf("  Testing double precision %s\n", c_table[i].name);
				wrt_fp_op(PUT_REG_MS, 1, 0x77777777);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 1, 0x99999999);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 2, c_table[i].r2_ms);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 2, c_table[i].r2_ls);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 3, c_table[i].r3_ms);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 3, c_table[i].r3_ls);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 4, c_table[i].r4_ms);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 4, c_table[i].r4_ls);
				ck_stat(ST_DONTCARE);
				c_wrt_fp_op(c_table[i].op_dp, 1+k, 2, 3, 4);
				stat = ck_stat(c_table[i].status);
				if(stat != c_table[i].status)
				{
					printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", c_table[i].name, c_table[i].status, stat);
					if(stop_on_error)
						exit(-1);
				}
				rd_fp_op(GET_REG_MS, 1+k, &temp1);
				rd_fp_op(GET_REG_LS, 1+k, &temp2);
				if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
				{
					if((temp1 != (k?c_table[i].r2_ms:0x77777777)) || (temp2 != (k?c_table[i].r2_ls:0x99999999)))
					{
						printf("DP Comparison Failure: %s\n", c_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", (k?c_table[i].r2_ms:0x77777777), (k?c_table[i].r2_ls:0x99999999));
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				} else
				{
					if((temp1 != c_table[i].result_ms) || (temp2 != c_table[i].result_ls))
					{
						printf("DP Comparison Failure: %s\n", c_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", c_table[i].result_ms, c_table[i].result_ls);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				}
			}
			for(i = 0; sincos_table[i].name; i++)	/* for sincos instructions */
			{
				if(sincos_table[i].op_dp == 0)
					continue;
				if(verbose_flag)
					printf("  Testing double precision %s\n", sincos_table[i].name);
				wrt_fp_op(PUT_REG_MS, 1, 0x77777777);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 1, 0x99999999);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 2, sincos_table[i].r2_ms);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 2, sincos_table[i].r2_ls);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 3, sincos_table[i].r3_ms);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 3, sincos_table[i].r3_ls);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_MS, 4, 0xdddddddd);
				ck_stat(ST_DONTCARE);
				wrt_fp_op(PUT_REG_LS, 4, 0x66666666);
				ck_stat(ST_DONTCARE);
				c_wrt_fp_op(sincos_table[i].op_dp, 1+k, 2, 3, 4);
				stat = ck_stat(sincos_table[i].status);
				if(stat != sincos_table[i].status)
				{
					printf("Incorrect Status for %s, Expected: %01lx, Returned: %01lx\n", sincos_table[i].name, sincos_table[i].status, stat);
					if(stop_on_error)
						exit(-1);
				}
				rd_fp_op(GET_REG_MS, 1+k, &temp1);
				rd_fp_op(GET_REG_LS, 1+k, &temp2);
				if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
				{
					if((temp1 != (k?sincos_table[i].r2_ms:0x77777777)) || (temp2 != (k?sincos_table[i].r2_ls:0x99999999)))
					{
						printf("DP Comparison Failure: %s: sin result\n", sincos_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", (k?sincos_table[i].r2_ms:0x77777777), (k?sincos_table[i].r2_ls:0x99999999));
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				} else
				{
					if((temp1 != sincos_table[i].result_ms) || (temp2 != sincos_table[i].result_ls))
					{
						printf("DP Comparison Failure: %s: sin result\n", sincos_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", sincos_table[i].result_ms, sincos_table[i].result_ls);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				}
				rd_fp_op(GET_REG_MS, 4, &temp1);
				rd_fp_op(GET_REG_LS, 4, &temp2);
				if((stat > ST_FINITEINEXACT) || ((stat == ST_FINITEINEXACT) && (fpa->fp_imask & FPA_INEXACT)))
				{
					if((temp1 != 0xdddddddd) || (temp2 != 0x66666666))
					{
						printf("DP Comparison Failure: %s: cosin result\n", sincos_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", 0xdddddddd, 0x66666666);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				} else
				{
					if((temp1 != sincos_table[i].r4_ms) || (temp2 != sincos_table[i].r4_ls))
					{
						printf("DP Comparison Failure: %s: cosin result\n", sincos_table[i].name);
						printf("    Expected: %08lx(ms), %08lx(ls)\n", sincos_table[i].r4_ms, sincos_table[i].r4_ls);
						printf("    Returned: %08lx(ms), %08lx(ls)\n", temp1, temp2);
						if(stop_on_error)
							exit(-1);
					}
				}
			}
		}
	}
	if(!quiet_flag)
		printf("Finishing Test of Double Precision Command Operations\n");
}
/*
 *	Test User Registers
 */
reg_test()
{
int context = 0;
int register_no;
long pattern = 0x55555555;
long context_no;

	if(!quiet_flag)
		printf("Starting  Register Test\n");
	init_fpa();
	/* takes advantage of the fact that context allocation
	   is done round-robin */
	for(context = 0; context < 32; context++)
	{
		for(register_no = 0; register_no < 32; register_no++)
		{
		/* write */
			fpa->fp_data[register_no].fpl_data[0] = (((1<<16) + (context << 24) + register_no) ^ pattern);
			fpa->fp_data[register_no].fpl_data[1] = (((2<<16) + (context << 24) + register_no) ^ pattern);
		/* non-destructive register test */
			*(long *)REG_TEST_N = 0;
			while(!(fpa->fp_pipe_status & FPA_STABLE))
				;
			if(fpa->fp_pipe_status & FPA_HUNG)
			{
				printf("Registers failed non-destructive test\n");
				if(stop_on_error)
					exit(-1);
			}
		/* read */
			if(fpa->fp_data[register_no].fpl_data[0] != (((1<<16) + (context << 24) + register_no) ^ pattern))
			{
				printf("Data miscompare - Wrote: %08lx, Read: %08lx\n", ((context << 24) + register_no) ^ pattern, fpa->fp_data[register_no].fpl_data[0]);
				if(stop_on_error)
					exit(-1);
			}
			if(fpa->fp_data[register_no].fpl_data[1] != (((2<<16) + (context << 24) + register_no) ^ pattern))
			{
				printf("Data miscompare - Wrote: %08lx, Read: %08lx\n", ((context << 24) + register_no) ^ pattern, fpa->fp_data[register_no].fpl_data[1]);
				if(stop_on_error)
					exit(-1);
			}
		}
	/* destructive register test */
		*(long *)REG_TEST_D = 0;
		while(!(fpa->fp_pipe_status & FPA_STABLE))
			;
		if(fpa->fp_pipe_status & FPA_HUNG)
		{
			printf("Registers failed destructive test\n");
			if(stop_on_error)
				exit(-1);
		}
	/* skip to next context */
		close(fpa_fd);
		if((fpa_fd = open("/dev/fpa", O_RDWR, 0)) == -1)
		{
			perror("can't open /dev/fpa");
			return(2);
		}
		if(verbose_flag)
			printf(".");
	}
	if(verbose_flag)
		printf("\n");
	if(!quiet_flag)
		printf("Finishing Register Test\n");
}
/*
 *	Test Generation of all Weitek Status Codes
 */
status_test()
{
int i;
long stat;

	if(!quiet_flag)
		printf("Starting  Weitek Status Test\n");
	init_fpa();
	for(i = 0; i < 16; i++)
	{
		if(i == 4)
			continue;
	/*  the old way */
/* illegal now!
		c_wrt_fp_op(WRITE_WEITEK_STATUS | (i<<2), 0, 0, 0, 0);
		while(!(fpa->fp_pipe_status & FPA_STABLE))
			;
		stat = fpa->fp_wstatus_stable;
		if((stat & FPA_WEITEK_STATUS) != i << FPA_STATUS_SHIFT)
		{
			printf("Status Expected (old): %x, Read: %x\n", i, stat);
		}
		if(fpa->fp_pipe_status & FPA_HUNG)
			fpa->fp_clear_pipe = 0;
 */
	/*  the new way */
		fpa->fp_restore_wstatus = i << FPA_STATUS_SHIFT;
		while(!(fpa->fp_pipe_status & FPA_STABLE))
			;
		stat = fpa->fp_wstatus_stable;
		if((stat & FPA_WEITEK_STATUS) != i << FPA_STATUS_SHIFT)
		{
			printf("Status Expected (new): XXXXX%xXX, Read: %x\n", i, stat);
		}
		if(fpa->fp_pipe_status & FPA_HUNG)
			fpa->fp_clear_pipe = 0;
	}
	/*  test the wstatus valid bit */
	fpa->fp_clear_pipe = 0;
	if(fpa->fp_wstatus_stable & FPA_STATUS_VALID)
		printf("After a clear pipe, wstatus is not valid\n");
	/*  test the unimplimented bit */
	fpa->fp_unimplemented = 0;
	got_fpe_error = 0;
	while(!(fpa->fp_pipe_status & FPA_STABLE))
		if(got_fpe_error)
		{
			printf("got bus error doing unimplemented\n");
			exit(-1);
		}
	if(fpa->fp_pipe_status & FPA_HUNG)
	{
		if(!(fpa->fp_wstatus_stable & FPA_NONEXIST_INSTR))
			printf("After an unimplemented instruction bad wstatus: %08lx\n", fpa->fp_wstatus_stable);
		fpa->fp_clear_pipe = 0;
	} else
	{
		printf("Pipe didn't hang doing an unimplemented instruction\n");
		if(fpa->fp_wstatus_stable & FPA_NONEXIST_INSTR)
			printf("   status ok though\n");
		else
			printf("   bad wstatus: %08lx\n", fpa->fp_wstatus_stable);
	}
	if(!quiet_flag)
		printf("Finishing Weitek Status Test\n");
}
/*
 *	Test Imask Register
 */
imask_test()
{
	if(!quiet_flag)
		printf("Starting  Imask Test\n");
	fpa->fp_imask = 0;
	wrt_fp_op(SP_SQR, 0, ssqrthalf);
	while(!(fpa->fp_pipe_status & FPA_STABLE))
		;
	if(fpa->fp_pipe_status & FPA_HUNG)
	{
		printf("Pipe hung for inexact with mask = 0\n");
		printf("   Weitek status is: %08x\n", fpa->fp_wstatus_stable);
		fpa->fp_clear_pipe = 0;
		if(stop_on_error)
			exit(-1);
	}
	fpa->fp_imask = FPA_INEXACT;
	wrt_fp_op(SP_SQR, 0, ssqrthalf);
	while(!(fpa->fp_pipe_status & FPA_STABLE))
		;
	if(!(fpa->fp_pipe_status & FPA_HUNG))
	{
		printf("Pipe didn't hang for inexact with mask = 1\n");
		printf("   Weitek status is: %08x\n", fpa->fp_wstatus_stable);
		if(stop_on_error)
			exit(-1);
	} else
	{
		fpa->fp_clear_pipe = 0;
	}
	if(!quiet_flag)
		printf("Finishing Imask Test\n");
}
/*
 *	Test Mode Register
 */
mode_test()
{
int temp;

	if(!quiet_flag)
		printf("Starting  Mode Test\n");

	fpa->fp_imask = 0;
	fpa->fp_restore_mode3_0 = 0x1; /* FAST mode */
	ck_stat(ST_DONTCARE);
	wrt_fp_op(PUT_REG_SP, 0, sminsub);
	ck_stat(ST_DONTCARE);
	wrt_fp_op(SP_ADD, 0, 0);
	while(!(fpa->fp_pipe_status & FPA_STABLE))
		;
	if(fpa->fp_pipe_status & FPA_HUNG)
	{
		printf("Pipe hung on op in FAST mode with DNRM operand\n");
		printf("Pipe Status is: %08x\n", fpa->fp_pipe_status);
		printf("Wstatus is: %08x\n", fpa->fp_wstatus_stable);
		printf("Address is: %08x\n", fpa->fp_reg_ust_addr);
		fpa->fp_clear_pipe = 0;
		if(stop_on_error)
			exit(-1);
	} else
	{
		rd_fp_op(GET_REG_SP, 0, &temp);
		ck_stat(ST_DONTCARE);
		if(temp != 0)
		{
			printf("Adding 0 to sminsub FAST returned: %08x\n", temp);
		}
	}
	fpa->fp_restore_mode3_0 = 0x0; /* IEEE mode */
	wrt_fp_op(PUT_REG_SP, 0, sminsub);
	ck_stat(ST_DONTCARE);
	wrt_fp_op(SP_ADD, 0, 0);
	while(!(fpa->fp_pipe_status & FPA_STABLE))
		;
	if(fpa->fp_pipe_status & FPA_HUNG)
	{
		fpa->fp_clear_pipe = 0;
	} else
	{
		printf("Adding 0 to sminsub IEEE didn't hang.\n");
	}
	if(!quiet_flag)
		printf("Finishing Mode Test\n");
}
/*
 *	Test Shadow Registers
 */
shadow_test()
{
long temp1, temp2;

	if(!quiet_flag)
		printf("Starting  Shadow Test\n");
	got_fpe_error = 0;
	fpa->fp_data[0].fpl_data[0] = 0x55555555;
	sleep(1);
	temp1 = fpa->fp_shad[0].fpl_data[0];
	fpa->fp_data[0].fpl_data[1] = 0xAAAAAAAA;
	sleep(1);
	temp2 = fpa->fp_shad[0].fpl_data[1];
	if(temp1 != 0x55555555 || temp2 != 0xAAAAAAAA)
	{
		printf("Shadow miscompare (msw) - Read: %08lx, Wrote: %08lx\n", temp1, 0x55555555);
		printf("Shadow miscompare (lsw) - Read: %08lx, Wrote: %08lx\n", temp2, 0xAAAAAAAA);
		if(stop_on_error)
			exit(-1);
	}


	fpa->fp_data[0].fpl_data[0] = stwo;
	*(long *)SP_DIV = sfour;
	temp1 = fpa->fp_shad[0].fpl_data[0];
	if(temp1 != shalf)
	{
		printf("Shadow miscompare1 - Read: %08lx, Expected: %08lx\n", temp1, shalf);
		if(stop_on_error)
			exit(-1);
	}


	fpa->fp_data[0].fpl_data[0] = stwo;
	fpa->fp_data[1].fpl_data[0] = seight;
	*(long *)SP_DIV = sfour;
	*(long *)(SP_DIV | (1 << 3)) = sfour;
	temp1 = fpa->fp_shad[1].fpl_data[0];
	if(temp1 != stwo)
	{
		printf("Shadow miscompare2 - Read: %08lx, Expected: %08lx\n", temp1, stwo);
		if(stop_on_error)
			exit(-1);
	}


	if(!quiet_flag)
		printf("Finishing Shadow Test\n");

}
/*
 *	This test is used for debugging specific problems.
 *	Change code to fit current problem.
 */
debug_test()
{
int temp, i;

	if(!quiet_flag)
		printf("Starting  Debug Test\n");
	while(1)
	{
		for(i = 0; i < 16; i++)
		{
			*(long *)0xE00008D0 = i;
			sleep(1);
			temp = *(long *)0xE0000F38;
			temp &= 0xF;
			if(i != temp)
				printf("MODE -- wrote: %x, read: %x\n", i, temp);
			close(fpa_fd);
			sleep(1);
			if((fpa_fd = open("/dev/fpa", O_RDWR, 0)) == -1)
			{
				perror("can't open /dev/fpa");
				return(2);
			}
		}
	}
}
/*
 *	Test Matrix Instructions
 */
matrix_test()
{
int i;
long temp, temp1, temp2;
long stat;

	if(!quiet_flag)
		printf("Starting  Test of Single Precision Matrix Operations\n");
	for(i = 0; m_table[i].name; i++)
	{
		if(m_table[i].op_sp == 0)
			continue;
		if(verbose_flag)
			printf("  Testing single precision %s\n", m_table[i].name);
		fill(m_table[i].start);
		c_wrt_fp_op(m_table[i].op_sp, 0x10, 0x0, 4, 4);
		stat = ck_stat(m_table[i].status);
		ckfill(m_table[i].result_sp);
	}
	if(!quiet_flag)
		printf("Finishing Test of Single Precision Matrix Operations\n");
	if(!quiet_flag)
		printf("Starting  Test of Double Precision Matrix Operations\n");
	for(i = 0; m_table[i].name; i++)
	{
		if(m_table[i].op_dp == 0)
			continue;
		if(verbose_flag)
			printf("  Testing double precision %s\n", m_table[i].name);
		fill(m_table[i].start);
		c_wrt_fp_op(m_table[i].op_dp, 0x10, 0x0, 4, 4);
		stat = ck_stat(m_table[i].status);
		ckfill(m_table[i].result_dp);
	}
	if(!quiet_flag)
		printf("Finishing Test of Double Precision Matrix Operations\n");
}
fill(regs)
long regs[FPA_NDATA_REGS][2];
{
int i;

	for(i = 0; i < FPA_NDATA_REGS; i++)
	{
		wrt_fp_op(PUT_REG_MS, i, regs[i][0]);
		ck_stat(ST_DONTCARE);
		wrt_fp_op(PUT_REG_LS, i, regs[i][1]);
		ck_stat(ST_DONTCARE);
	}
}
ckfill(regs)
long regs[FPA_NDATA_REGS][2];
{
int i;
long temp1, temp2;

	for(i = 0; i < FPA_NDATA_REGS; i++)
	{
		rd_fp_op(GET_REG_MS, i, &temp1);
		ck_stat(ST_DONTCARE);
		rd_fp_op(GET_REG_LS, i, &temp2);
		ck_stat(ST_DONTCARE);
		if(temp1 != regs[i][0])
			printf("  Miscompare (ms) - Reg: %02x, Read: %08lx, Expected: %08lx\n", i, temp1, regs[i][0]);
		if(temp2 != regs[i][1])
			printf("  Miscompare (ls) - Reg: %02x, Read: %08lx, Expected: %08lx\n", i, temp2, regs[i][1]);
	}
}
/*
 *	Provides a mechanism for the user to interactively modify FPA
 *	registers. (E.g. the user types "c00 r" and the contents of
 *	0xE0000C00 will be Read; typing "c00 15" will set 0xE0000c00
 *	to 0x15).
 */
interactive_test()
{
long add, data;
int no;
char string[10];

	*(long *)0xe0000c00 = 0;	/* to prime fpa (after hard reset)*/
	while(1)
	{
		printf("Enter Address and Data: ");
		no = scanf("%lx %lx", &add, &data);
		if(no == EOF)
			return(0);
		if(no == 0)
		{
			scanf("%s", string);
			printf("illegal command\n");
			continue;
		}
		if(add >= 0x2000)
		{
			printf("illegal address\n");
			scanf("%s", string);
			continue;
		}
		add += 0xe0000000;
		if(no == 2)
		{
			*(long *)add = data;
			printf("Address: %08lx\n", add);
			printf("Wrote: %08lx\n", data);
		} else if(no == 1)
		{
			data = *(long *)add;
			printf("Address: %08lx\n", add);
			printf("Read: %08lx\n", data);
			scanf("%s", string);
		}
	}
}
	
/*	Restore Test
 * since writing over the read register is the fullest test
 * of all restore logic, all tests will use the same register
 * for the operand and the result.
 */
restore_test()
{

}
/*
 *	Write a FPA operation.
 */
wrt_fp_op(op, reg, data)
long op;
long reg;
long data;
{
	got_fpe_error = 0;
	op += (reg<<3);
	*(long *)op = data;
	if(!ignore_fpe_error)
		if(got_fpe_error)
		{
			printf("FPE error: write, op = %08lx, reg = %d\n", op, reg);
			exit(-1);
		}
}
/*
 *	Check the FPA status.
 */
ck_stat(stat)
long stat;
{
int i;

	while(!(fpa->fp_pipe_status & FPA_STABLE))
		;
	if(((fpa->fp_wstatus_stable & FPA_WEITEK_STATUS) >> FPA_STATUS_SHIFT) > (3 - (fpa->fp_imask & 0x1)))
	{
		if(!(fpa->fp_pipe_status & FPA_HUNG))
		{
			printf("Pipe DIDN'T hang after doing a: %08lx\n", fpa->fp_pipe_act_instr);
			printf("Pipe Status is: %08x\n", fpa->fp_pipe_status);
			printf("Wstatus is: %08x\n", fpa->fp_wstatus_stable);
			printf("Address is: %08x\n", fpa->fp_reg_ust_addr);
			if(stop_on_error)
				exit(-1);
		} else
		{
			stat = fpa->fp_wstatus_stable & FPA_WEITEK_STATUS;
			fpa->fp_clear_pipe = 0;		/* just in case */
		}
		return(stat);
	}
	else
	{
		if(fpa->fp_pipe_status & FPA_HUNG)
		{
			printf("Pipe hung after doing a: %08lx\n", fpa->fp_pipe_act_instr);
			printf("Pipe Status is: %08x\n", fpa->fp_pipe_status);
			printf("Wstatus is: %08x\n", fpa->fp_wstatus_stable);
			printf("Address is: %08x\n", fpa->fp_reg_ust_addr);
			fpa->fp_clear_pipe = 0;		/* just in case */
			if(stop_on_error)
				exit(-1);
		}
	}
	if(stat != ST_DONTCARE)
		return(fpa->fp_wstatus_stable & FPA_WEITEK_STATUS);
	else
		return(stat);
}
/*
 *	Write an extended format instruction.
 */
x_wrt_fp_op(op, reg1, reg2, reg3, data_ms, data_ls)
long op;
long reg1, reg2, reg3;
long data_ms, data_ls;
{
long op2;

	got_fpe_error = 0;
	op += (reg2 << 3);
	*(long *)op = data_ms;
	op2 = X_2ND + (reg3 << 7) + (reg1 << 3); 
	*(long *)op2 = data_ls;
	if(!ignore_fpe_error)
		if(got_fpe_error)
		{
			printf("FPE error: x_write, op = %08lx, reg1 = %d, reg2 = %d, reg3 = %d\n", op, reg1, reg2, reg3);
			exit(-1);
		}
}
/*
 *	Write a command register format instruction.
 */
c_wrt_fp_op(op, reg1, reg2, reg3, reg4)
long op;
int reg1, reg2, reg3, reg4;
{
	got_fpe_error = 0;
	*(long *)op = reg1 + (reg2 << 6) + (reg3 << 16) + (reg4 << 26);
	if(!ignore_fpe_error)
		if(got_fpe_error)
		{
			printf("FPE error: write, op = %08lx, reg1 = %d, reg2 = %d, reg3 = %d, reg4 = %d\n", op, reg1, reg2, reg3, reg4);
			exit(-1);
		}
}
/*
 *	Read an FPA operation.
 */
rd_fp_op(op, reg, dataptr)
long op;
long reg;
long *dataptr;
{
	got_fpe_error = 0;
	op += (reg << 3);
	*dataptr = *(long *)op;
	if(!ignore_fpe_error)
		if(got_fpe_error)
		{
			printf("FPE error: read, op = %08lx, reg = %d\n", op, reg);
			printf("The IERR register reads: %08lx\n", fpa->fp_ierr);
			exit(-1);
		}
}
fpe_error()
{
	signal(SIGFPE, fpe_error);
	got_fpe_error++;
	if(got_fpe_error > 1000)
	{
		printf("The WSTATUS register reads: %08lx\n", fpa->fp_wstatus_stable);
		printf("The PIPE STATUS register reads: %08lx\n", fpa->fp_pipe_status);
		printf("The IERR register reads: %08lx\n", fpa->fp_ierr);
		printf("got many fpe's\n");
		exit(-1);
	}
}
bus_error()
{
	signal(SIGBUS, bus_error);
	printf("Got a bus error\n");
	exit(-1);
}
segv_error()
{
	signal(SIGSEGV, segv_error);
	printf("Got a segment violation\n");
	exit(-1);
}
/*
 *	Test Checksum in Constant RAM
 */
ck_test()
{
long cksum;
int const_add;

	if(!quiet_flag)
		printf("Starting  Checksum Test\n");
	cksum = 0;
	for(const_add = 0x400; const_add < 0x7d0; const_add++)
	{
		fpa->fp_load_ptr = const_add << 2;
		cksum ^= *(long *)LDP_MS;
	}
	if(verbose_flag)
		printf("checksum (msw): %08lx\n", cksum);
	fpa->fp_load_ptr = 0x7ff << 2;
	cksum ^= *(long *)LDP_MS;
	if(cksum != 0xbeadface)
		printf("checksum error (msw): %08lx\n", cksum);
	cksum = 0;
	for(const_add = 0x400; const_add < 0x7d0; const_add++)
	{
		fpa->fp_load_ptr = const_add << 2;
		cksum ^= *(long *)LDP_LS;
	}
	if(verbose_flag)
		printf("checksum (lsw): %08lx\n", cksum);
	fpa->fp_load_ptr = 0x7ff << 2;
	cksum ^= *(long *)LDP_LS;
	if(cksum != 0xbeadface)
		printf("checksum error (lsw): %08lx\n", cksum);
	if(!quiet_flag)
		printf("Finishing Checksum Test\n");
}
/*
 *	Test Sequential Issuance of Instructions to Full Pipe.
 */
pipe_test()
{
long temp1, temp2;

	if(!quiet_flag)
		printf("Starting  Pipe Test\n");
	init_fpa();
	wrt_fp_op(PUT_REG_MS, 1, 0x40600000);
	wrt_fp_op(PUT_REG_LS, 1, 0x00000000);
	*(long *)CD_EXP = 0x41;
	*(long *)CD_LN = 0x41;
	*(long *)CD_LN = 0x41;
	*(long *)CD_EXP = 0x41;
	rd_fp_op(GET_REG_MS, 1, &temp1);
	rd_fp_op(GET_REG_LS, 1, &temp2);
	if(temp1 != 0x405fffff || temp2 != 0xfffffffe)
	{
		printf("Bad pipe sequencing\n");
		printf("	Expected (msw): %08lx; Returned: %08lx\n", 0x405fffff, temp1);
		printf("	Expected (lsw): %08lx; Returned: %08lx\n", 0xfffffffe, temp2);
	}
	if(!quiet_flag)
		printf("Finishing Pipe Test\n");
}
/*
 *	Print version numbers.
 */
version_test()
{
	*(long *)REG_CPY_DP = 0xfc00;
	printf("Microcode = level: %lx, date: %06lx\n", *(long *)GET_REG_MS, *(long *)GET_REG_LS);
       	*(long *)REG_CPY_DP = 0xfc40;
	printf("Constants = level: %lx, date: %06lx\n", *(long *)GET_REG_MS, *(long *)GET_REG_LS);
	printf("**** FPA Installed and Active ****\n");
}
