/*
 * @(#)printer_msg.c - Rev 1.1 - 7/30/92
 *
 * printer_msg.c:  message file for printer.c.
 *
 */

char *test_usage_msg   = "[p0/p1/p2/p3]";
char *opendev_err_msg  = "Couldn't open device '%s'.";
char *writedev_err_msg = "Failed in writing to '%s'.";
char *write_status_msg = "\nAfter writing 0xff, status = %x\n";
char *prt_status_msg   = "Asserted all 1's on data lines\
 but didn't get the expected %s status signal on '%s'.";
char *send_status_msg  = "\nAfter sending pattern %x, status = %x\n";
char *send_err_msg     = "Sent pattern %x, but got the unexpected %s status\
 signal on '%s'.";
char *routine_msg      = "Routine specific arguments [defaults]:\
 \np0/p1/p2/p3 = the printer port to be tested [default=p0]\n\n";
