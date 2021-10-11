#if !defined(lint) && defined(SCCSIDS) 
static  char sccsid[] = "@(#)msgsys.c 1.1 92/07/30 SMI"; 
#endif

#include	<syscall.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/msg.h>


/* msgsys dispatch argument */
#define	MSGGET	0
#define	MSGCTL	1
#define	MSGRCV	2
#define	MSGSND	3


msgget(key, msgflg)
key_t key;
int msgflg;
{
	return(syscall(SYS_msgsys, MSGGET, key, msgflg));
}

msgctl(msqid, cmd, buf)
int msqid, cmd;
struct msqid_ds *buf;
{
	return(syscall(SYS_msgsys, MSGCTL, msqid, cmd, buf));
}

msgrcv(msqid, msgp, msgsz, msgtyp, msgflg)
int msqid;
struct msgbuf *msgp;
int msgsz;
long msgtyp;
int msgflg;
{
	return(syscall(SYS_msgsys, MSGRCV, msqid, msgp, msgsz, msgtyp, msgflg));
}

msgsnd(msqid, msgp, msgsz, msgflg)
int msqid;
struct msgbuf *msgp;
int msgsz, msgflg;
{
	return(syscall(SYS_msgsys, MSGSND, msqid, msgp, msgsz, msgflg));
}

