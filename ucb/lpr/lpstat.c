/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)lpstat.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * lpstat -- print LP status information
 *
 * lpstat prints information about the current status of the
 * LP line printer system
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <grp.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>
#include "lp.h"

#define SEP " -"

char *user[MAXUSERS];				/* users to process */
int  users;					/* # of users in user array */
int  requ[MAXREQUESTS];				/* job nbr of spool entries */
int  requests;					/* nbr of entries in requ */
char  *printers[MAXREQUESTS];			/* printers to process */
int  numprint;					/* nbr of printers in array */
char *person;
char *printer;
char *getlogin();
int  userid;
char argbuf[1024];

main(argc, argv)

	int  argc;
	char **argv;
{
	int optsw;
	extern struct passwd *getpwnam();
	extern struct passwd *getpwuid();
	struct passwd *pw;
	register char *arg;
	char **av;
	int ac;

	name = argv[0];
	gethostname(host, sizeof host);
	openlog("lpd", 0, LOG_LPR);

	/*
	 * No arg specified, print status of all jobs belonging to
	 * the caller and queued on the default printer
	 */
	if (argc == 1) {
		if (((person = getlogin()) == NULL) ||
				((pw = getpwnam(person)) == NULL)) {
			userid = getuid();
			if ((pw = getpwuid(userid)) == NULL)
				fatal("Who are you?");
			person = pw->pw_name;
		}
		printer = getenv("LPDEST");
		if (printer == NULL)
			printer = getenv("PRINTER");
		if (printer == NULL)
			printer = DEFLP;
		user[users++] = person;
		displayq(0);
		exit(0);
	}

	av = (char **)malloc((argc + 2) * sizeof (char *));
	for (ac = 0; ac <argc; ac++)
		av[ac] = argv[ac];
	av[ac++] = "--";

	while (ac > 1 && av[1][0] == '-') {
		ac--;
		arg = *++av;
		switch (arg[1]) {
		case 'a':
		case 'p':
			getargs(av);
			prtstat(argbuf);
			break;
		case 'c':
			class(argbuf);
			break;
		case 'd':
			defprinter();
			break;
		case 'o':
			getargs(av);
			outstat(argbuf);
			break;
		case 'r':
			lpdstat();
			break;
		case 's':
			lpdstat();
			defprinter();
			class();
			getargs(av);
			printdev(argbuf);
			break;
		case 't':
			lpdstat();
			defprinter();
			class();
			getargs(av);
			printdev(argbuf);
			prtstat(argbuf);
			outstat(argbuf);
			break;
		case 'u':
			getargs(av);
			userstat(argbuf);
			break;
		case 'v':
			getargs(av);
			printdev(argbuf);
			break;
		case '-':
			break;
		default:
			fprintf(stderr,
	"Usage:lpstat [-a][list] [-c][list] [-d] [-o][list] [-p][list]\n");
			fprintf(stderr,
	"      [-r] [-s] [-t] [-u][list] [-v][list]\n");
		}
	}

	exit(0);
	/* NOTREACHED */
}


/*
 * Not supported
 */
class()
{
	printf("lpstat: printer classes not supported\n\n");
}


/*
 * Print basic info about the default printer
 */
defprinter()
{
	int ret;
	char buf[1024];
	char *c = buf;

	printer = getenv("LPDEST");
	if (printer == NULL)
		printer = getenv("PRINTER");
	if (printer == NULL)
		printer = DEFLP;

	if ((ret = pgetent(line, printer)) <0)
		fprintf(stderr, "lpstat: Cannot open /etc/printcap\n");
	else if (ret == 0) {
		fprintf(stderr,
	"default printer is %s, no corresponding entry in /etc/printcap\n",
			printer);
	} else {
		LP = pgetstr("lp", &c);
		if (LP != NULL && *LP != '\0')
			printf("the default printer is %s\n", LP);
		else {
			RM = pgetstr("rm", &c);
			RP = pgetstr("rp", &c);
			if (RP != NULL && *RP != '\0') {
				printf("the default printer is %s", RP);
				if (RM != NULL && *RM != '\0')
					printf(" on remote host %s", RM);
				putchar('\n');
			}
			else if (RM != NULL && *RM != '\0')
				printf("the default printer is on remote host %s\n", RM);
			else
				printf("no default printer specified\n");
		}
	}
	printf("\n");
}


/*
 * Print the status of the lpd daemon (running or not)
 */
lpdstat()
{
	FILE *lfp;
	int daemon, err;

	if ((lfp = fopen(MASTERLOCK, "r")) == NULL)
		fatal("Could not open master lock file\n");
	if (!getline(lfp)) {
		(void) fclose(lfp);
		printf("No local scheduler present\n");
		return (0);
	}
	daemon = atoi(line);
	if (kill(daemon, 0) < 0) {
		err = errno;
		(void) fclose(lfp);
		if (err == ESRCH)
			printf("No local scheduler present\n");
		return (0);
	}
	else
		printf("Local scheduler running\n");
	printf("\n");

}


/*
 * Print the device associated with the argument, or all printers
 */
printdev(arg)
  char *arg;
  {
	int all=0;
	int status;
	char *getprnam(), prtbuf[32];
	char *printer = prtbuf;
	char *getnext();

	if (!strcmp(arg, SEP)) {
		while (getprent(line) != 0) {
			if (getprnam(prtbuf, line) == NULL)
				fatal("No printer name specified");
			getprinfo(printer);
		}
		endprent();
	}
	else {
		while ((printer = getnext(&arg)) != 0) {
			if ((status = pgetent(line, printer)) <0)
				printf("lpstat: Cannot open printer description file\n");
			else if (status == 0)
				printf("lpstat: unknown printer %s\n", printer);
			else
				getprinfo(printer);
		}
	}
	printf("\n");
}


/*
 * get device name and the remote machine associated with printer
 */
getprinfo(printer)
char *printer;
{
	char buf[1024];
	char *c = buf;

	LP = pgetstr("lp", &c);
	RM = pgetstr("rm", &c);
	RP = pgetstr("rp", &c);
	if (LP != NULL && *LP != '\0')
		printf("device for %s is %s\n", printer, LP);
	else if (RP != NULL && *RP != '\0') {
		printf("device for %s is the remote printer %s", printer, RP);
		if (RM != NULL && *RM != '\0')
			printf(" on %s", RM);
		putchar('\n');
	} else if (RM != NULL && *RM != '\0')
		printf("device for %s is on  %s\n", printer, RM);
	else
		printf("No device or machine is	specified for %s\n", printer);
}



/*
 * Print status of jobs of the users specified
 */
userstat(arg)
char *arg;
{
	char *userarg, *getnext();

	if (!strcmp(arg, SEP)) {
		users =0;
		requests = 0;
	}
	else {
		while ((userarg = getnext(&arg)) != NULL) {
			if (users >= MAXUSERS)
				fatal("Too many users");
			user[users++] = userarg;
		}
	}

	printer = getenv("LPDEST");
	if (printer == NULL)
		printer = getenv("PRINTER");
	if (printer == NULL)
		printer = DEFLP;

	displayq(0);
	putchar('\n');
}


/*
 * Print status of the printers of jobs specified
 */
outstat(arg)
char *arg;
{
	char *c, *getnext();
	char prtbuf[32];

	if (!strcmp(arg, SEP)) {
		while (getprent(line) != 0) {
			if (getprnam(prtbuf, line) == NULL)
				fatal("No printer name specified");
			users = 0;
			requests = 0;
			printer = prtbuf;
			printf("\n%s:\n", printer);
			displayq(0);
		}
		endprent();
	}
	else {
		while ((c = getnext(&arg)) != NULL) {
			if (isdigit(*c)) {
				if (requests >= MAXREQUESTS)
					fatal("Too many requests");
				requ[requests++] = atoi(c);
			}
			else {
				if (numprint >= MAXREQUESTS)
					fatal("Too many printers");
				printers[numprint++] = c;
			}
		}

		if (requests > 0) {
			printer = getenv("LPDEST");
			if (printer == NULL)
				printer = getenv("PRINTER");
			if (printer == NULL)
				printer = DEFLP;
			printf("%s:\n", printer);
			displayq(0);
		}
		users = 0;
		requests = 0;
		while (numprint > 0) {
			printer = printers[--numprint];
			printf("\n%s:\n", printer);
			displayq(0);
		}
	}
	printf("\n");
}



/*
 * Print the status of each queue listed or all the queues.
 */
prtstat(arg)
	char *arg;
{
	register int c, status;
	register char *cp1, *cp2;
	char prbuf[100];

	if (!strcmp(arg, SEP)) {
		printer = prbuf;
		while (getprent(line) > 0) {
			cp1 = prbuf;
			cp2 = line;
			while ((c = *cp2++) && c != '|' && c != ':')
				*cp1++ = c;
			*cp1 = '\0';
			prstat();
		}
		return;
	}
	while ((printer = getnext(&arg)) != NULL) {
		if ((status = pgetent(line, printer)) < 0) {
			printf("cannot open printer description file\n");
			continue;
		} else if (status == 0) {
			printf("unknown printer %s\n", printer);
			continue;
		}
		prstat();
	}
	printf("\n");
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
		while ((i = read(fd, line, sizeof line)) > 0)
			(void) fwrite(line, 1, i, stdout);
		(void) close(fd);	/* unlocks as well */
	}
}


/*
 * Put all arguments to an option in the argbuf
 */
getargs(arg)
char **arg;
{
	argbuf[0] = '\0';
	if (arg[0][2])
		strcpy(argbuf, (arg[0]+2));
	while (**++arg != '-') {
		strcat(argbuf, " ");
		strcat(argbuf, *arg);
	}
	strcat(argbuf, SEP);
}


/*
 * Extract the most common name used for the printer from the printcap
 * entry
 */
char *getprnam(ptr, line)
char *ptr, *line;
{
	char *c;

	c = line;
	while ((*c != '\0') && (*c != '|') && (*c != ':')) c++;
	if (*c == '\0')
		fatal("lp: No printer name in /etc/printcap entry");
	else if (*c == ':') {
		c = line;
		while (*c != ':') *ptr++ = *c++;
		*ptr = '\0';
		return (ptr);
	}
	else {
		c++;
		while ((*c != '\0') && (*c != '|') && (*c != ':'))
			*ptr++ = *c++;
		*ptr = '\0';
		return (ptr);
	}
}



/*
 * Get the next argument to the current option
 */
char *getnext(arg)
char **arg;
{
	char *c, *c2;

	c = *arg;
	if (c == NULL || *c == '\0' || *c == '-')
		return (NULL);
	if (*c == '"') 
		c++;
	c += strspn(c, " \t,");
	if (*c == '\0' || *c == '"' || *c == '-')
		return (NULL);

	c2 = c + strspn(c, " ,\"");
	if (*c2 == '\0') {
		*arg = NULL;
		return (c);
	}
	else {
		*c2 = '\0';
		*arg = ++c2;
		return (c);
	}
}
