#ifndef lint
static	char sccsid[] = "@(#)computer.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <signal.h>
#include "defs.h"

int computermoves[4][3];	/* saved move */
int cmove = 0;
int togammonfd;		/* fd of pipe to write to backgammon */
char errbuf[512];	/* buffer for error messages from bkg */
int errind;
int compnomove;		/* computer couldn't move on last turn */

startgammon()
{
	int to[2], from[2], parentpid;

	if (gammonpid > 0) {
		kill(gammonpid, SIGINT);
		close(gammonfd);
		close(togammonfd);
		while (wait(0) > 0)
			;
	}

	if (pipe(to) < 0) {
		fprintf(stderr, "Pipe failed\n");
		exit(1);
	}
	if (pipe(from) < 0) {
		fprintf(stderr, "Pipe failed\n");
		exit(1);
	}
	parentpid = getpid();
	if ((gammonpid = fork()) < 0) {
		fprintf(stderr, "Fork failed\n");
		exit(1);
	}
	if (gammonpid == 0) {	/* child process */
		int fd;

		if (dup2(to[0], 0) < 0) {
			fprintf(stderr, "dup2 failed\n");
			kill(parentpid, 9);
			exit(1);
		}
		if (dup2(from[1], 1) < 0) {
			fprintf(stderr, "dup2 failed\n");
			kill(parentpid, 9);
			exit(1);
		}
		for (fd = 3; fd < 20; fd++)
			close(fd);
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGWINCH, SIG_DFL);
		if (execbkg) {
			if (execl(gammonbin, "bkg", 0) < 0) {
				fprintf(stderr, "Couldn't find %s\n", gammonbin);
				kill(parentpid, 9);
				exit(1);
			}
		} else {
			bkg();
		}
	}
	close(to[0]);
	close(from[1]);
	gammonfd = from[0];
	togammonfd = to[1];
	cmove = 0;
}

readfromgammon()
{
	int c;

	if (state == GAMEOVER || state == STARTGAME)
		return;
	switch (readchar()) {
	case ACCEPT_DBLE:
		if (state != HUMANDOUBLING) {
			message(ERR, "ERROR - computer tried to accept a double you didn't make");
			break;
		}
		message(MSG, "The computer accepts your double.  Your roll.");
		gamevalue *= 2;
		lastdoubled = HUMAN;
		drawcube();
		state = ROLL;
		break;
	case REFUSE_DBLE:
		if (state != HUMANDOUBLING) {
			message(ERR, "ERROR - computer tried to refuse a double you didn't make");
			break;
		}
		win(HUMAN, REFUSEDDBLE);
		break;
	case DOUBLEREQ:
		message(MSG, "The computer wants to double.  Do you accept?");
		state = COMPUTERDOUBLING;
		setcursor(DOUBLING_CUR);
		if (logfp != NULL)
			fprintf(logfp, "Computer doubled.\n");
		break;
	case NODOUBLE:
		startcomputerturn();
		break;
	case CANTMOVE:
		if (logfp != NULL)
			fprintf(logfp, "Computer: (%d, %d) Can't move.\n", computerdieroll[0], computerdieroll[1]);
		message(MSG, "The computer can't move.  Your roll.");
		message(ERR, "");
		state = ROLL;
		setcursor(ROLL_CUR);
		compnomove = 1;
		break;
	case CANMOVE:
		readcomputermove();
		message(ERR, "");
		lastmoved = COMPUTER;
		if (computerboard[HOME] == 15) {
			if (humanboard[BAR] > 0) {
				win(COMPUTER, BACKGAMMON);
			} else {
				for (c = 24; c > 0; c--)
					if (humanboard[c] > 0)
						break;
				if (c >= 19)
					win(COMPUTER, BACKGAMMON);
				else if (humanboard[HOME] == 0)
					win(COMPUTER, GAMMON);
				else
					win(COMPUTER, STRAIGHT);
			}
		} else {
			message(MSG, "Your roll.");
			state = ROLL;
			setcursor(ROLL_CUR);
		}
		break;
	case ERRMSG:
		strcpy(errbuf, "ERROR from bkg: ");
		errind = 16;
		while ((c = readchar()) != EOF && c != '\n')
			errbuf[errind++] = c;
		errbuf[errind] = '\0';
		if (logfp != NULL) {
			fprintf(logfp, "\n%s\n", errbuf);
			fprintf(logfp, "Die roll was: Human (%d, %d), Computer (%d, %d)\n",
				humandieroll[0], humandieroll[1], computerdieroll[0], computerdieroll[1]);
		}
		message(ERR, errbuf);
		message(MSG, "Game over due to errors - please report");
		break;
	case QUIT:
		state = GAMEOVER;
		setcursor(ORIGINAL_CUR);
		break;
	case EOF:
		message(ERR, "ERROR - premature EOF on pipe from backgammon");
		break;
	default:
		message(ERR, "ERROR - unknown code sent by backgammon");
		break;
	}
}

static
readchar()
{
	char c;

	if (read(gammonfd, &c, 1) != 1)
		return(EOF);
	return(c);
}

startcomputerturn()
{
	state = THINKING;
	setcursor(THINKING_CUR);
	if (alreadyrolled)
		alreadyrolled = 0;
	else
		rolldice(COMPUTER);
	sendtogammon(DIEROLL);
	sendtogammon(computerdieroll[0] + '0');
	sendtogammon(computerdieroll[1] + '0');
	compnomove = 0;
}

sendtogammon(c)
char c;
{
	write(togammonfd, &c, 1);
}

readcomputermove()
{
	int from = 0, to = 0, c = 0, first = 1;

	cmove = 0;
	while (c != '\n') {
		c = readchar();
		if (c >= '0' && c <= '9') {
			if (first)
				from = from * 10 + c - '0';
			else
				to = to * 10 + c - '0';
		} else if (c == 'B' && first) {
			from = BAR;
		} else if (c == 'H' && !first) {
			to = HOME;
		} else if (c == '-') {
			first = 0;
		} else if (c == ',' || c == '\n') {
			if (humanboard[to] == 1 && to != HOME) {
				movepiece(to, BAR, HUMAN);
				computermoves[cmove][HITBLOT] = 1;
			} else {
				computermoves[cmove][HITBLOT] = 0;
			}
			movepiece(from, to, COMPUTER);
			computermoves[cmove][FROM] = from;
			computermoves[cmove][TO] = to;
			cmove++;
			first = 1;
			from = to = 0;
		}
	}
	if (logfp != NULL) {
		fprintf(logfp, "Computer: (%d, %d) ", computerdieroll[0], computerdieroll[1]);
		for (c = 0; c < cmove; c++) {
			if (c > 0)
				fprintf(logfp, ", ");
			fprintf(logfp, "%d%c%d", computermoves[c][FROM],
				computermoves[c][HITBLOT] ? 'x' : '-',
				computermoves[c][TO]);
		}
		putc('\n', logfp);
	}
}

showlastcomputermove()
{
	int index;

	if (cmove == 0) {
		message(ERR, "Nobody has moved yet!");
		return;
	}
	if (!compnomove) {
		for (index = 0; index < cmove; index++) {
			movepiece(computermoves[index][TO], computermoves[index][FROM], COMPUTER);
			if (computermoves[index][HITBLOT])
				movepiece(BAR, computermoves[index][TO], HUMAN);
		}
	}
	if (dicedisplayed == HUMAN)
		showlastdice();
	sleep(2);
	if (!compnomove) {
		for (index--; index >= 0; index--) {
			if (computermoves[index][HITBLOT])
				movepiece(computermoves[index][TO], BAR, HUMAN);
			movepiece(computermoves[index][FROM], computermoves[index][TO], COMPUTER);
		}
	}
	if (dicedisplayed == HUMAN)
		showlastdice();
}
