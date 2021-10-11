#ifndef lint
static	char sccsid[] = "@(#)od.c 1.1 92/07/30 SMI"; /* from UCB 5.12 5/14/84 */
#endif
/*
 * od -- octal, hex, decimal, character dump of data in a file.
 *
 * usage:  od [-abBcdDefFhHiIlLopPs[n]vw[n]xX] [file] [[+]offset[.][b] [label]]
 *
 * where the option flags have the following meaning:
 *   character	object	radix	signed?
 *	a	byte	(10)	(n.a.)	ASCII named byte stream
 *	b	byte	  8	 no	byte octal
 *	c	byte	 (8)	(no)	character with octal non-graphic bytes
 *	d	short	 10	 no
 *	D	long	 10	 no
 *	e,F	double	(10)		double precision floating pt.
 *	f	float	(10)		single precision floating pt.
 *	h,x	short	 16	 no
 *	H,X	long	 16	 no
 *	i	short	 10	yes
 *	I,l,L	long	 10	yes
 *	o,B	short	  8	 no	(default conversion)
 *	O	long	  8	 no
 *	s[n]	string	 (8)		ASCII graphic strings
 *
 *	p				indicate EVEN parity on 'a' conversion
 *	P				indicate ODD parity on 'a' conversion
 *	v				show all data - don't skip like lines.
 *	w[n]				bytes per display line
 *
 * More than one format character may be given.
 * If {file} is not specified, standard input is read.
 * If {file} is not specified, then {offset} must start with '+'.
 * {Offset} may be HEX (0xnnn), OCTAL (0nn), or decimal (nnn.).
 * The default is octal. The same radix will be used to display the address.
 */

#include <stdio.h>
#include <ctype.h>
#include <locale.h>

#define DBUF_SIZE	BUFSIZ
#define BIG_DBUF	32
#define NO		0
#define YES		1
#define EVEN	       -1
#define ODD		1
#define UNSIGNED	0
#define SIGNED		1
#define PADDR		1
#define MIN_SLEN	3

int	a_put();
int	b_put();
int	c_put();
int	s_put();
int	us_put();
int	l_put();
int	f_put();
int	d_put();
int	st_put();

struct dfmt {
	int	df_field;	/* external field required for object */
	int	df_size;	/* size (bytes) of object */
	int	df_radix;	/* conversion radix */
	int	df_signed;	/* signed? flag */
	int	df_paddr;	/* "put address on each line?" flag */
	int	(*df_put)();	/* function to output object */
	char	*df_fmt;	/* output string format */
} *conv_vec[32];		/* vector of conversions to be done */

struct dfmt	ascii	= { 3, sizeof (char),   10,        0, PADDR,  a_put, 0};
struct dfmt	byte	= { 3, sizeof (char),    8, UNSIGNED, PADDR,  b_put, 0};
struct dfmt	cchar	= { 3, sizeof (char),    8, UNSIGNED, PADDR,  c_put, 0};
struct dfmt	u_s_oct	= { 6, sizeof (short),   8, UNSIGNED, PADDR, us_put, 0};
struct dfmt	u_s_dec	= { 5, sizeof (short),  10, UNSIGNED, PADDR, us_put, 0};
struct dfmt	u_s_hex	= { 4, sizeof (short),  16, UNSIGNED, PADDR, us_put, 0};
struct dfmt	u_l_oct	= {11, sizeof (long),    8, UNSIGNED, PADDR,  l_put, 0};
struct dfmt	u_l_dec	= {10, sizeof (long),   10, UNSIGNED, PADDR,  l_put, 0};
struct dfmt	u_l_hex	= { 8, sizeof (long),   16, UNSIGNED, PADDR,  l_put, 0};
struct dfmt	s_s_dec	= { 6, sizeof (short),  10,   SIGNED, PADDR,  s_put, 0};
struct dfmt	s_l_dec	= {11, sizeof (long),   10,   SIGNED, PADDR,  l_put, 0};
struct dfmt	flt	= {14, sizeof (float),  10,   SIGNED, PADDR,  f_put, 0};
struct dfmt	dble	= {21, sizeof (double), 10,   SIGNED, PADDR,  d_put, 0};
struct dfmt	string	= { 0,               0,  8,        0,    NO, st_put, 0};


char	usage[]	="usage: od [-abcdfhilopswvx] [file] [[+]offset[.][b] [label]]";
char	dbuf[DBUF_SIZE];
char	lastdbuf[DBUF_SIZE];
int	addr_base	= 8;		/* default address base is OCTAL */
long	addr		= 0L;		/* current file offset */
long	label		= -1L;		/* current label; -1 is "off" */
int	dbuf_size	= 16;		/* file bytes / display line */
int	_parity		= NO;		/* show parity on ascii bytes */
char	fmt[]	= "            %s";	/* 12 blanks */
char	*icvt();
char	*scvt();
char	*underline();
long	get_addr();

main(argc, argv)
int	argc;
char	**argv;
{
	register char *p;
	register char *l;
	register n, same;
	struct dfmt	*d;
	struct dfmt	**cv = conv_vec;
	int	showall = NO;
	int	field, llen, nelm;
	int	max_llen = 0;

	setlocale(LC_CTYPE, "");

	argv++;
	argc--;

	if(argc > 0)
	{
		p = *argv;
		if(*p == '-')
		{
			while(*++p != '\0')
			{
				switch(*p)
				{
				case 'a':
					d = &ascii;
					break;
				case 'b':
					d = &byte;
					break;
				case 'c':
					d = &cchar;
					break;
				case 'd':
					d = &u_s_dec;
					break;
				case 'D':
					d = &u_l_dec;
					break;
				case 'e':
				case 'F':
					d = &dble;
					break;
				case 'f':
					d = &flt;
					break;
				case 'h':
				case 'x':
					d = &u_s_hex;
					break;
				case 'H':
				case 'X':
					d = &u_l_hex;
					break;
				case 'i':
#ifdef S5EMUL
				case 's':
#endif
					d = &s_s_dec;
					break;
				case 'I':
				case 'l':
				case 'L':
					d = &s_l_dec;
					break;
				case 'o':
				case 'B':
					d = &u_s_oct;
					break;
				case 'O':
					d = &u_l_oct;
					break;
				case 'p':
					_parity = EVEN;
					continue;
				case 'P':
					_parity = ODD;
					continue;
#ifndef S5EMUL
				case 's':
#endif
				case 'S':
					d = &string;
					*(cv++) = d;
					while (isdigit(p[1]))
						d->df_size = (10 * d->df_size) + (*++p - '0');
					if (d->df_size <= 0)
						d->df_size = MIN_SLEN;
					showall = YES;
					continue;
				case 'w':
					dbuf_size = 0;
					while (isdigit(p[1]))
						dbuf_size = (10 * dbuf_size) + (*++p - '0');
					if (dbuf_size == 0)
						dbuf_size = BIG_DBUF;
					continue;
				case 'v':
					showall = YES;
					continue;
				default:
					printf("od: bad flag -%c\n", *p);
					puts(usage);
					exit(1);
				}
				*(cv++) = d;
			}
			argc--;
			argv++;
		}
	}

	/*
	 * if nothing spec'd, setup default conversion.
	 */
	if(cv == conv_vec)
		*(cv++) = &u_s_oct;

	*cv = (struct dfmt *)0;

	/*
	 * calculate display parameters
	 */
	for (cv = conv_vec; d = *cv; cv++)
	{
		nelm = (dbuf_size + d->df_size - 1) / d->df_size;
		llen = nelm * (d->df_field + 1);
		if (llen > max_llen)
			max_llen = llen;
	}

	/*
	 * setup df_fmt to point to uniform output fields.
	 */
	for (cv = conv_vec; d = *cv; cv++)
	{
		if (d->df_field)	/* only if external field is known */
		{
			nelm = (dbuf_size + d->df_size - 1) / d->df_size;
			field = max_llen / nelm;
			d->df_fmt = fmt + 12 - (field - d->df_field);
		}
	}

	/*
	 * input file specified ?
	 */
	if(argc > 0 && **argv != '+')
	{
		if (freopen(*argv, "r", stdin) == NULL)
		{
			(void) fprintf(stderr, "od: ");
			perror(*argv);
			exit(2);
		}
		argv++;
		argc--;
	}

	/*
	 * check for possible offset [label]
	 */
	if (argc > 0)
	{
		addr = get_addr(*argv);
		offset(addr);
		argv++;
		argc--;

		if (argc > 0)
			label = get_addr(*argv);
	}

	/*
	 * main dump loop
	 */
	same = -1;
	while ((n = fread(dbuf, 1, dbuf_size, stdin)) > 0)
	{
		if (same>=0 && bcmp(dbuf, lastdbuf, dbuf_size) == 0 && !showall)
		{
			if (same==0)
			{
				printf("*\n");
				same = 1;
			}
		}
		else
		{
			line(n);
			same = 0;
			p = dbuf;
			l = lastdbuf;
			for (nelm = 0; nelm < dbuf_size; nelm++)
			{
				*l++ = *p;
				*p++ = '\0';
			}
		}
		addr += n;
		if (label >= 0)
			label += n;
	}

	/*
	 * Some conversions require "flushing".
	 */
	n = 0;
	for (cv = conv_vec; *cv; cv++)
	{
		if ((*cv)->df_paddr)
		{
			if (n++ == 0)
				put_addr(addr, label, '\n');
		}
		else
			(*((*cv)->df_put))(0, *cv);
	}
	exit(0);
	/* NOTREACHED */
}

put_addr(a, l, c)
long	a;
long	l;
char	c;
{
	fputs(icvt(a, addr_base, UNSIGNED, 7), stdout);
	if (l >= 0)
		printf(" (%s)", icvt(l, addr_base, UNSIGNED, 7));
	if (c != '\0')
		putchar(c);
}

line(n)
int	n;
{
	register i, first;
	register struct dfmt *c;
	register struct dfmt **cv = conv_vec;

	first = YES;
	while (c = *cv++)
	{
		if (c->df_paddr)
		{
			if (first)
			{
#ifdef S5EMUL
				put_addr(addr, label, '\0');
#else
				put_addr(addr, label, ' ');
#endif
				first = NO;
			}
			else
			{
				putchar('\t');
				if (label >= 0)
					fputs("\t  ", stdout);
			}
		}
		i = 0;
		while (i < n)
			i += (*(c->df_put))(dbuf+i, c);
		if (c->df_paddr)
			putchar('\n');
	}
}

s_put(n, d)
short	*n;
struct dfmt	*d;
{
	printf(d->df_fmt, icvt((long)*n, d->df_radix, d->df_signed, d->df_field));
	return(d->df_size);
}

us_put(n, d)
unsigned short	*n;
struct dfmt	*d;
{
	printf(d->df_fmt, icvt((long)*n, d->df_radix, d->df_signed, d->df_field));
	return(d->df_size);
}

l_put(n, d)
long	*n;
struct dfmt	*d;
{
	printf(d->df_fmt, icvt(*n, d->df_radix, d->df_signed, d->df_field));
	return(d->df_size);
}

d_put(f, d)
double	*f;
struct dfmt *d;
{
	char fbuf[24];
	struct l { long n[2]; };

#if	vax
	if ((((struct l *)f)->n[0] & 0xff00) == 0x8000)	/* Vax illegal f.p. */
		sprintf(fbuf, "    %08x %08x",
			((struct l *)f)->n[0], ((struct l *)f)->n[1]);
	else
#endif

		sprintf(fbuf, "%21.14e", *f);
	printf(d->df_fmt, fbuf);
	return(d->df_size);
}

f_put(f, d)
float	*f;
struct dfmt *d;
{
	char fbuf[16];

#if	vax
	if ((*(long *)f & 0xff00) == 0x8000)	/* Vax illegal f.p. form */
		sprintf(fbuf, "      %08x", *(long *)f);
	else
#endif
		sprintf(fbuf, "%14.7e", *f);
	printf(d->df_fmt, fbuf);
	return(d->df_size);
}


char	asc_name[34][4] = {
/* 000 */	"nul",	"soh",	"stx",	"etx",	"eot",	"enq",	"ack",	"bel",
/* 010 */	" bs",	" ht",	" nl",	" vt",	" ff",	" cr",	" so",	" si",
/* 020 */	"dle",	"dc1",	"dc2",	"dc3",	"dc4",	"nak",	"syn",	"etb",
/* 030 */	"can",	" em",	"sub",	"esc",	" fs",	" gs",	" rs",	" us",
/* 040 */	" sp",	"del"
};

a_put(cc, d)
char	*cc;
struct dfmt *d;
{
	int c = *cc;
	register char *s = "   ";
	register pbit = parity((int)c & 0377);

	c &= 0177;
	if (isgraph(c))
	{
		s[2] = c;
		if (pbit == _parity)
			printf(d->df_fmt, underline(s));
		else
			printf(d->df_fmt, s);
	}
	else
	{
		if (c == 0177)
			c = ' ' + 1;
		if (pbit == _parity)
			printf(d->df_fmt, underline(asc_name[c]));
		else
			printf(d->df_fmt, asc_name[c]);
	}
	return(1);
}

parity(word)
int	word;
{
	register int p = 0;
	register int w = word;

	if (w)
		do
		{
			p ^= 1;
		} while(w &= (~(-w)));
	return (p? ODD:EVEN);
}

char *
underline(s)
char	*s;
{
	static char ulbuf[16];
	register char *u = ulbuf;

	while (*s)
	{
		if (*s != ' ')
		{
			*u++ = '_';
			*u++ = '\b';
		}
		*u++ = *s++;
	}
	*u = '\0';
	return(ulbuf);
}

b_put(b, d)
char	*b;
struct dfmt *d;
{
	printf(d->df_fmt, icvt((long)*b & 0377, d->df_radix, d->df_signed, d->df_field));
	return(1);
}

c_put(cc, d)
char	*cc;
struct dfmt *d;
{
	register char	*s;
	register int	n;
	register int	c = *cc & 0377;

	s = scvt(c, d);
	for (n = d->df_field - strlen(s); n > 0; n--)
		putchar(' ');
	printf(d->df_fmt, s);
	return(1);
}

char *scvt(c, d)
int	c;
struct dfmt	*d;
{
	static char s[2];

	switch(c)
	{
		case '\0':
			return("\\0");

		case '\b':
			return("\\b");

		case '\f':
			return("\\f");

		case '\n':
			return("\\n");

		case '\r':
			return("\\r");

		case '\t':
			return("\\t");

		default:
			if (isprint(c))
			{
				s[0] = c;
				return(s);
			}
			return(icvt((long)c, d->df_radix, d->df_signed, d->df_field));
	}
}

/*
 * Look for strings.
 * A string contains bytes > 037 && < 177, and ends with a null.
 * The minimum length is given in the dfmt structure.
 */

#define CNULL		'\0'
#define S_EMPTY	0
#define S_FILL	1
#define	S_CONT	2
#define SBUFSIZE	1024

static unsigned char	str_buf[SBUFSIZE];
static int		str_mode = S_EMPTY;
static unsigned char	*str_ptr;
static long		str_addr;
static long		str_label;

st_put(cc, d)
char	*cc;
struct dfmt	*d;
{
	register int	c;

	if (cc == 0)
	{
		pr_sbuf(d, YES);
		return(1);
	}

	c = (*cc & 0377);

	if (str_mode & S_FILL)
	{
		if (isprint(c) || ((isspace(c) && c != '\v') || c == '\b'))
			put_sbuf(c, d);
		else
		{
			*str_ptr = CNULL;
			if (c == NULL)
				pr_sbuf(d, YES);
			str_mode = S_EMPTY;
		}
	}
	else if (isprint(c) || ((isspace(c) && c != '\v') || c == '\b'))
	{
		str_mode = S_FILL;
		str_addr = addr + (cc - dbuf);	  /* ugly */
		if ((str_label = label) >= 0)
			str_label += (cc - dbuf); /*  ''  */
		str_ptr = str_buf;
		put_sbuf(c, d);
	}

	return(1);
}

put_sbuf(c, d)
int	c;
struct dfmt	*d;
{
	*str_ptr++ = c;
	if (str_ptr >= (str_buf + SBUFSIZE))
	{
		pr_sbuf(d, NO);
		str_ptr = str_buf;
		str_mode |= S_CONT;
	}
}

pr_sbuf(d, end)
struct dfmt	*d;
int	end;
{
	register unsigned char	*p = str_buf;

	if (str_mode == S_EMPTY
	    || (!(str_mode & S_CONT) && (str_ptr - str_buf) < d->df_size))
		return;

	if (!(str_mode & S_CONT))
		put_addr(str_addr, str_label, ' ');

	while (p < str_ptr)
		fputs(scvt(*p++, d), stdout);

	if (end)
		putchar('\n');
}

/*
 * integer to ascii conversion
 *
 * This code has been rearranged to produce optimized runtime code.
 */

#define MAXINTLENGTH	32
static char	_digit[] = "0123456789abcdef";
static char	_icv_buf[MAXINTLENGTH+1];
static long	_mask = 0x7fffffff;

char *
icvt (value, radix, signed, ndigits)
long	value;
int	radix;
int	signed;
int	ndigits;
{
	register long	val = value;
	register long	rad = radix;
	register char	*b = &_icv_buf[MAXINTLENGTH];
	register char	*d = _digit;
	register long	tmp1;
	register long	tmp2;
	long	rem;
	long	kludge;
	int	sign;

	if (val == 0)
	{
		*--b = '0';
		sign = 0;
		goto done; /*return(b);*/
	}

	if (signed && (sign = (val < 0)))	/* signed conversion */
	{
		/*
		 * It is necessary to do the first divide
		 * before the absolute value, for the case -2^31
		 *
		 * This is actually what is being done...
		 * tmp1 = (int)(val % rad);
		 * val /= rad;
		 * val = -val
		 * *--b = d[-tmp1];
		 */
		tmp1 = val / rad;
		*--b = d[(tmp1 * rad) - val];
		val = -tmp1;
	}
	else				/* unsigned conversion */
	{
		sign = 0;
		if (val < 0)
		{	/* ALL THIS IS TO SIMULATE UNSIGNED LONG MOD & DIV */
			kludge = _mask - (rad - 1);
			val &= _mask;
			/*
			 * This is really what's being done...
			 * rem = (kludge % rad) + (val % rad);
			 * val = (kludge / rad) + (val / rad) + (rem / rad) + 1;
			 * *--b = d[rem % rad];
			 */
			tmp1 = kludge / rad;
			tmp2 = val / rad;
			rem = (kludge - (tmp1 * rad)) + (val - (tmp2 * rad));
			val = ++tmp1 + tmp2;
			tmp1 = rem / rad;
			val += tmp1;
			*--b = d[rem - (tmp1 * rad)];
		}
	}

	while (val)
	{
		/*
		 * This is really what's being done ...
		 * *--b = d[val % rad];
		 * val /= rad;
		 */
		tmp1 = val / rad;
		*--b = d[val - (tmp1 * rad)];
		val = tmp1;
	}

done:
	if (sign)
		*--b = '-';

	tmp1 = ndigits - (&_icv_buf[MAXINTLENGTH] - b);
	tmp2 = signed? ' ':'0';
	while (tmp1 > 0)
	{
		*--b = tmp2;
		tmp1--;
	}

	return(b);
}

long get_addr(s)
register char *s;
{
	register char *p;
	register long a;
	register int d;

	if (*s=='+')
		s++;
	if (*s=='x')
	{
		s++;
		addr_base = 16;
	}
	else if (*s=='0' && s[1]=='x')
	{
		s += 2;
		addr_base = 16;
	}
	else if (*s == '0')
		addr_base = 8;
	p = s;
	while(*p)
	{
		if (*p++=='.')
			addr_base = 10;
	}
	for (a=0; *s; s++)
	{
		d = *s;
		if(isdigit(d))
			a = a*addr_base + d - '0';
		else if (isxdigit(d) && addr_base==16) {
			if (isupper(d))
				d = tolower(d);
			a = a*addr_base + d + 10 - 'a';
		} else
			break;
	}

	if (*s == '.')
		s++;
	if(*s=='b')
		a *= 512;
	if(*s=='B')
		a *= 1024;

	return(a);
}

offset(a)
long	a;
{
	if (canseek(stdin))
	{
		/*
		 * in case we're accessing a raw disk,
		 * we have to seek in multiples of a physical block.
		 */
		fseek(stdin, a & 0xfffffe00L, 0);
		a &= 0x1ffL;
	}
	dumbseek(stdin, a);
}

dumbseek(s, offset)
FILE	*s;
long	offset;
{
	char	buf[BUFSIZ];
	int	n;
	int	nr;

	while (offset > 0)
	{
		nr = (offset > BUFSIZ) ? BUFSIZ : (int)offset;
		if ((n = fread(buf, 1, nr, s)) != nr)
		{
			fprintf(stderr, "EOF\n");
			exit(1);
		}
		offset -= n;
	}
}

#include <sys/types.h>
#include <sys/stat.h>

canseek(f)
FILE	*f;
{
	struct stat statb;

	return( (fstat(fileno(f),&statb)==0) &&
		((statb.st_mode&S_IFMT) != S_IFIFO) &&	/*!pipe*/
		(!isatty(fileno(f))) );
}
