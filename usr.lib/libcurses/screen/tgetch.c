/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static        char sccsid[] = "@(#)tgetch.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.5 */
#endif

#include	"curses_inc.h"
#include	<signal.h>
#include	<errno.h>
#ifdef FIONREAD
#include	<sys/types.h>
#include	<sys/time.h>
#endif  /* FIONREAD */
#ifdef	DEBUG
#include	<ctype.h>
#endif	/* DEBUG */

#ifdef FIONREAD
# ifdef FD_SET
static int	ndescriptors = 0;
# endif  /* FD_SET */
#endif  /* FIONREAD */

/*
 * Read a key typed from the terminal
 *
 * interpret:	= 0 for single-char key only
 * 	 	= 1 for matching function key and macro patterns.
 * 		= 2 same as 1 but no time-out for funckey matching.
 */

int
tgetch(interpret)
int	interpret;
{
    int			inp, i = 0, j, collapse = 1;
    register	short	*inputQ = cur_term->_input_queue;
    char		*chars_onQ = &(cur_term->_chars_on_queue);

#if	defined(SYSV) && !defined(FIONREAD)
    /*
     * Register the fact that getch is being used so
     * that typeahead checking can be done.
     * This code should GO AWAY when a poll() or FIONREAD can
     * be done on the file descriptor as then the check
     * will be non-destructive.
     */
    cur_term->fl_typeahdok = TRUE;
#endif	/* SYSV && !FIONREAD */

    /* ask for input */
    if (cur_term->_ungotten > 0)
    {
	cur_term->_ungotten--;
	/* decode an ungetch()'d character */
	inp = -(inputQ[0] + 0100);
    }
    else
    {
	/* Only read a character if there is no typeahead/peekahead. */
	if (*chars_onQ == 0)
	{
	    (*chars_onQ)++;
#ifdef	FIONREAD
	    inp = _readchar();
#else	/* FIONREAD */
	    if ((inp = _pk()) == ERR)
	    {
		/*
		 * interpret is set to 0 so that down below we don't
		 * drop into getkey since we already know there can't be
		 * a key that starts with -1.  Also, we don't want to
		 * access funckeystarter[-1].
		 */
		interpret = FALSE;
	    }
#endif	/* FIONREAD */
	}
	else
	    inp = inputQ[0];

#ifdef	DEBUG
	if (outf)
	  {
	    if (inp >= 0)
		fprintf(outf, "TGETCH read '%s'\n", unctrl(inp));
	    else
		fprintf(outf, "TGETCH read %d\n", inp);	/* ERR */
	  }
#endif	/* DEBUG */

	/* Check for arrow and function keys */
	if (interpret && cur_term->funckeystarter[inp])
	    collapse = _getkey(interpret - 1, &inp);
    }

/* Collapse the input queue to remove the escape sequence from the stack. */

    j = *chars_onQ;
    (*chars_onQ) -= collapse;
    while (collapse < j)
	inputQ[i++] = inputQ[collapse++];
    return (inp);
}

#ifdef	FIONREAD
static	int
_readchar()
{
    register	int	i;
    unsigned	char	c;

    if (cur_term->_delay == 0)
    {
	int	arg;

	(void) ioctl(cur_term->_inputfd, FIONREAD, &arg);
#ifdef	DEBUG
	if (outf) fprintf(outf, "FIONREAD returns %d\n", arg);
#endif	/* DEBUG */
	if (arg < 1)
	    return (-1);
    }
    else
	if (cur_term->_delay > 0)
	{
	    char	c;
#ifdef FD_SET
	    fd_set	infd;
#else
	    int		infd;
#endif
	    struct timeval t;

#ifdef FD_SET
	    if (ndescriptors == 0)
		ndescriptors = getdtablesize();
	    FD_ZERO(&infd);
	    FD_SET(cur_term->_inputfd, &infd);
#else
	    infd = 1 << cur_term->_inputfd;
#endif
	    t.tv_sec = cur_term->_delay / 1000;
	    t.tv_usec = (cur_term->_delay % 1000) * 1000;
#ifdef FD_SET
	    i = select(ndescriptors, &infd, (fd_set *)NULL, (fd_set *)NULL,
	        &t);
#else
	    i = select(20, &infd, (int *)NULL, (int *)NULL, &t);
#endif
	    if (i < 0)
		return (ERR);
	}

    i = read(cur_term->_inputfd, &c, 1);

#ifdef	DEBUG
    if (outf)
	fprintf(outf, "read from %d returns %d chars, first %o\n",
	    cur_term->_inputfd, i, c);
#endif	/* DEBUG */

    if (i > 0)
	return (c);
    else
	return (ERR);
}
#endif	/* !FIONREAD */

#ifdef	DEBUG
extern	char	*_asciify();
#endif	/* DEBUG */

    /*
     * This algorithm is a "learning" algorithm. The premise is
     * that keys used once are like to be used again and again.
     * Since the time for a linear search of the table is so
     * expensive, we move keys that are found up to the top of
     * the list, making the access to a repeated key very fast and
     * keys that have been used before close to the top.
     */

static	int
_getkey(blockpeek, inp)
register	int	*inp;
register	int	blockpeek;
{
    register	_KEY_MAP	**kp = cur_term->_keys;
    register	int		i, key, num_keys = cur_term->_ksz;
    short			*inputQ = cur_term->_input_queue;
    char			*chars_onQ = &(cur_term->_chars_on_queue),
				flag = cur_term->funckeystarter[*inp];
    int				first, collapse = 1;

#ifdef	DEBUG
    if (outf)
	fprintf(outf, "getkey(): looking in linear table, inp=%d\n", *inp);
#endif	/* DEBUG */

    if (flag & _KEY)
	key = 0;
    else
    {
	key = cur_term->_first_macro;
	blockpeek = TRUE;
    }
    first = key;

    for (; key < num_keys; key++)
    {
	if (kp[key]->_sends[0] == *inp)
	{
	    for (i = 1; i < INP_QSIZE; i++)
	    {
		/* found it? */
		if (kp[key]->_sends[i] == '\0')
		    break;
		/* partial match? peek ahead. */
		if (*chars_onQ == i)
		{
		    (*chars_onQ)++;
		    switch (inputQ[i] = (blockpeek) ? _pk() : _fpk())
		    {
			case -2:
			    /*
			     * Since -2 signifies a timeout we don't really
			     * want to put it on the queue so we decrement
			     * our counter.
			     */
			    (*chars_onQ)--;
#ifdef	DEBUG
			    if (outf)
				fprintf(outf, "Timed out\n");
#endif	/* DEBUG */
			    if (flag & _MACRO)
			    {
#ifdef	DEBUG
				if (outf)
				    fprintf(outf, "Found macro\n");
#endif	/* DEBUG */
				/*
				 * We have to decrement one because key will be
				 * incremented at the bottom of the out loop.
				 */
				key = (first = blockpeek = cur_term->_first_macro) - 1;
				goto outerloop;
			    }
			case -1:
			    goto ret;
		    }
		}

		/* not this one? */
		if (kp[key]->_sends[i] != inputQ[i])
		    goto outerloop;
	    }

	    /* We found it! Read in any chars left in _sends */

	    if ((collapse = i) == INP_QSIZE)
		for ( ; kp[key]->_sends[i]; i++)
		    (void) _fpk();

	    /* move key to top of ordered list */
	    if (key != first)
	    {
		_KEY_MAP	*savekey = kp[key];
		short		*lorder;
		int		j;

		if (key > cur_term->_first_macro)
		    lorder = &(cur_term->_lastmacro_ordered);
		else
		    lorder = &(cur_term->_lastkey_ordered);
		/*
		 * If we're below the last ordered key, swap next unordered
		 * key with this one and ripple from there.
		 */
		if (key > *lorder)
		    kp[key] = kp[(i = ++(*lorder))];
		else
		    i = key;
		/* ripple the ordered keys down */
		for (j = i--; j > first; )
		    kp[j--] = kp[i--];
		kp[first] = savekey;
	    }
	    *inp = kp[first]->_keyval;
	    goto ret;
	}
outerloop:
	;
    }

ret:
    /* key not found */
#ifdef	DEBUG
    if (outf)
	if (key == num_keys)
	    fprintf(outf, "Did not match anything.\n");
#endif	/* DEBUG */
    return (collapse);
}

/*
 * Fast peek key.  Like getchar but if the right flags are set, times out
 * quickly if there is nothing waiting, returning -1.
 * f is an output stdio descriptor, we read from the fileno.
 * We wait for long enough for a terminal to send another character
 * (at 15cps repeat rate, this is 67 ms, I'm using 100ms to allow
 * a bit of a fudge factor) and time out more quickly.
 * -2 is returned if we time out, -1 is returned if interrupted, and the
 * character is returned otherwise.
 */

#ifndef	FIONREAD

/*
 * Traditional implementation.  The best resolution we have is 1 second,
 * so we set a 1 second alarm and try to read.  If we fail for 1 second,
 * we assume there is no key waiting.  Problem here is that 1 second is
 * too long; people can type faster than this.
 *
 * Another possible implementation of changing VMIN/VTIME before and
 * after each read does not work because the tty driver's timeout
 * mechanism is too unreliable when the timeouts are changed too quickly.
 */

static	char	sig_caught;

static
_catch_alarm()
{
    sig_caught = 1;
}

static
_fpk()
{
    extern	int	_catch_alarm();
    unsigned	char	c;
    int		rc, infd = cur_term->_inputfd;
#ifdef	SIGPOLL	/* Vr3 and beyond */
    void	(*oldsig)();
#else	/* SIGPOLL */
    int		(*oldsig)();
#endif	/* SIGPOLL */
    unsigned	int	oldalarm, alarm();

    /* turn off any user alarms and set our own */
    oldalarm = alarm(0);
    sig_caught = 0;
    oldsig = signal(SIGALRM, _catch_alarm);
    (void) alarm(1);
    rc = read(cur_term->_inputfd, (char *) &c, 1);
    (void) alarm(0);

    /*
     * This code is to take care of the possibility of
     * the process getting swapped out in the middle of
     * read() call above. The interrupt will cause the
     * read() call to return, even if a character is really
     * on the clist. So we do a non-blocking read() to make
     * sure that there really isn't a character there.
     */

    if (sig_caught && rc != 1)
	if (cur_term->_check_fd != -1)
	    rc = read(cur_term->_check_fd, (char *) &c, 1);
	else
	{
#include	<fcntl.h>
	    register	int	fcflags = fcntl(infd, F_GETFL);

	    (void) fcntl(infd, F_SETFL, fcflags | O_NDELAY);
	    rc = read(infd, (char *) &c, 1);
	    (void) fcntl(infd, F_SETFL, fcflags);
	}

    /* restore the user alarms */
	(void) signal(SIGALRM, oldsig);
	if (sig_caught && oldalarm > 1)
	    oldalarm--;
	(void) alarm(oldalarm);
	if (rc == 1)			/* got a character */
	    return (c);
	else
	    if (sig_caught)		/* timed out */
		return (-2);
	    else			/* EOF or got interrupted */
		return (-1);
}
#else	/* FIONREAD */
/*
 * If we have the select system call, we can do much better than the
 * traditional method. Even if we don't have the real 4.2BSD select, we
 * can emulate it with napms and FIONREAD.  napms might be done with only
 * 1 second resolution, but this is no worse than what we have in the
 * traditional implementation.
 */
static
_fpk()
{
    int		rc;
#ifdef FD_SET
    fd_set	infd;
#else
    int		infd;
#endif  /* FD_SET */
    unsigned	char	c;
    struct	timeval t;

#ifdef FD_SET
    if (ndescriptors == 0)
	ndescriptors = getdtablesize();
    FD_ZERO(&infd);
    FD_SET(cur_term->_inputfd, &infd);
#else
    infd = 1 << cur_term->_inputfd;
#endif  /* FD_SET */
    t.tv_sec = 0;
    t.tv_usec = 250000;		/* 250 milliseconds */
#ifdef FD_SET
    rc = select(ndescriptors, &infd, (fd_set *)NULL, (fd_set *)NULL, &t);
#else
    rc = select(20, &infd, (int *)NULL, (int *)NULL, &t);
#endif  /* FD_SET */

    if (rc < 0)
	rc = -1;   /* error */
    else if (rc == 0)
	rc = -2;   /* timeout */
    else
	/* (rc == 1) : input ready */
	rc = (read(cur_term->_inputfd, &c, 1) == 1) ? c : -1;

    return (rc);
}
#endif	/* FIONREAD */

/*
 * Plain peekchar function.  Nothing fancy.  This is just like _fpk
 * but will wait forever rather than time out.
 */

static
_pk()
{
    unsigned	char	c;

    return ((read(cur_term->_inputfd, (char *) &c, 1) == 1) ? c : ERR);
}
