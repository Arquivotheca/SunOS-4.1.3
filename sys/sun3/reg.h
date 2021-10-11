/*      @(#)reg.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _sun3_reg_h
#define _sun3_reg_h

#include <m68k/reg.h>
/*
 * Struct for floating point registers and general state
 * for the MC68881 (the sky fpp has no user visible state).
 * If fps_flags == FPS_UNUSED, the other 68881 fields have no meaning.
 * fps_flags is a software implemented field; it is not used when set
 * by user level programs.
 */

#ifndef LOCORE
typedef	struct ext_fp {
	int	fp[3];
} ext_fp;		/* extended 96-bit 68881 fp registers */

struct fp_status {
	ext_fp	fps_regs[8];		/* 68881 floating point regs */
	int	fps_control;		/* 68881 control reg */
	int	fps_status;		/* 68881 status reg */
	int	fps_iaddr;		/* 68881 instruction address reg */
	int	fps_flags;		/* r/o - unused, idle or busy */
};

/*
 * Values defined for `fps_flags'.
 */
#define	FPS_UNUSED	0		/* 68881 never used yet */
#define	FPS_IDLE	1		/* 68881 instruction completed */
#define	FPS_BUSY	2		/* 68881 instruction interrupted */

/*
 * The EXT_FPS_FLAGS() macro is used to convert a pointer to an
 * fp_istate into a value to be used for the user visible state
 * found in fps_flags.  As a speed optimization, this convertion
 * is only done is required (e.g.  the PTRACE_GETFPREGS ptrace
 * call or when dumping core) instead of on each context switch.
 * The tests that we base the state on are that a fpis_vers of
 * FPIS_VERSNULL means NULL state, else a fpis_bufsiz of FPIS_IDLESZ
 * means IDLE state, else we assume BUSY state.
 */
#define	FPIS_VERSNULL	0x0
#define	FPIS_IDLESIZE	0x18

#define EXT_FPS_FLAGS(istatep) \
	((istatep)->fpis_vers == FPIS_VERSNULL ? FPS_UNUSED : \
	    (istatep)->fpis_bufsiz == FPIS_IDLESIZE ? FPS_IDLE : FPS_BUSY)

/* 
 * Structures for the status and data registers are defined here.
 * Struct fpa_status are included in the u area.
 * Struct fpa_regs is included in struct core.
 */

/* struct fpa_status is saved/restored during context switch */
struct fpa_status {
	unsigned int	fpas_state;	/* STATE, supervisor privileged reg */
	unsigned int	fpas_imask;	/* IMASK */
	unsigned int	fpas_load_ptr;	/* LOAD_PTR */
	unsigned int	fpas_ierr;	/* IERR */
	unsigned int	fpas_act_instr; /* pipe active instruction halves */
	unsigned int	fpas_nxt_instr; /* pipe next instruction halves */
	unsigned int	fpas_act_d1half;/* pipe active data first half */
	unsigned int	fpas_act_d2half;/* pipe active data second half */
	unsigned int	fpas_nxt_d1half;/* pipe next data first half */
	unsigned int	fpas_nxt_d2half;/* pipe next data second half */
	unsigned int	fpas_mode3_0;	/* FPA MODE3_0 register */
	unsigned int	fpas_wstatus;	/* FPA WSTATUS register */
};

/* 
 * Since there are 32 contexts supported by the FPA hardware,
 * when we do context switch on the FPA, we don't save/restore
 * the data registers between the FPA and the u area.
 * If there are already 32 processes using the fpa concurrently,
 * we give an error message to the 33rd process trying to use the fpa.
 * (Hopefully there will not be this many processes using FPA concurrently.)
 */

#define FPA_NCONTEXTS		32
#define FPA_NDATA_REGS		32

typedef struct fpa_long {
	int     fpl_data[2];
} fpa_long;		 /* 64 bit double precision registers */

/* Struct fpa_regs is included in struct core. */
struct fpa_regs {
	unsigned int	fpar_flags; /* if zero, other fields are meaningless */
        struct fpa_status	fpar_status;
        fpa_long	fpar_data[FPA_NDATA_REGS];
};

/*
 * The size of struct fpa_regs is changed from 141 ints in 3.0 to
 * 77 ints in 3.x.  A pad of this size difference is added to struct core.
 */
#define CORE_PADLEN	64

/* 
 * If there is going to be external FPU state then we must define the FPU 
 * variable
 */
struct fpu {
	struct 	fp_status f_fpstatus;	/* External FPP state, if any */
	struct  fpa_regs f_fparegs;	/* FPA registers, if any */
	int	f_pad[CORE_PADLEN];	/* see comment above */
};
#define ip_fpa_flags	ip_fpu.f_fparegs.fpar_flags
#define ip_fpa_state	ip_fpu.f_fparegs.fpar_status.fpas_state
#define ip_fpa_status	ip_fpu.f_fparegs.fpar_status
#define ip_fpa_data	ip_fpu.f_fparegs.fpar_data
#endif !LOCORE

/*
 * Defining FPU means that there is externel floating point state 
 * available to the user.  This state could be either the 68881 or
 * the fpa.  We do not make that distinction here.  This variable
 * cause all the appropriate code to be included.  It is defined 
 * here instead of in the makefile since we do not know for sure 
 * that an 68881 or fpa will be in the system.  Be prepared.
 */
#ifdef KERNEL
#define FPU
#endif KERNEL

#endif /*!_sun3_reg_h*/
