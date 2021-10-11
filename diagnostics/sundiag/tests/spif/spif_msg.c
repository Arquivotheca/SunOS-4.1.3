#ifndef lint
static	char sccsid[] = "@(#)spif_msg.c 1.1 7/30/92 Copyright 1992 Sun Microsystems,Inc.";
#endif

/*
 * spif_msg.c -
 *
 * Contains all of the test messages in spiftest.
- Type any key at the terminal keyboard and observe its echo. \n\
 */

char	*test_usage_msg = "<D=/dev/ttyz00../dev/ttyz17, \/dev/stclp0../dev/stclp2>\
 <T=1..31> <B=110..38400> [<C=5..8> <S=1,2> <P=none\/odd\/even> \
<I=5\/a\/r>]";
char	*start_test_msg = "\nBegin %s test";
char	*end_test_msg 	= "\nEnd %s test";
char	*routine_msg	= "%s specific arguments:\n\
D=ttyy0..ttyy17\/stclp0..stclp2, device to be tested\n\
T=test nunmber.  A hex number specifying a test or a \n\
combination of tests\n\
B=110..38400, baud rate\n\
C=5..8, character size\n\
S=1 or 2, stop bit\n\
P=none\/odd\/even, parity\n\
F=xonxoff\/rtscts\/both, flow control\n\
I=5\/a\/random, test pattern\n";
char	*bad_device_msg	= "Cannot use %s for this test\n";
char	*probe_err_msg	= "No SPC/S card found or device driver not installed\n";
char	*parms_msg	= "\nBaud rate=%d, character size=%d, \
stop bit=%d, parity=%s, control flow=%s ";
char	*data_msg	= " data pattern = 0x%x";
char	*internal_test	= "Internal Test";
char	*lp96_test	= "96-pin Loopback Test";
char	*lp96_msg	= "96-pin Loopback Test \n\
Attach a 96-pin loopback plug firmly to the SPC/S board under test";
char 	*lp25_test	= "25-pin Loopback Test";
char 	*lp25_msg	= "25-pin Loopback Test \n\
Attach a 25-pin loopback plug to the serial port under test";
char	*echotty_test	= "Echo TTY Test";
char	*echotty_msg	= "Echo TTY Test \n\
Connect a TTY terminal to the serial port and type at the tty keyboard\n\
- To terminate, type ctrl-C at the tty keyboard";
char	*printer_test	= "Printer Test";
char	*printer_msg	= "Printer Test \n\
Connect a parallel printer to the printer port under test";
char	*pploop_test	= "Parallel Port Loopback Test";
char	*modemloop_test = "Modem Loopback Test";
char	*dataloop_test	= "Data Loopback test";
char	*open_err_msg	= "Open error on %s - device driver may not be \
installed properly";
char	*read_err_msg	= "Read error on %s";
char	*write_err_msg	= "Write error on %s";
char	*testing_msg	= "Testing device %s";
char 	*device_open	= "Device %s is already opened";
char	*tcsets_err	= "Ioctl TCSETS failed on %s";
char	*tiocmget_err 	= "Ioctl TIOCMGET failed on %s";
char	*tiocmset_err 	= "Ioctl TIOCMSET failed on %s";
char	*tiocmbic_err	= "Ioctl TIOCMBIC failed on %s";
char	*stc_regiow_cor2	=  "Ioctl STC_DCONTROL(STC_REGIOW-COR2) error \
in %s";
char	*stc_regior_ccr = "Ioctl STC_DCONTROL(STC_REGIOR-CCR) error in %s";
char	*stc_regiow_ccr = "Ioctl STC_DCONTROL(STC_REGIOW-CCR) error in %s";
char	*stc_ppcregw	= "Ioctl STC_DCONTROL(STC_PPCREGW-PDATA) error in %s";
char	*stc_ppcregr	= "Ioctl STC_DCONTROL(STC_PPCREGR-PDATA) error in %s";
char	*stc_dcontrol	= "Ioctl STC_DCONTROL error in %s";
char	*stc_sdefaults	= "Ioctl STC_SDEFAULTS error on %s";
char	*stc_gdefaults	= "Ioctl STC_GDEFAULTS error on %s";
char	*stc_gppc	= "Ioctl STC_GPPC error on device %s";
char	*offline_err	= "Off-line error on device %s";
char	*paper_out_err	= "Paper out error on device %s";
char	*busy_err	= "Busy error on device %s";
char	*pp_err		= "Error on device %s";
char	*compare_err_msg	= "Expected = 0x%x, observed = 0x%x";
char	*mismatch_err_msg	= "Expected %d bytes, observed %d bytes";
char	*dsr_clear_err 	= "%s: Expected DSR set, observed clear";
char	*dsr_set_err 	= "%s: Expected DSR clear, observed set";
char	*internal_test_err	= "Internal test failed on %s";
char	*data_test_err 		= "Data Loopback test failed on %s";
char	*pploop_test_err	= "Parallel port loopback test failed on %s";
char	*modemloop_test_err	= "Modem Loopback test failed on %s";
char	*lp_err_msg	= "Timeout error on %s.  Check loopback plug.";
char	*tty_err_msg	= "Timeout error on %s.  Check tty terminal \
connected to %s";
char	*end_echotty	= "End of Echo TTY test";
char	*timeout_err	= "Timeout error on %s";
