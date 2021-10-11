/*  @(#)tmp_subr.c 1.1 92/07/30 SMI */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <debug/debug.h>
#include <vm/anon.h>
#include <vm/seg_map.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/ucred.h>
#include <tmpfs/tmp.h>
#include <tmpfs/tmpnode.h>

newnode(t, mode, uid, gid)
	struct tmpnode *t;
	u_int mode;
	uid_t uid;
	gid_t gid;
{
	t->tn_attr.va_mode = mode;
	t->tn_attr.va_uid = uid;
	t->tn_attr.va_gid = gid;
	t->tn_attr.va_rdev = 0;
	created(t);
}

created(t)
	struct tmpnode *t;
{
	modified(t);
	t->tn_attr.va_ctime = t->tn_attr.va_mtime;
}

modified(t)
	struct tmpnode *t;
{
	accessed(t);
	t->tn_attr.va_mtime = t->tn_attr.va_atime;
}

accessed(t)
	struct tmpnode *t;
{
	extern struct timeval time;

	t->tn_attr.va_atime = time;
}

isparent(from, to)
	struct tmpnode *from;
	struct tmpnode *to;
{
	int error;
	struct tmpnode *prev_tp, *curr_tp;
	struct ucred rootcred;

	rootcred.cr_uid = 0;
	curr_tp = to;

	tmpnode_get(curr_tp);
	while (from != curr_tp) {
		error = tdirlookup(curr_tp, "..", &prev_tp, &rootcred);
		if (error) {
			tmpnode_put(curr_tp);
			return (0);
		}
		if (curr_tp == prev_tp) {	/* at root */
			tmpnode_put(curr_tp);
			tmpnode_put(prev_tp);
			return (0);
		}
		tmpnode_put(curr_tp);
		curr_tp = prev_tp;
	}
	tmpnode_put(curr_tp);
	return (1);
}

#define	MODESHIFT	3

taccess(tp, cred, access)
	struct tmpnode *tp;
	struct ucred *cred;
	int access;
{

	/*
	 * Superuser always gets access
	 */
	if (cred->cr_uid == 0)
		return (0);
	/*
	 * Check access based on owner, group and
	 * public permissions in tmpnode.
	 */
	if (cred->cr_uid != tp->tn_attr.va_uid) {
		access >>= MODESHIFT;
		if (ingroup(tp->tn_attr.va_gid, cred) == 0)
			access >>= MODESHIFT;
	}
	if ((tp->tn_attr.va_mode & access) == access)
		return (0);
	return (EACCES);
}

ingroup(gid, cred)
	gid_t gid;
	struct ucred *cred;
{
	int i;

	if (gid == cred->cr_gid) {
		return (1);
	}
	for (i = 0; i < NGROUPS && cred->cr_groups[i] != NOGROUP; i++) {
	    if (gid == cred->cr_groups[i]) {
		    return (1);
	    }
	}
	return (0);
}

/*
 * alloc index numbers (inode #) for a new tmpfs file.
 * vfs_getnum() does part of the job for us.  hope no one hacks on it.
 */
long
tmpimapalloc(tm)
	register struct tmount *tm;
{
	register struct tmpimap *tmapp;
	register int i, bitnum;

	for (i = 0, tmapp = &tm->tm_inomap; tmapp;
	    i++, tmapp = tmapp->timap_next) {
		if ((bitnum = vfs_getnum((char *)tmapp->timap_bits,
		    TMPIMAPSIZE)) != -1)
			return ((i*TMPIMAPNODES)+bitnum);
		if (tmapp->timap_next == (struct tmpimap *)NULL)
			tmapp->timap_next = (struct tmpimap *)tmp_memalloc(tm,
			    sizeof (struct tmpimap));
	}
	return (-1);
}

/*
 * free index number of a (being destroyed) tmpfs file
 * vfs_putnum() does part of the job for us. hope no one hacks on it.
 * XXX should free allocated tmpimap struct memory when files free up
 */
tmpimapfree(tm, number)
	register struct tmount *tm;
	register long number;
{
	register int i;
	register struct tmpimap *tmapp;
	void vfs_putnum();

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tmpimapfree: freeing tm %x number %d index %d\n", tm,
		    number, (number % TMPIMAPNODES));
#endif TMPFSDEBUG
	for (i = 1, tmapp = &tm->tm_inomap; tmapp;
	    i++, tmapp = tmapp->timap_next) {
		if (number < i*TMPIMAPNODES) { 	/* this is our map! */
			vfs_putnum((char *)tmapp->timap_bits,
			    (int)(number % TMPIMAPNODES));
			return (0);
		}
	}
	return (-1);
}

/*
 * tmpfs kernel memory allocation
 * does some bookkeeping, calls kmem_zalloc() for the honey
 */
char *
tmp_memalloc(tm, size)
	register struct tmount *tm;
	register u_int size;
{
	extern u_int tmpfs_maxkmem;
	extern int tmp_kmemspace;
#ifdef TMPFSDEBUG
	extern func_t caller();
#endif TMPFSDEBUG

#ifdef TMPFSDEBUG
	if (tmpdebugalloc)
		printf("tmp_memalloc: tm %x size %x\n", tm, size);
#endif TMPFSDEBUG
	if ((tm->tm_kmemspace + size) < tmpfs_maxkmem) {
		tm->tm_kmemspace += size;
		tmp_kmemspace += size;
		return (new_kmem_zalloc(size, KMEM_SLEEP));
	}

#ifdef TMPFSDEBUG
	if (tmpdebugerrs)
		printf("tmp_memalloc: out of space. Called by %x\n", caller());
#endif TMPFSDEBUG
	return (NULL);
}


/*
 * tmpfs kernel memory freer
 * does some bookkeeping, calls kmem_free()
 */
void
tmp_memfree(tm, cp, size)
	register struct tmount *tm;
	register char *cp;
	register u_int size;
{

#ifdef TMPFSDEBUG
	if (tmpdebugalloc)
		printf("tmp_memfree: tm %x cp %x size %d\n", tm, cp, size);
#endif TMPFSDEBUG
	if (tm->tm_kmemspace < size) {
		panic("tmp_memfree()- bad memory bookkeeping");
	}
	kmem_free(cp, size);
	tm->tm_kmemspace -= size;
	tmp_kmemspace -= size;
}
