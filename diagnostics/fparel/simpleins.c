/*
 *  static char     fpasccsid[] = "@(#)simpleins.c 1.1 2/14/86 Copyright Sun Microsystems";
 */
#include <sys/types.h>
#include "fpa.h"


struct  test_instructions {   
    
     int   address; 
     int   status; 
}; 
 
struct  test_instructions  instr1[] = { 
   /*  address    status   */ 
 
   {  0xe0000000,    0x01 },         /*SP NOP*/ 
   {  0xe0000004,    0x1d },         /*DP NOP*/
   {  0xe0001000,    0x01 },         /*DP NOP*/ 
   {  0xe000095c,    0x13 },         /*UNIMPL*/
   {  0x00000,       0x00 } 
}; 

/*
 * This test makes sure that the simplest instruction can be executed.
 * The instructions are:
 *
 *    1) single precision NOP
 *    2) double precision NOP
 *    3) single precision unimplimented instruction
 */



sim_ins_test()
{
        u_long index, temp_value;
        u_long *pipe_status, *addr_ptr;

        pipe_status = (u_long *)(FPA_BASE + FPA_STABLE_PIPE_STATUS); /* for stabel pipe status */


        for (index = 0; instr1[index].address != 0; index++) {
                addr_ptr = (u_long *)instr1[index].address;
                *addr_ptr = 0;
                temp_value = (*pipe_status & 0xFF0000) >> 16;
                if (temp_value != instr1[index].status)
                       		return(-1);
                                
        }
        /* clear the hard pipe */
        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;
        return(0);
}


