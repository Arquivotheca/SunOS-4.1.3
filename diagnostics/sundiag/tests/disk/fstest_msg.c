/*
 * @(#)fstest_msg.c - Rev 1.1 - 7/30/92
 *
 * fstest_msg.c:  message file for fstest.c.
 *
 */

char *test_usage_msg   = "[D=<device name>] [p={s/0/1/a/5/r}]";
char *reread_err_msg   = "\nRereading and recomparing block %d on %s.\n\n";
char *open_file_msg    = "Opening %s, file1  %s, file2  %s.";
char *mount_err_msg    = "Couldn't get mount table entry for %s.";
char *auto_mnt_err_msg = "Couldn't auto mount any partition on device: %s.";
char *unmnt_err_msg    = "Couldn't unmount directory %s because it was %s.";
char *add_err_msg      = "Couldn't add mount table entry for %s.";
char *free_err_msg     = "Couldn't check free blocks on %s.";
char *transfer_err_msg = "%s failed on %s '%s', blk %d: %s.";
char *flag_err_msg     = "Couldn't %s file '%s' on %s: %s.";
char *pmount_err_msg   = "No partition has been mounted on %s.";
char *pwrite_err_msg   = "No writable partition on %s.";
char *blocks_err_msg   = "Not enough free blocks on %s.";
char *opendev_err_msg  = "Cannot open device for writing: %s.";
char *auto_err_msg     = "%s failed on file %s.";
char *nofs_err_msg     = "No Filesystem on device: %s.";
char *routine_msg      = "Routine specific arguments [defaults]:\n\
 D=<device name>\n\
 p= data pattern {s/0/1/a/5/r}\n\
         s: sequential   0: all zeros,   1: all ones\n\
         a: all a's      5: all fives,   r: random\n";
char *debug_info_msg   = "\nTesting %s, dir = %s, space = %d, blocks = %d,\
 fd1 = %d, name1 = %s, file1_created = %d, fd2 = %d, name2 = %s,\
 file2_created = %d, blks written = %d, blks read = %d.\n";
char *display1_msg   = "\nBlock %d on %s was written with a repeating hex\
 pattern of '%08x'.\n";
char *display2_msg  = "Data from %s '%s' starting at the word in error was:\n";
char *data_msg       = "%08x ";
char *continue_msg   = "\nContinuing with the next block.\n\n";
char *recompare_msg  = "\nRecomparing block %d on %s.\n\n";
char *recompare2_msg = "The compare after a reread of block %d on %s was\
 successful.\n\n";
char *good_recomp_msg= "The recompare of block %d on %s was successful.\n\n";
