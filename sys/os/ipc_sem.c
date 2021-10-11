/*	@(#)ipc_sem.c 1.1 92/07/30 SMI; from S5R3.1 10.10	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Inter-Process Communication Semaphore Facility.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/systm.h>


struct sem_undo	*semunp;		/* ptr to head of undo chain */
struct sem_undo	*semfup;		/* ptr to head of free undo chain */

struct semid_ds	*semconv();
struct ipc_perm *ipcget();


/*
 *	semaoe - Create or update adjust on exit entry.
 */
semaoe(val, id, num)
	short	val;	/* operation value to be adjusted on exit */
	int	id;	/* semid */
	short	num;	/* semaphore # */
{
	register struct undo		*uup,	/* ptr to entry to update */
					*uup2;	/* ptr to move entry */
	register struct sem_undo	*up,	/* ptr to process undo struct */
					*up2;	/* ptr to undo list */
	register int			i,	/* loop control */
					found;	/* matching entry found flag */

	if (val == 0)
		return (0);
	if (val > seminfo.semaem || val < -seminfo.semaem) {
		u.u_error = ERANGE;
		return (1);
	}
	/*
	 * If this process doesn't yet have any undo structures,
	 * get a free one and initialize it
	 */
	if ((up = sem_undo[u.u_procp - proc]) == NULL)
		if (up = semfup) {
			semfup = up->un_np;
			up->un_np = NULL;
			sem_undo[u.u_procp - proc] = up;
		} else {
			u.u_error = ENOSPC;
			return (1);
		}
	/*
	 * Check for an existing undo structure for this semaphore
	 */
	for (uup = up->un_ent, found = i = 0; i < up->un_cnt; i++) {
		if (uup->un_id < id ||
		    (uup->un_id == id && uup->un_num < num)) {
			uup++;
			continue;
		}
		if (uup->un_id == id && uup->un_num == num)
			found = 1;
		break;
	}
	/*
	 * If this is a new undo structure, link it into the active undo list
	 * Then look for the undo entry insertion point, keeping the entries
	 * sorted by id; if necessary, shuffle remaining entries up
	 */
	if (!found) {
		if (up->un_cnt >= seminfo.semume) {
			u.u_error = EINVAL;
			return (1);
		}
		if (up->un_cnt == 0) {
			up->un_np = semunp;
			semunp = up;
		}
		uup2 = &up->un_ent[up->un_cnt++];
		while (uup2-- > uup)
			*(uup2 + 1) = *uup2;

		/* Insert a new undo entry, then done */
		uup->un_id = id;
		uup->un_num = num;
		uup->un_aoe = -val;
		return (0);
	}
	/*
	 * If an entry already existed for this semaphore, adjust it
	 */
	uup->un_aoe -= val;
	if (uup->un_aoe > seminfo.semaem || uup->un_aoe < -seminfo.semaem) {
		u.u_error = ERANGE;
		uup->un_aoe += val;
		return (1);
	}
	/*
	 * If new adjust-on-exit value is zero, remove this undo entry
	 * from the list.  If new undo entry count is zero, remove this
	 * undo structure from the active undo list; however, retain this
	 * structure for the current process until semexit()
	 */
	if (uup->un_aoe == 0) {
		uup2 = &up->un_ent[--(up->un_cnt)];
		while (uup++ < uup2)
			*(uup - 1) = *uup;
		if (up->un_cnt == 0) {

			/* Remove process from undo list. */
			if (semunp == up)
				semunp = up->un_np;
			else
				for (up2 = semunp; up2 != NULL;
				    up2 = up2->un_np)
					if (up2->un_np == up) {
						up2->un_np = up->un_np;
						break;
					}
			up->un_np = NULL;
		}
	}
	return (0);
}

/*
 *	semconv - Convert user supplied semid into a ptr to the associated
 *		semaphore header.
 */
struct semid_ds *
semconv(s)
	register int	s;	/* semid */
{
	register struct semid_ds	*sp;	/* ptr to associated header */

	if (s < 0) {
		u.u_error = EINVAL;
		return (NULL);
	}
	sp = &sema[s % seminfo.semmni];
	if ((sp->sem_perm.mode & IPC_ALLOC) == 0 ||
	    s / seminfo.semmni != sp->sem_perm.seq) {
		u.u_error = EINVAL;
		return (NULL);
	}
	return (sp);
}

/*
 *	semctl - Semctl system call.
 */
semctl()
{
	register struct a {
		int	semid;
		uint	semnum;
		int	cmd;
		int	arg;
	}	*uap = (struct a *)u.u_ap;
	register struct	semid_ds	*sp;	/* ptr to semaphore header */
	register struct sem		*p;	/* ptr to semaphore */
	register int			i;	/* loop control */

	if ((sp = semconv(uap->semid)) == NULL)
		return;
	u.u_rval1 = 0;
	switch (uap->cmd) {

	/* Remove semaphore set. */
	case IPC_RMID:
		if (u.u_uid != sp->sem_perm.uid &&
		    u.u_uid != sp->sem_perm.cuid &&
		    !suser())
			break;
		semunrm(uap->semid, 0, sp->sem_nsems);
		for (i = sp->sem_nsems, p = sp->sem_base; i--; p++) {
			p->semval = p->sempid = 0;
			if (p->semncnt) {
				wakeup((caddr_t)&p->semncnt);
				p->semncnt = 0;
			}
			if (p->semzcnt) {
				wakeup((caddr_t)&p->semzcnt);
				p->semzcnt = 0;
			}
		}
		rmfree(semmap, (long)sp->sem_nsems,
		    (u_long)((sp->sem_base - sem) + 1));
		if (uap->semid + seminfo.semmni < 0)
			sp->sem_perm.seq = 0;
		else
			sp->sem_perm.seq++;
		sp->sem_perm.mode = 0;
		return;

	/* Set ownership and permissions. */
	case IPC_SET: {
		struct semid_ds id;

		if (u.u_uid != sp->sem_perm.uid &&
		    u.u_uid != sp->sem_perm.cuid &&
		    !suser())
			break;
		if (u.u_error =
		    copyin((caddr_t)uap->arg, (caddr_t)&id, sizeof (id))) {
			break;
		}
		sp->sem_perm.uid = id.sem_perm.uid;
		sp->sem_perm.gid = id.sem_perm.gid;
		sp->sem_perm.mode = id.sem_perm.mode & 0777 | IPC_ALLOC;
		sp->sem_ctime = (time_t)time.tv_sec;
		break;
	    }

	/* Get semaphore data structure. */
	case IPC_STAT:
		if (ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if (u.u_error = copyout((caddr_t)sp, (caddr_t)uap->arg,
		    sizeof (*sp)))
			break;
		break;

	/* Get # of processes sleeping for greater semval. */
	case GETNCNT:
		if (ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if (uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		u.u_rval1 = (sp->sem_base + uap->semnum)->semncnt;
		break;

	/* Get pid of last process to operate on semaphore. */
	case GETPID:
		if (ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if (uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		u.u_rval1 = (sp->sem_base + uap->semnum)->sempid;
		break;

	/* Get semval of one semaphore. */
	case GETVAL:
		if (ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if (uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		u.u_rval1 = (sp->sem_base + uap->semnum)->semval;
		break;

	/* Get all semvals in set. */
	case GETALL: {
		register unsigned short *vals;
		unsigned vsize;

		if (ipcaccess(&sp->sem_perm, SEM_R))
			break;

		/* allocate space to hold all semaphore values */
		vsize = sp->sem_nsems * sizeof (short);
		vals = (unsigned short *) new_kmem_alloc(vsize, KMEM_SLEEP);

		for (i = 0, p = sp->sem_base; i < sp->sem_nsems; p++)
			vals[i++] = p->semval;
		u.u_error = copyout((caddr_t)vals, (caddr_t)uap->arg, vsize);

		kmem_free((caddr_t)vals, vsize);
		break;
	    }

	/* Get # of processes sleeping for semval to become zero. */
	case GETZCNT:
		if (ipcaccess(&sp->sem_perm, SEM_R))
			break;
		if (uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		u.u_rval1 = (sp->sem_base + uap->semnum)->semzcnt;
		break;

	/* Set semval of one semaphore. */
	case SETVAL:
		if (ipcaccess(&sp->sem_perm, SEM_A))
			break;
		if (uap->semnum >= sp->sem_nsems) {
			u.u_error = EINVAL;
			break;
		}
		if ((unsigned)uap->arg > seminfo.semvmx) {
			u.u_error = ERANGE;
			break;
		}
		if ((p = sp->sem_base + uap->semnum)->semval = uap->arg) {
			if (p->semncnt) {
				p->semncnt = 0;
				wakeup((caddr_t)&p->semncnt);
			}
		} else
			if (p->semzcnt) {
				p->semzcnt = 0;
				wakeup((caddr_t)&p->semzcnt);
			}
		p->sempid = u.u_procp->p_pid;
		semunrm(uap->semid, uap->semnum, uap->semnum);
		break;

	/* Set semvals of all semaphores in set. */
	case SETALL: {
		register unsigned short *vals;
		unsigned vsize;

		if (ipcaccess(&sp->sem_perm, SEM_A))
			break;

		/* allocate space to hold all semaphore values */
		vsize = sp->sem_nsems * sizeof (short);
		vals = (unsigned short *) new_kmem_alloc(vsize, KMEM_SLEEP);

		if (u.u_error = copyin((caddr_t)uap->arg, (caddr_t)vals, vsize))
			goto seterr;

		for (i = 0; i < sp->sem_nsems; )
			if (vals[i++] > seminfo.semvmx) {
				u.u_error = ERANGE;
				goto seterr;
			}
		semunrm(uap->semid, 0, sp->sem_nsems);
		for (i = 0, p = sp->sem_base; i < sp->sem_nsems;
		    (p++)->sempid = u.u_procp->p_pid) {
			if (p->semval = vals[i++]) {
				if (p->semncnt) {
					p->semncnt = 0;
					wakeup((caddr_t)&p->semncnt);
				}
			} else
				if (p->semzcnt) {
					p->semzcnt = 0;
					wakeup((caddr_t)&p->semzcnt);
				}
		}
seterr:
		kmem_free((caddr_t)vals, vsize);
		break;
	    }
	default:
		u.u_error = EINVAL;
		break;
	}
}

/*
 *	semexit - Called by exit(kern_exit.c) to clean up on process exit.
 */
semexit()
{
	register struct sem_undo	*up,	/* process undo struct ptr */
					*p;	/* undo struct ptr */
	register struct semid_ds	*sp;	/* semid being undone ptr */
	register int			i;	/* loop control */
	register long			v;	/* adjusted value */
	register struct sem		*semp;	/* semaphore ptr */

	if ((up = sem_undo[u.u_procp - proc]) == NULL)
		return;
	if (up->un_cnt == 0)
		goto cleanup;
	for (i = up->un_cnt; i--; ) {
		if ((sp = semconv(up->un_ent[i].un_id)) == NULL)
			continue;
		v = (long)(semp = sp->sem_base + up->un_ent[i].un_num)->semval +
		    up->un_ent[i].un_aoe;
		if (v < 0 || v > seminfo.semvmx)
			continue;
		semp->semval = v;
		if (v == 0 && semp->semzcnt) {
			semp->semzcnt = 0;
			wakeup((caddr_t)&semp->semzcnt);
		}
		if (up->un_ent[i].un_aoe > 0 && semp->semncnt) {
			semp->semncnt = 0;
			wakeup((caddr_t)&semp->semncnt);
		}
	}
	up->un_cnt = 0;
	if (semunp == up)
		semunp = up->un_np;
	else
		for (p = semunp; p != NULL; p = p->un_np)
			if (p->un_np == up) {
				p->un_np = up->un_np;
				break;
			}

cleanup:
	sem_undo[u.u_procp - proc] = NULL;
	up->un_np = semfup;
	semfup = up;
}

/*
 *	semget - Semget system call.
 */
semget()
{
	register struct a {
		key_t	key;
		int	nsems;
		int	semflg;
	}	*uap = (struct a *)u.u_ap;
	register struct semid_ds	*sp;	/* semaphore header ptr */
	register unsigned int		i;	/* temp */
	int				s;	/* ipcget status return */

	if ((sp = (struct semid_ds *) ipcget(uap->key, uap->semflg,
	    (struct ipc_perm *)sema, seminfo.semmni, sizeof (*sp), &s)) == NULL)
		return;

	if (s) {
		/* This is a new semaphore set.  Finish initialization. */
		if (uap->nsems <= 0 || uap->nsems > seminfo.semmsl) {
			u.u_error = EINVAL;
			sp->sem_perm.mode = 0;
			return;
		}
		if ((i = rmalloc(semmap, (long)uap->nsems)) == NULL) {
			u.u_error = ENOSPC;
			sp->sem_perm.mode = 0;
			return;
		}
		/* point to first semaphore */
		sp->sem_base = sem + (i - 1);
		sp->sem_nsems = uap->nsems;
		sp->sem_ctime = (time_t)time.tv_sec;
		sp->sem_otime = (time_t)0;
	} else
		if (uap->nsems && sp->sem_nsems < uap->nsems) {
			u.u_error = EINVAL;
			return;
		}
	/* sem_perm.seq incremented by semctl() when semaphore group released */
	u.u_rval1 = sp->sem_perm.seq * seminfo.semmni + (sp - sema);

}

/*
 *	seminit - Called by main(init_main.c) to initialize the semaphore map.
 */
seminit()
{
	register i;

	/* initialize the semaphore map with semmns entries */
	rminit(semmap, (long)seminfo.semmns, (long)1, "semmap", seminfo.semmap);

	/* link all free undo structures together */
	semfup = (struct sem_undo *) semu;
	for (i = 0; i < seminfo.semmnu - 1; i++) {
		semfup->un_np = (struct sem_undo*)((uint)semfup+seminfo.semusz);
		semfup = semfup->un_np;
	}
	semfup->un_np = NULL;
	semfup = (struct sem_undo *)semu;
}

/*
 *	semop - Semop system call.
 */
semop()
{
	register struct a {
		int		semid;
		struct sembuf	*sops;
		uint		nsops;
	}	*uap = (struct a *)u.u_ap;
	register struct sembuf		*op;	/* ptr to operation */
	register int			i;	/* loop control */
	register struct semid_ds	*sp;	/* ptr to associated header */
	register struct sem		*semp;	/* ptr to semaphore */
	int	again;
	struct sembuf			*uops;	/* ptr to copy of user ops */

	if ((sp = semconv(uap->semid)) == NULL)
		return;
	if (uap->nsops > seminfo.semopm) {
		u.u_error = E2BIG;
		return;
	}

	/* allocate space to hold all semaphore operations */
	uops = (struct sembuf *) new_kmem_alloc(
			uap->nsops * sizeof (*uops), KMEM_SLEEP);

	if (u.u_error = copyin((caddr_t)uap->sops, (caddr_t)uops,
	    sizeof (*uops) * uap->nsops))
		goto semoperr;

	/* Verify that sem #s are in range and permissions are granted. */
	for (i = 0, op = uops; i++ < uap->nsops; op++) {
		if (ipcaccess(&sp->sem_perm,
		    (unsigned)(op->sem_op ? SEM_A : SEM_R)))
			goto semoperr;
		if ((unsigned)op->sem_num >= sp->sem_nsems) {
			u.u_error = EFBIG;
			goto semoperr;
		}
	}
	again = 0;
check:
	/*
	 * Loop waiting for the operations to be satisified atomically.
	 * Actually, do the operations and undo them if a wait is needed
	 * or an error is detected.
	 */
	if (again) {
		/* Verify that the semaphores haven't been removed. */
		if (semconv(uap->semid) == NULL) {
			u.u_error = EIDRM;
			goto semoperr;
		}
	}
	again = 1;

	for (i = 0, op = uops; i < uap->nsops; i++, op++) {
		semp = sp->sem_base + op->sem_num;

		if (op->sem_op > 0) {
			if (op->sem_op + (long)semp->semval > seminfo.semvmx ||
			    (op->sem_flg & SEM_UNDO &&
			    semaoe(op->sem_op, uap->semid, op->sem_num))) {
				if (u.u_error == 0)
					u.u_error = ERANGE;
				if (i)
					semundo(uops, i, uap->semid, sp);
				goto semoperr;
			}
			semp->semval += op->sem_op;
			if (semp->semncnt) {
				semp->semncnt = 0;
				wakeup((caddr_t)&semp->semncnt);
			}
			if (semp->semval == 0 && semp->semzcnt) {
				semp->semzcnt = 0;
				wakeup((caddr_t)&semp->semzcnt);
			}
			continue;
		}
		if (op->sem_op < 0) {
			if (semp->semval >= -op->sem_op) {
				if (op->sem_flg & SEM_UNDO && semaoe(op->sem_op,
				    uap->semid, op->sem_num)) {
					if (i)
						semundo(uops, i,
						    uap->semid, sp);
					goto semoperr;
				}
				semp->semval += op->sem_op;
				if (semp->semval == 0 && semp->semzcnt) {
					semp->semzcnt = 0;
					wakeup((caddr_t)&semp->semzcnt);
				}
				continue;
			}
			if (i)
				semundo(uops, i, uap->semid, sp);
			if (op->sem_flg & IPC_NOWAIT) {
				u.u_error = EAGAIN;
				goto semoperr;
			}
			semp->semncnt++;
			if (sleep((caddr_t)&semp->semncnt, PCATCH | PSEMN)) {
				if ((semp->semncnt)-- <= 1) {
					semp->semncnt = 0;
					wakeup((caddr_t)&semp->semncnt);
				}
				u.u_error = EINTR;
				goto semoperr;
			}
			goto check;
		}
		if (semp->semval) {
			if (i)
				semundo(uops, i, uap->semid, sp);
			if (op->sem_flg & IPC_NOWAIT) {
				u.u_error = EAGAIN;
				goto semoperr;
			}
			semp->semzcnt++;
			if (sleep((caddr_t)&semp->semzcnt, PCATCH | PSEMZ)) {
				if ((semp->semzcnt)-- <= 1) {
					semp->semzcnt = 0;
					wakeup((caddr_t)&semp->semzcnt);
				}
				u.u_error = EINTR;
				goto semoperr;
			}
			goto check;
		}
	}

	/* All operations succeeded.  Update sempid for accessed semaphores. */
	for (i = 0, op = uops; i++ < uap->nsops;
		(sp->sem_base + (op++)->sem_num)->sempid = u.u_procp->p_pid);
	sp->sem_otime = (time_t) time.tv_sec;

semoperr:
	/* Before leaving, deallocate the buffer that held the user semops */
	kmem_free((caddr_t)uops, (sizeof (*uops) * uap->nsops));

}

/*
 *	semsys - System entry point for semctl, semget, and semop system calls.
 *
 *		Assumes SEMCTL/SEMGET/SEMOP definitions in
 *				...src/lib/libc/sys/common/semsys.c
 */
semsys()
{
	int	semctl(),
		semget(),
		semop();
	static int	(*calls[])() = {semctl, semget, semop};
	register struct a {
		uint	id;	/* function code id */
	}	*uap = (struct a *)u.u_ap;

	if (uap->id > 2) {
		u.u_error = EINVAL;
		return;
	}
	u.u_ap = &u.u_arg[1];
	(*calls[uap->id])();
}

/*
 *	semundo - Undo work done up to finding an operation that can't be done.
 */
semundo(op, n, id, sp)
	register struct sembuf	*op;	/* first operation that was done ptr */
	register int		n,	/* # of operations that were done */
				id;	/* semaphore id */
	register struct semid_ds *sp;	/* semaphore data structure ptr */
{
	register struct sem	*semp;	/* semaphore ptr */

	for (op += n - 1; n--; op--) {
		if (op->sem_op == 0)
			continue;
		semp = sp->sem_base + op->sem_num;
		semp->semval -= op->sem_op;
		if (op->sem_flg & SEM_UNDO)
			(void) semaoe(-op->sem_op, id, op->sem_num);
	}
}

/*
 *	semunrm - Undo entry remover.
 *
 * This routine is called to clear all undo entries for a set of semaphores
 * that are being removed from the system or are being reset by SETVAL or
 * SETVALS commands to semctl.
 */
semunrm(id, low, high)
	int	id;	/* semid */
	ushort	low,	/* lowest semaphore being changed */
		high;	/* highest semaphore being changed */
{
	register struct sem_undo	*pp,	/* ptr to predecessor to p */
					*p;	/* ptr to current entry */
	register struct undo		*up;	/* ptr to undo entry */
	register int			i,	/* loop control */
					j;	/* loop control */

	pp = NULL;
	p = semunp;
	while (p != NULL) {

		/* Search through current structure for matching entries. */
		for (up = p->un_ent, i = 0; i < p->un_cnt; ) {
			if (id < up->un_id)
				break;
			if (id > up->un_id || low > up->un_num) {
				up++;
				i++;
				continue;
			}
			if (high < up->un_num)
				break;
			/* squeeze out one entry */
			for (j = i; ++j < p->un_cnt;
			    p->un_ent[j - 1] = p->un_ent[j])
				;
			p->un_cnt--;
		}

		/* Reset pointers for next round. */
		if (p->un_cnt == 0) {

			/* Remove from linked list. */
			if (pp == NULL) {
				semunp = p->un_np;
				p->un_np = NULL;
				p = semunp;
			} else {
				pp->un_np = p->un_np;
				p->un_np = NULL;
				p = pp->un_np;
			}
		} else {
			pp = p;
			p = p->un_np;
		}
	}
}
