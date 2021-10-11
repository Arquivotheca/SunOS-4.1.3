#ifndef lint
static	char sccsid[] = "@(#)evalmove.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Determines the value of a move (0-1000)
 */
#include <stdio.h>

extern int	board[26];		/* board:  negative values are white,
				   positive are red */
extern int	p[5];			/* starting position of moves */
extern int	g[5];			/* ending position of moves (goals) */
extern int	cturn;			/* whose turn it currently is:
					-1 = white
					 1 = red
					 0 = just quitted
					-2 = white just lost
					 2 = red just lost */
extern int	bar;			/* position of bar for current player */
extern int	home;			/* position of home for current player */
extern int	*offptr;		/* pointer to off for current player */
extern int	*inptr;			/* pointer to in for current player */
extern int	mvlim;			/* 'move limit':  max. number of moves */
extern int	race;			/* game reduced to a race */
extern FILE *logfd;

#define	bay(x)		((abs(x) - 1) / 6)
#define	baypos(x)	(bay(x) * bay(x))
#define	pos(x)		(abs(x) * abs(x) / 24)

/*
 * returns a number between 0 and 1000 which is the value of this move
 * 1000 is the best move possible.
 */
evalmove()
{
	register int	i, em;
	int val = 0;
	int openmen;	/* number of open men */
	int openpos;	/* open men distance 0 - 100 */
	int chance;	/* chance of getting hit  0 - 100 */
	int points;	/* number of points controlled */
	int pointpos;	/* number of points controlled 0 - 100 */
	int distance;	/* distance from home range 0 - 100 */
	int menoff;	/* men off board */
	int menin;	/* men in home area */
	int barmen;	/* bad guys on the bar */
	int homefilled;	/* number of points held in our home */
	int hisfilled;	/* number of points held in his home */
	int inpos;	/* men in position to build blocked points in home */
	int myblock;	/* points in a row blocked (1-6) */
	int hisblock;	/* points in a row he has blocked (1-6) */

	odds(0, 0);
	chance = openmen = openpos = points = pointpos = 0;
	homefilled = hisfilled = barmen = 0;
	if (!race)  {
		for (i = 1; i < 25; i++)  {
			if (board[i] == cturn) {
				openpos += baypos(bar-i);
				chance = canhit(i,1);
				openmen++;
			}
		}
		if (chance == 0) {
			chance = openmen;
		}
		openpos = openpos * 7;
		for (i = bar+cturn; i != home; i += cturn) {
			if (board[i]*cturn > 1) {
				pointpos += baypos(bar-i);
				points++;
			}
		}
		pointpos = (pointpos * 4) / 3;
		homefilled = filled(home);
		hisfilled = filled(bar);
		barmen = abs(board[home]);
		inpos = readytofill(home);
		myblock = blockage(home);
		hisblock = blockage(bar);
	}
	distance = 0;
	for (em = bar; em != home; em += cturn)  {
		if (board[em]*cturn > 0) {
			distance += pos(home-em) * board[em]*cturn;
		}
	}
	distance = distance / 2;
	menin = *inptr;
	menoff = *offptr;
	if (menoff < 0) {
		menoff = 0;
	}

	if (menoff == 15) {
		/*
		 * all men off, best move!
		 */
		val = 10000;
	} else if (race)  {
		/*
		 * The race is on
		 */
		if (menin + menoff == 15) {
			/*
			 * all men in, go for men off, or distance.
			 */
			val = 1000 + menoff * 200 - distance;
		} else {
			/*
			 * all men not in, move furthest man
			 */
			val = 200 - distance;
		}
	} else  {
		/*
		 * no race
		 */
		if (homefilled == 6 && barmen) {
			/*
			 * he is trapped
			 */
			val = 3000 + menoff * 100 - distance;
		} else {
			val = homefilled * 150
			    + menoff * 50
			    + inpos * 30
			    + pointpos * 4
			    + points * 3
			    + menin
			    + (barmen * homefilled * homefilled * (110 - openpos)) / 2
			    - distance * 2
			    - ((chance * openpos * hisfilled * hisfilled * hisfilled) / 200)
			    + 3 * myblock
			    - hisblock;
		}
	}
	if (logfd != NULL) {
		for (i = 0; i < mvlim; i++)  {
			fprintf(logfd, "%d-%d ", p[i], g[i]);
		}
		fprintf(logfd, "= %d\n", val);
		fprintf(logfd,
"race %d openmen %d openpos %d chance %d points %d pointpos %d inpos %d distance %d\nmenoff %d menin %d, barmen %d homefilled %d hisfilled %d myblock %d hisblock %d\n",
race, openmen, openpos, chance, points, pointpos, inpos, distance, menoff, menin, barmen, homefilled, hisfilled, myblock, hisblock);
		fflush(logfd);
	}
	return(val);
}



