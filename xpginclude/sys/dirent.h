/*	@(#)dirent.h 1.1 92/07/30 SMI 	*/

/*
 * Filesystem-independent directory information. 
 */

#ifndef _dirent_h
#define _dirent_h

#define	MAXNAMLEN	255

#define d_ino	d_fileno		/* compatability */

/*
 * Definitions for library routines operating on directories.
 */
typedef struct _dirdesc {
	int	dd_fd;			/* file descriptor */
	long	dd_loc;             /* buf offset of entry from last readddir() */
	long	dd_size;		/* amount of valid data in buffer */
	long	dd_bsize;		/* amount of entries read at a time */
	long	dd_off;             	/* Current offset in dir (for telldir) */
	char	*dd_buf;		/* directory data buffer */
} DIR;

#ifndef NULL
#define NULL 0
#endif
extern	DIR *opendir();
extern	struct dirent *readdir();
extern	long telldir();
extern	void seekdir();
#define rewinddir(dirp)	seekdir((dirp), (long)0)
extern	int closedir();


/* Thus section is included from <sys/dirent.h>, but
 * will disappear in xpg89, as this will no longer call sys/dirent.h.
 *
 */

#define	MAXNAMLEN	255

struct	dirent {
	off_t   d_off;			/* offset of next disk directory entry */
	u_long	d_fileno;		/* file number of entry */
	u_short	d_reclen;		/* length of this record */
	u_short	d_namlen;		/* length of string in d_name */
	char	d_name[MAXNAMLEN + 1];	/* name (up to MAXNAMLEN + 1) */
};

/* 
 * The macro DIRSIZ(dp) gives the minimum amount of space required to represent
 * a directory entry.  For any directory entry dp->d_reclen >= DIRSIZ(dp).
 * Specific filesystem types may use this use this macro to construct the value
 * for d_reclen.
 */
#undef DIRSIZ
#define DIRSIZ(dp)  \
	(((sizeof (struct dirent) - (MAXNAMLEN+1) + ((dp)->d_namlen+1)) + 3) & ~3)
#endif /*!_dirent_h*/
