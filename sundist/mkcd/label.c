/*      @(#)label.c 1.1 92/07/30 SMI      */
#include <stdio.h>
#include <sys/types.h>
#include <sun/dkio.h>
#include <sun/dklabel.h>
#include <math.h>
#include "label.h"

#ifndef lint
static  char sccsid[] = "@(#)label.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif  lint

#define	CK_CHECKSUM	0	/* check checksum */
#define CK_MAKESUM	1	/* generate checksum */

unsigned int
cd_putlabel(map, label_sector)
struct cd_map	*map;
char *	label_sector;
{
	struct dk_label	label;
	double	partsize_d;
	double	cylsize_d;
	int	cylsize;
	int	ncyl;
	int	next_cyl;
	unsigned int	total = 0;
	int	i;

	/*
	 * Check for valid values in map structure
	 */
	if (map->cdm_npart > (NCDPART - 1)) {
		fprintf(stderr, "Label: Too many ufs partitions\n");
		return (-1);
	}
	if ((map->cdm_isosize + (CD_NSECT * CD_NHEAD) +
	    (map->cdm_npart * CD_UPARTSIZE)) > CD_SIZE) {
		fprintf(stderr, 
			"Label: Cannot fit everything in a single CD-ROM\n");
		return (-1);
	}

	bzero((char *)&label, sizeof (struct dk_label));
	bzero((char *)(map->cdm_paddr), sizeof (int) * NCDPART);

	/* 
	 * Fill in geometry information
	 */
	label.dkl_ncyl = CD_NCYL;
	label.dkl_nhead = CD_NHEAD;
	label.dkl_nsect = CD_NSECT;
	label.dkl_intrlv = 1;
	label.dkl_rpm = CD_RPM;

	(void) sprintf(label.dkl_asciilabel, "%s cyl %d alt %d hd %d sec %d",
		       "CD-ROM Disc for SunOS Installation",
		       label.dkl_ncyl, label.dkl_acyl, label.dkl_nhead,
		       label.dkl_nsect);
	
	/*
	 * create the first parition (ISO partition)
	 */
	label.dkl_map[0].dkl_cylno = (daddr_t)0;
	partsize_d = (double)map->cdm_isosize;
	cylsize_d = (double)(cylsize = (label.dkl_nhead * label.dkl_nsect));
	ncyl = ceil(partsize_d / cylsize_d);
/*	label.dkl_map[0].dkl_nblk = ncyl * cylsize;*/
	label.dkl_map[0].dkl_nblk = (ncyl * cylsize) + 
	                            (CD_UPARTSIZE * map->cdm_npart);
	next_cyl = ncyl;
	map->cdm_paddr[0] = 0;

	/*
	 * create the rest of the partitions (UFS)
	 */
	ncyl = CD_UPARTSIZE / cylsize;
	for (i = 1; i <= map->cdm_npart; i++) {
		label.dkl_map[i].dkl_cylno = next_cyl;
		label.dkl_map[i].dkl_nblk = CD_UPARTSIZE;
		map->cdm_paddr[i] = next_cyl * cylsize;
		next_cyl += ncyl;
	}
	total = next_cyl * cylsize;

	/*
	 * put the magic cookie
	 */
	label.dkl_magic = DKL_MAGIC;

	/*
	 * generate checksum 
	 */
	(void) checksum(&label, CK_MAKESUM);
	/*
	 * now, writing out the label
	 */
	bcopy(&label, label_sector, sizeof (struct dk_label));

	return (total);
}

/*
 * This routine reads in the label and returns the cd_map structure.
 * Returns 0 if it is a valid label, and return non-zero others.
 */
int
cd_getlabel(map, label_sector)
struct cd_map	*map;
char	*label_sector;
{
	struct dk_label label;
	int	i;
	int	last_address;

	bzero(map, sizeof (struct cd_map));

	/*
	 * read into the label structure
	 */
	bcopy(label_sector, &label, sizeof (struct dk_label));

	/*
	 * check the magic cookie and checksum
	 */
	if (label.dkl_magic != DKL_MAGIC) {
		return (-1);
	}
	if (checksum(&label, CK_CHECKSUM) != 0) {
		return (-1);
	}

	last_address = label.dkl_map[0].dkl_nblk;

	map->cdm_paddr[0] = 0;	/* 1st part (ISO) always starts on 0 sector */
	for (i=1; i!=NCDPART; i++) {
		if (label.dkl_map[i].dkl_nblk == 0) {
			map->cdm_paddr[i] = -1;
		} else {
			map->cdm_paddr[i] = label.dkl_map[i].dkl_cylno *
			                    label.dkl_nhead * label.dkl_nsect;
			map->cdm_npart++;
			last_address = map->cdm_paddr[i] + 
			               label.dkl_map[i].dkl_nblk;
		}
	}
	map->cdm_size = last_address;

	return (0);
}

/*
 * This routine checks or calculates the label checksum, depending on
 * the mode it is called in.
 */
int
checksum(label, mode)
	struct	dk_label *label;
	int	mode;
{
	register short *sp, sum = 0;
	register short count = (sizeof (struct dk_label)) / (sizeof (short));

	/*
	 * If we are generating a checksum, don't include the checksum
	 * in the rolling xor.
	 */
	if (mode == CK_MAKESUM)
		count -= 1;
	sp = (short *)label;
	/*
	 * Take the xor of all the half-words in the label.
	 */
	while (count--)
		sum ^= *sp++;
	/*
	 * If we are checking the checksum, the total will be zero for
	 * a correct checksum, so we can just return the sum.
	 */
	if (mode == CK_CHECKSUM)
		return (sum);
	/*
	 * If we are generating the checksum, fill it in.
	 */
	else {
		label->dkl_cksum = sum;
		return (0);
	}
}


