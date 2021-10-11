#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)localtime.c 1.1 92/07/30 SMI"; /* from Arthur Olson's 3.2 */
#endif

/*LINTLIBRARY*/

/*
** sys/param.h is included to get MAXPATHLEN, and to get time_t by virtue
** of including sys/types.h.
*/

#include <sys/param.h>
#include <tzfile.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif /* !defined TRUE */

extern char *		getenv();

struct tm *		offtime();

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
};

struct state {
	int		timecnt;
	int		typecnt;
	int		charcnt;
	time_t		*ats;
	unsigned char	*types;
	struct ttinfo	*ttis;
	char		*chars;
};

#ifdef TEST
struct state	*sp;
#else
static struct state	*sp;
#endif

static int		tz_is_set;

#ifdef S5EMUL
char *			tzname[2] = {
	"GMT",
	"GMT"
};

time_t			timezone = 0;
int			daylight = 0;
#endif /* S5EMUL */

static long
detzcode(codep)
char *	codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return result;
}

/*
** Free all the items pointed to by the specified "state" structure (except for
** "chars", which might have other references to it), and zero out all the
** pointers to those items.
*/
static void
freeall(s)
register struct state *	s;
{
	if (s->ttis) {
		free(s->ttis);
		s->ttis = 0;
	}
	if (s->types) {
		free(s->types);
		s->types = 0;
	}
	if (s->ats) {
		free(s->ats);
		s->ats = 0;
	}
}

#ifdef S5EMUL
static void
settzname()
{
	register struct state *s = sp;
	register int	i;

	tzname[0] = tzname[1] = &s->chars[0];
	timezone = -s->ttis[0].tt_gmtoff;
	daylight = 0;
	for (i = 1; i < s->typecnt; ++i) {
		register struct ttinfo *	ttisp;

		ttisp = &s->ttis[i];
		if (ttisp->tt_isdst) {
			tzname[1] = &s->chars[ttisp->tt_abbrind];
			daylight = 1;
		} else {
			tzname[0] = &s->chars[ttisp->tt_abbrind];
			timezone = -ttisp->tt_gmtoff;
		}
	}
}
#endif

static
tzload(name)
register char *	name;
{
	register char *	p;
	register int	i;
	register int	fid;
	register struct state *s = sp;

	if (s == 0) {
		s = (struct state *)calloc(1, sizeof (*s));
		if (s == 0)
			return -1;
		sp = s;
	}

	if (name == 0 && (name = TZDEFAULT) == 0)
		return -1;
	{
		register int	doaccess;
		char		fullname[MAXPATHLEN];

		if (name[0] == ':')
			name++;
		doaccess = name[0] == '/';
		if (!doaccess) {
			if ((p = TZDIR) == NULL)
				return -1;
			if ((strlen(p) + strlen(name) + 1) >= sizeof fullname)
				return -1;
			(void) strcpy(fullname, p);
			(void) strcat(fullname, "/");
			(void) strcat(fullname, name);
			/*
			** Set doaccess if '.' (as in "../") shows up in name.
			*/
			if (strchr(name, '.') != NULL)
				doaccess = TRUE;
			name = fullname;
		}
		if (doaccess && access(name, 4) != 0)
			return -1;
		if ((fid = open(name, 0)) == -1)
			return -1;
	}
	{
		register struct tzhead *	tzhp;
		char				buf[8192];

		i = read(fid, buf, sizeof buf);
		if (close(fid) != 0 || i < sizeof *tzhp)
			return -1;
		tzhp = (struct tzhead *) buf;
		s->timecnt = (int) detzcode(tzhp->tzh_timecnt);
		s->typecnt = (int) detzcode(tzhp->tzh_typecnt);
		s->charcnt = (int) detzcode(tzhp->tzh_charcnt);
		if (s->timecnt > TZ_MAX_TIMES ||
			s->typecnt == 0 ||
			s->typecnt > TZ_MAX_TYPES ||
			s->charcnt > TZ_MAX_CHARS)
				return -1;
		if (i < sizeof *tzhp +
			s->timecnt * (4 + sizeof (char)) +
			s->typecnt * (4 + 2 * sizeof (char)) +
			s->charcnt * sizeof (char))
				return -1;
		freeall(s);
		if (s->timecnt != 0) {
			s->ats =
			  (time_t *)calloc(s->timecnt, sizeof (time_t));
			if (s->ats == 0)
				return -1;
			s->types =
			  (unsigned char *)calloc(s->timecnt,
			  sizeof (unsigned char));
			if (s->types == 0) {
				freeall(s);
				return -1;
			}
		}
		s->ttis =
		  (struct ttinfo *)calloc(s->typecnt, sizeof (struct ttinfo));
		if (s->ttis == 0) {
			freeall(s);
			return -1;
		}
		s->chars =
		  (char *)calloc(s->charcnt+1, sizeof (char));
		if (s->chars == 0) {
			freeall(s);
			return -1;
		}
		p = buf + sizeof *tzhp;
		for (i = 0; i < s->timecnt; ++i) {
			s->ats[i] = detzcode(p);
			p += 4;
		}
		for (i = 0; i < s->timecnt; ++i)
			s->types[i] = (unsigned char) *p++;
		for (i = 0; i < s->typecnt; ++i) {
			register struct ttinfo *	ttisp;

			ttisp = &s->ttis[i];
			ttisp->tt_gmtoff = detzcode(p);
			p += 4;
			ttisp->tt_isdst = (unsigned char) *p++;
			ttisp->tt_abbrind = (unsigned char) *p++;
		}
		for (i = 0; i < s->charcnt; ++i)
			s->chars[i] = *p++;
		s->chars[i] = '\0';	/* ensure '\0' at end */
	}
	/*
	** Check that all the local time type indices are valid.
	*/
	for (i = 0; i < s->timecnt; ++i)
		if (s->types[i] >= s->typecnt)
			return -1;
	/*
	** Check that all abbreviation indices are valid.
	*/
	for (i = 0; i < s->typecnt; ++i)
		if (s->ttis[i].tt_abbrind >= s->charcnt)
			return -1;
#ifdef S5EMUL
	/*
	** Set tzname elements to initial values.
	*/
	settzname();
#endif /* S5EMUL */
	return 0;
}

struct rule {
	int	r_type;		/* type of rule */
	int	r_day;		/* day number of rule */
	int	r_week;		/* week number of rule */
	int	r_mon;		/* month number of rule */
	long	r_time;		/* transition time of rule */
};

#define	JULIAN_DAY		0	/* Jn - Julian day */
#define	DAY_OF_YEAR		1	/* n - day of year */
#define	MONTH_NTH_DAY_OF_WEEK	2	/* Mm.n.d - month, week, day of week */

static int	mon_lengths[2][MONS_PER_YEAR] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int	year_lengths[2] = {
	DAYS_PER_NYEAR, DAYS_PER_LYEAR
};

/*
** Given a pointer into a time zone string, scan until a character that is not
** a valid character in a zone name is found.  Return a pointer to that
** character.
*/
static char *
getzname(strp)
register char *	strp;
{
	register char	c;

	while ((c = *strp) != '\0' && !isdigit(c) && c != ',' && c != '-'
	    && c != '+')
			++strp;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a number from that string.
** Check that the number is within a specified range; if it is not, return
** NULL.
** Otherwise, return a pointer to the first character not part of the number.
*/
static char *
getnum(strp, nump, min, max)
register char *	strp;
int *	nump;
int	min;
int	max;
{
	register char	c;
	register int	num;

	num = 0;
	while ((c = *strp) != '\0' && isdigit(c)) {
		num = num*10 + (c - '0');
		if (num > max)
			return NULL;	/* illegal value */
		++strp;
	}
	if (num < min)
		return NULL;		/* illegal value */
	*nump = num;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a time, in hh[:mm[:ss]]
** form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the time.
*/
static char *
gettime(strp, timep)
register char *	strp;
long *	timep;
{
	int num;

	strp = getnum(strp, &num, 0, HOURS_PER_DAY / 2);
	if (strp == NULL)
		return NULL;
	*timep = num*SECS_PER_HOUR;
	if (*strp == ':') {
		++strp;
		strp = getnum(strp, &num, 0, MINS_PER_HOUR - 1);
		if (strp == NULL)
			return NULL;
		*timep += num*SECS_PER_MIN;
		if (*strp == ':') {
			++strp;
			strp = getnum(strp, &num, 0, SECS_PER_MIN - 1);
			if (strp == NULL)
				return NULL;
			*timep += num;
		}
	}
	return strp;
}

/*
** Given a pointer into a time zone string, extract an offset, in
** [+-]hh[:mm[:ss]] form, from the string.
** If any error occurs, return NULL.
** Otherwise, return a pointer to the first character not part of the time.
*/
static char *
getoffset(strp, offsetp)
register char *	strp;
long *	offsetp;
{
	register int neg;

	if (*strp == '-') {
		neg = 1;
		++strp;
	} else if (*strp == '+' || isdigit(*strp))
		neg = 0;
	else
		return NULL;		/* illegal offset */
	strp = gettime(strp, offsetp);
	if (strp == NULL)
		return NULL;		/* illegal time */
	if (neg)
		*offsetp = -*offsetp;
	return strp;
}

/*
** Given a pointer into a time zone string, extract a rule in the form
** date[/time].  See POSIX section 8 for the format of "date" and "time".
** If a valid rule is not found, return NULL.
** Otherwise, return a pointer to the first character not part of the rule.
*/
static char *
getrule(strp, rulep)
char *	strp;
register struct rule *	rulep;
{
	if (*strp == 'J') {
		/*
		** Julian day.
		*/
		rulep->r_type = JULIAN_DAY;
		++strp;
		strp = getnum(strp, &rulep->r_day, 1, DAYS_PER_NYEAR);
	} else if (*strp == 'M') {
		/*
		** Month, week, day.
		*/
		rulep->r_type = MONTH_NTH_DAY_OF_WEEK;
		++strp;
		strp = getnum(strp, &rulep->r_mon, 1, MONS_PER_YEAR);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_week, 1, 5);
		if (strp == NULL)
			return NULL;
		if (*strp++ != '.')
			return NULL;
		strp = getnum(strp, &rulep->r_day, 0, DAYS_PER_WEEK - 1);
	} else if (isdigit(*strp)) {
		/*
		** Day of year.
		*/
		rulep->r_type = DAY_OF_YEAR;
		strp = getnum(strp, &rulep->r_day, 0, DAYS_PER_LYEAR - 1);
	} else
		return NULL;		/* invalid format */
	if (strp == NULL)
		return NULL;
	if (*strp == '/') {
		/*
		** Time specified.
		*/
		++strp;
		strp = gettime(strp, &rulep->r_time);
		if (strp == NULL)
			return NULL;
	} else
		rulep->r_time = 2*SECS_PER_HOUR;	/* default = 2:00:00 */
	return strp;
}

/*
** Given the Epoch-relative time of January 1, 00:00:00 GMT, in a year, the
** year, a rule, and the offset from GMT at the time that rule takes effect,
** calculate the Epoch-relative time that rule takes effect.
*/
static time_t
transtime(janfirst, year, rulep, offset)
time_t	janfirst;
int	year;
register struct rule *	rulep;
long	offset;
{
	register int	leapyear;
	register time_t	value;
	register int	i;
	int	d, m1, yy0, yy1, yy2, dow;

	leapyear = isleap(year);
	switch (rulep->r_type) {

	case JULIAN_DAY:
		/*
		** Jn - Julian day, 1 == January 1, 60 == March 1 even in leap
		** years.
		** In non-leap years, or if the day number is 59 or less, just
		** add SECS_PER_DAY times the day number-1 to the time of
		** January 1, midnight, to get the day.
		*/
		value = janfirst + (rulep->r_day - 1)*SECS_PER_DAY;
		if (leapyear && rulep->r_day >= 60)
			value += SECS_PER_DAY;
		break;

	case DAY_OF_YEAR:
		/*
		** n - day of year.
		** Just add SECS_PER_DAY times the day number to the time of
		** January 1, midnight, to get the day.
		*/
		value = janfirst + rulep->r_day*SECS_PER_DAY;
		break;

	case MONTH_NTH_DAY_OF_WEEK:
		/*
		** Mm.n.d - nth "dth day" of month m.
		*/
		value = janfirst;
		for (i = 0; i < rulep->r_mon - 1; ++i)
			value += mon_lengths[leapyear][i]*SECS_PER_DAY;

		/*
		** Use Zeller's Congruence to get day-of-week of first day of
		** month.
		*/
		m1 = (rulep->r_mon + 9)%12 + 1;
		yy0 = (rulep->r_mon <= 2) ? (year - 1) : year;
		yy1 = yy0 / 100;
		yy2 = yy0 % 100;
		dow = ((26*m1 - 2)/10 + 1 + yy2 + yy2/4 + yy1/4 - 2*yy1)%7;
		if (dow < 0)
			dow += DAYS_PER_WEEK;

		/*
		** "dow" is the day-of-week of the first day of the month.  Get
		** the day-of-month (zero-origin) of the first "dow" day of the
		** month.
		*/
		d = rulep->r_day - dow;
		if (d < 0)
			d += DAYS_PER_WEEK;
		for (i = 1; i < rulep->r_week; ++i) {
			if (d + DAYS_PER_WEEK >= mon_lengths[leapyear][rulep->r_mon - 1])
				break;
			d += DAYS_PER_WEEK;
		}

		/*
		** "d" is the day-of-month (zero-origin) of the day we want.
		*/
		value += d*SECS_PER_DAY;
		break;
	}

	/*
	** "value" is the Epoch-relative time of 00:00:00 GMT on the day in
	** question.  To get the Epoch-relative time of the specified local
	** time on that day, add the transition time and the current offset
	** from GMT.
	*/
	return value + rulep->r_time + offset;
}

/*
** The U.S. tables, including the latest hack.
*/

/*
** Define MONTH_NTH_DAY_OF_WEEK rule for U.S. federal tables.
*/
#define	MD_RULE(week, month)	{ MONTH_NTH_DAY_OF_WEEK, 0, week, month, 2*SECS_PER_HOUR }

/*
** Define DAY_OF_YEAR rule for U.S. federal tables.
*/
#define	DOY_RULE(day)		{ DAY_OF_YEAR, day, 0, 0, 2*SECS_PER_HOUR }

static struct rule usdaytab[] = {
		/* 1970: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1971: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1972: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1973: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1974: Jan 6 - last Sun. in Oct */
	DOY_RULE(5),   MD_RULE(5, 10),
		/* 1975: last Sun. in Feb - last Sun. in Oct */
	MD_RULE(5, 2), MD_RULE(5, 10),
		/* 1976: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1977: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1978: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1979: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1980: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1981: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1982: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1983: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1984: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1985: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
		/* 1986: last Sun. in Apr - last Sun. in Oct */
	MD_RULE(5, 4), MD_RULE(5, 10),
};

#define	N_US_RULES	(sizeof usdaytab / sizeof usdaytab[0])

static struct rule repeating[] = {
		/* 1987 on: first Sun. in Apr - last Sun. in Oct */
	MD_RULE(0, 4), MD_RULE(5, 10)
};

/*
** Given a POSIX section 8-style TZ string, fill in the rule tables as
** appropriate.
*/
static
tzparse(name)
char *	name;
{
	char *	stdname;
	char *	dstname;
	int	stdlen;
	int	dstlen;
	long	stdoffset;
	long	dstoffset;
	struct rule	start;
	struct rule	end;
	register struct state *s = sp;
	register int	year;
	register time_t	janfirst;
	register time_t *	atp;
	register unsigned char *	typep;
	register char *	cp;
	register int	i;
	time_t	starttime;
	time_t	endtime;

	/*
	** If "s" is zero, it's because "tzload" couldn't allocate a "state"
	** structure ("tzparse" is never called without "tzload" first having
	** been called); we just give up if this is the case.
	*/
	if (s == 0)
		return -1;

	stdname = name;
	name = getzname(name);
	stdlen = name - stdname;	/* length of standard zone name */
	if (stdlen == 0)
		return -1;
	name = getoffset(name, &stdoffset);
	if (name == NULL)
		return -1;
	freeall(s);
	if (*name != '\0') {
		dstname = name;
		name = getzname(name);
		dstlen = name - dstname;	/* length of DST zone name */
		if (dstlen == 0)
			return -1;
		if (*name != '\0' && *name != ',' && *name != ';') {
			name = getoffset(name, &dstoffset);
			if (name == NULL)
				return -1;
		} else
			dstoffset = stdoffset - 1*SECS_PER_HOUR;
		s->typecnt = 2;	/* standard time and DST */
		s->charcnt = stdlen + 1 + dstlen + 1;
		/*
		** Two transitions per year, from 1970 to 2038.
		*/
		s->timecnt = 2*(2038 - 1970 + 1);
		if (s->timecnt > TZ_MAX_TIMES)
			return -1;
		s->ats =
		  (time_t *)calloc(s->timecnt, sizeof (time_t));
		if (s->ats == 0)
			return -1;
		s->types =
		  (unsigned char *)calloc(s->timecnt,
		  sizeof (unsigned char));
		if (s->types == 0) {
			freeall(s);
			return -1;
		}
		s->ttis =
		  (struct ttinfo *)calloc(s->typecnt,
		      sizeof (struct ttinfo));
		if (s->ttis == 0) {
			freeall(s);
			return -1;
		}
		s->ttis[0].tt_gmtoff = -dstoffset;
		s->ttis[0].tt_isdst = 1;
		s->ttis[0].tt_abbrind = stdlen + 1;
		s->ttis[1].tt_gmtoff = -stdoffset;
		s->ttis[1].tt_isdst = 0;
		s->ttis[1].tt_abbrind = 0;
		if (*name == ',' || *name == ';') {
			++name;
			if ((name = getrule(name, &start)) == NULL) {
				freeall(s);
				return -1;
			}
			if (*name++ != ',') {
				freeall(s);
				return -1;
			}
			if ((name = getrule(name, &end)) == NULL) {
				freeall(s);
				return -1;
			}
			if (*name != '\0') {
				freeall(s);
				return -1;
			}
			atp = s->ats;
			typep = s->types;
			for (year = 1970, janfirst = 0; year <= 2038; year++) {
				starttime = transtime(janfirst, year, &start,
				    stdoffset);
				endtime = transtime(janfirst, year, &end,
				    dstoffset);
				if (starttime > endtime) {
					*atp++ = endtime;
					*typep++ = 1;	/* DST ends */
					*atp++ = starttime;
					*typep++ = 0;	/* DST begins */
				} else {
					*atp++ = starttime;
					*typep++ = 0;	/* DST begins */
					*atp++ = endtime;
					*typep++ = 1;	/* DST ends */
				}
				janfirst +=
				    year_lengths[isleap(year)]*SECS_PER_DAY;
			}
		} else {
			if (*name != '\0') {
				freeall(s);
				return -1;
			}
			atp = s->ats;
			typep = s->types;
			for (year = 1970, janfirst = 0, i = 0; i < N_US_RULES;
			    ++year, i += 2) {
				*atp++ = transtime(janfirst, year,
				    &usdaytab[i], stdoffset);
				*typep++ = 0;	/* DST begins */
				*atp++ = transtime(janfirst, year,
				    &usdaytab[i + 1], dstoffset);
				*typep++ = 1;	/* DST ends */
				janfirst +=
				    year_lengths[isleap(year)]*SECS_PER_DAY;
			}
			for (; year <= 2038; year++) {
				*atp++ = transtime(janfirst, year,
				    &repeating[0], stdoffset);
				*typep++ = 0;	/* DST begins */
				*atp++ = transtime(janfirst, year,
				    &repeating[1], dstoffset);
				*typep++ = 1;	/* DST ends */
				janfirst +=
				    year_lengths[isleap(year)]*SECS_PER_DAY;
			}
		}
	} else {
		s->typecnt = 1;		/* only standard time */
		s->timecnt = 0;
		s->ats = 0;
		s->types = 0;
		s->charcnt = stdlen + 1;
		s->ttis =
		  (struct ttinfo *)calloc(s->typecnt, sizeof (struct ttinfo));
		if (s->ttis == 0)
			return -1;
		s->ttis[0].tt_gmtoff = -stdoffset;
		s->ttis[0].tt_isdst = 0;
		s->ttis[0].tt_abbrind = 0;
	}
	cp = (char *)calloc(s->charcnt+1, sizeof (char));
	if (cp == 0) {
		freeall(s);
		return -1;
	}
	s->chars = cp;
	(void) strncpy(cp, stdname, stdlen);
	cp += stdlen;
	*cp++ = '\0';
	if (s->typecnt == 2) {
		(void) strncpy(cp, dstname, dstlen);
		*(cp + dstlen) = '\0';
	}
#ifdef S5EMUL
	settzname();
#endif /* S5EMUL */
	return 0;
}

static
tzsetgmt()
{
	register struct state *s = sp;

	if (s == 0) {
		s = (struct state *)calloc(1, sizeof (*s));
		if (s == 0)
			return;
		sp = s;
	}
	s->timecnt = 0;
	s->typecnt = 1;
	s->charcnt = 4;
	freeall(s);
	s->ttis = (struct ttinfo *)calloc(1, sizeof (struct ttinfo));
	s->ttis[0].tt_gmtoff = 0;
	s->ttis[0].tt_abbrind = 0;
	s->chars = (char *)malloc(4);
	(void) strcpy(s->chars, "GMT");
#ifdef S5EMUL
	settzname();
#endif /* S5EMUL */
}

void
tzset()
{
	register char *	name;

	tz_is_set = TRUE;
	name = getenv("TZ");
	if (name != 0 && *name == '\0')
		tzsetgmt();		/* GMT by request */
	else if (tzload(name) != 0) {
		if (name[0] == ':' || tzparse(name) != 0)
			tzsetgmt();
	}
}

void
tzsetwall()
{
	tz_is_set = TRUE;
	if (tzload((char *) 0) != 0)
		tzsetgmt();
}

struct tm *
localtime(timep)
time_t *	timep;
{
	register struct ttinfo *	ttisp;
	register struct tm *		tmp;
	register int			i;
	time_t				t;
	register struct state *s;

	if (!tz_is_set)
		tzset();
	s = sp;
	if (s == 0)
		return (0);
	t = *timep;
	if (s->timecnt == 0 || t < s->ats[0]) {
		i = 0;
		while (s->ttis[i].tt_isdst)
			if (++i >= s->typecnt) {
				i = 0;
				break;
			}
	} else {
		for (i = 1; i < s->timecnt; ++i)
			if (t < s->ats[i])
				break;
		i = s->types[i - 1];
	}
	ttisp = &s->ttis[i];
	tmp = offtime(&t, ttisp->tt_gmtoff);
	tmp->tm_isdst = ttisp->tt_isdst;
#ifdef S5EMUL
	tzname[tmp->tm_isdst] = &s->chars[ttisp->tt_abbrind];
#endif /* S5EMUL */
	tmp->tm_zone = &s->chars[ttisp->tt_abbrind];
	return tmp;
}

struct tm *
gmtime(clock)
time_t *	clock;
{
	register struct tm *	tmp;

	tmp = offtime(clock, 0L);
#ifdef S5EMUL
	tzname[0] = "GMT";
#endif /* S5EMUL */
	tmp->tm_zone = "GMT";		/* UCT ? */
	return tmp;
}

struct tm *
offtime(clock, offset)
time_t *	clock;
long		offset;
{
	register struct tm *	tmp;
	register long		days;
	register long		rem;
	register int		y;
	register int		yleap;
	register int *		ip;
	static struct tm	tm;

	tmp = &tm;
	days = *clock / SECS_PER_DAY;
	rem = *clock % SECS_PER_DAY;
	rem += offset;
	while (rem < 0) {
		rem += SECS_PER_DAY;
		--days;
	}
	while (rem >= SECS_PER_DAY) {
		rem -= SECS_PER_DAY;
		++days;
	}
	tmp->tm_hour = (int) (rem / SECS_PER_HOUR);
	rem = rem % SECS_PER_HOUR;
	tmp->tm_min = (int) (rem / SECS_PER_MIN);
	tmp->tm_sec = (int) (rem % SECS_PER_MIN);
	tmp->tm_wday = (int) ((EPOCH_WDAY + days) % DAYS_PER_WEEK);
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYS_PER_WEEK;
	y = EPOCH_YEAR;
	if (days >= 0)
		for ( ; ; ) {
			yleap = isleap(y);
			if (days < (long) year_lengths[yleap])
				break;
			++y;
			days = days - (long) year_lengths[yleap];
		}
	else do {
		--y;
		yleap = isleap(y);
		days = days + (long) year_lengths[yleap];
	} while (days < 0);
	tmp->tm_year = y - TM_YEAR_BASE;
	tmp->tm_yday = (int) days;
	ip = mon_lengths[yleap];
	for (tmp->tm_mon = 0; days >= (long) ip[tmp->tm_mon]; ++(tmp->tm_mon))
		days = days - (long) ip[tmp->tm_mon];
	tmp->tm_mday = (int) (days + 1);
	tmp->tm_isdst = 0;
	tmp->tm_zone = "";
	tmp->tm_gmtoff = offset;
	return tmp;
}
