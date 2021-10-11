/*
 *      static char     fpasccsid[] = "@(#)memory.c 1.1 7/30/92 Sun right Sun Microsystems";
 */
#include <sys/types.h>
#include "fpa.h"


map_ram()
{
	u_long i, temp_val, tmp_data;
	u_long *ram_ptr, *rw_ram_ptr;
	u_char no_data_flag = 0xff; 

	ram_ptr = (u_long *)FPA_RAM_ACC;
	rw_ram_ptr = (u_long *)FPA_RW_RAM;
	*ram_ptr = TEST_PTR(0, MR_LB);
	temp_val = *rw_ram_ptr;
	temp_val = temp_val & MR_DMASK;
	if (temp_val)
		no_data_flag = 0x0; /* data  is non zero */
        for (i=1 ; i < MAXSIZE_MAP_RAM ; i++) {
                *ram_ptr = TEST_PTR(i, MR_LB);
                tmp_data = *rw_ram_ptr & MR_DMASK;
		if (tmp_data)
			no_data_flag = 0x0; /* data is non zero */
		temp_val = (temp_val ^ tmp_data) & MR_DMASK; 
        }
	if (no_data_flag)
		return(-1); /* data is zero , error */
	if (temp_val != 0xadface) /* the checksum should be ADFACE */
		return(-1);
	return(0);
}

ustore_ram()
{
        u_long i, tmp_val, tmp_data; 
	u_long tmp_val1, tmp_data1;
	u_long tmp_val2, tmp_data2;
	u_char no_data_flag = 0xff; 
	u_long *ram_ptr, *rw_ram_ptr; 

        ram_ptr = (u_long *)FPA_RAM_ACC; 
        rw_ram_ptr = (u_long *)FPA_RW_RAM; 
	 
	*ram_ptr = TEST_PTR(0, UR_LBL); 
        tmp_val = *rw_ram_ptr; 
        *ram_ptr = TEST_PTR(0, UR_LBM); 
        tmp_val1 = *rw_ram_ptr; 
        *ram_ptr = TEST_PTR(0, UR_LBH); 
        tmp_val2 = *rw_ram_ptr & UR_DMASK; 
	if ((tmp_val) || (tmp_val1) || (tmp_val2))
		no_data_flag = 0x0; /* data is non zero */

        for (i=1 ; i < MAXSIZE_USTORE_RAM ; i++) {

                *ram_ptr = TEST_PTR(i, UR_LBL);
                tmp_data = *rw_ram_ptr;
                *ram_ptr = TEST_PTR(i, UR_LBM);
                tmp_data1 = *rw_ram_ptr;
                *ram_ptr = TEST_PTR(i, UR_LBH);
                tmp_data2 = *rw_ram_ptr & UR_DMASK;
		if ((tmp_data) || (tmp_data1) || (tmp_data2))
			no_data_flag = 0x0;  /* data is non zero */

		tmp_val = tmp_val ^ tmp_data;
		tmp_val1 = tmp_val1 ^ tmp_data1;
		tmp_val2 = (tmp_val2 ^ tmp_data2) & UR_DMASK;
	}
	if (no_data_flag)
		return(-1); /* data is zero , error */
	if ((tmp_val == 0xBEADFACE) && (tmp_val1 == 0xBEADFACE) && (tmp_val2 == 0xCE)) 
			
			return(0);
	
	else
		return(-1);
}

reg_uh_ram()
{
        u_long i, tmp_val, tmp_data;  
        u_long tmp_val1, tmp_data1; 
        u_long *ram_ptr, *rw_ram_ptr;  
	u_char no_data_flag = 0xff;

	ram_ptr = (u_long *)FPA_RAM_ACC;
        rw_ram_ptr = (u_long *)FPA_RW_LREG_RR;
	*ram_ptr = TEST_PTR(0x400, RR_LB); 
        tmp_val = *rw_ram_ptr;
	if (tmp_val)
		no_data_flag = 0x0; /* data is non zero */

        for (i=0x401; i <= MAXSIZE_CON_RAM ; i++) {

                *ram_ptr = TEST_PTR(i, RR_LB);
                tmp_data = *rw_ram_ptr;
		if (tmp_data) 
	                no_data_flag = 0x0; /* data is non zero */
		tmp_val = tmp_val ^ tmp_data;
        }
	*ram_ptr = TEST_PTR(0x7FF,RR_LB);
	tmp_data = *rw_ram_ptr;
	if (tmp_data)
                no_data_flag = 0x0; /* data is non zero */
	tmp_val = tmp_val ^ tmp_data;

        rw_ram_ptr = (u_long *)FPA_RW_MREG_RR;
        *ram_ptr = TEST_PTR(0x400, RR_LB);  
        tmp_val1 = *rw_ram_ptr; 
	if (tmp_val1) 
                no_data_flag = 0x0; /* data is non zero */

        for (i=0x401; i <= MAXSIZE_CON_RAM ; i++) {

                *ram_ptr = TEST_PTR(i, RR_LB);
		tmp_data1 = *rw_ram_ptr; 
		if (tmp_data1) 
	                no_data_flag = 0x0; /* data is non zero */
                tmp_val1 = tmp_val1 ^ tmp_data1;
        }
	*ram_ptr = TEST_PTR(0x7FF, RR_LB);
        tmp_data1 = *rw_ram_ptr;
        if (tmp_data1)    
                no_data_flag = 0x0; /* data is non zero */
	tmp_val1 = tmp_val1 ^ tmp_data1;

	if (no_data_flag) {
		return(-1); /* data is zero , error */	
	}
	if ((tmp_val == 0xBEADFACE) && (tmp_val1 == 0xBEADFACE))
		
			return(0);
        else 
                return(-1); 
	
} 
 
 
