/*      @(#)fpa.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#define fpa_debug	1

/*
 * This file contains definitions related to the Floating Point
 * Accelerator used for the Sun-3 machines.
 */

#ifdef KERNEL
#include "../machine/reg.h"
#else
#include <machine/reg.h>
#endif

/*
 * open("/dev/fpa", filemode, createmode) returns ENXIO if there is
 * no FPA or 68881 attached. It returns  EBUSY if there is no FPA
 * context available.
 */

/* ioctl commands */
#define FPA_ACCESS_ON		_IO(p, 1)
#define FPA_ACCESS_OFF		_IO(p, 2)
#define FPA_LOAD_ON		_IO(p, 3)
#define FPA_LOAD_OFF		_IO(p, 4)

#define FPA_NSHAD_REGS 8
#define FPA_NDATA_REGS		32
typedef struct fpa_long {
	int     fpl_data[2];
	} fpa_long; /* 64 bit double presition registers */

/*
 * Struct fpa_device defines the control registers in the FPA.
 * The base address of this struct is 0xE0000000.
 * Since FPA control registers can only be accessed in 32 bit,
 * accessing their fields is via "masks" instead of "field".
 * E.g., to check the load enable bit of the STATE register, we use
 * 	if (fpa.state & FPA_LOAD_BIT)
 * to turn on this bit, we use
 * 	fpa.state |= FPA_LOAD_BIT;
 */
struct fpa_device {
	/* 0x000 */
	u_char	fp_filler1[0x8D0];

	/*
	 * Restore MODE3_0 instr: 1000,1101,0000(8D0) in the address line.
	 * Data line is u.u_fpa_status.fpas_mode3_0.
	 */
	u_int	fp_restore_mode3_0;

	/* 0x8D4 */
	u_char	fp_filler2[0x84];

	/* 0x958, write back address of WSTATUS */
	u_int	fp_restore_wstatus;

	/*
	 * Turn on unimplemented instr bit in WSTATUS reg: 1001,0101,1100(95C)
	 * on the address line. Data line is "dont care".
	 */
	u_int	fp_unimplemented;

	/* 0x960 */
	u_char	fp_filler3[0x18];

	/*
	 * Initialize: 1001,0111,1000(978) in the address line.
	 * Data line is "dont care".
	 */
	u_int	fp_initialize;

	/*
	 * Restore shadow regs: 1001,0111,1100(97C) in the address line.
	 * Data line is "dont care".
	 */
	u_int	fp_restore_shadow;

	/* 0x980 */
	u_char fp_filler4[0x280];

	/* 0xC00, data reg[0] */
	fpa_long	fp_data[FPA_NDATA_REGS];

	/* 0xD00 */
	u_char	fp_filler5[0x100];

	/* 0xE00, shadow registers */
	fpa_long	fp_shad[FPA_NSHAD_REGS];

	/* 0xE40 */
	u_char  fp_filler6[0xB8];

	/* 0xEF8 */
	u_char	fp_filler7[0x18];

	/* 0xF10, STATE register */
	u_int	fp_state;
#define FPA_LOAD_BIT		0x80
#define FPA_ACCESS_BIT		0x40
#define FPA_PBITS		0xC0
#define FPA_CONTEXT		0x1F

	/* 0xF14, IMASK register */
	u_int	fp_imask;
#define FPA_INEXACT		0x01

	/* 0xF18, LOAD_PTR register */
	u_int	fp_load_ptr;
#define FPA_RAM_ADDR		0x3FFC
#define FPA_SEG_SELECT		0x03
	/*
	 * Microstore bit segment select, e.g.
	 * if ((fpa.load_ptr & FPA_SEG_SELECT) == FPA_BIT_71_64)
	 */
#define FPA_BIT_71_64		0x00
#define FPA_BIT_63_32		0x01
#define FPA_BIT_31_0		0x02
#define FPA_BIT_23_0		0x03

	/* 0xF1C, IERR register */
	u_int	fp_ierr;
#define FPA_ILLE_CTL_ADDR	0x800000
#define FPA_RETRY		0x400000
#define FPA_HUNG_PIPE		0x200000
#define FPA_ILLEGAL_SEQ		0x100000
#define FPA_EXEC_UCODE		0x080000
#define FPA_ILLEGAL_ACCESS	0x040000
#define FPA_PROTECTION		0x020000
#define FPA_NON32_ACCESS	0x010000

	/* 0xF20, PIPE_ACT_INS register */
	u_int	fp_pipe_act_instr;
	/* 0xF24, PIPE_NXT_INS register */
	u_int	fp_pipe_nxt_instr;
#define FPA_FIRST_V		0x80000000
#define FPA_FIRST_HALF		0x1FFC0000
#define FPA_SECOND_V		0x00008000
#define FPA_SECOND_HALF		0x00001FFC
#define FPA_ADDR_SHIFT		16

	/* 0xF28, 0xF2C, 0xF30, 0xF34, PIPE_XXX_DX registers */
	u_int	fp_pipe_act_d1half;
	u_int	fp_pipe_act_d2half;
	u_int	fp_pipe_nxt_d1half;
	u_int	fp_pipe_nxt_d2half;
	
	/* 0xF38, MODE register */
	u_int	fp_mode_stable;
#define FPA_WEITEK_MODE		0x0F

	/* 0xF3C, WSTATUS register */
	u_int	fp_wstatus_stable;
#define FPA_WEITEK_ERROR	0x08000
#define FPA_NONEXIST_INSTR	0x04000
#define FPA_STATUS_VALID	0x02000
#define FPA_WEITEK_STATUS	0x00F00
#define FPA_DECODED_STATUS	0x0001F
#define FPA_STATUS_SHIFT	8
	/*
	 * Weitek comparison conditions e.g. 
	 * if ((fpa.wstatus_stable & FPA_DECODED_STATUS) == FPA_LT)
	 */
#define FPA_EQ			0x00004
#define FPA_LT			0x00019
#define FPA_GT			0x00000
#define FPA_UNORDERED		0x00002
#define FPA_OTHERS		0x00000

	/* 0xF40 */
	u_char fp_filler8[0x8];

	/* 0xF48, PIPE_STATUS register */
	u_int	fp_pipe_status;
#define FPA_SECOND_V_NXT	0x800000
#define FPA_FIRST_V_NXT		0x400000
#define FPA_SECOND_V_ACT	0x200000
#define FPA_FIRST_V_ACT		0x100000
#define FPA_IDLE2		0x080000
#define FPA_IDLE1		0x040000
#define FPA_HUNG		0x020000
#define FPA_STABLE		0x010000

	/* 0xF4C */
	u_char fp_filler9[0x14];

	/* 0xF60, READ_REG register */
	u_int	fp_read_reg;

	/* 0xF64, REG_UST_ADDR register */
	u_int	fp_reg_ust_addr;
#define FPA_MUX_SELECT		0xE0000000
#define FPA_REGRAM_ADDR		0x0FFF0000
#define FPA_USTORE_ADDR		0x00003FFF

	/* 0xF68 */
	u_char fp_filler10[0x18];

	/* 0xF80, hard CLEAR_PIPE register, only used by diagnostic routines */
	u_int	fp_hard_clear_pipe;

	/* 0xF84, CLEAR_PIPE register, used by Unix */
	u_int	fp_clear_pipe;

	/* 0xF88 */
	u_char	fp_filler11[0x30];

	/* 0xFB8, MODE read clear, fields same as fp_mode3_0_stable (0xF38) */
	u_int	fp_mode3_0_clear;

	/* 0xFBC, WSTATUS read clear, fields same as fp_wstatus_stable(0xF3C) */
	u_int	fp_wstatus_clear;

	/* 0xFC0, LD_RAM register */
	u_int	fp_ld_ram;
} *fpa; /* fpa gets address from fpaprobe() in fpa.c */

#define FPA_MAXLOOP		256
