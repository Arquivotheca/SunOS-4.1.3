static char     sccsid[] = "@(#)fstest.c 1.1 7/30/92 Copyright 1984 Sun Micro";
 /*
 * this progam writes two files with random data, and then 
 * compares them. The file sizes are 1/2MB each for a hard disk. 
 * If any errors occur in the process, we log errors and stop. 
 */

#include <stdio.h>
#ifdef	 SVR4
#include <unistd.h>
#endif	 SVR4
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#ifdef	 SVR4
#include <sys/fcntl.h>
#else
#include <sys/file.h>
#endif	 SVR4
#include <sys/param.h>
#ifdef   SVR4
#include <sys/fs/ufs_fs.h>
#else
#include <ufs/fs.h>
#endif	 SVR4
#include <signal.h>
#ifdef	 SVR4
#include <sys/statvfs.h>
#include <sys/statfs.h>
#else
#include <sys/vfs.h>
#endif	 SVR4
#include <sys/mount.h>
#ifdef	 SVR4
#include <sys/dkio.h>
#include <sys/mntent.h>
#include <sys/mnttab.h>
#else
#include <sun/dkio.h>
#include <mntent.h>
#endif	 SVR4
#include "fstest.h"
#include "fstest_msg.h"
#include "sdrtns.h"             /* sundiag standard header file*/
#include "../../../lib/include/libonline.h"   /* online library include */

#define MAXTRIES 300
#define TIME_MAX 16

#ifdef SVR4
struct mnttab *dupmnttab();
#else SVR4
void freemntlist();
struct mntent * dupmntent();
void freemntent();
#endif SVR4

extern int      process_disk_args();
extern int	routine_usage();
extern int      errno;                      /* system error number */
extern char    *mktemp();

int             auto_mount = 0;
int		oldmask=0;
int             drive = 0, files_open = 0, ind;
int		display_read_data = FALSE, sd_quick_test = FALSE;
int             file1_bad = FALSE, reread_block = FALSE;
int             cmp_error = FILE1_CMP_ERROR;
int             recompare = FALSE, already_recompared = FALSE;
int             reread = FALSE, read_retry = FALSE;
int             data_pattern = 100, usr_selected_pattern = FALSE;
int		return_code = 0, failed = FALSE;
int 		bsize = BSIZE, maxblock = MAXBLOCK;

char		device_diskname[32]="";
char	       *device = device_diskname;
char            *name1, *name2;
char            testing_devices[MAX_DRIVES * 5] = {""};
char		tmp_dir[100];

u_long          pattern, block_patterns[MAXBLOCK];
u_long          d_buf[3][BSIZE / sizeof(u_long)];
#ifdef SVR4
u_long     *b1 = d_buf[0], *b2 = d_buf[1], *b3 = d_buf[2];
#else
int u_long     *b1 = d_buf[0], *b2 = d_buf[1], *b3 = d_buf[2];
#endif SVR4

main(argc, argv)
    int             argc;
    char           *argv[];
{
    char	diskname[8];
    int		i;
    int		pid, cpattern;

    versionid = "3.33"; 		/* SCCS version id */
    diskname[0] = '\0';         /* initialize to null string */
    strcpy(device, testing[0].device);
    device_name = device;

    test_init(argc, argv, process_disk_args, routine_usage, test_usage_msg);
    strcpy(diskname, device);
    get_devices(diskname);

    for (i = drive; strcmp(testing[i].device, "") && i < MAX_DRIVES ;i++) {
        strcpy(device, testing[i].device);

        if (!testing[i].file1_created) {
		name1 = testing[i].name1;
		name2 = testing[i].name2;
		name1 = mktemp(name1);
		name2 = mktemp(name2);
        }
        send_message(0, VERBOSE, open_file_msg, device, 
                     testing[i].name1, testing[i].name2);

#ifndef SVR4
	enter_critical(); /* enter critical code area */
#endif SVR4
        if ((testing[i].fd1=open(testing[i].name1,OMODE1,0660)) == EOF)
            send_message(-FILE1_OPEN_ERROR, ERROR, flag_err_msg, "open",
                         testing[i].name1, device, errmsg(errno));
	testing[i].file1_created = TRUE;
	files_open++;

	if ((testing[i].fd2=open(testing[i].name2,OMODE1,0660)) == EOF)
            send_message(-FILE2_OPEN_ERROR, ERROR, flag_err_msg, "open",
                         testing[i].name2, device, errmsg(errno));
	testing[i].file2_created = TRUE;
	testing[i].blocks_written = 0;
	files_open++;
#ifndef SVR4
	exit_critical(); /* exit critical code area */
#endif SVR4
    }

    if (!usr_selected_pattern) {
        if (++data_pattern > DATA_RANDOM)
	    data_pattern = DATA_SEQUENTAL;
         pattern = -1;
    }

    cpattern = -1;
    pid = getpid();

    while (files_open) {
        switch (data_pattern) {
        case DATA_SEQUENTAL:
	    cpattern++;

       /*
	* The pattern will contain the
	* pid in upper 16 bits and block 
	* number in lower 16 bits of word.
	* Start with pattern = 0x----0000
	* End with pattern   = 0x----03ff
	* (The hyphens will have the pid)
	* The layout of each pattern will be
	*
	*       31                                     0
	*        |                                     |
	* WORD  [....|....|....|....|....|....|....|....]
        *	 <------ pid ----- > < --- blkno. ---- >
	*
	* Each block is 512 Bytes. The test writes 1024
	* blocks. So it will write 1/2 MB of data into 
	* each file.
	*/

	    pattern = ((pid&0xffff) << 16) | (cpattern&0xffff);
	    break;
        case DATA_ZERO:
	    pattern = 0;
	    break;
        case DATA_ONES:
	    pattern = 0xffffffff;
	    break;
        case DATA_A:
	    pattern = 0xaaaaaaaa;
	    break;
        case DATA_5:
	    pattern = 0x55555555;
	    break;
        case DATA_RANDOM:
#           ifdef SVR4
	    pattern = rand();
#           else
	    pattern = random();
#           endif
        }
        lfill(b1, bsize, pattern);

        for (i= drive; strcmp(testing[i].device, "") && i < MAX_DRIVES; i++) {
	    if (testing[i].fd1) {
	        strcpy(device, testing[i].device);
	        block_patterns[testing[i].blocks_written] = pattern;
	        if (write(testing[i].fd1, b1, bsize) != bsize) 
		    transfer_error("Write", testing[i].name1,
			           FILE1_WRITE_ERROR, 
                                   testing[i].blocks_written);
	        if (write(testing[i].fd2, b1, bsize) != bsize) 
		    transfer_error("Write", testing[i].name2,
			           FILE2_WRITE_ERROR, 
                                   testing[i].blocks_written);
	        if (++testing[i].blocks_written >= testing[i].blocks) {
		   send_message(0, VERBOSE, "Closing %s, blocks written = %d.",
                                testing[i].device, 
                                testing[i].blocks_written);
	            if (testing[i].fd1 = close(testing[i].fd1)) 
                        send_message(-FILE1_CLOSE_ERROR, ERROR, flag_err_msg, 
                                     "close", testing[i].name1, device, 
                                     errmsg(errno));
		    files_open--;
		    if (testing[i].fd2 = close(testing[i].fd2)) 
                        send_message(-FILE2_CLOSE_ERROR, ERROR, flag_err_msg, 
                                     "close", testing[i].name2, device, 
                                     errmsg(errno));
		    files_open--;
	        }
	    }
        }
    }
    if (!quick_test)
        sleep(10);
    for (i = drive; strcmp(testing[i].device, "") && i < MAX_DRIVES ; i++) {
	strcpy(device, testing[i].device);
	if ((testing[i].fd1 = open(testing[i].name1, OMODE2)) == EOF)
            send_message(-FILE1_REOPEN_ERROR, ERROR, flag_err_msg, "reopen",
                         testing[i].name1, device, errmsg(errno));
	files_open++;
	if ((testing[i].fd2 = open(testing[i].name2, OMODE2)) == EOF)
            send_message(-FILE2_REOPEN_ERROR, ERROR, flag_err_msg, "reopen",
                         testing[i].name2, device, errmsg(errno));
	testing[i].blocks_read = 0;
	files_open++;
    }

    while (files_open) {
	for (i = drive; strcmp(testing[i].device, "") && i < MAX_DRIVES; i++) {
	    if (testing[i].fd1) {
	        strcpy(device, testing[i].device);
	        lfill(b3, bsize, block_patterns[testing[i].blocks_read]);
		if (read_retry) 
                    send_message(0, ERROR, reread_err_msg,
                                 testing[i].blocks_read, device);
		if (read(testing[i].fd1, b1, bsize) != bsize) 
		    transfer_error("Read", testing[i].name1,
			         FILE1_READ_ERROR, testing[i].blocks_read);
	        file1_bad = lcmp(b1, b3, bsize);
		if (read(testing[i].fd2, b2, bsize) != bsize) 
		    transfer_error("Read", testing[i].name2,
			         FILE2_READ_ERROR, testing[i].blocks_read);
		if ((ind = lcmp(b1, b2, bsize)) || file1_bad) {
		    compare_error();
		    while (recompare) {
		        recompare = FALSE;
                        send_message(0, ERROR, recompare_msg,
                                     testing[i].blocks_read, device);
		        file1_bad = lcmp(b1, b3, bsize);
		        if ((ind = lcmp(b1, b2, bsize)) || file1_bad) 
			    compare_error();
		        else
                            send_message(0, ERROR, good_recomp_msg,
				         testing[i].blocks_read, device);
		    }
	        } else if (read_retry) {
		    read_retry = FALSE;
                    send_message(0, ERROR, recompare2_msg,
		                 testing[i].blocks_read, device);
	        }
	        if (++testing[i].blocks_read >= testing[i].blocks) {
		    send_message(0,VERBOSE,"Closing %s, blocks read = %d.",
                            testing[i].device, testing[i].blocks_read);

		    if (testing[i].fd1 = close(testing[i].fd1)) 
                        send_message(-FILE1_RECLOSE_ERROR, ERROR, flag_err_msg,
                                     "reclose", testing[i].name1,
                                     device, errmsg(errno));
		    files_open--;
		    if (testing[i].fd2 = close(testing[i].fd2)) 
                        send_message(-FILE2_RECLOSE_ERROR, ERROR, flag_err_msg,
                                     "reclose", testing[i].name2,
                                     device, errmsg(errno));
		    files_open--;
		}
	    }
	    if (reread_block)
	    {
	        reread_block = FALSE;
		i--;
	    }
	}
    }

    clean_up();
    sleep(10);

    if (failed) 
	exit(return_code);
    test_end();		/* Sundiag normal exit */
}

get_dir(name)
char	*name;
{
  FILE	*mtabp;
  int length;
#ifdef SVR4
  struct mnttab mnt;

   /*
   * Initially set return_code to a sentinel value so that
   * we know whether we have status yet.
   */

  return_code = 1;

    sync();
    if ((mtabp = fopen("/etc/mnttab","r")) == NULL)
	send_message(-FS_PROBE_ERROR, ERROR, mount_err_msg, name);
    length = strlen(name);
    while ((getmntent(mtabp,&mnt)) != -1) {
	if (!strcmp(mnt.mnt_fstype, MNTTYPE_UFS)) { /* 4.2 file system only */
	    if (!strncmp(mnt.mnt_special+5, name, length)) { /* the right drive */
		if (strstr(mnt.mnt_mntopts, MNTOPT_RW) != NULL) { /* be writable */
		    do_stat(mnt.mnt_mountp, mnt.mnt_special);
		    if ( return_code == 0 )
			break; /* There is enough space! */
	   	}
		else {
		    return_code = -NOT_WRITABLE_ERROR;
		    break;
		}
	    }
	}
	else
		return_code = -1;	/* Not a 4.2 file system */
    }
#else
  struct mntent	*mnt;

   /*
   * Initially set return_code to a sentinel value so that
   * we know whether we have status yet.
   */

  return_code = 1;

  sync();
  if ((mtabp = setmntent(MOUNTED, "r")) == NULL)
	send_message(-FS_PROBE_ERROR, ERROR, mount_err_msg, name);

  length = strlen(name);
  while ((mnt = getmntent(mtabp)) != NULL) {
    if (!strcmp(mnt->mnt_type, MNTTYPE_42)) {   /* 4.2 file system only */
      if (!strncmp(mnt->mnt_fsname+5, name, length)) { /* the right drive */
	  if ( hasmntopt(mnt, MNTOPT_RW) != (char *)0) {    /* must be writable */
		do_stat(mnt->mnt_dir, mnt->mnt_fsname) ;
		if ( return_code == 0 )
			break; /* There is enough space! */
      }
      else
	   return_code = -NOT_WRITABLE_ERROR;
	   /* remember that we had a read-only fs mounted */
      }
    } 
    else
      return_code = -NOT_WRITABLE_ERROR;  /* not a 4.2 file system */
  }
  endmntent(mtabp);
#endif SVR4

  if (return_code == 1)
      auto_mnt(name);
}

do_stat(dir,name)
char *dir, *name;
{
# ifdef SVR4
  struct statvfs fs ;
# else
  struct statfs	fs;
# endif
  int free;

# ifdef SVR4
  if (statvfs(dir,&fs) < 0)
# else
  if (statfs(dir, &fs) < 0) 
# endif
    send_message(-FS_PROBE_ERROR, ERROR, free_err_msg, name);
  else {
#ifdef SVR4
    if ((free = fs.f_bavail*fs.f_frsize/1024) >= 1024) {
#else SVR4
    if ((free = fs.f_bavail*fs.f_bsize/1024) >= 1024) {
#endif SVR4
      if (free < 2048) {
        maxblock = 10;
      }
      strcpy(testing[drive].directory, dir);
      if ( auto_mount == FALSE )
      	strcat(testing[drive].directory, "/tmp");
      if (access(testing[drive].directory, F_OK))
	  mkdir(testing[drive].directory, 0600);
      testing[drive].space = free;
      return_code = 0;  /* must be enough free blocks */
    }
    else {
	/* must be not enough free blocks */
	return_code = -NOT_ENOUGH_FREE_ERROR;
    }
  }
}

get_devices(diskname)
char	*diskname;
{
    int i  = drive;

	strcpy(testing[i].device, diskname);

	get_dir(diskname);

	if ( return_code < 0 )
		error_exit(diskname);

	if (testing[i].space > maxblock)
	    testing[i].blocks = maxblock;
	else
	    testing[i].blocks = testing[i].space;

	testing[i].fd1 = 0;
	strcpy(testing[i].name1, testing[i].directory);
	strcat(testing[i].name1, "/");
	strcat(testing[i].name1, NAME1);
	testing[i].file1_created = FALSE;
	testing[i].fd2 = 0;
	strcpy(testing[i].name2, testing[i].directory);
	strcat(testing[i].name2, "/");
	strcat(testing[i].name2, NAME2);
	testing[i].file2_created = FALSE;
	testing[i].blocks_written = 0;
	testing[i++].blocks_read = 0;
	if ( i < MAX_DRIVES )
		strcpy(testing[i].device, "");

    for (i = drive; strcmp(testing[i].device, "") && i < MAX_DRIVES; i++) {
	strcat(testing_devices, " ");
	strcat(testing_devices, testing[i].device);
	if (quick_test || sd_quick_test)
	    testing[i].blocks = 2;
	send_message(0, DEBUG, debug_info_msg, testing[i].device,
                     testing[i].directory, testing[i].space,
                     testing[i].blocks, testing[i].fd1,
                     testing[i].name1, testing[i].file1_created,
                     testing[i].fd2, testing[i].name2,
                     testing[i].file2_created, 
                     testing[i].blocks_written,testing[i].blocks_read);
    }
}

transfer_error(type, filename, code, rec)
    char           *type;
    char           *filename;
    int             code, rec;
{
    send_message(-code, ERROR, transfer_err_msg, type, device, filename, 
                 rec, errmsg(errno));
}

compare_error()
{
char		msg_buffer[100];
char		*msg = msg_buffer;
    if (file1_bad) {
	ind = file1_bad;
	file1_bad = FALSE;
	cmp_error = FILE1_BAD;
	sprintf(msg, "'%s'", testing[drive].name1);
    } else
	sprintf(msg, "between '%s' and '%s'",
		testing[drive].name1, testing[drive].name2);

    if (display_read_data || run_on_error) {
	failed = TRUE;
	return_code = cmp_error;
        send_message(0, ERROR, "Compare error on %s %s, blk %d, offset %d.", device, msg, testing[drive].blocks_read, bsize - sizeof(u_long) * ind);	/* compare error */
        display_data();
    } else
        send_message(-cmp_error, ERROR, "Compare error on %s %s, blk %d, offset %d.", device, msg, testing[drive].blocks_read, bsize - sizeof(u_long) * ind);
}

display_data()
{
    int             i;
    char            ch[20];

    send_message(0, CONSOLE, display1_msg, testing[drive].blocks_read, device,
                 block_patterns[testing[drive].blocks_read]);
    send_message(0, CONSOLE, display2_msg, device, testing[drive].name1);

    for (i = 0; i < 8; i++) {
        send_message(0, CONSOLE, data_msg, d_buf[0][i + 
                     ((bsize - sizeof(u_long) * ind) / sizeof(u_long))]);
    }

    send_message(0, CONSOLE, display2_msg, device, testing[drive].name2);

    for (i = 0; i < 8; i++) {
        send_message(0, CONSOLE, data_msg, d_buf[1][i + 
                     ((bsize - sizeof(u_long) * ind) / sizeof(u_long))]);
    }

    send_message(0, CONSOLE, "\n");

    if (!run_on_error) {
        send_message(0, CONSOLE, "\nContinue check? y/n: ");
	scanf("%s", ch);
	if (ch[0] == 'n') {
            send_message(0, CONSOLE, "Remove test files? y/n: ");
	    scanf("%s", ch);
	    if (ch[0] == 'y') {
		clean_up();
		exit(cmp_error);
	    } else
		exit(LEAVE_FILES);
	}
        send_message(0, CONSOLE, "Redo the compare? y/n ");
	scanf("%s", ch);
	if (ch[0] == 'y') {
	    recompare = TRUE;
	} else {
            send_message(0, CONSOLE, "Reread the same blocks? y/n ");
	    scanf("%s", ch);
	    if (ch[0] == 'y') {
		lseek(testing[drive].fd1, -(bsize), 1);
		lseek(testing[drive].fd2, -(bsize), 1);
		testing[drive].blocks_read--;
		reread_block = TRUE;
		read_retry = TRUE;
	    } else
		read_retry = FALSE;
	}
    } else {	/* run_on_error */
	if (!already_recompared) {
	    already_recompared = TRUE;
	    recompare = TRUE;
	} else {
	    if (!reread) {
		reread = TRUE;
		lseek(testing[drive].fd1, -(bsize), 1);
		lseek(testing[drive].fd2, -(bsize), 1);
		testing[drive].blocks_read--;
		reread_block = TRUE;
		read_retry = TRUE;
	    } else {
		read_retry = FALSE;
		already_recompared = FALSE;
		reread = FALSE;
                send_message(0, CONSOLE, continue_msg);
	    }
	}
    }
}

/*
 * lcmp : compares the contents of two memory buffers, one 32-bit(u_long)
 * word at a time. This comparison starts at the low end of the buffer, and
 * proceeds towards high. 
 *
 *
 * return : the size of the buffer, in the unit of 32-bit(u_long), less the
 * offset from starting address to the first location that does not compare.
 * 0 if everything compares. 
 *
 * note: the residue(after divided by sizeof(u_long)) won't be compared. 
 */
lcmp(add1, add2, count)
    register u_long *add1, *add2;
    register u_long count;
{
    count /= sizeof(u_long);	       /* to be sure */

    while (count--)
	if (*add1++ != *add2++)
	    return (++count);

    return (0);				       /* everything compares */
}

/*
 * lfill : fills "count" bytes of memory, starting at "add" with "pattern"
 * ,32-bit (u_long) word at a time. The residue(after divided by
 * sizeof(u_long)) will not be filled. 
 *
 * return: this function always return the integer -1. 
 */
int 
lfill(add, count, pattern)
    register u_long *add;
    register u_long count, pattern;
{
    count /= sizeof(u_long);	       /* to be sure */

    while (count--)
	*add++ = pattern;
}

process_disk_args(argv, arrcount)
    char    *argv[];
    int     arrcount;
{
     if (strncmp(argv[arrcount], "p=", 2) == 0) {
         usr_selected_pattern = TRUE;
         if (argv[arrcount][2] == 's') 
             data_pattern = DATA_SEQUENTAL;
         if (argv[arrcount][2] == '0')
             data_pattern = DATA_ZERO;
         if (argv[arrcount][2] == '1')
             data_pattern = DATA_ONES;
         if (argv[arrcount][2] == 'a') 
             data_pattern = DATA_A;
         if (argv[arrcount][2] == '5') 
             data_pattern = DATA_5;
         if (argv[arrcount][2] == 'r') 
             data_pattern = DATA_RANDOM;
     } else if (strncmp(argv[arrcount],"D=", 2) == 0) {  /* sundiag */
         strcpy(device, &argv[arrcount][2]);  /* set diskname */
         verbose = FALSE;
     } else 
	 return (FALSE);

     return (TRUE);
}

routine_usage()
{
    (void) send_message(0, CONSOLE, routine_msg);
}

/*
 * This function auto mounts partitions on the disk device,
 * if a file system exists on it and checks if it has enough
 * space.
 */
auto_mnt(name)
char *name;
{
	char nbuf[25];
	struct fs fs;
	int partition, fd;

        for ( partition = 'a' ; partition <= 'h' ; partition++ ) {
		if ( partition == 'b' )
			continue;	/* Ignore partition 'b' */
                sprintf(nbuf, "/dev/%s%c", name, partition);

		/* check to see if a file system exists here. */
		if ( (fd = open(nbuf, O_RDWR)) > 0 ) { /* Device open succeeded ! */
			lseek(fd, BBSIZE, 0); /* Go to start of super block */
			read(fd, &fs, sizeof(fs));
			close(fd);
			if ( fs.fs_magic != FS_MAGIC ) {/* No fs */
				return_code = -NOFS_ERROR; 
				continue; /* No file system here! */
			}
		}
		else { 		/* Partition does not exist! */
			return_code = -OPENDEV_ERROR;
			continue;
		}

	        if (access("/tmp", F_OK))
		  	mkdir("/tmp", 0600);

		/* Build the temporary directory name */
		sprintf(tmp_dir,"/tmp/%s%c.XXXXXX", name, partition);
		strcpy(tmp_dir,mktemp(tmp_dir));

		/* Now that the device exists, try mounting it! */
		return_code = mnt(nbuf);
		if (return_code >= 0) {/* now check for enough space */
			do_stat(tmp_dir, nbuf);
			if (return_code < 0) {/* Try next partition */
				auto_mount = FALSE;
				unmnt(tmp_dir);
			} else 
				break; /* Success! */
		}
	}
}

/*
 * The actual mount.
 */
mnt(nbuf)
char *nbuf;
{
#ifndef SVR4
        struct ufs_args args;
	enter_critical(); /* enter critical code area */
#endif
	mkdir(tmp_dir, 0600);
#ifdef SVR4
	if ( mount(nbuf,tmp_dir, MS_DATA, "ufs") )
#else
	args.fspec = nbuf;
	if ( mount(MNTTYPE_42, tmp_dir, M_NEWTYPE, &args) )
#endif SVR4
	{	/* problems mounting? */
		rmdir(tmp_dir);
		return(-AUTO_MNT_ERROR); /* Try next partition */
	}
	auto_mount = TRUE;	/* successful mount */
#ifndef SVR4
	exit_critical(); /* exit critical code area */
#endif SVR4

	return(0);
}

/*
 * The actual unmount and removal
 * of temporary directory.
 */
unmnt(tmp_dir)
char *tmp_dir;
{
#ifdef SVR4
	if ( umount(tmp_dir) < 0)
#else
	enter_critical(); /* enter critical code area */
	if (unmount(tmp_dir) < 0)
#endif SVR4
		send_message(-AUTO_UNMNT_ERROR, ERROR, unmnt_err_msg, tmp_dir, errno == EBUSY ? "busy" : "not mounted");
	rmdir(tmp_dir);
#ifndef SVR4
	exit_critical(); /* exit critical code area */
#endif SVR4
	return(0);
}

/*
 * Print error message and exit
 */
error_exit(diskname)
char *diskname;
{
	switch (return_code) {
	case  0:
		break;

	case -NOT_ENOUGH_FREE_ERROR :/* not enough free blocks on drive */
		send_message(-NOT_ENOUGH_FREE_ERROR, ERROR, blocks_err_msg, diskname);

	case -NOT_WRITABLE_ERROR:	/* no writable partition on drive */

		send_message(-NOT_WRITABLE_ERROR, ERROR, pwrite_err_msg, diskname);
	case -NOFS_ERROR :

		send_message(-NOFS_ERROR, ERROR, nofs_err_msg, diskname);

	case -AUTO_MNT_ERROR :

		send_message(-AUTO_MNT_ERROR, ERROR, auto_mnt_err_msg, diskname);
	case -OPENDEV_ERROR :

		send_message(-OPENDEV_ERROR, ERROR, opendev_err_msg, diskname);
	}
}

#ifndef SVR4
/*
 * This function is called just before executing
 * some critical part of the code. The SIGINT
 * signal when/if received while executing this 
 * part of the code will be temporarily blocked 
 * and serviced at a later stage(in exit_critical()).
 */
enter_critical()
{
	oldmask = sigblock(sigmask(SIGINT));
}

/*
 * This function is called just after executing
 * some critical part of the code. The SIGINT
 * signal will be  unblocked and the old signal
 * mask will be restored, so that the interrupts
 * may be serviced as originally intended.
 */
exit_critical()
{
	(void) sigsetmask(oldmask);
}
#endif SVR4

clean_up()
{
    int 	i;
    name1 = testing[0].name1;
    name2 = testing[0].name2;

    for (i = drive; strcmp(testing[i].device, "") && i < MAX_DRIVES; i++) {
	strcpy(device, testing[i].device);
	if (testing[i].file1_created) {
	    if (testing[i].fd1 > 0)
		testing[i].fd1 = close(testing[i].fd1);
	    testing[i].file1_created = FALSE;
	    if (unlink(testing[i].name1)) 
                send_message(-FILE1_UNLINK_ERROR, ERROR, flag_err_msg, 
                             "unlink", testing[i].name1, device, 
                             errmsg(errno));
	}
	if (testing[i].file2_created) {
	    if (testing[i].fd2 > 0)
		testing[i].fd2 = close(testing[i].fd2);
	    testing[i].file2_created = FALSE;
	    if (unlink(testing[i].name2)) {
                send_message(-FILE2_UNLINK_ERROR, ERROR, flag_err_msg, 
                             "unlink", testing[i].name2, device, 
                             errmsg(errno));
	    }
	}
    }
    if ( auto_mount == TRUE )
    {
	auto_mount = FALSE;
	unmnt(tmp_dir);
    }
}
