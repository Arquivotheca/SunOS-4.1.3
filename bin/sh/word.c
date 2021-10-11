/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)word.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.11 */
#endif

/*
 * UNIX shell
 */

#include	"defs.h"
#include	"sym.h"


/* ========	character handling for command lines	========*/


word()
{
	register unsigned char	c, d;
	struct argnod	*arg = (struct argnod *)locstak();
	register char	*argp = arg->argval;
	char	*oldargp;
	int		alpha = 1;

	wdnum = 0;
	wdset = 0;

	while (1)
	{
		while (c = nextc(), space(c))		/* skipc() */
			;

		if (c == COMCHAR)
		{
			while ((c = readc()) != NL && c != EOF);
			peekc = c;
		}
		else
		{
			break;	/* out of comment - white space loop */
		}
	}
	if (!eofmeta(c))
	{
		do
		{
			if (c == LITERAL)
			{
				oldargp = argp;
				while ((c = readc()) && c != LITERAL)
				{ /* quote each character within single quotes */
					if (argp >= brkend)
						growstak(argp);
					*argp++ = '\\'; 
					if (argp >= brkend)
						growstak(argp);
					*argp++ = c;
					if (c == NL)
						chkpr();
				}
				if(argp == oldargp) { /* null argument - '' */
				/* Word will be represented by quoted null 
				   in macro.c if necessary */
					if (argp >= brkend)
						growstak(argp);
					*argp++ = '"';
					if (argp >= brkend)
						growstak(argp);
					*argp++ = '"';
				}
			}
			else
			{
				if (argp >= brkend)
					growstak(argp);
				*argp++ = (c);
				if(c == '\\') {
					if (argp >= brkend)
						growstak(argp);
					*argp++ = readc();
				}
				if (c == '=')
					wdset |= alpha;
				if (!alphanum(c))
					alpha = 0;
				if (qotchar(c))
				{
					d = c;
					for (;;)
					{
						if (argp >= brkend)
							growstak(argp);
						if ((*argp++ = (c = nextc())) == 0
						    || c == d)
							break;
						if (c == NL)
							chkpr();
						/* don't interpret quoted 
						   characters */
						if (c == '\\') {
							if (argp >= brkend)
								growstak(argp);
							*argp++ = readc();
						}
					}
				}
			}
		} while ((c = nextc(), !eofmeta(c)));
		argp = endstak(argp);
		if (!letter(arg->argval[0]))
			wdset = 0;

		peekn = c | MARK;
		if (arg->argval[1] == 0 && 
		    (d = arg->argval[0], digit(d)) && 
		    (c == '>' || c == '<'))
		{
			word();
			wdnum = d - '0';
		}
		else		/*check for reserved words*/
		{
			if (reserv == FALSE || (wdval = syslook(arg->argval,reserved, no_reserved)) == 0)
			{
				wdarg = arg;
				wdval = 0;
			}
		}
	}
	else if (dipchar(c))
	{
		if ((d = nextc()) == c)
		{
			wdval = c | SYMREP;
			if (c == '<')
			{
				if ((d = nextc()) == '-')
					wdnum |= IOSTRIP;
				else
					peekn = d | MARK;
			}
		}
		else
		{
			peekn = d | MARK;
			wdval = c;
		}
	}
	else
	{
		if ((wdval = c) == EOF)
			wdval = EOFSYM;
		if (iopend && eolchar(c))
		{
			copy(iopend);
			iopend = 0;
		}
	}
	reserv = FALSE;
	return(wdval);
}

unsigned char skipc()
{
	register unsigned char c;

	while (c = nextc(), space(c))
		;
	return(c);
}

unsigned char nextc()
{
	register unsigned char	c, d;

retry:
	if ((d = readc()) == ESCAPE)
	{
		if ((c = readc()) == NL)
		{
			chkpr();
			goto retry;
		}
		peekc = c | MARK;
	}
	return(d);
}

unsigned char readc()
{
	unsigned register char	c;
	register int	len;
	register struct fileblk *f;

	if (peekn)
	{
		c = peekn;
		peekn = 0;
		return(c);
	}
	if (peekc)
	{
		c = peekc;
		peekc = 0;
		return(c);
	}
	f = standin;
retry:
	if (f->fnxt != f->fend)
	{
		if ((c = *f->fnxt++) == 0)
		{
			if (f->feval)
			{
				if (estabf(*f->feval++))
					c = EOF;
				else
					c = SP;
			}
			else
				goto retry;	/* = c = readc(); */
		}
		if (flags & readpr && standin->fstak == 0)
			prc(c);
		if (c == NL)
			f->flin++;
	}
	else if (f->feof || f->fdes < 0)
	{
		c = EOF;
		f->feof++;
	}
	else if ((len = readb()) <= 0)
	{
		close(f->fdes);
		f->fdes = -1;
		c = EOF;
		f->feof++;
	}
	else
	{
		f->fend = (f->fnxt = f->fbuf) + len;
		goto retry;
	}
	return(c);
}

static
readb()
{
	register struct fileblk *f = standin;
	register int	len;

	if (setjmp(INTbuf) == 0)
		trapjmp = 1;
	do
	{
		if (trapnote & SIGSET)
		{
			newline();
			sigchk();
		}
		else if ((trapnote & TRAPSET) && (rwait > 0))
		{
			newline();
			chktrap();
			clearup();
		}
	} while ((len = read(f->fdes, f->fbuf, f->fsiz)) < 0 && trapnote);
	trapjmp = 0;
	return(len);
}
