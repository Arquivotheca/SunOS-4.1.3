static char     fmhsccsid[] = "@(#)fpa.h 1.1 92/07/30 SMI";

#include "fpa.def.h"

/*
 * ========================================
 * sundiag related standard define
 * ========================================
 */

#ifdef SOFT
#define Device                  "softfp"
#define TEST_NAME               "softfp"
#endif

/*******/
#ifdef sun3
#ifdef FPA
#define Device                  "FPA"
#define TEST_NAME               "fpatest"

#ifndef FPA_VERSION
#define FPA_VERSION     0xE     /* fpa version in imask/version register */
#endif FPA_VERSION

#define FPA_TYPE        0       /* original fpa board */
#define FPA3X_TYPE      1       /* fpa-3x */
#endif

#ifdef MC68881
#define Device                  "MC68881"
#define TEST_NAME               "mc68881"
#endif
#endif sun3
/*******/

/*******/
#ifdef sun4
#ifdef FPU
#define Device                  "FPU"
#define TEST_NAME               "fputest"
#endif FPU
#endif sun4
/*******/
/* 
 * ========================================
 * pointer definition of instruction set
 * ========================================
 */

#define FPA_REG_LACC	0xE84
#define FPA_REG_MACC	0xE80
#define FPA_REG_LACC_RR	0xE8C
#define FPA_REG_MACC_RR 0xE88
#define FPA_RW_LREG	(FPA_BASE+FPA_REG_LACC)
#define FPA_RW_MREG     (FPA_BASE+FPA_REG_MACC)
#define FPA_RW_LREG_RR  (FPA_BASE+FPA_REG_LACC_RR)
#define FPA_RW_MREG_RR  (FPA_BASE+FPA_REG_MACC_RR)

#define FPA_RAM_ACC	(FPA_BASE+FPA_LOAD_PTR)
#define FPA_RW_RAM	(FPA_BASE+FPA_LD_RAM)
#define FPA_STATE_PTR	(FPA_BASE+FPA_STATE)
#define FPA_IERR_PTR	(FPA_BASE+FPA_IERR)
#define FPA_IMASK_PTR	(FPA_BASE+FPA_IMASK)
#define FPA_CLEAR_PIPE_PTR  (FPA_BASE+FPA_CLEAR_PIPE)
#define MAXSIZE_CON_RAM 0x7CF
/*
 * ========================================
 * index definition of rams/registers 
 * ========================================
 */

#define ALL		0
#define MAPPING_RAM 	1
#define USTORE_RAM 	2
#define REGISTER_RAM	3
#define LOAD_PTR_REG	4
#define IERR_REG	5
#define IMASK_REG	6
#define STATE_REG	7
#define LD_RAM_REG	8
#define MAPPING_USTORE_RAM	9
#define JUST_ADDRESS    10	

#define STATE_REG_MASK  0xE0
#define PROBE_DEEP	10
#define PTR_SHIFT	2
#define MR_LB		3
#define MR_DMASK	0x00ffffff
#define UR_LBH		0
#define UR_LBM		1
#define UR_LBL		2
#define LOW_USTORE	1
#define MEDIUM_USTORE	2
#define HIGH_USTORE	3

#define LOW_REGRAM	1
#define HIGH_REGRAM	2

#define UR_DMASK	0x000000ff
#define LPTR_MASK	0x00003fff
#define IERR_MASK	0x00ff0000

#define NON_MASK	0xffffffff
#define RR_LB		0
#define MARK		0xff

#define CNT_ROTATE	32
#define ROTATE_DATA	0xd2210408
#define MAXSIZE_REG_RAM	2048
#define MAXSIZE_USTORE_RAM	4096
#define MAXSIZE_MAP_RAM	4096
#define LSLW		1
#define MSLW		0
#define HSLW		2
#define ZERO_BACKGROUND	0x0
#define ONE_BACKGROUND	0xffffffff
#define WALKING_ZERO	0x0
#define WALKING_ONE	0xffffffff

#define TEST_PTR(x,y)	(x << PTR_SHIFT) | y

							   
