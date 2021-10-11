/*
 *"@(#)fpu.systest.c 1.1 7/30/92  Copyright Sun Microsystems";
 *
 *
 *
 *      This is the main file.
 *
 *      Author : Chad B. Rao
 *
 *      Date   : 1/1/87       ....   Revision A
 *		 5/24/88   BDS   Upgrading FPU support
 *
 */
#include <sys/types.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
                /* for FPU_FAIL */ 


/* 
 * program names used for this program residing in other 
 * modules
 */



char *err_msg[] = {
       
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

char send_err_msg [120];
char broad_msg[120];
char kernal_msg[120];
char success_msg[120];
char    temp_str[180];
int     sig_err_flag;
u_long  contexts; /* to get the context in case it fails */
int     verbose_mode = 0, no_times = 1;
int	halt_on_error;
unsigned long error_ok;
int	pass_count;
int     run_on_error = 0;
int	fpu_present = -1;

main(argc, argv) 
        int argc;
        char *argv[];
{
        int     value, i, val_rotate, j;
        long    io_value;
        u_char    val;
        u_long  *st_reg_ptr;
        char  *s;
        char    null_str[2];
         
        null_str[0] = '\0';
        send_err_msg[0] = '\0';
        broad_msg[0] = '\0';
        kernal_msg[0] = '\0';
        success_msg[0] = '\0';
        temp_str[0] = '\0';
        sig_err_flag = 0;
	halt_on_error = 1; 


        for (i = 1; i < argc ; i++){
                if (*argv[i] == '-'){
                        switch(argv[i][1]){
                                case 'v':
                                        verbose_mode = 1;
                                        break;
                                case 'p':
                                        if (!(no_times = atoi(&argv[i][2]))) no_times = 0x7fffffff;
                                        break;
				case 'r':
					halt_on_error = 0;
					break;
                                default:
                                        printf("Unknown option %c\n", argv[i][1]);
                                        exit(1);
                                        break;
                        }
                } else {
                        printf("Usage: %s [-v] [-r] [-p<pass_count>]\n", *argv);
                        exit(1);
                }
        }

/*
 * check for the fpu chip here 
 * if not there getout quietly
 */
	if (winitfp()) {
		if (verbose_mode) {
                        printf("        Program Could not talk to FPU chip.\n");
			printf("	Check the presence or placement of FPU chip.\n");
		}
		exit(0);
	}		
	fpu_present = 0;		/* Advise present (got this far)*/
        if (verbose_mode)                        
                printf("FPU System (Reliability) Test : ");

	pass_count = 1;
        do {
/*
 *  Report the type of FPU installed 
 */

		if (verbose_mode) display_device_name();

		if (verbose_mode) 
                        printf("\n        FSR Register Test.\n");

		if (fsr_test()) {
		   if (halt_on_error) {
			fail_close(0);
			exit(1);
		   }
		}	

		if (verbose_mode)
			printf("	Registers Test.\n");
                if (registers_two()) {
                   if (halt_on_error) {
			fail_close(1);
			exit(1);
		   }
		}
		if (verbose_mode)
			printf("	Nack Test.\n");
                if (fpu_nack_test()) {
                   if (halt_on_error) {
			fail_close(2); 
                        exit(1); 
		   }
                }       
                if (verbose_mode) 
                        printf("        Move Registers Test.\n");
                if (fmovs_ins()) {
                   if (halt_on_error) {
			fail_close(3); 
                        exit(1); 
		   }
                }       
                if (verbose_mode) 
                        printf("        Negate: Positive to Negative Test.\n");
                if (get_negative_value_pn()) {
                   if (halt_on_error) {
			fail_close(4); 
                        exit(1); 
		   }
                }       
                if (verbose_mode) 
                        printf("        Negate: Negative to Positive Test.\n");
                if (get_negative_value_np()) {
                   if (halt_on_error) {
			fail_close(5); 
                        exit(1); 
		   }
                }       
                if (verbose_mode) 
                        printf("        Absolute Test.\n");
                if (fabs_ins()) {
                   if (halt_on_error) {
			fail_close(6); 
                        exit(1); 
		   }
                }       
                if (verbose_mode) 
                        printf("        Integer to Floating Point: Single Precision Test.\n");
                if (integer_to_float_sp()) {
                   if (halt_on_error) {
			fail_close(7); 
                        exit(1); 
		   }
                }       
                if (verbose_mode) 
                        printf("        Integer to Floating Point: Double Precision Test.\n");
                if (integer_to_float_dp()) {
                   if (halt_on_error) {
			fail_close(8); 
                        exit(1); 
		   }
                }       
                if (verbose_mode) 
                        printf("        Floating to Integer: Single Precision Test.\n");
                if (float_to_integer_sp()){
                   if (halt_on_error) {
                        fail_close(9);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Floating to Integer: Double Precision Test.\n");
                if (float_to_integer_dp()) {
                   if (halt_on_error) {
                        fail_close(10);  
                        exit(1); 
		   }
                }        
/*DEAD CODE
 *                if (verbose_mode)  
 *                        printf("        Single to Integer Round Toward Zero Test.\n");
 *                if (single_int_round()){
 *                   if (halt_on_error) {
 *                        fail_close(11);  
 *                        exit(1); 
 *		   }
 *                }        
 *                if (verbose_mode)  
 *                        printf("        Double to Integer Round Toward Zero Test.\n");
 *                if (double_int_round()){
 *                   if (halt_on_error) {
 *                        fail_close(12);  
 *                        exit(1); 
 *		   }
 *                }        
 */
                if (verbose_mode)  
                        printf("        Format Conversion: Single to Double Test.\n"); 
                if (single_doub()){
                   if (halt_on_error) {
                        fail_close(13);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Format Conversion: Double to Single Test.\n");
                if (double_sing()){
                   if (halt_on_error) {
                        fail_close(14);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Addition: Single Precision Test.\n");
                if (addition_test_sp()){
                   if (halt_on_error) {
                        fail_close(15);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Addition: Double Precision Test.\n");
                if (addition_test_dp()) {
                   if (halt_on_error) {
                        fail_close(16);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Subtraction: Single Precision Test.\n");
                if (subtraction_test_sp()){
                   if (halt_on_error) {
                        fail_close(17);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Subtraction: Double Precision Test.\n"); 
                if (subtraction_test_dp()){
                   if (halt_on_error) {
                        fail_close(18);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Multiply: Single Precision Test.\n");
                if (multiplication_test_sp()) {
                   if (halt_on_error) {
                        fail_close(19);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Multiply: Double Precision Test.\n");
                if (multiplication_test_dp()){
                   if (halt_on_error) {
                        fail_close(20);  
                        exit(1); 
		   }
                }        

		if (fpu_is_ti8847()) {
                	if (verbose_mode)  
                        	printf("    	Squareroot: Single Precision Test.\n");
                	if (squareroot_test_sp()) {
                   		if (halt_on_error) {
                        		fail_close(37);  
                        		exit(1); 
                        	}
                	}        
		}

		if (fpu_is_ti8847()) {
                	if (verbose_mode)  
                        	printf("    	Squareroot: Double Precision Test.\n");
                	if (squareroot_test_dp()){
                   		if (halt_on_error) {
                        		fail_close(38);  
                        		exit(1); 
                        	}
                	}        
		}


                if (verbose_mode)  
                        printf("        Division: Single Precision Test.\n");
                if (division_test_sp()){
                   if (halt_on_error) {
                        fail_close(21);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Division: Double Precision Test.\n");
                if (division_test_dp()){
                   if (halt_on_error) {
                        fail_close(22);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Compare: Single Precision Test.\n"); 

                if (compare_sp()){
                   if (halt_on_error) {
                        fail_close(23);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Compare: Double Precision Test.\n");
                if (compare_dp()) {
                   if (halt_on_error) {
                        fail_close(24);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Compare And Exception If Unordered: Single Precision Test.\n");
                if (compare_sp_except()){
                   if (halt_on_error) {
                        fail_close(25);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Compare And Exception If Unordered: Double Precision Test.\n");
                if (compare_dp_except()) {
                   if (halt_on_error) {
                        fail_close(26);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Branching On Condition Instructions Test.\n");

                if (branching()){
                   if (halt_on_error) {
                        fail_close(27);  
                        exit(1); 
		   }
                }        

                if (verbose_mode)  
                        printf("        No Branching on Condition Instructions Test.\n");
                if (no_branching()){
                   if (halt_on_error) {
                        fail_close(28);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Chaining (Single Precision) Test.\n");

                if (chain_sp_test()) {
                   if (halt_on_error) {
                        fail_close(29);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Chaining (Double Precision) Test.\n");
                if (chain_dp_test()){
                   if (halt_on_error) {
                        fail_close(30);  
                        exit(1); 
		   }
                }        

                if (verbose_mode)  
                        printf("        FPP Status Test.\n"); 
                if (fpu_ws()){
                   if (halt_on_error) {
                        fail_close(31);  
                        exit(1); 
		   }
                }        

                if (verbose_mode)  
                        printf("        FPP Status (NXM=1) Test.\n");
                if (fpu_ws_nxm()){
                   if (halt_on_error) {
                        fail_close(32);  
                        exit(1); 
		   }
                }        

/*DEAD CODE
 *               if (verbose_mode)  
 *                        printf("        Lock Test.\n");
 *
 *                if (fpu_lock()){
 *                   if (halt_on_error) {
 *                        fail_close(33);  
 *                        exit(1); 
 *		   }
 *                }        
 */
                if (verbose_mode)  
                        printf("        Datapath (Single Precision) Test.\n");
                if (data_path_sp()){
                   if (halt_on_error) {
                        fail_close(34);  
                        exit(1); 
		   }
                }        
                if (verbose_mode)  
                        printf("        Datapath (Double Precision) Test.\n");
                if (data_path_dp()){
                   if (halt_on_error) {
                        fail_close(35);  
                        exit(1); 
		   }
                }        
		if (verbose_mode)
                        printf("        Load Test.\n");
                if (timing_test()) {
                   if (halt_on_error) {
			fail_close(36);
			exit(1);
		   }
		}


                if (verbose_mode)  
                        printf("        Linpack Test.\n");
                if (lin_pack_test()) { 
                   if (halt_on_error) {
                        fail_close(37);
                        exit(1);
		   }
                }

		if (verbose_mode)
                        printf("PASS COUNT = %d\n",pass_count);
                pass_count++;
                no_times--; /* reduce the count */
        } while (no_times != 0);         

        time_of_day();
        make_diaglog_msg(0,0);
        restore_signals();
        exit(0);
}

fpu_open_fail(fail_code)
        int  fail_code;
{        
        char  *ptr1;
        FILE    *input_file, *fopen();

        time_of_day();

        if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
        {
	  if (verbose_mode)
          printf("could not access /usr/adm/diaglog file, can be accessed only under root\n");
          return;
        }
        ptr1 = send_err_msg;

        if (fail_code == ENETDOWN) {
                        strcat(ptr1," : FPU was disabled by kernal while doing ");
                        strcat(ptr1,"reliability test, may be due to h/w problem.");
        }
        else if (fail_code == EBUSY) {
                strcat(ptr1," : No FPU context is Available.");
        }
        else if (fail_code == EEXIST) {
                strcat(ptr1," : Duplicate open on FPU context.");
        }        
         
        strcat(ptr1," FPU reliability test is terminated.\n");
        fputs(ptr1,input_file);
	fclose(input_file);
}
fail_close(val)
        int val;
{
	if (verbose_mode)
	printf("Pass = %d.\n", pass_count);
	restore_signals();
        time_of_day();
         
        if (make_diaglog_msg(1,val))
                        return;
        make_broadcast_msg();
/*DEAD CODE
 *        make_unix_msg();
 */
}

/*
 * the following routine gets the time of the day and puts in an array
 *
 */
time_of_day()
{
        char    *tempptr, *temp;
        long     temptime, i;
        char    *ptr1, *ptr2, *ptr3, *ptr4;
 
        time(&temptime); 
        tempptr = ctime(&temptime);

        ptr1 = send_err_msg;    /* for error, writing into the diaglog file */
        ptr2 = broad_msg;       /* for error, sending it to broadcast */
        ptr3 = kernal_msg;      /* for error, sending it to kernal */
        ptr4 = success_msg;     /* for successful message */
        strcat(ptr1,"\n");
        strcat(ptr2,"\n");
        strcat(ptr3,"\n");
        strcat(ptr4,"\n");
        strncat(ptr1, tempptr, 24);
        strncat(ptr2, tempptr, 24);
        strncat(ptr3, tempptr, 24);
        strncat(ptr4, tempptr, 24);
         
}

make_unix_msg()
{
        char    *ptr1, *ptr2, *ptr3;
        int     i, j, val1;
        char    total_str[180];
        int     temp;

        total_str[0] = '\0';
        ptr1 = send_err_msg;
        ptr2 = temp_str;
        ptr3 = total_str;

        strcat(ptr3,ptr1);

        ptr1 = total_str;

/*DEAD CODE
 *        if ((temp = ioctl(dev_no,FPA_FAIL,(char *)total_str)) < 0)
 *        {
 *                if (errno == EPERM)
 *                        printf("Not a Super User :Fpurel unable to disable FPU. \n");
 *                else
 *                 
 *                        perror("fpurel");
 *        }
 */
}

make_diaglog_msg( suc_err, val)
        int     val; /* for the string */
        int     suc_err; /* should print success = 0; error = 1 */
{
        FILE    *input_file, *fopen();
        char    *ptr1, *ptr2, *ptr3, *ptr4;
        char    i, flag_comma, val1;
        char    number_str[4];
        u_long  val_rotate;

        number_str[0] = '\0';
        ptr1 = send_err_msg;
        ptr2 = temp_str;
        ptr3 = number_str;
        ptr4 = err_msg[val];

         
        if ((input_file = fopen("/usr/adm/diaglog","a")) == NULL)
        {
		if (verbose_mode)
                printf("could not access /usr/adm/diaglog file, can be accessed only under root permission.\n");
                return(-1);
        }
        if (suc_err == 1) {
/*DEAD CODE
 *                strcat(ptr1," : Sun FPU Reliability Test Failed, Sun FPU DISABLED -  Service Required.\n");
 */
		strcat(ptr1," : Sun FPU Reliability Test (");
		strcat(ptr1, ptr4);
		strcat(ptr1, ") Failed.\n");
	}
        else if (suc_err == 0)
                strcat(ptr1," : Sun FPU Reliability Test Passed.\n");

	fputs(ptr1,input_file);
        fputs(ptr2,input_file);
        fclose(input_file);              
        return(0);                       
}


make_broadcast_msg()
{
        FILE    *input_file, *fopen();
        char    val;
        char    *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
        char    file_name[20];   
        char    file_full_name[40];
        char    wall_str[40];
        char    rm_str[40];

        file_name[0] = '\0';
        wall_str[0] = '\0';
        rm_str[0] = '\0';
        file_full_name[0] = '\0';
         
        ptr1 = broad_msg;
/*DEAD CODE
 *        strcat(ptr1," : Sun FPU Reliability Test Failed, Sun FPU DISABLED - Service Required.\n");
 */
	strcat(ptr1," : Sun FPU Reliability Test Failed.\n");

        ptr2 = file_full_name;
        ptr3 = wall_str;
        ptr4 = rm_str;
        tmpnam(ptr2);   /* get the temporary name */
        input_file = fopen(ptr2,"w");    
        fputs(ptr1, input_file);
        fclose(input_file);
        strcpy(ptr3,"wall ");
        strcat(ptr3,ptr2);
        strcat(ptr4,"rm -f ");
        strcat(ptr4,ptr2);
        system(ptr3);
        system(ptr4); 
}

/*
 *	Display the Name of the Device based on probing
 */

display_device_name()
{
	if (fpu_is_weitek()) {
		printf("\nA Weitek 1164/1165");
	} else if (fpu_is_fab6()) {
		printf("\nA FAB6 for FPC6");
	} else {

		if(fpu_is_ti8847()) {
		   printf("\nA TI 74ACT8847");
		   if (fpu_badti())
			printf("(*GB* Step)");
		}
	}
	printf(" Floating Point Controller is installed.");
}

lin_pack_test()
{
       if( slinpack_test() ) /* single precision linpack test */
                return(-1);
        if( dlinpack_test() ) /* double precision linpack test */
                return(-1);
	return(0);
}
