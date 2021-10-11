/*	@(#)snake.h 1.1 92/07/30 SMI; from UCB 4.1 83/07/04	*/

# include <stdio.h>
# include <assert.h>
# include <sys/types.h>
# include <sgtty.h>
# include <signal.h>
# include <math.h>

#define ESC	'\033'

struct tbuffer {
	long t[4];
} tbuffer;

char	*CL, *UP, *DO, *ND, *BS,
	*HO, *CM,
	*TA, *LL,
	*KL, *KR, *KU, *KD,
	*TI, *TE, *KS, *KE;
int	LINES, COLUMNS;	/* physical screen size. */
int	lcnt, ccnt;	/* user's idea of screen size */
char	xBC, PC;
int	AM, BW;
char	tbuf[1024], tcapbuf[128];
char	*tgetstr(), *tgoto();
int	Klength;	/* length of KX strings */
int	chunk;		/* amount of money given at a time */
#ifdef	debug
#define	cashvalue	(loot-penalty)/25
#else
#define cashvalue	chunk*(loot-penalty)/25
#endif

struct point {
	int col, line;
};
struct point cursor;
struct sgttyb orig, new;
#ifdef TIOCLGET
struct ltchars olttyc, nlttyc;
#endif
struct point *point();
