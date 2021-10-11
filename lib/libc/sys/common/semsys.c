#if !defined(lint) && defined(SCCSIDS) 
static  char sccsid[] = "@(#)semsys.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

#include	<syscall.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>

/* semsys dispatch argument */
#define SEMCTL  0
#define SEMGET  1
#define SEMOP   2

/*VARARGS3*/
semctl(semid, semnum, cmd, arg)
int semid, cmd;
int semnum;
union semun arg;
{
	switch (cmd) {

	case SETVAL:
	case GETALL:
	case SETALL:
	case IPC_STAT:
	case IPC_SET:
		return(syscall(SYS_semsys,SEMCTL,semid,semnum,cmd,arg.val));

	default:
		return(syscall(SYS_semsys,SEMCTL,semid,semnum,cmd,0));
	}
}

semget(key, nsems, semflg)
key_t key;
int nsems, semflg;
{
	return(syscall(SYS_semsys, SEMGET, key, nsems, semflg));
}

semop(semid, sops, nsops)
int semid;
struct sembuf (*sops)[];
int nsops;
{
	return(syscall(SYS_semsys, SEMOP, semid, sops, nsops));
}
