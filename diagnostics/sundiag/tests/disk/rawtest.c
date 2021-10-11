#ifndef lint
static	char sccsid[] = "@(#)rawtest.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/signal.h>
#ifndef	 SVR4
#include <fstab.h>
#endif
#include <sys/types.h>
#include <sys/file.h>
#ifdef   SVR4
#include <sys/dkio.h>
#include <sys/mntent.h>
#include <sys/mnttab.h>
#include <sys/fcntl.h>
#else
#include <sun/dkio.h>
#include <mntent.h>
#endif
#include "sdrtns.h"				/* sundiag standard header file */
#include "rawtest.h"				/* fdtest defines */
#include "rawtest_msg.h"			/* messages header file */

#include "../../../lib/include/libonline.h"     /* online library include */

 /*
  * This progam writes random data on the raw device, and then reads and
  * compares them. If any error occurs, the process logs them and stops. 
  */

main(argc, argv)
    int argc;
    char *argv[];
{
   	versionid = "1.18";	/* SCCS version id */
	device_name = device_diskname;
	device = device_file;
	test_init(argc, argv, process_rawdisk_args, routine_usage, test_usage_msg);
	dev_test();
	test_end();
}

process_rawdisk_args(argv, arrcount)
char    *argv[];
int     arrcount;
{
	if (strncmp(argv[arrcount],"D=", 2) == 0)   /* sundiag */
	{
		strncpy(device_name, &argv[arrcount][2], 20);  /* set raw diskname */
	} else if (strncmp(argv[arrcount],"W", 1) == 0)	/* get test type */ 
		ok_to_write = 1;
	else if (strncmp(argv[arrcount],"P=", 2) == 0)   /* sundiag */
	{
		partition = get_part(&argv[arrcount][2]); /* set partition */
	}
	else if (strncmp(argv[arrcount],"S=", 2) == 0)   /* sundiag */
	{
		spec_size = atoi(&argv[arrcount][2]); /* get user specified test size in MB */
	}
	else
		return (FALSE);

	return (TRUE);
}

routine_usage()
{
    (void) send_message(0, CONSOLE, routine_msg);
}

/* dev_test() -
 * Begin the actual test.
 * Here the decision is made based on the ok_to_write
 * flag; if its a readonly or write/read operation.
 * If its a write/read mode, checks are conducted to 
 * ensure that there are no mounted file systems on
 * the specified partition.
 */
dev_test()
{
	char name[20];
	
	sprintf(device, "/dev/r%s%c", device_name, partition);
	if ( ok_to_write )
	{
		/* start check for mounted partitions */
		if ( partition == 'c' )	{
		/* If its a 'c' we need to check for all partitions */
			sprintf(name, "%s", device_name);
		}
		else { /* Check only the specified partition */
			sprintf(name, "%s%c", device_name, partition);
		}
		if ( check_mtab(name) ) {       /* Mounted filesystem on device ! */
			send_message(-MOUNTED_ERROR, ERROR, mnt_err_msg);
		}
		/* end check for mounted partitions */

		spawn_test(O_RDWR);
	}
	else
		spawn_test(O_RDONLY);

	/* If it comes here then the test must have passed !! */
	send_message(0, VERBOSE, test_pass_msg);
}

/*
 * do_nblock_io(num, startblk, nblks, mode) -
 * buffer size is NBLOCKS*BLOCKSIZE, loops writing NBLOCKS blocks at a time.
 * O_RDWR mode -> fill buffer with pattern, seek and write buffer.
 * O_RDONLY mode -> seek and read buffer, check pattern.
 * End loop after you have written/read as many buffers as possible
 * that are <= nblks, or you have reached eof.
 */
static	int	do_nblock_io(num, startblk, nblks, mode)
    int num;
    int startblk;
    int nblks;
    int mode;
{
	
	dsk_info dsk;

	dskp = &dsk;
	which_io = "Big";
	
	dskp->diskfd = open_dev(mode);	/* set the disk file descriptor here */
	dskp->startblk  = startblk;	/* set the starting blkno here */

	/* allocate space for the test and backup buffers */

	backup_buf = (u_char *)valloc(NBLOCKS*BLOCKSIZE);
	dev_buf = (u_char *)valloc(NBLOCKS*BLOCKSIZE);

    	filepos(SET);	/* set starting block position */

	if (debug)
		printf("Child process(pid = %d), file i/o start block = %d.\n",
							 getpid(), startblk);

	for (blkno = startblk; blkno <= ((startblk+nblks) - NBLOCKS); blkno += NBLOCKS) 
	{
		switch (mode) {
		case O_RDWR:
			backup_data();
			pattern = (blkno << 4) + 1;
			(void)lfill(pattern);
			write_data();
		case O_RDONLY:
			read_data();
		    	break;
		}
		if ( ok_to_write )
		{
			compare_data(pattern);
			restore_data();
		}
	}
	close_dev(dskp->diskfd);
	if ( backup_buf )
		free(backup_buf);
	if ( dev_buf )
		free(dev_buf);
	return(0);
}

/*
 * open_dev(mode) - open the device as per mode: O_RDONLY, O_WRONLY,
 * or whatever, return file descriptor, return -1 if open failed, 0 if OK
 */
static	int	open_dev(mode)
    int mode;
{
    int diskfd = 0;
    if ((diskfd = open(device, mode, 0666)) < 0)
        send_message(-OPEN_ERROR, ERROR, open_err_msg, device);
    return(diskfd);
}

/*
 * close_dev() - close file descriptor for the
 * device. Return -1 if close failed, 0 if OK
 */
close_dev(diskfd)
int diskfd;
{
    if ((close(diskfd)) < 0) {
        send_message(0, ERROR, close_err_msg, device);
        if (!run_on_error)
            exit(-CLOSE_ERROR);
	return(-1);
    }
    return(0);
}
 
/*
 * backup_data() - write
 * data->buf of NBLOCKS*BLOCKSIZE to file descriptor for device.
 * return 0 if no error, exit if write failed.
 */
static	int	backup_data()
{
	int cc;
	filepos(GET);
	if ((cc = read(dskp->diskfd, backup_buf, NBLOCKS*BLOCKSIZE)) != NBLOCKS*BLOCKSIZE) {
		if (cc == -1) {
			send_message(-READ_ERROR, ERROR, read_fail_msg, which_io, blkno, errmsg(errno));
		 }else
			send_message(-READ_ERROR, ERROR, end_media_fail_msg,
			which_io, cc, blkno);
	}
	filepos(RESET);
	return(0);
}      
 
/*
 * write_data() - write
 * data->buf of NBLOCKS*BLOCKSIZE to file descriptor for device.
 * return 0 if no error, exit if write failed.
 */
static	int	write_data()
{
    int cc;
    need_to_restore = 1;
    if ((cc = write(dskp->diskfd, dev_buf, NBLOCKS*BLOCKSIZE)) != NBLOCKS*BLOCKSIZE) {
        send_message(-WRITE_ERROR, ERROR, write_fail_msg,
	which_io, blkno, errmsg(errno));
    }
    filepos(RESET);
    return(0);
}      

/*
 * restore_data() - write
 * data->buf of NBLOCKS*BLOCKSIZE to file descriptor for device.
 * return 0 if no error, exit if write failed.
 */
static	int	restore_data()
{
    int cc;
    filepos(RESET);
    need_to_restore = 0;
    if ((cc = write(dskp->diskfd, backup_buf, NBLOCKS*BLOCKSIZE)) != NBLOCKS*BLOCKSIZE){
        send_message(-WRITE_ERROR, ERROR, write_fail_msg,
		 which_io, blkno, errmsg(errno));
    }
    return(0);
}      
 
 
/*
 * read_data() - read
 * data->buf of NBLOCKS*BLOCKSIZE to file descriptor for device.
 * return 0 if no error, exit if read failed.
 */
static	int	read_data()
{
    int cc;
 
    if ((cc = read(dskp->diskfd, dev_buf, NBLOCKS*BLOCKSIZE)) != NBLOCKS*BLOCKSIZE) {
	if (cc == -1) {
		send_message(-READ_ERROR, ERROR, read_fail_msg, which_io, blkno, errmsg(errno));
         }else
		send_message(-READ_ERROR, ERROR, end_media_fail_msg,
		which_io, cc, blkno);
    }
    return(0);
}

/*
 * compare_data(pattern) - compare
 * data->buf of NBLOCKS*BLOCKSIZE to pattern. If an error_offset is returned,
 * find the first error_byte, and record the info into an error message.
 * return 0 if no error, exit if compare failed.
 */
static	int	compare_data(pattern)
    u_long pattern;
{
    u_long error_offset;
 
    if ((error_offset = lcheck(pattern, dev_buf))) {
        send_message(-CHECK_ERROR, ERROR, check_err_msg, which_io, 
        "device", blkno, error_offset);
    }
    return(0);
}

/*
 * lfill : fills "count" bytes of memory, starting at "add" with "pattern"
 * ,32-bit (u_long) word at a time. The residue(after divided by
 * sizeof(u_long)) will not be filled.
 *
 * This function always return -1.
 */
static int lfill(pattern)
register u_long pattern;
{
    register u_char *add = dev_buf; 
    register u_long count = NBLOCKS*BLOCKSIZE;

    count /= sizeof(u_long);            /* to be sure */

    while (count-- != 0)
        *add++ = pattern;
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
static u_long lcheck(pattern)
register u_long pattern;
{
    register u_char *add = dev_buf; 
    register u_long count = NBLOCKS*BLOCKSIZE;
    register int i;
 
    count /= sizeof(u_long);            /* to be sure */
 
    for (i = 0; i < count; i++) {
        if (*add++ != pattern)
            return (i * sizeof(u_long));
    }
    return (0);                                /* everything compares */
}

check_mtab(name)
char *name;
{
  FILE	*mtabp;
  int length;
# ifdef SVR4
  struct mnttab mnt;

  sync();

  mtabp = fopen("/etc/mnttab", "r");
  length = strlen(name);
  while ((getmntent(mtabp,&mnt)) != -1) {
    if (!strcmp(mnt.mnt_fstype, MNTTYPE_UFS)) {/* 4.2 file system only */
	if (!strncmp(mnt.mnt_special+5, name, length)) /* the right drive */
	      return(1);
    }
  }
# else
  struct mntent	*mnt;

  sync();

  mtabp = setmntent(MOUNTED, "r");

  length = strlen(name);
  while ((mnt = getmntent(mtabp)) != NULL) {
    if (!strcmp(mnt->mnt_type, MNTTYPE_42)) {	/* 4.2 file system only */
      if (!strncmp(mnt->mnt_fsname+5, name, length)) /* the right drive */
	  return(1);
    }
  }
# endif SVR4
  return(0);		/* not found! */
}

/*
 * filepos() -
 * Modify current file position.
 */
filepos(action)
short action;
{
	static u_long block_offset;
	off_t byte_offset;
	switch ( action ) {
	
	case SET :
		/* set initial filepos */
		byte_offset = (dskp->startblk)*BLOCKSIZE;
#ifdef SVR4
		byte_offset = lseek(dskp->diskfd, byte_offset, SEEK_SET);
#else
		byte_offset = lseek(dskp->diskfd, byte_offset, L_SET);
#endif SVR4
		block_offset = byte_offset/BLOCKSIZE;
		break;
	
	case GET :
		/* Set file position to start of current disk frame */
#ifdef SVR4
		byte_offset = lseek(dskp->diskfd, 0L,SEEK_CUR);
#else
		byte_offset = lseek(dskp->diskfd, 0L,L_INCR);
#endif SVR4
		block_offset = byte_offset/BLOCKSIZE;
		break;

	default  :
		/* reset filepos */
		byte_offset = block_offset*BLOCKSIZE;
#ifdef SVR4
		byte_offset = lseek(dskp->diskfd, byte_offset, SEEK_SET);
#else
		byte_offset = lseek(dskp->diskfd, byte_offset, L_SET);
#endif SVR4
		block_offset = byte_offset/BLOCKSIZE;
		break;
	
	}
	if ( block_offset == -1 )
		perror("seek");
}

static char
get_part(str)
char *str;
{
	int i;
	char c = *str;

	for ( i = 0; *str != 0 ; i++ )
		str++;

	if ( i != 1 )
		c = 'c';
	else if ( c >= 'a' && c <= 'h' )
		;
	else
		c = 'c';
	return(c);
}

/*
 * Calculate the number of tests to run on the specified disk 
 * partition. The minimum test size is 100 MB OR size of partition
 * whichever is less. This is to safeguard against too many tests
 * running simultaneously, thereby overloading the system.
 */
spawn_test(mode)
int mode;
{
	struct dk_map map;		/* disk info */
	int i, fd, st;
	u_long totalblks;
	u_long sblks;			/* initialized to minimum test size */
	int status, pid;
	int startblk = 0;

	fd = open_dev(mode);

#ifdef sun386
	if (strcmp(device, "/dev/rfd0c")) {
		if (ioctl(fd, DKIOCGPART, &map) == -1) 
		    send_message(-IOCTL_ERROR, ERROR, ioctl_err_msg,
			errmsg(errno));
		totalblks = map.dkl_nblk;
	} else
		totalblks = 1440;
#else
	if (ioctl(fd, DKIOCGPART, &map) == -1) 
	    send_message(-IOCTL_ERROR, ERROR, ioctl_err_msg,
			device, errmsg(errno));
	totalblks = map.dkl_nblk;
#endif
	close_dev(fd);

	send_message(0, VERBOSE, start_test_msg, "disk", device);
	if (quick_test && totalblks > 500)
		totalblks = NBLOCKS;
	if (totalblks < 100*2048) /* if size is < 100 MB run only one copy */
		sblks = totalblks;
	else {
		if (spec_size < 100)
			sblks = totalblks;
		else
			sblks = spec_size*2048;
	}

	if ( sblks > totalblks )
		sblks = totalblks;

	/*
         * Calculate the number of children based on 
	 * the user specified test size in MB "S=<> MB"
	 */
	children = (sblks/2 + totalblks)/sblks;
	
	send_message(0, VERBOSE, block_operation_msg, children, sblks/126*children, mode == O_RDONLY ? "reads": "writes/reads", NBLOCKS, BLOCKSIZE);
	if (children == 1) { /* one child only - no fork necessary */
		do_nblock_io(0, 0, totalblks, mode);
	}
	else if (children > 1) {	/* spawn as many children as needed */

		if (sblks*children > totalblks) { /* Make sure it does'nt exceed */
			sblks = totalblks/children;
		}
		for(i = 1; i <= children; i++)
		{
			switch(pid = fork())
			{
				case 0 :	/* child */
					parent = FALSE;
					do_nblock_io(i, startblk, sblks, mode);
					i = children + 1;
					break;
				case -1 :	/* error */
					perror("fork");
					break;
				default :	/* parent */
					parent = TRUE;
					child_pid[i] = pid;
					break;
			}
			startblk += sblks;
		}
		if ( pid != 0 ) {       /* parent should wait */
		   for(i = 1; i <= children; i++) {
			if (stopped_child_pid = wait(&status)) {
				if ( debug )
					printf("Stopped Child = %d\n", stopped_child_pid);
				if ( stopped_child_pid > 0 )
				{ /* valid child pid */
					 st = exit_status(&status);
					if ( st != 0 ) {
						kill_children();
						exit(st);
					}
				}
			}
		   }
		}
		else {
		   if ( debug )
			printf("A child has exitted!\n");
		   exit(0);	/* exit child normally */
		}
	}
}

kill_children()
{
	int i;
	for( i = 1; i <= children; i++ )
	{
		if ( child_pid[i] != stopped_child_pid ) {
			if ( kill(child_pid[i], SIGINT ) != 0 )
			{
				if ( debug )
					printf("Cannot signal(SIGINT) rawtest child(pid = %d)\n", child_pid[i]);
			}
			else
				if ( debug )
					printf("Killed rawtest child(pid = %d)\n", child_pid[i]);
		}
	}
}

/* 
 * Process the exit status of the child
 * and return the encoded exit code to
 * the parent for further processing.
 */
exit_status(s)
int *s;
{

	char exit_code=0;
	int tmp_status = *s;

	switch (tmp_status & 0xff) { /* check low order 8 bits */

	   case 0:
		if ( debug )
			printf("Status = 0x%x\n", tmp_status);
		exit_code = tmp_status >> 8;
		if ( debug )
			printf("exit_code = %d\n", exit_code);
		if (exit_code != 0 ) {
			if ( debug )
				printf("Abnormal exit!! stopped_child_pid = %d\n", stopped_child_pid);
			/* child failed test! */
		}
		else {
			if ( debug )
				printf("Normal exit!! stopped_child_pid = %d\n", stopped_child_pid);
		}
		break;
	   case 0177:
			exit_code = tmp_status >> 8;
			if (debug)
				printf("Child stopped due to signal %d\n", exit_code);
			break;
	   default:
			exit_code = tmp_status & 0x0000007F;
			if (debug)
				printf("Child stopped due to a signal %d\n", exit_code);
	}
	return(exit_code);
}
 
/*
 * clean_up() - cleanup our act before exiting.
 */
clean_up()
{
	if ( parent == TRUE ) /* parent should stop all children */
		kill_children();
	if ( need_to_restore )
		restore_data();
}
