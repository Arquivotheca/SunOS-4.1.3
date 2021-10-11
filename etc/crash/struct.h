#include <stab.h>
#include <machine/pte.h>
#include <sys/core.h>
#include <sys/user.h>

#define addr_t unsigned
struct adb_raddr {
    enum reg_type { r_normal, r_gzero, r_window, r_floating,  r_invalid }
	     ra_type;
    int	    *ra_raddr;
} ;

/* Integer Unit (IU)'s "r registers" */
#define REG_RN(n)	((n) - FIRST_STK_REG)
#define REG_G0		 0
#define REG_G1		 1
#define REG_G2		 2
#define REG_G3		 3
#define REG_G4		 4
#define REG_G5		 5
#define REG_G6		 6
#define REG_G7		 7
#define REG_O0		 8
#define REG_O1		 9
#define REG_O2		10
#define REG_O3		11
#define REG_O4		12
#define REG_O5		13
#define REG_O6		14
#define REG_O7		15
#define REG_L0		16
#define REG_L1		17
#define REG_L2		18
#define REG_L3		19
#define REG_L4		20
#define REG_L5		21
#define REG_L6		22
#define REG_L7		23
#define REG_I0		24
#define REG_I1		25
#define REG_I2		26
#define REG_I3		27
#define REG_I4		28
#define REG_I5		29
#define REG_I6		30
#define REG_I7		31	/* (Return address minus eight) */
#define REG_SP		14	/* Stack pointer == REG_O6 */
#define	REG_FP		30	/* Frame pointer == REG_I6 */

/* Miscellaneous registers */
#define	REG_Y		32
#define	REG_PSR		33
#define	REG_WIM		34
#define	REG_TBR		35
#define	REG_PC		36
#define	REG_NPC		37
#define LAST_NREG	37	/* last normal (non-Floating) register */
#define REG_FSR		38	/* FPU status */
#define REG_FQ		39	/* FPU queue */

/* Floating Point Unit (FPU)'s "f registers" */
#define REG_F0		40
#define REG_F16		56
#define REG_F31		71

#define	NREGISTERS	(REG_F31+1)
#define	FIRST_STK_REG	REG_O0
#define	LAST_STK_REG	REG_I5
#define NWINDOW 7 /* # of implemented windows */
#define	MAXINT		0x7fffffff
#define	MAXFILE		0xffffffff

#define REG_WINDOW( win, reg ) \
	( NREGISTERS - REG_O0 + (win)*(16) + (reg))
#define MIN_WREGISTER REG_WINDOW( 0,         REG_O0 )
#define MAX_WREGISTER REG_WINDOW( NWINDOW-1, REG_I7 )
#define	REGADDR(r)	(4 * (r))

    ;
/*
 * A "stackpos" contains everything we need to know in
 * order to do a stack trace.
 */
struct stackpos {
	 u_int	k_pc;		/* where we are in this proc */
	 u_int	k_fp;		/* this proc's frame pointer */
	 u_int	k_nargs;	/* This we can't figure out on sparc */
	 u_int	k_entry;	/* this proc's entry point */
	 u_int	k_caller;	/* PC of the call that called us */
	 u_int	k_flags;	/* sigtramp & leaf info */
	 u_int	k_regloc[ LAST_STK_REG - FIRST_STK_REG +1 ];
};
	/* Flags for k_flags:  */
#define K_LEAF		1	/* this is a leaf procedure */
#define K_CALLTRAMP	2	/* caller is _sigtramp */
#define K_SIGTRAMP	4	/* this is _sigtramp */
#define K_TRAMPED	8	/* this was interrupted by _sigtramp */
#define NARG_REGS 6
#define FR_L0		(0*4)	/* (sr_frame).fr_w.w_local[ 0 ] */
#define FR_LREG(reg)	(FR_L0 + (((reg)-REG_L0) * 4))
#define FR_I0		(8*4)	/* (sr_frame).fr_w.w_in[ 0 ] */
#define FR_IREG(reg)	(FR_I0 + (((reg)-REG_I0) * 4))
#define FR_SAVFP	(14*4)	/* (sr_frame).fr_w.w_in[ 6 ] */
#define FR_SAVPC	(15*4)	/* (sr_frame).fr_w.w_in[ 7 ] */
#define FR_ARG0         (17*4)
#define FR_ARG1         (18*4)
#define FR_ARG2         (19*4)
#define FR_XTRARGS	(23*4)  /* >6 Outgoing parameters */

/* Values for the main "op" field, bits <31..30> */
#define SR_FMT2_OP		0
#define SR_CALL_OP		1
#define SR_FMT3a_OP		2

/* Values for the tertiary "op3" field, bits <24..19> */
#define SR_JUMP_OP		0x38

/* Address space constants	  cmd	which space		*/
#define	NSP	0		/* =	(no- or number-space)	*/
#define	ISP	1		/* ?	(object/executable)	*/
#define	DSP	2		/* /	(core file)		*/
#define	SSP	3		/* @	(source file addresses) */
#define	STAR	4
#define	NSYM	0		/* no symbol space */
#define	ISYM	1		/* symbol in I space */
#define	DSYM	1		/* symbol in D space (== I space on VAX) */
#define	NVARS	10+26+26	/* variables [0..9,a..z,A..Z] */

#define	FILEX(file,line)	(((file)<<(2*NBBY))|(line))
#define	XLINE(filex)		((filex)&((1<<(2*NBBY))-1))
#define INFINITE        0x7fffffff
#define GSEGSIZE        150  /* allocate global symbol buckets 150 per shot */
#define HSIZE   255
#define adb_oreg (adb_regs.r_outs)
#define adb_ireg (adb_regs.r_ins)
#define adb_lreg (adb_regs.r_locals)
char	*exform(), *savestr(), *errflg, *corfil, *symfil;

struct allregs {
        int             r_psr;
        int             r_pc;
        int             r_npc;
        int             r_tbr;
        int             r_wim;
        int             r_y;
        int             r_globals[7];
        int             r_outs[8];
        int             r_locals[8];
        int             r_ins[8];
}adb_regs;
#undef s_name
struct asym {
	char	*s_name;
	int	s_flag;
	int	s_type;
	struct	afield *s_f;
	int	s_fcnt, s_falloc;
	int	s_value;
	int	s_fileloc;
	struct	asym *s_link;
} **globals;
int	nglobals, NGLOBALS;

struct afield {
	char	*f_name;
	int	f_type;
	int	f_offset;
	struct	afield *f_link;
} *fields;
int	nfields, NFIELDS;

struct linepc {
	int	l_fileloc;
	int	l_pc;
} *linepc, *linepclast, *pcline;
int	nlinepc, NLINEPC;

/* General 32-bit sign extension macro */
/* The (long) cast is necessary in case we're given unsigned arguments */
#define SR_SEXT(sex,size) ((((long)(sex)) << (32 - size)) >> (32 - size))

/* Change a word address to a byte address (call & branches need this) */
#define SR_WA2BA(woff) ((woff) << 2)

/* Instruction Field Width definitions */
#define IFW_OP		2
#define IFW_DISP30	30

struct fmt1Struct {
	u_int op		:IFW_OP;	/* op field = 00 for format 1 */
	u_int disp30		:IFW_DISP30;	/* PC relative, word aligned,
								disp. */
};

#define X_OP(x)		( ((struct fmt1Struct *)&(x))->op )	
#define OP		X_OP(instruction)
#define AOUT 1
#define IFW_RD		5
#define IFW_OP2	3
#define IFW_DISP22	22
#define SR_SEX22( sex ) SR_SEXT(sex, IFW_DISP22 )
#define SR_HI(disp22) ((disp22) << (32 - IFW_DISP22) )

struct fmt2Struct {
	u_int op		:IFW_OP;	/* op field = 01 for format 2 */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op2		:IFW_OP2;	/* opcode for format 2
							instructions */
	u_int disp22		:IFW_DISP22;	/* 22 bit displacement */
};

#define X_RD(x)		( ((struct fmt2Struct *)&(x))->rd )
#define X_OP2(x)	( ((struct fmt2Struct *)&(x))->op2 )
#define X_DISP22(x)	( ((struct fmt2Struct *)&(x))->disp22 )

#define IFW_ANNUL	1
#define IFW_COND	4
struct condStruct {
	u_int op		:IFW_OP;
	u_int annul		:IFW_ANNUL;	/* annul field for BICC,
								and BFCC */
	u_int cond		:IFW_COND;	/* cond field */
	u_int op2		:IFW_OP2;
	u_int disp22		:IFW_DISP22;
};
#define X_COND(x)	( ((struct condStruct *)&(x))->cond )

#define IFW_OP3		6
#define IFW_RS1		5
#define IFW_IMM		1
#define IFW_OPF		8
#define IFW_RS2		5

struct fmt3Struct {
	u_int op		:IFW_OP;	/* op field = 1- for format 3 */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op3		:IFW_OP3;	/* opcode distinguishing
							fmt3 instrns */
	u_int rs1		:IFW_RS1;	/* register source 1 */
	u_int imm		:IFW_IMM;	/* immediate. distinguishes fmt3
						and fmt3I */
	u_int opf		:IFW_OPF;	/* floating-point opcode */
	u_int rs2		:IFW_RS2;	/* register source 2 */
};

#define X_OP3(x)	( ((struct fmt3Struct *)&(x))->op3 )
#define X_RS1(x)	( ((struct fmt3Struct *)&(x))->rs1 )
#define X_IMM(x)	( ((struct fmt3Struct *)&(x))->imm )
#define X_OPF(x)	( ((struct fmt3Struct *)&(x))->opf )
#define X_RS2(x)	( ((struct fmt3Struct *)&(x))->rs2 )

#define IFW_SIMM13	13
#define SR_SEX13( sex ) SR_SEXT(sex, 13 )

struct fmt3IStruct {
	u_int op		:IFW_OP;	/* op field = 1- for format
									3I */
	u_int rd		:IFW_RD;	/* destination register */
	u_int op3		:IFW_OP3;	/* opcode distinguishing
								fmt3 instrns */
	u_int rs1		:IFW_RS1;	/* register source 1 */
	u_int imm		:IFW_IMM;	/* immediate. distinguishes fmt3
						and fmt3I */
	u_int simm13		:IFW_SIMM13;	/* immediate source 2 */
};

#define X_SIMM13(x)	( ((struct fmt3IStruct *)&(x))->simm13 )
#define SR_SETHI_OP             4
#define SR_OR_OP 	        2

union {
	struct	user U;
	char	UU[ctob(UPAGES)];
} udot;
#define	u	udot.U

struct map_range {
	int					mpr_b,mpr_e,mpr_f;
	char				*mpr_fn;
	int					mpr_fd;
	struct map_range	*mpr_next;
};

struct map {
	struct map_range	*map_head, *map_tail;
} txtmap, datmap;

/*
 * Structure describing a source file.
 */
struct	file_new {
	char	*f_name;		/* name of this file */
	int	f_flags;		/* see below */
	off_t	*f_lines;		/* array of seek pointers */
/* line i stretches from lines[i-1] to lines[i] - 1, if first line is 1 */
	int	f_nlines;		/* number of lines in file */
/* lines array actually has nlines+1 elements, so last line is bracketed */
} *file_new, *filenfile;

int     nfile, NFILE, curfile, hadaddress, count, wtflag;
int	dot, dotinc, pid, fcor, fsym;
int     pclcmp(), lpccmp(), symvcmp();
unsigned int difference;
static int      SHLIB_OFFSET;

struct  asym *nextglob, *global_segment;
struct  asym *trampsym, *funcsym, *curfunc, *curcommon, *lookup(), *cursym;
struct  afield *fieldhash[HSIZE], *globalfield();
struct  user *uarea;    /* a ptr to the currently mapped uarea */
struct	core	 core;
struct	adb_raddr adb_raddr;
struct	exec filhdr;
struct pcb u_pcb ;

extern	errno;
extern char *corfile;
