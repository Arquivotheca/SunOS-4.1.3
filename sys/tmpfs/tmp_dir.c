/*  @(#)tmp_dir.c 1.1 92/07/30 SMI */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/ucred.h>
#include <sys/stat.h>
#include <debug/debug.h>
#include <tmpfs/tmpnode.h>
#include <tmpfs/tmp.h>
#include <tmpfs/tmpdir.h>
#include <sys/debug.h>

/*
 * search directory 'parent' for entry 'name'
 * return tmpnode LOCKED in 'foundtp'
 * XXX should use dnlc
 */
tdirlookup(parent, name, foundtp, cred)
	struct tmpnode *parent;
	char *name;
	struct tmpnode **foundtp;
	struct ucred *cred;
{
	int error;
	struct tdirent *tdp;
	extern func_t caller();

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tdirlookup: parent %x name %s\n", parent, name);
#endif TMPFSDEBUG
	if (error = taccess(parent, cred, VEXEC))
		return (error);
	if (error = tdiropen(parent, cred)) {
		return (error);
	}
	for (tdp = parent->tn_dir; tdp; tdp = tdp->td_next) {
		if (strcmp(tdp->td_name, name) == 0) {
			*foundtp = tdp->td_tmpnode;
			tmpnode_get(*foundtp);
#ifdef TMPFSDEBUG
			if (tmpdirdebug)
				printf("tdirlookup: returning %x\n", *foundtp);
#endif TMPFSDEBUG
			tdirclose(parent);
			return (0);
		}
	}
	tdirclose(parent);
	return (ENOENT);
}

/*
 * enter 'name' and tmpnode * 'tp' into directory of tmpnode 'dir' on tmpfs 'tm'
 */
tdirenter(tm, dir, name, tp, cred)
	struct tmount *tm;
	struct tmpnode *dir;
	char *name;
	struct tmpnode *tp;
	struct ucred *cred;
{
	int error;
	struct tdirent tdtemplate, *tdp;
	struct tdirent *tpdp;
	extern char *strcpy();

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tdirenter: tm %x dir %x name %s tp %x\n", tm, dir,
		    name, tp);
#endif TMPFSDEBUG
	/* check permissions */
	if (error = taccess(dir, cred, VEXEC))
		return (error);
	if (error = tdiropen(dir, cred)) {
		return (error);
	}
	if (error = taccess(dir, cred, VWRITE)) {
		tdirclose(dir);
		return (error);
	}
	/* set up the directory entry template */
	tdtemplate.td_namelen = strlen(name);
	tdtemplate.td_reclen = TDIRSIZ(&tdtemplate);
	if ((tdp = (struct tdirent *)tmp_memalloc(tm,
	    tdtemplate.td_reclen)) == NULL) {
		tdirclose(dir);
#ifdef TMPFSDEBUG
		if (tmpdebugerrs || tmpdirdebug)
			printf("tdirenter:- no space\n");
#endif TMPFSDEBUG
		return (ENOSPC);
	}
	dir->tn_attr.va_size += tdtemplate.td_reclen;

	/* now initialize the real dir entry */
	tdp->td_namelen = tdtemplate.td_namelen;
	tdp->td_reclen = tdtemplate.td_reclen;
	tdp->td_tmpnode = tp;
	tdp->td_next = NULL;
	(void) strcpy(tdp->td_name, name);

	/* check for virgin directory */
	if ((tpdp = dir->tn_dir) == NULL)
		dir->tn_dir = tdp;
	else {	/* install at end of of directory list */
		/* XXX FIX ME should install after . && .. ?? */
		while (tpdp->td_next != NULL)
			tpdp = tpdp->td_next;
		tpdp->td_next = tdp;
	}

	tdirclose(dir);
	tp->tn_attr.va_nlink++;
	tm->tm_direntries++;
	return (0);
}

/*
 * Delete entry whose tmpnode is tp from directory 'dir' on tmpfs 'tm'.
 * Remember to free dir entry space and decrement link count on tmpnode 'tp'
 */
/*ARGSUSED*/
tdirdelete(tm, dir, tp, nm, cred)
	register struct tmount *tm;
	register struct tmpnode *dir, *tp;
	register char *nm;
	struct ucred *cred;
{
	register struct tdirent *tpdp, *lasttdp = NULL;
	register int error;
	register u_int size;

#ifdef TMPFSDEBUG
	if (tmpfsdebug || tmpdirdebug)
		printf("tdirdelete: tm %x dir %x tp %x nm %s\n", tm, dir,
		    tp, nm);
#endif TMPFSDEBUG
	if (error = tdiropen(dir, cred)) {
		return (error);
	}
	if (error = taccess(dir, cred, VEXEC|VWRITE)) {
		return (error);
	}
	if ((dir->tn_attr.va_mode & TSVTX) && cred->cr_uid != 0 &&
	    cred->cr_uid != dir->tn_attr.va_uid &&
	    tp->tn_attr.va_uid != cred->cr_uid) {
		return (EPERM);
	}
	if ((tpdp = dir->tn_dir) == NULL)
		panic("null directory list");
	for (; tpdp; lasttdp = tpdp, tpdp = tpdp->td_next) {
		if ((tp == tpdp->td_tmpnode) &&
		    (strcmp(tpdp->td_name, nm) == 0)) {
			size = tpdp->td_reclen;
			dir->tn_attr.va_size -= size;
			tm->tm_direntries--;
			lasttdp->td_next = tpdp->td_next;
			tmp_memfree(tm, (char *)tpdp, size);
			ASSERT(tp->tn_attr.va_nlink > 0);
			tp->tn_attr.va_nlink--;
			if (tp->tn_attr.va_type == VDIR && tp != dir) {
				/*
				 * account for ".." entry in directory
				 */
				dir->tn_attr.va_nlink--;
			}
			return (0);
		}
	}
	return (ENOENT);
}

/*
 * check directory permission before doing an operation
 */
tdiropen(dir, cred)
	struct tmpnode *dir;
	struct ucred *cred;
{
	int error;

	if (error = taccess(dir, cred, VREAD)) {
		return (error);
	}
	if (dir->tn_attr.va_type != VDIR) {
		return (ENOTDIR);
	}
	return (0);
}

/*ARGSUSED*/
tdirclose(tp)
	struct tmpnode *tp;
{
}
