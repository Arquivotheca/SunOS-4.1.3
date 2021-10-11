
#ifndef lint
static  char sccsid[] = "@(#)menu_command.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains functions that implement the command menu commands.
 */
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>
#include "global.h"
#include "analyze.h"
#include "menu.h"
#include "param.h"
#include "defect.h"


extern	struct menu_item menu_partition[];
extern	struct menu_item menu_analyze[];
extern	struct menu_item menu_defect[];
extern	long time();

/*
 * This routine implements the 'disk' command.  It allows the user to
 * select a disk to be current.  The list of choices is the list of
 * disks that were found at startup time.
 */
int
c_disk()
{
	register struct ctlr_info *ctlr;
	register struct disk_info *disk;
	struct	bounds bounds;
	int	deflt, *defltptr = NULL, index, i;

	printf("\n\nAVAILABLE DISK SELECTIONS:\n");
	/*
	 * Loop through the list of found ctlrs.
	 */
	for (i = 0, ctlr = ctlr_list; ctlr != NULL; ctlr = ctlr->ctlr_next) {
#ifdef		not
		/*
		 * Print out the ctlr line.
		 */
		pr_ctlrline(ctlr);
#endif		not

		/*
		 * Loop through the list of found disks.
		 */
		for (disk = disk_list; disk != NULL; disk = disk->disk_next) {
			/*
			 * If the disk is on this ctlr, print out the disk line.
			 */
			if (disk->disk_ctlr == ctlr) {
				/*
				 * If this is the current disk, mark it as
				 * the default.
				 */
				if (cur_disk == disk) {
					deflt = i;
					defltptr = &deflt;
				}
				pr_diskline(disk, i++);
			}
		}
	}
	/*
	 * Ask the user which disk he would like to make current.
	 */
	bounds.lower = 0;
	bounds.upper = i - 1;
	index = input(FIO_INT, "Specify disk (enter its number)", ':',
	    (char *)&bounds, defltptr, DATA_INPUT);
	/*
	 * Find the disk he picked.
	 */
	i = 0;
	for (ctlr = ctlr_list; ctlr != NULL; ctlr = ctlr->ctlr_next) {
		for (disk = disk_list; disk != NULL; disk = disk->disk_next) {
			if (disk->disk_ctlr == ctlr) {
				if (i == index)
					goto found;
				i++;
			}
		}
	}
found:
	/*
	 * Update the state.  We lock out interrupts so the state can't
	 * get half-updated.
	 */
	enter_critical();
	init_globals(disk);
	exit_critical();
	/*
	 * If type unknown and interactive, ask user to specify type.
	 * Also, set partition table (best guess) too.
	 */
	if (!option_f  &&  ncyl == 0  &&  nhead == 0  &&  nsect == 0)
		(void) c_type();
	return (0);
}

/*
 * This routine implements the 'type' command.  It allows the user to
 * specify the type of the current disk.  It should be necessary only
 * if the disk was not labelled or was somehow labelled incorrectly.
 * The list of legal types for the disk comes from information that was
 * in the data file.
 */
int
c_type()
{
	register struct disk_type *type, *tptr, *oldtype;
	struct	bounds bounds;
	int	i, index, deflt, *defltptr = NULL;
	char	*asciilabel;
	u_short	ncyl, acyl, pcyl, nhead, nsect, rpm;
	int	bps, dr_type, buf_sk, wr_prcmp;

	/*
	 * There must be a current disk.
	 */
	if (cur_disk == NULL) {
		eprint("Current Disk is not set.\n");
		return (-1);
	}
	oldtype = cur_disk->disk_type;
	type = cur_ctype->ctype_dlist;
	/*
	 * Print out the list of choices.
	 */
	printf("\n\nAVAILABLE DRIVE TYPES:\n");
	for (i = 0, tptr = type; tptr != NULL; tptr = tptr->dtype_next) {
		/*
		 * If we pass the current type, mark it to be the default.
		 */
		if (cur_dtype == tptr) {
			deflt = i;
			defltptr = &deflt;
		}
		printf("        %d. %s\n", i++, tptr->dtype_asciilabel);
	}
	printf("        %d. other\n", i);
	bounds.lower = 0;
	bounds.upper = i;
	/*
	 * Ask the user which type the disk is.
	 */
	index = input(FIO_INT, "Specify disk type (enter its number)", ':',
	    (char *)&bounds, defltptr, DATA_INPUT);
	/*
	 * Find the type he chose.
	 */
	for (i = 0, tptr = type; i < index; i++, tptr = tptr->dtype_next)
		;
	/*
	 * If the user selected 'other', he is going to give us a new
	 * disk type.
	 */
	if (tptr == NULL) {
		/*
		 * Get the standard information on the new type.
		 */
		bounds.lower = 1;
		bounds.upper = MAX_CYLS;
		ncyl = (u_short)input(FIO_INT, "Enter number of data cylinders",
		    ':', (char *)&bounds, (int *)NULL, DATA_INPUT);
		bounds.lower = deflt = 2;
		bounds.upper = MAX_CYLS - ncyl;
		acyl = (u_short)input(FIO_INT,
		    "Enter number of alternate cylinders", ':',
		    (char *)&bounds, &deflt, DATA_INPUT);
		bounds.lower = deflt = ncyl + acyl;
		bounds.upper = MAX_CYLS;
		pcyl = (u_short)input(FIO_INT,
		    "Enter number of physical cylinders", ':',
		    (char *)&bounds, &deflt, DATA_INPUT);
		bounds.lower = 1;
		bounds.upper = MAX_HEADS;
		nhead = (u_short)input(FIO_INT, "Enter number of heads", ':',
		    (char *)&bounds, (int *)NULL, DATA_INPUT);
		bounds.lower = 1;
		bounds.upper = MAX_SECTS;
		nsect = (u_short)input(FIO_INT,
		    "Enter number of data sectors/track", ':',
		    (char *)&bounds, (int *)NULL, DATA_INPUT);
		bounds.lower = MIN_RPM;
		bounds.upper = MAX_RPM;
		deflt = AVG_RPM;
		rpm = (u_short)input(FIO_INT, "Enter rpm of drive", ':',
		    (char *)&bounds, &deflt, DATA_INPUT);
		/*
		 * Get any ctlr specific info we need.
		 */
		if (cur_ctype->ctype_flags & CF_SMD_DEFS) {
			bounds.lower = MIN_BPS;
			bounds.upper = MAX_BPS;
			bps = input(FIO_INT, "Enter total bytes/sector",
			    ':', (char *)&bounds, (int *)NULL, DATA_INPUT);
		}
		if (cur_ctype->ctype_flags & CF_450_TYPES) {
			bounds.lower = 0;
			bounds.upper = 3;
			dr_type = input(FIO_INT, "Enter drive type",
			    ':', (char *)&bounds, (int *)NULL, DATA_INPUT);
		}
		if (cur_ctype->ctype_flags & CF_BSK_WPR) {
			bounds.lower = 0;
			bounds.upper = 100;
			deflt = 2;
			buf_sk = input(FIO_INT, "Enter buffer skew", ':',
			    (char *)&bounds, &deflt, DATA_INPUT);
			bounds.upper = deflt = pcyl;
			wr_prcmp = input(FIO_INT,
			    "Enter write precomp cylinder", ':',
			    (char *)&bounds, &deflt, DATA_INPUT);
		}
		/*
		 * Get the name of the new type.
		 */
		asciilabel = (char *)input(FIO_OSTR,
		    "Enter disk type name (remember quotes)", ':',
		    (char *)NULL, (int *)NULL, DATA_INPUT);
		/*
		 * Add the new type to the list of possible types for
		 * this controller.  We lock out interrupts so the lists
		 * can't get munged.  We put off actually allocating the
		 * structure till here in case the user wanted to
		 * interrupt while still inputting information.
		 */
		enter_critical();
		tptr = (struct disk_type *)zalloc(sizeof *tptr);
		if (type == NULL)
			cur_ctype->ctype_dlist = tptr;
		else {
			while (type->dtype_next != NULL)
				type = type->dtype_next;
			type->dtype_next = tptr;
		}
		tptr->dtype_next = NULL;
		tptr->dtype_asciilabel = asciilabel;
		tptr->dtype_pcyl = pcyl;
		tptr->dtype_ncyl = ncyl;
		tptr->dtype_acyl = acyl;
		tptr->dtype_nhead = nhead;
		tptr->dtype_nsect = nsect;
		tptr->dtype_rpm = rpm;
		tptr->dtype_bpt = INFINITY;
		if (cur_ctype->ctype_flags & CF_SMD_DEFS)
			tptr->dtype_bps = bps;
		if (cur_ctype->ctype_flags & CF_450_TYPES)
			tptr->dtype_dr_type = dr_type;
		if (cur_ctype->ctype_flags & CF_BSK_WPR) {
			tptr->dtype_buf_sk = buf_sk;
			tptr->dtype_wr_prcmp = wr_prcmp;
		}
	}
	/*
	 * Check for mounted file systems in the format zone.  One potential
	 * problem with this would be that check() always returns 'yes'
	 * when running out of a file.  However, it is actually ok
	 * because we don't let the program get started if there are
	 * mounted file systems and we are running from a file.
	 */
	if ((tptr != oldtype)  &&
	    checkmount(cur_ctlr->ctlr_dname, cur_disk->disk_unit,
	               (daddr_t)-1, (daddr_t)-1)) {
		eprint("Cannot set disk type while it has mounted partitions.\n\n");
		return (-1);
	}
	/*
	 * If the type selected is different from the previous type,
	 * mark the disk as not labelled and reload the current
	 * partition info.  This is not essential but probably the
	 * right thing to do, since the size of the disk has probably
	 * changed.
	 */
	enter_critical();
	if (tptr != oldtype) {
		cur_disk->disk_type = tptr;
		cur_disk->disk_parts = NULL;
		cur_disk->disk_flags &= ~DSK_LABEL;
	}
	/*
	 * Initialize the state of the current disk.
	 */
	init_globals(cur_disk);
	(void) get_partition();
	exit_critical();
	return (0);
}

/*
 * This routine implements the 'partition' command.  It simply runs
 * the partition menu.
 */
int
c_partition()
{

	/*
	 * There must be a current disk type (and therefore a current disk).
	 */
	if (cur_dtype == NULL) {
		eprint("Current Disk Type is not set.\n");
		return (-1);
	}
	cur_menu++;
	last_menu = cur_menu;

#ifdef	not
	/*
	 * If there is no current partition table, make one.  This is
	 * so the commands within the menu never have to check for
	 * a non-existent table.
	 */
	if (cur_parts == NULL)
		eprint("making partition.\n");
		make_partition();
#endif	not

	/*
	 * Run the menu.
	 */
	run_menu(menu_partition, "PARTITION", "partition", 0);
	cur_menu--;
	return (0);
}

/*
 * This routine implements the 'current' command.  It describes the
 * current disk.
 */
int
c_current()
{

	/*
	 * If there is no current disk, say so.  Note that this is
	 * not an error since it is a legitimate response to the inquiry.
	 */
	if (cur_disk == NULL) {
		printf("No Current Disk.\n");
		return (0);
	}
	/*
	 * Print out the info we have on the current disk.
	 */
	if (cur_ctlr->ctlr_ctype->ctype_ctype != DKC_PANTHER) {
		printf("Current Disk = %s%d: <", cur_ctlr->ctlr_dname,
		    cur_disk->disk_unit);
	} else {
		printf("Current Disk = %s%03x: <", cur_ctlr->ctlr_dname,
		    cur_disk->disk_unit);
	}
	if (cur_dtype == NULL)
		printf("type unknown");
	else
		printf("%s cyl %d alt %d hd %d sec %d",
		    cur_dtype->dtype_asciilabel, ncyl, acyl, nhead, nsect);
	printf(">\n\n");
	return (0);
}

/*
 * This routine implements the 'format' command.  It allows the user
 * to format and verify any portion of the disk.
 */
int
c_format()
{
	daddr_t	start, end;
	long	clock;
	int	format_time, format_tracks, format_cyls;
	u_int	format_interval, format_sleep;
	int	i, deflt, status, pid;
	union	wait pid_status;
	struct	bounds bounds;

	/*
	 * There must be a current disk type (and therefore a current disk).
	 */
	if (cur_dtype == NULL) {
		eprint("Current Disk Type is not set.\n");
		return (-1);
	}
	/*
	 * There must be a current defect list.  Except for
	 * unformatted SCSI disks.  For them the defect list
	 * can only be retrieved after formatting the disk.
	 */
	if (((cur_ctype->ctype_flags & CF_SCSI)  &&
	    (cur_ctype->ctype_flags & CF_DEFECTS)  &&
	    ! (cur_flags & DISK_FORMATTED)) || EMBEDDED_SCSI) {
		cur_list.flags |= LIST_RELOAD;
	} else if (cur_list.list == NULL) {
		eprint("Current Defect List must be initialized.\n");
		return (-1);
	}
	/*
	 * Ask for the bounds of the format.  We always use the whole
	 * disk as the default, since that is the most likely case.
	 * Note, for disks which must be formatted accross the whole disk,
	 * don't bother the user.
	 */
	bounds.lower = start = 0;
	if (cur_ctype->ctype_flags & CF_SCSI)
		bounds.upper = end = datasects() - 1;
	else if (cur_ctype->ctype_flags & CF_IPI)
		bounds.upper = end = physsects() - 1;
	else
		bounds.upper = end = physsects() - 1;

	if (! (cur_ctlr->ctlr_flags & DKI_FMTVOL)) {
		deflt = bounds.lower;
		start = (daddr_t)input(FIO_BN, "Enter starting block number", ':',
		    (char *)&bounds, &deflt, DATA_INPUT);
		bounds.lower = start;
		deflt = bounds.upper;
		end = (daddr_t)input(FIO_BN, "Enter ending block number", ':',
		    (char *)&bounds, &deflt, DATA_INPUT);
	}
	/*
	 * Some disks can format tracks.  Make sure the whole track is
	 * specified for them.
	 */
	if (cur_ctlr->ctlr_flags & DKI_FMTTRK) {
		if (bn2s(start) != 0 || bn2s(end) != sectors(bn2h(end)) - 1) {
			eprint("Controller requires formatting of ");
			eprint("entire tracks.\n");
			return (-1);
		}
	}
	/*
	 * Check for mounted file systems in the format zone, and if we
	 * find any, make sure they are really serious.  One potential
	 * problem with this would be that check() always returns 'yes'
	 * when running out of a file.  However, it is actually ok
	 * because we don't let the program get started if there are
	 * mounted file systems and we are running from a file.
	 */
	if (checkmount(cur_ctlr->ctlr_dname, cur_disk->disk_unit, start, end)) {
		eprint("Cannot format disk while it has mounted partitions.\n\n");
		return (-1);
	}
	/*
	 * Compute estimated formatting time and make sure they are serious.
	 * Formatting time is (2 * time of 1 spin * number of tracks) +
	 * (step rate * number of cylinders) rounded up to the nearest
	 * minute.  Note, a 10% fudge factor is thrown in for insurance. 
	 */
	if (cur_dtype->dtype_fmt_time == 0)
		cur_dtype->dtype_fmt_time = 2;

	format_tracks = ((end - start) / cur_dtype->dtype_nsect) + 1;
	format_cyls = format_tracks / cur_dtype->dtype_nhead;
	format_tracks = format_tracks * cur_dtype->dtype_fmt_time;

	format_time = ((60000 / cur_dtype->dtype_rpm) +1) * format_tracks +
		format_cyls * 7; /* ms. */
	format_interval = format_time / 5000; /* 20% done tick (sec) */
	format_time = (format_time + 59999) / 60000; /* min. */
	format_sleep = 10;

	/*
	 * Check format time values and make adjustments
	 * to prevent sleeping too long (forever?) or
	 * too short.
	 */
	if (format_time <= 1)
		/* Format time is less than 1 minute. */
		format_time = 1;		/* failsafe */

	if ((int)format_interval < 11) {
		/* Format time is less than 1 minute. */
		if ((int)format_interval < 2)
			format_interval = 2;	/* failsafe */
		format_sleep = (int)format_interval * 6;
		format_interval = 10;
	} else {
		/* Format time is greater than 1 minute. */
		format_interval -= 10;
	}

	printf("Ready to format. Formatting cannot be interrupted \nand takes %d minutes (estimated). ", format_time) ;
	if (check("Continue")) 
		return (-1);
	/*
	 * Print the time so that the user will know when format started. 
	 * Lock out interrupts.  This could be a problem, since it could
	 * cause the user to sit for quite awhile with no control, but we
	 * don't have any other good way of keeping his gun from going off.
	 */
	clock = time((long *)0);
	printf("Beginning format. The current time is %s\n",ctime(&clock));
	enter_critical();
	/*
	 * Mark the defect list dirty so it will be rewritten when we are
	 * done.  It is possible to qualify this so it doesn't always
	 * get rewritten, but it's not worth the trouble.
	 */
	cur_list.flags |= LIST_DIRTY;
	/*
	 * If we are formatting over any of the lables, mark the label
	 * dirty so it will be rewritten.
	 */
	if (start < totalsects() && end >= datasects()) {
		if (cur_disk->disk_flags & DSK_LABEL)
			cur_flags |= LABEL_DIRTY;
	}
	if (start == 0) {
		cur_flags |= LABEL_DIRTY;
	}
	/*
	 * Do the format.
	 */
	printf("Formatting...");
	(void) fflush(stdout);

#ifdef BUG1009138
	pid = fork();
	if (pid <= 0) {
		status = (*cur_ops->format)(start, end, &cur_list);
		if (pid == 0)
			exit(status & 0xff);
	} else {
		status = 1;
		if (option_msg) {
			for (i = 1; i <= format_time; i = i +1) {
				if (i == format_time) {
					(void) wait(&pid_status);
					status = pid_status.w_retcode & 0xff;
					break;
				}
				sleep(10);
				if (wait3(&pid_status, WNOHANG,
				    (struct rusage *)0))  {
					status = pid_status.w_retcode & 0xff;
					break;
				}
				sleep(50);
				printf("%d minutes\nFormatting...", i);
			}
		} else {
			for (i = 20; i <= 100; i = i +20) {
				if (i == 100) {
					(void) wait(&pid_status);
					status = pid_status.w_retcode & 0xff;
					break;
				}
				sleep(format_sleep);
				if (wait3(&pid_status, WNOHANG,
				    (struct rusage *)0))  {
					status = pid_status.w_retcode & 0xff;
					break;
				}
				sleep(format_interval);
				printf("%d%s done\nFormatting...", i, "%");
			}
		}
	}
#endif BUG1009138

	status = (*cur_ops->format)(start, end, &cur_list);
	if (status) {
		exit_critical();
		eprint("failed\n");
		return (-1);
	}
	printf("done\n");
	cur_flags |= DISK_FORMATTED;
	/*
	 * If the defect list or label is dirty, write them out again.
	 * Note, for SCSI we have to wait til now to load defect list
	 * since we can't access it until after formatting a virgin disk.
	 */
	/* enter_critical(); */
	if (cur_list.flags & LIST_RELOAD) {
		/*
		 * For embedded scsi, extract the P&G lists.
		 */
		if (EMBEDDED_SCSI) {
			status = (*cur_ops->ex_cur)(&cur_list);
			status = (*cur_ops->ex_cur)(&work_list);
		} else {
			status = (*cur_ops->ex_man)(&cur_list);
		}
		if (status) {
			eprint("Warning: unable to reload defect list\n");
			cur_list.flags &= LIST_DIRTY;
			return (-1);
		}
		cur_list.flags |= LIST_DIRTY;
		cur_list.flags &= ~LIST_RELOAD;
	}

	if (cur_list.flags & LIST_DIRTY) {
		write_deflist(&cur_list);
		cur_list.flags = 0;
	}
	if (cur_flags & LABEL_DIRTY) {
		(void) write_label();
		cur_flags &= ~LABEL_DIRTY;
	}
	/*
	 * Come up for air, since the verify step does not need to
	 * be atomic (it does it's own lockouts when necessary).
	 */
	exit_critical();
	/*
	 * If we are supposed to verify, we do the 'write' test over
	 * the format zone.  The rest of the analysis parameters are
	 * left the way they were.
	 */
	if (scan_auto) {
		scan_entire = 0;
		scan_lower = start;
		scan_upper = end;
		printf("\nVerifying media...");
		status = do_scan(SCAN_PATTERN,F_SILENT);
	}
	/*
	 * If the defect list or label is dirty, write them out again.
	 */
	if (cur_list.flags & LIST_DIRTY) {
		cur_list.flags = 0;
		write_deflist(&cur_list);
	}
	if (cur_flags & LABEL_DIRTY) {
		cur_flags &= ~LABEL_DIRTY;
		(void) write_label();
	}
	return (status);
}

/*
 * This routine implements the 'repair' command.  It allows the user
 * to reallocate sectors on the disk that have gone bad.
 */
int
c_repair()
{
	daddr_t	bn;
	int	status;
	struct	bounds bounds;

	/*
	 * There must be a current disk type (and therefore a current disk).
	 */
	if (cur_dtype == NULL) {
		eprint("Current Disk Type is not set.\n");
		return (-1);
	}
	/*
	 * The current disk must be formatted for repair to work.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}
	/*
	 * Repair is an optional command for controllers, so it may
	 * not be supported.
	 */
	if (cur_ops->repair == NULL) {
		eprint("Controller does not support repairing.\n");
		return (-1);
	}
	/*
	 * There must be a defect list since we will add to it.
	 */
	if (cur_list.list == NULL) {
		eprint("Current Defect List must be initialized.\n");
		return (-1);
	}
	/*
	 * For SCSI, we require that the working defect list
	 * contain the grown defects, to maintain a consistent
	 * sun defect list.
	 */
	if (EMBEDDED_SCSI) {
		if ((cur_list.flags & LIST_PGLIST) == 0) {
			eprint("\
Please use the 'extract' command and commit a current defect\n\
list, before repairing any defects\n");
			return(-1);
		}
	}
	/*
	 * Ask the user which sector has gone bad.
	 */
	bounds.lower = 0;
	bounds.upper = physsects() - 1;
	bn = (daddr_t)input(FIO_BN, "Enter block number of defect", ':',
	    (char *)&bounds, (int *)NULL, DATA_INPUT);
	/*
	 * Check to see if there is a mounted file system over the
	 * specified sector.  If there is, make sure the user is
	 * really serious.
	 */
	if (checkmount(cur_ctlr->ctlr_dname, cur_disk->disk_unit, bn, bn)) {
		if (check("Repair is in a mounted partition, continue"))
			return (-1);
	} else if (check("Ready to repair defect, continue")) {
		return (-1);
	}

	/*
	 * Lock out interrupts so the disk can't get out of sync with
	 * the defect list.
	 */
	enter_critical();
	printf("Repairing block %d  (", bn);
	pr_dblock(printf, bn);
	printf(")...");
	/*
	 * Do the repair.
	 */
	status = (*cur_ops->repair)(bn,F_NORMAL);
	if (status)
		eprint("failed.\n\n");
	else {
		/*
		 * It succeeded.  Add the bad sector to the defect list,
		 * write out the defect list, and kill off the working
		 * list so it will get synced up with the current
		 * defect list next time we need it.
		 */
		printf("done\n\n");
		if (cur_ctype->ctype_flags & CF_WLIST) {
			kill_deflist(&cur_list);
			(*cur_ops->ex_cur)(&cur_list);
			printf("Current list updated\n");
		} else {
			add_ldef(bn, &cur_list);
			write_deflist(&cur_list);
		}
		kill_deflist(&work_list);
	}
	exit_critical();
	/*
	 * Return status.
	 */
	return (status);
}

/*
 * This routine implements the 'show' command.  It translates a disk
 * block given in any format into decimal, hexadecimal, and
 * cylinder/head/sector format.
 */
int
c_show()
{
	struct	bounds bounds;
	daddr_t	bn;

	/*
	 * There must be a current disk type, so we will know the geometry.
	 */
	if (cur_dtype == NULL) {
		eprint("Current Disk Type is not set.\n");
		return (-1);
	}
	/*
	 * The MD21 SCSI controller does not support a command to
	 * translate a logical block to a physical location on the
	 * drive, i.e. cylinder/head/sector.
	 */
	if (cur_ctype->ctype_ctype == DKC_MD21 ||
			cur_ctype->ctype_ctype == DKC_SCSI_CCS) {
		printf("SCSI controllers do not support the 'show' command.\n");
		return (0);
	}

	/*
	 * Ask the user for a disk block.
	 */
	bounds.lower = 0;
	bounds.upper = physsects() - 1;
	bn = (daddr_t)input(FIO_BN, "Enter a disk block", ':',
	    (char *)&bounds, (int *)NULL, DATA_INPUT);
	/*
	 * Echo it back.
	 */
	printf("Disk block = %d = 0x%x = (", bn, bn);
	pr_dblock(printf, bn);
	printf(")\n\n");
	return (0);
}

/*
 * This routine implements the 'label' command.  It writes the
 * primary and backup labels onto the current disk.
 */
int
c_label()
{
	int status;

	/*
	 * There must be a current disk type (and therefore a current disk).
	 */
	if (cur_dtype == NULL) {
		eprint("Current Disk Type is not set.\n");
		return (-1);
	}
	/*
	 * The current disk must be formatted to label it.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}
	/*
	 * Check to see if there are any mounted file systems anywhere
	 * on the current disk.  If so, refuse to label the disk, but
	 * only if the partitions would change for the mounted partitions.
	 *
	 */
	if (checkmount(cur_ctlr->ctlr_dname, cur_disk->disk_unit,
	    (daddr_t)-1, (daddr_t)-1)) {
		/* Bleagh, too descriptive */
		if(check_label_with_mount(cur_ctlr->ctlr_dname,
				cur_disk->disk_unit)) {
			eprint("Cannot label disk while it has mounted partitions.\n\n");
			return (-1);
		}
	}
	/*
	 * If there is not a current partition map, warn the user we
	 * are going to use the default.  The default is the first
	 * partition map we encountered in the data file.  If there is
	 * no default we give up.
	 */
	if (cur_parts == NULL) {
		printf("Current Partition Table is not set, using default.\n");
		cur_disk->disk_parts = cur_parts = cur_dtype->dtype_plist;
		if (cur_parts == NULL) {
			eprint("No default available, cannot label.\n");
			return (-1);
		}
	}
	/*
	 * Make sure the user is serious.
	 */
	if (check("Ready to label disk, continue"))
		return (-1);
	/*
	 * Write the labels out (this will also notify unix) and
	 * return status.
	 */
	printf("\n");
	if (status = write_label())
		eprint("Label failed.\n");
	return (status);
}

/*
 * This routine implements the 'analyze' command.  It simply runs
 * the analyze menu.
 */
int
c_analyze()
{

	/*
	 * There must be a current disk type (and therefor a current disk).
	 */
	if (cur_dtype == NULL) {
		eprint("Current Disk Type is not set.\n");
		return (-1);
	}
	cur_menu++;
	last_menu = cur_menu;

	/*
	 * Run the menu.
	 */
	run_menu(menu_analyze, "ANALYZE", "analyze", 0);
	cur_menu--;
	return (0);
}

/*
 * This routine implements the 'defect' command.  It simply runs
 * the defect menu.
 */
int
c_defect()
{
	int	i;

	/*
	 * There must be a current disk type (and therefor a current disk).
	 */
	if (cur_dtype == NULL) {
		eprint("Current Disk Type is not set.\n");
		return (-1);
	}
	cur_menu++;
	last_menu = cur_menu;

	/*
	 * Lock out interrupt while we manipulate the defect lists.
	 */
	enter_critical();
	/*
	 * If the working list is null but there is a current list,
	 * update the working list to be a copy of the current list.
	 */
	if ((work_list.list == NULL) && (cur_list.list != NULL)) {
		work_list.header = cur_list.header;
		work_list.list = (struct defect_entry *)zalloc(
		    LISTSIZE(work_list.header.count) * SECSIZE);
		for (i = 0; i < work_list.header.count; i++)
			*(work_list.list + i) = *(cur_list.list + i);
		work_list.flags = 0;
	}
	exit_critical();
	/*
	 * Run the menu.
	 */
	run_menu(menu_defect, "DEFECT", "defect", 0);
	cur_menu--;

	/*
	 * If the user has modified the working list but not committed
	 * it, warn him that he is probably making a mistake.
	 */
	if (work_list.flags & LIST_DIRTY) {
		eprint("Warning: working defect list modified; but not committed.\n");
		if ( !check("Do you wish to commit changes to current defect list"))
			(void) do_commit();
	}
	return (0);
}

/*
 * This routine implements the 'backup' command.  It allows the user
 * to search for backup labels on the current disk.  This is useful
 * if the primary label was lost and the user wishes to recover the
 * partition information for the disk. The disk is relabeled and
 * the current defect list is written out if a backup label is found.
 */
int
c_backup()
{
	struct	dk_label label;
	struct	disk_type *dtype;
	struct	partition_info *parts, *plist;
	daddr_t	bn;
	int	sec, head, i;

	/*
	 * There must be a current disk type (and therefore a current disk).
	 */
	if (cur_dtype == NULL) {
		eprint("Current Disk Type is not set.\n");
		return (-1);
	}
	/*
	 * The disk must be formatted to read backup labels.
	 */
	if (!(cur_flags & DISK_FORMATTED)) {
		eprint("Current Disk is unformatted.\n");
		return (-1);
	}
	/*
	 * If we found a primary label on this disk, make sure
	 * the user is serious.
	 */
	if ((cur_disk->disk_flags & DSK_LABEL) &&
	    (check("Disk had a primary label, still continue")))
		return (-1);
	printf("Searching for backup labels...");
	(void) fflush(stdout);
	/*
	 * Some disks have the backup labels in a strange place.
	 */
	if (cur_ctype->ctype_flags & CF_BLABEL)
		head = 2;
	else
		head = nhead - 1;
	/*
	 * Loop through each copy of the backup label.
	 */
	for (sec = 1; sec < BAD_LISTCNT * 2 + 1; sec += 2) {
		bn = chs2bn(ncyl + acyl - 1, head, sec);
		/*
		 * Attempt to read it.
		 */
		if ((*cur_ops->rdwr)(DIR_READ, cur_file, (daddr_t)bn, 1,
		    (char *)&label, F_NORMAL))
			continue;
		/*
		 * Verify that it is a reasonable label.
		 */
		if (!checklabel(&label))
			continue;
		if (trim_id(label.dkl_asciilabel))
			continue;
		/*
		 * Lock out interrupts while we manipulate lists.
		 */
		enter_critical();
		printf("found.\n");
		/*
		 * Find out which disk type the backup label claims.
		 */
		for (dtype = cur_ctype->ctype_dlist; dtype != NULL;
		    dtype = dtype->dtype_next)
			if (dtype_match(&label, dtype))
				break;
		/*
		 * If it disagrees with our current type, something
		 * real bad is happening.
		 */
		if (dtype != cur_dtype) {
			if (dtype == NULL) {
				eprint("Backup label claims different type.\n");
				exit_critical();
				return (-1);
			}
			printf("Backup label claims different type.\n");
			if (check("Continue")) {
				exit_critical();
				return (-1);
			}
			cur_dtype = dtype;
		}
		/*
		 * Try to match the partition map with a known map.
		 */
		for (parts = dtype->dtype_plist; parts != NULL;
		    parts = parts->pinfo_next)
			if (parts_match(&label, parts))
				break;
		/*
		 * If we couldn't match it, allocate space for a new one,
		 * fill in the info, and add it to the list.  The name
		 * for the new map is derived from the disk name.
		 */
		if (parts == NULL) {
			parts = (struct partition_info *)zalloc(sizeof *parts);
			plist = dtype->dtype_plist;
			if (plist == NULL)
				dtype->dtype_plist = parts;
			else {
				while (plist->pinfo_next != NULL)
					plist = plist->pinfo_next;
				plist->pinfo_next = parts;
			}
			i = strlen("original ") + DK_DEVLEN;
			parts->pinfo_name = zalloc(i);
			(void) sprintf(parts->pinfo_name, "%s%s%d", "original ",
			    cur_ctlr->ctlr_dname, cur_disk->disk_unit);
			for (i = 0; i < NDKMAP; i++)
				parts->pinfo_map[i] = label.dkl_map[i];
		}
		/*
		 * We now have a partition map.  Make it the current map.
		 */
		cur_disk->disk_parts = cur_parts = parts;
		exit_critical();
		/*
		 * Rewrite the labels and defect lists.
		 */
		printf("Restoring primary label and defect list.\n");
		if (write_label())
			return (-1);
		if (cur_list.list != NULL)
			write_deflist(&cur_list);
		printf("\n");
		return (0);
	}
	/*
	 * If we didn't find any backup labels, say so.
	 */
	printf("not found.\n\n");
	return (0);
}

