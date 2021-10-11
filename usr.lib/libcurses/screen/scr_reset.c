/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)scr_reset.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.10 */
#endif

#include	"curses_inc.h"
#include	<sys/types.h>
#include	<sys/stat.h>

/*
 * Initialize the screen image to be the image contained
 * in the given file. This is usually used in a child process
 * to initialize its idea of the screen image to be that of its
 * parent.
 *
 * filep:	pointer to the output stream
 * type:	0: <curses> should assume that the physical screen is
 *		   EXACTLY as stored in the file. Therefore, we take
 *		   special care to make sure that /dev/tty and the terminal
 *		   did not change in any way.  This information can then
 *		   be used in the update optimization of the new program
 *		   so that the screen does not have to be cleared.  Instead,
 *		   curses, by knowing what's on the screen can optimally
 *		   update it with the information of the new program.
 *
 *		1: Tell <curses> that the stored image should be
 *		   the physical image.  Sort of like a huge waddstr onto
 *		   curscr.  This can be used when a library wants to save
 *		   a screen image and restore it at a later time.
 *
 *		2: Tell <curses> that the stored image is the physical
 *		   image and also it is what the new program wants on the
 *		   screen.  This can be be thought of as a screen inheritance
 *		   function.
 */

extern	char	*ttyname();

scr_reset(filep, type)
FILE	*filep;
int	type;
{
    register	WINDOW	*win = NULL, *win1 = NULL;
    register	int	*hash, y;
    short		clearit = FALSE, magic;
    struct	stat	statbuf;
    time_t		ttytime;

    if (type != 1 && exit_ca_mode && *exit_ca_mode && non_rev_rmcup)
    {
	if (type == 0)
	    goto err;
	else
	{
#ifdef	DEBUG
	if (outf)
	    fprintf(outf, "clear it because of exit_ca_mode\n");
#endif	/* DEBUG */
	    clearit = TRUE;
	}
    }

    /* check magic number */
    if (fread((char *) &magic, sizeof(short), 1, filep) != 1)
	goto err;
    if (magic != SVR3_DUMP_MAGIC_NUMBER)
	goto err;

    /* get modification time of image in file */
    if (fread((char *) &ttytime, sizeof(time_t), 1, filep) != 1)
	goto err;

    if ((type != 1) && ((ttyname(cur_term->Filedes) == NULL) ||
	(fstat(cur_term->Filedes, &statbuf) < 0) ||
	(statbuf.st_mtime != ttytime)))
    {
	if (type == 0)
	    goto err;
	else
	{
#ifdef	DEBUG
	if (outf)
	    fprintf(outf, "Filedes = %hd, statbuf.st_mtime = %d, ttytime = %d\n", cur_term->Filedes, statbuf.st_mtime, ttytime);
#endif	/* DEBUG */
	    clearit = TRUE;
	}
    }

    /* if get here, everything is ok, read the curscr image */
    if (((win = getwin(filep)) == NULL) ||
	((type == 2) && ((win1 = dupwin(win)) == NULL)) ||
	(win->_maxy != curscr->_maxy) || (win->_maxx != curscr->_maxx) ||
	/* soft labels */
	(fread((char *) &magic, sizeof(int), 1, filep) != 1))
	goto err;

    /*
     * if soft labels were dumped, we would like either read them 
     * or advance the file pointer pass them
     */
    if (magic)
    {
	short	i, labmax, lablen;
	SLK_MAP	*slk = SP->slk;
	/*
	 * Why doesn't the following line and the two below
	 * that access those variables work ?
	 */
	/* char	**labdis = SP->slk->_ldis, **labval = SP->slk->_lval; */

	if ((fread((char *) &labmax, sizeof(short), 1, filep) != 1) ||
	    (fread((char *) &lablen, sizeof(short), 1, filep) != 1))
	{
	    goto err;
	}

        if (slk != NULL)
	{
	    if ((labmax != slk->_num) || (lablen != (slk->_len + 1)))
	        goto err;

	    for (i = 0; i < labmax; i++)
	    {
	        /* if ((fread(labdis[i], sizeof(char), lablen, filep) != lablen) ||
		    (fread(labval[i], sizeof(char), lablen, filep) != lablen)) */
	        if ((fread(slk->_ldis[i], sizeof(char), lablen, filep) != lablen) ||
		    (fread(slk->_lval[i], sizeof(char), lablen, filep) != lablen))
	        {
		    goto err;
	        }
	    }
	    (*_do_slk_tch)();
        }
	else
	{
	    if (fseek(filep, (long) (2 * labmax * lablen * sizeof(char)), 1) != 0)
	    {
err:
		if (win != NULL)
		    (void) delwin(win);
		if (win1 != NULL)
		    (void) delwin(win1);
		if (type == 0)
		    curscr->_clear = TRUE;
		return (ERR);
	    }
	}
    }

    /* substitute read in window for the curscr */
    switch (type)
    {
	case 1:
	case 2:
	    (void) delwin(_virtscr);
	    hash = _VIRTHASH;
	    if (type == 1)
	    {
		SP->virt_scr = _virtscr = win;
		_VIRTTOP = 0;
		_VIRTBOT = curscr->_maxy - 1;
		break;
	    }
	    SP->virt_scr = _virtscr = win1;
	    _VIRTTOP = curscr->_maxy;
	    _VIRTBOT = -1;
	    /* clear the hash table */
	    for(y = curscr->_maxy; y > 0; --y)
		*hash++ = _NOHASH;
	case 0:
	{
	    int	saveflag = curscr->_flags & _CANT_BE_IMMED;

	    (void) delwin(curscr);
	    SP->cur_scr = curscr = win;
	    curscr->_sync = TRUE;
	    curscr->_flags |= saveflag;
	    hash = _CURHASH;
	}
    }

    /* clear the hash table */
    for(y = curscr->_maxy; y > 0; --y)
	*hash++ = _NOHASH;

    curscr->_clear = clearit;
    return (OK);
}
