#ifndef lint 
#ident "@(#)hsfs_node.c 1.1 92/07/30" 
#endif

/*
 * Directory operations for High Sierra filesystem
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ucred.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/dirent.h>

#include <hsfs/hsfs_spec.h>
#include <hsfs/hsfs_isospec.h>
#include <hsfs/hsfs_node.h>
#include <hsfs/hsfs_private.h>
#include <hsfs/hsfs_susp.h>
#include <hsfs/hsfs_rrip.h>

#include <vm/pvn.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <vm/seg_vn.h>
#include <vm/rm.h>
#include <vm/swap.h>
#include <machine/seg_kmem.h>


enum dirblock_result { FOUND_ENTRY, WENT_PAST, HIT_END };
static enum dirblock_result process_dirblock();

extern struct vnodeops hsfs_vnodeops;
extern char *strncpy();

static void hs_remhash();
static void hs_addfreeb();
static void hs_addfreef();

/*
 * hs_access
 * Return 0 if the desired access may be granted.
 * Otherwise return error code.
 */
int
hs_access(vp, m, cred)
	register struct vnode *vp;
	register int m;
	register struct ucred *cred;
{
	register int *gp;
	register struct hsnode *hp;

	if (m & VWRITE) {
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
			return (EROFS);
	}
	if (cred->cr_uid == 0)
		return (0);		/* super-user always gets access */

	hp = VTOH(vp);

	/*
	 * XXX - For now, use volume protections.
	 *	 Also, always grant EXEC access for directories
	 *	 if READ access is granted.
	 */
	if ((vp->v_type == VDIR) && (m & VEXEC)) {
		m &= ~VEXEC;
		m |= VREAD;
	}
	if (cred->cr_uid != hp->hs_dirent.uid) {
		m >>= 3;
		/*
		 * Have to check groups manually since groupmember()
		 * checks uarea instead of a credentials list.
		 */
		if (cred->cr_gid == hp->hs_dirent.gid)
			goto found;
		for (gp = cred->cr_groups;
		    gp < &(cred->cr_groups[NGROUPS]) && *gp != NOGROUP;
		    gp++)
			if (*gp == hp->hs_dirent.gid)
				goto found;
		m >>= 3;
	}
found:
	if ((m & hp->hs_dirent.mode) == m)
		return (0);
	return (EACCES);
}

#if ((HS_HASHSIZE & (HS_HASHSIZE - 1)) == 0)
#define	HS_HASH(l)	((u_long)(l) & (HS_HASHSIZE - 1))
#else
#define	HS_HASH(l)	((u_long)(l) % HS_HASHSIZE)
#endif
#define	HS_HPASH(hp)	HS_HASH((hp)->hs_dirent.ext_lbn)		\

extern int physmem;

/*
 * initialize incore hsnode table size
 *
 */
struct hstable *
hs_inithstbl(vfsp)
	struct vfs *vfsp;
{
	register struct hstable *htp;
	int size;
	int i;
	int nohsnode;

	/* allocate incore hsnode space based on memory size */
	/* minimum 16 K for 4M machine, 64K for others */
	size = (physmem <= 512) ? HS_HSTABLESIZE : 4 * HS_HSTABLESIZE;

	htp = (struct hstable *)new_kmem_alloc((u_int)size, KMEM_SLEEP);
	htp -> hs_vfsp = vfsp;
	htp -> hs_tablesize = size;
	for (i = 0; i < HS_HASHSIZE; i++)
		htp->hshash[i] = NULL;
	htp->hsfree_f = NULL;
	htp->hsfree_b = NULL;
	htp->hs_refct = 0;
	htp->hs_nohsnode = nohsnode
		= (size - sizeof (struct hstable) + sizeof (struct hsnode))/
		sizeof (struct hsnode);

	for (i = 0; i < nohsnode; i++) {
		htp->hs_node[i].hs_dirent.ext_lbn = 0;
		htp->hs_node[i].hs_hash = NULL;
		htp->hs_node[i].hs_freef = NULL;
		htp->hs_node[i].hs_freeb = NULL;
		htp->hs_node[i].hs_dirent.sym_link = (char *)NULL;
		(void) hs_addfreeb(htp, &htp->hs_node[i]);
	}

	/*
	 * could initialize more stuff in this routine (e.g. vnode)
	 * do it next time...
	 */

	return (htp);
}

/*
 * initialize incore hsnode table size
 *
 */
void
hs_freehstbl(vfsp)
	struct vfs *vfsp;
{
	register struct hstable *htp;

	htp = ((struct hsfs *)VFS_TO_HSFS(vfsp))->hsfs_hstbl;
	kmem_free((caddr_t) htp, (u_int) htp->hs_tablesize);
}

/*
 * Add an hsnode to the end of the free list.
 */
static void
hs_addfreeb(htp, hp)
	register struct hstable *htp;
	register struct hsnode *hp;
{
	register struct hsnode *ep;

	ep = htp->hsfree_b;
	htp->hsfree_b = hp;		/* hp is the last entry in free list */
	hp->hs_freef = NULL;
	hp->hs_freeb = ep;		/* point at previous last entry */
	if (ep == NULL)
		htp->hsfree_f = hp;	/* hp is only entry in free list */
	else
		ep->hs_freef = hp;	/* point previous last entry at hp */
}

/*
 * Add an hsnode to the front of the free list.
 */
static void
hs_addfreef(htp, hp)
	register struct hstable *htp;
	register struct hsnode *hp;
{
	register struct hsnode *ep;

	ep = htp->hsfree_f;
	htp->hsfree_f = hp;		/* hp is the first entry in free list */
	hp->hs_freeb = NULL;
	hp->hs_freef = ep;		/* point at previous last entry */
	if (ep == NULL)
		htp->hsfree_b = hp;	/* hp is only entry in free list */
	else
		ep->hs_freeb = hp;	/* point previous last entry at hp */
}

/*
 * Get an hsnode from the front of the free list.
 */
static struct hsnode *
hs_getfree(htp)
	register struct hstable *htp;
{
	register struct hsnode *hp;

	hp = htp->hsfree_f;
	if (hp != NULL) {
		htp->hsfree_f = hp->hs_freef;
		if (htp->hsfree_f != NULL)
			htp->hsfree_f->hs_freeb = NULL;
		else
			htp->hsfree_b = NULL;
	} else
		printf("hsfs: hsnode table full\n");
	return (hp);
}

/*
 * Remove an hsnode from the free list.
 */
static void
hs_remfree(htp, hp)
	register struct hstable *htp;
	register struct hsnode *hp;
{

	if (hp->hs_freef != NULL)
		hp->hs_freef->hs_freeb = hp->hs_freeb;
	else
		htp->hsfree_b = hp->hs_freeb;
	if (hp->hs_freeb != NULL)
		hp->hs_freeb->hs_freef = hp->hs_freef;
	else
		htp->hsfree_f = hp->hs_freef;
}

/*
 * Look for hsnode in hash list.
 * Check equality of fsid and starting block of data extent.
 * If found, reactivate it if inactive.
 */
struct vnode *
hs_findhash(ext, vfsp)
	u_int ext;
	struct vfs *vfsp;
{
	register struct hsnode *tp;
	register struct hstable *htp;

	htp = ((struct hsfs *)VFS_TO_HSFS(vfsp))->hsfs_hstbl;
	for (tp = htp->hshash[HS_HASH(ext)]; tp != NULL; tp = tp->hs_hash) {
		if (tp->hs_dirent.ext_lbn == ext) {
			if ((HTOV(tp)->v_count)++ == 0) {
				/*
				 * reactivating a free hsnode:
				 * remove from free list
				 */
				hs_remfree(htp, tp);
				(htp->hs_refct)++;
			}
			return (HTOV(tp));
		}
	}
	return (NULL);
}

static void
hs_addhash(htp, hp)
	register struct hstable *htp;
	struct hsnode *hp;
{
	register u_int hashno;

	hashno = (u_int)HS_HPASH(hp);
	hp->hs_hash = htp->hshash[hashno];
	htp->hshash[hashno] = hp;
}

static void
hs_remhash(htp, hp)
	register struct hstable *htp;
	struct hsnode *hp;
{
	register struct hsnode **tp;

	for (tp = &htp->hshash[HS_HPASH(hp)];
		*tp != NULL; tp = &(*tp)->hs_hash) {
		if (*tp == hp) {
			struct vnode *vp;

			vp = HTOV(hp);
			/* file is no longer reference, destroy all old pages */
			if (vp->v_pages != NULL)
				/*  pvn_vplist_dirty will abort all old pages */
				(void) pvn_vplist_dirty(vp, 0, B_INVAL);
			*tp = hp->hs_hash;
			break;
		}
	}
}

/* destroy all old pages */
void
hs_synchash(vfsp)
	struct vfs *vfsp;
{
	register struct hstable *htp;
	register int i;
	register struct hsnode *hp;

	htp = ((struct hsfs *)VFS_TO_HSFS(vfsp))->hsfs_hstbl;
	for (i = 0; i < HS_HASHSIZE; i++) {
		for (hp = htp->hshash[i]; hp != NULL; hp = hp->hs_hash) {
			if ((HTOV(hp))->v_pages != NULL)
				(void) pvn_vplist_dirty((HTOV(hp)), 0, B_INVAL);
		}
	}

	/* now destroy all sym_link buffers */
	for (i = 0; i < HS_HASHSIZE; i++) {
		for (hp = htp->hshash[i]; hp != NULL; hp = hp->hs_hash) {
			if (hp->hs_dirent.sym_link != (char *)NULL) {
				kmem_free((caddr_t)hp->hs_dirent.sym_link,
				(u_int) SYM_LINK_LEN(hp->hs_dirent.sym_link));
				hp->hs_dirent.sym_link = (char *)NULL;
			}
		}
	}
}

/*
 * hs_makenode
 *
 * Construct an hsnode.
 * Caller specifies the directory entry, the block number and offset
 * of the directory entry, and the vfs pointer.
 * note: off is the sector offset, not lbn offset
 * if NULL is returned implies file system hsnode table full
 */
struct vnode *
hs_makenode(dp, lbn, off, vfsp)
	struct hs_direntry *dp;
	u_int lbn;
	u_int off;
	struct vfs *vfsp;
{
	register struct hsnode *hp;
	register struct vnode *vp;
	register struct hstable *htp;
	struct vnode *newvp;


	htp = ((struct hsfs *)VFS_TO_HSFS(vfsp))->hsfs_hstbl;
	/* look for hsnode in cache first */
	if ((vp = hs_findhash(dp->ext_lbn, vfsp)) == NULL) {
		/*
		 * not in cache, get one off freelist or else allocate one
		 */
		if ((hp = hs_getfree(htp)) != NULL) {
			hs_remhash(htp, hp);
		} else {
			return (NULL);
		}

		if (hp->hs_dirent.sym_link != (char *)NULL)
			kmem_free(hp->hs_dirent.sym_link,
				(u_int) SYM_LINK_LEN(hp->hs_dirent.sym_link));

		bzero((caddr_t)hp, sizeof (*hp));
		bcopy((caddr_t)dp, (caddr_t)&hp->hs_dirent, sizeof (*dp));
		hp->hs_dir_lbn = lbn;
		hp->hs_dir_off = off;
		hp->hs_offset = 0;
		if (off > HS_SECTOR_SIZE)
			printf("hs_makenode: bad offset\n");

		/* initialize for VDIR */
		hp->hs_ptbl_idx = NULL;
		vp = HTOV(hp);
		VN_INIT(vp, vfsp, dp->type, dp->r_dev);
		vp->v_op = &hsfs_vnodeops;
		vp->v_data = (caddr_t)hp;


		/*
		* if it's a device, call specvp
		*/
		if (ISVDEV(vp->v_type)) {
			newvp = specvp(vp, vp->v_rdev, vp->v_type);
			VN_RELE(vp);
			vp = newvp;
			goto end;
		}

		hs_addhash(htp, hp);
		(htp->hs_refct)++;
	}

end:
	return (vp);
}

/*
 * hs_freenode
 *
 * Deactivate an hsnode.
 * Leave it on the hash list but put it on the free list.
 * if the vnode does not have any pages, put in front of free list
 * else put in back of the free list
 *
 */
void
hs_freenode(hp, vfsp, nopage)
	struct hsnode *hp;
	struct vfs *vfsp;
	int nopage;		/* 1 if no page, 0 otherwise */
{
	register struct hstable *htp;

	htp = ((struct hsfs *)VFS_TO_HSFS(vfsp))->hsfs_hstbl;
	if (nopage)
		hs_addfreef(htp, hp); /* add to front of free list */
	else
		hs_addfreeb(htp, hp); /* add to back of free list */
	(htp->hs_refct)--;
}

/*
 * hs_remakenode
 *
 * Reconstruct a vnode given the location of its directory entry.
 * Caller specifies the the block number and offset
 * of the directory entry, and the vfs pointer.
 * Returns an error code or 0.
 */
int
hs_remakenode(lbn, off, vfsp, vpp)
	u_int lbn;
	u_int off;
	struct vfs *vfsp;
	struct vnode **vpp;
{
	register struct hsfs *fsp;
	register u_int secno;
	u_char *dirp;
	struct hs_direntry hd;
	int error;
	u_char *buffer;

	buffer = (u_char *)new_kmem_alloc((size_t)HS_SECTOR_SIZE, KMEM_SLEEP);

	if (buffer == (u_char *)NULL)
		return (ENOMEM);

	bzero ((caddr_t) &hd, sizeof (hd));

	/* Convert to sector and offset */
	fsp = VFS_TO_HSFS(vfsp);
	if (off > HS_SECTOR_SIZE) {
		printf("hs_remakenode: bad offset\n");
		error = EINVAL;
		goto end;
	}
	secno = LBN_TO_SEC(lbn, vfsp);
	dirp = buffer;

	/* Read sector and parse directory entry */
	if (error = hs_readsector(fsp->hsfs_devvp, secno, dirp))
		goto end;

	error = hs_parsedir(fsp, &dirp[off], &hd, (char *)NULL, (int *)NULL);

	if (!error) {
		*vpp = hs_makenode(&hd, lbn, off, vfsp);
		if (*vpp == NULL)
			error = ENFILE;
	}

end:
	(void) kmem_free((caddr_t)buffer, (size_t)HS_SECTOR_SIZE);
	return (error);
}


/*
 * hs_dirlook
 *
 * Look for a given name in a given directory.
 * If found, construct an hsnode for it.
 */
int
hs_dirlook(dvp, name, namlen, vpp, cred)
	struct vnode *dvp;
	register char *name;
	int namlen;			/* length of 'name' */
	struct vnode **vpp;
	struct ucred *cred;
{
	register struct hsnode *dhp;
	struct hsfs *fsp;
	int error = 0;
	u_int offset;			/* real offset in directory */
	u_int last_offset;		/* last index into current dir block */
	char *cmpname;			/* case-folded name */
	int cmpnamelen;
	int adhoc_search;		/* did we start at begin of dir? */
	int end;
	u_int hsoffset;
	struct fbuf *fbp;
	int bytes_wanted;
	int dirsiz;
	int is_rrip;

	if (error = hs_access(dvp, (int) VEXEC, cred))
		return (error);

	cmpname = (char *)new_kmem_alloc(MAXNAMELEN + 1, KMEM_SLEEP);
	if (cmpname == (char *)NULL)
		return (ENOMEM);

	dhp = VTOH(dvp);
	fsp = VFS_TO_HSFS(dvp->v_vfsp);

	is_rrip = IS_RRIP_IMPLEMENTED(fsp);

	/*
	 * For the purposes of comparing the name against dir entries,
	 * fold it to upper case.
	 */
	if (is_rrip) {
		(void) strcpy(cmpname, name);
		cmpnamelen = namlen;
	} else {
		cmpnamelen = hs_uppercase_copy(name, cmpname, namlen);
	}

	/* make sure dirent is filled up with all info */
	if (dhp->hs_dirent.ext_size == 0)
		hs_filldirent(dvp, &dhp->hs_dirent);

	/*
	 * No lock is needed - hs_offset is used as starting
	 * point for searching the directory.
	 */
	offset = dhp->hs_offset;
	hsoffset = offset;
	adhoc_search = (offset != 0);

	end = dhp->hs_dirent.ext_size;
	dirsiz = end;

tryagain:

	while (offset < end) {

		if ((offset & MAXBMASK) + MAXBSIZE > dirsiz)
			bytes_wanted = dirsiz - (offset & MAXBMASK);
		else
			bytes_wanted = MAXBSIZE;

		error = fbread(dvp, (offset & MAXBMASK),
			(unsigned int) bytes_wanted, S_READ, &fbp);
		if (error) {
			fbrelse(fbp, S_READ);
			goto done;
		}
		last_offset = (offset & MAXBMASK) + fbp->fb_count - 1;

#define	rel_offset(offset) ((offset) & MAXBOFFSET)  /* index into cur blk */

		switch (process_dirblock((u_char *)fbp->fb_addr, &offset,
					last_offset, cmpname,
					cmpnamelen, fsp, dhp, dvp,
					vpp, &error, is_rrip)) {
		case FOUND_ENTRY:
			/* found an entry, either correct or not */
			fbrelse(fbp, S_READ);
			goto done;

		case WENT_PAST:
			/*
			 * If we get here we know we didn't find it on the
			 * first pass. If adhoc_search, then we started a
			 * bit into the dir, and need to wrap around and
			 * search the first entries.  If not, then we started
			 * at the beginning and didn't find it.
			 */
			fbrelse(fbp, S_READ);
			if (adhoc_search) {
				offset = 0;
				end = hsoffset;
				adhoc_search = 0;
				goto tryagain;
			} else {
				error = ENOENT;
				goto done;
			}
			/* NOTREACHED */
			break;

		case HIT_END:
			fbrelse(fbp, S_READ);
			goto tryagain;
		}
	}
	/*
	 * End of all dir blocks, didn't find entry.
	 */
	if (adhoc_search) {
		offset = 0;
		end = hsoffset;
		adhoc_search = 0;
		goto tryagain;
	}
	error = ENOENT;
done:
	kmem_free(cmpname, MAXNAMELEN + 1);
	return (error);
}


/*
 * hs_parsedir
 *
 * Parse a Directory Record into an hs_direntry structure.
 * High Sierra and ISO directory are almost the same
 * except the flag and date
 */
int
hs_parsedir(fsp, dirp, hdp, dnp, dnlen)
	struct hsfs *fsp;
	u_char *dirp;
	struct hs_direntry *hdp;
	char *dnp;
	int *dnlen;
{
	u_char flags;
	int namelen, error;
	int name_change_flag = 0;	/* set if name was gotten in SUA */

	hdp->ext_lbn = HDE_EXT_LBN(dirp);
	hdp->ext_size = HDE_EXT_SIZE(dirp);
	hdp->xar_len = HDE_XAR_LEN(dirp);
	hdp->intlf_sz = HDE_INTRLV_SIZE(dirp);
	hdp->intlf_sk = HDE_INTRLV_SKIP(dirp);
	if (fsp->hsfs_vol_type == HS_VOL_TYPE_HS) {
		flags = HDE_FLAGS(dirp);
		(void) hs_parse_dirdate(HS_VOL_TYPE_HS,
			HDE_cdate(dirp), &hdp->cdate);
		(void) hs_parse_dirdate(HS_VOL_TYPE_HS,
			HDE_cdate(dirp), &hdp->adate);
		(void) hs_parse_dirdate(HS_VOL_TYPE_HS,
			HDE_cdate(dirp), &hdp->mdate);
		if (HDE_REGULAR_FILE(flags)) {
			hdp->type = VREG;
			hdp->mode = IFREG;
			hdp->nlink = 1;
		} else if (HDE_REGULAR_DIR(flags)) {
			hdp->type = VDIR;
			hdp->mode = IFDIR;
			hdp->nlink = 2;
		} else {
			printf("hsfs: filetype (0x%x) not supported\n", flags);
			return (EINVAL);
		}
		hdp->uid = fsp -> hsfs_vol.vol_uid;
		hdp->gid = fsp -> hsfs_vol.vol_gid;
		hdp->mode = hdp-> mode | (fsp -> hsfs_vol.vol_prot & 0777);
	} else if (fsp->hsfs_vol_type == HS_VOL_TYPE_ISO) {
		flags = IDE_FLAGS(dirp);
		(void) hs_parse_dirdate(HS_VOL_TYPE_ISO,
			IDE_cdate(dirp), &hdp->cdate);
		(void) hs_parse_dirdate(HS_VOL_TYPE_ISO,
			IDE_cdate(dirp), &hdp->adate);
		(void) hs_parse_dirdate(HS_VOL_TYPE_ISO,
			IDE_cdate(dirp), &hdp->mdate);
		if (HDE_REGULAR_FILE(flags)) {
			hdp->type = VREG;
			hdp->mode = IFREG;
			hdp->nlink = 1;
		} else if (HDE_REGULAR_DIR(flags)) {
			hdp->type = VDIR;
			hdp->mode = IFDIR;
			hdp->nlink = 2;
		} else {
			printf("hsfs: filetype (0x%x) not supported\n", flags);
			return (EINVAL);
		}
		hdp->uid = fsp -> hsfs_vol.vol_uid;
		hdp->gid = fsp -> hsfs_vol.vol_gid;
		hdp->mode = hdp-> mode | (fsp -> hsfs_vol.vol_prot & 0777);
                /*
		* Having this all filled in, let's see if we have any
		* SUA susp to look at.
		*/
		if (IS_SUSP_IMPLEMENTED(fsp)) {
			error = parse_sua((u_char *)dnp, dnlen,
					&name_change_flag, dirp, hdp, fsp,
					(u_char *)NULL, NULL);
			if (error) {
				if (hdp->sym_link) {
					kmem_free((caddr_t)hdp->sym_link,
					(u_int) SYM_LINK_LEN(hdp->sym_link));
					hdp->sym_link = (char *)NULL;
				}
				return (error);
			}
		}

	}
	hdp->xar_prot = (HDE_PROTECTION & flags) != 0;

#if dontskip
	if (hdp->xar_len > 0) {
		printf("hsfs: extended attributes not supported\n");
		return (EINVAL);
	}
#endif

	/* check interleaf size and skip factor */
	/* must both be zero or non-zero */
	if (hdp->intlf_sz + hdp->intlf_sk) {
		if ((hdp->intlf_sz == 0) || (hdp->intlf_sk == 0)) {
			printf("hsfs: interleaf size or skip factor error\n");
			return (EINVAL);
		}
		if (hdp->ext_size == 0) {
			printf("hsfs:");
			printf(" interleaving specified on zero length file\n");
			return (EINVAL);
		}
	}

	if (HDE_VOL_SET(dirp) != 1) {
		if (fsp->hsfs_vol.vol_set_size != 1 &&
		    fsp->hsfs_vol.vol_set_size != HDE_VOL_SET(dirp)) {
			printf("hsfs: multivolume file?\n");
			return (EINVAL);
		}
	}

        /*
	* If the name changed, then the NM field for RRIP was hit and
	* we should not copy the name again, just return.
	*/
	if (NAME_HAS_CHANGED(name_change_flag))
		return (0);



	/* return the pointer to the directory name and its length */
	if (dnp != NULL)
		namelen = hs_namecopy((char *) HDE_name(dirp), dnp,
			(int) HDE_NAME_LEN(dirp));
	else
		namelen = (int) HDE_NAME_LEN(dirp);
	if (dnlen != NULL)
		*dnlen = namelen;

	return (0);
}

/*
 * hs_namecopy
 *
 * Parse a file/directory name into UNIX form.
 * Delete trailing blanks, upper-to-lower case, add NULL terminator.
 * Returns the (possibly new) length.
 */
int
hs_namecopy(from, to, size)
	char *from;
	char *to;
	int size;
{
	register u_int i;
	register u_char c;
	register int lastspace;

	/* special handling for '.' and '..' */
	if (size == 1) {
		if (*from == '\0') {
			*to++ = '.';
			*to = '\0';
			return (1);
		} else if (*from == '\1') {
			*to++ = '.';
			*to++ = '.';
			*to = '\0';
			return (2);
		}
	}

	for (i = 0, lastspace = -1; i < size; i++) {
		c = from[i] & 0x7f;
		if (c == ';')
			break;
		if (c <= ' ') {
			if (lastspace == -1)
				lastspace = i;
		} else
			lastspace = -1;
		if ((c >= 'A') && (c <= 'Z'))
			c += 'a' - 'A';
		to[i] = c;
	}
	if (lastspace != -1)
		i = lastspace;
	to[i] = '\0';
	return (i);
}

/*
 * hs_uppercase_copy
 *
 * Convert a UNIX-style name into its HSFS equivalent.
 * Map to upper case.
 * Returns the (possibly new) length.
 */
hs_uppercase_copy(from, to, size)
	char *from;
	char *to;
	int size;
{
	register u_int i;
	register u_char c;

	/* special handling for '.' and '..' */

	if (size == 1 && *from == '.') {
		*to = '\0';
		return (1);
	} else if (size == 2 && *from == '.' && *(from+1) == '.') {
		*to = '\1';
		return (1);
	}

	for (i = 0; i < size; i++) {
		c = *from++;
		if ((c >= 'a') && (c <= 'z'))
			c = c - 'a' + 'A';
		*to++ = c;
	}
	return (size);
}

void
hs_filldirent(vp, hdp)
	struct vnode *vp;
	struct hs_direntry *hdp;
{
	u_int 	secno;
	u_int	secoff;
	struct vnode *realvp;
	struct hsfs *fsp;
	u_char *secp;
	u_char *buffer;

	if (vp->v_type != VDIR) {
		printf("hsfs_filldirent: vp (0x%x) not a directory", vp);
		return;
	}

	buffer = (u_char *)new_kmem_alloc((size_t)HS_SECTOR_SIZE, KMEM_SLEEP);

	if (buffer == (u_char *)NULL)
		return;

	secp = buffer;
	fsp = VFS_TO_HSFS(vp ->v_vfsp);
	realvp = fsp -> hsfs_devvp;
	secno = LBN_TO_SEC(hdp->ext_lbn+hdp->xar_len, vp->v_vfsp);
	secoff = LBN_TO_BYTE(hdp->ext_lbn+hdp->xar_len, vp->v_vfsp) &
			MAXHSOFFSET;
	if (hs_readsector(realvp, secno, secp))
		goto end;

	/* quick check */
	if (hdp->ext_lbn != HDE_EXT_LBN(&secp[secoff])) {
		printf("hsfs_filldirent: dirent not match\n");
		/* keep on going */
	}
	(void) hs_parsedir(fsp, &secp[secoff], hdp, (char *)NULL, (int *)NULL);

end:
	(void) kmem_free((caddr_t)buffer, (size_t)HS_SECTOR_SIZE);
	return;
}



/*
 * Look through a directory block for a matching entry.
 */
static enum dirblock_result
process_dirblock(blkp, offset, last_offset, nm, nmlen, fsp, dhp, dvp, vpp,
		error, is_rrip)
	u_char *blkp;		/* dir block */
	u_int *offset;		/* lower index */
	u_int last_offset;	/* upper index */
	char *nm;		/* name to compare against */
	int nmlen;		/* length of name */
	struct hsfs *fsp;
	struct hsnode *dhp;
	struct vnode *dvp;
	struct vnode **vpp;
	int *error;		/* return value: errno */
	int is_rrip;		/* 1 if rock ridge is implemented */
{
	char *dname;		/* name in directory entry */
	int dnamelen;		/* length of name */
	struct hs_direntry hd;
	int hdlen;
	register u_char *dirp;	/* the directory entry */
	int res, parsedir_res;
	char	*rrip_name_str;
	char	*rrip_tmp_name;
	enum dirblock_result	err;

	if (is_rrip) {
		rrip_name_str = (char *)new_kmem_alloc(MAXNAMELEN + 1,
								KMEM_SLEEP);
		if (rrip_name_str == (char *)NULL) {
			/*
			 * XXX we really should return an error indication
			 * here
			 */
			return (HIT_END);
		}
		rrip_tmp_name = (char *)new_kmem_alloc(MAXNAMELEN + 1,
								KMEM_SLEEP);
		if (rrip_tmp_name == (char *)NULL) {
			/*
			 * XXX we really should return an  error indication
			 * here
			 */
			kmem_free(rrip_name_str, MAXNAMELEN + 1);
			return (HIT_END);
		}

		rrip_name_str[0] = '\0';
		rrip_tmp_name[0] = '\0';
	}

	while (*offset < last_offset) {

		/*
		 * Directory Entries cannot span sectors.
		 * Unused bytes at the end of each sector are zeroed.
		 * Therefore, detect this condition when the size
		 * field of the directory entry is zero.
		 */
		hdlen = (int)((u_char)
				HDE_DIR_LEN(&blkp[rel_offset(*offset)]));
		if (hdlen == 0) {
			/* advance to next sector boundary */
			*offset = (*offset & MAXHSMASK) + HS_SECTOR_SIZE;

			if (*offset > last_offset) {
				err = HIT_END;	/* end of block */
				goto do_ret;
			} else
				continue;
		}

		/*
		 * Zero out the hd to read new directory
		 */
		bzero((caddr_t)&hd, sizeof (hd));

		/*
		 * Just ignore invalid directory entries.
		 * XXX - maybe hs_parsedir() will detect EXISTENCE bit
		 */
		dirp = &blkp[rel_offset(*offset)];
		dname = (char *)HDE_name(dirp);
		dnamelen = (int) ((u_char) HDE_NAME_LEN(dirp));

		/*
		 * If the rock ridge is implemented, then we copy the name
		 * from the SUA area to rrip_name_str. If no Alternate
		 * name is found, then use the uppercase NM in the
		 * rrip_name_str char array.
		 */
		if (is_rrip) {
			int rr_namelen;

			rrip_name_str[0] = '\0';
			rr_namelen = rrip_namecopy(nm, &rrip_name_str[0],
					&rrip_tmp_name[0], dirp, fsp, &hd);
			if (rr_namelen == -1){ /* use iso name instead */
				register int i;
				for (i = dnamelen; (dname[i] != ';') &&
						(i >= 0); i--);
				if (dname[i] == ';')
					dnamelen = i;
			} else{
				dname = (char *)&rrip_name_str[0];
				dnamelen = rr_namelen;
			}
		} else {
			register int i;
			/*
			 * make sure that we get rid of ';' in the dname of
			 * an iso direntry, as we should have no knowledge
			 * of file versions.
			 */
			for (i = dnamelen; (dname[i] != ';') && (i >= 0); i--);

			if (dname[i] == ';')
				dnamelen = i;
		}

		/*
		 * Quickly screen for a non-matching entry, but not for RRIP.
		 */
		if (!is_rrip && *nm < *dname) {
			err = WENT_PAST;
			goto do_ret;
		}

		if (*nm != *dname || nmlen != dnamelen) {
			/* look at next dir entry */
			RESTORE_NM(rrip_tmp_name, nm);
			*offset += hdlen;
			continue;
		}

		if ((res = nmcmp(dname, nm, nmlen)) == 0) {
			/* name matches */
			parsedir_res =
				hs_parsedir(fsp, dirp, &hd, (char *)NULL,
					    (int *) NULL);
			if (!parsedir_res) {
				u_int lbn;	/* logical block number */
				u_int secoff;

				lbn = dhp->hs_dirent.ext_lbn +
					dhp->hs_dirent.xar_len +
						(*offset/
						fsp->hsfs_vol.lbn_size);
				secoff = *offset & MAXHSOFFSET;
				*vpp = hs_makenode(&hd, lbn,
						secoff, dvp->v_vfsp);
				if (*vpp == NULL) {
					*error = ENFILE;
					RESTORE_NM(rrip_tmp_name, nm);
					err = FOUND_ENTRY;
					goto do_ret;
				}
				dhp->hs_offset = *offset;
				RESTORE_NM(rrip_tmp_name, nm);
				err = FOUND_ENTRY;
				goto do_ret;
			} else {
				/* improper dir entry */
				*error = parsedir_res;
				RESTORE_NM(rrip_tmp_name, nm);
				err = FOUND_ENTRY;
				goto do_ret;
			}
		} else if (!is_rrip && res < 0) {
			/* name < dir entry */
			err = WENT_PAST;
			goto do_ret;
		}
		/*
		 * name > dir entry,
		 * look at next one.
		 */
		*offset += hdlen;
	}
	RESTORE_NM(rrip_tmp_name, nm);
	err = HIT_END;

do_ret:
	if (is_rrip) {
		kmem_free(rrip_name_str, MAXNAMELEN + 1);
		kmem_free(rrip_tmp_name, MAXNAMELEN + 1);
	}
	return (err);
}


/*
 * Compare the names, returning < 0 if a < b,
 * 0 if a == b, and > 0 if a > b.
 */
static int
nmcmp(a, b, len)
	register char *a;
	register char *b;
	register int len;
{
	while (len--) {
		if (*a == *b) {
			b++; a++;
		} else {
			/* if file version, stop */
			if ((*a == ';') && (*b == '\0'))
				return (0);
			return ((u_char)*b - (u_char)*a);
		}
	}
	return (0);
}
