/*
 *       %Z%%M% %I% %E% Copyright(c) Sun Microsystems, Inc.
 */

/************************************************************************/
/*                                                                      */
/*   NAME:                                                              */
/*      MP shared memory test                                           */
/*                                                                      */
/*   DESCRIPTION:                                                       */
/*      To device an on-line MP data I/O test requires two or more      */
/*      unix processes, each locks to one CPU, to writes data           */
/*      to the dataio memory.  The modified data is immediately         */
/*      recognized by the other CPUs being tested.                      */
/*                                                                      */
/************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "mptest.h"
#include "mptest_msg.h"
#include "sdrtns.h"

/* variables for data I/O test */
int io_testfile_create, io_testfile_fd, dataio_pagesize;
char iotestfile[MAX_STRING];
unsigned char *mmap_addr;

extern int number_processors;
extern int errno;
extern char *sys_errlist[];
 
setup_io_test()
{

    int pid;
    unsigned char *temp_address;
    char *pagedata;
    caddr_t mmap();
    char *valloc();

    func_name = "io_test";
    TRACE_IN

    io_testfile_create = FALSE;

    /* get page size */
    dataio_pagesize = getpagesize();

    /* Create a unique /tmp filename */
    pid = getpid();

    /* Intitialize array test file */
    bzero(iotestfile, MAX_STRING);
    sprintf(iotestfile, "%s%d", "/tmp/iotest.", pid);
 
    /* Open the test file */
    if ((io_testfile_fd = open(iotestfile, O_RDWR | O_CREAT | O_SYNC, 0666)) ==
-1)
    {
        send_message(-OPEN_FILE_ERR, ERROR, open_file_err, iotestfile);
    }
    else
    {
        io_testfile_create = TRUE;
    }
    if (io_testfile_create)
    {
        /* Put null characters into file before mmap */
        pagedata = (char *)calloc(dataio_pagesize, sizeof(char));
        write(io_testfile_fd, pagedata, dataio_pagesize);
        free(pagedata);
 
 
        /* Do mmap */
        mmap_addr = (unsigned char *)valloc(dataio_pagesize);
        if (((unsigned char *)mmap((char *)mmap_addr, dataio_pagesize,
            PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, io_testfile_fd, 0))
            == (unsigned char *)-1)
        {
            send_message(-MMAP_ERR, ERROR, mmap_err, "Data I/O Test");
            exit(1);
        }
 
        /* Clear memory */
        init_mem(mmap_addr, dataio_pagesize, CLEAR);
    }
 
    TRACE_OUT
 
}

init_mem(start_address, size, value)
unsigned char *start_address;
int size;
unsigned char value;
{
 
    int number_words;
    unsigned char *temp_address;
 
    func_name = "init_mem";
    TRACE_IN
 
    number_words = size;
    temp_address = start_address;
    while (number_words--)
    {
        *temp_address++ = value;
    }
       
    TRACE_OUT
}
 
io_test(process_number, processor, start_address, size, first_writer)
unsigned char *start_address;
int process_number, processor, size, first_writer;
{
    unsigned char *begin_read_write_address;
    unsigned char *flag_array;
    int writer, i, j, number_sections, test_num;

    func_name = "io_test";
    TRACE_IN

    send_message(0, VERBOSE, io_test_start, process_number, processor);
 
    /* Assign the first writer */
    writer = first_writer;
 
    /* Find out either or both i/o test, shared memory test */
    test_num = (first_writer / number_processors) + 1;
 
   /*
    * Calculate number of sections in the page
    * use the first 16 bytes for processors' flags
    */
    number_sections = (size - FLAG_SIZE) / BLOCK_SIZE;

    /* Get the starting address for reading and writing section */
    flag_array = start_address;
    begin_read_write_address = start_address + 16;
 
    while (number_sections--)
    {
        /* This processor is a writer */
        if (writer == process_number)
        {
 
            /* Write 16 bytes at a time */
            send_message(0, DEBUG, mem_write, "Data I/O Test", process_number, processor);
            write_mem(processor, begin_read_write_address, PATTERN, BLOCK_SIZE); 
            /* Set done writing for the writer flag */
            flag_array[writer] = DONE_WRITING;
 
            /*
             * Loop until everyone else (except the writer) is done with reading             * Clear each reader after he is done
             */
            for (i = first_writer; i < test_num*number_processors; i++)
            {
                if (i != writer)
                {
                    while (flag_array[i] != DONE_READING);
                    flag_array[i] = CLEAR;
                }
            }
 
            /* Clear the writer flag */
            flag_array[writer] = CLEAR;
        }
        /* This processor is a reader */
        else
        {
 
            /* Looping until the writer is done with writing */
            while (flag_array[writer] != DONE_WRITING);
 
            /* Read 16 bytes at a time */
            send_message(0, DEBUG, mem_read, "Data I/O Test", process_number, processor);
            read_mem(processor, begin_read_write_address, PATTERN, BLOCK_SIZE);
 
            /* Set done reading flag */
            flag_array[process_number] = DONE_READING;
 
            /* Looping until the writer is clear */
            while (flag_array[writer] == DONE_WRITING);
 
        } /* else */
 
        /* Calculate the beginning address for next section */
        begin_read_write_address += 16;
 
        /* Calculate new writer */
        writer = (writer + 1) % (test_num*number_processors);
        if ((first_writer != 0) && (writer == 0))
            writer = first_writer;
 
    } /* while */

    send_message(0, VERBOSE, io_test_end, process_number, processor);
 
    TRACE_OUT
}
 
write_mem(processor, start_address, pattern, size)
int processor;
unsigned char *start_address;
unsigned char pattern;
int size;
{
 
    int number_words;
    unsigned char *temp_address;

    func_name = "write_mem";
    TRACE_IN
 
    temp_address = start_address;
    number_words = size/sizeof(unsigned long);
    while (number_words--)
    {
        *temp_address = pattern;
        temp_address++;
    }
 
    TRACE_OUT
 
}
 
int
read_mem(processor, start_address, pattern, size)
int processor;
unsigned char *start_address;
unsigned char pattern;
int size;
{
 
    unsigned char read_data;
    unsigned char *temp_address;
    int number_words, error, count;
 
    func_name = "read_mem";
    TRACE_IN
 
    number_words = size/sizeof(unsigned long);
    error = 0;
    count = 0;
 
    temp_address = start_address;
 
    while (count < number_words)
    {
        read_data = *temp_address;      /* read shared mem data */
 
        if (read_data != pattern)
        {
            send_message(-RDM_COMPARE_ERR, ERROR, processor, read_data, PATTERN);
 
            error++;
            break;
        }
        else
        {
            temp_address++;
            count++;
        }
    }
 
    TRACE_OUT
    return(error);
 
}

