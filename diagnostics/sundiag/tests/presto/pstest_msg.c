#ifndef lint
static  char sccsid[] = "@(#)pstest_msg.c 1.1 92/07/30 Copyright Sun Micro";
#endif
/* message file for pstest.c */

char *routine_msg = "%s specific arguments: \n\
        e    : Performance warning enable\n";
char *test_usage_msg = "[e]";
char *testend_err_msg = "PRESTOSERVE TESTS FAILED!";
char *memcheck_err_msg = "memory_check (pass %d) failed.";
char *filecheck_err_msg = "fileio_check (pass %d) failed.";
char *prgetstatus_err_msg = "Checking prstatus failed: %s";
char *prsetstate1_err_msg = "Failed to turn prestoserve DOWN: %s";
char *prsetstate2_err_msg = "Failed to turn prestoserve UP: %s";
char *prsetstate3_err_msg = "Failed to restore prestoserve state: %s";
char *prreset_err_msg = "Failed to reinitialize Prestoserve: %s";
char *state_err_msg = "Prestoserve is in the ERROR state.\n\
Possible causes:\n\
    1) Errors have occured on a disk drive.";

char *battery_err_msg = "Some batteries are not good or not enabled.";
char *performance_err_msg = 
	"File I/O test failed with low performance ratio:  %4.2f:1\n\
Please also check that presto is configured for /tmp.";

char *performance_warning_msg =
        "Warning: File I/O test has low performance ratio:  %4.2f:1.";

char *mmap_err_msg = "Failed to mmap prestoserve: %s";
char *munmap_err_msg = "Failed to munmap prestoserve: %s";
char *compare1_err_msg = "byte compare: loc=%d, obs= %x, exp= %x";
char *compare2_err_msg = "word compare: loc=%d, obs= %x, exp= %x";
char *compare3_err_msg = "long compare: loc=%d, obs= %x, exp= %x";
char *compare4_err_msg = "data compare failed.";
char *flock1_err_msg = "Failed to lock prestoserve: %s";
char *flock2_err_msg = "Failed to unlock prestoserve: %s";
char *lseek_err_msg = "Lseek failed: %s";
char *open1_err_msg = "Failed to open prestoserve board: %s\n\
Possible causes:\n\
   1) Prestoserve hardware not installed.\n\
   2) Board not fully seated.\n\
   3) Prestoserve software not installed.\n\
   4) Not in superuser mode.";

char *open2_err_msg = "File %s open failed: %s";
char *write_err_msg = "File write failed: %s";
char *read_err_msg = "File read failed: %s";

