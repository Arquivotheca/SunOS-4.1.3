#ifndef lint
static	char sccsid[] = "@(#)move.c 1.1 92/07/30 SMI"; /* from UCB 4.1 07/04/83 */
#endif

/*************************************************************************
 *
 *	MOVE LIBRARY
 *
 *	This set of subroutines moves a cursor to a predefined
 *	location, independent of the terminal type.  If the
 *	terminal has an addressable cursor, it uses it.  If
 *	not, it optimizes for tabs (currently) even if you don't
 *      have them.
 *
 *	At all times the current address of the cursor must be maintained,
 *	and that is available as structure cursor.
 *
 *	The following calls are allowed:
 *		move(sp)	move to point sp.
 *		up()		move up one line.
 *		down()		move down one line.
 *		bs()		move left one space (except column 0).
 *		nd()		move right one space(no write).
 *		clear()		clear screen.
 *		home()		home.
 *		ll()		move to lower left corner of screen.
 *		cr()		carriage return (no line feed).
 *		printf()	just like standard printf, but keeps track
 *				of cursor position. (Uses pstring).
 *		aprintf()	same as printf, but first argument is &point.
 *				(Uses pstring).
 *		pstring(s)	output the string of printing characters.
 *				However, '\r' is interpreted to mean return
 *				to column of origination AND do linefeed.
 *				'\n' causes <cr><lf>.
 *		putpad(str)	calls tputs to output character with proper
 *					padding.
 *		outch()		the output routine for a character used by
 *					tputs. It just calls putchar.
 *		pch(ch)		output character to screen and update
 *					cursor address (must be a standard
 *					printing character). WILL SCROLL.
 *		pchar(ps,ch)	prints one character if it is on the
 *					screen at the specified location;
 *					otherwise, dumps it.(no wrap-around).
 *
 *		getcap()	initializes strings for later calls.
 *		cap(string)	outputs the string designated in the termcap
 *					data base. (Should not move the cursor.)
 *		done(int)	returns the terminal to intial state.  If int
 *					is not 0, it exits.
 *
 *		same(&p1,&p2)	returns 1 if p1 and p2 are the same point.
 *		point(&p,x,y)	return point set to x,y.
 *
 *		baudrate(x)	returns the baudrate of the terminal.
 *		delay(t)	causes an approximately constant delay
 *					independent of baudrate.
 *					Duration is ~ t/20 seconds.
 *
 ******************************************************************************/

#include "snake.h"

int CMlength;
int NDlength;
int BSlength;
int delaystr[10];
short ospeed;

static char str[80];

move(sp)
struct point *sp;
{
	int distance;
	int tabcol,ct;
	struct point z;

	if (sp->line <0 || sp->col <0 || sp->col > COLUMNS){
		printf("move to [%d,%d]?",sp->line,sp->col);
		return;
	}
	if (sp->line >= LINES){
		move(point(&z,sp->col,LINES-1));
		while(sp->line-- >= LINES)putchar('\n');
		return;
	}

	if (CM != 0) {
		char *cmstr = tgoto(CM, sp->col, sp->line);

		CMlength = strlen(cmstr);
		if(cursor.line == sp->line){
			distance = sp->col - cursor.col;
			if(distance == 0)return;	/* Already there! */
			if(distance > 0){	/* Moving to the right */
				if(distance*NDlength < CMlength){
					right(sp);
					return;
				}
				if(TA){
					ct=sp->col&7;
					tabcol=(cursor.col|7)+1;
					do{
						ct++;
						tabcol=(tabcol|7)+1;
					}
					while(tabcol<sp->col);
					if(ct<CMlength){
						right(sp);
						return;
					}
				}
			} else {		/* Moving to the left */
				if (-distance*BSlength < CMlength){
					gto(sp);
					return;
				}
			}
			if(sp->col < CMlength){
				cr();
				right(sp);
				return;
			}
				/* No more optimizations on same row. */
		}
		distance = sp->col - cursor.col;
		distance = distance > 0 ?
			distance*NDlength : -distance * BSlength;
if(distance < 0)printf("ERROR: distance is negative: %d",distance);
		distance += abs(sp->line - cursor.line);
		if(distance >= CMlength){
			putpad(cmstr);
			cursor.line = sp->line;
			cursor.col = sp->col;
			return;
		}
	}

	/*
	 * If we get here we have a terminal that can't cursor
	 * address but has local motions or one which can cursor
	 * address but can get there quicker with local motions.
	 */
	 gto(sp);
}
gto(sp)
struct point *sp;
{

	int distance,f,tfield,j;

	if (cursor.line > LINES || cursor.line <0 ||
	    cursor.col <0 || cursor.col > COLUMNS)
		printf("ERROR: cursor is at %d,%d\n",
			cursor.line,cursor.col);
	if (sp->line > LINES || sp->line <0 ||
	    sp->col <0 || sp->col >  COLUMNS)
		printf("ERROR: target is %d,%d\n",sp->line,sp->col);
	tfield = (sp->col) >> 3;
	if (sp->line == cursor.line){
		if (sp->col > cursor.col)right(sp);
		else{
			distance = (cursor.col -sp->col)*BSlength;
			if (((TA) && 
			     (distance > tfield+((sp->col)&7)*NDlength)
			    ) ||
			    (((cursor.col)*NDlength) < distance)
			   ){
				cr();
				right(sp);
			}
			else{
				while(cursor.col > sp->col) bs();
			}
		}
		return;
	}
				/*must change row */
	if (cursor.col - sp->col > (cursor.col >> 3)){
		if (cursor.col == 0)f = 0;
		else f = -1;
	}
	else f = cursor.col >> 3;
	if (((sp->line << 1) + 1 < cursor.line - f) && (HO != 0)){
			/*
			 * home quicker than rlf:
			 * (sp->line + f > cursor.line - sp->line)
			 */
		putpad(HO);
		cursor.col = cursor.line = 0;
		gto(sp);
		return;
	}
	if (((sp->line << 1) > cursor.line + LINES+1 + f) && (LL != 0)){
		/* home,rlf quicker than lf
		 * (LINES+1 - sp->line + f < sp->line - cursor.line) 
		 */
		if (cursor.line > f + 1){
		/* is home faster than wraparound lf?
		 * (cursor.line + 20 - sp->line > 21 - sp->line + f)
		 */
			ll();
			gto(sp);
			return;
		}
	}
	if ((LL != 0) && (sp->line > cursor.line + (LINES >> 1) - 1))
		cursor.line += LINES;
	while(sp->line > cursor.line)down();
	while(sp->line < cursor.line)up();
	gto(sp);		/*can recurse since cursor.line = sp->line */
}

right(sp)
struct point *sp;
{
	int field,tfield;
	int tabcol,strlength;

	if (sp->col < cursor.col)
		printf("ERROR:right() can't move left\n");
	if(TA){		/* If No Tabs: can't send tabs because ttydrive
			 * loses count with control characters.
			 */
		field = cursor.col >> 3;
/*
 *	This code is useful for a terminal which wraps around on backspaces.
 *	(Mine does.)  Unfortunately, this is not specified in termcap, and
 *	most terminals don't work that way.  (Of course, most terminals
 *	have addressible cursors, too).
 */
		if (BW && (CM == 0) &&
		    ((sp->col << 1) - field > (COLUMNS - 8) << 1 )
		   ){
	 		if (cursor.line == 0){  
	 			outch('\n');
	 		}
	 		outch('\r');
	 		cursor.col = COLUMNS + 1;
	 		while(cursor.col > sp->col)bs();
	 		if (cursor.line != 0) outch('\n');
	 		return;
	 	}

		tfield = sp->col >> 3;

		while (field < tfield){
			putpad(TA);
			cursor.col = ++field << 3;
		}
		tabcol = (cursor.col|7) + 1;
		strlength = (tabcol - sp->col)*BSlength + 1;
		/* length of sequence to overshoot */
		if (((sp->col - cursor.col)*NDlength > strlength) &&
		    (tabcol < COLUMNS)
		   ){
			/*
			 * Tab past and backup
			 */
			putpad(TA);
			cursor.col = (cursor.col | 7) + 1;
			while(cursor.col > sp->col)bs();
		}
	}
	while (sp->col > cursor.col){
		nd();
	}
}

cr(){
	outch('\r');
	cursor.col = 0;
}

clear(){
	int i;

	if (CL){
		putpad(CL);
		cursor.col=cursor.line=0;
	} else {
		for(i=0; i<LINES; i++) {
			putchar('\n');
		}
		cursor.line = LINES - 1;
		home();
	}
}

home(){
	struct point z;

	if(HO != 0){
		putpad(HO);
		cursor.col = cursor.line = 0;
		return;
	}
	z.col = z.line = 0;
	move(&z);
}

ll(){
	int j,l;
	struct point z;

	l = lcnt + 2;
	if(LL != NULL && LINES==l){
		putpad(LL);
		cursor.line = LINES-1;
		cursor.col = 0;
		return;
	}
	z.col = 0;
	z.line = l-1;
	move(&z);
}

up(){
	putpad(UP);
	cursor.line--;
}

down(){
	putpad(DO);
	cursor.line++;
	if (cursor.line >= LINES)cursor.line=LINES-1;
}
bs(){
	if (cursor.col > 0){
		putpad(BS);
		cursor.col--;
	}
}

nd(){
	putpad(ND);
	cursor.col++;
	if (cursor.col == COLUMNS+1){
		cursor.line++;
		cursor.col = 0;
		if (cursor.line >= LINES)cursor.line=LINES-1;
	}
}

pch(c)
{
	outch(c);
	if(++cursor.col >= COLUMNS && AM) {
		cursor.col = 0;
		++cursor.line;
	}
}

aprintf(ps,st,v0,v1,v2,v3,v4,v5,v6,v7,v8,v9)
struct point *ps;
char *st;
int v0,v1,v2,v3,v4,v5,v6,v7,v8,v9;

{
	struct point p;

	p.line = ps->line+1; p.col = ps->col+1;
	move(&p);
	sprintf(str,st,v0,v1,v2,v3,v4,v5,v6,v7,v8,v9);
	pstring(str);
}

printf(st,v0,v1,v2,v3,v4,v5,v6,v7,v8,v9)
char *st;
int v0,v1,v2,v3,v4,v5,v6,v7,v8,v9;
{
	sprintf(str,st,v0,v1,v2,v3,v4,v5,v6,v7,v8,v9);
	pstring(str);
}

pstring(s)
char *s;{
	struct point z;
	int stcol;

	stcol = cursor.col;
	while (s[0] != '\0'){
		switch (s[0]){
		case '\n':
			move(point(&z,0,cursor.line+1));
			break;
		case '\r':
			move(point(&z,stcol,cursor.line+1));
			break;
		case '\t':
			z.col = (((cursor.col + 8) >> 3) << 3);
			z.line = cursor.line;
			move(&z);
			break;
		case '\b':
			bs();
			break;
		case CTRL(g):
			outch(CTRL(g));
			break;
		default:
			if (s[0] < ' ')break;
			pch(s[0]);
		}
		s++;
	}
}

pchar(ps,ch)
struct point *ps;
char ch;{
	struct point p;
	p.col = ps->col + 1; p.line = ps->line + 1;
	if (
		(p.col >= 0) &&
		(p.line >= 0) &&
		(
			(
				(p.line < LINES) &&
				(p.col < COLUMNS)
			) ||
			(
	    			(p.col == COLUMNS) &&
				(p.line < LINES-1)
			)
	  	)
	){
		move(&p);
		pch(ch);
	}
}

			
outch(c)
{
	putchar(c);
}

putpad(str)
char *str;
{
	if (str)
		tputs(str, 1, outch);
}
baudrate()
{

	switch (orig.sg_ospeed){
	case B300:
		return(300);
	case B1200:
		return(1200);
	case B4800:
		return(4800);
	case B9600:
		return(9600);
	default:
		return(0);
	}
}
delay(t)
int t;
{
	int k,j;

	k = baudrate() * t / 300;
	for(j=0;j<k;j++){
		putchar(PC);
	}
}

done()
{
	cook();
	exit(0);
}

cook()
{
	delay(1);
	putpad(TE);
	putpad(KE);
	fflush(stdout);
	stty(0, &orig);
#ifdef TIOCSLTC
	ioctl(0, TIOCSLTC, &olttyc);
#endif
}

raw()
{
	stty(0, &new);
#ifdef TIOCSLTC
	ioctl(0, TIOCSLTC, &nlttyc);
#endif
}

same(sp1,sp2)
struct point *sp1, *sp2;
{
	if ((sp1->line == sp2->line) && (sp1->col == sp2->col))return(1);
	return(0);
}

struct point *point(ps,x,y)
struct point *ps;
int x,y;
{
	ps->col=x;
	ps->line=y;
	return(ps);
}

char *ap;

getcap()
{
	char *getenv();
	char *term;
	char *xPC;
	struct point z;
	int stop();

	term = getenv("TERM");
	if (term==0) {
		fprintf(stderr, "No TERM in environment\n");
		exit(1);
	}

	switch (tgetent(tbuf, term)) {
	case -1:
		fprintf(stderr, "Cannot open termcap file\n");
		exit(2);
	case 0:
		fprintf(stderr, "%s: unknown terminal", term);
		exit(3);
	}

	ap = tcapbuf;

	LINES = tgetnum("li");
	COLUMNS = tgetnum("co");
	lcnt = LINES;
	ccnt = COLUMNS - 1;

	AM = tgetflag("am");
	BW = tgetflag("bw");

	ND = tgetstr("nd", &ap);
	UP = tgetstr("up", &ap);

	DO = tgetstr("do", &ap);
	if (DO == 0)
		DO = "\n";

	BS = tgetstr("bc", &ap);
	if (BS == 0 && tgetflag("bs"))
		BS = "\b";
	if (BS)
		xBC = *BS;

	TA = tgetstr("ta", &ap);
	if (TA == 0 && tgetflag("pt"))
		TA = "\t";

	HO = tgetstr("ho", &ap);
	CL = tgetstr("cl", &ap);
	CM = tgetstr("cm", &ap);
	LL = tgetstr("ll", &ap);

	KL = tgetstr("kl", &ap);
	KR = tgetstr("kr", &ap);
	KU = tgetstr("ku", &ap);
	KD = tgetstr("kd", &ap);
	Klength = strlen(KL);
		/*	NOTE:   If KL, KR, KU, and KD are not
		 *		all the same length, some problems
		 *		may arise, since tests are made on
		 *		all of them together.
		 */

	TI = tgetstr("ti", &ap);
	TE = tgetstr("te", &ap);
	KS = tgetstr("ks", &ap);
	KE = tgetstr("ke", &ap);

	xPC = tgetstr("pc", &ap);
	if (xPC)
		PC = *xPC;

	NDlength = strlen(ND);
	BSlength = strlen(BS);
	if ((CM == 0) &&
		(HO == 0 | UP==0 || BS==0 || ND==0)) {
		fprintf(stderr, "Terminal must have addressible ");
		fprintf(stderr, "cursor or home + 4 local motions\n");
		exit(5);
	}
	if (tgetflag("os")) {
		fprintf(stderr, "Terminal must not overstrike\n");
		exit(5);
	}
	if (LINES <= 0 || COLUMNS <= 0) {
		fprintf(stderr, "Must know the screen size\n");
		exit(5);
	}

	gtty(0, &orig);
	new=orig;
	new.sg_flags &= ~(ECHO|CRMOD|ALLDELAY|XTABS);
	new.sg_flags |= CBREAK;
	signal(SIGINT,stop);
	ospeed = orig.sg_ospeed;
#ifdef TIOCGLTC
	ioctl(0, TIOCGLTC, &olttyc);
	nlttyc = olttyc;
	nlttyc.t_suspc = '\377';
	nlttyc.t_dsuspc = '\377';
#endif
	raw();

	if ((orig.sg_flags & XTABS) == XTABS) TA=0;
	putpad(KS);
	putpad(TI);
	point(&cursor,0,LINES-1);
}
