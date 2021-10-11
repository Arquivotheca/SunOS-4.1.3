/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)sh.print.c 1.1 92/07/30 SMI; from UCB 5.2 6/6/85";
#endif

#include "sh.h"
#include <sys/ioctl.h>
/*
 * C Shell
 */

psecs(l)
	long l;
{
	register int i;

	i = l / 3600;
	if (i) {
		printf("%d:", i);
		i = l % 3600;
		p2dig(i / 60);
		goto minsec;
	}
	i = l;
	printf("%d", i / 60);
minsec:
	i %= 60;
	printf(":");
	p2dig(i);
}

p2dig(i)
	register int i;
{

	printf("%d%d", i / 10, i % 10);
}

char linbuf[128];
char *linp = linbuf;

#ifdef MBCHAR
/* putbyte() send a byte to SHOUT.  No interpretation is done
 * except an un-QUOTE'd control character, which is displayed 
 * as ^x.
 */
putbyte(c)
	register int c;
{

	if ((c & QUOTE) == 0 && (c == 0177 || c < ' ' && c != '\t' && c != '\n')) {
		putbyte('^');
		if (c == 0177)
			c = '?';
		else
			c |= 'A' - 1;
	}
	c &= TRIM;
	*linp++ = c;

	if (c == '\n' || linp >= &linbuf[sizeof linbuf - 1 - MB_CUR_MAX])
		flush();/* 'cause the next putchar() call may
			  * overflow the buffer.
			  */
}

/* putchar(tc) does what putbyte(c) do for a byte c.
 * Note that putbyte(c) just send the byte c (provided c is not
 * a control character) as it is, while putchar(tc) may expand the
 * character tc to some byte sequnce that represents the character
 * in EUC form.
 */
putchar(tc)
     register tchar tc;
{
	int	n;

	if( isascii(tc&TRIM) ){
		putbyte((int)tc);
		return;
	}
	tc &= TRIM;
	n=wctomb(linp, tc);
	if(n==-1) return;
	linp+=n;
	if ( linp >= &linbuf[sizeof linbuf - 1 - MB_CUR_MAX])
		flush(); 
}
#else/*!MBCHAR*/
/* putbyte() send a byte to SHOUT.  No interpretation is done
 * except an un-QUOTE'd control character, which is displayed 
 * as ^x.
 */
putbyte(c)
	register int c;
{

	if ((c & QUOTE) == 0 && (c == 0177 || c < ' ' && c != '\t' && c != '\n')) {
		putbyte('^');
		if (c == 0177)
			c = '?';
		else
			c |= 'A' - 1;
	}
	c &= TRIM;
	*linp++ = c;
	if (c == '\n' || linp >= &linbuf[sizeof linbuf - 2])
		flush();
}

/* putchar(tc) does what putbyte(c) do for a byte c.
 * For single-byte character only environment, there is no
 * difference between putchar() and putbyte() though.
 */
putchar(tc)
     register tchar tc;
{
	putbyte((int)tc);
}
#endif/*!MBCHAR*/
draino()
{

	linp = linbuf;
}

flush()
{
	register int unit;
	int lmode;

	if (linp == linbuf)
		return;
	if (haderr)
		unit = didfds ? 2 : SHDIAG;
	else
		unit = didfds ? 1 : SHOUT;
#ifdef TIOCLGET
	if (didfds == 0 && ioctl(unit, TIOCLGET,  (char *)&lmode) == 0 &&
	    lmode&LFLUSHO) {
		lmode = LFLUSHO;
		(void) ioctl(unit, TIOCLBIC,  (char *)&lmode);
		(void) write(unit, "\n", 1);
	}
#endif
	(void) write(unit, linbuf, linp - linbuf);
	linp = linbuf;
}

/*
 * Should not be needed.
 */
write_string(s)
	char *s;
{
	register int unit;
	/*
	 * First let's make it sure to flush out things.
	 */
	 flush();

	if (haderr)
		unit = didfds ? 2 : SHDIAG;
	else
		unit = didfds ? 1 : SHOUT;

	(void) write(unit, s, strlen(s));
}

