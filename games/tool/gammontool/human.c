#ifndef lint
static	char sccsid[] = "@(#)human.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sunwindow/win_input.h>
#include "defs.h"

#define INTERMED	8

int humanmoves[4][4];
int intpnts[3][2], intcntr;	/* intermediate points in a move */
int rollstack[4], rollcntr;	/* rolls used in a move */
int humannomove;		/* human couldn't move on last turn */

gammon_reader(ie)
struct inputevent *ie;
{
	static int movingpiece = 0, from, to, oldx = -1, oldy = -1;

	if (state != MOVE && ie->ie_code == LOC_WINEXIT)
		return;
	if (state != MOVE && ie->ie_code == LOC_MOVEWHILEBUTDOWN)
		return;
	if (win_inputposevent(ie)) {
		if (ie->ie_code != LOC_MOVEWHILEBUTDOWN)
			message(ERR, "");
		switch (state) {
		case STARTGAME:
			if (ie->ie_code == MS_LEFT || ie->ie_code == MS_MIDDLE)
				initgame();
			break;
		case ROLL:
			if (ie->ie_code == MS_LEFT || ie->ie_code == MS_MIDDLE) {
				rolldice(HUMAN);
				message(MSG, "Your turn to move.");
				starthumanturn();
			}
			break;
		case MOVE:
			if (ie->ie_code == MS_LEFT || ie->ie_code == MS_MIDDLE) {
				from = getpoint(ie->ie_locx, ie->ie_locy);
				if (legalstart(from))
					movingpiece = 1;
			} else if (ie->ie_code == LOC_MOVEWHILEBUTDOWN) {
				if (!movingpiece)
					break;
				if (oldx >= 0 && oldy >= 0)
					drawoutline(oldx, oldy);
				oldx = ie->ie_locx;
				oldy = ie->ie_locy;
				drawoutline(oldx, oldy);
				break;
			} else if (ie->ie_code == LOC_WINEXIT) {
				if (!movingpiece)
					break;
				movingpiece = 0;
				if (oldx >= 0 && oldy >= 0) {
					drawoutline(oldx, oldy);
					oldx = oldy = -1;
				}
			}
			break;
		case THINKING:
			message(ERR, "It's not your turn!");
			break;
		case GAMEOVER:
			message(ERR, "The game is over!");
			break;
		case COMPUTERDOUBLING:
			message(ERR, "You must accept or refuse the computer's double!");
			break;
		case HUMANDOUBLING:
			message(ERR, "Hold on, the computer is thinking about whether to accept your double!");
			break;
		} /* end switch */
	} else {	/* this is a negative event */
		if ((ie->ie_code == MS_LEFT || ie->ie_code == MS_MIDDLE) && movingpiece) {
			to = getpoint(ie->ie_locx, ie->ie_locy);
			if (oldx >= 0 || oldy >= 0) {
				drawoutline(oldx, oldy);
				oldx = oldy = -1;
			}
			movingpiece = 0;
			if (legalend(to))
				if (legalmove(from, to))
					domove(from, to);
		}
	}
}

legalstart(point)
int point;
{
	if (point == OUTOFBOUNDS || point == HOME)
		return(0);
	if (humanboard[point] == 0)
		return(0);
	if (point != BAR && humanboard[BAR] != 0) {
		message(ERR, "You must remove your barmen!");
		return(0);
	}
	return(1);
}

legalend(point)
int point;
{
	if (point == OUTOFBOUNDS || point == BAR)
		return(0);
	if (computerboard[point] > 1 && point != HOME) {
		message(ERR, "You cannot land on a blocked point!");
		return(0);
	}
	return(1);
}

legalmove(from, to)
int from, to;
{
	int rollneeded, dice[2], numdice, index, furthest, midpnt;

	if (to == HOME) {
		for (index = 24; index > 6; index--) {
			if (index == from && humanboard[index] == 1)
				continue;
			if (humanboard[index] > 0) {
				message(ERR, "You cannot bear off until all your pieces are in your inner table!");
				return(0);
			}
		}
	}
	clearintermediatepnts();
	if (from == to)
		return(0);
	if (from == BAR && to == HOME)	/* never possible, and it messes */
		return(0);		/* up the algorithm */
	if (from == BAR)
		rollneeded = 25 - to;
	else if (to == HOME)
		rollneeded = from;
	else
		rollneeded = from - to;
	if (rollneeded < 0) {
		message(ERR, "Backwards move!");
		return(0);
	}
        if (humanboard[BAR] > 1 && rollneeded == humandieroll[0] + humandieroll[1]) {
                message(ERR, "You must remove all of your barmen!");
                return(0);
	}
	if (to == HOME) {
		for (furthest = 6; furthest > 1; furthest--) {
			if (humanboard[furthest] > 0)
				break;
		}
	}
	if ((humandieroll[0] & NUMMASK) == (humandieroll[1] & NUMMASK)) {
		if ((humandieroll[0] & ROLLUSED) == 0)
			numdice = 4;
		else if ((humandieroll[0] & SECONDROLLUSED) == 0)
			numdice = 3;
		else if ((humandieroll[1] & ROLLUSED) == 0)
			numdice = 2;
		else
			numdice = 1;
		if (rollneeded % (humandieroll[0] & NUMMASK) == 0) {
			if (rollneeded / (humandieroll[0] & NUMMASK) > numdice) {
				message(ERR, "You didn't roll that move!");
				return(0);
			}
			midpnt = from;
			for (index = 0; index < numdice; index++) {
				if ((rollneeded -= (humandieroll[0] & NUMMASK)) <= 0) {
					useroll(humandieroll[0] & NUMMASK);
					break;
				}
				midpnt -= humandieroll[0] & NUMMASK;
				if (computerboard[midpnt] == 1) {
					addintermediatepnt(midpnt, 1);
				} else if (computerboard[midpnt] <= 0) {
					addintermediatepnt(midpnt, 0);
				} else {
					message(ERR, "You cannot touch down on a blocked point!");
					return(0);
				}
				useroll(humandieroll[0] & NUMMASK);
			}
			return(1);
		} else if (to == HOME) {
			if (numdice * (humandieroll[0] & NUMMASK) < rollneeded) {
				message(ERR, "You didn't roll that move!");
				return(0);
			}
			if (from != furthest) {
				message(ERR, "You didn't roll that move!");
				return(0);
			}
			midpnt = from;
			for (index = 0; index < numdice; index++) {
				if ((rollneeded -= (humandieroll[0] & NUMMASK)) <= 0) {
					useroll(humandieroll[0] & NUMMASK);
					break;
				}
				midpnt -= humandieroll[0] & NUMMASK;
				for (furthest = 6; furthest > 1; furthest--) {
					if (humanboard[furthest] > 0 && !(furthest == from && humanboard[from] == 1))
						break;
				}
				if (midpnt != furthest) {
					message(ERR, "You didn't roll that move!");
					return(0);
				}
				if (computerboard[midpnt] == 1) {
					addintermediatepnt(midpnt, 1);
				} else if (computerboard[midpnt] <= 0) {
					addintermediatepnt(midpnt, 0);
				} else {
					message(ERR, "You cannot touch down on a blocked point!");
					return(0);
				}
				useroll(humandieroll[0] & NUMMASK);
			}
			return(1);
		} else {
			message(ERR, "You didn't roll that move!");
			return(0);
		}
	} else {	/* not doubles */
		if (humandieroll[0] & ROLLUSED) {
			dice[0] = humandieroll[1] & NUMMASK;
			numdice = 1;
		} else if (humandieroll[1] & ROLLUSED) {
			dice[0] = humandieroll[0] & NUMMASK;
			numdice = 1;
		} else {
			dice[0] = humandieroll[0] & NUMMASK;
			dice[1] = humandieroll[1] & NUMMASK;
			numdice = 2;
		}
		if (rollneeded == dice[0]) {
			useroll(dice[0]);
			return(1);
		}
		if (numdice == 2 && rollneeded == dice[1]) {
			useroll(dice[1]);
			return(1);
		}
		if (numdice == 2 && rollneeded == dice[0] + dice[1]) {
			midpnt = from - (humandieroll[0] & NUMMASK);
			if (computerboard[midpnt] == 1) {
				useroll(dice[0]);
				useroll(dice[1]);
				addintermediatepnt(midpnt, 1);
				return(1);
			}
			if (computerboard[midpnt] <= 0) {
				useroll(dice[0]);
				useroll(dice[1]);
				addintermediatepnt(midpnt, 0);
				return(1);
			}
			midpnt = from - (humandieroll[1] & NUMMASK);
			if (computerboard[midpnt] == 1) {
				useroll(dice[0]);
				useroll(dice[1]);
				addintermediatepnt(midpnt, 1);
				return(1);
			}
			if (computerboard[midpnt] <= 0) {
				useroll(dice[0]);
				useroll(dice[1]);
				addintermediatepnt(midpnt, 0);
				return(1);
			}
			message(ERR, "You cannot touch down on a blocked point!");
			return(0);
		}
		if (to != HOME) {
			message(ERR, "You didn't roll that move!");
			return(0);
		}
		if (dice[0] > dice[1]) {
			if (dice[0] > rollneeded) {
				if (from != furthest) {
					message(ERR, "You didn't roll that move!");
					return(0);
				}
				useroll(dice[0]);
				return(1);
			}
		} else {
			if (dice[1] > rollneeded) {
				if (from != furthest) {
					message(ERR, "You didn't roll that move!");
					return(0);
				}
				useroll(dice[1]);
				return(1);
			}
		}
		if (numdice < 2) {
			message(ERR, "You didn't roll that move!");
			return(0);
		}
		if (dice[0] + dice[1] > rollneeded) {
			if (from != furthest) {
				message(ERR, "You didn't roll that move!");
				return(0);
			}
			midpnt = from - (humandieroll[0] & NUMMASK);
			for (furthest = 6; furthest > 1; furthest--) {
				if (humanboard[furthest] > 0 && !(furthest == from && humanboard[from] == 1))
					break;
			}
			if (midpnt != furthest) {
				message(ERR, "You didn't roll that move!");
				return(0);
			}
			if (computerboard[midpnt] == 1) {
				useroll(dice[0]);
				useroll(dice[1]);
				addintermediatepnt(midpnt, 1);
				return(1);
			}
			if (computerboard[midpnt] <= 0) {
				useroll(dice[0]);
				useroll(dice[1]);
				addintermediatepnt(midpnt, 0);
				return(1);
			}
			midpnt = from - (humandieroll[1] & NUMMASK);
			if (midpnt != furthest) {
				message(ERR, "You didn't roll that move!");
				return(0);
			}
			if (computerboard[midpnt] == 1) {
				useroll(dice[0]);
				useroll(dice[1]);
				addintermediatepnt(midpnt, 1);
				return(1);
			}
			if (computerboard[midpnt] <= 0) {
				useroll(dice[0]);
				useroll(dice[1]);
				addintermediatepnt(midpnt, 0);
				return(1);
			}
			message(ERR, "You cannot touch down on a blocked point!");
			return(0);
		}
		message(ERR, "You didn't roll that  move!");
		return(0);
	}
}

useroll(num)
int num;
{
	if (num == (humandieroll[0] & NUMMASK)) {
		if ((humandieroll[0] & ROLLUSED) == 0) {
			humandieroll[0] |= ROLLUSED;
			rollstack[rollcntr++] = humandieroll[0] & NUMMASK;
		} else if ((humandieroll[0] & SECONDROLLUSED) == 0) { /* must be dbles */
			humandieroll[0] |= SECONDROLLUSED;
			rollstack[rollcntr++] = humandieroll[0] & NUMMASK;
		} else if ((humandieroll[1] & ROLLUSED) == 0) {
			humandieroll[1] |= ROLLUSED;
			rollstack[rollcntr++] = humandieroll[1] & NUMMASK;
		} else {
			humandieroll[1] |= SECONDROLLUSED;
			rollstack[rollcntr++] = humandieroll[1] & NUMMASK;
		}
	} else {
		humandieroll[1] |= ROLLUSED;
		rollstack[rollcntr++] = humandieroll[1] & NUMMASK;
	}
}

domove(from, to)
int from, to;
{
	int ind = 0;

	humanmoves[movesmade][FROM] = from;
	if (intcntr) {
		for (ind = 0; ind < intcntr; ind++) {
			humanmoves[movesmade][TO] = intpnts[ind][0];
			if (intpnts[ind][1]) {
				movepiece(intpnts[ind][0], BAR, COMPUTER);
				humanmoves[movesmade][HITBLOT] = 1;
			} else {
				humanmoves[movesmade][HITBLOT] = 0;
			}
			humanmoves[movesmade][DIEUSED] = rollstack[ind] | INTERMED;
			humanmoves[++movesmade][FROM] = intpnts[ind][0];
		}
	}
	if (computerboard[to] == 1 && to != HOME) {
		movepiece(to, BAR, COMPUTER);
		humanmoves[movesmade][HITBLOT] = 1;
	} else {
		humanmoves[movesmade][HITBLOT] = 0;
	}
	humanmoves[movesmade][TO] = to;
	humanmoves[movesmade][DIEUSED] = rollstack[ind];
	movepiece(from, to, HUMAN);
	if (++movesmade == maxmoves || humanboard[HOME] == 15)
		endhumanturn();
}

int rollsleft, dicecopy[4], boardcopy[26], numdice;

starthumanturn()
{
	int index;

	/* find the maximum number of moves the human can make */

	boardcopy[BAR] = humanboard[BAR];
	for (index = 1; index < 25; index++) {
		boardcopy[25 - index] = humanboard[index];
		if (computerboard[index] > 1)
			boardcopy[25 - index] = BLOCKED;
	}
	dicecopy[0] = humandieroll[0];
	dicecopy[1] = humandieroll[1];
	if (humandieroll[0] == humandieroll[1]) {
		dicecopy[2] = dicecopy[3] = dicecopy[0];
		numdice = 4;
		rollsleft = 4;
	} else {
		numdice = 2;
		rollsleft = 2;
	}
	maxmoves = 0;
	findmaxrolls(0);
	if (!diddouble)
		sendtogammon(NODOUBLE);
	diddouble = 0;
	if (maxmoves > 0) {
		state = MOVE;
		setcursor(MOVE_CUR);
		movesmade = 0;
		humannomove = 0;
		sendtogammon(CANMOVE);
		sendtogammon(DIEROLL);
		sendtogammon((humandieroll[0] & NUMMASK) + '0');
		sendtogammon((humandieroll[1] & NUMMASK) + '0');
		lastmoved = HUMAN;
	} else {
		message(MSG, "You can't move...");
		humannomove = 1;
		setcursor(THINKING_CUR);
		sleep(2);
		message(MSG, "Thinking...");
		sendtogammon(CANTMOVE);
		state = THINKING;
		if (logfp != NULL)
			fprintf(logfp, "Human: (%d, %d) Can't move.\n", humandieroll[0], humandieroll[1]);
	}
}

findmaxrolls(movesdone)
int movesdone;
{
	int point, roll;

	if (boardcopy[BAR] > 0) {
		for (roll = 0; roll < numdice; roll++) {
			if (dicecopy[roll] & ROLLUSED)
				continue;
			if (boardcopy[dicecopy[roll]] != BLOCKED) {
				boardcopy[dicecopy[roll]]++;
				boardcopy[BAR]--;
				dicecopy[roll] |= ROLLUSED;
				if (++movesdone > maxmoves)
					maxmoves = movesdone;
				if (--rollsleft == 0)
					return(1);
				if (findmaxrolls(movesdone))
					return(1);
				dicecopy[roll] &= ~ROLLUSED;
				boardcopy[dicecopy[roll]]--;
				boardcopy[BAR]++;
				rollsleft++;
				movesdone--;
			}
		}
		if (boardcopy[BAR] > 0)
			return(0);
	}
	for (point = 1; point <= 24; point++) {
		if (boardcopy[point] <= 0)
			continue;
		for (roll = 0; roll < numdice; roll++) {
			if (dicecopy[roll] & ROLLUSED)
				continue;
			if (dicecopy[roll] + point >= 25) {
				int c;

				for (c = 1; c < 24; c++)
					if (boardcopy[c] > 0)
						break;
				if (c < 19)
					continue;
				if (dicecopy[roll] + point > 25 && point != c)
					continue;
				boardcopy[HOME]++;
			} else if (boardcopy[dicecopy[roll] + point] != BLOCKED) {
				boardcopy[dicecopy[roll] + point]++;
			} else {
				continue;
			}
			boardcopy[point]--;
			dicecopy[roll] |= ROLLUSED;
			if (++movesdone > maxmoves)
				maxmoves = movesdone;
			if (--rollsleft == 0)
				return(1);
			if (findmaxrolls(movesdone))
				return(1);
			dicecopy[roll] &= ~ROLLUSED;
			if (dicecopy[roll] + point >= 25)
				boardcopy[HOME]--;
			else
				boardcopy[dicecopy[roll] + point]--;
			boardcopy[point]++;
			rollsleft++;
			movesdone--;
		}
	}
	return(0);
}

endhumanturn()
{
	int c;

	if (humanboard[HOME] == 15) {
		if (computerboard[BAR] > 0) {
			win(HUMAN, BACKGAMMON);
		} else {
			for (c = 1; c < 25; c++)
				if (computerboard[c] > 0)
					break;
			if (c <= 6)
				win(HUMAN, BACKGAMMON);
			else if (computerboard[HOME] == 0)
				win(HUMAN, GAMMON);
			else
				win(HUMAN, STRAIGHT);
		}
	} else {
		sendhumanmove();
		message(MSG, "Thinking...");
		state = THINKING;
		setcursor(THINKING_CUR);
	}
}

undoonemove()
{
	if (movesmade == 0)
		return;
	movesmade--;
	movepiece(humanmoves[movesmade][TO], humanmoves[movesmade][FROM], HUMAN);
	if (humanmoves[movesmade][HITBLOT])
		movepiece(BAR, humanmoves[movesmade][TO], COMPUTER);

	/* painstakingly find which roll was used and reclaim it */

	if ((humandieroll[0] & NUMMASK) == (humandieroll[1] & NUMMASK)) {
		if (humandieroll[1] & SECONDROLLUSED) {
			humandieroll[1] &= ~SECONDROLLUSED;
		} else if (humandieroll[1] & ROLLUSED) {
			humandieroll[1] &= ~ROLLUSED;
		} else if (humandieroll[0] & SECONDROLLUSED) {
			humandieroll[0] &= ~SECONDROLLUSED;
		} else {
			humandieroll[0] &= ~ROLLUSED;
		}
	} else {
		if ((humandieroll[0] & NUMMASK) == (humanmoves[movesmade][DIEUSED] & ~INTERMED)) {
			humandieroll[0] &= ~ROLLUSED;
		} else {
			humandieroll[1] &= ~ROLLUSED;
		}
	}
	if (movesmade && (humanmoves[movesmade-1][DIEUSED] & INTERMED))
		undoonemove();
}

undowholemove()
{
	while (movesmade)
		undoonemove();
}

sendhumanmove()
{
	int c;

	for (c = 0; c < movesmade; c++) {
		if (c > 0)
			sendtogammon(',');
		if (humanmoves[c][FROM] == BAR) {
			sendtogammon('B');
		} else {
			if (humanmoves[c][FROM] > 9)
				sendtogammon(humanmoves[c][FROM] / 10 + '0');
			sendtogammon(humanmoves[c][FROM] % 10 + '0');
		}
		sendtogammon('-');
		if (humanmoves[c][TO] == HOME) {
			sendtogammon('H');
		} else {
			if (humanmoves[c][TO] > 9)
				sendtogammon(humanmoves[c][TO] / 10 + '0');
			sendtogammon(humanmoves[c][TO] % 10 + '0');
		}
	}
	sendtogammon('\n');
	if (logfp != NULL) {
		fprintf(logfp, "Human: (%d, %d) ", humandieroll[0] & NUMMASK, humandieroll[1] & NUMMASK);
		for (c = 0; c < movesmade; c++) {
			if (c > 0)
				fprintf(logfp, ", ");
			fprintf(logfp, "%d%c%d", humanmoves[c][FROM],
				humanmoves[c][HITBLOT] ? 'x' : '-',
				humanmoves[c][TO]);
		}
		putc('\n', logfp);
	}
}

showlasthumanmove()
{
	int index;

	if (movesmade == 0) {
		message(ERR, "Nobody has moved yet!");
		return;
	}
	if (!humannomove) {
		for (index = 0; index < movesmade; index++) {
			movepiece(humanmoves[index][TO], humanmoves[index][FROM], HUMAN);
			if (humanmoves[index][HITBLOT])
				movepiece(BAR, humanmoves[index][TO], COMPUTER);
		}
	}
	if (dicedisplayed == COMPUTER)
		showlastdice();
	sleep(2);
	if (!humannomove) {
		for (index--; index >= 0; index--) {
			if (humanmoves[index][HITBLOT])
				movepiece(humanmoves[index][TO], BAR, COMPUTER);
			movepiece(humanmoves[index][FROM], humanmoves[index][TO], HUMAN);
		}
	}
	if (dicedisplayed == COMPUTER)
		showlastdice();
}

clearintermediatepnts()
{
	int i;

	for (i = 0; i < 3; i++)
		intpnts[i][0] = intpnts[i][1] = 0;
	intcntr = 0;
	rollcntr = 0;
}

addintermediatepnt(point, hitblot)
int point, hitblot;
{
	intpnts[intcntr][0] = point;
	intpnts[intcntr][1] = hitblot;
	intcntr++;
}
