#ifndef lint
static	char sccsid[] = "@(#)gammonscore.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

#include <stdio.h>
#include <pwd.h>
#include <sys/file.h>
#include "score.h"

#define	NSCORES 1000
#define	UNUSED	-99
#define	MAXNAME	16

struct scores {
	char name[MAXNAME];
	int  spread;
	int  human;
	int  computer;
} scores[NSCORES];

struct scorerec srecs[NSCORES];

main(argc, argv)
	int argc;
	char *argv[];
{
	int scomp();
	int nscores;
	struct scores *sp;
	int nsrecs;

	argc--;
	argv++;

	nsrecs = getscorefile(srecs, NSCORES);
	srecs[nsrecs].uid = UNUSED;

	sp = scores;
	nscores = 0;
	if (argc) {
		while (argc--) {
			if (nametoscore(srecs, *argv, sp) >= 0) {
				argv++;
				sp++;
				nscores++;
			} else {
				fprintf(stdout, "No score for %s\n", *argv);
			}
		}
	} else {
		nscores = srecstoscores(srecs, scores);
	}

	if (nscores) {
		qsort((char *)scores, nscores, sizeof(struct scores), scomp);
		printscores(scores, nscores);
	}
	exit(0);
	/* NOTREACHED */

}

printscores(scores, nscores)
	struct scores *scores;
	int nscores;
{
	int tspread, tcomp, thum;

	tcomp = thum = 0;
	printf("%-*s spread human computer\n", MAXNAME, "name");
	while (nscores--) {
		printf("%-*s %-5d  %-5d %-5d\n", MAXNAME, scores->name,
		    scores->spread, scores->human, scores->computer);
		tcomp += scores->computer;
		thum += scores->human;
		scores++;
	}
	printf("-----------------------------------------\n");
	printf("%-*s %-5d  %-5d %-5d\n", MAXNAME, "total",
	    thum - tcomp, thum, tcomp);
}

getscorefile(srecs, nsrecs)
	struct scorerec *srecs;
{
	int bytes;
	int fd;

	fd = open(SCOREFILE, O_RDONLY, 0);
	if (fd < 0) {
		perror(SCOREFILE);
		exit(1);
	}
	bytes = read(fd, srecs, nsrecs * sizeof(*srecs));
	if (bytes <= 0) {
		perror(SCOREFILE);
		exit(1);
	}
	if (bytes % sizeof(*srecs)) {
		fprintf(stderr, "Corrupted score file: %s\n", SCOREFILE);
		exit(1);
	}
	close(fd);
	return (bytes / sizeof(*srecs));
}

nametoscore(srp, name, sp)
	struct scorerec *srp;
	char *name;
	struct scores *sp;
{
	struct passwd *pwd;
	int uid;

	pwd = getpwnam(name);
	if (pwd == NULL) {
		return (-1);
	}
	uid = pwd->pw_uid;

	while (srp->uid != UNUSED && srp->uid != uid)
		srp++;

	if (srp->uid == uid) {
		strncpy(sp->name, name, MAXNAME);
		sp->spread = srp->human - srp->computer;
		sp->human = srp->human;;
		sp->computer = srp->computer;
		return (0);
	}
	return (-1);
}

srecstoscores(srp, sp)
	struct scorerec *srp;
	struct scores *sp;
{
	struct passwd *pwd;
	int count;

	count = 0;
	while (srp->uid != UNUSED) {
		pwd = getpwuid(srp->uid);
		if (pwd == NULL) {
			sprintf(sp->name, "%d", srp->uid);
		} else {
			strncpy(sp->name, pwd->pw_name, MAXNAME);
		}
		sp->spread = srp->human - srp->computer;
		sp->human = srp->human;
		sp->computer = srp->computer;
		count++;
		sp++;
		srp++;
	}
	return (count);
}

scomp(s1, s2)
	struct scores *s1, *s2;
{
	return (s2->spread - s1->spread);
}
