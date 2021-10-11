#ifndef lint
static  char sccsid[] = "@(#)io.c 1.1 92/07/30 SMI";
#endif

#include "old.h"

rline()
{
	char *p1;
	short c;

	p1 = sbuf;
	while((c = getchar()) != '\n')
		if(c <= 0)
			onhup(); else
			*p1++ = c;
	*p1++ = '\0';
}

pboard()
{
	short i, x, y, c, p;

	c = 0;
	i = 0;
	x = 8;
	while(x--) {
		if(!mantom || mfmt)
			pchar('1'+x); else
			pchar('8'-x);
		pchar(' ');
		c++;
		y = 8;
		while(y--) {
			c++;
			pchar(' ');
			if(p = board[i++])
				pchar("kqrbnp PNBRQK"[p+6]); else
				if((c&1)!=0)
					pchar('*'); else
					pchar('-');
		}
		pchar('\n');
		if(intrp)
			return;
	}
	if(mfmt)
		printf("\n   a b c d e f g h"); else
		printf("\n   q q q q k k k k\n   r n b     b n r");
		printf("\n");
}

out1(m)
{
	printf("%d. ", moveno);
	if(mantom)
		printf("... ");
	out(m);
	pchar('\n');
}

out(m)
short m;
{
	short from, to, epf, pmf;

	from = m>>8;
	to = m&0377;
	if(mfmt) {
		algco(from);
		algco(to);
		return;
	}
	mantom? bmove(m): wmove(m);
	epf = pmf = 0;
	switch(amp[-1]) {

	case 0:
	case 1:
		stdp(board[to]);
	ed:
		pchar('/');
		stdb(from);
		if(amp[-2]) {
			pchar('x');
			stdp(amp[-2]);
			pchar('/');
		} else
			pchar('-');
		stdb(to);
		break;

	case 3:
		pchar('o');
		pchar('-');

	case 2:
		pchar('o');
		pchar('-');
		pchar('o');
		break;

	case 4:
		epf = 1;
		pchar('p');
		goto ed;

	case 5:
		pmf = 1;
		pchar('p');
		goto ed;
	}
	if(pmf) {
		pchar('(');
		pchar('q');
		pchar(')');
	}
	if(epf) {
		pchar('e');
		pchar('p');
	}
	if(check())
		pchar('+');
	mantom? bremove(): wremove();
}

stdp(p)
short p;
{

	if(p < 0)
		p = -p;
	p = "ppnbrqk"[p];
	pchar(p);
}

stdb(b)
short b;
{
	short r, f;

	r = b/8;
	if((f = b%8) < 4)
		pchar('q'); else {
		pchar('k');
		f = 7-f;
	}
	f = "rnb\0"[f];
	if(f)
		pchar(f);
	pchar(mantom? r+'1': '8'-r);
}

algco(p)
short p;
{
	pchar('a'+(p%8));
	pchar('8'-(p/8));
}

prtime(a, b)
{

	printf("time = %d/%d\n", a, b);
}

score1(m)
{
	if(intrp)
		return;
	if(!mantom) {
		if(moveno < 10)
			pchar(' '); else
			pchar(moveno/10 + '0');
		pchar(moveno%10 + '0');
		pchar('.');
		pchar(' ');
	} else
		while(column < 20)
			pchar(' ');
	out(m);
	if(mantom)
		pchar('\n');
}

score()
{
	short *p;

	pchar('\n');
	p = amp;
	while(amp[-1] != -1) {
		mantom? wremove(): bremove();
		decrem();
	}
	posit(score1, p);
	pchar('\n');
}

pchar(c)
{

	switch(c) {
		case '\t':
			do
				pchar(' ');
			while(column%8);
			return;
	
		case '\n':
			column = 0;
			break;
	
		default:
			column++;
	}
	putchar(c);
}
