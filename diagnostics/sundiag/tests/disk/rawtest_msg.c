/*
 * @(#)rawtest_msg.c - Rev 1.1 - 7/30/92
 *
 * rawtest_msg.c:  message file for rawtest.c.
 *
 */

char *start_test_msg   = "\nBegin %s test on %s.";
char *write_blk_msg    = "Writing %d blocks.";
char *read_blk_msg     = "Reading %d blocks.";
char *test_pass_msg    = "Test passed.";
char *open_err_msg     = "Couldn't open %s.";
char *close_err_msg    = "Couldn't close %s.";
char *write_fail_msg   = "%s write failed on disk, block %d: %s."; 
char *read_fail_msg    = "%s read failed on disk, block %d: %s."; 
char *check_err_msg    = "%s compare failed on '%s', block %d, offset %d";
char *mnt_err_msg      = "Mounted file system on device!";
char *ioctl_err_msg    = "ioctl() failed on %s: %s.";
char *block_operation_msg   = "Running %d rawtest process(es) with %d %s, each of size %d blocks (%d bytes/block).\n";
char *test_usage_msg   = "[D=<device name>] [P=<partition>] [S=<size> MB]";
char *routine_msg      = "Routine specific arguments [defaults]:\n\
\tW\t=                   enable write mode\n\
\tD\t= <device name>:    test disk name\n\
\tP\t= <partition name>: partition to test (default is \"c\")\n\
\tS\t= <size>:           size in MB to test (can be used to fork multiple copies)\n";
char *end_media_fail_msg    = "%s read failed due to unexpected end of media:\
%d bytes read, block %d."; 

/* debug messages */
char *raw_parent_read_msg = "Rawtest: Parent: Start Block(%d): Reading(%d)Bytes From \
Block(%d).\n";
char *raw_child_read_msg = "Rawtest: Child(%d): Start Block(%d): Reading(%d)Bytes From \
Block(%d).\n";
char *raw_parent_write_msg = "Rawtest: Parent: Start Block(%d): Writing(%d)Bytes To \
Block(%d).\n";
char *raw_child_write_msg = "Rawtest: Child(%d): Start Block(%d): Writing(%d)Bytes To \
Block(%d).\n";
