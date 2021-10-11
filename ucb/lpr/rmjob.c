/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)rmjob.c 1.1 92/07/30 SMI"; /* from UCB 5.1 6/6/85 */
#endif not lint

/*
 * rmjob - remove the specified jobs from the queue.
 */

#include "lp.h"

/*
 * Stuff for handling lprm specifications
 */
extern char	*user[];		/* users to process */
extern int	users;			/* # of users in user array */
extern int	requ[];			/* job number of spool entries */
extern int	requests;		/* # of spool requests */
extern char	*person;		/* name of person doing lprm */

static char	root[] = "root";
static int	all = 0;		/* eliminate all files (root only) */
static int	cur_daemon;		/* daemon's pid */
static char	current[MAXFILENAMELEN]; /* active control file name */

int	iscf();
void	exit();
void	sleep();

rmjob()
{
	register int i, nitems;
	int assasinated = 0;
	struct direct **files;

	if ((i = pgetent(line, printer)) < 0)
		fatal("cannot open printer description file");
	else if (i == 0)
		fatal("unknown printer");
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((LP = pgetstr("lp", &bp)) == NULL)
		LP = DEFDEVLP;
	if ((RP = pgetstr("rp", &bp)) == NULL)
		RP = DEFLP;
	RM = pgetstr("rm", &bp);
	if (RM != NULL && *RM == '\0')
		RM = NULL;

	/*
	 * If the format was `lprm -' and the user isn't the super-user,
	 *  then fake things to look like he said `lprm user'.
	 */
	if (users < 0) {
		if (getuid() == 0)
			all = 1;	/* all files in local queue */
		else {
			user[0] = person;
			users = 1;
		}
	}
	if (!strcmp(person, "-all")) {
		if (from == host)
			fatal("The login name \"-all\" is reserved");
		all = 1;	/* all those from 'from' */
		person = root;
	}

	if (chdir(SD) < 0)
		fatal("cannot chdir to spool directory");
	if ((nitems = scandir(".", &files, iscf, (int (*)())NULL)) < 0)
		fatal("cannot access spool directory");

	if (nitems) {
		/*
		 * Check for an active printer daemon (in which case we
		 *  kill it if it is reading our file) then remove stuff
		 *  (after which we have to restart the daemon).
		 */
		if (lockchk(LO) && chk(current)) {
			assasinated = kill(cur_daemon, SIGINT) == 0;
			if (!assasinated)
				fatal("cannot kill printer daemon");
		}
		/*
		 * process the files
		 */
		for (i = 0; i < nitems; i++)
			process(files[i]->d_name);
	}
	chkremote();
	/*
	 * Restart the printer daemon if it was killed,
	 * but first wait until the daemon is really dead.
	 */
	if (assasinated) {
		int lfd;    /* lock file descriptor */

		lfd = open(LO, O_WRONLY|O_CREAT, 0644);
		if (lfd < 0)
			fatal("cannot create %s", LO);

		/* when lock succeeds it's ok to restart the daemon */
		if (flock(lfd, LOCK_EX) < 0)
			fatal("cannot lock %s", LO);
		(void) close(lfd);	/* implicit unlock */
	}
	if (assasinated && !startdaemon(printer))
		fatal("cannot restart printer daemon\n");
	/* don't exit if called from cancel */
	if (strcmp(name, "cancel"))
		exit(0);
}

/*
 * Process a lock file: collect the pid of the active
 *  daemon and the file name of the active spool entry.
 * Return boolean indicating existence of a lock file.
 */
static
lockchk(s)
	char *s;
{
	register FILE *fp;
	register int n;
	register unsigned i;

	if ((fp = fopen(s, "r")) == NULL)
		if (errno == EACCES)
			fatal("can't access lock file");
		else
			return 0;
	if (!getline(fp)) {
		(void)fclose(fp);
		return 0;		/* no daemon present */
	}
	cur_daemon = atoi(line);
	if (kill(cur_daemon, 0) < 0) {
		(void)fclose(fp);
		return 0;		/* no daemon present */
	}
	for (i = 1; i <= 5; i++) {
		n = fread(current, sizeof (char), sizeof current, fp);
		if (n > 0)
			break;
		sleep(i);
	}
	(void)fclose(fp);
	if (n > 0) 
		current[n - 1] = '\0';
	return (n > 0);
}

/*
 * Process a control file.
 */
static
process(file)
	char *file;
{
	FILE *cfp;

	if (!chk(file))
		return;
	if ((cfp = fopen(file, "r")) == NULL)
		fatal("cannot open %s", file);
	while (getline(cfp)) {
		switch (line[0]) {
		case 'U':  /* unlink associated files */
			if (from != host)
				printf("%s: ", host);
			printf(unlink(line+1) ? "cannot dequeue %s\n" :
				"%s dequeued\n", line+1);
		}
	}
	(void) fclose(cfp);
	if (from != host)
		printf("%s: ", host);
	printf(unlink(file) ? "cannot dequeue %s\n" : "%s dequeued\n", file);
}

/*
 * Do the dirty work in checking
 */
static
chk(file)
	char *file;
{
	register int *r, n;
	register char **u, *cp;
	FILE *cfp;

	/*
	 * Check for valid cf file name (mostly checking current).
	 */
	if (strlen(file) < 7 || file[0] != 'c' || file[1] != 'f')
		return 0;

	if (all && (from == host || !strcmp(from, file+6)))
		return 1;

	/*
	 * get the owner's name from the control file.
	 */
	if ((cfp = fopen(file, "r")) == NULL)
		return 0;
	while (getline(cfp)) {
		if (line[0] == 'P')
			break;
	}
	(void) fclose(cfp);
	if (line[0] != 'P')
		return 0;

	if (users == 0 && requests == 0)
		return (!strcmp(file, current) && isowner(line+1, file));
	/*
	 * Check the request list
	 */
	for (n = 0, cp = file+3; isdigit(*cp); )
		n = n * 10 + (*cp++ - '0');
	for (r = requ; r < &requ[requests]; r++)
		if (*r == n && isowner(line + 1, file))
			return 1;
	/*
	 * Check to see if it's in the user list
	 */
	for (u = user; u < &user[users]; u++)
		if (!strcmp(*u, line + 1) && isowner(line + 1, file))
			return 1;
	return 0;
}

/*
 * If root is removing a file on the local machine, allow it.
 * If root is removing a file from a remote machine, only allow
 * files sent from the remote machine to be removed.
 * Normal users can only remove the file from where it was sent.
 */
static
isowner(owner, file)
	char *owner, *file;
{
	if (!strcmp(person, root) && (from == host || !strcmp(from, file+6)))
		return 1;
	if (!strcmp(person, owner) && !strcmp(from, file+6))
		return 1;
	if (from != host)
		printf("%s: ", host);
	printf("%s: Permission denied\n", file);
	return 0;
}

/*
 * Check to see if we are sending files to a remote machine. If we are,
 * then try removing files on the remote machine.
 */
static
chkremote()
{
	register char *cp;
	register int i, rem;
	char buf[BUFSIZ];

	if (*LP || RM == NULL)
		return;	/* not sending to a remote machine */

	/*
	 * Flush stdout so the user can see what has been deleted
	 * while we wait (possibly) for the connection.
	 */
	fflush(stdout);

	sprintf(buf, "\5%s %s", RP, all ? "-all" : person);
	cp = buf;
	for (i = 0; i < users; i++) {
		cp += strlen(cp);
		*cp++ = ' ';
		strcpy(cp, user[i]);
	}
	for (i = 0; i < requests; i++) {
		cp += strlen(cp);
		(void) sprintf(cp, " %d", requ[i]);
	}
	strcat(cp, "\n");
	rem = getport(RM);
	if (rem < 0) {
		if (from != host)
			printf("%s: ", host);
		printf("connection to %s is down\n", RM);
	} else {
		i = strlen(buf);
		if (write(rem, buf, i) != i)
			fatal("Lost connection");
		while ((i = read(rem, buf, sizeof buf)) > 0)
			(void) fwrite(buf, 1, i, stdout);
		(void) close(rem);
	}
}

/*
 * Return 1 if the filename begins with 'cf'
 */
static
iscf(d)
	struct direct *d;
{
	return (d->d_name[0] == 'c' && d->d_name[1] == 'f');
}
