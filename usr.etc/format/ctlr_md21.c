
#ifndef lint
static	char sccsid[] = "@(#)ctlr_md21.c 1.1 92/07/30 SMI";
#endif	lint

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains the ctlr dependent routines for the md21 ctlr.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/fcntl.h>

#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>
#include <sundev/sdreg.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>

#include "global.h"
#include "startup.h"
#include "scsi_com.h"

extern	int md_rdwr(), md_ck_format(), md_format(), md_ex_man(), 
	md_ex_cur(), md_repair();

extern  int acb_rdwr(), acb_ck_format(); /* cheat by calling acb routines */

struct	ctlr_ops md21ops = {
	md_rdwr,
	md_ck_format,
	md_format,
	md_ex_man,
	md_ex_cur,
	md_repair,
	0,
};

/*
 * Read or write the disk.
 * Just call the acb_routine.  This should really be more
 * cleverly integrated, but this is the easy, sleazy way
 * to do it for now.
 */ 
int
md_rdwr(dir, file, blkno, secnt, bufaddr, flags)
        int     dir, file, secnt, flags;
	daddr_t blkno;
	caddr_t bufaddr;

{
	return (acb_rdwr(dir, file, blkno, secnt, bufaddr, flags));
}
/*
 * Check to see if the disk has been formatted.
 * If we are able to read the first track, we conclude that
 * the disk has been formatted.
 */
int
md_ck_format()
{

	/*
	 * Cheat by calling the acb routine.  Dunno if this really
	 * works...
	 */
	return (acb_ck_format());
}


/*
 * Format the disk, the whole disk, and nothing but the disk.
 */
int
md_format(start, end, list)
	int start, end;				/* irrelevant for us */
	struct defect_list *list;
{
	struct dk_cmd cmdblk;
	struct ms_error_parms {
		struct ccs_modesel_head header;
		struct ccs_err_recovery page1;
	} ms_error_parms;
	struct ms_cache_parms {
		struct ccs_modesel_head header;
		struct ccs_cache page38;
	} ms_cache_parms;
	struct ms_cyl_parms {
		struct ccs_modesel_head header;
		struct ccs_geometry page4;
	} ms_cyl_parms;
	struct ms_reco_parms {
		struct ccs_modesel_head header;
		struct ccs_disco_reco page2;
	} ms_reco_parms;
	struct ms_parms {
		struct ccs_modesel_head header;
		struct ccs_format page3;
	} ms_parms;
	struct  scsi_format_params *format_params;
	struct  defect_entry *defect_entry;
	int	status, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
	int	i;
	int	msg_flag = 0;
	int	num_defects;
	int	bn;

#ifdef	lint
	start = start;
	end = end;
#endif	lint

	/*
	 * Setup default parameters for mode selects.
	 * Note, optimized for MD21.
	 */
	if ((cur_dtype->dtype_options & SUP_ATRKS) == 0)
		cur_dtype->dtype_atrks = nhead;

	if ((cur_dtype->dtype_options & SUP_TRKS_ZONE) == 0)
		cur_dtype->dtype_trks_zone = 1;

	if ((cur_dtype->dtype_options & SUP_ASECT) == 0)
		cur_dtype->dtype_asect = 1;


	/*
	 * SET DISK ERROR RECOVERY PARAMETERS (PAGE 1)
	 */
	bzero((char *)&ms_error_parms, sizeof (ms_error_parms));
	bzero((char *)&cmdblk, sizeof (cmdblk));
	cmdblk.dkc_bufaddr = (caddr_t) &ms_error_parms;
	cmdblk.dkc_buflen = sizeof ms_error_parms;
	cmdblk.dkc_blkno = ERR_RECOVERY_PARMS + 0x80;
	cmdblk.dkc_cmd = SC_MODE_SENSE;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	tmp1 = ms_error_parms.page1.retry_count;

	ms_error_parms.header.medium = 0;
	ms_error_parms.header.reserved1 = 0;
	ms_error_parms.header.block_desc_length = 8;
	ms_error_parms.header.block_length = 512;

	ms_error_parms.page1.reserved = 0;
	ms_error_parms.page1.per = 1;
	ms_error_parms.page1.retry_count = 16;
	if (option_msg) {
		msg_flag = 1;
		printf("\015PAGE 1: retries= %d (%d)\n",
			ms_error_parms.page1.retry_count, tmp1);
	}

	cmdblk.dkc_bufaddr = (caddr_t) &ms_error_parms;
	cmdblk.dkc_buflen = sizeof ms_error_parms;
	cmdblk.dkc_cmd = SC_MODE_SELECT;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	if (status) {
		/* If failed, try not saving mode select params. */
		cmdblk.dkc_blkno = ERR_RECOVERY_PARMS;
		status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	}
	if (status  &&  option_msg) {
		msg_flag = 1;
		eprint("\015Warning: Using default error recovery params.\n\n");
	}


	/*
	 * SET DISK DISCONNECT/RECONNECT PARAMETERS (PAGE 2)
	 */
	bzero((char *)&ms_reco_parms, sizeof (ms_reco_parms));
	cmdblk.dkc_bufaddr = (caddr_t) &ms_reco_parms;
	cmdblk.dkc_buflen = sizeof ms_reco_parms;
	cmdblk.dkc_blkno = DISCO_RECO_PARMS + 0x80;
	cmdblk.dkc_cmd = SC_MODE_SENSE;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	tmp1 = ms_reco_parms.page2.bus_inactivity_limit;

	ms_reco_parms.header.medium = 0;
	ms_reco_parms.header.reserved1 = 0;
	ms_reco_parms.header.block_desc_length = 8;
	ms_reco_parms.header.block_length = 512;

	ms_reco_parms.page2.reserved = 0;
	ms_reco_parms.page2.bus_inactivity_limit = 40;
	if (option_msg) {
		msg_flag = 1;
		printf("PAGE 2: inactivity limit= %d (%d)\n",
			ms_reco_parms.page2.bus_inactivity_limit, tmp1);
	}

	cmdblk.dkc_bufaddr = (caddr_t) &ms_reco_parms;
	cmdblk.dkc_buflen = sizeof ms_reco_parms;
	cmdblk.dkc_cmd = SC_MODE_SELECT;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	if (status) {
		/* If failed, try not saving mode select params. */
		cmdblk.dkc_blkno = DISCO_RECO_PARMS;
		status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	}
	if (status  &&  option_msg) {
		msg_flag = 1;
		eprint("\015Warning: Using default disconnect/reconnect parameters.\n\n");
	}


	/*
	 * SET DISK GEOMETRY PARAMETERS (PAGE 4)
	 */
	bzero((char *)&ms_cyl_parms, sizeof (ms_cyl_parms));
	cmdblk.dkc_bufaddr = (caddr_t) &ms_cyl_parms;
	cmdblk.dkc_buflen = sizeof ms_cyl_parms;
	cmdblk.dkc_blkno = GEOMETRY_PARMS;
	cmdblk.dkc_cmd = SC_MODE_SENSE;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	tmp1 = (ms_cyl_parms.page4.cyl_mb << 8) + ms_cyl_parms.page4.cyl_lb;
	tmp2 = ms_cyl_parms.page4.heads;

	/*
	 * If page 4 disk geometry is different, fix it with a mode select.
	 */
	if ((tmp1 != cur_dtype->dtype_pcyl)  ||
	    (ms_cyl_parms.page4.heads != cur_dtype->dtype_nhead)) {

		ms_cyl_parms.header.medium = 0;
		ms_cyl_parms.header.reserved1 = 0;
		ms_cyl_parms.header.block_desc_length = 8;
		ms_cyl_parms.header.block_length = 512;

		ms_cyl_parms.page4.reserved = 0;
		ms_cyl_parms.page4.cyl_mb = (cur_dtype->dtype_pcyl >> 8) & 0xff;
		ms_cyl_parms.page4.cyl_lb = cur_dtype->dtype_pcyl & 0xff;
		ms_cyl_parms.page4.heads = cur_dtype->dtype_nhead;

		cmdblk.dkc_bufaddr = (caddr_t) &ms_cyl_parms;
		cmdblk.dkc_buflen = sizeof ms_cyl_parms;
		cmdblk.dkc_cmd = SC_MODE_SELECT;
		status = acb_cmd(cur_file, &cmdblk, F_SILENT);
		if (status  &&  option_msg) {
			msg_flag = 1;
			eprint("\015Warning: Using default drive geometry.\n\n");
		}
	}
	if (option_msg) {
		msg_flag = 1;
		printf("PAGE 4: cylinders= %d (%d)    heads= %d (%d)\n",
			cur_dtype->dtype_pcyl, tmp1,
			ms_cyl_parms.page4.heads, tmp2);
	}


	/*
	 * IF ENABLED, SET DISK CACHE PARAMETERS (PAGE 38)
	 */
	if (cur_dtype->dtype_options & SUP_CACHE) {
		bzero((char *)&ms_cache_parms, sizeof (ms_cache_parms));
		cmdblk.dkc_bufaddr = (caddr_t) &ms_cache_parms;
		cmdblk.dkc_buflen = sizeof ms_cache_parms;
		cmdblk.dkc_blkno = CACHE_PARMS + 0x00;
		cmdblk.dkc_cmd = SC_MODE_SENSE;
		status = acb_cmd(cur_file, &cmdblk, F_SILENT);
		tmp1 = ms_cache_parms.page38.mode;
		tmp2 = ms_cache_parms.page38.threshold;
		tmp3 = ms_cache_parms.page38.min_prefetch;
		tmp4 = ms_cache_parms.page38.max_prefetch;

		ms_cache_parms.header.medium = 0;
		ms_cache_parms.header.reserved1 = 0;
		ms_cache_parms.header.block_desc_length = 8;
		ms_cache_parms.header.block_length = 512;
		ms_cache_parms.page38.reserved = 0;


		if (cur_dtype->dtype_options & SUP_CACHE) {
			ms_cache_parms.page38.mode = cur_dtype->dtype_cache;
		}
		if (cur_dtype->dtype_options & SUP_PREFETCH) {
			ms_cache_parms.page38.threshold =
				cur_dtype->dtype_threshold;
		}
		if (cur_dtype->dtype_options & SUP_CACHE_MIN) {
			ms_cache_parms.page38.min_prefetch = 
				cur_dtype->dtype_prefetch_min;
		}
		if (cur_dtype->dtype_options & SUP_CACHE_MAX) {
			ms_cache_parms.page38.max_prefetch = 
				cur_dtype->dtype_prefetch_max;
		}

		if (option_msg) {
			msg_flag = 1;
			printf("PAGE 38: cache mode= 0x%x (0x%x)\n",
				ms_cache_parms.page38.mode, tmp1);
			printf("         min. prefetch multiplier= %d   ",
				ms_cache_parms.page38.min_multiplier);
			printf("max. prefetch multiplier= %d\n",
				ms_cache_parms.page38.max_multiplier);
			printf("         threshold= %d (%d)   ",
				ms_cache_parms.page38.threshold, tmp2);
			printf("min. prefetch= %d (%d)   ",
				ms_cache_parms.page38.min_prefetch, tmp3);
			printf("max. prefetch= %d (%d)\n",
				ms_cache_parms.page38.max_prefetch, tmp4);
		}

		cmdblk.dkc_blkno = CACHE_PARMS + 0x80;
		cmdblk.dkc_bufaddr = (caddr_t) &ms_cache_parms;
		cmdblk.dkc_buflen = sizeof ms_cache_parms;
		cmdblk.dkc_cmd = SC_MODE_SELECT;
		status = acb_cmd(cur_file, &cmdblk, F_SILENT);
		if (status) {
			/* If failed, try not saving mode select params. */
			cmdblk.dkc_blkno = CACHE_PARMS;
			status = acb_cmd(cur_file, &cmdblk, F_SILENT);
		}
		if (status  &&  option_msg) {
			msg_flag = 1;
			eprint("\015Warning: Using default cache params.\n\n");
		}
	}


	/*
	 * SET DISK FORMAT PARAMETERS (PAGE 3)
	 */
	bzero((char *)&ms_parms, sizeof (ms_parms));
	cmdblk.dkc_bufaddr = (caddr_t) &ms_parms;
	cmdblk.dkc_buflen = sizeof (ms_parms);
	cmdblk.dkc_blkno = FORMAT_PARMS + 0x80;
	cmdblk.dkc_cmd = SC_MODE_SENSE;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	tmp1 = ms_parms.page3.track_skew;
	tmp2 = ms_parms.page3.cylinder_skew;
	tmp3 = ms_parms.page3.sect_track;
	tmp4 = ms_parms.page3.tracks_per_zone;
	tmp5 = ms_parms.page3.alt_tracks_vol;
	tmp6 = ms_parms.page3.alt_sect_zone;

	if (cur_dtype->dtype_cyl_skew != 0)
		ms_parms.page3.cylinder_skew = cur_dtype->dtype_cyl_skew;
	if (cur_dtype->dtype_trk_skew != 0)
		ms_parms.page3.track_skew = cur_dtype->dtype_trk_skew;

	ms_parms.header.medium = 0;
	ms_parms.header.reserved1 = 0;
	ms_parms.header.block_desc_length = 8;
	ms_parms.header.block_length = 512;

	ms_parms.page3.reserved = 0;
	ms_parms.page3.tracks_per_zone = cur_dtype->dtype_trks_zone;
	ms_parms.page3.alt_sect_zone = cur_dtype->dtype_asect;
	ms_parms.page3.alt_tracks_vol = cur_dtype->dtype_atrks;
	ms_parms.page3.sect_track = nsect + 1;
	ms_parms.page3.data_sect = 512;
	ms_parms.page3.interleave = 1;

	if (option_msg) {
		msg_flag = 1;
		printf("PAGE 3: trk skew= %d (%d)   cyl skew= %d (%d)   ",
			ms_parms.page3.track_skew, tmp1,
			ms_parms.page3.cylinder_skew, tmp2);
		printf("sects/trk= %d (%d)\n",
			ms_parms.page3.sect_track -1, tmp3 -1);
		printf("        trks/zone= %d (%d)   alt trks= %d (%d)   ",
			ms_parms.page3.tracks_per_zone, tmp4,
			ms_parms.page3.alt_tracks_vol, tmp5);
		printf("alt sects/zone= %d (%d)\n",
			ms_parms.page3.alt_sect_zone, tmp6);
	}

	cmdblk.dkc_bufaddr = (caddr_t) &ms_parms;
	cmdblk.dkc_buflen = sizeof (ms_parms);
	cmdblk.dkc_cmd = SC_MODE_SELECT;
	cmdblk.dkc_blkno = FORMAT_PARMS;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	if (status) {
		msg_flag = 1;
		eprint("\015Warning: Using default mode select parameters.\n");
		eprint("Warning: Drive format may not be correct.\n\n");
	}

	/*
	 * Mash the defect list into the proper format.
	 */

	if (cur_list.list != NULL) {
		format_params = (struct scsi_format_params *)zalloc((list->header.count * 8) + 4);
		if (format_params == 0) {
			eprint("can't zalloc defect list memory\n");
			return(-1);
		}
		defect_entry = list->list;
		for (num_defects = 0 , i = 0; i < list->header.count; i++) {
			if (defect_entry->bfi != -1) {
				format_params->list[i].cyl = defect_entry->cyl;
				format_params->list[i].head = defect_entry->head;
				format_params->list[i].bytes_from_index =
					defect_entry->bfi;
				num_defects++;
			}
			defect_entry++;
		}
		format_params->length = num_defects * 8;
		cmdblk.dkc_buflen = (num_defects * 8) + 4;
	} else {
		format_params = (struct scsi_format_params *)zalloc(4);
		if (format_params == 0) {
			eprint("can't malloc defect list memory\n");
			return(-1);
		}
		cmdblk.dkc_buflen = 4;
		if (option_msg) {
			msg_flag = 1;
			printf("\015Warning: Using internal drive defect list only.\n");
		}
	}
	if (msg_flag)
		printf("\nFormatting...");
	cmdblk.dkc_cmd = SC_FORMAT;
	cmdblk.dkc_bufaddr = (caddr_t) format_params;
	/* cmdblk.dkc_buflen = (list->header.count * 8) + 4; */
	status = acb_cmd(cur_file, &cmdblk, F_NORMAL);
	if (status)
		return (status);
	if (cur_list.list != NULL) {
	    defect_entry = list->list;
	    for (i = 0; i < list->header.count; i++) {
		if (defect_entry->bfi == -1) {
		    bn = chs2bn(defect_entry->cyl,defect_entry->head,defect_entry->sect);
		    status = md_repair(bn,0);
		    if (status)
			return(status);
		}
		defect_entry++;
	    }
	}
	return (status);
}

/*
 * Extract the manufacturer's defect list.
 */
int
md_ex_man(list)
	struct  defect_list *list;
{
	struct dk_cmd cmdblk;
	struct scsi_defect_list def_list;
	struct scsi_defect_hdr *hdr = (struct scsi_defect_hdr *)&def_list;
	int	status;
	int 	len;		/* length of returned defect list in bytes */

	/*
	 * First get length of grown list by asking for the header only.
	 */
	bzero((char *)&cmdblk, sizeof cmdblk);
	bzero((char *)&def_list, sizeof def_list);

/* #ifdef	notdef */
	/* stuff format into header, so we know what kind of list */
	hdr->descriptor = DLD_GROWN_DEF_LIST | DLD_BFI_FORMAT;
	cmdblk.dkc_cmd = SC_READ_DEFECT_LIST;
	cmdblk.dkc_bufaddr = (caddr_t) hdr;
	cmdblk.dkc_buflen = sizeof (struct scsi_defect_hdr);
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	hdr->descriptor = DLD_GROWN_DEF_LIST | DLD_BFI_FORMAT;

	if (status == 0  &&  hdr->length != 0) {
		/* printf(" Grown defect list found\n"); */
		goto STORE_LIST;
	} else if (option_msg) {
		eprint("\nNo grown defect list.\n");
	}
/* #endif	notdef */

	/*
	 * Next try for manufacturer's defect list.
	 */
	/* stuff format into header, so we know what kind of list */
	hdr->descriptor = DLD_MAN_DEF_LIST | DLD_BFI_FORMAT;
	cmdblk.dkc_buflen = sizeof (struct scsi_defect_hdr);
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	hdr->descriptor = DLD_MAN_DEF_LIST | DLD_BFI_FORMAT;

	if (status || hdr->length == 0) {
		if (option_msg)
			eprint("\nNo manufacturer's defect list.\n");
		return (-1);
	}

STORE_LIST:
	len = hdr->length;
	cmdblk.dkc_buflen = len + sizeof (struct scsi_defect_hdr);
	cmdblk.dkc_bufaddr = (caddr_t)zalloc((int)cmdblk.dkc_buflen);
	if (cmdblk.dkc_bufaddr == 0) {
		eprint("can't malloc defect list memory\n");
		return(-1);
	}
	*(struct scsi_defect_hdr *)cmdblk.dkc_bufaddr = *(struct scsi_defect_hdr *)hdr;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);

	if (status) {
		eprint("can't read defect list 2nd time");
		free(cmdblk.dkc_bufaddr);
		return (-1);
	}

	if (len != hdr->length) {
		eprint("not enough defects");
		free(cmdblk.dkc_bufaddr);
		return (-1);
	}
	(void) convert_scsi_list_to_new(list,(struct scsi_defect_list *)cmdblk.dkc_bufaddr,DLD_BFI_FORMAT);
	free(cmdblk.dkc_bufaddr);
	return (0);
}

/*
 * Extract the current defect list.
 * If we find an old format defect list, we should upgrade it
 * to a new list.
 * Just a wrapper for the ACB4000 routine.
 */
int
md_ex_cur(list)
	struct  defect_list *list;
{
	return (acb_ex_cur(list));
}

/*
 * Map a block.
 */
int
md_repair(bn,flag)
	int bn,flag;
{
	struct dk_cmd cmdblk;
#ifdef lint
	flag = flag;
#endif lint
	bzero((caddr_t)&cmdblk, sizeof cmdblk);

	cmdblk.dkc_cmd = SC_REASSIGN_BLOCK;
	cmdblk.dkc_blkno = bn;
	cmdblk.dkc_bufaddr = (caddr_t) 0;
	cmdblk.dkc_buflen = 0;

	return (acb_cmd(cur_file, &cmdblk, F_SILENT));
}

/*
 * Convert a SCSI (MD21) style defect list to our generic format.
 * We can handle different format lists.
 */
static
convert_scsi_list_to_new(list, def_list, list_format)
	struct defect_list *list;
	struct scsi_defect_list *def_list;
	u_short list_format;
{
	register u_short elem_size;
	caddr_t old_defect;
	register struct scsi_bfi_defect *old_bfi;
	register struct defect_entry *new_defect;
	register int old_len;
	register int len;
	int i;

	/*
	 * This is kind of a cheat, but we know that the length of
	 * the header is constant.
	 */
	old_defect = (caddr_t) (def_list->list);
	old_len = def_list->length;

	switch (list_format) {

	case DLD_BFI_FORMAT:
		elem_size = sizeof (struct scsi_bfi_defect);
		break;

	default:
		printf("convert_scsi_list_to_new: can't deal with it\n");
		exit (0);
	}

	/*
	 * Allocate space for the rest of the list.
	 */
	list->header.count = len = old_len / elem_size;
	list->header.magicno = DEFECT_MAGIC;

	list->list = new_defect = (struct defect_entry *)
			zalloc(LISTSIZE(list->header.count) * SECSIZE);

	for (i = 0; i < len; i++, new_defect++, old_defect += elem_size) {
		/* copy 'em in */

		switch (list_format) {

		case DLD_BFI_FORMAT:
			old_bfi = (struct scsi_bfi_defect *)old_defect;
			new_defect->cyl = (short) old_bfi->cyl;
			new_defect->head = (short) old_bfi->head;
			new_defect->bfi = (int) old_bfi->bytes_from_index;
			new_defect->nbits = UNKNOWN;	/* size of defect */
			break;
		/* other cases go here */
		}
	}

	(void)checkdefsum(list, CK_MAKESUM);
}
