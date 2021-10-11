#ifndef lint
static	char sccsid[] = "@(#)ipcs.c 1.1 92/07/30 SMI"; /* from SYSV.2 1.5 */
#endif

/*
**	ipcs - IPC status
**	Examine and print certain things about message queues, semaphores,
**		and shared memory.
**/

#include	<sys/types.h>

#define KERNEL
#include	<sys/ipc.h>
#include	<sys/msg.h>
#include	<sys/sem.h>
#include	<sys/shm.h>
#undef KERNEL

#include	<nlist.h>
#include	<fcntl.h>
#include	<time.h>
#include	<grp.h>
#include	<pwd.h>
#include	<stdio.h>
#include	<kvm.h>

#define	TIME	0
#define	MSG	1
#define	SEM	2
#define	SHM	3
#define	MSGINFO	4
#define	SEMINFO	5
#define	SHMINFO	6

struct nlist nl[] = {		/* name list entries for IPC facilities */
	{"_time"},
	{"_msgque"},
	{"_sema"},
	{"_shmem"},
	{"_msginfo"},
	{"_seminfo"},
	{"_shminfo"},
	{NULL}
};
char	chdr[] = "T     ID     KEY        MODE       OWNER    GROUP",
				/* common header format */
	chdr2[] = "  CREATOR   CGROUP",
				/* c option header format */
	*name = NULL,		/* default name list: use live kernel */
	*mem = NULL,		/* default memory file: use live kernel */
	opts[] = "abcmopqstC:N:";/* allowable options for getopt */
extern char	*optarg;	/* arg pointer for getopt */
int		bflg,		/* biggest size:
					segsz on m; qbytes on q; nsems on s */
		cflg,		/* creator's login and group names */
		mflg,		/* shared memory status */
		oflg,		/* outstanding data:
					nattch on m; cbytes, qnum on q */
		pflg,		/* process id's: lrpid, lspid on q;
					cpid, lpid on m */
		qflg,		/* message queue status */
		sflg,		/* semaphore status */
		tflg,		/* times: atime, ctime, dtime on m;
					ctime, rtime, stime on q;
					ctime, otime on s */
		err;		/* option error count */
extern int	optind;		/* option index for getopt */
kvm_t		*kd;		/* kernel descriptor */

extern char		*ctime();
extern struct group	*getgrgid();
extern struct passwd	*getpwuid();
extern long		lseek();

main(argc, argv)
int	argc;	/* arg count */
char	**argv;	/* arg vector */
{
	register int	i,	/* loop control */
			o;	/* option flag */
	time_t		time;	/* date in memory file */
	struct shmid_ds	mds;	/* shared memory data structure */
	struct shminfo shminfo;	/* shared memory information structure */
	struct msqid_ds	qds;	/* message queue data structure */
	struct msginfo msginfo;	/* message information structure */
	struct semid_ds	sds;	/* semaphore data structure */
	struct seminfo seminfo;	/* semaphore information structure */
	long		offset;	/* used for indirect seeks */
	char		hostnm[64];	/* used for gethostname() */

	/* Go through the options and set flags. */
	while((o = getopt(argc, argv, opts)) != EOF)
		switch(o) {
		case 'a':
			bflg = cflg = oflg = pflg = tflg = 1;
			break;
		case 'b':
			bflg = 1;
			break;
		case 'c':
			cflg = 1;
			break;
		case 'C':
			mem = optarg;
			break;
		case 'm':
			mflg = 1;
			break;
		case 'N':
			name = optarg;
			break;
		case 'o':
			oflg = 1;
			break;
		case 'p':
			pflg = 1;
			break;
		case 'q':
			qflg = 1;
			break;
		case 's':
			sflg = 1;
			break;
		case 't':
			tflg = 1;
			break;
		case '?':
			err++;
			break;
		}
	if(err || (optind < argc)) {
		fprintf(stderr,
		    "usage:  ipcs [-abcmopqst] [-C corefile] [-N namelist]\n");
		exit(1);
	}
	if((mflg + qflg + sflg) == 0)
		mflg = qflg = sflg = 1;

	/* initialize for Kernel VM access. */
	if((kd = kvm_open(name, mem, NULL, O_RDONLY, "ipcs")) == NULL) {
		exit(1);
	}
	if (kvm_nlist(kd, nl) == -1) {
		fprintf(stderr, "ipcs:  bad namelist\n");
		exit(1);
	}

	if (mem == NULL) {
		gethostname(hostnm, sizeof(hostnm));
		mem = hostnm;
	}
	readsym(TIME, &time, sizeof(time));
	printf("IPC status from %s as of %s", mem, ctime(&time));

	/* Print Message Queue status report. */
	if(qflg) {
		i = 0;
		if ((nl[MSGINFO].n_type != 0) &&
		  (readsym(MSGINFO, &msginfo, sizeof(msginfo)),
		  (msginfo.msgmni != 0))) {
		    /* find the real address of the id pool */
		    readsym(MSG, &offset, sizeof(offset));

		    printf("%s%s%s%s%s%s\nMessage Queues:\n", chdr,
			    cflg ? chdr2 : "",
			    oflg ? " CBYTES  QNUM" : "",
			    bflg ? " QBYTES" : "",
			    pflg ? " LSPID LRPID" : "",
			    tflg ? "   STIME    RTIME    CTIME " : "");
		    while(i < msginfo.msgmni) {
			reade(offset, &qds, sizeof(qds));
			offset += sizeof(qds);
			if(!(qds.msg_perm.mode & IPC_ALLOC)) {
				i++;
				continue;
			}
			hp('q', "SRrw-rw-rw-", &qds.msg_perm, i++, msginfo.msgmni);
			if(oflg)
				printf("%7u%6u", qds.msg_cbytes, qds.msg_qnum);
			if(bflg)
				printf("%7u", qds.msg_qbytes);
			if(pflg)
				printf("%6u%6u", qds.msg_lspid, qds.msg_lrpid);
			if(tflg) {
				tp(qds.msg_stime);
				tp(qds.msg_rtime);
				tp(qds.msg_ctime);
			}
			printf("\n");
		    }
		} else {
		    printf("Message Queue facility not in system.\n");
		}
	}

	/* Print Shared Memory status report. */
	if(mflg) {
		i = 0;
		if ((nl[SHMINFO].n_type != 0) &&
		  (readsym(SHMINFO, &shminfo, sizeof(shminfo)),
		  (shminfo.shmmni != 0))) {
		    /* find the real address of the id pool */
		    readsym(SHM, &offset, sizeof(long));

		    if(oflg || bflg || tflg || !qflg || !nl[MSG].n_value)
			    printf("%s%s%s%s%s%s\n", chdr,
				    cflg ? chdr2 : "",
				    oflg ? " NATTCH" : "",
				    bflg ? "  SEGSZ" : "",
				    pflg ? "  CPID  LPID" : "",
				    tflg ? "   ATIME    DTIME    CTIME " : "");
		    printf("Shared Memory:\n");
		    while(i < shminfo.shmmni) {
			reade(offset, &mds, sizeof(mds));
			offset += sizeof(mds);
			if(!(mds.shm_perm.mode & IPC_ALLOC)) {
				i++;
				continue;
			}
			/* IPC_STAT needed to update shm_nattch */
			if (shmctl(i + shminfo.shmmni * mds.shm_perm.seq, 
							IPC_STAT, &mds) < 0) {
				perror("shmctl");
				exit(1);
			}
			hp('m', "DCrw-rw-rw-", &mds.shm_perm, i++, shminfo.shmmni);
			if(oflg)
				printf("%7u", mds.shm_nattch);
			if(bflg)
				printf("%7d", mds.shm_segsz);
			if(pflg)
				printf("%6u%6u", mds.shm_cpid, mds.shm_lpid);
			if(tflg) {
				tp(mds.shm_atime);
				tp(mds.shm_dtime);
				tp(mds.shm_ctime);
			}
			printf("\n");
		    }
		} else {
		    printf("Shared Memory facility not in system.\n");
		}
	}

	/* Print Semaphore facility status. */
	if(sflg) {
		i = 0;
		if ((nl[SEMINFO].n_type != 0) &&
		  (readsym(SEMINFO, &seminfo, sizeof(seminfo)),
		  (seminfo.semmni != 0))) {
		    /* find the real address of the id pool */
		    readsym(SEM, &offset, sizeof(long));

		    if(bflg || tflg || (!qflg || !nl[MSG].n_value) &&
			(!mflg || !nl[SHM].n_value))
			    printf("%s%s%s%s\n", chdr,
				    cflg ? chdr2 : "",
				    bflg ? " NSEMS" : "",
				    tflg ? "   OTIME    CTIME " : "");
		    printf("Semaphores:\n");
		    while(i < seminfo.semmni) {
			reade(offset, &sds, sizeof(sds));
			offset += sizeof(sds);
			if(!(sds.sem_perm.mode & IPC_ALLOC)) {
				i++;
				continue;
			}
			hp('s', "--ra-ra-ra-", &sds.sem_perm, i++, seminfo.semmni);
			if(bflg)
				printf("%6u", sds.sem_nsems);
			if(tflg) {
				tp(sds.sem_otime);
				tp(sds.sem_ctime);
			}
			printf("\n");
		    }
		} else {
		    printf("Semaphore facility not in system.\n");
		}
	}
	exit(0);
	/* NOTREACHED */
}

/*
**	readsym - read kernel symbol value with error exit
*/

readsym(n, b, s)
int	n,	/* nl offset */
	s;	/* size */
char	*b;	/* buffer address */
{
	if (nl[n].n_type == 0) {
		fprintf(stderr, "ipcs:  '%s' not in namelist\n", nl[n].n_name);
		exit(1);
	}
	return (reade(nl[n].n_value, b, s));
}

/*
**	reade - read with error exit
*/

reade(a, b, s)
u_long	a;	/* address */
int	s;	/* size */
char	*b;	/* buffer address */
{
	if (kvm_read(kd, a, b, s) != s) {
		fprintf(stderr, "ipcs:  kernel read error\n");
		exit(1);
	}
	return (0);
}

/*
**	hp - common header print
*/

hp(type, modesp, permp, slot, slots)
char				type,	/* facility type */
				*modesp;/* ptr to mode replacement characters */
register struct ipc_perm	*permp;	/* ptr to permission structure */
int				slot,	/* facility slot number */
				slots;	/* # of facility slots */
{
	register int		i,	/* loop control */
				j;	/* loop control */
	register struct group	*g;	/* ptr to group group entry */
	register struct passwd	*u;	/* ptr to user passwd entry */

	printf("%c%7d%s%#8.8x ", type, slot + slots * permp->seq,
		permp->key ? " " : " 0x", permp->key);
	for(i = 02000;i;modesp++, i >>= 1)
		printf("%c", (permp->mode & i) ? *modesp : '-');
	if((u = getpwuid(permp->uid)) == NULL)
		printf("%9d", permp->uid);
	else
		printf("%9.8s", u->pw_name);
	if((g = getgrgid(permp->gid)) == NULL)
		printf("%9d", permp->gid);
	else
		printf("%9.8s", g->gr_name);
	if(cflg) {
		if((u = getpwuid(permp->cuid)) == NULL)
			printf("%9d", permp->cuid);
		else
			printf("%9.8s", u->pw_name);
		if((g = getgrgid(permp->cgid)) == NULL)
			printf("%9d", permp->cgid);
		else
			printf("%9.8s", g->gr_name);
	}
}

/*
**	tp - time entry printer
*/

tp(time)
time_t	time;	/* time to be displayed */
{
	register struct tm	*t;	/* ptr to converted time */

	if(time) {
		t = localtime(&time);
		printf(" %2d:%2.2d:%2.2d", t->tm_hour, t->tm_min, t->tm_sec);
	} else
		printf(" no-entry");
}
