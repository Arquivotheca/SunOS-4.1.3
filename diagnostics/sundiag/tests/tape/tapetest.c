#ifndef lint
static  char sccsid[] = "@(#)tapetest.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sundev/tmreg.h>
#include <sundev/xtreg.h>
#include <sundev/arreg.h>
#include "sdrtns.h"		/* sundiag standard header file */
#include "../../../lib/include/libonline.h"
#include "tapetest.h"
#include "tapetest_msg.h"

#include <sys/mtio.h>

#include <sys/buf.h>
#include <sun/dkio.h>
#include <sundev/scsi.h>

extern char *sprintf();
extern int process_tape_args();
extern int routine_usage();

static char *print_error_key();
static short error_key_code=0;

struct	mt_tape_info	tapes[]=MT_TAPE_INFO;

short t_type;
Tape_type tape_type;

int	pass = 0;
char	device_tapename[50];
char 	tname[50];
char	device_file[50];
char	*device = device_tapename;	

int	do_file = FALSE;
int 	skip_retension = FALSE;
int	switch_8_and_0 = FALSE;
int	cart_tape = FALSE;
int	run_recon = FALSE;
int	read_only = FALSE;
int	test_all = FALSE;
int	flt=FALSE;
#ifdef NEW
int	exb8500=FALSE;
#endif NEW
int	no_comp=TRUE;
int	read_compare = FALSE;
int	after_op_sleep=1;
int	eot=TRUE;		/* default to TRUE, if eot test */
int	do_sleep = TRUE;	/* TRUE, if sleep between operation */
char	nonrewind[52];
char	*which_io;
unsigned nblks= -1;		/* initialized to MAXINT */

struct  commands com[] = {
        { "fsf",        MTFSF,  1,      1 },
        { "nbsf",       MTNBSF,  1,      1 },
        { "rewind",     MTREW,  1,      0 },
        { "status",     MTNOP,  1,      0 },
        { "retension",  MTRETEN,1,      0 },
        { "erase",      MTERASE,0,      0 },
        { 0 }
};

static	u_long	lcheck();
static  int	density_code, device_arg = FALSE;

/*
 * tapetest checks the status, rewinds the tape, erases/retensions it 
 * if it's a cartridge tape, writes a pattern to nblks or eot(default),
 * rewinds the tape, and then does a read and compare of the pattern. 
 * tapetest will first write "large blocks":
 *		 (NBLOCKS * BLOCKSIZE) = 126 * 512 = 64512
 * and then writes out whatever "small blocks" are left: 
 *		 BLOCKSIZE = 512
 * If "file test(ft)" is enabled, tapetest also writes 3 files, 
 * and tests the forward space file (Xylogics and SCSI) and backward space 
 * file (SCSI only) tape options (see mtio(4)). 
 * Note that the Xylogics tape driver does not currently support erase, 
 * bsf, and eot(see bugs database on elmer).
 */
main(argc, argv)
    int argc;
    char *argv[];
{

    versionid = "1.38";		/* get sccs version id */
    test_init(argc, argv, process_tape_args, routine_usage, test_usage_msg);
    if (!device_arg) {
        display_usage(test_usage_msg);
        send_message (USAGE_ERROR, ERROR, devname_err_msg);
    }
    valid_dev_name();
    mt_command(tape_status, device, 0, 0);
    setup_params();
    run_tests();
    test_end();
}

/*
 * process_test_args() - special tapetest arguments
 */
process_tape_args(argv, arrcount)
char	*argv[];
int	arrcount;
{
    if (strncmp(argv[arrcount], "D=/dev/", 2) == 0) {
	device_name = device_file;
        (void)strcpy(device, &argv[arrcount][2]);
	(void)strcpy(device_name, device+6);
	if (*(device+6) == 'x') 
            *(device+6) = 'm';
	device_arg = TRUE;
    } else if (strcmp(argv[arrcount], "nr") == 0)
        skip_retension = TRUE;
    else if (strcmp(argv[arrcount], "sq") == 0)
        switch_8_and_0 = TRUE;
    else if (strcmp(argv[arrcount], "ns") == 0)
	do_sleep = FALSE;
    else if (strncmp(argv[arrcount], "b=", 2) == 0) {
	eot = FALSE;
        nblks = atoi(argv[arrcount]+2);
    } else if (strncmp(argv[arrcount], "ro", 2) == 0)
	read_only = TRUE;
    else if (strncmp(argv[arrcount], "ft", 2) == 0)
	do_file = TRUE;
    else if (strncmp(argv[arrcount], "rc", 2) == 0)
	run_recon = TRUE;
    else if (exec_by_sundiag && strncmp(argv[arrcount], "op=", 3) == 0)
	process_sundiag_option(atoi(argv[arrcount]+3));
    else
	return (FALSE);

    return (TRUE);
}

/*
 * process_sundiag_option(), translates the encoded sundiag test options.
 */
process_sundiag_option(code)
    unsigned code;
{
    int	tmp;

    density_code = code & 7;

    if ((tmp = (code & 0x18) >> 3) != 0) 
        eot = FALSE;
    if (tmp == 1)
        nblks = 25300;		/* default */
    else if (tmp == 2)		/* long */
        nblks = 70000;
    else if (tmp == 3)		/* short */
        nblks = 1000;

    read_only = code & 0x100;
    do_file = !(code & 0x20);
    do_sleep = !(code & 0x40); 
    run_recon = code & 0x80;
    skip_retension = code & 0x200;
}

/*
 * routine_usage() - usage arguments specific to tapetest
 */
routine_usage()
{
    send_message(0, CONSOLE, routine_msg, test_name);
}

/*
 * valid_dev_name(),
 * make sure that it's a character device, see if it's SCSI (st), 
 * Archive (ar), or Xylogics (mt), and get the unit number -> 
 */
valid_dev_name()
{
    char *pbuf;

   func_name = "valid_dev_name";
   TRACE_IN
    pbuf = device+5;
    if (*pbuf != 'r') 
        send_message(-BAD_DEVICE_NAME, ERROR, devfile_err_msg, device);
    pbuf++;

    if (!strncmp(pbuf, "st", 2))
	tape_type = st;
    else if (!strncmp(pbuf, "mt", 2))
	tape_type = mt;
    else 
        send_message(-BAD_DEVICE_NAME, ERROR, bad_devfile_msg, device);

    t_type = 0;
    mt_command(tape_status, device, 0, 0);	/* Get tape information */
    if (t_type == MT_ISHP || t_type == MT_ISKENNEDY) { 
	flt_dev();     /* FLT device, check for "compression" mode */
    }
#ifdef NEW
    if (t_type == MT_ISEXB8500) {
	exb8500_dev(); /* EXB8500 device, check for "compression" mode */
    }
    if (flt || exb8500) 
#else NEW
    if (flt) 
#endif NEW
        switch_8_and_0 = FALSE;	/* always run one density at a time */
    else if (tape_type == mt) {
        if (density_code == 2) 
            switch_dev();		/* 6250-BPI */
        else if (density_code == 0) 
            switch_8_and_0 = TRUE;/* Both */
    } else if (density_code == 1) 
        switch_dev();		/* QIC-24 */
    else if (density_code == 2) 
        switch_8_and_0 = TRUE;	/* QIC-11&QIC-24 */
    
   TRACE_OUT
}

setup_params()
{
   func_name = "setup_params";
   TRACE_IN
    if (read_only && do_file && !exec_by_sundiag)
	send_message(0, VERBOSE, read_only_msg);

    if (quick_test && nblks > 500)
        nblks = (3 * NBLOCKS) + 4;	/* 3 large blocks, 4 small blocks */

    if (tape_type == st)
	cart_tape = TRUE;
	
    if (do_sleep && nblks > 1000)
        after_op_sleep = 60;

    if (quick_test) 
        do_sleep = FALSE;
   TRACE_OUT
}

run_tests()
{
    register max_pass;
    func_name = "run_tests";
    TRACE_IN
    send_message(0, VERBOSE, "Test started.");

    if (do_sleep) {
      send_message(0, VERBOSE, sharing_msg);
      sleep(60);
    }

    send_message(0, VERBOSE, rewind_msg);
    mt_command(rewind, device, 0, 0);		/* Rewind tape */

    if (do_sleep)
	sleep(10);

#ifdef NEW
    if (cart_tape && !skip_retension && !quick_test && !flt && !exb8500) {
#else NEW
    if (cart_tape && !skip_retension && !quick_test && !flt) {
#endif NEW
	if (read_only) {
	    send_message(0, VERBOSE, retension_msg);
	    mt_command(retension, device, 0, 0);
	} else {
	    send_message(0, VERBOSE, erase_msg);
	    mt_command(erase, device, 0, 0);
	}
        if (do_sleep)
	    sleep(after_op_sleep);
    }

    pass = 0;
    while (TRUE) {			/* until break */
	if (flt)
		max_pass = 3;
#ifdef NEW
	else if (exb8500)
		max_pass = 2;
#endif NEW
	else
		max_pass = 0;
	++pass;
	dev_test();

	if (do_file && !read_only) 
	    file_test();

        if (switch_8_and_0 && pass != 2) {
	  send_message(0, VERBOSE, switch_2nd_msg);
	  switch_dev();
 	} else if (test_all && (pass < max_pass || (pass == max_pass && !no_comp))) {
          send_message(0, VERBOSE, switch_next_msg, pass + 1);
          sprintf(device+8, "%d", atoi(device+8) + 8);  /* next_dev */
        } else
          break;
    }

#ifdef NEW
    if (run_recon && !flt && !exb8500) {
#else NEW
    if (run_recon && !flt) {
#endif NEW
	send_message(0, VERBOSE, reconnect_msg);
	recon();
    }

    send_message(0, VERBOSE, "Test passed.");

    if (!cart_tape && do_sleep) {		/* if 1/2 tapes */
        send_message(0, VERBOSE, sleep_msg);
        sleep(12*60);				/* sleep for 12 minutes */
    }
    TRACE_OUT
}

/*
 * switch_dev() - switches the device name between the QIC-11
 *                and QIC-24 SCSI tape device names
 */
switch_dev()
{
  int	dev_num;
  func_name = "switch_dev";
  TRACE_IN

  if ((dev_num = atoi(device+8)) < 8)
    dev_num += 8;
  else
    dev_num -= 8;

  sprintf(device+8, "%d", dev_num);
  
  TRACE_OUT
}

/*
 * flt_dev() - generate proper device file for the front load tape drives.
 */
flt_dev()
{
   int xfd ;
   func_name = "flt_dev";
   TRACE_IN
  flt = TRUE;
  (void)sprintf(tname, "/dev/rst%d", 24 + atoi(device+8));
  if ((xfd=open(tname, O_RDONLY)) != -1)
  	no_comp = FALSE;     /* yes it supports "compression" mode */

  switch(density_code) {
	  case 0:
		break;
	  case 1:
	    sprintf(device+8, "%d", 8 + atoi(device+8));
		break;
	  case 2:
	    sprintf(device+8, "%d", 16 + atoi(device+8));
		break;
	  case 4:
	    sprintf(device+8, "%d", 24 + atoi(device+8));
		break;
	  default: 
	    test_all = TRUE;
  }
  if ( xfd != -1 ) close(xfd);

  TRACE_OUT

}

#ifdef NEW
/*
 * exb8500_dev() - generate proper device file for the exabyte 8500 tape drives.
 */
exb8500_dev()
{
   int xfd ;

   func_name = "exb8500_dev";
   TRACE_IN
  exb8500 = TRUE;
  (void)sprintf(tname, "/dev/rst%d", 16 + atoi(device+8));

  if ((xfd=open(tname, O_RDONLY)) != -1)
 	 no_comp = FALSE;     /* yes it supports "compression" mode */

  switch(density_code) {
	  case 0:
		break;
	  case 1:
	    sprintf(device+8, "%d", density_code*8 + atoi(device+8));
		break;
	  case 3:
	    sprintf(device+8, "%d", 16 + atoi(device+8));
		break;
	  default: 
	    test_all = TRUE;
  }
  if ( xfd != -1 ) close(xfd);

  TRACE_OUT
}
#endif NEW

dev_test()
{
   func_name = "dev_test";
   TRACE_IN
    send_message(0, VERBOSE, start_test_msg, "tape", device);

    if (!read_only) {
        if (eot) 
	    send_message(0, VERBOSE, write_end_msg);
	else 	
	    send_message(0, VERBOSE, write_blk_msg, nblks);
        write_file(FILE_1);

	send_message(0, VERBOSE, rewind_after_msg, "write");
        mt_command(rewind, device, 0, 0);

        if (do_sleep)
	    sleep(after_op_sleep);
    }

    if (eot) 
        send_message(0, VERBOSE, read_end_msg);
    else 	
        send_message(0, VERBOSE, read_blk_msg, nblks);

    read_file(FILE_1);

    send_message(0, VERBOSE, rewind_after_msg, "read");
        mt_command(rewind, device, 0, 0);

    if (do_sleep)
	sleep(after_op_sleep);
    
   TRACE_OUT
}

/*
 * file_test- write 4 files, and check out forward space and (SCSI only)
 * backward space file.	
 */
file_test()
{
    int	save_eot, save_nblks;

    func_name = "file_test";
    TRACE_IN
    send_message(0, VERBOSE, start_test_msg, "file", device);
    save_eot = eot;
    save_nblks = nblks;

    eot = FALSE;
    send_message(0, VERBOSE, file_test_msg, "writing");
    nblks = (2 * NBLOCKS) + 2;	        /* 2 large blocks, 2 small blocks */
    write_file(FILE_1);
    nblks = (3 * NBLOCKS) + 2;		/* 3 large blocks, 2 small blocks */
    write_file(FILE_2);
    nblks = (1 * NBLOCKS) + 2;		/* 1 large block, 2 small blocks */
    write_file(FILE_3);
    nblks = (2 * NBLOCKS) + 3;		/* 2 large blocks, 3 small blocks */
    write_file(FILE_4);
    send_message(0, VERBOSE, rewind_after_msg, "writing files.");
        mt_command(rewind, device, 0, 0);
    if (do_sleep)
	sleep(20);

    /* start reading here */

    send_message(0, VERBOSE, file_test_msg, "reading");
    nblks = (1 * NBLOCKS) + 0;		/* read part way into first file */
    read_file(FILE_1);
    mt_command(fsf, device, 1, 0);	/* forward space file to second file */
    nblks = (3 * NBLOCKS) + 2;		/* read second file */
    read_file(FILE_2);
    mt_command(fsf, device, 1, 0);	/* forward space file to third file */
    eot = TRUE;				/* fake eot flag */
    send_message(0, VERBOSE, "File test, reading to eof on 3rd file.");
    nblks = (1 * NBLOCKS) + 2 + 1;	/* read to eof on third file */
    read_file(FILE_3);
    send_message(0, VERBOSE, "File test, reading to eof on 4th file.");
    nblks = (2 * NBLOCKS) + 3 + 1;	/* read to eof on 4th file */
    read_file(FILE_4);
    eot = FALSE;

    if (tape_type == st) {		/* if SCSI */
	send_message(0, VERBOSE, "Backspacing to start of 2nd file.");
        mt_command(nbsf, device, 3, 0);	/* back space file to second file */
	send_message(0, VERBOSE, "Reading 2nd file again.");
        nblks = (3 * NBLOCKS) + 2;	/* read second file */
        read_file(FILE_2);
    }

    send_message(0, VERBOSE, rewind_after_msg, "reading files.");
        mt_command(rewind, device, 0, 0);
    if (do_sleep)
	sleep(20);

    eot = save_eot;
    nblks = save_nblks;
    TRACE_OUT
}

/*
 * write_file(file_num) - opens the device, writes
 * all the possible large blocks (126 * 512), then whatever small blocks (512),
 * closes the file, and returns 0 if no problem, otherwise returns -1
 * if (eot), the write routines return 1 if the returned byte count <cc> 
 * is not equal to <nbytes>.
 * Note that there is a bug in the Xylogics tape driver 
 * w/writing to eot. file_num is used to encode which file is being written
 * to as part of the lfill/lcheck pattern, so we can make sure later that
 * we're reading the right file.
 */
static	int	write_file(file_num)
int file_num;
{
    int fd, status = 0;
    int little_start_block = 0;
 
   func_name = "write_file";
   TRACE_IN
    fd = open_dev(O_WRONLY);
    if ((status=do_nblock_io(O_WRONLY, &fd, &little_start_block, file_num))) {
        close_dev(fd);
	send_message(0, VERBOSE, hit_eot_msg, little_start_block+1);
        return(-1);			/* eot */
    }

    if (!status)
        if (do_blocksize_io(O_WRONLY, fd, &little_start_block, file_num)) {
            close_dev(fd);
	    send_message(0, VERBOSE, hit_eot_msg, little_start_block+1);
            return(-1);			/* eot */
        }

    close_dev(fd);
    
    TRACE_OUT
    return(0);
}

/*
 * read_file(file_num) - opens the device, reads
 * all the possible large blocks (126 * 512), then whatever small blocks (512),
 * closes the file, and returns 0 if no problem, otherwise returns -1
 * if (eot), the read routines return 1 if the returned byte count <cc> 
 * is not equal to <nbytes>.
 * file_num was used to encode which file was written
 * to as part of the lfill/lcheck pattern, so we can check now that
 * we're reading the right file.
 */
static	int	read_file(file_num)
int file_num;
{
    int fd, status = 0;
    int little_start_block = 0;

   func_name = "read_file";
   TRACE_IN
    fd = open_dev(O_RDONLY);
    if ((status=do_nblock_io(O_RDONLY, &fd, &little_start_block, file_num))) {
        close_dev(fd);
	send_message(0, VERBOSE, hit_eot_msg, little_start_block+1);
        return(-1);			/* eot */
    }

    if (!status)
        if (do_blocksize_io(O_RDONLY, fd, &little_start_block, file_num)) {
            close_dev(fd);
	    send_message(0, VERBOSE, hit_eot_msg, little_start_block+1);
            return(-1);			/* eot */
        }

    close_dev(fd);
    TRACE_OUT
    return(0);
}

#define	MAXBLOCKS	3999996	/* max. # of blocks in a tape file */

/*
 * do_nblock_io(mode, fd, start_block, file_num) -
 * buffer size is 126 * 512, loops writing 126 blocks at a time.
 * O_WRONLY mode -> fill buffer with file_num pattern, seek and write buffer.
 * O_RDONLY mode -> seek and read buffer, check file_num pattern if not eot.
 * End loop after you have written/read as many buffers as possible
 * that are <= nblks, or you have reached eof, return status from
 * write_dev/read_dev routines, and next block to seek to (start_block).
 */
static	int	do_nblock_io(mode, fd, start_block, file_num)
    int mode;
    int *fd;
    int *start_block;
    int file_num;
{
    u_char buff[NBLOCKS*BLOCKSIZE + 6];
    u_char *dev_buf;
    int i, status = 0;
    u_long pattern;

   func_name = "do_nblock_io";
   TRACE_IN
    which_io = "Big";
    dev_buf = (u_char *)(((unsigned)buff + 4) & ~0x03);	/* long word aligned */

    for (i = 0; (i + NBLOCKS) <= nblks;) {
	pattern = (i << 4) + file_num;

        switch (mode) {
        case O_WRONLY:
            (void)lfill(dev_buf, NBLOCKS*BLOCKSIZE, pattern);
            status = write_dev(*fd, dev_buf, NBLOCKS*BLOCKSIZE, i+1);
            break;
        case O_RDONLY:
            status = read_dev(*fd, dev_buf, NBLOCKS*BLOCKSIZE, i+1);
            if ((!read_only || read_compare) && !status)
                  compare_data(dev_buf, NBLOCKS*BLOCKSIZE, pattern, i+1);
            break;
        }

	if (do_sleep)
	    sleep(2);

        if (status)     /* eot */
            break;

	i += NBLOCKS;

	if ((i % MAXBLOCKS) == 0) {
	    close_dev(*fd);
	    if (mode == O_RDONLY)
                mt_command(fsf, device, 1, 0);
	    *fd = open_dev(mode);  /* reopen the tape to avoid 2GB limit */
	}
    }

    *start_block = i;
    
    TRACE_OUT
    return(status);
}

/*
 * do_blocksize_io(mode, fd, start_block, file_num) -
 * buffer size is 512, loops writing 1 block at a time.
 * O_WRONLY mode -> fill buffer with file_num pattern, seek and write buffer.
 * O_RDONLY mode -> seek and read buffer, check file_num pattern if not eot.
 * End loop after you have written/read as many buffers as possible
 * that are <= nblks, or you have reached eot, return status from
 * write_dev/read_dev routines.
 */
static	int	do_blocksize_io(mode, fd, start_block, file_num)
    int mode;
    int fd;
    int *start_block;
    int file_num;
{
    u_char buff[BLOCKSIZE + 6];
    u_char *dev_buf;
    int i, status = 0;
    u_long pattern;

   func_name = "do_blocksize_io";
   TRACE_IN
    which_io = "Small";
    dev_buf = (u_char *)(((unsigned)buff + 4) & ~0x03);	/* long word aligned */

    for (i= *start_block; i < nblks && !status; i++) {
	pattern = (i << 4) + file_num;

        switch (mode) {
        case O_WRONLY:
            (void)lfill(dev_buf, BLOCKSIZE, pattern);
            status = write_dev(fd, dev_buf, BLOCKSIZE, i+1);
            break;
        case O_RDONLY:
            status = read_dev(fd, dev_buf, BLOCKSIZE, i+1);
            if ((!read_only || read_compare) && !status)
                compare_data(dev_buf, BLOCKSIZE, pattern, i+1);
            break;
        }
    }

    *start_block = i;
    
    TRACE_OUT
    return(status);
}

/*
 * open_dev(mode) - open the device as per mode: O_RDONLY, O_WRONLY,
 * or whatever, return file descriptor, return -1 if open failed, 0 if OK
 */
static	int	open_dev(mode)
    int mode;
{
    int fd, omask;

    func_name = "open_dev";
    TRACE_IN
    strcpy(nonrewind, "/dev/n");
    strcat(nonrewind, device+5);  /* convert it to "non-rewinding" device */
    omask = enter_critical();

    if ((fd = open(nonrewind, mode)) < 0)
        send_message(-OPEN_ERROR, ERROR, open_err_msg,nonrewind,errmsg(errno));
    exit_critical(omask);
    
    TRACE_OUT
    return(fd);
}

/*
 * open_rdev(mode) - open the raw device as per mode: O_RDONLY, O_WRONLY,
 * or whatever, return file descriptor, return -1 if open failed, 0 if OK
 */
static	int	open_rdev(mode)
    int mode;
{
    int fd, omask;

   func_name = "open_rdev";
   TRACE_IN
    omask = enter_critical();
    if ((fd = open(device, mode)) < 0)
        send_message(-OPEN_ERROR, ERROR, open_err_msg, device, errmsg(errno));
    exit_critical(omask);
    
    TRACE_OUT
    return(fd);
}

/*
 * close_dev(fd) - close file descriptor for the device
 * return -1 if close failed, 0 if OK
 */
close_dev(fd)
    int fd;
{
   func_name = "close_dev";
   TRACE_IN
    if ((close(fd)) < 0) {
        send_message(0, ERROR, close_err_msg, nonrewind, errmsg(errno));
        if (!run_on_error)
            exit(CLOSE_ERROR);
	return(-1);
    }
    
    TRACE_OUT
    return(0);
}
 
/*
 * close_rdev(fd) - close the raw file descriptor for the device
 * return -1 if close failed, 0 if OK
 */
close_rdev(fd)
    int fd;
{
   func_name = "close_rdev";
   TRACE_IN
    if ((close(fd)) < 0) {
        send_message(0, ERROR, close_err_msg, device, errmsg(errno));
        if (!run_on_error)
            exit(CLOSE_ERROR);
	return(-1);
    }
     
    TRACE_OUT
    return(0);
}
 
static	int	get_sense(fd)
    int	fd;
{
    struct mtget mt_status;

   func_name = "get_sense";
   TRACE_IN
	if (ioctl(fd, MTIOCGET, &mt_status) < 0) 
		return(-1);
	error_key_code = mt_status.mt_erreg;
	
        TRACE_OUT
	return(error_key_code);
}

/* Define SCSI sense key error messages. */
static char *key_error_str[] = SENSE_KEY_INFO;
#define MAX_KEY_ERROR_STR \
	(sizeof(key_error_str)/sizeof(key_error_str[0]))

/*
 * Return the text string associated with the sense key value.
 */
static char *
print_error_key()
{
	static char *unknown_key = "unknown key";

	if ((error_key_code > MAX_KEY_ERROR_STR -1)  ||
	    key_error_str[error_key_code] == NULL) {

		return (unknown_key);
	}
	return (key_error_str[error_key_code]);
}

/*
 * write_dev(fd, buf, bufsize, block) - write
 * data->buf of bufsize to file descriptor for device. If (eot) ignore
 * a returned byte count not equal to bufsize.
 * block is the block number.
 * return 0 if OK, 1 if eot.
 */
static	int	write_dev(fd, buf, bufsize, block)
int fd;
char *buf;
int bufsize;
int block;   
{
    int cc;
 
   func_name = "write_dev";
   TRACE_IN
    errno = 0;
    if ((cc = write(fd, buf, bufsize)) != bufsize) {
	if (errno == 0)			/* hit eot */
	    if (eot)
                return(1);
	    else
	        send_message(-WRITE_ERROR, ERROR, 
	                     "%s write failed on %s, block %d: EOT reached.",
	                     which_io, nonrewind, block);

        send_message(-WRITE_ERROR, ERROR, 
	            "%s write failed on %s, block %d: %s, sense key(0x%x) = %s.",
	            which_io, nonrewind, block, errmsg(errno), get_sense(fd),
		    print_error_key());
    }
   TRACE_OUT
    return(0);
}      
 
/*
 * read_dev(fd, buf, bufsize, block) - read
 * data->buf of bufsize to file descriptor for device. If (eot) ignore
 * a returned byte count not equal to bufsize.
 * block is the block number.
 * return 0 if OK, 1 if eot
 */
static	int	read_dev(fd, buf, bufsize, block)
int fd;
char *buf;
int bufsize;
int block;
{
    int cc;
 
   func_name = "read_dev";
   TRACE_IN
    errno = 0;
    if ((cc = read(fd, buf, bufsize)) != bufsize) {
	if (errno == 0) {			/* hit eot */
	    if (eot)
                return(1);
	    else
	        send_message(-READ_ERROR, ERROR, 
	                     "%s read failed on %s, block %d: EOF reached.",
	                     which_io, nonrewind, block);
        }

        send_message(-READ_ERROR, ERROR, "%s read failed on %s, block %d ",
				 which_io, nonrewind, block);
    }

   TRACE_OUT
    return(0);
}

/*
 * compare_data(buf, bufsize, pattern, which_blk) - compare
 * data->buf of bufsize to pattern. If an error_offset is returned,
 * find the first error_byte, and record the info into an error message.
 * which_blk is the block number.
 * return 0 if no error, exit if compare failed.
 */
static	int	compare_data(buf, bufsize, pattern, which_blk)
    u_char	buf[];
    int bufsize;
    u_long pattern;
    int which_blk;
{
    u_long error_offset, *tmp;
 
   func_name = "compare_data";
   TRACE_IN
    if ((error_offset = lcheck(buf, bufsize, pattern))) {
	tmp = (u_long *)&buf[error_offset];
        send_message(-COMPARE_ERROR, ERROR, compare_err_msg, which_io, 
                     nonrewind, which_blk, error_offset, pattern, *tmp);
    }
   TRACE_OUT
    return(0);
}

/*
 * lfill : fills "count" bytes of memory, starting at "add" with "pattern"
 * ,32-bit (u_long) word at a time. The residue(after divided by
 * sizeof(u_long)) will not be filled.
 *
 * retrun: this function always return the integer -1.
 */
static int lfill(add, count, pattern)
    register u_long *add;
    register u_long count, pattern;
{
   func_name = "lfill";
   TRACE_IN
    count /= sizeof(u_long);            /* to be sure */

    while (count-- != 0)
        *add++ = pattern;
   TRACE_OUT
}

/*
 * lcheck : compares "pattern", one 32-bit(u_long) word at a time, to the
 * contents of memory starting at "add" and extending for "count" bytes.
 *
 * return : the size of the buffer, in the unit of 32-bit(u_long), less the
 * offset from starting address to the first location that does not compare.
 * 0 if everything compares.
 *
 * note : the residue(after divided by sizeof(u_long)) won't be checked.
 */
static u_long lcheck(add, count, pattern)
    register u_long *add;
    register u_long count, pattern;
{
    register int i;
 
   func_name = "lcheck";
   TRACE_IN
    count /= sizeof(u_long);            /* to be sure */
 
    for (i = 0; i < count; i++) {
        if (*add++ != pattern)
            return (i * sizeof(u_long));
    }
   TRACE_OUT
    return (0);                                /* everything compares */
}

/*
 * from mt.c:
 */
static short tape_io_status(bp)
struct mtget *bp;
{
    char buf[BUFSIZ-1];
    register struct mt_tape_info	*mt;

   func_name = "tape_io_status";
   TRACE_IN
    for (mt = tapes; mt->t_type && mt->t_type != bp->mt_type; mt++)
        ;

    if (!mt->t_type) 
	send_message(0, WARNING, "Unknown tape drive type (%d).", bp->mt_type);

    if (bp->mt_flags & MTF_SCSI)
    { 			/* Handle SCSI tape drives specially. */
        send_message(0, DEBUG, scsi_io_msg, mt->t_name, bp->mt_erreg,
                     bp->mt_resid, bp->mt_dsreg, bp->mt_fileno, 
                     bp->mt_blkno);
    } else { 		/* Handle other drives here. */
        send_message(0, DEBUG, nonscsi_io_msg, mt->t_name,
                     bp->mt_resid, bp->mt_dsreg, bp->mt_erreg);
    }
   TRACE_OUT
    return(mt->t_type);
}

/*
 * mt_command(type, device, count) - from mt.c, will open the device and 
 * do the appropriate "type" ioctl, w/count if bsf or fsf, otherwise 
 * count is set to 1
 */
mt_command(type, device, count, tfd)
    Mt_com_type type;
    char *device;
    int count;
    int tfd;
{
    int fd;
    struct mtop mt_com;
    struct mtget mt_status;
    struct commands *comp;
    char  buf[BUFSIZ-1];

   func_name = "mt_command";
   TRACE_IN
    if (!tfd) {
    	switch (type) {
            case erase:
            	fd = open_rdev(O_RDWR);
            	break;
            case fsf:
            case nbsf:
		fd = open_dev(O_RDONLY);
		break;
            case tape_status:
            case retension:
            case rewind:
            	fd = open_rdev(O_RDONLY);
            	break;
    	}
    } else
	fd = tfd;

    if (type == tape_status) {
        if (ioctl(fd, MTIOCGET, (char *)&mt_status) < 0) 
            send_message(-MTIOCGET_ERROR, ERROR, "%s tape MTIOCGET ioctl: %s",
	                 device, errmsg(errno));
            t_type = tape_io_status(&mt_status);
    } else {
        comp = &com[(int)type];
        mt_com.mt_op = comp->c_code;
        mt_com.mt_count = (type == nbsf || type == fsf) ? count : 1;
        if (ioctl(fd, MTIOCTOP, &mt_com) < 0) 
            send_message(-MTIOCTOP_ERROR, ERROR, "%s failed on %s: %s.",
                         comp->c_name, device, errmsg(errno));
        send_message(0, DEBUG, tape_status_msg, device,
                     comp->c_name, mt_com.mt_op, mt_com.mt_count);
    }
    if (!tfd)
        (void)close_rdev(fd);

   TRACE_OUT
}

/************************************************
 Just a dummy routine to satisfy libsdrtns.a.
*************************************************/
clean_up()
{
}

