#ifndef lint
static	char sccsid[] = "@(#)ctlr_id.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains the ctlr dependent routines for the IPI ISP-80 (PANTHER) ctlr.
 */
#include "global.h"
#include "analyze.h"
#include <sys/dkbad.h>
#include <sys/buf.h>
#include <sys/param.h>

#include <sundev/ipvar.h>
#include <sundev/ipdev.h>

#include <sundev/idvar.h>
#include "param.h"

/* #define DEBUG */
#define	ALIGN(x, i)	(u_long)(((u_long)x + (u_long)i) & ~(u_long)(i - 1))
#define MAXCYLS		10

extern	int id_rdwr(), id_ck_format();
extern	int id_format(), id_ex_man(), id_ex_cur(), id_wr_cur();
extern	int id_repair();
/*
 * This is the operation vector for the IPI ISP-80 (PANTHER).  It is referenced
 * in init_ctypes.c where the supported controllers are defined.
 */
struct	ctlr_ops idops = {
	id_rdwr,
	id_ck_format,
	id_format,
	id_ex_man,
	id_ex_cur,
	id_repair,
	0,
	id_wr_cur,
};


struct idmdef {
    u_long  cyl;
    u_short hd;
    u_long  offset;
    u_short len;
};

struct idmdef_a  {
	u_short cyl_h;
	u_short cyl_l;
	u_short hd;
	u_short offset_h;
	u_short offset_l;
	u_short len;
};

#define MAN_DEFECT_RW	0x0	/* MANUFACTURER's list (working permenant) */
#define PAN_DEFECT_RW	0x6	/* PANTHER  list       (suspect temporary) */
#define PAN_DEFECT_CR	0xA	/* PANTHER  list       (suspect temporary) */
#define FOR_DEFECT_RW	0x2	/* disk state	       (working temporary) */

#define MAN_DEFECT_F  8
#define PAN_DEFECT_F  2
#define M_P_DEFECT_F  4

/*
 * The following routines are the controller specific routines accessed
 * through the controller ops vector.
 */

/*
 * This routine is used to read/write the disk.  The flags determine
 * what mode the operation should be executed in (silent, diagnostic, etc). 
 */
int
id_rdwr(dir, file, blkno, secnt, bufaddr, flags)
	int	dir, file, secnt, flags;
	daddr_t	blkno;
	caddr_t	bufaddr;
{
	struct	dk_cmd cmdblk;
	u_short opmod;

	/*
	 * Fill in a command block with the command info.
	 */
	opmod		= 1;
	flags		= F_ALLERRS;
	cmdblk.dkc_blkno = blkno;
	cmdblk.dkc_secnt = secnt;
	cmdblk.dkc_bufaddr = bufaddr;
	cmdblk.dkc_buflen = secnt * SECSIZE;
	cmdblk.dkc_flags=  F_ALLERRS;

	if (dir == DIR_READ)
		cmdblk.dkc_cmd = (opmod << 8) | IP_READ;
	else
		cmdblk.dkc_cmd = (opmod << 8) | IP_WRITE;
	/*
	 * Run the command and return status.
	 */
	return (idcmd(file, &cmdblk, flags));
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
id_ck_format()
{

	struct	dk_cmd cmdblk;
	int	status, i;
	u_short opmod;

	opmod = 1;
	for( i=0; i < ncyl; i += ncyl/4) {
		/*
		 * Fill in a command block with the command info. We run the
		 * command in silent mode because the error message is not
		 * important, just the success/failure.
		 */
		cmdblk.dkc_cmd = (opmod << 8) | IP_READ;
		cmdblk.dkc_blkno = (daddr_t)i * nsect * nhead;
		cmdblk.dkc_secnt = 4;
		cmdblk.dkc_bufaddr = cur_buf;
		cmdblk.dkc_buflen = SECSIZE * 4;
		cmdblk.dkc_flags  = F_SILENT;

		status = idcmd(cur_file, &cmdblk, F_SILENT);
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
 * This routine formats a portion of the current disk.
 * This routines grain size is a cylinder.
 */
int
id_format(start, end, dlist)
	daddr_t	start, end;
	struct	defect_list *dlist;
{
	struct	idmdef_a		mdef[16 * DEV_BSIZE + 4];
	struct	idmdef_a		*md, *sav_md;
	struct	defect_entry	*pdef;
	struct	dk_cmd cmdblk;
	int	startcyl, startblkno;
	int	endcyl, count;
	int	cylstoformat;
	int     status;
	int	i;
	u_short	opmod;


	md			= (struct idmdef_a *)ALIGN(mdef, 4);
	sav_md			= md;

#ifdef lint
        md = md;
	pdef = 0;
	pdef = pdef;
#endif

	pdef = dlist->list;

	for (i=0; i < dlist->header.count; i++) {
	    md->cyl_h	=  (pdef->cyl >> 16) & 0xffff;
	    md->cyl_l	=  (pdef->cyl & 0xffff);
	    md->hd	= pdef->head;
	    md->len	= pdef->nbits;
	    md->offset_h = (pdef->bfi >> 16) & 0xffff;
	    md->offset_l = pdef->bfi & 0xffff;

	    pdef++; md++;
	}



	opmod			= PAN_DEFECT_CR;
	cmdblk.dkc_cmd		= (opmod << 8) | IP_WRITE_DEFLIST;

	cmdblk.dkc_flags	= 0;

	cmdblk.dkc_blkno	= 0;
	cmdblk.dkc_bufaddr	= (char *)sav_md;
	cmdblk.dkc_buflen	= i * (sizeof (struct idmdef_a)); 
	cmdblk.dkc_secnt	= 16;

	status = idcmd(cur_file, &cmdblk, 0);

	if (status == -1) {
	    fprintf(stderr, "Could not write ISP-80 working LIST\n");
	    return(-1);
	}


	/* start formatting */


	startcyl    =  start / (nsect * nhead);
	startblkno  = startcyl * nsect * nhead;

	if (start != startblkno) {
	    printf("Starting block no. not on cylinder boundry\n");
	    return(-1);
	}

	endcyl	    = end / (nsect * nhead);
	count	    = endcyl - startcyl + 1;

	i = count;

	printf("      ");
	while (count > 0) {

	    cylstoformat	= MIN (count, 3);
				     /* Request the driver and IPI controller
					to format only a few cylinders
					at a time.
					This limits the duration of the IPI
					controller command execution,
					without increasing the overall
					execution time of the format utility.
					If controller command
					execution time becomes too long, and
					there are many drives being formatted
					simultaneously, the driver may
					timeout and consider the controller
					failed.
					This hack avoids fixing the driver
					to understand the IPI controller takes
					different amounts of time, especially
					for format commands. */

	    printf("  %3d%c\b\b\b\b\b\b", (i - count) * 100 /i, '%');
	    (void) fflush(stdout);

	    opmod			= 1;
	    cmdblk.dkc_cmd		= (opmod << 8) | IP_FORMAT;
	    cmdblk.dkc_flags	= 0;
	    cmdblk.dkc_buflen	= 0;
	    cmdblk.dkc_blkno	= startblkno;
	    cmdblk.dkc_secnt	= cylstoformat * (nsect * nhead);
	    status = idcmd(cur_file, &cmdblk, 0);

	    if (status == -1)
		return(-1);
	
	    startblkno	+= (cylstoformat * nsect * nhead);
	    count	-= cylstoformat;

	}
	printf("      \b\b\b\b\b\b");	    /* clean up the field */
	return(0);



}

/*
 * This routine extracts the manufacturer's defect list from the disk.
 */
int
id_ex_man(dlist)
	struct	defect_list *dlist;
{
	struct	idmdef_a		mdef[16 * DEV_BSIZE+4];
	struct	idmdef_a		*md;
	struct	dk_cmd		cmdblk;
	struct	defect_entry	def;
	int			index;
	int			status;
	u_short			opmod;

	opmod			= MAN_DEFECT_RW;
	cmdblk.dkc_cmd		= (opmod << 8) |  IP_READ_DEFLIST;
	cmdblk.dkc_flags	= 0;
	md			= (struct idmdef_a *)ALIGN(mdef, 4);
	cmdblk.dkc_bufaddr	= (char *)md;
	bzero ((char *)md,  (16 * DEV_BSIZE));
	cmdblk.dkc_buflen	= 16 * DEV_BSIZE;
	cmdblk.dkc_secnt	= 16 * DEV_BSIZE; /* eek ! this is in bytes */

	status = idcmd(cur_file, &cmdblk, 0);

	if (status == -1) {
	         fprintf(stderr, "Manufacturers list extraction Failed\n");
	         return(-1);
	}

    	dlist->header.magicno = DEFECT_MAGIC;
	dlist->header.count = 0;
	dlist->list = (struct defect_entry *)zalloc(16 * SECSIZE);


	while (md->len != 0 && md < &mdef[16 * DEV_BSIZE] ) {

		def.cyl	    = (md->cyl_h << 16) | ( md->cyl_l);
		def.head    = md->hd;
		def.bfi    = (md->offset_h << 16) | md->offset_l;
		def.nbits   = md->len;

		index	    = sort_defect(&def, dlist);
		add_def	(&def, dlist, index);

		md++;
	}

	if (dlist->header.count == 0) {	  /* empty defect list is */
		printf("NULL MANUFACTURER'S list: Ignoring this list\n");
		return(-1);		  /* an error condition forcibly */
	}


	return(0);
}

/*
 * This routine extracts the current defect list from the disk.  It does
 * so by reading the ISP-80 (PANTHER) defect list.
 */
int
id_ex_cur(dlist)
	struct	defect_list *dlist;
{
	struct	idmdef_a		mdef[16 * DEV_BSIZE+4];
	struct	idmdef_a		*md;
	struct	dk_cmd		cmdblk;
	struct	defect_entry	def;
	int			index;
	int			status;
	u_short			opmod;

	opmod			= PAN_DEFECT_RW;
	cmdblk.dkc_cmd		= (opmod << 8) | IP_READ_DEFLIST;
	cmdblk.dkc_flags	= 0;
	md			= (struct idmdef_a *)ALIGN(mdef, 4);
	cmdblk.dkc_bufaddr	= (char *)md;
	bzero ((char *)md,  (16 * DEV_BSIZE));
	cmdblk.dkc_buflen	= 16 * DEV_BSIZE;
	cmdblk.dkc_secnt	= 16;

	status = idcmd(cur_file, &cmdblk, 0);

	if (status == -1) {
	         fprintf(stderr, "ISP-80 working list extraction Failed\n");
	         return(-1);
	}

    	dlist->header.magicno = DEFECT_MAGIC;
	dlist->header.count = 0;
	dlist->list = (struct defect_entry *)zalloc(16 * SECSIZE);


	while (md->len != 0 && md < &mdef[16 * DEV_BSIZE] ) {

		def.cyl	    = (md->cyl_h << 16) | ( md->cyl_l);
		def.head    = md->hd;
		def.bfi    = (md->offset_h << 16) | md->offset_l;
		def.nbits   = md->len;

		index	    = sort_defect(&def, dlist);
		add_def	(&def, dlist, index);

		md++;
	}

	if (dlist->header.count == 0) {	  /* empty defect list is */
		printf("NULL ISP-80 working list: Ignoring this list\n");
		return(-1);		  /* an error condition forcibly */
	}

	return(0);
}

/*
 * This routine is used to repair defective sectors.  It is assumed that
 * a higher level routine will take care of adding the defect to the
 * defect list.  The approach is to try to slid the sector, and if that
 * fails map it.
 */
int
id_repair(blkno,flag)
	daddr_t	blkno;
	int	flag;
{
    	struct	dk_cmd		cmdblk;
	u_short			opmod;
	int			status;

#ifdef lint
	flag = flag;
#endif

	opmod			= 1;
	cmdblk.dkc_cmd		= (opmod << 8) | IP_REALLOC;

	cmdblk.dkc_blkno	= blkno;
	cmdblk.dkc_bufaddr	= (char *)0;
	cmdblk.dkc_buflen	= 0;
	cmdblk.dkc_secnt	= 1;
	cmdblk.dkc_flags	= 0;

	status = idcmd(cur_file, &cmdblk, 0);

	if (status == -1) {
	         fprintf(stderr, "Could not repair block %d\n", blkno);
	         return(-1);
	}
	return(0);
	
}
int
id_wr_cur(dlist)
	struct	defect_list *dlist;
{
	struct	idmdef_a		mdef[16 * DEV_BSIZE + 4];
	struct	idmdef_a		*md, *sav_md;
	struct	defect_entry	*pdef;
	struct	dk_cmd cmdblk;
	int     status;
	int	i;
	u_short	opmod;


	md			= (struct idmdef_a *)ALIGN(mdef, 4);
	sav_md			= md;
	(void) bzero((char *)md, 16 * DEV_BSIZE);

#ifdef lint
        md = md;
	pdef = 0;
	pdef = pdef;
#endif

	pdef = dlist->list;

	for (i=0; i < dlist->header.count; i++) {
	    md->cyl_h	=  (pdef->cyl >> 16) & 0xffff;
	    md->cyl_l	=  (pdef->cyl & 0xffff);
	    md->hd	= pdef->head;
	    md->len	= pdef->nbits;
	    md->offset_h = (pdef->bfi >> 16) & 0xffff;
	    md->offset_l = pdef->bfi & 0xffff;

	    pdef++; md++;
	}



	printf("Write ISP-80 working list: count is %d\n", i);
	opmod			= PAN_DEFECT_CR;
	cmdblk.dkc_cmd		= (opmod << 8) | IP_WRITE_DEFLIST;

	cmdblk.dkc_flags	= 0;

	cmdblk.dkc_blkno	= 0;
	cmdblk.dkc_bufaddr	= (char *)sav_md;
	cmdblk.dkc_buflen	= i * (sizeof (struct idmdef_a));
	cmdblk.dkc_secnt	= 16;

	status = idcmd(cur_file, &cmdblk, 0);

	if (status == -1) {
	    fprintf(stderr, "Could not write ISP-80 working LIST\n");
	    return(-1);
	}
	return(0);
}

/* 
 * This routine runs all the commands that use the generic command ioctl
 * interface.  It is the back door into the driver. 
 */
static int
idcmd(file, cmdblk, flags)
	int	file, flags;
	struct	dk_cmd *cmdblk;
{
	int	status;
	struct	dk_diag	    dkdiag;
#ifdef  DEBUG
	printf("cmd %x blkno %d, count %d, bufaddr %x, buflen %x\n",
	    cmdblk->dkc_cmd,
	    cmdblk->dkc_blkno,cmdblk->dkc_secnt,cmdblk->dkc_bufaddr,
	    cmdblk->dkc_buflen);
#endif
#ifdef lint
        flags = flags;
#endif

	cmdblk->dkc_flags = DK_DIAGNOSE;
	status = ioctl(file, DKIOCSCMD, cmdblk);


        if (status == -1) {
	    if ((status = ioctl(file, DKIOCGDIAG, &dkdiag)) == -1) {
		perror("format diag failed");	/* full abort?? */
		exit(-1);
	    }

	    media_error = 0;
	    switch (dkdiag.dkd_errno) {
		case IDE_ERRNO(IDE_CORR):
		case IDE_ERRNO(IDE_UNCORR):
		case IDE_ERRNO(IDE_DATA_RETRIED):
		    media_error= 1;
		    printf("Sector in error %d\n", dkdiag.dkd_errsect);
		    return(-1);
		default:
		    break;
	    }
#ifdef  DEBUG
	    printf("dkd_severe %x dkd_errno %x Media error %x\n",
	      dkdiag.dkd_severe, dkdiag.dkd_errno,  media_error);
#endif
	    perror("Unknown error to format\nAborting\n");
	    exit(1);


	}

	return(status);
}
    
