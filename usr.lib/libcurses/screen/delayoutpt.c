#ifndef lint
static	char sccsid[] = "@(#)delayoutpt.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

/*
 * Code for various kinds of delays.  Most of this is nonportable and
 * requires various enhancements to the operating system, so it won't
 * work on all systems.  It is included in curses to provide a portable
 * interface, and so curses itself can use it for function keys.
 */

#include "curses.ext"
#include <signal.h>

/*
 * Delay the output for ms milliseconds.
 * Note that this is NOT the same as a high resolution sleep.  It will
 * cause a delay in the output but will not necessarily suspend the
 * processor.  For applications needing to sleep for 1/10th second,
 * this is not a usable substitute.  It causes a pause in the displayed
 * output, for example, for the eye wink in snake.  It is disrecommended
 * for "delay" to be much more than 1/2 second, especially at high
 * baud rates, because of all the characters it will output.  Note
 * that due to system delays, the actual pause could be even more.
 * Some games won't work decently with this routine.
 */
delay_output(ms)
int ms;
{
	extern int _outchar();		/* it's in putp.c */

	return _delay(ms*10, _outchar);
}
