#ifndef lint
static char version[] = "Version 1.1";
static char sccsid[]  = "@(#)mono_msg.c 1.1 7/30/92 Copyright Sun Micro";
#endif

/*
 * mono_msg.c:  message file for mono.c.
 */

char *test_usage_msg = "/dev/<device name>";
char *e_usage_msg = "Must provide device name!";
char *e_ioctl_msg = "ioctl() failed: couldn't get info on frame buffer type.";
char *e_openfb_msg = "Unable to open /dev/fb.";
char *v_start_msg = "Started on %s.";
char *e_pr_open_msg = "Couldn't open file '%s'. System error message is '%s'.";
char *v_fbsize_msg = "frame buffer size is %dx%d. (%d bytes).";
char *v_completed_msg = "Monochrome test completed successfully.";
char *e_open_msg = "open() of %s failed.";
char *e_mmap_msg = "Mmap() failed. System error message is '%s'";
char *v_fbaddr_msg = "Monochrome test starting at my process' address 0x%x.";
char *e_const_msg = "Constant pattern test:  found error at offset 0x%x\
 (from base of frame buffer);  expected = 0x%x; obs = %x.";
char *e_addr_msg = "Address test (byte mode):  found error at offset 0x%x\
 (from base of frame buffer); expected = 0x%x; observed = 0x%x.";
char *e_laddr_msg = "Address test (long mode): found error at offset 0x%x\
 (from base of frame buffer); expected = 0x%x; observed = 0x%x.";
char *e_rand_msg = "Random pattern test:  found error at offset (from base\
 of frame buffer) 0x%x; expected = 0x%x; observed = 0x%x.";
char *e_stripe_msg = "Stripe pattern test:  found error at offset 0x%x\
 (from base of frame buffer);  expected = 0%x; obs = 0%x.";
char *routine_msg = "Routine specific arguments [defaults]:\n/dev/<device\
 name> = name of monochrome device(required)\n";
