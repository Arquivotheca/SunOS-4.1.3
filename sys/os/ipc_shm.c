/*	@(#)ipc_shm.c 1.1 92/07/30 SMI; from S5R3.1 10.11	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Inter-Process Communication Shared Memory Facility.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/mman.h>

#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>
#include <vm/seg_vn.h>
#include <vm/anon.h>

struct	shmid_ds	*shmconv();
struct	ipc_perm	*ipcget();
struct	anon_map	*shm_vmlookup();

extern struct seg	*as_segat();

extern struct anon_map	*anonmap_fast_alloc();
extern void		 anonmap_fast_free();

/*
 * Shmat (attach shared segment) system call.
 */
shmat()
{
	register struct a {
		int	shmid;
		addr_t	addr;
		int	flag;
	}	*uap = (struct a *)u.u_ap;

	/*
	 * Clear special flag which is not accessible from user mode.
	 * For VPIX, we had to allow a way to specify the the address
	 * 0 is to be the actual attach address.  Normally 0 means to let
	 * the system pick.  To this end, if addr is 0 and SHM_RND is set,
	 * this tells the system to use the address 0.  We clear SHM_RND
	 * here that that from user land this cannot be done.  This assures
	 * compatibility.
	 */
	if (uap->addr == 0)
		uap->flag &= ~SHM_RND;

	(void) kshmat(uap->shmid, uap->addr, uap->flag, 0, 0);
}

/*
 * Shmat (attach shared segment) kernel callable routine.
 */
kshmat(shmid, addr, flag, size, offset)
	int shmid;
	addr_t addr;
	int flag;
	u_int size;
	u_int offset;
{
	register struct shmid_ds	*sp;	/* shared memory header ptr */
	struct segvn_crargs		crargs;	/* segvn create arguments */

	u.u_error = 0;		/* init error code */

	if ((sp = shmconv(shmid)) == NULL)
		return (-1);
	if ((flag & SHM_RDONLY) == 0) {
		if (ipcaccess(&sp->shm_perm, SHM_W))
			goto errret;
	}
	if (ipcaccess(&sp->shm_perm, SHM_R))
		goto errret;

	if (size == 0)
		size = sp->shm_segsz;   /* map as much as possible */

	if (size > sp->shm_segsz || (offset & PAGEOFFSET) ||
		offset + size > sp->shm_segsz) {
		u.u_error = EINVAL;
		goto errret;
	}

	/*
	 * There is a hack in here to allow the user to specify that 0
	 * is to be used as the attach address.  If the user specifies
	 * and attach address of 0 AND sets the flag to round the address,
	 * we will try to use 0 as the attach address.  If that fails,
	 * then we will let the system pick the address.
	 */
	if (addr != 0 || (flag & SHM_RND)) {
		/* Use the user-supplied attach address */
		addr_t base;
		uint len;

		if (flag & SHM_RND)
			addr = (addr_t)((uint)addr & ~(SHMLBA - 1));

		/*
		 * Check that the address range
		 *  1) is properly aligned
		 *  2) is correct in unix terms (i.e., not in the u-area)
		 *  3) is within an unmapped address segment
		 */
		base = addr;
		len = size;		/* use aligned size */
		if (((uint)base & PAGEOFFSET) || ((uint)base >= USRSTACK) ||
		    (((uint)base + len) > USRSTACK) ||
		    as_hole(u.u_procp->p_as, len, &base, &len, AH_LO) !=
		    A_SUCCESS) {
			u.u_error = EINVAL;
			goto errret;
		}
	} else {
		/* Let the system pick the attach address */
		map_addr(&addr, sp->shm_amp->size, (off_t)0, 1);
		if (addr == NULL) {
			u.u_error = ENOMEM;
			goto errret;
		}
	}

	/* Initialize the create arguments and map the segment */
	crargs = *(struct segvn_crargs *)zfod_argsp;	/* structure copy */
	crargs.offset = offset;
	crargs.type = MAP_SHARED;
	crargs.amp = sp->shm_amp;
	crargs.prot = (flag & SHM_RDONLY) ? (PROT_ALL & ~PROT_WRITE) : PROT_ALL;
	crargs.maxprot = crargs.prot;

	u.u_error = as_map(u.u_procp->p_as, addr, size, segvn_create,
	    (caddr_t)&crargs);
	if (u.u_error)
		goto errret;

	sp->shm_atime = (time_t)time.tv_sec;
	sp->shm_lpid = u.u_procp->p_pid;
	u.u_rval1 = (int)addr;
errret:
	SHMUNLOCK(sp);

	if (u.u_error == 0)
		return (u.u_rval1);

	return (-1);
}

/*
 * Convert user supplied shmid into a ptr to the associated
 * shared memory header.  The returned shmid is locked with SHMLOCK
 * and must be explicitly unlocked with SHMUNLOCK.
 */
struct shmid_ds *
shmconv(s)
	register int s;			/* shmid */
{
	register struct shmid_ds *sp;	/* ptr to associated header */

	if (s < 0) {
		u.u_error = EINVAL;
		return (NULL);
	}
	sp = &shmem[s % shminfo.shmmni];
	SHMLOCK(sp);
	if (!(sp->shm_perm.mode & IPC_ALLOC) ||
	    ((s / shminfo.shmmni) != sp->shm_perm.seq)) {
		SHMUNLOCK(sp);
		u.u_error = EINVAL;
		return (NULL);
	}
	return (sp);
}

/*
 * Shmctl system call.
 */
shmctl()
{
	register struct a {
		int		shmid;
		int		cmd;
		struct shmid_ds	*arg;
	}	*uap = (struct a *)u.u_ap;

	(void) kshmctl(uap->shmid, uap->cmd, uap->arg);
}

/*
 * Shmctl kernel callable routine.
 */
kshmctl(shmid, cmd, arg)
	int shmid;
	int cmd;
	register struct shmid_ds *arg;
{
	register struct shmid_ds	*sp;	/* shared memory header ptr */
	struct shmid_ds			ds;	/* hold area for IPC_SET */

	if ((sp = shmconv(shmid)) == NULL)
		return (-1);

	u.u_error = 0;		/* init error code */

	switch (cmd) {

	/* Remove shared memory identifier. */
	case IPC_RMID:
		if (u.u_uid != sp->shm_perm.uid &&
		    u.u_uid != sp->shm_perm.cuid && !suser())
			break;

		if (((int)(++(sp->shm_perm.seq)*shminfo.shmmni + (sp - shmem)))
		    < 0)
			sp->shm_perm.seq = 0;

		if (--sp->shm_amp->refcnt == 0) {	/* if no attachments */
			anon_free(sp->shm_amp->anon, sp->shm_amp->size);
			if (sp->shm_amp->swresv)
				anon_unresv(sp->shm_amp->swresv);
			kmem_free((caddr_t)sp->shm_amp->anon,
			    ((sp->shm_amp->size >> PAGESHIFT) *
			    sizeof (struct anon *)));
			anonmap_fast_free(sp->shm_amp);
		}
		sp->shm_amp = NULL;
		SHMUNLOCK(sp);
		sp->shm_perm.mode = 0;
		return (0);

	/* Set ownership and permissions. */
	case IPC_SET:
		if (u.u_uid != sp->shm_perm.uid &&
		    u.u_uid != sp->shm_perm.cuid && !suser())
			break;
		u.u_error = copyin((caddr_t)arg, (caddr_t)&ds,
		    sizeof (ds));
		if (!u.u_error) {
			sp->shm_perm.uid = ds.shm_perm.uid;
			sp->shm_perm.gid = ds.shm_perm.gid;
			sp->shm_perm.mode = (ds.shm_perm.mode & 0777) |
			    (sp->shm_perm.mode & ~0777);
			sp->shm_ctime = (time_t) time.tv_sec;
		}
		break;

	/* Get shared memory data structure. */
	case IPC_STAT:
		if (ipcaccess(&sp->shm_perm, SHM_R))
			break;

		/* get number of attached processes for the user */
		sp->shm_nattch = sp->shm_amp->refcnt - 1;
		u.u_error = copyout((caddr_t)sp, (caddr_t)arg,
		    sizeof (*sp));
		break;

	/* Lock shared segment in memory. */	/* NOTE: no-op for now */
	case SHM_LOCK:
		if (!suser())
			break;
		break;

	/* Unlock shared segment. */		/* NOTE: no-op for now */
	case SHM_UNLOCK:
		if (!suser())
			break;
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
	SHMUNLOCK(sp);

	if (u.u_error == 0)
		return (0);

	return (-1);
}

/*
 * Detach shared memory segment
 */
shmdt()
{
	struct a {
		addr_t	addr;
	}	*uap = (struct a *)u.u_ap;

	(void) kshmdt(uap->addr);
}

/*
 * Detach shared memory segment (kernel callable)
 */
kshmdt(addr)
	addr_t addr;
{
	register struct shmid_ds	*sp;
	register struct anon_map	*amp;
	struct as *as;
	struct seg *seg;

	/*
	 * Find matching shmem address in process address space.
	 *
	 * XXX - This could return a non-SysV shared segment.
	 */
	as = u.u_procp->p_as;
	amp = shm_vmlookup(as, addr);
	if (amp == NULL) {
		u.u_error = EINVAL;
		return (-1);
	}

	seg = as_segat(as, addr);
	if (as_unmap(as, addr, seg->s_size) != A_SUCCESS) {
		u.u_error = EINVAL;
		return (-1);
	}

	/*
	 * Find shmem anon_map ptr in system-wide table.
	 * If not found, IPC_RMID has already been done.
	 * If found, SHMLOCK prevents races in IPC_STAT and IPC_RMID.
	 */
	for (sp = shmem; sp < &shmem[shminfo.shmmni]; sp++)
		if (sp->shm_amp == amp) {
			SHMLOCK(sp);
			if (sp->shm_amp == amp) {	/* still there? */
				sp->shm_dtime = (time_t) time.tv_sec;
				sp->shm_lpid = u.u_procp->p_pid;
			}
			SHMUNLOCK(sp);
			break;
		}

	if (u.u_error == 0)
		return (0);

	return (-1);
}

/*
 * Shmget (create new shmem) system call.
 */
shmget()
{
	register struct a {
		key_t	key;
		uint	size;
		int	shmflg;
	}	*uap = (struct a *)u.u_ap;

	(void) kshmget(uap->key, uap->size, uap->shmflg);
}

/*
 * kshmget (create new shmem) kernel callable routine.
 */
kshmget(key, size, shmflg)
	key_t key;
	register uint size;
	int shmflg;
{
	register struct shmid_ds	*sp;	/* shared memory header ptr */
	int				s;	/* ipcget status */
	register uint			npages;	/* how many pages */

	u.u_error = 0;				/* init error code */

	if ((sp = (struct shmid_ds *) ipcget(key, shmflg,
	    &shmem[0].shm_perm, shminfo.shmmni, sizeof (*sp), &s)) == NULL)
		return (-1);

	if (s) {
		/*
		 * This is a new shared memory segment.
		 * Allocate an anon_map structure and anon array and
		 * finish initialization.
		 */
		if (size < shminfo.shmmin || size > shminfo.shmmax) {
			u.u_error = EINVAL;
			sp->shm_perm.mode = 0;
			return (-1);
		}

		if (anon_resv(size) == 0) {
			/*
			 * Cannot reserve swap space
			 */
			u.u_error = ENOMEM;
			sp->shm_perm.mode = 0;
			return (-1);
		}

		/*
		 * Get number of pages required by this segment (round up).
		 */
		npages = btopr(size);

		/*
		 * Lock shmid in case kmem_zalloc() sleeps and a
		 * bogus request comes in with this shmid.
		 */
		SHMLOCK(sp);
		sp->shm_amp = anonmap_fast_alloc();
		sp->shm_amp->anon = (struct anon **) new_kmem_zalloc(
				npages * sizeof (struct anon *), KMEM_SLEEP);
		sp->shm_amp->swresv = sp->shm_amp->size = ptob(npages);
		sp->shm_amp->refcnt = 1;	/* keep until IPC_RMID */

		/*
		 * Store the original user's requested size, in bytes,
		 * rather than the page-aligned size.  The former is
		 * used for IPC_STAT and shmget() lookups.  The latter
		 * is saved in the anon_map structure and is used for
		 * calls to the vm layer.
		 */
		sp->shm_segsz = size;
		sp->shm_atime = (time_t) 0;
		sp->shm_dtime = (time_t) 0;
		sp->shm_ctime = (time_t) time.tv_sec;
		sp->shm_lpid = 0;
		sp->shm_cpid = u.u_procp->p_pid;
		SHMUNLOCK(sp);
	} else {
		/*
		 * Found an existing segment.  Check size
		 */
		if (size && size > sp->shm_segsz) {
			u.u_error = EINVAL;
			return (-1);
		}
	}
	u.u_rval1 = sp->shm_perm.seq * shminfo.shmmni + (sp - shmem);

	if (u.u_error == 0)
		return (u.u_rval1);

	return (-1);
}

/*
 * System entry point for shmat, shmctl, shmdt, and shmget system calls.
 */

int	(*shmcalls[])() = {shmat, shmctl, shmdt, shmget};

shmsys()
{
	register struct a {
		uint	id;
	}	*uap = (struct a *)u.u_ap;

	if (uap->id > 3) {
		u.u_error = EINVAL;
		return;
	}
	u.u_ap = &u.u_arg[1];
	(*shmcalls[uap->id])();
}

/*
 * Shm_vmlookup - Try to determine whether a given user address is mapped
 * to a shared memory segment.  Return the anon_map structure ptr or NULL.
 *
 * XXX	This routine is a (hopefully) temporary kludge pending some
 *	vm-system semantics for doing this.  It could fail to disqualify
 *	an invalid shared segment if it looks an awful lot like the
 *	segment set up by shmat().
 */
struct anon_map *
shm_vmlookup(as, addr)
	struct as *as;
	addr_t addr;
{
	register struct seg *seg;
	register struct segvn_data *segvn;

	/* Find an address segment that contains this address */
	if ((seg = as_segat(as, addr)) == NULL)
		return (NULL);

	/* Make sure it is a segvn segment */
	if (seg->s_ops != &segvn_ops)
		return (NULL);

	/* Get the private data and make sure the map index is zero */
	segvn = (struct segvn_data *)seg->s_data;
	if (segvn->amp == NULL)
		return (NULL);

	/* Check that the entire anon region is mapped by the segment */
	if ((segvn->amp->size != seg->s_size) || (seg->s_base != addr))
		return (NULL);

	/* Check other flags, etc., to try to verify validity */
	    if ((segvn->vp != NULL) || (segvn->type != MAP_SHARED))
		return (NULL);

	/* Check same maxprot that shmat could set for shared segment */
	if ((segvn->maxprot == PROT_ALL) || (segvn->maxprot == (PROT_ALL & ~PROT_WRITE)))
		return (segvn->amp);

	return (NULL);
}
