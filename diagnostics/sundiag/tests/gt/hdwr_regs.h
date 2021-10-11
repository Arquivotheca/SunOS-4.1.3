/*      @(#)hdwr_regs.h 1.1 92/07/30 SMI      */

/*
* Copyright (c) 1991 by Sun Microsystems, Inc.
*/



#define REG(i)	i

#define F_PBM_CSR	0xff080020
#define FBD_VCXR	0xff20008c
#define FBD_VCYR	0xff200094
#define FBD_WID_CUR	0xff200400
#define FBD_WID_CTRL	0xff200404
#define FBD_CONST_Z_SRC	0xff20040c
#define FBD_Z_CTRL	0xff200410
#define FBD_IWMR	0xff200414
#define FBD_WWMR	0xff200418
#define FBD_FC_SET	0xff200434
#define FBD_ROP_OP	0xff200424
#define F_PBM_ASR	0xff08001c
#define BUF_SR		0xff200204
#define XYADDR		0xff080080
#define DATAADDR	0xff080088
#define DEPTHADDR	0xff080094
#define PP_CSR		0xff200434
#define FBD_WLUT_0	0xff318000
#define FBD_WLUT_CSR0	0xff310000
#define FBD_CLUT_0	0xff308000
#define FBD_CLUT_CSR0	0xff300000
#define VIDEO_MODE0	0xff320020
#define VIDEO_MODE1	0xff320024
#define SSA0		0xff320000
#define SSA1		0xff320004
#define LIN_OS		0xff320008
#define HOZREG		0xff320080
#define VERREG		0xff3200c0
#define STATE		0xff080020
#define STEREO		0xff2000c8

#define VIDEO_MEM_START 0xfe000000

#define REGS \
unsigned int f_pbm_csr; \
unsigned int fbd_vcxr; \
unsigned int fbd_vcyr; \
unsigned int fbd_wid_cur; \
unsigned int fbd_wid_ctrl; \
unsigned int fbd_const_z_src; \
unsigned int fbd_z_ctrl; \
unsigned int fbd_iwmr; \
unsigned int fbd_wwmr; \
unsigned int fbd_fc_set; \
unsigned int fbd_rop_op; \
unsigned int f_pbm_asr; \
unsigned int buf_sr;


#define SAVE_REGS \
reg = (unsigned *)(F_PBM_CSR); \
f_pbm_csr = *reg; \
*reg = 1; \
reg = (unsigned *)(FBD_VCXR); \
fbd_vcxr = *reg; \
reg = (unsigned *)(FBD_VCYR); \
fbd_vcyr = *reg; \
reg = (unsigned *)(FBD_WID_CUR); \
fbd_wid_cur = *reg; \
reg = (unsigned *)(FBD_WID_CTRL); \
fbd_wid_ctrl = *reg; \
reg = (unsigned *)(FBD_CONST_Z_SRC); \
fbd_const_z_src = *reg; \
reg = (unsigned *)(FBD_Z_CTRL); \
fbd_z_ctrl = *reg; \
reg = (unsigned *)(FBD_IWMR); \
fbd_iwmr = *reg; \
reg = (unsigned *)(FBD_WWMR); \
fbd_wwmr = *reg; \
reg = (unsigned *)(FBD_FC_SET); \
fbd_fc_set = *reg; \
reg = (unsigned *)(FBD_ROP_OP); \
fbd_rop_op = *reg; \
reg = (unsigned *)(F_PBM_ASR); \
f_pbm_asr = *reg; \
reg = (unsigned *)(BUF_SR); \
buf_sr = *reg;

#define RESTORE_REGS \
reg = (unsigned *)(FBD_VCXR); \
*reg = fbd_vcxr; \
reg = (unsigned *)(FBD_VCYR); \
*reg = fbd_vcyr; \
reg = (unsigned *)(FBD_WID_CUR); \
*reg = fbd_wid_cur; \
reg = (unsigned *)(FBD_WID_CTRL); \
*reg = fbd_wid_ctrl; \
reg = (unsigned *)(FBD_CONST_Z_SRC); \
*reg = fbd_const_z_src; \
reg = (unsigned *)(FBD_Z_CTRL); \
*reg = fbd_z_ctrl; \
reg = (unsigned *)(FBD_IWMR); \
*reg = fbd_iwmr; \
reg = (unsigned *)(FBD_WWMR); \
*reg = fbd_wwmr; \
reg = (unsigned *)(FBD_FC_SET); \
*reg = fbd_fc_set; \
reg = (unsigned *)(FBD_ROP_OP); \
*reg = fbd_rop_op; \
reg = (unsigned *)(F_PBM_ASR); \
*reg = f_pbm_asr; \
reg = (unsigned *)(BUF_SR); \
*reg = buf_sr; \
reg = (unsigned *)(F_PBM_CSR); \
*reg = f_pbm_csr;

typedef struct {
    unsigned int *addr;
    unsigned int expect;
    unsigned int actual;
} RESULT;
