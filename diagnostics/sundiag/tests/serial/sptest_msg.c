/*
 * @(#)sptest_msg.c - Rev 1.1 - 7/30/92
 *
 * sptest_msg.c:  message file for sptest.c.
 *
 */

char *ports_msg   = "Ports = %d, testing '%s', board %c, sending %d chars.\n";
char *test_ok_msg        = "Device '%s' tested OK.";
char *open_err_msg       = "Couldn't open file '%s'.";
char *transmit_err_msg   = "Transmit failed on '%s'.";
char *receive_err_msg    = "Receive failed on '%s'.";
char *data_err_msg       = "data error on device '%s', exp = 0x%x, obs = 0x%x.";
char *ports_test_msg     = "%s, port(s) to test = %s.\n";
char *no_ports_msg       = "No serial ports selected.";
char *no_response_msg    = "Device '%s' does not respond. Check loopback.";
char *close_xmit_msg     = "Close transmit port '%s'.\n";
char *open_port_msg      = "Open %s port '%s'; file descriptor %d.\n";
char *close_port_msg     = "Close %s port; file descriptor %d.\n";
char *no_close_msg       = "%s: Not able to close RDescriptor %d.\n";
char *no_status_msg      = "%s: Not able to %s RDESCRIPTOR %d status.\n";
char *not_able_msg       = "%s: Not able to %s on %sDescriptor %d.\n";
#ifdef SVR4
char *not_able_to_push_msg = "%s: Not able to push Tty compatibility STREAMS module.\n";
#endif SVR4
char *not_complete_msg   = "%s: Can't complete %s on RDescriptor %d.\n";
char *no_release_msg     = "%s: Can't release EXCLUSIVE use %sDescriptor %d.\n";
char *no_restore_msg     = "%s: Can't restore TTY mode on %sDescriptor %d.\n";
char *no_expand_msg      = "%s: unable to expand the %s%s argument.\n";
char *test_usage_msg     = "[device {device}]";
