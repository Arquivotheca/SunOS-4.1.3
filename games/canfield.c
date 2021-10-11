#ifndef lint
static	char sccsid[] = "@(#)canfield.c 1.1 92/07/30 SMI"; /* from UCB 5.4 1/13/86 */
#endif

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

/*
 * The canfield program 
 *
 * Authors:
 *	Originally written: Steve Levine
 *	Converted to use curses and debugged: Steve Feldman
 *	Card counting: Kirk McKusick and Mikey Olson
 *	User interface cleanups: Eric Allman and Kirk McKusick
 *	Betting by Kirk McKusick
 *	Pipe stuff for canfieldtool by James Carrington 
 */

#include <curses.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>

#ifdef	SYSV
# ifndef	attron
#	define	erasechar()	_tty.c_cc[VERASE]
#	define	killchar()	_tty.c_cc[VKILL]
# endif
#else
# ifndef	erasechar
#	define	erasechar()	_tty.sg_erase
#	define	killchar()	_tty.sg_kill
# endif
#endif

#define	decksize	52
#define originrow	0
#define origincol	0
#define	basecol		1
#define	boxcol		42
#define	tboxrow		2
#define	bboxrow		17
#define	movecol		43
#define	moverow		16
#define	msgcol		43
#define	msgrow		15
#define	titlecol	30
#define	titlerow	0
#define	sidecol		1
#define	ottlrow		6
#define	foundcol	11
#define	foundrow	3
#define	stockcol	2
#define	stockrow	8
#define	fttlcol		10
#define	fttlrow		1
#define	taloncol	2
#define	talonrow	13
#define	tabrow		8
#define ctoprow		21
#define cbotrow		23
#define cinitcol	14
#define cheightcol	1
#define cwidthcol	4
#define handstatrow	21
#define handstatcol	7
#define talonstatrow	22
#define talonstatcol	7
#define stockstatrow	23
#define stockstatcol	7
#define	Ace		1
#define	Jack		11
#define	Queen		12
#define	King		13
#define	atabcol		11
#define	btabcol		18
#define	ctabcol		25
#define	dtabcol		32

#define	spades		's'
#define	clubs		'c'
#define	hearts		'h'
#define	diamonds	'd'
#define	black		'b'
#define	red		'r'

#define stk		1
#define	tal		2
#define tab		3
#define INCRHAND(row, col) {\
	row -= cheightcol;\
	if (row < ctoprow) {\
		row = cbotrow;\
		col += cwidthcol;\
	}\
}
#define DECRHAND(row, col) {\
	row += cheightcol;\
	if (row > cbotrow) {\
		row = ctoprow;\
		col -= cwidthcol;\
	}\
}


struct cardtype {
	char            suit;
	char            color;
	bool            visible;
	bool            paid;
	int             rank;
	struct cardtype *next;
};

#define	NIL	((struct cardtype *) 0)

struct cardtype *deck[decksize];
struct cardtype cards[decksize];
struct cardtype *bottom[4], *found[4], *tableau[4];
struct cardtype *talon, *hand, *stock, *basecard;
int             length[4];
int             cardsoff, base, cinhand, taloncnt, stockcnt, timesthru;
char            suitmap[4] = {spades, clubs, hearts, diamonds};
char            colormap[4] = {black, black, red, red};
char            pilemap[4] = {atabcol, btabcol, ctabcol, dtabcol};
char            srcpile, destpile;
int             mtforigin, tempbase;
int             coldcol, cnewcol, coldrow, cnewrow;
bool            errmsg, done;
bool            mtfdone, Cflag = FALSE;
bool            PIPE = FALSE;

char           *basicinstructions[] = {
	      "Here are brief instructions to the game of Canfield:\n\n",
	"     If you have never played solitaire before, it is recom-\n",
	"mended  that  you  consult  a solitaire instruction book. In\n",
	"Canfield, tableau cards may be built on each other  downward\n",
	"in  alternate colors. An entire pile must be moved as a unit\n",
	"in building. Top cards of the piles are available to be able\n",
	 "to be played on foundations, but never into empty spaces.\n\n",
	"     Spaces must be filled from the stock. The top  card  of\n",
	"the  stock  also is available to be played on foundations or\n",
	"built on tableau piles. After the stock  is  exhausted,  ta-\n",
	"bleau spaces may be filled from the talon and the player may\n",
		       "keep them open until he wishes to use them.\n\n",
	"     Cards are dealt from the hand to the  talon  by  threes\n",
	"and  this  repeats until there are no more cards in the hand\n",
	"or the player quits. To have cards dealt onto the talon  the\n",
	"player  types  'ht'  for his move. Foundation base cards are\n",
	"also automatically moved to the foundation when they  become\n",
				       "available.\n\n",
				  "push any key when you are finished: ",
				       0};

char           *bettinginstructions[] = {
	"     The rules for betting are  somewhat  less  strict  than\n",
	"those  used in the official version of the game. The initial\n",
	"deal costs $13. You may quit at this point  or  inspect  the\n",
	"game.  Inspection  costs  $13 and allows you to make as many\n",
	"moves as is possible without moving any cards from your hand\n",
	"to  the  talon.  (the initial deal places three cards on the\n",
	"talon; if all these cards are  used,  three  more  are  made\n",
	"available)  Finally, if the game seems interesting, you must\n",
	"pay the final installment of $26.  At  this  point  you  are\n",
	"credited  at the rate of $5 for each card on the foundation;\n",
	"as the game progresses you are credited  with  $5  for  each\n",
	"card  that is moved to the foundation.  Each run through the\n",
	"hand after the first costs $5.  The  card  counting  feature\n",
	"costs  $1  for  each unknown card that is identified. If the\n",
	"information is toggled on, you are only  charged  for  cards\n",
	"that  became  visible  since it was last turned on. Thus the\n",
	"maximum cost of information is $34.  Playing time is charged\n",
				       "at a rate of $1 per minute.\n\n",
				  "push any key when you are finished: ",
					 0};
#define INSTRUCTIONBOX	1
#define BETTINGBOX	2
#define NOBOX		3
int             status = INSTRUCTIONBOX;

/*
 * Basic betting costs 
 */
#define costofhand		13
#define costofinspection	13
#define costofgame		26
#define costofrunthroughhand	 5
#define costofinformation	 1
#define secondsperdollar	60
#define maxtimecharge		 3
#define valuepercardup	 	 5
/*
 * Variables associated with betting 
 */
struct betinfo {
	long            hand;	/* cost of dealing hand */
	long            inspection;	/* cost of inspecting hand */
	long            game;	/* cost of buying game */
	long            runs;	/* cost of running through hands */
	long            information;	/* cost of information */
	long            thinktime;	/* cost of thinking time */
	long            wins;	/* total winnings */
	long            worth;	/* net worth after costs */
};
struct betinfo  this, total;
bool            startedgame = FALSE, infullgame = FALSE;
time_t          acctstart;
int             dbfd = -1;

char
toRank(num)
	int             num;
{
	switch (num) {
	case 1:
		return 'A';
	case 2:
		return '2';
	case 3:
		return '3';
	case 4:
		return '4';
	case 5:
		return '5';
	case 6:
		return '6';
	case 7:
		return '7';
	case 8:
		return '8';
	case 9:
		return '9';
	case 10:
		return '0';
	case 11:
		return 'J';
	case 12:
		return 'Q';
	case 13:
		return 'K';
	default:
		exit(1);
	}
	/*NOTREACHED*/
}
/*
 * The following procedures print the board onto the screen using the
 * addressible cursor. The end of these procedures will also be
 * separated from the rest of the program. 
 *
 * procedure to set the move command box 
 */
movebox()
{
	if (PIPE == FALSE) {
		switch (status) {
		case BETTINGBOX:
			printtopbettingbox();
			break;
		case NOBOX:
			clearabovemovebox();
			break;
		case INSTRUCTIONBOX:
			printtopinstructions();
			break;
		}
		move(moverow, boxcol);
		(void) printw("|                          |");
		move(msgrow, boxcol);
		(void) printw("|                          |");
		switch (status) {
		case BETTINGBOX:
			printbottombettingbox();
			break;
		case NOBOX:
			clearbelowmovebox();
			break;
		case INSTRUCTIONBOX:
			printbottominstructions();
			break;
		}
		refresh();
	}
}

/*
 * print directions above move box 
 */
printtopinstructions()
{
	move(tboxrow, boxcol);
	(void) printw("*--------------------------*");
	move(tboxrow + 1, boxcol);
	(void) printw("|         MOVES            |");
	move(tboxrow + 2, boxcol);
	(void) printw("|s# = stock to tableau     |");
	move(tboxrow + 3, boxcol);
	(void) printw("|sf = stock to foundation  |");
	move(tboxrow + 4, boxcol);
	(void) printw("|t# = talon to tableau     |");
	move(tboxrow + 5, boxcol);
	(void) printw("|tf = talon to foundation  |");
	move(tboxrow + 6, boxcol);
	(void) printw("|## = tableau to tableau   |");
	move(tboxrow + 7, boxcol);
	(void) printw("|#f = tableau to foundation|");
	move(tboxrow + 8, boxcol);
	(void) printw("|ht = hand to talon        |");
	move(tboxrow + 9, boxcol);
	(void) printw("|c = toggle card counting  |");
	move(tboxrow + 10, boxcol);
	(void) printw("|b = present betting info  |");
	move(tboxrow + 11, boxcol);
	(void) printw("|q = quit to end the game  |");
	move(tboxrow + 12, boxcol);
	(void) printw("|==========================|");
}

/*
 * Print the betting box. 
 */
printtopbettingbox()
{

	move(tboxrow, boxcol);
	(void) printw("                            ");
	move(tboxrow + 1, boxcol);
	(void) printw("*--------------------------*");
	move(tboxrow + 2, boxcol);
	(void) printw("|Costs        Hand   Total |");
	move(tboxrow + 3, boxcol);
	(void) printw("| Hands                    |");
	move(tboxrow + 4, boxcol);
	(void) printw("| Inspections              |");
	move(tboxrow + 5, boxcol);
	(void) printw("| Games                    |");
	move(tboxrow + 6, boxcol);
	(void) printw("| Runs                     |");
	move(tboxrow + 7, boxcol);
	(void) printw("| Information              |");
	move(tboxrow + 8, boxcol);
	(void) printw("| Think time               |");
	move(tboxrow + 9, boxcol);
	(void) printw("|Total Costs               |");
	move(tboxrow + 10, boxcol);
	(void) printw("|Winnings                  |");
	move(tboxrow + 11, boxcol);
	(void) printw("|Net Worth                 |");
	move(tboxrow + 12, boxcol);
	(void) printw("|==========================|");
}

/*
 * clear info above move box 
 */
clearabovemovebox()
{
	int             i;

	for (i = 0; i <= 11; i++) {
		move(tboxrow + i, boxcol);
		(void) printw("                            ");
	}
	move(tboxrow + 12, boxcol);
	(void) printw("*--------------------------*");
}

/*
 * print instructions below move box 
 */
printbottominstructions()
{
	move(bboxrow, boxcol);
	(void) printw("|Replace # with the number |");
	move(bboxrow + 1, boxcol);
	(void) printw("|of the tableau you want.  |");
	move(bboxrow + 2, boxcol);
	(void) printw("*--------------------------*");
}

/*
 * print betting information below move box 
 */
printbottombettingbox()
{
	move(bboxrow, boxcol);
	(void) printw("|x = toggle information box|");
	move(bboxrow + 1, boxcol);
	(void) printw("|i = list instructions     |");
	move(bboxrow + 2, boxcol);
	(void) printw("*--------------------------*");
}

/*
 * clear info below move box 
 */
clearbelowmovebox()
{
	int             i;

	move(bboxrow, boxcol);
	(void) printw("*--------------------------*");
	for (i = 1; i <= 2; i++) {
		move(bboxrow + i, boxcol);
		(void) printw("                            ");
	}
}

/*
 * procedure to put the board on the screen using addressable cursor 
 */
makeboard()
{
	if (PIPE == FALSE) {
		clear();
		refresh();
		move(titlerow, titlecol);
		(void) printw("=-> CANFIELD <-=");
		move(fttlrow, fttlcol);
		(void) printw("foundation");
		move(foundrow - 1, fttlcol);
		(void) printw("=---=  =---=  =---=  =---=");
		move(foundrow, fttlcol);
		(void) printw("|   |  |   |  |   |  |   |");
		move(foundrow + 1, fttlcol);
		(void) printw("=---=  =---=  =---=  =---=");
		move(ottlrow, sidecol);
		(void) printw("stock     tableau");
		move(stockrow - 1, sidecol);
		(void) printw("=---=");
		move(stockrow, sidecol);
		(void) printw("|   |");
		move(stockrow + 1, sidecol);
		(void) printw("=---=");
		move(talonrow - 2, sidecol);
		(void) printw("talon");
		move(talonrow - 1, sidecol);
		(void) printw("=---=");
		move(talonrow, sidecol);
		(void) printw("|   |");
		move(talonrow + 1, sidecol);
		(void) printw("=---=");
		move(tabrow - 1, atabcol);
		(void) printw("-1-    -2-    -3-    -4-");
		movebox();
	}
}

/*
 * clean up the board for another game 
 */
cleanupboard()
{
	int             cnt, row, col;
	struct cardtype *ptr;

	if (PIPE == FALSE) {
		if (Cflag) {
			clearstat();
			for (ptr = stock, row = stockrow;
			     ptr != NIL;
			     ptr = ptr->next, row++) {
				move(row, sidecol);
				(void) printw("     ");
			}
			move(row, sidecol);
			(void) printw("     ");
			move(stockrow + 1, sidecol);
			(void) printw("=---=");
			move(talonrow - 2, sidecol);
			(void) printw("talon");
			move(talonrow - 1, sidecol);
			(void) printw("=---=");
			move(talonrow + 1, sidecol);
			(void) printw("=---=");
		}
		move(stockrow, sidecol);
		(void) printw("|   |");
		move(talonrow, sidecol);
		(void) printw("|   |");
		move(foundrow, fttlcol);
		(void) printw("|   |  |   |  |   |  |   |");
		for (cnt = 0; cnt < 4; cnt++) {
			switch (cnt) {
			case 0:
				col = atabcol;
				break;
			case 1:
				col = btabcol;
				break;
			case 2:
				col = ctabcol;
				break;
			case 3:
				col = dtabcol;
				break;
			}
			for (ptr = tableau[cnt], row = tabrow;
			     ptr != NIL;
			     ptr = ptr->next, row++)
				removecard(col, row);
		}
	}
}

/*
 * procedure to create a deck of cards 
 */
initdeck()
{
	int             i;
	int             scnt;
	char            s;
	int             r;

	i = 0;
	for (scnt = 0; scnt < 4; scnt++) {
		s = suitmap[scnt];
		for (r = Ace; r <= King; r++) {
			deck[i] = &cards[i];
			cards[i].rank = r;
			cards[i].suit = s;
			cards[i].color = colormap[scnt];
			cards[i].next = NIL;
			i++;
		}
	}
}

/*
 * procedure to shuffle the deck 
 */
shuffle()
{
	int             i, j;
	struct cardtype *temp;
	long            random();

	for (i = 0; i < decksize; i++) {
		deck[i]->visible = FALSE;
		deck[i]->paid = FALSE;
	}
	for (i = decksize - 1; i >= 0; i--) {
		j = random() % decksize;
		if (i != j) {
			temp = deck[i];
			deck[i] = deck[j];
			deck[j] = temp;
		}
	}
}
/*
 * procdedure to encode the col to send to canfieldtool 
 */
encode_col(a)
	int             a;
{
	int             x;

	switch (a) {
	case 2:
		x = 1;
		break;
	case 11:
		x = 2;
		break;
	case 18:
		x = 3;
		break;
	case 25:
		x = 4;
		break;
	case 32:
		x = 5;
		break;
	default:
		x = 0;
#ifdef DEBUG
		(void) fprintf(stderr, "cf: bad column number: %d\n", a);
#endif
		break;
	}
	return x;
}
/*
 * procdedure to encode the row to send to canfieldtool 
 */
char
encode_row(b)
	int             b;
{
	char            y;

	switch (b) {
	case 3:
		y = 'a';
		break;
	case 8:
		y = 'b';
		break;
	case 9:
		y = 'c';
		break;
	case 10:
		y = 'd';
		break;
	case 11:
		y = 'e';
		break;
	case 12:
		y = 'f';
		break;
	case 13:
		y = 'g';
		break;
	case 14:
		y = 'h';
		break;
	case 15:
		y = 'i';
		break;
	case 16:
		y = 'j';
		break;
	case 17:
		y = 'k';
		break;
	case 18:
		y = 'l';
		break;
	case 19:
		y = 'm';
		break;
	case 20:
		y = 'n';
		break;
	case 21:
		y = 'o';
		break;
	default:
		y = 'z';
#ifdef DEBUG
		(void) fprintf(stderr, "cf: bad row number: %d\n", b);
#endif
		break;
	}
	return y;
}
/*
 * procedure to remove the card from the board 
 */
removecard(a, b)
{
	int             x;
	char            y;

	if (PIPE == FALSE) {
		move(b, a);
		(void) printw("   ");
	} else {
		x = encode_col(a);
		y = encode_row(b);
		(void) printf("%d%cRM\n", x, y);
	}
}

/*
 * procedure to print the cards on the board 
 */
printrank(a, b, cp, inverse)
	struct cardtype *cp;
	bool            inverse;
{
	move(b, a);
	if (cp->rank != 10)
		addch(' ');
	if (inverse)
		standout();
	switch (cp->rank) {
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		(void) printw("%d", cp->rank);
		break;
	case Ace:
		addch('A');
		break;
	case Jack:
		addch('J');
		break;
	case Queen:
		addch('Q');
		break;
	case King:
		addch('K');
	}
	if (inverse)
		standend();
}

/*
 * procedure to print out a card 
 */
printcard(a, b, cp)
	int             a, b;
	struct cardtype *cp;
{
	int             x;
	char            y;

#ifdef DEBUG
	(void) fprintf(stderr, "cf: printcard -- a = %d -- b = %d\n", a, b);
#endif
	if (PIPE == FALSE) {
		if (cp == NIL)
			removecard(a, b);
		else if (cp->visible == FALSE) {
			move(b, a);
			(void) printw(" ? ");
		} else {
			bool            inverse = (cp->suit == 'd' || cp->suit == 'h');

			printrank(a, b, cp, inverse);
			if (inverse)
				standout();
			addch(cp->suit);
			if (inverse)
				standend();
		}
	} else {
		if (cp == NIL)
			removecard(a, b);
		else {
			x = encode_col(a);
			y = encode_row(b);
#ifdef DEBUG
			(void) fprintf(stderr, "cf: printing card: row %d col %c suit %c rank %c\n", x, y, cp->suit, toRank(cp->rank));
#endif
			(void) printf("%d%c%c%c\n", x, y, cp->suit, toRank(cp->rank));

		}
	}
}
/*
 * procedure to move the top card from one location to the top
 * of another location. The pointers always point to the top
 * of the piles. 
 */
transit(source, dest)
	struct cardtype **source, **dest;
{
	struct cardtype *temp;

	temp = *source;
	*source = (*source)->next;
	temp->next = *dest;
	*dest = temp;
}

/*
 * Procedure to set the cards on the foundation base when available.
 * Note that it is only called on a foundation pile at the beginning of
 * the game, so the pile will have exactly one card in it. 
 */
fndbase(cp, column, row)
	struct cardtype **cp;
{
	bool            nomore;

	if (*cp != NIL)
		do {
			if ((*cp)->rank == basecard->rank) {
				base++;
				printcard(pilemap[base], foundrow, *cp);
				if (*cp == tableau[0])
					length[0] = length[0] - 1;
				if (*cp == tableau[1])
					length[1] = length[1] - 1;
				if (*cp == tableau[2])
					length[2] = length[2] - 1;
				if (*cp == tableau[3])
					length[3] = length[3] - 1;
				transit(cp, &found[base]);
				if (cp == &talon)
					usedtalon();
				if (cp == &stock)
					usedstock();
				if (*cp != NIL) {
					printcard(column, row, *cp);
					nomore = FALSE;
				} else {
					removecard(column, row);
					nomore = TRUE;
				}
				cardsoff++;
				if (infullgame) {
					this.wins += valuepercardup;
					total.wins += valuepercardup;
				}
			} else
				nomore = TRUE;
		} while (nomore == FALSE);
}

/*
 * procedure to initialize the things necessary for the game 
 */
initgame()
{
	register        i;

	for (i = 0; i < 18; i++) {
		deck[i]->visible = TRUE;
		deck[i]->paid = TRUE;
	}
	stockcnt = 13;
	stock = deck[12];
	for (i = 12; i >= 1; i--)
		deck[i]->next = deck[i - 1];
	deck[0]->next = NIL;
	found[0] = deck[13];
	deck[13]->next = NIL;
	for (i = 1; i < 4; i++)
		found[i] = NIL;
	basecard = found[0];
	for (i = 14; i < 18; i++) {
		tableau[i - 14] = deck[i];
		deck[i]->next = NIL;
	}
	for (i = 0; i < 4; i++) {
		bottom[i] = tableau[i];
		length[i] = tabrow;
	}
	hand = deck[18];
	for (i = 18; i < decksize - 1; i++)
		deck[i]->next = deck[i + 1];
	deck[decksize - 1]->next = NIL;
	talon = NIL;
	base = 0;
	cinhand = 34;
	taloncnt = 0;
	timesthru = 0;
	cardsoff = 1;
	coldrow = ctoprow;
	coldcol = cinitcol;
	cnewrow = ctoprow;
	cnewcol = cinitcol + cwidthcol;
}

/*
 * procedure to print the beginning cards and to start each game 
 */
startgame()
{
	register int    j;

	shuffle();
	initgame();
	this.hand = costofhand;
	total.hand += costofhand;
	this.inspection = 0;
	this.game = 0;
	this.runs = 0;
	this.information = 0;
	this.wins = 0;
	this.thinktime = 0;
	infullgame = FALSE;
	startedgame = FALSE;
	printcard(foundcol, foundrow, found[0]);
	printcard(stockcol, stockrow, stock);
	printcard(atabcol, tabrow, tableau[0]);
	printcard(btabcol, tabrow, tableau[1]);
	printcard(ctabcol, tabrow, tableau[2]);
	printcard(dtabcol, tabrow, tableau[3]);
	if (PIPE == FALSE) {
		printcard(taloncol, talonrow, talon);
		move(foundrow - 2, basecol);
		(void) printw("Base");
		move(foundrow - 1, basecol);
		(void) printw("Rank");
		printrank(basecol, foundrow, found[0], 0);
	}
	for (j = 0; j <= 3; j++)
		fndbase(&tableau[j], pilemap[j], tabrow);
	fndbase(&stock, stockcol, stockrow);
	showstat();		/* show card counting info to cheaters */
	movetotalon();
	updatebettinginfo();
}

/*
 * procedure to clear the message printed from an error 
 */
clearmsg()
{
	int             i;

	if (errmsg == TRUE) {
		errmsg = FALSE;
		if (PIPE == FALSE) {
			move(msgrow, msgcol);
			for (i = 0; i < 25; i++)
				addch(' ');
			refresh();
		}
	}
}

/*
 * procedure to print an error message if the move is not listed 
 */
dumberror()
{
	errmsg = TRUE;
	if (PIPE == FALSE) {
		move(msgrow, msgcol);
		(void) printw("Not a proper move       ");
	} else
		(void) printf("NG  \n");
}

/*
 * procedure to print an error message if the move is not possible 
 */
destinerror()
{
	errmsg = TRUE;
	if (PIPE == FALSE) {
		move(msgrow, msgcol);
		(void) printw("Error: Can't move there");
	} else
		(void) printf("NG  \n");
}

/*
 * function to see if the source has cards in it 
 */
bool
notempty(cp)
	struct cardtype *cp;
{
	if (cp == NIL) {
		errmsg = TRUE;
		move(msgrow, msgcol);
		(void) printw("Error: no cards to move");
		return (FALSE);
	} else
		return (TRUE);
}

/*
 * function to see if the rank of one card is less than another 
 */
bool
ranklower(cp1, cp2)
	struct cardtype *cp1, *cp2;
{
	if (cp2->rank == Ace)
		if (cp1->rank == King)
			return (TRUE);
		else
			return (FALSE);
	else if (cp1->rank + 1 == cp2->rank)
		return (TRUE);
	else
		return (FALSE);
}

/*
 * function to check the cardcolor for moving to a tableau 
 */
bool
diffcolor(cp1, cp2)
	struct cardtype *cp1, *cp2;
{
	if (cp1->color == cp2->color)
		return (FALSE);
	else
		return (TRUE);
}

/*
 * function to see if the card can move to the tableau 
 */
bool
tabok(cp, des)
	struct cardtype *cp;
{
	if ((cp == stock) && (tableau[des] == NIL))
		return (TRUE);
	else if (tableau[des] == NIL)
		if (stock == NIL && 
		    cp != bottom[0] && cp != bottom[1] && 
		    cp != bottom[2] && cp != bottom[3])
			return (TRUE);
		else
			return (FALSE);
	else if (ranklower(cp, tableau[des]) && diffcolor(cp, tableau[des]))
		return (TRUE);
	else
		return (FALSE);
}

/*
 * procedure to turn the cards onto the talon from the deck 
 */
movetotalon()
{
	int             i, fin;

	if (PIPE == FALSE) {
		if (cinhand <= 3 && cinhand > 0) {
			move(msgrow, msgcol);
			(void) printw("Hand is now empty        ");
		}
	}
	if (cinhand >= 3)
		fin = 3;
	else if (cinhand > 0)
		fin = cinhand;
	else if (talon != NIL) {
		timesthru++;
		errmsg = TRUE;
		if (PIPE == FALSE)
			move(msgrow, msgcol);
		if (timesthru != 4) {
			if (PIPE == FALSE)
				(void) printw("Talon is now the new hand");
			else
				(void) printf("HAND\n");
			this.runs += costofrunthroughhand;
			total.runs += costofrunthroughhand;
			while (talon != NIL) {
				transit(&talon, &hand);
				cinhand++;
			}
			if (cinhand >= 3)
				fin = 3;
			else
				fin = cinhand;
			taloncnt = 0;
			coldrow = ctoprow;
			coldcol = cinitcol;
			cnewrow = ctoprow;
			cnewcol = cinitcol + cwidthcol;
			if (PIPE == FALSE) {
				clearstat();
				showstat();
			}
		} else {
			fin = 0;
			done = TRUE;
			if (PIPE == FALSE) {
				(void) printw("I believe you have lost");
				refresh();
			} else
				(void) printf("LOST\n");
			sleep(5);
		}
	} else {
		errmsg = TRUE;
		if (PIPE == FALSE) {
			move(msgrow, msgcol);
			(void) printw("Talon and hand are empty");
		} else
			(void) printf("EMPT\n");
		fin = 0;
	}
	for (i = 0; i < fin; i++) {
		transit(&hand, &talon);
		INCRHAND(cnewrow, cnewcol);
		INCRHAND(coldrow, coldcol);
		removecard(cnewcol, cnewrow);
		if (i == fin - 1)
			talon->visible = TRUE;
		if (Cflag) {
			if (talon->paid == FALSE && talon->visible == TRUE) {
				this.information += costofinformation;
				total.information += costofinformation;
				talon->paid = TRUE;
			}
			printcard(coldcol, coldrow, talon);
		}
	}
	if (fin != 0) {
		printcard(taloncol, talonrow, talon);
		cinhand -= fin;
		taloncnt += fin;
		if (Cflag) {
			move(handstatrow, handstatcol);
			(void) printw("%3d", cinhand);
			move(talonstatrow, talonstatcol);
			(void) printw("%3d", taloncnt);
		}
		fndbase(&talon, taloncol, talonrow);
	}
}


/*
 * procedure to print card counting info on screen 
 */
showstat()
{
	int             row, col;
	register struct cardtype *ptr;

	if (PIPE)
		return;
	if (!Cflag)
		return;
	move(talonstatrow, talonstatcol - 7);
	(void) printw("Talon: %3d", taloncnt);
	move(handstatrow, handstatcol - 7);
	(void) printw("Hand:  %3d", cinhand);
	move(stockstatrow, stockstatcol - 7);
	(void) printw("Stock: %3d", stockcnt);
	for (row = coldrow, col = coldcol, ptr = talon;
	     ptr != NIL;
	     ptr = ptr->next) {
		if (ptr->paid == FALSE && ptr->visible == TRUE) {
			ptr->paid = TRUE;
			this.information += costofinformation;
			total.information += costofinformation;
		}
		printcard(col, row, ptr);
		DECRHAND(row, col);
	}
	for (row = cnewrow, col = cnewcol, ptr = hand;
	     ptr != NIL;
	     ptr = ptr->next) {
		if (ptr->paid == FALSE && ptr->visible == TRUE) {
			ptr->paid = TRUE;
			this.information += costofinformation;
			total.information += costofinformation;
		}
		INCRHAND(row, col);
		printcard(col, row, ptr);
	}
}

/*
 * procedure to clear card counting info from screen 
 */
clearstat()
{
	int             row;

	if (PIPE)
		return;
	move(talonstatrow, talonstatcol - 7);
	(void) printw("          ");
	move(handstatrow, handstatcol - 7);
	(void) printw("          ");
	move(stockstatrow, stockstatcol - 7);
	(void) printw("          ");
	for (row = ctoprow; row <= cbotrow; row++) {
		move(row, cinitcol);
		(void) printw("%56s", " ");
	}
}

/*
 * procedure to update card counting base 
 */
usedtalon()
{
	removecard(coldcol, coldrow);
	DECRHAND(coldrow, coldcol);
	if (talon != NIL && (talon->visible == FALSE)) {
		talon->visible = TRUE;
		if (Cflag) {
			this.information += costofinformation;
			total.information += costofinformation;
			talon->paid = TRUE;
			printcard(coldcol, coldrow, talon);
		}
	}
	taloncnt--;
	if (Cflag) {
		move(talonstatrow, talonstatcol);
		(void) printw("%3d", taloncnt);
	}
}

/*
 * procedure to update stock card counting base 
 */
usedstock()
{
	stockcnt--;
	if (Cflag) {
		move(stockstatrow, stockstatcol);
		(void) printw("%3d", stockcnt);
	}
}

/*
 * let 'em know how they lost! 
 */
showcards()
{
	register struct cardtype *ptr;
	int             row;

	if (!Cflag || cardsoff == 52)
		return;
	for (ptr = talon; ptr != NIL; ptr = ptr->next) {
		ptr->visible = TRUE;
		ptr->paid = TRUE;
	}
	for (ptr = hand; ptr != NIL; ptr = ptr->next) {
		ptr->visible = TRUE;
		ptr->paid = TRUE;
	}
	showstat();
	move(stockrow + 1, sidecol);
	(void) printw("     ");
	move(talonrow - 2, sidecol);
	(void) printw("     ");
	move(talonrow - 1, sidecol);
	(void) printw("     ");
	move(talonrow, sidecol);
	(void) printw("     ");
	move(talonrow + 1, sidecol);
	(void) printw("     ");
	for (ptr = stock, row = stockrow; ptr != NIL; ptr = ptr->next, row++) {
		move(row, stockcol - 1);
		(void) printw("|   |");
		printcard(stockcol, row, ptr);
	}
	if (stock == NIL) {
		move(row, stockcol - 1);
		(void) printw("|   |");
		row++;
	}
	move(handstatrow, handstatcol - 7);
	(void) printw("          ");
	move(row, stockcol - 1);
	(void) printw("=---=");
	if ( cardsoff == 52 )
		getcmd(moverow, movecol, "Hit return to exit");
}

/*
 * procedure to update the betting values 
 */
updatebettinginfo()
{
	long            thiscosts, totalcosts;
	time_t          now;
	register long   dollars;

	(void) time(&now);
	dollars = (now - acctstart) / secondsperdollar;
	if (dollars > 0) {
		acctstart += dollars * secondsperdollar;
		if (dollars > maxtimecharge)
			dollars = maxtimecharge;
		this.thinktime += dollars;
		total.thinktime += dollars;
	}
	thiscosts = this.hand + this.inspection + this.game +
		this.runs + this.information + this.thinktime;
	totalcosts = total.hand + total.inspection + total.game +
		total.runs + total.information + total.thinktime;
	this.worth = this.wins - thiscosts;
	total.worth = total.wins - totalcosts;
	if (status != BETTINGBOX)
		return;
	move(tboxrow + 3, boxcol + 13);
	(void) printw("%4d%9d", this.hand, total.hand);
	move(tboxrow + 4, boxcol + 13);
	(void) printw("%4d%9d", this.inspection, total.inspection);
	move(tboxrow + 5, boxcol + 13);
	(void) printw("%4d%9d", this.game, total.game);
	move(tboxrow + 6, boxcol + 13);
	(void) printw("%4d%9d", this.runs, total.runs);
	move(tboxrow + 7, boxcol + 13);
	(void) printw("%4d%9d", this.information, total.information);
	move(tboxrow + 8, boxcol + 13);
	(void) printw("%4d%9d", this.thinktime, total.thinktime);
	move(tboxrow + 9, boxcol + 13);
	(void) printw("%4d%9d", thiscosts, totalcosts);
	move(tboxrow + 10, boxcol + 13);
	(void) printw("%4d%9d", this.wins, total.wins);
	move(tboxrow + 11, boxcol + 13);
	(void) printw("%4d%9d", this.worth, total.worth);
}

/*
 * procedure to move a card from the stock or talon to the tableau 
 */
simpletableau(cp, des)
	struct cardtype **cp;
{
	int             origin;

	if (notempty(*cp)) {
		if (tabok(*cp, des)) {
			if (*cp == stock)
				origin = stk;
			else
				origin = tal;
			if (tableau[des] == NIL)
				bottom[des] = *cp;
			transit(cp, &tableau[des]);
			length[des]++;
			printcard(pilemap[des], length[des], tableau[des]);
			timesthru = 0;
			if (origin == stk) {
				usedstock();
				printcard(stockcol, stockrow, stock);
			} else {
				usedtalon();
				printcard(taloncol, talonrow, talon);
			}
		} else
			destinerror();
	}
}

/*
 * print the tableau 
 */
tabprint(sour, des)
{
	int             dlength, slength, i;
	struct cardtype *tempcard;

	for (i = tabrow; i <= length[sour]; i++)
		removecard(pilemap[sour], i);
	dlength = length[des] + 1;
	slength = length[sour];
	if (slength == tabrow)
		printcard(pilemap[des], dlength, tableau[sour]);
	else
		while (slength != tabrow - 1) {
			tempcard = tableau[sour];
			for (i = 1; i <= slength - tabrow; i++)
				tempcard = tempcard->next;
			printcard(pilemap[des], dlength, tempcard);
			slength--;
			dlength++;
		}
}

/*
 * procedure to move from the tableau to the tableau 
 */
tabtotab(sour, des)
	register int sour, des;
{
	struct cardtype *temp;

	if (notempty(tableau[sour])) {
		if (tabok(bottom[sour], des)) {
			tabprint(sour, des);
			temp = bottom[sour];
			bottom[sour] = NIL;
			if (bottom[des] == NIL)
				bottom[des] = temp;
			temp->next = tableau[des];
			tableau[des] = tableau[sour];
			tableau[sour] = NIL;
			length[des] = length[des] + (length[sour] - (tabrow - 1));
			length[sour] = tabrow - 1;
			timesthru = 0;
		} else
			destinerror();
	}
}

/*
 * functions to see if the card can go onto the foundation 
 */
bool
rankhigher(cp, let)
	struct cardtype *cp;
{
	if (found[let]->rank == King)
		if (cp->rank == Ace)
			return (TRUE);
		else
			return (FALSE);
	else if (cp->rank - 1 == found[let]->rank)
		return (TRUE);
	else
		return (FALSE);
}

/*
 * function to determine if two cards are the same suit 
 */
samesuit(cp, let)
	struct cardtype *cp;
{
	if (cp->suit == found[let]->suit)
		return (TRUE);
	else
		return (FALSE);
}

/*
 * procedure to move a card to the correct foundation pile 
 */
movetofound(cp, source)
	struct cardtype **cp;
{
	tempbase = 0;
	mtfdone = FALSE;
	if (notempty(*cp)) {
		do {
			if (found[tempbase] != NIL)
				if (rankhigher(*cp, tempbase)
				    && samesuit(*cp, tempbase)) {
					if (*cp == stock)
						mtforigin = stk;
					else if (*cp == talon)
						mtforigin = tal;
					else
						mtforigin = tab;
					transit(cp, &found[tempbase]);
					printcard(pilemap[tempbase],
						  foundrow, found[tempbase]);
					timesthru = 0;
					if (mtforigin == stk) {
						usedstock();
						printcard(stockcol, stockrow, stock);
					} else if (mtforigin == tal) {
						usedtalon();
						printcard(taloncol, talonrow, talon);
					} else {
						removecard(pilemap[source], length[source]);
						length[source]--;
					}
					cardsoff++;
					if (infullgame) {
						this.wins += valuepercardup;
						total.wins += valuepercardup;
					}
					mtfdone = TRUE;
				} else
					tempbase++;
			else
				tempbase++;
		} while ((tempbase != 4) && !mtfdone);
		if (!mtfdone)
			destinerror();
	}
}
/*
 * get a command via the pipe 
 */
get_pipe_cmd(cmd)
	char            cmd[2];
{
#ifdef DEBUG
	(void) fprintf(stderr, "cf: get_pipe_cmd()\n");
#endif

	(void) scanf("%c%c", &cmd[0], &cmd[1]);

#ifdef DEBUG
	(void) fprintf(stderr, "cf: command received is %c%c\n", cmd[0], cmd[1]);
#endif
}
/*
 * procedure to get a command 
 */
getcmd(row, col, cp)
	int             row, col;
	char           *cp;
{
	char            cmd[2], ch;
	int             i;

	i = 0;
	if (PIPE == TRUE)
		get_pipe_cmd(cmd);
	else {
		move(row, col);
		(void) printw("%-24s", cp);
		col += 1 + strlen(cp);
		move(row, col);
		refresh();
		do {
			ch = getch() & 0177;
			if (ch >= 'A' && ch <= 'Z')
				ch += ('a' - 'A');
			if (ch == '\f') {
				(void) wrefresh(curscr);
				refresh();
			} else if (i >= 2 && ch != erasechar() && ch != killchar()) {
				if (ch != '\n' && ch != '\r' && ch != ' ')
					(void) write(1, "\007", 1);
			} else if (ch == erasechar() && i > 0) {
				(void) printw("\b \b");
				refresh();
				i--;
			} else if (ch == killchar() && i > 0) {
				while (i > 0) {
					(void) printw("\b \b");
					i--;
				}
				refresh();
#ifndef SIGTSTP
			} else if (ch == '\032') {	/* Control-Z */
				suspend();
				move(row, col + i);
				refresh();
#endif
			} else if (isprint(ch)) {
				cmd[i++] = ch;
				addch(ch);
				refresh();
			}
		} while (ch != '\n' && ch != '\r' && ch != ' ');
	}
	srcpile = cmd[0];
	destpile = cmd[1];
#ifdef DEBUG
	(void) fprintf(stderr, "cf: leaving getcmd\n");
#endif
}

#ifndef SIGTSTP
/*
 * Suspend the game (shell escape if no process control on system) 
 */
suspend()
{
	char           *getenv();
	char           *sh;
	register int	pid;
	register int	wpid;
	register int	(*istat)(), (*qstat)();

	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
	fflush(stdout);
	sh = getenv("SHELL");
	if (sh == NULL)
		sh = "/bin/sh";
	if ((pid = fork()) == 0) {
		execlp(sh, sh, (char *)0);
		exit(127);
	}
	if (pid != 0) {
		istat = signal(SIGINT, SIG_IGN);
		qstat = signal(SIGQUIT, SIG_IGN);
		while((wpid = wait((int *)NULL)) != pid && wpid != -1)
			;
		(void) signal(SIGINT, istat);
		(void) signal(SIGQUIT, qstat);
	}		
	crmode();
	noecho();
	clearok(curscr, TRUE);
	wrefresh(curscr);
}
#endif

/*
 * procedure to evaluate and make the specific moves 
 */
movecard()
{
	int             source, dest;
	char            osrcpile, odestpile;

	done = FALSE;
	errmsg = FALSE;
	do {
		if (talon == NIL && hand != NIL)
			movetotalon();
		if (cardsoff == 52) {
			if (PIPE == FALSE)
				refresh();
			srcpile = 'q';
		} else
			getcmd(moverow, movecol, "Move:");
		clearmsg();
		if (srcpile >= '1' && srcpile <= '4')
			source = (int) (srcpile - '1');
		if (destpile >= '1' && destpile <= '4')
			dest = (int) (destpile - '1');
		if (!startedgame &&
		    (srcpile == 't' || srcpile == 's' || srcpile == 'h' ||
		     srcpile == '1' || srcpile == '2' || srcpile == '3' ||
		     srcpile == '4')) {
			startedgame = TRUE;
			osrcpile = srcpile;
			odestpile = destpile;
			if (status != BETTINGBOX)
				srcpile = 'y';
			else
				do {
					getcmd(moverow, movecol, "Inspect game?");
				} while (srcpile != 'y' && srcpile != 'n');
			if (srcpile == 'n') {
				srcpile = 'q';
			} else {
				this.inspection += costofinspection;
				total.inspection += costofinspection;
				srcpile = osrcpile;
				destpile = odestpile;
			}
		}
		if (PIPE) {
			if ((srcpile == 't') && (taloncnt == 0)) {
				(void) printf("TALO\n");
				goto m_done;
			}
			if ((srcpile == 's') && (stockcnt == 0)) {
				(void) printf("STOC\n");
				goto m_done;
			}
		}
		switch (srcpile) {
		case 't':
			if (destpile == 'f' || destpile == 'F')
				movetofound(&talon, source);
			else if (destpile >= '1' && destpile <= '4')
				simpletableau(&talon, dest);
			else
				dumberror();
			break;
		case 's':
			if (destpile == 'f' || destpile == 'F')
				movetofound(&stock, source);
			else if (destpile >= '1' && destpile <= '4')
				simpletableau(&stock, dest);
			else
				dumberror();
			break;
		case 'h':
			if (destpile != 't' && destpile != 'T') {
				dumberror();
				break;
			}
			if (infullgame) {
				movetotalon();
				break;
			}
			if (status == BETTINGBOX) {
				if (PIPE == FALSE) {
					do {
						getcmd(moverow, movecol,
						       "Buy game?");
					} while (srcpile != 'y' &&
						 srcpile != 'n');
					if (srcpile == 'n') {
						showcards();
						done = TRUE;
						break;
					}
				}
			}
			infullgame = TRUE;
			this.wins += valuepercardup * cardsoff;
			total.wins += valuepercardup * cardsoff;
			this.game += costofgame;
			total.game += costofgame;
			movetotalon();
			break;
		case 'q':
			showcards();
			done = TRUE;
			break;
		case 'b':
			printtopbettingbox();
			printbottombettingbox();
			status = BETTINGBOX;
			break;
		case 'x':
			clearabovemovebox();
			clearbelowmovebox();
			status = NOBOX;
			break;
		case 'i':
			printtopinstructions();
			printbottominstructions();
			status = INSTRUCTIONBOX;
			break;
		case 'c':
			Cflag = !Cflag;
			if (Cflag)
				showstat();
			else
				clearstat();
			break;
		case '1':
		case '2':
		case '3':
		case '4':
			if (destpile == 'f' || destpile == 'F')
				movetofound(&tableau[source], source);
			else if (destpile >= '1' && destpile <= '4')
				tabtotab(source, dest);
			else
				dumberror();
			break;
		default:
			dumberror();
		}
m_done:
		fndbase(&stock, stockcol, stockrow);
		fndbase(&talon, taloncol, talonrow);
		updatebettinginfo();
	} while (!done);
#ifdef DEBUG
	(void) fprintf(stderr, "cf: exiting movecard -- quiting or winning\n");
#endif

}
/*
 * procedure to printout instructions 
 */
instruct()
{
	register char **cp;

	move(originrow, origincol);
	(void) printw("This is the game of solitaire called Canfield.  Do\n");
	(void) printw("you want instructions for the game?");
	do {
		getcmd(originrow + 3, origincol, "y or n?");
	} while (srcpile != 'y' && srcpile != 'n');
	if (srcpile == 'n')
		return;
	clear();
	for (cp = basicinstructions; *cp != 0; cp++)
		(void) printw(*cp);
	refresh();
	getch();
	clear();
	move(originrow, origincol);
	(void) printw("Do you want instructions for betting?");
	do {
		getcmd(originrow + 2, origincol, "y or n?");
	} while (srcpile != 'y' && srcpile != 'n');
	if (srcpile == 'n')
		return;
	clear();
	for (cp = bettinginstructions; *cp != 0; cp++)
		(void) printw(*cp);
	refresh();
	getch();
}

/*
 * procedure to initialize the game 
 */
initall()
{
	int             uid, i;
	long            lseek();

	srandom(getpid());
	(void) time(&acctstart);
	initdeck();
	uid = getuid();
	if (uid < 0)
		return;
	dbfd = open("/usr/games/lib/cfscores", 2);
	if (dbfd < 0)
		return;
	if (lseek(dbfd, (long)uid * sizeof (struct betinfo), 0) < 0) {
		(void) close(dbfd);
		dbfd = -1;
		return;
	}
	i = read(dbfd, (char *) &total, sizeof (total));
	if (i < 0) {
		(void) close(dbfd);
		dbfd = -1;
		return;
	}
	(void) lseek(dbfd, (long)uid * sizeof (struct betinfo), 0);
}

/*
 * procedure to end the game 
 */
bool
finish()
{
	int             row, col;

	if (PIPE == FALSE) {
		if (cardsoff == 52) {
			getcmd(moverow, movecol, "Hit return to exit");
			clear();
			refresh();
			move(originrow, origincol);
			(void) printw("CONGRATULATIONS!\n");
			(void) printw("You won the game. That is a feat to be proud of.\n");
			row = originrow + 5;
			col = origincol;
		} else {
			move(msgrow, msgcol);
			(void) printw("You got %d card", cardsoff);
			if (cardsoff > 1)
				(void) printw("s");
			(void) printw(" off    ");
			move(msgrow, msgcol);
			row = moverow;
			col = movecol;
		}
		do {
			getcmd(row, col, "Play again (y or n)?");
		} while (srcpile != 'y' && srcpile != 'n');
		errmsg = TRUE;
		clearmsg();
		if (srcpile == 'y')
			return (FALSE);
		else
			return (TRUE);
	} else {
#ifdef DEBUG
		(void) fprintf(stderr, "cf: finish() -- cardsoff == %d\n", cardsoff);
#endif
		if (cardsoff == 52) {
			(void) printf("WIN \n");
			return (TRUE);
		} else {
			return (TRUE);
		}
	}
	/*NOTREACHED*/
}

/*
 * Field an interrupt. 
 */
askquit()
{
	(void) signal(SIGINT, SIG_IGN);
	if (PIPE == FALSE) {
		move(msgrow, msgcol);
		(void) printw("Really wish to quit?    ");
		do {
			getcmd(moverow, movecol, "y or n?");
		} while (srcpile != 'y' && srcpile != 'n');
		clearmsg();
		if (srcpile == 'y')
			cleanup();
		(void) signal(SIGINT, askquit);
	}
}

/*
 * procedure to clean up and exit 
 */
cleanup()
{
#ifdef DEBUG
(void) fprintf(stderr, "cf: enterning cleanup\n");
#endif

	total.thinktime += 1;
	status = NOBOX;
	updatebettinginfo();
	if (dbfd != -1) {
		(void) write(dbfd, (char *) &total, sizeof (total));
		(void) close(dbfd);
	}
	if (PIPE == FALSE) {
		move(LINES - 1, 0);
		clrtoeol();
		refresh();
		endwin();
	}
#ifdef DEBUG
(void) fprintf(stderr, "cf: about to leave cleanup\n");
#endif
	exit(0);
	/* NOTREACHED */
}

/*
 * Can you tell that this used to be a Pascal program? 
 */
/*ARGSUSED*/
main(argc, argv)
	int             argc;
	char           *argv[];
{
#ifdef DEBUG
	(void) fprintf(stderr, "cf: entering main\n");
#endif
#ifdef MAXLOAD
	double          vec[3];

	loadav(vec);
	if (vec[2] >= MAXLOAD) {
		puts("The system load is too high.  Try again later.");
		exit(0);
	}
#endif
	(void) signal(SIGINT, askquit);
	(void) signal(SIGHUP, cleanup);
	(void) signal(SIGTERM, cleanup);
	if (!isatty(0)) {
		PIPE = TRUE;
		setbuf(stdout, (char *)NULL);
		setbuf(stdin, (char *)NULL);
	}
	if (PIPE == FALSE) {
		(void) initscr();
		crmode();
		noecho();
	}
	initall();
	if (PIPE == FALSE) {
		instruct();
	}
	makeboard();
	for (;;) {
		startgame();
#ifdef DEBUG
		(void) fprintf(stderr, "cf: entering movecard\n");
#endif
		movecard();
		if (finish())
			break;
		if (cardsoff == 52)
			makeboard();
		else
			cleanupboard();
	}
	cleanup();
	/* NOTREACHED */
}
