/*	@(#)mtio.h 1.1 92/07/30 SMI; from UCB 4.10 83/01/17	*/

/*
 * Structures and definitions for mag tape io control commands
 */

/* structure for MTIOCTOP - mag tape op command */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/* operations */
#define	MTWEOF		0	/* write an end-of-file record */
#define	MTFSF		1	/* forward space over file mark */
#define	MTBSF		2	/* backward space over file mark (1/2" only) */
#define	MTFSR		3	/* forward space to inter-record gap */
#define	MTBSR		4	/* backward space to inter-record gap */
#define	MTREW		5	/* rewind */
#define	MTOFFL		6	/* rewind and put the drive offline */
#define	MTNOP		7	/* no operation, sets status only */
#define	MTRETEN		8	/* retension the tape (cartridge tape only) */
#define	MTERASE		9	/* erase the entire tape */
#define	MTEOM		10	/* position to end of media */
#define	MTNBSF		11	/* backward space file to BOF */


/* structure for MTIOCGET - mag tape get status command */
struct	mtget	{
	short	mt_type;	/* type of magtape device */
/* the following two registers are grossly device dependent */
	short	mt_dsreg;	/* ``drive status'' register */
	short	mt_erreg;	/* ``error'' register */
/* optional error info. */
	daddr_t	mt_resid;	/* residual count */
	daddr_t	mt_fileno;	/* file number of current position */
	daddr_t	mt_blkno;	/* block number of current position */
	u_short	mt_flags;
	short	mt_bf;		/* optimum blocking factor */
};

/*
 * values for mt_flags
 */
#define	MTF_SCSI	0x01
#define	MTF_REEL	0x02
#define	MTF_ASF		0x04

/*
 * Constants for mt_type byte
 */
#define	MT_ISTS		0x01		/* vax: unibus ts-11 */
#define	MT_ISHT		0x02		/* vax: massbus tu77, etc */
#define	MT_ISTM		0x03		/* vax: unibus tm-11 */
#define	MT_ISMT		0x04		/* vax: massbus tu78 */
#define	MT_ISUT		0x05		/* vax: unibus gcr */
#define	MT_ISCPC	0x06		/* sun: multibus cpc */
#define	MT_ISAR		0x07		/* sun: multibus archive */
#define	MT_ISSC		0x08		/* sun: SCSI archive */
#define	MT_ISXY		0x09		/* sun: Xylogics 472 */
#define	MT_ISSYSGEN11	0x10		/* sun: SCSI Sysgen, QIC-11 only */
#define	MT_ISSYSGEN	0x11		/* sun: SCSI Sysgen QIC-24/11 */
#define	MT_ISDEFAULT	0x12		/* sun: SCSI default CCS */
#define	MT_ISCCS3	0x13		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISMT02	0x14		/* sun: SCSI Emulex MT02 */
#define	MT_ISVIPER1	0x15		/* sun: SCSI Archive QIC-150 Viper */
#define	MT_ISWANGTEK1	0x16		/* sun: SCSI Wangtek QIC-150 */
#define	MT_ISCCS7	0x17		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS8	0x18		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS9	0x19		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS11	0x1a		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS12	0x1b		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS13	0x1c		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS14	0x1d		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS15	0x1e		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS16	0x1f		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCDC	0x20		/* sun: SCSI CDC 1/2" cartridge */
#define	MT_ISFUJI	0x21		/* sun: SCSI Fujitsu 1/2" cartridge */
#define	MT_ISKENNEDY	0x22		/* sun: SCSI Kennedy 1/2" reel */
#define	MT_ISHP		0x23		/* sun: SCSI HP 1/2" reel */
#define	MT_ISCCS21	0x24		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS22	0x25		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS23	0x26		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS24	0x27		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISEXABYTE	0x28		/* sun: SCSI Exabyte 8mm cartridge */
#define	MT_ISEXB8500	0x29		/* sun: SCSI Exabyte 8500 8mm cart */
#define	MT_ISCCS27	0x2a		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS28	0x2b		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS29	0x2c		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS30	0x2d		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS31	0x2e		/* sun: SCSI generic (unknown) CCS */
#define	MT_ISCCS32	0x2f		/* sun: SCSI generic (unknown) CCS */

/*
 * Device table structure and data for looking tape name from
 * tape id number.  Used by mt.c.
 */
struct mt_tape_info {
	short	t_type;		/* type of magtape device */
	char	*t_name;	/* printing name */
	char	*t_dsbits;	/* "drive status" register */
	char	*t_erbits;	/* "error" register */
};
#define	MT_TAPE_INFO  {\
{ MT_ISCPC,		"TapeMaster 1/2-inch",		TMS_BITS, 0 }, \
{ MT_ISXY,		"Xylogics 472 1/2-inch",	XTS_BITS, 0 }, \
{ MT_ISAR,		"Archive QIC-11",	ARCH_CTRL_BITS,	ARCH_BITS }, \
{ MT_ISSYSGEN11,	"Sysgen QIC-11",		0, 0 }, \
{ MT_ISSYSGEN,		"Sysgen QIC-24",		0, 0 }, \
{ MT_ISMT02,		"Emulex MT-02 QIC-24",		0, 0 }, \
{ MT_ISVIPER1,		"Archive QIC-150",		0, 0 }, \
{ MT_ISWANGTEK1,	"Wangtek QIC-150",		0, 0 }, \
{ MT_ISKENNEDY,		"Kennedy 9612 1/2-inch",	0, 0 }, \
{ MT_ISHP,		"HP 88780 1/2-inch",		0, 0 }, \
{ MT_ISEXABYTE,		"Exabyte EXB-8200 8mm",		0, 0 }, \
{ MT_ISEXB8500,		"Exabyte EXB-8500 8mm",		0, 0 }, \
{ 0 } \
}

/* mag tape io control commands */
#define	MTIOCTOP	_IOW(m, 1, struct mtop)		/* do a mag tape op */
#define	MTIOCGET	_IOR(m, 2, struct mtget)	/* get tape status */

#ifndef KERNEL
/*
 * WARNING --	In 4.1, rmt12 is synonym for nrmt8 --
 *			tape unit 0, no-rewind 1600 BPI.
 *		In 4.1_PSR_A, this drive exists --
 *			tape unit 4, yes-rewind, 1600 BPI.
 * so don't use DEFTAPE.
 */
#define	DEFTAPE	"/dev/rmt12"
#endif

/*
 * Layout of minor device byte
 * 7    6    5    4    3    2    1    0
 * ------------------------------------
 * |    |    |    |    |    |    |----| Unit #. lower 2 bits
 * |    |    |    |    |    |---------- No rewind on close bit....
 * |    |    |    |----|--------------- Density Select
 * |    |    |------------------------- Reserved (additional campus density bit)
 * |    |------------------------------ Unit #. high bit
 * |----------------------------------- Reserved
 */

#define	MTUNIT(dev)	(((minor(dev) & 0x40) >> 4) + (minor(dev) & 0x3))
#define	MT_NOREWIND	(1 <<2)
#define	MT_DENSITY_MASK	(3 <<3)
#define	MT_DENSITY1	(0 <<3)		/* Lowest density/format */
#define	MT_DENSITY2	(1 <<3)
#define	MT_DENSITY3	(2 <<3)
#define	MT_DENSITY4	(3 <<3)		/* Highest density/format */
#define	MTMINOR(unit)	(((unit & 0x04) << 4) + (unit & 0x3))
#define	MT_DENSITY(dev)	((minor(dev) & MT_DENSITY_MASK) >> 3)
