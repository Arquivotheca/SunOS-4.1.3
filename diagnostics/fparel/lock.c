/*
 *	"@(#)lock.c 1.1 7/30/92 Copyright Sun Microsystems"; 
 *
 */
#include <sys/types.h>
#include "fpa.h"
#include "fpa3x.h"

u_long dps[] = {

	0xE0000504,	/* dp short add */
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


struct dp_ext 
{
	u_long	addr;
	u_long  addr_lsw;
};

struct dp_ext  ext[] = {

	0xE0001484,   0xE0001800,	/* dp ext divide */
        0xE000148C,   0xE0001808, 
        0xE0001494,   0xE0001810, 
        0xE000149C,   0xE0001818, 
        0xE00014A4,   0xE0001820, 
        0xE00014AC,   0xE0001828, 
        0xE00014B4,   0xE0001830, 
        0xE00014BC,   0xE0001838, 
        0xE00014C4,   0xE0001840, 
        0xE00014CC,   0xE0001848, 
        0xE00014D4,   0xE0001850, 
        0xE00014DC,   0xE0001858, 
        0xE00014E4,   0xE0001860, 
        0xE00014EC,   0xE0001868, 
        0xE00014F4,   0xE0001870, 
        0xE00014FC,   0xE0001878 
};

u_long nxt_cmd[] = {

	0x000,		/* non numbers */
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

u_long cmd[] = {
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

/* ***************
 * Local Globals
 * ***************/

u_long  res_a_msw, res_a_lsw, res_n_msw, res_n_lsw;
u_long  res_i_msw, res_i_lsw;
u_long  res0_msw, res0_lsw, shad_res_msw, shad_res_lsw;
u_long  value_i, value_i_lsw, value_0, value_0_lsw, value_00, value_00_lsw;

lock_test()
{

	/* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;

	*(u_long *)FPA_IMASK_PTR = 0x1;
        if (next_dp_short_test())
                return(-1);
	if (dp_short_test())
		return(-1);
	if (dp_ext_test())
		return(-1);
	if (dp_cmd_test())
		return(-1);	
	if (next_dp_short_test())
		return(-1);
	if (next_dp_ext_test())
		return(-1);
	if (next_dp_cmd_test())
		return(-1);
        *(u_long *)FPA_IMASK_PTR = 0x0;

	return(0);
}

next_dp_short_test()
{
        int     i, j;
        u_long  *reg0_addr, *reg0_addr_lsw;
	u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw; 
	u_long	*shadow_j, *shadow_j_lsw;
 
	clear_user_regs(16);

        res_a_msw    = 0x3FD55555; 	/* active result - dp 0.33333333333 */
        res_a_lsw    = 0x55555555;
	res_n_msw    = 0x3FE55555; 	/* next result - dp 0.6666666666 */
	res_n_lsw    = 0x55555555;
	value_0      = 0x3FF00000;
	value_0_lsw  = 0x0;
	value_i      = 0x40000000;
	value_i_lsw  = 0x0;
	value_00     = 0x3FCC71C7;	/* computed value: reg under test = 0 */
	value_00_lsw = 0x1C71C71C;

        reg0_addr = (u_long *)dps[0];     	/* active inst using reg0 */
        reg0_addr_lsw = (u_long *)0xE0001000;  

        for (i = 0; i < 16; i++)
        {

                 reg_i_addr = (u_long *)dps[i]; 	/* next inst using */
                 reg_i_addr_lsw = (u_long *)0xE0001000; /* reg under test */
		 reg_i = (u_long *)INS_REGFC_M(i);
		 reg_i_lsw = (u_long *)INS_REGFC_L(i);
                 for (j = 0; j < 8; j++)
                 {

					/* reg0 = dp 1 - active stage */
			*(u_long *)INS_REGFC_M(0) = value_0;
			*(u_long *)INS_REGFC_L(0) = 0x0;
			*reg_i = value_i; /* reg i = dp 2 - next stage */
			*reg_i_lsw = 0x0;

        /* Pipe will hang due to inexact error from calculation. */

			*(u_long *)FPA_IMASK_PTR = 0x1;/* enable inexact errs */
                        *reg0_addr = 0x40080000;  /* operand = dp 3 - active */
                        *reg0_addr_lsw = 0x0;
			*reg_i_addr = 0x40080000; /* operand = dp 3 - next */
                        *reg_i_addr_lsw = 0x0;
		
        /* Bus error occurs if shadow register accessed corresponds to */
        /* destination register of instructions in pipeline. Driver catches */
        /* bus error and forces re-calculation to obtain result in */
        /* register. */

			shad_res_msw = *(u_long *)SHAD_REG_M(j);
			shad_res_lsw = *(u_long *)SHAD_REG_L(j);

        /* Pipe needs to be cleared if shadow register accessed does not */
        /* correspond to any destination register.  Calculated result will */
        /* not appear in destination register. */

			*(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;

                        res_i_msw = *reg_i; 	  /* read result from reg  i*/
                        res_i_lsw = *reg_i_lsw;
			res0_msw = *(u_long *)INS_REGFC_M(0);
			res0_lsw = *(u_long *)INS_REGFC_L(0);

			*(u_long *)FPA_IMASK_PTR = 0x0;	/* disable */

			if (det_lock_res(i,j) != 0)
			    return(-1);
			
			*(u_long *)INS_REGFC_M(0)= 0x0;
			*(u_long *)INS_REGFC_L(0) = 0x0;
			*reg_i = 0x0;
			*reg_i_lsw = 0x0;
		}
        }
        return(0);

}
next_dp_ext_test()
{
        int     i, j;
        u_long  *reg0_addr, *reg0_addr_lsw;
        u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw;
        u_long  *shadow_j, *shadow_j_lsw;

	clear_user_regs(16);

        res_a_msw    = 0x3FD55555; /* active result - dp 0.33333333333 */
        res_a_lsw    = 0x55555555;
        res_n_msw    = 0x3FE55555; /* next result - dp 0.6666666666 */
        res_n_lsw    = 0x55555555;
        value_0      = 0x3FF00000;
	value_0_lsw  = 0x0;
        value_i      = 0x40000000;
	value_i_lsw  = 0x0;
	value_00     = 0x3FCC71C7;
	value_00_lsw = 0x1C71C71C;

        reg0_addr = (u_long *)ext[0].addr; 	/* active inst using reg0 */
	reg0_addr_lsw = (u_long *)ext[0].addr_lsw; 

        for (i = 0; i < 16; i++)
        {

                 reg_i_addr = (u_long *)ext[i].addr; 	/* next inst using */
                 reg_i_addr_lsw = (u_long *)ext[i].addr_lsw;/* reg under test */
		 reg_i = (u_long *)INS_REGFC_M(i);
		 reg_i_lsw = (u_long *)INS_REGFC_L(i);
                 for (j = 0; j < 8; j++)
                 {

					/* reg0 = dp 1 - active stage */
			*(u_long *)INS_REGFC_M(0) = 0x3FF00000;
			*(u_long *)INS_REGFC_L(0) = 0x0;
                        *reg_i = 0x40000000; /* reg i = dp 2 - next stage */
                        *reg_i_lsw = 0x0;

        /* Pipe will hang due to inexact error from calculation. */

			*(u_long *)FPA_IMASK_PTR = 0x1;/* enable inexact errs */

                        *reg0_addr = 0x40080000;  /* operand = dp 3 - active */
                        *reg0_addr_lsw = 0x0;
                        *reg_i_addr = 0x40080000; /* operand = dp 3 - next */
                        *reg_i_addr_lsw = 0x0;

        /* Bus error occurs if shadow register accessed corresponds to */
        /* destination register of instructions in pipeline. Driver catches */ 
        /* bus error and forces re-calculation to obtain result in */
        /* register. */

			shad_res_msw = *(u_long *)SHAD_REG_M(j);
			shad_res_lsw = *(u_long *)SHAD_REG_L(j);

        /* Pipe needs to be cleared if shadow register accessed does not */
        /* correspond to any destination register.  Calculated result will */
        /* not appear in destination register. */

                        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;

                        res_i_msw = *reg_i; 	  /* read result from reg  i*/
                        res_i_lsw = *reg_i_lsw;
                        res0_msw = *(u_long *)INS_REGFC_M(0);
                        res0_lsw = *(u_long *)INS_REGFC_L(0);

			*(u_long *)FPA_IMASK_PTR = 0x0;	/* disable */

			if (det_lock_res(i,j) != 0)
				return(-1);

                        *(u_long *)INS_REGFC_M(0) = 0x0;
                        *(u_long *)INS_REGFC_L(0) = 0x0;
                        *reg_i = 0x0;
                        *reg_i_lsw = 0x0;
                }
        }
        return(0);

}
next_dp_cmd_test()
{
        int     i, j;
        u_long  *reg0_addr, *reg0_addr_lsw;
        u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw;
        u_long  *shadow_j, *shadow_j_lsw;

	clear_user_regs(32);

        res_a_msw    = 0x1; /* active result - dp 0.33333333333 */
        res_a_lsw    = 0x0;
        res_n_msw    = 0x2; /* next result - dp 0.6666666666 */
        res_n_lsw    = 0x0;
        value_0      = 0x1;
	value_0_lsw  = 0x0; 
        value_i      = 0x2;
	value_i_lsw  = 0x0;
	value_00     = value_i;
	value_00_lsw = value_i_lsw;

	reg0_addr = (u_long *)0xE0000AC4;   /* active inst */
	reg_i_addr =(u_long *)0xE0000AC4;   /* weitek direct dp add: op + 0 */

        for (i = 0; i < 31; i++)
        {

		 reg_i = (u_long *)INS_REGFC_M(i);
		 reg_i_lsw = (u_long *)INS_REGFC_L(i);
                 for (j = 0; j < 8; j++)
                 {
 
					/* reg0 = dp 1 - active stage */
                        *(u_long *)INS_REGFC_M(0) = value_0;  
                        *(u_long *)INS_REGFC_L(0) = 0x0;  
                        *reg_i = value_i; /* reg i = dp 2 - next stage */
                        *reg_i_lsw = 0x0;

	/* Pipe will hung due to inexact error from calculation. */

			*(u_long *)FPA_IMASK_PTR = 0x1;/* enable inexact errs */

                        *reg0_addr = nxt_cmd[0]; /* nota # + 0 weitek op */

                        *reg_i_addr = nxt_cmd[i]; /* nota # + 0 weitek op */


	/* Bus error occurs if shadow register accessed corresponds to */
	/* destination register of instructions in pipeline.  Driver catches */
	/* bus error and forces re-calculation to obtain result in */
	/* register. */

			shad_res_msw = *(u_long *)SHAD_REG_M(j);
			shad_res_lsw = *(u_long *)SHAD_REG_L(j);

	/* Pipe needs to be cleared if shadow register accessed does not */
	/* correspond to any destination register.  Calculated result will */
	/* not appear in destination register. */

                        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;

                        res_i_msw = *reg_i; 	  /* read result from reg  i*/
                        res_i_lsw = *reg_i_lsw;
                        res0_msw = *(u_long *)INS_REGFC_M(0);
                        res0_lsw = *(u_long *)INS_REGFC_L(0);

			*(u_long *)FPA_IMASK_PTR = 0x0;	/* disable */

			if (det_lock_res(i,j) != 0)
				return(-1);

                        *(u_long *)INS_REGFC_M(0) = 0x0;
                        *(u_long *)INS_REGFC_L(0) = 0x0;
                        *reg_i = 0x0;
                        *reg_i_lsw = 0x0;
                }
        }
        return(0);

}

det_lock_res(i,j)
int i;
int j;
{

	if ((j == 0) && (j == i)) {	/* 2 div 9 */
		if ((shad_res_msw != value_00) ||
		    (res_i_msw!= value_00) ||
		    (res0_msw != value_00))
	       		return(-1); 
		if ((shad_res_lsw != value_00_lsw) ||
                    (res_i_lsw != value_00_lsw) ||
                    (res0_lsw != value_00_lsw))
                        return(-1);
	}

	else if ((j == 0) && (j != i)) {
		if ((shad_res_msw != res_a_msw) || 
		    (res_i_msw != res_n_msw) 
		    || (res0_msw != res_a_msw))
                	return(-1); 
		if ((shad_res_lsw != res_a_lsw) ||
                    (res_i_lsw != res_n_lsw)
                    || (res0_lsw != res_a_lsw))
                        return(-1);
	}

	else if((j != 0) && (j == i)) {
		if ((shad_res_msw != res_n_msw) || 
		    (res_i_msw != res_n_msw) 
		    || (res0_msw != res_a_msw))
              		return(-1);
		if ((shad_res_lsw != res_n_lsw) ||
                    (res_i_lsw != res_n_lsw)
                    || (res0_lsw != res_a_lsw))
                        return(-1);
        }

	else if ((j != 0) && (j != i)) {
		if (i == 0) {
			if ((shad_res_msw != 0x0)||
			    (res_i_msw != value_i) ||
			    (res0_msw != value_i)) 
	       			return(-1);  
			if ((shad_res_lsw != 0x0)||
			    (res_i_lsw != value_i_lsw) ||
			    (res0_lsw != value_i_lsw))
                                return(-1);
                }
		else {
			if ((shad_res_msw != 0x0) || 
		    	    (res_i_msw != value_i) || 
		    	    (res0_msw != value_0))
	                	return(-1);    
			if ((shad_res_lsw != 0x0) ||
                            (res_i_lsw != value_i_lsw) ||
                            (res0_lsw != value_0_lsw))
                                return(-1);
		}
        }  	
	return(0);
}

dp_short_test()
{
	u_long	i, j;
	u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw; 
	
	clear_user_regs(16);

	value_i     = 0x3FD55555; /* result = 0.3333333333333333 */
	value_i_lsw = 0x55555555;

	for (i = 0; i < 8; i++)
	{
                reg_i_addr = (u_long *)dps[i];    /* dp short divide */
                reg_i_addr_lsw = (u_long *)0xE0001000; /* w/ reg under test */
		reg_i = (u_long *)INS_REGFC_M(i);      /* reg under test */
		reg_i_lsw = (u_long *)INS_REGFC_L(i);

		for (j = 0; j < 8; j++)
		{

			*reg_i     = 0x3FF00000; /* reg i = dp 1 */
	                *reg_i_lsw = 0x0;

        /* Pipe will hang due to inexact error from calculation. */

			*(u_long *)FPA_IMASK_PTR = 0x1;/* enable inexact errs */

	                *reg_i_addr = 0x40080000; /* operand = dp 3 */ 
	                *reg_i_addr_lsw  = 0x0;

        /* Bus error occurs if shadow register accessed corresponds to */
        /* destination register of instruction in pipeline. Driver catches */ 
        /* bus error and forces re-calculation to obtain result in */
        /* register. */

			shad_res_msw = *(u_long *)SHAD_REG_M(j);
			shad_res_lsw = *(u_long *)SHAD_REG_L(j);

        /* Pipe needs to be cleared if shadow register accessed does not */
        /* correspond to destination register.  Calculated result will */
        /* not appear in destination register. */

			*(u_long *)FPA_CLEAR_PIPE_PTR = 0x0; 

			res_i_msw = *reg_i; /* read the result from the reg */
			res_i_lsw = *reg_i_lsw;

			*(u_long *)FPA_IMASK_PTR = 0x0;	/* disable */

			if (i == j) {
				if ((value_i != res_i_msw) || 
				    (value_i != shad_res_msw))
					return(-1); 
				if ((value_i_lsw != res_i_lsw) ||
                                    (value_i_lsw != shad_res_lsw))
                                        return(-1);
			}
			if (i != j) {	
				if ((shad_res_msw != 0x0) || 
				    (res_i_msw != 0x3FF00000)) 
					return(-1); 
				if ((shad_res_lsw != 0x0) ||
                                    (res_i_lsw != 0x0))
                                        return(-1);
			}

		}
		*reg_i = 0x0;
		*reg_i_lsw = 0x0;
	}
	return(0);
}			

dp_ext_test()
{
        u_long  i, j;
        u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw;

	clear_user_regs(16);

        value_i     = 0x3FD55555; /* the result is always dp value 0.33333333 */
        value_i_lsw = 0x55555555;

        for (i = 0; i < 16; i++)
        {
		reg_i_addr = (u_long *)ext[i].addr;
		reg_i_addr_lsw = (u_long *)ext[i].addr_lsw;
		reg_i = (u_long *)INS_REGFC_M(i);
		reg_i_lsw = (u_long *)INS_REGFC_L(i);

                for (j = 0; j < 8; j++)
                {
                        *reg_i = 0x3FF00000; /* reg i = dp 1 */
                        *reg_i_lsw = 0x0;

        /* Pipe will hang due to inexact error from calculation. */

			*(u_long *)FPA_IMASK_PTR = 0x1;/* enable inexact errs */

                        *reg_i_addr = 0x40080000; /* operand = dp 3 */ 
                        *reg_i_addr_lsw = 0x0;

        /* Bus error occurs if shadow register accessed corresponds to */
        /* destination register of instructions in pipeline. Driver catches */ 
        /* bus error and forces re-calculation to obtain result in */
        /* register. */

			shad_res_msw = *(u_long *)SHAD_REG_M(j);
			shad_res_lsw = *(u_long *)SHAD_REG_L(j);

        /* Pipe needs to be cleared if shadow register accessed does not */
        /* correspond to destination register.  Calculated result will */
        /* not appear in destination register. */

                        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;

                        res_i_msw = *reg_i; /* read the result from the reg */
                        res_i_lsw = *reg_i_lsw;

			*(u_long *)FPA_IMASK_PTR = 0x0;	/* disable */
			
			if (i == j) {
                        	if ((value_i != res_i_msw) && 
				    (value_i != shad_res_msw))
					   return(-1);
				if ((value_i_lsw != res_i_lsw) &&
                                    (value_i_lsw != shad_res_lsw))
                                           return(-1);
                        }
                        else  { 
				if ((shad_res_msw != 0x0) || 
				    (res_i_msw != 0x3FF00000))
				      return(-1);
				if ((shad_res_lsw != 0x0) ||
                                    (res_i_lsw != 0x0))
                                      return(-1);
                        }

                }
                *reg_i = 0x0;
                *reg_i_lsw = 0x0;
        }
        return(0);
}                        

dp_cmd_test()
{
        u_long  i, j, k, l;
        u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw;
	u_long  *reg_k, *reg_k_lsw, *reg_l, *reg_l_lsw;

	clear_user_regs(32);

        value_i     = 0x3FD55555; /* result = dp 0.3333333333 */
        value_i_lsw = 0x55555555;

        for (i = 0; i < 30; i++)
        {
                reg_i_addr = (u_long *)0xE0000A34; /* dp div w/ weitek spec */ 
		reg_i = (u_long *)INS_REGFC_M(i);
		reg_i_lsw = (u_long *)INS_REGFC_L(i);
		k = i + 1;
		l = i + 2;
		reg_k = (u_long *)INS_REGFC_M(k);
		reg_k_lsw = (u_long *)INS_REGFC_L(k);
		reg_l = (u_long *)INS_REGFC_M(l);
		reg_l_lsw = (u_long *)INS_REGFC_L(l);
		
                for (j = 0; j < 8; j++)
                {
                        *reg_k = 0x3FF00000; /* register has dp value 1 */
			*reg_k_lsw = 0x0;
			*reg_l = 0x40080000; /* register has dp value 3 */
			*reg_l_lsw = 0x0;

        /* Pipe will hang due to inexact error from calculation. */

			*(u_long *)FPA_IMASK_PTR = 0x1;/* enable inexact errs */

                        *reg_i_addr = cmd[i]; 

        /* Bus error occurs if shadow register accessed corresponds to */
        /* destination register of instructions in pipeline. Driver catches */ 
        /* bus error and forces re-calculation to obtain result in */
        /* register. */

			shad_res_msw = *(u_long *)SHAD_REG_M(j);
			shad_res_lsw = *(u_long *)SHAD_REG_L(j);

        /* Pipe needs to be cleared if shadow register accessed does not */
        /* correspond to destination register.  Calculated result will */
        /* not appear in destination register. */

                        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;

                        res_i_msw = *reg_i; /* read the result from the reg */
                        res_i_lsw = *reg_i_lsw;

			*(u_long *)FPA_IMASK_PTR = 0x0;	/* disable */
			
			if (i == j) {
	                        if ((value_i != res_i_msw) || 
				    (value_i != shad_res_msw))
				    return(-1);
				if ((value_i_lsw != res_i_lsw) || 
                                    (value_i_lsw != shad_res_lsw))
                                    return(-1);
                        }
                        else if (j == k) {
				if ((res_i_msw != 0x0) || 
				    (shad_res_msw != 0x3FF00000)) 
			            return(-1);
				if ((res_i_lsw != 0x0) ||
                                    (shad_res_lsw != 0x0))
                                    return(-1);
			}
			else if (j == l) {
				if ((res_i_msw != 0x0) || 
				    (shad_res_msw != 0x40080000))  
			            return(-1); 
				if ((res_i_lsw != 0x0) ||
                                    (shad_res_lsw != 0x0))
                                    return(-1);
                        } 
			*reg_i = 0x0;
			*reg_i_lsw = 0x0;
                }
        }
        return(0);
}                        


clear_user_regs(reg_cnt)
int reg_cnt;		/* num of user regs to clear */
{
	int i;

        for (i = 0; i < reg_cnt; i++)
        {
                *(u_long *)INS_REGFC_M(i) = 0x0;
                *(u_long *)INS_REGFC_L(i) = 0x0; 
        }
}

