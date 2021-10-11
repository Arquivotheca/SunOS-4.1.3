/* static char sccsid[] = "@(#)fpa3x.h 1.1 7/30/92 Copyright Sun Microsystems";*/

/*
 * ****************************************************************************
 * Source File     : fparel.h
 * Original Engr   : Nancy Chow
 * Date            : 10/11/88
 * Function        : This file contains the constant and macro definitions
 *		   : used by the fparel for fpa-3x.
 * Revision #1 Engr: 
 * Date            : 
 * Change(s)       :
 * ****************************************************************************
 */

/* ***********
 * Externals
 * ***********/

extern int	dev_no;			/* file desc of current context */
extern int      errno;			/* system error code */
extern int	no_times;		/* test pass count */
extern int	sig_err_flag;           /* expect or not expect buserr */
extern int	seg_sig_flag;           /* expect or not expect seg violation */
extern int	verbose;		/* verbose message display? */

/* ************
 * Constatnts
 * ************/

#define FAIL		0
#define PASS		1

#define FALSE		0
#define TRUE		1

#define FIRST_BIT	0x1		/* initial context bit mask */
#define MAX_CXTS	32		/* max no. of FPA contexts */
#define MAX_TEST_CNT 	22		/* no. of tests covered */
#define RAM_LH_FAIL	0xFF		/* lower reg ram failure */
#define RAM_LH_ERR	7		/* lower reg ram failure */

#define INS_WRMODE	0xE00008D0	/* mode reg write uroutine address */
#define INS_REGFLP_L	0xE0000E84      /* Register File via LOAD_PTR (least) */
#define INS_REGFLP_M	0xE0000E80      /* Register File via LOAD_PTR (most) */
#define REG_IERR	0xE0000F1C	/* IERR register address */
#define REG_IMASK	0xE0000F14	/* IMASK register address */
#define REG_LDRAM       0xE0000FC0      /* LD_RAM register address */
#define REG_LOADPTR	0xE0000F18	/* LOAD_PTR register address */
#define REG_SCLEARPIPE	0xE0000F84	/* soft clear pipe address */
#define REG_STATE	0xE0000F10	/* STATE register address */
#define REG_WSTATUS_S   0xE0000F3C      /* WSTATUS (stable) register address */

#define CXT_MASK	0x1F		/* mask for context in STATE reg */
#define IERR_BMASK	0xFF0000	/* mask for IERR bits */
#define IERR_SHFT	16		/* shift count for IERR bits */
#define PIPE_HUNG	0x200000	/* IERR pipe hung bit */
#define NOT_HUNG	1		/* pipe not hung for nack test */
#define FPA_BUS		2		/* bus error due to FPA error */
#define BUS_ERR		3		/* bus error */

#define NONE		0		/* no action */
#define LOAD_ON		1		/* set Load Enable */
#define LOAD_OFF	2		/* reset Load Enable */
#define SEG_SET		3		/* expect segmentation violation */
#define SEG_RSET	4		/* don't expect seg violation */
#define SIG_SET		5		/* expect buserror */
#define SIG_RSET	6		/* don't expect buserror */
#define VERSION_FPA	0x0		/* version ID for FPA */
#define VERSION_MASK	0x0E		/* mask for FPA version */ 

#define MIN_REG_RAM	0               /* min reg ram addr */
#define MAX_REG_RAM	(1024 * 2)      /* max reg ram addr */
#define MIN_MAP_RAM     0               /* min map ram addr */
#define MAX_MAP_RAM     (1024 * 4)      /* max map ram addr */
#define MIN_USTORE      (1024 * 4)      /* min ustore addr */
#define MAX_USTORE      16384           /* max ustore addr */
#define MR_CKSUM_H	0		/* XXX Map Ram MSH checksum */
#define MR_CKSUM_L	0		/* XXX Map Ram LSH checksum */ 
#define UR_CKSUM_H	0		/* XXX Ustore MSH checksum */
#define UR_CKSUM_L	0		/* XXX Ustore LSH checksum */
#define CKSUM_XOR	0xBEADFACE	/* checksum XOR value */
#define RR_CKSUM_START	0x400		/* start loc of Reg Ram checksum calc */
#define RR_CKSUM_END	0x7D0		/* end loc of Reg Ram checksum calc */
#define CKSUM_LOC	0x7FF		/* loc of expected checksum value */

#define LD_PTR_SHFT     2               /* shift value for LOAD PTR addr */
#define US_MSH          0               /* most sig half thru LOAD PTR */
#define US_LSH          1               /* least sig half thru LOAD PTR */
#define LD_PTR_ADDR(x)  (x << LD_PTR_SHFT) /* fill LOAD_PTR addr bits */
#define LD_PTR_LSH(x)   ((x << LD_PTR_SHFT) | US_LSH) /* addr LSH thru LOAD PTR */
#define LD_PTR_MSH(x)   ((x << LD_PTR_SHFT) | US_MSH) /* addr MSH thru LOAD PTR */

#define			INS_REGFC_M(x)  (0xE0000C00+(x<<3)) /* Register File context (most) */
#define			INS_REGFC_L(x)  (0xE0000C04+(x<<3)) /* Register File context (least) */
#define			SHAD_REG_M(x)	(0xE0000E00+(x<<3)) /* Shadow Register (most) */
#define			SHAD_REG_L(x)	(0xE0000E04+(x<<3)) /* Shadow Register (least) */

