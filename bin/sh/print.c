/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)print.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.12 */
#endif

/*
 * UNIX shell
 *
 */

#include	"defs.h"
#include	<sys/param.h>
#include	<ctype.h>

#define		BUFLEN		256

static char	buffer[BUFLEN];
static int	index = 0;
char		numbuf[12];

extern void	prc_buff();
extern void	prs_buff();
extern void	prn_buff();
extern void	prs_cntl();
extern void	prn_buff();

/*
 * printing and io conversion
 */
prp()
{
	if ((flags & prompt) == 0 && cmdadr)
	{
		prs_cntl(cmdadr);
		prs(colon);
	}
}

prs(as)
char	*as;
{
	register char	*s;

	if (s = as)
		write(output, s, length(s) - 1);
}

prc(c)
char	c;
{
	if (c)
		write(output, &c, 1);
}

#define	HZ	60

prt(t)
long	t;
{
	register int hr, min, sec;

	t += HZ / 2;
	t /= HZ;
	sec = t % 60;
	t /= 60;
	min = t % 60;

	if (hr = t / 60)
	{
		prn_buff(hr);
		prc_buff('h');
	}

	prn_buff(min);
	prc_buff('m');
	prn_buff(sec);
	prc_buff('s');
}

prn(n)
	int	n;
{
	itos(n);

	prs(numbuf);
}

itos(n)
{
	register char *abuf;
	register unsigned a, i;
	int pr, d;

	abuf = numbuf;

	pr = FALSE;
	a = n;
	for (i = 10000; i != 1; i /= 10)
	{
		if ((pr |= (d = a / i)))
			*abuf++ = d + '0';
		a %= i;
	}
	*abuf++ = a + '0';
	*abuf++ = 0;
}

stoi(icp)
char	*icp;
{
	register char	*cp = icp;
	register int	r = 0;
	register char	c;

	while ((c = *cp, digit(c)) && c && r >= 0)
	{
		r = r * 10 + c - '0';
		cp++;
	}
	if (r < 0 || cp == icp)
		failed(icp, badnum);
	else
		return(r);
}

prl(n)
long n;
{
	int i;

	i = 11;
	while (n > 0 && --i >= 0)
	{
		numbuf[i] = n % 10 + '0';
		n /= 10;
	}
	numbuf[11] = '\0';
	prs_buff(&numbuf[i]);
}

void
flushb()
{
	if (index)
	{
		buffer[index] = '\0';
		write(1, buffer, length(buffer) - 1);
		index = 0;
	}
}

void
prc_buff(c)
	char c;
{
	if (c)
	{
		if (index + 1 >= BUFLEN)
			flushb();

		buffer[index++] = c;
	}
	else
	{
		flushb();
		write(1, &c, 1);
	}
}

void
prs_buff(s)
	char *s;
{
	register int len = length(s) - 1;

	if (index + len >= BUFLEN)
		flushb();

	if (len >= BUFLEN)
		write(1, s, len);
	else
	{
		movstr(s, &buffer[index]);
		index += len;
	}
}


clear_buff()
{
	index = 0;
}


void
prs_cntl(s)
	register char *s;
{
	register char *ptr = buffer;
	register int buflen;
	register unsigned char c;
	register int meta;

	while ((c = *s++) != '\0') 
	{
		if ((buflen = ptr - buffer) > BUFLEN - 6)
		{
			write(output, buffer, buflen);
			ptr = buffer;
			buflen = 0;
		}

		/* translate a control character into a printable sequence */

		if (isprint(c))
		{	/* printable character */
			*ptr++ = c;
		}
		else
		{
			if (!isascii(c))
			{
				*ptr++ = '(';
				*ptr++ = 'M';
				*ptr++ = '-';
				c = toascii(c);
				meta = 1;
			}
			else
				meta = 0;
			if (iscntrl(c))
			{
				*ptr++ = '^';
				*ptr++ = (c + 0100);
			}
			else if (c == '\177')
			{
				*ptr++ = '^';
				*ptr++ = '?';
			}
			else
				*ptr++ = c;
			if (meta)
				*ptr++ = ')';
		}
	}

	if ((buflen = ptr - buffer) > 0)
		write(output, buffer, buflen);
}


void
prn_buff(n)
	int	n;
{
	itos(n);

	prs_buff(numbuf);
}
