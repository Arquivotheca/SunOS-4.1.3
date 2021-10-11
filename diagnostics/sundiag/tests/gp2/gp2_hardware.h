
/*      @(#)gp2_hardware.h 1.1 88/04/21 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */
/*
 *      GPCI Diagnostic sub commands for GP2
 */
#define GPCI_DIAG 0x7f000000
/*
 *      XP diag sub commands
 */

#define XPMEMTST        0x300
#define XPRDLRAM        0x301
#define XPSEQ           0x302
#define XPALUTST        0x303
#define XPFIFO          0x304

/*
 *      RP diag sub commands
 */

#define RPMEMTST        0x200
#define RPRDLRAM        0x201
#define RPSEQ           0x202
#define RPALUTST        0x203
#define RPFIFO          0x205
/*
 *      PP diag sub commands
 */
#define PPFIFO          0x75
#define PPLDXAGO        0x76
#define PPADYAGO        0x77
#define PPADXAGO        0x78
#define PPXCEAGO        0x79
#define PPYCEAGO        0x7a
#define PPXYCEAGO       0x7b
#define PPSEQ           0x7c
#define PPALU           0x7d
#define PPRW            0x7e
#define PPZBUF          0x7f

#define PP_LD_REG 	1

u_long seq_tst[12][4] = {
    {0,0xfde23ceb,0xfde23ceb,0}, /* eqz */
    {0,0xfde23ceb,0xfde13ceb,1}, /* eqz */
    {1,0x00000002,0x00000002,1}, /* nez */
    {1,0x12345678,0x12345677,0}, /* nez */
    {2,0x00000334,0x00000034,0}, /* gez */
    {2,0x00000334,0x00000334,0}, /* gez */
    {3,0x00000004,0x0000ffff,0}, /* ltz */
    {4,0x70000001,0x70000000,0}, /* gtz */
    {5,0x07777777,0x07777777,0}, /* lez */
    {5,0x80000000,0x07777777,0}, /* lez */
    {6,0x00000044,0x00000044,0}, /* nov */
    {7,0x80000000,0xffffffff,1}, /* ovf */
};

u_long alu_tst[8][4] = {
    {0,0x30,0x40,0x70},                   /* add */
    {1,0x40,0x30,0x10},                   /* sub */
    {2,0xffffffff,0x12345678,0xedcba987}, /* xor */
    {3,0xaaaa5555,0xffffffff,0xaaaa5555}, /* xnor */
    {4,0xffffffff,0x12345678,0x12345678}, /* and */
    {5,0xaa55aa55,0x55aa55aa,0xffffffff}, /* or */
    {6,0x99339933,0x66cc66cc,0x00000000}, /* nor */
    {7,0x12345678,0x00000000,0xedcba987}, /* not */
};

u_long ppseq_tst[12][4] = {
    {0,0xfde23ceb,0xfde23ceb,1}, /* eq */
    {0,0xfde23ceb,0xfde13ceb,0}, /* eq */
    {1,0x00000002,0x00000002,0}, /* ne */
    {1,0x12345678,0x12345677,1}, /* ne */
    {2,0x70000001,0x70000000,1}, /* gt */
    {2,0x00000334,0x00000334,0}, /* gt */
    {3,0x00000004,0x00000004,1}, /* le */
    {3,0x00000004,0x00000005,1}, /* le */
    {4,0x70000001,0x70000001,1}, /* ge */
    {4,0x00600600,0x70000000,0}, /* ge */
    {5,0x00000004,0x00007777,1}, /* lt */
    {5,0x0000ffff,0x00000004,0}, /* lt */
};

u_long ppalu_tst[8][4] = {
    {0,0x30,0x40,0x70},                   /* add */
    {1,0x40,0x30,0xfffffff0},             /* sub */
    {2,0x30,0x40,0xfffffff0},             /* subr */
    {3,0xaa55aa55,0x55aa55aa,0xffffffff}, /* or */
    {4,0xffffffff,0x12345678,0x12345678}, /* and */
    {5,0x12345678,0xffffffff,0xedcba987}, /* notrs */
    {6,0xffffffff,0x12345678,0xedcba987}, /* exor */
    {7,0xaaaa5555,0xffffffff,0xaaaa5555}, /* exnor */
};
