/******************************************************************************
	@(#)fpa3x_dev.c - Rev 1.1 - 7/30/92
	Copyright (c) 1988 by Sun Microsystems, Inc.
	Deyoung Hong

	FPA-3X device specific test functions.
******************************************************************************/

#include <stdio.h>
#include <errno.h>
#include <varargs.h>
#include <sys/types.h>
#include <sundev/fpareg.h>
#include "../../include/sdrtns.h"	/* make sure not from local directory */
#include "fpa3x_def.h"
#include "fpa3x_msg.h"


/* External declarations */
extern struct fpa_device *fpa;
extern int fpa_version;
extern int fd;

 
/* WSTATUS expected value read corresponding to value written 0 thru F */
static u_int wstatexp[] =
{
	WSTAT0, WSTAT1, WSTAT2, WSTAT3, WSTAT4, WSTAT5, WSTAT6, WSTAT7,
	WSTAT8, WSTAT9, WSTATA, WSTATB, WSTATC, WSTATD, WSTATE, WSTATF, 0
};


/* Simple instruction and status test */
static struct
{
	int inst;		/* instruction sequence */
	int stat;		/* expected pipe status */
} simple[] =
{
	{ SP_NOP,		FPA_PIPE_CLEAR				    },
	{ DP_NOP,		FPA_PIPE_CLEAR | FPA_WAIT2 | FPA_IDLE2 |
							    FPA_FIRST_V_ACT },
	{ DP_LSW,		FPA_PIPE_CLEAR				    },
	{ INS_UNIMPLEMENT,	FPA_PIPE_CLEAR | FPA_HUNG | FPA_FIRST_V_ACT },
	{ 0,			0					    }
};


/* Jump condition data test table */
static struct
{
	u_int inst;		/* command instruction */
	u_int data;		/* reg2 data value */
	u_int result;		/* reg1 result expected */
} jumpcond[] =
{
	{ CSP_SINE,	SZERO,	SZERO		},
	{ CSP_SINE,	SVAL1,	SINE_SVAL1	},
	{ CSP_SINE,	SVAL2,	SINE_SVAL2	},
	{ CSP_SINE,	SONE,	SINE_SONE	},
	{ CSP_SINE,	SMONE,	SINE_SMONE	},
	{ CSP_ARCTAN,	STWO,	ATAN_STWO	},
	{ CSP_ARCTAN,	SVAL3,	ATAN_SVAL3	},
	{ CSP_ARCTAN,	SVAL4,	ATAN_SVAL4	},
	{ 0,		0,	0		}
};


/* Single precision short instruction operation test table */
static struct
{
	u_int inst;		/* instruction */
	u_int reg1;		/* reg1 value */
	u_int op;		/* operand value */
	u_int result;		/* reg1 result expected */
} spshort[] =
{
	{ SP_NEGATE,	X,		SONE,		SMONE		},
	{ SP_ABS,	X,		SMONE,		SONE		},
	{ SP_FLOAT,	X,		4,		SFOUR		},
	{ SP_FIX,	X,		SSIXTEEN,	16		},
	{ SP_TODOUBLE,	X,		SNINE,		DNINE_M		},
	{ SP_SQUARE,	X,		SFOUR,		SSIXTEEN	},
	{ SP_ADD,	STHREE,		SSIX,		SNINE		},
	{ SP_SUB,	SNINE,		SONE,		SEIGHT		},
	{ SP_MULT,	STWO,		SFIVE,		STEN		},
	{ SP_DIVIDE,	SEIGHT,		SFOUR,		STWO		},
	{ SP_RSUB,	STHREE,		SFOUR,		SONE		},
	{ SP_RDIVIDE,	SONE,		SEIGHT,		SEIGHT		},
	{ 0,		0,		0,		0		}
};


/* Double precision short instruction operation test table */
static struct
{
	u_int inst;		/* instruction */
	u_int reg1;		/* reg1 value */
	u_int op;		/* operand most significant value */
	u_int result;		/* reg1 result expected */
} dpshort[] =
{
	{ DP_NEGATE,	X,		DONE_M,		DMONE_M		},
	{ DP_ABS,	X,		DMONE_M,	DONE_M		},
	{ DP_FLOAT,	X,		4,		DFOUR_M		},
	{ DP_FIX,	X,		DSIXTEEN_M,	16		},
	{ DP_TOSINGLE,	X,		DNINE_M,	SNINE		},
	{ DP_SQUARE,	X,		DFOUR_M,	DSIXTEEN_M	},
	{ DP_ADD,	DTHREE_M,	DSIX_M,		DNINE_M		},
	{ DP_SUB,	DNINE_M,	DONE_M,		DEIGHT_M	},
	{ DP_MULT,	DTWO_M,		DFIVE_M,	DTEN_M		},
	{ DP_DIVIDE,	DEIGHT_M,	DFOUR_M,	DTWO_M		},
	{ DP_RSUB,	DTHREE_M,	DFOUR_M,	DONE_M		},
	{ DP_RDIVIDE,	DONE_M,		DEIGHT_M,	DEIGHT_M	},
	{ 0,		0,		0,		0		}
};


/* Single precision extended instruction operation test table */
static struct
{
	u_int inst;		/* instruction */
	u_int reg2;		/* reg2 value */
	u_int op;		/* operand value */
	u_int reg1;		/* reg1 result value */
} spext[] =
{
	{ XSP_ADD,	STHREE,		SSIX,		SNINE		},
	{ XSP_SUB,	SNINE,		SONE,		SEIGHT		},
	{ XSP_MULT,	STWO,		SFIVE,		STEN		},
	{ XSP_DIVIDE,	SEIGHT,		SFOUR,		STWO		},
	{ XSP_RSUB,	STHREE,		SFOUR,		SONE		},
	{ XSP_RDIVIDE,	SONE,		SEIGHT,		SEIGHT		},
	{ 0,		0,		0,		0		}
};


/* Single precision extended two operations test table */ 
static struct
{
	u_int inst;		/* instruction */
	u_int op2;		/* second operand value */
	u_int reg2;		/* reg2 value */
	u_int op1;		/* first operand value */
	u_int reg1;		/* reg1 result value */
} spext2[] =
{
	{ XSP_R1_O2aR2xO,	STWO,	SONE,	STHREE,	SFIVE		},
	{ XSP_R1_O2sR2xO,	STEN,	STWO,	STHREE,	SFOUR		},
	{ XSP_R1_mO2aR2xO,	SONE,	STWO,	STWO,	STHREE		},
	{ XSP_R1_O2xR2aO,	SONE,	STHREE,	SSIX,	SNINE		},
	{ XSP_R1_O2xR2sO,	SONE,	STWO,	STHREE,	SMONE		},
	{ XSP_R1_O2xmR2aO,	SFOUR,	STWO,	SSIX,	SSIXTEEN	},
	{ 0,			0,	0,	0,	0		}
};


/* Double precision extended instruction operation test table */
static struct
{
	u_int inst;		/* instruction */
	u_int reg2;		/* reg2 value */
	u_int op;		/* operand value */
	u_int reg1;		/* reg1 result value */
} dpext[] =
{
	{ XDP_ADD,	DTHREE_M,	DSIX_M,		DNINE_M		},
	{ XDP_SUB,	DNINE_M,	DONE_M,		DEIGHT_M	},
	{ XDP_MULT,	DTWO_M,		DFIVE_M,	DTEN_M		},
	{ XDP_DIVIDE,	DEIGHT_M,	DFOUR_M,	DTWO_M		},
	{ XDP_RSUB,	DTHREE_M,	DFOUR_M,	DONE_M		},
	{ XDP_RDIVIDE,	DONE_M,		DEIGHT_M,	DEIGHT_M	},
	{ 0,		0,		0,		0		}
};


/* Single precision command register operation test */
static struct
{
	u_int inst;		/* instruction */
	u_int reg3;		/* reg3 value */
	u_int reg2;		/* reg2 value */
	u_int reg4;		/* reg4 value */
	u_int reg1;		/* reg1 result value */
} spcmd[] =
{
	{ CSP_R1_R2,		X,	STEN,	X,	STEN		},
	{ CSP_R1_R3aR2xR4,	STHREE,	STWO,	STHREE,	SNINE		},
	{ CSP_R1_R3sR2xR4,	STEN,	SONE,	STWO,	SEIGHT		},
	{ CSP_R1_mR3aR2xR4,	SSIX,	STWO,	STHREE,	SZERO		},
	{ CSP_R1_R3xR2aR4,	SFOUR,	STWO,	STWO,	SSIXTEEN	},
	{ CSP_R1_R3xR2sR4,	SNINE,	SFOUR,	STHREE,	SNINE		},
	{ CSP_R1_R3xmR2aR4,	STEN,	STWO,	STHREE,	STEN		},
	{ 0,			0,	0,	0,	0		}
};


/* Double precision command register operation test */
static struct
{
	u_int inst;		/* instruction */
	u_int reg3;		/* reg3 most significant value */
	u_int reg2;		/* reg2 most significant value */
	u_int reg4;		/* reg4 most significant value */
	u_int reg1;		/* reg1 most significant result value */
} dpcmd[] =
{
	{ CDP_R1_R2,		X,	   DTEN_M,    X,	DTEN_M     },
	{ CDP_R1_R3aR2xR4,	DTHREE_M,  DTWO_M,    DTHREE_M, DNINE_M    },
	{ CDP_R1_R3sR2xR4,	DTEN_M,	   DONE_M,    DTWO_M,   DEIGHT_M   },
	{ CDP_R1_mR3aR2xR4,	DSIX_M,	   DTWO_M,    DTHREE_M, DZERO_M    },
	{ CDP_R1_R3xR2aR4,	DFOUR_M,   DTWO_M,    DTWO_M,   DSIXTEEN_M },
	{ CDP_R1_R3xR2sR4,	DNINE_M,   DFOUR_M,   DTHREE_M, DNINE_M    },
	{ CDP_R1_R3xmR2aR4,	DTEN_M,	   DTWO_M,    DTHREE_M, DTEN_M     },
	{ 0,			0,	   0,	      0,	0	   }
};


/* TI status test table structure */
struct tistat_tbl
{
	u_int inst;		/* operation */
	u_int reg2_ms, reg2_ls;	/* reg2 */
	u_int reg3_ms, reg3_ls;	/* reg3 */
	u_int stat;		/* TI expected status */
};


/* TI status test table with ALU functions */
static struct tistat_tbl alu[] =
{
	{ CWSP_ADD,    SZERO,     X,         SZERO,     X,         TI_ZERO   },
	{ CWDP_ADD,    DZERO_M,   DZERO_L,   DZERO_M,   DZERO_L,   TI_ZERO   },
	{ CWSP_ADD,    SINF,      X,         SINF,      X,         TI_INFIN  },
	{ CWDP_ADD,    DINF_M,    DINF_L,    DINF_M,    DINF_L,    TI_INFIN  },
	{ CWSP_ADD,    SONE,      X,         SONE,      X,         TI_FINEX  },
	{ CWDP_ADD,    DONE_M,    DONE_L,    DONE_M,    DONE_L,    TI_FINEX  },
	{ CWSP_DIVIDE, SONE,      X,         SPI,       X,         TI_FININ  },
	{ CWDP_DIVIDE, DONE_M,    DONE_L,    DPI_M,     DPI_L,     TI_FININ  },
	{ CWSP_ADD,    SMAXNORM,  X,         SMAXNORM,  X,         TI_OVFIN  },
	{ CWDP_ADD,    DMAXNORM_M,DMAXNORM_L,DMAXNORM_M,DMAXNORM_L,TI_OVFIN  },
	{ CWSP_DIVIDE, SMINNORM,  X,         STWO,      X,         TI_UNDFEX },
	{ CWDP_DIVIDE, DMINNORM_M,DMINNORM_L,DTWO_M,    DTWO_L,    TI_UNDFEX },
	{ CWSP_DIVIDE, SMINNORM,  X,         SPI,       X,         TI_UNDFIN },
	{ CWDP_DIVIDE, DMINNORM_M,DMINNORM_L,DPI_M,     DPI_L,     TI_UNDFIN },
	{ CWSP_DIVIDE, SMAXSUB,   X,         STWO,      X,         TI_ADNORM },
	{ CWDP_DIVIDE, DMAXSUB_M, DMAXSUB_L, DTWO_M,    DTWO_L,    TI_ADNORM },
	{ CWSP_DIVIDE, STWO,      X,         SMAXSUB,   X,         TI_BDNORM },
	{ CWDP_DIVIDE, DTWO_M,    DTWO_L,    DMAXSUB_M, DMAXSUB_L, TI_BDNORM },
	{ CWSP_DIVIDE, SMAXSUB,   X,         SMAXSUB,   X,         TI_ABDNORM},
	{ CWDP_DIVIDE, DMAXSUB_M, DMAXSUB_L, DMAXSUB_M, DMAXSUB_L, TI_ABDNORM},
	{ CWSP_DIVIDE, SONE,      X,         SZERO,     X,         TI_DIVZERO},
	{ CWDP_DIVIDE, DONE_M,    DONE_L,    DZERO_M,   DZERO_L,   TI_DIVZERO},
	{ CWSP_ADD,    SSNAN,     X,         SZERO,     X,         TI_ANAN   },
	{ CWDP_ADD,    DSNAN_M,   DSNAN_L,   DZERO_M,   DZERO_L,   TI_ANAN   },
	{ CWSP_ADD,    SZERO,     X,         SSNAN,     X,         TI_BNAN   },
	{ CWDP_ADD,    DZERO_M,   DZERO_L,   DSNAN_M,   DSNAN_L,   TI_BNAN   },
	{ CWSP_ADD,    SSNAN,     X,         SSNAN,     X,         TI_ABNAN  },
	{ CWDP_ADD,    DSNAN_M,   DSNAN_L,   DSNAN_M,   DSNAN_L,   TI_ABNAN  },
	{ CWSP_DIVIDE, SINF,      X,         SINF,      X,         TI_INVAL  },
	{ CWDP_DIVIDE, DINF_M,    DINF_L,    DINF_M,    DINF_L,    TI_INVAL  },
	{ 0,           0,         0,         0,         0,         0         }
};


/* TI status test table with multiplier functions */
static struct tistat_tbl mul[] =
{
	{ CWSP_MULT,   SZERO,     X,         SZERO,     X,         TI_ZERO   },
	{ CWDP_MULT,   DZERO_M,   DZERO_L,   DZERO_M,   DZERO_L,   TI_ZERO   },
	{ CWSP_MULT,   SINF,      X,         SINF,      X,         TI_INFIN  },
	{ CWDP_MULT,   DINF_M,    DINF_L,    DINF_M,    DINF_L,    TI_INFIN  },
	{ CWSP_MULT,   SONE,      X,         SONE,      X,         TI_FINEX  },
	{ CWDP_MULT,   DONE_M,    DONE_L,    DONE_M,    DONE_L,    TI_FINEX  },
	{ CWSP_MULT,   SPI,       X,         SPI,       X,         TI_FININ  },
	{ CWDP_MULT,   DPI_M,     DPI_L,     DPI_M,     DPI_L,     TI_FININ  },
	{ CWSP_MULT,   SMAXNORM,  X,         SMAXNORM,  X,         TI_OVFIN  },
	{ CWDP_MULT,   DMAXNORM_M,DMAXNORM_L,DMAXNORM_M,DMAXNORM_L,TI_OVFIN  },
	{ CWSP_MULT,   SMINNORM,  X,         SHALF,     X,         TI_UNDFEX },
	{ CWDP_MULT,   DMINNORM_M,DMINNORM_L,DHALF_M,   DHALF_L,   TI_UNDFEX },
	{ CWSP_MULT,   SMIN1,     X,         SPIO4,     X,         TI_UNDFIN },
	{ CWDP_MULT,   DMIN1_M,   DMIN1_L,   DPIO4_M,   DPIO4_L,   TI_UNDFIN },
	{ CWSP_MULT,   SMINSUB,   X,         SHALF,     X,         TI_ADNORM },
	{ CWDP_MULT,   DMINSUB_M, DMINSUB_L, DHALF_M,   DHALF_L,   TI_ADNORM },
	{ CWSP_MULT,   SHALF,     X,         SMINSUB,   X,         TI_BDNORM },
	{ CWDP_MULT,   DHALF_M,   DHALF_L,   DMINSUB_M, DMINSUB_L, TI_BDNORM },
	{ CWSP_MULT,   SMINSUB,   X,         SMINSUB,   X,         TI_ABDNORM},
	{ CWDP_MULT,   DMINSUB_M, DMINSUB_L, DMINSUB_M, DMINSUB_L, TI_ABDNORM},
	{ CWSP_MULT,   SSNAN,     X,         SZERO,     X,         TI_ANAN   },
	{ CWDP_MULT,   DSNAN_M,   DSNAN_L,   DZERO_M,   DZERO_L,   TI_ANAN   },
	{ CWSP_MULT,   SZERO,     X,         SSNAN,     X,         TI_BNAN   },
	{ CWDP_MULT,   DZERO_M,   DZERO_L,   DSNAN_M,   DSNAN_L,   TI_BNAN   },
	{ CWSP_MULT,   SSNAN,     X,         SSNAN,     X,         TI_ABNAN  },
	{ CWDP_MULT,   DSNAN_M,   DSNAN_L,   DSNAN_M,   DSNAN_L,   TI_ABNAN  },
	{ CWSP_MULT,   SINF,      X,         SZERO,     X,         TI_INVAL  },
	{ CWDP_MULT,   DINF_M,    DINF_L,    DZERO_M,   DZERO_L,   TI_INVAL  },
	{ 0,           0,         0,         0,         0,         0         }
};


/* TI timing test table with single precision short instructions */
static struct
{
	u_int inst;		/* instruction */
	u_int reg1;		/* reg1 value */
	u_int op;		/* operand value */
	u_int result;		/* reg1 result expected */
} timsp[] =
{
	{ SP_SQUARE,	X,		SONE,		SONE		},
	{ SP_SQUARE,	X,		STWO,		SFOUR		},
	{ SP_SQUARE,	X,		STHREE,		SNINE		},
	{ SP_SQUARE,	X,		SFOUR,		SSIXTEEN	},
	{ SP_DIVIDE,	SSIX,		STHREE,		STWO		},
	{ SP_DIVIDE,	SSIX,		STWO,		STHREE		},
	{ SP_DIVIDE,	SEIGHT,		SFOUR,		STWO		},
	{ SP_DIVIDE,	SSIXTEEN,	SFOUR,		SFOUR		},
	{ 0,		0,		0,		0		}
};


/* TI timing test table with double precision short instructions */
static struct
{
	u_int inst;		/* instruction */
	u_int reg1;		/* reg1 value */
	u_int op;		/* operand value */
	u_int result;		/* reg1 result expected */
} timdp[] =
{
	{ DP_SQUARE,	X,		DONE_M,		DONE_M		},
	{ DP_SQUARE,	X,		DTWO_M,		DFOUR_M		},
	{ DP_SQUARE,	X,		DTHREE_M,	DNINE_M		},
	{ DP_SQUARE,	X,		DFOUR_M,	DSIXTEEN_M	},
	{ DP_DIVIDE,	DSIX_M,		DTHREE_M,	DTWO_M		},
	{ DP_DIVIDE,	DSIX_M,		DTWO_M,		DTHREE_M	},
	{ DP_DIVIDE,	DEIGHT_M,	DFOUR_M,	DTWO_M		},
	{ DP_DIVIDE,	DSIXTEEN_M,	DFOUR_M,	DFOUR_M		},
	{ 0,		0,		0,		0		}
};


/* TI timing test table with extended dp instructions */
static struct
{
	u_int inst;		/* instruction */
	u_int op;		/* data operand */
	u_int reg3;		/* data assigned to reg3 */
	u_int reg2;		/* data assigned to reg2 */
	u_int reg1;		/* expected result in reg1 */
} timext[] =
{
	{ XDP_R1_R3aR2xO,  DTWO_M,     DTWO_M,     DFOUR_M,  DTEN_M },
	{ XDP_R1_R3sR2xO,  DTWO_M,     DSIXTEEN_M, DTHREE_M, DTEN_M },
	{ XDP_R1_mR3aR2xO, DTWO_M,     DTWO_M,     DSIX_M,   DTEN_M },
	{ XDP_R1_R3xR2aO,  DTWO_M,     DTWO_M,     DTHREE_M, DTEN_M },
	{ XDP_R1_R3xR2sO,  DTWO_M,     DFIVE_M,    DFOUR_M,  DTEN_M },
	{ XDP_R1_R3xmR2aO, DSEVEN_M,   DTWO_M,	   DTWO_M,   DTEN_M },
	{ XDP_R1_OaR3xR2,  DFOUR_M,    DTHREE_M,   DTWO_M,   DTEN_M },
	{ XDP_R1_OsR3xR2,  DSIXTEEN_M, DTHREE_M,   DTWO_M,   DTEN_M },
	{ XDP_R1_mOaR3xR2, DTWO_M,     DSIX_M,     DTWO_M,   DTEN_M },
	{ XDP_R1_OxR3aR2,  DTWO_M,     DFOUR_M,    DONE_M,   DTEN_M },
	{ XDP_R1_OxR3sR2,  DTWO_M,     DSIX_M,     DONE_M,   DTEN_M },
	{ 0,               0,          0,          0,        0      }
};


/* TI timing test table with command register dp instructions */
static struct
{
	u_int inst;		/* instruction */
	u_int ms2, ls2;		/* data assigned to reg2 */
	u_int ms1, ls1;		/* expected result in reg1 */
} timcmd[] =
{
	{ CDP_R1_eR2s1,	DTHIRTY2_M, DTHIRTY2_L, DEXP32S1_M, DEXP32S1_L },
	{ CDP_R1_eR2s1, DSIXTEEN_M, DSIXTEEN_L, DEXP16S1_M, DEXP16S1_L },
	{ CDP_R1_eR2s1, DEIGHT_M,   DEIGHT_L,   DEXP8S1_M,  DEXP8S1_L  },
	{ CDP_R1_eR2s1, DFOUR_M,    DFOUR_L,    DEXP4S1_M,  DEXP4S1_L  },
	{ 0,            0,          0,          0,          0          }
};


/*
 * This function tests the IERR register by write/read all patterns
 * in bits 23-16 of the register.
 */
ierr_test()
{
	register u_int w, k;
	u_int r;

	send_message(0,VERBOSE,tst_reg,"IERR");

	for (k = 0; k <= BYTE_MAX; k++)
	{
		w = k << IERR_SHIFT;
		if (write_fpa(&fpa->fp_ierr,w,ON)) return;
		if (read_fpa(&fpa->fp_ierr,&r,ON)) return;
		r &= IERR_MASK;
		if (w != r) send_message(FPERR,ERROR,er_reg,"IERR",w,r);
	}
}


/*
 * This function tests the IMASK register by write all patterns in bits 3-0
 * of register, then verifies bits 3-1 are always 001 and bit 0 is compared.
 */
imask_test()
{
	register u_int w;
	u_int r;

	send_message(0,VERBOSE,tst_reg,"IMASK");

	for (w = 0; w <= NIBBLE_MAX; w++)
	{
		if (write_fpa(&fpa->fp_imask,w,ON)) return;
		if (read_fpa(&fpa->fp_imask,&r,ON)) return;
		r &= IMASK_MASK;
		if ((w & FPA_INEXACT | fpa_version) != r)
			send_message(FPERR,ERROR,er_reg,"IMASK",w,r);
	}
}


/*
 * This function tests the LOAD_PTR register by write/read all hex patterns 
 * from 0 to FFFF.
 */
loadptr_test()
{
	register u_int w;
	u_int r;

	send_message(0,VERBOSE,tst_reg,"LOAD_PTR");

	for (w = 0; w <= WORD_MAX; w++)
	{
		if (write_fpa(&fpa->fp_load_ptr,w,ON)) return;
		if (read_fpa(&fpa->fp_load_ptr,&r,ON)) return;
		r &= LOADPTR_MASK;
		if (w != r) send_message(FPERR,ERROR,er_reg,"LOAD_PTR",w,r);
	}
}


/*
 * This function performs write/read test to the MODE register bits 3-0.
 * The MODE register can be written only via micro-instruction.
 */
mode_test()
{
	register u_int w;
	u_int r;

	send_message(0,VERBOSE,tst_reg,"MODE");

	for (w = 0; w <= NIBBLE_MAX; w++)
	{
		if (write_fpa(&fpa->fp_restore_mode3_0,w,ON)) return;
		if (read_fpa(&fpa->fp_mode3_0_clear,&r,ON)) return;
		r &= MODE_MASK;
		if (w != r) send_message(FPERR,ERROR,er_reg,"MODE",w,r);
	}
}


/*
 * This function tests the WSTATUS register by write to bits 11-8, then
 * read back and verified.  Bits 11-8 will decode value for bits 15 and 4-0.
 * For this test, the inexact error bit in IMASK register will be masked.
 * This test will be done twice, once with inexact error bit mask, once
 * without.  The WSTATUS register can be written only via micro-instruction.
 */
wstatus_test()
{
	register u_int w, exp, imask, k;
	u_int r;

	send_message(0,VERBOSE,tst_reg,"WSTATUS");

	for (imask = 0; imask <= 1; imask++)
	{
		if (write_fpa(&fpa->fp_imask,imask,ON)) return;

		for (k = 0; k <= NIBBLE_MAX; k++)
		{
			w = k << WSTATUS_SHIFT;
			if (write_fpa(&fpa->fp_restore_wstatus,w,ON)) return;
			if (read_fpa(&fpa->fp_wstatus_clear,&r,ON)) return;

			r &= WSTATUS_MASK;
			exp = wstatexp[k];
			if (!imask && (w == WS_COND)) exp |= WS_ERROR;

			if (r != exp) send_message(FPERR,ERROR,er_wstatus,w,exp,r);
		}
	}
}


/*
 * This function tests all registers of current context in register file by
 * write/read using address pattern.
 */
datareg_test()
{
	register u_int reg;
	u_int ms, ls;

	send_message(0,VERBOSE,tst_datareg);

	/* Write address pattern to all data registers */
	for (reg = 0; reg < FPA_NDATA_REGS; reg++)
		if (write_datareg(reg,reg,~reg)) return;

	/* Read back to verify all data registers */
	for (reg = 0; reg < FPA_NDATA_REGS; reg++)
	{
		if (read_datareg(reg,&ms,&ls)) return;
		if (ms != reg || ls != ~reg)
			send_message(FPERR,ERROR,er_datareg,reg,reg,~reg,ms,ls);
	}
}


/*
 * Function to write to register file in current context.
 * Function returns 0 if successful, else nonzero.
 */
write_datareg(reg,ms,ls)
u_int reg, ms, ls;
{
	register u_int *addr;

	addr = (u_int *)&fpa->fp_data[reg];
	if (reg < FPA_NDATA_REGS)
	{
		if (write_fpa(addr,ms,ON)) return(TRUE);
		if (write_fpa(addr+1,ls,ON)) return(TRUE);
	}
	return(FALSE);
}


/*
 * Function to read register file in current context.
 * Function returns 0 if successful, else nonzero.
 */
read_datareg(reg,ms,ls)
u_int reg, *ms, *ls;
{
	register u_int *addr;

	addr = (u_int *)&fpa->fp_data[reg];
	if (reg < FPA_NDATA_REGS)
	{
		if (read_fpa(addr,ms,ON)) return(TRUE);
		if (read_fpa(addr+1,ls,ON)) return(TRUE);
	}
	return(FALSE);
}


/*
 * This function tests shadow RAM by write to the register file and read
 * back from the shadow RAM register.
 */
shadow_test()
{
	register u_int reg;
	u_int ms, ls;

	send_message(0,VERBOSE,tst_shadow);

	/* Write inverse address pattern to all data registers */
	for (reg = 0; reg < FPA_NDATA_REGS; reg++)
		if (write_datareg(reg,~reg,reg)) return;

	/* Read back to verify Shadow RAM registers */
	for (reg = 0; reg < FPA_NSHAD_REGS; reg++)
	{
		read_shadow(reg,&ms,&ls);
		if (ms != ~reg || ls != reg)
			send_message(FPERR,ERROR,er_shadow,reg,~reg,reg,ms,ls);
	}
}


/*
 * Function to read a shadow register in current context.
 */
read_shadow(reg,ms,ls)
u_int reg, *ms, *ls;
{
	register u_int *addr;

	addr = (u_int *)&fpa->fp_shad[reg];
	if (reg < FPA_NSHAD_REGS)
	{
		if (read_fpa(addr,ms,ON)) return;
		if (read_fpa(addr+1,ls,ON)) return;
	}
}


/*
 * This function generates error conditions then verifies negative
 * acknowledegement status from the FPA board.
 */
nack_test()
{
	register int nack, addr, k;
	u_int val;

	send_message(0,VERBOSE,tst_nack);

	/* non 32-bit access */
	addr = (u_int) &fpa->fp_imask;
	for (k = 1; k < 4; k++)
	{
		addr += k;
		nack = read_fpa((u_int *)addr,&val,OFF);
		check_nack(nack,FPA_NON32_ACCESS);
	}

	/* write to supervisor space in user mode (hard clear pipe) */
	nack = write_fpa(&fpa->fp_hard_clear_pipe,X,OFF);
	check_nack(nack,FPA_PROTECTION);

	/* write to supervisor space in user mode (state) */
	nack = write_fpa(&fpa->fp_state,X,OFF);
	check_nack(nack,FPA_PROTECTION);

	/* write to Register File while it is disabled */
	if (read_fpa(&fpa->fp_state,&val,ON)) return;
	if (!(val & FPA_ACCESS_BIT))
	{
		if (write_fpa(&fpa->fp_load_ptr,X,ON)) return;
		nack = write_fpa((u_int *)INS_LOADRF_M,X,OFF);
		check_nack(nack,FPA_PROTECTION);
	}

	/* write to Microstore RAM while it is disabled */
	if (read_fpa(&fpa->fp_state,&val,ON)) return;
	if (!(val & FPA_LOAD_BIT))
	{
		if (write_fpa(&fpa->fp_load_ptr,0,ON)) return;
		nack = write_fpa(&fpa->fp_ld_ram,X,OFF);
		check_nack(nack,FPA_PROTECTION);
	}

	/* access illegal address */
	nack = read_fpa((u_int *)FPA3X_ILLADDR,&val,OFF);
	check_nack(nack,FPA_PROTECTION);

	/* access illegal diagnostic only address */
	nack = read_fpa((u_int *)FPA3X_ILLDIAG,&val,OFF);
	check_nack(nack,FPA_PROTECTION);

	/* read FPA-3X write-only address */
	nack = read_fpa(&fpa->fp_clear_pipe,&val,OFF);
	check_nack(nack,FPA_ILLEGAL_ACCESS);

	/* write FPA-3X read-only address */
	nack = write_fpa(&fpa->fp_pipe_status,X,OFF);
	check_nack(nack,FPA_ILLEGAL_ACCESS);

	/* illegal access sequence */
	if (write_fpa((u_int *)DP_NOP,X,ON)) return;
	nack = write_fpa((u_int *)X_LSW,X,OFF);
	check_nack(nack,FPA_ILLEGAL_SEQ);

	/* access pipe while pipe hung */
	if (write_fpa(&fpa->fp_unimplemented,X,ON)) return;
	nack = read_fpa((u_int *)&fpa->fp_data[0],&val,OFF);
	check_nack(nack,FPA_HUNG_PIPE);

	/* access control while pipe hung */
	if (write_fpa(&fpa->fp_unimplemented,X,ON)) return;
	nack = read_fpa(&fpa->fp_mode3_0_clear,&val,OFF);
	check_nack(nack,FPA_HUNG_PIPE);

	/* access shadow RAM while pipe hung */
	if (write_fpa(&fpa->fp_unimplemented,X,ON)) return;
	nack = read_fpa((u_int *)&fpa->fp_shad[0],&val,OFF);
	check_nack(nack,FPA_HUNG_PIPE);

	/* access illegal control register */
	nack = read_fpa((u_int *)FPA3X_ILLCTRL,&val,OFF);
	check_nack(nack,FPA_ILLE_CTL_ADDR);
}


/*
 * This function verifies that nack occurs and checks the IERR status.
 * The IERR register will be cleared afterwards, and clear pipe is issued.
 */
check_nack(nack,ierr_exp)
register int nack;
register int ierr_exp;
{
	u_int ierr_obs;

	if (!nack) send_message(FPERR,ERROR,er_nonack,ierr_exp);
	else
	{
		if (read_fpa(&fpa->fp_ierr,&ierr_obs,ON)) return;
		ierr_obs &= IERR_MASK;
		if (ierr_obs != ierr_exp)
			send_message(FPERR,ERROR,er_nack,ierr_exp,ierr_obs);
		if (write_fpa(&fpa->fp_ierr,0,ON)) return;
	}
	if (write_fpa(&fpa->fp_clear_pipe,X,ON)) return;
}


/*
 * Function to issue simple instructions and check pipe status.
 */
simpleins_test()
{
	register u_int inst, k;
	u_int pipestat;

	send_message(0,VERBOSE,tst_simpleins);

	for (k = 0; inst = simple[k].inst; k++)
	{
		if (write_fpa((u_int *)inst,X,ON)) return;
		if (read_fpa(&fpa->fp_pipe_status,&pipestat,ON)) return;
		pipestat &= PIPESTAT_MASK;
		if (pipestat != simple[k].stat)
			send_message(FPERR,ERROR,er_simpleins,inst,simple[k].stat,pipestat);
	}
}


/*
 * Function to test the pointers 1 thru 4 using single and double precision
 * short, extended, and command register instructions.
 */
pointer_test()
{
	send_message(0,VERBOSE,tst_pointer,"short");
	pointer_short();
	send_message(0,VERBOSE,tst_pointer,"extended");
	pointer_extend();
	send_message(0,VERBOSE,tst_pointer,"command register");
	pointer_cmdreg();
	send_message(0,VERBOSE,tst_pointer,"increment/decrement");
	pointer_incdec();
}


/*
 * Function to test pointer 2 using single and double precision short
 * instructions.  This function uses the operation:  reg1 = reg1 + operand.
 * Where reg1 uses data register 0 thru 15, and holds value 2, and the
 * operand holds value 6.  Result of reg1 should be 8.
 */
pointer_short()
{
	register u_int dreg, r1, inst;
	u_int ms, ls, ms_exp, ls_exp;

	/* single precision short test */
	for (r1 = 0; r1 < FPA_NSHORT_REGS; r1++)
	{
		/* clear all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
			if (write_datareg(dreg,SZERO,SZERO)) return;

		/* load register value and execute instruction */
		if (write_datareg(r1,STWO,SZERO)) return;
		inst = SP_ADD | SP_REG1(r1);
		if (write_fpa((u_int *)inst,SSIX,ON)) return;

		/* read to check all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
		{
			if (read_datareg(dreg,&ms,&ls)) return;
			ms_exp = (dreg == r1) ? SEIGHT : SZERO;
			ls_exp = SZERO;
			if (ms != ms_exp || ls != ls_exp)
				send_message(FPERR,ERROR,er_pointer,dreg,ms_exp,ls_exp,ms,ls);
		}
	}

	/* double precision short test */
	for (r1 = 0; r1 < FPA_NSHORT_REGS; r1++)
	{
		/* clear all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
			if (write_datareg(dreg,DZERO_M,DZERO_L)) return;

		/* load register value and execute instruction */
		if (write_datareg(r1,DTWO_M,DTWO_L)) return;
		inst = DP_ADD | DP_REG1(r1);
		if (write_fpa((u_int *)inst,DSIX_M,ON)) return;
		if (write_fpa((u_int *)DP_LSW,DSIX_L,ON)) return;

		/* read to check all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
		{
			if (read_datareg(dreg,&ms,&ls)) return;
			ms_exp = (dreg == r1) ? DEIGHT_M : DZERO_M;
			ls_exp = DZERO_L;
			if (ms != ms_exp || ls != ls_exp)
				send_message(FPERR,ERROR,er_pointer,dreg,ms_exp,ls_exp,ms,ls);
    		}
	}
}


/*
 * Function to test pointer 1 thru 3 using the extended instructions.
 * For single precision, it uses the operation:  reg1 = op2 + (reg2 * op1).
 * Where reg1 uses data register 0 thru 14, and should hold result 9;
 *	 reg2 uses data register 1 thru 15, and holds value 4;
 *	 operand1 holds value 2, and operand2 holds value 1.
 * For double precision, it uses the operation:  reg1 = reg3 + (reg2 * op1).
 * Where reg1 uses data register 0 thru 13, and should hold result 10;
 *	 reg2 uses data register 1 thru 14, and holds value 2;
 *	 reg3 uses data register 2 thru 15, and holds value 4;
 *	 operand1 holds value 3.
 */
pointer_extend()
{
	register u_int dreg, r1, r2, r3, inst;
	u_int ms, ls, ms_exp, ls_exp;

	/* extended single precision test */
	for (r1=0, r2=1; r2 < FPA_NSHORT_REGS; r1++, r2++)
	{
		/* clear all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
			if (write_datareg(dreg,SZERO,SZERO)) return;

		/* load register value and execute instruction */
		if (write_datareg(r2,SFOUR,SZERO)) return;
		inst = XSP_R1_O2aR2xO | X_REG2(r2);
		if (write_fpa((u_int *)inst,STWO,ON)) return;
		inst = X_LSW | X_REG1(r1);
		if (write_fpa((u_int *)inst,SONE,ON)) return;

		/* read to check all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
		{
			if (read_datareg(dreg,&ms,&ls)) return;
			ms_exp = (dreg == r1) ? SNINE : 
				 (dreg == r2) ? SFOUR : SZERO;
			ls_exp = SZERO;
			if (ms != ms_exp || ls != ls_exp)
				send_message(FPERR,ERROR,er_pointer,dreg,ms_exp,ls_exp,ms,ls);
		}
	}

	/* extended double precision test */
	for (r1=0, r2=1, r3=2; r3 < FPA_NSHORT_REGS; r1++, r2++, r3++)
	{
		/* clear all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
			if (write_datareg(dreg,DZERO_M,DZERO_L)) return;

		/* load register value and execute instruction */
		if (write_datareg(r2,DTWO_M,DTWO_L)) return;
		if (write_datareg(r3,DFOUR_M,DFOUR_L)) return;
		inst = XDP_R1_R3aR2xO | X_REG2(r2);
		if (write_fpa((u_int *)inst,DTHREE_M,ON)) return;
		inst = X_LSW| X_REG1(r1) | X_REG3(r3); 
		if (write_fpa((u_int *)inst,DTHREE_L,ON)) return;

		/* read to check all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
		{
			if (read_datareg(dreg,&ms,&ls)) return;
			ms_exp = (dreg == r1) ? DTEN_M  : 
				 (dreg == r2) ? DTWO_M  :
				 (dreg == r3) ? DFOUR_M : DZERO_M;
			ls_exp = DZERO_L;
			if (ms != ms_exp || ls != ls_exp)
				send_message(FPERR,ERROR,er_pointer,dreg,ms_exp,ls_exp,ms,ls);
		}
	}
}


/*
 * Function to test pointers 1 thru 4 using command register instructions.
 * This function uses the operation:  reg1 = reg3 + (reg2 * reg4). 
 * Where reg1 uses data register 0 thru 28, and should hold result 16;
 *       reg2 uses data register 1 thru 29, and holds value 2;
 *       reg3 uses data register 2 thru 30, and holds value 8;
 *       reg4 uses data register 3 thru 31, and holds value 4.
 */
pointer_cmdreg()
{
	register u_int r1, r2, r3, r4, data, dreg;
	u_int ms, ls, ms_exp, ls_exp;

	/* single precision command register test */
	for (r1=0, r2=1, r3=2, r4=3; r4<FPA_NDATA_REGS; r1++, r2++, r3++, r4++)
	{
		/* clear all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
			if (write_datareg(dreg,SZERO,SZERO)) return;

		/* load register value and execute instruction */
		if (write_datareg(r2,STWO,SZERO)) return;
		if (write_datareg(r3,SEIGHT,SZERO)) return;
		if (write_datareg(r4,SFOUR,SZERO)) return;
		data = C_REG4(r4) | C_REG3(r3) | C_REG2(r2) | C_REG1(r1);
		if (write_fpa((u_int *)CSP_R1_R3aR2xR4,data,ON)) return;

		/* read to check all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
		{
			if (read_datareg(dreg,&ms,&ls)) return;
			ms_exp = (dreg == r1) ? SSIXTEEN :
				 (dreg == r2) ? STWO     :
				 (dreg == r3) ? SEIGHT   :
				 (dreg == r4) ? SFOUR    : SZERO;
			ls_exp = SZERO;
			if (ms != ms_exp || ls != ls_exp)
				send_message(FPERR,ERROR,er_pointer,dreg,ms_exp,ls_exp,ms,ls);
    		}
	}

	/* double precision command register */
	for (r1=0, r2=1, r3=2, r4=3; r4<FPA_NDATA_REGS; r1++, r2++, r3++, r4++)
	{
		/* clear all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
			if (write_datareg(dreg,DZERO_M,DZERO_L)) return;

		/* load register value and execute instruction */
		if (write_datareg(r2,DTWO_M,DTWO_L)) return;
		if (write_datareg(r3,DEIGHT_M,DEIGHT_L)) return;
		if (write_datareg(r4,DFOUR_M,DFOUR_L)) return;
		data = C_REG4(r4) | C_REG3(r3) | C_REG2(r2) | C_REG1(r1);
		if (write_fpa((u_int *)CDP_R1_R3aR2xR4,data,ON)) return;

		/* read to check all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
		{
			if (read_datareg(dreg,&ms,&ls)) return;
			ms_exp = (dreg == r1) ? DSIXTEEN_M :
				 (dreg == r2) ? DTWO_M     :
				 (dreg == r3) ? DEIGHT_M   :
				 (dreg == r4) ? DFOUR_M    : DZERO_M;
			ls_exp = DZERO_L;
			if (ms != ms_exp || ls != ls_exp)
				send_message(FPERR,ERROR,er_pointer,dreg,ms_exp,ls_exp,ms,ls);
    		}
	}
}


/*
 * Function to test the pointers increment decrement using the command
 * register instructions.
 * For single precision, it uses the operation transpose 4x4 matrix.
 * Where reg1 starts from data register 0 thru 15.
 *	Original Matrix		===>	    Transpose Matrix
 * 	   0  1  2  3				0  4  8  C
 *	   4  5  6  7				1  5  9  D
 *	   8  9  A  B				2  6  A  E
 *	   C  D  E  F				3  7  B  F
 * For double precision, it uses the operation:
 *   reg1 = (reg2*reg3)+(reg2+1)*(reg3+1)+(reg2+2)*(reg3+2)+(reg2+3)*(reg3+3)
 * Where reg1 uses data register 0 thru 29, and should hold the result 40;
 *	 reg2 uses data register 1 thru 30, and holds value 1;
 *	 reg3 uses data register 2 thru 31, and holds value 2;
 *	 reg3+1 holds value 3;
 *	 reg3+2 holds value 4;
 *	 reg3+3 holds value 5;
 */
pointer_incdec()
{
	register u_int r1, r2, r3, data, dreg;
	u_int ms, ls, ms_exp, ls_exp;

	/* single precision command register matrix 4x4 transpose test */
	for (r1 = 0; r1 < FPA_NSHORT_REGS; r1++)
	{
		/* fill all data regs with 0xFF */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
			if (write_datareg(dreg,0xFF,0xFF)) return;
		
		/* set up 4x4 matrix start at reg1 */
		for (dreg = 0; dreg < 16; dreg++)
			if (write_datareg(dreg+r1,dreg,0xFF)) return;

		/* execute instruction */
		data = C_REG1(r1);
		if (write_fpa((u_int *)CSP_TRANS4x4,data,ON)) return;

		/* read to check all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
		{
			if (read_datareg(dreg,&ms,&ls)) return;
			ms_exp = ls_exp = 0xFF;
			if (r1 <= dreg && dreg <= (r1+15))
			{
				ms_exp = dreg - r1;
				ms_exp = (ms_exp % 4) * 4 + (ms_exp / 4);
			}
			if (ms != ms_exp || ls != ls_exp)
				send_message(FPERR,ERROR,er_pointer,dreg,ms_exp,ls_exp,ms,ls);
    		}
	}

	/* double precision command register dot product test */
	for (r1=0, r2=1, r3=2; r3<(FPA_NDATA_REGS-3); r1++, r2++, r3++)
	{
		/* clear all data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
			if (write_datareg(dreg,DZERO_M,DZERO_L)) return;

		/* load register value and execute instruction */
		if (write_datareg(r2,DONE_M,DONE_L)) return;
		if (write_datareg(r3,DTWO_M,DTWO_L)) return;
		if (write_datareg(r3+1,DTHREE_M,DTHREE_L)) return;
		if (write_datareg(r3+2,DFOUR_M,DFOUR_L)) return;
		if (write_datareg(r3+3,DFIVE_M,DFIVE_L)) return;
		data = C_REG3(r3) | C_REG2(r2) | C_REG1(r1);
		if (write_fpa((u_int *)CDP_DOTPROD,data,ON)) return;

		/* read to check data registers */
		for (dreg = 0; dreg < FPA_NDATA_REGS; dreg++)
		{
			if (read_datareg(dreg,&ms,&ls)) return;
			ms_exp = (dreg == r1) ? DFORTY_M :
				 (dreg == r2) ? DONE_M   :
				 (dreg == r3) ? DTWO_M   :
				 (dreg == (r3+1)) ? DTHREE_M :
				 (dreg == (r3+2)) ? DFOUR_M  :
				 (dreg == (r3+3)) ? DFIVE_M  : DZERO_M;
			ls_exp = DZERO_L;
			if (ms != ms_exp || ls != ls_exp)
				send_message(FPERR,ERROR,er_pointer,dreg,ms_exp,ls_exp,ms,ls);
    		}
	}
}


/*
 * Function to perform shadow register lock test in both active and
 * next stages.
 */
lock_test()
{
	send_message(0,VERBOSE,tst_lock,"short");
	lock_short(FALSE);
	send_message(0,VERBOSE,tst_lock,"extended");
	lock_extend(FALSE);
	send_message(0,VERBOSE,tst_lock,"command register");
	lock_cmdreg(FALSE);
	send_message(0,VERBOSE,tst_lock,"short, next");
	lock_short(TRUE);
	send_message(0,VERBOSE,tst_lock,"extended, next");
	lock_extend(TRUE);
	send_message(0,VERBOSE,tst_lock,"command register, next");
	lock_cmdreg(TRUE);
}


/*
 * Function to perform lock test using single and double short instructions:
 *	reg1 = reg1 / operand, where reg1 = 1 and operand = 3.
 * This operation will hang and lock the shadow register assign to reg1.
 */
lock_short(next)
register int next;
{
	register u_int r1, r1_next, inst;

	/* enable imask to hang operation */
	if (write_fpa(&fpa->fp_imask,FPA_INEXACT,ON)) return;

	/* single precision short test */
	for (r1 = 0; r1 < FPA_NSHAD_REGS; r1++)
	{
		if (write_datareg(r1,SONE,SZERO)) return;
		if (next)
		{
			r1_next = (r1 + 1) % FPA_NSHAD_REGS;
			if (write_datareg(r1_next,SONE,SZERO)) return;
		}

		inst = SP_DIVIDE | SP_REG1(r1);
		if (write_fpa((u_int *)inst,STHREE,ON)) return;
		if (next)
		{
			inst = SP_DIVIDE | SP_REG1(r1_next);
			if (write_fpa((u_int *)inst,STHREE,ON)) return;
			check_lock(2,r1,r1_next);
		}
		else check_lock(1,r1);
	}

	/* double precision short test */
	for (r1 = 0; r1 < FPA_NSHAD_REGS; r1++)
	{
		if (write_datareg(r1,DONE_M,DONE_L)) return;
		if (next)
		{
			r1_next = (r1 + 1) % FPA_NSHAD_REGS;
			if (write_datareg(r1_next,DONE_M,DONE_L)) return;
		}

		inst = DP_DIVIDE | DP_REG1(r1);
		if (write_fpa((u_int *)inst,DTHREE_M,ON)) return;
		if (write_fpa((u_int *)DP_LSW,DTHREE_L,ON)) return;
		if (next)
		{
			inst = DP_DIVIDE | DP_REG1(r1_next);
			if (write_fpa((u_int *)inst,DTHREE_M,ON)) return;
			if (write_fpa((u_int *)DP_LSW,DTHREE_L,ON)) return;
			check_lock(2,r1,r1_next);
		}
		else check_lock(1,r1);
	}
}


/*
 * Function to perform lock test using single and double extended instruction:
 *	reg1 = reg2 / operand, where reg2 = 1 and operand = 3.
 * This operation will hang and lock the shadow register assign to reg1.
 */
lock_extend(next)
register int next;
{
	register u_int r1, r2, r1_next, r2_next, inst;

	/* enable imask to hang operation */
	if (write_fpa(&fpa->fp_imask,FPA_INEXACT,ON)) return;

	/* single precision extended test */
	for (r1=0, r2=1; r2 < FPA_NSHAD_REGS; r1++, r2++)
	{
		if (write_datareg(r2,SONE,SZERO)) return;
		if (next)
		{
			r2_next = (r2 + 1) % FPA_NSHAD_REGS;
			r1_next = r1 + 1;
			if (write_datareg(r2_next,SONE,SZERO)) return;
		}

		inst = XSP_DIVIDE | X_REG2(r2);
		if (write_fpa((u_int *)inst,STHREE,ON)) return;
		inst = X_LSW | X_REG1(r1);
		if (write_fpa((u_int *)inst,SZERO,ON)) return;
		if (next)
		{
			inst = XSP_DIVIDE | X_REG2(r2_next);
			if (write_fpa((u_int *)inst,STHREE,ON)) return;
			inst = X_LSW | X_REG1(r1_next);
			if (write_fpa((u_int *)inst,SZERO,ON)) return;
			check_lock(2,r1,r1_next);
		}
		else check_lock(1,r1);
	}

	/* double precision extended test */
	for (r1=0, r2=1; r2 < FPA_NSHAD_REGS; r1++, r2++)
	{
		if (write_datareg(r2,DONE_M,DONE_L)) return;
		if (next)
		{
			r2_next = (r2 + 1) % FPA_NSHAD_REGS;
			r1_next = r1 + 1;
			if (write_datareg(r2_next,DONE_M,DONE_L)) return;
		}

		inst = XDP_DIVIDE | X_REG2(r2);
		if (write_fpa((u_int *)inst,DTHREE_M,ON)) return;
		inst = X_LSW | X_REG1(r1);
		if (write_fpa((u_int *)inst,DTHREE_M,ON)) return;
		if (next)
		{
			inst = XDP_DIVIDE | X_REG2(r2_next);
			if (write_fpa((u_int *)inst,DTHREE_M,ON)) return;
			inst = X_LSW | X_REG1(r1_next);
			if (write_fpa((u_int *)inst,DTHREE_L,ON)) return;
			check_lock(2,r1,r1_next);
		}
		else check_lock(1,r1);
	}
}


/*
 * Function to perform lock test using Weitek command register instructions:
 *	1) reg1 = sine(reg2) and reg4 = cosine(reg2), where reg2 = 1.
 *	2) reg1 = reg2 / reg3, where reg2 = 1 and reg3 = 3.
 * The first operation will hang and lock all shadow registers.
 * The second operation will hang and lock the shadow register assign to reg1.
 */
lock_cmdreg(next)
register int next;
{
	register u_int r1, r2, r4, data;
	register u_int r1_next, r2_next, r3_next;

	/* enable imask to hang operation */
	if (write_fpa(&fpa->fp_imask,FPA_INEXACT,ON)) return;

	/* single precision command register test */
	for (r1=0, r2=1, r4=2; r4 < FPA_NSHAD_REGS; r1++, r2++, r4++)
	{
		if (write_datareg(r2,SONE,SZERO)) return;

		if (next)
		{
			r3_next = (r4 + 1) % FPA_NSHAD_REGS;
			r2_next = r2 + 1;
			r1_next = r1 + 1;
			if (write_datareg(r2_next,SONE,SZERO)) return;
			if (write_datareg(r3_next,STHREE,SZERO)) return;
			data = C_REG3(r3_next) | C_REG2(r2_next) | C_REG1(r1_next);
			if (write_fpa((u_int *)CWSP_DIVIDE,data,ON)) return;
		}

		data = C_REG4(r4) | C_REG2(r2) | C_REG1(r1);
		if (write_fpa((u_int *)CSP_SINCOS,data,ON)) return;

		check_lock(FPA_NSHAD_REGS,0,1,2,3,4,5,6,7);
	}

	/* double precision command register test */
	for (r1=0, r2=1, r4=2; r4 < FPA_NSHAD_REGS; r1++, r2++, r4++)
	{
		if (write_datareg(r2,DONE_M,DONE_L)) return;

		if (next)
		{
			r3_next = (r4 + 1) % FPA_NSHAD_REGS;
			r2_next = r2 + 1;
			r1_next = r1 + 1;
			if (write_datareg(r2_next,DONE_M,DONE_L)) return;
			if (write_datareg(r3_next,DTHREE_M,DTHREE_L)) return;
			data = C_REG3(r3_next) | C_REG2(r2_next) | C_REG1(r1_next);
			if (write_fpa((u_int *)CWDP_DIVIDE,data,ON)) return;
		}

		data = C_REG4(r4) | C_REG2(r2) | C_REG1(r1);
		if (write_fpa((u_int *)CDP_SINCOS,data,ON)) return;

		check_lock(FPA_NSHAD_REGS,0,1,2,3,4,5,6,7);
	}
}


/*
 * Function to verify that all indicated shadow registers are locked, and
 * all others are not.
 * VARARGS.
 */
check_lock(va_alist)
va_dcl
{
	va_list ap;
	register int stat, reg, count, lock, n;
	u_int shadow[FPA_NSHAD_REGS], *addr, val, ierr_exp;

	/* get arguments for all shadow locks */
	va_start(ap);
	count = va_arg(ap,int);
	for (lock = 0; lock < count; lock++)
		shadow[lock] = va_arg(ap,u_int);
	va_end(ap);

	/* verify that each indicated shadow register is locked */
	for (reg = 0; reg < FPA_NSHAD_REGS; reg++)
	{
		/* see if this one is supposed to be locked */
		for (lock = 0; lock < count; lock++)
			if (shadow[lock] == reg) break;

		/* check both most and least significant of register */
		addr = (u_int *)&fpa->fp_shad[reg];
		for (n = 0; n < 2; n++)
		{
			stat = read_fpa(addr+n,&val,OFF);
			if (lock < count)		/* lock expected */
			{
				ierr_exp = FPA_HUNG_PIPE;
				if (!stat) send_message(FPERR,ERROR,er_nolock,reg);
			}
			else				/* no lock expected */
			{
				ierr_exp = 0;
				if (stat) send_message(FPERR,ERROR,er_lock,reg);
			}
			if (read_fpa(&fpa->fp_ierr,&val,ON)) return;
			if ((val &= IERR_MASK) != ierr_exp)
				send_message(FPERR,ERROR,er_lockierr,ierr_exp,val);
			if (write_fpa(&fpa->fp_ierr,0,ON)) return;
		}
	}

	/* clear pipe to unlock */
	if (write_fpa(&fpa->fp_clear_pipe,X,ON)) return;
}


/*
 * Function to perform jump condition test using command register instructions.
 */
jumpcond_test()
{
	register u_int data, dreg;
	u_int ms, ls, ms_exp;

	send_message(0,VERBOSE,tst_jumpcond);

	/* assign value to data register 1 thru 7 */
	for (dreg = 0; dreg < FPA_NSHAD_REGS; dreg++)
		if (write_datareg(dreg,jumpcond[dreg].data,SZERO)) return;

	/* execute instructions */
	for (dreg = 0; dreg < FPA_NSHAD_REGS; dreg++)
	{
		data = C_REG2(dreg) | C_REG1(dreg);
		if (write_fpa((u_int *)jumpcond[dreg].inst,data,ON)) return;
	}

	/* read all data registers and check result */
	for (dreg = 0; dreg < FPA_NSHAD_REGS; dreg++)
	{
		if (read_datareg(dreg,&ms,&ls)) return;
		ms_exp = jumpcond[dreg].result;
		if (ms != ms_exp || ls != SZERO)
			send_message(FPERR,ERROR,er_jumpcond,dreg,ms_exp,SZERO,ms,ls);
	}
}


/*
 * Function to test the TI data path using the TI addition and multiplication
 * single and double precision command register instructions.
 * For addition:	reg1 = reg2 + reg3,
 *		where reg2 = 0 and reg3 = different values.
 * For multiplication:	reg1 = reg2 * reg3,
 *		where reg2 = 1 and reg3 = different values.
 */
tipath_test()
{
	register u_int s, e, f, r1, r2, r3, data;
	u_int ms_exp, ls_exp, ms_obs, ls_obs;

	/* single precision operations */
	send_message(0,VERBOSE,tst_tipath,"single");
	r1 = 0; r2 = 1; r3 = 2;
	for (s = 0; s <= 1; s++)
	{
		for (e = 1; e < S_EMAX; e<<=1)
		{
			for (f = 0; f < S_FBITS; f++)
			{
				ms_exp = (s<<S_SIGNBIT) | (e<<S_FBITS) | (1<<f);
				ls_exp = SZERO;
				if (write_datareg(r3,ms_exp,ls_exp)) return;
				data = C_REG3(r3) | C_REG2(r2) | C_REG1(r1);
				if (write_datareg(r2,SZERO,ls_exp)) return;
				if (write_fpa((u_int *)CWSP_ADD,data,ON)) return;
				if (read_datareg(r1,&ms_obs,&ls_obs)) return;
				if (ms_obs != ms_exp || ls_obs != ls_exp)
					send_message(FPERR,ERROR,er_tipath,ms_exp,ls_exp,ms_obs,ls_obs);
				if (write_datareg(r2,SONE,X)) return;
				if (write_fpa((u_int *)CWSP_MULT,data,ON)) return;
				if (read_datareg(r1,&ms_obs,&ls_obs)) return;
				if (ms_obs != ms_exp || ls_obs != ls_exp)
					send_message(FPERR,ERROR,er_tipath,ms_exp,ls_exp,ms_obs,ls_obs);
				if (++r1 >= FPA_NDATA_REGS) r1 = 0;
				if (++r2 >= FPA_NDATA_REGS) r2 = 0;
				if (++r3 >= FPA_NDATA_REGS) r3 = 0;
			}
		}
	}

	/* double precision operations */
	send_message(0,VERBOSE,tst_tipath,"double");
	r1 = 0; r2 = 1; r3 = 2;
	for (s = 0; s <= 1; s++)
	{
		for (e = 1; e < D_EMAX; e<<=1)
		{
			for (f = 0; f < D_FBITS; f++)
			{
				ls_exp = 1 << f;
				ms_exp = (s<<(D_SIGNBIT-32)) | (e<<(D_FBITS-32));
				if (f >= 32) ms_exp += (1<<(f-32));

				if (write_datareg(r3,ms_exp,ls_exp)) return;
				data = C_REG3(r3) | C_REG2(r2) | C_REG1(r1);
				if (write_datareg(r2,DZERO_M,DZERO_L)) return;
				if (write_fpa((u_int *)CWDP_ADD,data,ON)) return;
				if (read_datareg(r1,&ms_obs,&ls_obs)) return;
				if (ms_obs != ms_exp || ls_obs != ls_exp)
					send_message(FPERR,ERROR,er_tipath,ms_exp,ls_exp,ms_obs,ls_obs);
				if (write_datareg(r2,DONE_M,DONE_L)) return;
				if (write_fpa((u_int *)CWDP_MULT,data,ON)) return;
				if (read_datareg(r1,&ms_obs,&ls_obs)) return;
				if (ms_obs != ms_exp || ls_obs != ls_exp)
					send_message(FPERR,ERROR,er_tipath,ms_exp,ls_exp,ms_obs,ls_obs);
				if (++r1 >= FPA_NDATA_REGS) r1 = 0;
				if (++r2 >= FPA_NDATA_REGS) r2 = 0;
				if (++r3 >= FPA_NDATA_REGS) r3 = 0;
			}
		}
	}
}


/*
 * Function to test most of FPA-3X instructions operations.
 */
tiop_test()
{
	send_message(0,VERBOSE,tst_tiop,"single precision short");
	tiop_spshort();
	send_message(0,VERBOSE,tst_tiop,"double precision short");
	tiop_dpshort();
	send_message(0,VERBOSE,tst_tiop,"single precision extended");
	tiop_spextend();
	send_message(0,VERBOSE,tst_tiop,"single precision 2-operand extended");
	tiop_spextend2();
	send_message(0,VERBOSE,tst_tiop,"double precision extended");
	tiop_dpextend();
	send_message(0,VERBOSE,tst_tiop,"single precision command register");
	tiop_spcmdreg();
	send_message(0,VERBOSE,tst_tiop,"double precision command register");
	tiop_dpcmdreg();
}


/*
 * Function to test single precision short operation.
 */
tiop_spshort()
{
	register u_int inst, r1, k, exp;
	u_int ms, ls;

	for (k = 0; spshort[k].inst; k++)
	{
		for (r1 = 0; r1 < FPA_NSHORT_REGS; r1++)
		{
			if (write_datareg(r1,spshort[k].reg1,SZERO)) return;
			inst = spshort[k].inst | SP_REG1(r1);
			if (write_fpa((u_int *)inst,spshort[k].op,ON)) return;
			if (read_datareg(r1,&ms,&ls)) return;
			exp = spshort[k].result;
			if (ms != exp || ls != SZERO)
				send_message(FPERR,ERROR,er_tiop,inst,exp,SZERO,ms,ls);
		}
	}
}


/*
 * Function to test double precision short operation.
 */
tiop_dpshort()
{
	register u_int inst, r1, k, exp;
	u_int ms, ls;

	for (k = 0; dpshort[k].inst; k++)
	{
		for (r1 = 0; r1 < FPA_NSHORT_REGS; r1++)
		{
			if (write_datareg(r1,dpshort[k].reg1,DZERO_L)) return;
			inst = dpshort[k].inst | DP_REG1(r1);
			if (write_fpa((u_int *)inst,dpshort[k].op,ON)) return;
			if (write_fpa((u_int *)DP_LSW,DZERO_L,ON)) return;
			if (read_datareg(r1,&ms,&ls)) return;
			exp = dpshort[k].result;
			if (ms != exp || ls != DZERO_L)
				send_message(FPERR,ERROR,er_tiop,inst,exp,DZERO_L,ms,ls);
		}
	}
}


/*
 * Function to test single precision extended operation.
 */
tiop_spextend()
{
	register u_int inst, r1, r2, exp, k;
	u_int ms, ls;

	for (k = 0; spext[k].inst; k++)
	{
		for (r1=0, r2=1; r2<FPA_NSHORT_REGS; r1++, r2++)
		{
			if (write_datareg(r1,SZERO,SZERO)) return;
			if (write_datareg(r2,spext[k].reg2,SZERO)) return;
			inst = spext[k].inst | X_REG2(r2);
			if (write_fpa((u_int *)inst,spext[k].op,ON)) return;
			inst = X_LSW | X_REG1(r1);
			if (write_fpa((u_int *)inst,X,ON)) return;
			if (read_datareg(r1,&ms,&ls)) return;
			exp = spext[k].reg1;
			if (ms != exp || ls != SZERO)
				send_message(FPERR,ERROR,er_tiop,inst,exp,SZERO,ms,ls);
		}
	}
}


/*
 * Function to test single precision extended operation with 2 operands.
 */
tiop_spextend2()
{
	register u_int inst, r1, r2, exp, k;
	u_int ms, ls;

	for (k = 0; spext2[k].inst; k++)
	{
		for (r1=0, r2=0; r2<FPA_NSHORT_REGS; r1++, r2++)
		{
			if (write_datareg(r1,SZERO,SZERO)) return;
			if (write_datareg(r2,spext2[k].reg2,SZERO)) return;
			inst = spext2[k].inst | X_REG2(r2);
			if (write_fpa((u_int *)inst,spext2[k].op1,ON)) return;
			inst = X_LSW | X_REG1(r1);
			if (write_fpa((u_int *)inst,spext2[k].op2,ON)) return;
			if (read_datareg(r1,&ms,&ls)) return;
			exp = spext2[k].reg1;
			if (ms != exp || ls != SZERO)
				send_message(FPERR,ERROR,er_tiop,inst,exp,SZERO,ms,ls);
		}
	}
}


/*
 * Function to test double precision extended operation.
 */
tiop_dpextend()
{
	register u_int inst, r1, r2, exp, k;
	u_int ms, ls;

	for (k = 0; dpext[k].inst; k++)
	{
		for (r1=0, r2=1; r2<FPA_NSHORT_REGS; r1++, r2++)
		{
			if (write_datareg(r1,DZERO_M,DZERO_L)) return;
			if (write_datareg(r2,dpext[k].reg2,DZERO_L)) return;
			inst = dpext[k].inst | X_REG2(r2);
			if (write_fpa((u_int *)inst,dpext[k].op,ON)) return;
			inst = X_LSW | X_REG1(r1);
			if (write_fpa((u_int *)inst,X,ON)) return;
			if (read_datareg(r1,&ms,&ls)) return;
			exp = dpext[k].reg1;
			if (ms != exp || ls != DZERO_L)
				send_message(FPERR,ERROR,er_tiop,inst,exp,DZERO_L,ms,ls);
		}
	}
}


/*
 * Function to test single precision command register operation.
 */
tiop_spcmdreg()
{
	register u_int inst, r1, r2, r3, r4, data, exp, k;
	u_int ms, ls;

	for (k = 0; inst = spcmd[k].inst; k++)
	{
		for (r1=0, r2=1, r3=2, r4=3; r4<FPA_NDATA_REGS; r1++, r2++, r3++, r4++)
		{
			if (write_datareg(r1,SZERO,SZERO)) return;
			if (write_datareg(r2,spcmd[k].reg2,SZERO)) return;
			if (write_datareg(r3,spcmd[k].reg3,SZERO)) return;
			if (write_datareg(r4,spcmd[k].reg4,SZERO)) return;
			data = C_REG1(r1) | C_REG2(r2) | C_REG3(r3) | C_REG4(r4);
			if (write_fpa((u_int *)inst,data,ON)) return;
			if (read_datareg(r1,&ms,&ls)) return;
			exp = spcmd[k].reg1;
			if (ms != exp || ls != SZERO)
				send_message(FPERR,ERROR,er_tiop,inst,exp,SZERO,ms,ls);
		}
	}
}


/*
 * Function to test double precision command register operation.
 */
tiop_dpcmdreg()
{
	register u_int inst, r1, r2, r3, r4, data, exp, k;
	u_int ms, ls;

	for (k = 0; inst = dpcmd[k].inst; k++)
	{
		for (r1=0, r2=1, r3=2, r4=3; r4<FPA_NDATA_REGS; r1++, r2++, r3++, r4++)
		{
			if (write_datareg(r1,DZERO_M,DZERO_L)) return;
			if (write_datareg(r2,dpcmd[k].reg2,DZERO_L)) return;
			if (write_datareg(r3,dpcmd[k].reg3,DZERO_L)) return;
			if (write_datareg(r4,dpcmd[k].reg4,DZERO_L)) return;
			data = C_REG1(r1) | C_REG2(r2) | C_REG3(r3) | C_REG4(r4);
			if (write_fpa((u_int *)inst,data,ON)) return;
			if (read_datareg(r1,&ms,&ls)) return;
			exp = dpcmd[k].reg1;
			if (ms != exp || ls != DZERO_L)
				send_message(FPERR,ERROR,er_tiop,inst,exp,DZERO_L,ms,ls);
		}
	}
}


/*
 * Function to test TI status by generating all possible status reported
 * to the WSTATUS register.
 */
tistatus_test()
{
	send_message(0,VERBOSE,tst_tistat,"alu");
	ti_status(alu);
	send_message(0,VERBOSE,tst_tistat,"multiplier");
	ti_status(mul);
}


/*
 * Function uses the specified test table to issue instruction then
 * verifies the TI status reported in WSTATUS register bits 8 thru 11.
 * Note that all Weitek operation takes the form:  reg1 = reg2 op reg 3.
 */
ti_status(tbl)
struct tistat_tbl *tbl;
{
	register u_int inst, data, k;
	u_int wstat, wstat_exp;

	for (k = 0; inst = tbl[k].inst; k++)
	{
		if (write_datareg(2,tbl[k].reg2_ms,tbl[k].reg2_ls)) return;
		if (write_datareg(3,tbl[k].reg3_ms,tbl[k].reg3_ls)) return;
		data = C_REG2(2) | C_REG3(3);
		if (write_fpa((u_int *)inst,data,ON)) return;
		if (read_fpa(&fpa->fp_wstatus_stable,&wstat,ON)) return;
		wstat &= TISTAT_MASK;
		wstat_exp = tbl[k].stat;
		if (fpa_version == FPA3X_VERSION)
		{
			if (wstat_exp == TI_FINEX) wstat_exp = TI_INFIN;
		}
		if (wstat != wstat_exp)
			send_message(FPERR,ERROR,er_tistat,wstat_exp,wstat);
		if (write_fpa(&fpa->fp_clear_pipe,X,ON)) return;
	}
}


/*
 * Function to test the TI timing when many similar instructions are
 * quickly issued in succession.
 */
timing_test()
{
	register int k;

	send_message(0,VERBOSE,tst_timing);

	for (k = 0; k < 100; k++)
	{
		timing_spshort();
		timing_dpshort();
		timing_extend();
		timing_cmdreg();
	}
}


/*
 * Function to test TI timing using single precision short instructions.
 */
timing_spshort()
{
	register u_int k, inst, r1;
	u_int ms, ls;

	r1 = 0;
	for (k = 0; inst = timsp[k].inst; k++)
	{
		if (write_datareg(r1,timsp[k].reg1,SZERO)) return;
		inst |= SP_REG1(r1);
		if (write_fpa((u_int *)inst,timsp[k].op,ON)) return;
		if (read_datareg(r1,&ms,&ls)) return;
		if (ms != timsp[k].result || ls != SZERO)
			send_message(FPERR,ERROR,er_timing,timsp[k].result,SZERO,ms,ls);
		if (++r1 >= FPA_NDATA_REGS) r1 = 0;
	}
}


/*
 * Function to test TI timing using double precision short instructions.
 */
timing_dpshort()
{
	register u_int k, inst, r1;
	u_int ms, ls;

	r1 = 0;
	for (k = 0; inst = timdp[k].inst; k++)
	{
		if (write_datareg(r1,timdp[k].reg1,DZERO_L)) return;
		inst |= SP_REG1(r1);
		if (write_fpa((u_int *)inst,timdp[k].op,ON)) return;
		if (write_fpa((u_int *)DP_LSW,DZERO_L,ON)) return;
		if (read_datareg(r1,&ms,&ls)) return;
		if (ms != timdp[k].result || ls != DZERO_L)
			send_message(FPERR,ERROR,er_timing,timdp[k].result,DZERO_L,ms,ls);
		if (++r1 >= FPA_NDATA_REGS) r1 = 0;
	}
}


/*
 * Function to test TI timing using extended instructions.
 */
timing_extend()
{
	register u_int k, inst, r1, r2, r3;
	u_int ms, ls;

	r1 = 0; r2 = 1; r3 = 2;
	for (k = 0; inst = timext[k].inst; k++)
	{
		if (write_datareg(r2,timext[k].reg2,DZERO_L)) return;
		if (write_datareg(r3,timext[k].reg3,DZERO_L)) return;
		inst |= X_REG2(r2);
		if (write_fpa((u_int *)inst,timext[k].op,ON)) return;
		inst = X_LSW | X_REG1(r1) | X_REG3(r3);
		if (write_fpa((u_int *)inst,DZERO_L,ON)) return;
		if (read_datareg(r1,&ms,&ls)) return;
		if (ms != timext[k].reg1 || ls != DZERO_L)
			send_message(FPERR,ERROR,er_timing,timext[k].reg1,DZERO_L,ms,ls);
		if (++r1 >= FPA_NDATA_REGS) r1 = 0;
		if (++r2 >= FPA_NDATA_REGS) r2 = 0;
		if (++r3 >= FPA_NDATA_REGS) r3 = 0;
	}
}


/*
 * Function to test TI timing using command register instructions.
 */
timing_cmdreg()
{
	register u_int k, inst, r1, r2, data;
	u_int ms, ls;

	r1 = 0; r2 = 1;
	for (k = 0; inst = timcmd[k].inst; k++)
	{
		if (write_datareg(r2,timcmd[k].ms2,timcmd[k].ls2)) return;
		data = C_REG2(r2) | C_REG1(r1);
		if (write_fpa((u_int *)inst,data,ON)) return;
		if (read_datareg(r1,&ms,&ls)) return;
		if (ms != timcmd[k].ms1 || ls != timcmd[k].ls1)
			send_message(FPERR,ERROR,er_timing,timcmd[k].ms1,timcmd[k].ls1,ms,ls);
		if (++r1 >= FPA_NDATA_REGS) r1 = 0;
		if (++r2 >= FPA_NDATA_REGS) r2 = 0;
	}
}


