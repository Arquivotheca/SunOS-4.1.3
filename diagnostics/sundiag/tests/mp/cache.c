/*
 *       @(#)cache.c 1.1 92/07/30 Copyright(c) Sun Microsystems, Inc.
 */

/************************************************************************/
/*									*/
/*   NAME:								*/
/*	MP cache consistency test					*/
/*									*/
/*   DESCRIPTION:							*/
/*      To device an on-line MP cache consistency check requires two    */
/*      or more UNIX processes, each locks to one CPU, to access the    */
/*      same PHYSICAL page such that change made by one process will    */
/*      be reflected in the other process(es). The following steps      */
/*      shows how this is achieved.                                     */
/*                                                                      */
/*      (1) Process locks to one CPU                                    */
/*      (2) Process allocates a page of virtual memory                  */
/*          (so it is cacheble)                                         */
/*      (3) issue mmap() to declare SHARED                              */
/*      (4) Locks this page into memory(so mapping wouldn't go away)    */
/*      (5) Fork children processes, each locks to another CPU          */
/*                                                                      */
/*      MP cache consistency test consists of single cache line test    */
/*      and multiple cache line test.                                   */
/*                                                                      */
/*      Single cache line test:                                         */
/*              |<--------  32 bytes  --------->|                       */
/*              ---------------------------------                       */
/*              |byte|                          |                       */
/*              ---------------------------------                       */
/*                                                                      */
/*      (1) Each CPU set write pointer to first byte in the cache       */
/*      (2) Each CPU wait for its ordinal number(0,-1,2,-3,4...)        */
/*          to come, when it comes, increment and negate the content    */
/*      (3) Increment the write pointer to the next byte in the cache   */
/*          line.                                                       */
/*      (4) Repeat until all 32 bytes are tested.                       */
/*                                                                      */
/*      Multiple cache line test:                                       */
/*                                                                      */
/*              |<--------  32 bytes  --------->|                       */
/*              ---------------------------------                       */
/*      write ->| word |                        |                       */
/*              ---------------------------------                       */
/*              | word |                        |                       */
/*              ---------------------------------                       */
/*              | word |                        |                       */
/*              ---------------------------------                       */
/*              | word |                        |                       */
/*              ---------------------------------                       */
/*      (1) Each CPU set write pointer to beginning of first line       */
/*      (2) Each wait its ordinal number to come up                     */
/*      (3) When it comes, increment and negate the content             */
/*      (4) Increment the write point to the first word in the NEXT     */
/*          cache line                                                  */
/*      (5) Repeat (1)-(4) until all lines has be tested                */
/*                                                                      */
/*                                                                      */
/************************************************************************/
#include <stdio.h>
#include <math.h>
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

int mem_opened, cache_test_fd, ordinal;
char *vaddr; 
char *wrtptr;

setup_cache_consistency_test()
{
 
    func_name = "setup_cache_consistency_test";
    TRACE_IN
 
    /* create cachable page */
    vaddr = (char *)valloc(CACHE_BLOCK);
 
    mem_opened = FALSE;

    if ((cache_test_fd = open("/dev/zero", O_RDWR)) < 0)
    {
        send_message(-OPEN_MEM_ERR, ERROR, open_mem_err);
    }
    else
        mem_opened = TRUE;
 
    /* map a page to physical memory */
    if ((mmap(vaddr, 512, PROT_READ | PROT_WRITE,
        MAP_FIXED | MAP_SHARED,
        cache_test_fd, (off_t)0)) == (caddr_t)-1)
    {
        send_message(-MMAP_ERR, ERROR, mmap_err, "Cache Consistency Test");
    }
 
    /* lock down a page to physical memory */
    mlock(vaddr, CACHE_BLOCK);
 
    memset(vaddr, 0x8f, CACHE_BLOCK);
    wrtptr = vaddr;
    *wrtptr = 0;
    ordinal = 0;
 
    TRACE_OUT
}

cache_consistency_test(process_number, cpu_mask)
int process_number;
unsigned cpu_mask;
{
    int cpu;

    cpu = nint(log2((double)cpu_mask));

    /* one line cache test */
    one_cache_line_test(process_number, cpu);

    /* multiple line cache test */
    multi_cache_line_test(process_number, cpu);

    munlock(vaddr, CACHE_BLOCK);
}

one_cache_line_test(process_number, cpu)
int process_number; 
int cpu;
{

    int lines, v;

    lines = CACHE_LINE;

    send_message(0, VERBOSE, single_cache_test_start, process_number, cpu);

    while (lines--)
    {
        send_message(0, DEBUG, single_line_cache_spin, process_number, cpu);
 
        while (*wrtptr != ordinal);

        send_message(0, DEBUG, single_line_cache_write, process_number, cpu, *wrtptr);

        v = rotate(number_processors, ordinal);

        send_message(0, DEBUG, single_line_cache_wrote, process_number, cpu, v);

        if ( v )
        {      /* not the last cpu */
            *wrtptr = v ;
            wrtptr++;
        }
        else            /* last cpu */
        {
                send_message(0, DEBUG, next_single_line_cache);
                wrtptr++;
                *wrtptr = v ;
        }
    }
    send_message(0, VERBOSE, single_cache_test_end, process_number, cpu);
}

multi_cache_line_test(process_number, cpu_mask)
int process_number; 
unsigned cpu_mask;
{
    int lines, v, n, cpu;
 
    lines = n = number_processors;

    send_message(0, VERBOSE, multi_cache_test_start, process_number, cpu);

    while (lines--)
    {
        send_message(0, DEBUG, 
        multi_line_cache_spin, process_number, cpu);

        while (*wrtptr != ordinal);

        send_message(0, DEBUG,
        multi_line_cache_write, process_number, cpu, *wrtptr);

        v = rotate(n, ordinal);
        n = number_processors;

        send_message(0, DEBUG, 
        multi_line_cache_wrote, process_number, cpu, v);

        if ( v )       /* not the last cpu */
        {
            *wrtptr = v ;
            wrtptr += (CACHE_LINE/sizeof(int)) ;
        }
        else            /* last cpu */
        {
            send_message(0, DEBUG, next_multi_line_cache);
            wrtptr += (CACHE_LINE/sizeof(int)) ;
            *wrtptr = v ;
        }
    }
    send_message(0, VERBOSE, multi_cache_test_end, process_number, cpu);
}
