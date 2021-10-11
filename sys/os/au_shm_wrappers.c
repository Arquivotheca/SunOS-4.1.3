#ifndef lint
static char sccsid[] = "@(#)au_shm_wrappers.c 1.1 92/07/30 SMI";	/* */
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
