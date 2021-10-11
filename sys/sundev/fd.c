/*
 * Copyright 1988, 1989, 1990 by Sun Microsystems, Inc.
 */

#ident "@(#)fd.c 1.1 92/07/30"
#include "fd.h"
#if	NFD > 0

/*
 *  IBM PC AT  Compatible Floppy disk driver
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sundev/fdreg.h>
#include <machine/psl.h>
#include <sys/vm.h>
#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>

#include <machine/cpu.h>

#include <sundev/mbvar.h>
#include <sys/fcntl.h>

/* Sony MP-F17W-50D Drive Parameters
 *				Double
 *
 *	Capacity unformatted	2Mb
 *	Capacity formatted	???
 *	Recording density	17434 bpi
 *	Track density		135 tpi
 *	Cylinders		80
 *	Tracks			160
 *	Encoding method		MFM
 *	Rotational speed	300 rpm
 *	Transfer rate		250/500 kbps
 *	Latency (average)	??? ms
 *	Access time
 *	  Average		??? ms
 *	  Track to track	3 ms
 *	  Head settling time	15 ms
 *	Head load time		??? ms
 *	Motor start time	500 ms
 *	R/W heads		2
 */


extern int nfd;
extern struct  fdstate fd[];        /* Floppy drive state */
extern struct  unit fdunits[];      /* Unit information */
extern struct  mb_device *fddinfo[];
extern struct  mb_ctlr *fdcinfo[];

struct fdraw_old {
        u_char  fr_cmd[9];      /* user-supplied command bytes */
        u_char  fr_result[7];   /* controller-supplied result bytes */
        short   fr_cnum;        /* number of command bytes */
        short   fr_nbytes;      /* number to transfer if read/write command */
        char   *fr_addr;        /* where to transfer if read/write command */
};
#define F_RAW_OLD  _IOWR(F, 1, struct fdraw_old)

struct	fdcstat fdcst;		/* Controller state */
struct	fdraw	fdexec;		/* Commands info and execution results */
struct	buf	fdkbuf;		/* Buffer for driver to build request */
struct	fdc_reg *fdctlr_reg = 0; /* Floppy control registers */

/*
 * floppy disk partition tables
 */
struct fdpartab	quadpart[FDNPART] = {
	{0, 79},
	{79,  1},
	{0, 80},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
	{0, 0},
};

/* Error messages */
static char *fd_ctrerr[] =
{
	"command timeout",
	"status timeout",
	"busy",
};
static char *fd_drverr[] =
{
	"Missing data address mark",
	"Cylinder marked bad",
	"",
	"",
	"Seek error (wrong cylinder)",
	"Uncorrectable data read error",
	"Sector marked bad",
	"",
	"Missing header address mark",
	"Write protected",
	"Sector not found",
	"",
	"Data overrun",
	"Header read error",
	"",
	"",
	"Illegal sector specified",
};

int fdprobe(), fdslave(), fdattach(), fdgo(), fddone(), fdpoll();

struct mb_driver fdcdriver = {
        fdprobe, fdslave, fdattach, fdgo, fddone, fdpoll,
        6, "fd", fddinfo, "fdc", fdcinfo, 0,
};

#define FDDEBUG

#ifdef FDDEBUG
int fd_spurious = 0;

int	fddebug = 0x0;
#define FDDINFO	0x1
#define FDDERR	0x2
#define	FDDINTR	0x4
#define	FDDTIM	0x8

#define fdprintf(flg,x)  if ((flg) & fddebug) printf x
#else
#define fdprintf(flg,x)
#endif FDDEBUG


#define	RATEWR(x)	fdctlr_reg->fdc_control = x
#define	STATUSRD(x)	x = fdctlr_reg->fdc_control
#define	FIFOWR(x)	fdctlr_reg->fdc_fifo = x
#define	FIFORD(x)	x = fdctlr_reg->fdc_fifo
#define CTLWR()		fdctlr_reg->fdc_ctl = fdcst.fd_ctl

/*
#define FDPRI           (PZERO + 1)|PCATCH
*/
#define FDPRI           PRIBIO



/* do probe for floppy controller */
fdprobe(reg, ctlr)
	register struct fdc_reg *reg;
	register int ctlr;
{
	char status;

	fdprintf(FDDINFO,("FDPROBE: reg=0x%x,ctlr=0x%x\n",reg,ctlr));
	status = peekc((char *)&reg->fdc_control);
	fdprintf(FDDINFO,("FDPROBE: status=0x%x\n",status));
	if (status == -1) {
		return(0);
	}
	fdctlr_reg = reg;

	/* reset the controller */
	fdreset();
	return(1);
}

/*ARGSUSED*/
fdslave(md, reg)
        struct mb_device *md;
        caddr_t reg;
{
	fdprintf(FDDINFO,("FDSLAVE: return 1,md=0x%x,reg=0x%x\n",md,reg));
        return (1);     /* non-zero => unit exists */
}

fdattach(md)
        register struct mb_device *md;
{
        register struct unit *un = &fdunits[md->md_unit];
        register struct mb_ctlr *mc = md->md_mc;

	fdprintf(FDDINFO,("FDATTACH: md=0x%x\n",md));
	/* found controller, setup initial drive information */
	fd[md->md_unit].fdd_hst = T50MS;
	fd[md->md_unit].fdd_mst = T750MS;

	/* save unit information */
	
	fdctlr_reg->fdc_int = mc->mc_intr->v_vec;
        un->un_md = md;
        un->un_mc = md->md_mc;
	fdcst.fd_pri = un->un_mc->mc_intpri;
} 

#ifdef lint
fdgo()
#else
fdgo(mc)
        register struct mb_ctlr *mc;
#endif
{
        panic("fdgo");          /* ZZZ */
}


/*
 * Handle a polling disk interrupt.
 */

fdpoll()
{
        return(fdintr());
}

#ifdef lint
fddump()
#else
fddump(dev, addr, blkno, nblk)
        dev_t dev;
        caddr_t addr;
        daddr_t blkno, nblk;
#endif
{
        printf("fddump: not implemented\n");
}

fdsize(dev)
        dev_t dev;
{
	int *cyls;

	/* Check for valid unit number */
	if (UNIT(dev) >= nfd) {
		fdprintf(FDDERR,("FDSIZE: invalid unit=0x%x\n",UNIT(dev)));
		return(-1);
	}
	cyls = (int *) &(fd[UNIT(dev)].fdd_cylsiz);
	if (*cyls == 0) {
		*cyls = SECPCYL;
	}
	fdprintf(FDDINFO,("FDSIZE: returning 0x%x\n",
		quadpart[PARTITION(dev)].numcyls * *cyls));
	return(quadpart[PARTITION(dev)].numcyls * *cyls);
}

/* open floppy drive */
fdopen(dev, flags)
dev_t	dev;
int	flags;
{
	register struct fdstate *f;
	int  unit;
	int rtn = 0;

	unit = UNIT(dev);

	/* Check for valid unit number */
	if (unit >= nfd) {
		fdprintf(FDDERR,("FDOPEN: invalid unit=0x%x\n",unit));
		u.u_error = ENXIO;
		return(ENXIO);
	}
	f = &fd[unit];

	/* check exclusive open */
	if ((f->fdd_status & EXCLUSV) || ((flags & FEXCL) &&
			(f->fdd_status & OPEN))) {
		u.u_error = EBUSY;
		return(EBUSY);
	}
	if (flags & FEXCL)
		f->fdd_status |= EXCLUSV;

	/* check for label */
	if (!(f->fdd_status & OPEN)) {
		f->fdd_device = dev;
		f->fdd_proc = u.u_procp;
		f->fdd_status |= OPEN;
		f->fdd_dtl = 0xFF;
		f->fdd_fil = 0xe5;
		f->fdd_den = DEN_MFM;
		f->fdd_maxerr = NORM_EMAX;
		f->fdd_bps = 2; /* 512 byte sectors */
		f->fdd_gpln= GPLN;
		f->fdd_gplf= GPLF;
		f->fdd_dstep = 0;
		f->fdd_secsiz = NBPSCTR;
		f->fdd_secsft = SCTRSHFT;
		f->fdd_secmsk = NBPSCTR - 1;
		f->fdd_nsides = 2;
		f->fdd_cylskp = quadpart[PARTITION(dev)].startcyl;
		f->fdd_nsects = 18;
		fdsizes(f);
		
		if (!(flags & O_NDELAY)) {
			rtn = fdgetlabel(f, dev);
		}
	}
	return(rtn);
}

/*
 * Read the label from the disk.
 */
static int tr[2]={MFM_R500K, MFM_R250K};

fdgetlabel(f,dev)
	struct fdstate *f;
	dev_t dev;
{
	struct fk_label *l;
	struct buf *bp = &fdkbuf;
	int i, rate_found = 0;
	int oldpri;

	/* acquire fdkbuf */
	oldpri = splr(pritospl(fdcst.fd_pri));
	while (bp-> b_flags&B_BUSY) {
		bp-> b_flags |= B_WANTED;
		if (sleep((caddr_t) bp, FDPRI)) {
			bp-> b_flags &= ~B_WANTED;
			u.u_error = EINTR;
			(void) splx(oldpri);
			return(u.u_error);
		}
        }
	bp->b_flags = B_BUSY;
	bp->b_proc = u.u_procp;
	(void) splx(oldpri);
 
	/* make sure controller is free */
	if (fdqueue(bp)) {
		u.u_error = EIO;
		return(EIO);
	}

	/*
	 * Check reading from floppy
	 */
	l = &(f->fdd_l);
	for (i = 0;i < 2;i++) {
		f->fdd_trnsfr = tr[i];;
		RATEWR(f->fdd_trnsfr & 0x3);
		DELAY(4);
		bp->b_flags = B_BUSY|B_READ|B_PHYS;
		bp->b_resid = 0;
		bp->b_fdcmd = GETLABEL;
		bp->b_dev = dev;
		bp->b_blkno = 1;
		fdcst.fd_bpart = bp->b_bcount = 3;
		fdcst.fd_addr  = (u_long) l;
		bp->b_un.b_addr = (char *) l;
		fdcst.fd_buf = bp;
		fdstart(bp);
		(void) biowait(bp);
		bp->b_flags = 0;
		if (bp->b_resid == 0) {
			rate_found = 1;
			break;
		}
	}
	if (rate_found == 0) {
		f->fdd_trnsfr = tr[0];;
		RATEWR(f->fdd_trnsfr & 0x3);
		DELAY(4);
		l->fkl_type = 0;
		fdprintf(FDDERR,("FDGETLABEL: Couldn't read floppy\n"));
		return(EIO);
	} else {
                /* otherwise we could read a sector - density is known */
                switch (f->fdd_trnsfr) {
                case MFM_R500K:
                        f->fdd_nsects = 18;
                        f->fdd_nsides = 2;
                        break;
                case MFM_R250K:
                        f->fdd_nsects = 9;
                        f->fdd_nsides = 2;
                        break;
                default:
                        /* this should never happen */
                        break;
                }
        }
	fdsizes(f);
	return(0);
}


/* close floppy driver */
fdclose(dev)
	dev_t	dev;
{
	register struct fdstate *f;
	int	unit;

	unit = UNIT(dev);
	f = &fd[unit];

	/* Clear flags for others to open device */
	f->fdd_status &= ~(EXCLUSV | OPEN);
}

int	fdstrategy();

/* Raw device read */
fdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = UNIT(dev);

	if (unit >= nfd) {
		fdprintf(FDDERR,("FDREAD: invalid unit=0x%x\n",unit));
		return (EINVAL);
	}
	return (physio(fdstrategy, &fdunits[unit].un_rtab, dev, B_READ, 
		minphys, uio));
}

/* Raw device write */
fdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = UNIT(dev);

	if (unit >= nfd) {
		fdprintf(FDDERR,("FDWRITE: invalid unit=0x%x\n",unit));
		return (EINVAL);
	}
	return (physio(fdstrategy, &fdunits[unit].un_rtab, dev, B_WRITE, 
		minphys, uio));
}

struct fdraw fdrcs;

int sectab[4] = {128, 256, 512, 1024};

/* Ioctl entry, there are lots of ioctls.  See sun/dkio.h and sundev/fdreg.h */
fdioctl(dev, cmd, arg)
int cmd;
dev_t dev;
caddr_t arg;
{
	register struct buf *bp;
	register secno, entry, i;
	register ushort cylinder;
	register caddr_t bptr;
	struct fdstate	*f;
	struct fdpartab	*fp;
	union io_arg karg;
	ushort head, secsiz;

	struct fdraw_old fdexec_old;

        struct unit *un;
        struct dk_info *inf;
        struct dk_geom *geomp;
        int unit;
	caddr_t argp;

	u.u_error = 0;		/* clear error for now */

	/* check for valid unit */
        unit = UNIT(dev);
        if (unit >= nfd) {
                return (ENXIO);
        }
	f = &fd[unit];
 
        un = &fdunits[unit];
	argp = arg;


	/* Which command did the user specify */
	switch(cmd) {
	/* floppy auto eject */
	case FDKEJECT:
		fdcst.fd_ctl = (1 << (SELSHIFT - unit)) | EJECT;
		CTLWR();
		fdcst.fd_ctl = 0;
		DELAY(1);
		CTLWR();
		break;

	/* DKIOC... commands are generic disk ioctl's */
	case DKIOCINFO:
		inf = (struct dk_info *)arg;
		inf->dki_ctlr = getdevaddr(un->un_mc->mc_addr);
		inf->dki_unit = un->un_md->md_slave;
		inf->dki_ctype = DKC_INTEL82072;
		inf->dki_flags = DKI_FMTVOL;
		break;
	case DKIOCGGEOM:
		geomp = (struct dk_geom *)arg;
		geomp->dkg_ncyl = f->fdd_ncyls;       /* # of data cylinders */
		geomp->dkg_acyl = 1;       /* # of alternate cylinders */
		geomp->dkg_bcyl = 0;       /* cyl offset (for fixed head area) */
		geomp->dkg_nhead = 2;      /* # of heads */
		geomp->dkg_obs1 = 0;       /* obsolete */
		geomp->dkg_nsect = f->fdd_nsects;      /* # of data sectors per track */
		geomp->dkg_intrlv = 1;     /* interleave factor */
		geomp->dkg_obs2 = 0;       /* obsolete */
		geomp->dkg_obs3 = 0;       /* obsolete */
		geomp->dkg_apc = 0;        /* alternates per cyl (SCSI only) */
		geomp->dkg_rpm = 300;        /* revolutions per minute */
		geomp->dkg_pcyl = f->fdd_ncyls;       /* # of physical cylinders */

		break;
	case DKIOCSGEOM:
		fdprintf(FDDERR,("FDIOCTL: DKIOCSGEOM not implemented\n"));
		return (EINVAL);
	case DKIOCGPART:
		{
		struct dk_map *dkmap;

		dkmap = (struct dk_map *)arg;
		fp = &quadpart[PARTITION(dev)];
		dkmap->dkl_cylno = fp->startcyl;
		dkmap->dkl_nblk = fp->numcyls * f->fdd_cylsiz;
		break;
		}
	case DKIOCSPART:
		{
		struct dk_map *dkmap;

		dkmap = (struct dk_map *)arg;
		fp = &quadpart[PARTITION(dev)];
		fp->startcyl = dkmap->dkl_cylno;
		fp->numcyls = (dkmap->dkl_nblk + f->fdd_cylsiz-1) / f->fdd_cylsiz;
		break;
		}
	case DKIOCGAPART:
		{
		struct dk_map *dkmap;

		for (i = 0; i < FDNPART; i++) {
			dkmap = &((struct dk_map *)arg)[i];
			fp = &quadpart[i];
			dkmap->dkl_cylno = fp->startcyl;
			dkmap->dkl_nblk = fp->numcyls * f->fdd_cylsiz;
		}
		}
		break;
	case DKIOCSAPART:
		{
		struct dk_map *dkmap;

		for (i = 0; i < FDNPART; i++) {
			dkmap = &((struct dk_map *)arg)[i];
			fp = &quadpart[i];
			fp->startcyl = dkmap->dkl_cylno;
			fp->numcyls = (dkmap->dkl_nblk + f->fdd_cylsiz-1) /
			    f->fdd_cylsiz;
		}
		break;
		}

	/* Get the characteristics of the floppy disk drive */
	case FDKIOGCHAR:
		{
		struct fdk_char *fdchar;

		fdchar = (struct fdk_char *)arg;

		/* transfer rate */
		switch (f->fdd_trnsfr) {
		    case MFM_R250K:
			fdchar->transfer_rate = 250;
			break;
		    case MFM_R300K:
			fdchar->transfer_rate = 300;
			break;
		    case MFM_R500K:
			fdchar->transfer_rate = 500;
		}
		fdchar->medium = 0;			/* disk medium */
		fdchar->ncyl = f->fdd_ncyls;		/* # of data cylinders */
		fdchar->nhead = f->fdd_nsides; 		/* # of heads */
		fdchar->sec_size = f->fdd_secsiz;	/* sector size */
		fdchar->secptrack = f->fdd_nsects;	/* sectors per track */
		break;
		}

	/* Set the characteristics of floppy disk drive */
	case FDKIOSCHAR:
		{
		struct fdk_char *fdchar;

		fdchar = (struct fdk_char *)arg;
		switch (fdchar->transfer_rate) {
		    case 250:
		    case MFM_R250K:
			f->fdd_trnsfr = MFM_R250K;
			break;
		    case 300:
		    case MFM_R300K:
			f->fdd_trnsfr = MFM_R300K;
			break;
		    case 500:
		    case MFM_R500K:
			f->fdd_trnsfr = MFM_R500K;
			break;
		    default:
			return(EIO);
		}
		f->fdd_ncyls = fdchar->ncyl;		/* # of data cylinders */
		f->fdd_nsides = fdchar->nhead; 		/* # of heads */
		f->fdd_secsiz = fdchar->sec_size;	/* sector size */
		f->fdd_nsects = fdchar->secptrack;	/* sectors per track */
		break;
		}

	/*
	 * Send down floppy specific commands.  Used mainly to support VPC
	 * and make a compatible interface with the scsi floppy.
	 */
	case FDKIOCSCMD:
		{
		struct dk_cmd *dkcmd;
		struct uio fdkuio;
		struct iovec iovec;
		int dir = B_READ;
		struct fdraw fdkexec;

		dkcmd = (struct dk_cmd *) &((struct fdk_cmd *)arg)->dcmd;
		fdprintf(FDDINFO,("FDIOCSCMD: cmd=%d\n",dkcmd->dkc_cmd));
		switch (dkcmd->dkc_cmd) {

		    /*
		     * If it's a read/write command, then it should act
		     * like a normal raw read/write. So set up buffer and
		     * uio, and let physio do the rest.
		     */
		    case FKWRITE:
			dir = B_WRITE;
		    case FKREAD:
			iovec.iov_base = dkcmd->dkc_bufaddr;
			iovec.iov_len = dkcmd->dkc_buflen;
			fdkuio.uio_iov = &iovec;
			fdkuio.uio_iovcnt = 1;
			fdkuio.uio_offset = dbtob(dkcmd->dkc_blkno);
			fdkuio.uio_resid = dkcmd->dkc_buflen;
			fdunits[unit].un_rtab.b_resid = 0;
			u.u_error = physio(fdstrategy, &fdunits[unit].un_rtab,
				dev, dir, minphys, &fdkuio);
			break;

		    /* Seek and rezero can be done through the raw interface */
		    case FKSEEK:
			fdkexec.fd_cmdb1 = SEEK;
			fdkexec.fd_cmdb2 = UNIT(dev);
			fdkexec.fd_track = dkcmd->dkc_blkno / f->fdd_cylsiz;
			fdkexec.fr_cnum = 3;
			argp = (caddr_t)&fdkexec;
			goto rawfdcmd;
		    case FKREZERO:
			fdkexec.fd_cmdb1 = REZERO;
			fdkexec.fd_cmdb2 = UNIT(dev);
			fdkexec.fr_cnum = 2;
			argp = (caddr_t)&fdkexec;
			goto rawfdcmd;

		    /* Format using the special raw formatting command */
		    case FKFORMAT_TRACK:
			karg.ia_fmt.start_trk = dkcmd->dkc_blkno / f->fdd_nsects;
			karg.ia_fmt.num_trks = 1;
			karg.ia_fmt.intlv = 1;
			goto formattrack;

		    /* Format unit not implemented at this time */
		    case FKFORMAT_UNIT:
		    default:
			fdprintf(FDDERR,("FDIOCTL:Unknown dkc_cmd 0x%x\n",dkcmd->dkc_cmd));
			break;
		}
		break;
		}
	case FDKGETCHANGE:
		fdprintf(FDDERR,("FDIOCTL:Unsupported FDKGETCHANGE\n"));
		return(EIO);

	/* Special raw formatting command, user supplied format information */
	case V_FORMAT: /* format tracks */
		/*
		 * Get the user's argument.
		 */
		bcopy(arg, (caddr_t)&karg, sizeof(karg));
formattrack:
		/*
		 * Calculate starting head and cylinder numbers.
		 */
		head = karg.ia_fmt.start_trk % f->fdd_nsides;
		cylinder = karg.ia_fmt.start_trk / f->fdd_nsides;

		/*
		 * Get encoded bytes/sector from table.
		 */
		for (secsiz = 0; secsiz < sizeof(sectab); secsiz++)
			if (sectab[secsiz] == f->fdd_secsiz)
				break;
		bp = geteblk((int) f->fdd_nsects * sizeof(struct fdformid));

		/*
		 * Format all the requested tracks.
		 */
		for (i = 0; i < karg.ia_fmt.num_trks; i++) {
			if ((cylinder >= f->fdd_ncyls) || (head >= f->fdd_nsides)) {
				u.u_error = EINVAL;
				brelse(bp);
				return(u.u_error);
			}

			/*
			 * Initialize the buffer.
			 */
			bp->b_bcount = f->fdd_nsects * sizeof(struct fdformid);
			bzero(bp->b_un.b_addr, (u_int) bp->b_bcount);

			/*
			 * Build the format data.  For each sector, we have to
			 * have 4 bytes: cylinder, head, sector, and encoded 
			 * sector size.
			 */
			entry = 0;
			secno = 1;  /* 1-based for DOS */
			do {
				bptr = &bp->b_un.b_addr[entry];
				if (bptr[2] == '\0') {
					*bptr++ = cylinder;
					*bptr++ = head;
					*bptr++ = secno++;
					*bptr = secsiz;
					entry = (entry + karg.ia_fmt.intlv * sizeof(struct fdformid))
								% bp->b_bcount;
				} else
					entry += sizeof(struct fdformid);
			} while (secno <= f->fdd_nsects);

			f->fdd_maxerr = FORM_EMAX;
			bp->b_dev = dev;
			bp->b_fdcmd = FORMAT;
			bp->b_flags = B_BUSY | B_PHYS;
			if (fdqueue(bp)) {
                                break;
                        }
			fdstart(bp);
			(void) biowait(bp);
			if (bp->b_flags & B_ERROR) {
				u.u_error = ENXIO;
				break;
			}
			if(ISSIG(u.u_procp,1)) {
				bp->b_flags |= B_ERROR;
				u.u_error = EINTR;
				break;
			}

			/* Check for side for cylinder to format */
			if (++head >= f->fdd_nsides) {
				head = 0;
				cylinder++;
			}
		}
		brelse(bp);
		f->fdd_maxerr = NORM_EMAX;
		break;

	/* Raw command.  User specifies the exact controller commands */
	case F_RAW_OLD:
	case F_RAW:
rawfdcmd:
	bp = &fdkbuf;
	{
		int n = UNIT(dev);
		int oldpri;
		int rw = B_READ;
		char *a;
		int c;

		/* wait for the raw command buffer to become free */
        	oldpri = splr(pritospl(fdcst.fd_pri));
        	while (bp->b_flags&B_BUSY) {
                	bp->b_flags |= B_WANTED;
                	if (sleep((caddr_t)bp, FDPRI)) {
                		bp->b_flags &= ~B_WANTED;
				u.u_error = EINTR;
        			(void) splx(oldpri);
				return(u.u_error);
			}
        	}
		bp->b_flags = B_BUSY;
		bp->b_proc = u.u_procp;
        	(void) splx(oldpri);

		/* Put my request on the queue */
		if (fdqueue(bp))
			return(u.u_error);

		/* copy user command buffer into driver command area */
		/*   (Convert from old format to new if necessary)   */
		if (cmd == F_RAW_OLD) {
			bcopy(argp, (caddr_t)&(fdexec_old), sizeof(fdexec_old));
			i = 0;
             		while (i < 9) {
				fdexec.fr_cmd[i] = fdexec_old.fr_cmd[i];
				++i;
			}
			i = 0;
             		while (i < 7) {
				fdexec.fr_result[i] = fdexec_old.fr_result[i];
				++i;
			}
			fdexec.fr_cnum = fdexec_old.fr_cnum;
			fdexec.fr_addr = fdexec_old.fr_addr;
			fdexec.fr_nbytes = fdexec_old.fr_nbytes;
		} else {
			bcopy(argp, (caddr_t)&(fdexec), sizeof(fdexec));
		}

		/* Find the command to execute */
		switch(fdexec.fd_cmdb1 & 0xF) {

		    /* These commands have no data transfers */
		    case SEEK:
		    case SENSE_INT:
		    case READID:
		    case REZERO:
		    case SENSE_DRV:
		    case SPECIFY:
			fdexec.fr_nbytes = 0;
			break;

		    /* Commands which perform write data transfers */
		    case FORMAT:
		    case WRCMD:
		    case WRITEDEL:
			rw = 0;

		    /* Commands which perform read data transfers */
		    case CMD_RD:
		    case READDEL:
		    case READTRACK:
			if (fdexec.fr_nbytes == 0) {
				u.u_error = EINVAL;
				return(u.u_error);
			}
			break;
		    default:
			u.u_error = EINVAL;
			return(u.u_error);
		}

		/*
 		 * What F_RAW's caller thinks is the unit #
		 * doesn't necessarily have anything
 		 * to do with the driver's idea of the
		 * unit #.  Better fix it up now.
 		 * NOTE: SPECIFY is the only command which
		 * uses fd_cmdb2 for anything other
 		 * than head/unit info.
 		 */
		if ( fdexec.fd_cmdb1 != SPECIFY ) {
			fdexec.fd_cmdb2 &= ~0x03;
			fdexec.fd_cmdb2 |= n;
		}
		else
			/* make sure no DMA */
			fdexec.fr_cmd[2] |= 0x01;

		bp->b_flags |= rw;
		bp->b_resid = 0;	/* used for error codes --
					   b_error is same as b_fdcmd */
		bp->b_fdcmd = RAWCMD;	/* so driver can tell */
		bp->b_dev = dev;

		bp->b_flags |= B_PHYS;
		a = bp->b_un.b_addr = fdexec.fr_addr;
		c = fdcst.fd_bpart = bp->b_bcount = fdexec.fr_nbytes;
		if(c){

			/*
			 * Fault in the user data area and lock it
			 * into memory.  Then map in the buffer to
			 * kernel space.
			 */
			if (as_fault(u.u_procp->p_as, a, (u_int)c, 
			    F_SOFTLOCK, (bp->b_flags & B_READ)? 
			    S_WRITE : S_READ) != (faultcode_t)A_SUCCESS)
				panic("fdioctl lock");
			bp_mapin(bp);
		}
		fdcst.fd_addr = (u_long) bp->b_un.b_addr;
		fdstart(bp);
		(void) biowait(bp);
		if(c){

			/*
			 * Unlock the user data area and unmap the
			 * buffer from kernel space.
			 */
			if (as_fault(u.u_procp->p_as,
			    a,(u_int)c, 
			    F_SOFTUNLOCK, (bp->b_flags & B_READ)? 
			    S_WRITE : S_READ) != (faultcode_t)A_SUCCESS)
				panic("fdioctl unlock");
			bp_mapout(bp);
		}

		u.u_error = 0;
		if (fdexec.fr_nbytes)
			fdexec.fr_nbytes = fdcst.fd_curcnt;

		/* Wake up anyone waiting on buffer */
		oldpri = splr(pritospl(fdcst.fd_pri));
                if (bp->b_flags&B_WANTED)
                       	wakeup((caddr_t)bp);
        	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
                (void) splx(oldpri);

		/*  Copy out results to the user                  */
		/*  (Convert from new format to old if necessary) */
		if (cmd == F_RAW_OLD) {
			i = 0;
             		while (i < 9) {
				fdexec_old.fr_cmd[i] = fdexec.fr_cmd[i];
				++i;
			}
			i = 0;
             		while (i < 7) {
				fdexec_old.fr_result[i] = fdexec.fr_result[i];
				++i;
			}
			fdexec_old.fr_cnum = fdexec.fr_cnum;
			fdexec_old.fr_addr = fdexec.fr_addr;
			fdexec_old.fr_nbytes = fdexec.fr_nbytes;
			bcopy((caddr_t)&(fdexec_old), arg, sizeof(fdexec_old));
		} else {
			bcopy((caddr_t)&(fdexec), arg, sizeof(fdexec));
		}
		break;
	}
	default:
	        fdprintf(FDDERR,("FDIOCTL:Unknown ioctl call 0x%x\n",cmd));
		u.u_error = EINVAL;
	}
	return(u.u_error);
}

/* Floppy strategy routine */
fdstrategy(bp)
register struct buf *bp;
{
	register struct fdstate *f;
	int	unit;

	/* Check for valid unit */
	unit = UNIT(bp->b_dev);
	if (unit >= nfd) {
		fdprintf(FDDERR,("FDSTRAT: invalid unit==0x%x\n",unit));
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		iodone(bp);
		return;
	}

	/* Check if we really have any data */
	if (bp->b_bcount == 0) {
		iodone(bp);
		return;
	}
	f = &fd[unit];

	/* Check if block number is bigger then number of available blocks */
	if (bp->b_blkno >= f->fdd_n512b) {
		fdprintf(FDDINFO,("FDSTRAT: blkno error blkno==0x%x,n512b=0x%x\n",bp->b_blkno,f->fdd_n512b));
		if (bp->b_blkno > f->fdd_n512b || (bp->b_flags&B_READ) == 0) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
		}
		bp->b_resid = bp->b_bcount;
		iodone(bp);
		return;
	}

	/* set data transfer command */
	bp->b_fdcmd = (bp->b_flags&B_READ) ? RDCMD : WRCMD;

	/* get on queue to run */
	if (fdqueue(bp) == 0) {

		/* mapin buffer to kernel space */
		bp->b_proc = u.u_procp;
		bp_mapin(bp);

		/* begin data transfer */
		fdstart(bp);
	}
}

/*
 * All requests must check to see if the floppy controller is active.  If so,
 * the get put on the wait queue until it's your turn.
 */
fdqueue(bp)
struct buf *bp;
{
	register int oldpri;
	int unit;

	unit = UNIT(bp->b_dev);
	oldpri = splr(pritospl(fdcst.fd_pri));

	/* wait until this buffer is active */
	while (fdcst.fd_buf != (struct buf *) NULL) {
		fdcst.fd_cstat |= WBUF;
                if (sleep((caddr_t) &fdcst, FDPRI)) {
			fdcst.fd_cstat &= ~WBUF;
			bp->b_flags |= B_ERROR;
			u.u_error = EINTR;
        		(void) splx(oldpri);
			return(u.u_error);
		}
	}
	fdcst.fd_buf = bp;
	(void) splx(oldpri);
	fdcst.fd_curdrv = unit;
	return(0);
}



/* Set up controller to start a floppy command */
/* All commands come through this execution point */
fdstart(bp)
register struct buf *bp;
{
	register int i;
	struct fdstate *f;
	int fdtimer(), fdwait();
	int n = fdcst.fd_curdrv;

	f = &fd[n];

	/*
	 * Raw commands are special cases, there will only be one command
	 * executed, and the results will be passed back to the user.
	 */
	if (bp->b_fdcmd == RAWCMD) {
		RATEWR(f->fdd_trnsfr & 0x3);
		DELAY(4);
		fd[n].fdd_mtimer = RUNTIM;
		/* Select device and start motor */
		if ((1 << (SELSHIFT - n)) != (fdcst.fd_ctl & 0x18)) {
			fdcst.fd_ctl = (1 << (SELSHIFT - n)) | MTRON;
			CTLWR();
			timeout(fdwait, (caddr_t) bp, (int) fd[n].fdd_mst);
               		(void) sleep((caddr_t)&fdcst.fd_ctl, FDPRI);
		}

		if (!(fdcst.fd_cstat & WTIMER)) {
			fdcst.fd_cstat |= WTIMER;
			timeout(fdtimer, (caddr_t)0, MTIME);
		}

		/* Do command, and maybe wait for command completion */
		if (fdio(bp) > 0) {
			fddata(bp);
		}
		return;

	}
	if (bp->b_fdcmd == FORMAT) {

		/* Format track information */
		fdexec.fd_track = ((struct fdformid *)bp->b_un.b_addr)->fdf_track;
		fdexec.fd_head = ((struct fdformid *)bp->b_un.b_addr)->fdf_head;
		fdcst.fd_btot = bp->b_bcount;
	} else {

		/*
		 * Read/write information.  Calculate the disk parameters
		 * to do data transfer, block number, transfer count, track,...
		 */
		i = bp->b_blkno;
		if (i + (bp->b_bcount >> SCTRSHFT) > f->fdd_n512b)
			fdcst.fd_btot = (f->fdd_n512b - i) << SCTRSHFT;
		else
			fdcst.fd_btot = bp->b_bcount;
		fdcst.fd_blkno = ((long)(unsigned)i << SCTRSHFT) >> f->fdd_secsft;
		i = f->fdd_cylsiz;
		fdexec.fd_track = fdcst.fd_blkno / i;
		i = fdcst.fd_blkno % i;
		fdexec.fd_head = i / f->fdd_nsects;

		/* sectors start at 1 */
		fdexec.fd_sector = (i % f->fdd_nsects) + 1;
	}
	if (bp->b_fdcmd == GETLABEL) {
		bp->b_fdcmd = RDCMD;
	    	fdexec.fd_track = 0;
	} else
	    	fdexec.fd_track += f->fdd_cylskp;
	fdcst.fd_addr = (long)(bp->b_un.b_addr);
	bp->b_resid = fdcst.fd_btot;
	RATEWR(f->fdd_trnsfr & 0x3);
	DELAY(4);

	/* Set state to check if drive is recalibrated */
	fdcst.fd_state = CHK_RECAL;
	fddata(bp);
}

/* Interrupt handler */
fdintr()
{
	register struct buf *bp;
	unsigned char msr;
	int fddata();

	STATUSRD(msr);		/* Read status register */

	/*
	 * This is a spurious interrupt if we do not have a valid active
	 * buffer, or if there is no request waiting for an interrupt.
	 */
	if (((bp = fdcst.fd_buf) == NULL) || !(fdcst.fd_cstat & WINTR)) {
#ifdef FDDEBUG
		fd_spurious++;
#endif FDDEBUG
		fdcst.fd_ctl |= TC;
		CTLWR();
		fdcst.fd_ctl &= ~TC;
		CTLWR();
		(void) fdresult(fdexec.fr_result, 7);
		return(1);
	}

	/* Check if we are waiting for a read from FIFO interrupt */
	fd[fdcst.fd_curdrv].fdd_mtimer = RUNTIM;
	if (fdcst.fd_cstat & WREAD) {
		if (msr & IORDY) {
			FIFORD(*fdcst.fd_curaddr++);
			for (fdcst.fd_curcnt--;fdcst.fd_curcnt != 0; fdcst.fd_curcnt--) {

				/*
	 		 	 * Read main status register to see if
				 * OK to read data.
	 		 	 */
				STATUSRD(msr);

				/* Check to see if FIFO is ready */
				if ((msr & (IODIR|IORDY)) != (IODIR|IORDY))
					break;
				FIFORD(*fdcst.fd_curaddr++);
			}

			/*
			 * Is transfer count 0, if so, we are done.
			 * Turn off wait for read flag and toggle the
			 * Terminal Count to let the controller know we
			 * are done.
			 */
			if (fdcst.fd_curcnt == 0) {
				fdcst.fd_cstat &= ~WREAD;
				fdcst.fd_ctl |= TC;
				CTLWR();
				fdcst.fd_ctl &= ~TC;
				CTLWR();
			} else
				return(1);
		} else {
			/*
			fdcst.fd_cstat &= ~WREAD;
			fdcst.fd_state = XFER_DONE;
			*/
			return(0);
		}

	/* Check if we are waiting for a write to FIFO interrupt */
	} else if (fdcst.fd_cstat & WWRITE) {
		if ((msr & (IODIR|IORDY)) == IORDY) {
			FIFOWR(*fdcst.fd_curaddr++);
			for (fdcst.fd_curcnt--;fdcst.fd_curcnt != 0; fdcst.fd_curcnt--) {
				/*
	 		 	 * Read main status register to see if
				 * OK to read data.
	 		 	 */
				STATUSRD(msr);

				/* Check to see if FIFO is ready */
				if ((msr & (IODIR|IORDY)) != IORDY)
					break;
				FIFOWR(*fdcst.fd_curaddr++);
			}

			/*
			 * Is transfer count 0, if so, we are done.
			 * Turn off wait for write flag and toggle the
			 * Terminal Count to let the controller know we
			 * are done.
			 */
			if (fdcst.fd_curcnt == 0) {
				fdcst.fd_cstat &= ~WWRITE;

				/* Format command won't toggle TC */
				if ((fdexec.fd_cmdb1 & 0xf) != FORMAT) {
					fdcst.fd_ctl |= TC;
					CTLWR();
					fdcst.fd_ctl &= ~TC;
					CTLWR();
				} else {
					fdcst.fd_cstat |= WFMT;
					return(1);
				}
				
			} else
				return(1);
		} else {
			if ((msr & (IODIR|IORDY)) == (IODIR|IORDY)) {
				FIFORD(fdexec.fr_result[0]);
				fdcst.fd_cstat |= WWERR; 
			}
			return(1);
		}
	} else  if  (fdcst.fd_cstat & WFMT) {
		STATUSRD(msr);

		/* Check to see if FIFO is ready */
		if ((msr & (IODIR|IORDY)) != (IODIR|IORDY))
			return(1);
		FIFORD(fdexec.fr_result[0]);
		goto fmtdone;
	}

	/*
	 * For seeks and rezeros, we need to do a sense interrupt to clear the
	 * interrupt bit and find out the status of the command.  For read/write
	 * commands, we read the results of the command from the FIFO in the
	 * result phase of the command.
	 */
	if (fdexec.fd_cmdb1 == SEEK) {
		fdsense(fdexec.fd_track);
	} else if (fdexec.fd_cmdb1 == REZERO) {
		fdsense(0);
	} else {
		if (fdresult(fdexec.fr_result, 7))
			fdexec.fd_st0 |= ABNRMTERM;
	}
	
fmtdone:
	/*
	 * We are done with this command, turn of interrupt wait flag, goto
	 * next state, and wake up the process using this buffer.
	 */
	fdcst.fd_cstat &= ~WINTR;
	if (fdcst.fd_state < XFER_DONE)
		fdcst.fd_state++;
	softcall(fddata, (caddr_t) bp);
	return(1);
}
			
/* Check results of data transfer */
fdck_results(bp,f)
register struct buf *bp;
struct fdstate *f;
{
	int oldpri;

	oldpri = splr(pritospl(fdcst.fd_pri));

	fdcst.fd_overrun = 0;

	/* If we transfered all the data, we are done */
	if ((bp->b_resid -= fdcst.fd_bpart) == 0) {
		fddone(bp);
		goto ret;
	}

	/*
	 * Do we have to transfer more data to this track, then continue
	 * transfer.  Otherwise, check if we need to seek to a new track
	 * or just use the ofher head of the disk.
	 */
	if (fdcst.fd_bpart < ((f->fdd_nsects + 1) -
	     fdexec.fd_sector) << f->fdd_secsft) {
		fdexec.fd_sector += fdcst.fd_bpart >> f->fdd_secsft;
		fdcst.fd_state = XFER_BEG; /* stay in data xfer state */
	} else {
		if (++fdexec.fd_head >= f->fdd_nsides) {
			fdexec.fd_head = 0;
			fdexec.fd_track++;
			fdcst.fd_state = CHK_SEEK;  /* go back to seek state */
		} else
			fdcst.fd_state = XFER_BEG; /* stay in data xfer state */
		fdexec.fd_sector = 1;
	}

	/* Adjust the address and block number of transfer */
	fdcst.fd_addr += fdcst.fd_bpart;
	fdcst.fd_blkno += (fdcst.fd_bpart >> f->fdd_secsft);
ret:
        (void) splx(oldpri);
}


/* All done with transfer */
fddone(bp)
register struct buf *bp;
{
	register struct fdstate *f;

	f = &fd[fdcst.fd_curdrv];

	/* Check for error in data transfer */
	f->fdd_errcnt = 0;
	if ((bp->b_resid += (bp->b_bcount - fdcst.fd_btot)) != 0)
		bp->b_flags |= B_ERROR;
	bp->b_error = 0;

	iodone(bp);

	/* mapout the buffer from kernel space */
	bp_mapout(bp);

	/* If someone is queued to execute, wake it up */
	fdcst.fd_buf = (struct buf *) NULL;
	if (fdcst.fd_cstat & WBUF) {
		fdcst.fd_cstat &= ~WBUF;
		wakeup((caddr_t) &fdcst);
	}
}

/* Do sense interrupt for end of seek and rezero */
fdsense(cylnum)
char cylnum;
{
	u_char sense_int = SENSE_INT;

	if (fdexec.fr_result[2] = fdcmd(&sense_int, 1))
		return;
	if (fdexec.fr_result[2] = fdresult(fdexec.fr_result, 2))
		return;
	if (fd[fdcst.fd_curdrv].fdd_dstep)
		cylnum <<= 1;

	/* Check result of the seek or rezero */
	if ((fdexec.fr_result[0] & (INVALID|ABNRMTERM|SEEKEND|EQCHK|NOTRDY)) !=
	    SEEKEND || fdexec.fr_result[1] != cylnum)
		fdexec.fr_result[2] = 1;
}

/* error handling */
fderror(bp,type)
struct buf *bp;
int type;
{
	register struct fdstate *f;
	register int i;
	unsigned char ltrack;
	int error;

	f = &fd[fdcst.fd_curdrv];

	if (type == 1) {
		f->fdd_status |= RECAL;

		/*
		 * If it's a data tranfer state, and it's not a format
		 * command, then we can adjust the command to restart.
		 */
		if (fdcst.fd_state >= XFER_BEG && bp->b_fdcmd != FORMAT) {
			ltrack = fdexec.fd_strack;
			if (f->fdd_dstep)
				ltrack >>= 1;
			if (ltrack == fdexec.fd_track)
				i = fdexec.fd_ssector;
			else
				i = fd[fdcst.fd_curdrv].fdd_nsects + 1;
			i -= fdexec.fd_sector;
			fdexec.fd_track = ltrack;
			fdexec.fd_sector = fdexec.fd_ssector;
			fdcst.fd_blkno += i;
			i <<= f->fdd_secsft;
			fdcst.fd_addr += i;
			bp->b_resid -= i;
		}
	}
	fdprintf(FDDERR,("FDERROR:type=0x%x errcnt=%d maxerr=%d\n",type,f->fdd_errcnt,f->fdd_maxerr));
	/* Are we over the maximum number of errors */
	if (++f->fdd_errcnt > f->fdd_maxerr) {
		if (type == 1) {

			/* find the correct error message */
			if (fdcst.fd_state >= XFER_BEG) {
				f->fdd_lsterr = fdexec.fd_st1;
				error = (fdexec.fd_st1<<8)|fdexec.fd_st2;
				for (i = 0; i < 16 && ((error&01) == 0); i++)
					error >>= 1;
			} else
				i = 4; /* fake a seek error */

			/* print drive error message to user */
			if (bp->b_fdcmd == FORMAT) {
				uprintf("FD  drv %d, trk %d: %s\n",fdcst.fd_curdrv,
				    f->fdd_curcyl, fd_drverr[i]);
			} else {
				uprintf("FD  drv %d, blk %d: %s\n",fdcst.fd_curdrv,
				    fdcst.fd_blkno, fd_drverr[i]);
			}
		} else {

			/* print controller error message use and console */
			printf("FD controller %s\n",fd_ctrerr[type-2]);
			uprintf("FD controller %s\n",fd_ctrerr[type-2]);
		}

		/* All done with error*/
		fddone(bp);
		fdcst.fd_state = FDERROR;
		return(1);
	}

	/* Should we try a reset yet?  Start the request over */
	if (f->fdd_errcnt == TRYRESET)
		fdreset();
	fdcst.fd_state = CHK_RECAL;
	return(0);
}

/*
 * entered after a timeout expires when waiting for
 * floppy drive to perform something that takes too
 * long to busy wait for.
 */
fdwait()
{
	wakeup((caddr_t)&fdcst.fd_ctl);
}



/* Timer popped, check to see if we need to kill request or set timer again. */
fdtimer()
{
	register int i;
	register struct buf *bp;
	int oldpri;

        oldpri = splr(pritospl(fdcst.fd_pri));
	for (i = 0; i < nfd; i++) {
		fdprintf(FDDTIM,("FD_TIMER[%d]=%d,curcnt=0x%x\n",i,
		  fd[i].fdd_mtimer,fdcst.fd_curcnt));
		if (--fd[i].fdd_mtimer == 0) {
			fdcst.fd_cstat &= ~WTIMER;
			fdcst.fd_ctl = 0;
			CTLWR();
			if (((bp = fdcst.fd_buf) == NULL) ||
			    !(fdcst.fd_cstat & WINTR)) {
				goto timerdone;
			}

			/*
			 * Timer is done and request must be hung, turn
			 * off motor  and reset controller and stat info
			 * if this is the correct device.
			 */
			if (i == fdcst.fd_curdrv) {
				unsigned char status = 0;
				if (bp->b_fdcmd == RAWCMD) {
					if ((fdcst.fd_cstat & WREAD) &&
					    (fdcst.fd_curcnt != fdcst.fd_bpart)){
						fdexec.fd_st0 =
						    *(fdcst.fd_curaddr - 1);
						(void) fdresult(&fdexec.fd_st1,6);
					} else if (fdcst.fd_cstat & WWERR) {
						(void) fdresult(&fdexec.fd_st1,6);
						fdcst.fd_cstat &= ~WWERR;
					}
					fdcst.fd_cstat &= ~(WINTR|WREAD|WWRITE);
					fddata(bp);
					goto timerdone;
				}

				if (fdcst.fd_cstat & WWERR) {
					fdprintf(FDDERR,("FDWRITE\n"));
					status = fdexec.fd_st0;
					fdcst.fd_cstat &= ~WWERR;
				} else if ((fdcst.fd_cstat & WREAD) &&
				    (fdcst.fd_curcnt != fdcst.fd_bpart)) {
					fdprintf(FDDERR,("FDREAD\n"));
					status = *(fdcst.fd_curaddr - 1);
				}
				if (status  & ABNRMTERM) {
					fdexec.fd_st0 = status;
					(void) fdresult(&fdexec.fd_st1, 6);
					if (fdexec.fd_st1 == OVRRUN && 
		    		  	      fdcst.fd_overrun < OVERUNLIMIT) {
						fdprintf(FDDERR,("FDOVERRUN:
						  overrun=0x%x limit=%d\n",
			    		  	  fdcst.fd_overrun, OVERUNLIMIT));
						fdcst.fd_overrun++;
						fdcst.fd_state = XFER_BEG;
					} else {
						short err,error;

						error = (fdexec.fd_st1<<8)|fdexec.fd_st2;
						for (err = 0; err < 16 && ((error&01) == 0); err++)
							error >>= 1;
						fdprintf(FDDERR,("FDTIMEOUT,cstat=0x%x,st1=0x%x\n",
					  	fdcst.fd_cstat,fdexec.fd_st1));
						printf("FD  drv %d, blk %d: %s\n",fdcst.fd_curdrv,
				    		fdcst.fd_blkno, fd_drverr[err]);
						fdreset();
						fdcst.fd_state = FDERROR;
						bp->b_flags |= B_ERROR;
					}
				} else {
					fdprintf(FDDERR,("FDTIMEOUT,cstat=0x%x\n",
					  fdcst.fd_cstat));
					fdreset();
					fdcst.fd_state = FDERROR;
					bp->b_flags |= B_ERROR;
				}
				fdcst.fd_cstat &= ~(WINTR|WREAD|WWRITE);
				fddata(bp);
			}
		} else {
			/* Set timer if there is an active request */
			timeout(fdtimer, (caddr_t)0, MTIME);
		}
	}

timerdone:
        (void) splx(oldpri);
}


/* Reset the controller */
fdreset()
{
	register int i;

	fdprintf(FDDINFO,("FDRESET: reseting floppy\n"));

	/* All devices must recalibrate */
	for (i = 0; i < nfd; i++)
		fd[i].fdd_status |= RECAL;
	fdcst.fd_cstat &= ~(WINTR|WREAD|WWRITE);

	/* Do a software reset to controller */
	RATEWR(SWRESET);
	DELAY(4);

	/* Set the default transfer rate */
	RATEWR(fd[fdcst.fd_curdrv].fdd_trnsfr & 0x3);
	DELAY(4);

	/* Configure the controller */
	fdrcs.fd_cmdb1 = CONFIG;
	fdrcs.fr_cmd[1] = 0x64;
	/* implied seek on!! */
	fdrcs.fr_cmd[2] = 0x5d;
	fdrcs.fr_cmd[3] = 0x0;
	(void) fdcmd((u_char *) fdrcs.fr_cmd, 4);

	/* Specify the characteristics */
	fdrcs.fd_cmdb1 = SPECIFY;
	fdrcs.fr_cmd[1] = 0xc2;
	fdrcs.fr_cmd[2] = 0x33;
	(void) fdcmd((u_char *) fdrcs.fr_cmd, 3);
}


/*
 * Error message, called from Unix 5.3 kernel via bdevsw
 */
#ifdef lint
fdprint (str)
char	*str;
{
	fdprint(str);
}
#else
fdprint (dev, str)
dev_t	dev;
char	*str;
{
	printf("fdprint: %s\n", str);
}
#endif

/* Calculate the number of 512byte blocks */
fdsizes(f)
register struct fdstate *f;
{
	ushort nblocks;

	f->fdd_ncyls = TRK80 - f->fdd_cylskp;
	f->fdd_cylsiz = f->fdd_nsides * f->fdd_nsects;
	nblocks = f->fdd_ncyls * f->fdd_cylsiz;
	f->fdd_n512b = ((long)nblocks << f->fdd_secsft) >> SCTRSHFT;
}


/*
 * Routine to return diskette drive status information
 * The diskette controller data register is read the 
 * requested number of times and the results placed in
 * consecutive memory locations starting at the passed
 * address.
 */
fdresult(addr, bcnt)
char *addr;
int	bcnt;
{
	unsigned char	msr;
	register int	i;
	int		ntries;

	for (i = bcnt; i > 0; i--) {
		/*
		 * read main status register to see if OK to read data.
		 */
		ntries = FCRETRY;
		while (TRUE) {
			STATUSRD(msr);
			if ((msr & (IODIR|IORDY)) == (IODIR|IORDY))
				break;
			if (--ntries <= 0) {
				fdprintf(FDDERR,("FDRESULT==>TIMEOUT,msr=0x%x\n",msr));
				fdreset();
				return(RTIMOUT);
			}
			else
				DELAY(60);
		}

		DELAY(10);
		FIFORD(*addr++);
	}
	return(0);
}

/*
 * routine to program a command into the floppy disk controller.
 * the requested number of bytes is copied from the given address
 * into the floppy disk data register.
 */

fdcmd(cmdp, size)
u_char *cmdp;
int	size;
{
	unsigned char	msr;
	register int	i;
	int		ntries;

#ifdef FDDEBUG
	if (fddebug & FDDINFO) {
		printf("fdcmd ");
		for (i = 0; i < size && i < 10; i++)
			printf("%x ", cmdp[i]);
		printf("\n");
	}
#endif FDDEBUG
	for (i = size; i > 0; i--) {
		/*
		 * read main status register to see if OK to write data.
		 */
		ntries = FCRETRY;
		while (TRUE) {
			STATUSRD(msr);
			if ((msr & (IODIR|IORDY)) == IORDY)
				break;
			if (--ntries <= 0) {
				fdprintf(FDDERR,("FDCMD==>TIMEOUT, msr=0x%x\n",msr));
				fdreset();
				return(CTIMOUT);
			}
			else
				DELAY(10);
		}

		DELAY(10);
		FIFOWR(*cmdp++);
	}
	return(0);
}


/* Do the floppy command */
fdio(bp)
register struct buf *bp;
{
	int rnum = CMDWAIT;		/* result information */
	int oldpri;

        oldpri = splr(pritospl(fdcst.fd_pri));
	/* set timer */
	fdcst.fd_cstat |= WINTR;

	/*
	 * Set up buffer address, count, and flags for data transfer commands.
	 * Some commands will not have to wait for an interrupt, read those
	 * results now and return.
	 */
	switch (fdexec.fd_cmdb1 & 0xf) {
	case CMD_RD:
	case READTRACK:
	case READDEL:
		fdprintf(FDDINFO,("READ buf=0x%x cnt=0x%x\n",bp->b_un.b_addr,bp->b_bcount));
		fdprintf(FDDINFO,("     fd_addr=0x%x fd_bpart=0x%x\n",
			fdcst.fd_addr,fdcst.fd_bpart));
		fdcst.fd_cstat |= WREAD;
		fdcst.fd_curcnt = fdcst.fd_bpart;
		fdcst.fd_curaddr = (caddr_t) fdcst.fd_addr;
		break;
	case WRCMD:
	case FORMAT:
	case WRITEDEL:
		fdprintf(FDDINFO,("WRIT buf=0x%x cnt=0x%x\n",bp->b_un.b_addr,bp->b_bcount));
		fdprintf(FDDINFO,("     fd_addr=0x%x fd_bpart=0x%x\n",
			fdcst.fd_addr,fdcst.fd_bpart));
		fdcst.fd_cstat |= WWRITE;
		fdcst.fd_curcnt = fdcst.fd_bpart;
		fdcst.fd_curaddr = (caddr_t) fdcst.fd_addr;
		break;
	/* no interrupt will be received in these cases */
	case SENSE_INT:
		rnum = 2;
		break;
	case SPECIFY:
		rnum = 0;
		break;
	case SENSE_DRV:
		rnum = 1;
		break;
	}
	if (fdcmd((u_char *) fdexec.fr_cmd, fdexec.fr_cnum)) { /* controller error */
		bp->b_flags |= B_ERROR;
		bp->b_resid = EIO;
		rnum = EIO;
		goto get_out;
	}

	/* Don't wait for interrupt, read results and return */
	if (rnum != CMDWAIT) {
		fdcst.fd_cstat &= ~(WINTR|WREAD|WWRITE);
		if (fdresult(fdexec.fr_result, rnum)) {
			bp->b_flags |= B_ERROR;
			bp->b_resid = EIO;	/* controller error */
			rnum = EIO;
		} else {
			rnum = 1;
		}
	} else {
		rnum = 0;
	}
get_out:
        (void) splx(oldpri);
	return(rnum);
}


/* Perform the series of commands to do data transfer */
fddata(bp)
register struct buf *bp;
{
	int retval = 0;
	int	fdwait();
	int	n = fdcst.fd_curdrv;
	struct fdstate *f;
	int fdtimer();

#ifdef FDDEBUG
	if (fd_spurious) {
		fdprintf(FDDINTR,("FD: spurious interrupts %d\n",fd_spurious));
		fd_spurious = 0;
	}
#endif FDDEBUG
	f = &fd[n];
	if (bp->b_fdcmd == RAWCMD) {
		if (fdcst.fd_cstat & WFMT) {
			if (fdresult(&fdexec.fr_result[1], 6))
				fdexec.fd_st0 |= ABNRMTERM;
			fdcst.fd_cstat &= ~WFMT;
		}
		/* That's all for raw command, trannsfer is done */
		fdcst.fd_state = XFER_DONE;
		fdcst.fd_overrun = 0;
		f->fdd_errcnt = 0;
		iodone(bp);

		/* If someone is queued to execute, wake it up */
		fdcst.fd_buf = (struct buf *) NULL;
		if (fdcst.fd_cstat & WBUF) {
			fdcst.fd_cstat &= ~WBUF;
			wakeup((caddr_t) &fdcst);
		}
		return;
	}
	fd[n].fdd_mtimer = RUNTIM;
	/* Select device and start motor */
	if ((1 << (SELSHIFT - n)) != (fdcst.fd_ctl & 0x18)) {
		fdcst.fd_ctl = (1 << (SELSHIFT - n)) | MTRON;
		CTLWR();
		timeout(fddata, (caddr_t) bp, (int) fd[n].fdd_mst);
		return;
	}
	if (!(fdcst.fd_cstat & WTIMER)) {
		fdcst.fd_cstat |= WTIMER;
		timeout(fdtimer, (caddr_t)0, MTIME);
	}

	/* set device number and head in 2 byte of command buffer */
	if (fdcst.fd_cstat & WFMT) {
		if (fdresult(&fdexec.fr_result[1], 6))
			fdexec.fd_st0 |= ABNRMTERM;
		fdcst.fd_cstat &= ~WFMT;
	}

xfer_again:
        fdexec.fd_cmdb2 = (fdexec.fd_head<<2)|fdcst.fd_curdrv;
	/* Do all needed states until transfer is done, state is XFER_DONE */
		switch(fdcst.fd_state) {
		case CHK_RECAL: /* check for rezero */
			if (f->fdd_status & RECAL) {

				/* Set up REZERO command */
				fdexec.fd_cmdb1 = REZERO;
				fdexec.fr_cnum = 2;
				break;
			}
			goto seek;
		case RECAL_DONE: /* come here for REZERO completion */
			if (fdexec.fr_result[2]) {

				/* Error during REZERO */
				retval = 1;
				break;
			}

			/* REZERO worked, check if we need to do seek */
			f->fdd_curcyl = 0;
			f->fdd_status &= ~RECAL;
seek:
			fdcst.fd_state = CHK_SEEK;
		case CHK_SEEK:

			/* Is our curent track the track given in the command */
			if (f->fdd_curcyl != fdexec.fd_track) {

				/* Set up seek command */
				fdexec.fd_cmdb1 = SEEK;
				fdexec.fr_cnum = 3;
				if (f->fdd_dstep)
					fdexec.fd_track <<= 1;
				break;
			}

			/* Seek not needed, begin data transfer */
			fdcst.fd_state = XFER_BEG;
		case XFER_BEG: /* come here for r/w operation */
			if ((fdexec.fd_cmdb1 = bp->b_fdcmd) == FORMAT) {

				/* Set up format command */
				fdexec.fd_bps = f->fdd_bps;
				fdexec.fd_spt = f->fdd_nsects;
				fdexec.fd_fil = f->fdd_fil;
				fdexec.fd_gap = f->fdd_gplf;
				n = bp->b_bcount;
				fdexec.fr_cnum = 6;
			} else {

				/* Set up normal read/write command */
				if ((n = ((f->fdd_nsects + 1) -
			    	fdexec.fd_sector) << f->fdd_secsft) > bp->b_resid)
					n = bp->b_resid;
				fdexec.fd_lstsec = f->fdd_nsects;
				fdexec.fd_gpl    = f->fdd_gpln;
				fdexec.fd_ssiz   = f->fdd_bps;
				fdexec.fd_dtl    = f->fdd_dtl;
				fdexec.fr_cnum = 9;
			}

			/* set density of disk, MFM or FM */
			fdexec.fd_cmdb1 |= f->fdd_den;

			/* set transfer count */
			if (n > bp->b_resid)
				n = bp->b_resid;
			fdcst.fd_bpart = n;
			break;
		case SEEK_DONE: /* come here for SEEK completion */
			if (f->fdd_dstep)
				fdexec.fd_track >>= 1;
			if (fdexec.fr_result[2]) {

				/* Seek error */
				retval = 1;
				break;
			}

			/* Set current track number to correct track */
			f->fdd_curcyl = fdexec.fd_track;
			fdcst.fd_state = XFER_BEG;

			/* Wait for head settle time */
			timeout(fddata, (caddr_t) bp, (int)f->fdd_hst);
			return;
		case FDERROR:
			fddone(bp);
			return;
		case XFER_DONE:
			fdck_results(bp,f);
			if (fdcst.fd_state != XFER_DONE) {
				goto xfer_again;
			} else {
				return;
			}
		default:
			fdprintf(FDDERR,("** FDIO: Unknown fdcst.fd_state=0x%x\n",fdcst.fd_state));
			retval = 1;
		}
		if (retval) {

			/* There is an error in state*/
			if (fderror(bp,retval) == 1)	/* Check error */
				return;			/* FDERROR, give up */
			retval = 0;			/* else, try again */
			goto xfer_again;
		} else {

			/* Check for errors IO command */
			if ((fdio(bp) > 0) || bp->b_flags & B_ERROR) {
				bp->b_flags |= B_ERROR;
				fdcst.fd_state = FDERROR;
				fddone(bp);
			}
		}
}
#endif	/* NFD > 0 */
