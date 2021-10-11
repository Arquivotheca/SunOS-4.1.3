#include <stdio.h>

/*
 * Simulate tossing of coins according to I Ching
 * 6 tosses of three coins each. The result of each toss
 * is the sum of the values of the coins where heads is 2
 * and tails is 3.  Thus each toss is between 6 and 9, with
 * 7 and 8 being more probable than 6 and 9.
 * Output of this program is an argument to 'phx'
 */
main()
{
	int toss, result, coin;
	int c, seed = getpid();

	while ((c = getchar()) != EOF)
		seed += c;
	srand(seed);

	for (toss=0; toss<6; toss++) {
		result = 0;
		for (coin=0; coin<3; coin++)
			result += throw();
		putchar('0' + result);
	}
	putchar('\n');
	exit(0);
	/* NOTREACHED */
}

throw()
{
	return (2 + ((rand() >> 4) & 1));
}
