/*	@(#)filai.h 1.1 92/07/30 SMI	*/

#ifndef _sys_filai_h
#define	_sys_filai_h

/*
 * struct for FIOAI ioctl():
 *	Input:
 *		fai_off	  - byte offset in file
 *		fai_size  - byte range (0 == EOF)
 *		fai_num   - number of entries in fai_daddr
 *		fai_daddr - array of daddr_t's
 *	Output:
 *		fai_off	  - resultant offset in file
 *		fai_size  - unchanged
 *		fai_num   - number of entries returned in fai_daddr
 *		fai_daddr - array of daddr_t's (0 entry == hole)
 *
 * Allocation information is returned in DEV_BSIZE multiples.  fai_off
 * is rounded down to a DEV_BSIZE multiple, and fai_size is rounded up.
 */

struct filai {
	off_t	fai_off;	/* byte offset in file */
	size_t	fai_size;	/* byte range */
	u_long	fai_num;	/* # entries in array */
	daddr_t	*fai_daddr;	/* array of daddr_t's */
};
#define	FILAI_HOLE	((daddr_t)(-1))

#endif /* !_sys_filai_h */
