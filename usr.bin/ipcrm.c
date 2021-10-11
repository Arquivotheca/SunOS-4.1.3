#ifndef lint
static	char sccsid[] = "@(#)ipcrm.c 1.1 92/07/30 SMI"; /* from S5R2 1.4 */
#endif

/*
**	ipcrm - IPC remove
**	Remove specified message queues, semaphore sets and shared memory ids.
**/

#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/msg.h>
#include	<sys/sem.h>
#include	<sys/shm.h>
#include	<errno.h>
#include	<stdio.h>

char opts[] = "q:m:s:Q:M:S:";	/* allowable options for getopt */
extern char	*optarg;	/* arg pointer for getopt */
extern int	optind;		/* option index for getopt */

key_t getkey();

main(argc, argv)
int	argc;	/* arg count */
char	**argv;	/* arg vector */
{
	register int	o;	/* option flag */
	register int	err;	/* error count */
	register int	ipc_id;	/* id to remove */
	register key_t	ipc_key;/* key to remove */
	extern	long	atol();

	/* Go through the options */
	err = 0;
	while ((o = getopt(argc, argv, opts)) != EOF)
		switch(o) {

		case 'q':	/* message queue */
			ipc_id = atoi(optarg);
			if (msgctl(ipc_id, IPC_RMID, 0) == -1)
				oops("msqid", (long)ipc_id);
			break;

		case 'm':	/* shared memory */
			ipc_id = atoi(optarg);
			if (shmctl(ipc_id, IPC_RMID, 0) == -1)
				oops("shmid", (long)ipc_id);
			break;

		case 's':	/* semaphores */
			ipc_id = atoi(optarg);
			if (semctl(ipc_id, IPC_RMID, 0) == -1)
				oops("semid", (long)ipc_id);
			break;

		case 'Q':	/* message queue (by key) */
			if ((ipc_key = getkey(optarg)) == IPC_PRIVATE)
				break;
			if ((ipc_id=msgget(ipc_key, 0)) == -1
				|| msgctl(ipc_id, IPC_RMID, 0) == -1)
				oops("msgkey", ipc_key);
			break;

		case 'M':	/* shared memory (by key) */
			if ((ipc_key = getkey(optarg)) == IPC_PRIVATE)
				break;
			if ((ipc_id=shmget(ipc_key, 0, 0)) == -1
				|| shmctl(ipc_id, IPC_RMID, 0) == -1)
				oops("shmkey", ipc_key);
			break;

		case 'S':	/* semaphores (by key) */
			if ((ipc_key = getkey(optarg)) == IPC_PRIVATE)
				break;
			if ((ipc_id=semget(ipc_key, 0, 0)) == -1
				|| semctl(ipc_id, IPC_RMID, 0) == -1)
				oops("semkey", ipc_key);
			break;

		default:
		case '?':	/* anything else */
			err++;
			break;
		}
	if (err || (optind < argc)) {
		fprintf(stderr,
		   "usage: ipcrm [ [-q msqid] [-m shmid] [-s semid]\n%s\n",
		   "	[-Q msgkey] [-M shmkey] [-S semkey] ... ]");
		exit(1);
	}
	exit(0);
	/* NOTREACHED */
}

oops(s, i)
char *s;
long   i;
{
	char *e;

	switch (errno) {

	case	ENOENT:	/* key not found */
	case	EINVAL:	/* id not found */
		e = "not found";
		break;

	case	EPERM:	/* permission denied */
		e = "permission denied";
		break;
	default:
		e = "unknown error";
	}

	fprintf(stderr, "ipcrm: %s(%ld): %s\n", s, i, e);
}

key_t
getkey(str)
char *str;
{
	key_t x;
	if ((x = (key_t)atol(str)) == IPC_PRIVATE)
		fprintf(stderr, "ipcrm: illegal key: %s\n", str);
	return (x);
}
