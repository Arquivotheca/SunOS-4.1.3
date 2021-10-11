/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)cmds.c 1.1 92/07/30 SMI"; /* from UCB 5.2 3/30/86 */
#endif not lint

/*
 * lpc -- line printer control program -- commands:
 */

#include "lp.h"
#include <sys/time.h>

static char *next_printer();

/*
 * kill an existing daemon and disable printing.
 */
abort(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: abort {all | printer ...}\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0)
			printf("unknown printer %s\n", printer);
		else
			abortpr(1);
	}
}

abortpr(dis)
	int dis;
{
	register FILE *fp;
	struct stat stbuf;
	int pid, fd;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);

	/*
	 * Turn on the owner execute bit of the lock file to disable printing.
	 */
	if (dis) {
		if (stat(line, &stbuf) >= 0) {
			if (chmod(line, (stbuf.st_mode & 0777) | 0100) < 0)
				printf("\tcannot disable printing\n");
			else
				printf("\tprinting disabled\n");
		} else if (errno == ENOENT) {
			if ((fd = open(line, O_WRONLY|O_CREAT, 0760)) < 0)
				printf("\tcannot create lock file\n");
			else {
				(void) close(fd);
				printf("\tprinting disabled\n");
				printf("\tno daemon to abort\n");
			}
			return;
		} else {
			printf("\tcannot stat lock file\n");
			return;
		}
	}
	/*
	 * Kill the current daemon to stop printing now.
	 */
	if ((fp = fopen(line, "r")) == NULL) {
		printf("\tcannot open lock file\n");
		return;
	}
	if (!getline(fp) || flock(fileno(fp), LOCK_SH|LOCK_NB) == 0) {
		(void) fclose(fp);	/* unlocks as well */
		printf("\tno daemon to abort\n");
		return;
	}
	(void) fclose(fp);
	if (kill(pid = atoi(line), SIGTERM) < 0)
		printf("\tWarning: daemon (pid %d) not killed\n", pid);
	else
		printf("\tdaemon (pid %d) killed\n", pid);
}

/*
 * Remove all spool files and temporaries from the spooling area.
 */
clean(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: clean {all | printer ...}\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else
			cleanpr();
	}
}

/*
 * Select spooling files.
 */
sselect(d)
	struct direct *d;
{
	register int c = d->d_name[0];
	register int f = d->d_name[1];

	return (f == 'f' && (c == 'c' || c == 'd' || c == 't'));
}

/*
 * Comparison routine for scandir. Sort by job number and machine, then
 * by `cf', `tf', or `df', then by the sequence letter A-Z, a-z.
 */
sortq(d1, d2)
	struct direct **d1, **d2;
{
	register int c1, c2;

	c1 = strcmp((*d1)->d_name + 3, (*d2)->d_name + 3);
	if (c1 != 0)
		return (c1);

	c1 = (*d1)->d_name[0];
	c2 = (*d2)->d_name[0];
	if (c1 == c2)
		return ((*d1)->d_name[2] - (*d2)->d_name[2]);
	else if (c1 == 'c')
		return (-1);
	else if (c2 == 'c')
		return (1);
	else
		return (c2 - c1);
}

/*
 * Remove incomplete jobs from spooling area.
 */
cleanpr()
{
	struct direct **queue;
	struct stat s;
	register struct direct **qp;
	register char *lp;
	register int nitems;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	printf("%s:\n", printer);

	(void) strcpy(line, SD);
	lp = line + strlen(SD) + 1;
	lp[-1] = '/';

	nitems = scandir(SD, &queue, sselect, sortq);
	if (nitems < 0) {
		printf("\tcannot examine spool directory\n");
		return;
	}
	for (qp = &queue[0]; nitems != 0; qp++, --nitems) {
		strcpy(lp, (*qp)->d_name);
		if (stat(line, &s) == 0)
			unlinkf(line);
	}
}

unlinkf(name)
	char	*name;
{
	if (unlink(name) < 0)
		printf("\tcannot remove %s\n", name);
	else
		printf("\tremoved %s\n", name);
}

/*
 * Enable queuing to the printer (allow lpr's).
 */
enable(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: enable {all | printer ...}\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else
			enablepr();
	}
}

enablepr()
{
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);

	/*
	 * Turn off the group execute bit of the lock file to enable queuing.
	 */
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, stbuf.st_mode & 0767) < 0)
			printf("\tcannot enable queuing\n");
		else
			printf("\tqueuing enabled\n");
	}
}

/*
 * Disable queuing.
 */
disable(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: disable {all | printer ...}\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else
			disablepr();
	}
}

disablepr()
{
	register int fd;
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);
	/*
	 * Turn on the group execute bit of the lock file to disable queuing.
	 */
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 010) < 0)
			printf("\tcannot disable queuing\n");
		else
			printf("\tqueuing disabled\n");
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0670)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void) close(fd);
			printf("\tqueuing disabled\n");
		}
		return;
	} else
		printf("\tcannot stat lock file\n");
}

/*
 * Disable queuing and printing and put a message into the status file
 * (reason for being down).
 */
down(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: down {all | printer} [message ...]\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : argv[1];
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else
			putmsg(argc - 2, argv + 2);
		if (!do_all)
			break;
	}
}

putmsg(argc, argv)
	int argc;
	char **argv;
{
	register int fd;
	register char *cp1, *cp2;
	char buf[1024];
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((ST = pgetstr("st", &bp)) == NULL)
		ST = DEFSTAT;
	printf("%s:\n", printer);
	/*
	 * Turn on the group execute bit of the lock file to disable queuing and
	 * turn on the owner execute bit of the lock file to disable printing.
	 */
	(void) sprintf(line, "%s/%s", SD, LO);
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 0110) < 0)
			printf("\tcannot disable queuing\n");
		else
			printf("\tprinter and queuing disabled\n");
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0770)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void) close(fd);
			printf("\tprinter and queuing disabled\n");
		}
		return;
	} else
		printf("\tcannot stat lock file\n");
	/*
	 * Write the message into the status file.
	 */
	(void) sprintf(line, "%s/%s", SD, ST);
	fd = open(line, O_WRONLY|O_CREAT, 0664);
	if (fd < 0 || flock(fd, LOCK_EX) < 0) {
		printf("\tcannot create status file\n");
		return;
	}
	(void) ftruncate(fd, (off_t)0);
	if (argc <= 0) {
		(void) write(fd, "\n", 1);
		(void) close(fd);
		return;
	}
	cp1 = buf;
	while (--argc >= 0) {
		cp2 = *argv++;
		while (*cp1++ = *cp2++)
			;
		cp1[-1] = ' ';
	}
	cp1[-1] = '\n';
	*cp1 = '\0';
	(void) write(fd, buf, strlen(buf));
	(void) close(fd);
}

/*
 * Exit lpc
 */
quit(argc, argv)
	int argc;
	char *argv[];
{
	exit(0);
}

/*
 * Kill and restart the daemon.
 */
restart(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: restart {all | printer ...}\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else {
			abortpr(0);
			startpr(0);
		}
	}
}

/*
 * Enable printing on the specified printer and startup the daemon.
 */
start(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: start {all | printer ...}\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else
			startpr(1);
	}
}

startpr(enable)
	int enable;
{
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);

	/*
	 * Turn off the owner execute bit of the lock file to enable printing.
	 */
	if (enable && stat(line, &stbuf) >= 0) {
		if (chmod(line, stbuf.st_mode & (enable==2 ? 0666 : 0677)) < 0)
			printf("\tcannot enable printing\n");
		else
			printf("\tprinting enabled\n");
	}
	if (!startdaemon(printer))
		printf("\tcouldn't start daemon\n");
	else
		printf("\tdaemon started\n");
}

/*
 * Print the status of each queue listed or all the queues.
 */
status(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	do_all = (argc == 1) || (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else
			prstat();
	}
}

/*
 * Print the status of the printer queue.
 */
prstat()
{
	struct stat stbuf;
	register int fd, i;
	register struct direct *dp;
	DIR *dirp;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((ST = pgetstr("st", &bp)) == NULL)
		ST = DEFSTAT;
	printf("%s:\n", printer);
	(void) sprintf(line, "%s/%s", SD, LO);
	if (stat(line, &stbuf) >= 0) {
		printf("\tqueuing is %s\n",
			(stbuf.st_mode & 010) ? "disabled" : "enabled");
		printf("\tprinting is %s\n",
			(stbuf.st_mode & 0100) ? "disabled" : "enabled");
	} else {
		printf("\tqueuing is enabled\n");
		printf("\tprinting is enabled\n");
	}
	if ((dirp = opendir(SD)) == NULL) {
		printf("\tcannot examine spool directory\n");
		return;
	}
	i = 0;
	while ((dp = readdir(dirp)) != NULL) {
		if (*dp->d_name == 'c' && dp->d_name[1] == 'f')
			i++;
	}
	closedir(dirp);
	if (i == 0)
		printf("\tno entries\n");
	else if (i == 1)
		printf("\t1 entry in spool area\n");
	else
		printf("\t%d entries in spool area\n", i);
	fd = open(line, O_RDONLY);
	if (fd < 0 || flock(fd, LOCK_SH|LOCK_NB) == 0) {
		(void) close(fd);	/* unlocks as well */
		printf("\tno daemon present\n");
		return;
	}
	(void) close(fd);
	putchar('\t');
	(void) sprintf(line, "%s/%s", SD, ST);
	fd = open(line, O_RDONLY);
	if (fd >= 0) {
		(void) flock(fd, LOCK_SH);
		while ((i = read(fd, line, sizeof(line))) > 0)
			(void) fwrite(line, 1, i, stdout);
		(void) close(fd);	/* unlocks as well */
	}
}

/*
 * Stop the specified daemon after completing the current job and disable
 * printing.
 */
stop(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: stop {all | printer ...}\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else
			stoppr();
	}
}

stoppr()
{
	register int fd;
	struct stat stbuf;

	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	(void) sprintf(line, "%s/%s", SD, LO);
	printf("%s:\n", printer);

	/*
	 * Turn on the owner execute bit of the lock file to disable printing.
	 */
	if (stat(line, &stbuf) >= 0) {
		if (chmod(line, (stbuf.st_mode & 0777) | 0100) < 0)
			printf("\tcannot disable printing\n");
		else
			printf("\tprinting disabled\n");
	} else if (errno == ENOENT) {
		if ((fd = open(line, O_WRONLY|O_CREAT, 0760)) < 0)
			printf("\tcannot create lock file\n");
		else {
			(void) close(fd);
			printf("\tprinting disabled\n");
		}
	} else
		printf("\tcannot stat lock file\n");
}

struct	queue **queue;
int	nitems;
time_t	mtime;

/*
 * Put the specified jobs at the top of printer queue.
 */
topq(argc, argv)
	int argc;
	char *argv[];
{
	register int n, i;
	struct stat stbuf;
	register char *cfname;
	int status, changed;

	if (argc < 3) {
		printf("Usage: topq printer [jobnum ...] [user ...]\n");
		return;
	}

	--argc;
	printer = *++argv;
	status = pgetent(line, printer);
	if (status < 0) {
		printf("cannot open printer description file\n");
		return;
	} else if (status == 0) {
		printf("%s: unknown printer\n", printer);
		return;
	}
	bp = pbuf;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	printf("%s:\n", printer);

	if (chdir(SD) < 0) {
		printf("\tcannot chdir to %s\n", SD);
		return;
	}
	nitems = getq(&queue);
	if (nitems == 0)
		return;
	changed = 0;
	mtime = queue[0]->q_time;
	for (i = argc; --i; ) {
		if (doarg(argv[i]) == 0) {
			printf("\tjob %s is not in the queue\n", argv[i]);
			continue;
		} else
			changed++;
	}
	for (i = 0; i < nitems; i++)
		free(queue[i]);
	free(queue);
	if (!changed) {
		printf("\tqueue order unchanged\n");
		return;
	}
	/*
	 * Turn on the public execute bit of the lock file to
	 * get lpd to rebuild the queue after the current job.
	 */
	if (changed && stat(LO, &stbuf) >= 0)
		(void) chmod(LO, (stbuf.st_mode & 0777) | 01);
} 

/*
 * Reposition the job by changing the modification time of
 * the control file.
 */
touch(q)
	struct queue *q;
{
	struct timeval tvp[2];

	tvp[0].tv_sec = tvp[1].tv_sec = --mtime;
	tvp[0].tv_usec = tvp[1].tv_usec = 0;
	return (utimes(q->q_name, tvp));
}

/*
 * Checks if specified job name is in the printer's queue.
 * Returns:  negative (-1) if argument name is not in the queue.
 */
doarg(job)
	char *job;
{
	register struct queue **qq;
	register int jobnum, n;
	register char *cp, *machine;
	int cnt = 0;
	FILE *fp;

	/*
	 * Look for a job item consisting of system name, colon, number 
	 * (example: ucbarpa:114)  
	 */
	if ((cp = index(job, ':')) != NULL) {
		machine = job;
		*cp++ = '\0';
		job = cp;
	} else
		machine = NULL;

	/*
	 * Check for job specified by number (example: 112 or 235ucbarpa).
	 */
	if (isdigit(*job)) {
		jobnum = 0;
		do
			jobnum = jobnum * 10 + (*job++ - '0');
		while (isdigit(*job));
		for (qq = queue + nitems; --qq >= queue; ) {
			n = 0;
			for (cp = (*qq)->q_name+3; isdigit(*cp); )
				n = n * 10 + (*cp++ - '0');
			if (jobnum != n)
				continue;
			if (*job && strcmp(job, cp) != 0)
				continue;
			if (machine != NULL && strcmp(machine, cp) != 0)
				continue;
			if (touch(*qq) == 0) {
				printf("\tmoved %s\n", (*qq)->q_name);
				cnt++;
			}
		}
		return (cnt);
	}
	/*
	 * Process item consisting of owner's name (example: henry).
	 */
	for (qq = queue + nitems; --qq >= queue; ) {
		if ((fp = fopen((*qq)->q_name, "r")) == NULL)
			continue;
		while (getline(fp) > 0)
			if (line[0] == 'P')
				break;
		(void) fclose(fp);
		if (line[0] != 'P' || strcmp(job, line+1) != 0)
			continue;
		if (touch(*qq) == 0) {
			printf("\tmoved %s\n", (*qq)->q_name);
			cnt++;
		}
	}
	return (cnt);
}

/*
 * Enable everything and start printer (undo `down').
 */
up(argc, argv)
	int argc;
	char *argv[];
{
	register int do_all, status;

	if (argc == 1) {
		printf("Usage: up {all | printer ...}\n");
		return;
	}

	do_all = (argc == 2 && strcmp(argv[1], "all") == 0);
	for (;;) {
		printer = do_all ? next_printer() : *++argv;
		if (printer == NULL)
			break;
		status = pgetent(line, printer);
		if (status < 0) 
			printf("cannot open printer description file\n");
		else if (status == 0) 
			printf("unknown printer %s\n", printer);
		else
			startpr(2);
	}
}

#define PRBUF_SIZE	200

static char *
next_printer()
{
	static char prbuf[PRBUF_SIZE];
	register int namelen;
	register char *p = NULL;

	if (getprent(line) > 0) {
		namelen = strcspn(line, "|:");
		if (namelen >= PRBUF_SIZE)
			printf("%s: name too long\n", line);
		else {
			(void)strncpy(prbuf, line, namelen);
			prbuf[namelen] = '\0';
			p = prbuf;
		}
	}
	return p;
}
