
/*
 *"@(#)wstatus.c 1.1 7/30/92  Copyright Sun Microsystems";
 */

/*
 * This test causes the Weitek chips to produce every possible
 * value at the S+ outputs.  These bits are then observed via
 * the WSTATUS register.  The ALU and multiplier are distinguished 
 * by the instruction address
 */
extern	unsigned long	wsadd_addr, wdadd_addr, wsdiv_addr, wddiv_addr, wsmult_addr;
extern  unsigned long   wdmult_addr, fp_addr;

extern  unsigned long    actual_addr;
extern  unsigned long	trap_flag;
extern  unsigned long   fsr_data;
extern  unsigned long   fsr_at_trap;
extern int verbose_mode;


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
#define    nocare  0

#define	   add_sp  1
#define    add_dp  2
#define    div_sp  3
#define    div_dp  4
#define    mult_sp 5
#define    mult_dp 6

struct testws {

     unsigned long   a_msw;
     unsigned long   a_lsw;
     unsigned long   b_msw;
     unsigned long   b_lsw;
     unsigned long   instr;
     unsigned long   status;
};

 
struct testws test_ws[] = {
  /* a_msw     a_lsw     b_msw     b_lsw    instr   status  */
  	/* with nxm = 1 */
   {one_sp,   nocare,   maxm_sp,  nocare,   add_sp,  0x4},/* inexact  */
      /*with nxm = 0 or 1 does not care */
   {one_sp,   nocare,   zero_sp,  nocare,   div_sp,  0x4},/* div/zero */
   {maxm_sp,  nocare,   maxm_sp,  nocare,   mult_sp, 0x4},/* overflow */
   {zero_sp,  nocare,   zero_sp,  nocare,   div_sp,  0x4},/* not valid */
   {maxn_sp,  nocare,   maxn_sp,  nocare,   add_sp,  0x4},/* 5 -ovfl,inex*/   
   {maxn_sp,  nocare,   maxn_sp,  nocare,   mult_sp, 0x4},/* 5- ovfl,inex*/   
   {maxn_msw, maxn_lsw, maxn_msw, maxn_lsw, mult_dp, 0x4},
   {one_msw,  one_lsw,  zero_msw, zero_lsw, div_dp,  0x4}, 
/* {two_sp,   nocare,   maxd_sp,  nocare,   div_sp,  0x8}, */  /* 9-b-denorm*/
/* {one_msw,  one_lsw,  maxd_msw, maxd_lsw, div_dp,  0x8}, */
   {one_sp,   nocare,   nn_sp,    nocare,   add_sp,  0x8},
   {one_msw,  one_lsw,  nn_msw,   nn_lsw,   add_dp,  0x8},
   {one_sp,   nocare,   nn_sp,    nocare,   mult_sp, 0x8},
   {one_msw,  one_lsw,  nn_msw,   nn_lsw,   mult_dp, 0x8},
/* {minn_sp,  nocare,   two_sp,   nocare,   div_sp,  0x8}, */  /* 6 -unfl*/
/* {minn_msw, minn_lsw, two_msw,  two_lsw,  div_dp,  0x8},  */ 
   {maxd_sp,  nocare,   two_sp,   nocare,   div_sp,  0x8},/* 8-a-denorm*/
   {maxd_msw, maxd_lsw, two_msw,  two_lsw,  div_dp,  0x8},
/* {minn_sp,  nocare,   half_sp,  nocare,   mult_sp, 0x8}, */ /* 6-unfl*/
/* {minn_msw, minn_lsw, half_msw, half_lsw, mult_dp, 0x8}, */
   {min1_sp,  nocare,   pi_4_sp,  nocare,   mult_sp, 0x8},/* 7-unfl,inex*/
   {maxd_sp,  nocare,   half_sp,  nocare,   mult_sp, 0x8},/* 8 -a-denorm*/
   {maxd_msw, maxd_lsw, half_msw, half_lsw, mult_dp, 0x8},
   {half_sp,  nocare,   maxd_sp,  nocare,   mult_sp, 0x8},/* 9 -b-denorm*/
   {half_msw, half_lsw, maxd_msw, maxd_lsw, mult_dp, 0x8},
   {min1_msw, min1_lsw, pi_4_msw, pi_4_lsw, mult_dp, 0x8},
   {nan_sp,   nocare,   zero_sp,  nocare,   add_sp,  0x8},/* 12-a-nan*/
   {nan_msw,  nan_lsw,  zero_msw, zero_lsw, add_dp,  0x8}, 
   {zero_sp,  nocare,   nan_sp,   nocare,   add_sp,  0x8},/* 13 -b-nan*/
   {zero_sp,  nocare,   nan_msw,  nan_lsw,  add_dp,  0x8},
   {nan_sp,   nocare,   nan_sp,   nocare,   add_sp,  0x8},/* 14 -ab-nan*/
   {nan_msw,  nan_lsw,  nan_msw,  nan_lsw,  add_dp,  0x8},
   {nan_sp,   nocare,   zero_sp,  nocare,   mult_sp, 0x8},/* 11-a-nan*/
   {nan_msw,  nan_lsw,  zero_msw, zero_lsw, mult_dp, 0x8}, 
   {zero_sp,  nocare,   nan_sp,   nocare,   mult_sp, 0x8},/* 13-b-nan*/
   {zero_sp,  nocare,   nan_msw,  nan_lsw,  mult_dp, 0x8},
   {nan_sp,   nocare,   nan_sp,   nocare,   mult_sp, 0x8},/* 14-ab-nan*/
   {nan_msw,  nan_lsw,  nan_msw,  nan_lsw,  mult_dp, 0x8},
   {     00,       00,      000,      000,    0000,  0x0}
};

extern  int     donot_dq; 
extern  unsigned long error_ok;

fpu_ws()
{
	int	operation;
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

		trap_flag = 0x0;		/* unset the flag */	
		amsw = test_ws[i].a_msw;	/* get most significant word */
		alsw = test_ws[i].a_lsw;	/* get the least "      "  */
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
		if (i) {
		    if (!trap_flag) {
			if (verbose_mode)
			printf(" FPU Trap did not occur , i = %d.\n", i);

				return(-1);

		    }
		}
		else {
		    if (trap_flag) {
			if (verbose_mode)
			printf(" FPU Trap Should not occur but occured.\n");
                                return(-1);
                    }
                } 
		status = fsr_at_trap & 0x1c000;
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
int     operation;
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
         
		trap_flag = 0x0;		/* unset the trap flag */
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
			printf("Bus Error did not occur.\n");
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
                                printf("\nDid not create correct IEEE exception (Inexact): expected = 1, observed = %x\n", fsr_at_trap & 0x1f); 
                                        return(-1);
                        }
                        if (i == 1) 
                        if (!(fsr_at_trap & 0x2)) { 
                                printf("\nDid not create correct IEEE exception (Divide By zero) : expected = 2, observed = %x\n", fsr_at_trap & 0x1f);  
                                        return(-1); 
                        }
                        if (i == 2) 
                        if (!(fsr_at_trap & 0x8)) { 
                                printf("\nDid not create correct IEEE exception (Overflow) : expected = 8, observed = %x\n", fsr_at_trap & 0x1f);    
                                        return(-1); 
                        } 
                        if (i == 3) 
                        if (!(fsr_at_trap & 0x10)) {
                                printf("\nDid not create correct IEEE exception (Invalid) : expected = 0x10, observed = %x\n", fsr_at_trap & 0x1f); 
                                        return(-1);  
                        }
                } 
         
        }
	error_ok = 0;
	clear_regs(0);
        set_fsr(prev_status);
	return(0);
}


fpu_lock()
{
        int     operation;
        unsigned long i, cmp_status, tmp_var, amsw, alsw, bmsw, blsw;
        unsigned long *ptr, status;
	unsigned long tmp_val;
 

	status = get_fsr();
        status = status | 0xF800000;
        set_fsr(status);
        donot_dq = 0;
	error_ok = 1;

        for (i = 0; test_ws[i].instr != 0; i++) {
		trap_flag = 0x0;		/* unset the trap flag */         
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
			printf("\nBus error did not occur.\n");
				return(-1);
		}

		status = fp_addr;
		if (status != actual_addr) {
			printf("\nFailed: address expected / observed = %x / %x\n",
					actual_addr, status);
			set_fsr(0);
				return(-1);
		}

	}
	error_ok = 0;
	return(0);
}
	
