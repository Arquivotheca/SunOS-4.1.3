#ifndef lint
static	char sccsid[] = "@(#)search.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * search.c: routines dealing with searching for words in the grid
 */

#include <suntool/tool_hs.h>
#include <ctype.h>
#include <stdio.h>
#include "defs.h"

#define STACKSIZE	MWORDLEN

FILE *wfp;		/* file pointer for WORDLIST */
char wordbuf[MWORDLEN];	/* buffer to hold current word from WORDLIST */
int wordlen;		/* length of word in wordbuf */
int thismatch;		/* number of letters which match at the beginning of the current word and the last word processed */
int nextmatch;		/* number of letters which match at the beginning of the current word and the next word to be processed */

extern char board[16];

#define ROW(c)	(BOARD_BASELEFT + c * (TILE_WIDTH + BOARD_SPACING))
#define COL(c)	ROW(c)

struct cube {		/* structure for a letter cube on the display */
	char *letter;	/* pointer into board array to the right letter */
	int neighbors;	/* number of cubes adjacent to this one */
	struct cube *adjacent[16];	/* pointers to adjacent cubes */
	char used;	/* used by search; if this cube has already been used in the word, it is true */
	int x, y;	/* coordinates of cube on display */
	char inverse;	/* true if inversed for showcubes */
} cubes[16] = {
	{ &board[0], 3, { &cubes[1], &cubes[4], &cubes[5] }, 0, ROW(0), COL(0), 0 },
	{ &board[1], 5, { &cubes[0], &cubes[2], &cubes[4], &cubes[5], &cubes[6] }, 0, ROW(1), COL(0), 0 },
	{ &board[2], 5, { &cubes[1], &cubes[3], &cubes[5], &cubes[6], &cubes[7] }, 0, ROW(2), COL(0), 0 },
	{ &board[3], 3, { &cubes[2], &cubes[6], &cubes[7] }, 0, ROW(3), COL(0), 0 },
	{ &board[4], 5, { &cubes[0], &cubes[1], &cubes[5], &cubes[8], &cubes[9] }, 0, ROW(0), COL(1), 0 },
	{ &board[5], 8, { &cubes[0], &cubes[1], &cubes[2], &cubes[4], &cubes[6], &cubes[8], &cubes[9], &cubes[10] }, 0, ROW(1), COL(1), 0 },
	{ &board[6], 8, { &cubes[1], &cubes[2], &cubes[3], &cubes[5], &cubes[7], &cubes[9], &cubes[10], &cubes[11] }, 0, ROW(2), COL(1), 0 },
	{ &board[7], 5, { &cubes[2], &cubes[3], &cubes[6], &cubes[10], &cubes[11] }, 0, ROW(3), COL(1), 0 },
	{ &board[8], 5, { &cubes[4], &cubes[5], &cubes[9], &cubes[12], &cubes[13] }, 0, ROW(0), COL(2), 0 },
	{ &board[9], 8, { &cubes[4], &cubes[5], &cubes[6], &cubes[8], &cubes[10], &cubes[12], &cubes[13], &cubes[14] }, 0, ROW(1), COL(2), 0 },
	{ &board[10], 8, { &cubes[5], &cubes[6], &cubes[7], &cubes[9], &cubes[11], &cubes[13], &cubes[14], &cubes[15] }, 0, ROW(2), COL(2), 0 },
	{ &board[11], 5, { &cubes[6], &cubes[7], &cubes[10], &cubes[14], &cubes[15] }, 0, ROW(3), COL(2), 0 },
	{ &board[12], 3, { &cubes[8], &cubes[9], &cubes[13] }, 0, ROW(0), COL(3), 0 },
	{ &board[13], 5, { &cubes[8], &cubes[9], &cubes[10], &cubes[12], &cubes[14] }, 0, ROW(1), COL(3), 0 },
	{ &board[14], 5, { &cubes[9], &cubes[10], &cubes[11], &cubes[13], &cubes[15] }, 0, ROW(2), COL(3), 0 },
	{ &board[15], 3, { &cubes[10], &cubes[11], &cubes[14] }, 0, ROW(3), COL(3), 0 }
}, mastercube = {	/* pseudo-cube which neighbors every real cube */
	0, 16, { &cubes[0], &cubes[1], &cubes[2], &cubes[3], &cubes[4], &cubes[5], &cubes[6], &cubes[7], &cubes[8], &cubes[9], &cubes[10], &cubes[11], &cubes[12], &cubes[13], &cubes[14], &cubes[15] }, 0, -1, -1
};

struct frame {	/* stack frame for the search */
	struct cube *fr_cube;
	int fr_neighbor;
};

struct frame stack[STACKSIZE];	/* stack of frames */
int stacklength = 0;	/* how many frames are on stack */

/* stuff for the hash table, copied from /usr/bin/spellout */

/*
* Hash table for spelling checker has n bits.
* Each word w is hashed by k different (modular) hash functions, hi.
* The bits hi(w), i=1..k, are set for words in the dictionary.
* Assuming independence, the probability that no word of a d-word
* dictionary sets a particular bit is given by the Poisson formula
* P = exp(-y)*y**0/0!, where y=d*k/n.
* The probability that a random string is recognized as a word is then
* (1-P)**k.  For given n and d this is minimum when y=log(2), P=1/2,
* whence one finds, for example, that a 25000-word dictionary in a
* 400000-bit table works best with k=11.
*/

#define SHIFT	4
#define TABSIZE 25000	/* (int)(400000/(1<<SHIFT)) */
short	tab[TABSIZE];
long	p[] = {
	399871,
	399887,
	399899,
	399911,
	399913,
	399937,
	399941,
	399953,
	399979,
	399983,
	399989,
};
#define	NP	(sizeof(p)/sizeof(p[0]))
#define	NW	30

long	pow2[NP][NW];

#define get(h)	(tab[h>>SHIFT]&(1<<((int)h&((1<<SHIFT)-1))))
#define set(h)	tab[h>>SHIFT] |= 1<<((int)h&((1<<SHIFT)-1))

/*
 * initboggle: initializes the computer word search; executes once before
 *             the fork()
 */

initboggle()
{
	int c;
	FILE *fopen();
	register i, j;
	long h;
	register long *lp;

	srandom(time(0));	/* initialize the random number generator */
	wfp = fopen(dictionary, "r");	/* open the dictionary */
	if (wfp == NULL) {
		fprintf(stderr, "Couldn't open dictionary %s!\n", dictionary);
		exit(1);
	}
				/* initialize hash table for verify() */
	if (fread((char *)tab, sizeof(*tab), TABSIZE, wfp) != TABSIZE) {
		fprintf(stderr, "Couldn't read hash table!\n");
		exit(1);
	}
	for (i = 0; i < NP; i++) {
		h = *(lp = pow2[i]) = 1 << 14;
		for (j = 1; j < NW; j++)
			h = *++lp = (h << 7) % p[i];
	}
	if (reuse > 1)		/* if cubes are adjacent to themselves... */
		for (c = 0; c < 16; c++) /* add this to cube linked network */
			cubes[c].adjacent[cubes[c].neighbors++] = &cubes[c];
}

/*
 * getword: read a word from the alphabetical dictionary
 */

getword()
{
	int c;
	char *ptr;
	char buf[512];

	if ((thismatch = nextmatch) == EOF)	/* if EOF was reached */
		return(0);			/* return error condition */
	ptr = &wordbuf[thismatch];		/* point to place where first non-matching part of new word goes */
	wordlen = thismatch;			/* initialize word length */
	while (isalpha(c = getc(wfp))) {	/* copy in the non-matching characters */
		*ptr++ = c;
		wordlen++;			/* increase length */
	}
	nextmatch = c;				/* store match for next word */
	strncpy(buf, wordbuf, wordlen);
	buf[wordlen] = '\0';
	return(wordlen);			/* return length of this word */
}

/*
 * push: push a frame on the stack
 */

push(c)
struct cube *c;
{
	if (stacklength > STACKSIZE) {	/* check for overflow */
		fprintf(stderr, "Stack overflow\n");
		childquit(1);
	}
	stack[stacklength].fr_cube = c;
	stack[stacklength].fr_neighbor = -1;
	if (!reuse)
		c->used = 1;
	stacklength++;
}

/*
 * pop: discard the top frame on the stack
 */

pop()
{
	if (stacklength <= 0) {	/* check for underflow */
		fprintf(stderr, "Stack underflow\n");
		childquit(1);
	}
	stacklength--;	/* decrement counter */
	if (!reuse)	/* clear used flag on the cube that was pointed to */
		stack[stacklength].fr_cube->used = 0;
}

/*
 * peek: return the top value on the stack without disturning it
 */

struct frame *
peek()
{
	if (stacklength <= 0) {	/* check for underflow */
		fprintf(stderr, "Stack underflow\n");
		childquit(1);
	}
	return(&stack[stacklength-1]);	/* return top cube */
}

/*
 * clearstack: clear the stack
 */

clearstack()
{
	while (stacklength) {
		stacklength--;
		if (!reuse)
			stack[stacklength].fr_cube->used = 0;
	}
}

/*
 * findwords: search the grid for every word in the alphabetical dictionary
 */

findwords()
{
	register char *ptr;
	register int matched;	/* number of characters we have matched */
	register int maxmatched;	/* largest match found for this word */
	register struct frame *lastframe;
	struct frame *peek();

	fseek(wfp, sizeof(*tab) * TABSIZE, 0);	/* skip over hash table */
	thismatch = 0;
	nextmatch = getc(wfp);
	while (getword()) {
		if (wordlen < 3)
			continue;
		ptr = wordbuf;
		matched = 0;
		maxmatched = 0;
		clearstack();
		push(&mastercube);
		while (matched < wordlen) {
			lastframe = peek();
			if (++lastframe->fr_neighbor >= lastframe->fr_cube->neighbors) {
				if (matched > maxmatched)
					maxmatched = matched;
				if (matched == 0)
					break;
				matched--;
				ptr--;
				pop();
			} else if (*(lastframe->fr_cube->adjacent[lastframe->fr_neighbor]->letter) == *ptr) {
				if (!reuse && lastframe->fr_cube->adjacent[lastframe->fr_neighbor]->used)
					continue;
				matched++;
				ptr++;
				push(lastframe->fr_cube->adjacent[lastframe->fr_neighbor]);
			}
		}
		if (matched == wordlen) {
			wordbuf[wordlen] = '\0';
			write(toparent, wordbuf, wordlen+1);
		} else {
			while (nextmatch > maxmatched && getword())
				;
			if (nextmatch == EOF || thismatch == EOF)
				break;
		}
	}
}

/*
 * startwordfind: front end for findwords(); initializes the computer
 *                word search
 */

startwordfind()
{
	killchild();		/* make sure the last process is dead */

	if (pipe(pfd) < 0) {	/* create the pipe */
		perror("bogtool");
		exit(1);
	}

	freecompwords();	/* free the last list of words */

	switch (childpid = fork()) {	/* fork a child */
	case -1:	/* failed */
		perror("bogtool");
		exit(1);
	case 0:		/* this is the child */
		signal(SIGWINCH, SIG_DFL);	/* reset signals */
		signal(SIGINT, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		findwords();			/* find the words and send them */
		childquit(0);
	default:	/* this is the parent */
		setchildbits = 1;	/* set flag so output from child gets read */
		childfp = fdopen(fromchild, "r");	/* use stdio on the pipe */
		if (childfp == NULL) {
			fprintf(stderr, "Couldn't open pipe from child process!\n");
			exit(1);
		}
		break;
	}
}

/*
 * childquit: child process always exits through this procedure
 */

childquit(status)
int status;
{
	if (status)
		write(toparent, "EXIT", 5);
	else
		write(toparent, "DONE", 5);
	close(toparent);
	exit(status);
}

/*
 * checkword: verify that words human types in are in the grid
 */

checkword(str, len)
char str[];
int len;
{
	register int matched;	/* number of characters we have matched */
	register struct frame *lastframe;
	register char *ptr;
	struct frame *peek();
	int i1, i2, l2;
	char buf[64];

	/*
	 * check for words with a q but no u; then remove the u since
	 * the grid contains only the q (q cubes have qu printed on them
	 * but are stored as a single q)
	 */

	i1 = i2 = 0;
	l2 = len;
	while (i1 < len) {
		buf[i2++] = str[i1];
		if (str[i1] == 'q' && i1 < len-1) {
			if (str[i1 + 1] != 'u') {
				return(0);
			} else {
				i1++;	/* skip the u */
				l2--;	/* decrease the length */
			}
		}
		i1++;
	}

	/*
	 * now do the actual search in the grid
	 */

	ptr = buf;
	matched = 0;
	push(&mastercube);
	while (matched < l2) {
		lastframe = peek();
		if (++lastframe->fr_neighbor >= lastframe->fr_cube->neighbors) {
			if (matched == 0)
				break;
			matched--;
			ptr--;
			pop();
		} else if (*(lastframe->fr_cube->adjacent[lastframe->fr_neighbor]->letter) == *ptr) {
			if (!reuse && lastframe->fr_cube->adjacent[lastframe->fr_neighbor]->used)
				continue;
			matched++;
			ptr++;
			push(lastframe->fr_cube->adjacent[lastframe->fr_neighbor]);
		}
	}
	clearstack();

	/*
	 * return the results
	 */

	if (matched == l2)
		return(1);
	else
		return(0);
}

/*
 * readchild: read a word found by the child process
 */

readchild()
{
	int c, index = 0;
	char buf[MWORDLEN];

	while ((c = getc(childfp)) != EOF) {
		buf[index++] = c;
		if (c == '\0') {
			if (!strcmp(buf, "EXIT")) {
				fprintf(stderr, "bogtool: child died\n");
				return(0);
			} else if (!strcmp(buf, "DONE")) {
				return(0);
			} else {
				addcompword(buf);
				return(1);
			}
		} else if (c == 'q') {
			buf[index++] = 'u';
		}
	}
	return(0);
}

/*
 * verify: verify that a word the human has typed in is in the dictionary
 *         (uses the hashed dictionary for speed; hash code is taken directly
 *         from /usr/bin/spellout)
 */

verify(str)
char *str;
{
	register i, j;
	long h;
	register long *lp;
	char buf[MWORDLEN];
	register char *ptr;
	int i1, i2;

	/*
	 * remove u from qu pairs for check in dictionary
	 */

	i1 = i2 = 0;
	while (str[i1]) {
		buf[i2++] = str[i1];
		if (str[i1] == 'q') {
			if (str[i1 + 1] != 'u')
				return(0);	/* q without u */
			else
				i1++;	/* skip the u */
		}
		i1++;
	}
	buf[i2++] = '\n';	/* newline is necessary! */
	buf[i2] = '\0';
	for (i = 0; i < NP; i++) {
		for (ptr=buf, h=0, lp=pow2[i]; (j = *ptr)!='\0'; ++ptr, ++lp)
			h += j * *lp;
		h %= p[i];
		if (get(h) == 0)
			return(0);
	}
	return(1);
}

/*
 * showcubes: reverse-video the cubes which make up str; code is similar
 *            to checkword()
 */

showcubes(str)
char *str;
{
	register int matched;	/* number of characters we have matched */
	register struct frame *lastframe;
	register char *ptr;
	struct frame *peek();
	int i1, i2, l2, len;
	char buf[64];

	strcpy(laststr, str);
	len = strlen(str);

	/*
	 * check for words with a q but no u; then remove the u since
	 * the grid contains only the q (q cubes have qu printed on them
	 * but are stored as a single q)
	 */

	i1 = i2 = 0;
	l2 = len;
	while (i1 < len) {
		buf[i2++] = str[i1];
		if (str[i1] == 'q' && i1 < len-1) {
			if (str[i1 + 1] != 'u') {
				return(0);
			} else {
				i1++;	/* skip the u */
				l2--;	/* decrease the length */
			}
		}
		i1++;
	}

	/*
	 * now search for the word in the grid
	 */

	ptr = buf;
	matched = 0;
	push(&mastercube);
	while (matched < l2) {
		lastframe = peek();
		if (++lastframe->fr_neighbor >= lastframe->fr_cube->neighbors) {
			if (matched == 0)
				break;
			matched--;
			ptr--;
			pop();
		} else if (*(lastframe->fr_cube->adjacent[lastframe->fr_neighbor]->letter) == *ptr) {
			if (!reuse && lastframe->fr_cube->adjacent[lastframe->fr_neighbor]->used)
				continue;
			matched++;
			ptr++;
			push(lastframe->fr_cube->adjacent[lastframe->fr_neighbor]);
		}
	}
	if (matched != l2) {	/* word isn't there */
		flash(1);
		return;
	}

	/*
	 * the stack now contains all the cubes to highlight
	 */

	lastframe = peek();
	while (lastframe->fr_cube->x > 0 && lastframe->fr_cube->y > 0) {
		pw_write(bogwin, lastframe->fr_cube->x, lastframe->fr_cube->y,
			TILE_WIDTH-1, TILE_WIDTH, PIX_NOT(PIX_DST), NULL, 0, 0);
		lastframe->fr_cube->inverse = 1;
		pop();
		lastframe = peek();
	}
	pop();
}

/*
 * unshowword: un-reverse-video the cubes highlighted in showcubes()
 */

unshowword()
{
	int index;

	if (laststr[0] == '\0')
		return;
	laststr[0] = '\0';
	for (index = 0; index < 16; index++) {
		if (cubes[index].inverse) {
			cubes[index].inverse = 0;
			pw_write(bogwin, cubes[index].x, cubes[index].y,
				TILE_WIDTH-1, TILE_WIDTH, PIX_NOT(PIX_DST),
				NULL, 0, 0);
		}
	}
}
