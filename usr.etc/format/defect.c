#ifndef lint
static  char sccsid[] = "@(#)defect.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains routines that manipulate the defect list.
 */
#include "global.h"
#include <assert.h>
#include <sys/dkbad.h>
#include "param.h"



/*
 * This structure is the bad block table for the current disk if the disk
 * uses bad-144 defect mapping.
 */
struct	dkbad badmap;

/*
 * This routine reads the defect list off the disk.  It also reads in the
 * bad block table if the disk is a BAD144 type.  The defect list is
 * located on the first 2 tracks of the 2nd alternate cylinder of all disks.
 * The bad block map is located on the first 5 even sectors of the last
 * track of the last cylinder.
 */
read_list(list)
	struct	defect_list *list;
{
	int	size, head, sec, status;
	struct	bt_bad *bt;


	/* Panther's working list is maintained by the controller */
	if (cur_ctype->ctype_flags  & CF_WLIST) {
		
		if ((status = (*cur_ops->ex_cur)(list) == 0)) {
		    if (list->header.magicno != DEFECT_MAGIC) {
			printf("Defect list BAD\n");
		    }
		    printf("ISP-80 working list found\n");
		    return(0);
		}

		if ((status = (*cur_ops->ex_man)(list) == 0)) {
		    if (list->header.magicno != DEFECT_MAGIC) {
			printf("Defect list BAD\n");
		    }
		    printf("MANUFACTURER's list found\n");
		    return(0);
		}

		if (status == -1) {
		    printf("No defect list found\n");
		    return(-1);
		}
	}

	/*
	 * Loop for each copy of the defect list until we get a good one.
	 */
	for (head = 0; head < LISTCOUNT; head++) {
		/*
		 * Try to read the list header.
		 */
		if ((*cur_ops->rdwr)(DIR_READ, cur_file,
		    chs2bn(ncyl + 1, head, 0), 1,
		    (char *)&list->header), F_NORMAL)
			continue;
		/*
		 * If the magic number is wrong, this copy is corrupt.
		 */
		if (list->header.magicno != DEFECT_MAGIC)
			continue;
		/*
		 * Allocate space for the rest of the list.
		 */
		size = LISTSIZE(list->header.count);
		list->list = (struct defect_entry *)zalloc(size * SECSIZE);
		/*
		 * Try to read in the rest of the list. If there is an
		 * error, or the checksum is wrong, this copy is corrupt.
		 */
		if ((*cur_ops->rdwr)(DIR_READ, cur_file,
		    chs2bn(ncyl + 1, head, 1), size,
		    (char *)list->list, F_NORMAL) ||
		    checkdefsum(list, CK_CHECKSUM)) {
			/*
			 * Destroy the list and go on.
			 */
			kill_deflist(list);
			continue;
		}
		/*
		 * For SCSI, a valid defect list consists of the
		 * the grown list.
		 */
		if (EMBEDDED_SCSI) {
			list->flags |= LIST_PGLIST;
		}
		/*
		 * Got a good copy, stop searching.
		 */
		break;
	}
	/*
	 * Check the list for non-bfi defects (SMD and Adaptec disks only). 
	 * This is required because the logical sectors in 451 defect lists
	 * are off by the head skew. 
	 */ 
	(void) makebfi(list,(struct defect_entry *)NULL);	
	if (!(cur_ctlr->ctlr_flags & DKI_BAD144))
		return;
	/*
	 * The disk uses BAD144, read in the bad-block table.
	 */
	for (sec = 0; sec < BAD_LISTCNT * 2; sec += 2) {
		status = (*cur_ops->rdwr)(DIR_READ, cur_file,
		    chs2bn(ncyl + acyl - 1, nhead - 1, sec), 1,
		    &badmap, F_NORMAL);
		if (status)
			continue;
		/*
		 * Do a sanity check on the list read in.  If it passes,
		 * stop searching.
		 */
		if (badmap.bt_mbz != 0)
			continue;
		for (bt = badmap.bt_bad; bt - badmap.bt_bad < NDKBAD; bt++) {
			if (bt->bt_cyl < 0)
				break;
			if (bt->bt_trksec < 0)
				continue;
			head = bt->bt_trksec >> 8;
			if ((bt->bt_cyl >= pcyl) || (head >= nhead) ||
			    ((bt->bt_trksec & 0xff) >= sectors(head))) {
				status = -1;
				break;
			}
		}
		if (status)
			continue;
		return;
	}
	/*
	 * If we couldn't find the bad block table, initialize it to
	 * zero entries.
	 */
	for (bt = badmap.bt_bad; bt - badmap.bt_bad < NDKBAD; bt++)
		bt->bt_cyl = bt->bt_trksec = -1;
	badmap.bt_mbz = badmap.bt_csn = badmap.bt_flag = 0;
}

/*
 * Put all logical sector defects into Bytes From Index (bfi) format.
 * This is necessary 
 * because 450/451 formatted drives have all their logical sectors off
 * by the head skew but the 7053 does not. 
 * We are called by add_ldef() and d_add() when a defect is being added
 * to the list, or by read_list() when a defect list is read in. 
 */
int
makebfi(list,def)
	struct	defect_list *list;
	struct	defect_entry *def;
{
	struct	defect_entry *ptr, save;
	struct	defect_entry *end = list->list + list->header.count - 1;
	struct dk_info dkinfo;
	int prevdef, index, skew = 0;
	int i, savpos;
	int is_acb4000 = 0;

	/* 
	 * Only do this stuff for SMD and Adaptec controllers. 
	 * Note sector skew for * 450/451 controllers.
	 */
	if (ioctl(cur_file, DKIOCINFO, &dkinfo) >= 0) {
		switch (dkinfo.dki_ctype) {
			case DKC_XY450:
				skew = 1;
				break;
			case DKC_XD7053:
				break;
			case DKC_ACB4000:
				is_acb4000 = 1;
				break;
			default: 
				return(0);	
		}
	}
	if (def != NULL ) {
		/*
	 	 * Calculate the bfi for this defect. The caller will 
		 * sort it and add it to the list.
	 	 */
		if (is_acb4000) {
			if (acb_calc_bfi(def)) 
				return(0);
		}
		else {
			(void) calc_bfi(list, def, end, skew);
		}
		return(1);
	}
	/*
	 * if there is no defect list then return
	 */
	if (list->list ==  (struct defect_entry *) 0)
		return(0);
	/*
	 * Sort the defect list in place.
	 */
	for (ptr = list->list; ptr - list->list < list->header.count; ptr++) {
		if (ptr->bfi != UNKNOWN) 
			continue; 
		/* 
		 * Got a non-bfi defect. Look through the list up to this
		 * point for a previous defects on this track. If the FIRST
		 * defect on the track is closer to sector 0 than this one, 
		 * we have to add the number of bytes/sector when we calculate
		 * the bfi, as we are "ahead of" the slipped sector.
		 * If the defect is numerically less than the first defect on
		 * the track, no addition is necessary.
		 * 
		 * NOTE: all this assumes we only slip once per track and that
		 * logical defects always follow bfi defects in the defect list.
		 * If the list is all logical defects, we assume chronological
		 * ordering. Thus, sector 10 will be slipped if it is first in 
		 * the list, even if followed by sector 5.
		 */

		if (is_acb4000) {
			if (acb_calc_bfi(ptr))
				return(0);      /* something went wrong */
		}
		else {
			(void) calc_bfi(list, ptr, ptr, skew);
		}

	}
	/* 
	 * Sort the list. We take the expedient route and remove the defect from 
	 * the list and use add_def to put it back in. Not pretty, but it works.
	 */
	for (ptr = list->list; ptr - list->list < list->header.count; ptr++) {
		bcopy((char *)ptr, (char *)&save, sizeof(struct defect_entry));
		savpos = ptr - list->list;
		for (i = savpos; i < list->header.count; i++)
			*(list->list + i) = *(list->list + i + 1);
		--list->header.count;
		index = sort_defect(&save, list);
		add_def(&save, list, index);
	}
	/*
	 * Make sure to redo the checksum.
	 */
	(void) checkdefsum (list,CK_MAKESUM);
	return(1);
}

/*
 * Calculate the bytes from index for this defect. If the first defect
 * on this track (determined by its position in the defect list) was 
 * closer to logical sector 0, then we add the number of bytes/sector, 
 * since this one is "ahead" of the slipped sector. We mark the length 
 * of the defect in bits as being the length of a sector to differentiate 
 * it from manufacturers and bfi defects added by the user.
 */
calc_bfi(list, def, end, skew)
	struct	defect_list *list;
	struct	defect_entry *def;
	struct	defect_entry *end;
	int skew;

{
	struct	defect_entry *tmp;
	int bps = cur_dtype->dtype_bps;
	int a, b;

	def->bfi = bps * ((def->sect + def->head * skew) %  (nsect + 1));
	def->nbits = SECSIZE;

	if (list->list == (struct defect_entry *) 0)
		return;
	for( tmp = list->list; tmp < end; tmp++) { 
		if( tmp->cyl != def->cyl || tmp->head != def->head )
			continue;
		/*
		 * Found another defect on this track. See if it's closer
		 * numerically to logical sector 0. If it is, bump our defect's 
		 * bfi by the number of bytes per sector.
		 */
		a = tmp->bfi;
		b = def->bfi;
		if (tmp->bfi < tmp->head * bps)
			a = tmp->bfi + ((nsect + 1 - tmp->head) * bps);
		if (def->bfi < def->head * bps)
			b = def->bfi + ((nsect + 1 - def->head) * bps);
		if (a <= b) {
			def->bfi += bps;
			return;
		} else
			break;
	}
	return;
}
 
/* 
 * Add the logical sector information for each bfi entry.  This is
 * done to be compatible with the diag program
 * This has to be done anytime that the defect list can be written to
 * the disk or a file
 * A disk on a 450 without a spare and with a runt does not work properly.
 * This bug is left in because it would mean that the disk might have to
 * be formatted to determine what the track headers look like.  I didn't
 * do that because the code to figure out if the disk needs to be formatted 
 * to determine the header geometry is error prone (and destructive).
 */
int
makelsect(list)
	struct  defect_list *list;
{
	struct  defect_entry *ptr,*tmp;
	struct  dk_info dkinfo;
	struct  dk_type type;
	int	skew = 0;
	int	runt = 1;
	int	hsect = nsect;

	/*
	 * Only do for SMD disks which for now are 7053 & 451/450
	 */
	if (ioctl(cur_file, DKIOCINFO, &dkinfo) >= 0) {
	    switch (dkinfo.dki_ctype) {
		case DKC_XY450:
		    skew = 1;
		    if (!ioctl(cur_file, DKIOCGTYPE, &type))
			if ((hsect = type.dkt_hsect) == (nsect + 1))
				runt = 0;
		    break;
		case DKC_XD7053:
		    break;
		default:
		    return(0);
	    }
	}
	/*
	 * if there is no defect list then return
	 */
	if (list->list ==  (struct defect_entry *) 0)
		return(0);

	for (ptr = list->list; ptr - list->list < list->header.count; ptr++) {
		if (ptr->bfi == UNKNOWN)
			continue;
		ptr->sect = ptr->bfi/cur_dtype->dtype_bps - ptr->head * skew ;
		if (ptr->sect < 0)
			ptr->sect +=  runt ? (hsect - 1) : hsect;
		/*
		 * correct the logical sector for track that has more than
		 * one defect that are on seperate physical sectors
		 */
		if (ptr != list->list) { /* wait until a defect to compare to */
			tmp = &ptr[-1];
			if (ptr->cyl == tmp->cyl && ptr->head == tmp->head &&
			    ptr->sect != tmp->sect && tmp->bfi != ptr->bfi)
				ptr->sect -= 1;
		}
	}
	/*
	 * Make sure to redo the checksum.
	 */
	(void) checkdefsum (list,CK_MAKESUM);
	return(0);
}

/*
 * This routine either checks or calculates the checksum for a defect
 * list, depending on the mode parameter. In check mode, it returns
 * whether or not the checksum is correct.
 */
int
checkdefsum(list, mode)
	struct	defect_list *list;
	int	mode;
{
	register int *lp, i, sum = 0;

	/*
	 * Perform the rolling xor to get what the checksum should be.
	 */
	lp = (int *)list->list;
	for (i = 0; i < (list->header.count * sizeof (struct defect_entry) /
	    sizeof (int)); i++)
		sum ^= *(lp + i);
	/*
	 * If in check mode, return whether header checksum was correct.
	 */
	if (mode == CK_CHECKSUM)
		return (sum != list->header.cksum);
	/*
	 * If in create mode, set the header checksum.
	 */
	else {
		list->header.cksum = sum;
		return (0);
	}
}

/*
 * This routine prints a single defect to stdout in a readable format.
 */
pr_defect(def, num)
	struct	defect_entry *def;
	int	num;
{

	/*
	 * Make defect numbering look 1 relative.
	 */
	++num;
	/*
	 * Print out common values.
	 */
	printf("%4d%8d%7d", num, def->cyl, def->head);
	/*
	 * The rest of the values may be unknown. If they are, just
	 * print blanks instead.  Also, only print length only if bfi is
	 * known, and assume that a known bfi implies an unknown sect.
	 */
	if (def->bfi != UNKNOWN) {
		printf("%8d", def->bfi);
		if (def->nbits != UNKNOWN)
			printf("%8d", def->nbits);
	} else {
		printf("                ");
		printf("%8d", def->sect);
	}
	printf("\n");
}

/*
 * This routine calculates where in a defect list a given defect should
 * be sorted. It returns the index that the defect should become.  The
 * algorithm used sorts all bfi based defects by cylinder/head/bfi, and
 * adds all logical sector defects to the end of the list.  This is necessary
 * because the ordering of logical sector defects is significant when
 * sector slipping is employed.
 */
int
sort_defect(def, list)
	struct	defect_entry *def;
	struct	defect_list *list;
{
	struct	defect_entry *ptr;

	/*
	 * If it's a logical sector defect, return the entry at the end
	 * of the list.
	 */
	if (def->bfi == UNKNOWN)
		return (list->header.count);
	/*
	 * It's a bfi defect.  Loop through the defect list.
	 */
	for (ptr = list->list; ptr - list->list < list->header.count; ptr++) {
		/*
		 * If we get to a logical sector defect, put this defect
		 * right before it.
		 */
		if (ptr->bfi == UNKNOWN)
			goto found;
		/*
		 * If we get to a defect that is past this one in
		 * cylinder/head/bfi, put this defect right before it.
		 */
		if (def->cyl < ptr->cyl)
			goto found;
		if (def->cyl != ptr->cyl)
			continue;
		if (def->head < ptr->head)
			goto found;
		if (def->head != ptr->head)
			continue;
		if (def->bfi < ptr->bfi)
			goto found;
	}
found:
	/*
	 * Return the index to put the defect at.
	 */
	return (ptr - list->list);
}

/*
 * This routine writes the defect list on the back on the disk.  It also
 * writes the bad block table to disk if bad-144 mapping applies to the
 * current disk.
 */
write_deflist(list)
	struct	defect_list *list;
{
	int	size, status, head, sec;
	caddr_t	bad_ptr = (caddr_t)&badmap;


	/* Panther's working list is maintained by the controller */
	if (cur_ctype->ctype_flags & CF_WLIST) {
		if ((status = (*cur_ops->wr_cur)(list) == 0)) {
		    return(0);
		}
		return(-1);
	}

	/*
	 * If the list is null, there is nothing to write.
	 */
	if (list->list != NULL) {
		/*
		 * Make sure that the defect list is compatible with diag
		 */
		(void) makelsect(list);
								   
		/*
		 * calculate how many sectors the defect list will occupy.
		 */
		size = LISTSIZE(list->header.count);
		/*
		 * Loop for each copy of the list to be written.  Write out
		 * the header of the list followed by the data.
		 */
		for (head = 0; head < LISTCOUNT; head++) {
			status = (*cur_ops->rdwr)(DIR_WRITE, cur_file,
			    chs2bn(ncyl + 1, head, 0), 1,
			    (char *)&list->header, F_NORMAL);
			if (status) {
				eprint("Warning: error saving defect list.\n");
				continue;
			}
			status = (*cur_ops->rdwr)(DIR_WRITE, cur_file,
			    chs2bn(ncyl + 1, head, 1), size,
			    (char *)list->list, F_NORMAL);
			if (status)
				eprint("Warning: error saving defect list.\n");
		}
	}
	if (!(cur_ctlr->ctlr_flags & DKI_BAD144))
		return;

	assert(!EMBEDDED_SCSI);
	/*
	 * Current disk uses bad-144 mapping.  Loop for each copy of the
	 * bad block table to be written and write it out.
	 */
	for (sec = 0; sec < BAD_LISTCNT * 2; sec += 2) {
		status = (*cur_ops->rdwr)(DIR_WRITE, cur_file,
		    chs2bn(ncyl + acyl - 1, nhead - 1, sec), 1,
		    &badmap, F_NORMAL);
		if (status) {
			eprint("Warning: error saving bad block map table.\n");
			continue;
		}
	}
	/*
	 * Execute an ioctl to tell unix about the new bad block table.
	 */
	if (ioctl(cur_file, DKIOCSBAD, &bad_ptr))
		eprint("Warning: error telling SunOS bad block map table.\n");
}

/*
 * This routine adds a logical sector to the given defect list.
 */
add_ldef(blkno, list)
	daddr_t	blkno;
	struct	defect_list *list;
{
	struct	defect_entry def;
	int	index;

	/*
	 * Calculate the fields for the defect struct.
	 */
	def.cyl = bn2c(blkno);
	def.head = bn2h(blkno);
	def.sect = bn2s(blkno);
	/*
	 * Initialize the unknown fields.
	 */
	def.bfi = def.nbits = UNKNOWN;
	/*
	 * Make this a bfi defect.
	 */
	(void) makebfi(list,&def);
	/*
	 * Calculate the index into the list that the defect belongs at.
	 */
	index = sort_defect(&def, list);
	/*
	 * Add the defect to the list.
	 */
	add_def(&def, list, index);
}

/*
 * This routine adds the given defect struct to the defect list at
 * a precalculated index.
 */
add_def(def, list, index)
	struct	defect_entry *def;
	struct	defect_list *list;
	int	index;
{
	int	count, i;

	/*
	 * If adding this defect makes the list overflow into another
	 * sector, allocate the necessary space.
	 */
	count = list->header.count;
	if (LISTSIZE(count + 1) > LISTSIZE(count))
		list->list = (struct defect_entry *)rezalloc((char *)list->list,
		    LISTSIZE(count + 1) * SECSIZE);
	/*
	 * Slip all the defects after this one down one slot in the list.
	 */
	for (i = count; i > index; i--)
		*(list->list + i) = *(list->list + i - 1);
	/*
	 * Fill in the created hole with this defect.
	 */
	*(list->list + i) = *def;
	/*
	 * Increment the count and calculate a new checksum.
	 */
	list->header.count++;
	(void)checkdefsum(list, CK_MAKESUM);
}

/*
 * This routine sets the given defect list back to null.
 */
kill_deflist(list)
	struct	defect_list *list;
{

	/*
	 * If it's already null, we're done.
	 */
	if (list->list == NULL)
		return;
	/*
	 * Free the malloc'd space it's using.
	 */
	destroy_data((char *)list->list);
	/*
	 * Mark it as null.
	 */
	list->list = NULL;
	list->flags = 0;
}

