/*
 * Simulation of termcap using terminfo.
 */

#include "curses.ext"

/*	@(#)tgetflag.c 1.1 92/07/30 SMI	(1.8	3/6/83)	*/

/* Make a 2 letter code into an integer we can switch on easily */
#define two(s1, s2) (s1 + 256*s2)
#define twostr(str) two(*str, str[1])

int
tgetflag(id)
char *id;
{
	register int rv;
	register char *p;

	switch (twostr(id)) {
	case two('b','w'): rv = auto_left_margin; break;
	case two('a','m'): rv = auto_right_margin; break;
	case two('x','b'): rv = beehive_glitch; break;
	case two('x','s'): rv = ceol_standout_glitch; break;
	case two('x','n'): rv = eat_newline_glitch; break;
	case two('e','o'): rv = erase_overstrike; break;
	case two('g','n'): rv = generic_type; break;
	case two('h','c'): rv = hard_copy; break;
	case two('k','m'): rv = has_meta_key; break;
	case two('h','s'): rv = has_status_line; break;
	case two('i','n'): rv = insert_null_glitch; break;
	case two('d','a'): rv = memory_above; break;
	case two('d','b'): rv = memory_below; break;
	case two('m','i'): rv = move_insert_mode; break;
	case two('m','s'): rv = move_standout_mode; break;
	case two('o','s'): rv = over_strike; break;
	case two('e','s'): rv = status_line_esc_ok; break;
	case two('x','t'): rv = teleray_glitch; break;
	case two('h','z'): rv = tilde_glitch; break;
	case two('u','l'): rv = transparent_underline; break;
	case two('x','o'): rv = xon_xoff; break;
	case two('b','s'):
		p = cursor_left;
		rv = p && *p==8 && p[1] == 0;
		break;
	case two('p','t'):
		p = tab;
		rv = p && *p==9 && p[1] == 0;
		break;
	case two('n','c'):
		p = carriage_return;
		rv = ! (p && *p==13 && p[1] == 0);
		break;
	case two('n','s'):
		p = scroll_forward;
		rv = ! (p && *p==10 && p[1] == 0);
		break;
	default: rv = 0;
	}
	return rv;
}
