#ifndef lint
static	char sccsid[] = "@(#)score.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

#include <stdio.h>
#include <sys/file.h>
#include <pwd.h>
#include "score.h"
#include "defs.h"

getscore(name, humanp, computerp)
	char *name;
	int *humanp;
	int *computerp;
{
	struct passwd *pwd;
	struct scorerec srec;
	int uid;
	FILE *score;

	uid = getuid();
	pwd = getpwuid(uid);
	if (pwd == NULL) {
		strcpy(name, "human");
	} else {
		strncpy(name, pwd->pw_name, MAXNAME);
	}
	*humanp = 0;
	*computerp = 0;

	score = fopen(SCOREFILE, "r");
	if (score == NULL) {
		return;
	}

	while (fread(&srec, sizeof (srec), 1, score) != NULL) {
		if (srec.uid != uid) {
			continue;
		}
		*humanp = srec.human;
		*computerp = srec.computer;
		break;
	}

	fclose(score);
}

savescore(human, computer)
	int human;
	int computer;
{
	int uid;
	struct scorerec srecs[1000];
	struct scorerec *sp;
	int nbytes;
	int nrecs;
	int score;

	uid = getuid();
	score = open(SCOREFILE, O_CREAT|O_RDWR, 0644);
	if (score < 0) {
		message(ERR, "can't open score file");
		return;
	}

	while ((nbytes = read(score, srecs, sizeof (srecs))) > 0) {
		if (nbytes % sizeof (struct scorerec)) {
			message(ERR, "Corrupted score file");
			close(score);
			return;
		}
		nrecs = nbytes / sizeof (struct scorerec);
		for (sp = srecs; sp < &srecs[nrecs] && sp->uid != uid; sp++)
			;
		if (sp->uid == uid) {
			break;
		}
	}

	if (nbytes < 0) {
		message(ERR, "can't read score file");
		close(score);
		return;
	}

	if (nbytes > 0 && sp < &srecs[nrecs]) {
		if (lseek(score, (int)sp - (int)&srecs[nrecs], 1) < 0) {
			message(ERR, "can't seek in score file");
			close(score);
			return;
		}
	} else {
		sp = srecs;
		if (lseek(score, 0, 2) < 0) {
			message(ERR, "can't seek in score file");
			close(score);
			return;
		}
	}

	sp->human = human;
	sp->computer = computer;
	sp->uid = uid;

	if (write(score, sp, sizeof(struct scorerec))
	    != sizeof(struct scorerec)) {
		message(ERR, "can't write score file");
	}

	close(score);
}
