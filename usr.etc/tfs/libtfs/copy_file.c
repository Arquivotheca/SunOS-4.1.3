#ifndef lint
static char sccsid[] = "@(#)copy_file.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/time.h>
#include <ar.h>
#include <errno.h>
#include <nse/util.h>
#include <nse/isafile.h>

extern long	lseek();

/*
 * Copy one file to another.  (Recursive copy not handled.)  Code stolen from
 * cp.c.
 */
Nse_err
_nse_copy_file(to, from, pflag)
	char		*from;
	char		*to;
	int		pflag;
{
	bool_t		lib;

	return _nse_copy_file_libscan(to, from, pflag, &lib);
}

/*
 * Copy a file and check if it is a library.
 */
Nse_err
_nse_copy_file_libscan(to, from, pflag, ranlibp)
	char		*from;
	char		*to;
	int		pflag;
	bool_t		*ranlibp;
{
	int fold, fnew, i, n, exists;
	char buf[MAXBSIZE];
	struct stat stfrom, stto;
	Nse_err rt;
	int has_holes;				/* File has holes in it? */
	int offset;
	bool_t first;

	fold = open(from, 0);
	if (fold < 0) {
		return nse_err_format_errno("Open of \"%s\"", from);
	}
	if (fstat(fold, &stfrom) < 0) {
		rt = nse_err_format_errno("Fstat of \"%s\"", from);
		(void) close(fold);
		return rt;
	}
	if ((stfrom.st_mode&S_IFMT) == S_IFDIR) {
		(void) close(fold);
		errno = EISDIR;
		return nse_err_format_errno("Copy from \"%s\"", from);
	}

	exists = stat(to, &stto) == 0;
	if ((exists) && (stto.st_mode&S_IFMT) == S_IFDIR) {
		(void) close(fold);
		errno = EISDIR;
		return nse_err_format_errno("Copy to \"%s\"", from);
	}
	if (exists) {
		if (stfrom.st_dev == stto.st_dev &&
		   stfrom.st_ino == stto.st_ino) {
			(void) close(fold);
			return NULL;
		}
	}
	fnew = creat(to, (int) (stfrom.st_mode & 07777));
	if (fnew < 0) {
		rt = nse_err_format_errno("Creat of \"%s\"", to);
		(void) close(fold);
		return rt;
	}
	if (exists && pflag) {
		(void) fchmod(fnew, (int) (stfrom.st_mode & 07777));
	}
	has_holes = (stfrom.st_blocks * DEV_BSIZE < stfrom.st_size);
	offset = 0;
	*ranlibp = FALSE;
	first = TRUE;
	for (;;) {
		n = read(fold, buf, sizeof buf);
		if (n == 0)
			break;
		if (n < 0) {
			rt = nse_err_format_errno("Read of \"%s\"", from);
			(void) close(fold);
			(void) close(fnew);
			return rt;
		}
		if (first) {
			first = FALSE;
			if (n >= sizeof(NSE_RANLIB_HDR) && 
			    strncmp(buf, NSE_RANLIB_HDR, 
				    sizeof(NSE_RANLIB_HDR) - 1) == 0) {
				*ranlibp = TRUE;
			}
		}
		if (has_holes) {
			/*
			 * Preserve any holes in the file.
			 */
			i = 0;
			do {
				if (buf[i] != 0) {
					break;
				}
				i++;
			} while (i < n);
			if (i == n) {
				offset += n;
				continue;
			}
			if (lseek(fnew, (long) offset, L_SET) < 0) {
				rt = nse_err_format_errno("Lseek to %d in \"%s\"",
				    offset, to);
				(void) close(fold);
				(void) close(fnew);
				return rt;
			}
			offset += n;
		}
		if (write(fnew, buf, n) != n) {
			rt = nse_err_format_errno("Write to \"%s\"", to);
			(void) close(fold);
			(void) close(fnew);
			return rt;
		}
	}
	(void) close(fold);
	/* In 4.0, sometimes close was returning a random error so check
	 * errno as well as the return value from close.
	 */
	errno = 0;
	if (close(fnew) < 0 && errno != 0) {
		return nse_err_format_errno("Close of \"%s\"", to);
	}
	if (pflag)
		return _nse_copy_setimes(to, &stfrom);
	return NULL;
}


Nse_err
_nse_copy_setimes(path, statp)
	char *path;
	struct stat *statp;
{
	struct timeval tv[2];

	tv[0].tv_sec = statp->st_atime;
	tv[1].tv_sec = statp->st_mtime;
	tv[0].tv_usec = tv[1].tv_usec = 0;
	if (utimes(path, tv) < 0) {
		return nse_err_format_errno("Utimes of \"%s\"", path);
	}
	return NULL;
}
