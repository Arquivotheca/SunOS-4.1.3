#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)doscan.c 1.1 92/07/30 SMI"; /* from S5R3.1 2.18 */
#endif
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*LINTLIBRARY*/
#include <stdio.h>
#include <ctype.h>
#include <varargs.h>
#include <values.h>
#include <floatingpoint.h>
#include <errno.h>

#define NCHARS	(1 << BITSPERBYTE)
#define locgetc()	(chcount+=1,getc(iop))
#define locungetc(x)	(chcount-=1,ungetc(x,iop))

extern char *memset();
static int chcount,flag_eof;

#ifdef S5EMUL
#define	isws(c)		isspace(c)
#else
/*
 * _sptab[c+1] is 1 iff 'c' is a white space character according to the
 * 4.2BSD "scanf" definition - namely, SP, TAB, and NL are the only
 * whitespace characters.
 */
static char _sptab[1+256] = {
	0,				/* EOF - not a whitespace char */
	0,0,0,0,0,0,0,0,
	0,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};

#define	isws(c)		((_sptab + 1)[c] != 0)
#endif

int
_doscan(iop, fmt, va_alist)
register FILE *iop;
register unsigned char *fmt;
va_list va_alist;
{
	extern unsigned char *setup();
	char tab[NCHARS];
	register int ch;
	int nmatch = 0, len, inchar, stow, size;
	chcount=0; flag_eof=0;

	/*******************************************************
	 * Main loop: reads format to determine a pattern,
	 *		and then goes to read input stream
	 *		in attempt to match the pattern.
	 *******************************************************/
	for ( ; ; )
	{
		if ( (ch = *fmt++) == '\0')
			return(nmatch); /* end of format */
		if (isws(ch))
		{
		  	if (!flag_eof) 
			{
			   while (isws(inchar = locgetc()))
				;
			   if (inchar == EOF) {
				chcount--;
				flag_eof = 1;
			   }
			   else if (locungetc(inchar) == EOF)
				flag_eof = 1;
			}
		  continue;
		}
		if (ch != '%' || (ch = *fmt++) == '%')
                {
			if ( (inchar = locgetc()) == ch )
				continue;
			if (inchar != EOF) {
				if (locungetc(inchar) != EOF)
					return(nmatch); /* failed to match input */
			} else {
				chcount--;
			}
			break;
		}
		if (ch == '*')
		{
			stow = 0;
			ch = *fmt++;
		}
		else
			stow = 1;

		for (len = 0; isdigit(ch); ch = *fmt++)
			len = len * 10 + ch - '0';
		if (len == 0)
			len = MAXINT;
		if ( (size = ch) == 'l' || (size == 'h') || (size == 'L') )
			ch = *fmt++;
		if (ch == '\0' ||
		    ch == '[' && (fmt = setup(fmt, tab)) == NULL)
			return(EOF); /* unexpected end of format */
		if (isupper(ch))  /* no longer documented */
		{
			size = 'l';
#ifdef S5EMUL
			ch = _tolower(ch);
#else
			ch = tolower(ch);
#endif
		}
		switch(ch)
		{
		 case 'c':
		 case 's':
		 case '[':
			  if ((size = string(stow,ch,len,tab,iop,&va_alist)) < 0)
				goto out;	/* EOF seen, nothing converted */
			  break;
                 case 'n':
			  if (size == 'h')
				*va_arg(va_alist, short *) = (short) chcount;
		          else if (size == 'l')
				*va_arg(va_alist, long *) = (long) chcount;
			  else
			  	*va_arg(va_alist, int *) = (int) chcount;
			  continue;
                 default:
			 if ((size = number(stow, ch, len, size, iop, &va_alist)) < 0)
				goto out;	/* EOF seen, nothing converted */
			 break;
                 }
		   if (size)
			nmatch += stow;
		   else 
			return((flag_eof && !nmatch) ? EOF : nmatch);
		continue;
	}
out:
	return (nmatch != 0 ? nmatch : EOF); /* end of input */
}

/***************************************************************
 * Functions to read the input stream in an attempt to match incoming
 * data to the current pattern from the main loop of _doscan().
 ***************************************************************/
static int
number(stow, type, len, size, iop, listp)
int stow, type, len, size;
register FILE *iop;
va_list *listp;
{
	char numbuf[64], inchar, lookahead;
	register char *np = numbuf;
	register int c, base;
	int digitseen = 0, floater = 0, negflg = 0;
	long lcval = 0;
	switch(type)
	{
	case 'e':
	case 'f':
	case 'g':
		floater++;
	case 'd':
	case 'u':
	case 'i':
		base = 10;
		break;
	case 'o':
		base = 8;
		break;
	case 'x':
		base = 16;
		break;
	default:
		return(0); /* unrecognized conversion character */
	}
	if (!flag_eof)
	{
		while (isws(c = locgetc()))
			;
	}
	else
		c = locgetc();
	if (c == EOF) {
		chcount--;
		return(-1);	/* EOF before match */
	}
        if (floater != 0) {     /* Handle floating point with
                                 * file_to_decimal. */
                decimal_mode    dm;
                decimal_record  dr;
                fp_exception_field_type efs;
                enum decimal_string_form form;
                char           *echar;
                int             nread, ic;
                char            buffer[1024];
                char           *nb = buffer;

                locungetc(c);
		if (len > 1024)
			len = 1024;
                file_to_decimal(&nb, len, 0, &dr, &form, &echar, iop, &nread);
                if (stow && (form != invalid_form)) {
                        dm.rd = fp_direction;
                        if (size == 'l') {      /* double */
                                decimal_to_double((double *) va_arg(*listp, double *), &dm, &dr, &efs);
                        } else if (size == 'L') {      /* quad */
                                decimal_to_quadruple((quadruple *)va_arg(*listp, double *), &dm, &dr, &efs);
                        } else {/* single */
                                decimal_to_single((float *) va_arg(*listp, float *), &dm, &dr, &efs);
                        }          
                }
		chcount += nread;	/* Count characters read. */
                c = *nb;        /* Get first unused character. */
                ic = c;
                if (c == NULL) {
                        ic = locgetc();
                        c = ic;
                        /*
                         * If null, first unused may have been put back
                         * already.
                         */
                }         
                if (ic == EOF) {
                        chcount--;
                        flag_eof = 1;
                } else if (locungetc(c) == EOF)
                        flag_eof = 1;
                return ((form == invalid_form) ? 0 : 1);        /* successful match if
                                                                 * non-zero */
        }
	switch(c) {
	case '-':
		negflg++;
		if (type == 'u')
			break;
	case '+': /* fall-through */
		if (--len <= 0)
			break;
		if ( (c = locgetc()) != '0')
			break;
        case '0':
                if ( (type != 'i') || (len <= 1) )  
		   break;
	        if ( ((inchar = locgetc()) == 'x') || (inchar == 'X') ) 
	        {
		      /* If not using sscanf and *
		       * at the buffer's end     *
		       * then LOOK ahead         */

                   if ( (iop->_flag & _IOSTRG) || (iop->_cnt != 0) )
		      lookahead = locgetc();
		   else
		   {
		      if ( read(fileno(iop),np,1) == 1)
		         lookahead = *np;
                      else
		         lookahead = EOF;
                      chcount += 1;
                   }    
		   if ( isxdigit(lookahead) )
		   {
		       base =16;

		       if ( len <= 2)
		       {
			  locungetc(lookahead);
			  len -= 1;            /* Take into account the 'x'*/
                       }
		       else 
		       {
		          c = lookahead;
			  len -= 2;           /* Take into account '0x'*/
		       }
                   }
	           else
	           {
	               locungetc(lookahead);
	               locungetc(inchar);
                   }
		}
	        else
	        {
		    locungetc(inchar);
	            base = 8;
                }
	}
	if (!negflg || type != 'u')
	    for (; --len  >= 0 ; *np++ = c, c = locgetc()) 
	    {
		if (np > numbuf + 62)           
		{
		    errno = ERANGE;
		    return(0);
                }
		if (isdigit(c))
		{
			register int digit;
			digit = c - '0';
			if (base == 8)
			{
				if (digit >= 8)
					break;
				if (stow)
					lcval = (lcval<<3) + digit;
			}
			else
			{
				if (stow)
				{
					if (base == 10)
						lcval = (((lcval<<2) + lcval)<<1) + digit;
					else /* base == 16 */
						lcval = (lcval<<4) + digit;
				}
			}
			digitseen++;


			continue;
		}
		else if (base == 16 && isxdigit(c))
		{
			register int digit;
			digit = c - (isupper(c) ? 'A' - 10 : 'a' - 10);
			if (stow)
				lcval = (lcval<<4) + digit;
			digitseen++;
			continue;
		}
		break;
	    }


	if (stow && digitseen)
		{
	 	/* suppress possible overflow on 2's-comp negation */
			if (negflg && lcval != HIBITL)
				lcval = -lcval;
			if (size == 'l')
				*va_arg(*listp, long *) = lcval;
			else if (size == 'h')
				*va_arg(*listp, short *) = (short)lcval;
			else
				*va_arg(*listp, int *) = (int)lcval;
		}
	if (c == EOF) {
		chcount--;
		flag_eof=1;
	} else if (locungetc(c) == EOF)
		flag_eof=1;
	return (digitseen); /* successful match if non-zero */
}

static int
string(stow, type, len, tab, iop, listp)
register int stow, type, len;
register char *tab;
register FILE *iop;
va_list *listp;
{
	register int ch;
	register char *ptr;
	char *start;

	start = ptr = stow ? va_arg(*listp, char *) : NULL;
	if (type == 's')
	{
		if (!flag_eof)
		{
			while (isws(ch = locgetc()))
				;
		}
		else
			ch = locgetc();
		if (ch == EOF)
			return(-1);	/* EOF before match */
		while (ch != EOF && !isws(ch))
		{
			if (stow)
				*ptr = ch;
			ptr++;
			if (--len <= 0)
				break;
			ch = locgetc();
		}
	} else if (type == 'c') {
		if (len == MAXINT)
			len = 1;
		while ( (ch = locgetc()) != EOF)
		{
			if (stow)
				*ptr = ch;
			ptr++;
			if (--len <= 0)
				break;
		}
	} else { /* type == '[' */
		while ( (ch = locgetc()) != EOF && !tab[ch])
		{
			if (stow)
				*ptr = ch;
			ptr++;
			if (--len <= 0)
				break;
		}
	}
	if (ch == EOF )
	{
		chcount-=1;
		flag_eof = 1;
	}
	else if (len > 0 && locungetc(ch) == EOF)
		flag_eof = 1;
	if (ptr == start)
		return(0); /* no match */
	if (stow && type != 'c')
		*ptr = '\0';
	return (1); /* successful match */
}

static unsigned char *
setup(fmt, tab)
register unsigned char *fmt;
register char *tab;
{
	register int b, c, d, t = 0;

	if (*fmt == '^')
	{
		t++;
		fmt++;
	}
	(void) memset(tab, !t, NCHARS);
	if ( (c = *fmt) == ']' || c == '-')  /* first char is special */
	{
		tab[c] = t;
		fmt++;
	}
	while ( (c = *fmt++) != ']')
	{
		if (c == '\0')
			return(NULL); /* unexpected end of format */
		if (c == '-' && (d = *fmt) != ']' && (b = fmt[-2]) < d)
		{
			(void) memset(&tab[b], t, d - b + 1);
			fmt++;
		}
		else
			tab[c] = t;
	}
	return (fmt);
}
