
#ifndef lint
static  char sccsid[] = "@(#)menu_analyze.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains functions implementing the analyze menu commands.
 */
#include "global.h"
#include "analyze.h"
#include "misc.h"
#include "param.h"

extern	long random();
extern	char *confirm_list[];

/*
 * This routine implements the 'read' command.  It performs surface
 * analysis by reading the disk.  It is ok to run this command on
 * mounted file systems.
 */
int
a_read()
{
	/*
	 * The current disk must be formatted before disk analysis.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}

	if (check("Ready to analyze (won't harm SunOS). This takes a long time,\nbut is interruptable with CTRL-C. Continue"))
		return (-1);
	return (do_scan(SCAN_VALID,F_NORMAL));
}

/*
 * This routine implements the 'refresh' command.  It performs surface
 * analysis by reading the disk then writing the same data back to the disk.
 * It is ok to run this command on file systems, but not while they are
 * mounted.
 */
int
a_refresh()
{
	/*
	 * The current disk must be formatted before disk analysis.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}

	if (check("Ready to analyze (won't harm data). This takes a long time,\nbut is interruptable with CTRL-C. Continue"))
		return (-1);
	return (do_scan(SCAN_VALID | SCAN_WRITE,F_NORMAL));
}

/*
 * This routine implements the 'test' command.  It performs surface
 * analysis by reading the disk, writing then reading a pattern on the disk,
 * then writing the original data back to the disk.
 * It is ok to run this command on file systems, but not while they are
 * mounted.
 */
int
a_test()
{
	/*
	 * The current disk must be formatted before disk analysis.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}

	if (check("Ready to analyze (won't harm data). This takes a long time,\nbut is interruptable with CTRL-C. Continue"))
		return (-1);
	return (do_scan(SCAN_VALID | SCAN_PATTERN | SCAN_WRITE,F_NORMAL));
}

/*
 * This routine implements the 'write' command.  It performs surface
 * analysis by writing a pattern to the disk then reading it back.
 * It is not ok to run this command on any data you want to keep.
 */
int
a_write()
{
	/*
	 * The current disk must be formatted before disk analysis.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}

	if (check("Ready to analyze (will corrupt data). This takes a long time,\nbut is interruptable with CTRL-C. Continue"))
		return (-1);
	return (do_scan(SCAN_PATTERN,F_NORMAL));
}

/*
 * This routine implements the 'compare' command.  It performs surface
 * analysis by writing a pattern to the disk, reading it back, then
 * checking the data to be sure it's the same.
 * It is not ok to run this command on any data you want to keep.
 */
int
a_compare()
{
	/*
	 * The current disk must be formatted before disk analysis.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}

	if (check("Ready to analyze (will corrupt data). This takes a long time,\nbut is interruptable with CTRL-C. Continue"))
		return (-1);
	return (do_scan(SCAN_PATTERN | SCAN_COMPARE,F_NORMAL));
}

/*
 * This routine implements the 'print' command.  It displays the data
 * buffer in hexadecimal.  It is only useful for checking the disk for
 * a specific set of data (by reading it then printing it).
 */
int
a_print()
{
	int	i, j, lines, nomore = 0;
	int	c, one_line;

#ifdef	lint
	one_line = 0;
#endif	lint

	/*
	 * If we are running out of command file, don't page the output.
	 * Otherwise we are running with a user.  Turn off echoing of
	 * input characters so we can page the output.
	 */
	if (option_f)
		nomore++;
	else {
		enter_critical();
		echo_off();
		charmode_on();
		exit_critical();
	}
	/*
	 * Loop through the data buffer.
	 */
	lines = 0;
	for (i = 0; i < BUF_SECTS * SECSIZE / sizeof (int); i += 6) {
		/*
		 * Print the data.
		 */
		for (j = 0; j < 6; j++)
			if (i + j < BUF_SECTS * SECSIZE / sizeof (int))
				printf("0x%08x  ", *((int *)cur_buf + i + j));
		printf("\n");
		lines++;

		/*
		 * If we are paging and hit the end of a page, wait for
		 * the user to hit either space-bar, "q", return,
		 * or ctrl-C before going on.
		 */
		if (one_line  ||
		    (!nomore  &&  (lines % (TTY_LINES - 1) == 0))) {
			/* Print until first screenfull */
			if (lines < (TTY_LINES -1))
				continue;
			/*
			 * Get the next character.
			 */
			printf("- hit space for more - ");
			c = getchar();
			printf("\015");
			one_line = 0;
			/* Handle display one line command (return key) */
			if (c == '\012') {
				one_line++;
			}
			/* Handle Quit command */
			if (c == 'q') {
				printf("                       \015");
				goto PRINT_EXIT;
			}
		}
	}
	/*
	 * If we were doing paging, turn echoing back on.
	 */
PRINT_EXIT:
	if (!nomore) {
		enter_critical();
		charmode_off();
		echo_on();
		exit_critical();
	}
	return (0);
}

/*
 * This routine implements the 'setup' command.  It allows the user
 * to program the variables that drive surface analysis.  The approach
 * is to prompt the user for the value of each variable, with the current
 * value as the default.
 */
int
a_setup()
{
	int	deflt, size;
	struct	bounds bounds;

	/*
	 * Because of the polarity of the yes/no structure (yes is 0),
	 * we have to invert the values for all yes/no questions.
	 */
	deflt = !scan_entire;
	scan_entire = !input(FIO_MSTR, "Analyze entire disk", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);
	/*
	 * If we are not scanning the whole disk, input the bounds of the scan.
	 */
	if (!scan_entire) {
		bounds.lower = 0;
		if (cur_ctype->ctype_flags & CF_SCSI)
			bounds.upper = datasects() - 1;
		else
			bounds.upper = physsects() - 1;

		scan_lower = (daddr_t)input(FIO_BN,
		    "Enter starting block number", ':',
		    (char *)&bounds, (int *)&scan_lower, DATA_INPUT);
		bounds.lower = scan_lower;
		if (scan_upper < scan_lower)
			scan_upper = scan_lower;
		scan_upper = (daddr_t)input(FIO_BN,
		    "Enter ending block number", ':',
		    (char *)&bounds, (int *)&scan_upper, DATA_INPUT);
	}
	deflt = !scan_loop;
	scan_loop = !input(FIO_MSTR, "Loop continuously", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);
	/*
	 * If we are not looping continuously, input the number of passes.
	 */
	if (!scan_loop) {
		bounds.lower = 1;
		bounds.upper = 100;
		scan_passes = input(FIO_INT, "Enter number of passes", ':',
		    (char *)&bounds, &scan_passes, DATA_INPUT);
	}
	deflt = !scan_correct;
	scan_correct = !input(FIO_MSTR, "Repair defective blocks", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);
	deflt = !scan_stop;
	scan_stop = !input(FIO_MSTR, "Stop after first error", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);
	deflt = !scan_random;
	scan_random = !input(FIO_MSTR, "Use random bit patterns", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);
	bounds.lower = 1;
	/*
	 * The number of blocks per transfer is limited by the buffer
	 * size, or the scan boundaries, whichever is smaller.
	 */
	if (scan_entire)
		size = physsects() - 1;
	else
		size = scan_upper - scan_lower + 1;
	bounds.upper = min(size, BUF_SECTS);
	if (scan_size > bounds.upper)
		scan_size = bounds.upper;
	scan_size = input(FIO_BN, "Enter number of blocks per transfer", ':',
	    (char *)&bounds, &scan_size, DATA_INPUT);
	deflt = !scan_auto;
	scan_auto = !input(FIO_MSTR, "Verify media after formatting", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);

	deflt = !option_msg;
	option_msg = !input(FIO_MSTR, "Enable extended messages", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);
	deflt = !scan_restore_defects;
	scan_restore_defects = !input(FIO_MSTR, "Restore defect list", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);
	deflt = !scan_restore_label;
	scan_restore_label = !input(FIO_MSTR, "Restore disk label", '?',
	    (char *)confirm_list, &deflt, DATA_INPUT);
	printf("\n");
	return (0);
}

/*
 * This routine implements the 'config' command.  It simply prints out
 * the values of all the variables controlling surface analysis.  It
 * is meant to complement the 'setup' command by allowing the user to
 * check the current setup.
 */
int
a_config()
{

	printf("        Analyze entire disk? ");
	if (scan_entire)
		printf("yes\n");
	else {
		printf("no\n");
		printf("        Enter starting block number: %d, ", scan_lower);
		pr_dblock(printf, scan_lower);
		printf("\n        Enter ending block number: %d, ", scan_upper);
		pr_dblock(printf, scan_upper);
		printf("\n");
	}
	printf("        Loop continuously? ");
	if (scan_loop)
		printf("yes\n");
	else {
		printf("no\n");
		printf("        Enter number of passes: %d\n", scan_passes);
	}

	printf("        Repair defective blocks? ");
	if (scan_correct)
		printf("yes\n");
	else
		printf("no\n");

	printf("        Stop after first error? ");
	if (scan_stop)
		printf("yes\n");
	else
		printf("no\n");

	printf("        Use random bit patterns? ");
	if (scan_random)
		printf("yes\n");
	else
		printf("no\n");

	printf("        Enter number of blocks per transfer: %d, ", scan_size);
	pr_dblock(printf, (daddr_t)scan_size);
	printf("\n");

	printf("        Verify media after formatting? ");
	if (scan_auto)
		printf("yes\n");
	else
		printf("no\n");

	printf("        Enable extended messages? ");
	if (option_msg)
		printf("yes\n");
	else
		printf("no\n");
	printf("        Restore defect list? ");
	if (scan_restore_defects)
		printf("yes\n");
	else
		printf("no\n");
	printf("        Restore disk label? ");
	if (scan_restore_defects)
		printf("yes\n");
	else
		printf("no\n");
	printf("\n");
	return (0);
}
/*
 * This routine implements the 'purge' command.  It purges the disk
 * by writing three patterns to the disk then reading the last one back.
 * It is not ok to run this command on any data you want to keep.
 */
int
a_purge()
{
	int status = 0;

	/*
	 * The current disk must be formatted before disk analysis.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}
	if (scan_random) {
		printf("The purge command does not write random data\n");
		scan_random = 0;
	}
	if (scan_passes != NPPATTERNS) {
		printf("The purge command runs %d passes, plus a last pass if the\nfirst %d passes were successful\n",NPPATTERNS,NPPATTERNS);
		scan_passes = NPPATTERNS;
	}
	if (check("Ready to purge (will corrupt data). This takes a long time,\nbut is interruptable with CTRL-C. Continue"))
		return (-1);
	if (status = do_scan(SCAN_PATTERN | SCAN_PURGE,F_NORMAL))
		return (status);
	else {
		scan_passes = 1;
		printf("The first %d passes were successful, running last pass\n",NPPATTERNS);
		return (do_scan(SCAN_PATTERN | SCAN_PURGE_ALPHA_PASS,F_NORMAL));
	}
}
