#ifndef lint
static	char sccsid[] = "@(#)_delay.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/* Copyright (c) 1979 Regents of the University of California */

#include <ctype.h>
#include "curses.h"
#include "term.h"
#ifdef NONSTANDARD
# include "ns_curses.h"
#endif

/*
 * The following array gives the number of tens of milliseconds per
 * character for each speed as returned by gtty.  Thus since 300
 * baud returns a 7, there are 33.3 milliseconds per char at 300 baud.
 */
static
short	tmspc10[] = {
	/* 0   50    75   110 134.5 150  200  300   baud */
	   0, 2000, 1333, 909, 743, 666, 500, 333,
	/* 600 1200 1800 2400 4800 9600 19200 38400 baud */
	   166, 83,  55,  41,  20,  10,   5,    2
};

/*
 * Insert a delay into the output stream for "delay/10" milliseconds.
 * Round up by a half a character frame, and then do the delay.
 * Too bad there are no user program accessible programmed delays.
 * Transmitting pad characters slows many terminals down and also
 * loads the system.
 */
_delay(delay, outc)
register int delay;
int (*outc)();
{
	register int mspc10;
	register int pc;
	register int outspeed;

#ifndef 	NONSTANDARD
# ifdef USG
	outspeed = cur_term->Nttyb.c_cflag&CBAUD;
# else
	outspeed = cur_term->Nttyb.sg_ospeed;
# endif
#else		NONSTANDARD
	outspeed = outputspeed(cur_term);
#endif		NONSTANDARD
	if (outspeed <= 0 || outspeed >= (sizeof tmspc10 / sizeof tmspc10[0]))
		return ERR;

	mspc10 = tmspc10[outspeed];
	delay += mspc10 / 2;
	if (pad_char)
		pc = *pad_char;
	else
		pc = 0;
	for (delay /= mspc10; delay > 0; delay--)
		(*outc)(pc);
	return OK;
}
