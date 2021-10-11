#ifndef lint
static  char sccsid[] = "@(#)sundiag_msg.c	1.1  7/30/92 Copyright Sun Micro";
#endif

/*******************************************************************************
  Sundiag info messages for Sunview part.
*******************************************************************************/
char *start_sundiag_info = "*Start Sundiag %s*";
char *quit_sundiag_info = "*Quit Sundiag*";
char *quit_abnorm_info = "*Quit Sundiag(abnormally)*";
char *start_all_info = "*Start all tests*";
char *stop_all_info = "*Stop all tests*";
char *reset_pecount_info = "*Reset pass/error counts*";
char *period_log_info = "*Periodic status log*";
char *schedule_start_info = "*Scheduler starts all tests*";
char *schedule_stop_info = "*Scheduler stops all tests*";
char *enable_test_info = "*Enable test*\n%s";
char *disable_test_info = "*Disable test*\n%s";
char *test_failed_info = "*Failed test*\n%s";
char *loaded_option_file = "*Loaded Option file: %s*";
char *batch_period_info = "*Done batch testing option file %s; runtime %d min(s); waittime %d min(s).\n";


/*******************************************************************************
  Sundiag error messages for Sunview part.
*******************************************************************************/
char *pid_valid_error = "pid %d is not valid.";   /* pid */
char *core_dump_error = "(%s) %s(pid %d) dumped core."; 
				/* tests[idx]->devname, tests[idx]->testname, 
					   pid */
char *term_signal_error = "(%s) %s(pid %d) died due to signal %d.";
				/* tests[idx]->devname, tests[idx]->testname,
                        		   pid, status->w_termsig */
char *send_message_error = "(%s) %s failed sending message to sundiag.";
			        /* tests[idx]->devname, tests[idx]->testname */
char *rpcinit_error = "(%s) %s failed to initialize RPC.";
                                /* tests[idx]->devname, tests[idx]->testname */
char *fork_error = "Failed forking %s for device (%s)";
				/* tests[idx]->testname, tests[idx]->devname */
