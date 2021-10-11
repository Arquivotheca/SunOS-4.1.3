static char     sccsid[] = "@(#)gpmtest.c 1.1 7/30/92 Copyright 1985 Sun Micro";

/*
 * gpmtest.c: executes a series of microcode test on the Graphics Processor
 * and Graphics Buffer boards
 * 
 * Each test starts by the host writing data to shared memory.  While the host
 * reads this data, the GP passes the data through its hardware and finally
 * back into shared memory.  The host then compares this data with the
 * written data and reports errors.
 * 
 * A series of tests exists, each touching different parts of the GP and GB.
 * These tests run in sequence;  the last test touches nearly all of the GP
 * and GB boards.
 * 
 * Jan 1985 rev 28 Mar 1985 - made pp prom test an option and added exit(6) and
 * exit(7) and added creation date output message John Fetter
 * 
 * May 1985 rev 10 May 1985 - Modified to run under Sundiag as gpmtest. Frank
 * Jones
 * 
 * June 1985 rev 3 June 1985 - Removed .o off the micro-code files. Frank Jones
 * 
 * Possible improvements: add microstore write/read tests on error exit, reset
 * gp
 */

#include "gp1.h"
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <strings.h>
#include <sys/mman.h>
#include "sdrtns.h" 		/* standard sundiag include */
#include "../../../lib/include/libonline.h"   /* online library include */


char            file_name_buffer[30];
char           *file_name = file_name_buffer;

extern char     *hostname;

#define BLOCK_SIZE_1	8*1024		       /* half of shared memory in
					        * 16-bit words */
#define BLOCK_SIZE_2	4*1024		       /* quarter of shared memory in
					        * 16-bit words */
#define GP_TESTS	20
#define GB_TESTS	6
#define PASS_COUNT	10
#define PROM_SIZE	16*1024
#define	HUNG_STATE	10
#define PPPROM_TEST	25

short          *gp1_base;
short          *gp1_shmem;

static int      gp1_fd;
static caddr_t  allocp;

int		pass = 0;
int		errors = 0;
char		msg_buffer[MESSAGE_SIZE];
char		*msg = msg_buffer;

int             simulate_error = 0;
char            perror_msg_buffer[30];
char           *perror_msg = perror_msg_buffer;

char            sundiag_directory[50];
char           *SD = sundiag_directory;
char		*test_usage_msg = "[gb] [ppprom]";
char		*routine_msg    = "%s specific arguments:\n\
	gb	= graphics buffer\n\
	ppprom	= pp prom\n"; 

int             test;
static int	num_of_tests;
static int	ppprom_flag;

main(argc, argv)
    int             argc;
    char          **argv;
{
    int             passes;
    int             start_test;
    char            state[256];
    extern int process_gpmtest_args();
    extern int routine_usage();

    versionid = "1.1";
    device_name = "gpone0";
    start_test = 1;
    num_of_tests = GP_TESTS;
    ppprom_flag = 0;
    test_init(argc, argv, process_gpmtest_args, routine_usage, test_usage_msg);

    sprintf(perror_msg, "%s: perror says", test_name);

    if (verbose) {
	sprintf(msg, "Started testing graphics processor%s.", 
		num_of_tests == GP_TESTS ? "" : " and buffer");
	send_message(0, VERBOSE, msg);
    }

    gp1_open();

    initstate(1000, state, 256);
    passes = (quick_test) ? 1 : PASS_COUNT;

    while (pass++ == 0) {

	for (test = start_test; test <= num_of_tests; test++) {
	    if (test != PPPROM_TEST | ppprom_flag == 1)
		runtest(test, passes);
	    else {
		if (!(exec_by_sundiag)) {
		    printf("TEST %2d: ", PPPROM_TEST);
		    printf("you did not enable the pp prom test");
		    printf("\n\n");
		}
	    }
	}
	if (quick_test)
	    break;

	sleep(5);
    }
    gp1_reset();
    test_end();
}

routine_usage()
{
	send_message(0, CONSOLE, routine_msg, test_name);
}

/* function to load selected microcode and run a test */

runtest(test, passes)
    int             test, passes;
{
    int             errcount;
    gp1_reset();

    switch (test) {			       /* Viewing Processor only
					        * tests */

    case 1:
	gp1_load("gpmtest.shmem.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->shmem (byte)", test);
	break;
    case 2:
	gp1_load("gpmtest.shmem.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->shmem (word)", test);
	break;
    case 3:
#ifndef sun4
	gp1_load("gpmtest.shmem.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->shmem (integer)", test);
#endif
	break;
    case 4:
	gp1_load("gpmtest.vp_29116.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->VP_29116->shmem", test);
	break;
    case 5:
	gp1_load("gpmtest.fprega.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->fpreg A->shmem", test);
	break;
    case 6:
	gp1_load("gpmtest.fpregb.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->fpreg B->shmem", test);
	break;
    case 7:
	gp1_load("gpmtest.fpalu.2p");
	if (verbose) 
	    sprintf
	     (msg,"TEST %2d: shmem->fpreg->Weitek ALU->fpreg->shmem", test);
	break;
    case 8:
	gp1_load("gpmtest.fpmult.2p");
	if (verbose) 
	    sprintf
	      (msg,"TEST %2d: shmem->fpreg->Weitek ALU & MULT->fpreg->shmem",
		 test);
	break;
    case 9:
	gp1_load("gpmtest.vpprom.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: vpprom->shmem", test);
	break;
	/* Viewing Processor and Painting Processor tests */
    case 10:
	gp1_load("gpmtest.fifo_vme.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->fifo->VME (word)->shmem", test);
	break;
    case 11:
	gp1_load("gpmtest.fifo_vme_dec.2p");
	if (verbose) sprintf(msg, 
	    "TEST %2d: shmem->fifo->VME (dec count)->shmem", test);
	break;
    case 12:
	gp1_load("gpmtest.vme_byte.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->fifo->VME (byte)->shmem", test);
	break;
    case 13:
	gp1_load("gpmtest.int_flag.2p");
	if (verbose) 
	    sprintf
	      (msg,"TEST %2d: shmem->fifo->VME->shmem (test int flag)", test);
	break;
    case 14:
	gp1_load("gpmtest.vme_read.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->VME (word)->fifo->shmem", test);
	break;
    case 15:
	gp1_load("gpmtest.vme_read_byte.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->VME (byte)->fifo->shmem", test);
	break;
    case 16:
	gp1_load("gpmtest.pp_29116.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->fifo->PP_29116->VME->shmem", test);
	break;
    case 17:
	gp1_load("gpmtest.scrpad.2p");
	if (verbose) 
	    sprintf(msg, "TEST %2d: shmem->fifo->scrpad->VME->shmem", test);
	break;
    case 18:
	gp1_load("gpmtest.ppfifo.2p");
	if (verbose)
	    sprintf(msg, "TEST %2d: shmem->fifo->scrpad->fifo->shmem", test);
	break;
    case 19:
	gp1_load("gpmtest.fifo_vme.2p");
	if (verbose)
	    sprintf(msg, "TEST %2d: status flags", test);
	break;
    case 20:
	gp1_load("gpmtest.allbutgb.2p");
	if (verbose) sprintf(msg,
	    "TEST %2d: shmem->VP_29116->fpreg->fifo->PP_29116->scrpad->VME->shmem", test);
	break;
	/* Graphics Buffer tests */
    case 21:
	gp1_load("gpmtest.xoperand.2p");
	if (verbose)
	    sprintf
		(msg, "TEST %2d: shmem->fifo->X_operand->VME->shmem", test);
	break;
    case 22:
	gp1_load("gpmtest.yoperand.2p");
	if (verbose)
	    sprintf
		(msg, "TEST %2d: shmem->fifo->Y_operand->VME->shmem", test);
	break;
    case 23:
	gp1_load("gpmtest.gbnorm.2p");
	if (verbose)
	    sprintf(msg,
	      "TEST %2d: shmem->fifo->g_buffer (normal)->VME->shmem", test);
	break;
    case 24:
	gp1_load("gpmtest.gbrmw.2p");
	if (verbose)
	    sprintf(msg,
		"TEST %2d: shmem->fifo->g_buffer (rmw)->VME->shmem", test);
	break;
    case 25:
	gp1_load("gpmtest.ppprom.2p");
	if (verbose)
	    sprintf(msg, "TEST %2d: shmem->pprom->VME->shmem", test);
	break;
    case 26:
	gp1_load("gpmtest.all.2p");
	if (verbose)
	    sprintf(msg, "TEST %2d: shmem->VP_29116->fpreg->fifo->PP_29116->scrpad->int_mult->gb->VME->shmem", test);
	break;
    }
    if (verbose) send_message(0, VERBOSE, msg);
    gp1_vp_start(0);
    gp1_pp_start(0);
    switch (test) {
	/* Viewing Processor only tests */
    case 1:
	errcount = shmem_test16k_byte(passes);
	break;
    case 2:
	errcount = shmem_test16k_word(passes);
	break;
    case 3:
#ifdef sun4
	errcount = 0;
#else
	errcount = shmem_test16k_int(passes);
#endif
	break;
    case 4:
	errcount = shmem_test8k(passes);
	break;
    case 5:
	errcount = shmem_test8k(passes);
	break;
    case 6:
	errcount = shmem_test8k(passes);
	break;
    case 7:
	errcount = shmem_test8k(passes);
	break;
    case 8:
	errcount = shmem_test8k(passes);
	break;
    case 9:
	errcount = test_prom();
	break;
	/* Viewing Processor and Painting Processor tests */
    case 10:
	errcount = shmem_test8k(passes);
	break;
    case 11:
	errcount = shmem_test8k(passes);
	break;
    case 12:
	errcount = shmem_test8k(passes);
	break;
    case 13:
	errcount = shmem_iflag(passes);
	break;
    case 14:
	errcount = shmem_test8k(passes);
	break;
    case 15:
	errcount = shmem_test8k(passes);
	break;
    case 16:
	errcount = shmem_test8k(passes);
	break;
    case 17:
	errcount = shmem_test8k(passes);
	break;
    case 18:
	errcount = shmem_test8k(passes);
	break;
    case 19:
	errcount = test_sflag();
	break;
    case 20:
	errcount = shmem_test8k(passes);
	break;
	/* Graphics Buffer tests */
    case 21:
	errcount = shmem_test8k(passes);
	break;
    case 22:
	errcount = shmem_test8k(passes);
	break;
    case 23:
	errcount = shmem_testgb();
	break;
    case 24:
	errcount = shmem_testgb();
	break;
    case 25:
	errcount = test_prom();
	break;
    case 26:
	errcount = shmem_test8k(passes);
	break;
    }
    return (errcount);
}

/* function to test shared memory with byte accesses */
shmem_test16k_byte(passes)
    int             passes;
{
    int             pass, error1 = 0, error2 = 0, i;
    unsigned char   readback, data[2 * BLOCK_SIZE_1];
    unsigned char  *ptr;

    for (pass = 1; pass <= passes; pass++) {
	/* Write data to shared memory */
	ptr = (unsigned char *) &gp1_shmem[0x0];
	for (i = 0; i < 2 * BLOCK_SIZE_1; i++) {
	    data[i] = (char) random();
	    *ptr++ = data[i];
	}
	if (simulate_error == TEST1_VERIFY_ERROR ||
	    simulate_error == TEST1_PATH_ERROR) {
	    ptr = (unsigned char *) &gp1_shmem[0x10];
	    readback = *ptr;
	    *ptr = readback + 1;
	}
	/* Readback the just-written data */
	ptr = (unsigned char *) &gp1_shmem[0x0];
	for (i = 0; i < 2 * BLOCK_SIZE_1; i++) {
	    readback = *ptr++;
	    if (readback != data[i] && simulate_error != TEST1_PATH_ERROR) {
		error1++;
		errors++;
		sprintf(msg, 
"data compare error on memory verify, byte address = 0x%x, exp = 0x%x, actual = 0x%x, test %.", i, data[i], readback, test);
		send_message(-TEST1_VERIFY_ERROR, ERROR, msg);
	    }
	}
	/* Readback the data passed through the GP */
	ptr = (unsigned char *) &gp1_shmem[BLOCK_SIZE_1];
	for (i = 0; i < 2 * BLOCK_SIZE_1; i++) {
	    readback = *ptr++;
	    if (readback != data[i]) {
		error2++;
		errors++;
		sprintf(msg, 
"data compare error, byte address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.", 
		    i + 2 * BLOCK_SIZE_1, data[i], readback, test);
		send_message(-TEST1_PATH_ERROR, ERROR, msg);
	    }
	}
    }
    return (error1 + error2);
}

/* function to test shared memory with word (16-bit) accesses */
shmem_test16k_word(passes)
    int             passes;
{
    int             pass, error1 = 0, error2 = 0, i;
    unsigned short  readback, data[BLOCK_SIZE_1];
    unsigned short *ptr;

    for (pass = 1; pass <= passes; pass++) {
	/* Write data to shared memory */
	ptr = (unsigned short *) &gp1_shmem[0x0];
	for (i = 0; i < BLOCK_SIZE_1; i++) {
	    data[i] = (short) random();
	    *ptr++ = data[i];
	}
	if (simulate_error == TEST2_VERIFY_ERROR ||
	    simulate_error == TEST2_PATH_ERROR) {
	    ptr = (unsigned short *) &gp1_shmem[0x10];
	    readback = *ptr;
	    *ptr = readback + 1;
	}
	/* Readback the just-written data */
	ptr = (unsigned short *) &gp1_shmem[0x0];
	for (i = 0; i < BLOCK_SIZE_1; i++) {
	    readback = *ptr++;
	    if (readback != data[i] && simulate_error != TEST2_PATH_ERROR) {
		error1++;
		errors++;
		sprintf(msg,
"data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.", i, data[i], readback, test);
		send_message(-TEST2_VERIFY_ERROR, ERROR, msg);
	    }
	}
	/* Readback the data passed through the GP */
	ptr = (unsigned short *) &gp1_shmem[BLOCK_SIZE_1];
	for (i = 0; i < BLOCK_SIZE_1; i++) {
	    readback = *ptr++;
	    if (readback != data[i]) {
		error2++;
		errors++;
		sprintf(msg,
"data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.",
		    i + BLOCK_SIZE_1, data[i], readback, test);
		send_message(-TEST2_PATH_ERROR, ERROR, msg);
	    }
	}
    }
    return (error1 + error2);
}

/* function to test shared memory with integer (32-bit) accesses */
shmem_test16k_int(passes)
    int             passes;
{
    int             pass, error1 = 0, error2 = 0, i;
    unsigned int    readback, data[2 * BLOCK_SIZE_1];
    unsigned int   *ptr;

    for (pass = 1; pass <= passes; pass++) {
	/* Write data to shared memory */
	ptr = (unsigned int *) &gp1_shmem[0x0];
	for (i = 0; i < (BLOCK_SIZE_1 / 2); i++) {
	    data[i] = (int) random();
	    *ptr++ = data[i];
	}
	if (simulate_error == TEST3_VERIFY_ERROR ||
	    simulate_error == TEST3_PATH_ERROR) {
	    ptr = (unsigned int *) &gp1_shmem[0x10];
	    readback = *ptr;
	    *ptr = readback + 1;
	}
	/* Readback the just-written data */
	ptr = (unsigned int *) &gp1_shmem[0x0];
	for (i = 0; i < (BLOCK_SIZE_1 / 2); i++) {
	    readback = *ptr++;
	    if (readback != data[i] && simulate_error != TEST3_PATH_ERROR) {
		error1++;
		errors++;
		sprintf(msg,
"data compare error on memory verify, integer address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.", i, data[i], readback, test);
		send_message(-TEST3_VERIFY_ERROR, ERROR, msg);
	    }
	}
	/* Readback the data passed through the GP */
	ptr = (unsigned int *) &gp1_shmem[BLOCK_SIZE_1];
	for (i = 0; i < (BLOCK_SIZE_1 / 2); i++) {
	    readback = *ptr++;
	    if (readback != data[i]) {
		error2++;
		errors++;
		sprintf(msg,
"data compare error, integer address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.",
		    i + (BLOCK_SIZE_1 / 2), data[i], readback, test);
		send_message(-TEST3_PATH_ERROR, ERROR, msg);
	    }
	}
    }
    return (error1 + error2);
}

/* function to test shmem -> ... -> shmem microcode */
shmem_test8k(passes)
    int             passes;
{
    int             pass, error1 = 0, error2 = 0, i, hung_count = 0;
    unsigned short  readback, data[BLOCK_SIZE_2];
    short          *ptr;

    for (pass = 1; pass <= passes; pass++) {
	/* write data to shared memory */
	ptr = (short *) &gp1_shmem[0x1000];
	for (i = 0; i < BLOCK_SIZE_2; i++) {
	    data[i] = (short) random();
	    *ptr++ = data[i];
	}
	if (simulate_error == TEST4_VERIFY_ERROR ||
	    simulate_error == TEST4_PATH_ERROR) {
	    ptr = (short *) &gp1_shmem[0x1010];
	    readback = *ptr;
	    *ptr = readback + 1;
	}
	gp1_shmem[0] = 0x800;		       /* set semaphore */
	hung_count = 0;
	do {				       /* readback the just-written
					        * data */
	    ptr = (short *) &gp1_shmem[0x1000];
	    for (i = 0; i < BLOCK_SIZE_2; i++) {
		readback = *ptr++;
		if (readback != data[i] && simulate_error != TEST4_PATH_ERROR) {
		    error1++;
		    errors++;
		    sprintf(msg,
"data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.", i + 0x1000, data[i], readback, test);
		    send_message(-TEST4_VERIFY_ERROR, ERROR, msg);
		}
	    }
	    if (++hung_count == HUNG_STATE) {
		errors++;
		sprintf(msg, "the graphics processor hung, test %d.", test);
		send_message(-TEST4_HUNG_ERROR, ERROR, msg);
	    }
	}				       /* semaphore reset ?? */
	while (gp1_shmem[0] != 0 || simulate_error == TEST4_HUNG_ERROR);
	/* readback the data passed through the GP */
	ptr = (short *) &gp1_shmem[0x2000];
	for (i = 0; i < BLOCK_SIZE_2; i++) {
	    readback = *ptr++;
	    if (readback != data[i]) {
		error2++;
		errors++;
		sprintf(msg,
"data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.",
		    i + 0x2000, data[i], readback, test);
		send_message(-TEST4_PATH_ERROR, ERROR, msg);
	    }
	}
    }
    return (error1 + error2);
}

/* function to read shmem contents (prom -> shmem) and test checksum */
test_prom()
{
    int             i, checksum = 0;
    unsigned short  readback;
    int             error = 0;
    /* ensure time for GP to move prom contents to shared memory */
    sleep(2);
    /* compute and check checksum */
    for (i = 0; i < PROM_SIZE - 1; i++) {
	checksum += gp1_shmem[i];
	checksum &= 0xffff;
    }
    readback = gp1_shmem[PROM_SIZE - 1];
    if (simulate_error == CHECKSUM_ERROR)
	readback++;
    if (((unsigned short) checksum) == readback) {
	send_message(0, VERBOSE, "CHECKSUM MATCH; value= %x", checksum);
    } else {
	error++;
	errors++;
	send_message(-CHECKSUM_ERROR, ERROR, "checksum incorrect, \
		checksum exp = 0x%x, checksum actual = 0x%x, test %d.",
		checksum, readback, test);
    }
    return (error);
}

/* function to test vme-readable status flags */
test_sflag()
{
    int             pass, passes = 32, error1 = 0, error2 = 0, error3 = 0;
    int             i, hung_count = 0;
    unsigned short  readback, data[BLOCK_SIZE_2];
    short          *ptr;
    unsigned short  sflag4, sflag8;

    if (quick_test)
	passes = 1;

    for (pass = 1; pass <= passes; pass++) {

	/*
	 * Mask off the last four bits of pass number and (ones) complement
	 * them. This should equal the state of the Viewing Processor and
	 * Painting Processor status flags
	 */

	sflag4 = 0xf - (pass & 0xf);
	sflag8 = (sflag4 << 4) + sflag4;
	readback = gp1_base[GP1_STATUS_REG] & 0xff;
	if (simulate_error == STATUS_ERROR)
	    readback++;
	if (sflag8 != readback) {
	    error3++;
	    errors++;
	    sprintf(msg,
 	  "status incorrect, exp = 0x%x, actual = 0x%x, test %d.",
		sflag8, readback, test);
	    send_message(-STATUS_ERROR, ERROR, msg);
	}
	/* write data to shared memory */
	ptr = (short *) &gp1_shmem[0x1000];
	for (i = 0; i < BLOCK_SIZE_2; i++) {
	    data[i] = (short) random();
	    *ptr++ = data[i];
	}
	if (simulate_error == TEST5_VERIFY_ERROR ||
	    simulate_error == TEST5_PATH_ERROR) {
	    ptr = (short *) &gp1_shmem[0x1010];
	    readback = *ptr;
	    *ptr = readback + 1;
	}
	gp1_shmem[0] = 0x800;		       /* set semaphore */
	hung_count = 0;

	/*
	 * readback the just-written data until the GP is done moving the
	 * data
	 */
	do {
	    ptr = (short *) &gp1_shmem[0x1000];
	    for (i = 0; i < BLOCK_SIZE_2; i++) {
		readback = *ptr++;
		if (readback != data[i] && simulate_error != TEST5_PATH_ERROR) {
		    error1++;
		    errors++;
		    sprintf(msg,
"data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.", i + 0x1000, data[i], readback, test);
		    send_message(-TEST5_VERIFY_ERROR, ERROR, msg);
		}
	    }
	    if (++hung_count == HUNG_STATE) {
		errors++;
		sprintf(msg, 
		    "the graphics processor hung, test %d.", test);
		send_message(-TEST5_HUNG_ERROR, ERROR, msg);
	    }
	}				       /* semaphore reset ?? */
	while (gp1_shmem[0] != 0 || simulate_error == TEST5_HUNG_ERROR);
	/* readback the data passed through the GP by the microcode */
	ptr = (short *) &gp1_shmem[0x2000];
	for (i = 0; i < BLOCK_SIZE_2; i++) {
	    readback = *ptr++;
	    if (readback != data[i]) {
		error2++;
		errors++;
		sprintf(msg,
"data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.",
		    i + 0x2000, data[i], readback, test);
		send_message(-TEST5_PATH_ERROR, ERROR, msg);
	    }
	}
    }
    return (error1 + error2 + error3);
}

/*
 * function to write an integer to/from the vme bus by making it into two
 * 16-bit pieces of data. This solves the VME bus error problem encountered
 * on the Sun4 architecture
 */
move_int(p1, p2)
    register short *p1, *p2;
{
    *p1++ = *p2++;
    *p1 = *p2;
}

/* function to test shmem -> ... -> shmem with graphics buffer microcode */
shmem_testgb()
{
    int             pass, passes = 256, error1 = 0, error2 = 0, i, hung_count = 0;
    int             gbaddr;
    unsigned short  readback, data[BLOCK_SIZE_2];
    short          *ptr;
    int            *ptr_gbaddr;

    if (quick_test)
	passes = 1;

    ptr_gbaddr = (int *) &gp1_shmem[2];
    for (pass = 1; pass <= passes; pass++) {
	/* 256 = 1 Megaword divided by 4k words */
	/* write data to shared memory */
	ptr = (short *) &gp1_shmem[0x1000];
	for (i = 0; i < BLOCK_SIZE_2; i++) {
	    data[i] = (short) random();
	    *ptr++ = data[i];
	}
	gbaddr = 0x1000 * (pass - 1);
	move_int(ptr_gbaddr, &gbaddr);
	/* *ptr_gbaddr = gbaddr; */

	if (simulate_error == TEST6_VERIFY_ERROR ||
	    simulate_error == TEST6_PATH_ERROR) {
	    ptr = (short *) &gp1_shmem[0x1010];
	    readback = *ptr;
	    *ptr = readback + 1;
	}
	gp1_shmem[0] = 0x800;		       /* set semaphore */
	hung_count = 0;

	/*
	 * readback the just-written data until the GP is done moving the
	 * data
	 */
	do {
	    ptr = (short *) &gp1_shmem[0x1000];
	    for (i = 0; i < BLOCK_SIZE_2; i++) {
		readback = *ptr++;
		if (readback != data[i] && simulate_error != TEST6_PATH_ERROR) {
		    error1++;
		    errors++;
		    sprintf(msg,
"data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.", i + 0x1000, data[i], readback, test);
		    send_message(-TEST6_VERIFY_ERROR, ERROR, msg);
		}
	    }
	    if (++hung_count == HUNG_STATE) {
		errors++;
		sprintf(msg, "the graphics processor hung, test %d.", test);
		send_message(-TEST6_HUNG_ERROR, ERROR, msg);
	    }
	}				       /* semaphore reset ?? */
	while (gp1_shmem[0] != 0 || simulate_error == TEST6_HUNG_ERROR);
	/* readback the data passed through the GP by the microcode */
	ptr = (short *) &gp1_shmem[0x2000];
	for (i = 0; i < BLOCK_SIZE_2; i++) {
	    readback = *ptr++;
	    if (readback != data[i]) {
		error2++;
		errors++;
		sprintf(msg,
"data compare error, word address = 0x%x, gb address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.", i + 0x2000, i + gbaddr, data[i], readback, test);
		send_message(-TEST6_PATH_ERROR, ERROR, msg);
	    }
	}
    }
    return (error1 + error2);
}

/*
 * function to test shmem -> ... -> shmem microcode using interrupt flag as
 * reset semaphore instead of shared memory location 0
 */
shmem_iflag(passes)
    int             passes;
{
    int             pass, error1 = 0, error2 = 0, error3 = 0, i, hung_count = 0;
    unsigned short  readback, data[BLOCK_SIZE_2];
    short          *ptr;
    /* ensure interrupt enable is turned off */
    if (simulate_error == INT1_ERROR)
	gp1_base[GP1_CONTROL_REG] = 0x0100;
    else
	gp1_base[GP1_CONTROL_REG] = 0x0200;
    if ((gp1_base[GP1_STATUS_REG] & 0x4000) != 0) {
	errors++;
	sprintf(msg, "interrupt cannot be disabled, test %d.", test);
	send_message(-INT1_ERROR, ERROR, msg);
    }
    for (pass = 1; pass <= passes; pass++) {
	/* write data to shared memory */
	ptr = (short *) &gp1_shmem[0x1000];
	for (i = 0; i < BLOCK_SIZE_2; i++) {
	    data[i] = (short) random();
	    *ptr++ = data[i];
	}
	if (simulate_error == TEST7_VERIFY_ERROR ||
	    simulate_error == TEST7_PATH_ERROR) {
	    ptr = (short *) &gp1_shmem[0x1010];
	    readback = *ptr;
	    *ptr = readback + 1;
	}
	gp1_shmem[0] = 0;		       /* reset semaphore for VP */
	gp1_shmem[0] = 0x800;		       /* set semaphore */
	hung_count = 0;

	/*
	 * readback the just-written data until the GP is done moving the
	 * data
	 */
	do {
	    ptr = (short *) &gp1_shmem[0x1000];
	    for (i = 0; i < BLOCK_SIZE_2; i++) {
		readback = *ptr++;
		if (readback != data[i] && simulate_error != TEST7_PATH_ERROR) {
		    error1++;
		    errors++;
		    sprintf(msg,
"data compare error on memory verify, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.", i + 0x1000, data[i], readback, test);
		    send_message(-TEST7_VERIFY_ERROR, ERROR, msg);
		}
	    }
	    if (++hung_count == HUNG_STATE) {
		errors++;
		sprintf(msg, "the graphics processor hung, test %d.", test);
		send_message(-TEST7_HUNG_ERROR, ERROR, msg);
	    }
	}
	/* semaphore (int flag) reset? ? */
	while ((gp1_base[GP1_STATUS_REG] & 0x8000) == 0 ||
	       simulate_error == TEST7_HUNG_ERROR);
	/* reset interrupt flag (the PP semaphore) */
	if (simulate_error == INT2_ERROR)
	    gp1_base[GP1_CONTROL_REG] = 0x0000;
	else
	    gp1_base[GP1_CONTROL_REG] = 0x8000;
	/* did the interrupt flag reset */
	if ((gp1_base[GP1_STATUS_REG] & 0x8000) != 0) {
	    error3++;
	    errors++;
	    sprintf(msg, "interrupt flag did not reset, test %d.", test);
	    send_message(-INT2_ERROR, ERROR, msg);
	}
	/* readback the data passed through the GP by the microcode */
	ptr = (short *) &gp1_shmem[0x2000];
	for (i = 0; i < BLOCK_SIZE_2; i++) {
	    readback = *ptr++;
	    if (readback != data[i]) {
		error2++;
		errors++;
		sprintf(msg,
"data compare error, word address = 0x%x, exp = 0x%x, actual = 0x%x, test %d.",
		    i + 0x2000, data[i], readback, test);
		send_message(-TEST7_PATH_ERROR, ERROR, msg);
	    }
	}
    }
    return (error1 + error2 + error3);
}

gp1_open()
{
    int             i, p;
    int             align;
    caddr_t         gpm;
    extern char    *malloc();

    if (simulate_error == VME24_NOT_OPEN)
	strcpy(file_name, "/dev/vme24.invalid");
    else
	strcpy(file_name, "/dev/vme24");

    if ((gp1_fd = open(file_name, O_RDWR)) < 0) {
	perror(perror_msg);
	sprintf(msg, "Couldn't open file '%s'.", file_name);
	send_message(-VME24_NOT_OPEN, FATAL, msg);
    }
    align = getpagesize();
    if ((allocp = malloc(VME_GP1SIZE + align)) == 0 ||
	simulate_error == NO_MALLOC) {
	perror(perror_msg);
	send_message(-NO_MALLOC, FATAL, "Couldn't allocate address space.");
    }
    p = ((int) allocp + align - 1) & ~(align - 1);
    if (((i = (int)mmap(p, VME_GP1SIZE, PROT_READ|PROT_WRITE,
        MAP_SHARED|MAP_FIXED, gp1_fd, VME_GP1BASE)) == -1) ||
        (i != p) ||
	simulate_error == NO_MAP) {
	perror(perror_msg);
	send_message(-NO_MAP, FATAL, "Couldn't map the graphics processor.");
    }
    gp1_base = (short *) p;
    gpm = (caddr_t) (p + GP1_SHMEM_OFFSET);
    gp1_shmem = (short *) gpm;
}

gp1_close()
{
    if (gp1_base) {
	close(gp1_fd);
	free(allocp);
	gp1_base = 0;
    }
}

gp1_reset()
{
    gp1_hwreset();
    gp1_swreset();
}

gp1_hwreset()
{
    gp1_base[GP1_CONTROL_REG] = GP1_CR_CLRIF | GP1_CR_INT_DISABLE |
	GP1_CR_RESET;
    gp1_base[GP1_CONTROL_REG] = 0;
}

gp1_swreset()
{
    register int   *shmem = (int *) gp1_shmem;
    register short  i;
    int             x = 0x800000FF;
    int             y = 0;

    i = 133;
    while (--i) {
	move_int(shmem, &y);
    }

    move_int(&gp1_shmem[10], &x);
    /* *((int *)&gp1_shmem[10]) = 0x800000FF; */
}

gp1_load(filename)
    char           *filename;
{
    FILE           *fp;
    u_short         tadd, nlines;
    u_short         ucode[4096 * 4];
    int             nwords;
    register u_short *ptr;
    register short *gp1_ucode;

    strcpy(SD, "./");
    if (exec_by_sundiag) {
	if (simulate_error == UFILE_NOT_OPEN)
	    strcat(SD, "no.ucode");
	else
	    strcat(SD, filename);
    }
    else 
	strcat(SD, filename);

    if ((fp = fopen(SD, "r")) == NULL) 
	send_message(-UFILE_NOT_OPEN, FATAL, 
		"Couldn't open microcode file '%s'.", SD);
    gp1_ucode = &gp1_base[GP1_UCODE_DATA_REG];
    while (fread(&tadd, sizeof(tadd), 1, fp) == 1) {
	/* starting microcode address */
	gp1_base[GP1_UCODE_ADDR_REG] = tadd;
	fread(&nlines, sizeof(nlines), 1, fp);
	/* number of microcode lines  */
	while (nlines > 0) {
	    nwords = (nlines > 4096) ? 4096 : nlines;
	    nlines -= nwords;
	    fread(ucode, sizeof(u_short), 4 * nwords, fp);
	    for (ptr = ucode; nwords > 0; nwords--) {
		*gp1_ucode = *ptr++;
		*gp1_ucode = *ptr++;
		*gp1_ucode = *ptr++;
		*gp1_ucode = *ptr++;
	    }
	}
    }

    fclose(fp);
}

gp1_vp_start(cont_flag)
    int             cont_flag;
{
    register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

    *gp1_cntrl = 0;
    if (cont_flag)
	*gp1_cntrl = GP1_CR_VP_CONT;
    else
	*gp1_cntrl = GP1_CR_VP_STRT0 | GP1_CR_VP_CONT;
}

gp1_vp_halt()
{
    register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

    *gp1_cntrl = 0;
    *gp1_cntrl = GP1_CR_VP_HLT;
}

gp1_pp_start(cont_flag)
    int             cont_flag;
{
    register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

    *gp1_cntrl = 0;
    if (cont_flag)
	*gp1_cntrl = GP1_CR_PP_CONT;
    else
	*gp1_cntrl = GP1_CR_PP_STRT0 | GP1_CR_PP_CONT;
}

gp1_pp_halt()
{
    register short *gp1_cntrl = &gp1_base[GP1_CONTROL_REG];

    *gp1_cntrl = 0;
    *gp1_cntrl = GP1_CR_PP_HLT;
}

int process_gpmtest_args (argv, arrcount)
char	*argv[];
int	arrcount;
{
    if (argv[arrcount][0] == 'e') {
        simulate_error = atoi(&argv[arrcount][1]);
        if (simulate_error > 0 && simulate_error < END_ERROR)
            return (TRUE); 
    }
    else if (strcmp(argv[arrcount], "gb") == 0) 
        num_of_tests += GB_TESTS;
    else if (strcmp(argv[arrcount], "ppprom") == 0) 
        ppprom_flag = TRUE;
    else
	return (FALSE);
    return (TRUE);
}

/*******************************
 Dummy code to satisfy libtest.a
********************************/
clean_up()
{
}
