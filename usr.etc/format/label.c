
#ifndef lint
static	char sccsid[] = "@(#)label.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains the code relating to label manipulation.
 */
#include "global.h"

/*
 * This routine checks the given label to see if it is valid.
 */
int
checklabel(label)
	register struct dk_label *label;
{

	/*
	 * Check the magic number.
	 */
	if (label->dkl_magic != DKL_MAGIC)
		return (0);
	/*
	 * Check the checksum.
	 */
	if (checksum(label, CK_CHECKSUM) != 0)
		return (0);
	return (1);
}

/*
 * This routine checks or calculates the label checksum, depending on
 * the mode it is called in.
 */
int
checksum(label, mode)
	struct	dk_label *label;
	int	mode;
{
	register short *sp, sum = 0;
	register short count = (sizeof (struct dk_label)) / (sizeof (short));

	/*
	 * If we are generating a checksum, don't include the checksum
	 * in the rolling xor.
	 */
	if (mode == CK_MAKESUM)
		count -= 1;
	sp = (short *)label;
	/*
	 * Take the xor of all the half-words in the label.
	 */
	while (count--)
		sum ^= *sp++;
	/*
	 * If we are checking the checksum, the total will be zero for
	 * a correct checksum, so we can just return the sum.
	 */
	if (mode == CK_CHECKSUM)
		return (sum);
	/*
	 * If we are generating the checksum, fill it in.
	 */
	else {
		label->dkl_cksum = sum;
		return (0);
	}
}

/*
 * This routine is used to extract the id string from the string stored
 * in a disk label.  The problem is that the string in the label has
 * the physical characteristics of the drive appended to it.  The approach
 * is to find the beginning of the physical attributes portion of the string
 * and truncate it there.
 */
int
trim_id(id)
	char	*id;
{
	register char *c;

	/*
	 * Start at the end of the string.  When we match the word ' cyl',
	 * we are at the beginning of the attributes.
	 */
	for (c = id + strlen(id); c >= id; c--)
		if (strncmp(c, " cyl", strlen(" cyl")) == 0)
			break;
	/*
	 * If we ran off the beginning of the string, something is wrong.
	 */
	if (c < id)
		return (-1);
	/*
	 * Truncate the string.
	 */
	*c = '\0';
	return (0);
}

/*
 * This routine constructs and writes a label on the disk.  It writes both
 * the primary and backup labels.  It assumes that there is a current
 * partition map already defined.  It also notifies the SunOS kernel of
 * the label and partition information it has written on the disk.
 */
int
write_label()
{
	int	error = 0, head, sec, i;
	struct	dk_label label;
	struct	dk_label new_label;

	/*
	 * Fill in a label structure with the geometry information.
	 */
	bzero((char *)&label, sizeof (struct dk_label));
	(void) sprintf(label.dkl_asciilabel, "%s cyl %d alt %d hd %d sec %d",
	     cur_dtype->dtype_asciilabel, ncyl, acyl, nhead, nsect);
	label.dkl_pcyl = pcyl;
	label.dkl_ncyl = ncyl;
	label.dkl_acyl = acyl;
	label.dkl_nhead = nhead;
	label.dkl_nsect = nsect;
	label.dkl_apc = apc;
	label.dkl_intrlv = 1;
	label.dkl_rpm = cur_dtype->dtype_rpm;
	/*
	 * Also fill in the current partition information.
	 */
	for (i = 0; i < NDKMAP; i++)
		label.dkl_map[i] = cur_parts->pinfo_map[i];
	label.dkl_magic = DKL_MAGIC;
	/*
	 * Generate the correct checksum.
	 */
	(void) checksum(&label, CK_MAKESUM);
	/*
	 * Lock out interrupts so we don't write half of the copies of the
	 * label.
	 */
	enter_critical();
	/*
	 * Write and verify the primary label.
	 */
	if ((*cur_ops->rdwr)(DIR_WRITE, cur_file, 0, 1,
	      (caddr_t)&label, F_NORMAL)  ||
	    (*cur_ops->rdwr)(DIR_READ, cur_file, 0, 1,
             (caddr_t)&new_label, F_NORMAL)) {
		eprint("Warning: error writing primary label.\n");
		error = -1;
	}

	/*
	 * Calcualate where the backup labels go.  They are always on
	 * the last alternate cylinder, but some older drives put them
	 * on head 2 instead of the last head.  They are always on the
	 * first 5 odd sectors of the appropriate track.
	 */
	if (cur_ctype->ctype_flags & CF_BLABEL)
		head  = 2;
	else
		head = nhead - 1;
	/*
	 * Write and verify the backup labels.
	 */
	for (sec = 1; sec < BAD_LISTCNT * 2 + 1; sec += 2) {
		if ((*cur_ops->rdwr)(DIR_WRITE, cur_file,
		      chs2bn(ncyl + acyl - 1, head, sec), 1,
		      (caddr_t)&label, F_NORMAL)  ||
		    (*cur_ops->rdwr)(DIR_READ, cur_file,
		     chs2bn(ncyl + acyl - 1, head, sec), 1,
               	     (caddr_t)&new_label, F_NORMAL)) {
			eprint("Warning: error writing backup label.\n");
			error = -1;
		}
	}
	/*
	 * Mark the current disk as labelled and notify the kernel of what
	 * has happened.
	 */
	cur_disk->disk_flags |= DSK_LABEL;
	if (notify_unix())
		error = -1;
	exit_critical();
	return (error);
}

