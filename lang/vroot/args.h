/*LINTLIBRARY*/
/*	@(#)args.h 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/syscall.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

extern	int	errno;
extern	void	translate_with_thunk();

typedef enum { rw_read, rw_write} rwt, *rwpt;

union Args {
	struct { int mode;} access;
	struct { int mode;} chmod;
	struct { int user; int group;} chown;
	struct { int mode;} creat;
	struct { char **argv; char **environ;} execve;
	struct { struct stat *buffer;} lstat;
	struct { int mode;} mkdir;
	struct { char *name; int mode;} mount;
	struct { int flags; int mode;} open;
	struct { char *buffer; int buffer_size;} readlink;
	struct { struct stat *buffer;} stat;
	struct { struct statfs *buffer;} statfs;
	struct { int length;} truncate;
	struct { struct timeval **time;} utimes;
} vroot_args;
int	vroot_result;
