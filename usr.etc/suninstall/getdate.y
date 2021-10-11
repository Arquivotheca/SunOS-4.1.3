%token MONTH MERIDIAN NUMBER FULLYEAR

%{
#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)getdate.y 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)getdate.y 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */
 

#include <sys/types.h>
#include <ctype.h>
#include <time.h>

extern time_t timelocal();

static struct tm tmbuf;
static int	merid;
static char	*lptr;
static int	us_canada_flag;

static int	lookup();

#define	MERID_NONE	0
#define MERID_AM	1
#define MERID_PM	2
%}

%%
datetime: timespec datespec
	| datespec timespec;

timespec: NUMBER MERIDIAN
		= { maketime($1, 0, 0, $2); }
	| NUMBER ':' NUMBER
		= { maketime($1, $3, 0, MERID_NONE); }
	| NUMBER '.' NUMBER
		= { maketime($1, $3, 0, MERID_NONE); }
	| NUMBER ':' NUMBER MERIDIAN
		= { maketime($1, $3, 0, $4); }
	| NUMBER '.' NUMBER MERIDIAN
		= { maketime($1, $3, 0, $4); }
	| NUMBER ':' NUMBER ':' NUMBER
		= { maketime($1, $3, $5, MERID_NONE); }
	| NUMBER '.' NUMBER '.' NUMBER
		= { maketime($1, $3, $5, MERID_NONE); }
	| NUMBER ':' NUMBER ':' NUMBER MERIDIAN
		= { maketime($1, $3, $5, $6); }
	| NUMBER '.' NUMBER '.' NUMBER MERIDIAN
		= { maketime($1, $3, $5, $6); }
	;

datespec: NUMBER '/' NUMBER '/' NUMBER
		= { if (us_canada_flag)
			makedate($3, $1 - 1, $5);
		    else
			makedate($1, $3 - 1, $5);
		}
	| NUMBER '/' NUMBER '/' FULLYEAR
		= { if (us_canada_flag)
			makedate($3, $1 - 1, $5);
		    else
			makedate($1, $3 - 1, $5);
		}
	| NUMBER '.' NUMBER '.' FULLYEAR
		= { makedate($1, $3 - 1, $5); }
	| NUMBER '-' NUMBER '-' FULLYEAR
		= { makedate($1, $3 - 1, $5); }
	| NUMBER '-' NUMBER '-' NUMBER
		= { makedate($1, $3 - 1, $5); }
	| MONTH NUMBER ',' FULLYEAR
		= { makedate($2, $1, $4); }
	| NUMBER MONTH FULLYEAR
		= { makedate($1, $2, $3); }
	;
%%

static
maketime(hour, minute, second, meridian)
	int hour;
	int minute;
	int second;
	int meridian;
{
	tmbuf.tm_hour = hour;
	tmbuf.tm_min = minute;
	tmbuf.tm_sec = second;
	merid = meridian;
}

static
makedate(day, month, year)
	int day;
	int month;
	int year;
{
	tmbuf.tm_mday = day;
	tmbuf.tm_mon = month;
	tmbuf.tm_year = year;
}

static int dmsize[12] = {
	31,
	28,
	31,
	30,
	31,
	30,
	31,
	31,
	30,
	31,
	30,
	31
};

#define isleap(y) (((y) % 4) == 0 && ((y) % 100) != 0 || ((y) % 400) == 0)

time_t
getdate(p, us_canada)
	char *p;
	int us_canada;
{
	register int mdays;

	lptr = p;
	us_canada_flag = us_canada;

	if (yyparse())
		return (-1);

	if (tmbuf.tm_year >= 1900)
		tmbuf.tm_year -= 1900;
	if (tmbuf.tm_year < 70 || tmbuf.tm_mon < 0 || tmbuf.tm_mon > 11)
		return (-1);

	mdays = dmsize[tmbuf.tm_mon];
	if (tmbuf.tm_mon == 1 && isleap(tmbuf.tm_year))
		mdays++;
	if (tmbuf.tm_mday < 1 || tmbuf.tm_mday > mdays)
		return (-1);

	switch (merid) {

	case MERID_AM:
		if (tmbuf.tm_hour < 1 || tmbuf.tm_hour > 12)
			return(-1);
		tmbuf.tm_hour = tmbuf.tm_hour%12;
		break;

	case MERID_PM:
		if (tmbuf.tm_hour < 1 || tmbuf.tm_hour > 12)
			return(-1);
		tmbuf.tm_hour = tmbuf.tm_hour%12 + 12;
		break;

	case MERID_NONE:
		if (tmbuf.tm_hour < 0 || tmbuf.tm_hour > 23)
			return (-1);
		break;

	default:
		return (-1);
	}

	if (tmbuf.tm_min < 0 || tmbuf.tm_min > 59
	    || tmbuf.tm_sec < 0 || tmbuf.tm_sec > 59)
		return (-1);

	return (timelocal(&tmbuf));
}

yylex()
{
	extern int yylval;
	register char c;
	register char *p;
	char idbuf[128+1];

	for (;;) {
		while (isspace(*lptr))
			lptr++;

		if (isdigit(c = *lptr)) {
			yylval = 0;
			while (isdigit(c = *lptr++))
				yylval = 10*yylval + c - '0';
			lptr--;
			if (yylval >= 1900) {
				yylval -= 1900;
				return (FULLYEAR);
			} else
				return (NUMBER);
		} else if (isalpha(c)) {
			p = idbuf;
			while (isalpha(c = *lptr++) || c == '.') {
				if (p >= &idbuf[128])
					return (-1);
				if (isupper(c))
					c = tolower(c);
				*p++ = c;
			}
			*p = '\0';
			lptr--;
			return (lookup(idbuf));
		} else
			return (*lptr++);
	}
}

struct table {
	char	*name;
	int	value;
};

static struct table monthtab[] = {
	{ "january",	0 },
	{ "jan",	0 },
	{ "jan.",	0 },
	{ "february",	1 },
	{ "feb",	1 },
	{ "feb.",	1 },
	{ "march",	2 },
	{ "mar",	2 },
	{ "mar.",	2 },
	{ "april",	3 },
	{ "apr",	3 },
	{ "apr.",	3 },
	{ "may",	4 },
	{ "june",	5 },
	{ "jun",	5 },
	{ "jun.",	5 },
	{ "july",	6 },
	{ "jul",	6 },
	{ "jul.",	6 },
	{ "august",	7 },
	{ "aug",	7 },
	{ "aug.",	7 },
	{ "september",	8 },
	{ "sept",	8 },
	{ "sept.",	8 },
	{ "sep",	8 },
	{ "sep.",	8 },
	{ "october",	9 },
	{ "oct",	9 },
	{ "oct.",	9 },
	{ "november",	10 },
	{ "nov",	10 },
	{ "nov.",	10 },
	{ "december",	11 },
	{ "dec",	11 },
	{ "dec.",	11 },
};

#define	NMONTHTAB	(sizeof monthtab / sizeof monthtab[0])

static struct table meridtab[] = {
	{ "a.m.",	MERID_AM },
	{ "am",		MERID_AM },
	{ "p.m.",	MERID_PM },
	{ "pm",		MERID_PM },
};

#define	NMERIDTAB	(sizeof meridtab / sizeof meridtab[0])

static int
lookup(id)
	char *id;
{
	register struct table *tabp;

	for (tabp = &monthtab[0]; tabp < &monthtab[NMONTHTAB]; tabp++) {
		if (strcmp(id, tabp->name) == 0) {
			yylval = tabp->value;
			return (MONTH);
		}
	}

	for (tabp = &meridtab[0]; tabp < &meridtab[NMERIDTAB]; tabp++) {
		if (strcmp(id, tabp->name) == 0) {
			yylval = tabp->value;
			return (MERIDIAN);
		}
	}

	return(-1);
}

/*VARARGS*/
yyerror(s) char *s;
{
#ifdef lint
	s = s;
#endif
}
