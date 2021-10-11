#ifndef lint
static  char sccsid[] = "@(#)auto_proc.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <netinet/in.h>
#include "nfs_prot.h"
#define NFSCLIENT
typedef nfs_fh fhandle_t;
#include <sys/mount.h>
#include "automount.h"

/*
 * add up sizeof (valid + fileid + name + cookie) - strlen(name)
 */
#define ENTRYSIZE (3 * BYTES_PER_XDR_UNIT + NFS_COOKIESIZE)
/*
 * sizeof(status + eof)
 */
#define JUNKSIZE (2 * BYTES_PER_XDR_UNIT)

attrstat *
nfsproc_getattr_2(fh)
	nfs_fh *fh;
{
	struct vnode *vnode;
	static attrstat astat;

	vnode = fhtovn(fh);
	if (vnode == NULL) {
		astat.status = NFSERR_STALE;
		return(&astat);
	}
	astat.status = NFS_OK;
	astat.attrstat_u.attributes = vnode->vn_fattr;
	return(&astat);
}

attrstat *
nfsproc_setattr_2(args)
	sattrargs *args;
{
	static attrstat astat;
	struct vnode *vnode;

	vnode = fhtovn(&args->file);
	if (vnode == NULL)
		astat.status = NFSERR_STALE;
	else
		astat.status = NFSERR_ROFS;
	return(&astat);
}

void *
nfsproc_root_2()
{
	return(NULL);
}


diropres *
nfsproc_lookup_2(args, cred)
	diropargs *args;
	struct authunix_parms *cred;
{
	struct vnode *vnode;
	static diropres res;
	nfsstat status;

	vnode = fhtovn(&args->dir);
	if (vnode == NULL) {
		res.status = NFSERR_STALE;
		return(&res);
	}
	if (vnode->vn_type != VN_DIR) {
		res.status = NFSERR_NOTDIR;
		return(&res);
	}
	status = lookup((struct autodir *)(vnode->vn_data),
		args->name, &vnode, cred);
	if (status != NFS_OK) {
		res.status = status;
		return(&res);
	}
	res.diropres_u.diropres.file = vnode->vn_fh;
	res.diropres_u.diropres.attributes = vnode->vn_fattr;
	res.status = NFS_OK;
	return(&res);
}
			
	
readlinkres *
nfsproc_readlink_2(fh, cred)
	nfs_fh *fh;
	struct authunix_parms *cred;
{
	struct vnode *vnode;
	struct link *link;
	static readlinkres res;
	nfsstat status;

	vnode = fhtovn(fh);
	if (vnode == NULL) {
		res.status = NFSERR_STALE;
		return(&res);
	}
	if (vnode->vn_type != VN_LINK) {
		res.status = NFSERR_STALE;	/* XXX: no NFSERR_INVAL */
		return(&res);
	}
	link = (struct link *)(vnode->vn_data);
	if (time_now >= link->link_death) {
		status = lookup(link->link_dir, link->link_name, &vnode, cred);
		if (status != NFS_OK) {
			free_link(link);
			res.status = status;
			return(&res);
		}
		link = (struct link *)(vnode->vn_data);
	}

	link->link_death = time_now + max_link_time;
	if (link->link_fs)
		link->link_fs->fs_rootfs->fs_death = time_now + max_link_time;
	res.readlinkres_u.data = link->link_path;
	res.status = NFS_OK;
	return(&res);
}

/*ARGSUSED*/
readres *
nfsproc_read_2(args)
	readargs *args;
{
	static readres res;

	res.status = NFSERR_ISDIR;	/* XXX: should return better error */
	return(&res);
}

void *
nfsproc_writecache_2()
{
	return(NULL);
}	

/*ARGSUSED*/
attrstat *
nfsproc_write_2(args)
	writeargs *args;
{
	static attrstat res;

	res.status = NFSERR_ROFS;	/* XXX: should return better error */
	return(&res);
}

/*ARGSUSED*/
diropres *
nfsproc_create_2(args, cred)
	createargs *args;
	struct authunix_parms *cred;
{
	static diropres res;

	res.status = NFSERR_ROFS;
	return(&res);
}

/*ARGSUSED*/
nfsstat *
nfsproc_remove_2(args)
	diropargs *args;
{
	static nfsstat status;

	status = NFSERR_ROFS;
	return(&status);
}

/*ARGSUSED*/
nfsstat *
nfsproc_rename_2(args)
	renameargs *args;
{
	static nfsstat status;

	status = NFSERR_ROFS;
	return(&status);
}

/*ARGSUSED*/
nfsstat *
nfsproc_link_2(args)
	linkargs *args;
{
	static nfsstat status;

	status = NFSERR_ROFS;
	return(&status);
}

/*ARGSUSED*/
nfsstat *
nfsproc_symlink_2(args, cred)
	symlinkargs *args;
	struct authunix_parms *cred;
{
	static nfsstat status;

	status = NFSERR_ROFS;
	return(&status);
}

/*ARGSUSED*/
diropres *
nfsproc_mkdir_2(args, cred)
	createargs *args;
	struct authunix_parms *cred;
{
	static diropres res;

	res.status = NFSERR_ROFS;
	return(&res);
}

/*ARGSUSED*/
nfsstat *
nfsproc_rmdir_2(args)
	diropargs *args;	
{
	static nfsstat status;

	status = NFSERR_ROFS;
	return(&status);
}

readdirres *
nfsproc_readdir_2(args)
	readdirargs *args;
{
	static readdirres res;

	struct vnode *vnode;
	struct entry *e, *nexte;
	struct entry **entp;
	struct autodir *dir;
	struct link *link;
	int cookie;
	int count;
	int entrycount;

	/*
	 * Free up old stuff
	 */
	e = res.readdirres_u.reply.entries;
	while (e != NULL) {
		nexte = e->nextentry;
		free((char *)e);
		e = nexte;
	}
	res.readdirres_u.reply.entries = NULL;

	vnode = fhtovn(&args->dir);
	if (vnode == NULL) {
		res.status = NFSERR_STALE;
		return(&res);
	}
	if (vnode->vn_type != VN_DIR) {
		res.status = NFSERR_NOTDIR;
		return(&res);
	}
	dir = (struct autodir *)vnode->vn_data;
	cookie = *(unsigned *)args->cookie;
	count = args->count - JUNKSIZE;

	entrycount = 0;
	entp = &res.readdirres_u.reply.entries;
	for (link = TAIL(struct link, dir->dir_head); link;
	    link = PREV(struct link, link)) {
		if (count <= ENTRYSIZE) 
			goto full;
		if (entrycount++ < cookie)
			continue;
		if (time_now >= link->link_death) {
			++cookie;
			continue;
		}
		*entp = (struct entry *) malloc(sizeof(entry));
		if (*entp == NULL) {
			syslog(LOG_ERR, "Memory allocation failed: %m");
			break;
		}
		(*entp)->fileid = link->link_vnode.vn_fattr.fileid;
		if (link->link_death && time_now >= link->link_death)
			(*entp)->fileid = 0;
		else
			(*entp)->fileid = link->link_vnode.vn_fattr.fileid;
		(*entp)->name = link->link_name;
		*(unsigned *)((*entp)->cookie) = ++cookie;
		(*entp)->nextentry = NULL;
		entp = &(*entp)->nextentry;
		count -= (ENTRYSIZE + strlen(link->link_name));
	}
full:
	if (count > ENTRYSIZE)
		res.readdirres_u.reply.eof = TRUE;
	else
		res.readdirres_u.reply.eof = FALSE;
	res.status = NFS_OK;
	return(&res);
}
		
statfsres *
nfsproc_statfs_2()
{
	static statfsres res;

	res.status = NFS_OK;
	res.statfsres_u.reply.tsize = 512;
	res.statfsres_u.reply.bsize = 512;
	res.statfsres_u.reply.blocks = 0;
	res.statfsres_u.reply.bfree = 0;
	res.statfsres_u.reply.bavail = 0;
	return(&res);
}
