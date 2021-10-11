#ifndef lint
static char sccsid[] = "@(#)tfs_fd.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/types.h>
#include "headers.h"
#include "vnode.h"
#include "subr.h"
#include "tfs.h"
#include <nse/util.h>

/* 
 * File caching routines
 */

/*
 * File descriptor cache entry.
 */
struct tfd {
	struct pnode	*pnode;
	int		flags;
	short		prev;
	short		next;
};

static struct tfd *tfd_table;

static short	tfd_first = -1;
static short	tfd_last = -1;
static short	tfd_max;
static short	tfd_count = 0;

static int	current_fd;

extern int	chmod();
extern int	fchmod();

DIR            *tfs_opendir();
void            tfs_closedir();
int		get_rw_fd();
int             open_fd();
void		close_fd();
int		rdwr_fd();
void            use_fd();
int             get_open_fd_flags();
static void     enqueue_fd();
static void     dequeue_fd();
int		init_fd_cache();
int		change_to_dir();
static int	tfs_fchdir();
#ifdef TFSDEBUG
void		tfs_getwd();
#endif TFSDEBUG


/*
 * Special versions of the opendir() and closedir() library routines which
 * make use of the TFS file descriptor cache to save the fd of the directory.
 */

/*
 * open a directory.
 */
DIR            *
tfs_opendir(pp)
	struct pnode   *pp;
{
	static DIR	dir_desc;	/* Only tfs_readdir calls this, so
					 * this is safe for now. */
	int             fd;

	if (pp->p_type != PTYPE_DIR) {
		errno = ENOTDIR;
		return (NULL);
	}
	fd = pp->p_fd;
	if (fd != 0) {
		use_fd(fd);
		(void) lseek(fd, (long) 0, L_SET);
	} else {
		fd = open_fd(pp, 0, 0);
		if (fd == -1) {
			return (NULL);
		}
	}
	BZERO((char *) &dir_desc, sizeof(DIR));
	dir_desc.dd_buf = readdir_buffer;
	dir_desc.dd_bsize = NFS_MAXDATA;
	dir_desc.dd_fd = fd;
	return (&dir_desc);
}


/*
 * close a directory.
 */
void
tfs_closedir(dirp)
	DIR            *dirp;
{
	dirp->dd_fd = -1;
	dirp->dd_loc = 0;
}


/*
 * Get file descriptor to read/write.  Returns fd, or -1 if an error occurs.
 */
int
get_rw_fd(vp, rw)
	struct vnode	*vp;
	enum uio_rw	rw;
{
	struct pnode	*pp;
	int             fd;
	int             openflags;
	int		prevflags;
	struct nfsfattr attrs;

	pp = vp->v_pnode;
	openflags = (rw == UIO_READ) ? O_RDONLY : O_RDWR;
	fd = pp->p_fd;
	if (fd != 0) {
		prevflags = get_open_fd_flags(fd);
		if ((prevflags != openflags) && (prevflags != O_RDWR)) {
			close_fd(pp);
			/*
			 * We have now seen both reads & writes on this file,
			 * so open it read/write for the duration.
			 */
			openflags = O_RDWR;
		} else {
			return (fd);
		}
	}
	fd = open_fd(pp, openflags, 0);
	if (fd >= 0 || errno != EACCES) {
		return (fd);
	}
	if (getattrs_of(vp, &attrs) < 0) {
		errno == EACCES;
		return (-1);
	}
	if (vp->v_created && attrs.na_uid == current_user_id()) {
		/*
		 * If the open fails, and this file was created by the TFS,
		 * then assume that the user created the file with
		 * insufficient perms for read or write, and then the fd
		 * was later closed (by a tfs_flush, say).  Chmod the file
		 * temporarily so that the user can open it.
		 */
		if (modify_file(pp, chmod, fchmod, 0744) != 0) {
			tfsd_perror("tfsd: warning: get_rw_fd: chmod");
			return (-1);
		}
		fd = open_fd(pp, openflags, 0);
		(void) modify_file(pp, chmod, fchmod, (int) attrs.na_mode);
		return (fd);
	} else if (rw == UIO_READ && has_access(&attrs, X_OK)) {
		/*
		 * If the open fails and the user is trying to read a
		 * file that is executable, assume that the user is
		 * trying to execute the file and open an fd.
		 */
		change_user_id(0);
		fd = open_fd(pp, openflags, 0);
		change_user_id(current_user_id());
		return (fd);
	}
	errno = EACCES;
	return (-1);
}


/*
 * Make a tfd entry in the file descriptor cache.  Returns fd or -1 if error
 */
int
open_fd(pp, flags, mode)
	struct pnode   *pp;
	int             flags, mode;
{
	char		pn[MAXPATHLEN];
	int		fd;

	/* if there are no available tfds, close the lru */
	if (tfd_count >= tfd_max) {
		close_fd(tfd_table[tfd_first].pnode);
	}
	ptoname_or_pn(pp, pn);
	if ((fd = open(pn, flags, mode)) < 0) {
		return (-1);
	}

	/* add tfd to end of list */
	tfd_table[fd].pnode = pp;
	tfd_table[fd].flags = flags & 03;

	enqueue_fd(fd);

	pp->p_fd = fd;
	if (pp->p_type == PTYPE_REG) {
		pp->p_offset = 0;
	}
	return (fd);
}

/*
 * Close the file descriptor refered to by physical node 'pp'.
 * The close may fail with ENOSPC if the file is on a full network
 * file system and delayed writes are pending. Anything else indicates
 * a serious error.
 */
void
close_fd(pp)
	struct pnode	*pp;
{
	int		fd = pp->p_fd;

	if (pp->p_needs_write) {
		sync_wcache(pp);
	}
	if (close(fd) < 0) {
		if (errno != ENOSPC) {
			nse_log_message("panic: can't close cached fd #%d",fd);
			tfsd_perror("");
			panic(pp);
			/* NOTREACHED */
		}
		pp->p_write_error = ENOSPC;
	}
	pp->p_fd = 0;
	dequeue_fd(fd);
	if (fd == current_fd) {
		current_fd = 0;
		(void) chdir("/");
	}
}


/*
 * Read/write file with pnode 'pp' at 'offset'.  Don't lseek unless it's
 * necessary.
 */
int
rdwr_fd(pp, buf, length, offset, rw)
	struct pnode	*pp;
	char		*buf;
	int		length;
	long		offset;
	enum uio_rw	rw;
{
	int		count;

	if (offset != pp->p_offset) {
		pp->p_offset = offset;
		if (lseek((int) pp->p_fd, offset, L_SET) < 0) {
			close_fd(pp);
			return (-1);
		}
	}
	if (rw == UIO_READ) {
		count = read((int) pp->p_fd, buf, length);
	} else {
		count = write((int) pp->p_fd, buf, length);
	}
	if (count >= 0) {
		pp->p_offset += count;
	} else {
		pp->p_offset = -1;
	}
	return (count);
}


void
use_fd(fd)
	int             fd;
{
	if (tfd_last != fd) {
		dequeue_fd(fd);
		enqueue_fd(fd);
	}
}


int
get_open_fd_flags(fd)
{
#ifdef TFSDEBUG
	if (fd == 0) {
		printf("get_fd_flags: fd == 0?\n");
	}
#endif TFSDEBUG
	return (tfd_table[fd].flags);
}


static void
enqueue_fd(fd)
	int             fd;
{
	tfd_table[fd].prev = tfd_last;
	tfd_table[fd].next = -1;
	if (tfd_last >= 0) {
		tfd_table[tfd_last].next = fd;
	}
	if (tfd_first < 0) {
		tfd_first = fd;
	}
	tfd_last = fd;
	tfd_count++;
}


static void
dequeue_fd(fd)
	int             fd;
{
	if (fd == tfd_first) {
		tfd_first = tfd_table[fd].next;
		if (tfd_first == -1) {
			/* Have dequeued the only entry in the queue */
			tfd_last = -1;
		} else {
			tfd_table[tfd_first].prev = -1;
		}
	} else if (fd == tfd_last) {
		tfd_last = tfd_table[fd].prev;
		tfd_table[tfd_last].next = -1;
	} else {
		tfd_table[tfd_table[fd].prev].next = tfd_table[fd].next;
		tfd_table[tfd_table[fd].next].prev = tfd_table[fd].prev;
	}
	tfd_count--;
}


int
init_fd_cache()
{
	int		nfds;

	nfds = getdtablesize();
	tfd_table = (struct tfd *) nse_calloc((unsigned) nfds,
					      sizeof(struct tfd));
	tfd_max = nfds - 10;
}


#ifdef TFSDEBUG
print_fd_cache()
{
	char		pn[MAXPATHLEN];
	int		fd;

	dprint(0, 0, "fd cache:\n");
	fd = tfd_last;
	while (fd != -1) {
		ptopn(tfd_table[fd].pnode, pn);
		dprint(0, 0, "  %3d  (%s)\n", fd, pn);
		fd = tfd_table[fd].prev;
	}
}
#endif TFSDEBUG


/*
 * chdir to the directory with pnode 'pp'.
 */
int
change_to_dir(pp)
	struct pnode	*pp;
{
	int		numtries = 0;

again:
	if (pp->p_fd == 0) {
		if (open_fd(pp, O_RDONLY, 0) < 0) {
			return (errno);
		}
	}
	if (tfs_fchdir((int) pp->p_fd) < 0) {
		numtries++;
		if (numtries == 2) {
			return (errno);
		}
		close_fd(pp);
		goto again;
	}
	return (0);
}


static int
tfs_fchdir(fd)
	int		fd;
{
	use_fd(fd);
	if (fd != current_fd) {
		current_fd = fd;
		if (nse_fchdir(fd) != 0) {
			return (-1);
		}
	}
	return (0);
}


#ifdef TFSDEBUG
void
tfs_getwd(path)
	char		*path;
{
	if (current_fd != 0) {
		ptopn(tfd_table[current_fd].pnode, path);
	} else {
		path[0] = '\0';
	}
}
#endif TFSDEBUG

/* Avoid SunOs 4.0 close bug by redefining 'close' and checking for
 * spurious value of errno. This can be removed as soon as nobody in
 * the world is running SunOs 4.0.
 */

#include <syscall.h>

int
close(fd)
	int fd;
{
	extern int	errno;
	int		saved_errno = errno;
	int		ret;
	
	if ((ret = syscall(SYS_close, fd)) < 0) {
		if (errno < 0) {
			errno = saved_errno;
			return 0;
		}
	}
	return ret;
}
