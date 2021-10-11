/*
 * @(#)zebra_msg.c - Rev 1.1 - 7/30/92
 *
 * zebra_msg.c: message file for zebra.c.
 *
 */
char *start_test_msg  = "\nBegin %s test on '%s'.";
char *end_test_msg    = "\nEnd %s test on '%s'.";
char *init_test_msg   = "\ninitialized the bpp buffer '%s'.";
char *init_fail_msg   = "\nFailed to initialize the bpp buffer '%s'.";
char *test_pass_msg   = "Test passed.";
char *open_err_msg    = "Couldn't open %s.";
char *close_err_msg   = "Couldn't close '%s'.";
char *write_fail_msg  = "Error %d Failed in writing to '%s'.\n\
\t\t\t\t    Requested bytes to write: '%d'\n\
\t\t\t\t    Bytes successfully written: '%d'"; 
char *writeimg_fail_msg = "Failed in writing ras image to '%s'.";
char *read_fail_msg   = "Failed in reading from '%s'."; 
char *check_err_msg   = "Read compare failed, expected:0x%x, observed:0x%x, '%s'";
char *ioctl_err_msg   = "ioctl() failed on %s: %s.";
char *paper_err_msg   = "No tray installed %s: %s.";
char *devname_err_msg = "Must provide device name!";
char *cable_err_msg   = "Printer cable not plugged in '%s'";
char *mmap_err_msg    = "Couldn't map %s device"; 
char *slot_fail_msg   = "Invalid slot number '%s'";
char *dev_err_msg     = "Device does not exist %s!"; 
char *test_usage_msg  = "[<D=device name> W <I=file name\/default\/user_defined> <R=300\/400> <M=fast\/medium\/extended>]";
char *testbp_usage_msg = "[<D=device name> W <M=fast\/medium\/extended>]";
char *routine_msg     = "Routine specific arguments:\n\
<D=lpvi0\/lpvi1\/lpvi2> = device to be tested\n\
 W=write mode of operation\n\
 I=default\/raster file name\/user_defined\n\
 R=resolution of printer\n\
 M=determines how frequent to print images to the SPARC printer\n\n";
char *routinebp_msg = "Routine specific arguments:\n\
<D=bpp0\/bpp1\/bpp2> = device to be tested\n\
 W=write mode of operation\n\
 M=determines how frequent to print text to the parallel printer\n\n";
