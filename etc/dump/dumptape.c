#ifndef lint
static char sccsid[] = "@(#)dumptape.c 1.1 92/07/30 SMI"; /* from Caltech 5.4 7/1/86 */
#endif

#include <setjmp.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sun/dkio.h>		/* for FDKEJECT */
#include <sys/mtio.h>
#include "dump.h"

static char (*tblock)[TP_BSIZE];/* pointer to malloc()ed buffer for tape */
static int writesize;		/* size of malloc()ed buffer for tape */
static long lastspclrec = -1;	/* tape block number of last written header */
static int dumptoarchive = 1;	/* mark records to be archived */
static int trecno = 0;		/* next record to write in current block */
static int arch;		/* descriptor to which to write archive file */
static long inos[TP_NINOS];	/* starting inodes on each tape */
extern int ntrec;		/* blocking factor on tape */
extern int tenthsperirg;	/* tenths of an inch per inter-record gap */
extern int read(), write();
extern char *host;

/*
 * Concurrent dump mods (Caltech) - disk block reading and tape writing
 * are exported to several slave processes.  While one slave writes the
 * tape, the others read disk blocks; they pass control of the tape in
 * a ring via signals.	The parent process traverses the filesystem and
 * sends spclrec()'s and lists of daddr's to the slaves via pipes.
 */
struct req {			/* instruction packets sent to slaves */
	short count;
	short flag;
	daddr_t dblk;
} *req;
static int reqsiz;

#define SLAVES 3		/* 1 reading pipe, 1 reading disk, 1 writing */
static int slavefd[SLAVES];	/* pipes from master to each slave */
static int slavepid[SLAVES];	/* used by killall() */
static int rotor;		/* next slave to be instructed */
static int master;		/* pid of master, for sending error signals */
static int bufrecs;		/* tape records (not blocks) per buffer */
union u_spcl *nextspcl; 	/* where to copy next taprec record */

/*
 * Allocate tape buffer, on page boundary for tape write() efficiency,
 * with array of req packets immediately preceeding it so both can be
 * written together by flusht().
 */
alloctape()
{
	int pgoff = getpagesize() - 1;	    /* pagesize better be power of 2 */
	char *buf, *malloc();

	writesize = ntrec * TP_BSIZE;

	if (diskette) {
		bufrecs = ntrec;
	} else {
		bufrecs = ntrec + (20/ntrec)*ntrec;
	}

	reqsiz = (bufrecs+1) * sizeof (struct req);
	buf = malloc((u_int)(reqsiz + bufrecs * TP_BSIZE + pgoff));
	if (buf == NULL)
		return (0);

	tblock = (char (*)[TP_BSIZE]) (((long)buf + reqsiz + pgoff) &~ pgoff);
	nextspcl = (union u_spcl *)tblock;
	req = (struct req *) (tblock[0] - reqsiz);
	return (1);
}

/*
 * Start a process to collect the bitmap and directory information
 * being written to the front of the tape. When the pipe to this
 * process is closed, it writes out the collected data to the
 * requested file.
 */
setuparchive()
{
	struct list {
		struct	list *next;
		char	*data;
		long	size;
	} bufhead;
	register struct list *blp, *nblp;
	int cmd[2], pid, fd, punt = 0;
	char *buf;

	if (pipe(cmd) < 0 || (pid = fork()) < 0) {
		perror("  DUMP: cannot create child to write archive file");
		return (0);
	}
	if (pid > 0) {
		/* parent process */
		close(cmd[0]);
		arch = cmd[1];
		return (pid);
	}
	/* child process */
	close(cmd[1]);
	for (blp = &bufhead; blp->size > 0; ) {
		nblp = (struct list *)malloc(sizeof (struct list));
		buf = (char *)malloc((u_int)writesize);
		if (nblp == 0 || buf == 0) {
			if (!punt)
				msg("Out of memory to create archive file %s\n",
					archivefile);
			punt++;
			if (blp == &bufhead)
				dumpabort();
			blp->next = 0;
		} else {
			nblp->data = buf;
			blp->next = nblp;
			blp = nblp;
		}
		blp->size = atomic(read, cmd[0], blp->data, writesize);
	}
	if (punt)
		Exit(X_ABORT);
	close(cmd[0]);
	if ((fd = open(archivefile, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
		msg("Cannot create archive file %s: ", archivefile);
		perror("");
	}
	for (blp = bufhead.next; blp && blp->size == writesize; blp = blp->next)
		if (write(fd, blp->data, writesize) != writesize)
			perror(archivefile);
	close(fd);
	Exit(X_FINOK);
	/* NOTREACHED */
}

spclrec()
{
	register int s, i, *ip;

	if (spcl.c_type == TS_END) {
		spcl.c_flags |= DR_INODEINFO;
		bcopy((char *)inos, (char *)spcl.c_inos, sizeof inos);
	}
	spcl.c_inumber = ino;
	spcl.c_magic = NFS_MAGIC;
	spcl.c_checksum = 0;
	ip = (int *)&spcl;
	s = 0;
	i = sizeof (union u_spcl) / (4 * sizeof (int));
	while (--i >= 0) {
		s += *ip++; s += *ip++;
		s += *ip++; s += *ip++;
	}
	spcl.c_checksum = CHECKSUM - s;
	if (spcl.c_type == TS_END && archive && !doingverify &&
	    write(arch, (char *)&spcl, TP_BSIZE) != TP_BSIZE) {
		msg("Cannot write archive file %s", archivefile);
		perror("");
	}
	taprec((char *)&spcl);
	if (spcl.c_type == TS_END)
		spcl.c_flags &=~ DR_INODEINFO;
}

taprec(dp)
	char *dp;
{
	struct s_spcl *spclp;

	spclp = &((union u_spcl *)dp)->s_spcl;
	if (spclp->c_type == TS_INODE &&
	    (spclp->c_dinode.di_mode & IFMT) != IFDIR)
		dumptoarchive = 0;
	req[trecno].flag = dumptoarchive;
	req[trecno].dblk = (daddr_t)0;
	req[trecno].count = 1;
	*nextspcl++ = *(union u_spcl *)dp;			/* block move */
	lastspclrec = spcl.c_tapea;
	trecno++;
	spcl.c_tapea++;
	if (trecno >= bufrecs ||
		(spcl.c_type == TS_EOM || spcl.c_type == TS_END) &&
			trecno % ntrec == 0)
		flusht();
}

dmpblk(blkno, size)
	daddr_t blkno;
	long size;
{
	register int avail, tpblks;
	daddr_t dblkno;

	dblkno = fsbtodb(sblock, blkno);
	tpblks = size / TP_BSIZE;
	while ((avail = MIN(tpblks, bufrecs - trecno)) > 0) {
		req[trecno].dblk = dblkno;
		req[trecno].count = avail;
		req[trecno].flag = dumptoarchive;
		trecno += avail;
		spcl.c_tapea += avail;
		if (trecno >= bufrecs)
			flusht();
		dblkno += avail * (TP_BSIZE / DEV_BSIZE);
		tpblks -= avail;
	}
}

static int nogripe = 0;

void
tperror()
{

	if (pipeout) {
		msg("Write error on %s\n", tape);
		msg("Cannot recover\n");
		dumpabort();
		/* NOTREACHED */
	}
	if (!doingverify) {
		if (diskette)
			msg("Diskette write error %ld blocks into volume %d\n",
				asize*2, tapeno);
		else
			msg("Tape write error %ld feet into tape %d\n",
				asize/120L, tapeno);
		broadcast("WRITE ERROR!\n");
		if (!query("Do you want to restart?"))
			dumpabort();
		if (!diskette) {
			msg("This tape will rewind.  After it is rewound,\n");
			msg("replace the faulty tape with a new one;\n");
			msg("this dump volume will be rewritten.\n");
		}
		killall();
		nogripe = 1;
		close_rewind();
		Exit(X_REWRITE);
	} else {
		msg("Tape verification error %ld feet into tape %d\n",
			asize/120L, tapeno);
		broadcast("TAPE VERIFICATION ERROR!\n");
		if (!query("Do you want to rewrite?"))
			dumpabort();
		msg("This tape will be rewritten and then verified\n");
		killall();
		rewind();
		Exit(X_REWRITE);
	}
}

/* compatibility routine */
tflush(i)
	int i;
{

	for (i = 0; i < ntrec; i++)
		spclrec();
}

flusht()
{
	int siz = (char *)nextspcl - (char *)req;

	req[trecno].count = 0;			/* Sentinel */
	if (atomic(write, slavefd[rotor], (char *)req, siz) != siz) {
		perror("  DUMP: error writing command pipe");
		dumpabort();
	}
	if (++rotor >= SLAVES)
		rotor = 0;
	nextspcl = (union u_spcl *)tblock;
	if (diskette)
		asize += trecno;  /* asize == blocks written to diskette */
	else
		asize += (writesize/density + tenthsperirg) * trecno / ntrec;
	blockswritten += trecno;
	trecno = 0;

	/* prevent infinite loops while writing end of media record */
	if (writing_eom)
		return;

	if (!pipeout && asize > tsize) {
		if (verify && !doingverify)
			rewind();
		else {
			if (diskette)
				write_end_of_media();
			close_rewind();
		}
		otape(0);
	}
	timeest();
}

rewind()
{
	int f;

	for (f = 0; f < SLAVES; f++)
		close(slavefd[f]);
	while (wait((union wait *)NULL) >= 0)
		;	/* wait for any signals from slaves */
	if (pipeout)
		return;
	if (diskette) {
		/* blindly toss diskette */
		(void)ioctl( to, FDKEJECT, 0 );
	} else {
		if (doingverify) {
			/*
			 * Space to the end of the tape.
			 * Backup first in case we already read the EOF.
			 */
			if (host) {
				(void) rmtioctl(MTBSR, 1);
				if (rmtioctl(MTEOM, 1) < 0)
					(void) rmtioctl(MTFSF, 1);
			} else {
				static struct mtop bsr = { MTBSR, 1 };
				static struct mtop eom = { MTEOM, 1 };
				static struct mtop fsf = { MTFSF, 1 };

				(void) ioctl(to, MTIOCTOP, &bsr);
				if (ioctl(to, MTIOCTOP, &eom) < 0)
					(void) ioctl(to, MTIOCTOP, &fsf);
			}
		}
		msg("Tape rewinding\n");
		if (host) {
			rmtclose();
			while (rmtopen(tape, 0) < 0)
				sleep(10);
			rmtclose();
		} else {
			close(to);
			while ((f = open(tape, 0)) < 0)
				sleep(10);
			close(f);
		}
	}
}

close_rewind()
{

	rewind();

	if (!nogripe) {
		msg("Change Volumes: Mount volume #%d\n", tapeno+1);
		broadcast("CHANGE VOLUMES!\7\7\n");
	}
	while (!query("Is the new volume mounted and ready to go?"))
		if (query ("Do you want to abort?")) {
			dumpabort();
			/*NOTREACHED*/
		}
}

/*
 *	We implement taking and restoring checkpoints on the tape level.
 *	When each tape is opened, a new process is created by forking; this
 *	saves all of the necessary context in the parent.  The child
 *	continues the dump; the parent waits around, saving the context.
 *	If the child returns X_REWRITE, then it had problems writing that tape;
 *	this causes the parent to fork again, duplicating the context, and
 *	everything continues as if nothing had happened.
 */
otape(top)
	int top;
{
	int parentpid;
	int childpid;
	int status;
	int waitpid, killedpid;
	long blks, i;
	static int archivepid = 0;
	void (*interrupt)() = signal(SIGINT, SIG_IGN);

	parentpid = getpid();

	if (verify) {
		if (doingverify)
			doingverify = 0;
		else
			Exit(X_VERIFY);
	}
    restore_check_point:
	if (archive && top && !doingverify) {
		killedpid = archivepid;
		if (archivepid)
			kill(archivepid, SIGKILL);
		archivepid = setuparchive();
	}

	(void)signal(SIGINT, interrupt);
	fflush(stderr);
	/*
	 *	All signals are inherited...
	 */
	childpid = fork();
	if (childpid < 0) {
		msg("Context save fork fails in parent %d\n", parentpid);
		Exit(X_ABORT);
	}
	if (childpid != 0) {
		/*
		 *	PARENT:
		 *	save the context by waiting
		 *	until the child doing all of the work returns.
		 *	don't catch the interrupt
		 */
		(void)signal(SIGINT, SIG_IGN);
#ifdef TDEBUG
		msg("Volume: %d; parent process: %d child process %d\n",
			tapeno+1, parentpid, childpid);
#endif TDEBUG
		for (;;) {
			waitpid = wait((union wait *)&status);
			if (waitpid == childpid)
				break;
			if (waitpid == killedpid)
				continue;
			msg("Parent %d waiting for child %d has another child %d return\n",
			    parentpid, childpid, waitpid);
		}
		if (status & 0xFF) {
			msg("Child %d returns LOB status %d\n",
				childpid, status&0xFF);
		}
		status = (status >> 8) & 0xFF;
#ifdef TDEBUG
		switch (status) {
		case X_FINOK:
			msg("Child %d finishes X_FINOK\n", childpid);
			break;
		case X_ABORT:
			msg("Child %d finishes X_ABORT\n", childpid);
			break;
		case X_REWRITE:
			msg("Child %d finishes X_REWRITE\n", childpid);
			break;
		case X_VERIFY:
			msg("Child %d finishes X_VERIFY\n", childpid);
			break;
		default:
			msg("Child %d finishes unknown %d\n", childpid, status);
			break;
		}
#endif TDEBUG
		switch (status) {
		case X_FINOK:
			close(arch);
			while (wait((union wait *)NULL) >= 0)
				;	/* wait for archive process */
			Exit(X_FINOK);
		case X_ABORT:
			Exit(X_ABORT);
		case X_VERIFY:
			doingverify++;
			goto restore_check_point;
		case X_REWRITE:
			if (archive && !top && dumptoarchive)
				Exit(X_REWRITE);
			doingverify = 0;
			goto restore_check_point;
		default:
			msg("Bad return code from dump: %d\n", status);
			Exit(X_ABORT);
		}
		/*NOTREACHED*/
	} else {	/* we are the child; just continue */
#ifdef TDEBUG
		sleep(4);	/* allow time for parent's message to get out */
		msg("Child on Volume %d has parent %d, my pid = %d\n",
		    tapeno+1, parentpid, getpid());
#endif
		while ((to = host ? rmtopen(tape, 2) :
		    pipeout ? 1 :
		    doingverify ? open(tape, 0) : creat(tape, 0666)) < 0)
			if (!query("Cannot open volume.  Do you want to retry the open?"))
				dumpabort();

		if (doingverify) {
			/*
			 * If we're using the non-rewinding tape device,
			 * the tape will be left positioned after the
			 * EOF mark.  We need to back up to the beginning
			 * of this tape file (cross two tape marks in the
			 * reverse direction and one in the forward
			 * direction) before the verify pass.
			 */
			if (host) {
				if (rmtioctl(MTBSF, 2) >= 0)
					(void) rmtioctl(MTFSF, 1);
				else
					(void) rmtioctl(MTNBSF, 1);
			} else {
				static struct mtop bsf = { MTBSF, 2 };
				static struct mtop fsf = { MTFSF, 1 };
				static struct mtop nbsf = { MTNBSF, 1 };

				if (ioctl(to, MTIOCTOP, &bsf) >= 0)
					(void) ioctl(to, MTIOCTOP, &fsf);
				else
					(void) ioctl(to, MTIOCTOP, &nbsf);
			}
		}

		enslave();  /* Share open tape file descriptor with slaves */
		asize = 0;
		tapeno++;		/* current tape sequence */
		newtape++;		/* new tape signal */
		blks = 0;
		if (spcl.c_type != TS_END)
			for (i = 0; i < spcl.c_count; i++)
				if (spcl.c_addr[i] != 0)
					blks++;
		spcl.c_count = blks + 1 - spcl.c_tapea + lastspclrec;
		spcl.c_volume++;
		if (tapeno == 1)
			inos[1] = 2;
		else if (tapeno < TP_NINOS)
			inos[tapeno] = ino;
		spcl.c_type = TS_TAPE;
		spcl.c_flags |= DR_NEWHEADER;
		spclrec();
		spcl.c_flags &=~ DR_NEWHEADER;
		if (doingverify)
			msg("Starting verify pass\n");
		else if (tapeno > 1)
			msg("Volume %d begins with blocks from ino %d\n",
			    tapeno, ino);
	}
}

void
dumpabort()
{

	if (master != 0 && master != getpid())
		kill(master, SIGTERM);	/* Signals master to call dumpabort */
	else {
		killall();
		msg("The ENTIRE dump is aborted.\n");
	}
	Exit(X_ABORT);
}

Exit(status)
{

#ifdef TDEBUG
	msg("pid = %d exits with status %d\n", getpid(), status);
#endif TDEBUG
	exit(status);
}

void
sigpipe()
{

	msg("Broken pipe\n");
	dumpabort();
}

killall()
{
	register int i;

	for (i = 0; i < SLAVES; i++)
		if (slavepid[i] > 0)
			kill(slavepid[i], SIGKILL);
}

static int ready, caught;
static jmp_buf jbuf;

void
proceed()
{

	if (ready)
		longjmp(jbuf, 1);
	caught++;
}

enslave()
{
	int cmd[2];			/* file descriptors */
	register int i, j;

	master = getpid();
	(void)signal(SIGTERM, dumpabort); /* Slave sends SIGTERM on dumpabort() */
	(void)signal(SIGPIPE, sigpipe);
	(void)signal(SIGIOT, tperror);    /* Slave sends SIGIOT on tape errors */
	(void)signal(SIGTRAP, proceed);
	for (i = 0; i < SLAVES; i++) {
		if (pipe(cmd) < 0 || (slavepid[i] = fork()) < 0) {
			perror("  DUMP: can't create child");
			dumpabort();
		}
		slavefd[i] = cmd[1];
		if (slavepid[i] == 0) {		/* Slave starts up here */
			int next;		    /* pid of neighbor */
#ifdef TDEBUG
		sleep(4);	/* allow time for parent's message to get out */
		msg("Neighbor has pid = %d\n", getpid());
#endif

			for (j = 0; j <= i; j++)
				close(slavefd[j]);
			close(fi);		    /* Need our own seek ptr */
			if ((fi = open(disk, 0)) < 0) {
				perror("  DUMP: can't reopen disk");
				dumpabort();
			}
			signal(SIGINT, SIG_IGN);    /* Master handles this */
			atomic(read, cmd[0], (char *)&next, sizeof (next));
			doslave(cmd[0], next, i);
			Exit(X_FINOK);
		}
		close(cmd[0]);
	}
	for (i = 0; i < SLAVES; i++)
		atomic(write, slavefd[i], 
			(char *)&slavepid[(i + 1) % SLAVES], sizeof (int));
	kill(slavepid[0], SIGTRAP);
	master = 0;
	rotor = 0;
}

doslave(cmd, next, mynum)
	int cmd, next, mynum;
{
	int nread;
	char *rbuf;
	long archivesize, size;

	if (doingverify) {
		rbuf = (char *)malloc((u_int)writesize);
		if (rbuf == 0) {
			kill(master, SIGIOT);	/* Restart from checkpoint */
			pause();
		}
	}
	while ((nread = atomic(read, cmd, (char *)req, reqsiz)) == reqsiz) {
		register struct req *p;
		register int nrec = 0, trec;
		register char *tp;

		for (p = req, trecno = 0; p->count > 0;
		    trecno += p->count, p += p->count) {
			if (p->dblk == 0)
				trec = trecno - nrec++;
		}
		if (nrec > 0 &&
		    atomic(read, cmd, (char *)tblock[trec], nrec*TP_BSIZE) !=
		    nrec*TP_BSIZE) {
			msg("Master/slave protocol botched\n");
			dumpabort();
		}
		archivesize = 0;
		for (p = req, trecno = 0; p->count > 0;
		    trecno += p->count, p += p->count) {
			size = p->count * TP_BSIZE;
			if (p->flag)
				archivesize += size;
			if (p->dblk)
				bread(p->dblk, tblock[trecno], size);
			else if (trecno < trec)
				*(union u_spcl *)tblock[trecno] =
				    *(union u_spcl *)tblock[trec++];
		}
		if (setjmp(jbuf) == 0) {
			ready = 1;
			if (!caught)
				pause();
		}
		ready = caught = 0;
		for (tp = tblock[0]; (trecno -= ntrec) >= 0; tp += writesize) {
			if (archive && !doingverify && archivesize > 0) {
				size = archivesize < writesize ?
					archivesize : writesize;
				archivesize -= size;
				if (write(arch, tp, (int)size) != size) {
					msg("Cannot write archive file %s",
						archivefile);
					perror("");
					archive = 0;
				}
			}
			if (host) {		/* prime the pipeline */
				if (!doingverify) {
					rmtwrite0(writesize);
					rmtwrite1(tp, writesize);
					if (mynum == 0) {
						--mynum;
						continue;
					}
					if (rmtwrite2() == writesize)
						continue;
					rmtwrite2();	/* ignore 2nd error */
				} else {
					if (rmtread(rbuf, writesize) ==
					    writesize &&
					    !bcmp(rbuf, tp, writesize))
						continue;
				}
			} else {
				if (!doingverify) {
					if (write(to, tp, writesize) ==
					    writesize)
						continue;
				} else {
					if (read(to, rbuf, writesize) ==
					    writesize &&
					    !bcmp(rbuf, tp, writesize))
						continue;
				}
			}
			kill(master, SIGIOT);	/* Restart from checkpoint */
			pause();
		}
		kill(next, SIGTRAP);	    /* Next slave's turn */
	}
	if (nread != 0) {
		perror("  DUMP: error reading command pipe");
		dumpabort();
	}
	if (host) {
		if (setjmp(jbuf) == 0) {
			ready++;
			if (!caught)
				pause();
		}
		ready = caught = 0;
		if (mynum < 0 && rmtwrite2() != writesize) {
			kill(master, SIGIOT);
			pause();
		}
		kill(next, SIGTRAP);
	}
}

/*
 * Since a read from a pipe may not return all we asked for,
 * or a write may not write all we ask if we get a signal,
 * loop until the count is satisfied (or error).
 */
atomic(func, fd, buf, count)
	int (*func)(), fd, count;
	char *buf;
{
	int got, need = count;
	extern int errno;

	while (need > 0) {
		got = (*func)(fd, buf, MIN(need, 4096));
		if (got < 0 && errno == EINTR)
			continue;
		if (got <= 0)
			break;
		buf += got;
		need -= got;
	}
	return ( (count-=need) == 0 ? got : count);
}

int
write_end_of_media()
{
	int i;

	/*
	 * prevent flusht() from calling write_end_of_media()
	 * recursively while writing end of media record
	 */
	writing_eom = 1;
	spcl.c_type = TS_EOM;
	for (i=trecno; i < ntrec; i++)
		spclrec();
	writing_eom = 0;
}
