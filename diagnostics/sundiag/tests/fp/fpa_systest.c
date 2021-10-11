
/*
 * static char     fpasccsid[] = "@(#)fpa_systest.c 1.1 7/30/92 Copyright Sun Microsystems"; 
 *
 * Copyright (c) 1985 by Sun Microsystems, Inc 
 *
 *
 *
 *
 * Author : Chad B. Rao 
 *
 * Date   : 1/16/86       ....   Revision A 3/3/86               Revision B
 * Clayton Woo Modified to fit under sysdiag enviroment 
 */
#include <sys/types.h>
#include "fpa.h"
#include <sys/ioctl.h>
#include <sundev/fpareg.h>
#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/file.h>
#include <math.h>
#include <signal.h>
#include "fpcrttypes.h"

#define		SYSTEST_ERROR	6
#define		FATAL		2

typedef	void	(*func_ptr)();	/* to be used for casting */

char           *err_msg[] = {

			     "Ierr",
			     "Imask",
			     "Ldptr",
			     "Mapping Ram",
			     "Micro Store Ram",
			     "Register Ram Upper Half",
			     "Simple Instruction",
			     "Register Ram Lower Half",
			     "Shadow Ram",
			     "Pointer",
			     "Pointer Incr. Decr.",
			     "Lock",
			     "F+",
			     "Mode Register",
			     "Wstatus Register",
			     "Weitek Data Path",
			     "Weitek Operation",
			     "Weitek Status",
			     "Jump Conditions",
			     "Timing",
			     "Linpack"

};
struct shadow_regs {
    u_long          shreg_msw;
    u_long          shreg_lsw;
};
struct shadow_regs shadow[] = {

			       0xE0000E00, 0xE0000E04,
			       0xE0000E08, 0xE0000E0C,
			       0xE0000E10, 0xE0000E14,
			       0xE0000E18, 0xE0000E1C,
			       0xE0000E20, 0xE0000E24,
			       0xE0000E28, 0xE0000E2C,
			       0xE0000E30, 0xE0000E34,
			       0xE0000E38, 0xE0000E3C
};
/* Lock test */
struct registers {
    u_long          reg_msw;
    u_long          reg_lsw;
};
struct registers user[] = {

			   0xE0000C00, 0xE0000C04,
			   0xE0000C08, 0xE0000C0C,
			   0xE0000C10, 0xE0000C14,
			   0xE0000C18, 0xE0000C1C,
			   0xE0000C20, 0xE0000C24,
			   0xE0000C28, 0xE0000C2C,
			   0xE0000C30, 0xE0000C34,
			   0xE0000C38, 0xE0000C3C,
			   0xE0000C40, 0xE0000C44,
			   0xE0000C48, 0xE0000C4C,
			   0xE0000C50, 0xE0000C54,
			   0xE0000C58, 0xE0000C5C,
			   0xE0000C60, 0xE0000C64,
			   0xE0000C68, 0xE0000C6C,
			   0xE0000C70, 0xE0000C74,
			   0xE0000C78, 0xE0000C7C,
			   0xE0000C80, 0xE0000C84,
			   0xE0000C88, 0xE0000C8C,
			   0xE0000C90, 0xE0000C94,
			   0xE0000C98, 0xE0000C9C,
			   0xE0000CA0, 0xE0000CA4,
			   0xE0000CA8, 0xE0000CAC,
			   0xE0000CB0, 0xE0000CB4,
			   0xE0000CB8, 0xE0000CBC,
			   0xE0000CC0, 0xE0000CC4,
			   0xE0000CC8, 0xE0000CCC,
			   0xE0000CD0, 0xE0000CD4,
			   0xE0000CD8, 0xE0000CDC,
			   0xE0000CE0, 0xE0000CE4,
			   0xE0000CE8, 0xE0000CEC,
			   0xE0000CF0, 0xE0000CF4,
			   0xE0000CF8, 0xE0000CFC
};

/*
 * struct shadow_regs shadow[] = { 
 *
 * 0xE0000E00,   0xE0000E04, 0xE0000E08,   0xE0000E0C, 0xE0000E10,   0xE0000E14,
 * 0xE0000E18,   0xE0000E1C, 0xE0000E20,   0xE0000E24, 0xE0000E28,  
 * 0xE0000E2C, 0xE0000E30,   0xE0000E34, 0xE0000E38,   0xE0000E3C }; 
 */

struct dp_short {
    u_long          addr;
};
/*
 * struct dp_short dps[] = { 
 *
 * 0xE0000384, 0xE000038C, 0xE0000394, 0xE000039C, 0xE00003A4, 0xE00003AC,  
 * 0xE00003B4,  0xE00003BC, 0xE00003C4, 0xE00003CC, 0xE00003D4, 0xE00003DC,
 * 0xE00003E4, 0xE00003EC, 0xE00003F4, 0xE00003FC }; 
 */
struct dp_short dps[] = {

			 0xE0000504,
			 0xE000050C,
			 0xE0000514,
			 0xE000051C,
			 0xE0000524,
			 0xE000052C,
			 0xE0000534,
			 0xE000053C,
			 0xE0000544,
			 0xE000054C,
			 0xE0000554,
			 0xE000055C,
			 0xE0000564,
			 0xE000056C,
			 0xE0000574,
			 0xE000057C
};


struct dp_ext {
    u_long          addr;
    u_long          addr_lsw;
};

struct dp_ext   ext[] = {

			 0xE0001484, 0xE0001800,
			 0xE000148C, 0xE0001808,
			 0xE0001494, 0xE0001810,
			 0xE000149C, 0xE0001818,
			 0xE00014A4, 0xE0001820,
			 0xE00014AC, 0xE0001828,
			 0xE00014B4, 0xE0001830,
			 0xE00014BC, 0xE0001838,
			 0xE00014C4, 0xE0001840,
			 0xE00014CC, 0xE0001848,
			 0xE00014D4, 0xE0001850,
			 0xE00014DC, 0xE0001858,
			 0xE00014E4, 0xE0001860,
			 0xE00014EC, 0xE0001868,
			 0xE00014F4, 0xE0001870,
			 0xE00014FC, 0xE0001878
};

struct dp_cmd {
    u_long          data;
};
struct dp_cmd   nxt_cmd[] = {

			     0x000,
			     0x041,
			     0x082,
			     0x0C3,
			     0x104,
			     0x145,
			     0x186,
			     0x1C7,
			     0x208,
			     0x249,
			     0x28A,
			     0x2CB,
			     0x30C,
			     0x34D,
			     0x38E,
			     0x3CF,
			     0x410,
			     0x452,
			     0x493,
			     0x4D4,
			     0x515,
			     0x556,
			     0x597,
			     0x5D8,
			     0x619,
			     0x65A,
			     0x69B,
			     0x6DC,
			     0x71D,
			     0x75E,
			     0x79F
};
struct dp_cmd   cmd[] = {

			 0x020040,
			 0x030081,
			 0x0400C2,
			 0x050103,
			 0x060144,
			 0x070185,
			 0x0801C6,
			 0x090207,
			 0x0A0248,
			 0x0B0289,
			 0x0C02CA,
			 0x0D030B,
			 0x0E034C,
			 0x0F038D,
			 0x1003CE,
			 0x11040F,
			 0x120450,
			 0x130491,
			 0x1404D2,
			 0x150513,
			 0x160554,
			 0x170595,
			 0x1805D6,
			 0x190617,
			 0x1A0658,
			 0x1B0699,
			 0x1C06DA,
			 0x1D071B,
			 0x1E075C,
			 0x1F079D
};

/* Pointer test */

struct pointers {

    u_long          start_addr;		       /* starting address */
    u_long          incr;		       /* increment */
    u_long          no_times;		       /* number of times to do */
    u_long          dp_addr;		       /* starting address for dp */
    u_long          dp_incr;		       /* increment for dp address */
    u_long          reg_value;		       /* value to be put into
					        * register */
    u_long          opr_value;		       /* value used as an oeprand */
    u_long          result;		       /* the result to be checked
					        * against */
};
struct pointers shrt[] =
{
 {0xE0000380, 8, 16, 0xE0001000, 0, 0x40000000, 0x40000000, 0x41000000},
 {0xE0000384, 8, 16, 0xE0001000, 0, 0x40000000, 0x40180000, 0x40200000},
 {0x0, 0, 0, 0x0, 0, 0, 0, 0}
};
struct pointers extend[] =
{
 {0xE0001300, 8, 16, 0xE0001800, 8, 0, 0, 0},
 {0xE0001304, 8, 16, 0xE0001800, 8, 0, 0, 0},
 {0x0, 0, 0, 0x0, 0, 0, 0, 0}
};
u_long          sp_short[] =
{
 0xE0000380,
 0xE0000388,
 0xE0000390,
 0xE0000398,
 0xE00003A0,
 0xE00003A8,
 0xE00003B0,
 0xE00003B8,
 0xE00003C0,
 0xE00003C8,
 0xE00003D0,
 0xE00003D8,
 0xE00003E0,
 0xE00003E8,
 0xE00003F0,
 0xE00003F8
};
u_long          dp_short[] =
{
 0xE0000384,
 0xE000038C,
 0xE0000394,
 0xE000039C,
 0xE00003A4,
 0xE00003AC,
 0xE00003B4,
 0xE00003BC,
 0xE00003C4,
 0xE00003CC,
 0xE00003D4,
 0xE00003DC,
 0xE00003E4,
 0xE00003EC,
 0xE00003F4,
 0xE00003FC
};
struct single_ext {
    u_long          high;
    u_long          low;
};
struct single_ext dp_extd[] =
{
 0xE000180C, 0xE0001900,
 0xE0001814, 0xE0001988,
 0xE000181C, 0xE0001A10,
 0xE0001824, 0xE0001A98,
 0xE000182C, 0xE0001B20,
 0xE0001834, 0xE0001BA8,
 0xE000183C, 0xE0001C30,
 0xE0001844, 0xE0001CB8,
 0xE000184C, 0xE0001D40,
 0xE0001854, 0xE0001DC8,
 0xE000185C, 0xE0001E50,
 0xE0001864, 0xE0001ED8,
 0xE000186C, 0xE0001F60,
 0xE0001874, 0xE0001FE8,
 0xE000187C, 0xE0001870,
 0xE0001804, 0xE00018F8
};
struct single_ext sp_extd[] =
{
 0xE0001008, 0xE0001800,
 0xE0001010, 0xE0001808,
 0xE0001018, 0xE0001810,
 0xE0001020, 0xE0001818,
 0xE0001028, 0xE0001820,
 0xE0001030, 0xE0001828,
 0xE0001038, 0xE0001830,
 0xE0001040, 0xE0001838,
 0xE0001048, 0xE0001840,
 0xE0001050, 0xE0001848,
 0xE0001058, 0xE0001850,
 0xE0001060, 0xE0001858,
 0xE0001068, 0xE0001860,
 0xE0001070, 0xE0001868,
 0xE0001078, 0xE0001870,
 0xE0001000, 0xE0001878
};
u_long          cmd_reg[] =
{
 0x0C020040,
 0x10030081,
 0x140400C2,
 0x18050103,
 0x1C060144,
 0x20070185,
 0x240801C6,
 0x28090207,
 0x2C0A0248,
 0x300B0289,
 0x340C02CA,
 0x380D030B,
 0x3C0E034C,
 0x400F038D,
 0x441003CE,
 0x4811040F,
 0x4C120450,
 0x50130491,
 0x541404D2,
 0x58150513,
 0x5C160554,
 0x60170595,
 0x641805D6,
 0x68190617,
 0x6C1A0658,
 0x701B0699,
 0x741C06DA,
 0x781D071B,
 0x7C1E075C
};
struct sp_dp_cmd {
    u_long          reg1;
    u_long          reg2;
    u_long          reg3;
    u_long          res;
};
struct sp_dp_cmd sp_dp_res[] =
{
 0x40000000, 0x41000000, 0x40800000, 0x41800000,
 0x40000000, 0x40200000, 0x40100000, 0x40300000
};


/* ptr_indec test */

struct REGS {
    u_long          reg;
};

struct REGS     users[] = {

			   0xE0000C00,
			   0xE0000C08,
			   0xE0000C10,
			   0xE0000C18,
			   0xE0000C20,
			   0xE0000C28,
			   0xE0000C30,
			   0xE0000C38,
			   0xE0000C40,
			   0xE0000C48,
			   0xE0000C50,
			   0xE0000C58,
			   0xE0000C60,
			   0xE0000C68,
			   0xE0000C70,
			   0xE0000C78,
			   0xE0000C80,
			   0xE0000C88,
			   0xE0000C90,
			   0xE0000C98,
			   0xE0000CA0,
			   0xE0000CA8,
			   0xE0000CB0,
			   0xE0000CB8,
			   0xE0000CC0,
			   0xE0000CC8,
			   0xE0000CD0,
			   0xE0000CD8,
			   0xE0000CE0,
			   0xE0000CE8,
			   0xE0000CF0,
			   0xE0000CF8
};
struct ptr_command {
    u_long          data;
};
struct ptr_command ptr_cmd[] =
{
 0x10005,
 0x20046,
 0x30087,
 0x400C8,
 0x50109,
 0x6014A,
 0x7018B,
 0x801CC,
 0x9020D,
 0xA024E,
 0xB028F,
 0xC02D0,
 0xD0311,
 0xE0352,
 0xF0393,
 0x1003D4,
 0x110415,
 0x120456,
 0x130497,
 0x1404D8,
 0x150519,
 0x16055A,
 0x17059B,
 0x1805DC,
 0x19061D,
 0x1A065E,
 0x1B069F
};
u_long          val[] = {

			 0x3FF00000,	       /* for dp 1 */
			 0x40000000,	       /* for dp 2 */
			 0x40080000,	       /* for dp 3 */
			 0x40100000,	       /* for dp 4 */
			 0x40140000	       /* for dp 5 */
};

/* wlwf test */
struct fvalue {
    u_long          addr;
    u_long          reg2;
    u_long          reg3;
    u_long          result;
};


struct fvalue   fval[] = {

			  0xE0000A00, 0x40800000, 0x40400000, 0x3F800000,
/* single subtract, f32 - f32, 4 - 3 = 1 */
			  0xE0000A04, 0x40100000, 0x40080000, 0x3FF00000,
/* double subtract, f32/64 - f32/64, 4 - 3 = 1 */
			  0xE0000A10, 0xC0000000, 0x40000000, 0x40800000,
/* single magnitude of difference, |f32 - f32|, |-2 - 2| = 4 */
			  0xE0000A20, 0xC0800000, 0xC0000000, 0x40000000,
/* single subtract of magnitudes, |f32| - |f32|, |-4| - |-2| = 2 */
			  0xE0000A40, 0x40000000, 0x00000000, 0xC0000000,
/* single negate, -f32 + 0 , 2 = -2 */
			  0xE0000A80, 0x40000000, 0x40400000, 0x40A00000,
/* single add, f32 + f32, 2 + 3 = 5 */
			  0x0, 0x0, 0x0, 0x0   /* end of table */
};

/* immed23 test */

#define         base   FPA_BASE

struct cmd_ptr_path {
    u_long          addr;
    u_long          data_start;
    u_long          data_incr;
    u_long          num_data;
    u_long          data_shift;
    u_long          data_mask;
    u_long          mux_sel;
    u_long          cntxt_mask;
    u_long          dorin;
};


struct cmd_ptr_path cmdrimm[] = {
/*
 * addr         dstart       dincr    numd   dshft  dmsk  muxsel  cntxtmsk
 * dorin 
 */
     {base + 0x800, 0x00000000, 0x00000040, 512, 06, 0x01f, 6, 0x1f, 0x000},	/* imm2 - reg */
     {base + 0x800, 0x00008000, 0x00000040, 512, 06, 0x1ff, 6, 0x00, 0x400},	/* imm2 - constant */
     {base + 0x800, 0x00000000, 0x00010000, 512, 16, 0x01f, 7, 0x1f, 0x000},	/* imm3 - reg */
     {base + 0x800, 0x02000000, 0x00010000, 512, 16, 0x1ff, 7, 0x00, 0x400},	/* imm3 - constant */
		  {0x000, 0x00000000, 0x00000000, 00, 00, 0x000, 0, 0x00, 0}	/* end   */
};

struct cmd_ptr_addr {

    u_long          addr_start;
    u_long          addr_incr;
    u_long          numa;
    u_long          ashift;
    u_long          amask;
    u_long          muxsel;
};

struct cmd_ptr_addr spaddrimm[] = {

				   {base + 0x0000, 4, 32, 3, 0x0f, 6},	/* sp, dp short */
				   {base + 0x0c00, 4, 64, 3, 0x1f, 6},	/* same */
				   {base + 0x0e00, 4, 16, 3, 0x07, 6},
				   {base + 0x1000, 8, 16, 3, 0x0f, 6},	/* extended - reg 2 */
				   {000, 0, 0, 0, 0, 0}	/* end of table */
};

struct cmd_ptr_addr dpaddrimm[] = {

				   {base + 0x1800, 4, 32, 3, 0x0f, 6},	/* ext immed2 */
				   {base + 0x1800, 0x80, 16, 7, 0x0f, 7},
				   {000, 0, 0, 0, 0, 0}	/* end of table */
};


/* simpleins test */

struct test_instructions {

    int             address;
    int             status;
};

struct test_instructions instr1[] = {
/* address    status   */

				     {0xe0000000, 0x01},	/* SP NOP */
				     {0xe0000004, 0x1d},	/* DP NOP */
				     {0xe0001000, 0x01},	/* DP NOP */
				     {0xe000095c, 0x13},	/* UNIMPL */
				     {0x00000, 0x00}
};

/* micro store register test */

u_long          wstatus_res[] = {

				 0xa004,
				 0xa119,
				 0xa200,
				 0xa300,
				 0x2400,
				 0x2500,
				 0x2600,
				 0x2700,
				 0x2800,
				 0x2900,
				 0x2a00,
				 0x2b00,
				 0x2c00,
				 0x2d00,
				 0x2e00,
				 0x2f02
};

u_long          wstatus_res1[] = {

				  0xa004,
				  0xa119,
				  0xa200,
				  0x2300,
				  0x2400,
				  0x2500,
				  0x2600,
				  0x2700,
				  0x2800,
				  0x2900,
				  0x2a00,
				  0x2b00,
				  0x2c00,
				  0x2d00,
				  0x2e00,
				  0x2f02
};

/* Weitek status test */

#define    zero_sp    0x00000000
#define    zero_msw   0x00000000
#define    zero_lsw   0x00000000

#define    half_sp    0x3f000000
#define    half_msw   0x3fe00000
#define    half_lsw   0x00000000

#define    one_sp    0x3f800000
#define    one_msw    0x3ff00000
#define    one_lsw    0x00000000

#define    two_sp    0x40000000
#define    two_msw   0x40000000
#define    two_lsw   0x00000000

#define    pi_sp    0x40490fdb
#define    pi_msw   0x400921fb
#define    pi_lsw   0x54442d18

#define    pi_4_sp  0x3f490fdb
#define    pi_4_msw 0x3fe921fb
#define    pi_4_lsw 0x54442d18

#define    inf_sp   0x7f800000
#define    inf_msw  0x7ff00000
#define    inf_lsw  0x00000000

#define    nan_sp   0x7fbfffff
#define    nan_msw  0x7ff7ffff
#define    nan_lsw  0xffffffff

#define    denorm_sp    0x00000001
#define    denorm_msw   0x00000000
#define    denorm_lsw   0x00000001

#define    maxn_sp   0x7f7fffff
#define    maxn_msw  0x7fefffff
#define    maxn_lsw  0xffffffff

#define    minn_sp   0x00800000
#define    minn_msw  0x00100000
#define    minn_lsw  0x00000000

#define    min1_sp   0x00800001
#define    min1_msw  0x00100001
#define    min1_lsw  0x00010001

#define    maxd_sp   0x007fffff
#define    maxd_msw  0x000fffff
#define    maxd_lsw  0xffffffff

#define    nocare  0

#define    add_sp   (base + 0xa80)
#define    add_dp   (base + 0xa84)
#define    div_sp   (base + 0xa30)
#define    div_dp   (base + 0xa34)
#define    mult_sp  (base + 0xa08)
#define    mult_dp  (base + 0xa0c)


struct testws {

    u_long          a_msw;
    u_long          a_lsw;
    u_long          b_msw;
    u_long          b_lsw;
    u_long          instr;
    u_long          status;
};

struct testws   test_ws[] = {
/* a_msw       a_lsw       b_msw       b_lsw      instr     status */
/* ALU */
			     {zero_sp, nocare, zero_sp, nocare, add_sp, 0x0},	/* zero,ex */
		      {zero_msw, zero_lsw, zero_msw, zero_lsw, add_dp, 0x0},
			     {inf_sp, nocare, inf_sp, nocare, add_sp, 0x1},	/* inf,ex */
			  {inf_msw, inf_lsw, inf_msw, inf_lsw, add_dp, 0x1},
			     {one_sp, nocare, one_sp, nocare, add_sp, 0x2},	/* fin,ex */
			  {one_msw, one_lsw, one_msw, one_lsw, add_dp, 0x2},
			     {one_sp, nocare, pi_sp, nocare, div_sp, 0x3},	/* fin,inex */
			     {one_msw, one_lsw, pi_msw, pi_lsw, div_dp, 0x3},
/* status 0x4 is unused on both chips *//* unused */
			     {maxn_sp, nocare, maxn_sp, nocare, add_sp, 0x5},	/* ovfl,inex */
		      {maxn_msw, maxn_lsw, maxn_msw, maxn_lsw, add_dp, 0x5},
			     {minn_sp, nocare, two_sp, nocare, div_sp, 0x6},	/* unfl */
			{minn_msw, minn_lsw, two_msw, two_lsw, div_dp, 0x6},
			     {minn_sp, nocare, pi_sp, nocare, div_sp, 0x7},	/* unfl,inex */
			  {minn_msw, minn_lsw, pi_msw, pi_lsw, div_dp, 0x7},
			     {maxd_sp, nocare, two_sp, nocare, div_sp, 0x8},	/* a-denorm */
			{maxd_msw, maxd_lsw, two_msw, two_lsw, div_dp, 0x8},
			     {two_sp, nocare, maxd_sp, nocare, div_sp, 0x9},	/* b-denorm */
			{one_msw, one_lsw, maxd_msw, maxd_lsw, div_dp, 0x9},
			     {maxd_sp, nocare, maxd_sp, nocare, div_sp, 0xa},	/* ab-denorm */
		      {maxd_msw, maxd_lsw, maxd_msw, maxd_lsw, div_dp, 0xa},
			     {one_sp, nocare, zero_sp, nocare, div_sp, 0xb},	/* div 0 */
			{one_msw, one_lsw, zero_msw, zero_lsw, div_dp, 0xb},
			     {nan_sp, nocare, zero_sp, nocare, add_sp, 0xc},	/* a-nan */
			{nan_msw, nan_lsw, zero_msw, zero_lsw, add_dp, 0xc},
			     {zero_sp, nocare, nan_sp, nocare, add_sp, 0xd},	/* b-nan */
			   {zero_sp, nocare, nan_msw, nan_lsw, add_dp, 0xd},
			     {nan_sp, nocare, nan_sp, nocare, add_sp, 0xe},	/* ab-nan */
			  {nan_msw, nan_lsw, nan_msw, nan_lsw, add_dp, 0xe},
			     {inf_sp, nocare, inf_sp, nocare, div_sp, 0xf},	/* invalid */
			  {inf_msw, inf_lsw, inf_msw, inf_lsw, div_dp, 0xf},
/* MULT */
			   {zero_sp, nocare, zero_sp, nocare, mult_sp, 0x0},	/* zero,ex */
		     {zero_msw, zero_lsw, zero_msw, zero_lsw, mult_dp, 0x0},
			     {inf_sp, nocare, inf_sp, nocare, mult_sp, 0x1},	/* inf,ex */
			 {inf_msw, inf_lsw, inf_msw, inf_lsw, mult_dp, 0x1},
			     {one_sp, nocare, one_sp, nocare, mult_sp, 0x2},	/* fin,ex */
			 {one_msw, one_lsw, one_msw, one_lsw, mult_dp, 0x2},
			     {pi_sp, nocare, pi_sp, nocare, mult_sp, 0x3},	/* fin,inex */
			     {pi_msw, pi_lsw, pi_msw, pi_lsw, mult_dp, 0x3},
/* status 0x4 is unused on both chips */
			   {maxn_sp, nocare, maxn_sp, nocare, mult_sp, 0x5},	/* ovfl,inex */
		     {maxn_msw, maxn_lsw, maxn_msw, maxn_lsw, mult_dp, 0x5},
			   {minn_sp, nocare, half_sp, nocare, mult_sp, 0x6},	/* unfl */
		     {minn_msw, minn_lsw, half_msw, half_lsw, mult_dp, 0x6},
			   {min1_sp, nocare, pi_4_sp, nocare, mult_sp, 0x7},	/* unfl,inex */
		     {min1_msw, min1_lsw, pi_4_msw, pi_4_lsw, mult_dp, 0x7},
			   {maxd_sp, nocare, half_sp, nocare, mult_sp, 0x8},	/* a-denorm */
		     {maxd_msw, maxd_lsw, half_msw, half_lsw, mult_dp, 0x8},
			   {half_sp, nocare, maxd_sp, nocare, mult_sp, 0x9},	/* b-denorm */
		     {half_msw, half_lsw, maxd_msw, maxd_lsw, mult_dp, 0x9},
		       {denorm_sp, nocare, denorm_sp, nocare, mult_sp, 0xa},	/* ab-denorm */
	     {denorm_msw, denorm_lsw, denorm_msw, denorm_lsw, mult_dp, 0xa},
/* status 0xb is  divide by zero and is unused by the MULT */
			     {nan_sp, nocare, zero_sp, nocare, mult_sp, 0xc},	/* a-nan */
		       {nan_msw, nan_lsw, zero_msw, zero_lsw, mult_dp, 0xc},
			     {zero_sp, nocare, nan_sp, nocare, mult_sp, 0xd},	/* b-nan */
			  {zero_sp, nocare, nan_msw, nan_lsw, mult_dp, 0xd},
			     {nan_sp, nocare, nan_sp, nocare, mult_sp, 0xe},	/* ab-nan */
			 {nan_msw, nan_lsw, nan_msw, nan_lsw, mult_dp, 0xe},
			     {inf_sp, nocare, zero_sp, nocare, mult_sp, 0xf},	/* invalid */
		       {inf_msw, inf_lsw, zero_msw, zero_lsw, mult_dp, 0xf},
			     {00, 00, 000, 000, 0000, 0x0}
};


extern int      verbose, debug;
int             errno;
u_long          contexts;
int             dev_no;
u_long         *ram_ptr, *rw_ram_ptr, *st_reg_ptr;
int             open_fpa;
extern int      lin_pack_test();
int             sig_err_flag = 0x0;
int             seg_sig_flag = 0x0;
char            null_str[2];
int             res_seg_svmask;
int             res_seg_svonstack;
void            (*res_seg_svhandler) ();

int             restore_svmask;
int             restore_svonstack;
void            (*restore_svhandler) ();
int             temp_test_number;

struct sigvec   newfpe, oldfpe, oldseg, newseg;
void            sigsegv_handler(), sigfpe_handler();

fpa_systest()
{
    int             value, i, val_rotate;
    int             val, io_value;

    signal(SIGSEGV, sigsegv_handler);
    signal(SIGFPE, sigfpe_handler);

    if (verbose)
	printf("FPA: FPA System(Reliability) Test.\n");
    st_reg_ptr = (u_long *) FPA_STATE_PTR;
    close(open_fpa);
    if (!(winitfp_())) {
	if (verbose)
	    printf("FPA: Could not open FPA.\n");
	exit(-1);
    }
	contexts = 0x0;			       /* initialize the context
					        * number */

	value = open_new_context();
	if (value == 0) {
	    val_rotate = *st_reg_ptr & 0x1F;
	    contexts = (1 << val_rotate);
	    if (verbose)
		printf("FPA: All Tests - Context Number = %d\n", val_rotate);
	    temp_test_number = 0;
	    if (debug || verbose)
		printf("FPA: 	Ierr Test.\n");
	    if (ierr_test()) {
		printf("FAILED Ierr Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 1;
	    if (verbose || debug)
		printf("FPA: 	Imask Test.\n");
	    if (imask_test()) {
		printf("FAILED Imask Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 2;
	    if (verbose || debug)
		printf("FPA: 	Load Pointer Test.\n");
	    if (ldptr_test()) {
		printf("FAILED Load Pointer Test\n");
		fail_close();
		return (-1);
	    }
	    /* turn the load enable bit to check the rams */

	    if ((io_value = ioctl(dev_no, FPA_LOAD_ON, (char *) null_str)) >= 0) {	/* do the ram tests only
											 * if the user is super
											 * user */
		temp_test_number = 3;
		if (verbose || debug)
		    printf("FPA: 	Mapping Ram Test.\n");
		if (map_ram()) {
		    printf("FAILED Mapping Ram Test\n");
		    ioctl(dev_no, FPA_LOAD_OFF, (char *) null_str);
		    fail_close();
		    return (-1);
		}
		temp_test_number = 4;
		if (verbose || debug)
		    printf("FPA:    Micro Store Ram Test.\n");
		if (ustore_ram()) {
		    printf("FAILED Micro Store Ram Test\n");
		    ioctl(dev_no, FPA_LOAD_OFF, (char *) null_str);
		    fail_close();
		    return (-1);
		}
		ioctl(dev_no, FPA_LOAD_OFF, (char *) null_str);
	    }
	    temp_test_number = 5;
	    if (verbose || debug)
		printf("FPA:    Register Ram Upper Half Test.\n");
	    if (reg_uh_ram()) {
		printf("FAILED Register Ram Upper Half Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 6;
	    if (verbose || debug)
		printf("FPA:    Register Ram Lower Half Test.\n");
	    if (reg_ram()) {
		printf("FAILED Register Ram Lower Half Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 7;
	    if (verbose || debug)
		printf("FPA:    Simple Instruction Test.\n");
	    if (sim_ins_test()) {
		printf("FAILED Simple Instruction Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 8;
	    if (verbose || debug)
		printf("FPA:    Shadow Ram Test.\n");
	    if (shadow_ram()) {
		printf("FAILED Shadow Ram Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 9;
	    if (verbose || debug)
		printf("FPA:    Pointer Test.\n");
	    if (pointer_test()) {
		printf("FAILED Pointer Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 10;
	    if (verbose || debug)
		printf("FPA:    Pointer Increment and Decrement Test.\n");
	    if (ptr_incdec_test()) {
		printf("FAILED Pointer Increment and Decrement Test\n");
		fail_close();
		return (-1);
	    }
	    sig_err_flag = 0xff;	       /* set the flag for error
					        * handler */
	    temp_test_number = 11;
	    if (verbose || debug)
		printf("FPA:    Lock Test.\n");
	    if (lock_test()) {
		printf("FAILED Lock Test\n");
		fail_close();
		return (-1);
	    }
	    sig_err_flag = 0;		       /* reset the flag for error
					        * handler */

	    seg_sig_flag = 0xff;	       /* set the flag for seg error
					        * handler */
	    temp_test_number = 12;
	    if (verbose || debug)
		printf("FPA:	Nack Test.\n");
	    nack2_test();		       /* if anything goes wrong the
					        * program will not come back */
	    seg_sig_flag = 0x0;		       /* reset the flag for seg
					        * error handler */
	    temp_test_number = 13;
	    if (verbose || debug)
		printf("FPA     F+ Test.\n");
	    if (wlwf_test()) {
		printf("FAILED F+ Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 14;
	    if (verbose || debug)
		printf("FPA:    Mode Register Test.\n");
	    if (test_mode_reg()) {
		printf("FAILED Mode Register Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 15;
	    if (verbose || debug)
		printf("FPA:    Wstatus Register Test.\n");
	    if (test_wstatus_reg()) {
		printf("FAILED Wstatus Register Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 16;
	    if (verbose || debug)
		printf("FPA:    Weitek Data Path Test.\n");
	    if (fpa_wd()) {
		printf("FAILED Weitek Data Path Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 17;
	    if (verbose || debug)
		printf("FPA:    Weitek Operation Test.\n");
	    if (w_op_test()) {
		printf("FAILED Weitek Operation Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 18;
	    if (verbose || debug)
		printf("FPA:    Weitek Status Test.\n");
	    if (fpa_ws()) {
		printf("FAILED Weitek Status Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 19;
	    if (verbose || debug)
		printf("FPA:    Jump Conditions Test.\n");
	    if (w_jump_cond_test()) {
		printf("FAILED Jump Condition Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 20;
	    if (verbose || debug)
		printf("FPA:    Timing Test.\n");
	    if (timing()) {
		printf("FAILED Timing Test\n");
		fail_close();
		return (-1);
	    }
	    temp_test_number = 21;
	    if (verbose || debug)
		printf("FPA:    Linpack Test.\n");
	    if (val = lin_pack_test()) {
		if (val == 1) {
		    fail_close();
		} else {
		    fail_close();
		}
		return (-1);
	    }
	    close_new_context();
	} else {
	    restore_signals();
	    return (-1);
	}
	temp_test_number = 22;
	if (other_contexts()) {
	    printf("FAILED other contexts\n");
	    fail_close();
	    restore_signals();
	    return (-1);
	}

    open_new_context();
    close_new_context();
}

fail_close()
{
    printf("FPA:                         FAILED!\n");
    close_new_context();		       /* close the context first */
}


/*
 * This routine opens the FPA context, and if it could not checks the error
 * no and prints the error 
 */

open_new_context()
{
    u_long          value;		       /* *st_reg_ptr; */

    st_reg_ptr = (u_long *) FPA_STATE_PTR;
    dev_no = open("/dev/fpa", O_RDWR);

    if (dev_no < 0) {			       /* could not open */

	switch (errno) {

	case ENXIO:
	    printf("FPA: Cannot find FPA.\n");
	    break;
	case ENOENT:
	    printf("FPA: Cannot find 68881.\n");
	    break;
	case EBUSY:
	    printf("FPA: No FPA context Available.\n");
	    break;
	case ENETDOWN:
	    printf("FPA: Disabled FPA, could not access.\n");
	    break;
	case EEXIST:
	    printf("FPA: Duplicate Open on FPA.\n");
	    break;
	}
	return (-1);
    } else
	return (0);
}

close_new_context()
{

    close(dev_no);
}

other_contexts()
{
    int             i, j, value;
    u_long          val, val1;
    /* u_long	*st_reg_ptr; */

    st_reg_ptr = (u_long *) FPA_STATE_PTR;
    for (i = 1; i < 32; i++) {

	if (open_new_context())
	    return (0);
	val = *st_reg_ptr & 0x1F;	       /* get the context number */
	val1 = (1 << val);
	if (verbose  || debug)
	    printf("FPA:    Register Ram Upper Half Test, Context Number = %d\n", val);

	contexts = (contexts | val1);

	if (reg_ram()) {
	    close_new_context();
	    return (-1);
	}
	close_new_context();
    }
    return (0);
}



temp_print()
{
    u_long         *ptr, i;
    u_long         *ptr1;

    ptr1 = (u_long *) (FPA_BASE + FPA_STABLE_PIPE_STATUS);
    ptr = (u_long *) (FPA_BASE + FPA_PIPE_ACT_INS);

    printf("press any key to continue:\n");
    i = getchar();
    printf("        pipe addr = %x,pipe stable3 = %x\n", ptr1, *ptr1);

    for (i = 0; i < 6; i++) {

	printf(" 	addr = %x, value = %x\n", ptr, *ptr);
	ptr++;
    }
}

lock_test()
{

    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    *(u_long *) FPA_IMASK_PTR = 0x1;

    if (dp_short_test())
	return (-1);
    if (dp_ext_test())
	return (-1);
    if (dp_cmd_test())
	return (-1);
    if (next_dp_short_test())
	return (-1);
    if (next_dp_ext_test())
	return (-1);
    if (next_dp_cmd_test())
	return (-1);

    return (0);
}

next_dp_short_test()
{
    u_long          res_a_msw, res_a_lsw, res_n_msw, res_n_lsw, res_i_msw, res_i_lsw;
    int             i, j, k;
    u_long          res0_msw, res0_lsw, shad_res_msw, shad_res_lsw, value_i, value_0;
    u_long         *soft_clear, *reg0, *reg0_lsw, *reg0_addr, *reg0_addr_lsw;
    u_long         *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw, *shadow_j, *shadow_j_lsw;
    u_long         *ptr4_lsw, *ptr4, *ptr5_lsw, *ptr5;
    u_long          res4_msw, res5_msw, res4_lsw, res5_lsw;
    u_long         *ptr2, *ptr2_lsw;

    soft_clear = (u_long *) FPA_CLEAR_PIPE_PTR;
    for (i = 0; i < 16; i++) {
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;

	/* initialize */
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    res_a_msw = 0x3FD55555;		       /* the result is always active
					        * - dp value 0.33333333333 */
    res_a_lsw = 0x55555555;
    res_n_msw = 0x3FE55555;		       /* the result of next should
					        * be always be 0.6666666666 */
    res_n_lsw = 0x55555555;
    value_0 = 0x3FF00000;
    value_i = 0x40000000;
    reg0 = (u_long *) REGISTER_ZERO_MSW;       /* always this register will
					        * be active */
    reg0_lsw = (u_long *) REGISTER_ZERO_LSW;

    reg0_addr = (u_long *) dps[0].addr;	       /* for higher significant
					        * value */
    reg0_addr_lsw = (u_long *) 0xE0001000;     /* for least significant value */

    for (i = 0; i < 16; i++) {

	reg_i_addr = (u_long *) dps[i].addr;   /* for higher significant
					        * value */
	reg_i_addr_lsw = (u_long *) 0xE0001000;/* for least significant value */
	reg_i = (u_long *) user[i].reg_msw;
	reg_i_lsw = (u_long *) user[i].reg_lsw;
	for (j = 0; j < 8; j++) {

	    shadow_j = (u_long *) shadow[j].shreg_msw;
	    shadow_j_lsw = (u_long *) shadow[j].shreg_lsw;
	    *reg0 = 0x3FF00000;		       /* register 0 has dp value of
					        * 1 - active stage */
	    *reg0_lsw = 0x0;
	    *reg_i = 0x40000000;	       /* register has dp value of 2
					        * for next stage2 for next
					        * stage */
	    *reg_i_lsw = 0x0;

	    *(u_long *) FPA_IMASK_PTR = 0x1;

	    *reg0_addr = 0x40080000;	       /* operand has dp value of 3
					        * active stage */
	    *reg0_addr_lsw = 0x0;
	    *reg_i_addr = 0x40080000;	       /* operand  has dp value 3
					        * next stage */
	    *reg_i_addr_lsw = 0x0;

	    /* may be soft clear */
	    shad_res_msw = *shadow_j;	       /* read the shadow register */
	    shad_res_lsw = *shadow_j_lsw;
	    *soft_clear = 0x0;

	    res_i_msw = *reg_i;		       /* read the result from the
					        * reg  i */
	    res_i_lsw = *reg_i_lsw;
	    res0_msw = *reg0;
	    res0_lsw = *reg0_lsw;

	    *(u_long *) FPA_IMASK_PTR = 0x0;

	    if ((j == 0) && (j == i)) {
		if ((shad_res_msw != 0x3FCC71C7) || (res_i_msw != 0x3FCC71C7) || (res0_msw != 0x3FCC71C7))
		    /*
		     * if ((shad_res_msw != res_n_msw) || (res_i_msw !=
		     * res_n_msw) || (res0_msw != res_n_msw)) 
		     */
		{

		    printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res0_lsw = %x, shad_lsw = %x, resi_msw = %x, resi_lsw = %x\n",
			   res0_lsw, shad_res_lsw, res_i_msw, res_i_lsw);
		    return (-1);
		}
	    } else if ((j == 0) && (j != i)) {
		if ((shad_res_msw != res_a_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw)) {

		    printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    } else if ((j != 0) && (j == i)) {

		if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw)) {
		    printf("Err3:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    } else if ((j != 0) && (j != i)) {
		if (i == 0) {
		    if ((shad_res_msw != 0x0) || (res_i_msw != value_i) || (res0_msw != value_i)) {
			printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			       i, j, res0_msw, shad_res_msw);
			printf("res_i_msw = %x\n", res_i_msw);
			return (-1);
		    }
		} else if ((shad_res_msw != 0x0) || (res_i_msw != value_i) || (res0_msw != value_0)) {
		    printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    }
	    *reg0 = 0x0;
	    *reg0_lsw = 0x0;
	    *reg_i = 0x0;
	    *reg_i_lsw = 0x0;
	}
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    return (0);

}
next_dp_ext_test()
{
    u_long          res_a_msw, res_a_lsw, res_n_msw, res_n_lsw, res_i_msw, res_i_lsw;
    int             i, j, k;
    u_long          res0_msw, res0_lsw, shad_res_msw, shad_res_lsw, value_i, value_0;
    u_long         *soft_clear, *reg0, *reg0_lsw, *reg0_addr, *reg0_addr_lsw;
    u_long         *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw, *shadow_j, *shadow_j_lsw;
    u_long         *ptr4_lsw, *ptr4, *ptr5_lsw, *ptr5;
    u_long          res4_msw, res5_msw, res4_lsw, res5_lsw;
    u_long         *ptr2, *ptr2_lsw;

    soft_clear = (u_long *) FPA_CLEAR_PIPE_PTR;
    for (i = 0; i < 16; i++) {
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;

	/* initialize */
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    res_a_msw = 0x3FD55555;		       /* the result is always active
					        * - dp value 0.33333333333 */
    res_a_lsw = 0x55555555;
    res_n_msw = 0x3FE55555;		       /* the result of next should
					        * be always be 0.6666666666 */
    res_n_lsw = 0x55555555;
    value_0 = 0x3FF00000;
    value_i = 0x40000000;
    reg0 = (u_long *) REGISTER_ZERO_MSW;       /* always this register will
					        * be active */
    reg0_lsw = (u_long *) REGISTER_ZERO_LSW;

    reg0_addr = (u_long *) ext[0].addr;	       /* for higher significant
					        * value */
    reg0_addr_lsw = (u_long *) ext[0].addr_lsw;/* for least significant value */
    for (i = 0; i < 16; i++) {

	reg_i_addr = (u_long *) ext[i].addr;   /* for higher significant
					        * value */
	reg_i_addr_lsw = (u_long *) ext[i].addr_lsw;	/* for least significant
							 * value */
	reg_i = (u_long *) user[i].reg_msw;
	reg_i_lsw = (u_long *) user[i].reg_lsw;
	for (j = 0; j < 8; j++) {

	    shadow_j = (u_long *) shadow[j].shreg_msw;
	    shadow_j_lsw = (u_long *) shadow[j].shreg_lsw;
	    *reg0 = 0x3FF00000;		       /* register 0 has dp value of
					        * 1 - active stage */
	    *reg0_lsw = 0x0;
	    *reg_i = 0x40000000;	       /* register has dp value of 2
					        * for next stage2 for next
					        * stage */
	    *reg_i_lsw = 0x0;
	    *(u_long *) FPA_IMASK_PTR = 0x1;

	    *reg0_addr = 0x40080000;	       /* operand has dp value of 3
					        * active stage */
	    *reg0_addr_lsw = 0x0;
	    *reg_i_addr = 0x40080000;	       /* operand  has dp value 3
					        * next stage */
	    *reg_i_addr_lsw = 0x0;
	    /* may be soft clear */
	    shad_res_msw = *shadow_j;	       /* read the shadow register */
	    shad_res_lsw = *shadow_j_lsw;
	    *soft_clear = 0x0;
	    res_i_msw = *reg_i;		       /* read the result from the
					        * reg  i */
	    res_i_lsw = *reg_i_lsw;
	    res0_msw = *reg0;
	    res0_lsw = *reg0_lsw;
	    *(u_long *) FPA_IMASK_PTR = 0x0;

	    if ((j == 0) && (j == i)) {
		if ((shad_res_msw != 0x3FCC71C7) || (res_i_msw != 0x3FCC71C7) || (res0_msw != 0x3FCC71C7))
		    /*
		     * if ((shad_res_msw != res_n_msw) || (res_i_msw !=
		     * res_n_msw) || (res0_msw != res_n_msw)) 
		     */
		{

		    printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res0_lsw = %x, shad_lsw = %x, resi_msw = %x, resi_lsw = %x\n",
			   res0_lsw, shad_res_lsw, res_i_msw, res_i_lsw);
		    return (-1);
		}
	    } else if ((j == 0) && (j != i)) {
		if ((shad_res_msw != res_a_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw)) {

		    printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    } else if ((j != 0) && (j == i)) {

		if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw)) {
		    printf("Err3:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    } else if ((j != 0) && (j != i)) {
		if (i == 0) {
		    if ((shad_res_msw != 0x0) || (res_i_msw != value_i) || (res0_msw != value_i)) {
			printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			       i, j, res0_msw, shad_res_msw);
			printf("res_i_msw = %x\n", res_i_msw);
			return (-1);
		    }
		} else if ((shad_res_msw != 0x0) || (res_i_msw != value_i) || (res0_msw != value_0)) {
		    printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    }
	    *reg0 = 0x0;
	    *reg0_lsw = 0x0;
	    *reg_i = 0x0;
	    *reg_i_lsw = 0x0;
	}
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    return (0);

}
next_dp_cmd_test()
{
    u_long          res_a_msw, res_a_lsw, res_n_msw, res_n_lsw, res_i_msw, res_i_lsw;
    int             i, j, k;
    u_long          res0_msw, res0_lsw, shad_res_msw, shad_res_lsw, value_i, value_0;
    u_long         *soft_clear, *reg0, *reg0_lsw, *reg0_addr, *reg0_addr_lsw;
    u_long         *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw, *shadow_j, *shadow_j_lsw;
    u_long         *ptr4_lsw, *ptr4, *ptr5_lsw, *ptr5;
    u_long          res4_msw, res5_msw, res4_lsw, res5_lsw;
    u_long         *ptr2, *ptr2_lsw;

    soft_clear = (u_long *) FPA_CLEAR_PIPE_PTR;
    for (i = 0; i < 32; i++) {
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;

	/* initialize */
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    res_a_msw = 0x1;			       /* the result is always active
					        * - 0.333333333333 */
    res_a_lsw = 0x0;
    res_n_msw = 0x2;			       /* the result of next should
					        * be always be 0.6666666666 */
    res_n_lsw = 0x0;
    value_0 = 0x1;
    value_i = 0x2;
    reg0 = (u_long *) REGISTER_ZERO_MSW;       /* always this register will
					        * be active */
    reg0_lsw = (u_long *) REGISTER_ZERO_LSW;

    reg0_addr = (u_long *) 0xE0000AC4;
    reg_i_addr = (u_long *) 0xE0000AC4;
    for (i = 0; i < 31; i++) {

	reg_i = (u_long *) user[i].reg_msw;
	reg_i_lsw = (u_long *) user[i].reg_lsw;
	for (j = 0; j < 8; j++) {

	    shadow_j = (u_long *) shadow[j].shreg_msw;
	    shadow_j_lsw = (u_long *) shadow[j].shreg_lsw;
	    *reg0 = value_0;		       /* register 0 has dp value of
					        * 0.333 - active stage */
	    *reg0_lsw = 0x0;
	    *reg_i = value_i;		       /* register has dp value of
					        * 0.666for next stage2 for
					        * next stage */
	    *reg_i_lsw = 0x0;
	    *(u_long *) FPA_IMASK_PTR = 0x1;

	    *reg0_addr = nxt_cmd[0].data;      /* nota number + 0 weitek op */

	    *reg_i_addr = nxt_cmd[i].data;     /* nota number + 0 weitek op */

	    /* may be soft clear */
	    shad_res_msw = *shadow_j;	       /* read the shadow register */
	    shad_res_lsw = *shadow_j_lsw;

	    *soft_clear = 0x0;

	    res_i_msw = *reg_i;		       /* read the result from the
					        * reg  i */
	    res_i_lsw = *reg_i_lsw;

	    res0_msw = *reg0;
	    res0_lsw = *reg0_lsw;
	    *(u_long *) FPA_IMASK_PTR = 0x0;

	    if ((j == 0) && (j == i)) {
		if ((shad_res_msw != value_i) || (res_i_msw != value_i) || (res0_msw != value_i))
		    /*
		     * if ((shad_res_msw != res_n_msw) || (res_i_msw !=
		     * res_n_msw) || (res0_msw != res_n_msw)) 
		     */
		{

		    printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res0_lsw = %x, shad_lsw = %x, resi_msw = %x, resi_lsw = %x\n",
			   res0_lsw, shad_res_lsw, res_i_msw, res_i_lsw);
		    return (-1);
		}
	    } else if ((j == 0) && (j != i)) {
		if ((shad_res_msw != res_a_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw)) {

		    printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    } else if ((j != 0) && (j == i)) {

		if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw)) {
		    printf("Err3:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    } else if ((j != 0) && (j != i)) {
		if (i == 0) {
		    if ((shad_res_msw != 0x0) || (res_i_msw != value_i) || (res0_msw != value_i)) {
			printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			       i, j, res0_msw, shad_res_msw);
			printf("res_i_msw = %x\n", res_i_msw);
			return (-1);
		    }
		} else if ((shad_res_msw != 0x0) || (res_i_msw != value_i) || (res0_msw != value_0)) {
		    printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res0_msw, shad_res_msw);
		    printf("res_i_msw = %x\n", res_i_msw);
		    return (-1);
		}
	    }
	    *reg0 = 0x0;
	    *reg0_lsw = 0x0;
	    *reg_i = 0x0;
	    *reg_i_lsw = 0x0;
	}
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    return (0);

}

dp_short_test()
{
    u_long          res, res1_lsw, res1_msw, res2_lsw, res2_msw, res3_lsw, res3_msw;
    u_long          i, j;
    u_long         *soft_clear, *ptr1, *ptr2, *ptr1_lsw, *ptr2_lsw, *ptr3, *ptr3_lsw;
    u_long         *pipe;

    pipe = (u_long *) (FPA_BASE + FPA_STABLE_PIPE_STATUS);
    soft_clear = (u_long *) FPA_CLEAR_PIPE_PTR;
    for (i = 0; i < 16; i++) {
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;

	/* initialize */
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    res3_msw = 0x3FD55555;		       /* the result is always dp
					        * value 0.3333333333333333 */
    res3_lsw = 0x55555555;

    for (i = 0; i < 8; i++) {
	ptr1 = (u_long *) dps[i].addr;	       /* for higher significant
					        * value */
	ptr1_lsw = (u_long *) 0xE0001000;      /* for least significant value */
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;

	for (j = 0; j < 8; j++) {

	    ptr3 = (u_long *) shadow[j].shreg_msw;
	    ptr3_lsw = (u_long *) shadow[j].shreg_lsw;
	    *ptr2 = 0x3FF00000;		       /* register has dp value 1 */
	    *ptr2_lsw = 0x0;
	    *(u_long *) FPA_IMASK_PTR = 0x1;


	    *ptr1 = 0x40080000;		       /* operand is a dp value  3 */

	    *ptr1_lsw = 0x0;

	    res1_msw = *ptr3;		       /* read the shadow register */
	    res1_lsw = *ptr3_lsw;

	    *soft_clear = 0x0;

	    res2_msw = *ptr2;		       /* read the result from the
					        * reg */
	    res2_lsw = *ptr2_lsw;
	    *(u_long *) FPA_IMASK_PTR = 0x0;

	    if (i == j) {

		if ((res3_msw != res2_msw) || (res3_msw != res1_msw)) {
		    printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res2_msw, res1_msw);

		    return (-1);
		}
	    }
	    if (i != j) {
		if ((res1_msw != 0x0) || (res2_msw != 0x3FF00000)) {
		    printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n", i, j, res2_msw, res1_msw);

		    return (-1);
		}
	    }
	}
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    return (0);
}

dp_ext_test()
{
    u_long          res, res1_lsw, res1_msw, res2_lsw, res2_msw, res3_lsw, res3_msw;
    int             i, j;
    u_long         *soft_clear, *ptr1, *ptr2, *ptr1_lsw, *ptr2_lsw, *ptr3, *ptr3_lsw;

    soft_clear = (u_long *) FPA_CLEAR_PIPE_PTR;
    for (i = 0; i < 16; i++) {
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;

	/* initialize */
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    res3_msw = 0x3FD55555;		       /* the result is always dp
					        * value 0.33333333 */
    res3_lsw = 0x55555555;

    for (i = 0; i < 16; i++) {
	ptr1 = (u_long *) ext[i].addr;
	ptr1_lsw = (u_long *) ext[i].addr_lsw;
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;
	for (j = 0; j < 8; j++) {
	    ptr3 = (u_long *) shadow[j].shreg_msw;
	    ptr3_lsw = (u_long *) shadow[j].shreg_lsw;
	    *ptr2 = 0x3FF00000;		       /* register has dp value 1 */
	    *ptr2_lsw = 0x0;
	    *(u_long *) FPA_IMASK_PTR = 0x1;

	    *ptr1 = 0x40080000;		       /* operand is a dp value 3 */
	    *ptr1_lsw = 0x0;
	    res1_msw = *ptr3;		       /* read the shadow register */
	    res1_lsw = *ptr3_lsw;
	    *soft_clear = 0x0;

	    res2_msw = *ptr2;		       /* read the result from the
					        * reg */
	    res2_lsw = *ptr2_lsw;
	    *(u_long *) FPA_IMASK_PTR = 0x2;

	    if (i == j) {
		if ((res3_msw != res2_msw) && (res3_msw != res1_msw)) {
		    printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res2_msw, res1_msw);
		    return (-1);
		}
	    } else {
		if ((res1_msw != 0x0) || (res2_msw != 0x3FF00000)) {
		    printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n", i, j, res2_msw, res1_msw);
		    return (-1);
		}
	    }

	}
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    return (0);
}

dp_cmd_test()
{
    u_long          res, res1_lsw, res1_msw, res2_lsw, res2_msw, res3_lsw, res3_msw;
    int             i, j, k, l;
    u_long         *soft_clear, *ptr1, *ptr2, *ptr1_lsw, *ptr2_lsw, *ptr3, *ptr3_lsw;
    u_long         *ptr4, *ptr4_lsw, *ptr5, *ptr5_lsw;

    soft_clear = (u_long *) FPA_CLEAR_PIPE_PTR;
    for (i = 0; i < 32; i++) {
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;

	/* initialize */
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
    }
    res3_msw = 0x3FD55555;		       /* the result is always dp
					        * value 0.3333333333 */
    res3_lsw = 0x55555555;

    for (i = 0; i < 30; i++) {
	ptr1 = (u_long *) 0xE0000A34;	       /* for dp divide from weitek
					        * spec */
	ptr2 = (u_long *) user[i].reg_msw;
	ptr2_lsw = (u_long *) user[i].reg_lsw;
	k = i + 1;
	l = i + 2;
	ptr4 = (u_long *) user[k].reg_msw;
	ptr4_lsw = (u_long *) user[k].reg_lsw;
	ptr5 = (u_long *) user[l].reg_msw;
	ptr5_lsw = (u_long *) user[l].reg_lsw;

	for (j = 0; j < 8; j++) {
	    ptr3 = (u_long *) shadow[j].shreg_msw;
	    ptr3_lsw = (u_long *) shadow[j].shreg_lsw;
	    *ptr4 = 0x3FF00000;		       /* register has dp value 1 */
	    *ptr4_lsw = 0x0;
	    *ptr5 = 0x40080000;		       /* register has dp value 3 */
	    *ptr5_lsw = 0x0;

	    *(u_long *) FPA_IMASK_PTR = 0x1;

	    *ptr1 = cmd[i].data;
	    res1_msw = *ptr3;		       /* read the shadow register */
	    res1_lsw = *ptr3_lsw;
	    *soft_clear = 0x0;

	    res2_msw = *ptr2;		       /* read the result from the
					        * reg */
	    res2_lsw = *ptr2_lsw;
	    *(u_long *) FPA_IMASK_PTR = 0x0;

	    if (i == j) {

		if ((res3_msw != res2_msw) || (res3_msw != res1_msw)) {
		    printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res2_msw, res1_msw);
		    return (-1);
		}
	    } else if (j == k) {
		if ((res2_msw != 0x0) || (res1_msw != 0x3FF00000)) {
		    printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res2_msw, res1_msw);
		    return (-1);
		}
	    } else if (j == l) {
		if ((res2_msw != 0x0) || (res1_msw != 0x40080000)) {
		    printf("Err3:reg = %x, shadow = %x, rres= %x, sres = %x\n",
			   i, j, res2_msw, res1_msw);
		    return (-1);
		}
	    }
	    *ptr2 = 0x0;
	    *ptr2_lsw = 0x0;
	}
    }
    return (0);
}



map_ram()
{
    u_long          i, temp_val, tmp_data;
    u_long         *ram_ptr, *rw_ram_ptr;
    u_char          no_data_flag = 0xff;

    ram_ptr = (u_long *) FPA_RAM_ACC;
    if (debug)
	printf("passed ram_ptr = (u_long *)FPA_RAM_ACC\n");
    rw_ram_ptr = (u_long *) FPA_RW_RAM;
    if (debug)
	printf("passed rw_ram_ptr = (u_long *)FPA_RW_RAM\n");
    *ram_ptr = TEST_PTR(0, MR_LB);
    if (debug)
	printf("passed *ram_ptr = TEST_PTR(0, MR_LB)\n");
    temp_val = *rw_ram_ptr;
    if (debug)
	printf("passed temp_val = *rw_ram_ptr\n");
    temp_val = temp_val & MR_DMASK;
    if (debug)
	printf("passed temp_val = temp_val & MR_DMASK\n");
    if (temp_val)
	no_data_flag = 0x0;		       /* data  is non zero */
    if (debug)
	printf("temp_val = %x\n", temp_val);
    for (i = 1; i < MAXSIZE_MAP_RAM; i++) {
	*ram_ptr = TEST_PTR(i, MR_LB);
	if (debug && (i % 1024) == 0)
	    printf("passed *ram_ptr = TEST_PTR(i, MR_LB)\n");
	tmp_data = *rw_ram_ptr & MR_DMASK;
	if (debug && (i % 1024) == 0)
	    printf("passed tmp_data = *rw_ram_ptr & MR_DMASK\n");
	if (tmp_data)
	    no_data_flag = 0x0;		       /* data is non zero */
	if (debug && (i % 512) == 0)
	    printf("tmp_data = %d\n", tmp_data);
	temp_val = (temp_val ^ tmp_data) & MR_DMASK;
	if (debug && (i % 1024) == 0)
	    printf("passed temp_val = (temp_val ^ tmp_data) & MR_DMASK\n");
	if (debug && (i % 512) == 0)
	    printf("temp_val = %x\n", temp_val);
    }
    if (debug)
	printf("no_data_flag = %d\n", no_data_flag);
    if (no_data_flag)
	return (-1);			       /* data is zero , error */
    if (debug)
	printf("temp_val = %x\n", temp_val);
    if (temp_val == 0xadface)
	return (0);

    else
	return (-1);
}

ustore_ram()
{
    u_long          i, tmp_val, tmp_data;
    u_long          tmp_val1, tmp_data1;
    u_long          tmp_val2, tmp_data2;
    u_char          no_data_flag = 0xff;
    u_long         *ram_ptr, *rw_ram_ptr;

    ram_ptr = (u_long *) FPA_RAM_ACC;
    rw_ram_ptr = (u_long *) FPA_RW_RAM;

    *ram_ptr = TEST_PTR(0, UR_LBL);
    tmp_val = *rw_ram_ptr;
    *ram_ptr = TEST_PTR(0, UR_LBM);
    tmp_val1 = *rw_ram_ptr;
    *ram_ptr = TEST_PTR(0, UR_LBH);
    tmp_val2 = *rw_ram_ptr & UR_DMASK;
    if ((tmp_val) || (tmp_val1) || (tmp_val2))
	no_data_flag = 0x0;		       /* data is non zero */

    for (i = 1; i < MAXSIZE_USTORE_RAM; i++) {

	*ram_ptr = TEST_PTR(i, UR_LBL);
	tmp_data = *rw_ram_ptr;
	*ram_ptr = TEST_PTR(i, UR_LBM);
	tmp_data1 = *rw_ram_ptr;
	*ram_ptr = TEST_PTR(i, UR_LBH);
	tmp_data2 = *rw_ram_ptr & UR_DMASK;
	if ((tmp_data) || (tmp_data1) || (tmp_data2))
	    no_data_flag = 0x0;		       /* data is non zero */

	tmp_val = tmp_val ^ tmp_data;
	tmp_val1 = tmp_val1 ^ tmp_data1;
	tmp_val2 = (tmp_val2 ^ tmp_data2) & UR_DMASK;
    }
    if (no_data_flag)
	return (-1);			       /* data is zero , error */
    if ((tmp_val == 0xbeadface) && (tmp_val1 == 0xbeadface) && (tmp_val2 == 0xce))
	return (0);

    else
	return (-1);
}

reg_uh_ram()
{
    u_long          i, tmp_val, tmp_data;
    u_long          tmp_val1, tmp_data1;
    u_long         *ram_ptr, *rw_ram_ptr;
    u_char          no_data_flag = 0xff;

    ram_ptr = (u_long *) FPA_RAM_ACC;
    if (debug)
	printf("passed ram_ptr = (u_long *)FPA_RAM_ACC\n");
    rw_ram_ptr = (u_long *) FPA_RW_LREG_RR;
    if (debug)
	printf("passed rw_ram_ptr = (u_long *)FPA_RW_LREG_RR\n");
    *ram_ptr = TEST_PTR(0x400, RR_LB);
    if (debug)
	printf("passed *ram_ptr = TEST_PTR(0x400, RR_LB)\n");
    tmp_val = *rw_ram_ptr;
    if (debug)
	printf("passed tmp_val = *rw_ram_ptr \n");
    if (tmp_val)
	no_data_flag = 0x0;		       /* data is non zero */

    for (i = 0x401; i <= MAXSIZE_CON_RAM; i++) {

	*ram_ptr = TEST_PTR(i, RR_LB);
	tmp_data = *rw_ram_ptr;
	if (tmp_data)
	    no_data_flag = 0x0;		       /* data is non zero */
	tmp_val = tmp_val ^ tmp_data;
    }
    if (debug)
	printf("passed for loop\n");
    *ram_ptr = TEST_PTR(0x7FF, RR_LB);
    if (debug)
	printf("passed *ram_ptr = TEST_PTR(0x7FF, RR_LB)\n");
    tmp_data = *rw_ram_ptr;
    if (debug)
	printf("passed tmp_data = *rw_ram_ptr\n");
    if (tmp_data)
	no_data_flag = 0x0;		       /* data is non zero */
    tmp_val = tmp_val ^ tmp_data;
    if (debug)
	printf("passed tmp_val = tmp_val ^ tmp_data\n");

    rw_ram_ptr = (u_long *) FPA_RW_MREG_RR;
    if (debug)
	printf("passed rw_ram_ptr = (u_long *)FPA_RW_MREG_RR\n");
    *ram_ptr = TEST_PTR(0x400, RR_LB);
    if (debug)
	printf("passed *ram_ptr = TEST_PTR(0x400, RR_LB)\n");
    tmp_val1 = *rw_ram_ptr;
    if (debug)
	printf("tmp_val1 = %x", tmp_val);
    if (tmp_val1)
	no_data_flag = 0x0;		       /* data is non zero */

    for (i = 0x401; i <= MAXSIZE_CON_RAM; i++) {

	*ram_ptr = TEST_PTR(i, RR_LB);
	tmp_data1 = *rw_ram_ptr;
	if (tmp_data1)
	    no_data_flag = 0x0;		       /* data is non zero */
	tmp_val1 = tmp_val1 ^ tmp_data1;
    }
    if (debug)
	printf("passed for loop\n");
    *ram_ptr = TEST_PTR(0x7FF, RR_LB);
    if (debug)
	printf("passed *ram_ptr = TEST_PTR(0x7FF, RR_LB)\n");
    tmp_data1 = *rw_ram_ptr;
    if (debug)
	printf("passed tmp_data1 = *rw_ram_ptr\n");
    if (debug)
	printf("tmp_data1 = %x\n", tmp_data1);
    if (tmp_data1)
	no_data_flag = 0x0;		       /* data is non zero */
    tmp_val1 = tmp_val1 ^ tmp_data1;
    if (debug)
	printf("passed tmp_val1 = tmp_val1 ^ tmp_data1\n");
    if (debug)
	printf("no_data_flag = %d\n", no_data_flag);
    if (no_data_flag) {
	return (-1);			       /* data is zero , error */
    }
    if (debug)
	printf("tmp_val = %x    tmp_val1 = %x\n", tmp_val, tmp_val1);
    if ((tmp_val == 0xBEADFACE) && (tmp_val1 == 0xBEADFACE))
	return (0);
    else {
	return (-1);
    }
}

pointer_test()
{
    if (pointer_short())
	return (-1);
    if (pointer_sp_ext())
	return (-1);
    if (pointer_dp_ext())
	return (-1);
    if (pointer_cmd())
	return (-1);
    return (0);
}


pointer_cmd()
{
    u_long          temp_ptr3, temp_ptr4, res1, res2;
    u_long         *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
    u_char          i, j, k, l, m, n;

    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    for (n = 0; n < 2; n++) {
	if (n == 0)
	    ptr1 = (u_long *) 0xE0000888;
	else
	    ptr1 = (u_long *) 0xE000088C;


	for (i = 0; i < 28; i++) {
	    for (j = 0; j < 32; j++) {
		ptr2 = (u_long *) users[j].reg;
		*ptr2 = 0x0;
		ptr2++;
		*ptr2 = 0x0;		       /* low order word */
	    }
	    k = i + 1;
	    l = i + 2;
	    m = i + 3;

	    ptr2 = (u_long *) users[k].reg;
	    ptr3 = (u_long *) users[l].reg;
	    ptr4 = (u_long *) users[m].reg;

	    *ptr2 = sp_dp_res[n].reg1;
	    *ptr3 = sp_dp_res[n].reg2;
	    *ptr4 = sp_dp_res[n].reg3;

	    *ptr1 = cmd_reg[i];

	    for (j = 0; j < 32; j++) {
		ptr5 = (u_long *) users[j].reg;
		res1 = *ptr5;

		if (i == j) {
		    if (res1 != sp_dp_res[n].res)
			return (-1);
		} else if (j == k) {
		    if (res1 != sp_dp_res[n].reg1)
			return (-1);
		} else if (j == l) {
		    if (res1 != sp_dp_res[n].reg2)
			return (-1);
		} else if (j == m) {
		    if (res1 != sp_dp_res[n].reg3)
			return (-1);
		} else if (res1 != 0x0)
		    return (-1);
	    }
	}
    }
    return (0);
}

pointer_sp_ext()
{
    u_long          temp_ptr3, temp_ptr4, res1, res2;
    u_long         *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
    u_char          i, j, k, l, m;

    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    for (i = 0; i < 16; i++) {

	ptr4 = (u_long *) sp_extd[i].high;
	ptr5 = (u_long *) sp_extd[i].low;
	for (j = 0; j < 16; j++) {
	    ptr1 = (u_long *) users[j].reg;
	    *ptr1 = 0x0;
	    ptr1++;
	    *ptr1 = 0x0;
	}
	k = (i + 1) & 0xF;
	ptr2 = (u_long *) users[k].reg;
	*ptr2 = 0x40000000;		       /* sp 2 */

	*ptr4 = 0x40400000;		       /* operand1 sp 3 */
	*ptr5 = 0x40800000;		       /* operand2 sp 4 */

	for (j = 0; j < 16; j++) {

	    ptr1 = (u_long *) users[j].reg;
	    res1 = *ptr1;
	    if (i == j) {		       /* it should have the result */
		if (res1 != 0x41200000)
		    return (-1);
	    } else if (j == k) {	       /* reg1 + 1 should have value
					        * sp 2 */
		if (res1 != 0x40000000)
		    return (-1);
	    } else if (res1 != 0x0)
		return (-1);
	}
    }
    return (0);
}

pointer_dp_ext()
{
    u_long          temp_ptr3, temp_ptr4, res1, res2;
    u_long         *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
    u_char          i, j, k, l, m;

    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    for (i = 0; i < 16; i++) {

	ptr4 = (u_long *) dp_extd[i].high;
	ptr5 = (u_long *) dp_extd[i].low;
	for (j = 0; j < 16; j++) {
	    ptr1 = (u_long *) users[j].reg;
	    *ptr1 = 0x0;
	    ptr1++;
	    *ptr1 = 0x0;
	}
	/* the instru ction is reg[i] = reg[i+2] + (reg[i+1] * operand ) */
	k = (i + 1) & 0xF;
	l = (i + 2) & 0xF;

	ptr2 = (u_long *) users[k].reg;
	ptr3 = (u_long *) users[l].reg;
	*ptr2 = 0x40000000;		       /* sp 2 , reg[i+1] */
	*ptr3 = 0x40080000;		       /* sp 3 ,  reg[i+2] */

	*ptr4 = 0x40080000;		       /* operand sp 3 */
	*ptr5 = 0x0;

	for (j = 0; j < 16; j++) {

	    ptr1 = (u_long *) users[j].reg;
	    res1 = *ptr1;
	    if (i == j) {		       /* it should have the result */
		if (res1 != 0x40220000)
		    return (-1);

	    } else if (k == j) {	       /* reg1 + 1 should have value
					        * sp 2 */
		if (res1 != 0x40000000)
		    return (-1);
	    } else if (l == j) {	       /* reg1 + 2 should have value
					        * sp 3 */
		if (res1 != 0x40080000)
		    return (-1);
	    } else if (res1 != 0x0)
		return (-1);
	}
    }
    return (0);
}

pointer_short()
{
    u_long          i, j, k, l, m, n, temp_ptr3, temp_ptr4, res1, res2;
    u_long         *ptr1, *ptr2, *ptr3, *ptr4;


    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;



    for (i = 0; i < 16; i++) {
	ptr3 = (u_long *) sp_short[i];

	for (k = 0; k < 16; k++) {
	    ptr1 = (u_long *) users[k].reg;
	    *ptr1 = 0x0;
	    ptr1++;
	    *ptr1 = 0x0;
	}
	ptr1 = (u_long *) users[i].reg;
	*ptr1 = 0x40000000;		       /* 2 */
	*ptr3 = 0x40C00000;		       /* sp 6 */

	for (k = 0; k < 16; k++) {
	    ptr1 = (u_long *) users[k].reg;
	    res1 = *ptr1;
	    if (i == k) {
		if (res1 != 0x41000000)
		    return (-1);
	    } else {
		if (res1 != 0x0)
		    return (-1);
	    }
	}
    }
    for (i = 0; i < 16; i++) {
	ptr3 = (u_long *) dp_short[i];
	ptr4 = (u_long *) 0xE0001000;

	for (k = 0; k < 16; k++) {
	    ptr1 = (u_long *) users[k].reg;
	    *ptr1 = 0x0;
	    ptr1++;
	    *ptr1 = 0x0;
	}
	ptr1 = (u_long *) users[i].reg;
	*ptr1 = 0x40000000;		       /* dp 2 */
	*ptr3 = 0x40180000;		       /* dp 6 */
	*ptr4 = 0x0;

	for (k = 0; k < 16; k++) {
	    ptr1 = (u_long *) users[k].reg;
	    res1 = *ptr1;
	    if (i == k) {
		if (res1 != 0x40200000)
		    return (-1);
	    } else {
		if (res1 != 0x0)
		    return (-1);
	    }
	}
    }

    return (0);
}

ptr_incdec_test()
{
    u_long          i, j, k, l, m, n, res1, res2;
    u_long         *ptr, *ptr2, *ptr3;


    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;


    ptr = (u_long *) 0xE00009B0;	       /* for transposing */

    for (i = 0; i < 1; i++) {
	for (j = 0; j <= 15; j++) {
	    ptr2 = (u_long *) users[i + j].reg;
	    *ptr2 = j;

	    k = (i + j + 16) & 0x1F;	       /* so that the register will
					        * be 0 - 31 */
	    ptr3 = (u_long *) users[k].reg;
	    *ptr3 = 0x100;
	}
	*ptr = i;
	/* now read the transposed values */
	j = 0;
	for (m = 0; m < 4; m++) {
	    for (n = 0; n < 4; n++) {
		ptr2 = (u_long *) users[i + j].reg;
		k = (i + j + 16) & 0x1F;       /* so that the register will
					        * be 0 - 31 */
		ptr3 = (u_long *) users[k].reg;
		res1 = m + (n * 4);

		if (*ptr2 != res1)
		    return (-1);

		if (*ptr3 != 0x100)
		    return (-1);

		j++;
	    }
	}
    }

    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    ptr = (u_long *) 0xE00008CC;	       /* for dot product */

    for (i = 0; i < 32; i++) {
	ptr2 = (u_long *) users[i].reg;
	*ptr2 = 0x0;
    }
    for (i = 0; i <= 26; i++) {
	ptr3 = (u_long *) users[i + 5].reg;

	for (j = 0; j < 32; j++) {
	    ptr2 = (u_long *) users[j].reg;
	    *ptr2 = 0x0;
	    ptr2++;
	    *ptr2 = 0x0;
	}

	for (j = 0; j <= 4; j++) {
	    ptr2 = (u_long *) users[i + j].reg;
	    *ptr2 = val[j];
	}

	*ptr = ptr_cmd[i].data;		       /* send the insrtuction */
	res1 = *ptr3;			       /* get the result */
	if (res1 != 0x40440000)
	    return (-1);

    }
    return (0);
}

/*
 * This routine checks the ram registers for present context 
 */
reg_ram()
{
    u_long         *ptr, *ptr1, value1, value2, i, j, pattern, pattern1;

    ptr = (u_long *) 0xE0000C00;	       /* starting address for
					        * register ram for present
					        * context */
    pattern = 0x1;
    pattern1 = 0x1;

    for (j = 0; j < 32; j++) {
	ptr = (u_long *) 0xE0000C00;
	for (i = 0; i < 32; i++) {
	    *ptr = pattern;
	    ptr += 1;
	    *ptr = pattern1;
	    ptr += 1;
	}

	ptr = (u_long *) 0xE0000C00;
	for (i = 0; i < 32; i++) {
	    value1 = *ptr;		       /* for most significant word */
	    ptr += 1;
	    value2 = *ptr;		       /* for least significant word */
	    ptr += 1;

	    if (value1 != pattern)
		return (-1);
	    if (value2 != pattern1)
		return (-1);
	}
	pattern = (pattern << 1);
	pattern1 = (pattern1 << 1);
    }
    return (0);
}

/*
 * The following routine tests the shadow ram for the present context 
 */


shadow_ram()
{
    u_long         *ptr, *ptr1, i, j;

    ptr1 = (u_long *) SHADOW_RAM_START;	       /* pointer to the start of the
					        * shadow ram */


    ptr = (u_long *) 0xE0000C00;	       /* starting address for
					        * register ram for */
    /* present context */
    for (i = 0; i < 16; i += 2) {
	*ptr = i + 1;			       /* for most significant word */
	ptr += 1;

	*ptr = i;			       /* for least significant word */
	ptr += 1;
    }

    for (i = 0; i < 16; i += 2) {

	if (*ptr1 != (i + 1))		       /* higher word of shadow ram */
	    return (-1);

	ptr1 += 1;
	if (*ptr1 != i)			       /* lower word of shadow ram */
	    return (-1);

	ptr1 += 1;
    }
    return (0);
}


wlwf_test()
{
    u_long         *ptr1, *ptr2, *ptr3, *op_ptr, *ptr1_lsw, *ptr2_lsw, *ptr3_lsw;
    u_long          i, res1;


    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    ptr1 = (u_long *) REGISTER_ONE_MSW;
    ptr1_lsw = (u_long *) REGISTER_ONE_LSW;
    ptr2 = (u_long *) REGISTER_TWO_MSW;
    ptr2_lsw = (u_long *) REGISTER_TWO_LSW;
    ptr3 = (u_long *) REGISTER_THREE_MSW;
    ptr3_lsw = (u_long *) REGISTER_THREE_LSW;

    for (i = 0; fval[i].addr != 0x0; i++) {

	*ptr1 = 0x0;
	*ptr1_lsw = 0x0;
	*ptr2 = 0x0;
	*ptr2_lsw = 0x0;
	*ptr3 = 0x0;
	*ptr3_lsw = 0x0;

	op_ptr = (u_long *) fval[i].addr;
	*ptr2 = fval[i].reg2;
	*ptr3 = fval[i].reg3;

	*op_ptr = 0x30081;		       /* reg1 <- reg2 op reg3 */

	res1 = *ptr1;			       /* read the result */
	if (res1 != fval[i].result)
	    return (-1);

    }
    return (0);
}


check_clear_hardpipe()
{
    u_long         *ierr_ptr, *pipe_status, *hard_pipe;

    pipe_status = (u_long *) (FPA_BASE + FPA_PIPE_STATUS);
    hard_pipe = (u_long *) (FPA_BASE + FPA_CLR_PIPEHARD);
    ierr_ptr = (u_long *) (FPA_BASE + FPA_IERR);

    if (*ierr_ptr & 0x200000) {
	printf("warning: pipe is not cleared from prev. oper., clearing now\n");
	*hard_pipe = 0x0;
    }
}

/*
 * ================================================ immediate errors register
 * test subroutine ================================================ 
 */

ierr_test()
{
    register u_long i, j, tmp_data, test_data;
    u_long         *ptr;

    ptr = (u_long *) FPA_IERR_PTR;

    for (i = 0; i < 256; i++) {

	test_data = (i << 16) & 0xFF0000;
	*ptr = test_data;
	tmp_data = *ptr & IERR_MASK;
	if (test_data != tmp_data)
	    return (-1);


    }
    return (0);
}

/*
 * ================================================ fpa inexact error mask
 * register test subroutine ================================================ 
 */

imask_test()
{
    u_long         *ptr, i, temp_val;

    ptr = (u_long *) FPA_IMASK_PTR;

    *ptr = 0;
    if ((*ptr & 0x1) != 0)
	return (-1);


    *ptr = 0x1;
    if ((*ptr & 0x1) != 0x1)
	return (-1);

    return (0);
}
/*
 * ================================================ load pointer register
 * test subroutine ================================================ 
 */

ldptr_test()
{
    register u_long *ram_ptr, i, j, tmp_data, test_data;

    ram_ptr = (u_long *) FPA_RAM_ACC;
    test_data = ROTATE_DATA;
    for (i = 0; i < CNT_ROTATE; i++) {

	/* you need to mask the unused bits */
	test_data &= 0xFFFF;
	*ram_ptr = test_data;
	tmp_data = *ram_ptr & 0xFFFF;	       /* & LPTR_MASK; */
	if (test_data != tmp_data)
	    return (-1);

	test_data = test_data << 1;
    }
    return (0);
}



sim_ins_test()
{
    u_long          index, temp_value;
    u_long         *pipe_status, *addr_ptr;

    pipe_status = (u_long *) (FPA_BASE + FPA_STABLE_PIPE_STATUS);	/* for stabel pipe
									 * status */


    for (index = 0; instr1[index].address != 0; index++) {
	addr_ptr = (u_long *) instr1[index].address;
	*addr_ptr = 0;
	temp_value = (*pipe_status & 0xFF0000) >> 16;
	if (temp_value != instr1[index].status)
	    return (-1);

    }
    /* clear the hard pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;
    return (0);
}

/*
 * The following test is for testing the micro store type registers those are
 * pointer - 5, loop counter, mode register, wstat register 
 *
 */
test_mode_reg()
{

    int             i;
    u_long         *ptr1, *ptr2;


    /* set the IMASK register = 0 */
    *(u_long *) FPA_IMASK_PTR = 0x0;

    /* clear pipe */
    ptr1 = (u_long *) FPA_STATE_PTR;
    /* *ptr1 = 0x40; */
    ptr2 = (u_long *) FPA_CLEAR_PIPE_PTR;
    *ptr2 = 0x0;

    ptr1 = (u_long *) (FPA_BASE + FPA_MODE3_0C);
    ptr2 = (u_long *) (FPA_BASE + FPA_MODE3_0S);

    for (i = 0; i < 0x10; i++) {

	/* write into the mode reg. E00008D0 */

	*(u_long *) MODE_WRITE_REGISTER = i;
	/*
	 * now read at two places, mode-reg stable and mode reg clear
	 * mode_reg stable = 0xE0000F38, mode_reg clear = 0xE0000FB8 
	 */

	if ((*ptr1 & 0x0F) != i)
	    return (-1);

	if ((*ptr2 & 0x0F) != i)
	    return (-1);

    }
    /* clear hard pipe */

    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;
    return (0);
}

test_wstatus_reg()
{

    int             i, result;
    u_long         *ptr1, *ptr2, res1, res2;
    /* u_long  *st_reg_ptr; */


    /*
     * we test wstatus read stable and clear by writing 4bit pattern to
     * 0xE0000958 location.  because there are only 4 bits that matters 
     */
    /*
     * and check this with IMASK bit = 0 (errors are disabled) IMASK bit = 1
     * (errors are enabled)  
     */


    st_reg_ptr = (u_long *) FPA_STATE_PTR;
    /* *st_reg_ptr = 0x40; */

    ptr1 = (u_long *) FPA_IMASK_PTR;
    *ptr1 = 0x0;			       /* IMASK bit is 0 */

    ptr1 = (u_long *) (FPA_BASE + FPA_WSTATUSC);	/* wstatus clear - 0FBC */
    ptr2 = (u_long *) (FPA_BASE + FPA_WSTATUSS);	/* wstatus stable - 0F3c */
    for (i = 0; i < 0x10; i++) {

	*(u_long *) WSTATUS_WRITE_REGISTER = (i << 8);

	res1 = *ptr1 & 0xEF1F;
	res2 = *ptr2 & 0xEF1F;

	if (res1 != wstatus_res[i])
	    return (-1);

	if (res2 != wstatus_res[i])
	    return (-1);
    }

    ptr1 = (u_long *) FPA_IMASK_PTR;
    *ptr1 = 0x1;			       /* IMASK bit is 1 */

    ptr1 = (u_long *) (FPA_BASE + FPA_WSTATUSC);	/* wstatus clear */

    for (i = 0; i < 0x10; i++) {

	*(u_long *) WSTATUS_WRITE_REGISTER = (i << 8);

	res1 = *ptr1 & 0xEF1F;
	res2 = *ptr2 & 0xEF1F;

	if (res1 != wstatus_res1[i])
	    return (-1);
	if (res2 != wstatus_res1[i])
	    return (-1);
    }
    /* clear hard pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;
    /* do ttest using unimplemented instruction */
    /* do the valid bit test by clearing the pipe, write the status and check */
    /* st_reg_ptr = 0x0; */
    return (0);
}

w_jump_cond_test()
{
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;


    /* clear the pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;


    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    /* set the IMASK register to 0 */
    *(u_long *) FPA_IMASK_PTR = 0x0;

    *(u_long *) REGISTER_ONE_MSW = 0x01000000; /* */
    *(u_long *) REGISTER_TWO_MSW = 0x81000000;
    *(u_long *) REGISTER_THREE_MSW = 0x3F800000;
    *(u_long *) REGISTER_FOUR_MSW = 0xBF800000;
    *(u_long *) REGISTER_FIVE_MSW = 0x40000000;
    *(u_long *) REGISTER_SIX_MSW = 0x70000000;
    *(u_long *) REGISTER_SEVEN_MSW = 0xF0000000;

    *(u_long *) 0xE0000800 = 0x41;	       /* reg1 <- sine(reg1) */
    *(u_long *) 0xE0000800 = 0x82;	       /* reg2 <- sine(reg2) */
    *(u_long *) 0xE0000800 = 0xC3;	       /* reg3 <- sine(reg3) */
    *(u_long *) 0xE0000800 = 0x104;	       /* reg4 <- sine(reg4) */

    *(u_long *) 0xE0000818 = 0x145;	       /* reg5 <- atan(reg5) */
    *(u_long *) 0xE0000818 = 0x186;	       /* reg6 <- atan(reg6) */
    *(u_long *) 0xE0000818 = 0x1C7;	       /* reg7 <- atan(reg7) */

    val1_msw = *(u_long *) REGISTER_ONE_MSW;
    val1_lsw = *(u_long *) REGISTER_TWO_MSW;
    val2_msw = *(u_long *) REGISTER_THREE_MSW;
    val2_lsw = *(u_long *) REGISTER_FOUR_MSW;
    val3_msw = *(u_long *) REGISTER_FIVE_MSW;
    val3_lsw = *(u_long *) REGISTER_SIX_MSW;
    val4_msw = *(u_long *) REGISTER_SEVEN_MSW;

    if (val1_msw != 0x01000000)
	return (-1);

    if (val1_lsw != 0x81000000)
	return (-1);

    if (val2_msw != 0x3F576AA4)
	return (-1);

    if (val2_lsw != 0xBF576AA4)
	return (-1);

    if (val3_msw != 0x3F8DB70D)
	return (-1);

    if (val3_lsw != 0x3FC90FDB)
	return (-1);
    if (val4_msw != 0xBFC90FDB)
	return (-1);


    /* clear the pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;

    return (0);
}


/*
 * This test exercises the data bus of the Weitek chips.  To check the bus to
 * the ALU different data values are added to zero.  For the multiplier the
 * value is multiplied by one.  To give some test of the data paths in the
 * Weitek chips both single and double precision data values are used. 
 *
 * test the single precision case 
 *
 * load the microcode and mapping rams 
 *
 * the FPA users registers contain the following: reg0 = 0 reg1 = 1 reg2 = value
 * under test reg3 = reg0 + reg2 reg4 = reg1 * reg2 
 *
 */

fpa_wd()
{
    u_long         *add, *mult;
    u_long          fshift, shift2, value_msw, value_lsw, value;
    u_long          mult_result_lsw, mult_result_msw, add_result_lsw, add_result_msw;
    u_long          add_result, mult_result;
    int             i, j;



    /* clear the pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;
    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;


    /* setup reg0 and reg1 */

    *(u_long *) REGISTER_ZERO_MSW = 0;
    *(u_long *) REGISTER_ONE_MSW = 0x3f800000;
    add = (u_long *) 0xe0000a80;	       /* command registter format */
    mult = (u_long *) 0xe0000a08;	       /* command register format */

    /* transmit value and test */

    for (i = 0; i < 2; i++) {

	for (j = 1; j < 255; j++) {

	    for (fshift = 0; fshift < 23; fshift++) {

		value = (i << 31) | (j << 23) | (1 << fshift);
		*(u_long *) REGISTER_TWO_MSW = value;
		*add = 0x00020003;
		add_result = *(u_long *) REGISTER_THREE_MSW;	/* reg3 */
		if (add_result != value)
		    return (-1);

		*mult = 0x00020044;
		mult_result = *(u_long *) REGISTER_FOUR_MSW;	/* reg4 */
		if (mult_result != value)
		    return (-1);

	    }
	}
    }


    /* test the double precision case */


    /*
     * the FPA users registers contain the following: reg0 = 0 reg1 = 1 reg2
     * = value under test reg3 = reg0 + reg2 reg4 = reg1 * reg2 
     */

    /* setup reg0 and reg1 */

    *(u_long *) REGISTER_ZERO_MSW = 0;	       /* reg0 = 0 */
    *(u_long *) REGISTER_ZERO_LSW = 0;

    *(u_long *) REGISTER_ONE_MSW = 0x3ff00000; /* reg1 = 1 */
    *(u_long *) REGISTER_ONE_LSW = 0x00000000;

    add = (u_long *) 0xe0000a84;
    mult = (u_long *) 0xe0000a0c;

    /* transmit value and test */

    for (i = 0; i < 2; i++) {

	for (j = 1; j < 2047; j++) {

	    for (fshift = 0; fshift < 52; fshift++) {

		value_lsw = (1 << fshift);
		if (fshift > 32)
		    shift2 = fshift - 32;
		else
		    shift2 = 32;
		value_msw = (i << 31) | (j << 20) | (1 << shift2);
		*(u_long *) REGISTER_TWO_MSW = value_msw;
		*(u_long *) REGISTER_TWO_LSW = value_lsw;

		*add = 0x00020003;
		add_result_msw = *(u_long *) REGISTER_THREE_MSW;	/* reg3 */
		add_result_lsw = *(u_long *) REGISTER_THREE_LSW;	/* reg3 */

		if (add_result_msw != value_msw)
		    return (-1);

		if (add_result_lsw != value_lsw)
		    return (-1);


		*mult = 0x00020044;
		mult_result_msw = *(u_long *) REGISTER_FOUR_MSW;	/* reg4 */
		mult_result_lsw = *(u_long *) REGISTER_FOUR_LSW;	/* reg4 */

		if (mult_result_msw != value_msw)
		    return (-1);


		if (mult_result_lsw != value_lsw)
		    return (-1);

	    }
	}
    }
    return (0);
}

/*
 * Weitek operation 
 */

w_op_test()
{
    u_long          temp_val;
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;
    u_long         *opcode_addr;


    /* clear the pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;


    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    /* set the IMASK register = 0 */
    *(u_long *) FPA_IMASK_PTR = 0x0;

    w_op_com_sp();
    w_op_com_dp();
    w_op_ext_opr();
    w_op_ext_sp();
    w_op_ext_dp();
    w_op_short_sp();
    w_op_short_doublep();

    /* clear the pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;
    return (0);

}
w_op_com_sp()
{
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;

    *(u_long *) REGISTER_ONE_MSW = 0x3F800000; /* 1 */
    *(u_long *) REGISTER_TWO_MSW = 0x3F800000; /* 1 */
    *(u_long *) REGISTER_THREE_MSW = 0x40400000;	/* 3 */
    *(u_long *) REGISTER_FOUR_MSW = 0x40000000;/* 2 */
    *(u_long *) REGISTER_SIX_MSW = 0x3F800000; /* 1 */
    *(u_long *) REGISTER_SEVEN_MSW = 0x40400000;	/* 3 */
    *(u_long *) REGISTER_EIGHT_MSW = 0x40000000;	/* 2 */
    *(u_long *) REGISTER_TEN_MSW = 0x40400000; /* 3 */
    *(u_long *) REGISTER_ELEVEN_MSW = 0x40400000;	/* 3 */
    *(u_long *) REGISTER_TWELVE_MSW = 0x40000000;	/* 2 */

    *(u_long *) 0xE0000880 = 0x40;	       /* reg0 <- reg1 */
    *(u_long *) 0xE0000888 = 0x10030081;       /* reg1 <- reg3 + (reg2 *
					        * reg4) */
    *(u_long *) 0xE0000890 = 0x20070185;       /* reg5 <- reg7 - (reg6 *
					        * reg8) */
    *(u_long *) 0xE0000898 = 0x300B0289;       /* reg9 <- (-reg11) + (reg10 *
					        * reg12) */

    val1_msw = *(u_long *) REGISTER_ZERO_MSW;
    val2_msw = *(u_long *) REGISTER_ONE_MSW;
    val3_msw = *(u_long *) REGISTER_FIVE_MSW;
    val4_msw = *(u_long *) REGISTER_NINE_MSW;

    *(u_long *) REGISTER_TWO_MSW = 0x40000000; /* 2 */
    *(u_long *) REGISTER_THREE_MSW = 0x40000000;	/* 2 */
    *(u_long *) REGISTER_FOUR_MSW = 0x3F800000;/* 1 */
    *(u_long *) REGISTER_SIX_MSW = 0x40800000; /* 4 */
    *(u_long *) REGISTER_SEVEN_MSW = 0x40400000;	/* 3 */
    *(u_long *) REGISTER_EIGHT_MSW = 0x40000000;	/* 2 */
    *(u_long *) REGISTER_TEN_MSW = 0x3F800000; /* 1 */
    *(u_long *) REGISTER_ELEVEN_MSW = 0x40400000;	/* 3 */
    *(u_long *) REGISTER_TWELVE_MSW = 0x40800000;	/* 4 */

    *(u_long *) 0xE00008A0 = 0x10030081;       /* reg1 <- reg3 * (reg2 +
					        * reg4) */
    *(u_long *) 0xE00008A8 = 0x20070185;       /* reg5 <- reg7 * (reg6 -
					        * reg8) */
    *(u_long *) 0xE00008B0 = 0x300B0289;       /* reg9 <- reg11 * (-reg10 +
					        * reg12) */

    val1_lsw = *(u_long *) REGISTER_ONE_MSW;
    val2_lsw = *(u_long *) REGISTER_FIVE_MSW;
    val3_lsw = *(u_long *) REGISTER_NINE_MSW;

    if (val1_msw != 0x3F800000)
	return (-1);

    if (val2_msw != 0x40A00000)
	return (-1);

    if (val3_msw != 0x3F800000)
	return (-1);

    if (val4_msw != 0x40400000)
	return (-1);

    if (val1_lsw != 0x40C00000)
	return (-1);

    if (val2_lsw != 0x40C00000)
	return (-1);

    if (val3_lsw != 0x41100000)
	return (-1);

    return (0);

}
w_op_com_dp()
{
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;

    *(u_long *) REGISTER_ONE_MSW = 0x3FF00000; /* 1 */
    *(u_long *) REGISTER_ONE_LSW = 0x0;
    *(u_long *) REGISTER_TWO_MSW = 0x3FF00000; /* 1 */
    *(u_long *) REGISTER_TWO_LSW = 0x0;
    *(u_long *) REGISTER_THREE_MSW = 0x40080000;	/* 3 */
    *(u_long *) REGISTER_THREE_LSW = 0x0;
    *(u_long *) REGISTER_FOUR_MSW = 0x40000000;/* 2 */
    *(u_long *) REGISTER_FOUR_LSW = 0x0;
    *(u_long *) REGISTER_SIX_MSW = 0x3FF00000; /* 1 */
    *(u_long *) REGISTER_SIX_LSW = 0x0;
    *(u_long *) REGISTER_SEVEN_MSW = 0x40080000;	/* 3 */
    *(u_long *) REGISTER_SEVEN_LSW = 0x0;
    *(u_long *) REGISTER_EIGHT_MSW = 0x40000000;	/* 2 */
    *(u_long *) REGISTER_EIGHT_LSW = 0x0;
    *(u_long *) REGISTER_TEN_MSW = 0x40080000; /* 3 */
    *(u_long *) REGISTER_TEN_LSW = 0x0;
    *(u_long *) REGISTER_ELEVEN_MSW = 0x40080000;	/* 3 */
    *(u_long *) REGISTER_ELEVEN_LSW = 0x0;
    *(u_long *) REGISTER_TWELVE_MSW = 0x40000000;	/* 2 */
    *(u_long *) REGISTER_TWELVE_LSW = 0x0;

    *(u_long *) 0xE0000884 = 0x40;	       /* reg0 <- reg1 */
    *(u_long *) 0xE000088C = 0x10030081;       /* reg1 <- reg3 + (reg2 *
					        * reg4) */
    *(u_long *) 0xE0000894 = 0x20070185;       /* reg5 <- reg7 - (reg6 *
					        * reg8) */
    *(u_long *) 0xE000089C = 0x300B0289;       /* reg9 <- (-reg11) + (reg10 *
					        * reg12) */

    val1_msw = *(u_long *) REGISTER_ZERO_MSW;
    val1_lsw = *(u_long *) REGISTER_ZERO_LSW;
    val2_msw = *(u_long *) REGISTER_ONE_MSW;
    val2_lsw = *(u_long *) REGISTER_ONE_LSW;
    val3_msw = *(u_long *) REGISTER_FIVE_MSW;
    val3_lsw = *(u_long *) REGISTER_FIVE_LSW;
    val4_msw = *(u_long *) REGISTER_NINE_MSW;
    val4_lsw = *(u_long *) REGISTER_NINE_LSW;


    if (val1_msw != 0x3FF00000)
	return (-1);

    if (val1_lsw != 0x00000000)
	return (-1);

    if (val2_msw != 0x40140000)
	return (-1);

    if (val2_lsw != 0x00000000)
	return (-1);

    if (val3_msw != 0x3FF00000)
	return (-1);

    if (val3_lsw != 0x00000000)
	return (-1);

    if (val4_msw != 0x40080000)
	return (-1);

    if (val4_lsw != 0x00000000)
	return (-1);


    *(u_long *) REGISTER_TWO_MSW = 0x40000000; /* 2 */
    *(u_long *) REGISTER_TWO_LSW = 0x0;
    *(u_long *) REGISTER_THREE_MSW = 0x40000000;	/* 2 */
    *(u_long *) REGISTER_THREE_LSW = 0x0;
    *(u_long *) REGISTER_FOUR_MSW = 0x3FF00000;/* 1 */
    *(u_long *) REGISTER_FOUR_LSW = 0x0;
    *(u_long *) REGISTER_SIX_MSW = 0x40100000; /* 4 */
    *(u_long *) REGISTER_SIX_LSW = 0x0;
    *(u_long *) REGISTER_SEVEN_MSW = 0x40080000;	/* 3 */
    *(u_long *) REGISTER_SEVEN_LSW = 0x0;
    *(u_long *) REGISTER_EIGHT_MSW = 0x40000000;	/* 2 */
    *(u_long *) REGISTER_EIGHT_LSW = 0x0;
    *(u_long *) REGISTER_TEN_MSW = 0x3FF00000; /* 1 */
    *(u_long *) REGISTER_TEN_LSW = 0x0;
    *(u_long *) REGISTER_ELEVEN_MSW = 0x40080000;	/* 3 */
    *(u_long *) REGISTER_ELEVEN_LSW = 0x0;
    *(u_long *) REGISTER_TWELVE_MSW = 0x40100000;	/* 4 */
    *(u_long *) REGISTER_TWELVE_LSW = 0x0;

    *(u_long *) 0xE00008A4 = 0x10030081;       /* reg1 <- reg3 * (reg2 +
					        * reg4) */
    *(u_long *) 0xE00008AC = 0x20070185;       /* reg5 <- reg7 * (reg6 -
					        * reg8) */
    *(u_long *) 0xE00008B4 = 0x300B0289;       /* reg9 <- reg11 * (-reg10 +
					        * reg12) */

    val2_msw = *(u_long *) REGISTER_ONE_MSW;
    val2_lsw = *(u_long *) REGISTER_ONE_LSW;
    val3_msw = *(u_long *) REGISTER_FIVE_MSW;
    val3_lsw = *(u_long *) REGISTER_FIVE_LSW;
    val4_msw = *(u_long *) REGISTER_NINE_MSW;
    val4_lsw = *(u_long *) REGISTER_NINE_LSW;

    if (val2_msw != 0x40180000)
	return (-1);

    if (val2_lsw != 0x00000000)
	return (-1);

    if (val3_msw != 0x40180000)
	return (-1);

    if (val3_lsw != 0x00000000)
	return (-1);

    if (val4_msw != 0x40220000)
	return (-1);

    if (val4_lsw != 0x00000000)
	return (-1);

}
w_op_ext_opr()
{
    u_long          temp_val;
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;

    *(u_long *) REGISTER_TWO_MSW = 0x40000000; /* 2 */
    *(u_long *) REGISTER_FOUR_MSW = 0x40400000;/* 3 */
    *(u_long *) REGISTER_SIX_MSW = 0x40800000; /* 4 */
    *(u_long *) REGISTER_EIGHT_MSW = 0x3F800000;	/* 1 */
    *(u_long *) REGISTER_TEN_MSW = 0x40800000; /* 4 */
    *(u_long *) REGISTER_TWELVE_MSW = 0xC0000000;	/*-2 */

    *(u_long *) 0xE0001010 = 0x3F800000;       /* 1 *//* reg1 <- op2 + (reg2 *
					        * op1) */
    *(u_long *) 0xE0001808 = 0x40800000;       /* 4  op2 */
    *(u_long *) 0xE00010A0 = 0x3F800000;       /* 1 *//* reg3 <- op2 - (reg4 *
					        * op1) */
    *(u_long *) 0xE0001818 = 0x41100000;       /* 9 op2 */
    *(u_long *) 0xE0001130 = 0x40000000;       /* 2 *//* reg5 <- (-op2) +
					        * (reg6 * op1) */
    *(u_long *) 0xE0001828 = 0x40000000;       /* 2 op2 */
    *(u_long *) 0xE00011C0 = 0x3F800000;       /* 1 *//* reg7 <- op2 * (reg8
					        * + op1) */
    *(u_long *) 0xE0001838 = 0x40400000;       /* 3 op2 */
    *(u_long *) 0xE0001250 = 0x3F800000;       /* 1 *//* reg9 <- op2 * (reg10
					        * - op1) */
    *(u_long *) 0xE0001848 = 0x40000000;       /* 2 op2 */
    *(u_long *) 0xE00012E0 = 0x3F800000;       /* 1 *//* reg11 <- op2 *
					        * (-reg12 + op1) */
    *(u_long *) 0xE0001858 = 0x40000000;       /* 2  op2 */

    val1_msw = *(u_long *) REGISTER_ONE_MSW;
    val2_msw = *(u_long *) REGISTER_THREE_MSW;
    val3_msw = *(u_long *) REGISTER_FIVE_MSW;
    val4_msw = *(u_long *) REGISTER_SEVEN_MSW;
    val1_lsw = *(u_long *) REGISTER_NINE_MSW;
    val2_lsw = *(u_long *) REGISTER_ELEVEN_MSW;

    if (val1_msw != 0x40C00000)
	return (-1);

    if (val2_msw != 0x40C00000)
	return (-1);

    if (val3_msw != 0x40C00000)
	return (-1);

    if (val4_msw != 0x40C00000)
	return (-1);

    if (val1_lsw != 0x40C00000)
	return (-1);

    if (val2_lsw != 0x40C00000)
	return (-1);


}
w_op_ext_sp()
{
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;

    *(u_long *) REGISTER_TWO_MSW = 0x40400000; /* 3 */
    *(u_long *) REGISTER_FOUR_MSW = 0x41000000;/* 8 */
    *(u_long *) REGISTER_SIX_MSW = 0x40400000; /* 3 */
    *(u_long *) REGISTER_EIGHT_MSW = 0x41100000;	/* 9 */
    *(u_long *) REGISTER_TEN_MSW = 0x40400000; /* 3 */
    *(u_long *) REGISTER_TWELVE_MSW = 0x40400000;	/* 3 */

    *(u_long *) 0xE0001310 = 0x40400000;       /* 3 *//* reg1 <- reg2 +
					        * operand */
    *(u_long *) 0xE0001808 = 0x0;
    *(u_long *) 0xE00013A0 = 0x40000000;       /* 2 *//* reg3 <- reg4 -
					        * operand */
    *(u_long *) 0xE0001818 = 0x0;
    *(u_long *) 0xE0001430 = 0x40000000;       /* 2 *//* reg5 <- reg6 *
					        * operand */
    *(u_long *) 0xE0001828 = 0x0;
    *(u_long *) 0xE00014C0 = 0x40400000;       /* 3 *//* reg7 <- reg8 /
					        * operand */
    *(u_long *) 0xE0001838 = 0x0;
    *(u_long *) 0xE0001550 = 0x41100000;       /* 9 *//* reg9 <- operand -
					        * reg10 */
    *(u_long *) 0xE0001848 = 0x0;
    *(u_long *) 0xE00015E0 = 0x41100000;       /* 9 *//* reg11 <- operand /
					        * reg12 */
    *(u_long *) 0xE0001858 = 0x0;

    val1_msw = *(u_long *) REGISTER_ONE_MSW;
    val1_lsw = *(u_long *) REGISTER_THREE_MSW;
    val2_msw = *(u_long *) REGISTER_FIVE_MSW;
    val2_lsw = *(u_long *) REGISTER_SEVEN_MSW;
    val3_msw = *(u_long *) REGISTER_NINE_MSW;
    val3_lsw = *(u_long *) REGISTER_ELEVEN_MSW;

    if (val1_msw != 0x40C00000)
	return (-1);

    if (val1_lsw != 0x40C00000)
	return (-1);

    if (val2_msw != 0x40C00000)
	return (-1);

    if (val2_lsw != 0x40400000)
	return (-1);

    if (val3_msw != 0x40C00000)
	return (-1);

    if (val3_lsw != 0x40400000)
	return (-1);


}
w_op_ext_dp()
{
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;
    u_long          val5_msw, val5_lsw, val6_msw, val6_lsw;

    *(u_long *) REGISTER_TWO_MSW = 0x40080000; /* 3 */
    *(u_long *) REGISTER_TWO_LSW = 0x0;
    *(u_long *) REGISTER_FOUR_MSW = 0x40200000;/* 8 */
    *(u_long *) REGISTER_FOUR_LSW = 0x0;
    *(u_long *) REGISTER_SIX_MSW = 0x40080000; /* 3 */
    *(u_long *) REGISTER_SIX_LSW = 0x0;
    *(u_long *) REGISTER_EIGHT_MSW = 0x40220000;	/* 9 */
    *(u_long *) REGISTER_EIGHT_LSW = 0x0;
    *(u_long *) REGISTER_TEN_MSW = 0x40080000; /* 3 */
    *(u_long *) REGISTER_TEN_LSW = 0x0;
    *(u_long *) REGISTER_TWELVE_MSW = 0x40080000;	/* 3 */
    *(u_long *) REGISTER_TWELVE_LSW = 0x0;

    *(u_long *) 0xE0001314 = 0x40080000;       /* 3 *//* reg1 <- reg2 +
					        * operand */
    *(u_long *) 0xE000180C = 0x0;
    *(u_long *) 0xE00013A4 = 0x40000000;       /* 2 *//* reg3 <- reg4 -
					        * operand */
    *(u_long *) 0xE000181C = 0x0;
    *(u_long *) 0xE0001434 = 0x40000000;       /* 2 *//* reg5 <- reg6 *
					        * operand */
    *(u_long *) 0xE000182C = 0x0;
    *(u_long *) 0xE00014C4 = 0x40080000;       /* 3 *//* reg7 <- reg8 /
					        * operand */
    *(u_long *) 0xE000183C = 0x0;
    *(u_long *) 0xE0001554 = 0x40220000;       /* 9 *//* reg9 <- operand -
					        * reg10 */
    *(u_long *) 0xE000184C = 0x0;
    *(u_long *) 0xE00015E4 = 0x40220000;       /* 9 *//* reg11 <- operand /
					        * reg12 */
    *(u_long *) 0xE000185C = 0x0;

    val1_msw = *(u_long *) REGISTER_ONE_MSW;
    val1_lsw = *(u_long *) REGISTER_ONE_LSW;
    val2_msw = *(u_long *) REGISTER_THREE_MSW;
    val2_lsw = *(u_long *) REGISTER_THREE_LSW;
    val3_msw = *(u_long *) REGISTER_FIVE_MSW;
    val3_lsw = *(u_long *) REGISTER_FIVE_LSW;
    val4_msw = *(u_long *) REGISTER_SEVEN_MSW;
    val4_lsw = *(u_long *) REGISTER_SEVEN_LSW;
    val5_msw = *(u_long *) REGISTER_NINE_MSW;
    val5_lsw = *(u_long *) REGISTER_NINE_LSW;
    val6_msw = *(u_long *) REGISTER_ELEVEN_MSW;
    val6_lsw = *(u_long *) REGISTER_ELEVEN_LSW;


    if (val1_msw != 0x40180000)
	return (-1);

    if (val1_lsw != 0x00000000)
	return (-1);

    if (val2_msw != 0x40180000)
	return (-1);

    if (val2_lsw != 0x00000000)
	return (-1);

    if (val3_msw != 0x40180000)
	return (-1);

    if (val3_lsw != 0x00000000)
	return (-1);

    if (val4_msw != 0x40080000)
	return (-1);

    if (val4_lsw != 0x00000000)
	return (-1);

    if (val5_msw != 0x40180000)
	return (-1);

    if (val5_lsw != 0x00000000)
	return (-1);

    if (val6_msw != 0x40080000)
	return (-1);

    if (val6_lsw != 0x00000000)
	return (-1);


}
w_op_short_sp()
{
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;
    u_long          val5_msw, val5_lsw, val6_msw, val6_lsw;
    u_long          val7_msw, val7_lsw, val8_msw, val8_lsw;
    u_long          val9_msw, val9_lsw, val10_msw, val10_lsw;
    u_long          val11_msw, val11_lsw, val12_msw, val12_lsw;
    u_long          temp_res;

    *(u_long *) REGISTER_SEVEN_MSW = 0x3F800000;	/* 1 */
    *(u_long *) REGISTER_EIGHT_MSW = 0x40C00000;	/* 6 */
    *(u_long *) REGISTER_NINE_MSW = 0x40400000;/* 3 */
    *(u_long *) REGISTER_TEN_MSW = 0x41100000; /* 9 */
    *(u_long *) REGISTER_ELEVEN_MSW = 0x40400000;	/* 3 */
    *(u_long *) REGISTER_TWELVE_MSW = 0x40400000;	/* 3 */

    *(u_long *) 0xE0000088 = 0x3F800000;       /* 1  reg1 <- negate operand */
    *(u_long *) 0xE0000110 = 0x40400000;       /* 3 *//* reg2 <- absolute
					        * value operand */
    *(u_long *) 0xE0000198 = 0x4;	       /* 4 *//* reg3 <- float
					        * operand */
    *(u_long *) 0xE0000220 = 0x41800000;       /* 16 *//* reg4 <- fix operand */
    *(u_long *) 0xE00002A8 = 0x41100000;       /* 9 *//* reg5 <- convert (dp
					        * to sp) operand */
    *(u_long *) 0xE0000330 = 0x40800000;       /* 4 *//* reg6 <- square of
					        * operand */
    *(u_long *) 0xE00003B8 = 0x3F800000;       /* 1 *//* reg7 <- reg7 +
					        * operand */
    *(u_long *) 0xE0000440 = 0x40000000;       /* 2 *//* reg8 <- reg8 -
					        * operand */
    *(u_long *) 0xE00004C8 = 0x40000000;       /* 2 *//* reg9 <- reg9 *
					        * operand */
    *(u_long *) 0xE0000550 = 0x40400000;       /* 3 *//* reg10 <- reg10 /
					        * operand */
    *(u_long *) 0xE00005D8 = 0x41100000;       /* 9 *//* reg11 <- operand -
					        * reg11 */
    *(u_long *) 0xE0000660 = 0x41100000;       /* 9 *//* reg12 <- operand /
					        * reg12 */

    val1_msw = *(u_long *) REGISTER_ONE_MSW;
    val2_msw = *(u_long *) REGISTER_TWO_MSW;
    val3_msw = *(u_long *) REGISTER_THREE_MSW;
    val4_msw = *(u_long *) REGISTER_FOUR_MSW;
    val5_msw = *(u_long *) REGISTER_FIVE_MSW;
    val6_msw = *(u_long *) REGISTER_SIX_MSW;
    val7_msw = *(u_long *) REGISTER_SEVEN_MSW;
    val8_msw = *(u_long *) REGISTER_EIGHT_MSW;
    val9_msw = *(u_long *) REGISTER_NINE_MSW;
    val10_msw = *(u_long *) REGISTER_TEN_MSW;
    val11_msw = *(u_long *) REGISTER_ELEVEN_MSW;
    val12_msw = *(u_long *) REGISTER_TWELVE_MSW;

    if (val1_msw != 0xBF800000)
	return (-1);

    if (val2_msw != 0x40400000)
	return (-1);

    if (val3_msw != 0x40800000)
	return (-1);

    if (val4_msw != 0x10)
	return (-1);

    if (val5_msw != 0x40220000)
	return (-1);

    if (val6_msw != 0x41800000)
	return (-1);

    if (val7_msw != 0x40000000)
	return (-1);

    if (val8_msw != 0x40800000)
	return (-1);

    if (val9_msw != 0x40C00000)
	return (-1);

    if (val10_msw != 0x40400000)
	return (-1);

    if (val11_msw != 0x40C00000)
	return (-1);

    if (val12_msw != 0x40400000)
	return (-1);

    *(u_long *) 0xE0000680 = 0x3F800000;       /* operand compare with 0 */

    temp_res = *(u_long *) (FPA_BASE + FPA_WSTATUSC);
    val1_msw = (temp_res & 0xF00) >> 8;	       /* get the values of status
					        * register */
    val1_lsw = temp_res & 0xF;		       /* get the decoded value */
    if (val1_msw != 0x2)
	return (-1);

    if (val1_lsw != 0x0)
	return (-1);


    *(u_long *) REGISTER_ONE_MSW = 0x3F800000; /* 1 */
    *(u_long *) 0xE0000708 = 0x3F800000;       /* reg1 compare with operand */
    temp_res = *(u_long *) (FPA_BASE + FPA_WSTATUSC);
    val1_msw = (temp_res & 0xF00) >> 8;	       /* get the values of status
					        * register */
    val1_lsw = temp_res & 0xF;		       /* get the decoded value */
    if (val1_msw != 0x0)
	return (-1);

    if (val1_lsw != 0x4)
	return (-1);

    *(u_long *) REGISTER_ONE_MSW = 0x40000000; /* 2 */
    *(u_long *) 0xE0000788 = 0x40400000;       /* reg1 compaare magnitude
					        * with operrand */
    temp_res = *(u_long *) (FPA_BASE + FPA_WSTATUSC);
    val1_msw = (temp_res & 0xF00) >> 8;	       /* get the values of status
					        * register */
    val1_lsw = temp_res & 0x1F;		       /* get the decoded value */
    if (val1_msw != 0x1)
	return (-1);

    if (val1_lsw != 0x19)
	return (-1);


    return (0);
}

w_op_short_doublep()
{
    u_long          temp_res;
    u_long          val1_msw, val1_lsw, val2_msw, val2_lsw;
    u_long          val3_msw, val3_lsw, val4_msw, val4_lsw;
    u_long          val5_msw, val5_lsw, val6_msw, val6_lsw;
    u_long          val7_msw, val7_lsw, val8_msw, val8_lsw;
    u_long          val9_msw, val9_lsw, val10_msw, val10_lsw;
    u_long          val11_msw, val11_lsw, val12_msw, val12_lsw;

    *(u_long *) REGISTER_SEVEN_MSW = 0x3FF00000;	/* 1 */
    *(u_long *) REGISTER_SEVEN_LSW = 0x0;
    *(u_long *) REGISTER_EIGHT_MSW = 0x40180000;	/* 6 */
    *(u_long *) REGISTER_EIGHT_LSW = 0x0;
    *(u_long *) REGISTER_NINE_MSW = 0x40080000;/* 3 */
    *(u_long *) REGISTER_NINE_LSW = 0x0;
    *(u_long *) REGISTER_TEN_MSW = 0x40220000; /* 9 */
    *(u_long *) REGISTER_TEN_LSW = 0x0;
    *(u_long *) REGISTER_ELEVEN_MSW = 0x40080000;	/* 3 */
    *(u_long *) REGISTER_ELEVEN_LSW = 0x0;
    *(u_long *) REGISTER_TWELVE_MSW = 0x40080000;	/* 3 */
    *(u_long *) REGISTER_TWELVE_LSW = 0x0;

    *(u_long *) 0xE000008C = 0x3FF00000;       /* 1 *//* reg1 <- negate
					        * operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE0000114 = 0x40080000;       /* 3 *//* reg2 <- absolute
					        * value operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE000019C = 0x4;	       /* 4 *//* reg3 <- float
					        * operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE0000224 = 0x40300000;       /* 16 *//* reg4 <- fix operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE00002AC = 0x40220000;       /* 9 *//* reg5 <- convert (dp
					        * to sp) operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE0000334 = 0x40100000;       /* 4 *//* reg6 <- square of
					        * operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE00003BC = 0x3FF00000;       /* 1 *//* reg7 <- reg7 +
					        * operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE0000444 = 0x40000000;       /* 2 *//* reg8 <- reg8 -
					        * operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE00004CC = 0x40000000;       /* 2 *//* reg9 <- reg9 *
					        * operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE0000554 = 0x40080000;       /* 3 *//* reg10 <- reg10 /
					        * operand */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE00005DC = 0x40220000;       /* 9 *//* reg11 <- operand -
					        * reg11 */
    *(u_long *) 0xE0001000 = 0x0;
    *(u_long *) 0xE0000664 = 0x40220000;       /* 9 *//* reg12 <- operand /
					        * reg12 */
    *(u_long *) 0xE0001000 = 0x0;


    val1_msw = *(u_long *) REGISTER_ONE_MSW;
    val1_lsw = *(u_long *) REGISTER_ONE_LSW;
    val2_msw = *(u_long *) REGISTER_TWO_MSW;
    val2_lsw = *(u_long *) REGISTER_TWO_LSW;
    val3_msw = *(u_long *) REGISTER_THREE_MSW;
    val3_lsw = *(u_long *) REGISTER_THREE_LSW;
    val4_msw = *(u_long *) REGISTER_FOUR_MSW;
    val4_lsw = *(u_long *) REGISTER_FOUR_LSW;
    val5_msw = *(u_long *) REGISTER_FIVE_MSW;
    val5_lsw = *(u_long *) REGISTER_FIVE_LSW;
    val6_msw = *(u_long *) REGISTER_SIX_MSW;
    val6_lsw = *(u_long *) REGISTER_SIX_LSW;
    val7_msw = *(u_long *) REGISTER_SEVEN_MSW;
    val7_lsw = *(u_long *) REGISTER_SEVEN_LSW;
    val8_msw = *(u_long *) REGISTER_EIGHT_MSW;
    val8_lsw = *(u_long *) REGISTER_EIGHT_LSW;
    val9_msw = *(u_long *) REGISTER_NINE_MSW;
    val9_lsw = *(u_long *) REGISTER_NINE_LSW;
    val10_msw = *(u_long *) REGISTER_TEN_MSW;
    val10_lsw = *(u_long *) REGISTER_TEN_LSW;
    val11_msw = *(u_long *) REGISTER_ELEVEN_MSW;
    val11_lsw = *(u_long *) REGISTER_ELEVEN_LSW;
    val12_msw = *(u_long *) REGISTER_TWELVE_MSW;
    val12_lsw = *(u_long *) REGISTER_TWELVE_LSW;

    if (val1_msw != 0xBFF00000)
	return (-1);

    if (val1_lsw != 0x00000000)
	return (-1);

    if (val2_msw != 0x40080000)
	return (-1);

    if (val2_lsw != 0x00000000)
	return (-1);

    if (val3_msw != 0x40100000)
	return (-1);

    if (val3_lsw != 0x00000000)
	return (-1);

    if (val4_msw != 0x10)
	return (-1);

    if (val4_lsw != 0x00000000)
	return (-1);

    if (val5_msw != 0x41100000)
	return (-1);

    if (val5_lsw != 0x00000000)
	return (-1);


    if (val6_msw != 0x40300000)
	return (-1);

    if (val6_lsw != 0x00000000)
	return (-1);

    if (val7_msw != 0x40000000)
	return (-1);

    if (val7_lsw != 0x00000000)
	return (-1);

    if (val8_msw != 0x40100000)
	return (-1);

    if (val8_lsw != 0x00000000)
	return (-1);

    if (val9_msw != 0x40180000)
	return (-1);

    if (val9_lsw != 0x00000000)
	return (-1);

    if (val10_msw != 0x40080000)
	return (-1);

    if (val10_lsw != 0x00000000)
	return (-1);

    if (val11_msw != 0x40180000)
	return (-1);

    if (val11_lsw != 0x00000000)
	return (-1);

    if (val12_msw != 0x40080000)
	return (-1);

    if (val12_lsw != 0x00000000)
	return (-1);


    *(u_long *) 0xE0000684 = 0x3FF00000;       /* operand compare with 0 */
    *(u_long *) 0xE0001000 = 0x0;

    temp_res = *(u_long *) (FPA_BASE + FPA_WSTATUSC);
    val1_msw = (temp_res & 0xF00) >> 8;	       /* get the values of status
					        * register */
    val1_lsw = temp_res & 0xF;		       /* get the decoded value */
    if (val1_msw != 0x2)
	return (-1);

    if (val1_lsw != 0x0)
	return (-1);



    *(u_long *) REGISTER_ONE_MSW = 0x3FF00000; /* 1 */
    *(u_long *) REGISTER_ONE_LSW = 0x0;
    *(u_long *) 0xE000070C = 0x3FF00000;       /* 1 *//* reg1 compare with
					        * operand */
    *(u_long *) 0xE0001000 = 0x0;
    temp_res = *(u_long *) (FPA_BASE + FPA_WSTATUSC);
    val1_msw = (temp_res & 0xF00) >> 8;	       /* get the values of status
					        * register */
    val1_lsw = temp_res & 0xF;		       /* get the decoded value */
    if (val1_msw != 0x0)
	return (-1);

    if (val1_lsw != 0x4)
	return (-1);


    *(u_long *) REGISTER_ONE_MSW = 0x40000000; /* 2 */
    *(u_long *) REGISTER_ONE_LSW = 0x0;
    *(u_long *) 0xE000078C = 0x40080000;       /* 3 *//* reg1 compaare
					        * magnitude with operrand */
    *(u_long *) 0xE0001000 = 0x0;
    temp_res = *(u_long *) (FPA_BASE + FPA_WSTATUSC);
    val1_msw = (temp_res & 0xF00) >> 8;	       /* get the values of status
					        * register */
    val1_lsw = temp_res & 0x1F;		       /* get the decoded value */
    if (val1_msw != 0x1)
	return (-1);

    if (val1_lsw != 0x19)
	return (-1);


    return (0);
}

/*
 * This test causes the Weitek chips to produce every possible value at the
 * S+ outputs.  These bits are then observed via the WSTATUS register.  The
 * ALU and multiplier are distinguished by the instruction address 
 *
 */
fpa_ws()
{
    u_long          i, cmp_status, tmp_var;
    u_long         *ptr, *soft_clear, *wstatus;
    u_long          amsw, alsw, bmsw, blsw;

    /* Test both single and double precision */



    soft_clear = (u_long *) FPA_CLEAR_PIPE_PTR;
    /* stable only, do not read others get bus error */
    wstatus = (u_long *) (FPA_BASE + FPA_WSTATUSS);

    /* clear the hard pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;

    /* initialize by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    for (i = 0; test_ws[i].instr != 0; i++) {

	amsw = test_ws[i].a_msw;	       /* for the sake of assembler */
	alsw = test_ws[i].a_lsw;
	bmsw = test_ws[i].b_msw;
	blsw = test_ws[i].b_lsw;

	*(u_long *) REGISTER_ZERO_MSW = amsw;  /* reg0 = operand A */
	*(u_long *) REGISTER_ZERO_LSW = alsw;
	*(u_long *) REGISTER_ONE_MSW = bmsw;   /* reg1 = operand B */
	*(u_long *) REGISTER_ONE_LSW = blsw;

	tmp_var = test_ws[i].instr;	       /* for the sake of assembler */
	ptr = (u_long *) tmp_var;
	*ptr = 0x10002;			       /* reg2 = reg0 op reg1 */
	cmp_status = (*wstatus >> 8) & 0xf;
	if (cmp_status != test_ws[i].status)
	    return (-1);

	*soft_clear = 0;
	/* initialize by giving the diagnostic initialize command */
	*(u_long *) DIAG_INIT_CMD = 0x0;
	*(u_long *) MODE_WRITE_REGISTER = 0x2;

    }
    return (0);
}

/* 
 * state_reg = 0; for (i=0; i<1000; i++) 
 *
 * command register instructions - all operations DP set registers to the
 * following values: 1 = 4 2 = 8 3 = 16 4 = 32     5 = 0 6 = 0 7 = 0 8 = 0 
 *
 * send instructions to do the following: reg8 = (e**reg4) - 1      reg7 =
 * (e**reg3) - 1      reg6 = (e**reg2) - 1      reg5 = (e**reg1) - 1      
 *
 * read following registers in order specified: reg5 reg6 reg7 reg8 
 *
 * verify that registers read contain correct value 
 *
 *
 * extended - do all with DP repeat above process, but use pivot operations
 * instead of (e**x)-1 use reg1 - reg8 as operands and reg9 - reg12 for
 * results 
 *
 * DP short repeat process with square to test MULT and div to test ALU 
 *
 * SP short repeat process with square to test MULT and div to test ALU */

timing()
{

    u_long          i, val1_msw, val1_lsw, val2_msw, val2_lsw, val3_msw, val3_lsw;
    u_long          val4_msw, val4_lsw;
    u_long         *expo;



    /* clear the pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;
    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    expo = (u_long *) 0xE0000824;	       /* address for exponential
					        * operation */
    /* set the imask register = 0 */
    *(u_long *) FPA_IMASK_PTR = 0x0;

    for (i = 0; i < 1000; i++) {
	*(u_long *) REGISTER_ONE_MSW = 0x40100000;	/* d1ecimal 4 */
	*(u_long *) REGISTER_ONE_LSW = 0x00000000;
	*(u_long *) REGISTER_TWO_MSW = 0x40200000;	/* decimal 8 */
	*(u_long *) REGISTER_TWO_LSW = 0x00000000;
	*(u_long *) REGISTER_THREE_MSW = 0x40300000;	/* decimal 16 */
	*(u_long *) REGISTER_THREE_LSW = 0x00000000;
	*(u_long *) REGISTER_FOUR_MSW = 0x40400000;	/* decimal 32 */
	*(u_long *) REGISTER_FOUR_LSW = 0x00000000;
	*(u_long *) REGISTER_FIVE_MSW = 0x0000;
	*(u_long *) REGISTER_FIVE_LSW = 0x0000;
	*(u_long *) REGISTER_SIX_MSW = 0x0000;
	*(u_long *) REGISTER_SIX_LSW = 0x0000;
	*(u_long *) REGISTER_SEVEN_MSW = 0x0000;
	*(u_long *) REGISTER_SEVEN_LSW = 0x0000;
	*(u_long *) REGISTER_EIGHT_MSW = 0x0000;
	*(u_long *) REGISTER_EIGHT_LSW = 0x0000;
	*expo = 0x108;			       /* reg8 = reg4 op */
	*expo = 0xC7;			       /* reg7 = reg3 op */
	*expo = 0x86;			       /* reg6 = reg2 op */
	*expo = 0x45;			       /* reg5 = reg1 op */

	val4_msw = *(u_long *) REGISTER_FIVE_MSW;
	val4_lsw = *(u_long *) REGISTER_FIVE_LSW;
	val3_msw = *(u_long *) REGISTER_SIX_MSW;
	val3_lsw = *(u_long *) REGISTER_SIX_LSW;
	val2_msw = *(u_long *) REGISTER_SEVEN_MSW;
	val2_lsw = *(u_long *) REGISTER_SEVEN_LSW;
	val1_msw = *(u_long *) REGISTER_EIGHT_MSW;
	val1_lsw = *(u_long *) REGISTER_EIGHT_LSW;

	if (val1_msw != 0x42D1F43F)
	    return (-1);

	if (val1_lsw != 0xCC4B65EC)
	    return (-1);
	if (val2_msw != 0x4160F2EB)
	    return (-1);

	if (val2_lsw != 0xB0A80020)
	    return (-1);

	if (val3_msw != 0x40A747EA)
	    return (-1);

	if (val3_lsw != 0x7D470C6E)
	    return (-1);

	if (val4_msw != 0x404ACC90)
	    return (-1);
	if (val4_lsw != 0x2E273A58)
	    return (-1);

	/* extended instructions */
	*(u_long *) REGISTER_TWO_MSW = 0x40100000;
	*(u_long *) REGISTER_TWO_LSW = 0x00000000;	/* decimal 4 */
	*(u_long *) REGISTER_THREE_MSW = 0x40000000;
	*(u_long *) REGISTER_THREE_LSW = 0x00000000;	/* decimal 2 */

	*(u_long *) REGISTER_SIX_MSW = 0x40080000;
	*(u_long *) REGISTER_SIX_LSW = 0x00000000;	/* decimal 3 */
	*(u_long *) REGISTER_FIVE_MSW = 0x40300000;
	*(u_long *) REGISTER_FIVE_LSW = 0x00000000;	/* decimal 16 */

	*(u_long *) REGISTER_NINE_MSW = 0x40180000;
	*(u_long *) REGISTER_NINE_LSW = 0x00000000;	/* decimal 6 */
	*(u_long *) REGISTER_EIGHT_MSW = 0x40000000;
	*(u_long *) REGISTER_EIGHT_LSW = 0x00000000;	/* decimal 2 */

	*(u_long *) REGISTER_TWELVE_MSW = 0x40080000;
	*(u_long *) REGISTER_TWELVE_LSW = 0x00000000;	/* decimal 3 */
	*(u_long *) REGISTER_ELEVEN_MSW = 0x40000000;
	*(u_long *) REGISTER_ELEVEN_LSW = 0x00000000;	/* decimal 2 */
	*(u_long *) 0xE0001814 = 0x40000000;
	*(u_long *) 0xE0001988 = 0x00000000;
	*(u_long *) 0xE00018B4 = 0x40000000;
	*(u_long *) 0xE0001AA0 = 0x00000000;
	*(u_long *) 0xE000194C = 0x40000000;
	*(u_long *) 0xE0001C38 = 0x00000000;
	*(u_long *) 0xE00019E4 = 0x40000000;
	*(u_long *) 0xE0001DD0 = 0x00000000;

	val1_msw = *(u_long *) REGISTER_ONE_MSW;
	val1_lsw = *(u_long *) REGISTER_ONE_LSW;
	val2_msw = *(u_long *) REGISTER_FOUR_MSW;
	val2_lsw = *(u_long *) REGISTER_FOUR_LSW;
	val3_msw = *(u_long *) REGISTER_SEVEN_MSW;
	val3_lsw = *(u_long *) REGISTER_SEVEN_LSW;
	val4_msw = *(u_long *) REGISTER_TEN_MSW;
	val4_lsw = *(u_long *) REGISTER_TEN_LSW;

	if (val1_msw != 0x40240000)
	    return (-1);

	if (val1_lsw != 0x00000000)
	    return (-1);
	if (val2_msw != 0x40240000)
	    return (-1);
	if (val2_lsw != 0x00000000)
	    return (-1);
	if (val3_msw != 0x40240000)
	    return (-1);
	if (val3_lsw != 0x00000000)
	    return (-1);
	if (val4_msw != 0x40240000)
	    return (-1);
	if (val4_lsw != 0x00000000)
	    return (-1);

	*(u_long *) REGISTER_TWO_MSW = 0x40100000;
	*(u_long *) REGISTER_TWO_LSW = 0x00000000;	/* decimal 4 */
	*(u_long *) REGISTER_THREE_MSW = 0x40140000;
	*(u_long *) REGISTER_THREE_LSW = 0x00000000;	/* decimal 5 */

	*(u_long *) REGISTER_SIX_MSW = 0x40000000;
	*(u_long *) REGISTER_SIX_LSW = 0x00000000;	/* decimal 2 */
	*(u_long *) REGISTER_FIVE_MSW = 0x40000000;
	*(u_long *) REGISTER_FIVE_LSW = 0x00000000;	/* decimal 2 */

	*(u_long *) REGISTER_NINE_MSW = 0x40000000;
	*(u_long *) REGISTER_NINE_LSW = 0x00000000;	/* decimal 2 */
	*(u_long *) REGISTER_EIGHT_MSW = 0x40080000;
	*(u_long *) REGISTER_EIGHT_LSW = 0x00000000;	/* decimal 3 */

	*(u_long *) REGISTER_TWELVE_MSW = 0x40000000;
	*(u_long *) REGISTER_TWELVE_LSW = 0x00000000;	/* decimal 2 */
	*(u_long *) REGISTER_ELEVEN_MSW = 0x40080000;
	*(u_long *) REGISTER_ELEVEN_LSW = 0x00000000;	/* decimal 3 */
	*(u_long *) 0xE0001A14 = 0x40000000;
	*(u_long *) 0xE0001988 = 0x00000000;
	*(u_long *) 0xE0001AB4 = 0x401C0000;
	*(u_long *) 0xE0001AA0 = 0x00000000;
	*(u_long *) 0xE0001B4C = 0x40100000;
	*(u_long *) 0xE0001C38 = 0x00000000;
	*(u_long *) 0xE0001BE4 = 0x40300000;
	*(u_long *) 0xE0001DD0 = 0x00000000;

	val1_msw = *(u_long *) REGISTER_ONE_MSW;
	val1_lsw = *(u_long *) REGISTER_ONE_LSW;
	val2_msw = *(u_long *) REGISTER_FOUR_MSW;
	val2_lsw = *(u_long *) REGISTER_FOUR_LSW;
	val3_msw = *(u_long *) REGISTER_SEVEN_MSW;
	val3_lsw = *(u_long *) REGISTER_SEVEN_LSW;
	val4_msw = *(u_long *) REGISTER_TEN_MSW;
	val4_lsw = *(u_long *) REGISTER_TEN_LSW;

	if (val1_msw != 0x40240000)
	    return (-1);
	if (val1_lsw != 0x00000000)
	    return (-1);
	if (val2_msw != 0x40240000)
	    return (-1);
	if (val2_lsw != 0x00000000)
	    return (-1);
	if (val3_msw != 0x40240000)
	    return (-1);
	if (val3_lsw != 0x00000000)
	    return (-1);
	if (val4_msw != 0x40240000)
	    return (-1);
	if (val4_lsw != 0x00000000)
	    return (-1);

	*(u_long *) REGISTER_TWO_MSW = 0x40000000;
	*(u_long *) REGISTER_TWO_LSW = 0x00000000;	/* decimal 2 */
	*(u_long *) REGISTER_THREE_MSW = 0x40180000;
	*(u_long *) REGISTER_THREE_LSW = 0x00000000;	/* decimal 6 */

	*(u_long *) REGISTER_SIX_MSW = 0x3FF00000;
	*(u_long *) REGISTER_SIX_LSW = 0x00000000;	/* decimal 1 */
	*(u_long *) REGISTER_FIVE_MSW = 0x40100000;
	*(u_long *) REGISTER_FIVE_LSW = 0x00000000;	/* decimal 4 */

	*(u_long *) REGISTER_NINE_MSW = 0x3FF00000;
	*(u_long *) REGISTER_NINE_LSW = 0x00000000;	/* decimal 1 */
	*(u_long *) REGISTER_EIGHT_MSW = 0x40180000;
	*(u_long *) REGISTER_EIGHT_LSW = 0x00000000;	/* decimal 6 */
	*(u_long *) 0xE0001C14 = 0x40000000;
	*(u_long *) 0xE0001988 = 0x00000000;
	*(u_long *) 0xE0001CB4 = 0x40000000;
	*(u_long *) 0xE0001AA0 = 0x00000000;
	*(u_long *) 0xE0001D4C = 0x40000000;
	*(u_long *) 0xE0001C38 = 0x00000000;

	val1_msw = *(u_long *) REGISTER_ONE_MSW;
	val1_lsw = *(u_long *) REGISTER_ONE_LSW;
	val2_msw = *(u_long *) REGISTER_FOUR_MSW;
	val2_lsw = *(u_long *) REGISTER_FOUR_LSW;
	val3_msw = *(u_long *) REGISTER_SEVEN_MSW;
	val3_lsw = *(u_long *) REGISTER_SEVEN_LSW;

	if (val1_msw != 0x40240000)
	    return (-1);
	if (val1_lsw != 0x00000000)
	    return (-1);
	if (val2_msw != 0x40240000)
	    return (-1);
	if (val2_lsw != 0x00000000)
	    return (-1);
	if (val3_msw != 0x40240000)
	    return (-1);
	if (val3_lsw != 0x00000000)
	    return (-1);

	/* for DP short  square */
	*(u_long *) REGISTER_ONE_MSW = 0x0;
	*(u_long *) REGISTER_ONE_LSW = 0x0;
	*(u_long *) REGISTER_TWO_MSW = 0x0;
	*(u_long *) REGISTER_TWO_LSW = 0x0;
	*(u_long *) REGISTER_THREE_MSW = 0x0;
	*(u_long *) REGISTER_THREE_LSW = 0x0;
	*(u_long *) REGISTER_FOUR_MSW = 0x0;
	*(u_long *) REGISTER_FOUR_LSW = 0x0;
	*(u_long *) 0xE000030C = 0x40000000;   /* reg1 = square of (operand) */
	*(u_long *) 0xE0001000 = 0x00000000;   /* square  for decimal 2 */
	*(u_long *) 0xE0000314 = 0x40080000;   /* reg2 */
	*(u_long *) 0xE0001000 = 0x00000000;   /* square  for decimal 3 */
	*(u_long *) 0xE000031C = 0x40100000;   /* reg3 */
	*(u_long *) 0xE0001000 = 0x00000000;   /* square  for decimal 4 */
	*(u_long *) 0xE0000324 = 0x3FF00000;   /* reg4 */
	*(u_long *) 0xE0001000 = 0x00000000;   /* square  for decimal 1 */

	val1_msw = *(u_long *) REGISTER_ONE_MSW;
	val1_lsw = *(u_long *) REGISTER_ONE_LSW;
	val2_msw = *(u_long *) REGISTER_TWO_MSW;
	val2_lsw = *(u_long *) REGISTER_TWO_LSW;

	val3_msw = *(u_long *) REGISTER_THREE_MSW;
	val3_lsw = *(u_long *) REGISTER_THREE_LSW;
	val4_msw = *(u_long *) REGISTER_FOUR_MSW;
	val4_lsw = *(u_long *) REGISTER_FOUR_LSW;

	if (val1_msw != 0x40100000)
	    return (-1);
	if (val1_lsw != 0x00000000)
	    return (-1);
	if (val2_msw != 0x40220000)
	    return (-1);
	if (val2_lsw != 0x00000000)
	    return (-1);
	if (val3_msw != 0x40300000)
	    return (-1);
	if (val3_lsw != 0x00000000)
	    return (-1);
	if (val4_msw != 0x3FF00000)
	    return (-1);
	if (val4_lsw != 0x00000000)
	    return (-1);

	*(u_long *) REGISTER_ONE_MSW = 0x40180000;	/* 6 */
	*(u_long *) REGISTER_ONE_LSW = 0x0;
	*(u_long *) REGISTER_TWO_MSW = 0x40180000;	/* 6 */
	*(u_long *) REGISTER_TWO_LSW = 0x0;
	*(u_long *) REGISTER_THREE_MSW = 0x40200000;	/* 8 */
	*(u_long *) REGISTER_THREE_LSW = 0x0;
	*(u_long *) REGISTER_FOUR_MSW = 0x40240000;	/* 10 */
	*(u_long *) REGISTER_FOUR_LSW = 0x0;
	*(u_long *) 0xE000050C = 0x40080000;   /* reg1 */
	*(u_long *) 0xE0001000 = 0x00000000;   /* reg1 = reg1 /operand(3) */
	*(u_long *) 0xE0000514 = 0x40000000;   /* reg2 = reg2 / operand */
	*(u_long *) 0xE0001000 = 0x00000000;   /* divide for op = 2 */
	*(u_long *) 0xE000051C = 0x40100000;   /* reg3 */
	*(u_long *) 0xE0001000 = 0x00000000;   /* divide for op = 4 */
	*(u_long *) 0xE0000524 = 0x40000000;   /* reg4 */
	*(u_long *) 0xE0001000 = 0x00000000;   /* divide for op = 2 */

	val1_msw = *(u_long *) REGISTER_ONE_MSW;
	val1_lsw = *(u_long *) REGISTER_ONE_LSW;
	val2_msw = *(u_long *) REGISTER_TWO_MSW;
	val2_lsw = *(u_long *) REGISTER_TWO_LSW;

	val3_msw = *(u_long *) REGISTER_THREE_MSW;
	val3_lsw = *(u_long *) REGISTER_THREE_LSW;
	val4_msw = *(u_long *) REGISTER_FOUR_MSW;
	val4_lsw = *(u_long *) REGISTER_FOUR_LSW;

	if (val1_msw != 0x40000000)
	    return (-1);
	if (val1_lsw != 0x00000000)
	    return (-1);
	if (val2_msw != 0x40080000)
	    return (-1);
	if (val2_lsw != 0x00000000)
	    return (-1);
	if (val3_msw != 0x40000000)
	    return (-1);
	if (val3_lsw != 0x00000000)
	    return (-1);
	if (val4_msw != 0x40140000)
	    return (-1);
	if (val4_lsw != 0x00000000)
	    return (-1);

	/* single precision short */
	*(u_long *) REGISTER_ONE_MSW = 0x00000000;	/* 1 */
	*(u_long *) REGISTER_TWO_MSW = 0x00000000;	/* 2 */
	*(u_long *) REGISTER_THREE_MSW = 0x00000000;	/* 3 */
	*(u_long *) REGISTER_FOUR_MSW = 0x00000000;	/* 4 */
	*(u_long *) 0xE0000308 = 0x3F800000;   /* reg1 = square of 1 */
	*(u_long *) 0xE0000310 = 0x40000000;   /* reg2 = square of 2 */
	*(u_long *) 0xE0000318 = 0x40400000;   /* reg3 = square of 3 */
	*(u_long *) 0xE0000320 = 0x40800000;   /* reg4 = square of 4 */

	val1_msw = *(u_long *) REGISTER_ONE_MSW;
	val2_msw = *(u_long *) REGISTER_TWO_MSW;
	val3_msw = *(u_long *) REGISTER_THREE_MSW;
	val4_msw = *(u_long *) REGISTER_FOUR_MSW;

	if (val1_msw != 0x3F800000)
	    return (-1);
	if (val2_msw != 0x40800000)
	    return (-1);
	if (val3_msw != 0x41100000)
	    return (-1);
	if (val4_msw != 0x41800000)
	    return (-1);

	*(u_long *) REGISTER_ONE_MSW = 0x40C00000;	/* 6 */
	*(u_long *) REGISTER_TWO_MSW = 0x40C00000;	/* 6 */
	*(u_long *) REGISTER_THREE_MSW = 0x41000000;	/* 8 */
	*(u_long *) REGISTER_FOUR_MSW = 0x41800000;	/* 16 */
	*(u_long *) 0xE0000508 = 0x40400000;   /* reg1 = reg1 div 3 */
	*(u_long *) 0xE0000510 = 0x40000000;   /* reg2 = reg2 div 2 */
	*(u_long *) 0xE0000518 = 0x40800000;   /* reg3 = reg3 div 4 */
	*(u_long *) 0xE0000520 = 0x40800000;   /* reg4 = reg4 div 4 */

	val1_msw = *(u_long *) REGISTER_ONE_MSW;
	val2_msw = *(u_long *) REGISTER_TWO_MSW;
	val3_msw = *(u_long *) REGISTER_THREE_MSW;
	val4_msw = *(u_long *) REGISTER_FOUR_MSW;

	if (val1_msw != 0x40000000)
	    return (-1);
	if (val2_msw != 0x40400000)
	    return (-1);
	if (val3_msw != 0x40000000)
	    return (-1);
	if (val4_msw != 0x40800000)
	    return (-1);
    }
    /* clear the pipe */
    *(u_long *) FPA_CLEAR_PIPE_PTR = 0x0;
    return (0);
}
void 
sigsegv_handler()
{
    int            *ptr4;

    ptr4 = (int *) 0xE0000F1C;
    *ptr4 = 0;
    ptr4 = (int *) 0xE0000F84;
    *ptr4 = 0;

    printf("test number = %d, ierr register = %x, pipe status = %x\n",
	   temp_test_number, *(u_long *) FPA_IERR_PTR,
	   *(u_long *) (FPA_BASE + FPA_STABLE_PIPE_STATUS));
    printf("Act_inst = %x, Nxt_ins = %x, Act_d1 = %x\n",
	   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_INS),
	   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_INS),
	   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_D1));
    printf("Act_d2 = %x, Nxt_d1 = %x, Nxt_d2 = %x\n",
	   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_D2),
	   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_D1),
	   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_D2));
    send_message(-SYSTEST_ERROR, FATAL,
	"Sun FPA SysTest Failed due to System Segment Violation error.");
}

void 
sigfpe_handler(sig, code, scp)
    int             sig, code;
    struct sigcontext *scp;

{
    char           *ptr1, *ptr2, *ptr3;
    FILE           *input_file, *fopen();
    int            *ptr4, *ptr5;

    ptr4 = (int *) 0xE0000F84;		       /* soft clear pipe */
    ptr5 = (int *) 0xE0000F1C;		       /* ierr register to check for
					        * nack test */

    if ((seg_sig_flag) && (code == FPE_FPA_ERROR)) {

	*ptr4 = 0x0;
	if (((*ptr5 >> 16) & 0xff) != 0x20) {
	    printf("test number = %d, ierr register = %x, pipe status = %x\n",
		   temp_test_number, *(u_long *) FPA_IERR_PTR,
		   *(u_long *) (FPA_BASE + FPA_STABLE_PIPE_STATUS));
	    printf("Act_inst = %x, Nxt_ins = %x, Act_d1 = %x\n",
		   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_INS),
		   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_INS),
		   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_D1));
	    printf("Act_d2 = %x, Nxt_d1 = %x, Nxt_d2 = %x\n",
		   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_D2),
		   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_D1),
		   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_D2));

	    send_message(-SYSTEST_ERROR, FATAL, 
	"Sun FPA Reliability Test Failed. Nack Test: Pipe is not hung.");
	} else {
	    *ptr5 = 0x0;		       /* clear the ierr register */
	    return;
	}
    } else if (code == FPE_FPA_ERROR) {
	if (sig_err_flag) {
	    if (fpa_handler(sig, code, scp))   /* 4.0 change */
		return;
	} else {
	    printf("test number = %d, ierr register = %x, pipe status = %x\n",
		   temp_test_number, *(u_long *) FPA_IERR_PTR,
		   *(u_long *) (FPA_BASE + FPA_STABLE_PIPE_STATUS));
	    printf("Act_inst = %x, Nxt_ins = %x, Act_d1 = %x\n",
		   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_INS),
		   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_INS),
		   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_D1));
	    printf("Act_d2 = %x, Nxt_d1 = %x, Nxt_d2 = %x\n",
		   *(u_long *) (FPA_BASE + FPA_PIPE_ACT_D2),
		   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_D1),
		   *(u_long *) (FPA_BASE + FPA_PIPE_NXT_D2));

	    printf(" Sun FPA Reliability Test Failed due to bus error : FPA_ERROR");
	}
    } else {
	printf("test number = %d, ierr register = %x, pipe status = %x\n",
	       temp_test_number, *(u_long *) FPA_IERR_PTR,
	       *(u_long *) (FPA_BASE + FPA_STABLE_PIPE_STATUS));
	printf("Act_inst = %x, Nxt_ins = %x, Act_d1 = %x\n",
	       *(u_long *) (FPA_BASE + FPA_PIPE_ACT_INS),
	       *(u_long *) (FPA_BASE + FPA_PIPE_NXT_INS),
	       *(u_long *) (FPA_BASE + FPA_PIPE_ACT_D1));
	printf("Act_d2 = %x, Nxt_d1 = %x, Nxt_d2 = %x\n",
	       *(u_long *) (FPA_BASE + FPA_PIPE_ACT_D2),
	       *(u_long *) (FPA_BASE + FPA_PIPE_NXT_D1),
	       *(u_long *) (FPA_BASE + FPA_PIPE_NXT_D2));

	printf(" Sun FPA Reliability Test Failed due to bus error : ");
	switch (code) {
	case FPE_INTDIV_TRAP:
	    strcat(ptr1, "INTDIV_TRAP");
	    break;
	case FPE_CHKINST_TRAP:
	    strcat(ptr1, "CHKINST_TRAP");
	    break;
	case FPE_TRAPV_TRAP:
	    strcat(ptr1, "TRAPV_TRAP");
	    break;
	case FPE_FLTBSUN_TRAP:
	    strcat(ptr1, "FLTBSUN_TRAP");
	    break;
	case FPE_FLTINEX_TRAP:
	    strcat(ptr1, "FLTINEX_TRAP");
	    break;
	case FPE_FLTDIV_TRAP:
	    strcat(ptr1, "FLTDIV_TRAP");
	    break;
	case FPE_FLTUND_TRAP:
	    strcat(ptr1, "FLTUND_TRAP");
	    break;
	case FPE_FLTOPERR_TRAP:
	    strcat(ptr1, "FLTOPERR_TRAP");
	    break;
	case FPE_FLTOVF_TRAP:
	    strcat(ptr1, "FLTOVF_TRAP");
	    break;
	case FPE_FLTNAN_TRAP:
	    strcat(ptr1, "FLTNAN_TRAP");
	    break;
	case FPE_FPA_ENABLE:
	    strcat(ptr1, "FPA_ENABLE");
	    break;
	case FPE_FPA_ERROR:
	    strcat(ptr1, "FPA_ERROR");
	    break;
	default:
	    strcat(ptr1, "Other than fpa error code");
	}
    }
    strcat(ptr1, ".\n");
    exit(1);
}



int 
winitfp_()
/*
 * Procedure to determine if a physical FPA and 68881 are present and set
 * fp_state_sunfpa and fp_state_mc68881 accordingly. Also returns 1 if both
 * present, 0 otherwise. 
 */

{
    int             openfpa, mode81;
    long           *fpaptr;

    if (fp_state_sunfpa == fp_unknown) {
	if (minitfp_() != 1)
	    fp_state_sunfpa = fp_absent;
	else {
	    openfpa = open("/dev/fpa", O_RDWR);
	    open_fpa = openfpa;
	    if ((openfpa < 0) && (errno != EEXIST)) {	/* openfpa < 0 */
		if (errno == EBUSY) {
		    fprintf(stderr, "\n No Sun FPA contexts available - all in use\n");
		    fflush(stderr);
		}
		fp_state_sunfpa = fp_absent;
	    }
	     /* openfpa < 0 */ 
	    else {			       /* openfpa >= 0 */
		if (errno == EEXIST) {
		    fprintf(stderr, "\n  FPA was already open \n");
		    fflush(stderr);
		}
		/* to close FPA context on execve() */
		fcntl(openfpa, F_SETFD, 1);
		restore_svmask = oldfpe.sv_mask;
		restore_svonstack = oldfpe.sv_onstack;
		restore_svhandler = (func_ptr)oldfpe.sv_handler;
		newfpe.sv_mask = 0;
		newfpe.sv_onstack = 0;
		(func_ptr)newfpe.sv_handler = sigfpe_handler;
		sigvec(SIGFPE, &newfpe, &oldfpe);
		res_seg_svmask = oldseg.sv_mask;
		res_seg_svonstack = oldseg.sv_onstack;
		res_seg_svhandler = (func_ptr)oldseg.sv_handler;
		newseg.sv_mask = 0;
		newseg.sv_onstack = 0;
		(func_ptr)newseg.sv_handler = sigsegv_handler;
		sigvec(SIGSEGV, &newseg, &oldseg);
		fpaptr = (int *) 0xe00008d0;   /* write mode register */
		*fpaptr = 2;		       /* set to round integers
					        * toward zero */
		fpaptr = (int *) 0xe0000f14;
		*fpaptr = 1;		       /* set imask to one */
		fp_state_sunfpa = fp_enabled;
		/*
		 * if (!MA93N()) { fprintf(stderr,"\n Warning! Sun FPA works
		 * best with 68881 mask A93N \n") ; fflush(stderr) ; } 
		 */
	    }				       /* openfpa >= 0 */
	}
    }
    if (fp_state_sunfpa == fp_enabled)
	fp_switch = fp_sunfpa;
    return ((fp_state_sunfpa == fp_enabled) ? 1 : 0);
}

restore_signals()
{

    oldseg.sv_mask = res_seg_svmask;
    oldseg.sv_onstack = res_seg_svonstack;
    (func_ptr)oldseg.sv_handler = res_seg_svhandler;
    sigvec(SIGSEGV, &oldseg, &newseg);

    oldfpe.sv_mask = restore_svmask;
    oldfpe.sv_onstack = restore_svonstack;
    (func_ptr)oldfpe.sv_handler = restore_svhandler;
    sigvec(SIGFPE, &oldfpe, &newfpe);
}

nack2_test()
{
    /*
     * send an unimplemented instruction reg0 -> 0; check for bus error,
     * check ierr = 0x20 read any user register, check for bus error, check
     * ierr = 0x20; write another instruction to fpa, read wstatus reg.(clear
     * pipe) write the imask register and check the bus error 
     */
    u_long         *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7;
    int             value;


    /* Initialize  by giving the diagnostic initialize command */
    *(u_long *) DIAG_INIT_CMD = 0x0;
    *(u_long *) MODE_WRITE_REGISTER = 0x2;

    *(u_long *) REGISTER_ZERO_MSW = 0x0;
    *(u_long *) REGISTER_ZERO_LSW = 0x0;

    ptr1 = (u_long *) 0xE000096C;	       /* this is the unimplemented
					        * instruction */
    ptr2 = (u_long *) FPA_IERR_PTR;	       /* point to ierr register */
    ptr4 = (u_long *) 0xE0000824;	       /* address for exponential
					        * operation */
    ptr5 = (u_long *) 0xE0000824;	       /* address for exponential
					        * operation */
    ptr6 = (u_long *) (FPA_BASE + FPA_WSTATUSC);
    ptr7 = (u_long *) (FPA_BASE + FPA_IMASK);



    /* first the shadow register read */

    *ptr1 = 0x0;			       /* register zero */

    value = *(u_long *) 0xE0000E00;	       /* read the zero shadow ram */

    /*
     * the checking of ierr register and clearing the ierr register is done
     * in the signal handler 
     */
    /* Pipe line read */

    /* try to read any user register */


    /* try to read any user register */
    *ptr1 = 0x0;			       /* register zero */

    value = *(u_long *) REGISTER_ONE_MSW;
    /*
     * the checking of ierr register and clearing the ierr register is done
     * in the signal handler  
     */

    /* pipe line write */

    /* send two short instructions */
    *ptr1 = 0x0;			       /* register zero */
    *ptr4 = 0x108;

    *ptr5 = 0xC7;			       /* send third instruction,
					        * reg7 = reg3 op */
    /*
     * the checking of ierr register and clearing the ierr register is done
     * in the signal handler  
     */

    /* read the control register */
    *ptr1 = 0x0;			       /* register zero */
    value = *ptr6;			       /* read the wstatus register */

    /*
     * the checking of ierr register and clearing the ierr register is done
     * in the signal handler  
     */
    /* write the control register */
    *ptr7 = 0x0;			       /* write to imask register */
    /*
     * the checking of ierr register and clearing the ierr register is done
     * in the signal handler  
     */
    return (0);
}
