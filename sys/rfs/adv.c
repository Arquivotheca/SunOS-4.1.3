/*	@(#)adv.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	advertise / unadvertise
 */

#include <sys/types.h>
#include <rfs/rfs_misc.h>
#include <rfs/sema.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <rpc/types.h>
#include <nfs/nfs.h>
#include <sys/mount.h>
#include <rfs/comm.h>
#include <sys/errno.h>
#include <rfs/adv.h>
#include <rfs/cirmgr.h>
#include <rfs/nserve.h>
#include <rfs/rfs_mnt.h>
#include <rfs/rfs_serve.h>
#include <sys/debug.h>
#include <rfs/rdebug.h>

extern	int	bootstate;
extern	int	nadvertise;
char	*addalist();
struct advertise	*getadv();
struct rcvd *	get_rcvd();
extern void remalist();

/*
 *	Advertise a file system service to the name server.
 *
 */

advfs()
{
	struct	a	{
		int 	opcode;		/* "System call" number */
		char	*fs;		/* root of file system */
		char	*svcnm;		/* global name given to name server */
		int	rwflag;		/* readonly/read write flag	*/
		char	**clist;	/* client list			*/
	} *uap = (struct a *) u.u_ap;
	struct	vnode	*vp;
	struct	advertise	*ap, *a, *findadv();
	rcvd_t	gift;			/* rcvd for advertised resource */

	char	*to, *from, adv_name [NMSZ];	/* temps */
	extern	rcvd_t	cr_rcvd();
	extern struct vnodeops rfs_vnodeops;
	int	readv = 0;	/* flag when this is a re-advertise */
	struct	rd_user *user;
	struct	gdp	*gdpp;

	if (bootstate != DU_UP)  {
		u.u_error = ENONET;
		return;
	}

	if (maxserve == 0) {
		u.u_error = ENOMEM;
		return;
	}
	if (!suser())  
		return;

	/* get the name to be advertised */
	switch (upath(uap->svcnm,adv_name,NMSZ)) {
	case -2:	/* too long	*/
	case  0:	/* too short	*/
		u.u_error = EINVAL;
		return;
	case -1:	/* bad user address	*/
		u.u_error = EFAULT;
		return;
	}

	/*
	 * if this is a modify request, all we need to do is replace
	 * the client list and return.
	 */
	if (uap->rwflag & A_MODIFY) {
		if ((ap = findadv (adv_name)) == NULL) {
			u.u_error = ENODEV;
			return;
		}
		if(ap->a_flags & A_MINTER) {
			u.u_error = ENODEV;
			return;
		}
		/* remalist/addalist   can not go to sleep */
		if (uap->rwflag & A_CLIST) {
			if (ap->a_clist)
				remalist(ap->a_clist);
		
			ap->a_clist = (uap->clist)?addalist(uap->clist):NULL;
		}
		return;
	}

	u.u_error = lookupname(uap->fs, UIOSEG_USER, FOLLOW_LINK,
		(struct vnode **)0, &vp);
	if (u.u_error)
		return;

	if (vp->v_op == &rfs_vnodeops) {
		u.u_error = ERREMOTE;
		VN_RELE(vp);
		return;
	}
	if (vp->v_type != VDIR)  {
		u.u_error = ENOTDIR;
		VN_RELE(vp);
		return;
	}
	if (is_adv(vp)) {
		u.u_error = EADV;
		VN_RELE(vp);
		return;
	}

	/* search the advertise table for a free slot */
	for (ap = advertise, a = 0; ap < &advertise[nadvertise]; ap++) {
		if (ap->a_flags == A_FREE) {
			if (!a)
				a = ap;
		} else  {
			if (!strcmp(ap->a_name, adv_name)) {
			/* if inode is the same, this is a re-advertise */
				if (ap->a_queue->rd_vnode == vp) {
					readv = TRUE;
					a = ap;
					continue;
				} else {
					u.u_error = EBUSY;
					VN_RELE(vp);
					return;
				}
			} 
			if (ap->a_queue->rd_vnode == vp) {
				u.u_error = EEXIST;
				VN_RELE(vp);
				return;
			}
		}
	}
		
	if (!(ap = a)) {
		VN_RELE(vp);
		u.u_error = ENOSPC;
		return;
	}

	if (readv) {
		/*
		 * read-write permissions on the readvertise must match 
		 * those already in the table.
		 */
		if ((uap->rwflag & A_RDONLY) && !(ap->a_flags & A_RDONLY)) {
			VN_RELE(vp);
			u.u_error = EACCES;
			return;
		}
		ap->a_flags &= ~A_MINTER;
	} else {
	
		ap->a_flags = A_INUSE;

		/*
	 	* if file system was mounted read only,
	 	* the advertisement must be read only.
	 	*/
		if (((vp->v_vfsp)->vfs_flag & VFS_RDONLY) && 
		    !(uap->rwflag & A_RDONLY)) {
			VN_RELE(vp);
			u.u_error = EROFS;
			ap->a_flags = A_FREE;
			return;
		}
		if (uap->rwflag & A_RDONLY)
			ap->a_flags |= A_RDONLY;
	}
	/*
	 * now add authorization list, if necessary.
	 */
	if ((uap->rwflag & A_CLIST) && uap->clist != NULL) {
		if ((ap->a_clist=addalist(uap->clist)) == NULL) {
			VN_RELE(vp);
			ap->a_flags = A_FREE;
			return;
		}
	}
	else
		ap->a_clist = NULL;

	/* 
	 * check to see if any current clients are not on the new client list.
	 * if so, fail the readvertise.  otherwise, bump counts and return
	 */
	if (readv) {
		if (ap->a_clist != NULL) {
			/* check each current user */
			for (user=ap->a_queue->rd_user_list; 
			     user != NULL; 
			     user=user->ru_next) {
				/* search gdp to find name for this sysid */
				for (gdpp=gdp; 
				     gdpp->sysid != 
					srmount[user->ru_srmntindx].sr_sysid; 
				     gdpp++)
					;
				if (checkalist(ap->a_clist, 
				    gdpp->token.t_uname)== FALSE) {
					u.u_error = ESRMNT;
					VN_RELE(vp);
					remalist(ap->a_clist);
					ap->a_flags |= A_MINTER;
					return;
				}
			}
		}
		ap->a_queue->rd_refcnt++;
		VN_RELE(vp);
		return;
	}

	/*
	 * allocate queue for advertised object.
	 */
	if ((gift = cr_rcvd (MOUNT_QSIZE, GENERAL)) == NULL) { 
			u.u_error = ENOMEM;
			VN_RELE(vp);
			if (ap->a_clist)
				remalist(ap->a_clist);
			ap->a_flags = A_FREE;
			return;
	}
	ap->a_count = 0;
	ap->a_queue = gift;
	for (from = adv_name, to = ap->a_name;
		*from != NULL  &&  from < &adv_name[NMSZ];)
			*to++ = *from++;
	*to = NULL;
	gift->rd_vnode = vp;	/* associate gift with vnode */
	DUPRINT2(DB_MNT_ADV,"exit adv: u_error is %d\n",u.u_error);
}

/*
 *	Unadvertise a remotely accessible filesystem.
 */

unadvfs()
{
	struct	a	{
		int opcode;		/* "system call" number */
		char	*svcnm;		/* global name given to name server */
	} *uap = (struct a *) u.u_ap;
	struct	advertise	*ap, *findadv();
	char	adv_name [NMSZ];	/* temp */

	if (bootstate != DU_UP)  {
		u.u_error = ENONET;
		return;
	}

	if (!suser()) 
		return;

	/* get the name to be unadvertised */
	switch (upath(uap->svcnm,adv_name,NMSZ)) {
	case -2:	/* too long	*/
	case  0:	/* too short	*/
		u.u_error = EINVAL;
		return;
	case -1:	/* bad user address	*/
		u.u_error = EFAULT;
		return;
	}

	if ((ap = findadv (adv_name)) == NULL) {
		u.u_error = ENODEV;
		return;

	}
	u.u_error = unadv(ap);
}

int
unadv(ap)
	struct	advertise	*ap;
{
	struct	vnode	*vp;

	if(ap->a_flags & A_MINTER)
		return ENODEV;

	ap->a_flags |= A_MINTER;

	vp = ap->a_queue->rd_vnode;
	del_rcvd(ap->a_queue, (struct serv_proc *) NULL);
	/*
	 * get rid of auth list regardless of whether entry goes
	 * away now or later.
	 */
	if (ap->a_clist) {
		remalist(ap->a_clist);
		ap->a_clist = NULL;
	}
	if (ap->a_count <= 0) {
		ap->a_flags = A_FREE;
		VN_RELE(vp);	/* advertise did the lookupname VN_HOLD */
	}
	/* If still have clients, vnode will be released when all clients
	   are done and the resource is runmounted */
	return 0;
}

/*  Advertise table lookup using vnode pointer as the key.
 */

struct advertise *
getadv(vp)
register struct	vnode *vp;
{
	register struct	advertise *ap;
	
	for (ap = advertise; ap < &advertise[nadvertise]; ap++)
		if (ap->a_flags & A_INUSE)
		if (ap->a_queue->rd_vnode == vp)
			return (ap);
	return (NULL);
}

/* Advertise table lookup using a_name as the key.
 */

struct advertise *
findadv(name)
char *name;
{
	register struct advertise *ap;

	for (ap=advertise; ap < &advertise[nadvertise]; ap++)
		if ((ap->a_flags & A_INUSE) &&
		    (strcmp(name,ap->a_name) == NULL))
			return(ap);
	return((struct advertise *) NULL);
}


/* Determine if a vnode is advertised. Searches the advertise table for
   a vnode matching vp. Returns 1 if found, else 0. This routine replaces
   the flag IADV attached to the inode to signal that it is advertised.
*/
int 
is_adv(vp)
register struct vnode *vp;
{

	extern int nadvertise;
	register struct advertise *advp;
	register struct advertise *endtbl = &advertise[nadvertise];

	for (advp = advertise; advp < endtbl; advp++) {
		if ((advp->a_flags & A_INUSE) && !(advp->a_flags & A_MINTER) && 
		     advp->a_queue->rd_vnode == vp )
			return(1);
	}
	return(0);
}
