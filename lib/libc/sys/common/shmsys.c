#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)shmsys.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

#include	<syscall.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/shm.h>


/* shmsys dispatch argument */
#define	SHMAT	0
#define	SHMCTL	1
#define	SHMDT	2
#define SHMGET	3

char *
shmat(shmid, shmaddr, shmflg)
	int shmid;
	char *shmaddr;
	int shmflg;
{

	return ((char *)syscall(SYS_shmsys, SHMAT, shmid, shmaddr, shmflg));
}

shmctl(shmid, cmd, buf)
	int shmid, cmd;
	struct shmid_ds *buf;
{

	return (syscall(SYS_shmsys, SHMCTL, shmid, cmd, buf));
}

shmdt(shmaddr)
	char *shmaddr;
{

	return (syscall(SYS_shmsys, SHMDT, shmaddr));
}

shmget(key, size, shmflg)
	key_t key;
	int size, shmflg;
{

	return (syscall(SYS_shmsys, SHMGET, key, size, shmflg));
}
