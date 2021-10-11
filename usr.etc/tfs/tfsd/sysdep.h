/*	@(#)sysdep.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 Sun Microsystems, Inc.
 */


/********************************************************
 * UNIX SYSTEM 4.2
 ********************************************************/
#ifdef SYS4.2

#define uio_seg		uio_segflg
#define UIOSEG_USER     0
#define UIOSEG_KERNEL   1

#define LSTAT(p,b)	lstat(p,b)

#define BCOPY(s,d,n)	bcopy(s,d,n)
#define BCMP(s,d,n)	bcmp(s,d,n)
#define BZERO(s,n)	bzero(s,n)
#define INDEX(s,c)	index(s,c)
#define RINDEX(s,c)	rindex(s,c)

/* errno.h */
/* Network File System */
#define ESTALE          70		/* Stale NFS file handle */
#define EREMOTE         71		/* Too many levels of remote in path */

#else
/********************************************************
 * UNIX SYSTEM V
 ********************************************************/
#ifdef SYS5

#define LSTAT(p,b)	stat(p,b)

#include <memory.h>
#define BCOPY(s,d,n)	memcpy(d,s,n)
#define BCMP(s,d,n)	memcmp(d,s,n)
#define BZERO(s,n)	memset(s,0,n)
#define INDEX(s,c)	strchr(s,c)
#define RINDEX(s,c)	strrchr(s,c)

/* macro not defined in dir.h */
#define DIRBLKSIZ       512

#else
/********************************************************
 * SUN UNIX SYSTEM
 ********************************************************/
#ifdef SUN

#define LSTAT(p,b)	lstat(p,b)

#define BCOPY(s,d,n)	bcopy(s,d,n)
#define BCMP(s,d,n)	bcmp(s,d,n)
#define BZERO(s,n)	bzero(s,n)
#define INDEX(s,c)	index(s,c)
#define RINDEX(s,c)	rindex(s,c)

/* macro not defined in dir.h */
#define DIRBLKSIZ       512


#endif
#endif
#endif


/********************************************************
 * OPERATING SYSTEM DEPENDENT CALLS AND ROUTINES
 ********************************************************/
/* systems calls */

#define CHDIR(p)	chdir(p)
#define CHMOD(p,m)	chmod(p,m)
#define CHOWN(p,u,g)	chown(p,u,g)
#define UTIMES(p,t)	utimes(p,t)
#define TRUNCATE(p,s)	truncate(p,s)

#define OPEN(p,f,m)	open(p,f,m)
#define CREAT(p,m)	creat(p,m)
#define LSEEK(f,o,m)	lseek(f,o,m)
#define READ(f,b,n)	read(f,b,n)
#define WRITE(f,b,n)	write(f,b,n)
#ifdef XXX
#define FSYNC(f)	fsync(f)
#endif
#define CLOSE(f)	close(f)
#define RENAME(p,q)	rename(p,q)
#define MKDIR(p,m)	mkdir(p,m)
#define RMDIR(p)	rmdir(p)
#define LINK(p,q)	link(p,q)
#define UNLINK(p)	unlink(p)
#define SYMLINK(n,p)	symlink(n,p)
#define READLINK(p,b,l)	readlink(p,b,l)

/* library routines */

#define OPENDIR(d)	opendir(d)
#define READDIR(p)	readdir(p)
#define TELLDIR(p)	telldir(p)
#define SEEKDIR(p,l)	seekdir(p,l)
#define REWINDDIR(p)	rewinddir(p)
#define CLOSEDIR(P)	closedir(p)

#define STRCAT(d,s)	strcat(d,s)
#define STRNCAT(d,s,n)	strncat(d,s,n)
#define STRCMP(d,s)	strcmp(d,s)
#define STRNCMP(d,s,n)	strncmp(d,s,n)
#define STRCPY(d,s)	strcpy(d,s)
#define STRNCPY(d,s,n)	strncpy(d,s,n)
#define STRLEN(s)	strlen(s)
