#ifndef lint
#ident	"@(#)ctlr_scsi.c	1.1	92/07/30 SMI"
#endif	lint

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains the routines for embedded scsi disks
 */
#include "global.h"

#include <sys/types.h>
#include <sys/dk.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <assert.h>
#include <memory.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <scsi/generic/mode.h>
#include <scsi/generic/sense.h>
#include <scsi/generic/commands.h>
#include <scsi/generic/dad_mode.h>
#include <scsi/impl/mode.h>
#include <scsi/impl/uscsi.h>
#include <scsi/impl/commands.h>
#include <scsi/targets/sddef.h>

#include <sun/dkio.h>

extern	int	errno;

#ifndef	DAD_MODE_CACHE_CCS
#define	DAD_MODE_CACHE_CCS		0x38
#endif

#include "startup.h"
#include "scsi_com.h"
#include "misc.h"
#include "ctlr_scsi.h"
#include "analyze.h"
#include "param.h"
#include "io.h"


struct	ctlr_ops scsiops = {
	scsi_rdwr,
	scsi_ck_format,
	scsi_format,
	scsi_ex_man,
	scsi_ex_cur,
	scsi_repair,
	0,
};


#define	SCMD_UNKNOWN		0xff

/*
 * Names of commands.  Must have SCMD_UNKNOWN at end of list.
 */
struct scsi_command_name {
	int	command;
	char	*name;
} scsi_command_names[] = {
	SCMD_FORMAT,		"format",
	SCMD_READ,		"read",
	SCMD_WRITE,		"write",
	SCMD_READ|SCMD_GROUP1,	"read",
	SCMD_WRITE|SCMD_GROUP1,	"write",
	SCMD_INQUIRY,		"inquiry",
	SCMD_MODE_SELECT,	"mode select",
	SCMD_MODE_SENSE,	"mode sense",
	SCMD_REASSIGN_BLOCK,	"reassign block",
	SCMD_REQUEST_SENSE,	"request sense",
	SCMD_READ_DEFECT_LIST,	"read defect list",
	SCMD_UNKNOWN,		"unknown"
};


/*
 * Strings for printing mode sense page control values
 */
slist_t page_control_strings[] = {
	{ "current",	MODE_SENSE_PC_CURRENT },
	{ "changeable",	MODE_SENSE_PC_CHANGEABLE },
	{ "default",	MODE_SENSE_PC_DEFAULT },
	{ "saved",	MODE_SENSE_PC_SAVED }
};

/*
 * Strings for printing the mode select options
 */
slist_t mode_select_strings[] = {
	{ "",		0 },
	{ " (pf)",	MODE_SELECT_PF },
	{ " (sp)",	MODE_SELECT_SP },
	{ " (pf,sp)",	MODE_SELECT_PF|MODE_SELECT_SP }
};

/*
 * Enable extensive diagnostic messages
 */
int	diag_msg = 1;


/*
 * Read or write the disk.
 */
int
scsi_rdwr(dir, fd, blkno, secnt, bufaddr, flags)
	int	dir;
	int	fd;
	daddr_t blkno;
	int	secnt;
	caddr_t bufaddr;
	int	flags;

{
	struct uscsi_cmd	ucmd;
	union scsi_cdb		cdb;

	/*
	 * Build and execute the uscsi ioctl.  We build a group0
	 * or group1 command as necessary, since some targets
	 * do not support group1 commands.
	 */
	(void) memset((char *)&ucmd, 0, sizeof (ucmd));
	(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
	cdb.scc_cmd = (dir == DIR_READ) ? SCMD_READ : SCMD_WRITE;
	if (blkno < (2<<20) && secnt <= 0xff) {
		FORMG0ADDR(&cdb, blkno);
		FORMG0COUNT(&cdb, secnt);
		ucmd.uscsi_cdblen = CDB_GROUP0;
	} else {
		FORMG1ADDR(&cdb, blkno);
		FORMG1COUNT(&cdb, secnt);
		ucmd.uscsi_cdblen = CDB_GROUP1;
		cdb.scc_cmd |= SCMD_GROUP1;
	}
	ucmd.uscsi_cdb = (caddr_t) &cdb;
	ucmd.uscsi_bufaddr = bufaddr;
	ucmd.uscsi_buflen = secnt * SECSIZE;
	return (uscsi_cmd(fd, &ucmd, flags));
}


/*
 * Check to see if the disk has been formatted.
 * If we are able to read the first track, we conclude that
 * the disk has been formatted.
 */
int
scsi_ck_format()
{
	int	status;

	/*
	 * Try to read the first four blocks.
	 */
	status = scsi_rdwr(DIR_READ, cur_file, (daddr_t) 0, 4,
		(caddr_t) cur_buf, F_SILENT);
	return (!status);
}


/*
 * Format the disk, the whole disk, and nothing but the disk.
 */
/*ARGSUSED*/
int
scsi_format(start, end, list)
	int			start;		/* irrelevant for us */
	int			end;
	struct defect_list	*list;
{
	struct uscsi_cmd	ucmd;
	union scsi_cdb		cdb;
	struct scsi_defect_hdr	defect_hdr;
	int			status;
	int			flag;
	struct scsi_inquiry	*inq;
	char			rawbuf[MAX_MODE_SENSE_SIZE];


	/*
	 * Determine if the target appears to be SCSI-2
	 * compliant.  We handle mode sense/mode selects
	 * a little differently, depending upon CCS/SCSI-2
	 */
	if (uscsi_inquiry(cur_file, rawbuf, sizeof (rawbuf))) {
		eprint("Inquiry failed\n");
		return (-1);
	}
	inq = (struct scsi_inquiry *) rawbuf;
	flag = (inq->inq_rdf == RDF_SCSI2);

	/*
	 * Set up the various SCSI parameters specified before
	 * formatting the disk.  Each routine handles the
	 * parameters relevent to a particular page.
	 * If no parameters are specified for a page, there's
	 * no need to do anything.  Otherwise, issue a mode
	 * sense for that page.  If a specified parameter
	 * differs from the drive's default value, and that
	 * parameter is not fixed, then issue a mode select to
	 * set the default value for the disk as specified
	 * in format.dat.
	 */
	if (scsi_ms_page1(flag) || scsi_ms_page2(flag) ||
		scsi_ms_page4(flag) || scsi_ms_page38(flag) ||
			scsi_ms_page8(flag) || scsi_ms_page3(flag)) {
		return (-1);
	}

	/*
	 * If we're debugging the drive, dump every page
	 * the device supports, for thorough analysis.
	 */
	if (option_msg && diag_msg) {
		scsi_dump_mode_sense_pages(MODE_SENSE_PC_DEFAULT);
		scsi_dump_mode_sense_pages(MODE_SENSE_PC_CURRENT);
		scsi_dump_mode_sense_pages(MODE_SENSE_PC_SAVED);
		scsi_dump_mode_sense_pages(MODE_SENSE_PC_CHANGEABLE);
		eprint("\n");
	}

	if (sun4_flag) {
		int				i;
		int				num_defects;
		struct defect_entry		*defect;
		struct dk_cmd			dkcmd;
		struct defect_list		glist;
		struct scsi_format_params	*format_params;

		if (list->flags & LIST_PGLIST) {
			/*
			 * Format with both the P&G lists.  Since the
			 * sun4 driver interface does not allow us to
			 * control the cdb options, we extract the
			 * G list, and send it down with the format
			 * command itself.  Hopefully this is an
			 * equivalent operation.
			 */
			if (scsi_ex_grown(&glist)) {
				eprint("Cannot extract the grown defects list\n");
				return (-1);
			}
		} else {
			/*
			 * Format with just the P list.  Here we
			 * just fake out an empty G list.
			 */
			glist.header.count = 0;
		}
		format_params = (struct scsi_format_params *)
			zalloc((glist.header.count * 8) + 4);
		num_defects = 0;
		defect = glist.list;
		for (i = 0; i < glist.header.count; i++) {
			if (defect->bfi != -1) {
				format_params->list[i].cyl = defect->cyl;
				format_params->list[i].head = defect->head;
				format_params->list[i].bytes_from_index =
					defect->bfi;
				num_defects++;
			}
			defect++;
		}

		format_params->length = num_defects * 8;

		dkcmd.dkc_cmd = SCMD_FORMAT;
		dkcmd.dkc_bufaddr = (caddr_t) format_params;
		dkcmd.dkc_buflen = (num_defects * 8) + 4;

		/*
		 * Issue the format ioctl
		 */
		if (option_msg && diag_msg) {
			printf("\nFormatting...\n");
		}
		(void) fflush(stdout);
		flag = (option_msg && diag_msg) ? F_NORMAL : F_SILENT;
		status = scsi_dkcmd(cur_file, &dkcmd, flag);
	} else {
		/*
		 * Construct the uscsi format ioctl.  The form depends
		 * upon the defect list the user extracted.  If s/he
		 * extracted the "original" list, we format with only
		 * the P (manufacturer's defect) list.  Otherwise, we
		 * format with both the P and the G (grown) list.
		 * To format with the P and G list, we set the fmtData
		 * bit, and send an empty list.  To format with the
		 * P list only, we also set the cmpLst bit, meaning
		 * that the (empty) list we send down is the complete
		 * G list, thereby discarding the old G list..
		 */
		(void) memset((char *)&ucmd, 0, sizeof (ucmd));
		(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
		(void) memset((char *)&defect_hdr, 0, sizeof (defect_hdr));
		cdb.scc_cmd = SCMD_FORMAT;
		ucmd.uscsi_cdb = (caddr_t) &cdb;
		ucmd.uscsi_cdblen = CDB_GROUP0;
		ucmd.uscsi_bufaddr = (caddr_t) &defect_hdr;
		ucmd.uscsi_buflen = sizeof (defect_hdr);
		cdb.cdb_opaque[1] = FPB_DATA;
		if ((list->flags & LIST_PGLIST) == 0) {
			/*
			 * No G list.  The empty list we
			 * send down is the complete list.
			 */
			cdb.cdb_opaque[1] |= FPB_CMPLT;
		}
	
		/*
		 * Issue the format ioctl
		 */
		if (option_msg && diag_msg) {
			printf("\nFormatting...\n");
		}
		(void) fflush(stdout);
		flag = (option_msg && diag_msg) ? F_NORMAL : F_SILENT;
		status = uscsi_cmd(cur_file, &ucmd, flag);
	}

	return (status);
}


/*
 * Check disk error recovery parameters via mode sense.
 * Issue a mode select if we need to change something.
 */
/*ARGSUSED*/
int
scsi_ms_page1(scsi2_flag)
	int	scsi2_flag;
{
	struct mode_err_recov		*page1;
	struct mode_err_recov		*fixed;
	struct scsi_ms_header		header;
	struct scsi_ms_header		fixed_hdr;
	int				status;
	int				tmp1, tmp2;
	int				flag;
	int				length;
	int				sp_flags;
	union {
		struct mode_err_recov	page1;
		char			rawbuf[MAX_MODE_SENSE_SIZE];
	} u_page1, u_fixed;


	page1 = &u_page1.page1;
	fixed = &u_fixed.page1;

	/*
	 * If debugging, issue mode senses on the default and
	 * current values.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_ERR_RECOV,
			MODE_SENSE_PC_DEFAULT, (caddr_t) page1,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_ERR_RECOV,
			MODE_SENSE_PC_CURRENT, (caddr_t) page1,
			MAX_MODE_SENSE_SIZE, &header);
	}

	/*
	 * Issue a mode sense to determine the saved parameters
	 * If the device cannot return the saved parameters,
	 * use the current, instead.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_ERR_RECOV,
			MODE_SENSE_PC_SAVED, (caddr_t) page1,
			MAX_MODE_SENSE_SIZE, &header);
	if (status) {
		status = uscsi_mode_sense(cur_file, DAD_MODE_ERR_RECOV,
			MODE_SENSE_PC_CURRENT, (caddr_t) page1,
			MAX_MODE_SENSE_SIZE, &header);
		if (status) {
			return (0);
		}
	}

	/*
	 * We only need the common subset between the CCS
	 * and SCSI-2 structures, so we can treat both
	 * cases identically.  Whatever the drive gives
	 * us, we return to the drive in the mode select,
	 * delta'ed by whatever we want to change.
	 */
	length = MODESENSE_PAGE_LEN(page1);
	if (length < MIN_PAGE1_LEN) {
		return (0);
	}

	/*
	 * Ask for changeable parameters.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_ERR_RECOV,
		MODE_SENSE_PC_CHANGEABLE, (caddr_t) fixed,
		MAX_MODE_SENSE_SIZE, &fixed_hdr);
	if (status || MODESENSE_PAGE_LEN(fixed) < MIN_PAGE1_LEN) {
		return (0);
	}

	/*
	 * We need to issue a mode select only if one or more
	 * parameters need to be changed, and those parameters
	 * are flagged by the drive as changeable.
	 */
	flag = 0;
	tmp1 = page1->read_retry_count;
	tmp2 = page1->write_retry_count;
	if (cur_dtype->dtype_options & SUP_READ_RETRIES &&
			fixed->read_retry_count != 0) {
		flag |= (page1->read_retry_count !=
				cur_dtype->dtype_read_retries);
		page1->read_retry_count = cur_dtype->dtype_read_retries;
	}
	if (length > 8) {
		if (cur_dtype->dtype_options & SUP_WRITE_RETRIES &&
				fixed->write_retry_count != 0) {
			flag |= (page1->write_retry_count !=
					cur_dtype->dtype_write_retries);
			page1->write_retry_count =
					cur_dtype->dtype_write_retries;
		}
	}
	/*
	 * Report any changes so far...
	 */
	if (flag && option_msg) {
		eprint(
"PAGE 1: read retries= %d (%d)  write retries= %d (%d)\n",
			page1->read_retry_count, tmp1,
			page1->write_retry_count, tmp2);
	}
	/*
	 * Apply any changes requested via the change list method
	 */
	flag |= apply_chg_list(DAD_MODE_ERR_RECOV, length,
		(u_char *) page1, (u_char *) fixed,
			cur_dtype->dtype_chglist);
	/*
	 * If no changes required, do not issue a mode select
	 */
	if (flag == 0) {
		return (0);
	}
	/*
	 * We always want to set the Page Format bit for mode
	 * selects.  Set the Save Page bit if the drive indicates
	 * that it can save this page via the mode sense.
	 */
	sp_flags = MODE_SELECT_PF;
	if (page1->mode_page.ps) {
		sp_flags |= MODE_SELECT_SP;
	}
	page1->mode_page.ps = 0;
	header.mode_header.length = 0;
	header.mode_header.device_specific = 0;
	status = uscsi_mode_select(cur_file, DAD_MODE_ERR_RECOV,
		sp_flags, (caddr_t) page1, length, &header);
	if (status && (sp_flags & MODE_SELECT_SP)) {
		/* If failed, try not saving mode select params. */
		sp_flags &= ~MODE_SELECT_SP;
		status = uscsi_mode_select(cur_file, DAD_MODE_ERR_RECOV,
			sp_flags, (caddr_t) page1, length, &header);
		}
	if (status && option_msg) {
		eprint("\
Warning: Using default error recovery parameters.\n\n");
	}

	/*
	 * If debugging, issue mode senses on the current and
	 * saved values, so we can see the result of the mode
	 * selects.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_ERR_RECOV,
			MODE_SENSE_PC_CURRENT, (caddr_t) page1,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_ERR_RECOV,
			MODE_SENSE_PC_SAVED, (caddr_t) page1,
			MAX_MODE_SENSE_SIZE, &header);
	}

	return (0);
}

/*
 * Check disk disconnect/reconnect parameters via mode sense.
 * Issue a mode select if we need to change something.
 */
/*ARGSUSED*/
int
scsi_ms_page2(scsi2_flag)
	int	scsi2_flag;
{
	struct mode_disco_reco		*page2;
	struct mode_disco_reco		*fixed;
	struct scsi_ms_header		header;
	struct scsi_ms_header		fixed_hdr;
	int				status;
	int				flag;
	int				length;
	int				sp_flags;
	union {
		struct mode_disco_reco	page2;
		char			rawbuf[MAX_MODE_SENSE_SIZE];
	} u_page2, u_fixed;

	page2 = &u_page2.page2;
	fixed = &u_fixed.page2;

	/*
	 * If debugging, issue mode senses on the default and
	 * current values.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, MODEPAGE_DISCO_RECO,
			MODE_SENSE_PC_DEFAULT, (caddr_t) page2,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, MODEPAGE_DISCO_RECO,
			MODE_SENSE_PC_CURRENT, (caddr_t) page2,
			MAX_MODE_SENSE_SIZE, &header);
	}

	/*
	 * Issue a mode sense to determine the saved parameters
	 * If the device cannot return the saved parameters,
	 * use the current, instead.
	 */
	status = uscsi_mode_sense(cur_file, MODEPAGE_DISCO_RECO,
			MODE_SENSE_PC_SAVED, (caddr_t) page2,
			MAX_MODE_SENSE_SIZE, &header);
	if (status) {
		status = uscsi_mode_sense(cur_file, MODEPAGE_DISCO_RECO,
			MODE_SENSE_PC_CURRENT, (caddr_t) page2,
			MAX_MODE_SENSE_SIZE, &header);
		if (status) {
			return (0);
		}
	}

	/*
	 * We only need the common subset between the CCS
	 * and SCSI-2 structures, so we can treat both
	 * cases identically.  Whatever the drive gives
	 * us, we return to the drive in the mode select,
	 * delta'ed by whatever we want to change.
	 */
	length = MODESENSE_PAGE_LEN(page2);
	if (length < MIN_PAGE2_LEN) {
		return (0);
	}

	/*
	 * Ask for changeable parameters.
	 */
	status = uscsi_mode_sense(cur_file, MODEPAGE_DISCO_RECO,
		MODE_SENSE_PC_CHANGEABLE, (caddr_t) fixed,
		MAX_MODE_SENSE_SIZE, &fixed_hdr);
	if (status || MODESENSE_PAGE_LEN(fixed) < MIN_PAGE2_LEN) {
		return (0);
	}

	/*
	 * We need to issue a mode select only if one or more
	 * parameters need to be changed, and those parameters
	 * are flagged by the drive as changeable.
	 */
	flag = 0;
	/*
	 * Apply any changes requested via the change list method
	 */
	flag |= apply_chg_list(MODEPAGE_DISCO_RECO, length,
		(u_char *) page2, (u_char *) fixed,
			cur_dtype->dtype_chglist);
	/*
	 * If no changes required, do not issue a mode select
	 */
	if (flag == 0) {
		return (0);
	}
	/*
	 * Issue a mode select
	 */
	sp_flags = MODE_SELECT_PF;
	if (page2->mode_page.ps) {
		sp_flags |= MODE_SELECT_SP;
	}
	page2->mode_page.ps = 0;
	header.mode_header.length = 0;
	header.mode_header.device_specific = 0;
	status = uscsi_mode_select(cur_file, MODEPAGE_DISCO_RECO,
		sp_flags, (caddr_t) page2, length, &header);
	if (status && (sp_flags & MODE_SELECT_SP)) {
		/* If failed, try not saving mode select params. */
		sp_flags &= ~MODE_SELECT_SP;
		status = uscsi_mode_select(cur_file, MODEPAGE_DISCO_RECO,
			sp_flags, (caddr_t) page2, length, &header);
		}
	if (status && option_msg) {
		eprint("Warning: Using default .\n\n");
	}

	/*
	 * If debugging, issue mode senses on the current and
	 * saved values, so we can see the result of the mode
	 * selects.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, MODEPAGE_DISCO_RECO,
			MODE_SENSE_PC_CURRENT, (caddr_t) page2,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, MODEPAGE_DISCO_RECO,
			MODE_SENSE_PC_SAVED, (caddr_t) page2,
			MAX_MODE_SENSE_SIZE, &header);
	}

	return (0);
}

/*
 * Check disk format parameters via mode sense.
 * Issue a mode select if we need to change something.
 */
/*ARGSUSED*/
int
scsi_ms_page3(scsi2_flag)
	int	scsi2_flag;
{
	struct mode_format		*page3;
	struct mode_format		*fixed;
	struct scsi_ms_header		header;
	struct scsi_ms_header		fixed_hdr;
	int				status;
	int				tmp1, tmp2, tmp3;
	int				tmp4, tmp5, tmp6;
	int				flag;
	int				length;
	int				sp_flags;
	union {
		struct mode_format	page3;
		char			rawbuf[MAX_MODE_SENSE_SIZE];
	} u_page3, u_fixed;


	page3 = &u_page3.page3;
	fixed = &u_fixed.page3;

	/*
	 * If debugging, issue mode senses on the default and
	 * current values.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_FORMAT,
			MODE_SENSE_PC_DEFAULT, (caddr_t) page3,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_FORMAT,
			MODE_SENSE_PC_CURRENT, (caddr_t) page3,
			MAX_MODE_SENSE_SIZE, &header);
	}

	/*
	 * Issue a mode sense to determine the saved parameters
	 * If the device cannot return the saved parameters,
	 * use the current, instead.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_FORMAT,
			MODE_SENSE_PC_SAVED, (caddr_t) page3,
			MAX_MODE_SENSE_SIZE, &header);
	if (status) {
		status = uscsi_mode_sense(cur_file, DAD_MODE_FORMAT,
			MODE_SENSE_PC_CURRENT, (caddr_t) page3,
			MAX_MODE_SENSE_SIZE, &header);
		if (status) {
			return (0);
		}
	}

	/*
	 * We only need the common subset between the CCS
	 * and SCSI-2 structures, so we can treat both
	 * cases identically.  Whatever the drive gives
	 * us, we return to the drive in the mode select,
	 * delta'ed by whatever we want to change.
	 */
	length = MODESENSE_PAGE_LEN(page3);
	if (length < MIN_PAGE3_LEN) {
		return (0);
	}

	/*
	 * Ask for changeable parameters.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_FORMAT,
		MODE_SENSE_PC_CHANGEABLE, (caddr_t) fixed,
		MAX_MODE_SENSE_SIZE, &fixed_hdr);
	if (status || MODESENSE_PAGE_LEN(fixed) < MIN_PAGE3_LEN) {
		return (0);
	}

	/*
	 * We need to issue a mode select only if one or more
	 * parameters need to be changed, and those parameters
	 * are flagged by the drive as changeable.
	 */
	tmp1 = page3->track_skew;
	tmp2 = page3->cylinder_skew;
	tmp3 = page3->sect_track;
	tmp4 = page3->tracks_per_zone;
	tmp5 = page3->alt_tracks_vol;
	tmp6 = page3->alt_sect_zone;

	flag = (page3->data_bytes_sect != SECSIZE);
	page3->data_bytes_sect = SECSIZE;

	flag |= (page3->interleave != 1);
	page3->interleave = 1;

	if (cur_dtype->dtype_options & SUP_CYLSKEW &&
					fixed->cylinder_skew != 0) {
		flag |= (page3->cylinder_skew != cur_dtype->dtype_cyl_skew);
		page3->cylinder_skew = cur_dtype->dtype_cyl_skew;
	}
	if (cur_dtype->dtype_options & SUP_TRKSKEW &&
					fixed->track_skew != 0) {
		flag |= (page3->track_skew != cur_dtype->dtype_trk_skew);
		page3->track_skew = cur_dtype->dtype_trk_skew;
	}
	if (cur_dtype->dtype_options & SUP_PSECT &&
					fixed->sect_track != 0) {
		flag |= (page3->sect_track != psect);
		page3->sect_track = psect;
	}
	if (cur_dtype->dtype_options & SUP_TRKS_ZONE &&
					fixed->tracks_per_zone != 0) {
		flag |= (page3->tracks_per_zone != cur_dtype->dtype_trks_zone);
		page3->tracks_per_zone = cur_dtype->dtype_trks_zone;
	}
	if (cur_dtype->dtype_options & SUP_ASECT &&
					fixed->alt_sect_zone != 0) {
		flag |= (page3->alt_sect_zone != cur_dtype->dtype_asect);
		page3->alt_sect_zone = cur_dtype->dtype_asect;
	}
	if (cur_dtype->dtype_options & SUP_ATRKS &&
					fixed->alt_tracks_vol != 0) {
		flag |= (page3->alt_tracks_vol != cur_dtype->dtype_atrks);
		page3->alt_tracks_vol = cur_dtype->dtype_atrks;
	}
	/*
	 * Notify user of any changes so far
	 */
	if (flag && option_msg) {
		eprint("PAGE 3: trk skew= %d (%d)   cyl skew= %d (%d)   ",
			page3->track_skew, tmp1, page3->cylinder_skew, tmp2);
		eprint("sects/trk= %d (%d)\n", page3->sect_track, tmp3);
		eprint("        trks/zone= %d (%d)   alt trks= %d (%d)   ",
			page3->tracks_per_zone, tmp4,
			page3->alt_tracks_vol, tmp5);
		eprint("alt sects/zone= %d (%d)\n",
				page3->alt_sect_zone, tmp6);
	}
	/*
	 * Apply any changes requested via the change list method
	 */
	flag |= apply_chg_list(DAD_MODE_FORMAT, length,
		(u_char *) page3, (u_char *) fixed,
			cur_dtype->dtype_chglist);
	/*
	 * If no changes required, do not issue a mode select
	 */
	if (flag == 0) {
		return (0);
	}
	/*
	 * Issue a mode select
	 */
	/*
	 * We always want to set the Page Format bit for mode
	 * selects.  Set the Save Page bit if the drive indicates
	 * that it can save this page via the mode sense.
	 */
	sp_flags = MODE_SELECT_PF;
	if (page3->mode_page.ps) {
		sp_flags |= MODE_SELECT_SP;
	}
	page3->mode_page.ps = 0;
	header.mode_header.length = 0;
	header.mode_header.device_specific = 0;
	status = uscsi_mode_select(cur_file, DAD_MODE_FORMAT,
		sp_flags, (caddr_t) page3, length, &header);
	if (status && (sp_flags & MODE_SELECT_SP)) {
		/* If failed, try not saving mode select params. */
		sp_flags &= ~MODE_SELECT_SP;
		status = uscsi_mode_select(cur_file, DAD_MODE_FORMAT,
			sp_flags, (caddr_t) page3, length, &header);
		}
	if (status && option_msg) {
		eprint("Warning: Using default drive format parameters.\n");
		eprint("Warning: Drive format may not be correct.\n\n");
	}

	/*
	 * If debugging, issue mode senses on the current and
	 * saved values, so we can see the result of the mode
	 * selects.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_FORMAT,
			MODE_SENSE_PC_CURRENT, (caddr_t) page3,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_FORMAT,
			MODE_SENSE_PC_SAVED, (caddr_t) page3,
			MAX_MODE_SENSE_SIZE, &header);
	}

	return (0);
}

/*
 * Check disk geometry parameters via mode sense.
 * Issue a mode select if we need to change something.
 */
/*ARGSUSED*/
int
scsi_ms_page4(scsi2_flag)
	int	scsi2_flag;
{
	struct mode_geometry		*page4;
	struct mode_geometry		*fixed;
	struct scsi_ms_header		header;
	struct scsi_ms_header		fixed_hdr;
	int				status;
	int				tmp1, tmp2;
	int				flag;
	int				length;
	int				sp_flags;
	union {
		struct mode_geometry	page4;
		char			rawbuf[MAX_MODE_SENSE_SIZE];
	} u_page4, u_fixed;

	page4 = &u_page4.page4;
	fixed = &u_fixed.page4;

	/*
	 * If debugging, issue mode senses on the default and
	 * current values.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_GEOMETRY,
			MODE_SENSE_PC_DEFAULT, (caddr_t) page4,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_GEOMETRY,
			MODE_SENSE_PC_CURRENT, (caddr_t) page4,
			MAX_MODE_SENSE_SIZE, &header);
	}

	/*
	 * Issue a mode sense to determine the saved parameters
	 * If the device cannot return the saved parameters,
	 * use the current, instead.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_GEOMETRY,
			MODE_SENSE_PC_SAVED, (caddr_t) page4,
			MAX_MODE_SENSE_SIZE, &header);
	if (status) {
		status = uscsi_mode_sense(cur_file, DAD_MODE_GEOMETRY,
			MODE_SENSE_PC_CURRENT, (caddr_t) page4,
			MAX_MODE_SENSE_SIZE, &header);
		if (status) {
			return (0);
		}
	}

	/*
	 * We only need the common subset between the CCS
	 * and SCSI-2 structures, so we can treat both
	 * cases identically.  Whatever the drive gives
	 * us, we return to the drive in the mode select,
	 * delta'ed by whatever we want to change.
	 */
	length = MODESENSE_PAGE_LEN(page4);
	if (length < MIN_PAGE4_LEN) {
		return (0);
	}

	/*
	 * Ask for changeable parameters.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_GEOMETRY,
		MODE_SENSE_PC_CHANGEABLE, (caddr_t) fixed,
		MAX_MODE_SENSE_SIZE, &fixed_hdr);
	if (status || MODESENSE_PAGE_LEN(fixed) < MIN_PAGE4_LEN) {
		return (0);
	}

	/*
	 * We need to issue a mode select only if one or more
	 * parameters need to be changed, and those parameters
	 * are flagged by the drive as changeable.
	 */
	tmp1 = (page4->cyl_ub << 16) + (page4->cyl_mb << 8) + page4->cyl_lb;
	tmp2 = page4->heads;

	flag = 0;
	if (cur_dtype->dtype_options & SUP_PHEAD && fixed->heads != 0) {
		flag |= (page4->heads != phead);
		page4->heads = (u_char)phead;
	}
	/*
	 * Notify user of changes so far
	 */
	if (flag && option_msg) {
		eprint("PAGE 4: cylinders= %d    heads= %d (%d)\n",
			tmp1, page4->heads, tmp2);
	}
	/*
	 * Apply any changes requested via the change list method
	 */
	flag |= apply_chg_list(DAD_MODE_GEOMETRY, length,
		(u_char *) page4, (u_char *) fixed,
			cur_dtype->dtype_chglist);
	/*
	/*
	 * If no changes required, do not issue a mode select
	 */
	if (flag == 0) {
		return (0);
	}
	/*
	 * Issue a mode select
	 */
	/*
	 * We always want to set the Page Format bit for mode
	 * selects.  Set the Save Page bit if the drive indicates
	 * that it can save this page via the mode sense.
	 */
	sp_flags = MODE_SELECT_PF;
	if (page4->mode_page.ps) {
		sp_flags |= MODE_SELECT_SP;
	}
	page4->mode_page.ps = 0;
	header.mode_header.length = 0;
	header.mode_header.device_specific = 0;
	status = uscsi_mode_select(cur_file, DAD_MODE_GEOMETRY,
		sp_flags, (caddr_t) page4, length, &header);
	if (status && (sp_flags & MODE_SELECT_SP)) {
		/* If failed, try not saving mode select params. */
		sp_flags &= ~MODE_SELECT_SP;
		status = uscsi_mode_select(cur_file, DAD_MODE_GEOMETRY,
			sp_flags, (caddr_t) page4, length, &header);
		}
	if (status && option_msg) {
		eprint("Warning: Using default drive geometry.\n\n");
	}

	/*
	 * If debugging, issue mode senses on the current and
	 * saved values, so we can see the result of the mode
	 * selects.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_GEOMETRY,
			MODE_SENSE_PC_CURRENT, (caddr_t) page4,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_GEOMETRY,
			MODE_SENSE_PC_SAVED, (caddr_t) page4,
			MAX_MODE_SENSE_SIZE, &header);
	}

	return (0);
}

/*
 * Check SCSI-2 disk cache parameters via mode sense.
 * Issue a mode select if we need to change something.
 */
/*ARGSUSED*/
int
scsi_ms_page8(scsi2_flag)
	int	scsi2_flag;
{
	struct mode_cache		*page8;
	struct mode_cache		*fixed;
	struct scsi_ms_header		header;
	struct scsi_ms_header		fixed_hdr;
	int				status;
	int				flag;
	int				length;
	int				sp_flags;
	union {
		struct mode_cache	page8;
		char			rawbuf[MAX_MODE_SENSE_SIZE];
	} u_page8, u_fixed;

	page8 = &u_page8.page8;
	fixed = &u_fixed.page8;

	/*
	 * Only SCSI-2 devices support this page
	 */
	if (!scsi2_flag) {
		return (0);
	}

	/*
	 * If debugging, issue mode senses on the default and
	 * current values.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_CACHE,
			MODE_SENSE_PC_DEFAULT, (caddr_t) page8,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_CACHE,
			MODE_SENSE_PC_CURRENT, (caddr_t) page8,
			MAX_MODE_SENSE_SIZE, &header);
	}

	/*
	 * Issue a mode sense to determine the saved parameters
	 * If the device cannot return the saved parameters,
	 * use the current, instead.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_CACHE,
			MODE_SENSE_PC_SAVED, (caddr_t) page8,
			MAX_MODE_SENSE_SIZE, &header);
	if (status) {
		status = uscsi_mode_sense(cur_file, DAD_MODE_CACHE,
			MODE_SENSE_PC_CURRENT, (caddr_t) page8,
			MAX_MODE_SENSE_SIZE, &header);
		if (status) {
			return (0);
		}
	}

	/*
	 * We only need the common subset between the CCS
	 * and SCSI-2 structures, so we can treat both
	 * cases identically.  Whatever the drive gives
	 * us, we return to the drive in the mode select,
	 * delta'ed by whatever we want to change.
	 */
	length = MODESENSE_PAGE_LEN(page8);
	if (length < MIN_PAGE8_LEN) {
		return (0);
	}

	/*
	 * Ask for changeable parameters.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_CACHE,
		MODE_SENSE_PC_CHANGEABLE, (caddr_t) fixed,
		MAX_MODE_SENSE_SIZE, &fixed_hdr);
	if (status || MODESENSE_PAGE_LEN(fixed) < MIN_PAGE8_LEN) {
		return (0);
	}

	/*
	 * We need to issue a mode select only if one or more
	 * parameters need to be changed, and those parameters
	 * are flagged by the drive as changeable.
	 */
	flag = 0;
	/*
	 * Apply any changes requested via the change list method
	 */
	flag |= apply_chg_list(DAD_MODE_CACHE, length,
		(u_char *) page8, (u_char *) fixed,
			cur_dtype->dtype_chglist);
	/*
	 * If no changes required, do not issue a mode select
	 */
	if (flag == 0) {
		return (0);
	}
	/*
	 * Issue a mode select
	 */
	/*
	 * We always want to set the Page Format bit for mode
	 * selects.  Set the Save Page bit if the drive indicates
	 * that it can save this page via the mode sense.
	 */
	sp_flags = MODE_SELECT_PF;
	if (page8->mode_page.ps) {
		sp_flags |= MODE_SELECT_SP;
	}
	page8->mode_page.ps = 0;
	header.mode_header.length = 0;
	header.mode_header.device_specific = 0;
	status = uscsi_mode_select(cur_file, DAD_MODE_CACHE,
		sp_flags, (caddr_t) page8, length, &header);
	if (status && (sp_flags & MODE_SELECT_SP)) {
		/* If failed, try not saving mode select params. */
		sp_flags &= ~MODE_SELECT_SP;
		status = uscsi_mode_select(cur_file, DAD_MODE_CACHE,
			sp_flags, (caddr_t) page8, length, &header);
		}
	if (status && option_msg) {
		eprint("\
Warning: Using default SCSI-2 cache parameters.\n\n");
	}

	/*
	 * If debugging, issue mode senses on the current and
	 * saved values, so we can see the result of the mode
	 * selects.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_CACHE,
			MODE_SENSE_PC_CURRENT, (caddr_t) page8,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_CACHE,
			MODE_SENSE_PC_SAVED, (caddr_t) page8,
			MAX_MODE_SENSE_SIZE, &header);
	}

	return (0);
}

/*
 * Check CCS disk cache parameters via mode sense.
 * Issue a mode select if we need to change something.
 */
/*ARGSUSED*/
int
scsi_ms_page38(scsi2_flag)
	int	scsi2_flag;
{
	struct mode_cache_ccs		*page38;
	struct mode_cache_ccs		*fixed;
	struct scsi_ms_header		header;
	struct scsi_ms_header		fixed_hdr;
	int				status;
	int				tmp1, tmp2, tmp3, tmp4;
	int				flag;
	int				length;
	int				sp_flags;
	union {
		struct mode_cache_ccs	page38;
		char			rawbuf[MAX_MODE_SENSE_SIZE];
	} u_page38, u_fixed;

	page38 = &u_page38.page38;
	fixed = &u_fixed.page38;

	/*
	 * If debugging, issue mode senses on the default and
	 * current values.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_CACHE_CCS,
			MODE_SENSE_PC_DEFAULT, (caddr_t) page38,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_CACHE_CCS,
			MODE_SENSE_PC_CURRENT, (caddr_t) page38,
			MAX_MODE_SENSE_SIZE, &header);
	}

	/*
	 * Issue a mode sense to determine the saved parameters
	 * If the device cannot return the saved parameters,
	 * use the current, instead.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_CACHE_CCS,
			MODE_SENSE_PC_SAVED, (caddr_t) page38,
			MAX_MODE_SENSE_SIZE, &header);
	if (status) {
		status = uscsi_mode_sense(cur_file, DAD_MODE_CACHE_CCS,
			MODE_SENSE_PC_CURRENT, (caddr_t) page38,
			MAX_MODE_SENSE_SIZE, &header);
		if (status) {
			return (0);
		}
	}

	/*
	 * We only need the common subset between the CCS
	 * and SCSI-2 structures, so we can treat both
	 * cases identically.  Whatever the drive gives
	 * us, we return to the drive in the mode select,
	 * delta'ed by whatever we want to change.
	 */
	length = MODESENSE_PAGE_LEN(page38);
	if (length < MIN_PAGE38_LEN) {
		return (0);
	}

	/*
	 * Ask for changeable parameters.
	 */
	status = uscsi_mode_sense(cur_file, DAD_MODE_CACHE_CCS,
		MODE_SENSE_PC_CHANGEABLE, (caddr_t) fixed,
		MAX_MODE_SENSE_SIZE, &fixed_hdr);
	if (status || MODESENSE_PAGE_LEN(fixed) < MIN_PAGE38_LEN) {
		return (0);
	}

	/*
	 * We need to issue a mode select only if one or more
	 * parameters need to be changed, and those parameters
	 * are flagged by the drive as changeable.
	 */
	tmp1 = page38->mode;
	tmp2 = page38->threshold;
	tmp3 = page38->min_prefetch;
	tmp4 = page38->max_prefetch;

	flag = 0;
	if ((cur_dtype->dtype_options & SUP_CACHE) &&
			(fixed->mode & cur_dtype->dtype_cache) ==
				cur_dtype->dtype_cache) {
		flag |= (page38->mode != cur_dtype->dtype_cache);
		page38->mode = cur_dtype->dtype_cache;
	}
	if ((cur_dtype->dtype_options & SUP_PREFETCH) &&
		(fixed->threshold & cur_dtype->dtype_threshold) ==
				cur_dtype->dtype_threshold) {
		flag |= (page38->threshold != cur_dtype->dtype_threshold);
		page38->threshold = cur_dtype->dtype_threshold;
	}
	if ((cur_dtype->dtype_options & SUP_CACHE_MIN) &&
		(fixed->min_prefetch & cur_dtype->dtype_prefetch_min) ==
				cur_dtype->dtype_prefetch_min) {
		flag |= (page38->min_prefetch != cur_dtype->dtype_prefetch_min);
		page38->min_prefetch = cur_dtype->dtype_prefetch_min;
	}
	if ((cur_dtype->dtype_options & SUP_CACHE_MAX) &&
		(fixed->max_prefetch & cur_dtype->dtype_prefetch_max) ==
				cur_dtype->dtype_prefetch_max) {
		flag |= (page38->max_prefetch != cur_dtype->dtype_prefetch_max);
		page38->max_prefetch = cur_dtype->dtype_prefetch_max;
	}
	/*
	 * Notify the user of changes up to this point
	 */
	if (flag && option_msg) {
		eprint("PAGE 38: cache mode= 0x%x (0x%x)\n",
					page38->mode, tmp1);
		eprint("         min. prefetch multiplier= %d   ",
					page38->min_multiplier);
		eprint("max. prefetch multiplier= %d\n",
					page38->max_multiplier);
		eprint("         threshold= %d (%d)   ",
					page38->threshold, tmp2);
		eprint("min. prefetch= %d (%d)   ",
					page38->min_prefetch, tmp3);
		eprint("max. prefetch= %d (%d)\n",
					page38->max_prefetch, tmp4);
	}
	/*
	 * Apply any changes requested via the change list method
	 */
	flag |= apply_chg_list(DAD_MODE_CACHE_CCS, length,
		(u_char *) page38, (u_char *) fixed,
			cur_dtype->dtype_chglist);
	/*
	 * If no changes required, do not issue a mode select
	 */
	if (flag == 0) {
		return (0);
	}
	/*
	 * Issue a mode select
	 *
	 * We always want to set the Page Format bit for mode
	 * selects.  Set the Save Page bit if the drive indicates
	 * that it can save this page via the mode sense.
	 */
	sp_flags = MODE_SELECT_PF;
	if (page38->mode_page.ps) {
		sp_flags |= MODE_SELECT_SP;
	}
	page38->mode_page.ps = 0;
	header.mode_header.length = 0;
	header.mode_header.device_specific = 0;
	status = uscsi_mode_select(cur_file, DAD_MODE_CACHE_CCS,
		sp_flags, (caddr_t) page38, length, &header);
	if (status && (sp_flags & MODE_SELECT_SP)) {
		/* If failed, try not saving mode select params. */
		sp_flags &= ~MODE_SELECT_SP;
		status = uscsi_mode_select(cur_file, DAD_MODE_CACHE_CCS,
			sp_flags, (caddr_t) page38, length, &header);
		}
	if (status && option_msg) {
		eprint("Warning: Using default CCS cache parameters.\n\n");
	}

	/*
	 * If debugging, issue mode senses on the current and
	 * saved values, so we can see the result of the mode
	 * selects.
	 */
	if (option_msg && diag_msg) {
		(void) uscsi_mode_sense(cur_file, DAD_MODE_CACHE_CCS,
			MODE_SENSE_PC_CURRENT, (caddr_t) page38,
			MAX_MODE_SENSE_SIZE, &header);
		(void) uscsi_mode_sense(cur_file, DAD_MODE_CACHE_CCS,
			MODE_SENSE_PC_SAVED, (caddr_t) page38,
			MAX_MODE_SENSE_SIZE, &header);
	}

	return (0);
}


/*
 * Extract the manufacturer's defect list.
 */
int
scsi_ex_man(list)
	struct  defect_list	*list;
{
	int	i;

	i = scsi_read_defect_data(list, DLD_MAN_DEF_LIST);
	if (i != 0)
		return (i);
	list->flags &= ~LIST_PGLIST;
	return (0);
}


/*
 * Extract the current defect list.
 * For embedded scsi drives, this means both the P & G lists.
 */
int
scsi_ex_cur(list)
	struct  defect_list *list;
{
	int	i;

	i = scsi_read_defect_data(list, DLD_GROWN_DEF_LIST | DLD_MAN_DEF_LIST);
	if (i != 0)
		return (i);
	list->flags |= LIST_PGLIST;
	return (0);
}


/*
 * Extract just the G list.
 */
int
scsi_ex_grown(list)
	struct  defect_list *list;
{
	int	i;

	i = scsi_read_defect_data(list, DLD_GROWN_DEF_LIST);
	if (i != 0)
		return (i);
	list->flags |= LIST_PGLIST;
	return (0);
}


int
scsi_read_defect_data(list, pglist_flags)
	struct  defect_list	*list;
	int			pglist_flags;
{
	struct uscsi_cmd	ucmd;
	union scsi_cdb		cdb;
	struct scsi_defect_list	*defects;
	struct scsi_defect_list	def_list;
	struct scsi_defect_hdr	*hdr;
	int			status;
	int			nbytes;
	int			len;	/* returned defect list length */

	hdr = (struct scsi_defect_hdr *)&def_list;

	/*
	 * First get length of list by asking for the header only.
	 */
	(void) memset((char *)&def_list, 0, sizeof (def_list));

	/*
	 * Build and execute the uscsi ioctl
	 */
	(void) memset((char *)&ucmd, 0, sizeof (ucmd));
	(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
	cdb.scc_cmd = SCMD_READ_DEFECT_LIST;
	FORMG1COUNT(&cdb, sizeof (struct scsi_defect_hdr));
	cdb.cdb_opaque[2] = pglist_flags | DLD_BFI_FORMAT;
	ucmd.uscsi_cdb = (caddr_t) &cdb;
	ucmd.uscsi_cdblen = CDB_GROUP1;
	ucmd.uscsi_bufaddr = (caddr_t) hdr;
	ucmd.uscsi_buflen = sizeof (struct scsi_defect_hdr);
	status = uscsi_cmd(cur_file, &ucmd, F_SILENT);

	if (status != 0) {
		if (option_msg) {
			eprint("No %s defect list.\n",
				pglist_flags & DLD_GROWN_DEF_LIST ?
				"grown" : "manufacturer's");
		}
		return (-1);
	}

	/*
	 * Read the full list the second time
	 */
	len = hdr->length;
	nbytes = len + sizeof (struct scsi_defect_hdr);

	defects = (struct scsi_defect_list *) zalloc(nbytes);
	*(struct scsi_defect_hdr *)defects = *(struct scsi_defect_hdr *)hdr;

	(void) memset((char *)&ucmd, 0, sizeof (ucmd));
	(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
	cdb.scc_cmd = SCMD_READ_DEFECT_LIST;
	FORMG1COUNT(&cdb, nbytes);
	cdb.cdb_opaque[2] = pglist_flags | DLD_BFI_FORMAT;
	ucmd.uscsi_cdb = (caddr_t) &cdb;
	ucmd.uscsi_cdblen = CDB_GROUP1;
	ucmd.uscsi_bufaddr = (caddr_t) defects;
	ucmd.uscsi_buflen = nbytes;
	status = uscsi_cmd(cur_file, &ucmd, F_SILENT);

	if (status) {
		eprint("can't read defect list 2nd time");
		destroy_data((char *) defects);
		return (-1);
	}

	if (len != hdr->length) {
		eprint("not enough defects");
		destroy_data((char *) defects);
		return (-1);
	}
	scsi_convert_list_to_new(list, defects, DLD_BFI_FORMAT);
	destroy_data((char *) defects);
	return (0);
}


/*
 * Map a block.
 */
/*ARGSUSED*/
int
scsi_repair(bn, flag)
	int	bn;
	int	flag;
{
	struct uscsi_cmd		ucmd;
	union scsi_cdb			cdb;
	struct scsi_reassign_blk	defect_list;

	/*
	 * Build and execute the uscsi ioctl
	 */
	(void) memset((char *)&ucmd, 0, sizeof (ucmd));
	(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
	(void) memset((char *)&defect_list, 0,
		sizeof (struct scsi_reassign_blk));
	cdb.scc_cmd = SCMD_REASSIGN_BLOCK;
	ucmd.uscsi_cdb = (caddr_t) &cdb;
	ucmd.uscsi_cdblen = CDB_GROUP0;
	ucmd.uscsi_bufaddr = (caddr_t) &defect_list;
	ucmd.uscsi_buflen = sizeof (struct scsi_reassign_blk);
	defect_list.length = sizeof (defect_list.defect);
	defect_list.defect = bn;
	return (uscsi_cmd(cur_file, &ucmd, F_NORMAL));
}

/*
 * Convert a SCSI-style defect list to our generic format.
 * We can handle different format lists.
 */
void
scsi_convert_list_to_new(list, def_list, list_format)
	struct defect_list		*list;
	struct scsi_defect_list		*def_list;
	int				 list_format;
{
	register struct scsi_bfi_defect	*old_defect;
	register struct defect_entry	*new_defect;
	register int			len;
	register int			i;


	switch (list_format) {

	case DLD_BFI_FORMAT:
		/*
		 * Allocate space for the rest of the list.
		 */
		len = def_list->length / sizeof (struct scsi_bfi_defect);
		old_defect = def_list->list;
		new_defect = (len == 0) ? NULL :
			(struct defect_entry *)
				zalloc(LISTSIZE(len) * SECSIZE);

		list->header.count = len;
		list->header.magicno = (u_int) DEFECT_MAGIC;
		list->list = new_defect;

		for (i = 0; i < len; i++, new_defect++, old_defect++) {
			new_defect->cyl = (short) old_defect->cyl;
			new_defect->head = (short) old_defect->head;
			new_defect->bfi = (int) old_defect->bytes_from_index;
			new_defect->nbits = UNKNOWN;	/* size of defect */
		}

		break;

	default:
		eprint("scsi_convert_list_to_new: can't deal with it\n");
		exit (0);
		/*NOTREACHED*/
	}

	(void) checkdefsum(list, CK_MAKESUM);
}



/*
 * Execute a command and determine the result.
 * Uses the "uscsi" ioctl interface, which is
 * fully supported.
 */
int
uscsi_cmd(fd, ucmd, flags)
	int			fd;
	struct uscsi_cmd	*ucmd;
	int			flags;
{
	struct uscsi_cmd		rqcmd;
	union scsi_cdb			rqcdb;
	struct scsi_extended_sense	*rq;
	char				rqbuf[255];
	int				status;
	int				rqlen;

	/*
	 * If sun4, convert this uscsi command to
	 * the equivalent dkcmd for the old driver.
	 */
	if (sun4_flag) {
		return (scsi_maptodkcmd(fd, ucmd, flags));
	}

	/*
	 * Set function flags for driver.
	 */
	ucmd->uscsi_flags = USCSI_ISOLATE | USCSI_DIAGNOSE;
	if (flags & F_SILENT) {
		ucmd->uscsi_flags |= USCSI_SILENT;
	}

	/*
	 * If this command will perform a read, set the USCSI_READ flag
	 */
	if (ucmd->uscsi_buflen > 0) {
		switch (ucmd->uscsi_cdb[0]) {
		case SCMD_READ:
		case SCMD_READ|SCMD_GROUP1:
		case SCMD_MODE_SENSE:
		case SCMD_REQUEST_SENSE:
		case SCMD_INQUIRY:
		case SCMD_READ_DEFECT_LIST:
			ucmd->uscsi_flags |= USCSI_READ;
			break;
		}
	}

	/*
	 * Execute the ioctl
	 */
	status = ioctl(fd, USCSICMD, ucmd);
	if (status == 0 && ucmd->uscsi_status == 0)
		return (status);

	/*
	 * Check to see if this is the old sun4 driver which
	 * doesn't support the uscsi interface.
	 */
	if (status == -1 && (errno == EINVAL || errno == ENOTTY)) {
		sun4_flag = 1;
		return (scsi_maptodkcmd(fd, ucmd, flags));
	}

	/*
	 * Get the request sense info, if possible, to find out
	 * more about the error
	 */
	(void) memset((char *)rqbuf, 0, sizeof (rqbuf));
	(void) memset((char *)&rqcmd, 0, sizeof (rqcmd));
	(void) memset((char *)&rqcdb, 0, sizeof (union scsi_cdb));
	rqcdb.scc_cmd = SCMD_REQUEST_SENSE;
	FORMG0COUNT(&rqcdb, sizeof (rqbuf));
	rqcmd.uscsi_cdb = (caddr_t) &rqcdb;
	rqcmd.uscsi_cdblen = CDB_GROUP0;
	rqcmd.uscsi_bufaddr = (caddr_t) rqbuf;
	rqcmd.uscsi_buflen = sizeof (rqbuf);
	rqcmd.uscsi_flags = USCSI_ISOLATE | USCSI_DIAGNOSE | USCSI_READ;
	status = ioctl(fd, USCSICMD, &rqcmd);

	rq = (struct scsi_extended_sense *) rqbuf;
	rqlen = (int) rq->es_add_len + 8;
	if (status || rq->es_class != CLASS_EXTENDED_SENSE ||
			rqlen < MIN_REQUEST_SENSE_LEN) {
		if (option_msg) {
			eprint("Request sense for command %s failed\n",
				scsi_find_command_name(ucmd->uscsi_cdb[0]));
		}
		if (status == 0) {
			if (option_msg && diag_msg) {
				dump("sense:  ", (caddr_t) rqbuf,
					sizeof (rqbuf), HEX_ONLY);
			}
		}
		return (-1);
	}

	/*
	 * If the failed command is a Mode Select, and the
	 * target is indicating that it has rounded one of
	 * the mode select parameters, as defined in the SCSI-2
	 * specification, then we should accept the command
	 * as successful.
	 */
	if (ucmd->uscsi_cdb[0] == SCMD_MODE_SELECT) {
		if (rq->es_key == KEY_RECOVERABLE_ERROR &&
			rq->es_add_info[ADD_SENSE_CODE] ==
				ROUNDED_PARAMETER &&
			rq->es_add_info[ADD_SENSE_QUAL_CODE] == 0) {
				return (0);
		}
	}

	if (flags & F_ALLERRS) {
		media_error = (rq->es_key == KEY_MEDIUM_ERROR);
	}
	if (!(flags & F_SILENT)) {
		scsi_printerr(ucmd, rq, rqlen);
	}
	if ((rq->es_key != KEY_RECOVERABLE_ERROR) || (flags & F_ALLERRS)) {
		return (-1);
	}
	return (0);
}


/*
 * Execute a uscsi mode sense command.
 * This can only be used to return one page at a time.
 * Return the mode header/block descriptor and the actual
 * page data separately - this allows us to support
 * devices which return either 0 or 1 block descriptors.
 * Whatever a device gives us in the mode header/block descriptor
 * will be returned to it upon subsequent mode selects.
 */
int
uscsi_mode_sense(fd, page_code, page_control, page_data, page_size, header)
	int	fd;			/* file descriptor */
	int	page_code;		/* requested page number */
	int	page_control;		/* current, changeable, etc. */
	caddr_t	page_data;		/* place received data here */
	int	page_size;		/* size of page_data */
	struct	scsi_ms_header *header;	/* mode header/block descriptor */
{
	caddr_t			mode_sense_buf;
	struct mode_header	*hdr;
	struct mode_page	*pg;
	int			nbytes;
	struct uscsi_cmd	ucmd;
	union scsi_cdb		cdb;
	int			status;
	int			maximum;
	int			flags;

	assert(page_size >= 0 && page_size < 256);
	assert(page_control == MODE_SENSE_PC_CURRENT ||
		page_control == MODE_SENSE_PC_CHANGEABLE ||
			page_control == MODE_SENSE_PC_DEFAULT ||
				page_control == MODE_SENSE_PC_SAVED);
	/*
	 * Allocate a buffer for the mode sense headers
	 * and mode sense data itself.
	 */
	nbytes = sizeof (struct block_descriptor) +
				sizeof (struct mode_header) + page_size;
	nbytes = page_size;
	if ((mode_sense_buf = malloc((u_int) nbytes)) == NULL) {
		eprint("cannot malloc %ld bytes\n", nbytes);
		return (-1);
	}

	/*
	 * Use F_SILENT normally, but if we're in expert
	 * mode, allow all driver messages.
	 */
	flags = (option_msg && diag_msg) ? F_NORMAL : F_SILENT;

	/*
	 * Build and execute the uscsi ioctl
	 */
	(void) memset(mode_sense_buf, 0, nbytes);
	(void) memset((char *)&ucmd, 0, sizeof (ucmd));
	(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
	cdb.scc_cmd = SCMD_MODE_SENSE;
	FORMG0COUNT(&cdb, (u_char)nbytes);
	cdb.cdb_opaque[2] = page_control | page_code;
	ucmd.uscsi_cdb = (caddr_t) &cdb;
	ucmd.uscsi_cdblen = CDB_GROUP0;
	ucmd.uscsi_bufaddr = mode_sense_buf;
	ucmd.uscsi_buflen = nbytes;
	status = uscsi_cmd(fd, &ucmd, flags);
	if (status) {
		if (option_msg) {
			eprint("Mode sense page 0x%x failed\n",
				page_code);
		}
		free(mode_sense_buf);
		return (-1);
	}

	/*
	 * Verify that the returned data looks reasonabled,
	 * find the actual page data, and copy it into the
	 * user's buffer.  Copy the mode_header and block_descriptor
	 * into the header structure, which can then be used to
	 * return the same data to the drive when issuing a mode select.
	 */
	hdr = (struct mode_header *) mode_sense_buf;
	(void) memset((caddr_t) header, 0, sizeof (struct scsi_ms_header));
	if (hdr->bdesc_length != sizeof (struct block_descriptor) &&
				hdr->bdesc_length != 0) {
		if (option_msg) {
			eprint("\
\nMode sense page 0x%x: block descriptor length %d incorrect\n",
				page_code, hdr->bdesc_length);
			if (diag_msg)
				dump("Mode sense: ", mode_sense_buf,
					nbytes, HEX_ONLY);
		}
		free(mode_sense_buf);
		return (-1);
	}
	(void) memcpy((caddr_t) header, mode_sense_buf,
		(int) (sizeof (struct mode_header) + hdr->bdesc_length));
	pg = (struct mode_page *) ((u_long) mode_sense_buf +
		sizeof (struct mode_header) + hdr->bdesc_length);
	if (pg->code != page_code) {
		if (option_msg) {
			eprint("\
\nMode sense page 0x%x: incorrect page code 0x%x\n",
				page_code, pg->code);
			if (diag_msg)
				dump("Mode sense: ", mode_sense_buf,
					nbytes, HEX_ONLY);
		}
		free(mode_sense_buf);
		return (-1);
	}
	/*
	 * Accept up to "page_size" bytes of mode sense data.
	 * This allows us to accept both CCS and SCSI-2
	 * structures, as long as we request the greater
	 * of the two.
	 */
	maximum = page_size - sizeof (struct mode_page) - hdr->bdesc_length;
	if (((int) pg->length) > maximum) {
		if (option_msg) {
			eprint("\
Mode sense page 0x%x: incorrect page length %d - expected max %d\n",
				page_code, pg->length, maximum);
			if (diag_msg)
				dump("Mode sense: ", mode_sense_buf,
					nbytes, HEX_ONLY);
		}
		free(mode_sense_buf);
		return (-1);
	}

	(void) memcpy(page_data, (caddr_t) pg, MODESENSE_PAGE_LEN(pg));

	if (option_msg && diag_msg) {
		char *pc = find_string(page_control_strings, page_control);
		eprint("\nMode sense page 0x%x (%s):\n", page_code,
			pc != NULL ? pc : "");
		dump("header: ", (caddr_t) header,
			sizeof (struct scsi_ms_header), HEX_ONLY);
		dump("data:   ", page_data,
			MODESENSE_PAGE_LEN(pg), HEX_ONLY);
	}

	free(mode_sense_buf);
	return (0);
}


/*
 * Execute a uscsi mode select command.
 */
int
uscsi_mode_select(fd, page_code, options, page_data, page_size, header)
	int	fd;			/* file descriptor */
	int	page_code;		/* mode select page */
	int	options;		/* save page/page format */
	caddr_t	page_data;		/* place received data here */
	int	page_size;		/* size of page_data */
	struct	scsi_ms_header *header;	/* mode header/block descriptor */
{
	caddr_t				mode_select_buf;
	int				nbytes;
	struct uscsi_cmd		ucmd;
	union scsi_cdb			cdb;
	int				status;

	assert(((struct mode_page *) page_data)->ps == 0);
	assert(header->mode_header.length == 0);
	assert(header->mode_header.device_specific == 0);
	assert((options & ~(MODE_SELECT_SP|MODE_SELECT_PF)) == 0);

	/*
	 * Allocate a buffer for the mode select header and data
	 */
	nbytes = sizeof (struct block_descriptor) +
				sizeof (struct mode_header) + page_size;
	if ((mode_select_buf = malloc((u_int) nbytes)) == NULL) {
		eprint("cannot malloc %ld bytes\n", nbytes);
		return (-1);
	}

	/*
	 * Build the mode select data out of the header and page data
	 * This allows us to support devices which return either
	 * 0 or 1 block descriptors.
	 */
	(void) memset(mode_select_buf, 0, nbytes);
	nbytes = sizeof (struct mode_header);
	if (header->mode_header.bdesc_length ==
				sizeof (struct block_descriptor)) {
		nbytes += sizeof (struct block_descriptor);
	}

	/*
	 * Dump the structures if anyone's interested
	 */
	if (option_msg && diag_msg) {
		char *s;
		s = find_string(mode_select_strings,
			options & (MODE_SELECT_SP|MODE_SELECT_PF));
		eprint("\nMode select page 0x%x%s:\n", page_code,
			s != NULL ? s : "");
		dump("header: ", (caddr_t) header,
			nbytes, HEX_ONLY);
		dump("data:   ", (caddr_t) page_data,
			page_size, HEX_ONLY);
	}

	/*
	 * Put the header and data together
	 */
	(void) memcpy(mode_select_buf, (caddr_t) header, nbytes);
	(void) memcpy(mode_select_buf + nbytes, page_data, page_size);
	nbytes += page_size;

	/*
	 * Build and execute the uscsi ioctl
	 */
	(void) memset((char *)&ucmd, 0, sizeof (ucmd));
	(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
	cdb.scc_cmd = SCMD_MODE_SELECT;
	FORMG0COUNT(&cdb, (u_char)nbytes);
	cdb.cdb_opaque[1] = (u_char)options;
	ucmd.uscsi_cdb = (caddr_t) &cdb;
	ucmd.uscsi_cdblen = CDB_GROUP0;
	ucmd.uscsi_bufaddr = mode_select_buf;
	ucmd.uscsi_buflen = nbytes;
	status = uscsi_cmd(fd, &ucmd, F_NORMAL);

	if (status && option_msg) {
		eprint("Mode select page 0x%x failed\n", page_code);
	}

	free(mode_select_buf);
	return (status);
}


/*
 * Execute a uscsi inquiry command and return the
 * resulting data.
 */
int
uscsi_inquiry(fd, inqbuf, inqbufsiz)
	int		fd;
	caddr_t		inqbuf;
	int		inqbufsiz;
{
	struct uscsi_cmd	ucmd;
	union scsi_cdb		cdb;
	struct scsi_inquiry	*inq;
	int			n;
	int			status;

	assert(inqbufsiz >= sizeof (struct scsi_inquiry) &&
		inqbufsiz < 256);

	/*
	 * Build and execute the uscsi ioctl
	 */
	(void) memset((char *)inqbuf, 0, inqbufsiz);
	(void) memset((char *)&ucmd, 0, sizeof (ucmd));
	(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
	cdb.scc_cmd = SCMD_INQUIRY;
	FORMG0COUNT(&cdb, (u_char)inqbufsiz);
	ucmd.uscsi_cdb = (caddr_t) &cdb;
	ucmd.uscsi_cdblen = CDB_GROUP0;
	ucmd.uscsi_bufaddr = (caddr_t) inqbuf;
	ucmd.uscsi_buflen = inqbufsiz;
	status = uscsi_cmd(fd, &ucmd, F_SILENT);
	if (status) {
		if (option_msg) {
			eprint("Inquiry failed\n");
		}
	} else if (option_msg && diag_msg) {
		/*
		 * Dump the inquiry data if anyone's interested
		 */
		inq = (struct scsi_inquiry *) inqbuf;
		n = (int) inq->inq_len + 4;
		n = min(n, inqbufsiz);
		eprint("Inquiry:\n");
		dump("", (caddr_t) inqbuf, n, HEX_ASCII);
	}
	return (status);
}



void
scsi_dump_mode_sense_pages(page_control)
	int			page_control;
{
	struct uscsi_cmd	ucmd;
	union scsi_cdb		cdb;
	char			*msbuf;
	int			nbytes;
	char			*pc_str;
	int			status;
	struct mode_header	*mh;
	char			*p;
	struct mode_page	*mp;
	int			n;
	char			s[16];

	pc_str = find_string(page_control_strings, page_control);

	/*
	 * Allocate memory for the mode sense buffer.
	 */
	nbytes = 255;
	msbuf = (char *) zalloc(nbytes);

	/*
	 * Build and execute the uscsi ioctl
	 */
	(void) memset(msbuf, 0, nbytes);
	(void) memset((char *)&ucmd, 0, sizeof (ucmd));
	(void) memset((char *)&cdb, 0, sizeof (union scsi_cdb));
	cdb.scc_cmd = SCMD_MODE_SENSE;
	FORMG0COUNT(&cdb, (u_char)nbytes);
	cdb.cdb_opaque[2] = page_control | 0x3f;
	ucmd.uscsi_cdb = (caddr_t) &cdb;
	ucmd.uscsi_cdblen = CDB_GROUP0;
	ucmd.uscsi_bufaddr = msbuf;
	ucmd.uscsi_buflen = nbytes;
	status = uscsi_cmd(cur_file, &ucmd, F_SILENT);
	if (status) {
		eprint("\nMode sense page 0x3f (%s) failed\n",
			pc_str);
	} else {
		eprint("\nMode sense pages (%s):\n", pc_str);
		mh = (struct mode_header *) msbuf;
		nbytes = mh->length - sizeof (struct mode_header) -
				mh->bdesc_length + 1;
		p = msbuf + sizeof (struct mode_header) +
			mh->bdesc_length;
		dump("         ", msbuf, sizeof (struct mode_header) +
			(int)mh->bdesc_length, HEX_ONLY);
		while (nbytes > 0) {
			mp = (struct mode_page *) p;
			n = mp->length + sizeof (struct mode_page);
			nbytes -= n;
			if (nbytes < 0)
				break;
			sprintf(s, "   %3x:  ", mp->code);
			dump(s, p, n, HEX_ONLY);
			p += n;
		}
		if (nbytes < 0) {
			eprint("  Sense data formatted incorrectly:\n");
			dump("    ", msbuf, (int)mh->length+1, HEX_ONLY);
		}
		eprint("\n");
	}

	free(msbuf);
}


void
scsi_printerr(ucmd, rq, rqlen)
	struct uscsi_cmd		*ucmd;
	struct scsi_extended_sense	*rq;
	int				rqlen;
{
	int		blkno;

	switch (rq->es_key) {
	case KEY_NO_SENSE:
		eprint("No sense error");
		break;
	case KEY_RECOVERABLE_ERROR:
		eprint("Recoverable error");
		break;
	case KEY_NOT_READY:
		eprint("Not ready error");
		break;
	case KEY_MEDIUM_ERROR:
		eprint("Medium error");
		break;
	case KEY_HARDWARE_ERROR:
		eprint("Hardware error");
		break;
	case KEY_ILLEGAL_REQUEST:
		eprint("Illegal request");
		break;
	case KEY_UNIT_ATTENTION:
		eprint("Unit attention error");
		break;
	case KEY_WRITE_PROTECT:
		eprint("Write protect error");
		break;
	case KEY_BLANK_CHECK:
		eprint("Blank check error");
		break;
	case KEY_VENDOR_UNIQUE:
		eprint("Vendor unique error");
		break;
	case KEY_COPY_ABORTED:
		eprint("Copy aborted error");
		break;
	case KEY_ABORTED_COMMAND:
		eprint("Aborted command");
		break;
	case KEY_EQUAL:
		eprint("Equal error");
		break;
	case KEY_VOLUME_OVERFLOW:
		eprint("Volume overflow");
		break;
	case KEY_MISCOMPARE:
		eprint("Miscompare error");
		break;
	case KEY_RESERVED:
		eprint("Reserved error");
		break;
	default:
		eprint("Unknown error");
		break;
	}

	eprint(" during %s", scsi_find_command_name(ucmd->uscsi_cdb[0]));

	if (rq->es_valid) {
		blkno = (rq->es_info_1 << 24) | (rq->es_info_2 << 16) |
			(rq->es_info_3 << 8) | rq->es_info_4;
		eprint(": block %d (0x%x) (", blkno, blkno);
		pr_dblock(eprint, (daddr_t)blkno);
		eprint(")");
	}

	eprint("\n");

	if (rq->es_add_len >= 6) {
		eprint("ASC: 0x%x   ASCQ: 0x%x\n",
			rq->es_add_info[ADD_SENSE_CODE],
				rq->es_add_info[ADD_SENSE_QUAL_CODE]);
	}

	if (option_msg && diag_msg) {
		if (rq->es_key == KEY_ILLEGAL_REQUEST) {
			dump("cmd:    ", (caddr_t) ucmd,
				sizeof (struct uscsi_cmd), HEX_ONLY);
			dump("cdb:    ", (caddr_t) ucmd->uscsi_cdb,
				ucmd->uscsi_cdblen, HEX_ONLY);
		}
		dump("sense:  ", (caddr_t) rq, rqlen, HEX_ONLY);
	}
}



/*
 * Return a pointer to a string telling us the name of the command.
 */
char *
scsi_find_command_name(cmd)
	int	cmd;
{
	struct scsi_command_name *c;

	for (c = scsi_command_names; c->command != SCMD_UNKNOWN; c++)
		if (c->command == cmd)
			break;
	return (c->name);
}


/*
 * Return true if we support a particular mode page
 */
int
scsi_supported_page(page)
	int	page;
{
	return (page == 1 || page == 2 || page == 3 || page == 4 ||
		page == 8 || page == 0x38);
}


int
apply_chg_list(pageno, pagsiz, curbits, chgbits, chglist)
	int		pageno;
	int		pagsiz;
	u_char		*curbits;
	u_char		*chgbits;
	struct chg_list	*chglist;
{
	u_char		c;
	int		i;
	int		changed = 0;

	while (chglist != NULL) {
		if (chglist->pageno == pageno &&
				chglist->byteno < pagsiz) {
			i = chglist->byteno;
			c = curbits[i];
			switch (chglist->mode) {
			case CHG_MODE_SET:
				c |= (u_char) chglist->value;
				break;
			case CHG_MODE_CLR:
				c &= (u_char) chglist->value;
				break;
			case CHG_MODE_ABS:
				c = (u_char) chglist->value;
				break;
			}
			/*
			 * Figure out which bits changed, and
			 * are marked as changeable.  If this
			 * result actually differs from the
			 * current value, update the current
			 * value, and note that a mode select
			 * should be done.
			 */
			if (((c ^ curbits[i]) & chgbits[i]) != 0) {
				c &= chgbits[i];
				if (c != curbits[i]) {
					curbits[i] = c;
					changed = 1;
				}
			}
		}
		chglist = chglist->next;
	}

	return (changed);
}


/*
 * Search a string list for a particular value.
 * Return the string associated with that value.
 */
char *
find_string(slist, match_value)
	slist_t		*slist;
	int		match_value;
{
	for (; slist->str != NULL; slist++) {
		if (slist->value == match_value) {
			return (slist->str);
		}
	}

	return ((char *) NULL);
}


/*
 * Dump a structure in hexadecimal and optionally, ascii,
 * for diagnostic purposes
 */
#define	BYTES_PER_LINE		16

void
dump(hdr, src, nbytes, format)
	char	*hdr;
	caddr_t	src;
	int	nbytes;
	int	format;
{
	int	i;
	int	n;
	char	*p;
	char	s[256];

	assert(format == HEX_ONLY || format == HEX_ASCII);

	strcpy(s, hdr);
	for (p = s; *p; p++) {
		*p = ' ';
	}

	p = hdr;
	while (nbytes > 0) {
		eprint("%s", p);
		p = s;
		n = min(nbytes, BYTES_PER_LINE);
		for (i = 0; i < n; i++) {
			eprint("%02x ", src[i] & 0xff);
		}
		if (format == HEX_ASCII) {
			for (i = BYTES_PER_LINE-n; i > 0; i--) {
				eprint("   ");
			}
			eprint("    ");
			for (i = 0; i < n; i++) {
				eprint("%c",
					isprint(src[i]) ? src[i] : '.');
			}
		}
		eprint("\n");
		nbytes -= n;
		src += n;
	}
}


#define	GETG0COUNT(cdb)		((cdb)->g0_count0 & 0xff)
#define	GETG1COUNT(cdb)		((((cdb)->g1_count1 << 8) & 0xff) + \
				((cdb)->g1_count0 & 0xff))

int
scsi_maptodkcmd(fd, ucmd, flags)
	int			fd;
	struct uscsi_cmd	*ucmd;
	int			flags;
{
	union scsi_cdb		*cdb;
	struct dk_cmd		dkcmd;
	struct scsi_defect_hdr	*hdr;
	short			g1;

	cdb = (union scsi_cdb *) ucmd->uscsi_cdb;

	(void) memset((char *)&dkcmd, 0, sizeof (struct dk_cmd));

	dkcmd.dkc_cmd = cdb->scc_cmd;
	dkcmd.dkc_bufaddr = ucmd->uscsi_bufaddr;
	dkcmd.dkc_buflen = ucmd->uscsi_buflen;

	switch (dkcmd.dkc_cmd) {
	case SCMD_INQUIRY:
		assert(ucmd->uscsi_cdblen == CDB_GROUP0);
		break;

	case SCMD_MODE_SELECT:
		/*
		 * Sun4 driver interface does not allow us
		 * to control the MODE_SELECT_PF bit,
		 * unfortunately.
		 */
		assert(ucmd->uscsi_cdblen == CDB_GROUP0);
		dkcmd.dkc_blkno = 0;
		if (cdb->cdb_opaque[1] & MODE_SELECT_SP) {
			dkcmd.dkc_blkno |= 0x80;
		}
		break;

	case SCMD_MODE_SENSE:
		assert(ucmd->uscsi_cdblen == CDB_GROUP0);
		dkcmd.dkc_blkno = cdb->cdb_opaque[2] & 0xff;
		break;

	case SCMD_READ_DEFECT_LIST:
		assert(ucmd->uscsi_cdblen == CDB_GROUP1);
		hdr = (struct scsi_defect_hdr *) dkcmd.dkc_bufaddr;
		hdr->descriptor = cdb->cdb_opaque[2] & 0x1f;
		break;

	case SCMD_REASSIGN_BLOCK:
		assert(ucmd->uscsi_cdblen == CDB_GROUP0);
		break;

	case SCMD_READ:
	case SCMD_WRITE:
	case SCMD_READ|SCMD_GROUP1:
	case SCMD_WRITE|SCMD_GROUP1:
		dkcmd.dkc_cmd &= ~SCMD_GROUP1;
		g1 = (ucmd->uscsi_cdblen == CDB_GROUP1);
		dkcmd.dkc_secnt = (g1 ? GETG1COUNT(cdb) : GETG0COUNT(cdb));
		dkcmd.dkc_blkno = (g1 ? GETG1ADDR(cdb) : GETG0ADDR(cdb));
		assert (dkcmd.dkc_buflen == (SECSIZE * dkcmd.dkc_secnt));
		break;

	default:
		eprint("Unsupported SCSI command (dkcmd): 0x%x\n",
			dkcmd.dkc_cmd);
		fullabort();
	}

	return (scsi_dkcmd(fd, &dkcmd, flags));
}


int
scsi_dkcmd(fd, dkcmd, flags)
	int		fd;
	struct dk_cmd	*dkcmd;
	int		flags;
{
	struct dk_diag	diag;
	struct dk_err	*errptr;
	int		status;

	/*
	 * Set function flags for driver.
	 */
	dkcmd->dkc_flags = DK_ISOLATE | DK_DIAGNOSE;
	if (flags & F_SILENT) {
		dkcmd->dkc_flags |= DK_SILENT;
	}

	status = ioctl(fd, DKIOCSCMD, dkcmd);
	if (status == 0) {
		return (status);
	}

	status = ioctl(fd, DKIOCGDIAG, &diag);
	if (status || (diag.dkd_severe == DK_NOERROR)) {
		eprint("unexpected ioctl error from sd");
		return (-1);
	}
	errptr = scsi_dk_finderr(diag.dkd_errno);
	if (flags & F_ALLERRS) {
		if (errptr->errtype == DK_ISMEDIA) {
			media_error = 1;
		} else {
			media_error = 0;
		}
	}
	if (!(flags & F_SILENT)) {
		scsi_dk_printerr(diag.dkd_severe, errptr->errtype, errptr->errmsg,
			dkcmd->dkc_cmd, (u_long) diag.dkd_errsect);
	}
	if ((diag.dkd_severe == DK_FATAL) || (flags & F_ALLERRS)) {
		return (-1);
	}
	return (0);
}


void
scsi_dk_printerr(severity, type, msg, cmd, blkno)
	u_char	severity, type;
	char	*msg;
	u_short	cmd;
	u_long	blkno;
{

	eprint("\015Block %d  (",blkno);
	pr_dblock(eprint, (daddr_t)blkno);
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
		eprint("), Unknown severity value.\n");
		fullabort();
		break;
	}
	if (type == DK_ISMEDIA) {
		eprint("media error");
	} else {
		eprint("non-media error");
	}

	eprint(" (%s) during %s\n", msg, scsi_find_command_name(cmd));
}



/*
 * List of possible errors.
 */
struct dk_err dk_errors[] = {
	SC_NO_SENSE, 			ERRLVL_FAULT, 	DK_NONMEDIA,
	"no sense info",

	SC_RECOVERABLE_ERROR, 		ERRLVL_RETRY, 	DK_ISMEDIA,
	"soft error",

	SC_NOT_READY,			ERRLVL_RETRY,	DK_NONMEDIA,
	"not ready",

	SC_MEDIUM_ERROR,		ERRLVL_RETRY,	DK_ISMEDIA,
	"media error",

	SC_HARDWARE_ERROR,		ERRLVL_RETRY,	DK_NONMEDIA,
	"hardware error",

	SC_ILLEGAL_REQUEST,		ERRLVL_FATAL,	DK_NONMEDIA,
	"illegal request",

	SC_UNIT_ATTENTION,		ERRLVL_RETRY,	DK_NONMEDIA,
	"unit attention",

	SC_WRITE_PROTECT,		ERRLVL_FATAL,	DK_NONMEDIA,
	"write protected",

	SC_BLANK_CHECK,			ERRLVL_FATAL,	DK_NONMEDIA,
	"blank check",

	SC_VENDOR_UNIQUE,		ERRLVL_FATAL,	DK_NONMEDIA,
	"vendor unique",

	SC_COPY_ABORTED,		ERRLVL_FATAL,	DK_NONMEDIA,
	"copy aborted",

	SC_ABORTED_COMMAND,		ERRLVL_FATAL,	DK_NONMEDIA,
	"aborted command",

	SC_EQUAL,			ERRLVL_FATAL,   DK_NONMEDIA,
	"equal error",

	SC_VOLUME_OVERFLOW,		ERRLVL_FATAL,	DK_NONMEDIA,
	"volume overflow",

	SC_MISCOMPARE,			ERRLVL_FATAL,   DK_NONMEDIA,
	"miscompare error",

	SC_RESERVED,			ERRLVL_FATAL,   DK_NONMEDIA,
	"reserved",

	SC_ERR_UNKNOWN,			ERRLVL_FATAL,	DK_NONMEDIA,
	"unknown error",
};


/*
 * Returns a pointer to the error struct for the error.
 */
struct dk_err *
scsi_dk_finderr(errno)
	register u_char errno;
{
	struct dk_err	*e;

	for (e= dk_errors; e->errno != SCMD_UNKNOWN; e++)
		if (e->errno == errno)
			break;
	return (e);
}
