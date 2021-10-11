/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)scr_rest.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

/*
    Initialize the curses data structures to believe that what is stored in
    the file is what should be on the screen. This is for library functions
    that diddle with the screen and then want to restore the screen to what
    was being shown before. The file must have been dumped with scr_dump().
*/

#include "curses.ext"
#include <sys/types.h>

static long input (file)
register FILE *file;
{
    chtype i;
    register int ret = fread ((char *)&i, sizeof (i), 1, file);
    if (ret == 0)
	return EOF;
    else
	return i;
}

int scr_restore (filename)
register char *filename;
{
    register int c, l, i, len, j;
    register FILE *file;
    int magic_number;
    char tty[20];
    time_t tty_mtime;

    file = fopen (filename, "r");
    if (file == NULL)
	return ERR;

    /* check for magic number */
    magic_number = input (file);
    if (magic_number != DUMP_MAGIC_NUMBER)
	goto error;

    /* get name of /dev/tty and mod time. ignore info for now. */
    (void) fread(tty, 20, 1, file);
    (void) fread((char *)&tty_mtime, sizeof(time_t), 1, file);

    c = input (file);			/* read # of lines and cols */
    l = input (file);
    if ((l != lines) || (c != columns))
	{
	(void) fprintf (stderr, "bad restore file received in scr_rest()!");
	goto error;
	}

    for (i = 0; i < lines; i++)		/* restore the screen */
	{
	if ((len = input (file)) > 0)
	    {
	    for (j = 0; j < len; j++)
		{
		_ll_move(i, j);
		if (SP->virt_x++ < columns)
		    *SP->curptr++ = input(file);
		}
	    for ( ; j < c; j++)
		{
		_ll_move(i, j);
		if (SP->virt_x++ < columns)
		    *SP->curptr++ = ' ';
		}
	    }
	else
	    for (j = 0; j < c; j++)
		{
		_ll_move(i, j);
		if (SP->virt_x++ < columns)
		    *SP->curptr++ = ' ';
		}
	}

    /* take care of the screen labels */
    if (SP->slk)
	{
	int labmax, labwidth;
	char label[9];
	(void) input (file);		/* skip whether screen had labels */

	if ((labmax = input (file)) > 0)
	    {
	    labwidth = input (file);
	    label[labwidth] = '\0';
	    for (i = 0; i < labmax; i++)
		{
		for (j = 0; j < labwidth; j++)
		    label[j] = input (file) & A_CHARTEXT;
		if (label[0] != ' ')			/* left justify? */
		    slk_set (i+1, label, 0);
		else if (strlen(label) == labwidth &&	/* right justify? */
			 label[labwidth-1] != ' ')
		    slk_set (i+1, label, 2);
		else					/* centered */
		    slk_set (i+1, label, 1);
		}
	    }
	else
	    for (i = 0; i < 8; i++)
		slk_set (i+1, "", 0);
	slk_noutrefresh();
	}

    SP->virt_y = input (file);
    SP->virt_x = input (file);

    (void) fclose (file);

    /* permit doupdate to be called without a refresh(). */
    SP->leave_lwin = (WINDOW *) 1;
    SP->leave_leave = TRUE;
    return OK;

error:
    (void) fclose (file);
    return ERR;
}
