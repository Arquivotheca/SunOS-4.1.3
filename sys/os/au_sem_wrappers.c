#ifndef lint
static char sccsid[] = "@(#)au_sem_wrappers.c 1.1 92/07/30 SMI";	/* */
#endif
/*
 *	Inter-Process Communication Message Facility.
 *	Wrapper functions for auditing.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/vm.h>
#include <sys/mman.h>

#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>
#include <vm/seg_vn.h>
#include <vm/anon.h>

/*
 *	Inter-Process Communication Semaphore Facility.
 */

struct semid_ds	*semconv();

/*
 *	semctl - Semctl system call.
 */
au_semctl()
{
	register struct a {
		int	semid;
		uint	semnum;
		int	cmd;
		int	arg;
	}	*uap = (struct a *)u.u_ap;
	unsigned vsize;
	register struct	semid_ds *sp;	/* ptr to semaphore header */

	if ((sp = semconv(uap->semid)) == NULL)
		return;

	semctl();

	switch (uap->cmd) {
	case IPC_RMID:
		au_sysaudit(AUR_SEMCTL3, AU_DCREATE,
		    u.u_error, u.u_rval1, 3,
		    sizeof (uap->semid), (caddr_t)&(uap->semid),
		    sizeof (uap->semnum), (caddr_t)&(uap->semnum),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd));
		break;
	case IPC_SET:
		au_sysaudit(AUR_SEMCTL, AU_DWRITE,
		    u.u_error, u.u_rval1, 4,
		    sizeof (uap->semid), (caddr_t)&(uap->semid),
		    sizeof (uap->semnum), (caddr_t)&(uap->semnum),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER | sizeof (struct semid_ds),
		    (caddr_t)&(uap->arg));
		break;
	case IPC_STAT:
		au_sysaudit(AUR_SEMCTL, AU_DREAD,
		    u.u_error, u.u_rval1, 4,
		    sizeof (uap->semid), (caddr_t)&(uap->semid),
		    sizeof (uap->semnum), (caddr_t)&(uap->semnum),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER | sizeof (struct semid_ds),
		    (caddr_t)&(uap->arg));
		break;
	case GETNCNT:
	case GETPID:
	case GETVAL:
	case GETZCNT:
		au_sysaudit(AUR_SEMCTL3, AU_DREAD,
		    u.u_error, u.u_rval1, 3,
		    sizeof (uap->semid), (caddr_t)&(uap->semid),
		    sizeof (uap->semnum), (caddr_t)&(uap->semnum),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd));
		break;
	case GETALL:
		vsize = sp->sem_nsems * sizeof (short);
		au_sysaudit(AUR_SEMCTLALL, AU_DREAD,
		    u.u_error, u.u_rval1, 4,
		    sizeof (uap->semid), (caddr_t)&(uap->semid),
		    sizeof (uap->semnum), (caddr_t)&(uap->semnum),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER | vsize, (caddr_t)&(uap->arg));
		break;
	case SETALL:
		vsize = sp->sem_nsems * sizeof (short);
		au_sysaudit(AUR_SEMCTLALL, AU_DWRITE,
		    u.u_error, u.u_rval1, 4,
		    sizeof (uap->semid), (caddr_t)&(uap->semid),
		    sizeof (uap->semnum), (caddr_t)&(uap->semnum),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER | vsize, (caddr_t)&(uap->arg));
		break;
	case SETVAL:
		au_sysaudit(AUR_SEMCTL, AU_DWRITE,
		    u.u_error, u.u_rval1, 4,
		    sizeof (uap->semid), (caddr_t)&(uap->semid),
		    sizeof (uap->semnum), (caddr_t)&(uap->semnum),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER | sizeof (int), (caddr_t)&(uap->arg));
		break;
	default:
		break;
	}
}

/*
 *	semget - Semget system call.
 */
au_semget()
{
	register struct a {
		key_t	key;
		int	nsems;
		int	semflg;
	} *uap = (struct a *)u.u_ap;
	int au;

	au = AU_DREAD |
	    au_ipcexists(uap->key, (struct ipc_perm *)sema,
		seminfo.semmni, sizeof (struct semid_ds)) |
	    au_ipcwrite(uap->semflg);

	semget();

	au_sysaudit(AUR_SEMGET, au, u.u_error, u.u_rval1, 3,
	    sizeof (uap->key), (caddr_t)&(uap->key),
	    sizeof (uap->nsems), (caddr_t)&(uap->nsems),
	    sizeof (uap->semflg), (caddr_t)&(uap->semflg));
}

/*
 *	semop - Semop system call.
 */
au_semop()
{
	register struct a {
		int		semid;
		struct sembuf	*sops;
		uint		nsops;
	}	*uap = (struct a *)u.u_ap;
	struct sembuf *uops;	/* ptr to copy of user ops */
	struct sembuf *sbp;	/* ptr to an op */
	u_int opsize;
	int au = AU_DREAD;
	int i;

	semop();

	/*
	 * Allocate space to hold all semaphore operations.
	 * If it doesn't fit, report the fact.
	 */
	if (uap->nsops > seminfo.semopm) {
		i = 1;
		au_sysaudit(AUR_SEMOP, au, u.u_error, u.u_rval1, 3,
		    sizeof (uap->semid), (caddr_t)&(uap->semid),
		    sizeof (uap->nsops), (caddr_t)&i,
		    AUP_USER | (sizeof (*uops)), (caddr_t)&(uap->sops));
		return;
	}
	opsize = uap->nsops * sizeof (*uops);
	uops = (struct sembuf *) new_kmem_alloc(opsize, KMEM_SLEEP);
	if (copyin((caddr_t)uap->sops, (caddr_t)uops, opsize) == 0) {
		for (i = 0, sbp = uops; i < uap->nsops; i++, sbp++) {
			if (sbp->sem_op != 0) {
				au = AU_DWRITE;
				break;
			}
		}
	}
	kmem_free((caddr_t)uops, opsize);

	au_sysaudit(AUR_SEMOP, au, u.u_error, u.u_rval1, 3,
	    sizeof (uap->semid), (caddr_t)&(uap->semid),
	    sizeof (uap->nsops), (caddr_t)&(uap->nsops),
	    AUP_USER | (sizeof (*uops) * uap->nsops), (caddr_t)&(uap->sops));
}

/*
 *	semsys - System entry point for semctl, semget, and semop system calls.
 *
 *		Assumes SEMCTL/SEMGET/SEMOP definitions in
 *				...src/lib/libc/sys/common/semsys.c
 */
au_semsys()
{
	int au_semctl();
	int au_semget();
	int au_semop();
	static int (*calls[])() = {au_semctl, au_semget, au_semop};
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
