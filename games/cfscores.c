#ifndef lint
static	char sccsid[] = "@(#)cfscores.c 1.1 92/07/30 SMI"; /* from UCB 5.1 85/05/29 */
#endif

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

#include <pwd.h>

struct betinfo {
	long	hand;		/* cost of dealing hand */
	long	inspection;	/* cost of inspecting hand */
	long	game;		/* cost of buying game */
	long	runs;		/* cost of running through hands */
	long	information;	/* cost of information */
	long	thinktime;	/* cost of thinking time */
	long	wins;		/* total winnings */
	long	worth;		/* net worth after costs */
};

char *scorefile = "/usr/games/lib/cfscores";
int dbfd;

main(argc, argv)
	int argc;
	char *argv[];
{
	register struct passwd *pw;
	int uid;

	if (argc > 2) {
		printf("Usage: cfscores [user]\n");
		exit(1);
	}
	dbfd = open(scorefile, 0);
	if (dbfd < 0) {
		perror(scorefile);
		exit(2);
	}
	setpwent();
	if (argc == 1) {
		uid = getuid();
		pw = getpwuid(uid);
		if (pw == 0) {
			printf("You are not listed in the password file?!?\n");
			exit(2);
		}
		printuser(pw, 1);
		exit(0);
	}
	if (strcmp(argv[1], "-a") == 0) {
		while ((pw = getpwent()) != 0)
			printuser(pw, 0);
		exit(0);
	}
	pw = getpwnam(argv[1]);
	if (pw == 0) {
		printf("User %s unknown\n", argv[1]);
		exit(3);
	}
	printuser(pw, 1);
	exit(0);
	/* NOTREACHED */
}

/*
 * print out info for specified password entry
 */
printuser(pw, printfail)
	register struct passwd *pw;
	int printfail;
{
	struct betinfo total;
	int i;

	if (pw->pw_uid < 0) {
		printf("Bad uid %d\n", pw->pw_uid);
		return;
	}
	i = lseek(dbfd, pw->pw_uid * sizeof(struct betinfo), 0);
	if (i < 0) {
		perror("lseek");
		return;
	}
	i = read(dbfd, (char *)&total, sizeof(total));
	if (i < 0) {
		perror("read");
		return;
	}
	if (i == 0 || total.hand == 0) {
		if (printfail)
			printf("%s has never played canfield.\n", pw->pw_name);
		return;
	}
	printf("*----------------------*\n");
	if (total.worth >= 0)
		printf("* Winnings for %-8s*\n", pw->pw_name);
	else
		printf("* Losses for %-10s*\n", pw->pw_name);
	printf("*======================*\n");
	printf("|Costs           Total |\n");
	printf("| Hands       %8d |\n", total.hand);
	printf("| Inspections %8d |\n", total.inspection);
	printf("| Games       %8d |\n", total.game);
	printf("| Runs        %8d |\n", total.runs);
	printf("| Information %8d |\n", total.information);
	printf("| Think time  %8d |\n", total.thinktime);
	printf("|Total Costs  %8d |\n", total.wins - total.worth);
	printf("|Winnings     %8d |\n", total.wins);
	printf("|Net Worth    %8d |\n", total.worth);
	printf("*----------------------*\n\n");
}
