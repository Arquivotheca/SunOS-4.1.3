/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)displayq.c 1.1 92/07/30 SMI"; /* from UCB 5.1 6/6/85 */
#endif not lint

/*
 * Routines to display the state of the queue.
 */

#include "lp.h"

#define JOBCOL	40		/* column for job # in -l format */
#define OWNCOL	7		/* start of Owner column in normal */
#define SIZCOL	62		/* start of Size column in normal */

/*
 * Stuff for handling job specifications
 */
extern char	*user[];	/* users to process */
extern int	users;		/* # of users in user array */
extern int	requ[];		/* job number of spool entries */
extern int	requests;	/* # of spool requests */

static int	lflag;		/* long output option */
static char	current[MAXFILENAMELEN]; /* current file being printed */
static int	rank;		/* order to be printed (-1=none, 0=active) */
static long	totsize;	/* total print job size in bytes */
static int	first;		/* first file in ``files'' column? */
static int	col;		/* column on screen */
static int	sendtorem;	/* are we sending to a remote? */
static int	remitems;	/* any items from remote machine? */
static char	file[MAXFILENAMELEN]; /* print file name */

static char	head0[] = "Rank   Owner      Job  Files";
static char	head1[] = "Total Size\n";
static char	noentries[] = "no entries\n";

/*
 * Display the current state of the queue. Format = 1 if long format.
 */
displayq(format)
	int format;
{
	register struct queue *q;
	register int i, nitems, fd;
	struct queue **queue;
	struct stat statb;
	FILE *fp;
	static int gotprintcap = 0;
	static char buf[BUFSIZ];

	lflag = format;
	totsize = 0;
	rank = -1;
	remitems = sendtorem = 0;

	/* don't go to localcheck_done if called from lpstat */
	if (gotprintcap && strcmp(name, "lpstat"))
		goto localcheck_done;

	bp = buf;
	if ((i = pgetent(line, printer)) < 0)
		fatal("cannot open printer description file");
	else if (i == 0)
		fatal("unknown printer");
	if ((LP = pgetstr("lp", &bp)) == NULL)
		LP = DEFDEVLP;
	if ((RP = pgetstr("rp", &bp)) == NULL)
		RP = DEFLP;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((LO = pgetstr("lo", &bp)) == NULL)
		LO = DEFLOCK;
	if ((ST = pgetstr("st", &bp)) == NULL)
		ST = DEFSTAT;
	RM = pgetstr("rm", &bp);
	if (RM != NULL && *RM == '\0')
		RM = NULL;
	gotprintcap = 1;

	/*
	 * Figure out whether the local machine is the same as the remote
	 * machine entry (if it exists).  If not, then ignore the local
	 * queue information.
	 */
	 if (RM != NULL) {
		char hname[MAXHOSTNAMELEN + 1];
		struct hostent *hp;

		/* get the standard network name of the local host */
		gethostname(hname, sizeof hname);
		hname[(sizeof hname) - 1] = '\0';
		hp = gethostbyname(hname);
		if (hp == (struct hostent *)NULL) {
			printf("unable to get network name for local machine %s\n",
			    hname);
			goto localcheck_done;
		}
		strcpy(hname, hp->h_name);

		/* get the network standard name of RM */
		hp = gethostbyname(RM);
		if (hp == (struct hostent *)NULL) {
			printf("unable to get hostname for remote machine %s\n",
			    RM);
			goto localcheck_done;
		}

		/* if printer is not on local machine, ignore LP */
		if (strcmp(hname, hp->h_name) != 0)
			*LP = '\0';
	}
localcheck_done:

	/*
	 * If there is no local printer, then print the queue on
	 * the remote machine and then what's in the queue here.
	 * Note that a file in transit may not show up in either queue.
	 */
	if (*LP == '\0') {
		register char *cp;

		sendtorem++;
		(void) sprintf(line, "%c%s", format + '\3', RP);
		cp = line;
		for (i = 0; i < requests; i++) {
			cp += strlen(cp);
			(void) sprintf(cp, " %d", requ[i]);
		}
		for (i = 0; i < users; i++) {
			cp += strlen(cp);
			*cp++ = ' ';
			strcpy(cp, user[i]);
		}
		strcat(line, "\n");
		fd = getport(RM);
		if (fd < 0) {
			if (from != host)
				printf("%s: ", host);
			printf("connection to %s is down\n", RM);
		} else {
			int first = 1;

			i = strlen(line);
			if (write(fd, line, i) != i)
				fatal("Lost connection");
			while ((i = read(fd, line, sizeof line)) > 0) {
				if (first) {
					first = 0;
					if (strncmp(line, noentries,
					    strlen(noentries)) != 0)
						remitems = 1;
				}
				(void) fwrite(line, 1, i, stdout);
			}
			(void) close(fd);
		}
	}
	/*
	 * Find all the control files in the spooling directory
	 */
	if (chdir(SD) < 0)
		fatal("cannot chdir to spooling directory");
	if ((nitems = getq(&queue)) < 0)
		fatal("cannot examine spooling area\n");
	if (stat(LO, &statb) >= 0) {
		if ((statb.st_mode & 0110) && sendtorem)
			printf("\n");
		if (statb.st_mode & 0100) {
			if (sendtorem)
				printf("%s: ", host);
			printf("Warning: %s is down: ", printer);
			fd = open(ST, O_RDONLY);
			if (fd >= 0) {
				(void) flock(fd, LOCK_SH);
				while ((i = read(fd, line, sizeof line)) > 0)
					(void) fwrite(line, 1, i, stdout);
				(void) close(fd);	/* unlocks as well */
			} else
				putchar('\n');
		}
		if (statb.st_mode & 010) {
			if (sendtorem)
				printf("%s: ", host);
			printf("Warning: %s queue is turned off\n", printer);
		}
	}
	if (nitems == 0) {
		if (!sendtorem)
			printf(noentries);
		return remitems;
	}
	fp = fopen(LO, "r");
	if (fp == NULL)
		warn();
	else {
		register char *cp;

		/* get daemon pid */
		cp = current;
		while ((*cp = getc(fp)) != EOF && *cp != '\n')
			cp++;
		*cp = '\0';
		i = atoi(current);
		if (i <= 0 || kill(i, 0) < 0)
			warn();
		else {
			/* read current file name */
			cp = current;
			while ((*cp = getc(fp)) != EOF && *cp != '\n')
				cp++;
			*cp = '\0';
			/*
			 * Print the status file.
			 */
			if (sendtorem)
				printf("\n%s: ", host);
			fd = open(ST, O_RDONLY);
			if (fd >= 0) {
				(void) flock(fd, LOCK_SH);
				while ((i = read(fd, line, sizeof line)) > 0)
					(void) fwrite(line, 1, i, stdout);
				(void) close(fd);	/* unlocks as well */
			} else
				putchar('\n');
		}
		(void) fclose(fp);
	}
	/*
	 * Now, examine the control files and print out the jobs to
	 * be done for each user.
	 */
	if (!lflag)
		header();
	for (i = 0; i < nitems; i++) {
		q = queue[i];
		inform(q->q_name);
		free((char *)q);
	}
	free((char *)queue);
	return nitems;
}

/*
 * Print a warning message if there is no daemon present.
 */
warn()
{
	if (sendtorem)
		printf("\n%s: ", host);
	printf("Warning: no daemon present\n");
	current[0] = '\0';
}

/*
 * Print the header for the short listing format
 */
static
header()
{
	printf(head0);
	col = strlen(head0)+1;
	blankfill(SIZCOL);
	printf(head1);
}

static
inform(cf)
	char *cf;
{
	register int j;
	FILE *cfp;

	/*
	 * There's a chance the control file has gone away
	 * in the meantime; if this is the case just keep going
	 */
	if ((cfp = fopen(cf, "r")) == NULL)
		return;

	if (rank < 0)
		rank = 0;
	if (sendtorem || strcmp(cf, current))
		rank++;
	j = 0;
	while (getline(cfp)) {
		switch (line[0]) {
		case 'P': /* Was this file specified in the user's list? */
			if (!inlist(line+1, cf)) {
				fclose(cfp);
				return;
			}
			if (lflag) {
				printf("\n%s: ", line+1);
				col = strlen(line+1) + 2;
				prank(rank);
				blankfill(JOBCOL);
				printf(" [job %s]\n", cf+3);
			} else {
				col = 0;
				prank(rank);
				blankfill(OWNCOL);
				printf("%-10s %-3d  ", line+1, atoi(cf+3));
				col += 16;
				first = 1;
			}
			continue;
		default: /* some format specifer and file name? */
			if (line[0] < 'a' || line[0] > 'z')
				continue;
			if (j == 0 || strcmp(file, line+1) != 0)
				strcpy(file, line+1);
			j++;
			continue;
		case 'N':
			show(line+1, file, j);
			file[0] = '\0';
			j = 0;
		}
	}
	fclose(cfp);
	if (!lflag) {
		blankfill(SIZCOL);
		printf("%D bytes\n", totsize);
		totsize = 0;
	}
}

static
inlist(name, file)
	char *name, *file;
{
	register int *r, n;
	register char **u, *cp;

	if (users == 0 && requests == 0)
		return 1;
	/*
	 * Check to see if it's in the user list
	 */
	for (u = user; u < &user[users]; u++)
		if (!strcmp(*u, name))
			return 1;
	/*
	 * Check the request list
	 */
	for (n = 0, cp = file+3; isdigit(*cp); )
		n = n * 10 + (*cp++ - '0');
	for (r = requ; r < &requ[requests]; r++)
		if (*r == n && !strcmp(cp, from))
			return 1;
	return 0;
}

static
show(nfile, file, copies)
	register char *nfile, *file;
{
	if (strcmp(nfile, " ") == 0)
		nfile = "(standard input)";
	if (lflag)
		ldump(nfile, file, copies);
	else
		dump(nfile, file, copies);
}

/*
 * Fill the line with blanks to the specified column
 */
static
blankfill(n)
	register int n;
{
	while (col++ < n)
		putchar(' ');
}

/*
 * Give the abbreviated dump of the file names
 */
static
dump(nfile, file, copies)
	char *nfile, *file;
{
	register short n, fill;
	struct stat lbuf;

	/*
	 * Print as many files as will fit
	 *  (leaving room for the total size)
	 */
	 fill = first ? 0 : 2;	/* fill space for ``, '' */
	 if (((n = strlen(nfile)) + col + fill) >= SIZCOL-4) {
		if (col < SIZCOL) {
			printf(" ..."), col += 4;
			blankfill(SIZCOL);
		}
	} else {
		if (first)
			first = 0;
		else
			printf(", ");
		printf("%s", nfile);
		col += n+fill;
	}
	if (*file && !stat(file, &lbuf))
		totsize += copies * lbuf.st_size;
}

/*
 * Print the long info about the file
 */
static
ldump(nfile, file, copies)
	char *nfile, *file;
{
	struct stat lbuf;

	putchar('\t');
	if (copies > 1)
		printf("%-2d copies of %-19s", copies, nfile);
	else
		printf("%-32s", nfile);
	if (*file && !stat(file, &lbuf))
		printf(" %D bytes", lbuf.st_size);
	else
		printf(" ??? bytes");
	putchar('\n');
}

/*
 * Print the job's rank in the queue,
 *   update col for screen management
 */
static
prank(n)
{
	char line[100];
	static char *r[] = {
		"th", "st", "nd", "rd", "th", "th", "th", "th", "th", "th"
	};

	if (n == 0) {
		printf("active");
		col += 6;
		return;
	}
	if ((n%100)/10 == 1)
		(void) sprintf(line, "%dth", n);
	else
		(void) sprintf(line, "%d%s", n, r[n%10]);
	col += strlen(line);
	printf("%s", line);
}
