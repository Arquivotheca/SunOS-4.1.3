/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)vidupdate.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.10 */
#endif

#include	"curses_inc.h"

#define	NUM_OF_SPECIFIC_TURN_OFFS	3
extern	chtype	bit_attributes[];

void
vidupdate(newmode, oldmode, outc)
register	chtype	newmode, oldmode;
register	int	(*outc)();
{
    /* If you have set_attributes let the terminfo writer worry about it. */

    if (!set_attributes)
    {
	/*
	 * The trick is that we want to pre-process the new and oldmode
	 * so that we now know what they will really translate to on
	 * the physical screen.
	 * In the case where some attributes are being faked
	 * we get rid of the attributes being asked for and just have
	 * STANDOUT mode set.  Therefore, if STANDOUT and UNDERLINE were
	 * on the screen but UNDERLINE was being faked to STANDOUT; and
	 * the new mode is just UNDERLINE, we will get rid of any faked
	 * modes and be left with and oldmode of STANDOUT and a new mode
	 * of STANDOUT, in which case the check for newmode and oldmode
	 * being equal will be true.
	 *
	 *
	 * This test is similar to the concept explained above.
	 * counter is the maximum attributes allowed on a terminal.
	 * For instance, on an hp/tvi950 without set_attributes
	 * the last video sequence sent will be the one the terminal
	 * will be in (on that spot).  Therefore, in setupterm.c
	 * if ceol_standout_glitch or magic_cookie_glitch is set
	 * max_attributes is set to 1.  This is because on those terminals
	 * only one attribute can be on at once.  So, we pre-process the
	 * oldmode and the newmode and only leave the bits that are
	 * significant.  In other words, if on an hp you ask for STANDOUT
	 * and UNDERLINE it will become only STANDOUT since that is the
	 * first bit that is looked at.  If then the user goes from
	 * STANDOUT and UNDERLINE to STANDOUT and REVERSE the oldmode will
	 * become STANDOUT and the newmode will become STANDOUT.
	 *
	 * This also helps the code below in that on a hp or tvi950 only
	 * one bit will ever be set so that no code has to be added to
	 * cut out early in case two attributes were asked for.
	 */

	chtype	check_faked, modes[2];
	int	counter = max_attributes, i, j, tempmode, k;

	if (oldmode == cur_term->non_faked_mode)
	    oldmode = cur_term->sgr_mode;
	k = (cur_term->sgr_mode == oldmode) ? 1 : 2;
	modes[0] = cur_term->non_faked_mode = newmode;
	modes[1] = oldmode;

	while (k-- > 0)
	{
	    if ((check_faked = (modes[k] & cur_term->sgr_faked)) != A_NORMAL)
	    {
		modes[k] &= ~check_faked;
		modes[k] |= A_STANDOUT;
	    }

	    if ((j = counter) >= 0)
	    {
		tempmode = A_NORMAL;
		if (j > 0)
		{
		    for (i = 0; i < NUM_ATTRIBUTES; i++)
		    {
			if (modes[k] & bit_attributes[i])
			{
			    tempmode |= bit_attributes[i];
			    if (--j == 0)
				break;
			}
		    }
		}
		modes[k] = tempmode;
	    }
	}

	newmode = modes[0];
	oldmode = modes[1];
    }

    if (newmode == oldmode)
	return;

#ifdef	DEBUG
    if (outf)
	fprintf(outf, "vidupdate oldmode=%o, newmode=%o\n", oldmode, newmode);
#endif	/* DEBUG */

    if (set_attributes)
    {
	tputs(tparm(set_attributes,
			newmode & A_STANDOUT,
			newmode & A_UNDERLINE,
			newmode & A_REVERSE,
			newmode & A_BLINK,
			newmode & A_DIM,
			newmode & A_BOLD,
			newmode & A_INVIS,
			newmode & A_PROTECT,
			newmode & A_ALTCHARSET),
		1, outc);
    }
    else
    {
	register	chtype	turn_on, turn_off;
	int			i;

	/*
	 * If we are going to turn something on anyway and we are
	 * on a glitchy terminal, don't bother turning it off
	 * since by turning something on you turn everything else off.
	 */

	if ((ceol_standout_glitch || magic_cookie_glitch >= 0) &&
	    ((turn_on = ((oldmode ^ newmode) & newmode)) != A_NORMAL))
	{
	    goto turn_on_code;
	}

	if ((turn_off = (oldmode & newmode) ^ oldmode) != A_NORMAL)
	{
	    /*
	     * Check for things to turn off.
	     * First see if we are going to turn off something
	     * that doesn't have a specific turn off capability.
	     * If we are, and if there is a generic "turn all attributes
	     * off" capability, use it.
	     */

	    if (exit_attribute_mode != 0 &&
		((turn_off & ~(A_ALTCHARSET | A_STANDOUT | A_UNDERLINE)) ||
		(turn_off != (turn_off & cur_term->check_turn_off))))
	    {
		tputs(exit_attribute_mode, 1, outc);
		oldmode = A_NORMAL;
	    }
	    else
	    {
		for (i = 0; i < NUM_OF_SPECIFIC_TURN_OFFS; i++)
		    if (turn_off & bit_attributes[i])
			tputs(cur_term->turn_off_seq[i], 1, outc);

		/*
		 * We have to assume that any exit_video_mode will
		 * turn everything off, except for alternate char. set.
		 * So, the only thing we leave on in our oldmode is
		 * A_ALTCHARSET if it was and will be on.
		 */

		oldmode = (oldmode & newmode & A_ALTCHARSET);
	    }
	}

	if ((turn_on = ((oldmode ^ newmode) & newmode)) != A_NORMAL)
	{
turn_on_code:

	    /* Check for modes to turn on. */

	    for (i = 0; i < NUM_ATTRIBUTES; i++)
		if (turn_on & bit_attributes[i])
		{
		    tputs(cur_term->turn_on_seq[i], 1, outc);
		    /*
		     * Keep turning off the bit(s) that we just sent to
		     * the screen.  As soon as turn_on reaches A_NORMAL
		     * we don't have to turn anything else on and we can
		     * break out of the loop.
		     */
		    if ((turn_on &= ~bit_attributes[i]) == A_NORMAL)
			break;
		}
	}

	if (magic_cookie_glitch > 0)
	    tputs(cursor_left, 1, outc);
    }
    cur_term->sgr_mode = newmode;
}
