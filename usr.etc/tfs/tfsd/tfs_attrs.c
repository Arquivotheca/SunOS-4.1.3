#ifndef lint
static char sccsid[] = "@(#)tfs_attrs.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <nse/param.h>
#include <nse/util.h>
#include "headers.h"
#include "tfs.h"
#include "vnode.h"
#include "subr.h"
#include <signal.h>

/*
 * Routines to get attributes of files.  We maintain a (small) cache of
 * attributes to avoid redundant fstat's.
 */

#define MAX_ATTRS	3		/* Maximum # of entries in cache */
#define ATTR_TIMEOUT	10		/* Time that an attribute buffer
					 * allowed to stay in the cache
					 * after the last time it is used.
					 */

typedef struct Attr_cache {
	struct pnode	*pnode;
	struct nfsfattr	attrs;
	long		last_use;	/* Last time these attrs used */
	long		flush_time;	/* When these attrs should be
					 * flushed from the cache */
} *Attr_cache, Attr_cache_rec;

static Attr_cache_rec attr_cache[MAX_ATTRS];
static int	attr_timeout;
static long	use_count;		/* Incremented every time attr
					 * cache used */
bool_t		attr_cache_on;

int		getattrs_of();
int		get_realattrs_of();
void		fix_attrs();
void		set_cached_attrs();
static void	get_cached_attrs();
void		flush_cached_attrs();
bool_t		timeout_attr_cache();
void		flush_all_attrs();
static void	flush_and_close_fd();
void		init_attr_cache();
static Attr_cache find_attr_rec();
static Attr_cache find_lru_attr();
void		fattr_to_nattr();


int
getattrs_of(vp, attrs)
	struct vnode	*vp;
	struct nfsfattr *attrs;
{
	int		result;

	if (result = get_realattrs_of(vp, attrs)) {
		return (result);
	}
#ifdef TFSDEBUG
	dprint(tfsdebug, 3,
"  getattrs_of => ino %d size %d blocks %d  times: a %d m %d c %d\n",
		attrs->na_nodeid, attrs->na_size,
		attrs->na_blocks, attrs->na_atime.tv_sec,
		attrs->na_mtime.tv_sec, attrs->na_ctime.tv_sec);
#endif TFSDEBUG
	fix_attrs(vp, attrs);
	return(0);
}


/*
 * Get the attributes of the vnode 'vp'.  If 'vp' is a directory vnode
 * without an open file descriptor, open an fd for it (through
 * change_to_dir.)
 */
int
get_realattrs_of(vp, attrs)
	struct vnode	*vp;
	struct nfsfattr *attrs;
{
	struct pnode	*pp;
	char		name[MAXNAMLEN];
	struct stat	statb;
	int		fd;

	pp = vp->v_pnode;
	fd = pp->p_fd;
	if (fd != 0) {
		use_fd(fd);
	}
	if (pp->p_have_attrs) {
		get_cached_attrs(pp, attrs);
		return (0);
	}
	if (fd == 0) {
		if ((pp->p_type == PTYPE_DIR) && (change_to_dir(pp) == 0)) {
			fd = pp->p_fd;
		} else {
			ptoname(pp, name);
			if (LSTAT(name, &statb) < 0) {
				return (-1);
			}
			goto have_statb;
		}
	}
	if (fstat(fd, &statb) < 0) {
		close_fd(pp);
		return (-1);
	}
have_statb:
	fattr_to_nattr(&statb, attrs);
	if (attr_cache_on) {
		set_cached_attrs(pp, attrs);
	}
	return (0);
}


void
fix_attrs(vp, attrs)
	struct vnode	*vp;
	struct nfsfattr	*attrs;
{
	struct pnode	*pp;

	/*
	 * Set the nodeid to the fhandle id.  This is done because 1),
	 * nodeids shouldn't change when a file is copied to the front fs
	 * by a write operation, and 2), nodeids have to be unique to
	 * guarantee that getwd will always work.
	 */
	attrs->na_nodeid = vp->v_fhid;
	if (!IS_WRITEABLE(vp)) {
		if (vp->v_back_owner) {
			attrs->na_uid = current_user_id();
		}
		if (vp->v_mtime != 0) {
			attrs->na_mtime.tv_sec = vp->v_mtime;
		}
	}
	/*
	 * If there are writes pending for this file, then set the file
	 * size & mtime to the virtual size & mtime of the file.
	 */
	pp = vp->v_pnode;
	if (pp->p_type == PTYPE_REG && pp->p_needs_write) {
		attrs->na_mtime.tv_sec = pp->p_write_time;
		attrs->na_size = pp->p_size;
	}
}


void
set_cached_attrs(pp, attrs)
	struct pnode	*pp;
	struct nfsfattr	*attrs;
{
	Attr_cache	ac;

	if ((pp->p_type == PTYPE_REG) && (!pp->p_needs_write)) {
		pp->p_size = attrs->na_size;
	}
	ac = find_lru_attr();
	if (ac->pnode != NULL) {
		flush_and_close_fd(ac->pnode);
	}
	ac->pnode = pp;
	BCOPY((char *) attrs, (char *) &ac->attrs, sizeof(struct nfsfattr));
	ac->flush_time = get_current_time(FALSE) + attr_timeout;
	ac->last_use = use_count++;
	pp->p_have_attrs = 1;
}


static void
get_cached_attrs(pp, attrs)
	struct pnode	*pp;
	struct nfsfattr *attrs;
{
	Attr_cache	ac;

	ac = find_attr_rec(pp);
	BCOPY((char *) &ac->attrs, (char *) attrs, sizeof(struct nfsfattr));
	ac->flush_time = get_current_time(FALSE) + attr_timeout;
	ac->last_use = use_count++;
}


void
flush_cached_attrs(pp)
	struct pnode	*pp;
{
	Attr_cache	ac;

	if (pp->p_have_attrs) {
		ac = find_attr_rec(pp);
		ac->pnode = NULL;
		pp->p_have_attrs = 0;
	}
}


bool_t
timeout_attr_cache(now)
	long		now;
{
	int		i;
	bool_t		more_to_flush = FALSE;

	if (!attr_cache_on) {
		return (FALSE);
	}
	for (i = 0; i < MAX_ATTRS; i++) {
		if (attr_cache[i].pnode != NULL) {
			if (attr_cache[i].flush_time < now) {
#ifdef TFSDEBUG
{
	char		pn[MAXPATHLEN];

	ptopn(attr_cache[i].pnode, pn);
	dprint(tfsdebug, 3, "flushing attrs of %s (flush_time %u)\n", pn,
		attr_cache[i].flush_time);
}
#endif TFSDEBUG
				flush_and_close_fd(attr_cache[i].pnode);
				attr_cache[i].pnode = NULL;
			} else {
				more_to_flush = TRUE;
			}
		}
	}
	return (more_to_flush);
}


void
flush_all_attrs()
{
	int		i;

	for (i = 0; i < MAX_ATTRS; i++) {
		if (attr_cache[i].pnode != NULL) {
			flush_and_close_fd(attr_cache[i].pnode);
			attr_cache[i].pnode = NULL;
		}
	}
}


void
init_attr_cache()
{
	attr_cache_on = TRUE;
	attr_timeout = ATTR_TIMEOUT;
}


static void
flush_and_close_fd(pp)
	struct pnode	*pp;
{
	pp->p_have_attrs = 0;
	if ((pp->p_type == PTYPE_REG) && (pp->p_fd != 0) &&
	    (!pp->p_needs_write)) {
		close_fd(pp);
	}
}


static Attr_cache
find_attr_rec(pp)
	struct pnode	*pp;
{
	int		i;

	for (i = 0; i < MAX_ATTRS; i++) {
		if (attr_cache[i].pnode == pp) {
			return (&attr_cache[i]);
		}
	}
#ifdef TFSDEBUG
	nse_log_message("warning: find_attr_rec could not find attrs!\n");
#endif TFSDEBUG
	return (NULL);
}


/*
 * Find the least recently used attrs.
 */
static Attr_cache
find_lru_attr()
{
	long		min = attr_cache[0].last_use;
	int		min_index = 0;
	int		i;

	for (i = 0; i < MAX_ATTRS; i++) {
		if (attr_cache[i].pnode == NULL) {
			return (&attr_cache[i]);
		}
		if (attr_cache[i].last_use < min) {
			min = attr_cache[i].last_use;
			min_index = i;
		}
	}
	return (&attr_cache[min_index]);
}


/*
 * Convert stat struct information (from stat call) into nfsfattr struct.
 */
void
fattr_to_nattr(fap, na)
	struct stat    *fap;
	struct nfsfattr *na;
{
	switch (fap->st_mode & S_IFMT) {
	case S_IFDIR:
		na->na_type = NFDIR;
		break;
	case S_IFCHR:
		na->na_type = NFCHR;
		break;
	case S_IFBLK:
		na->na_type = NFBLK;
		break;
	case S_IFREG:
		na->na_type = NFREG;
		break;
	case S_IFLNK:
		na->na_type = NFLNK;
		break;
	case S_IFSOCK:
		na->na_type = NFNON;	/* XXX */
		break;
	default:
		na->na_type = NFNON;
		break;
	}
	na->na_nodeid = fap->st_ino;
	na->na_fsid = fap->st_dev;
	na->na_mode = fap->st_mode;
	na->na_nlink = fap->st_nlink;
	na->na_uid = fap->st_uid;
	na->na_gid = fap->st_gid;
	na->na_rdev = fap->st_rdev;
	na->na_size = fap->st_size;
	na->na_atime.tv_sec = fap->st_atime;
	na->na_atime.tv_usec = 0;
	na->na_mtime.tv_sec = fap->st_mtime;
	na->na_mtime.tv_usec = 0;
	na->na_ctime.tv_sec = fap->st_ctime;
	na->na_ctime.tv_usec = 0;
	na->na_blocksize = MIN(fap->st_blksize, NFS_MAXDATA);
	na->na_blocks = fap->st_blocks / (fap->st_blksize / na->na_blocksize);
}
