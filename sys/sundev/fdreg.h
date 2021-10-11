/* @(#)fdreg.h 1.1 92/07/30 SMI	*/

/*
 * Parameters for PC AT floppy disk driver.
 */

#ifndef	_sundev_fdreg_h
#define	_sundev_fdreg_h
#ident "@(#)fd.h	1.2 - 86/12/16"
/* @(#)fdreg.h	1.4	87/10/22 */

/*
 * the floppy disk minor device number is interpreted as follows:
 *     bits:
 *	 765 43 210
 * 	+---+--+---+
 * 	|RRR|uu|PPP|
 * 	+---+--+---+
 *     codes:
 *	R - Reserved, currently must be 0
 *	u - unit no.
 *	P - partition no.
 */

/* Minor number fields */
#define UNIT(x)		((minor(x) & 0x18) >> 3)
#define	PARTITION(x)	(minor(x) & 0x07)

/* Defines for controller access.  */
#define FD0BSY	0x01    /* drive is seeking */
#define FD1BSY	0x02
#define FD2BSY	0x04
#define FD3BSY	0x08
#define	FCBUSY	0x10	/* controller busy bit */
#define NODMA	0x20    /* controller in non-DMA mode */
#define	IODIR	0x40	/* data reg I/O direction, 1 = read, 0 = write */
#define	IORDY	0x80	/* data register ready to xfer bit */

/* Datarate select register bits */
#define	SWRESET 	0x80	/* Software reset */
#define	PWRDOWN		0x40	/* Power down */
#define	ENAPLL		0x20	/* Enable PLL */
#define	PRECOMP		0x1C	/* Pre-compensation */
#define	DRATESEL	0x03	/* Data rate select */

/* timeout and retry defines */
#define	FCRETRY		2000	/* this many ten microseconds equals 20ms. */
#define	CTIMOUT		0x02	/* Timed out waiting for IORDY in fdcmd */
#define	RTIMOUT		0x03	/* Timed out waiting for IORDY in fdresult*/
#define	NECERR		0x04	/* Controller wont go idle error flag */
#define OPEN_EMAX	1       /* max number of retries during open processing */
#define FORM_EMAX	3       /* max number of retries during format */
#define TRYRESET	3       /* try a reset after this many errrors */
#define NORM_EMAX	5      /* normal max number of retries */
#define OVERUNLIMIT	5	/* normal max number of retries */
#define RUNTIM		4
#define WAITTIM		8
#define LOADTIM		10

/* time defines */
#define MTIME   100	/* no. of clock ticks in one second */
#define ETIMOUT 50	/* no. of clock ticks in 1/2 second */
#define T25MS   3	/* no. of clock ticks in 25 milliseconds */
#define T500MS  50	/* no. of clock ticks in 50 milliseconds */
#define T50MS   5	/* no. of clock ticks in 50 milliseconds */
#define T750MS  75	/* no. of clock ticks in 750 milliseconds */

/* fdd_status flags */
#define OPEN    0x01
#define RECAL   0x02
#define EXCLUSV 0x04

/* fd_state flags */
#define CHK_RECAL   2
#define RECAL_DONE  3
#define CHK_SEEK    4
#define SEEK_DONE   5
#define XFER_BEG    6
#define XFER_DONE   7
#define FDERROR     0xD
#define RESTART     0xF

/* fd_cstat flags */
#define WINTR   0x01
#define WWRITE	0x02
#define WREAD	0x04
#define	WTIMER	0x08
#define	WBUF	0x10
#define WFMT	0x20
#define WWERR	0x40

/* Floppy controller commands */
#define SPECIFY     0x03	/* specify command */
#define SENSE_DRV   0x04        /* read status register 3 */
#define WRCMD       0x05	/* write data */
#define RDCMD       0x26	/* read data with skip */
#define	CMD_RD	    0x06	/* read data */
#define REZERO      0x07	/* move head to track 0 */
#define SENSE_INT   0x08	/* sense interrupt status */
#define WRITEDEL    0x09	/* write deleted data */
#define READID      0x0A	/* read id */
#define	CMD_MTR	    0x0B	/* motor on/off */
#define READDEL     0x0C	/* read deleted data */
#define FORMAT      0x0D	/* format track */
#define SEEK        0x0F	/* seek to track */
#define READTRACK   0x02	/* read entire track */
#define CONFIG	    0x13	/* configure */
#define	CMD_RSEEK   0x8f	/* relative seek */

/* Command set tuneable bits */
#define	CMD_MT		0x80	/* multi-track */
#define CMD_MFM		0x40	/* MFM/FM mode */
#define	CMD_SK		0x20	/* skip flag */
#define	MTR_ON		0x80	/* motor on */
#define	MTRU_SHFT	0x05	/* motor unit shift place */

/* Control register bits */
#define TC		0x1
#define EJECT		0x2
#define	MTRON		0x4
#define	DSEL1		0x8
#define	DSEL0		0x10

#define	SELSHIFT	0x4

/* command for raw controller command tru ioctl */
#define RAWCMD      0x7F

#define INVALID     0x80        /* status register 0 */
#define ABNRMTERM   0x40
#define SEEKEND     0x20
#define EQCHK       0x10
#define NOTRDY      0x08

#define EOCYL       0x80        /* status register 1 */
#define CRCERR      0x20
#define OVRRUN      0x10
#define NODATA      0x04
#define MADR        0x01

#define FAULT       0x80        /* status register 3 */
#define WPROT       0x40
#define RDY         0x20
#define TWOSIDE     0x08

/* NEW_HARDWARE CMOS drive descriptions */
#define DRV_NONE    0x00
#define DRV_DBL     0x01
#define DRV_QUAD    0x02
#define DRV_80QUAD  0x04
#define DRV_80DBL   0x08

/* encodings for the 'fdf_den' field in structure 'fdparam' */
#define DEN_MFM 0x40        /* double density disks */
#define DEN_FM  0x00        /* single density disks */


/* driver ioctl() commands */
#define V_FORMAT   _IOW(V, 5, union io_arg)     /* Format track(s) */

/* format defines */
#define SS8SPT          0xFE    /* single sided 8 sectors per track */
#define DS8SPT          0xFF    /* double sided 8 sectors per track */
#define SS9SPT          0xFC    /* single sided 9 sectors per track */
#define DS9SPT          0xFD    /* double sided 9 sectors per track */
#define DSHSPT          0xF9    /* High density */

/* misc defines */
#define	TRK80	80
#define	TRK40	40
#define GPLN	0x1b
#define	GPLF	0x54
#define	GETLABEL    0xff	/* get format label */
#define CMDWAIT	   -1
#define b_fdcmd b_error
#define NBPSCTR         512     /* Bytes per LOGICAL disk sector. */
#define SCTRSHFT        9       /* Shift for BPSECT.            */
#define SECPCYL		18 * 2
#define TRUE 1

/*
 * partition table for floppy disks
 * we support, 3 different partitions:
 *	a - the first cylinder on the disk
 * 	c - the whole disk;
 *	g - the whole disk minus the first cylinder
 */
#define	FDNPART	8	/* number of partitions supported */
struct fdpartab {
	int	startcyl;	/* cylinder no. where partition starts */
	int	numcyls;	/* number of cylinders in partition */
};

#define	FDNSECT	6	/* no. of sector size/count types supported */

struct fdcstat
{
        unsigned char	fd_curdrv;
        unsigned char	fd_state;
        unsigned char	fd_cstat;
	unsigned char	fd_ctl;
        ushort		fd_overrun;
        ushort		fd_blkno;
        ushort		fd_bpart;
        ushort		fd_btot;
	int		fd_pri;
	struct buf	*fd_buf;
        u_long		fd_addr;
        u_long		fd_curcnt;
        caddr_t		fd_curaddr;
};

struct  unit {
        struct  mb_device *un_md;       /* generic unit */
        struct  mb_ctlr *un_mc;         /* generic controller */
        struct  buf un_rtab;            /* for raw I/O */
};

struct fdstate {
	unsigned char  fdd_status;
	unsigned char  fdd_mtimer;
	unsigned char  fdd_den;
	unsigned char  fdd_trnsfr;

	unsigned char  fdd_hst;
	unsigned char  fdd_mst;
	unsigned char  fdd_dstep;
	unsigned char  fdd_secsft;

	unsigned char  fdd_nsides;
	unsigned char  fdd_nsects;
	unsigned char  fdd_cylsiz;
	unsigned char  fdd_curcyl;

	unsigned char  fdd_bps;
	unsigned char  fdd_gpln;
	unsigned char  fdd_gplf;
	unsigned char  fdd_dtl;

	unsigned char  fdd_fil;
	unsigned char  fdd_lsterr;
	unsigned char  fdd_maxerr;
	unsigned char  fdd_errcnt;

	struct fk_label fdd_l;

	unsigned short fdd_cylskp;
	unsigned short fdd_ncyls;

	unsigned short fdd_secsiz;
	unsigned short fdd_secmsk;

	unsigned short fdd_n512b;
	dev_t	       fdd_device;
	struct   proc *fdd_proc;
};

struct fdformid {
	char    fdf_track;
	char    fdf_head;
	char    fdf_sec;
	char    fdf_secsiz;
};

union   io_arg {
	struct  {
		ushort  ncyl;           /* number of cylinders on drive */
		unsigned char nhead;    /* number of heads/cyl */
		unsigned char nsec;     /* number of sectors/track */
		ushort  secsiz;         /* number of bytes/sector */
		} ia_cd;                /* used for Configure Drive cmd */
	struct  {
		ushort  flags;          /* flags (see below) */
		daddr_t bad_sector;     /* absolute sector number */
		daddr_t new_sector;     /* RETURNED alternate sect assigned */
		} ia_abs;               /* used for Add Bad Sector cmd */
	struct  {
		ushort  start_trk;      /* first track # */
		ushort  num_trks;       /* number of tracks to format */
		ushort  intlv;          /* interleave factor */
		} ia_fmt;               /* used for Format Tracks cmd */
} ;


/* descriptive names for command fields */
#define	fd_cmdb1	fr_cmd[0]
#define	fd_cmdb2	fr_cmd[1]
#define	fd_track	fr_cmd[2]
#define	fd_head		fr_cmd[3]
#define	fd_sector	fr_cmd[4]
#define	fd_ssiz		fr_cmd[5]
#define	fd_lstsec	fr_cmd[6]
#define	fd_gpl		fr_cmd[7]
#define	fd_dtl		fr_cmd[8]
#define fd_bps		fr_cmd[2]
#define fd_spt		fr_cmd[3]
#define fd_gap		fr_cmd[4]
#define fd_fil		fr_cmd[5]

/* descriptive names for result fields */
#define	fd_st0		fr_result[0]
#define	fd_st1		fr_result[1]
#define	fd_st2		fr_result[2]
#define	fd_strack	fr_result[3]
#define	fd_shead	fr_result[4]
#define	fd_ssector	fr_result[5]
#define	fd_ssecsiz	fr_result[6]

/* control register offset */
#define CTLREGOFF       0x400
#define INTREGOFF	0x800

/* registers */
struct fdc_reg {
	unsigned char	fdc_control;			/* Controller register */
	unsigned char	fdc_fifo;			/* FIFO register */
	unsigned char	f1[CTLREGOFF-2];		/* filler */
	unsigned char	fdc_ctl;			/* Control register */
	unsigned char	f2[INTREGOFF-CTLREGOFF-1];	/* filler */
	unsigned char	fdc_int;			/* Interrupt vector */
};

/* Data rate select bits for MFM */
#define	MFM_R1M		3	/* 1 Mbps */
#define MFM_R500K	0	/* 500 Kbps */
#define MFM_R300K	1	/* 300 Kbps */
#define MFM_R250K	2	/* 250 Kbps */

/* Data rate select bits for FM */
#define FM_R250K	0	/* 250 Kbps */
#define FM_R150K	1	/* 150 Kbps */
#define FM_R125K	2	/* 125 Kbps */

/* Debugging define for ioctl */
#define FAKETO  0xffe
#define FAKEINT  0xfff

#endif	/* !_sundev_fdreg_h */
