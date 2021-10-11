/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)newkey.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

#include	"curses_inc.h"

/*
 * Set a new key or a new macro.
 *
 * rcvchars: the pattern identifying the key
 * keyval: the value to return when the key is recognized
 * macro: if this is not a function key but a macro,
 * 	tgetch() will block on macros.
 */

newkey(rcvchars, keyval, macro)
char	*rcvchars;
short	keyval;
int	macro;
{
    register	_KEY_MAP	**keys, *key_info,
				**prev_keys = cur_term->_keys;
    short	*numkeys = &cur_term->_ksz, len;
    char	*str;

    if ((!rcvchars) || (*rcvchars == '\0') || (keyval < 0) ||
	(((keys = (_KEY_MAP **) malloc(sizeof (_KEY_MAP *) * (*numkeys + 1))) == NULL)))
    {
	goto bad;
    }

    len = strlen(rcvchars) + 1;

    if ((key_info = (_KEY_MAP *) malloc(sizeof (_KEY_MAP) + len)) == NULL)
    {
	free (keys);
bad :
	term_errno = TERM_BAD_MALLOC;
#ifdef	DEBUG
	strcpy(term_parm_err, "newkey");
#endif	/* DEBUG */
	return (ERR);
    }

    if (macro)
    {
	(void) memcpy((char *) keys, (char *) prev_keys, (int) (*numkeys * sizeof (_KEY_MAP *)));
	keys[*numkeys] = key_info;
    }
    else
    {
	short	*first = &(cur_term->_first_macro);

	(void) memcpy((char *) keys, (char *) prev_keys, (int) (*first * sizeof (_KEY_MAP *)));
	(void) memcpy((char *) &(keys[*first + 1]), (char *) &(prev_keys[*first]),
	    (int) ((*numkeys - *first) * sizeof (_KEY_MAP *)));
	keys[(*first)++] = key_info;
	cur_term->_lastmacro_ordered++;
    }
    if (prev_keys != NULL)
	free(prev_keys);
    cur_term->_keys = keys;

    (*numkeys)++;
    key_info->_sends = str = (char *) key_info + sizeof(_KEY_MAP);
    (void) memcpy(str, rcvchars, len);
    key_info->_keyval = keyval;
    cur_term->funckeystarter[*str] |= (macro ? _MACRO : _KEY);

    return (OK);
}
