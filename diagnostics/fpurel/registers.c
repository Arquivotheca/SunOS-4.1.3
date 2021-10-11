/*
 *"@(#)registers.c 1.1 7/30/92  Copyright Sun Microsystems";
 */

#include "fpu.h"

unsigned long  pat[] = {
        0x00000000,
        0x55555555,
        0xAAAAAAAA,
        0xCCCCCCCC,
        0xFFFFFFFF
};

static unsigned long fsr_testdata[] = {
	0x08000000,
	0x40000000,
	0x48000000,
	0x80000000,
	0x88000000,
	0xC0000000,
	0xC8000000
};

extern int	reg1m, reg1l, reg2m, reg2l;


extern int  verbose_mode;

/*
 *	The following routine writes the given patterns to all the registers
 *	and reads it back.
 */
/*
 *	The following routine tests all the possible values given for single 
 *	precision numbers.
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
                            printf("\nregister read/write failed : reg = %d, expected / observed = %x / %x\n",
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
 *	The following routine tests rotating ones to the registers
 */ 
registers_two()
{
	int	i,j;
	unsigned long result, value;

	for (j = 0; j < 32; j++) {
	
		for (i = 0; i < 32; i++) {
			value = (1 << i);
			if ((result = register_test(j, value)) != value) {
			if (verbose_mode)
		   printf("\nregister test-2 read/write failed : reg = %d, expected / observed = %x / %x\n",
				j, value, result);
				return(-1);
			}
		}
	}
	return(0);
}
/*DEAD CODE
 *registers_two()
 *{
 *	int     i,j;
 *        unsigned long result, value;
 * 
 * for (j = 0; j < 0x7fffffff; j++) {      
 *	for (i = 0; i < 32; i++) {
 * 
 *		simpletest(0xaaaaaaaa);
 *		if ((result = register_test(i, 0xffffffff)) != 0xffffffff) {
 *		printf("Register Test failed: background = 0xaaaaaaaa, reg = %d, written / read = 0xffffffff / %x\n",
 *					i, result);
 *		return(-1);
 *		}
 *	}	
 *		
 *	for (i = 0; i < 32; i++) { 
 * 
 *                simpletest(0xffffffff);
 *		if ((result = register_test(i, 0xaaaaaaaa)) != 0xaaaaaaaa) {
 *		printf("Register Test failed: background = 0xffffffff, reg = %d, written / read = 0xaaaaaaaa / %x\n",  
 *                                        i, result);    
 *                return(-1);
 *                } 
 *        }
 * }
 *	return(0);
 *}
 *
 *registers_two()
 *{
 *	int     i,j;
 *        unsigned long result, value;
 *
 *	for (i = 0; i < 32; i++) {
 *                        value = (1 << i);
 *                        if ((result = register_test(0, value)) != value) {
 *                   printf("\nregister test-2 read/write failed : reg = 0, expected / observed = %x / %x\n",
 *                                 value, result);
 *                                return(-1);
 *                        }
 *			
 *                }
 *	for (i = 0; i < 32; i++) { 
 *                        value = (1 << i); 
 *                        if ((result = register_test(1, value)) != value) { 
 *                   printf("\nregister test-2 read/write failed : reg =1, expected / observed = %x / %x\n",
 *                                 value, result);  
 *                                return(-1); 
 *                        } 
 *                         
 *                } 
 *	return(0);
 *}
 */	
others_two()
{
	int 	i, value;
	
	for (i = 0; i < 0x7fffffff; i++) {

		if ( value = simple3(i, i)) {

			printf("failed: value = %d, i = %x, reg1msw = %x, reg1lsw = %x\n", value, i, reg1m,reg1l);
			return(-1);
		}
	}
	return(0);
}

	
	
/*
 *	The following routine tests with the patttern given
 */
registers_one()
{
        int     i,j, result;

        for (i = 0; i < 32; i++) {
                for (j = 0; j < 5; j++) {
                        result = register_test(i, pat[j]);
                        if (result != pat[j]) {
                          printf("\nregister read/write failed : reg = %d, expected / observed = %x / %x\n",
                                        i, pat[j], result);
                         return(-1);
                        }
                }
        }
        return(0);
}  
/*
 *	The following routine is for the user given pattern
 */
registers_four()
{
	int	i;
	unsigned long	pattern, result; 
	int	pass, reg_no;

	for (i = 0; i < pass; i++) {

		result = register_test(reg_no, pattern);
		if (result != pattern) {
			printf("Register write/read failed : reg = %d, expected / observed = %x / %x\n",
					reg_no, pattern, result);
                        return(-1);
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
 *
 * History:     The earlier code was not checking for bits being defines
 *		as other than TI8847( WEITEK = 00, FAB6 = 01, TI = 10).
 *
 * History:     Updated FPU_ID_MASK to work with the LSI804 chip ----
 *              ---- aka "Meiko" FPU -------- 1/7/91.
 *
 * History:     Updated FPU_ID_MASK to ignore the unused bits 29,28 and 12.
 *
 * History:     Updated FPU_ID_MASK to ignore the ftt bits 14-16.
 *
 */

#define FPU_ID_MASK  0xCFF02FFF
fsr_test()
{
        int i;
        unsigned long result;

        for (i = 0; i < 7; i++) {  
         
                set_fsr(fsr_testdata[i]);
                result = (get_fsr() & FPU_ID_MASK);		/*sridhar*/
                if (result != fsr_testdata[i]) {		/*sridhar*/
			if (verbose_mode) {
                            printf("\nFSR Error: expected / observed = %x / %x\n",
				 fsr_testdata[i], result);
			}
                        return(-1);
                }
        }
        return(0);
}

psr_test()
{
	unsigned long testdata,result;

	printf("	PSR FPU bit Test:");
	
	/* now set the bit and check it (i.e. disabled) */
/*	set_psr( get_psr() | 0x1000);
 *	result = get_psr();
 *printf("result = %x\n", result);
 *	if (!(result & 0x01000)) {
 *		printf("PSR FPU bit error: expected / observed = 1 / 0\n");
 *		return(-1);
 *	}
 */
	/* now unset the bit and check it (i.e. enabled) */	
/*
 *	set_psr( get_psr() & 0xffffefff);      
 *        result = get_psr();    
 */
        if (result & 0x01000) {
                printf("\nFPU bit is disabled in the PSR, Check whether the FPU is placed correctly.\n");    
                return(-1);    
        }
	printf(" Passed.\n");
	return(0);
}

others_test()
{
        int     numb;


        printf("Others test with number of errors.\n");
        numb = zbug(1);
        printf("        Number of errors = %d\n", numb);
        printf("Others test with first error detected.\n");
numb = 0;
        numb = zbug(0);
        printf("     Number of errors = %d\n", numb); 
        if (numb)
                return(1);
        return(0);
}

