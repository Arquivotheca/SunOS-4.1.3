
/*
 *      static char     fpasccsid[] = "@(#)interface.c 1.2 2/25/86 Copyright Sun Microsystems";
 *
 *      Copyright (c) 1985 by Sun Microsystems, Inc 
 *
 */
#include <sys/types.h>
#include "fpa.h"
#include <sys/time.h>
#include <sys/wait.h>

char    *argv[4]; 
char    *argv1[4]; 
char    *file_dp, *file_sp;
char    file_double[20], file_single[20];

char   cmp_str[4] = { "cmp"};
char   s_str[3] = { "-s"};
char   file_str[] = { "/usr/etc/fpa/double.out"};
char   file_single_str[] = { "/usr/etc/fpa/single.out"};	
union wait      w;

extern char     **environ;

dp_compare()
{
	char *file_ptr;
	
	argv[0] = cmp_str;
	argv[1] = s_str;
	argv[2] = file_str;
	argv[3] = file_double;
        if (!fork()){
                execve("/bin/cmp", argv, environ);
        } else {
                wait(&w);
		return(w.w_retcode);

        }

}


sp_compare()
{
        char *file_ptr; 
 
        argv1[0] = cmp_str; 
        argv1[1] = s_str; 
        argv1[2] = file_single_str;

	argv1[3] = file_single;
        if (!fork()){
                execve("/bin/cmp", argv1, environ);
        } else { 
                wait(&w);
		return(w.w_retcode);
        }
}


run_linpack()
{
	char *ptr1 = "/usr/etc/fpa/linpack.sp >> /usr/etc/fpa/";
	char *ptr2 = "/usr/etc/fpa/linpack.dp >> /usr/etc/fpa/";
	char *name_sp, *name_dp;
	char *ptr_sp, *ptr_dp;
	char sp[80];
	char dp[80];
	
	ptr_sp = sp;
	ptr_dp = dp;

	name_sp = file_single;
	name_dp = file_double;

	tmpnam(name_sp); /* get the name pointer for sp */
	tmpnam(name_dp); /* get the name pointer for dp */
	
	strcpy(ptr_sp,ptr1);
	strcat(ptr_sp,name_sp);
	strcpy(ptr_dp, ptr2);
	strcat(ptr_dp, name_dp);
	system(ptr_sp);
	system(ptr_dp);
}
	
remove_temp_files()
{
	char *ptr1 = "rm -f /usr/etc/fpa/";
	char str[50];
	char *ptr_str, *single_ptr, *double_ptr;

	single_ptr = file_single;
	double_ptr = file_double;
	ptr_str = str;

	strcpy(ptr_str,ptr1);
	strcat(ptr_str,single_ptr);
	strcat(ptr_str," ");
	strcat(ptr_str,"/usr/etc/fpa/");
	strcat(ptr_str,double_ptr);
	system(ptr_str);
}		

int linpack_test()
{
	int val;

	run_linpack(); /* run the linpack tests for single precision and double precision */
	if ((val = dp_compare()) != 0)  { /* 0 - compared ok, 1 - not same, 2 - inaccessible */
		remove_temp_files();      /* remove the output file */ 
		return(val);
	}
	if ((val = sp_compare()) != 0) {
		remove_temp_files();      /* remove the output file */
		return(val);
	}
	remove_temp_files();      /* remove the output file */ 
	return(0);
}
