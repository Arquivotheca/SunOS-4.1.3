/*
 *       @(#)mplock.c 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
 */
/************************************************************************/
/*									*/
/*   NAME:								*/
/*	MP lock/unlock test      					*/
/*									*/
/*   DESCRIPTION:							*/
/*	main process fork one child process for each processor and lock */
/*	its PAM to the processor. All process shared a common cachable 	*/
/*	page. Than all process tris to get the lock by using the SPARC	*/
/*	atomic instruction - ldstub.					*/
/*	All process shares a common page which hold the lock and the	*/
/*	trace buffer. Each						*/
/*									*/
/*		-----------------------					*/
/*		|   lock    	      | <------- mplock_vaddr	        */
/*		-----------------------					*/
/*		| trace write pointer | ---  <-- mplock_vaddr+4		*/
/*		-----------------------	   |				*/
/*		|		      |    | <-- mplock_vaddr+8		*/
/*		| trace buffer	      |    |				*/
/*		|		      |<--- 				*/
/*		|    ...	      |					*/
/*		-----------------------					*/
/*									*/
/************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sun/mem.h>
#include <sys/ioctl.h>
#include "mptest.h"
#include "mptest_msg.h"
#include "sdrtns.h"

extern int number_processors;
extern int errno;
extern char *sys_errlist[];

char 	*valloc();
char **traceptr;
int mplock_created, mplock_fd, mplock_pagesize;
char *mplock_vaddr;

setup_mplock()
{

    func_name = "setup_mplock";
    TRACE_IN
 
    mplock_created = FALSE;
 
    mplock_pagesize = getpagesize();

    mplock_vaddr = valloc(mplock_pagesize) ; /* parent create a page */

    if ((mplock_fd = open("/dev/zero",O_RDWR)) < 0) 
    {
        send_message(-OPEN_MEM_ERR, ERROR, open_mem_err);
    }
    else mplock_created = TRUE;

    if (mplock_created)
    {
        if ( (mmap(mplock_vaddr, mplock_pagesize, PROT_READ | PROT_WRITE ,
               MAP_FIXED | MAP_SHARED,
               mplock_fd,(off_t)0)) == (caddr_t)-1) {
            send_message(-MMAP_ERR, ERROR, mmap_err, "Lock/Unlock");
        }
        mlock(mplock_vaddr, mplock_pagesize);

        traceptr = (char **)(mplock_vaddr + 4) ;
        *traceptr = mplock_vaddr + 8 ;
        *mplock_vaddr = 0 ;    /* initially not lock */
    } 

    TRACE_OUT

}

lock_unlock(process_number, cpu_mask)
int process_number; 
unsigned cpu_mask;
{
    int i, loop, cpu;

    loop = 32;
    cpu = nint(log2((double)cpu_mask));

    send_message(0, VERBOSE, lock_unlock_start, process_number, cpu);

    /* All process come down to here, all free spinning */
    i = loop;
    while (i--)
    {
	mplock(mplock_vaddr);
	/*
	 * ONLY one processor/thread is allowed in this code section
	 */
	 **traceptr = cpu_mask + 48 ; /* 1 -> '1' */
	 usleep(10000);
	 (*traceptr)++ ;
	 mpunlk(mplock_vaddr);
    }

    send_message(0, VERBOSE, lock_unlock_end, process_number, cpu);

}

print_trace_buffer(process_number, cpu_mask)
int process_number;
unsigned cpu_mask;
{
    int j, err, cpu;
    char *pt, **tail ;
    int status;
    char buffer[300], tmp_buffer[30];

    cpu = nint(log2((double)cpu_mask));

    pt = mplock_vaddr + 8 ;
    tail = (char **)(mplock_vaddr + 4);
    send_message(0, DEBUG, mplock_addr, mplock_vaddr + 8, *tail);
    send_message(0, DEBUG, mplock_end_test_loop);
    send_message(0, DEBUG, mplock_buff_count, *tail - mplock_vaddr - 8);
    send_message(0, DEBUG, "Lock/Unlock Test: trace buffer=");
    err = 0;
    for ( j = 0; j < number_processors*32; j++ ) 
    {
	if ( *pt == 0 )
	{
	    err = 1;
            sprintf(tmp_buffer, "%x ", *pt);
	}
	else 
	{
            sprintf(tmp_buffer, "%x ", *pt - 48);
	}
        strcat(buffer, tmp_buffer);
	pt++ ;
    }
    send_message(0, DEBUG, "%s", buffer);
    if ( err )
	send_message(-MPLOCK_FAIL, ERROR, mplock_fail, process_number, cpu);

}
