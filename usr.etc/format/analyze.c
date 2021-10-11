
#ifndef lint
static  char sccsid[] = "@(#)analyze.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains routines to analyze the surface of a disk.
 */
#include "global.h"
#include "analyze.h"
#include "param.h"

extern	long random();

/*
 * These global variables control the surface analysis process.  They
 * are set from a command in the defect menu.
 */
int	scan_entire = 1;		/* scan whole disk flag */
daddr_t	scan_lower = 0;			/* lower bound */
daddr_t	scan_upper = 0;			/* upper bound */
int	scan_correct = 1;		/* correct errors flag */
int	scan_stop = 0;			/* stop after error flag */
int	scan_loop = 0;			/* loop forever flag */
int	scan_passes = 2;		/* number of passes */
int	scan_random = 0;		/* random patterns flag */
int	scan_size = 0;			/* sectors/scan operation */
int	scan_auto = 1;			/* scan after format flag */
int	scan_restore_defects = 1;	/* restore defect list after writing */
int	scan_restore_label = 1;		/* restore label after writing */

/*
 * These are summary variables to print out info after analysis.
 * Values less than 0 imply they are invalid.
 */
daddr_t	scan_cur_block = -1;		/* current block */
int	scan_blocks_fixed = -1;		/* # blocks repaired */

/*
 * This variable is used to tell whether the most recent surface
 * analysis error was caused by a media defect or some other problem.
 */
int	media_error;			/* error was caused by defect */

/*
 * These are the data patterns used if random patterns are not chosen.
 * They are designed to show pattern dependent errors.
 */
int	scan_patterns[] = {
	0xc6dec6de,
	0x6db6db6d,
	0x00000000,
	0xffffffff,
	0xaaaaaaaa,
};
#define NPATTERNS	5		/* number of predefined patterns */

/*
 * These are the data patterns from the SunFed requirements document.
 */
int purge_patterns[]= {               /* patterns to be written */
        0xaaaaaaaa,             /* 10101010... */
        0x55555555,             /* 01010101...  == UUUU... */
        0xaaaaaaaa,             /* 10101010... */
        0xaaaaaaaa,             /* 10101010... */
};

int alpha_pattern =  0x40404040; 		/* 10000000...  == @@@@... */

/*
 * This routine performs a surface analysis based upon the global
 * parameters.  It is called from several commands in the defect menu,
 * and from the format command in the command menu (if post-format
 * analysis is enable).
 */
do_scan(flags,mode)
	int	flags,mode;
{
	daddr_t	start, end, curnt;
	int	pass, size, needinit, data;
	int	status, founderr, i, j;
	int	error = 0;

	/*
	 * Check to be sure we aren't correcting without a defect list
	 * if the controller can correct the defect.
	 */
	if (scan_correct && (cur_ops->repair != NULL) &&
			cur_list.list == NULL) {
		eprint("Current Defect List must be initialized to do ");
		eprint("automatic repair.\n");
		return (-1);
	}
	/*
	 * Define the bounds of the scan.
	 */
	if (scan_entire) {
		start = 0;
		if (cur_ctype->ctype_flags & CF_SCSI)
			end = datasects() - 1;
		else
			end = physsects() - 1;

	} else {
		start = scan_lower;
		end = scan_upper;
	}
	/*
	 * Make sure the user knows if we are scanning over a mounted
	 * partition.
	 */
	if ((flags & (SCAN_PATTERN | SCAN_WRITE)) &&
	    (checkmount(cur_ctlr->ctlr_dname, cur_disk->disk_unit,
	    start, end))) {
		printf("Cannot do analysis on a mounted partition.\n");
		return (-1);
	}
	/*
	 * If we are scanning destructively over certain sectors,
	 * we mark the defect list and/or label dirty so it will get rewritten.
	 */
	if (flags & (SCAN_PATTERN | SCAN_WRITE)) {
		if (start < totalsects() && end >= datasects()) {
			cur_list.flags |= LIST_DIRTY;
			if (cur_disk->disk_flags & DSK_LABEL)
				cur_flags |= LABEL_DIRTY;
		}
		if (start == 0) {
			if (cur_disk->disk_flags & DSK_LABEL)
				cur_flags |= LABEL_DIRTY;
		}
	}
	/*
	 * Initialize the summary info on sectors repaired.
	 */
	scan_blocks_fixed = 0;
	/*
	 * Loop through the passes of the scan. If required, loop forever.
	 */
	for (pass = 0; pass < scan_passes || scan_loop; pass++) {
		printf("\n        pass %d", pass);
		/*
		 * Determine the data pattern to use if pattern testing
		 * is to be done.
		 */
		if (flags & SCAN_PATTERN) {
			if (scan_random)
				data = (int)random();
			else if (flags & SCAN_PURGE_ALPHA_PASS)
				data = alpha_pattern;
			else if (flags & SCAN_PURGE) {
				if (((pass + 1) % NPPATTERNS) == 0)
					flags |= SCAN_PURGE_READ_PASS;
				data = purge_patterns[pass % NPPATTERNS];
			} else
				data = scan_patterns[pass % NPATTERNS];
			printf(" - pattern = 0x%x", data);
		}
		printf("\n");
		/*
		 * Mark the pattern buffer as corrupt, since it
		 * hasn't been initialized.
		 */
		needinit = 1;
		/*
		 * Print the first block number to the log file if
		 * logging is on so there is some record of what
		 * analysis was performed.
		 */
		if (log_file) {
			pr_dblock(lprint, start);
			lprint("\n");
		}
		/*
		 * Loop through this pass, each time analyzing an amount
		 * specified by the global parameters.
		 */
		for (curnt = start; curnt <= end; curnt += size) {
			if ((end - curnt) < scan_size)
				size = end - curnt + 1;
			else
				size = scan_size;
			/*
			 * Print out where we are, so we don't look dead.
			 * Also store it in summary info for logging.
			 */
			scan_cur_block = curnt;
			nolprint("   ");
			pr_dblock(nolprint, curnt);
			nolprint("  \015");
			(void) fflush(stdout);
			/*
			 * Do the actual analysis.
			 */
			status = analyze_blocks(flags, (daddr_t)curnt, size,
			    (unsigned)data, needinit, (F_ALLERRS | F_SILENT));
			/*
			 * If there were no errors, the pattern buffer is
			 * still initialized, and we just loop to next chunk.
			 */
			needinit = 0;
			if (!status)
				continue;
			/*
			 * There was an error. Mark the pattern buffer
			 * corrupt so it will get reinitialized.
			 */
			needinit = 1;
			/*
			 * If it was not a media error, ignore it.
			 */
			if (!media_error)
				continue;
			/*
			 * Loop 5 times through each sector of the chunk,
			 * analyzing them individually.
			 */
			nolprint("   ");
			pr_dblock(nolprint, curnt);
			nolprint("  \015");
			(void) fflush(stdout);
			founderr = 0;
			for (j = 0; j < size * 5; j++) {
				i = j % size;
				status = analyze_blocks(flags, (daddr_t)
				    (curnt + i), 1, (unsigned)data, needinit,
				    F_ALLERRS);
				needinit = 0;
				if (!status)
					continue;
				/*
				 * An error occurred.  Mark the buffer
				 * corrupt and see if it was media
				 * related.
				 */
				needinit = 1;
				if (!media_error)
					continue;
				/*
				 * We found a bad sector. Print out a message
				 * and fix it if required.
				 */
				founderr = 1;
				if (scan_correct) {
					enter_critical();
					if (cur_ops->repair == NULL) {
						eprint("Warning: Controller does ");
						eprint("not support repairing.\n\n");
						status = -1;
					} else {
						eprint("Repairing...");
						status = (*cur_ops->repair)
						    (curnt + i,mode);
					}
					if (status) {
						/*
						 * If the repair failed, we
						 * note it and will return the
						 * failure. However, the
						 * analysis goes on.
						 */
						eprint("failed.\n\n");
						error = -1;
					} else {
						/*
						 * If the repair worked, add
						 * the defect to the list and
						 * write the list out.  Also,
						 * kill the working list so it
						 * will get resynced with the
						 * current list.
						 */
						eprint("succeeded.\n\n");
						/*
						 * The next "if" statement reflects the
						 * fix for bug id 1026096 where the format
						 * program keeps adding the same defect to
						 * the defect list.
						 */
						if (cur_ctype->ctype_flags & CF_WLIST) {
							kill_deflist(&cur_list);
							(*cur_ops->ex_cur)(&cur_list);
							printf("Current list updated\n");
						} else {
							add_ldef(curnt + i, &cur_list);
							write_deflist(&cur_list);
						}

						kill_deflist(&work_list);

						/* Log the repair.  */
						scan_blocks_fixed++;
					}
					exit_critical();
				} else
					eprint("\n");
				/*
				 * Stop after the error if required.
				 */
				if (scan_stop)
					goto out;
			}
			/*
			 * Mark the pattern buffer corrupt to be safe.
			 */
			needinit = 1;
			/*
			 * We didn't find an individual sector that was bad.
			 * Print out a warning.
			 */
			if (!founderr) {
				eprint("Warning: unable to pinpoint ");
				eprint("defective block.\n");
				eprint("soft error may recur, reallocate ");
				eprint("it if it does.\n");
			}
		}
		/*
		 * Print the end of each pass to the log file.
		 */
		enter_critical();
		if (log_file) {
			pr_dblock(lprint, scan_cur_block);
			lprint("\n");
		}
		scan_cur_block = -1;
		exit_critical();
		printf("\n");
	}
out:
	/*
	 * We got here either by giving up after an error or falling
	 * through after all passes were completed.
	 */
	printf("\n");
	enter_critical();
	/*
	 * If the defect list is dirty, write it to disk, 
	 * if scan_restore_defects (the default) is true.
	 */
	if ((cur_list.flags & LIST_DIRTY) && (scan_restore_defects)) {
		cur_list.flags = 0;
		write_deflist(&cur_list);
	}
	/*
	 * If the label is dirty, write it to disk.
	 * if scan_restore_label (the default) is true.
	 */
	if ((cur_flags & LABEL_DIRTY) && (scan_restore_label)) {
		cur_flags &= ~LABEL_DIRTY;
		(void) write_label();
	}
	/*
	 * If we dropped down to here after an error, we need to write
	 * the final block number to the log file for record keeping.
	 */
	if (log_file && scan_cur_block >= 0) {
		pr_dblock(lprint, scan_cur_block);
		lprint("\n");
	}
	printf("Total of %d defective blocks repaired.\n", scan_blocks_fixed);
	/*
	 * Reinitialize the logging variables so they don't get used
	 * when they are not really valid.
	 */
	scan_blocks_fixed = scan_cur_block = -1;
	exit_critical();
	return (error);
}

/*
 * This routine analyzes a set of sectors on the disk.  It simply returns
 * an error if a defect is found.  It is called by do_scan().
 */
int
analyze_blocks(flags, blkno, blkcnt, data, init, driver_flags)
	int	flags, driver_flags, blkcnt, init;
	register unsigned data;
	daddr_t	blkno;
{
	int	corrupt = 0;
	register int	status, i, nints;
	register unsigned *ptr = (unsigned int *)pattern_buf;

	/*
	 * Initialize the pattern buffer if necessary.
	 */
	nints = blkcnt * SECSIZE / sizeof (int);
	if ((flags & SCAN_PATTERN) && init) {
		for (i = 0; i < nints; i++)
			*((int *)pattern_buf + i) = data;
	}
	/*
	 * Lock out interrupts so we can insure valid data will get
	 * restored. This is necessary because there are modes
	 * of scanning that corrupt the disk data then restore it at
	 * the end of the analysis.
	 */
	enter_critical();
	/*
	 * If the disk data is valid, read it into the data buffer.
	 */
	if (flags & SCAN_VALID) {
		status = (*cur_ops->rdwr)(DIR_READ, cur_file, blkno,
		    blkcnt, (caddr_t)cur_buf, driver_flags);
		if (status)
			goto bad;
	}
	/*
	 * If we are doing pattern testing, write and read the pattern
	 * from the pattern buffer.
	 */
	if (flags & SCAN_PATTERN) {
		/*
		 * If the disk data was valid, mark it corrupt so we know
		 * to restore it later.
		 */
		if (flags & SCAN_VALID)
			corrupt++;
		/*
		 * Only write if we're not on the read pass of SCAN_PURGE.
		 */
		if (!(flags & SCAN_PURGE_READ_PASS))
		    	status = (*cur_ops->rdwr)(DIR_WRITE, cur_file, blkno,
		    	    blkcnt, (caddr_t)pattern_buf, driver_flags);
		if (status)
			goto bad;
		/*
		 * Only read if we are on the read pass of SCAN_PURGE, if we
		 * are purging.
		 */
		if ((!(flags & SCAN_PURGE)) || (flags & SCAN_PURGE_READ_PASS))
			status = (*cur_ops->rdwr)(DIR_READ, cur_file, blkno,
		    	    blkcnt, (caddr_t)pattern_buf, driver_flags);
		if (status)
			goto bad;
	}
	/*
	 * If we are doing a data compare, make sure the pattern
	 * came back intact.
	 * Only compare if we are on the read pass of SCAN_PURGE, or
	 * we wrote random data instead of the expected data pattern.
	 */
	if ((flags & SCAN_COMPARE) || (flags & SCAN_PURGE_READ_PASS)) {
		for (i = nints, ptr = (unsigned *)pattern_buf; i--; )
			if (*ptr++ != data) {
				eprint("Data miscompare error (expecting ");
				eprint("0x%x, got 0x%x) at ", data,
			    	    *((int *)pattern_buf + (nints - i)));
				pr_dblock(eprint, blkno);
				eprint(", offset = 0x%x.\n", (nints - i) * sizeof (int));
				goto bad;
			}
	}
	/*
	 * If we are supposed to write data out, do so.
	 */
	if (flags & SCAN_WRITE) {
		status = (*cur_ops->rdwr)(DIR_WRITE, cur_file, blkno,
		    blkcnt, (caddr_t)cur_buf, driver_flags);
		if (status)
			goto bad;
	}
	exit_critical();
	/*
	 * No errors occurred, return ok.
	 */
	return (0);
bad:
	/*
	 * There was an error.  If the data was corrupted, we write it
	 * out from the data buffer to restore it.
	 */
	if (corrupt) {
		if ((*cur_ops->rdwr)(DIR_WRITE, cur_file, blkno,
		    blkcnt, (caddr_t)cur_buf, F_NORMAL))
			eprint("Warning: unable to restore original data.\n");
	}
	exit_critical();
	/*
	 * Return the error.
	 */
	return (-1);
}


