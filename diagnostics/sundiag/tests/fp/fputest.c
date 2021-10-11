/*
 *static char     frefsccsid[] = "@(#)fputest.c 1.1 7/30/92 Copyright Sun Microsystems";
 */
#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#ifdef SVR4
#include <siginfo.h>
#include <ucontext.h>
#include <sys/machsig.h>
#endif SVR4
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "sdrtns.h"

#define SYSTEST_ERROR		6

/* ================ Note on BDS Modifications Made on 7/12/88 ================
 *
 * The second level testing of "forced exceptions" is disabled in this version
 * for the following reasons:
 *
 * From dock@trinity Thu Jul  7 10:47:46 1988
 * To: bskreen@bienhoa
 * Subject: Re:  SunOS differences related to fpurel and fputest (sysdiag)
 * Cc: dgh@trinity
 *
 * The ftt bits should not be tested in a user level program diagnostic.
 * The kernel in 4.0 clears the bits and in 3.2 it left them as they were.
 * A stand alone diagnostic that runs in system mode is the appropriate
 * place for this check.  I suggest deleting the check of the ftt.
 * -dock
 *
 *  The ftt testing (confirming the reason for the exception) can be enabled
 *  on SunOS 3.2 in the future <highly recommended>. BDS
 *
 */



/*
 * 	main.c
 */
char *err_msg[] = {
	"",       
        "FSR Register Test",
        "Registers Test",
        "Nack Test",
        "Move Registers Test",
        "Negate: Positive to Negative Test",
        "Negate: Negative to Positive Test",
        "Absolute Test",
        "Integer to Floating Point: Single Precision Test",
        "Integer to Floating Point: Double Precision Test",
        "Floating to Integer: Single Precision Test",
        "Floating to Integer: Double Precision Test",
        "Single to Integer Round Toward Zero Test",
        "Double to Integer Round Toward Zero Test",
        "Format Conversion: Single to Double Test",
        "Format Conversion: Double to Single Test",
        "Addition: Single Precision Test",
        "Addition: Double Precision Test",
        "Subtraction: Single Precision Test",
        "Subtraction: Double Precision Test",
        "Multiply: Single Precision Test",
        "Multiply: Double Precision Test",
        "Division: Single Precision Test",
        "Division: Double Precision Test",
        "Compare: Single Precision Test",
        "Compare: Double Precision Test",
        "Compare And Exception If Unordered: Single Precision Test",
        "Compare And Exception If Unordered: Double Precision Test",
        "Branching On Condition Instructions Test",
        "No Branching on Condition Instructions Test",
        "Chaining (Single Precision) Test",
        "Chaining (Double Precision) Test",
        "FPP Status Test",
        "FPP Status (NXM=1) Test",
        "Lock Test",
        "Datapath (Single Precision) Test",
        "Datapath (Double Precision) Test",
        "Load Test",
        "Linpack Test"
}; 



int     sig_err_flag;
u_long  contexts; /* to get the context in case it fails */
int	donot_dq;
unsigned long error_ok;
unsigned long   fsr_at_trap;
unsigned long   trap_flag;
#ifdef SVR4
sigset_t     restore_samask;
int     restore_saonstack;
void    (*restore_sahandler)();
sigset_t     res_seg_samask;
int     res_seg_saonstack;
void    (*res_seg_sahandler)();
void    sigsegv_handler(int, siginfo_t *, ucontext_t *);
void    sigfpe_handler(int, siginfo_t *, ucontext_t *);
struct sigaction newfpu, oldfpu, oldseg, newseg;
#else SVR4
int     restore_svmask;
int     restore_svonstack;
void	(*restore_svhandler)();
int     res_seg_svmask;
int     res_seg_svonstack;
void	(*res_seg_svhandler)();
struct sigvec newfpu, oldfpu, oldseg, newseg;
#endif SVR4

extern 	int 	errno ;
extern  int	check_fpu();
extern	int	debug;
extern	int	simulate_error;
extern	char	*msg;

/*
 *	wstatus.c
 */
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
#define    maxm_sp   0x7eaaaa00
#define    maxm_msw  0x7fd55555
#define    maxm_lsw  0x55554000


#define    nn_sp     0x7f800400
#define    nn_msw    0x7ff00080
#define    nn_lsw    0x00000000
#define    nocare    0

#define    add_sp_l  1
#define    add_dp_l  2
#define    div_sp_l  3
#define    div_dp_l  4
#define    mult_sp_l 5
#define    mult_dp_l 6

struct testws {

     unsigned long   a_msw;
     unsigned long   a_lsw;
     unsigned long   b_msw;
     unsigned long   b_lsw;
     unsigned long   instr;
     unsigned long   status;
};

 
struct testws test_ws[] = {
 /* a_msw    a_lsw    b_msw    b_lsw   instr  status*/
        /* with nxm = 1 */
  {one_sp,   nocare,   maxm_sp,  nocare,   add_sp_l, 0x4 },/* inexact  */
       /*with nxm = 0 or 1 does not care */
  {one_sp,   nocare,   zero_sp,  nocare,   div_sp_l,  0x4},/* div/zero */
  {maxm_sp,  nocare,   maxm_sp,  nocare,   mult_sp_l, 0x4},/* overflow */
  {zero_sp,  nocare,   zero_sp,  nocare,   div_sp_l,  0x4},/* not a valid */
  {maxn_sp,  nocare,   maxn_sp,  nocare,   add_sp_l,  0x4},/* 5-ovfl,inex*/   
  {maxn_sp,  nocare,   maxn_sp,  nocare,   mult_sp_l, 0x4},/* 5-ovfl,inex*/   
  {maxn_msw, maxn_lsw, maxn_msw, maxn_lsw, mult_dp_l, 0x4},
  {one_msw,  one_lsw,  zero_msw, zero_lsw, div_dp_l,  0x4}, 
  {one_sp,   nocare,   nn_sp,    nocare,   add_sp_l,  0x8},
  {one_msw,  one_lsw,  nn_msw,   nn_lsw,   add_dp_l,  0x8},
  {one_sp,   nocare,   nn_sp,    nocare,   mult_sp_l, 0x8},
  {one_msw,  one_lsw,  nn_msw,   nn_lsw,   mult_dp_l, 0x8},
  {maxd_sp,  nocare,   two_sp,   nocare,   div_sp_l,  0x8},/* 8-a-denorm*/ 
  {maxd_msw, maxd_lsw, two_msw,  two_lsw,  div_dp_l,  0x8},
  {min1_sp,  nocare,   pi_4_sp,  nocare,   mult_sp_l, 0x8},/* 7-unfl,inex*/
  {maxd_sp,  nocare,   half_sp,  nocare,   mult_sp_l, 0x8},/* 8 -a-denorm*/
  {maxd_msw, maxd_lsw, half_msw, half_lsw, mult_dp_l, 0x8},
  {half_sp,  nocare,   maxd_sp,  nocare,   mult_sp_l, 0x8},/* 9 -b-denorm*/
  {half_msw, half_lsw, maxd_msw, maxd_lsw, mult_dp_l, 0x8},
  {min1_msw, min1_lsw, pi_4_msw, pi_4_lsw, mult_dp_l, 0x8},
  {nan_sp,   nocare,   zero_sp,  nocare,   add_sp_l,  0x8},/* 12-a-nan*/
  {nan_msw,  nan_lsw,  zero_msw, zero_lsw, add_dp_l,  0x8}, 
  {zero_sp,  nocare,   nan_sp,   nocare,   add_sp_l,  0x8},/* 13 -b-nan*/
  {zero_sp,  nocare,   nan_msw,  nan_lsw,  add_dp_l,  0x8},
  {nan_sp,   nocare,   nan_sp,   nocare,   add_sp_l,  0x8},/* 14 -ab-nan*/
  {nan_msw,  nan_lsw,  nan_msw,  nan_lsw,  add_dp_l,  0x8},
  {nan_sp,   nocare,   zero_sp,  nocare,   mult_sp_l, 0x8},/* 11-a-nan*/
  {nan_msw,  nan_lsw,  zero_msw, zero_lsw, mult_dp_l, 0x8}, 
  {zero_sp,  nocare,   nan_sp,   nocare,   mult_sp_l, 0x8},/* 13-b-nan*/
  {zero_sp,  nocare,   nan_msw,  nan_lsw,  mult_dp_l, 0x8},
  {nan_sp,   nocare,   nan_sp,   nocare,   mult_sp_l, 0x8},/* 14-ab-nan*/
  {nan_msw,  nan_lsw,  nan_msw,  nan_lsw,  mult_dp_l, 0x8},
  {     00,       00,      000,      000,       0000, 0x0}
};


/*
 *	registers.c
 */
unsigned long  pat[] = {
        0x00000000,
        0x55555555,
        0xAAAAAAAA,
        0xCCCCCCCC,
        0xFFFFFFFF
};


/*
 *  	instruction.c
 */	
struct  value {
        unsigned long floatsingle;
        unsigned long floatdouble;
};
struct value rnd[] = {
0x3f000000,   0x3fe00000,
0x3fc00000,   0x3ff80000,
0x40200000,   0x40040000,
0x40600000,   0x400c0000,
0x40900000,   0x40120000,
0x40b00000,   0x40160000,
0x40d00000,   0x401a0000,
0x40f00000,   0x401e0000,
0x41080000,   0x40210000,
0x41180000,   0x40230000,
0x41280000,   0x40250000,
0x41380000,   0x40270000,
0x41480000,   0x40290000,
0x41580000,   0x402a0000,
0x41680000,   0x402c0000,
0x41780000,   0x402f0000
};

struct value val[] = {
0         ,   0,
0x3F800000,   0x3FF00000,
0x40000000,   0x40000000,
0x40400000,   0x40080000,        
0x40800000,   0x40100000,
0x40A00000,   0x40140000,
0x40C00000,   0x40180000,
0x40E00000,   0x401C0000,
0x41000000,   0x40200000,
0x41100000,   0x40220000,
0x41200000,   0x40240000,
0x41300000,   0x40260000,
0x41400000,   0x40280000,
0x41500000,   0x402A0000,
0x41600000,   0x402C0000,
0x41700000,   0x402E0000,
0x41800000,   0x40300000,
0x41880000,   0x40310000,
0x41900000,   0x40320000,
0x41980000,   0x40330000,
0x41a00000,   0x40340000,
0x41a80000,   0x40350000,
0x41b00000,   0x40360000,
0x41b80000,   0x40370000,
0x41c00000,   0x40380000,
0x41c80000,   0x40390000,
0x41d00000,   0x403a0000,
0x41d80000,   0x403b0000,
0x41e00000,   0x403c0000,
0x41e80000,   0x403d0000,
0x41f00000,   0x403e0000,
0x41f80000,   0x403f0000,
0x42000000,   0x40400000,
0x42040000,   0x40408000,
0x42080000,   0x40410000,
0x420c0000,   0x40418000,
0x42100000,   0x40420000,
0x42140000,   0x40428000,
0x42180000,   0x40430000,
0x421c0000,   0x40438000,
0x42200000,   0x40440000,
0x42240000,   0x40448000,
0x42280000,   0x40450000,
0x422c0000,   0x40458000,
0x42300000,   0x40460000,
0x42340000,   0x40468000,
0x42380000,   0x40470000,
0x423c0000,   0x40478000,
0x42400000,   0x40480000,
0x42440000,   0x40488000,
0x42480000,   0x40490000,
0x424c0000,   0x40498000,
0x42500000,   0x404a0000,
0x42540000,   0x404a8000,
0x42580000,   0x404b0000,
0x425c0000,   0x404b8000,
0x42600000,   0x404c0000,
0x42640000,   0x404c8000,
0x42680000,   0x404d0000,
0x426c0000,   0x404d8000,
0x42700000,   0x404e0000,
0x42740000,   0x404e8000,
0x42780000,   0x404f0000,
0x427c0000,   0x404f8000,
0x42800000,   0x40500000,
0x42820000,   0x40504000,
0x42840000,   0x40508000,
0x42860000,   0x4050c000,
0x42880000,   0x40510000,
0x428a0000,   0x40514000,
0x428c0000,   0x40518000,
0x428e0000,   0x4051c000,
0x42900000,   0x40520000,
0x42920000,   0x40524000,
0x42940000,   0x40528000,
0x42960000,   0x4052c000,
0x42980000,   0x40530000,
0x429a0000,   0x40534000,
0x429c0000,   0x40538000,
0x429e0000,   0x4053c000,
0x42a00000,   0x40540000,
0x42a20000,   0x40544000,
0x42a40000,   0x40548000,
0x42a60000,   0x4054c000,
0x42a80000,   0x40550000,
0x42aa0000,   0x40554000,
0x42ac0000,   0x40558000,
0x42ae0000,   0x4055c000,
0x42b00000,   0x40560000,
0x42b20000,   0x40564000,
0x42b40000,   0x40568000,
0x42b60000,   0x4056c000,
0x42b80000,   0x40570000,
0x42ba0000,   0x40574000,
0x42bc0000,   0x40578000,
0x42be0000,   0x4057c000,
0x42c00000,   0x40580000,
0x42c20000,   0x40584000,
0x42c40000,   0x40588000,
0x42c60000,   0x4058c000,
0x42c80000,   0x40590000,
0x42ca0000,   0x40594000,
0x42cc0000,   0x40598000,
0x42ce0000,   0x4059c000,
0x42d00000,   0x405a0000,
0x42d20000,   0x405a4000,
0x42d40000,   0x405a8000,
0x42d60000,   0x405ac000,
0x42d80000,   0x405b0000,
0x42da0000,   0x405b4000,
0x42dc0000,   0x405b8000,
0x42de0000,   0x405bc000,
0x42e00000,   0x405c0000,
0x42e20000,   0x405c4000,
0x42e40000,   0x405c8000,
0x42e60000,   0x405cc000,
0x42e80000,   0x405d0000,
0x42ea0000,   0x405d4000,
0x42ec0000,   0x405d8000,
0x42ee0000,   0x405dc000,
0x42f00000,   0x405e0000,
0x42f20000,   0x405e4000,
0x42f40000,   0x405e8000,
0x42f60000,   0x405ec000,
0x42f80000,   0x405f0000,
0x42fa0000,   0x405f4000,
0x42fc0000,   0x405f8000,
0x42fe0000,   0x405fc000,
0x43000000,   0x40600000,
0x43010000,   0x40602000,
0x43020000,   0x40604000,
0x43030000,   0x40606000,
0x43040000,   0x40608000,
0x43050000,   0x4060a000,
0x43060000,   0x4060c000,
0x43070000,   0x4060e000,
0x43080000,   0x40610000,
0x43090000,   0x40612000,
0x430a0000,   0x40614000,
0x430b0000,   0x40616000,
0x430c0000,   0x40618000,
0x430d0000,   0x4061a000,
0x430e0000,   0x4061c000,
0x430f0000,   0x4061e000,
0x43100000,   0x40620000,
0x43110000,   0x40622000,
0x43120000,   0x40624000,
0x43130000,   0x40626000,
0x43140000,   0x40628000,
0x43150000,   0x4062a000,
0x43160000,   0x4062c000,
0x43170000,   0x4062e000,
0x43180000,   0x40630000,
0x43190000,   0x40632000,
0x431a0000,   0x40634000,
0x431b0000,   0x40636000,
0x431c0000,   0x40638000,
0x431d0000,   0x4063a000,
0x431e0000,   0x4063c000,
0x431f0000,   0x4063e000,
0x43200000,   0x40640000,
0x43210000,   0x40642000,
0x43220000,   0x40644000,
0x43230000,   0x40646000,
0x43240000,   0x40648000,
0x43250000,   0x4064a000,
0x43260000,   0x4064c000,
0x43270000,   0x4064e000,
0x43280000,   0x40650000,
0x43290000,   0x40652000,
0x432a0000,   0x40654000,
0x432b0000,   0x40656000,
0x432c0000,   0x40658000,
0x432d0000,   0x4065a000,
0x432e0000,   0x4065c000,
0x432f0000,   0x4065e000,
0x43300000,   0x40660000,
0x43310000,   0x40662000,
0x43320000,   0x40664000,
0x43330000,   0x40666000,
0x43340000,   0x40668000,
0x43350000,   0x4066a000,
0x43360000,   0x4066c000,
0x43370000,   0x4066e000,
0x43380000,   0x40670000,
0x43390000,   0x40672000,
0x433a0000,   0x40674000,
0x433b0000,   0x40676000,
0x433c0000,   0x40678000,
0x433d0000,   0x4067a000,
0x433e0000,   0x4067c000,
0x433f0000,   0x4067e000,
0x43400000,   0x40680000,
0x43410000,   0x40682000,
0x43420000,   0x40684000,
0x43430000,   0x40686000,
0x43440000,   0x40688000,
0x43450000,   0x4068a000,
0x43460000,   0x4068c000,
0x43470000,   0x4068e000,
0x43480000,   0x40690000,
0x43490000,   0x40692000,
0x434a0000,   0x40694000,
0x434b0000,   0x40696000,
0x434c0000,   0x40698000,
0x434d0000,   0x4069a000,
0x434e0000,   0x4069c000,
0x434f0000,   0x4069e000,
0x43500000,   0x406a0000,
0x43510000,   0x406a2000,
0x43520000,   0x406a4000,
0x43530000,   0x406a6000,
0x43540000,   0x406a8000,
0x43550000,   0x406aa000,
0x43560000,   0x406ac000,
0x43570000,   0x406ae000,
0x43580000,   0x406b0000,
0x43590000,   0x406b2000,
0x435a0000,   0x406b4000,
0x435b0000,   0x406b6000,
0x435c0000,   0x406b8000,
0x435d0000,   0x406ba000,
0x435e0000,   0x406bc000,
0x435f0000,   0x406be000,
0x43600000,   0x406c0000,
0x43610000,   0x406c2000,
0x43620000,   0x406c4000,
0x43630000,   0x406c6000,
0x43640000,   0x406c8000,
0x43650000,   0x406ca000,
0x43660000,   0x406cc000,
0x43670000,   0x406ce000,
0x43680000,   0x406d0000,
0x43690000,   0x406d2000,
0x436a0000,   0x406d4000,
0x436b0000,   0x406d6000,
0x436c0000,   0x406d8000,
0x436d0000,   0x406da000,
0x436e0000,   0x406dc000,
0x436f0000,   0x406de000,
0x43700000,   0x406e0000,
0x43710000,   0x406e2000,
0x43720000,   0x406e4000,
0x43730000,   0x406e6000,
0x43740000,   0x406e8000,
0x43750000,   0x406ea000,
0x43760000,   0x406ec000,
0x43770000,   0x406ee000,
0x43780000,   0x406f0000,
0x43790000,   0x406f2000,
0x437a0000,   0x406f4000,
0x437b0000,   0x406f6000,
0x437c0000,   0x406f8000,
0x437d0000,   0x406fa000,
0x437e0000,   0x406fc000,
0x437f0000,   0x406fe000
};

unsigned long  neg_val[] = {
        0x80000000,
        0xBF800000,
        0xC0000000,
        0xC0400000, 
        0xC0800000, 
        0xC0A00000, 
        0xC0C00000, 
        0xC0E00000, 
        0xC1000000, 
        0xC1100000,  
        0xC1200000,  
        0xC1300000,  
        0xC1400000,  
        0xC1500000,  
        0xC1600000,  
        0xC1700000,  
        0xC1800000 
}; 
    
/*
 *  	Datapath.c 
 */
/*
 *      The following routine for checking the data path between registers and
 *      memory and between memory and an floating register, and between the
 *      floating register and the weitek chips.  All the bits are covered 
 *	including the sign bit.
 *
 *      It works like this :
 *              f0 = value
 *              f1 = 0
 *      add  =  f4 = f0 + f1
 *
 *              for multiply
 *              f2 =1 
 *      mult =  f8 = f0 * f2
 *
 *              DOUBLE PRECISION
 *              f0 = msw of value
 *              f1 = lsw of value
 *              f2 = 0
 *              f3 = 0
 *      add =   f4 = f0 + f2
 *
 *              f6 = msw of 1
 *              f7 = lsw of 1
 *      mult =  f8 = f0 * f6
 */


unsigned long   result_lsw, result_msw;

                                 
data_path_sp()
{
        int     i, j, k ;
        unsigned long result, value;
         
        clear_regs(0);
        for (i = 0; i < 2; i++) { /* sign bit */
                for (j = 1; j < 255; j++) {
                        for (k = 0; k < 23; k++) {
                                value = (i << 31) | (j << 23) | (1 << k);

                                if (result = datap_add(value)) {

                                        send_message(0, ERROR, 
"\nAdd SP failed: expected / read = %x / %x\n", value, result);
                                        return(-1);
                                }
                                if (result = datap_mult(value)) {
                                        send_message(0, ERROR, 
"\nMultiply SP failed:expected / read = %x / %x\n", value, result); 
                                        return(-1);
                                }
                        }
                }
        }
        return(0);
}
 
data_path_dp()
{
        int     i, j, k, l; 
        unsigned long result_lsw, result_msw, value_lsw, value_msw; 
         
        clear_regs(0);

        for (i = 0; i < 2; i++) {
                 
                for (j = 1; j < 2047; j++) {
                        for (k = 0; k < 52; k++) {               
                 
                                value_lsw = (1 << k);
                                if (k > 32)
                                        l = k - 32;
                                else
                                        l = 32;
         
                                value_msw = (i << 31) | (j << 20) | (1 << l);    
                                 
                                if (datap_add_dp(value_msw, value_lsw)){
 
                                        send_message(0, ERROR,
"\nAdd DP failed: msw : expected / read = %x / %x\n\
               lsw : expected / read = %x / %x\n", 
                                                value_msw, result_msw,
                                                value_lsw, result_lsw);  
                                        return(-1);
                                }

                                if (datap_mult_dp(value_msw, value_lsw)){ 
 
                                        send_message(0, ERROR, "\nMultiply DP failed: msw : expected / read = %x / %x\n\
               lsw : expected / read = %x / %x\n",   
                                                value_msw, result_msw,
                                                value_lsw, result_lsw);      
                                        return(-1);
                                }
                        }
                }
        }
        return(0);
}


/*
 *
 *      The following test does 10 add operation continuously,
 *              10 multiply operations continusously and
 *      see any thing goes wrong.
 */      
timing_test() 
{
        int     i;
        unsigned long result, value_lsw;

        for (i = 0; i < 1000; i++) {
                clear_regs(0);
                if (result = timing_add_sp()) {
                        send_message(0, ERROR, "\nSingle Precision: add, expected / observed = 0x41200000 / 0x%x\n", result);      
                        return(-1);
                }
                clear_regs(0);
                if (result = timing_mult_sp()) {
                        send_message(0, ERROR, "\nSingle Precision: Multiply, expected / observed = 0x43470000 / 0x%x\n",
                                        result);
                        return(-1);
                }
                clear_regs(0);
                if (result = timing_add_dp(&value_lsw)){
                        send_message(0, ERROR,
"\nDouble Precision: Add, MSW : expcected / observed = 0x40240000 / 0x%x\n\
                       LSW : expected / observed = 0x0 / 0x%x\n",
                                        result, value_lsw);
                        return(-1);
                }
                clear_regs(0);
                if (result = timing_mult_dp(&value_lsw)) {
                        send_message(0, ERROR,
"\nDouble Precision: Multiply, MSW : expcected / observed = 0x4034000 / 0x%x\n\
                            LSW : expected / observed = 0x0 / 0x%x\n",
                                        result, value_lsw);  
                        return(-1);
                }

        }
        return(0);
}


chain_sp_test()
{
        int     i, result;

        clear_regs(0);
        set_fsr(0);
        for (i = 1; i < 60; i++) {

                if ((result = chain_sp(i)) != i) {
                        send_message(0, ERROR, "\nError: expected / observed = %x / 0x%x\n",
				i, result);
                                return(-1);
                }

        }
        return(0);
}

chain_dp_test()
{
        int     i, result;     
 
        clear_regs(0); 
        set_fsr(0);
        for (i = 1; i < 60; i++) {    
 
                if ((result = chain_dp(i)) != i) {      
                        send_message(0, ERROR, "\nError: expected / observed = %x / 0x%x\n",
				i, result); 
                                return(-1);    
                }      
        }      
        return(0);     
}
  
/*
 *	Instruction.c
 */

integer_to_float_sp(){
        int     i;
        unsigned long  result;

        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = int_float_s(i);
                if (result != val[i].floatsingle) {
                        send_message(0, ERROR, "\nfitos failed: int = %d, expected / observed = %x / %x\n",
                                        i,val[i].floatsingle, result);
                        return(-1);
                }
        }
        return(0);
}

integer_to_float_dp(){
        int     i;
        unsigned long  res1;

        clear_regs(0); 
        for (i = 0; i < 200; i++) {
                res1 = int_float_d(i);
                if (res1 != val[i].floatdouble) {
                        send_message(0, ERROR, "\nfitod failed: int = %d, expected / observed = %x / %x\n", 
                                        i,val[i].floatdouble, res1);
                        return(-1);
                }
        }
        return(0);
}

float_to_integer_sp() {
        int     i;
        unsigned long  result;

        clear_regs(0);
        for (i = 0; i < 200; i++) { 
                result = float_int_s(val[i].floatsingle);
                if (result != i) {
                        send_message(0, ERROR, "\nfstoi failed: int = %d, expected / observed = %x / %x\n",  
                                        i, i, result);
                        return(-1);
                }
        }
        return(0);
}

float_to_integer_dp() {
        int     i;
        unsigned long  res1, result;

        clear_regs(0);
        for (i = 0; i < 200; i++) {
                res1 = float_int_d(val[i].floatdouble);
                if (res1 != i) {
                        send_message(0, ERROR, "\nfdtoi failed: int = %d, expected / observed = %x / %x\n",   
                                        i, i, res1);
                        return(-1);
                }
        }
        return(0);
}

single_doub()
{
        int     i, j;
        unsigned long  result;

        clear_regs(0);

        for (i = 0; i < 200; i++) {
                result = convert_sp_dp(val[i].floatsingle);
                if (result != val[i].floatdouble) {
                        send_message(0, ERROR, "\nfstod failed: int = %d, expected / observed = %x / %x\n",   
                                       i, val[i].floatdouble, result); 
                        return(-1);
                } 
        }

        return(0);
}

double_sing()
{
        int     i, j;
        unsigned long  result;

        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = convert_dp_sp(val[i].floatdouble);
                if (result != val[i].floatsingle) {
                        send_message(0, ERROR, "\nfdtos failed: int = %d, expected / observed = %x / %x\n",    
                                       i, val[i].floatsingle, result); 
                        return(-1); 
                }  
        }
        return(0);
}

fmovs_ins()
{
        unsigned long  result;
         
        clear_regs(0);
        if ((result = move_regs(0x3F800000)) != 0x3F800000) {
                send_message(0, ERROR, "\nfmovs failed : written %x to f0, read from f31 = %x\n", 0x3F800000, result);
                return(-1);
        }
        return(0);
}

get_negative_value_pn()
{
        int     i;
        unsigned long  result;

        clear_regs(0);

        for (i = 0; i < 17 ; i++) {
                result = negate_value(val[i].floatsingle);
                if (result != neg_val[i]) {

                   send_message(0, ERROR, "\nfnegs failed(from pos to neg): int = %d,  expected / observed =  %x / %x\n", 
                                        i, neg_val[i], result);
                   return(-1);
                }
        }
        return(0);
}

get_negative_value_np()
{
        int     i;
        unsigned long  result;
 
        clear_regs(0);
        for (i = 0; i < 17; i++) {
                result = negate_value(neg_val[i]);
                if (result != val[i].floatsingle) {
                   send_message(0, ERROR, "\nfnegs failed (from neg. to pos): int = %d, expected / observed = %x / %x\n", 
                                i, val[i].floatsingle, result);
                  return(-1);
                }
        }
        return(0);
}

fabs_ins()
{
        int  i, j;
        unsigned long result;

        clear_regs(0);
        for (i = 0; i < 17; i++) {
                result = absolute_value(neg_val[i]);
                if (result != val[i].floatsingle) {
                          send_message(0, ERROR, "\nfabs failed: int = %d, expected / observed = %x / %x\n", 
                                i, val[i].floatsingle, result);
                          return(-1);
                }
        }
        return(0);
}

addition_test_sp()
{
        int  i;
        unsigned long result;

        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = add_sp(val[i].floatsingle, val[1].floatsingle);
                if (result != (val[i+1].floatsingle)) {
                       send_message(0, ERROR, "\nfadds failed: int = %d, f0 = %x, f2 = %x, f0+f2 = f4 = %x\n", 
                                i+1, val[i].floatsingle, val[1].floatsingle, result);
                        return(-1);
                }
        }
        return(0);
}

addition_test_dp()
{
        int  i;  
        unsigned long result;
 
        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = add_dp(val[i].floatdouble, val[1].floatdouble);
                if (result != (val[i+1].floatdouble)) {
                        send_message(0, ERROR, "\nfaddd failed: int = %d, f0 = %x, f2 = %x, f0+f2 = f4 =  %x\n", 
                                i+1, val[i].floatdouble, val[1].floatdouble, result);
                        return(-1);
                }
        }
        return(0);
}

subtraction_test_sp()
{
        int  i;
        unsigned long result;

        clear_regs(0);
        for (i = 1; i < 200; i++) {
                result = sub_sp(val[i].floatsingle, val[i-1].floatsingle);
                if (result != val[1].floatsingle) {
                        send_message(0, ERROR, "\nfsubs failed:int = %d, f0 = %x, f2 = %x, f0-f2 = f4 = %x\n", 
                                i-1,val[i].floatsingle, val[i-1].floatsingle, result);
                        return(-1);
                }
        }
        return(0);
}

subtraction_test_dp()
{
        int  i;
        unsigned long result;
 
        clear_regs(0); 
        for (i = 1; i < 200; i++) {
                result = sub_dp(val[i].floatdouble, val[i-1].floatdouble);
                if (result != val[1].floatdouble) {
                        send_message(0, ERROR, "\bfsubd failed: int = %d, f0 = %x, f2 = %x, f0-f2 = f4 = %x\n", 
                                i-1,val[i].floatdouble, val[i-1].floatdouble, result);
                        return(-1);
                }
        }
        return(0);
} 

squareroot_test_sp()
{
        int     i;
        unsigned long result, workvalue;
	
	if (fpu_is_weitek()) 
		return(0); 		/* skip on Weitek fpu */

        clear_regs(0);
        for (i = 1; i < 200; i++) {
		workvalue = val[i].floatsingle;
                result = square_sp( mult_sp(workvalue,workvalue) ); 
                if (result != workvalue) {
                	send_message(0, ERROR, "\nfsqrt failed: written / read = %x / %x\n",
				 workvalue, result);        
			return(1);
		}
	}
        return(0);		/* test passed - no errors detected */
}

squareroot_test_dp()
{
        int     i;
        unsigned long result, workvalue;

	if (fpu_is_weitek()) 
		return(0); 		/* skip on Weitek fpu */

        clear_regs(0);
        for (i = 1; i < 200; i++) {
                 
		workvalue = val[i].floatdouble;
                result = square_dp( mult_dp(workvalue,workvalue) ); 
                if (result != workvalue) {
                	send_message(0, ERROR, "\nfsqrt failed: written / read = %x / %x\n",
				 workvalue, result);        
			return(1);
		}
	}
        return(0);		/* test passed - no errors detected */
}

division_test_sp()
{
        int     i;
        unsigned long result;

        clear_regs(0);
        for (i = 1; i < 200; i++) {
                result = div_sp(val[i].floatsingle, val[1].floatsingle);
                if (result != val[i].floatsingle) {
                        send_message(0, ERROR, "\nfdivs failed: int = %d, f0 = %x, f2 = %x, f0 / f2 = f4 = %x\n",
                                        i, val[i].floatsingle, val[1].floatsingle, result);
                        return(-1);
                }

        }
        return(0);
}

division_test_dp()
{
        int     i;
        unsigned long result;
 
        clear_regs(0);
        for (i = 1; i < 200; i++) {
                result = div_dp(val[i].floatdouble, val[1].floatdouble);
                if (result != val[i].floatdouble) {
                        send_message(0, ERROR, "\nfdivd failed: int = %d, f0 = %x, f2 = %x, f0 / f2 = f4 = %x\n", 
                                        i, val[i].floatdouble, val[1].floatdouble, result);
                        return(-1);
                }
        }
        return(0);
}

multiplication_test_sp()
{
        int     i; 
        unsigned long result; 

        clear_regs(0);
        for (i = 0; i < 200; i++) { 
                result = mult_sp(val[i].floatsingle, val[1].floatsingle); 
                if (result != val[i].floatsingle) { 
                        send_message(0, ERROR, "\nfmuls failed: int = %d, f0 = %x, f2 = %x, f0 / f2 = f4 = %x\n", 
                                        i, val[i].floatsingle, val[1].floatsingle, result);
                        return(-1);
                }
        }
        return(0);
}

multiplication_test_dp()
{
        int     i; 
        unsigned long result;
 
        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = mult_dp(val[i].floatdouble, val[1].floatdouble); 
                if (result != val[i].floatdouble) { 
                        send_message(0, ERROR, "\nfmuld failed: int = %d, f0 = %x, f2 = %x, f0 / f2 = f4 = %x\n",  
                                        i, val[i].floatdouble, val[1].floatdouble, result); 
                        return(-1); 
                } 

        }
        return(0); 
} 

compare_sp()
{
        int     i;
        unsigned long  result;

        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = cmp_s(val[i].floatsingle, val[i].floatsingle);
                if ((result & 0xc00) != 0) {
                        send_message(0, ERROR, "\nfcmps failed: f0 = %d, f2 = %d : expected / observed = 0 / %x\n",
                                i,i,(result & 0xc00) >> 10);
                                return(-1);
                }
                result  = cmp_s(val[i].floatsingle, val[i+1].floatsingle);
                if ((result & 0xc00) != 0x400) {
                        send_message(0, ERROR, "\nfcmps failed: f0 = %d, f2 = %d : expected / observed = 1 /%x\n",
                                i,i+1, (result & 0xc00) >> 10); 
                                return(-1); 
                } 
                result  = cmp_s(val[i+1].floatsingle, val[i].floatsingle); 
                if ((result & 0xc00) != 0x800) { 
                        send_message(0, ERROR, "\nfcmps failed: f0 = %d, f2 = %d : expected / observed = 2 /%x\n",
                                i+1, i,(result & 0xc00) >> 10);  
                                return(-1);  
                }  
        
                result  = cmp_s(val[i].floatsingle,0x7f800400);
                if ((result & 0xc00) != 0xc00){  
                        send_message(0, ERROR, "\nfcmps failed: f0 = %d, f2 = NaN : expected / observed = 3 /%x\n", 
                                i,(result & 0xc00) >> 10);   
                                return(-1);  
                }
        }
        return(0);
}

compare_dp()
{
        int     i;
        unsigned long  result;
 
        clear_regs(0);
        for (i = 0; i < 200; i++) {
                result = cmp_d(val[i].floatdouble, val[i].floatdouble);
                if ((result & 0xc00) != 0) {
                        send_message(0, ERROR, "\nfcmpd failed: f0 = %d, f2 = %d : expected / observed = 0 / %x\n",
                                i,i,(result & 0xc00) >> 10); 
                      return(-1); 
                } 
                result = cmp_d(val[i].floatdouble, val[i+1].floatdouble); 
                if ((result & 0xc00) != 0x400) { 
                        send_message(0, ERROR, "\nfcmpd failed: f0 = %d, f2 = %d : expected / observed = 1 /%x\n", 
                                i, i+1,(result & 0xc00) >> 10);  
                        return(-1);  
                }  
                result = cmp_d(val[i+1].floatdouble, val[i].floatdouble); 
                if ((result & 0xc00) != 0x800) {  
                        send_message(0, ERROR, "\nfcmpd failed: f0 = %d, f2 = %d : expected / observed = 2 /%x\n", 
                                i+1, i,(result & 0xc00) >> 10);   
                       return(-1);  
                }   
                result = cmp_d(val[i].floatdouble, 0x7ff00080);
                if ((result & 0xc00) != 0xc00) {   
                        send_message(0, ERROR, "\nfcmpd failed: f0 = %d, f2 = NaN : expected / observed = 3 /%x\n",  
                                i, (result & 0xc00) >> 10);    
                       return(-1);   
                }
        }
        return(0);
}

branching()
{
        int     i;
        unsigned long   result;


	clear_regs(0);
        result = get_fsr();
		/* was    0xF0700000 - changed to ignore unused bits 29,28 and reserved bits 20,21 (for future compatibility) */
        result = result & 0xC0400000; /* set all exception bits to zero */
        set_fsr(result);
        error_ok = 1;

	for (i = 0; i < 64; i++) {

                if (result = branches(0, val[i].floatsingle, 0x7f800400)) {
                        send_message(0, ERROR, "\nFBU failed. result = %x\n", result);
                        return(-1);
                }
                if (result = branches(1, val[i+1].floatsingle, val[i].floatsingle)) {
                        send_message(0, ERROR, "\nFBG failed: f0 = %x, f2 = %x.\n", i+1, i);
                        return(-1);
                }
                if (result = branches(2, val[i].floatsingle, 0x7f800400)) {
                        send_message(0, ERROR, "\nFBUG (unordered) failed.\n");
                        return(-1);
                }
                if (result = branches(2, val[i+1].floatsingle, val[i].floatsingle)) {
                        send_message(0, ERROR, "\nFBUG (greater) failed: f0 = %x, f2 = %x.\n", i+1, i);
                        return(-1);   
                }
                if (result = branches(3, val[i].floatsingle,val[i+1].floatsingle)) {
                        send_message(0, ERROR, "\nFBL failed: f0 = %x, f2 = %x.\n", i, i+1);
                        return(-1);
                }
                if (result = branches(4, val[i].floatsingle, 0x7f800400)) {
                        send_message(0, ERROR, "\nFBUL (unordered) failed.\n");
                        return(-1);
                }
                if (result = branches(4,val[i].floatsingle,val[i+1].floatsingle)) { 
                        send_message(0, ERROR, "\nFBUL (Less) failed: f0 = %x, f2 = %x.\n", i, i+1);
                        return(-1);   
                } 
                if (result = branches(5, val[i].floatsingle,val[i+1].floatsingle)) {  
                        send_message(0, ERROR, "\nFBLG (Less) failed: f0 = %x, f2 = %x.\n", i, i+1); 
                        return(-1);    
                }  
                if (result = branches(5, val[i+1].floatsingle, val[i].floatsingle)) {
                        send_message(0, ERROR, "\nFBLG (Greater) failed: f0 = %x, f2 = %x.\n", i+1, i);
                        return(-1);     
                }   
                if (result = branches(6, val[i].floatsingle,val[i+1].floatsingle)) {   
                        send_message(0, ERROR, "\nFBNE failed: f0 = %x, f2 = %x.\n", i, i+1); 
                        return(-1);    
                }  
                if (result = branches(7, val[i].floatsingle,val[i].floatsingle)) {    
                        send_message(0, ERROR, "\nFBE failed : f0 = %x, f2 = %x.\n", i, i);  
                        return(-1);     
                }   

                if (result = branches(8, val[i].floatsingle, 0x7f800400)) { 
                        send_message(0, ERROR, "\nFBUE (unordered) failed.\n"); 
                        return(-1);   
                } 

                if (result = branches(8, val[i].floatsingle,val[i].floatsingle)) {     
                        send_message(0, ERROR, "\nFBUE (equal) failed : f0 = %x, f2 = %x.\n", i, i);      
                        return(-1);      
                }    
                if (result = branches(9, val[i].floatsingle,val[i].floatsingle)) {      
                        send_message(0, ERROR, "\nFBGE (equal) failed : f0 = %x, f2 = %x.\n", i, i);    
                        return(-1);       
                }     
                if (result = branches(9, val[i+1].floatsingle, val[i].floatsingle)) {
                        send_message(0, ERROR, "\nFBGE (greater) failed: f0 = %x, f2 = %x.\n", i+1, i);
                        return(-1);   
                }

                if (result = branches(10, val[i].floatsingle, 0x7f800400)) {  
                        send_message(0, ERROR, "\nFBUGE (unordered) failed.\n");  
                        return(-1);    
                }  

                if (result = branches(10, val[i].floatsingle,val[i].floatsingle)) {
                        send_message(0, ERROR, "\nFBUGE (equal) failed : f0 = %x, f2 = %x.\n", i, i); 
                        return(-1);        
                }      
                if (result = branches(10, val[i+1].floatsingle, val[i].floatsingle)) { 
                        send_message(0, ERROR, "\nFBUGE (greater) failed: f0 = %x, f2 = %x.\n", i+1, i); 
                        return(-1);    
                } 
                if (result = branches(11, val[i].floatsingle,val[i+1].floatsingle)) {   
                        send_message(0, ERROR, "\nFBLE (Less) failed: f0 = %x, f2 = %x.\n", i, i+1);  
                        return(-1);     
                }   
                if (result = branches(11, val[i].floatsingle,val[i].floatsingle)) {        
                        send_message(0, ERROR, "\nFBLE (equal) failed : f0 = %x, f2 = %x.\n", i, i);    
                        return(-1);         
                }       

                if (result = branches(12, val[i].floatsingle, 0x7f800400)) {   
                        send_message(0, ERROR, "\nFBULE (unordered) failed.\n");   
                        return(-1);     
                }   

                if (result = branches(12, val[i].floatsingle,val[i+1].floatsingle)) {    
                        send_message(0, ERROR, "\nFBULE (Less) failed: f0 = %x, f2 = %x.\n", i, i+1);   
                        return(-1);      
                }    
                if (result = branches(12, val[i].floatsingle,val[i].floatsingle)) {         
                        send_message(0, ERROR, "\nFBULE (equal) failed : f0 = %x, f2 = %x.\n", i, i);       
                        return(-1);          
                }        
                if (result = branches(13, val[i].floatsingle,val[i+1].floatsingle)) {
                        send_message(0, ERROR, "\nFBO failed: f0 = %x, f2 = %x.\n", i, i+1);    
                        return(-1);       
                }     
                if (result = branches(14, val[i].floatsingle,val[i+1].floatsingle)) {      
                        send_message(0, ERROR, "\nFBA failed: f0 = %x, f2 = %x.\n", i, i+1);     
                        return(-1);        
                }      
                if (result = branches(15, val[i].floatsingle,val[i+1].floatsingle)) {       
                        send_message(0, ERROR, "\nFBN failed: f0 = %x, f2 = %x.\n", i, i+1);      
                        return(-1);         
                }       
        }    
        error_ok = 0;
        return(0);

}

no_branching()
{
        int     i;
        unsigned long   result;

	clear_regs(0);
        result = get_fsr();
		/* was    0xF0700000 - changed to ignore unused bits 29,28 and reserved bits 20,21 (for future compatibility) */
        result = result & 0xC0400000; /* set all exception bits to zero */
        set_fsr(result);
        error_ok = 1;  


         
        for (i = 0; i < 64; i++) {      
         
                if (!(result = branches(0, val[i].floatsingle, val[i].floatsingle))) {
                        send_message(0, ERROR, "\nFBU failed.\n");
                        return(-1);
                }

                if (!(result = branches(1,val[i].floatsingle, val[i].floatsingle))) {
                        send_message(0, ERROR, "\nFBG failed: f0 = %x, f2 = %x.\n", i, i);
                        return(-1);
                }
                if (!(result = branches(2,val[i].floatsingle, val[i].floatsingle))) {  
                        send_message(0, ERROR, "\nFBUG  failed: f0 = %x, f2 = %x.\n", i, i);
                        return(-1);   
                }
                if (!(result = branches(3,val[i].floatsingle, val[i].floatsingle))) {  
                        send_message(0, ERROR, "\nFBLfailed: f0 = %x, f2 = %x.\n", i, i);
                        return(-1);
                }
                if (!(result = branches(4, val[i].floatsingle, val[i].floatsingle))) {  
                        send_message(0, ERROR, "\nFBUL failed: f0 = %x, f2 = %x.\n", i, i);
                        return(-1);
                }
                if (!(result = branches(5, val[i].floatsingle, val[i].floatsingle))) {  
                        send_message(0, ERROR, "\nFBLG failed: f0 = %x, f2 = %x.\n", i, i); 
                        return(-1);    
                } 

                if (!(result = branches(6, val[i].floatsingle, val[i].floatsingle))) {
                        send_message(0, ERROR, "\nFBNE failed: f0 = %x, f2 = %x.\n", i, i); 
                        return(-1); 
                } 
                if (!(result = branches(7, val[i+1].floatsingle, val[i].floatsingle))) {     
                        send_message(0, ERROR, "\nFBE failed: f0 = %x, f2 = %x.\n", i+1 , i); 
                        return(-1); 
                } 
                if (!(result = branches(8, val[i+1].floatsingle, val[i].floatsingle))) {
                        send_message(0, ERROR, "\nFBUE failed: f0 = %x, f2 = %x.\n", i+1 , i);
                        return(-1);
                }
                if (!(result = branches(9, val[i].floatsingle, val[i+1].floatsingle))) {
                        send_message(0, ERROR, "\nFBGEfailed: f0 = %x, f2 = %x.\n", i, i+1 );
                        return(-1);
                } 
                if (!(result = branches(10, val[i].floatsingle,val[i+1].floatsingle))) { 
                        send_message(0, ERROR, "\nFBUGEfailed: f0 = %x, f2 = %x.\n", i, i+1 ); 
                        return(-1); 
                }  
                if (!(result = branches(11, val[i+1].floatsingle, val[i].floatsingle))) {
                        send_message(0, ERROR, "\nFBLE failed: f0 = %x, f2 = %x.\n", i+1 , i);
                        return(-1);
                } 
                if (!(result = branches(12,  val[i+1].floatsingle, val[i].floatsingle))) { 
                        send_message(0, ERROR, "\nFBULE failed: f0 = %x, f2 = %x.\n", i+1 , i); 
                        return(-1); 
                }  

                if (!(result = branches(13, val[i].floatsingle, 0x7f800400))) {  
                        send_message(0, ERROR, "\nFBO failed.\n");
                        return(-1);         
                }       

        }    
	error_ok = 0;
        return(0);
}

compare_sp_except()
{
        int     i;
        unsigned long  result;

        clear_regs(0);
        result = get_fsr(); 
        result = result | 0xF800000; 
        set_fsr(result); 
        error_ok = 1;

        for (i = 0; i < 200; i++) {
                result = cmp_s_ex(val[i].floatsingle, 0x7fbfffff);
                if (!trap_flag) {
                        send_message(0, ERROR, "\nfcmpxs failed: Exception did not occur. fsr = %x\n", result);
                        return(-1);
                }
        }
        error_ok = 0;
        return(0);
} 

compare_dp_except()
{
        int     i;
        unsigned long  result; 
 
        clear_regs(0); 
        result = get_fsr();  
        result = result | 0xF800000;  
        set_fsr(result);  
        error_ok = 1; 

	  for (i = 0; i < 199; i++) {

                result = cmp_d_ex(val[i].floatdouble, 0x7ff00080);
                if (!trap_flag) { 
                        send_message(0, ERROR, "\nfcmpxd failed: Exception did not occur. fsr = %x\n", result); 
                                return(-1); 
                } 

        } 
        error_ok = 0;
        return(0); 
} 
 


/*
 *      Nack test : In this test the program will create all
 *                  possible floating point traps and
 *                  checks whether it occured or not.
 */

fpu_nack_test()
{
        unsigned long fsr_status;

        error_ok = 1;

        fsr_status = get_fsr();
        fsr_status = fsr_status | 0xF800000; /* set the TEM bits */
        set_fsr(fsr_status);
        donot_dq = 0x0;  
        trap_flag = 0x0;        /* unset the trap flag */
        /* Test that there should not be any exception */        

        wadd_sp(0x3f800000, 0x3f800000);
        if (trap_flag) {
                send_message(0, ERROR, "\nError: Bus error occured. ftt = %x ",
			 (fsr_at_trap & 0x1c000) >> 14);
		ftt_decode(fsr_at_trap);
                return(-1);
        }

        /* now test the IEEE exception */
        trap_flag = 0x0;        /* unset the trap flag */      
        wdiv_sp(0x3f800000, 0x00000);
        if (!trap_flag) {      
             send_message(0, ERROR, "\nError: Bus error did not occur(IEEE exception). ftt = %x", (fsr_at_trap & 0x0001C000) >> 14);
		ftt_decode(fsr_at_trap);
                return(-1);
        }

/*
 *  Second level:  Confirm that the ftt bits in the FSR are set correctly
 *                 *** Note this is not valid with SunOs >= 4.0 because
 *                 the OS clears the FSR on an exception.
 */
/*DISABLED - BDS
 *
 *      if ((fsr_at_trap & 0x1C000) != 0x4000) {
 *              send_message(0, ERROR, "\nError: ftt bits expected / observed = 1 / %x",
 *                              (fsr_at_trap & 0x0001C000) >> 14);
 *              ftt_decode(fsr_at_trap);
 *              return(-1);
 *      }
 */
        /* test for unfinished exception */
        trap_flag = 0x0;
        wadd_sp(0x7fbfffff, 0x0);

        if (!trap_flag) {
             send_message(0, ERROR, "\nError: Bus error did not occur(Unfinished exception). ftt = %x",
                        (fsr_at_trap & 0x0001C000) >> 14);
		ftt_decode(fsr_at_trap);
                return(-1);
        }
	wadd_sp(1,1);			/* clear the execption error */

/*
 *  Second level:  Confirm that the ftt bits in the FSR are set correctly
 *                 *** Note this is not valid with SunOs >= 4.0 because
 *                 the OS clears the FSR on an exception.
 */
/*DISABLED - BDS
 *      if ((fsr_at_trap & 0x1c000) != 0x8000) {
 *              send_message(0, ERROR, "\nError: ftt bits expected / observed = 2 / %x\n",
 *                               (fsr_at_trap & 0x0001c000) >> 14);
 *              ftt_decode(fsr_at_trap);
 *              return(-1);
 *       }
 */
	wadd_sp(1,1);			/* clear the execption error */
        set_fsr(0x0);
        error_ok = 0;
        return(0);
}        

/*
 *	retgisters.c
 *
 *      The following routine writes the given patterns to all the registers
 *      and reads it back.
 *      The following routine tests all the possible values given for single 
 *      precision numbers.
 *
 */
registers_three()
{
        int     i,j,k,l;
        unsigned long result, value;

        for (i = 0; i < 2; i++) {
            for (j = 1; j < 255; j++) {
                for (k = 0; k < 23; k++) {
                        for (l = 0; l < 32; l++) {
                                value = (i << 31) | (j << 23) | (1 << k);

                                if ((result = register_test(i, value)) != value) {
                            send_message(0, ERROR, "\nregister read/write failed : reg = %d, expected / observed = %x / %x\n",
                                        l, value, result);
                                        return(-1);
                                }
                        }
                }
            }
        }
        return(0);
}  
/*
 *      The following routine tests rotating ones to the registers
 */ 
registers_two()
{
        int     i,j;
        unsigned long result, value;

        for (j = 0; j < 32; j++) {
         
                for (i = 0; i < 32; i++) {
                        value = (1 << i);
                        if ((result = register_test(j, value)) != value) {
			send_message(0, ERROR, "\nregister test-2 read/write failed : reg = %d, expected / observed = %x / %x\n",
                                j, value, result);
                                return(-1);
                        }
                }
        }
        return(0);
}
         
/*
 *      The following routine tests with the patttern given
 */
registers_one()
{
        int     i,j, result;

        for (i = 0; i < 32; i++) {
                for (j = 0; j < 5; j++) {
                        result = register_test(i, pat[j]);
                        if (result != pat[j]) {
                          send_message(0, ERROR, "\nregister read/write failed : reg = %d, expected / observed = %x / %x\n",
                                        i, pat[j], result);
                         return(-1);
                        }
                }
        }
        return(0);
}  
         
/*
 * Function: 	Check the integrity of the FPU status register
 * 		by writing patterns and reading them back.
 *
 * History:	Updated to add FPU_ID_MASK (bds) so that the FPU ID
 *		is ignored. (Weitek = 00, TI = 01)
 *		6/2/89.  Changed FPU_ID_MASK to 0xFFF97FFF to accomodate
 *		campus.
 * History:	Updated FPU_ID_MASK to work with the LSI804 chip ----
 *		---- aka "Meiko" FPU -------- 1/7/91. 
 *
 * History:	Updated FPU_ID_MASK to ignore the unused bits 29,28 and 12.
 *
 * History:     Updated FPU_ID_MASK to ignore the ftt bits 14-16.
 *
 */

#define FPU_ID_MASK  0xCFF02FFF

static u_long fsr_testdata[] =
		{ 
			0x08000000,
	  		0x40000000,
			0x48000000,
			0x80000000,
			0x88000000,
			0xC0000000,
			0xC8000000
	  	};

fsr_test()
{
        int i;
        u_long result;

        for (i = 0; i < 7; i++) {  
                set_fsr(fsr_testdata[i]);
                result = (get_fsr() & FPU_ID_MASK);
                if (result != fsr_testdata[i]) {
                        send_message(0, ERROR, "\nFSR Error: expected / observed = %x / %x",
				 fsr_testdata[i], result);
			if ((result & 0x0001c000) != 0)
				ftt_decode(result);
                        return(-1);
                }
        }
        return(0);
}

psr_test()
{
        unsigned long testdata,result;

        printf("        PSR FPU bit Test:");
         
        if (result & 0x01000) {
                printf("\nFPU bit is disabled in the PSR, Check whether the FPU is placed correctly.\n");    
                return(-1);    
        }
        printf(" Passed.\n");
        return(0);
}

/*
 *	wstatus.c
 * This test causes the Weitek chips to produce every possible
 * value at the S+ outputs.  These bits are then observed via
 * the WSTATUS register.  The ALU and multiplier are distinguished 
 * by the instruction address
 */
fpu_ws()
{
        int     operation;
        unsigned long i, cmp_status, tmp_var, amsw, alsw, bmsw, blsw;
        unsigned long *ptr, status;
        unsigned long actual; 
        unsigned long tmp_val;



        set_fsr(0x0);
        donot_dq = 0;
        status = get_fsr();
        status = status | 0xF000000;
        set_fsr(status);
        error_ok = 1;

        for (i = 0; test_ws[i].instr != 0; i++) {

                trap_flag = 0x0;                /* unset the flag */     
                amsw = test_ws[i].a_msw;        /* get the most sig. word */
                alsw = test_ws[i].a_lsw;        /* get the least "      "  */
                bmsw = test_ws[i].b_msw;
                blsw = test_ws[i].b_lsw;
                operation = test_ws[i].instr;
                switch (operation) {

                        case 1 :
                                wadd_sp(amsw ,bmsw);
                                break;
                        case 2 :
                                wadd_dp(amsw, alsw, bmsw, blsw);
                                break;
                        case 3 :
                                wdiv_sp(amsw, bmsw);
                                break;
                        case 4 :
                                wdiv_dp(amsw, alsw, bmsw, blsw); 
                                break; 
                        case 5 :
                                wmult_sp(amsw, bmsw); 
                                break; 
                        case 6 :
                                wmult_dp(amsw, alsw, bmsw, blsw);  
                                break;  
                        default:
                                send_message(0, ERROR, "\nWrong code is given.\n");
                                break;
                }
                if (i) {
                    if (!trap_flag) {
                        send_message(0, ERROR, " FPU Trap did not occur , i = %d.\n", i);
#ifdef STOPONERROR
                                return(-1);
#endif
                    }
                }
                else {
                    if (trap_flag) {
                        send_message(0, ERROR, " FPU Trap Should not occur but occured.\n");
#ifdef STOPONERROR
                               return(-1);
#endif
                    }
                } 
                status = fsr_at_trap & 0x0001c000;
/*
 *  Second level:  Confirm that the ftt bits in the FSR are set correctly
 *                 *** Note this is not valid with SunOs >= 4.0 because
 *                 the OS clears the FSR on an exception.
 */
/*DISABLED - BDS
 *
 *                if (i) {
 *		if (((fsr_at_trap & 0xc000) >> 12) != test_ws[i].status) {
 *			printf("\nDid not create an correct exception, ");
 *			printf("ftt = %x should be = %x\n",
 *			     (fsr_at_trap & 0x1c000) >> 12, test_ws[i].status);
 *			ftt_decode(fsr_at_trap);
 *                       switch (operation) {
 *
 *                               case 1 : printf(" single precision add :");
 *                                       break;
 *                              case 2 :
 *                                      printf(" double precision add :");
 *                                      break;
 *                              case 3 :
 *                                      printf(" single precision divide :");
 *                                      break;
 *                              case 4  :
 *                                      printf(" double precision divide :");
 *                                      break;
 *                              case 5 :
 *                                      printf(" single precision multiply :");
 *                                      break;
 *                              case 6  :
 *                                      printf(" double precision multiply :");
 *                                      break;
 *                      }
 *                      if ((operation == 1) || (operation == 3) ||
 *					        (operation ==5))
 *                              printf("A = %x, B = %x\n",
 *					test_ws[i].a_msw, test_ws[i].b_msw);
 *                      else 
 *                              printf("Amsw = %x, Alsw = %x, Bmsw = %x, Blsw = %x\n",
 *                              	test_ws[i].a_msw, test_ws[i].a_lsw,
 *					test_ws[i].b_msw, test_ws[i].b_lsw);
 * #ifdef STOPONERROR
 *                      return(-1);
 * #endif
 *                }
 *              }
 *              if (i < 4) {
 *                      if (i == 1) 
 *                      if (!(fsr_at_trap & 0x2)) { 
 *                              printf("\nDid not create correct IEEE exception (Divide By zero) : expected = 2, observed = %x\n",
 *					 fsr_at_trap & 0x1f);  
 *					ftt_decode(fsr_at_trap);
 * #ifdef STOPONERROR
 *                                      return(-1); 
 * #endif
 *                      }
 *                      if (i == 2) 
 *                      if (!(fsr_at_trap & 0x8)) { 
 *                              printf("\nDid not create correct IEEE exception (Overflow) : expected = 8, observed = %x\n", fsr_at_trap & 0x1f);    
 *					ftt_decode(fsr_at_trap);
 * #ifdef STOPONERROR
 *					return(-1); 
 * #endif
 *                      } 
 *                      if (i == 3) 
 *                      if (!(fsr_at_trap & 0x10))  {
 *                              printf("\nDid not create correct IEEE exception (Invalid) : expected = 10, observed = %x\n", fsr_at_trap & 0x1f); 
 *					ftt_decode(fsr_at_trap);
 *					return(-1);  
 *                      }
 *              } 
 ***** END of Disabled Section */
        }
        error_ok = 0;
        return(0);
}

fpu_ws_nxm()
{
	int           operation;
        unsigned long i, cmp_status, tmp_var, amsw, alsw, bmsw, blsw;
        unsigned long *ptr, status;
        unsigned long prev_status;
        unsigned long actual;
        unsigned long tmp_val;

        prev_status = get_fsr();
        status = prev_status | 0xF800000;
        set_fsr(status);
        donot_dq = 0;
        error_ok = 1;

        for (i = 0; test_ws[i].instr != 0; i++) {
         
                trap_flag = 0x0;                /* unset the trap flag */
                amsw = test_ws[i].a_msw;        /* get most significant word */
                alsw = test_ws[i].a_lsw;        /* get the least "      "  */
                bmsw = test_ws[i].b_msw;
                blsw = test_ws[i].b_lsw;
                operation = test_ws[i].instr;
                switch (operation) {

                        case 1 :
                                wadd_sp(amsw ,bmsw);
                                break;
                        case 2 :
                                wadd_dp(amsw, alsw, bmsw, blsw);
                                break;
                        case 3 :
                                wdiv_sp(amsw, bmsw);
                                break;
                        case 4 :
                                wdiv_dp(amsw, alsw, bmsw, blsw); 
                                break; 
                        case 5 :
                                wmult_sp(amsw, bmsw); 
                                break; 
                        case 6 :
                                wmult_dp(amsw, alsw, bmsw, blsw);  
                                break;  
                        default:
                                printf("\nWrong code is given.\n");
                                break;
                }
                if (!trap_flag) {
                        send_message(0, ERROR, "Bus Error did not occur.\n");
                        return(-1);
                }
                status = fsr_at_trap & 0x1c000;
/*
 *  Second level:  Confirm that the ftt bits in the FSR are set correctly
 *                 *** Note this is not valid with SunOs >= 4.0 because
 *                 the OS clears the FSR on an exception.
 */
/*DISABLED - BDS
 *
 *		if (((fsr_at_trap & 0xc000) >> 12) != test_ws[i].status) {
 *			printf("\nDid not create an correct exception, ");
 *			printf("ftt = %x should be = %x\n",
 *			     (fsr_at_trap & 0x1c000) >> 12, test_ws[i].status);
 *
  *                       switch (operation) { 
 * 
 *                                case 1 : printf(" single precision add :");
 *                                        break;
 *                                case 2 : 
 *                                        printf(" double precision add :"); 
 *                                        break; 
 *                                case 3 :
 *                                        printf(" single precision divide :");
 *                                        break;  
 *                                case 4  :  
 *                                        printf(" double precision divide :"); 
 *                                        break;   
 *                                case 5 : 
 *                                        printf(" single precision multiply :"); 
 *                                        break;   
 *                                case 6  :   
 *                                        printf(" double precision multiply :");
 *                                        break;
 *                        } 
 *                        if ((operation == 1) || (operation == 3) || (operation ==5))
 *                                printf("A = %x, B = %x\n", test_ws[i].a_msw, test_ws[i].b_msw);
 *                        else
 *                                printf("Amsw = %x, Alsw = %x, Bmsw = %x, Blsw = %x\n",
 *                                test_ws[i].a_msw, test_ws[i].a_lsw, test_ws[i].b_msw, test_ws[i].b_lsw);         
 *#ifdef STOPONERROR
 *                        return(-1); 
 *#endif
 *
 *                }
 ********* End of Disabled CODE BDS  */

                if (i < 4) {
                        if (i == 0) 
                        if (!(fsr_at_trap & 0x1)) {
                                send_message(0, ERROR, "\nDid not create correct IEEE exception (Inexact): expected = 1, observed = %x\n", fsr_at_trap & 0x1f); 
#ifdef STOPONERROR
                                        return(-1);
#endif
                        }
                        if (i == 1) 
                        if (!(fsr_at_trap & 0x2)) { 
                                send_message(0, ERROR, "\nDid not create correct IEEE exception (Divide By zero) : expected = 2, observed = %x\n", fsr_at_trap & 0x1f);  
#ifdef STOPONERROR
                                        return(-1); 
#endif
                        }
                        if (i == 2) 
                        if (!(fsr_at_trap & 0x8)) { 
                                send_message(0, ERROR, "\nDid not create correct IEEE exception (Overflow) : expected = 8, observed = %x\n", fsr_at_trap & 0x1f);    
#ifdef STOPONERROR
                                        return(-1); 
#endif
                        } 
                        if (i == 3) 
                        if (!(fsr_at_trap & 0x10)) {
                                send_message(0, ERROR, "\nDid not create correct IEEE exception (Invalid) : expected = 10, observed = %x\n", fsr_at_trap & 0x1f); 
#ifdef STOPONERROR
                                        return(-1);  
#endif
                        }
                } 
         
        }
        error_ok = 0;
        clear_regs(0);
        set_fsr(prev_status);
        return(0);
}

/*
 * 	winitfp.c
 */
#ifdef SVR4
void	sigsegv_handler( int sig, siginfo_t *sip, ucontext_t *ucp)
#else SVR4
void	sigsegv_handler( sig, code, scp)
int sig, code ;
struct sigcontext *scp ;
#endif SVR4
{
#ifdef SVR4
	ucp->uc_mcontext.fpregs.fpu_qcnt = 0;
        fsr_at_trap = get_fsr();
        trap_flag = 0x1;
        if (error_ok)
                return;
#endif SVR4
	send_message(1, ERROR, "Sun FPU Reliability Test Failed due to System Segment Violation error.");
}

#ifdef SVR4
void sigfpe_handler( int sig,siginfo_t *sip, ucontext_t *ucp)
#else SVR4
void sigfpe_handler( sig, code, scp)
int sig, code ;
struct sigcontext *scp ;
#endif SVR4

{
        char    bus_msg[100];
        char    *ptr1, *ptr2, *ptr3;
        FILE    *input_file, *fopen();
        int     *ptr4, *ptr5;
        int     bus_errno;

#ifdef SVR4
	ucp->uc_mcontext.fpregs.fpu_qcnt = 0;
#endif SVR4
        fsr_at_trap = get_fsr();
        trap_flag = 0x1;
        if (error_ok)
                return;
	send_message(1, ERROR, "Sun FPU Reliability Test Failed due to fpu bus error.");

}

int winitfp()

/*
 *      Procedure to determine if a physical FPA and 68881 are present and
 *      set fp_state_sunfpa and fp_state_mc68881 accordingly.
 *      Also returns 1 if both present, 0 otherwise.
 */

{
int openfpa, mode81, psr_val;
long *fpaptr ;
                        /* get the psr */

	if (check_fpu())
		return(-1);

#ifdef SVR4
	restore_samask = oldfpu.sa_mask;
        restore_saonstack = oldfpu.sa_flags;
        restore_sahandler = oldfpu.sa_handler;
        sigemptyset(&newfpu.sa_mask);
        newfpu.sa_flags = SA_SIGINFO;
        newfpu.sa_handler = sigfpe_handler ;
        if(sigaction(SIGFPE, &newfpu, &oldfpu)) {
                perror("sigaction SIGFPE");
                exit();
        }
        res_seg_samask = oldseg.sa_mask;
        res_seg_saonstack = oldseg.sa_flags;
        res_seg_sahandler = oldseg.sa_handler;
        sigemptyset(&newseg.sa_mask);
        newseg.sa_flags = SA_SIGINFO;
        newseg.sa_handler = sigsegv_handler;
        if(sigaction(SIGFPE, &newseg, &oldseg)) {
                perror("sigaction SIGFPE");
                exit();
        }

#else SVR4
        restore_svmask = oldfpu.sv_mask;
        restore_svonstack = oldfpu.sv_onstack;
        restore_svhandler = oldfpu.sv_handler;
        newfpu.sv_mask = 0 ;
        newfpu.sv_onstack = 0 ;
        newfpu.sv_handler = sigfpe_handler ;
        sigvec( SIGFPE, &newfpu, &oldfpu ) ; 

        res_seg_svmask = oldseg.sv_mask; 
        res_seg_svonstack = oldseg.sv_onstack; 
        res_seg_svhandler = oldseg.sv_handler; 
        newseg.sv_mask = 0 ; 
        newseg.sv_onstack = 0 ; 
        newseg.sv_handler = sigsegv_handler; 
        sigvec(SIGSEGV, &newseg, &oldseg);

#endif SVR4
        return(0);
}


restore_signals()
{

#ifdef SVR4
        oldseg.sa_mask = res_seg_samask;
        oldseg.sa_flags = res_seg_saonstack;
        oldseg.sa_handler = res_seg_sahandler;
        if(sigaction(SIGFPE, &oldseg, &newseg)) {
                perror("sigaction SIGFPE");
                exit();
        }
        oldfpu.sa_mask = restore_samask;
        oldfpu.sa_flags = restore_saonstack;
        oldfpu.sa_handler = restore_sahandler;
        if(sigaction(SIGFPE, &oldfpu, &newfpu)) {
                perror("sigaction SIGFPE");
                exit();
        }
#else SVR4
        oldseg.sv_mask = res_seg_svmask;
        oldseg.sv_onstack = res_seg_svonstack;
        oldseg.sv_handler = res_seg_svhandler;
        sigvec(SIGSEGV,&oldseg, &newseg);
        oldfpu.sv_mask = restore_svmask;
        oldfpu.sv_onstack = restore_svonstack;
        oldfpu.sv_handler = restore_svhandler;
        sigvec(SIGFPE, &oldfpu, &newfpu);
#endif SVR4
        return(0);
         
}


/*
 *	main.c
 */

fpu_sysdiag() 
{
        int     value, i, val_rotate, j;
        int     val,io_value;
         
        send_message(0,VERBOSE+DEBUG,
			"FPU System (Reliability) Test");

/*
 * check for the fpu chip here 
 * if not there getout quietly
 */
        if (winitfp()) {
                        send_message(0, VERBOSE+DEBUG,
				"        Program Could not talk to FPU chip.\n\
        Check the presence or placement of FPU chip.\n");
                exit(-2);
        }                

		/*
 	 	 *  Identify the installed FPU for the log.
	 	 */
	         if (fpu_is_weitek()) {
		     send_message(0, VERBOSE,
 "FPU: Weitek 1164/1165 Floating Point Controller installed.");
	         } else if (fpu_is_wtl3170()) {
		     send_message(0, VERBOSE,
 "FPU: Weitek WTL3170/2 Floating Point Controller installed.");
	         } else if (fpu_is_ti8847()) {
		     send_message(0, VERBOSE,
 "FPU: TI 74ACT8847 Floating Point Controller installed.");
	             if (fpu_badti())
		         send_message(0, VERBOSE,
 "FPU: (GB* Step) Floating Point Controller installed.");
	         } else if (fpu_is_sunray()) {
		     send_message(0, VERBOSE,
 "FPU: SUN4/400 series - TI8847 Floating Point Controller installed.");
	             if (fpu_badti())
		         send_message(0, VERBOSE,
 "FPU: (GB* Step) Floating Point Controller installed.");
                 } else if ( fpu_is_meiko()) {   
                     send_message(0, VERBOSE,
 "FPU: Meiko LSIL L64804 Floating Point Controller installed.");
                 } else if ( fpu_is_none()) {
                     send_message(0, VERBOSE,
 "FPU: No Floating Point Controller installed.");
                 } 
                send_message(0, VERBOSE+DEBUG, "FSR Register Test.");
                if (fsr_test()) {
                        fail_close(1);
			if (!run_on_error) {
			  	restore_all();
				return(-1);
			}
                }        
                send_message(0, VERBOSE+DEBUG, "Registers Test.");
                if (registers_two()) {
                        fail_close(2);
			if (!run_on_error) {
                                restore_all(); 
                                return(-1);
                        }
                }
                send_message(0, VERBOSE+DEBUG, "Nack Test.");
                if (fpu_nack_test()) {
                        fail_close(3); 
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }       
                send_message(0, VERBOSE+DEBUG,
				"Move Registers Test.");
                if (fmovs_ins()) {
                        fail_close(4); 
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }       
                send_message(0, VERBOSE+DEBUG,
				"Negate: Positive to Negative Test.");
                if (get_negative_value_pn()) {
                        fail_close(5); 
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }       
                send_message(0, VERBOSE+DEBUG,
				"Negate: Negative to Positive Test.");
                if (get_negative_value_np()) {
                        fail_close(6); 
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }       
                send_message(0, VERBOSE+DEBUG, "Absolute Test.");
                if (fabs_ins()) {
                        fail_close(7); 
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }       
                send_message(0, VERBOSE+DEBUG,
		"Integer to Floating Point: Single Precision Test.");
                if (integer_to_float_sp()) {
                        fail_close(8); 
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }       
                send_message(0, VERBOSE+DEBUG,
		"Integer to Floating Point: Double Precision Test.");
                if (integer_to_float_dp()) {
                        fail_close(9); 
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }     
                send_message(0, VERBOSE+DEBUG,
			"Floating to Integer: Single Precision Test.");
                if (float_to_integer_sp()){
                        fail_close(10);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, 
		"Floating to Integer: Double Precision Test.");
                if (float_to_integer_dp()) {
                        fail_close(11);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        

                send_message(0, VERBOSE+DEBUG, 
		"Format Conversion: Single to Double Test."); 
                if (single_doub()){
                        fail_close(14);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, 
		"Format Conversion: Double to Single Test.");
                if (double_sing()){
                        fail_close(15);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, 
		"Addition: Single Precision Test.");
                if (addition_test_sp()){
                        fail_close(16);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Addition: Double Precision Test.");
                if (addition_test_dp()) {
                        fail_close(17);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Subtraction: Single Precision Test.");
                if (subtraction_test_sp()){
                        fail_close(18);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                        return(-1);
                }        
                send_message(0, VERBOSE+DEBUG, "Subtraction: Double Precision Test."); 
                if (subtraction_test_dp()){
                        fail_close(19);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Multiply: Single Precision Test.");
                if (multiplication_test_sp()) {
                        fail_close(20);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Multiply: Double Precision Test.");
                if (multiplication_test_dp()){
                        fail_close(21);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        

		if (!fpu_is_weitek())
                       send_message(0, VERBOSE+DEBUG, "Squareroot: Single Precision Test.");
               	if (squareroot_test_sp()) {
                        fail_close(20);  
                        if (!run_on_error) { 
                               	restore_all();  
                               	return(-1); 
                        }
                }        

		if (!fpu_is_weitek())
                       send_message(0, VERBOSE+DEBUG, "Squareroot: Double Precision Test.");
                if (squareroot_test_dp()){
                       	fail_close(21);  
                       	if (!run_on_error) { 
                               	restore_all();  
                               	return(-1); 
                       	}
                }        

                send_message(0, VERBOSE+DEBUG, "Division: Single Precision Test.");
                if (division_test_sp()){
                        fail_close(22);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Division: Double Precision Test.");
                if (division_test_dp()){
                        fail_close(23);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        

                send_message(0, VERBOSE+DEBUG, "Compare: Single Precision Test."); 
                if (compare_sp()){
                        fail_close(24);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Compare: Double Precision Test.");
                if (compare_dp()) {
                        fail_close(25);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        

                send_message(0, VERBOSE+DEBUG, "Compare And Exception If Unordered: Single Precision Test.");
                if (compare_sp_except()){
                        fail_close(26);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Compare And Exception If Unordered: Double Precision Test.");
                if (compare_dp_except()) {
                        fail_close(27);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Branching On Condition Instructions Test.");

                if (branching()){
                        fail_close(28);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }

                }        

                send_message(0, VERBOSE+DEBUG, "No Branching on Condition Instructions Test.");
                if (no_branching()){
                        fail_close(29);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        

                send_message(0, VERBOSE+DEBUG, "Chaining (Single Precision) Test.");

                if (chain_sp_test()) {
                        fail_close(30);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        

                send_message(0, VERBOSE+DEBUG, "Chaining (Double Precision) Test.");
                if (chain_dp_test()){
                        fail_close(31);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        

		if (fpu_is_weitek()) {
                       send_message(0, VERBOSE+DEBUG, "FPP Status Test."); 
                	if (fpu_ws()){
                       		fail_close(32);  
                       		if (!run_on_error) { 
                			restore_all();  
                               	 	return(-1); 
                        	}
                	}        
		}

		if (fpu_is_weitek()) {
                 send_message(0, VERBOSE+DEBUG, "FPP Status (NXM=1) Test.");
               		if (fpu_ws_nxm()){
                       		fail_close(33);  
                       		if (!run_on_error) { 
                               		restore_all();  
                               		return(-1); 
                       		}
               		}        
		}

                send_message(0, VERBOSE+DEBUG, "Datapath (Single Precision) Test.");
                if (data_path_sp()){
                        fail_close(35);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Datapath (Double Precision) Test.");
                if (data_path_dp()){
                        fail_close(36);  
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }        
                send_message(0, VERBOSE+DEBUG, "Load Test.");
                if (timing_test()) {
                        fail_close(37);
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }

                send_message(0, VERBOSE+DEBUG, "Linpack Test.");
                if (lin_pack_test()) { 
                        fail_close(38);
                        if (!run_on_error) { 
                                restore_all();  
                                return(-1); 
                        }
                }
        restore_signals();
        return(0);
}

fail_close(num)
	int num;
{
	send_message(-SYSTEST_ERROR, FATAL, "Failed systest for FPU.");
}
restore_all()
{
	restore_signals();
}

/*
 * Function:	Decode and display the meaning of the exception bits.
 */
ftt_decode(value)
unsigned long value;
{
    printf("\n  **** ");
    switch((value & 0x0001c000) >> 14)
	{
	case 0 : printf(" No Exception"); break;
	case 1 : printf(" IEEE Execption "); 
    		 switch(value & 0x0000001f) {
			case 0  : printf("(No Cause)"); break;
			case 1  : printf("(Inexact Rounding)"); break;
			case 2  : printf("(Division by 0)"); break;
			case 4  : printf("(Underflow)"); break;
			case 8  : printf("(Overflow)"); break;
			case 10 : printf("(Invalid Operation)"); break;
		 }
		 break;
	case 2 : printf(" Unfinished FPop "); break;
	case 3 : printf(" Unimplimented FPop "); break;
	case 4 : printf(" Sequence Error "); break;
	}
    if ((value & 0x00002000) > 0) {
	printf(" ** Queue not Empty ** ");
	}
    printf("\n");
}

lin_pack_test()
{
	if ( slinpack_test() )
		return -1;
	if ( dlinpack_test() )
		return -1;
	return(0);
}
