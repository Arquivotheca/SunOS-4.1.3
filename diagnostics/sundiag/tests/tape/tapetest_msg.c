/*
 * @(#)tapetest_msg.c - Rev 1.1 - 7/30/92
 *
 * tapetest_msg.c:  message file for tapetest.c and recon.c.
 *
 */

/* 
 * Messages for tapetest.c
 */
char *devname_err_msg  = "Must provide device name!";
char *devfile_err_msg  = "Bad device file %s, must be character device.";
char *bad_devfile_msg  = "Bad device file %s.";
char *read_only_msg    = "File test is disabled in readonly mode.";
char *sharing_msg      = "Sharing for 60 seconds.";
char *rewind_msg       = "Rewinding tape.";
char *retension_msg    = "Retensioning tape.";
char *erase_msg        = "Erasing tape.";
char *start_test_msg   = "Begin %s test on %s.";
char *write_end_msg    = "Writing to end of tape.";
char *read_end_msg     = "Reading to end of tape.";
char *write_blk_msg    = "Writing %d blocks.";
char *read_blk_msg     = "Reading %d blocks.";
char *rewind_after_msg = "Rewinding tape after %s.";
char *switch_2nd_msg   = "Switch to 2nd device.";
char *switch_next_msg  = "Switch to next(%d) device.";
char *reconnect_msg    = "Starting reconnect test.";
char *sleep_msg        = "Sleeping 12 minutes to achieve proper duty cycle.";
char *file_test_msg    = "File test, %s 4 files.";
char *hit_eot_msg      = "Hit eot at block %d.\n";
char *open_err_msg     = "Couldn't open %s: %s.";
char *close_err_msg    = "Couldn't close %s: %s.";
char *tape_status_msg  = "%s:  name = %s, operation = %d, count = %d.\n";
char *test_usage_msg   = "[D=</dev/<device name>] [b=block count] [sq] [nr] [ro]\
 [ns] [ft] [rc]";
char *compare_err_msg  = "%s compare failed on '%s', block %d, offset %d,\
 pattern 0x%x, data= 0x%x.";
char *scsi_io_msg      = "%s tape drive:\n sense key= 0x%x residual= %d,\
 retries= %d\n    file no= %d   block no= %d\n";
char *nonscsi_io_msg   = "%s tape drive: residual= %d ds %x er %x\n";
char *routine_msg      = "%s specific arguments [defaults]:\n\
	/dev/<device name> = name of tape device(required)\n\
	nr = no retension\n\
	ns = no tape sleep\n\
	sq = switch tape fromat\n\
	b  = [block count], specify # of blocks to be tested\n\
	ro = read only test\n\
	rc = enable reconnect test\n\
	ft = enable file test\n";

/* 
 * Messages for recon.c
 */
char *recon_start_msg = "recon: Started.";
char *recon_stop_msg  = "recon: Stopped.";
char *tape_name_msg   = "SCSI tape name: %s.";
char *disk_name_msg   = "SCSI disk name: %s.";
char *device_err_msg  = "recon : No Corresponding disk device.";
char *environ_err_msg = "Needs both SCSI disk and SCSI tape to run the test.";
char *reten_err_msg   = "Couldn't retension '%s'.";
char *file_err_msg    = "Couldn't read file '%s'.";
char *retry_msg       = "Retrying SCSI disconnect/reconnect.";
char *connect_err_msg = "SCSI disconnect/reconnect failed.";
char *spurious_msg    = "Spurious signal received from child.";
