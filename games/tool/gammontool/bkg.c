#ifndef lint
static	char sccsid[] = "@(#)bkg.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* modified version of /usr/games/backgammon for use with gammontool */

#include <stdio.h>
#include <sys/file.h>
#include "bkgcodes.h"

#define rnum(r)	(random()%r)
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#define D0	dice[0]
#define D1	dice[1]
#define swap	{D0 ^= D1; D1 ^= D0; D0 ^= D1; d0 = 1-d0;}

/*
 *
 * Some numerical conventions:
 *
 *	Arrays have white's value in [0], red in [1].
 *	Numeric values which are one color or the other use
 *	-1 for white, 1 for red.
 *	Hence, white will be negative values, red positive one.
 *	This makes a lot of sense since white is going in decending
 *	order around the board, and red is ascending.
 *
 */

static int	pnum;			/* color of player:
					-1 = white
					 1 = red
					 0 = both
					 2 = not yet init'ed */
       int	board[26];		/* board:  negative values are white,
				   positive are red */
static int	dice[2];		/* value of dice */
       int	mvlim;			/* 'move limit':  max. number of moves */
static int	mvl;			/* working copy of mvlim */
       int	p[5];			/* starting position of moves */
       int	g[5];			/* ending position of moves (goals) */
static int	h[4];			/* flag for each move if a man was hit */
       int	cturn;			/* whose turn it currently is:
					-1 = white
					 1 = red
					 0 = just quitted
					-2 = white just lost
					 2 = red just lost */
static int	d0;			/* flag if dice have been reversed from
				   original position */
static int	table[6][6];		/* odds table for possible rolls */
static int	gvalue;			/* value of game (64 max.) */
static int	dlast;			/* who doubled last (0 = neither) */
       int	bar;			/* position of bar for current player */
       int	home;			/* position of home for current player */
static int	off[2];			/* number of men off board */
       int	*offptr;		/* pointer to off for current player */
static int	*offopp;		/* pointer to off for opponent */
static int	in[2];			/* number of men in inner table */
       int	*inptr;			/* pointer to in for current player */
static int	*inopp;			/* pointer to in for opponent */

static int	ncin;			/* number of characters in cin */
static char	cin[100];		/* input line of current move
				   (used for reconstructing input after
				   a backspace) */

FILE *logfd = NULL;

bkg()
{
	setbuf(stdout, 0);
	init();				/* initialize board */

	pnum = 1;			/* set player colors */

	switch (getchar()) {		/* get first player */
	case HUMANFIRST:
		cturn = -1;
		break;
	case COMPUTERFIRST:
		cturn = 1;
		break;
	default:
		error("Bad first player sent by gammontool");
		break;
	}

	if (cturn == 1)  {		/* initialize according to first */
		home = 25;		/* player */
		bar = 0;
		inptr = &in[1];
		inopp = &in[0];
		offptr = &off[1];
		offopp = &off[0];
	} else  {
		home = 0;
		bar = 25;
		inptr = &in[0];
		inopp = &in[1];
		offptr = &off[0];
		offopp = &off[1];
	}

	if (logfd == NULL) {
		int fd = open("hitlog", O_TRUNC|O_WRONLY, 0666);

		if (fd >= 0) {
			logfd = fdopen(fd, "w");
		}
	}

	while (cturn == 1 || cturn == -1)  {	/* while nobody has won */
		if (cturn == pnum)  {		/* computer's turn */
			move(1);
						/* check for refused double */
			if (cturn == -2 || cturn == 2)
				break;
						/* check for winning move */
			if (*offopp == 15) {
				cturn *= -2;
				break;
			}
		} else {			/* player's turn */
						/* check for double */
			switch (getchar()) {
			case DOUBLEREQ:
				if (dlast == cturn || gvalue >= 64)
					error("Illegal double sent by gammontool");
				else
					dble();
				break;
			case NODOUBLE:
				break;
			default:
				error("Bad double status sent by gammontool");
				break;
			}
						/* check if he can move */
			switch (getchar()) {
			case CANMOVE:
				roll();
				if ((mvlim = movallow()) == 0) {
					error("Human can't move anywhere");
					nexturn();
				} else {
					getmove();
				}
				break;
			case CANTMOVE:
				nexturn();
				break;
			default:
				error("Bad move status sent by gammontool");
				break;
			}
		}
	} /* end while loop */

	quit();
}

/* send an error message to gammontool and quit */

error(str)
char *str;
{
	putchar(ERRMSG);
	puts(str);

	quit();
}

quit()
{
	putchar(QUIT);
	fflush(stdout);
	pause();	/* wait until we are killed */
	exit(0);
}

getmove()
{
	register int	i, c;

	c = 0;
	for (;;)  {
		i = checkmove(c);

		switch (i)  {
		case -1:
			if (movokay(mvlim)) {
				nexturn();
				if (*offopp == 15)
					cturn *= -2;
				return;
			}
			break;
		case -4:
			error("Not enough moves sent by gammontool");
			break;
		case 0:
			error("Too many moves sent by gammontool");
			break;
		case -3:
			error("Quit sent by gammontool");
			break;
		}
		c = -1;
	}
}

movokay(mv)
register int	mv;
{
	register int	i, m;

	if (d0)
		swap;

	for (i = 0; i < mv; i++)  {

		if (p[i] == g[i])  {
			error("Move to the same location sent by gammontool");
			return(0);
		}

		if (cturn*(g[i]-p[i]) < 0)  {
			error("Backwards move sent by gammontool");
			return(0);
		}

		if (abs(board[bar]) && p[i] != bar)  {
			error("Bar men not moved first by gammontool");
			return(0);
		}

		if (m = makmove(i))  {
			switch (m)  {

			case 1:
				error("Unrolled move sent by gammontool");
				break;

			case 2:
				error("Bad starting position sent by gammontool");
				break;

			case 3:
				error("Blocked destination sent by gammontool");
				break;

			case 4:
				error("Premature bearing off sent by gammontool");
				break;
			}
			return(0);
		}
	}
	return (1);
}

struct state	{
	char	ch;
	int	fcode;
	int	newst;
};

static struct state	atmata[] = {

	'R', 1, 0,	'?', 7, 0,	'Q', 0, -3,	'B', 8, 25,
	'9', 2, 25,	'8', 2, 25,	'7', 2, 25,	'6', 2, 25,
	'5', 2, 25,	'4', 2, 25,	'3', 2, 25,	'2', 2, 19,
	'1', 2, 15,	'0', 2, 25,	'.', 0, 0,	'9', 2, 25,
	'8', 2, 25,	'7', 2, 25,	'6', 2, 25,	'5', 2, 25,

	'4', 2, 25,	'3', 2, 25,	'2', 2, 25,	'1', 2, 25,
	'0', 2, 25,	'/', 0, 32,	'-', 0, 39,	'.', 0, 0,
	'/', 5, 32,	' ', 6, 3,	',', 6, 3,	'\n', 0, -1,
	'6', 3, 28,	'5', 3, 28,	'4', 3, 28,	'3', 3, 28,
	'2', 3, 28,	'1', 3, 28,	'.', 0, 0,	'H', 9, 61,

	'9', 4, 61,	'8', 4, 61,	'7', 4, 61,	'6', 4, 61,
	'5', 4, 61,	'4', 4, 61,	'3', 4, 61,	'2', 4, 53,
	'1', 4, 51,	'0', 4, 61,	'.', 0, 0,	'9', 4, 61,
	'8', 4, 61,	'7', 4, 61,	'6', 4, 61,	'5', 4, 61,
	'4', 4, 61,	'3', 4, 61,	'2', 4, 61,	'1', 4, 61,

	'0', 4, 61,	' ', 6, 3,	',', 6, 3,	'-', 5, 39,
	'\n', 0, -1,	'.', 0, 0
};

static
checkmove (ist)
int	ist;
{
	register int	j, n;
	register char	c;

domove:
	ist = mvl = ncin = 0;
	for (j = 0; j < 5; j++)
		p[j] = g[j] = -1;

dochar:
	c = getchar();

	n = dotable(c,ist);
	if (n >= 0)  {
		cin[ncin++] = c;
		ist = n;
		if (n)
			goto dochar;
		else
			goto domove;
	}

	if (n == -1 && mvl >= mvlim)
		return(0);
	if (n == -1 && mvl < mvlim-1)
		return(-4);

	if (n == -6)  {
		if (movokay(mvl+1))
			movback(mvl+1);
		ist = n = rsetbrd();
		goto dochar;
	}

	if (n != -5)
		return(n);
	error("Illegal character sent by gammontool");
	goto dochar;
}

dotable (c,i)
char		c;
register int	i;
{
	register int	a;
	int		test;

	test = (c == 'R');

	while ( (a = atmata[i].ch) != '.')  {
		if (a == c || (test && a == '\n'))  {
			switch  (atmata[i].fcode)  {

			case 1:
				error("Redisplay request sent by gammontool");
				break;

			case 2:
				if (p[mvl] == -1)
					p[mvl] = c-'0';
				else
					p[mvl] = p[mvl]*10+c-'0';
				break;

			case 3:
				if (g[mvl] != -1)  {
					if (mvl < mvlim)
						mvl++;
					p[mvl] = p[mvl-1];
				}
				g[mvl] = p[mvl]+cturn*(c-'0');
				if (g[mvl] < 0)
					g[mvl] = 0;
				if (g[mvl] > 25)
					g[mvl] = 25;
				break;

			case 4:
				if (g[mvl] == -1)
					g[mvl] = c-'0';
				else
					g[mvl] = g[mvl]*10+c-'0';
				break;

			case 5:
				if (mvl < mvlim)
					mvl++;
				p[mvl] = g[mvl-1];
				break;

			case 6:
				if (mvl < mvlim)
					mvl++;
				break;

			case 7:
				error("Help request sent by gammontool");
				break;

			case 8:
				p[mvl] = bar;
				break;

			case 9:
				g[mvl] = home;
			}

			if (!test || a != '\n')
				return (atmata[i].newst);
			else
				return (-6);
		}
		i++;
	}
	return (-5);
}

rsetbrd()
{
	register int	i, j, n;

	n = 0;
	mvl = 0;
	for (i = 0; i < 4; i++)
		p[i] = g[i] = -1;
	for (j = 0; j < ncin; j++)
		n = dotable(cin[j],n);
	return (n);
}

movallow()
{
	register int	i, m, iold;

	if (d0)
		swap;
	m = (D0 == D1 ? 4: 2);
	for (i = 0; i < 4; i++)
		p[i] = bar;
	i = iold = 0;
	while (i < m)  {
		if (*offptr == 15)
			break;
		h[i] = 0;
		if (board[bar])  {
			if (i == 1 || m == 4)
				g[i] = bar+cturn*D1;
			else
				g[i] = bar+cturn*D0;
			if (makmove(i))  {
				if (d0 || m == 4)
					break;
				swap;
				movback(i);
				if (i > iold)
					iold = i;
				for (i = 0; i < 4; i++)
					p[i] = bar;
				i = 0;
			} else
				i++;
			continue;
		}
		if ((p[i] += cturn) == home)  {
			if (i > iold)
				iold = i;
			if (m == 2 && i)  {
				movback(i);
				p[i--] = bar;
				if (p[i] != bar)
					continue;
				else
					break;
			}
			if (d0 || m == 4)
				break;
			swap;
			movback (i);
			for (i = 0; i < 4; i++)
				p[i] = bar;
			i = 0;
			continue;
		}
		if (i == 1 || m == 4)
			g[i] = p[i]+cturn*D1;
		else
			g[i] = p[i]+cturn*D0;
		if (g[i]*cturn > home)  {
			if (*offptr >= 0)
				g[i] = home;
			else
				continue;
		}
		if (board[p[i]]*cturn > 0 && makmove(i) == 0)
			i++;
	}
	movback (i);
	return(iold > i? iold: i);
}

struct BOARD  {				/* structure of game position */
	int	b_board[26];			/* board position */
	int	b_in[2];			/* men in */
	int	b_off[2];			/* men off */
	int	b_st[4], b_fn[4];		/* moves */

	struct BOARD	*b_next;		/* forward queue pointer */
};

struct BOARD *freeq = (struct BOARD *)-1;
struct BOARD *checkq = (struct BOARD *)-1;
struct BOARD *bsave();
struct BOARD *nextfree();

static int	cp[5];				/* candidate start position */
static int	cg[5];				/* candidate finish position */
       int	race;				/* game reduced to a race */

move(okay)		/* do the computer's move */
int	okay;				/* zero if first move */
{
	register int	i;		/* index */
	register int	l;		/* last man */

	if (!race) {
		for (i = 0; i < 26; i++)  {
			if (board[i] < 0)
				l = i;
		}
		for (i = 0; i < l; i++)  {
			if (board[i] > 0)
				break;
		}
		if (i == l)
			race = 1;
	}

	if (okay)  {
					/* see if comp should double */
		if (gvalue < 64 && dlast != cturn && dblgood())  {
			dble();			    /* double */
						    /* return if declined */
			if (cturn != 1 && cturn != -1)
				return;
		} else {
			putchar(NODOUBLE);
		}
		roll();
	}

	mvlim = movallow();		/* find out how many moves */
	if (logfd != NULL) {
		fprintf(logfd, "%d %d\n", D0, D1);
	}
	if (mvlim == 0) {
		if (logfd != NULL) {
			fprintf(logfd, "can't move\n");
		}
		putchar(CANTMOVE);
		nexturn();
		return;
	}

	for (i = 0; i < 4; i++)		/* initialize */
		cp[i] = cg[i] = 0;

	trymove(0,0);			/* strategize */
	pickmove();

	if (d0) swap;				/* fix dice */

	if (logfd) {
		fprintf(logfd, "picked ");
	}
	putchar(CANMOVE);
	for (i = 0; i < mvlim; i++)  {
		if (i > 0)
			putchar(',');
		p[i] = cp[i];
		if (p[i] == 0 || p[i] == 25)
			putchar('B');
		else
			printf("%d", p[i]);
		putchar('-');
		g[i] = cg[i];
		if (g[i] == 0 || g[i] == 25)
			putchar('H');
		else
			printf("%d", g[i]);
		makmove(i);
		if (logfd != NULL) {
			fprintf(logfd, "%d->%d ", p[i], g[i]);
		}
	}
	putchar('\n');
	if (logfd) {
		fprintf(logfd, "\n");
		fflush(logfd);
	}

	nexturn();			/* get ready for next turn */
}

trymove(mvnum, swapped)
register int	mvnum;			/* number of move (rel zero) */
int		swapped;		/* see if swapped also tested */
{
	register int	pos;			/* position on board */
	register int	rval;			/* value of roll */

		/* if recursed through all dice values, compare move */
	if (mvnum == mvlim)  {
		binsert (bsave());
		return;
	}

						/* make sure dice in always
						 * same order */
	if (d0 == swapped)
		swap;
						/* choose value for this move */
	rval = dice[mvnum != 0];

						/* find all legitimate moves */
	for (pos = bar; pos != home; pos += cturn)  {
						/* fix order of dice */
		if (d0 == swapped)
			swap;
						/* break if stuck on bar */
		if (board[bar] != 0 && pos != bar)
			break;
						/* on to next if not occupied */
		if (board[pos]*cturn <= 0)
			continue;
						/* set up arrays for move */
		p[mvnum] = pos;
		g[mvnum] = pos+rval*cturn;
		if (g[mvnum]*cturn >= home)  {
			if (*offptr < 0)
				break;
			g[mvnum] = home;
		}
						/* try to move */
		if (makmove (mvnum)) {
			continue;
		} else
			trymove (mvnum+1,2);
						/* undo move to try another */
		backone (mvnum);
	}

						/* swap dice and try again */
	if ((!swapped) && D0 != D1)
		trymove (0,1);
}

struct BOARD *
bsave()
{
	register int	i;		/* index */
	struct BOARD	*now;		/* current position */

	now = nextfree();		/* get free BOARD */

					/* store position */
	for (i = 0; i < 26; i++)
		now->b_board[i] = board[i];
	now->b_in[0] = in[0];
	now->b_in[1] = in[1];
	now->b_off[0] = off[0];
	now->b_off[1] = off[1];
	for (i = 0; i < mvlim; i++)  {
		now->b_st[i] = p[i];
		now->b_fn[i] = g[i];
	}
	return (now);
}

binsert(new)
struct BOARD	*new;					/* item to insert */
{
	register struct BOARD	*p = checkq;		/* queue pointer */
	register int		result;			/* comparison result */

	if (p == (struct BOARD *)-1)  {			/* check if queue empty */
		checkq = p = new;
		p->b_next = (struct BOARD *)-1;
		return;
	}

	result = bcomp(new,p);			/* compare to first element */
	if (result < 0)  {				/* insert in front */
		new->b_next = p;
		checkq = new;
		return;
	}
	if (result == 0)  {				/* duplicate entry */
		/*mvcheck(p,new);*/
		makefree(new);
		return;
	}

	while (p->b_next != (struct BOARD *)-1)  {	/* traverse queue */
		result = bcomp(new,p->b_next);
		if (result < 0)  {			/* found place */
			new->b_next = p->b_next;
			p->b_next = new;
			return;
		}
		if (result == 0)  {			/* duplicate entry */
			/*mvcheck(p->b_next,new);*/
			makefree(new);
			return;
		}
		p = p->b_next;
	}
						/* place at end of queue */
	p->b_next = new;
	new->b_next = (struct BOARD *)-1;
}

bcomp(a,b)
struct BOARD	*a;
struct BOARD	*b;
{
	register int	*aloc = a->b_board;	/* pointer to board a */
	register int	*bloc = b->b_board;	/* pointer to board b */

	for (aloc = a->b_board, bloc = b->b_board;
	     aloc < &a->b_board[26];
	     aloc++, bloc++)  {
		if (*aloc-*bloc) {
			return (*aloc-*bloc);		/* found inequality */
		}
	}
	return (0);				/* same position */
}

mvcheck(incumbent,candidate)
register struct BOARD 	*incumbent;
register struct BOARD 	*candidate;
{
	register int	i;
	register int	result;

	for (i = 0; i < mvlim; i++)  {
		result = cturn*(candidate->b_st[i]-incumbent->b_st[i]);
		if (result > 0)
			return;
		if (result < 0)
			break;
	}
	if (i == mvlim)
		return;
	for (i = 0; i < mvlim; i++)  {
		incumbent->b_st[i] = candidate->b_st[i];
		incumbent->b_fn[i] = candidate->b_fn[i];
	}
}

makefree(dead)
struct BOARD	*dead;			/* dead position */
{
	dead->b_next = freeq;			/* add to freeq */
	freeq = dead;
}

struct BOARD *
nextfree()
{
	struct BOARD	*new;

	if (freeq == (struct BOARD *)-1)  {
		new = (struct BOARD *)calloc (1,sizeof (struct BOARD));
		if (new == 0)  {
			error("Out of memory!");
			return(0);
		}
		new->b_next = (struct BOARD *)-1;
		return (new);
	}

	new = freeq;
	freeq = freeq->b_next;
	return (new);
}

pickmove()
{
						/* current game position */
	register struct BOARD	*now = bsave();
	register struct BOARD	*next;		/* next move */

	do  {				/* compare moves */
		brdcopy(checkq);
		next = checkq->b_next;
		makefree(checkq);
		checkq = next;
		movcmp();
	} while (checkq != (struct BOARD *)-1);

	brdcopy (now);
}

brdcopy(s)
register struct BOARD	*s;			/* game situation */
{
	register int	i;			/* index */

	for (i = 0; i < 26; i++)
		board[i] = s->b_board[i];
	for (i = 0; i < 2; i++)  {
		in[i] = s->b_in[i];
		off[i] = s->b_off[i];
	}
	for (i = 0; i < mvlim; i++)  {
		p[i] = s->b_st[i];
		g[i] = s->b_fn[i];
	}
}

int bestmove = -1;

movcmp()
{
	int newval, i;

	newval = evalmove();
	if (newval > bestmove || (cp[0] == 0 && cg[0] == 0)) {
		bestmove = newval;
		for (i = 0; i < mvlim; i++)  {
			cp[i] = p[i];
			cg[i] = g[i];
		}
	}
}

/*
 * Set dice table to one for roll r1,r2.  If r1 is 0
 * clear table, if r2 is 0 set entries for r1,*.
 */
odds(r1, r2)
register int	r1;
int		r2;
{
	register int	i, j;

	if (r1 == 0)  {
		for (i = 0; i < 6; i++)  
			for (j = 0; j < 6; j++)
				table[i][j] = 0;
		return;
	} else  {
		r1--;
		if (r2-- == 0) {
			for (i = 0; i < 6; i++)  {
				table[i][r1] = 1;
				table[r1][i] = 1;
			}
		} else  {
			table[r1][r2] = 1;
		}
	}
}

/*
 * add up dice rolls in table and return % roll that are set
 */
count()
{
	register int	i;
	register int	j;
	register int	total;

	total = 0;
	for (i = 0; i < 6; i++)
		for (j = 0; j < 6; j++)
			total += table[i][j];
	return ((total * 100) / 36);
}

/*
 * Returns the % probability that the piece at i will be hit.
 * If c is set the probability table is cleared first.
 */
canhit (i, c)
int	i, c;
{
	register int	j, k, b;
	int		a, dist;

	if (c == 0)
		odds(0, 0);
	if (board[i] > 0)  {
		a = -1;
		b = 25;
	} else  {
		a = 1;
		b = 0;
	}
	/*
	if (logfd) {
		fprintf(logfd, "canhit %d\n", i);
	}
	*/
	for (j = b; j != i; j += a)  {
		if (board[j]*a <= 0)  {
			/*
			 * no bad guys here, move on
			 */
			continue;
		}
		dist = abs(j-i);
		/*
		if (logfd) {
			fprintf(logfd, "from %d ", dist);
		}
		*/
		for (k = 1; k <= MIN(dist, 6); k++) {
			if (board[j+a*k]*a <= -2) {
				/*
				 * This spot blocked, by good guys, try next
				 */
				/*
				if (logfd) {
					fprintf(logfd, "(%d blkd %d) ", j+a*k, board[j+a*k]*a);
				}
				*/
				continue;
			}
			if (k == dist) {
				/*
				 * Can hit with one die
				 */
				/*
				if (logfd) {
					fprintf(logfd, "(%d,*) ", dist);
				}
				*/
				odds(dist, 0);
			} else if (dist - k < 7) {
				/*
				 * can hit with 2 dice
				 */
				/*
				if (logfd) {
					fprintf(logfd, "(%d,%d) ", k, dist-k);
				}
				*/
				odds(k, dist-k);
			} else if ( (board[j+a*2*k]*a >= 0) &&
			    (3*k == dist ||
			     (4*k == dist && board[j+a*3*k]*a >= 0)) ) {
				/*
				 * can hit with doubles
				 */
				/*
				if (logfd) {
					fprintf(logfd, "(%d,%d) ", k, k);
				}
				*/
				odds(k, k);
			} else {
				/*
				if (logfd) {
					fprintf(logfd, "(%d nope) ", k);
				}
				*/
			}
		}
		/*
		if (logfd) {
			fprintf(logfd, "\n");
		}
		*/
	}
	return(count());
}

/*
 * dble()
 *	Have the current player double and ask opponent to accept.
 */

dble()
{
	if (cturn == -pnum)  {			/* see if computer accepts */
		if (dblgood())  {		/* guess not */
			putchar(REFUSE_DBLE);
			nexturn();
			cturn *= -2;		/* indicate loss */
			return;
		} else {			/* computer accepts */
			putchar(ACCEPT_DBLE);
			gvalue *= 2;		/* double game value */
			dlast = cturn;
			return;
		}
	}

	putchar(DOUBLEREQ);
	switch (getchar()) {			/* ask if player accepts */
	case ACCEPT_DBLE:
		gvalue *= 2;
		dlast = cturn;
		return;
	case REFUSE_DBLE:
		nexturn();			/* declined */
		cturn *= -2;
		return;
	default:
		error("Bad response to double sent by gammontool");
		return;
	}
}

/*
 * dblgood ()
 *	Returns 1 if the computer would double in this position.  This
 * is not an exact science.  The computer will decline a double that he
 * would have made.  Accumulated judgments are kept in the variable n,
 * which is in "pips", i.e., the position of each man summed over all
 * men, with opponent's totals negative.  Thus, n should have a positive
 * value of 7 for each move ahead, or a negative value of 7 for each one
 * behind.
 */

dblgood()
{
	register int	n;			/* accumulated judgment */
	register int	OFFC = *offptr;		/* no. of computer's men off */
	register int	OFFO = *offopp;		/* no. of player's men off */

						/* get real pip value */
	n = eval() * cturn;

	/* below adjusts pip value according to position judgments */

	if (OFFC > -15 || OFFO > -15) {	/* check men moving off board */
		if (OFFC < 0 && OFFO < 0) {
			OFFC += 15;
			OFFO += 15;
			n +=((OFFC-OFFO)*7)/2;
		} else if (OFFC < 0) {
			OFFC += 15;
			n -= OFFO*7/2;
		} else if (OFFO < 0) {
			OFFO += 15;
			n += OFFC*7/2;
		}
		if (OFFC < 8 && OFFO > 8)
			n -= 7;
		if (OFFC < 10 && OFFO > 10)
			n -= 7;
		if (OFFC < 12 && OFFO > 12)
			n -= 7;
		if (OFFO < 8 && OFFC > 8)
			n += 7;
		if (OFFO < 10 && OFFC > 10)
			n += 7;
		if (OFFO < 12 && OFFC > 12)
			n += 7;
		n += ((OFFC-OFFO)*7)/2;
	}

	/*
	n += freemen(home);
	n -= freemen(bar);
	n += trapped(home,-cturn);
	n -= trapped(bar,cturn);
	*/
	if (!race) {
		n += blockage(home);
		n -= blockage(bar);
	}

	if (n > 20+rnum(7))	/* double if 2-3 moves ahead */
		return(1);
	return(0);
}

/*
 * returns the number of blocked spaces in home
 */
filled(home)
int	home;
{
	register int	i, nblock, turn;

	turn = (home ? 1 : -1);
	for (nblock = 0, i = home-turn; i != home-7*turn; i -= turn)
		if (board[i]*turn > 1)
			nblock++;
	return (nblock);
}

/*
 * returns the number of men in 3rd bay that can safely be used to build
 * in home.
 */
readytofill(home)
int	home;
{
	register int	i, nfree, turn;

	turn = (home ? -1 : 1);
	for (nfree = 0, i = home-7*turn; i != home-13*turn; i -= turn)
		if (board[i]*turn >= 1 && (board[i]*turn & 1))
			nfree++;
	return (nfree);
}

blockage(home)
int home;
{
	register int	i, j, turn;
	int totblock, nblock;
	int hisguys = 0;

	turn = (home ? 1 : -1);
	for (totblock = 0, i = home*turn; i != home-13*turn; i -= turn) {
		if (board[i]*turn < 0) {
			hisguys += -board[i]*turn;
			continue;
		}
		if (board[i]*turn <= 0) {
			continue;
		}
		for (nblock = 0, j = i; j != i-7*turn; j -= turn) {
			if (board[j]*turn > 1) {
				nblock++;
			}
		}
		nblock = nblock * nblock * hisguys;
		if (nblock > totblock) {
			totblock = nblock;
		}
	}
	return (totblock);
}

/*
freemen(b)
int	b;
{
	register int	i, inc, lim;

	odds(0, 0);
	if (board[b] == 0)
		return (0);
	inc = (b == 0? 1: -1);
	lim = (b == 0? 7: 18);
	for (i = b+inc; i != lim; i += inc)
		if (board[i]*inc < -1)
			odds(abs(b-i), 0);
	if (abs(board[b]) == 1)
		return ((36-count())/5);
	return (count()/5);
}

trapped(n,inc)
int	n, inc;
{
	register int	i, j, k;
	int		c, l, ct;

	ct = 0;
	l = n+7*inc;
	for (i = n+inc; i != l; i += inc)  {
		odds(0, 0);
		c = abs(i-l);
		if (board[i]*inc > 0)  {
			for (j = c; j < 13; j++)
				if (board[i+inc*j]*inc < -1)  {
					if (j < 7)
						odds(j, 0);
					for (k = 1; k < 7 && k < j; k++)
						if (j-k < 7)
							odds (k, j-k);
				}
			ct += abs(board[i])*(36-count());
		}
	}
	return (ct/5);
}
*/

eval()
{
	register int	i, j;

	for (j = i = 0; i < 26; i++)
		j += (board[i] >= 0 ? i*board[i] : (25-i)*board[i]);

	if (off[1] >= 0)
		j += 25*off[1];
	else
		j += 25*(off[1]+15);

	if (off[0] >= 0)
		j -= 25*off[0];
	else
		j -= 25*(off[0]+15);
	return (j);
}

makmove(i)
register int	i;
{
	register int	n, d, fin, start;
	int		max;

	d = d0;
	fin = g[i];
	start = p[i];
	n = abs(fin-start);
	max = (*offptr < 0? 7: last());
	if (board[start]*cturn <= 0)
		return(checkd(d)+2);
	if (fin != home && board[fin]*cturn < -1)
		return (checkd(d)+3);
	if (i || D0 == D1)  {
		if (n == max? D1 < n: D1 != n)
			return (checkd(d)+1);
	} else  {
		if (n == max? D0 < n && D1 < n: D0 != n && D1 != n)
			return (checkd(d)+1);
		if (n == max? D0 < n: D0 != n)  {
			if (d0)
				return (checkd(d)+1);
			swap;
		}
	}
	if (fin == home && *offptr < 0)
		return (checkd(d)+4);
	h[i] = 0;
	board[start] -= cturn;
	if (fin != home)  {
		if (board[fin] == -cturn)  {
			board[home] -= cturn;
			board[fin] = 0;
			h[i] = 1;
			if (abs(bar-fin) < 7)  {
				(*inopp)--;
				if (*offopp >= 0)
					*offopp -= 15;
			}
		}
		board[fin] += cturn;
		if (abs(home-fin) < 7 && abs(home-start) > 6)  {
			(*inptr)++;
			if (*inptr+*offptr == 0)
				*offptr += 15;
		}
	} else {
		(*offptr)++;
		(*inptr)--;
	}
	return (0);
}

checkd(d)
register int	d;
{
	if (d0 != d)
		swap;
	return (0);
}

last()
{
	register int	i;

	for (i = home-6*cturn; i != home; i += cturn)
		if (board[i]*cturn > 0)
			return (abs(home-i));
}

movback(i)
register int	i;
{
	register int	j;

	for (j = i-1; j >= 0; j--)
		backone(j);
}

backone(i)
register int	i;
{
	board[p[i]] += cturn;
	if (g[i] != home)  {
		board[g[i]] -= cturn;
		if (abs(g[i]-home) < 7 && abs(p[i]-home) > 6)  {
			(*inptr)--;
			if (*inptr+*offptr < 15 && *offptr >= 0)
				*offptr -= 15;
		}
	} else  {
		(*offptr)--;
		(*inptr)++;
	}
	if (h[i])  {
		board[home] += cturn;
		board[g[i]] = -cturn;
		if (abs(bar-g[i]) < 7)  {
			(*inopp)++;
			if (*inopp+*offopp == 0)
				*offopp += 15;
		}
	}
}

init()		/* initialize the board */
{
	register int	i;

	for (i = 0; i < 26;)
		board[i++] = 0;
	board[1] = 2;
	board[6] = board[13] = -5;
	board[8] = -3;
	board[12] = board[19] = 5;
	board[17] = 3;
	board[24] = -2;
	off[0] = off[1] = -15;
	in[0] = in[1] = 5;
	gvalue = 1;
	dlast = 0;
}

roll()
{
	if (getchar() != DIEROLL) {
		error("Bad die roll code sent by gammontool");
		return;
	}
	D0 = getchar() - '0';
	D1 = getchar() - '0';
	if (D0 > 6 || D1 > 6 || D0 < 1 || D1 < 0)
		error("Illegal die rolls sent by gammontool");
}

nexturn()
{
	register int	c;

	cturn = -cturn;
	c = cturn/abs(cturn);
	home = bar;
	bar = 25-bar;
	offptr += c;
	offopp -= c;
	inptr += c;
	inopp -= c;
}
