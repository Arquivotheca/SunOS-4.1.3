#ifndef lint
static  char sccsid[] = "@(#)chessprog.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems Inc.
 */
#include <stdio.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include "chesstool.h"

FILE *put, *get;

char letter[8] = {
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'
};
#define swap(a,b,tmp) tmp = a; a = b; b = tmp;

/* 
 * return fd of pipe to use for reading from chess
 */
startchess(argv)
	char **argv;
{
	int tochess[2], fromchess[2];
	int i, pid, ppid;
	char buf[100];
	union wait status;
	

	/* 
	 * Clean up from previous startchess() call
	 */
 	while (wait3(&status, WNOHANG, 0) > 0) {
#if 0
 		if (status.w_status != 0)
 			putmsg("Chess program died horribly");
#endif
	}

	if (pipe(tochess) < 0) {
		perror("chesstool: pipe");
		exit(1);
	}
	if (pipe(fromchess) < 0) {
		perror("chesstool: pipe");
		exit(1);
	}
	ppid = getpid();
	if ((pid = fork()) < 0) {
	    perror("chesstool: fork");
	    exit(1);
	}
	if (pid == 0) {		/* child */
		if (dup2(tochess[0], 0) < 0) {
			perror("chesstool: dup2");
			exit(1);
		}
		if (dup2(fromchess[1], 1) < 0) {
			perror("chesstool: dup2");
			exit(1);
		}
		for (i = 3; i < getdtablesize(); i++)
			close(i);
		if (execvp(argv[1], argv+1) < 0) {
			sprintf(buf, "chesstool: execvp: %s", argv[1]);
			perror(buf);
			kill(ppid, SIGINT);
			exit(1);
		}
		fprintf(stderr, "exec failed\n");
		exit(1);
	}
	if (close(tochess[0]) < 0) {
		perror("chesstool: close");
		exit(1);
	}
	if (close(fromchess[1]) < 0) {
		perror("chesstool: close");
		exit(1);
	}
	if ((put = fdopen(tochess[1], "w")) == NULL) {
	    fprintf(stderr, "can't fdopen\n");
	    exit(1);
	}
	if ((get = fdopen(fromchess[0], "r")) == NULL) {
	    fprintf(stderr, "can't fdopen\n");
	    exit(1);
	}
	setbuf(put, NULL);
	setbuf(get, NULL);
	return fromchess[0];
}

closepipes()
{
	fclose(get);
	fclose(put);
}

first() {
	fprintf(put, "alg\nfirst\n");
	thinking = 1;
	hourglass();
	putmsg("Thinking ...");
}

sendmove(a, b, c, d)
{
	char ans[80], move[5];
	
	sprintf(move, "%c%d%c%d", letter[a], b+1, letter[c], d+1);
	fprintf(put, "%s\n", move);
	while (1) {
		if (fscanf(get, "%s", ans) != 1)
			exit(0);
#ifdef DEBUG
		fprintf(stderr, "sendmove: %s**\n", ans);
#endif
		if (strncmp(ans, "Illegal", strlen("Illegal")) == 0) {
			/* flash message window */
			if (strlen(ans) < strlen("Illegal move"))
				fscanf(get, "%s", ans);
			putmsg("");
			putmsg("Illegal move");
			return -1;
		}
		if (strncmp(ans, "eh?", strlen("eh?")) == 0) {
			putmsg("internal error: returned eh?\n");
			return -1;
		}
		if (strcmp(ans, move) == 0) {
			hourglass();
			thinking = 1;
			putmsg("Thinking ...");
			return 0;
		}
	}
}

readfromchess()
{
	char ans[100];

	while (1) {
		if (fscanf(get, "%s", ans) == EOF) {
#ifdef DEBUG
			fprintf(stderr, "readfromchess: scanf get eof\n");
#endif
 			putmsg("program gives up");
 			thinking = 0;
			done = 1;
			restartchess();
			return;
		}
#ifdef DEBUG
		fprintf(stderr, "readfromchess: %s**\n", ans);
#endif
		if (strcmp(ans, "Chess") == 0)
			return;
		if (strncmp(ans, "Resign", strlen("Resign")) == 0) {
			putmsg("Resign");
			thinking = 0;
			done = 1;
			restartchess();
			return;
		}
		if (strncmp(ans, "Draw", strlen("Draw")) == 0) {
			putmsg("Draw");
			thinking = 0;
			done = 1;
			restartchess();
			return;
		}
		if (strncmp(ans, "Stale", strlen("Stale")) == 0) {
			putmsg("Stale mate");
			thinking = 0;
			done = 1;
			restartchess();
			return;
		}
		if (strncmp(ans, "White", strlen("White")) == 0) {
			putmsg("White wins!");
			thinking = 0;
			done = 1;
			restartchess();
			return;
		}
		if (strncmp(ans, "Black", strlen("Black")) == 0) {
			putmsg("Black wins!");
			thinking = 0;
			done = 1;
			restartchess();
			return;
		}
		if (strncmp(ans, "done", strlen("done")) == 0) {
			putmsg("done!");
			thinking = 0;
			done = 1;
			restartchess();
		}
		if (checkmove(ans)) {
			make_move(ans[0]-'a', ans[1]-'1',
			    ans[2]-'a', ans[3]-'1');
			unhourglass();
			thinking = 0;
			if (undoqueue) {
				sendundo();
				undoqueue = 0;
			}
			putmsg("Your Move");
			break;
		}
	}
}

static
checkmove(str)
	char str[];
{
	if (str[0] < 'a' || str[0] > 'h')
		return 0;
	if (str[1] < '1' || str[1] > '8')
		return 0;
	if (str[2] < 'a' || str[2] > 'h')
		return 0;
	if (str[3] < '1' || str[3] > '8')
		return 0;
	return 1;
}

make_move(i, j, k, l)
{
	int mv;
	
	movelist[movecnt++] = mv = encodemove(i, j, k, l);
	if (mv & WQCASTLE) {
		move_piece(i, j, k, l);
		move_piece(0, 0, 3, 0);
	}
	else if (mv & WKCASTLE) {
		move_piece(i, j, k, l);
		move_piece(7, 0, 5, 0);
	}
	else if (mv & BQCASTLE) {
		move_piece(i, j, k, l);
		move_piece(0, 7, 3, 7);
	}
	else if (mv & BKCASTLE) {
		move_piece(i, j, k, l);
		move_piece(7, 7, 5, 7);
	}
	else if (mv & WPROMOTE) {
		move_piece(i, j, k, l);
		piecearr[k][l] = QUEEN;
		paint_square(k, l);
	}
	else if (mv & BPROMOTE) {
		move_piece(i, j, k, l);
		piecearr[k][l] = QUEEN;
		paint_square(k, l);
	}
	else if (mv & WPASSANT) {
		move_piece(i, j, k, l);
		move_piece(k, l, k, l+1);
	}
	else if (mv & BPASSANT) {
		move_piece(i, j, k, l);
		move_piece(k, l, k, l-1);
	}
	else
		move_piece(i, j, k, l);
}

move_piece(i, j, k, l)
	int i, j, k, l;
{
	if (piecearr[k][l] != 0) {
		if (colorarr[k][l] == BLACK) {
			bcapture[bcapcnt++] = piecearr[k][l];
			paint_capture(BLACK, bcapcnt-1);
		}
		else {
			wcapture[wcapcnt++] = piecearr[k][l];
			paint_capture(WHITE, wcapcnt-1);
		}
	}
	piecearr[k][l] = piecearr[i][j];
	colorarr[k][l] = colorarr[i][j];
	piecearr[i][j] = 0;
	colorarr[i][j] = 0;
	paint_square(i, j);
	paint_square(k, l);
}

last_proc(sw, ip)
	char *sw;
	char *ip;
{
	int i, j, k, l;
	int tmp, mv;
	
	if (movecnt <= 0) {
		putmsg("No move to replay");
		return;
	}
	mv = movelist[movecnt-1];
	unpack(mv, &i, &j, &k, &l);
	undo(--movecnt);
	sleep(1);
	make_move(i, j, k, l);
}

sendundo()
{
	fprintf(put, "remove\n");
	undo(movecnt-1);
	if (movecnt == 1 || done) {
		movecnt--;
		return;
	}
	undo(movecnt-2);
	movecnt -= 2;
}

undo(cnt)
{
	int i, j, k, l, c, z, mv;
	
	mv = movelist[cnt];
	unpack(mv, &i, &j, &k, &l);

	/* 
	 * can't capture during a castle
	 */
	if (mv & WQCASTLE) {
		move_piece(k, l, i, j);
		move_piece(3, 0, 0, 0);
		return;
	}
	if (mv & WKCASTLE) {
		move_piece(k, l, i, j);
		move_piece(5, 0, 7, 0);
		return;
	}
	if (mv & BQCASTLE) {
		move_piece(k, l, i, j);
		move_piece(3, 7, 0, 7);
		return;
	}
	if (mv & BKCASTLE) {
		move_piece(k, l, i, j);
		move_piece(5, 7, 7, 7);
		return;
	}
	if (mv & WPROMOTE)
		piecearr[k][l] = PAWN;
	else if (mv & BPROMOTE)
		piecearr[k][l] = PAWN;
	if (mv & WPASSANT) {
		piecearr[i][j] = piecearr[k][l+1];
		colorarr[i][j] = colorarr[k][l+1];
		piecearr[k][l+1] = 0;
		colorarr[k][l+1] = 0;
		paint_square(k, l+1);
	}
	else if (mv & BPASSANT) {
		piecearr[i][j] = piecearr[k][l-1];
		colorarr[i][j] = colorarr[k][l-1];
		piecearr[k][l-1] = 0;
		colorarr[k][l-1] = 0;
		paint_square(k, l-1);
	}
	else {
		piecearr[i][j] = piecearr[k][l];
		colorarr[i][j] = colorarr[k][l];
	}
	paint_square(i, j);
	c = getcapture(cnt);
	if (c != 0) {
		piecearr[k][l] = c;
		colorarr[k][l] = 1 ^ colorarr[i][j];
		paint_square(k, l);
		if (colorarr[k][l] == BLACK) {
			bcapcnt--;
			clearbcaparea();
			for (z = 0; z < bcapcnt; z++)
				paint_capture(BLACK, z);
		}
		else {
			wcapcnt--;
			paint_capture(WHITE, wcapcnt);
			clearwcaparea();
			for (z = 0; z < wcapcnt; z++)
				paint_capture(WHITE, z);
		}
	}
	else {
		piecearr[k][l] = 0;
		colorarr[k][l] = 0;
		paint_square(k, l);
	}
}

/* 
 * routines for (en)decoding move into movelist
 */

encodemove(i, j, k, l)
{
	int ans;

	ans = 0;
	if (piecearr[i][j]==PAWN && colorarr[i][j] ==  WHITE && l == 7)
		ans |= WPROMOTE;
	else if (piecearr[i][j]==PAWN && colorarr[i][j] ==  BLACK && l == 0)
		ans |= BPROMOTE;
	else if (piecearr[i][j] == KING && i == 4 && j == 0 && k == 2)
		ans |= WQCASTLE;
	else if (piecearr[i][j] == KING && i == 4 && j == 0 && k == 6)
		ans |= WKCASTLE;
	else if (piecearr[i][j] == KING && i == 4 && j == 7 && k == 2)
		ans |= BQCASTLE;
	else if (piecearr[i][j] == KING && i == 4 && j == 7 && k == 6)
		ans |= BKCASTLE;
	else if (piecearr[i][j] == PAWN && colorarr[i][j] == WHITE && j == l)
		ans |= WPASSANT;
	else if (piecearr[i][j] == PAWN && colorarr[i][j] == BLACK && j == l)
		ans |= BPASSANT;
	if (piecearr[k][l] != 0)
		ans |= (piecearr[k][l] << CAPTURE);
	return (ans | (i << 9) | (j << 6) | (k << 3) | (l << 0));
}

unpack(val, i, j, k, l)
	int *i, *j, *k, *l;
{
	*i = ( val >> 9) & 0x7;
	*j = ( val >> 6) & 0x7;
	*k = ( val >> 3) & 0x7;
	*l = ( val >> 0) & 0x7;
}

getcapture(cnt)
{
	return ((movelist[cnt] >> CAPTURE) & 0x7);
}
