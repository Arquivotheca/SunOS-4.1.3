/*
 * 	label.h
 *      @(#)label.h 1.1 92/07/30 SMI     
 */

#define NCDPART	8

/*
 * geometry of a CD-ROM. The numbers are arbitrary except the size is
 * 640 Mbytes. The rule is:
 * 		CD_NCYL * CD_NHEAD * CD_NSECT = CD_SIZE
 * Also
 *		CD_UPARTSIZE / CD_NSECT = an integer
 */

#define CD_SECSIZE 512		/* sector size of a CD */
#define CD_SIZE	1310720		/* size of a CD. in (512 bytes) sectors */
#define CD_NCYL 2048		/* number of data cylinders */
#define CD_NHEAD 1		/* number of heads */
#define CD_NSECT 640		/* number of sector per track */
#define CD_RPM 350		/* rotations per minute. Average is 350 rpm */

#define CD_UPARTSIZE 33280	/* # of sectors for the UFS partitions */
#define MAXBOOTSIZE (CD_UPARTSIZE * CD_SECSIZE) /* # of bytes for a part. */

struct cd_map {
	unsigned int	cdm_isosize;	    /* # of sectors for ISO part. */
	int		cdm_npart;	    /* # of other UFS partitions */
	
	int		cdm_paddr[NCDPART]; /* address of each parition */
	unsigned int	cdm_size;	    /* # of sectors for the CD-ROM */
};
   
extern unsigned int	cd_putlabel();
extern	int		cd_getlabel();


