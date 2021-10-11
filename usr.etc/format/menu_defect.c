
#ifndef lint
static  char sccsid[] = "@(#)menu_defect.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains functions to implement the defect menu commands.
 */
#include "global.h"
#include <assert.h>
#include "misc.h"
#include "param.h"

/*
 * This is the working defect list.  All the commands here operate on
 * the working list, except for 'commit'.  This way the user can
 * change his mind at any time without having mangled the current defect
 * list.
 */
struct	defect_list work_list;

/*
 * This routine implements the 'restore' command.  It sets the working
 * list equal to the current list.
 */
int
d_restore()
{
	int	i;

	/*
	 * If embedded scsi, this command is not necessary.
	 */
	if (EMBEDDED_SCSI) {
		eprint("Restore command does not apply to embedded SCSI controllers\n");
		return (0);
	}

	/*
	 * If the working list has not been modified, there's nothing to do.
	 */
	if (!(work_list.flags & LIST_DIRTY)) {
		eprint("working list was not modified.\n");
		return (0);
	}
	/*
	 * Make sure the user is serious.
	 */
	if (check("Ready to update working list, continue"))
		return (-1);
	/*
	 * Lock out interrupts so the lists can't get mangled.
	 */
	enter_critical();
	/*
	 * Kill off the old working list.
	 */
	kill_deflist(&work_list);
	/*
	 * If the current isn't null, set the working list to be a
	 * copy of it.
	 */
	if (cur_list.list != NULL) {
		work_list.header = cur_list.header;
		work_list.list = (struct defect_entry *)zalloc(
		    LISTSIZE(work_list.header.count) * SECSIZE);
		for (i = 0; i < work_list.header.count; i++)
			*(work_list.list + i) = *(cur_list.list + i);
	}
	/*
	 * Initialize the flags since they are now in sync.
	 */
	work_list.flags = 0;
	if (work_list.list == NULL)
		printf("working list set to null.\n\n");
	else
		printf("working list updated, total of %d defects.\n\n",
		    work_list.header.count);
	exit_critical();
	return (0);
}

/*
 * This routine implements the 'original' command.  It extracts the
 * manufacturer's defect list from the current disk.
 */
int
d_original()
{
	int	status;

	/*
	 * If the controller does not support the extraction, we're out
	 * of luck.
	 */
	if (cur_ops->ex_man == NULL) {
		eprint("Controller does not support extracting ");
		eprint("manufacturer's defect list.\n");
		return (-1);
	}
	/*
	 * Make sure the user is serious.  Note, for SCSI disks
	 * since this is instantaneous, we will just do it and
	 * not ask for confirmation.
	 */
	if (! (cur_ctype->ctype_flags & (CF_SCSI|CF_IPI))  &&
	    check("Ready to update working list. This cannot be interrupted \nand may take a long while. Continue"))
		return (-1);
	/*
	 * Lock out interrupts so we don't get half the list.
	 */
	enter_critical();
	/*
	 * Kill off the working list.
	 */
	kill_deflist(&work_list);
	printf("Extracting manufacturer's defect list...");
	/*
	 * Do the extraction.
	 */
	status = (*cur_ops->ex_man)(&work_list);
	if (status)
		printf("Extraction failed.\n\n");
	else {
		printf("Extraction complete.\n");
		printf("Working list updated, total of %d defects.\n\n",
		    work_list.header.count);
	}
	/*
	 * Mark the working list dirty since we modified it.
	 */
	work_list.flags |= LIST_DIRTY;
	exit_critical();
	/*
	 * Return status.
	 */
	return (status);
}

/*
 * This routine implements the 'extract' command.  It extracts the
 * entire defect list from the current disk.
 */
int
d_extract()
{
	int	status;

	/*
	 * If the controller does not support the extraction, we are out
	 * of luck.
	 */
	if (cur_ops->ex_cur == NULL) {
		eprint("Controller does not support extracting ");
		eprint("current defect list.\n");
		return (-1);
	}

	/*
	 * If disk is unformatted, you really shouldn't do this.
	 * However, ask user to be sure.
	 */
	if (! (cur_flags & DISK_FORMATTED)  &&
	    (check("Cannot extract defect list from an unformatted disk. Continue")))
		return (-1);

	/*
	 * If this takes a long time, let's ask the user if he
	 * doesn't mind waiting.  Note, for SCSI disks
	 * this operation is instantaneous so we won't ask for
	 * for confirmation.
	 */
	if (! (cur_ctype->ctype_flags & (CF_SCSI|CF_IPI))  &&
	    check("Ready to extract working list. This cannot be interrupted \nand may take a long while. Continue"))
		return (-1);
	/*
	 * Lock out interrupts so we don't get half the list and
	 * Kill off the working list.
	 */
	enter_critical();
	kill_deflist(&work_list);
	printf("Extracting defect list...");

	/*
	 * Do the extraction.
	 */
	status = (*cur_ops->ex_cur)(&work_list);
	if (status) {
		if (cur_flags & DISK_FORMATTED)
			read_list(&work_list);

		if (work_list.list != NULL) {
			status = 0;
			printf("Extraction complete.\n");
			printf("Working list updated, total of %d defects.\n\n",
			    	work_list.header.count);
		} else {
			printf("Extraction failed.\n\n");
		}
	} else {
		printf("Extraction complete.\n");
		printf("Working list updated, total of %d defects.\n\n",
		    work_list.header.count);
	}
	/*
	 * Mark the working list dirty since we modified it.
	 */
	work_list.flags |= LIST_DIRTY;
	exit_critical();
	/*
	 * Return status.
	 */
	return (status);
}

/*
 * This routine implements the 'add' command.  It allows the user to
 * enter the working defect list manually.  It loops infinitely until the user
 * breaks out with a ctrl-C.
 */
int
d_add()
{
	int	type, bn, deflt, index;
	struct	bounds bounds;
	struct	defect_entry def;

	/*
	 * If embedded scsi, this command is not necessary.
	 */
	if (EMBEDDED_SCSI) {
		eprint("Add command does not apply to embedded SCSI controllers\n");
		eprint("Use the 'repair' command to repair grown defects, instead.\n");
		return (0);
	}

	/*
	 * Ask the user which mode of input he'd like to use.
	 */
	printf("        0. bytes-from-index\n");
	printf("        1. logical block\n");
	deflt = 0;
	bounds.lower = 0;
	bounds.upper = 1;
	type = input(FIO_INT, "Select input format (enter its number)", ':',
	    (char *)&bounds, &deflt, DATA_INPUT);
	printf("\nEnter Control-C to terminate.\n");
loop:
	if (type) {
		/*
		 * Mode selected is logical block.  Input the defective block
		 * and fill in the defect entry with the info.
		 */
		def.bfi = def.nbits = UNKNOWN;
		bounds.lower = 0;
		bounds.upper = physsects() - 1;
		bn = input(FIO_BN, "Enter defective block number", ':',
		    (char *)&bounds, (int *)NULL, DATA_INPUT);
		def.cyl = bn2c(bn);
		def.head = bn2h(bn);
		def.sect = bn2s(bn);
	} else {
		/*
		 * Mode selected is bytes-from-index.  Input the information
		 * about the defect and fill in the defect entry.
		 */
		def.sect = UNKNOWN;
		bounds.lower = 0;
		bounds.upper = pcyl - 1;
		def.cyl = (short)input(FIO_INT,
		    "Enter defect's cylinder number", ':',
		    (char *)&bounds, (int *)NULL, DATA_INPUT);
		bounds.upper = nhead - 1;
		def.head = (short)input(FIO_INT, "Enter defect's head number",
		    ':', (char *)&bounds, (int *)NULL, DATA_INPUT);
		bounds.upper = bpt - 1;
		def.bfi = input(FIO_INT, "Enter defect's bytes-from-index",
		    ':', (char *)&bounds, (int *)NULL, DATA_INPUT);
		bounds.lower = -1;
		bounds.upper = (bpt - def.bfi) * 8;
		if (bounds.upper >= 32 * 1024)
			bounds.upper = 32 * 1024 - 1;
		/*
		 * Note: a length of -1 means the length is not known.  We
		 * make this the default value.
		 */
		deflt = -1;
		def.nbits = input(FIO_INT, "Enter defect's length (in bits)",
		    ':', (char *)&bounds, &deflt, DATA_INPUT);
	}
	if (def.bfi == UNKNOWN) {
		/*
	 	 * Call makebfi() to put this defect into bfi format. This is
		 * currently only used on SMD disks and adaptec SCSI disks; it 
		 * is a no-op for everything else.
	  	 */
		(void) makebfi(&work_list, &def); 
	}
	/*
	 * Calculate where in the defect list this defect belongs
	 * and print it out.
	 */
	index = sort_defect(&def, &work_list);
	printf(DEF_PRINTHEADER);
	pr_defect(&def, index);

	/*
	 * Lock out interrupts so lists don't get mangled.
	 * Also, mark the working list dirty since we are modifying it.
	 */
	enter_critical();
	work_list.flags |= LIST_DIRTY;
	/*
	 * If the list is null, create it with zero length.  This is
	 * necessary because the routines to add a defect to the list
	 * assume the list is initialized.
	 */
	if (work_list.list == NULL) {
		work_list.header.magicno = DEFECT_MAGIC;
		work_list.header.count = 0;
		work_list.list = (struct defect_entry *)zalloc(
		    LISTSIZE(0) * SECSIZE);
	}
	/*
	 * Add the defect to the working list.
	 */
	add_def(&def, &work_list, index);
	printf("defect number %d added.\n\n", index + 1);
	exit_critical();
	/*
	 * Loop back for the next defect.
	 */
	goto loop;
}

/*
 * This routine implements the 'delete' command.  It allows the user
 * to manually delete a defect from the working list.
 */
int
d_delete()
{
	int	i, count, num;
	struct	bounds bounds;

	/*
	 * If embedded scsi, this command is not implemented.
	 */
	if (EMBEDDED_SCSI) {
		eprint("Delete command does not apply to embedded SCSI controllers\n");
		return (0);
	}

	/*
	 * If the working list is null or zero length, there's nothing
	 * to delete.
	 */
	count = work_list.header.count;
	if (work_list.list == NULL || count == 0) {
		eprint("No defects to delete.\n");
		return (-1);
	}
	/*
	 * Ask the user which defect should be deleted. Bounds are off by
	 * one because user sees a one-relative list.
	 */
	bounds.lower = 1;
	bounds.upper = count;
	num = input(FIO_INT, "Specify defect to be deleted (enter its number)",
	    ':', (char *)&bounds, (int *)NULL, DATA_INPUT);
	/* 
	 * The user thinks it's one relative but it's not really.
	 */
	--num;
	/*
	 * Print the defect selected and ask the user for confirmation.
	 */
	printf(DEF_PRINTHEADER);
	pr_defect(work_list.list + num, num);
	/*
	 * Lock out interrupts so the lists don't get mangled.
	 */
	enter_critical();
	/*
	 * Move down all the defects beyond the one deleted so the defect
	 * list is still fully populated.
	 */
	for (i = num; i < count - 1; i++)
		*(work_list.list + i) = *(work_list.list + i + 1);
	/*
	 * If the size of the list in sectors has changed, reallocate
	 * the list to shrink it appropriately.
	 */
	if (LISTSIZE(count - 1) < LISTSIZE(count))
		work_list.list = (struct defect_entry *)rezalloc(
		    (char *)work_list.list, LISTSIZE(count - 1) * SECSIZE);
	/*
	 * Decrement the defect count.
	 */
	work_list.header.count--;
	/*
	 * Recalculate the list's checksum.
	 */
	(void) checkdefsum(&work_list, CK_MAKESUM);
	/*
	 * Mark the working list dirty since we modified it.
	 */
	work_list.flags |= LIST_DIRTY;
	printf("defect number %d deleted.\n\n", ++num);
	exit_critical();
	return (0);
}

/*
 * This routine implements the 'print' command.  It prints the working
 * defect list out in human-readable format.
 */
int
d_print()
{
	int	i, nomore = 0;
	int	c, one_line = 0;

	/*
	 * If the working list is null, there's nothing to print.
	 */
	if (work_list.list == NULL) {
		if (EMBEDDED_SCSI) {
			printf("total of 0 defects\n\n");
			return (0);
		} else {
			eprint("No working list defined.\n");
			return (-1);
		}
	}
	/*
	 * If we're running from a file, don't use the paging scheme.
	 * If we are running interactive, turn off echoing.
	 */
	if (option_f)
		nomore++;
	else {
		enter_critical();
		echo_off();
		charmode_on();
		exit_critical();
	}
	/* Print out the banner. */
	if (work_list.header.count != 0)
		printf(DEF_PRINTHEADER);

	/*
	 * Loop through the each defect in the working list.
	 */
	for (i = 0; i < work_list.header.count; i++) {
		/*
		 * If we are paging and hit the end of a page, wait for
		 * the user to hit either space-bar, "q", or return
		 * before going on.
		 */
		if (one_line  ||
		    (!nomore  &&  ((i + 1) % (TTY_LINES - 1) == 0))) {
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
		/*
		 * Print the defect.
		 */
		pr_defect(work_list.list + i, i);
	}
	printf("total of %d defects.\n\n", i);
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
 * This routine implements the 'dump' command.  It writes the working
 * defect list to a file.
 */
int
d_dump()
{
	int	i, status = 0;
	char	*str;
	FILE	*fptr;
	struct	defect_entry *dptr;

	/*
	 * If the working list is null, there's nothing to do.
	 */
	if (work_list.list == NULL) {
		eprint("No working list defined.\n");
		return (-1);
	}
	/*
	 * make sure that the list is compatible with diag
	 */
	(void) makelsect(&work_list);

	/*
	 * Ask the user for the name of the defect file.  Note that the
	 * input will be in malloc'd space since we are inputting
	 * type OSTR.
	 */
	str = (char *)input(FIO_OSTR, "Enter name of defect file", ':',
	    (char *)NULL, (int *)NULL, DATA_INPUT);
	/*
	 * Lock out interrupts so the file doesn't get half written.
	 */
	enter_critical();
	/*
	 * Open the file for writing.
	 */
	if ((fptr = fopen(str, "w+")) == NULL) {
		eprint("unable to open defect file.\n");
		status = -1;
		goto out;
	}
	/*
	 * Print a header containing the magic number, count, and checksum.
	 */
	(void) fprintf(fptr, "0x%08x%8d  0x%08x\n",
		work_list.header.magicno,
	    	work_list.header.count, work_list.header.cksum);
	/*
	 * Loop through each defect in the working list.  Write the
	 * defect info to the defect file.
	 */
	for (i = 0; i < work_list.header.count; i++) {
		dptr = work_list.list + i;
		(void) fprintf(fptr, "%4d%8d%7d%8d%8d%8d\n",
			i+1, dptr->cyl, dptr->head,
		    	dptr->bfi, dptr->nbits, dptr->sect);
	}
	printf("defect file updated, total of %d defects.\n", i);
	/*
	 * Close the defect file.
	 */
	(void) fclose(fptr);
out:
	/*
	 * Destroy the string used for the file name.
	 */
	destroy_data(str);
	exit_critical();
	printf("\n");
	return (status);
}

/*
 * This routine implements the 'load' command.  It reads the working
 * list in from a file.
 */
int
d_load()
{
	int	i, items, status = 0, count, cksum;
	u_int	magicno;
	char	*str;
	TOKEN	filename;
	FILE	*fptr;
	struct	defect_entry *dptr;

	/*
	 * Ask the user for the name of the defect file.  Note that the
	 * input will be malloc'd space since we inputted type OSTR.
	 */
	str = (char *)input(FIO_OSTR, "Enter name of defect file", ':',
	    (char *)NULL, (int *)NULL, DATA_INPUT);
	/*
	 * Copy the file name into local space then destroy the string
	 * it came in.  This is simply a precaution against later having
	 * to remember to destroy this space.
	 */
	enter_critical();
	(void) strcpy(filename, str);
	destroy_data(str);
	exit_critical();
	/*
	 * See if the defect file is accessable.  If not, we can't load
	 * from it.  We do this here just so we can get out before asking
	 * the user for confirmation.
	 */
	status = access(filename, 4);
	if (status) {
		eprint("defect file not accessable.\n");
		return (-1);
	}
	/*
	 * Make sure the user is serious.
	 */
	if (check("ready to update working list, continue"))
		return (-1);
	/*
	 * Lock out interrupts so the list doesn't get half loaded.
	 */
	enter_critical();
	/*
	 * Open the defect file.
	 */
	if ((fptr = fopen(filename, "r")) == NULL) {
		eprint("unable to open defect file.\n");
		exit_critical();
		return (-1);
	}
	/*
	 * Scan in the header.
	 */
	items = fscanf(fptr, "0x%x%d  0x%x\n", &magicno, &count, &cksum);
	/*
	 * If the header is wrong, this isn't a good defect file.
	 */
	if (items != 3 || count < 0 || 
	    (magicno != DEFECT_MAGIC && magicno != NO_CHECKSUM)){ 
		eprint("Defect file is corrupted.\n");
		status = -1;
		goto out;
	}
	/*
	 * Kill off any old defects in the working list.
	 */
	kill_deflist(&work_list);
	/*
	 * Load the working list header with the header info.
	 */
	if (magicno == NO_CHECKSUM)
		work_list.header.magicno = DEFECT_MAGIC;
	else
		work_list.header.magicno = magicno;
	work_list.header.count = count;
	work_list.header.cksum = cksum;
	/*
	 * Allocate space for the new list.
	 */
	work_list.list = (struct defect_entry *)zalloc(LISTSIZE(count) *
	    SECSIZE);
	/*
	 * Mark the working list dirty since we are modifying it.
	 */
	work_list.flags |= LIST_DIRTY;
	/*
	 * If SCSI, treat it as a grown list, although the data may have
	 * come from the manufacturer's list.  We have no way of
	 * knowing, unfortunately.
	 */
	if (EMBEDDED_SCSI) {
		work_list.flags |= LIST_PGLIST;
	}
	/*
	 * Loop through each defect in the defect file.
	 */
	for (i = 0; i < count; i++) {
		dptr = work_list.list + i;
		/*
		 * Scan the info into the defect entry.
	 	 */
		items = fscanf(fptr, "%*d%hd%hd%d%hd%hd\n", &dptr->cyl,
		    &dptr->head, &dptr->bfi, &dptr->nbits, &dptr->sect);
		/*
		 * If it didn't scan right, give up.
		 */
		if (items != 5)
			goto bad;
	}
	/*
	 * Check to be sure the checksum from the defect file was correct
	 * unless there wasn't supposed to be a checksum.
	 * If there was supposed to be a valid checksum and there isn't
	 * then give up.
	 */
	if (magicno != NO_CHECKSUM && checkdefsum(&work_list, CK_CHECKSUM))
		goto bad;
	printf("working list updated, total of %d defects.\n", i);
	goto out;

bad:
	/*
	 * Some kind of error occurred.  Kill off the working list and
	 * mark the status bad.
	 */
	eprint("Defect file is corrupted, working list set to NULL.\n");
	kill_deflist(&work_list);
	status = -1;
out:
	/*
	 * Close the defect file.
	 */
	(void) fclose(fptr);
	exit_critical();
	/*
	 * Check the list for non-bfi defects and recast them into 
	 * bfi format (SMD disks and adaptec SCSI disks only).
	 */
	(void) makebfi(&work_list, (struct defect_entry *)NULL);
	/*
	 * Return status.
	 */
	printf("\n");
	return (status);
}

/*
 * This routine implements the 'commit' command.  It causes the current
 * defect list to be set equal to the working defect list.  It is the only
 * way that changes made to the working list can actually take effect in
 * the next format.
 */
int
d_commit()
{
	/*
	 * If the working list wasn't modified, no commit is necessary.
	 */
	if (work_list.list != NULL && !(work_list.flags & LIST_DIRTY)) {
		eprint("working list was not modified.\n");
		return (0);
	}

	/*
	 * Make sure the user is serious.
	 */
	if (check("Ready to update Current Defect List, continue"))
		return (-1);
	return (do_commit());
}

int
do_commit()
{
	int	i;

	/*
	 * Lock out interrupts so the list doesn't get half copied.
	 */
	enter_critical();
	/*
	 * Kill off any current defect list.
	 */
	kill_deflist(&cur_list);
	/*
	 * If the working list is null, initialize it to zero length.
	 * This is so the user can do a commit on a null list and get
	 * a zero length list.  Otherwise there would be no way to get
	 * a zero length list conveniently.
	 */
	if (work_list.list == NULL) {
		work_list.header.magicno = DEFECT_MAGIC;
		work_list.header.count = 0;
		work_list.list = (struct defect_entry *)zalloc(
		    LISTSIZE(0) * SECSIZE);
	}
	/*
	 * Copy the working list into the current list.
	 */
	cur_list.header = work_list.header;
	cur_list.list = (struct defect_entry *)zalloc(
	    LISTSIZE(cur_list.header.count) * SECSIZE);
	for (i = 0; i < cur_list.header.count; i++)
		*(cur_list.list + i) = *(work_list.list + i);
	/*
	 * Mark the working list clean, since it is now the same as the
	 * current list.  Note we do not mark the current list dirty,
	 * even though it has been changed.  This is because it does
	 * not reflect the state of disk, so we don't want it written
	 * out until a format has been done.  The format will mark it
	 * dirty and write it out.
	 */
	work_list.flags &= ~(LIST_DIRTY|LIST_RELOAD);
	cur_list.flags = work_list.flags;
	printf("Current Defect List updated, total of %d defects.\n",
	    cur_list.header.count);
	/*
	 * Remind the user to format the drive, since changing the list
	 * does nothing unless a format is performed.
	 */
	printf("Disk must be reformatted for changes to take effect.\n\n");
	exit_critical();
	return (0);
}
/*
 * This routine implements the 'create' command.  It creates the
 * manufacturer's defect on the current disk from the defect list
 */
int
d_create()
{
	int     status;

	/*
	 * If the controller does not support the extraction, we're out
	 * of luck.
	 */
	if (cur_ops->create == NULL) {
		eprint("Controller does not support creating ");
		eprint("manufacturer's defect list.\n");
		return (-1);
	}
	/*
	 * Make sure the user is serious.  Note, for SCSI disks
	 * since this is instantaneous, we will just do it and
	 * not ask for confirmation.
	 */
	if (! (cur_ctype->ctype_flags & CF_SCSI)  &&
  	    check("Ready to create the manufacturers defect information on the disk. \nThis cannot be interrupted and may take a long while \nIT WILL DESTROY ALL OF THE DATA ON THE DISK!. Continue"))
		return (-1);
	/*
	 * Lock out interrupts so we don't get half the list.
	 */
	enter_critical();
	printf("Creating manufacturer's defect list...");
	/*
	 * Do the Creation
	 */
	status = (*cur_ops->create)(&work_list);
	if (status)
		printf("Creation failed.\n\n");
	else       
		printf("Creation complete.\n");
	exit_critical();
	/*
	 * Return status.
	 */
	return (status);
}
