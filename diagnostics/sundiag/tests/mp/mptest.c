/*
 * @(#)mptest.c - Rev 1.1 - 7/30/92
 */
/***************************************************************************
 mptest
        This test covers the follwing functional tests:
        1) checks lock/unlock  
        2) checks I/O data path consistency
        3) checks cache data consistency
        4) checks FPU/IU consistency

****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sun/mem.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "mptest.h"
#include "mptest_msg.h"
#include "sdrtns.h"
#include "../../../lib/include/libonline.h" /* sundiag standard include file */

/* extern variables for I/O test */
extern char iotestfile[MAX_STRING];
extern int io_testfile_create, io_testfile_fd, dataio_pagesize;
extern unsigned char *mmap_addr;

/* extern variables for cache consistency test */
extern int cache_test_fd, mem_opened;
extern int ordinal;
extern char *vaddr;

/* extern variables for mplock test */
extern int mplock_created, mplock_fd, mplock_pagesize;
extern char *mplock_vaddr;
 
int process_id;
unsigned mask;
int number_processors;
int selection_tests, number_test;
int stopped_child_pid, process_id, child_pid[30];
int errno;
char *sys_errlist[];
int mplock_test;
 
main(argc, argv)
int argc;
char *argv[];
{

    extern int process_mptest_args(), routine_usage();

    versionid = "1.11";
    func_name = "main";

    test_init(argc, argv, process_mptest_args, routine_usage, test_usage_msg);
    device_name = "mp";

    option_error();

    mp_test(selection_tests);

    /* only parent print test:  Stopped sucessfully */
    if (process_id)
    { 
	clean_up();
        test_end();
    }

    exit(0);
}

process_mptest_args(argv, arrcount)
char *argv[];
int arrcount;
{
    func_name = "process_mptest_args";
    TRACE_IN


    /* initialize the arguments variable */
    if ( (mask = get_grandmask()) == 0 ) 
    {
        send_message(-NOT_MP_SYSTEM, ERROR, not_mp_system);
    } 

    if (strncmp(argv[arrcount], "T=", 2) == 0) 
    {
        selection_tests = atoi(&argv[arrcount][2]);
        TRACE_OUT
        return(TRUE);
    }
    else if (strncmp(argv[arrcount], "C=", 2) == 0)
    {
        mask = atoi(&argv[arrcount][2]);
        TRACE_OUT
	return(TRUE);
    }
    else 
    {
        TRACE_OUT
        return(FALSE);
    }

}

option_error()
{
    func_name = "default_args";
    TRACE_IN

    if ((number_processors = get_number_processors(mask)) < 2)
    {
	send_message(-CANNOT_RUN_MP, ERROR, cannot_run_mp);
    }

    if (selection_tests == 0)
    {
        send_message(-SUBTEST_SELECT_ERR, ERROR, subtest_select_err);
    }

    TRACE_OUT

}

mp_test(selection_tests)
int selection_tests;
{

    func_name = "mp_test";
    TRACE_IN

    setup_mptest(selection_tests);

    /* Spawn of processes */
    spawn_processes();

    /* parent wrap up and child should bypass */
    if (process_id) 
    { 

        kill_processes();

sleep(5);
        if (mplock_test) /* print buffer of lock/unlock test */
           print_trace_buffer();

    }


    TRACE_OUT

}

setup_mptest(selection_tests)
int selection_tests;
{
    int bit, i;

    func_name = "setup_mptest";
    TRACE_IN

    /* Initialize */
    number_test = 0;
    mplock_test = FALSE;
    
    for (bit = 1, i = 0; i < 4; bit <<= 1, i++) 
    {
        if (selection_tests & bit) 
        {
	    switch (i) 
            {
		case 0:
                    /* Setup for share memory test */
		    mplock_test = TRUE;
                    setup_mplock();
                    break;

		case 1:
            	    /* Setup for i/o test */
		    setup_io_test();
                    break;

                case 2:
                    break;

                case 3:
            	    /* Setup for cache consistency test */
                    setup_cache_consistency_test();
                    break;
		default:
		    send_message(-NUMBER_TESTS_ERR, ERROR, number_tests_err);
		    break;
            }

            /* Count the number of tests: mplock, I/O, fpu and cache test */
            number_test++;
        }
    }    

    TRACE_OUT
}

kill_processes()
{
    int j, status, st;

    func_name = "kill_processes";
    TRACE_IN

    /* Kill processes */
    if (process_id != 0) 
    {
        for (j = 0; j < number_test*number_processors; j++)
	{
            if (stopped_child_pid = wait(&status))
            {
		if (stopped_child_pid > 0)
		{
		    st = exit_status(&status);
                    if (st != 0)
		    {
                        kill_children();
                        TRACE_OUT
                        exit(st);
                    }
		}
            }
        }
    }
    else
    {
	TRACE_OUT
        exit(0);
    }

}

kill_children()
{
    int i;

    for (i = 0; i < (number_test*number_processors); i++)
    {
	if (child_pid[i] != stopped_child_pid)
        {
	    kill(child_pid, SIGKILL);
        }
    }
}

exit_status(status)
int *status;
{
    char exit_code = 0;
    int temp_status = *status;

    switch (temp_status & 0xff)
    {
	case 0:
	    exit_code = temp_status >> 8;
	    break;
	case 0177:
	    exit_code = temp_status >> 8;
	    break;
	default:
	    exit_code = temp_status >> 0x0000007f;
	    break;
    }
    return(exit_code);
}


routine_usage()
{
    func_name = "routine_usage";
    TRACE_IN

    send_message(0, CONSOLE, routine_msg, test_name);
 
    TRACE_OUT
}

clean_up()
{
    func_name = "clean_up";
    TRACE_IN

    /* Unmaps a page from address space */
    if (mplock_created == TRUE)
    {
	close(mplock_fd);
	munlock(mplock_vaddr, mplock_pagesize);
	mplock_created = FALSE;
    }

    /* Remove test file */
    if (io_testfile_create == TRUE) 
    {
	close(io_testfile_fd);
        (void)unlink(iotestfile);
        io_testfile_create = FALSE;
    }

    /* unlock the memmory for cache consistency test */
    if (mem_opened)
    {
	close(cache_test_fd);
        munlock(vaddr, CACHE_BLOCK);
    }

    TRACE_OUT
}

int
spawn_processes()
{
    int i, number_processes, status, temp_ordinal;
    unsigned bit_mask;

    func_name = "spawn_processes";
    TRACE_IN

    temp_ordinal = 0;
    bit_mask = 0;
    for (i = 0; i < number_processors*number_test; i++) 
    {
        bit_mask = getnextbitmsk(mask, bit_mask);
        switch(process_id = fork()) 
        {
            case -1:
		/****
		XXX - should gracefully show-down: kill all already-forked
 			processes before send_message() call clean-up()
			while release all resources.
		 ****/
                send_message(-FORK_ERR, ERROR, fork_err);
                break;
            case 0: /* child process */
		ordinal = temp_ordinal;
                assign_process_to_processor(i, bit_mask);
                return(0);
                break;
            default: /* parent process */
		send_message(0,  VERBOSE, fork_child, i, process_id);
		child_pid[i] = process_id;
		break;
        }
        temp_ordinal = rotate(number_processors, temp_ordinal);
    }    

    TRACE_OUT

}

assign_process_to_processor(process_number, bit_mask)
int process_number;
unsigned bit_mask;
{

    func_name = "assign_process_to_processor";
    TRACE_IN

    lock2cpu(&bit_mask);
    getcpu(&bit_mask);
    run_processes(process_number, bit_mask);

    TRACE_OUT
}

run_processes(process_number, cpu_mask)
int process_number;
unsigned cpu_mask;
{

    func_name = "run_processes";
    TRACE_IN
    
    switch (number_test) 
    {
        case ONE_TEST:
            run_one_test(process_number, cpu_mask); 
            break;
	case TWO_TESTS:
	    run_two_tests(process_number, cpu_mask);
            break;

	case THREE_TESTS:
	    run_three_tests(process_number, cpu_mask);
            break;

	case FOUR_TESTS:
	    run_four_tests(process_number, cpu_mask);
	    break;
    }

    TRACE_OUT
}

run_one_test(process_number, cpu_mask)
int process_number;
unsigned cpu_mask;
{
    int processor;

    func_name = "run_one_test";
    TRACE_IN

    processor = nint(log2((double)cpu_mask));

    switch(selection_tests)
    {
        case 1: 
            lock_unlock(process_number, cpu_mask);
            break;
        case 2:  
            io_test(process_number, processor, mmap_addr, dataio_pagesize, 0);
	    break; 
        case 4: 
            run_linpack_test(process_number, cpu_mask);
            break;
        case 8:
            cache_consistency_test(process_number, cpu_mask); 
            break;
    } 

    TRACE_OUT
}

run_two_tests(process_number, cpu_mask)
int process_number;
unsigned cpu_mask;
{
    int processor;

    func_name = "run_two_tests";
    TRACE_IN

    processor = nint(log2((double)cpu_mask));

    switch(selection_tests)
    {
        case 3: 
	    /* lock/unlock test and i/o test */
            if (process_number < number_processors)
                lock_unlock(process_number, cpu_mask);
            else
 	        io_test(process_number, processor, mmap_addr, 
                         dataio_pagesize, number_processors);
            break;
	case 5:
	    /* lock/unlock and linpack test */
            if (process_number < number_processors)
                lock_unlock(process_number, cpu_mask);
            else
                run_linpack_test(process_number, cpu_mask);
	    break;
        case 6:
	    /* i/o test and linpack test */
            if (process_number < number_processors)
 	        io_test(process_number, processor, mmap_addr, 
			 dataio_pagesize, 0);
            else
                run_linpack_test(process_number, cpu_mask);
	    break;
        case 9:
	    /* lock/unlock and cache consistency test */
            if (process_number < number_processors)
                lock_unlock(process_number, cpu_mask);
            else
                cache_consistency_test(process_number, cpu_mask);
	    break;
	case 10:
	    /* i/o test and cache consistency test */
            if (process_number < number_processors)
 	        io_test(process_number, processor, mmap_addr, 
                         dataio_pagesize, 0);
            else
                cache_consistency_test(process_number, cpu_mask); 
	    break;
	case 12:
	    /* cache consistency test and linpack test */
            if (process_number < number_processors) 
            {
               cache_consistency_test(process_number, cpu_mask);
	    }
            else
                run_linpack_test(process_number, cpu_mask);
	    break;
    }

    TRACE_OUT
}

run_three_tests(process_number, cpu_mask)
int process_number;
unsigned cpu_mask;
{
    int processor;

    func_name = "run_three_tests";
    TRACE_IN

    processor = nint(log2((double)cpu_mask));

    switch(selection_tests)
    {
	case 7:
	    /* lock/unlock, i/o and linpack test */
 	    if (process_number < number_processors)
                lock_unlock(process_number, cpu_mask);
	    else
	        if (process_number < 2*number_processors)
	 	    io_test(process_number, processor, mmap_addr, 
                             dataio_pagesize, number_processors);
	        else 
                    run_linpack_test(process_number, cpu_mask);
	    break;
	case 11:
	    /* lock/unlock, i/o and cache consistency test */
 	    if (process_number < number_processors)
                lock_unlock(process_number, cpu_mask);
	    else
	        if (process_number < 2*number_processors)
	 	    io_test(process_number, processor, mmap_addr, 
                             dataio_pagesize, number_processors);
	        else 
                    cache_consistency_test(process_number, cpu_mask);
	    break;
	case 13:
	    /* lock/unlock, cache consistency and linpack test */
 	    if (process_number < number_processors)
                lock_unlock(process_number, cpu_mask);
	    else
	        if (process_number < 2*number_processors)
		{
                    cache_consistency_test(process_number, cpu_mask);
		}
	        else 
                    run_linpack_test(process_number, cpu_mask);
	    break;
	case 14:
	    /* i/o test, cache consistency test and linpack test */
 	    if (process_number < number_processors)
                io_test(process_number, processor, mmap_addr,
                         dataio_pagesize, 0);
	    else
	        if (process_number < 2*number_processors) 
		{
                    cache_consistency_test(process_number, cpu_mask);
		}
	        else 
                    run_linpack_test(process_number, cpu_mask);
	    break;
    }
   
    TRACE_OUT
}

run_four_tests(process_number, cpu_mask)
int process_number;
unsigned cpu_mask;
{
    int processor;

    func_name = "run_four_tests";
    TRACE_IN

    processor = nint(log2((double)cpu_mask));

    if (process_number < number_processors)
        lock_unlock(process_number, cpu_mask);
    else
    {
	if (process_number < 2*number_processors)
	    io_test(process_number, processor, mmap_addr, dataio_pagesize, 
                     number_processors);
	else 
        {
	    if (process_number < 3*number_processors)
	    {
                cache_consistency_test(process_number, cpu_mask);
	    }
            else
                run_linpack_test(process_number, cpu_mask);
        }
    }

    TRACE_OUT
}

run_linpack_test(process_number, cpu_mask)
int process_number;
int cpu_mask;
{
    if (slinpack_test(process_number, cpu_mask))
        return -1;
    if (dlinpack_test(process_number, cpu_mask))
        return -1;
    return 0;
}
