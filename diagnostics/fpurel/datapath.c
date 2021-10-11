/*
 *"%Z%%M% %I% %G%  Copyright Sun Microsystems";
 */

/*
 * 	The following routine for checking the data path between registers and
 *	memory and between memory and an floating register, and between the
 *	floating register and the weitek chips.  All the bits are covered including
 *	the sign bit.
 *
 *	It works like this :
 *		f0 = value
 *		f1 = 0
 *	add  =  f4 = f0 + f1
 *
 *		for multiply
 *		f2 =1 
 *	mult =  f8 = f0 * f2
 *
 *		DOUBLE PRECISION
 *		f0 = msw of value
 *		f1 = lsw of value
 *		f2 = 0
 *		f3 = 0
 *	add =   f4 = f0 + f2
 *
 *		f6 = msw of 1
 *		f7 = lsw of 1
 *	mult =  f8 = f0 * f6
 */


unsigned long	result_lsw, result_msw;
extern int verbose_mode;
				
data_path_sp()
{
	int	i, j, k ;
	unsigned long result, value;
	
	clear_regs(0);
	for (i = 0; i < 2; i++) { /* sign bit */
		for (j = 1; j < 255; j++) {
			for (k = 0; k < 23; k++) {
				value = (i << 31) | (j << 23) | (1 << k);

				if (result = datap_add(value)) {
				   if (verbose_mode)
					printf("\nAdd SP failed: expected / read = %x / %x\n",
						value, result);
					return(-1);
				}
				if (result = datap_mult(value)) {
					if (verbose_mode)
					printf("\nMultiply SP failed:expected / read = %x / %x\n", 
                                                value, result); 
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
				  if (verbose_mode) { 
                                        printf("\nAdd DP failed: msw : expected / read = %x / %x\n", 
                                                value_msw, result_msw);
					printf("               lsw : expected / read = %x / %x\n",   
                                                value_lsw, result_lsw);	
				  }
					return(-1);
				}

				if (datap_mult_dp(value_msw, value_lsw)){ 
					if (verbose_mode) { 
                                        printf("\nMultiply DP failed: msw : expected / read = %x / %x\n",   
                                                value_msw, result_msw);              
                                        printf("               lsw : expected / read = %x / %x\n",     
                                                value_lsw, result_lsw);      
					}
					return(-1);
                                }
			}
		}
	}
	return(0);
}


/*
 *
 *	The following test does 10 add operation continuously,
 *		10 multiply operations continusously and
 *	see any thing goes wrong.
 */	
timing_test() 
{
	int	i;
	unsigned long result, value_lsw;

	for (i = 0; i < 1000; i++) {
		clear_regs(0);
		if (result = timing_add_sp()) {
			if (verbose_mode)
			printf("\nSingle Precision: add, expected / observed = 0x41200000 / 0x%x\n", result);	
			return(-1);
		}
		clear_regs(0);
		if (result = timing_mult_sp()) {
			if (verbose_mode)
			printf("\nSingle Precision: Multiply, expected / observed = 0x43470000 / 0x%x\n",
					result);
printf("i = %d\n", i);
			return(-1);
		}
		clear_regs(0);
		if (result = timing_add_dp(&value_lsw)){
		  if (verbose_mode){
			printf("\nDouble Precision: Add, MSW : expcected / observed = 0x40240000 / 0x%x\n",
					result);
			printf("                       LSW : expected / observed = 0x0 / 0x%x\n",
					value_lsw);
		   }
			return(-1);
		}
		clear_regs(0);
		if (result = timing_mult_dp(&value_lsw)) {
			if (verbose_mode) {
			printf("\nDouble Precision: Multiply, MSW : expcected / observed = 0x4034000 / 0x%x\n",
					result);
			printf("                            LSW : expected / observed = 0x0 / 0x%x\n", 
                                        value_lsw);  
			}
			return(-1);
		}

	}
	return(0);
}


chain_sp_test()
{
	int	i, result;


	clear_regs(0);
	set_fsr(0);
	for (i = 1; i < 60; i++) {

		if ((result = chain_sp(i)) != i) {
			if (verbose_mode)
			printf("\nError: expected / observed = %x / 0x%x\n", i, result);
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
			if (verbose_mode)
                        printf("\nError: expected / observed = %x / 0x%x\n", i, result); 
                                return(-1);    
                }      
        }      
        return(0);     
}
		
	

