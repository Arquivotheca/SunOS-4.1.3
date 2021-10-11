/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tparm.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.8 */
#endif

/* Copyright (c) 1979 Regents of the University of California */

#include	"curses_inc.h"

#ifndef	_CHCTRL
#define	_CHCTRL(c)	('c' & 037)
#endif	/* _CHCTRL */

char	*_branchto();

/*
 * Routine to perform parameter substitution.
 * instring is a string containing printf type escapes.
 * The whole thing uses a stack, much like an HP 35.
 * The following escapes are defined for substituting row/column:
 *
 *	%[:[-+ #0]][0-9][.][0-9][dsoxX]
 *		print pop() as in printf(3), as defined in the local
 *		sprintf(3), except that a leading + or - must be preceded
 *		with a colon (:) to distinguish from the plus/minus operators.
 *
 *	%c	print pop() like %c in printf(3)
 *	%l	pop() a string address and push its length.
 *	%P[a-z] set dynamic variable a-z
 *	%g[a-z] get dynamic variable a-z
 *	%P[A-Z] set static variable A-Z
 *	%g[A-Z] get static variable A-Z
 *
 *	%p[1-0]	push ith parm
 *	%'c'	char constant c
 *	%{nn}	integer constant nn
 *
 *	%+ %- %* %/ %m		arithmetic (%m is mod): push(pop() op pop())
 *	%& %| %^		bit operations:		push(pop() op pop())
 *	%= %> %<		logical operations:	push(pop() op pop())
 *	%A %O			logical AND, OR		push(pop() op pop())
 *	%! %~			unary operations	push(op pop())
 *	%%			output %
 *	%? expr %t thenpart %e elsepart %;
 *				if-then-else, %e elsepart is optional.
 *				else-if's are possible ala Algol 68:
 *				%? c1 %t %e c2 %t %e c3 %t %e c4 %t %e %;
 *	% followed by anything else
 *				is not defined, it may output the character,
 *				and it may not. This is done so that further
 *				enhancements to the format capabilities may
 *				be made without worrying about being upwardly
 *				compatible from buggy code.
 *
 * all other characters are ``self-inserting''.  %% gets % output.
 *
 * The stack structure used here is based on an idea by Joseph Yao.
 */

#define	_PUSH(i)	(stack[++top] = (i))
#define	_POP()		(stack[top--])

/* VARARGS */
char	*
tparm(instring, fp1, fp2, p3, p4, p5, p6, p7, p8, p9)
char	*instring;
long	fp1, fp2, p3, p4, p5, p6, p7, p8 , p9;
{
    static	char	result[512];
    static	char	added[100];
    int		vars[26];
    long	stack[10], top = 0;
    register	char	*cp = instring;
    register	char	*outp = result;
    register	int	c;
    register	int	op;
    int		sign;
    int		onrow = 0;
    long	p1 = fp1, p2 = fp2;	/* copy in case < 2 actual parms */
    char	*xp;
    char	formatbuffer[100];
    char	*format;
    int		looping;
    short	*regs = cur_term->_regs;

#ifdef	SYSV
    int		sprintf();
#else	/* SYSV */
    char	*sprintf();
#endif	/* SYSV */

    if (instring == 0)
    {
#ifdef	DEBUG
    if (outf)
	fprintf(outf, "TPARM: null arg\n");
#endif	/* DEBUG */
	return (0);
    }
    added[0] = 0;

    while (c = *cp++)
    {
	if (c != '%')
	{
	    *outp++ = c;
	    continue;
	}
	op = stack[top];
	switch (c = *cp++)
	{
	    /* PRINTING CASES */
	    case ':':
	    case ' ':
	    case '#':
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	    case '.':
	    case 'd':
	    case 's':
	    case 'o':
	    case 'x':
	    case 'X':
		format = formatbuffer;
		*format++ = '%';

		/* leading ':' to allow +/- in format */
		if (c == ':')
		    c = *cp++;

		/* take care of flags, width and precision */
		looping = 1;
		while (c && looping)
		    switch (c)
		    {
			case '-':
			case '+':
			case ' ':
			case '#':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
			    *format++ = c;
			    c = *cp++;
			    break;
			default:
			    looping = 0;
		    }

		/* add in the conversion type */
		switch (c)
		{
		    case 'd':
		    case 's':
		    case 'o':
		    case 'x':
		    case 'X':
			*format++ = c;
			break;
		    default:
#ifdef	DEBUG
			    if (outf)
				fprintf(outf, "TPARM: invalid conversion type\n");
#endif	/* DEBUG */
			    return (0);
		}
		*format = '\0';

		/*
		 * Pass off the dirty work to sprintf.
		 * It's debatable whether we should just pull in
		 * the appropriate code here. I decided not to for now.
		 */
		if (c == 's')
		    (void) sprintf(outp, formatbuffer, (char *) op);
		else
		    (void) sprintf(outp, formatbuffer, op);
		/*
		 * Advance outp past what sprintf just did. 
		 * sprintf returns an indication of its length on some
		 * systems, others the first char, and there's
		 * no easy way to tell which. The Sys V on
		 * BSD emulations are particularly confusing.
		 */
		while (*outp)
		    outp++;
		(void) _POP();
		continue;

	    case 'c':
		/*
		 * This code is worth scratching your head at for a
		 * while.  The idea is that various weird things can
		 * happen to nulls, EOT's, tabs, and newlines by the
		 * tty driver, arpanet, and so on, so we don't send
		 * them if we can help it.  So we instead alter the
		 * place being addessed and then move the cursor
		 * locally using UP or RIGHT.
		 *
		 * This is a kludge, clearly.  It loses if the
		 * parameterized string isn't addressing the cursor
		 * (but hopefully that is all that %c terminals do
		 * with parms).  Also, since tab and newline happen
		 * to be next to each other in ASCII, if tab were
		 * included a loop would be needed.  Finally, note
		 * that lots of other processing is done here, so
		 * this hack won't always work (e.g. the Ann Arbor
		 * 4080, which uses %B and then %c.)
		 */
		switch (op)
		{
		    /*
		     * Null.  Problem is that our 
		     * output is, by convention, null terminated.
		     */
		    case 0:
			op = 0200;   /* Parity should be ignored */
			break;
			/*
			 * Control D.  Problem is that certain very ancient
			 * hardware hangs up on this, so the current (!)
			 * UNIX tty driver doesn't xmit control D's.
			 */
		    case _CHCTRL(d):
			/*
			 * Newline.  Problem is that UNIX will expand
			 * this to CRLF.
			 */
		    case '\n':
			xp = onrow ? cursor_down : cursor_right;
			if (onrow && xp && op < lines-1 && cursor_up)
			{
			    op += 2;
			    xp = cursor_up;
			}
			if (xp && instring == cursor_address)
			{
			    (void) strcat(added, xp);
			    op--;
			}
			break;
			/*
			 * Tab used to be in this group too,
			 * because UNIX might expand it to blanks.
			 * We now require that this tab mode be turned
			 * off by any program using this routine,
			 * or using termcap in general, since some
			 * terminals use tab for other stuff, like
			 * nondestructive space.  (Filters like ul
			 * or vcrt will lose, since they can't stty.)
			 * Tab was taken out to get the Ann Arbor
			 * 4080 to work.
			 */
		}

		*outp++ = op;
		(void) _POP();
		break;

	    case 'l':
		xp = (char *) _POP();
		_PUSH(strlen(xp));
		break;

	    case '%':
		*outp++ = c;
		break;

	    /*
	     * %i: shorthand for increment first two parms.
	     * Useful for terminals that start numbering from
	     * one instead of zero (like ANSI terminals).
	     */
	    case 'i':
		p1++;
		p2++;
		break;

	    /* %pi: push the ith parameter */
	    case 'p':
		switch (c = *cp++)
		{
		    case '1':
			_PUSH(p1);
			break;
		    case '2':
			_PUSH(p2);
			break;
		    case '3':
			_PUSH(p3);
			break;
		    case '4':
			_PUSH(p4);
			break;
		    case '5':
			_PUSH(p5);
			break;
		    case '6':
			_PUSH(p6);
			break;
		    case '7':
			_PUSH(p7);
			break;
		    case '8':
			_PUSH(p8);
			break;
		    case '9':
			_PUSH(p9);
			break;
		    default:
#ifdef	DEBUG
			if (outf)
			    fprintf(outf, "TPARM: bad parm number\n");
#endif	/* DEBUG */
			return (0);
		}
		onrow = (c == '1');
		break;

	/* %Pi: pop from stack into variable i (a-z) */
	    case 'P':
		if (*cp >= 'a' && *cp <= 'z')
		    vars[*cp++ - 'a'] = _POP();
		else
		    if (*cp >= 'A' && *cp <= 'Z')
			regs[*cp++ - 'a'] = _POP();
#ifdef	DEBUG
		    else
			if (outf)
			    fprintf(outf, "TPARM: bad register name\n");
#endif	/* DEBUG */
		break;

	    /* %gi: push variable i (a-z) */
	    case 'g':
		if (*cp >= 'a' && *cp <= 'z')
		    _PUSH(vars[*cp++ - 'a']);
		else
		    if (*cp >= 'A' && *cp <= 'Z')
			_PUSH(regs[*cp++ - 'a']);
#ifdef	DEBUG
		    else
			if (outf)
			    fprintf(outf, "TPARM: bad register name\n");
#endif	/* DEBUG */
		break;

	    /* %'c' : character constant */
	    case '\'':
		_PUSH(*cp++);
		if (*cp++ != '\'')
		{
#ifdef	DEBUG
		    if (outf)
			fprintf(outf, "TPARM: missing closing quote\n");
#endif	/* DEBUG */
		    return (0);
		}
		break;

	    /* %{nn} : integer constant.  */
	    case '{':
		op = 0;
		sign = 1;
		if (*cp == '-')
		{
		    sign = -1;
		    cp++;
		}
		else
		    if (*cp == '+')
			cp++;
		while ((c = *cp++) >= '0' && c <= '9')
		{
			op = 10 * op + c - '0';
		}
		if (c != '}')
		{
#ifdef	DEBUG
		    if (outf)
			fprintf(outf, "TPARM: missing closing brace\n");
#endif	/* DEBUG */
		    return (0);
		}
		_PUSH(sign * op);
		break;

	    /* binary operators */
	    case '+':
		c = _POP();
		op = _POP();
		_PUSH(op + c);
		break;
	    case '-':
		c = _POP();
		op = _POP();
		_PUSH(op - c);
		break;
	    case '*':
		c = _POP();
		op = _POP();
		_PUSH(op * c);
		break;
	    case '/':
		c = _POP();
		op = _POP();
		_PUSH(op / c);
		break;
	    case 'm':
		c = _POP();
		op = _POP();
		_PUSH(op % c);
		break; /* %m: mod */
	    case '&':
		c = _POP();
		op = _POP();
		_PUSH(op & c);
		break;
	    case '|':
		c = _POP();
		op = _POP();
		_PUSH(op | c);
		break;
	    case '^':
		c = _POP();
		op = _POP();
		_PUSH(op ^ c);
		break;
	    case '=':
		c = _POP();
		op = _POP();
		_PUSH(op == c);
		break;
	    case '>':
		c = _POP();
		op = _POP();
		_PUSH(op > c);
		break;
	    case '<':
		c = _POP();
		op = _POP();
		_PUSH(op < c);
		break;
	    case 'A':
		c = _POP();
		op = _POP();
		_PUSH(op && c);
		break; /* AND */
	    case 'O':
		c = _POP();
		op = _POP();
		_PUSH(op || c);
		break; /* OR */

	    /* Unary operators. */
	    case '!':
		stack[top] = !stack[top];
		break;
	    case '~':
		stack[top] = ~stack[top];
		break;
	    /* Sorry, no unary minus, because minus is binary. */

	    /*
	     * If-then-else.  Implemented by a low level hack of
	     * skipping forward until the match is found, counting
	     * nested if-then-elses.
	     */
	    case '?':	/* IF - just a marker */
		break;

	    case 't':	/* THEN - branch if false */
		if (!_POP())
		    cp = _branchto(cp, 'e');
		break;

	    case 'e':	/* ELSE - branch to ENDIF */
		cp = _branchto(cp, ';');
		break;

	    case ';':	/* ENDIF - just a marker */
		break;

	    default:
#ifdef	DEBUG
		if (outf)
		    fprintf(outf, "TPARM: bad % sequence\n");
#endif	/* DEBUG */
		return (0);
	}
    }
    (void) strcpy(outp, added);
    return (result);
}

char	*
_branchto(cp, to)
register	char	*cp;
char		to;
{
    register	int	level = 0;
    register	char	c;

    while (c = *cp++)
    {
	if (c == '%')
	{
	    if ((c = *cp++) == to || c == ';')
	    {
		if (level == 0)
		{
		    return (cp);
		}
	    }
	    if (c == '?')
		level++;
	    if (c == ';')
		level--;
	}
    }
#ifdef	DEBUG
    if (outf)
	fprintf(outf, "TPARM: no matching ENDIF");
#endif	/* DEBUG */
    return (0);
}
