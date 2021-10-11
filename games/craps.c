#ifndef lint
static	char sccsid[] = "@(#)craps.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

# include	<stdio.h>
# include	<signal.h>

int	lent;
int	minus;
long	h_money;
long	drandno;
char	*get_bet();

static	char	lose[] =	"\nYou lose your bet of $%ld\n";
static	char	win[] =	"\nYou win with %d\n";
static	char	point[] =	"\nYou made your point of %d\n";

main()
{
	register int r;
	register int d;
	register int l;
	int	j;
	long	 money;
	long	patol();
	extern	int finish();
	char	line[BUFSIZ];
	char	*p;
	FILE	*iop;

	iop = stdin;
	h_money = 2000;
	signal(SIGINT,finish);
	time(&drandno);
	printf("Welcome to the Sun floating crap game.\n");
	printf("Commentary? ");
	if ((fgets(line,BUFSIZ,iop) == NULL))
		finish();
	if (line[0] == 'y') {
		printf("\nYou start out with $2,000.  If you  lose  it, the\n");
		printf("House will lend you an additional $2,000. A total\n");
		printf("of $20,000 will be lent.   Should  you  lose  all\n");
		printf("$22,000, you will be escorted from the casino.\n");
		printf("\nYou break the bank with $100,000 or more.\n\n");
	}

	gameloop:
	minus = lent = 0;
	h_money = 2000;
	while (1) {
		l = getdie();
		if (h_money >= 50000 && lent) {
			printf("You now have $%ld\n",h_money);
			printf("You owe the House money, it is ");
			printf("automatically reclaimed\n");
			printf("(The House needs working capital).\n");
			h_money = h_money - (lent * 2000);
			lent = 0;
		}
		if (h_money >= 100000) {
			broke();
			goto gameloop;
		}
		if (h_money <= 0) {
			if (lent == 10) {
				printf("You borrowed $20,000. ");
				printf("Don't you think that's ");
				printf("enough??\nGoodbye\n");
				exit (1);
			}
			printf("marker? ");
			fgets(line,BUFSIZ,iop);
			if (line[0] == 'y') {
				h_money = 2000;
				lent++;
			}
			else finish();
		}
		if (lent) {
			printf("You have %d marker(s) outstanding\n",lent);
			if (h_money > 2000) {
				printf("You now have $%ld\n",h_money);
				printf("Repay marker? ");
				if ((fgets(line,BUFSIZ,iop) == NULL))
					finish();
				if (line[0] == '\n' || line[0] == 'n')
					;
				else if (line[0] == 'y') {
					if (lent == 1) {
						h_money = h_money - 2000;
						lent--;
					}
					else how_many();
				}
			}
		}
		printf("Your bankroll is $%ld\n",h_money);
		if (p = get_bet())
			money = patol(p);
		else money = 2000000;
		while (money > h_money) {
			if (minus)
				--minus;
			printf("Illegal bet\n");
			if (p = get_bet())
				money = patol(p);
			else money = 2000000;
		}
		d = getdie();
		printf ("%d\t%d\n",l,d);
		r = d + l;
		if (r == 7 || r == 11) {
			if (minus) {
				printf(lose,money);
				h_money = h_money - money;
				--minus;
			}
			else {
				printf(win,r);
				h_money = h_money + money;
			}
		}
		else if (r == 2 || r == 3 || r == 12) {
			if (minus) {
				printf(win,r);
				h_money = h_money + money;
				--minus;
			}
			else {
				printf(lose,money);
				h_money = h_money - money;
			}
		}
		else {
			printf("The point is %d\n",r);
			j = 0;
			while (j != r) {
				l = getdie();
				d = getdie();
				printf ("\t%d\t%d\n",l,d);
				j = d + l;
				if (j == 7) {
					if (minus) {
						printf(win,j);
						h_money = h_money + money;
						--minus;
					}
					else {
						printf(lose,money);
						h_money = h_money - money;
					}
					break;
				}
			}
			if (j == r) {
				if (minus) {
					printf(lose,money);
					h_money = h_money - money;
					--minus;
				}
				else {
					printf(point,j);
					h_money = h_money + money;
				}
			}
		}
	}
}


getdie()
{
	register	int retval;

	do {
		drandno *= 2345699l;
		drandno += 12345701l;
	} while (!(retval = drandno % 7));
	if (retval < 0)
		retval += 7;
	return(retval);
}


finish()
{
	long	 money;

	money = ((lent * 2000) + 2000);
	h_money = h_money - money;
	if (h_money > 0)
		printf("\nYou win $%ld\n",h_money);
	else if (h_money == 0)
		printf("\nYou break even\n");
	else {
		h_money = (-h_money);
		printf("\nYou lose $%ld\n",h_money);
	}
	exit (1);
}


broke()
{
	char	line[BUFSIZ];
	FILE	*iop;

	iop = stdin;
	printf("\nYour luck is pretty good.  You broke the bank\n");
	printf("New game? ");
	fgets(line,BUFSIZ,iop);
	if (line[0] == 'y')
		return;
	exit (1);
}


a_number(l)
char	*l;
{
	register int j;

	for (j = 0; l[j] != '\0'; j++)
		if ('0' <= l[j] && l[j] <= '9')
			continue;
		else return(0);
	if (l[0] == '\0')
		return(0);
	return(1);
}


how_many()
{
	FILE *iop;
	register int repay;
	char	line[BUFSIZ];

	iop = stdin;
	repay = 0;
	while (repay <= lent) {
		printf("How many? ");
		if ((fgets(line,BUFSIZ,iop) == NULL)) {
			printf("\nYou refused to pay\n");
			finish();
		}
		line[strlen(line) - 1] = '\0';
		if (a_number(line)) {
			repay = atoi(line);
			if (repay > lent) {
				printf("You don't have that many markers\n");
				repay = 0;
				continue;
			}
			if (repay * 2000 > h_money) {
				printf("You don't have enough money\n");
				repay = 0;
				continue;
			}
			if (repay * 2000 == h_money) {
				printf("That wouldn't leave you much!!\n");
				repay = 0;
				continue;
			}
			h_money = h_money - (repay * 2000);
			lent = lent - repay;
			printf("You repayed %d marker(s)\n",repay);
			break;
		}
		else {
			printf("Illegal entry\n");
			repay = 0;
		}
	}
	return;
}


char	*get_bet()
{
	static char line[BUFSIZ];
	register char *p;
	FILE *iop;

	iop = stdin;
	printf("bet? ");
	if ((fgets(line,BUFSIZ,iop)) == NULL)
		finish();
	if (line[0] != '\n') {
		line[strlen(line) - 1] = '\0';
		p = line;
		if (line[0] == '-') {
			p++;
			minus++;
		}
		if (!a_number(p))
			return(0);
		else return(p);
	}
	else return(0);
}


long patol(s)
register char *s;
{
	long i;

	i = 0;
	while (*s >= '0' && *s <= '9')
		i = 10*i + *s++ - '0';

	if (*s)
		return(-1);
	return(i);
}
