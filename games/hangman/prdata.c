#ifndef lint
static	char sccsid[] = "%Z%%M% %I% %E% SMI"; /* from UCB */
#endif
# include	"hangman.h"

/*
 * prdata:
 *	Print out the current guesses
 */
prdata()
{
	register bool	*bp;

	move(GUESSY, GUESSX + sizeof "Guessed: ");
	bp = Guessed;
	while (bp < &Guessed[26])
		if (*bp++)
			addch((bp - Guessed) + 'a' - 1);
	clrtoeol();
	mvprintw(NUMBERY, NUMBERX + sizeof "Word #:          ", "%d", Wordnum);
	mvprintw(AVGY, AVGX + sizeof       "Current Average: ", "%.3f",
				(Average * (Wordnum - 1) + Errors) / Wordnum);
	mvprintw(AVGY + 1, AVGX + sizeof   "Overall Average: ", "%.3f", Average);
}
