#ifndef lint
static	char sccsid[] = "@(#)ctlr_acb4000.c 1.1 92/07/30 SMI";
#endif	lint

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains the ctlr dependent routines for the acb4000 ctlr.
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

#define NDEFCT           1000	/* gross hack */
#include "global.h"
#include "scsi_com.h"

#define OLD_DEFECT_MAGIC    0xdefe

extern	int acb_rdwr(), acb_ck_format(), acb_format(), acb_ex_man(), 
	acb_ex_cur(), acb_repair();

extern int media_error;

struct	ctlr_ops acb4000ops = {
	acb_rdwr,
	acb_ck_format,
	acb_format,
	acb_ex_man,
	acb_ex_cur,
	acb_repair,
	0,
};


/*
 * List of possible errors.
 */
struct acb_error {
	u_char errno;		/* error number */
	u_char errlevel;	/* error level (corrected, fatal, etc) */
	u_char errtype;		/* error type (media vs nonmedia) */
	char *errmsg;		/* error message */ 
} acb_errors[] = {
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

	SC_ERR_ACB_ERR_UNKNOWN,		ERRLVL_FATAL,	DK_NONMEDIA,
	"unknown error",
};

/*
 * Names of commands.  Must have SC_UNKNOWN at end of list.
 */
struct acb_command_name {
	u_char command;
	char *name;
} acb_command_names[] = {
	SC_TEST_UNIT_READY,	"test unit ready",
	SC_REZERO_UNIT,		"rezero unit",
	SC_SEEK,		"seek",
	SC_REQUEST_SENSE,	"request sense",
	SC_READ,		"read",
	SC_WRITE,		"write",
	SC_FORMAT,		"format",
	SC_UNKNOWN,		"unknown"
};


struct acb_error *acb_finderr();
char *acb_find_command_name();

/*
 * Read or write the disk
 */ 
int
acb_rdwr(dir, file, blkno, secnt, bufaddr, flags)
        int     dir, file, secnt, flags;
	daddr_t blkno;
	caddr_t bufaddr;
{
	struct	dk_cmd cmdblk;

	/*
	 * Build a command block, execute it with acb_cmd().
	 */
	cmdblk.dkc_blkno = blkno;
	cmdblk.dkc_secnt = secnt;
	cmdblk.dkc_bufaddr = bufaddr;
	cmdblk.dkc_buflen = secnt * SECSIZE;
	if (dir == DIR_READ)
		cmdblk.dkc_cmd = SC_READ;
	else
		cmdblk.dkc_cmd = SC_WRITE;
	return (acb_cmd(file, &cmdblk, flags));
}

/*
 * Check to see if the disk has been formatted.
 * If we are able to read the first track, we conclude that
 * the disk has been formatted.
 */
int
acb_ck_format()
{
	struct	dk_cmd cmdblk;
	int	status;

	/*
	 * Try to read the first four blocks.  Need to verify that
	 * this really is a good indicator of formattedness.
	 */
	cmdblk.dkc_cmd = SC_READ;
	cmdblk.dkc_blkno = 0;
	cmdblk.dkc_secnt = 4;
	cmdblk.dkc_bufaddr = cur_buf;
	cmdblk.dkc_buflen = SECSIZE * 4;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	if (status)
		return (0);
	else
		return (1);
}


/*
 * Format the disk, the whole disk, and nothing but the disk.
 * Unlike SMD land, this is an atomic operation on the entire disk.
 */
int
acb_format(start, end, list)
	int start, end;				/* irrelevant for us */
	struct defect_list *list;
{
	struct	dk_cmd cmdblk;
	struct  acb4000_mode_select_parms mode_select_parms;
	struct  scsi_format_params format_params;
	struct  defect_entry *defect_entry;
	int	status;
	int i;

#ifdef	lint
	start = start;
	end = end;
	list = list;
#endif	lint

	bzero((char *)&cmdblk, sizeof cmdblk);
	cmdblk.dkc_cmd = SC_MODE_SELECT;

	bzero((char *)&mode_select_parms, sizeof mode_select_parms);
	mode_select_parms.edl_len = 8;
	mode_select_parms.bsize = SECSIZE;
	mode_select_parms.fmt_code = 1;
	mode_select_parms.ncyl = ncyl + acyl;
	mode_select_parms.nhead = nhead;
	if (cur_ctype->ctype_flags & CF_BSK_WPR) {
		mode_select_parms.rwc_cyl = cur_dtype->dtype_wr_prcmp;
		mode_select_parms.sporc = cur_dtype->dtype_buf_sk;
	}
	cmdblk.dkc_bufaddr = (caddr_t) &mode_select_parms;
	cmdblk.dkc_buflen = SIZEOF_ACB4000_MODE_SELECT_PARMS;
	status = acb_cmd(cur_file, &cmdblk, F_SILENT);
	if (status) {
		/* can't perform mode select, signal error and exit. */
		eprint("mode select failed.\n");
		return (-1);
	}


	bzero((char *)&cmdblk, sizeof cmdblk);
	bzero((char *)&format_params, sizeof format_params);

	/*
	 * Mash the defect list into the proper format.
	 */
	defect_entry = list->list;
	for (i = 0; i < list->header.count && i < ESDI_NDEFECT; i++) {
		format_params.list[i].cyl = defect_entry->cyl;
		format_params.list[i].head = defect_entry->head;
		format_params.list[i].bytes_from_index = defect_entry->bfi;
		defect_entry++;
	}
	format_params.length = list->header.count * 8;
	format_params.reserved = 0;

	cmdblk.dkc_cmd = SC_FORMAT;
	cmdblk.dkc_bufaddr = (caddr_t) &format_params;
	cmdblk.dkc_buflen = (list->header.count * 8) + 4;
	return (acb_cmd(cur_file, &cmdblk, F_NORMAL));
}

/*
 * Extract the manufacturer's defect list.
 * This is a nop for this controller.
 */
int
acb_ex_man(list)
	struct  defect_list *list;
{
#ifdef	lint
	list = list;
#endif	lint

	/*
	 * I'm not sure if this is an error or not:
	 * are we returning a null list, or did we have
	 * trouble trying to read the list.
	 */
	return (-1);
}

/*
 * Extract the current defect list.
 * Looks only for an old format defect list, we upgrade to a new list.
 * A new style list would have been found when we selected the drive.
 */
int
acb_ex_cur(list)
	struct  defect_list *list;
{
	struct	dk_cmd cmdblk;
	struct scsi_format_params *old_defect_list;
	u_short sec_offset;

	bzero((char *)&cmdblk, sizeof cmdblk);
	cmdblk.dkc_cmd = SC_READ;
	cmdblk.dkc_secnt = 2;
	cmdblk.dkc_buflen = SECSIZE * 2;	/* 2 blocks worth */
	cmdblk.dkc_blkno = chs2bn(ncyl, 0, 0);
	cmdblk.dkc_bufaddr = (caddr_t)cur_buf;
	(caddr_t)old_defect_list = (caddr_t)cur_buf;

	/*
	 * Try hard to find the old defect list.
	 */
	for ( sec_offset = 0; sec_offset < nsect; sec_offset += 2) {
		cmdblk.dkc_blkno += 2;
		old_defect_list->reserved = 0;
		if (acb_cmd(cur_file, &cmdblk, F_SILENT)) {
			continue;
		}

		if (old_defect_list->reserved == OLD_DEFECT_MAGIC) {
			convert_old_list_to_new(list,
				(struct scsi_format_params *)cur_buf);
			return (0);
		}
	}
	return (-1);
}

/*
 * "Fix" (map) a block.
 * -- I'm not really sure what this should do.
 */
int
acb_repair(bn,flag)
	int bn,flag;
{
#ifdef	lint
	bn = bn;
	flag = flag;
#endif	lint

	return (-1);

}

/*
 * Execute a command and determine the result.
 */
acb_cmd(file, cmdblk, flags)
	int file, flags;
	struct  dk_cmd *cmdblk;
{
	struct  dk_diag diag;
	struct acb_error *errptr;
	int status;

	/*
	 * Set function flags for driver.
	 */
	cmdblk->dkc_flags = DK_ISOLATE | DK_DIAGNOSE;
	if (flags & F_SILENT)
		cmdblk->dkc_flags |= DK_SILENT;

	status = ioctl(file, DKIOCSCMD, cmdblk);
	if (status == 0)
		return (status);

	status = ioctl(file, DKIOCGDIAG, &diag);
	if (status  ||  (diag.dkd_severe == DK_NOERROR)) {
		eprint("unexpected ioctl error from sd");
		diag.dkd_severe = DK_FATAL;
		return (-1);
	}
	errptr = acb_finderr(diag.dkd_errno);
	if (flags & F_ALLERRS) {
		if (errptr->errtype == DK_ISMEDIA) {
			media_error = 1;
		} else {
			media_error = 0;
		}
	}
	if (!(flags & F_SILENT)) {
		acb_printerr(diag.dkd_severe, errptr->errtype, errptr->errmsg,
		    diag.dkd_errcmd, (u_long) diag.dkd_errsect);
	}
	if ((diag.dkd_severe == DK_FATAL)  ||  (flags & F_ALLERRS)) {
		return (-1);
	}
	return (0);
}


acb_printerr(severity, type, msg, cmd, blkno)
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
	if (option_msg)
		eprint(" (%s) during %s\n", msg, acb_find_command_name(cmd));
	else
		eprint(" (%s)\n", msg);
}

/*
 * This routine converts an old-format to a new-format defect list.
 * In the process, it allocates space for the new list.
 */
int
convert_old_list_to_new(list, old_defect_list)
	struct  defect_list *list;
	struct	scsi_format_params *old_defect_list;
{
	register u_short len = old_defect_list->length / 8;
	register struct scsi_bfi_defect *old_defect = old_defect_list->list;
	register struct defect_entry *new_defect;
	register int i;
	int size;	/* size in blocks of the allocated list */

	/*
	 * Allocate space for the rest of the list.
	 */
	list->header.magicno = DEFECT_MAGIC;
	list->header.count = len;

	size = LISTSIZE(list->header.count);
	list->list = new_defect = (struct defect_entry *)zalloc(size * SECSIZE);

	for (i = 0; i < len; i++, new_defect++, old_defect++) {
		/* copy 'em in */
		new_defect->cyl = (short) old_defect->cyl;
		new_defect->head = (short) old_defect->head;
		new_defect->bfi = (int) old_defect->bytes_from_index;
		new_defect->nbits = UNKNOWN;	/* size of defect */
	}
	(void)checkdefsum(list, CK_MAKESUM);
}

/*
 * Returns a pointer to the error struct for the error.
 */
struct acb_error *
acb_finderr(errno)
	register u_char errno;
{
	register struct acb_error *e;

	for (e= acb_errors; e->errno != SC_ERR_ACB_ERR_UNKNOWN; e++)
		if (e->errno == errno)
			break;
	return (e);
}

/*
 * Return a pointer to a string telling us the name of the command.
 */
char *acb_find_command_name(cmd)
	register u_char cmd;
{
	register struct acb_command_name *c;

	for (c = acb_command_names; c->command != SC_UNKNOWN; c++)
		if (c->command == cmd)
			break;
	return (c->name);
}

/*
 * Parameters of Adaptec bytes from index formula.
 */
#define	FIRST		150
#define	INCR		(SECSIZE + 66)
#define	LAST		(FIRST + (nsect - 1) * INCR)
#define def2bn(def)	(chs2bn(def->cyl,def->head,def->sect))

/*
 * Translate a logical sector to bfi fromat
 */
acb_calc_bfi(def)
	struct defect_entry *def;
{
	struct scsi_bfi_defect def1, def2;

	if (acb_translate(def2bn(def),&def1)) {
		def->cyl = def1.cyl;
		def->head = def1.head;
		def->bfi = def1.bytes_from_index;
		return(0);
	}
	if (!acb_translate(def2bn(def) - 1, &def1)) {
		eprint("Could not translate logical defect to bfi\n");
		return(-1);
	}
	if (!acb_translate(def2bn(def) + 1, &def2)) {
		eprint("Could not translate logical defect to bfi\n");
		return(-1);
	}
	/*
	 * Add a sector to the first.
	 */
	if (def1.bytes_from_index != LAST) {
		def1.bytes_from_index += INCR;
	} else {
		def1.bytes_from_index = FIRST;
		if (def1.head != 0) {
			def1.head --;
		} else {
			def1.head = nhead - 1;
			def1.cyl--;
		}
	}
	/*
	 * Subtract a sector from the second.
	 */
	if (def2.bytes_from_index != FIRST) {
		def2.bytes_from_index -= INCR;
	} else {
		def2.bytes_from_index = LAST;
		if (def2.head != 0) {
			def2.head--;
		} else {
			def2.head = nhead - 1;
			def2.cyl--;
		}
	}
	/*
	 * If equal, we win.
	 */
	if (def1.cyl == def2.cyl && def1.head == def2.head &&
	    def1.bytes_from_index == def2.bytes_from_index) {
		def->cyl = def1.cyl;
		def->head = def1.head;
		def->bfi = def1.bytes_from_index;
		return(0);
	} else {
		eprint("Could not translate logical defect to bfi\n");
		return(-1);
	}
}

/* 
 * Runs the actual command to translate from logical to physical
 */
static
acb_translate(bn,buf)
	struct scsi_bfi_defect *buf;
{
	struct	dk_cmd cmdblk;
	int i;

	/*
	 * Build a command block, execute it with acb_cmd().
	 */
	cmdblk.dkc_blkno = bn;
	cmdblk.dkc_secnt = 0;
	cmdblk.dkc_bufaddr = (caddr_t) buf;
	cmdblk.dkc_buflen = 8;
	cmdblk.dkc_cmd = SC_TRANSLATE;
	for (i = 10; i--; ) {
		if (!acb_cmd(cur_file, &cmdblk, F_SILENT))
			return(1);
	}
	return(0);  /* we failed */
}
