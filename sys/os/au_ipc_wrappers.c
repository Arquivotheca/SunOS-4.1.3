#ifndef lint
static char sccsid[] = "@(#)au_ipc_wrappers.c 1.1 92/07/30 SMI";	/* */
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

/* HACK :: msgbuf structure renamed to avoid kernel compilation conflicts */
#define	msgbuf ipcmsgbuf

/*
 *	msgctl - Msgctl system call.
 */
au_msgctl()
{
	register struct a {
		int msqid;
		int cmd;
		struct msqid_ds *buf;
	} *uap = (struct a *)u.u_ap;
	int au;

	msgctl();

	switch (uap->cmd) {
	case IPC_RMID:
		au_sysaudit(AUR_MSGCTLRMID, AU_DCREATE,
		    u.u_error, u.u_rval1, 2,
		    sizeof (uap->msqid), (caddr_t)&(uap->msqid),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd));
		return;
	case IPC_SET:
		au = AU_DWRITE;
		break;
	case IPC_STAT:
		au = AU_DREAD;
		break;
	default:
		u.u_error = EINVAL;
		return;
	}
	au_sysaudit(AUR_MSGCTL, au, u.u_error, u.u_rval1, 3,
	    sizeof (uap->msqid), (caddr_t)&(uap->msqid),
	    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
	    AUP_USER | sizeof (struct msqid_ds), (caddr_t)&(uap->buf));
}

/*
 *	msgget - Msgget system call.
 */
au_msgget()
{
	register struct a {
		key_t	key;
		int	msgflg;
	} *uap = (struct a *)u.u_ap;
	int au;

	au = AU_DREAD |
	    au_ipcexists(uap->key, (struct ipc_perm *)msgque,
		msginfo.msgmni, sizeof (struct msqid_ds)) |
	    au_ipcwrite(uap->msgflg);

	msgget();

	au_sysaudit(AUR_MSGGET, au, u.u_error, u.u_rval1, 2,
	    sizeof (uap->key), (caddr_t)&(uap->key),
	    sizeof (uap->msgflg), (caddr_t)&(uap->msgflg));
}

/*
 *	msgrcv - Msgrcv system call.
 */
au_msgrcv()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		long		msgtyp;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;

	msgrcv();

	au_sysaudit(AUR_MSGRCV, AU_DREAD, u.u_error, u.u_rval1, 3,
	    sizeof (uap->msqid), (caddr_t)&(uap->msqid),
	    sizeof (uap->msgflg), (caddr_t)&(uap->msgflg),
	    sizeof (long), (caddr_t)&(uap->msgtyp));
}

/*
 *	msgsnd - Msgsnd system call.
 */
au_msgsnd()
{
	register struct a {
		int		msqid;
		struct msgbuf	*msgp;
		int		msgsz;
		int		msgflg;
	}	*uap = (struct a *)u.u_ap;

	msgsnd();

	au_sysaudit(AUR_MSGSND, AU_DWRITE, u.u_error, u.u_rval1, 3,
	    sizeof (uap->msqid), (caddr_t)&(uap->msqid),
	    sizeof (uap->msgflg), (caddr_t)&(uap->msgflg),
	    AUP_USER | sizeof (long), (caddr_t)&(uap->msgp));
}

/*
 *	msgsys - System entry point for msgctl, msgget, msgrcv, and msgsnd
 *		system calls.
 *		Assumes MSGGET / MSGCTL / MSGSND / MSGRCV definitions in
 *				...src/lib/libc/sys/common/msgsys.c
 */
au_msgsys()
{
	int au_msgctl();
	int au_msgget();
	int au_msgrcv();
	int au_msgsnd();
	static int (*calls[])() = {au_msgget, au_msgctl, au_msgrcv, au_msgsnd};
	register struct a {
		unsigned	id;	/* function code id */
		int		*ap;	/* arg pointer for recvmsg */
	} *uap = (struct a *)u.u_ap;

	if (uap->id > 3) {
		u.u_error = EINVAL;
		return;
	}
	u.u_ap = &u.u_arg[1];
	(*calls[uap->id])();
}
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
	}	*uap = (struct a *)u.u_ap;
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
	 * allocate space to hold all semaphore operations
	 */
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
/*
 *	Inter-Process Communication Shared Memory Facility.
 */

/*
 * Shmctl system call.
 */
au_shmctl()
{
	register struct a {
		int		shmid;
		int		cmd;
		struct shmid_ds	*arg;
	}	*uap = (struct a *)u.u_ap;
	int au;

	shmctl();

	switch (uap->cmd) {
	case IPC_RMID:
		au_sysaudit(AUR_SHMCTLRMID, AU_DCREATE,
		    u.u_error, u.u_rval1, 2,
		    sizeof (uap->shmid), (caddr_t)&(uap->shmid),
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd));
		return;
	case IPC_SET:
		au = AU_DWRITE;
		break;
	case IPC_STAT:
		au = AU_DREAD;
		break;
	default:
		return;
	}
	au_sysaudit(AUR_SHMCTL, au, u.u_error, u.u_rval1, 3,
	    sizeof (uap->shmid), (caddr_t)&(uap->shmid),
	    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
	    AUP_USER | sizeof (struct shmid_ds), (caddr_t)&(uap->arg));
}

/*
 * Shmget (create new shmem) system call.
 */
au_shmget()
{
	register struct a {
		key_t	key;
		uint	size;
		int	shmflg;
	}	*uap = (struct a *)u.u_ap;
	int au;

	au = AU_DREAD |
	    au_ipcexists(uap->key, &shmem[0].shm_perm,
		shminfo.shmmni, sizeof (struct shmid_ds)) |
	    au_ipcwrite(uap->shmflg);

	shmget();

	au_sysaudit(AUR_SHMGET, au, u.u_error, u.u_rval1, 3,
	    sizeof (uap->key), (caddr_t)&(uap->key),
	    sizeof (uap->size), (caddr_t)&(uap->size),
	    sizeof (uap->shmflg), (caddr_t)&(uap->shmflg));
}

/*
 * Shmat (attach shared segment) system call.
 */
au_shmat()
{
	register struct a {
		int	shmid;
		addr_t	addr;
		int	flag;
	}	*uap = (struct a *)u.u_ap;
	int au;

	au = AU_DREAD | au_ipcwrite(uap->flag);

	shmat();

	au_sysaudit(AUR_SHMAT, au, u.u_error, u.u_rval1, 3,
	    sizeof (uap->shmid), (caddr_t)&(uap->shmid),
	    sizeof (uap->addr), (caddr_t)&(uap->addr),
	    sizeof (uap->flag), (caddr_t)&(uap->flag));
}

/*
 * System entry point for shmat, shmctl, shmdt, and shmget system calls.
 */

int shmdt();
int	(*au_shmcalls[])() = {au_shmat, au_shmctl, shmdt, au_shmget};

au_shmsys()
{
	register struct a {
		uint	id;
	}	*uap = (struct a *)u.u_ap;

	if (uap->id > 3) {
		u.u_error = EINVAL;
		return;
	}
	u.u_ap = &u.u_arg[1];
	(*au_shmcalls[uap->id])();
}

/*
 * simple check for the write bits in an ipc flag
 */

int
au_ipcwrite(flag)
	int flag;
{
	return ((flag & 0111) ? AU_DWRITE : 0);
}

/*
 * simple test to determine if a key is valid.
 */
int
au_ipcexists(key, base, count, size)
	key_t key;
	register struct ipc_perm *base;
	int count;
	unsigned size;
{
	register int i;	/* loop control */


	if (key == IPC_PRIVATE)
		return (AU_DCREATE);
	for (i = 0; i++ < count;
	    base = (struct ipc_perm *)(((char *)base) + size)) {
		if (base->key == key)
			return (0);
	}
	return (AU_DCREATE);
}
