/*
 * @(#)mptest_msg.c - Rev 1.1 - 7/30/92
 *
 * mptest_msg.c:  message file for mptest_msg.c
 *
 */

char *routine_msg = "%s specific arguments: \n\
	T=n    : The subtests to be run, n is an integer that represents the bit pattern of the subtests:\n\
		- Lock/Unlock		(bit 1, n = 1)\n\
		- Data I/O		(bit 2, n = 2)\n\
		- Cache consistency     (bit 3, n = 4)\n\
		- FPU Check		(bit 4, n = 8)\n\
        C=n    : The processors to be enabled, n is an integer that represents the bit pattern of the processors enabled\n";

char *test_usage_msg = "[T=n] [C=n]";
char *open_file_err = "FAIL: to open %s";
char *mmap_err = "%s:  FAIL: mmap address space to device";
char *shmget_err = "FAIL: to get share memory, err = %s";
char *rdm_compare_err = "FAIL: on read & compare:  Process %d, Processor %d, read=%x, exp=%x";
char *fork_err = "FAIL: to fork!";
char *linpack_err = "FAIL: %s precision FPU test, Process %d, Processor %d";
char *open_kmem_err = "FAIL: to open /dev/kmem";
char *open_mem_err = "FAIL: to open /dev/mem";
char *ioctl_err = "FAIL: ioctl, errno = %d, sys_errlist = %s";
char *ioctl_msg_err = "FAIL: ioctl, errno = %d, sys_errlist = %s";
char *number_tests_err = "Please specify the 'T=' argument again";
char *num_processors_enabled_err = "Please specify the 'C=n' argument again";
char *cannot_run_mp = "Can not run mptest!  You need to enable at least two processors.";
char *subtest_select_err = "Please select at least one subtest!";
char *not_mp_system = "Not an MP system!";
char *open_dev_null_err = "FAIL: to open /dev/null";
char *miocspam_err = "FAIL: MIOCSPAM, mask = %x";
char *getnextbitmsk_err = "FAIL: in function getnextbitmsk!";
char *one_line_cache_err = "FAIL: Single Cache Line Test: Process %d, Processor %d,line = %d, page content = %d";
char *multi_line_cache_err = "FAIL: Multiple Cache Line Test: Process %d, Processor %d,line = %d, page content = %d";
char *single_line_cache_spin = "Single Cache Line Test: Process %d, Processor %d, free spinning";
char *single_line_cache_write = "Single Cache Line Test: Process %d, Processor %d, turn to change shared page content = %d";
char *single_line_cache_wrote = "Single Cache Line Test: Process %d, Processor %d, changed shared page content = %d";
char *next_single_line_cache = "Single Cache Line Test: advance to next cache line";
char *multi_line_cache_spin = "Multi Cache Line Test: Process %d, Processor %d, free spinning";
char *multi_line_cache_write = "Multi Cache Line Test: Process %d, Processor %d, turn to change shared page content = %d";
char *multi_line_cache_wrote = "Multi Cache Line Test: Process %d, Processor %d, changed shared page content = %d";
char *next_multi_line_cache = "Multi Cache Line Test: advance to next cache line";
char *mem_write = "%s: Process %d, Processor %d, write cycle";
char *mem_read = "%s: Process %d, Processor %d, read cycle";
char *fork_child = "Forked child Process %d, process_id = %d";
char *mplock_addr = "Lock/Unlock Test:  head = %x, tail = %x";
char *mplock_end_test_loop = "Lock/Unlock Test:  Test loop complete";
char *mplock_buff_count = "Lock/Unlock Test:  Buffer Count = %d";
char *mplock_fail = "Lock/Unlock Test:  Trace buffer is corruptted due to broken lock";
char *io_test_start = "Data I/O Tests:  Process %d, Processor %d, START";
char *io_test_end = "Data I/O Test:  Process %d, Processor %d, END";
char *single_cache_test_start = "Single Cache Line Test:  Process %d, Processor %d, START";
char *single_cache_test_end = "Single Cache Line Test:  Process %d, Processor %d, END";
char *multi_cache_test_start = "Multiple Cache Line Test:  Process %d, Processor %d, START";
char *multi_cache_test_end = "Multiple Cache Line Test:  Process %d, Processor %d, END";
char *lock_unlock_start = "Lock/Unlock Test:  Process %d, Processor %d, START"; 
char *lock_unlock_end = "Lock/Unlock Test:  Process %d, Processor %d, END"; 
