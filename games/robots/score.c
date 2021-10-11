/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)score.c 1.1 92/07/30 SMI"; /* from UCB 5.1 85/05/30 */
#endif not lint

# include	"robots.h"
# include	<pwd.h>

typedef struct {
	int	s_uid;
	int	s_score;
	char	s_name[MAXNAME];
} SCORE;

typedef struct passwd	PASSWD;

char	*Scorefile = SCOREFILE;

int	Max_per_uid = MAX_PER_UID;

static SCORE	Top[MAXSCORES];

void	wperrorw();
#define	perrorw(str)	VOID(wperrorw(stdscr, str))

/*
 * score:
 *	Post the player's score, if reasonable, and then print out the
 *	top list.
 */
score()
{
	register int	inf;
	register SCORE	*scp;
	register int	uid;
	register bool	done_show = FALSE;
	static int	numscores, max_uid;

	Newscore = FALSE;
	if ((inf = open(Scorefile, 2)) < 0) {
		move(1, 15);
		perrorw(Scorefile);
		refresh();
		return;
	}

	if (read(inf, &max_uid, sizeof max_uid) == sizeof max_uid)
		read(inf, Top, sizeof Top);
	else {
		for (scp = Top; scp < &Top[MAXSCORES]; scp++)
			scp->s_score = -1;
		max_uid = Max_per_uid;
	}

	uid = getuid();
	if (Top[MAXSCORES-1].s_score <= Score) {
		numscores = 0;
		for (scp = Top; scp < &Top[MAXSCORES]; scp++)
			if (scp->s_score < 0 ||
			    (scp->s_uid == uid && ++numscores == max_uid)) {
				if (scp->s_score > Score)
					break;
				scp->s_score = Score;
				scp->s_uid = uid;
				set_name(scp);
				Newscore = TRUE;
				break;
			}
		if (scp == &Top[MAXSCORES]) {
			Top[MAXSCORES-1].s_score = Score;
			Top[MAXSCORES-1].s_uid = uid;
			set_name(&Top[MAXSCORES-1]);
			Newscore = TRUE;
		}
		if (Newscore)
			qsort(Top, MAXSCORES, sizeof Top[0], cmp_sc);
	}

	if (!Newscore) {
		Full_clear = FALSE;
		close(inf);
		return;
	}
	else
		Full_clear = TRUE;

	for (scp = Top; scp < &Top[MAXSCORES]; scp++) {
		if (scp->s_score < 0)
			break;
		move((scp - Top) + 1, 15);
		if (!done_show && scp->s_uid == uid && scp->s_score == Score)
			standout();
		printw(" %d\t%d\t%-8.8s ", (scp - Top) + 1, scp->s_score, scp->s_name);
		if (!done_show && scp->s_uid == uid && scp->s_score == Score) {
			standend();
			done_show = TRUE;
		}
	}
	Num_scores = scp - Top;
	refresh();

	if (Newscore) {
		lseek(inf, 0L, 0);
		write(inf, &max_uid, sizeof max_uid);
		write(inf, Top, sizeof Top);
	}
	close(inf);
}

set_name(scp)
register SCORE	*scp;
{
	register PASSWD	*pp;

	if ((pp = getpwuid(scp->s_uid)) == NULL)
		pp->pw_name = "???";
	strncpy(scp->s_name, pp->pw_name, MAXNAME);
}

/*
 * cmp_sc:
 *	Compare two scores.
 */
cmp_sc(s1, s2)
register SCORE	*s1, *s2;
{
	return s2->s_score - s1->s_score;
}

/*
 * show_score:
 *	Show the score list for the '-s' option.
 */
show_score()
{
	register SCORE	*scp;
	register int	inf;
	static int	max_score;

	if ((inf = open(Scorefile, 0)) < 0) {
		perror(Scorefile);
		return;
	}

	for (scp = Top; scp < &Top[MAXSCORES]; scp++)
		scp->s_score = -1;

	read(inf, &max_score, sizeof max_score);
	read(inf, Top, sizeof Top);
	close(inf);
	inf = 1;
	for (scp = Top; scp < &Top[MAXSCORES]; scp++)
		if (scp->s_score >= 0)
			printf("%d\t%d\t%.*s\n", inf++, scp->s_score, sizeof scp->s_name, scp->s_name);
}

extern	int errno;
extern	int sys_nerr;
extern	char *sys_errlist[];

void
wperrorw(win, s)
	register WINDOW *win;
	char *s;
{
	if (s && *s) {
		waddstr(win, s);
		waddstr(win, ": ");
	}
	waddstr(win, (unsigned)errno < sys_nerr ? sys_errlist[errno] :
	    "Unknown error");
}
