
#ifndef lint
static  char sccsid[] = "@(#)ctlr_xy450.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains the ctlr dependent routines for the xy450 ctlr.
 */
#include "global.h"
#include <sys/dkbad.h>
#include <sundev/xycom.h>
#include <sundev/xyreg.h>
#include <sundev/xyerr.h>
#include "analyze.h"

extern	int xy_rdwr(), xy_ck_format();
extern	int xy_format(), xy_ex_man(), xy_ex_cur();
extern	int xy_repair();
extern	u_int xy_match();

/*
 * This the largest value that can be passed to the controller
 * during formatting as the sector count for the command.
 */
#define	XY_MAX_FORMAT		(64 * 1024 - 1)

/*
 * These variables tell whether headers for the last disk checked
 * are in physical or pseudo-index format, and whether or not the disk
 * is set-up for slip sectoring.
 */
static int	physical_form = 1, spare_sector = 0;

/*
 * List of commands for the 451.  Used to print nice error messages.
 */
char *xycmdnames[] = {
	"nop",
	"write",
	"read",
	"write headers",
	"read headers",
	"seek",
	"drive reset",
	"format",
	"read all",
	"read status",
	"write all",
	"set drive size",
	"self test",
	"reserved",
	"buffer load",
	"buffer dump",
};

/*
 * This is the operation vector for the Xylogics 451.  It is referenced
 * in init_ctypes.c where the supported controllers are defined.
 */
struct	ctlr_ops xy450ops = {
	xy_rdwr,
	xy_ck_format,
	xy_format,
	xy_ex_man,
	xy_ex_cur,
	xy_repair,
	0,
};

/*
 * The following routines are the controller specific routines accessed
 * through the controller ops vector.
 */
/*
 * This routine is used to read/write the disk.  The flags determine
 * what mode the operation should be executed in (silent, diagnostic, etc).
 */
int
xy_rdwr(dir, file, blkno, secnt, bufaddr, flags)
	int	dir, file, secnt, flags;
	daddr_t	blkno;
	caddr_t	bufaddr;
{
	struct	dk_cmd cmdblk;

	/*
	 * Fill in a command block with the command info.
	 */
	cmdblk.dkc_blkno = blkno;
	cmdblk.dkc_secnt = secnt;
	cmdblk.dkc_bufaddr = bufaddr;
	cmdblk.dkc_buflen = secnt * SECSIZE;
	if (dir == DIR_READ)
		cmdblk.dkc_cmd = XY_READ;
	else
		cmdblk.dkc_cmd = XY_WRITE;
	/*
	 * Run the command and return status.
	 */
	return (xycmd(file, &cmdblk, flags));
}

/*
 * This routine is used to check whether the current disk is formatted.
 * The approach used is to attempt to read the 4 sectors from an area 
 * on the disk.  We can't just read one, cause that will succeed on a 
 * new drive if the defect info is just right. We are guaranteed that 
 * the first track is defect free, so we should never have problems 
 * because of that. We also check all over the disk now to make very
 * sure of whether it is formatted or not.
 */
int
xy_ck_format()
{

	struct	dk_cmd cmdblk;
	int	status, i;

	for( i=0; i<= ncyl; i += ncyl/4) {
		/*
		 * Fill in a command block with the command info. We run the
		 * command in silent mode because the error message is not
		 * important, just the success/failure.
		 */
		cmdblk.dkc_cmd = XY_READ;
		cmdblk.dkc_blkno = (daddr_t)i;
		cmdblk.dkc_secnt = 4;
		cmdblk.dkc_bufaddr = cur_buf;
		cmdblk.dkc_buflen = SECSIZE * 4;
		status = xycmd(cur_file, &cmdblk, F_SILENT);
		/*
		 * If any area checked returns no error, the disk is at
		 * least partially formatted. 
		 */
		if (!status)
			return (1);
	}
	/*
	 * OK, it really is unformatted...
	 */
	return (0);
}

/*
 * This routine formats a portion of the current disk.  It also repairs
 * the defects on the given list.
 */
int
xy_format(start, end, list)
	daddr_t	start, end;
	struct	defect_list *list;
{
	struct	dk_cmd cmdblk;
	struct	dk_type type;
	struct	xyhdr *hp;
	struct	defect_entry *def;
	int	i, cyl, head, sect, status, hdr_cnt,j;
	daddr_t	bn, blkno;
	int	error = 0, index, bfi, span, len, bps;
	daddr_t	curnt;
	int	size;
	u_int	*ptr = (u_int *)pattern_buf;

	/*
	 * Do an ioctl to get back the number of hard sectors on the disk.
	 * Note that on a 451 this includes any runt sector.
	 */
	status = ioctl(cur_file, DKIOCGTYPE, &type);
	if (status) {
		eprint("Unable to read drive configuration.\n");
		return (-1);
	}
	hdr_cnt = type.dkt_hsect;
	if (hdr_cnt < nsect) {
		eprint("Hard sector count less than nsect.\n");
		return (-1);
	}
	/*
	 * Remove any mappings for sectors that we are about to format.
	 * We mark these entries with a cylinder of 0 and trksec of -1,
	 * since invalidating them with a cylinder of -1 would mess up
	 * old unix drivers.
	 */
	for (i = 0; i < NDKBAD; i++) {
		/*
		 * If we hit the end of the list, quit searching.
		 */
		if ((cyl = badmap.bt_bad[i].bt_cyl) < 0)
			break;
		/*
		 * If we hit a pseudo-invalid entry, skip it.
		 */
		if (badmap.bt_bad[i].bt_trksec < 0)
			continue;
		head = badmap.bt_bad[i].bt_trksec >> 8;
		sect = badmap.bt_bad[i].bt_trksec & 0xff;
		bn = chs2bn(cyl, head, sect);
		/*
		 * If entry falls within format zone, pseudo-invalidate it.
		 */
		if (bn >= start && bn <= end) {
			badmap.bt_bad[i].bt_cyl = 0;
			badmap.bt_bad[i].bt_trksec = -1;
		}
	}
	/*
	 * Loop through the format zone.
	 */
	for (curnt = start; curnt <= end; curnt += size) {
		/*
		 * Calculate the size of the next section.  It is either
		 * the maximum allowable size or how much is left.
		 */
		if ((end - curnt) < (XY_MAX_FORMAT / nsect * nsect))
			size = end - curnt + 1;
		else
			size = XY_MAX_FORMAT / nsect * nsect;
		/*
		 * Fill in the command block and execute the format.
		 */
		cmdblk.dkc_cmd = XY_FORMAT;
		cmdblk.dkc_blkno = curnt;
		cmdblk.dkc_secnt = size;
		cmdblk.dkc_bufaddr = NULL;
		cmdblk.dkc_buflen = 0;
		status = xycmd(cur_file, &cmdblk, F_NORMAL);
		if (status)
			return (-1);
	}
	/*
	 * Check to see if there is a spare sector and/or sector skewing.
	 */
	if (xy_check_skew(hdr_cnt, pattern_buf)) {
		eprint("Unable to read drive configuration.\n");
		return (-1);
	}
	/*
	 * This  is a hack to handle out of spec eagles.  Sometimes the
	 * header for sector 0 on an eagle will get trashed when it
	 * is formatted.  We check for this bad header and replace it
	 * if necessary.
	 * Loop through the format zone a track at a time.
	 */
	for (bn = start; bn < end; bn += nsect) {
		/*
		 * Read in the headers for this track.
		 */
		status = xy_readhdr(bn, hdr_cnt, pattern_buf);
		if (status) {
			eprint("Unable to read track headers.\n");
			return (-1);
		}
		hp = (struct xyhdr *)pattern_buf;
		/*
		 * If the header looks ok, continue.
		 */
		if (XY_GET_SEC(hp) == 0 && XY_GET_CYL(hp) == bn2c(bn))
			continue;
		/*
		 * Rebuild the header to be correct and write it out.
		 */
		bzero((char *)hp, sizeof (struct xyhdr));
		hp->xyh_sec_hi = XY_SEC_HI(0);
		hp->xyh_sec_lo = XY_SEC_LO(0);
		hp->xyh_cyl_hi = XY_CYL_HI(bn2c(bn));
		hp->xyh_cyl_lo = XY_CYL_LO(bn2c(bn));
		hp->xyh_head = (u_char)bn2h(bn);
		hp->xyh_type = cur_dtype->dtype_dr_type;
		status = xy_writehdr(bn, hdr_cnt, pattern_buf);
		if (status) {
			eprint("Unable to write track headers.\n");
			return (-1);
		}
	}
	bps = cur_dtype->dtype_bps;
	/*
	 * Loop through the defects in the list passed in.
	 */
	for (i = 0; i < list->header.count; i++) {
		def = &list->list[i];
		cyl = def->cyl;
		head = def->head;
		bn = chs2bn(cyl, head, 0);
		/*
		 * If the defect is not in the format zone, continue.
		 */
		if (bn < start || bn > end)
			continue;
		bfi = def->bfi;
		/*
		 * Calculate the length in bytes.
		 */
		if (def->nbits == UNKNOWN)
			len = 1;
		else
			len = (def->nbits + 7) / 8;
		/*
		 * Loop for each sector that is affected by this defect.
		 */
		do {
			span = 0;
			/*
			 * Read in the headers for the appropriate track.
			 */
			status = xy_readhdr(bn, hdr_cnt, pattern_buf);
			if (status) {
				eprint("Unable to locate defect #%d.\n", i);
				error = -1;
				continue;
			}
			/*
			 * If bfi is unknown, use logical sector value to
			 * calculate index into headers.
			 */
			if (bfi == UNKNOWN) {
				sect = def->sect;
				index = xy_ls2index(sect, (u_int *)pattern_buf);
			/*
			 * Else use bfi to calculate index into headers and
			 * that to calculate logical sector value.
			 */
			} else {
				index = bfi / bps - head;
				if (index < 0)
					index += hdr_cnt;
				sect = xy_index2ls(index, (u_int *)pattern_buf);
				/*
				 * If defect spans to the next sector, set up
				 * state to go around again.
				 */
				if (((bfi % bps) + len) > bps) {
					span = 1;
					bfi += bps;
					len -= bps;
				}
			}
			/*
			 * We get funny here for drives without runts.  If the defect
			 * is in the last "sector" and there is no runt then we just
			 * ignore the error
			 */
		    	if ((index + head) == hdr_cnt) {
			    for (j = 0 ; j < hdr_cnt ; j++) {
				if (xy_match(((u_int *)pattern_buf)[j]) == XY_HDR_RUNT)
					break;
			    }
			    if (j == hdr_cnt)  /* if no runt then ignore it */
				continue;
			}
			/*
			 * If we have impossible values, assume the defect
			 * can't be found.
			 */
			if (index >= hdr_cnt) {
				eprint("Unable to locate defect #%d.\n", i);
				error = -1;
				continue;
			}
			/*
			 * If the sector we located has already been removed
			 * from use, nothing needs to be done.
			 */
			if (sect == (XY_HDR_RUNT & 0xffff) ||
			    sect == (XY_HDR_SLIP & 0xffff) ||
			    sect == (XY_HDR_ZAP & 0xffff))
				continue;
			/*
			 * if it is mapped then don't need to do any more 
			 */
			if (xy_isbad(cyl, head, sect)) 
				continue;
			/*
			 * If the sector we located is a spare, mark it
			 * as zapped so it won't get used later by accident.
			 */
			if (sect == (XY_HDR_SPARE & 0xffff)) {
				ptr[index] = XY_HDR_ZAP;
				status = xy_writehdr(bn, hdr_cnt, pattern_buf);
				if (status) {
					eprint("Unable to repair defect #%d.\n",
					    i);
					error = -1;
				}
				continue;
			}
			/*
			 * The defect is in a data sector. Calculate the
			 * actual sector number and repair it with xy_fixblk.
			 */
			blkno = chs2bn(cyl, head, sect);
			status = xy_fixblk(blkno, index, hdr_cnt, pattern_buf,F_SILENT);
			if (status) {
				eprint("Unable to repair defect #%d.\n", i);
				error = -1;
			}
		/*
		 * If the current defect spans to the next sector, loop again.
		 */
		} while (span);
	}
	/*
	 * Mark the disk formatted and return status.
	 */
	cur_flags |= DISK_FORMATTED;
	return (error);
}

/*
 * This routine extracts the manufacturer's defect list from the disk.  This
 * will only work on an unformatted disk, but we let the user try on any
 * disk in case something bad happens and the disk ends up partially
 * formatted.  This will allow the user to get the list off the unformatted
 * part.  The manufacturer's list is encoded on each track of the
 * drive.
 */
int
xy_ex_man(list)
	struct	defect_list *list;
{
	struct	dk_type type;
	int	cyl, head, status, cnt = 0;

	/*
	 * Initialize the defect list to zero length.  This is necessary
	 * because the routines that add defects to the list expect it
	 * to be initialized.
	 */
	list->header.magicno = DEFECT_MAGIC;
	list->header.count = 0;
	list->list = (struct defect_entry *)zalloc(LISTSIZE(0) * SECSIZE);
	/*
	 * Do an ioctl to get back the prom revision of the controller.
	 * If it's not a 451 (rev is less than 'a'), then it doesn't
	 * support the read defect list command.
	 */
	status = ioctl(cur_file, DKIOCGTYPE, &type);
	if (status) {
		eprint("\nUnable to read drive configuration.\n");
		return (-1);
	}
	if (type.dkt_promrev < 'a' - '@') {
		eprint("\nController firmware is outdated and doesn't ");
		eprint("support extraction.\n");
		return (-1);
	}
	/*
	 * Loop through each cylinder on the disk.
	 */
	nolprint("\n");
	for (cyl = 0; cyl < pcyl; cyl++) {
		/*
		 * Print out where we are, so we don't look dead.
		 */
		nolprint("   ");
		pr_dblock(nolprint, (daddr_t)(cyl * spc()));
		nolprint("  \015");
		(void) fflush(stdout);
		/*
		 * Loop through each track on this cylinder.
		 */
		for (head = 0; head < nhead; head++) {
			/*
			 * Extract the track's defects.
			 */
			if (xy_getdefs(cyl, head, list)) {
				/*
				 * The extract failed.  If the user has
				 * already said they don't care, go on.
				 */
				if (cnt < 0)
					continue;
				/*
				 * Keep track of how many failures in a row
				 * we have had.
				 */
				cnt++;
				if (cnt < 3)
					continue;
				/*
				 * We had three failures in a row.  Ask the
				 * user if they want to give up.  Note that
				 * this message MUST be polarized this way
				 * so that the extraction will abort when
				 * running from a command file.
				 */
				eprint("Disk is not fully encoded with ");
				eprint("defect info.\n");
				if (!check("Do you wish to stop extracting"))
					return (-1);
				/*
				 * If the user wants to go on anyway,
				 * don't bother them again.
				 */
				cnt = -1;
			} else {
				if (cnt < 0)
					continue;
				/*
				 * Clear the error count since we succeeded.
				 */
				cnt = 0;
			}
		}
	}
	printf("\n");
	return (0);
}

/*
 * This routine extracts the current defect list from the disk.  It does
 * so by reading the headers in for each track and figuring out which
 * sectors have been repaired.  It will accurately reproduce the defect
 * list except for multiple defects per sector and defects in runt sectors.
 * This will only work on formatted drives, but we let the user try on any
 * disk in case something bad happens and the disk ends up partially
 * formatted.
 */
int
xy_ex_cur(list)
	struct	defect_list *list;
{
	struct	dk_type type;
	int	cyl, head, hdr_cnt, status, ignore = 0;
	daddr_t	bn;

	/*
	 * Initialize the defect list to zero length.  This is necessary
	 * because the routines to add defects to the list assume it is
	 * initialized.
	 */
	list->header.magicno = DEFECT_MAGIC;
	list->header.count = 0;
	list->list = (struct defect_entry *)zalloc(LISTSIZE(0) * SECSIZE);
	/*
	 * Run an ioctl to get back the hard sector count for the disk.
	 */
	status = ioctl(cur_file, DKIOCGTYPE, &type);
	if (status) {
		eprint("\nUnable to read drive configuration.\n");
		return (-1);
	}
	hdr_cnt = type.dkt_hsect;
	/*
	 * If the hard sector count is less than our data sector count,
	 * something is wrong.
	 */
	if (hdr_cnt < nsect) {
		eprint("\nHard sector count less than nsect.\n");
		return (-1);
	}
	/*
	 * Check the drive for spare sectors and sector skewing.
	 */
	if (xy_check_skew(hdr_cnt, pattern_buf)) {
		eprint("\nUnable to read drive configuration.\n");
		return (-1);
	}
	/*
	 * Loop through each cylinder on the disk.
	 */
	nolprint("\n");
	for (cyl = 0; cyl < pcyl; cyl++) {
		/*
		 * Print out where we are so we don't look dead.
		 */
		nolprint("   ");
		pr_dblock(nolprint, (daddr_t)(cyl * spc()));
		nolprint("  \015");
		(void) fflush(stdout);
		/*
		 * Loop for each track on this cylinder.
		 */
		for (head = 0; head < nhead; head++) {
			bn = (daddr_t)(cyl * spc() + head * nsect);
			status = xy_readhdr(bn, hdr_cnt, pattern_buf);
			/*
			 * If we couldn't read the header, we assume it's
			 * because the disk is unformatted.  Ask the user
			 * if they want to go on anyway, since the disk
			 * could be half-formatted.  Note that the polarity
			 * of the message MUST be this way so that it will
			 * abort the command when running from a command file.
			 */
			if (status && (!ignore)) {
				eprint("Disk is not fully formatted.\n");
				if (!check("Do you with to stop extracting"))
					return (-1);
				/*
				 * The user has chosen to go on.  From now
				 * on we ignore errors.
				 */
				ignore = 1;
			}
			/*
			 * If we had an error but are forging on anyway,
			 * we still skip the extract for the header in error.
			 */
			if (status)
				continue;
			xy_getbad(bn, hdr_cnt, pattern_buf, list);
		}
	}
	printf("\n");
	return (0);
}

/*
 * This routine is used to repair defective sectors.  It is assumed that
 * a higher level routine will take care of adding the defect to the
 * defect list.  The approach is to try to slip the sector, and if that
 * fails map it.
 */
int
xy_repair(blkno,flag)
	daddr_t	blkno;
	int	flag;
{
	struct	dk_type type;
	int	status, hdr_cnt;
	daddr_t	bn;
	int	cyl, head, sect;
	int	index;

	/*
	 * Run an ioctl to get back the hard sector count for the disk.
	 */
	status = ioctl(cur_file, DKIOCGTYPE, &type);
	if (status) {
		eprint("Unable to read drive configuration.\n");
		return (-1);
	}
	hdr_cnt = type.dkt_hsect;
	/*
	 * If the hard sector count is less than our data sector count,
	 * something is wrong.
	 */
	if (hdr_cnt < nsect) {
		eprint("Hard sector count less than nsect.\n");
		return (-1);
	}
	/*
	 * Check the drive for spare sectors and sector skewing.
	 */
	if (xy_check_skew(hdr_cnt, pattern_buf)) {
		eprint("Unable to read drive configuration.\n");
		return (-1);
	}
	cyl = bn2c(blkno);
	head = bn2h(blkno);
	sect = bn2s(blkno);
	bn = chs2bn(cyl, head, 0);
	/*
	 * Read in the headers for the track containing the defect.
	 */
	status = xy_readhdr(bn, hdr_cnt, pattern_buf);
	if (status) {
		eprint("Unable to locate defect.\n");
		return (-1);
	}
	/*
	 * Calculate the index of the bad sector in the headers based on
	 * its logical sector value.
	 */
	index = xy_ls2index(sect, (u_int *)pattern_buf);
	/*
	 * If we have impossible values, assume the defect
	 * can't be found.
	 */
	if (index >= hdr_cnt) {
		eprint("Unable to locate defect.\n");
		return (-1);
	}
	/*
	 * Repair the bad sector using xy_fixblk and return status.
	 */
	status = xy_fixblk(blkno, index, hdr_cnt, pattern_buf, flag);
	return (status);
}

/*
 * The routines from here on are internal to the xy controller code, they
 * are not referenced from anywhere outside this file.
 */
/*
 * This routine repairs the given block using the headers and other info
 * passed in.
 */
static int
xy_fixblk(bn, index, hdr_cnt, header_buf, flags)
	daddr_t	bn;
	int	index, hdr_cnt;
	char	*header_buf;
	int	flags;
{
	struct	dk_cmd cmdblk;
	struct	xyhdr header;
	u_int	hdr, *ptr = (u_int *)header_buf;
	int	spare_avail = -1, must_map = 0, runt_sec = -1;
	int	i, status;
	daddr_t	newblk;
	short	cyl, head, sect, ns;

	cyl = bn2c(bn);
	head = bn2h(bn);
	sect = bn2s(bn);
	/*
	 * If the bad sector is already mapped, it can't be fixed again.
	 */
	if (xy_isbad(cyl, head, sect)) {
		eprint("Can't repair sector that is already mapped.\n");
		return (-1);
	}
	/*
	 * Calculate the number of sectors in the track beyond the bad one.
	 * This will be needed if we slip the bad sector.
	 */
	ns = nsect - sect - 1;
	/*
	 * Loop through the headers on the track.  If we find a spare,
	 * remember where it was.  If we find a mapped header, then we
	 * must map this one too or risk messing up the previous mapping
	 * (since slipping moves logical sectors around).  If we find
	 * a runt, we have to note its location because the runt header
	 * cannot be moved if we slip the sector (it is physically
	 * grounded).
	 */
	for (i = index + 1; i < hdr_cnt; i++) {
		hdr = xy_match(ptr[i]);
		if (hdr == XY_HDR_SPARE)
			spare_avail = i;
		if (hdr == XY_HDR_ZAP)
			must_map = 1;
		if (hdr == XY_HDR_RUNT)
			runt_sec = i;
	}
	/*
	 * If no spare was found, we must map the sector.
	 */
	if (spare_avail < 0)
		must_map = 1;
	/*
	 * Try 10 times to read the data out of the bad sector.  If we
	 * can't get it after that, print a warning message.
	 */
	if (!(flags & F_SILENT)) {
		for (i = 0; i < 10; i++) {
			cmdblk.dkc_cmd = XY_READ;
			cmdblk.dkc_blkno = bn;
			cmdblk.dkc_secnt = 1;
			cmdblk.dkc_bufaddr = cur_buf;
			cmdblk.dkc_buflen = SECSIZE;
			status = xycmd(cur_file, &cmdblk, F_SILENT);
			if (!status)
				break;
		}
		if (i >= 10)
			eprint("Warning: unable to save defective data for blkno %d.\n",bn);
	}
	/*
	 * If we are slipping the defect, read in the data for the
	 * rest of the track past the bad sector.  This is necessary
	 * because the headers will be moved when we're done.
	 */
	if (!must_map && ns > 0 && !(flags *F_SILENT)) {
		cmdblk.dkc_cmd = XY_READ;
		cmdblk.dkc_blkno = bn + 1;
		cmdblk.dkc_secnt = ns;
		cmdblk.dkc_bufaddr = cur_buf + SECSIZE;
		cmdblk.dkc_buflen = ns * SECSIZE;
		status = xycmd(cur_file, &cmdblk, F_NORMAL);
		if (status)
			eprint("Warning: unable to save track data.\n");
	}
	/*
	 * If we are mapping, check to be sure the sector isn't in one
	 * of the unmappable portions of the drive.  If it's ok, zap
	 * the header.
	 */
	if (must_map) {
		if (bn == 0 || (bn >= datasects() && bn < totalsects())) {
			eprint("Unable to find suitable alternate sector for blkno %d.\n",bn);
			return (-1);
		}
		ptr[index] = XY_HDR_ZAP;
	} else {
		/*
		 * Construct the header for the bad sector.
		 */
		bzero((char *)&header, sizeof (struct xyhdr));
		header.xyh_sec_hi = XY_SEC_HI(sect);
		header.xyh_sec_lo = XY_SEC_LO(sect);
		header.xyh_cyl_hi = XY_CYL_HI(cyl);
		header.xyh_cyl_lo = XY_CYL_LO(cyl);
		header.xyh_head = (u_char)head;
		header.xyh_type = cur_dtype->dtype_dr_type;
		/*
		 * Rotate the headers for all sectors on the track beyond
		 * the bad sector.
		 */
		for (i = spare_avail; i > index + 1; i--)
			ptr[i] = ptr[i - 1];
		/*
		 * Mark the bad sector as slipped, and fill in the one
		 * after it with the header we constructed.
		 */
		ptr[index + 1] = *(u_int *)&header;
		ptr[index] = XY_HDR_SLIP;
		/*
		 * If there was a runt sector and it got rotated above,
		 * move it back to its real position.
		 */
		if (runt_sec >= 0 && runt_sec < spare_avail) {
			hdr = ptr[runt_sec];
			ptr[runt_sec] = ptr[runt_sec + 1];
			ptr[runt_sec + 1] = hdr;
		}
	}
	/*
	 * Write out the new track headers.
	 */
	status = xy_writehdr(chs2bn(cyl, head, 0), hdr_cnt, header_buf);
	if (status) {
		eprint("Unable to repair track headers.\n");
		return (-1);
	}
	if (must_map) {
		/*
		 * Add the mapped sector to the bad block table.
		 */
		status = xy_addbad(bn, &newblk);
		if (status) {
			eprint("Bad block map table overflow.\n");
			return (-1);
		}
		/*
		 * Write the data from the bad sector out to its replacement.
		 */
		if (!(flags & F_SILENT)) {
			cmdblk.dkc_cmd = XY_WRITE;
			cmdblk.dkc_blkno = newblk;
			cmdblk.dkc_secnt = 1;
			cmdblk.dkc_bufaddr = cur_buf;
			cmdblk.dkc_buflen = SECSIZE;
			status = xycmd(cur_file, &cmdblk, F_NORMAL);
			if (status)
				eprint("Warning: unable to save defective data.\n");
		}
	} else {
		/*
		 * Write out the bad sector plus all the sectors after it.
		 * This is necessary because slipping moves all those sectors.
		 */
		if (!(flags & F_SILENT)) {
			cmdblk.dkc_cmd = XY_WRITE;
			cmdblk.dkc_blkno = bn;
			cmdblk.dkc_secnt = nsect - sect;
			cmdblk.dkc_bufaddr = cur_buf;
			cmdblk.dkc_buflen = (nsect - sect) * SECSIZE;
			status = xycmd(cur_file, &cmdblk, F_NORMAL);
			if (status)
				eprint("Warning: unable to save track data.\n");
		}
	}
	return (0);
}

/*
 * This routine checks to see if the given logical sector is in the
 * bad block table for the current disk.
 */
static int
xy_isbad(cyl, head, sect)
	int	cyl, head, sect;
{
	int	i;
	short	trksec;

	/*
	 * Loop through the bad block table.
	 */
	for (i = 0; i < NDKBAD; i++) {
		/*
		 * If we are at the end of the table, quit searching.
		 */
		if (badmap.bt_bad[i].bt_cyl < 0)
			break;
		/*
		 * If this entry is pseudo_invalid, skip it.
		 */
		if (badmap.bt_bad[i].bt_trksec < 0)
			continue;
		/*
		 * If the logical sector number isn't equal to the sector
		 * value passed in, skip it.
		 */
		if (badmap.bt_bad[i].bt_cyl != cyl)
			continue;
		trksec = (head << 8) | sect;
		if (badmap.bt_bad[i].bt_trksec != trksec)
			continue;
		/*
		 * Found a match.  Return that fact.
		 */
		return (1);
	}
	/*
	 * No match was found.
	 */
	return (0);
}

/* 
 * This routine adds the given logical sector to the bad block table and 
 * passes back the logical sector that has replaced it.
 */
static int
xy_addbad(bn, newbn)
	daddr_t	bn, *newbn;
{
	int	i;
	short	cyl, head, sect;

	cyl = bn2c(bn);
	head = bn2h(bn);
	sect = bn2s(bn);
	/*
	 * Look for the end of the bad block table or a pseudo-invalid
	 * entry.  If we don't find a slot to use, return an error.
	 */
	for (i = 0; i < NDKBAD; i++) {
		if (badmap.bt_bad[i].bt_cyl < 0)
			break;
		if (badmap.bt_bad[i].bt_trksec < 0)
			break;
	}
	if (i >= NDKBAD)
		return (-1);
	/*
	 * Fill in the entry with the sector number passed in.
	 */
	badmap.bt_bad[i].bt_cyl = cyl;
	badmap.bt_bad[i].bt_trksec = (head << 8) | sect;
	/*
	 * Fill in the return parameter with the block number of the
	 * replacement sector.
	 */
	*newbn = totalsects() - nsect - 1 - i;
	return (0);
}

/*
 * This routine looks up the given error in the xy driver error structures.
 * It is stolen from the xy driver.
 */
static struct xyerror *
xy_finderr(errno)
	register u_char errno;
{
	register struct xyerror *errptr;

	/*
	 * Do a linear search down the error structure for the given error.
	 * The structure is sorted by error frequency, so this algorithm is
	 * good enough.
	 */
	for (errptr = xyerrors; errptr->errno != XYE_UNKN; errptr++)
		if (errptr->errno == errno)
			break;
	return (errptr);
}

/*
 * This routine prints out error messages based on parameters describing
 * the error.  The parameters are gotten via an ioctl call.  This routine
 * is different than the actual driver routine, since we only see an
 * error after all recovery has been tried.
 */
static
xy_printerr(severity, type, msg, cmd, blkno)
	u_char	severity, type;
	char	*msg;
	u_short	cmd;
	daddr_t	blkno;
{

	/*
	 * Print out the severity.
	 */
	eprint("\015Block %d  (",blkno);
	pr_dblock(eprint, blkno);

	switch(severity) {
	    case DK_FATAL:
		eprint("), Fatal ");
		break;
	    case DK_RECOVERED:
		eprint("), Recovered ");
		break;
	    case DK_CORRECTED:
		eprint("), Corrected ");
		break;
	    default:
		eprint("), Unknown severity value from xy driver.\n");
		fullabort();
	}
	/*
	 * Print out the error type.
	 */
	if (type == DK_ISMEDIA)
		eprint("media error");
	else
		eprint("non-media error");
	/*
	 * Print out the rest of the error info.
	 */
	if (option_msg)
		eprint(" (%s) during %s\n", msg, xycmdnames[cmd >> 8]);
	else
		eprint(" (%s)\n", msg);

}

/*
 * This routine runs all the commands that use the generic command ioctl
 * interface.  It is the back door into the driver.
 */
static int
xycmd(file, cmdblk, flags)
	int	file, flags;
	struct	dk_cmd *cmdblk;
{
	struct	dk_diag diag;
	struct	xyerror *errptr;
	int	status;

	/*
	 * We always run the commands with these flags on.  The silent flag
	 * suppresses console messages, since we will print our own if
	 * necessary.  The isolate flag keeps these special commands from
	 * getting chained since they may take a long time.  The diagnose
	 * flags enables us to find out when a non-fatal has occurred.
	 */
	cmdblk->dkc_flags = DK_SILENT | DK_ISOLATE | DK_DIAGNOSE;
	/*
	 * Run the command.  If it succeeds, we are done.
	 */
	status = ioctl(file, DKIOCSCMD, cmdblk);
	if (status == 0)
		return (status);
	/*
	 * Something went wrong.  Run the diagnose ioctl to see what happened.
	 */
	status = ioctl(file, DKIOCGDIAG, &diag);
	if (status || diag.dkd_severe == DK_NOERROR) {
		eprint("Error: unexpected ioctl error from xy driver.\n");
		fullabort();
	}
	/*
	 * Look up the error.
	 */
	errptr = xy_finderr(diag.dkd_errno);
	/*
	 * If we are tracking all errors (ie surface analysis), set the
	 * media_error flags to indicate whether or not this error implies
	 * a defect.
	 */
	if (flags & F_ALLERRS) {
		if (errptr->errtype == DK_ISMEDIA)
			media_error = 1;
		else
			media_error = 0;
	}
	/*
	 * If we the command was not in silent mode, print an error message.
	 */
	if (!(flags & F_SILENT))
		xy_printerr(diag.dkd_severe, errptr->errtype, errptr->errmsg,
		    diag.dkd_errcmd, diag.dkd_errsect);
	/*
	 * Return status.
	 */
	if (diag.dkd_severe == DK_FATAL || flags & F_ALLERRS)
		return (-1);
	else
		return (0);
}

/* 
 * This routine takes a header and determines whether it is from a data 
 * sector or a special sector.  The elaborate comparisons are necessary 
 * because defective sectors often have garbage mixed into their headers. 
 */
static u_int
xy_match(hdr)
	u_int	hdr;
{

	/*
	 * If it matches a special header, return it.
	 */
	switch (hdr) {
	    case XY_HDR_SLIP:
	    case XY_HDR_RUNT:
	    case XY_HDR_SPARE:
	    case XY_HDR_ZAP:
		return (hdr);
	}
	/*
	 * If it matches enough of a special header to identify it,
	 * return the header it must represent.
	 */
	switch (hdr & 0x000000ff) {
	    case (XY_HDR_SLIP & 0x000000ff):
		return (XY_HDR_SLIP);
	    case (XY_HDR_RUNT & 0x000000ff):
		return (XY_HDR_RUNT);
	    case (XY_HDR_SPARE & 0x000000ff):
		return (XY_HDR_SPARE);
	    case (XY_HDR_ZAP & 0x000000ff):
		return (XY_HDR_ZAP);
	}
	switch (hdr & 0xff000000) {
	    case (XY_HDR_SLIP & 0xff000000):
		return (XY_HDR_SLIP);
	    case (XY_HDR_RUNT & 0xff000000):
		return (XY_HDR_RUNT);
	    case (XY_HDR_SPARE & 0xff000000):
		return (XY_HDR_SPARE);
	    case (XY_HDR_ZAP & 0xff000000):
		return (XY_HDR_ZAP);
	}
	if ((hdr & 0x38000000) == 0)
		return (hdr);
	switch (hdr & 0x00ffff00) {
	    case (XY_HDR_SLIP & 0x00ffff00):
		return (XY_HDR_SLIP);
	    case (XY_HDR_RUNT & 0x00ffff00):
		return (XY_HDR_RUNT);
	    case (XY_HDR_SPARE & 0x00ffff00):
		return (XY_HDR_SPARE);
	    case (XY_HDR_ZAP & 0x00ffff00):
		return (XY_HDR_ZAP);
	}
	/*
	 * It seems like a data header, return it.
	 */
	return (hdr);
}

/* 
 * This routine takes an index into a track headers and calculates which 
 * logical sector on the track is at the index. 
 */
static int
xy_index2ls(index, buf)
	int	index;
	u_int	*buf;
{
	int	i, ls;
	u_int	hdr;

	/*
	 * Start out assuming the sector number equals the index.
	 */
	ls = index;
	/*
	 * If there are any slipped or runt sectors before it, then it
	 * will be one lower than index for each.
	 */
	for (i = 0; i < index; i++) {
		hdr = xy_match(buf[i]);
		if (hdr == XY_HDR_RUNT || hdr == XY_HDR_SLIP)
			ls--;
	}
	/*
	 * If the header isn't a data header, return that fact.  Otherwise
	 * return the logical sector on the track that we calculated.
	 */
	hdr = xy_match(buf[i]);
	if (hdr == XY_HDR_RUNT || hdr == XY_HDR_SLIP ||
	    hdr == XY_HDR_SPARE || hdr == XY_HDR_ZAP)
		return (hdr & 0xffff);
	else
		return (ls);
}

/* 
 * This routine takes a logical sector value on a track and a track headers, 
 * and calculates the index into the headers of that logical sector.
 */
static int
xy_ls2index(ls, buf)
	int	ls;
	u_int	*buf;
{
	int	i, index;
	u_int	hdr;

	/*
	 * Start out assuming that the index equals the sector number.
	 */
	index = ls;
	/*
	 * If there are any slipped or runt sectors before it, the index
	 * will be one higher than the sector number.  Note that if we
	 * find a runt we must also search one sector farther, since the
	 * sector at index + 1 could be slipped.
	 */
	for (i = 0; i <= ls; i++) {
		hdr = xy_match(buf[i]);
		if (hdr == XY_HDR_RUNT) {
			ls++;
			index++;
		}
		if (hdr == XY_HDR_SLIP)
			index++;
	}
	/*
	 * Return the calculated index.  We assume the calling function
	 * knows whether the sector is data or special.
	 */
	return (index);
}

/*
 * This routine checks the current disk for spare sectors and sector
 * skewing.  Sector skewing is physically done on all 451s, but earlier
 * revs of proms read headers in from logical index instead of physical,
 * so it was transparent to the user.  Later revs read headers in
 * physically, so they have to be rotated by hand into logical order.
 * This routine must be called before any headers are read or
 * disaster would result.  The routine sets global variables to reflect
 * what was found.
 */
static int
xy_check_skew(cnt, buf)
	int	cnt;
	char	*buf;
{

	struct	dk_cmd cmdblk;
	u_int	*ptr = (u_int *)buf;
	int	lsec, i, status, found = 0;

	/*
	 * Start out assuming there are not spare sectors and headers
	 * are read in logical form.  We must assume these because of the
	 * way the routine works.
	 */
	spare_sector = 0;
	physical_form = 0;
	/*
	 * Read the headers off of track 1.  Any track would do, as long
	 * as it is near enough the beginning to work on all disks and
	 * it is not the first track on a cylinder (no sector skewing on
	 * the bottom track).
	 */
	cmdblk.dkc_cmd = XY_READHDR;
	cmdblk.dkc_blkno = nsect;
	cmdblk.dkc_secnt = cnt;
	cmdblk.dkc_bufaddr = buf;
	cmdblk.dkc_buflen = cnt * XY_HDRSIZE;
	status = xycmd(cur_file, &cmdblk, F_NORMAL);
	if (status)
		return (-1);
	/*
	 * Loop through the headers.
	 */
	for (lsec = i = 0; i < cnt; i++) {
		/*
		 * If we find a slipped sector, spare sectors exist for
		 * this disk.
		 */
		if (xy_match(ptr[i]) == XY_HDR_SLIP) {
			spare_sector = 1;
			continue;
		}
		/*
		 * If we find a spare sector, spare sectors exist for
		 * this disk.  We can also infer that headers are in
		 * physical form.  This is possible because logical
		 * form would put the spare at the end of the headers,
		 * and we don't loop that long.
		 */
		if (xy_match(ptr[i]) == XY_HDR_SPARE) {
			spare_sector = 1;
			physical_form = 1;
			found = 1;
			continue;
		}
		/*
		 * If we've already figured out the header form, we
		 * don't need to do the comparisons below.  However,
		 * we continue the loop to look for slipped sectors.
		 */
		if (found)
			continue;
		/*
		 * If we find a runt, skip it.
		 */
		if (xy_match(ptr[i]) == XY_HDR_RUNT)
			continue;
		/*
		 * If we find a mapped sector, skip it and increment the
		 * logical sector number used for comparisons.
		 */
		if (xy_match(ptr[i]) == XY_HDR_ZAP) {
			lsec++;
			continue;
		}
		/*
		 * Check to be sure the header is for a data sector.
		 */
		if (XY_GET_CYL((struct xyhdr *)&ptr[i]) != 0) {
			lsec++;
			continue;
		}
		/*
		 * If the header does not represent the expected sector
		 * for logical form, the headers must be in physical form.
		 */
		if (lsec != XY_GET_SEC((struct xyhdr *)&ptr[i]))
			physical_form = 1;
		found = 1;
	}
	/*
	 * If the disk does not seem set up for sparing, print a warning.
	 */
	if (spare_sector == 0)
		eprint("Warning: disk not set for slip-sectoring.\n");
	/*
	 * If we couldn't determine the header form, return an error.
	 */
	if (found)
		return (0);
	else
		return (-1);
}

/*
 * This routine reads in the headers for a given track.
 */
static int
xy_readhdr(bn, cnt, buf)
	daddr_t	bn;
	int	cnt;
	char	*buf;
{
	struct	dk_cmd cmdblk;
	struct	xyhdr hdr, *ptr = (struct xyhdr *)buf;
	int	i, j, status;

	/*
	 * Execute the command.
	 */
	cmdblk.dkc_cmd = XY_READHDR;
	cmdblk.dkc_blkno = bn;
	cmdblk.dkc_secnt = cnt;
	cmdblk.dkc_bufaddr = buf;
	cmdblk.dkc_buflen = cnt * XY_HDRSIZE;
	status = xycmd(cur_file, &cmdblk, F_NORMAL);
	if (status)
		return (-1);
	/*
	 * If the headers come in physical form, they must be converted to
	 * logical form for later use.  This is done by rotating them one
	 * sector for each track above zero they are on.
	 */
	if (physical_form) {
		for (i = 0; i < bn2h(bn); i++) {
			hdr = ptr[0];
			for (j = 0; j < cnt - 1; j++)
				ptr[j] = ptr[j + 1];
			ptr[cnt - 1] = hdr;
		}
	}
	return (0);
}

/*
 * This routine writes the given headers to the current disk.
 */
static int
xy_writehdr(bn, cnt, buf)
	daddr_t	bn;
	int	cnt;
	char	*buf;
{
	struct	dk_cmd cmdblk;
	u_int	hdr, *ptr = (u_int *)buf;
	int	i, j;

	/*
	 * All headers are written in physical form, so they must be
	 * converted to physical form before the write is executed.
	 * This is done by rotating them by one sector for each track
	 * above zero they are on.
	 */
	for (i = 0; i < bn2h(bn); i++) {
		hdr = ptr[cnt - 1];
		for (j = cnt - 1; j > 0; j--)
			ptr[j] = ptr[j - 1];
		ptr[0] = hdr;
	}
	/*
	 * Execute the command and return status.
	 */
	cmdblk.dkc_cmd = XY_WRITEHDR;
	cmdblk.dkc_blkno = bn;
	cmdblk.dkc_secnt = cnt;
	cmdblk.dkc_bufaddr = buf;
	cmdblk.dkc_buflen = cnt * XY_HDRSIZE;
	return (xycmd(cur_file, &cmdblk, F_NORMAL));
}

/* 
 * This routine examines the headers passed in and extracts a list 
 * of defective sectors from it.  The defective sectors are added 
 * to the given defect list. 
 */
static
xy_getbad(bn, cnt, buf, list)
	daddr_t	bn;
	int	cnt;
	char	*buf;
	struct	defect_list *list;
{
	u_int	*ptr = (u_int *)buf;
	int     sect, head, i;
	struct  defect_entry def;
	int     index, lsec;
	int bps = cur_dtype->dtype_bps;

	/*
	 * Loop through the headers.
	 */
	for (lsec = i = 0;i < cnt && lsec < (nsect + spare_sector);i++) {
		/*
		 * If we find a runt, skip it.
		 */
		if (xy_match(ptr[i]) == XY_HDR_RUNT) {
			continue;
		}
		/*
		 * If we find a spare, we are at the end of the headers.
		 */
		if (xy_match(ptr[i]) == XY_HDR_SPARE)
			break;
		/*
		 * Pull out the sector value for this header and do a
		 * sanity check on it.  If it looks right then continue.
		 */
		sect = XY_GET_SEC((struct xyhdr *)&ptr[i]);
		head = ((struct xyhdr *)&ptr[i])->xyh_head;
		if ((sect >= 0 && sect < nsect) &&
		    (head >= 0 && head < nhead)) {
			lsec++;
			continue;
		}
		/*
		 * This header was not recognizable as a data sector.
		 * Add the sector value for the header to the defect list.
		 */
		/*
		 * Calculate the fields for the defect struct.
		 */
		def.cyl = bn2c(bn);
		def.head = bn2h(bn);
		def.bfi =  bps * ((lsec + bn2h(bn)) % (nsect + 1));
		/*
		 * Initialize the unknown fields.
		 */
		def.nbits = UNKNOWN;
		def.sect  = UNKNOWN;
		/*
		 * Calculate the index into the list that the defect belongs at.
		 */
		index = sort_defect(&def, list);
		/*
		 * Add the defect to the list.
		 */
		add_def(&def, list, index);
		lsec++;
	}
}

/* 
 * This routine reads the manufacturer's defect list off of the given track 
 * and adds them to the defect list.  The algorithm used is to try
 * reading the defect info three times.  If all three tries fail,
 * return an error.
 */
static int
xy_getdefs(cyl, head, list)
	int	cyl, head;
	struct	defect_list *list;
{
	struct	xydefinfo trkdefs;
	struct	dk_cmd cmdblk;
	struct	defect_entry def;
	int	status, i, index, cnt;
	u_char	*ptr = trkdefs.info;
	int     def_cnt = 0;


	/*
	 * Try 3 times.
	 */
	for (cnt = 0; cnt < 3; cnt++) {
		/*
		 * Execute the read defect command for the track.
		 */
		cmdblk.dkc_cmd = XY_READALL | XY_DEFLST;
		cmdblk.dkc_blkno = cyl * spc() + head * nsect;
		cmdblk.dkc_secnt = 1;
		cmdblk.dkc_bufaddr = (caddr_t)ptr;
		cmdblk.dkc_buflen = sizeof (struct xydefinfo);
		status = xycmd(cur_file, &cmdblk, F_NORMAL);
		/*
		 * Reverse the bits in the defect info (due to 451 botch).
		 */
		xy_reverse(ptr, sizeof (struct xydefinfo));
		/*
		 * If the the defect info is corrupt, try again.
		 */
		if ((status) ||
		    (XY_MAN_SYNC(ptr) != XY_SYNCBYTE) ||
		    (XY_MAN_LAST(ptr) != XY_LASTBYTE) ||
		    (XY_MAN_CYL(ptr) != cyl) ||
		    (XY_MAN_HEAD(ptr) != head))
			continue;
		/*
		 * Everything looks cool.  Stop searching.
		 */
		break;
	}
	/*
	 * If we got here by running out of tries, return an error.
	 */
	if (cnt >= 3)
		return (-1);
	/*
	 * We have valid defect info.  Step through the array of defects.
	 */
	for (i = 0; i < 4; i++) {
		/*
		 * If the length is 0, this slot is empty.
		 */
		if (XY_MAN_LEN(ptr, i) == 0)
			continue;
		/*
		 * Fill in a defect struct and add it to the list.
		 */
		def.cyl = (short)cyl;
		def.head = (short)head;
		def.sect = UNKNOWN;
		def.nbits = (short)XY_MAN_LEN(ptr, i);
		def.bfi = (int)XY_MAN_BFI(ptr, i);
		index = sort_defect(&def, list);
		add_def(&def, list, index);
		def_cnt++;
	}
	/*
	 * if there are less then 4 errors then the track is marked bad
	 * because of a missing address mark.  These controllers don't care
	 * about that.
	 */
	if (def_cnt == 4 && XY_TRK_BAD(ptr)) {
		printf("track is bad. cyl = %d head = %x\n",def.cyl,def.head);
		return(-1);
	}
	return (0);
}

/*
 * This routine reverses the bits and bytes of the defect info struct.
 * The bits must be reversed because of a botch in the 451 hardware.
 * The bytes must be reversed because of the multibus->vme translation.
 */
static
xy_reverse(buf, cnt)
	u_char	*buf;
	int	cnt;
{
	u_char	x;
	int	i, j;

	/*
	 * Swap the bits.
	 */
	for (i = 0; i < cnt; i++) {
		x = 0;
		for (j = 0; j < 8; j++) {
			x = x << 1;
			x += buf[i] & 0x01;
			buf[i] = buf[i] >> 1;
		}
		buf[i] = x;
	}
	/*
	 * Now swap the bytes.
	 */
	for (i = 0; i < cnt; i += 2) {
		x = buf[i];
		buf[i] = buf[i + 1];
		buf[i + 1] = x;
	}
}

