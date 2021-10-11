/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)scr_init.c 1.1 92/07/30 SMI"; /* from S5R3 1.8 */
#endif

/*
    Initialize the curses data structures to believe that what is stored in
    the file is what is on the screen. This is for screen programs that need
    to call other screen programs to do some work, but the screens to be
    shown are similar and you don't want the screen flashing around. The file
    must have been dumped with scr_dump(). Note that exit_ca_mode should NOT
    be called just before this.
*/

#include "curses.ext"
#include <sys/types.h>
#include <sys/stat.h>

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

extern struct line *_line_alloc ();
extern char *ttyname();

int scr_init (filename)
register char *filename;
{
    register int c, l, i, len, j;
    register FILE *file;
    int magic_number;
    char tty[20];
    char *thistty;
    struct stat ttystat;
    time_t tty_mtime;

    /* the screen contents cannot be considered */
    /* valid if exit_ca_mode exists */
    SP->doclear = TRUE;
    if (exit_ca_mode && *exit_ca_mode && non_rev_rmcup)
        return ERR;

    file = fopen (filename, "r");
    if (file == NULL)
	return ERR;

    /* check for magic number */
    magic_number = input (file);
    if (magic_number != DUMP_MAGIC_NUMBER)
	goto error;

    fread(tty, 20, 1, file);		/* get name of /dev/tty */
    fread((char *)&tty_mtime, sizeof(time_t), 1, file);	/* get mod time */
    thistty = ttyname(fileno(SP->term_file));
    if ((tty[0] == '\0') ||			/* check validity of tty */
	(thistty == (char *) 0) ||
	(strcmp(thistty, tty) != 0) ||
	(stat(tty, &ttystat) == -1) ||
	(ttystat.st_mtime != tty_mtime) )	/* dump is no longer valid */
	goto error;

    c = input (file);			/* read # of lines and cols */
    l = input (file);
    if ((c != columns) || (l != lines))
	goto error;

    for (i = 1; i <= l; i++)		/* initialize screen */
	{
	if (!SP->std_body[i])
	    {
	    SP->std_body[i] = _line_alloc ();
	    SP->std_body[i]->length = 0;
	    }
	_line_free (SP->cur_body[i]);
	len = input (file);
	if (len > 0)
	    {
	    SP->cur_body[i] = _line_alloc ();
	    if (SP->cur_body[i] == NULL)
#ifdef LOGERRS
		_IUCerror (0, "NULL line pointer received in scr_init()!");
#else
		;
#endif /* LOGERRS */
	    else
		{
		SP->cur_body[i]->length = len;
		for (j = 0; j < len; j++)
		    SP->cur_body[i]->body[j] = input(file);
		}
	    }
	else
	    SP->cur_body[i] = NULL;
	}

    /* take care of the screen labels */
    if (SP->slk)
	{
	int labmax, labwidth;
	char label[9], *strncpy();
	struct slkdata *SLK = SP->slk;

	(void) input (file);		/* skip whether screen had labels */
	if ((labmax = input (file)) > 0)
	    {
	    SLK->fl_changed = TRUE;
	    labwidth = input (file);
	    label[8] = '\0';
	    for (i = 0; i < labmax; i++)
		{
		for (j = 0; j < labwidth; j++)
		    label[j] = input (file) & A_CHARTEXT;
		strncpy(SLK->scrlabel[i], label, SLK->len);
		SLK->changed[i] = TRUE;
		}
	    }
	}

    SP->phys_y = input (file);
    SP->phys_x = input (file);

    (void) fclose (file);

    SP->doclear = FALSE;
    return OK;

error:
    (void) fclose (file);
    return ERR;
}
