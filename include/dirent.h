/*	@(#)dirent.h 1.1 92/07/30 SMI 	*/

/*
 * Filesystem-independent directory information.
 */

#ifndef	__dirent_h
#define	__dirent_h

#include <sys/types.h>

#ifndef	_POSIX_SOURCE
#define	d_ino	d_fileno	/* compatability */
#ifndef	NULL
#define	NULL	0
#endif
#endif	/* !_POSIX_SOURCE */

/*
 * Definitions for library routines operating on directories.
 */
typedef	struct __dirdesc {
	int	dd_fd;		/* file descriptor */
	long	dd_loc;		/* buf offset of entry from last readddir() */
	long	dd_size;	/* amount of valid data in buffer */
	long	dd_bsize;	/* amount of entries read at a time */
	long	dd_off;		/* Current offset in dir (for telldir) */
	char	*dd_buf;	/* directory data buffer */
} DIR;

extern	DIR *opendir(/* char *dirname */);
extern	struct dirent *readdir(/* DIR *dirp */);
extern	int closedir(/* DIR *dirp */);
#ifndef	_POSIX_SOURCE
extern	void seekdir(/* DIR *dirp, int loc */);
extern	long telldir(/* DIR *dirp */);
#endif	/* POSIX_SOURCE */
extern	void rewinddir(/* DIR *dirp */);

#ifndef	lint
#define	rewinddir(dirp)	seekdir((dirp), (long)0)
#endif

#include <sys/dirent.h>

#endif	/* !__dirent_h */
