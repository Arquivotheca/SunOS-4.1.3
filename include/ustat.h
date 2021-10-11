/*	@(#)ustat.h 1.1 92/07/30 SMI; from S5R2 1.1	*/

#ifndef _ustat_h
#define _ustat_h

struct  ustat {
	daddr_t	f_tfree;	/* total free */
	ino_t	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};

#endif /*!_ustat_h*/
