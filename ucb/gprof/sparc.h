/*	@(#)sparc.h 1.1 92/07/30 SMI	*/

    /*
     *	opcodes of the call instructions
     */
    /*
     *	offset (in bytes) of the code from the entry address of a routine.
     *	(see asgnsamples for use and explanation.)
     */
#define OFFSET_OF_CODE	0 /* there is no mask on a SPARC */
#define	UNITS_TO_CODE	(OFFSET_OF_CODE / sizeof(UNIT))
    /*
     *  address at which text begins N_TXTADDR is defined in a.out.h
     */
#define TORIGIN ((unsigned long) N_TXTADDR(xbuf))
    /*
     *  Macros for manipulating instruction fields.  These use the
     *  structures defined below
     */
#define OP(x)		(((union instruct *) (x))->f_1.op)
#define DISP30(x)	(((union instruct *) (x))->f_1.disp30)
#define OP3(x)		(((union instruct *) (x))->f_3c.op3)
#define RD(x)		(((union instruct *) (x))->f_3c.rd)
#define IMMED(x)	(((union instruct *) (x))->f_3c.i)
#define SIMM13(x)	(((((union instruct *) (x))->f_3d.simm13) << 19) >> 19)
#define RS1(x)		(((union instruct *) (x))->f_3c.rs1)
#define RS2(x)		(((union instruct *) (x))->f_3c.rs2)
    /*
     *  a few values for operand and register fields
     */
#define CALL		0x1
#define FMT3_0x10	0x2
#define JMPL		0x38
#define R_G0		0x0
#define R_O7		0xF
#define R_I7		0x1F
    /*
     *  A macro for converting from instructp to the appropriate address in
     *  the program
     */
#define PC_VAL(x)	((x) - (unsigned long) textspace + TORIGIN)
	/*
	 *	structures for decoding instructions
	 */
struct f_1 {
    unsigned long   op:2,
		    disp30:30
};
struct f_2a {
    unsigned long   op:2,
		    rd:5,
		    op2:3,
		    imm22:22
};
struct f_2b {
    unsigned long   op:2,
		    a:1,
		    cond:4,
		    op2:3,
		    disp22:22
};
struct f_3a {
    unsigned long   op:2,
		    ign1:1,
		    cond:4,
		    op3:6,
		    rs1:5,
		    i:1,
		    ign2:8,
		    rs2:5
};
struct f_3b {
    unsigned long   op:2,
		    ign1:1,
		    cond:4,
		    op3:6,
		    rs1:5,
		    i:1,
		    simm13:13
};
struct f_3c {
    unsigned long   op:2,
		    rd:5,
		    op3:6,
		    rs1:5,
		    i:1,
		    asi:8,
		    rs2:5
};
struct f_3d {
    unsigned long   op:2,
		    rd:5,
		    op3:6,
		    rs1:5,
		    i:1,
		    simm13:13
};
struct f_3e {
    unsigned long   op:2,
		    rd:5,
		    op3:6,
		    rs1:5,
		    opf:9,
		    rs2:5
};

union instruct {
	struct f_1	f_1;
	struct f_2a	f_2a;
	struct f_2b	f_2b;
	struct f_3a	f_3a;
	struct f_3b	f_3b;
	struct f_3c	f_3c;
	struct f_3d	f_3d;
	struct f_3e	f_3e;
};

