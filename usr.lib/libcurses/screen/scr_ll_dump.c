/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)scr_ll_dump.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.2 */
#endif

#include	"curses_inc.h"
#include	<sys/types.h>
#include	<sys/stat.h>

extern	char	*ttyname();

scr_ll_dump(filep)
register	FILE	*filep;
{
    short	magic = SVR3_DUMP_MAGIC_NUMBER, rv = ERR;
    char	*thistty;
    SLK_MAP	*slk = SP->slk;
    struct	stat	statbuf;

    if (fwrite((char *) &magic, sizeof(short), 1, filep) != 1)
	goto err;

    /* write term name and modification time */
    if ((thistty = ttyname(cur_term->Filedes)) == NULL)
	statbuf.st_mtime = 0;
    else
	(void) stat(thistty, &statbuf);

    if (fwrite((char *) &(statbuf.st_mtime), sizeof(time_t), 1, filep) != 1)
	goto err;

    /* write curscr */
    if (_INPUTPENDING)
	(void) force_doupdate();
    if (putwin(curscr, filep) == ERR)
	goto err;

    /* next output: 0 no slk, 1 hardware slk, 2 simulated slk */

    magic = (!slk) ? 0 : (slk->_win) ? 2 : 1;
    if (fwrite((char *) &magic, sizeof(int), 1, filep) != 1)
	goto err;
    if (magic)
    {
	short	i, labmax = slk->_num, lablen = slk->_len + 1;

	/* output the soft labels themselves */
	if ((fwrite((char *) &labmax, sizeof(short), 1, filep) != 1) ||
	    (fwrite((char *) &lablen, sizeof(short), 1, filep) != 1))
	{
	    goto err;
	}
	for (i = 0; i < labmax; i++)
	    if ((fwrite(slk->_ldis[i], sizeof(char), lablen, filep) != lablen) ||
		(fwrite(slk->_lval[i], sizeof(char), lablen, filep) != lablen))
	    {
		goto err;
	    }
    }

    /* success */
    rv = OK;
err :
    return (rv);
}
