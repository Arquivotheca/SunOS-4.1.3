#ifndef lint
static	char sccsid[] = "@(#)bj.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
**	Compile as:  cc bj.c -o bj
*/

#include	<stdio.h>
#include	<signal.h>
#include	<setjmp.h>

#define		MINSHUF		8	/* minimum number of `shuffles' */
#define		MAXSHUF		30	/* maximum number of `shuffles' */

struct	card	{
	char	rank;			/* 2-9,	10=T, 11=J, 12=Q, 13=K,	14=A */
	char	suit;			/* 0=S,	1=H, 2=C, 3=D */
} deck[52];

char	curcrd[] = "XX";		/* current card */

int	nxtcrd;				/* next	card; index into `deck'	*/
long	action;				/* total bucks bet */
long	bux;				/* signed gain/loss */
int	ins;				/* player took insurance against bj */
int	shuff;				/* A shuffle has occured */
int	wager =	2;			/* bet per hand	*/
int	split;

jmp_buf	jbuf;				/* place to stash a stack frame */

main()
{
	register	r, s, aced;
	register	d1, d2, p1, p2, dtot;
	int		ptot[2];
	char		ddown[3];
	long		t;
	extern		leave();

		signal(SIGINT, leave);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT,	leave);
	time(&t);
	srandom(getpid() + (int)((t>>16) + t));	/* set random number seed */
	printf("Black Jack!\n\n");
	for(r =	2; r < 15; r++)			/* initialize the deck */
		for(s =	0; s < 4; s++)	{
			deck[nxtcrd].rank = r;
			deck[nxtcrd++].suit = s;
		}
	shuffle();				/* and shuffle it */
	setjmp(jbuf);	/* longjmps to here from win(), lose() & draw() */
	printf("New game\n");
	if(shuff) {
		printf("Shuffle\n");
		shuff = 0;
	}
	ins = split = aced = 0;
	d1 = getcard(0);
	printf(" up\n");
	d2 = getcard(1);
	strcpy(ddown, curcrd);
	if(d1 > 10)
		d1 = 10;
	if(d2 > 10)
		d2 = 10;
	dtot = d1 + d2;
	p1 = getcard(0);
	putchar('+');
	p2 = getcard(0);
	putchar('\n');
	if(d1 == 1)			/* dealer has an ace showing */
		if(getresp("Insurance"))
			ins++;
	if((aced = (d1 == 1 || d2 == 1)) && dtot == 11) { /* dealers got a bj */
		printf("Dealer has %s for blackjack!\n", ddown);
		if(ins)
			draw();
		else
			lose();
	}
	if(p1 == p2)
		if(getresp("Split pair"))
			split++;
	if(p1 > 10)
		p1 = 10;
	if(p2 > 10)
		p2 = 10;
	if(split) {
		wager *= 2;
		printf("First down card: ");
		p2 = getcard(0);
		if(p2 > 10)
			p2 = 10;
		putchar('\n');
		r = play(p1, p2);
		printf("Second down card: ");
		p2 = getcard(0);
		if(p2 > 10)
			p2 = 10;
		putchar('\n');
		s = play(p1, p2);
		if(r == 22 && s == 22)		/* two blackjacks! */
			win();
		if(r < 0 && s < 0)		/* two busts! */
			lose();
		ptot[0] = r;
		ptot[1] = s;
	}
	else
		switch(ptot[0] = play(p1, p2)) {
			case -1:
				lose();
			case 22:
				win();
		}
	printf("Dealer has %s", ddown);
	if(aced && dtot < 12 && dtot > 5)
		dtot += 10;
	while(dtot < 17) {
		putchar('+');
		d2 = getcard(0);
		if(d2 > 10)
			d2 = 10;
		if(d2 == 1 && !aced)
			aced = 1;
		dtot += d2;
		if(aced && dtot < 12 && dtot > 5)
			dtot += 10;
	}
	printf(" = ");
	if(dtot > 21) {
		printf("bust\n");
		win();
	}
	printf("%d\n", dtot);
	if(!(split && ptot[0] != ptot[1])) {
		if(ptot[0] > dtot)
			win();
		if(dtot > ptot[0])
			lose();
		draw();
	}
	if(dtot > ptot[0]) {
		if(dtot > ptot[1])
			lose();
		if(dtot == ptot[1]) {
			wager /= 2;
			lose();
		}
		draw();
	}
	if(ptot[0] > dtot) {
		if(ptot[1] > dtot)
			win();
		if(ptot[1] == dtot) {
			wager /= 2;
			win();
		}
		draw();
	}
	wager /= 2;
	if(dtot > ptot[1])
		lose();
	if(dtot < ptot[1])
		win();
	draw();
	/* NOTREACHED */
}

shuffle()
{
	register	i, j;
	struct card	temp;
	long		random();

	for (i = 52 - 1; i >= 0; i--) {
		j = random() % 52;
		if (i != j) {
			temp = deck[i];
			deck[i] = deck[j];
			deck[j] = temp;
		}
	}
	nxtcrd = 0;			/* reset deck index */
}

status()
{
	printf("\nAction $%ld\nYou're ", action);
	if(bux == 0)
		printf("even\n");
	else if(bux > 0)
		printf("up $%ld\n", bux);
	else
		printf("down $%ld\n", -bux);
}

getcard(f)
register f;
{
	register	r, s;
	static		char	brix[] = "TJQKA";
	static		char	suits[]	= "SHCD";

	if(nxtcrd > 51)			/* no more! */
		shuffle();
	r = deck[nxtcrd].rank;
	s = deck[nxtcrd++].suit;
	if(r > 9)
		curcrd[0] = brix[r - 10];
	else
		curcrd[0] = '0' + r;
	curcrd[1] = suits[s];
	if(!f)			/* not dealer's down card, so we can show it */
		printf("%s", curcrd);
	if(r ==	14)			/* ace */
		return(1);
	else
		return(r);
}

leave()
{
	status();
	printf("Bye!!\n");
	exit(0);
	/* NOTREACHED */
}

lose()
{
	action += wager;
	if(ins)			/* took insurance, dealer didn't have bj */
		wager += 1;
	printf("\nYou lose $%d\n", wager);
	bux -= wager;
	wager = 2;
	status();
	longjmp(jbuf, 0);
}

win()
{
	action += wager;
	if(ins)
		wager -= 1;
	printf("\nYou win $%d\n", wager);
	bux += wager;
	wager = 2;
	status();
	longjmp(jbuf, 0);
}

draw()
{
	action += wager;
	printf("\nYou break even\n");
	wager = 2;
	status();
	longjmp(jbuf, 0);
}

getresp(s)
char *s;
{
	char		buf[120];

	printf("\n%s? ", s);
	if (gets(buf) == NULL)
		leave();
	if(buf[0] == 'y')
		return(1);
	else
		return(0);
}

play(p1, p2)
register p1, p2;
{
	register	ptot, acep, p;

	ptot = p1 + p2;
	if((acep = (p1 == 1 || p2 == 1)) && ptot == 11 ) {
		printf("You have blackjack!\n");
		wager += 1;
		return(22);
	}
	if(ptot == 10 || ptot == 11)
		if(getresp("Double down")) {
			wager *= 2;
			p1 = getcard(0);
			putchar('\n');
			if(p1 > 10)
				p1 = 10;
			if(p1 == 1 && ptot == 10)
				p1 = 11;
			ptot += p1;
			goto stick;
		}
	while(getresp("Hit")) {
		p = getcard(0);
		putchar('\n');
		if(p == 1 && !acep)
			acep = 1;
		if(p > 10)
			p = 10;
		if((ptot += p) > 21) {
			printf("You bust!\n");
			return(-1);
		}
	}
stick:
	if(ptot < 12 && acep)
		ptot += 10;
	printf("You have %d\n\n", ptot);
	acep = 0;
	return(ptot);
}
