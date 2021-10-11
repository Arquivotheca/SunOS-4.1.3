#ifndef lint
static	char sccsid[] = "@(#)moo.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*  Game: MOO  */

#include <stdio.h>
#define SIZE  4
#define TEN   10

int nbulls, ncows, nattempts;
char number[SIZE], guess[TEN];

main(argc,argv)
char *argv[];

{
	extern char *optarg;
	extern int optind;
	int c,i,a;

	while ((c = getopt(argc,argv,"i"))!= EOF)
		switch(c)  {
		case 'i':
			instruct();
			break;

		case '?':
			printf("usage: moo [-i]\n");
			exit(2);
		}

	printf("MOO\n");
	for (;;) {
		printf("new game\n");
		nbulls = ncows = nattempts = 0;

		numgen();
		while (nbulls < 4) {
			nbulls = ncows = 0;
			for (i=0; i<SIZE; ++i) guess[i] = '\b';
			if (a = takeguess() == 1)
				exit(1);
			++nattempts;
			cmatch();
			printout();
		}

		printf("Attempts = %d\n", nattempts);
		continue;
	}
}

numgen()    /*  generate number consisting of four random digits  */
{
	int i,j,mark;

	i = 0;
	while (i < SIZE)  {
		mark = 0;
		number[i] = rand()%10 + '0';
		for ( j = i-1; j >= 0; --j)  {
			if (number[i] == number[j])  {
				mark = 1;
				break;
			}
		}
		if (mark == 0)
			++i;
	}
}

printout()
{
	printf("bulls = %d      cows = %d\n", nbulls,ncows);
}

takeguess()    /*  take input guess */
{
	int i,flag;
	char *s;

	s = guess;
	flag = 1;
	while (flag)  {
		flag = 0;
		printf("your guess?\n");
		if (gets(s) != NULL) {
			if (guess[0] == 'q' && guess[1] == '\0')
				return(1);
			for (i=0; i<SIZE; ++i)  {
				if (guess[i] == '\b' || guess[i]<'0' || guess[i]>'9') {
					flag = 1;
					printf("bad guess\n");
					break;
				}
			}
			if (guess[SIZE] != '\0')  {
				printf("bad guess\n");
				flag = 1;
			}
		}
		else
			return(1);
	}

}

cmatch()   /*  matching of player's guess and actual number  */
{
	int i,j;

	i=j=0;

	while(i < SIZE) {
		while(j < SIZE) {
			if (guess[i] != number[j])
				++j;
			else  {
				if (i == j)
					++nbulls;
				else
					++ncows;
				if (i <= SIZE - 1) {
					if (i == SIZE - 1) {
						++i;
						break;
					}
					++i;
					j = 0;
				}
			}
		}
		if (i < SIZE) {
			++i;
			j = 0;
		}
	}
}

    /*  set of instructions for the game  */
char *inst[] = {
	"How to play MOO:",
	"The computer selects a random number which consists of four",
	"different digits. The objective of the game is for the player",
	"to guess the correct digits and their correct positions. A",
	"correctly guessed digit and its position is called a bull.",
	"A cow is when a number is correctly guessed but not its position.",
	"A player correctly guesses the number when the number of bulls is",
	"equal to four. The number of attempts that the player took to",
	"guess is given at the end of each game. When a game is finished",
	"(bulls=4), another one begins immediately. If the player does not",
	"wish to continue playing, he or she should type control-C or",
	"type the character q.",
	"Have fun!",
	"",
	};

instruct()
{
	register char **cpp;

	printf("\n");

	for ( cpp = inst; **cpp != '\0'; ++cpp )
		printf("%s\n", *cpp);
}
