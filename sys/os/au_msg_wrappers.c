#ifndef lint
static char sccsid[] = "@(#)au_msg_wrappers.c 1.1 92/07/30 SMI";	/* */
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
