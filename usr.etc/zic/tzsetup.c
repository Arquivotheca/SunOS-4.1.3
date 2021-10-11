#ifndef lint
static	char sccsid[] = "@(#)tzsetup.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>

extern int	fprintf();
extern char	*sprintf();

#include <fcntl.h>

#include <errno.h>

#include <sys/param.h>

#include <sys/time.h>

extern time_t	time();
extern int	settimeofday();

#include <tzfile.h>

#include <values.h>

static long	detzcode(/*char *codep*/);
static int	tzload(/*char *name*/);
static void	get_timezone_info(/*struct timezone *tz*/);
static void	get_dsttime(/*struct timezone *tz*/);
static void	test(/*time_t t, int minuteswest, struct dayrules *dr,
    int weight*/);
static struct tm *oldlocaltime(/*time_t *tim, int minuteswest,
    struct dayrules *dr*/);
static int	sunday(/*struct tm *t, int d*/);
static char	*strerror(/*int errnum*/);

static time_t	now;

/*
 * The following tables specify the days that daylight savings time
 * started and ended for some year or, if the year in the table is
 * 0, for all years not explicitly mentioned in the table.
 * Both days are assumed to be Sundays.  For entries for specific years,
 * they are given as the day number of the Sunday of the change.  For
 * wildcard entries, it is assumed that the day is specified by a rule
 * of the form "first Sunday of <some month>" or "last Sunday of <some
 * month>."  In the former case, the negative of the day number of the
 * first day of that month is given; in the latter case, the day number
 * of the last day of that month is given.
 *
 * In the northern hemisphere, Daylight Savings Time runs for a period in
 * the middle of the year; thus, days between the start day and the end
 * day have DST active.  In the southern hemisphere, Daylight Savings Time
 * runs from the beginning of the year to some time in the middle of the
 * year, and from some time later in the year to the end of the year; thus,
 * days after the start day or before the end day have DST active.
 */
struct dstab {
	int	dayyr;
	int	daylb;
	int	dayle;
};

/*
 * The U.S. tables, including the latest hack.
 */
static struct dstab usdaytab[] = {
	1970,	119,	303,	/* 1970: last Sun. in Apr - last Sun. in Oct */
	1971,	119,	303,	/* 1971: last Sun. in Apr - last Sun. in Oct */
	1972,	119,	303,	/* 1972: last Sun. in Apr - last Sun. in Oct */
	1973,	119,	303,	/* 1973: last Sun. in Apr - last Sun. in Oct */
	1974,	5,	303,	/* 1974: Jan 6 - last Sun. in Oct */
	1975,	58,	303,	/* 1975: Last Sun. in Feb - last Sun. in Oct */
	1976,	119,	303,	/* 1976: last Sun. in Apr - last Sun. in Oct */
	1977,	119,	303,	/* 1977: last Sun. in Apr - last Sun. in Oct */
	1978,	119,	303,	/* 1978: last Sun. in Apr - last Sun. in Oct */
	1979,	119,	303,	/* 1979: last Sun. in Apr - last Sun. in Oct */
	1980,	119,	303,	/* 1980: last Sun. in Apr - last Sun. in Oct */
	1981,	119,	303,	/* 1981: last Sun. in Apr - last Sun. in Oct */
	1982,	119,	303,	/* 1982: last Sun. in Apr - last Sun. in Oct */
	1983,	119,	303,	/* 1983: last Sun. in Apr - last Sun. in Oct */
	1984,	119,	303,	/* 1984: last Sun. in Apr - last Sun. in Oct */
	1985,	119,	303,	/* 1985: last Sun. in Apr - last Sun. in Oct */
	1986,	119,	303,	/* 1986: last Sun. in Apr - last Sun. in Oct */
	0,	-90,	303,	/* 1987 on: first Sun. in Apr - last Sun. in Oct */
};

/*
 * Canada, same as the US, except no early 70's fluctuations and no late 80's
 * hack.
 */
static struct dstab candaytab[] = {
	1970,	119,	303,	/* 1970: last Sun. in Apr - last Sun. in Oct */
	1971,	119,	303,	/* 1971: last Sun. in Apr - last Sun. in Oct */
	1972,	119,	303,	/* 1972: last Sun. in Apr - last Sun. in Oct */
	1973,	119,	303,	/* 1973: last Sun. in Apr - last Sun. in Oct */
	1974,	119,	303,	/* 1974: last Sun. in Apr - last Sun. in Oct */
	1975,	119,	303,	/* 1975: Last Sun. in Apr - last Sun. in Oct */
	1976,	119,	303,	/* 1976: last Sun. in Apr - last Sun. in Oct */
	1977,	119,	303,	/* 1977: last Sun. in Apr - last Sun. in Oct */
	1978,	119,	303,	/* 1978: last Sun. in Apr - last Sun. in Oct */
	1979,	119,	303,	/* 1979: last Sun. in Apr - last Sun. in Oct */
	1980,	119,	303,	/* 1980: last Sun. in Apr - last Sun. in Oct */
	1981,	119,	303,	/* 1981: last Sun. in Apr - last Sun. in Oct */
	1982,	119,	303,	/* 1982: last Sun. in Apr - last Sun. in Oct */
	1983,	119,	303,	/* 1983: last Sun. in Apr - last Sun. in Oct */
	1984,	119,	303,	/* 1984: last Sun. in Apr - last Sun. in Oct */
	1985,	119,	303,	/* 1985: last Sun. in Apr - last Sun. in Oct */
	1986,	119,	303,	/* 1986: last Sun. in Apr - last Sun. in Oct */
	0,	-90,	303,	/* 1987 on: first Sun. in Apr - last Sun. in Oct */
};

/*
 * The Australian tables, for states with DST that don't shift the ending time
 * starting in 1986, but shift it starting in 1987.
 */
static struct dstab ausdaytab[] = {
	1970,	400,	0,	/* 1970: no daylight saving at all */
	1971,	303,	0,	/* 1971: daylight saving from last Sun. in Oct */
	1972,	303,	57,	/* 1972: Jan 1 -> Feb 27 & last Sun. in Oct -> Dec 31 */
	1973,	303,	-59,	/* 1973: -> first Sun. in Mar, last Sun. in Oct -> */
	1974,	303,	-59,	/* 1974: -> first Sun. in Mar, last Sun. in Oct -> */
	1975,	303,	-59,	/* 1975: -> first Sun. in Mar, last Sun. in Oct -> */
	1976,	303,	-59,	/* 1976: -> first Sun. in Mar, last Sun. in Oct -> */
	1977,	303,	-59,	/* 1977: -> first Sun. in Mar, last Sun. in Oct -> */
	1978,	303,	-59,	/* 1978: -> first Sun. in Mar, last Sun. in Oct -> */
	1979,	303,	-59,	/* 1979: -> first Sun. in Mar, last Sun. in Oct -> */
	1980,	303,	-59,	/* 1980: -> first Sun. in Mar, last Sun. in Oct -> */
	1981,	303,	-59,	/* 1981: -> first Sun. in Mar, last Sun. in Oct -> */
	1982,	303,	-59,	/* 1982: -> first Sun. in Mar, last Sun. in Oct -> */
	1983,	303,	-59,	/* 1983: -> first Sun. in Mar, last Sun. in Oct -> */
	1984,	303,	-59,	/* 1984: -> first Sun. in Mar, last Sun. in Oct -> */
	1985,	303,	-59,	/* 1985: -> first Sun. in Mar, last Sun. in Oct -> */
	1986,	-290,	-59,	/* 1986: -> first Sun. in Mar, first Sun. after Oct 17 -> */
	0,	-290,	79,	/* 1987 on: -> last Sun. before Mar 21, first Sun. after Oct 17 -> */
};

/*
 * The Australian tables, for states with DST that do shift the ending time
 * starting in 1986.  NSW does so; there seems to be a difference of opinion
 * about which other states do.  There is also a variation in 1983, but
 * Robert Elz didn't have it at hand when last he reported.
 * Extending the 1986 shift on to infinity is Elz's best guess.
 */
static struct dstab ausaltdaytab[] = {
	1970,	400,	0,	/* 1970: no daylight saving at all */
	1971,	303,	0,	/* 1971: daylight saving from last Sun. in Oct */
	1972,	303,	57,	/* 1972: Jan 1 -> Feb 27 & last Sun. in Oct -> Dec 31 */
	1973,	303,	-59,	/* 1973: -> first Sun. in Mar, last Sun. in Oct -> */
	1974,	303,	-59,	/* 1974: -> first Sun. in Mar, last Sun. in Oct -> */
	1975,	303,	-59,	/* 1975: -> first Sun. in Mar, last Sun. in Oct -> */
	1976,	303,	-59,	/* 1976: -> first Sun. in Mar, last Sun. in Oct -> */
	1977,	303,	-59,	/* 1977: -> first Sun. in Mar, last Sun. in Oct -> */
	1978,	303,	-59,	/* 1978: -> first Sun. in Mar, last Sun. in Oct -> */
	1979,	303,	-59,	/* 1979: -> first Sun. in Mar, last Sun. in Oct -> */
	1980,	303,	-59,	/* 1980: -> first Sun. in Mar, last Sun. in Oct -> */
	1981,	303,	-59,	/* 1981: -> first Sun. in Mar, last Sun. in Oct -> */
	1982,	303,	-59,	/* 1982: -> first Sun. in Mar, last Sun. in Oct -> */
	1983,	303,	-59,	/* 1983: -> first Sun. in Mar, last Sun. in Oct -> */
	1984,	303,	-59,	/* 1984: -> first Sun. in Mar, last Sun. in Oct -> */
	1985,	303,	-59,	/* 1985: -> first Sun. in Mar, last Sun. in Oct -> */
	0,	-290,	79,	/* 1986 on: -> last Sun. before Mar 21, first Sun. after Oct 17 -> */
};

/*
 * The European tables, based on investigations by PTB, Braunschweig, FRG.
 * Believed correct for:
 *	GB:	United Kingdom and Eire
 *	WE:	Portugal, Poland (in fact MET, following WE dst rules)
 *	ME:	Belgium, Luxembourg, Netherlands, Denmark, Norway,
 *		Austria, Czechoslovakia, Sweden, Switzerland,
 *		FRG, GDR,  France, Spain, Hungary, Italy, Yugoslavia,
 *		Western USSR (in fact EET+1; following ME dst rules)
 *	EE:	Finland, Greece, Israel?
 *
 * Problematic cases are:
 *	WE:	Iceland (no dst)
 *	EE:	Rumania, Turkey (in fact timezone EET+1)
 * Terra incognita:
 *		Albania (MET), Bulgaria (EET), Cyprus (EET)
 *
 * Years before 1986 are suspect (complicated changes caused
 * e.g. by enlargement of the European Community).
 * Years before 1983 are VERY suspect (sigh!).
 */
static struct dstab gbdaytab[] = {	/* GB and Eire */
	0,	89,	303,	/* all years: last Sun. in March - last Sun. in Oct */
};

static struct dstab cedaytab[] = {	/* Continental European */
	0,	89,	272,	/* all years: last Sun. in March - last Sun. in Sep */
};

static struct dayrules {
	int		dst_type;	/* number obtained from system */
	int		dst_hrs;	/* hours to add when dst on */
	struct	dstab *	dst_rules;	/* one of the above */
	enum {STH,NTH}	dst_hemi;	/* southern, northern hemisphere */
	int		dst_ontime;	/* hour when DST turns on */
	int		dst_offtime;	/* hour when DST turns off */
	int		dst_errs;	/* number of incorrect conversions */
} dayrules [] = {
	DST_USA,	1,	usdaytab,	NTH,	2,	1,	0,
	DST_CAN,	1,	candaytab,	NTH,	2,	1,	0,
	DST_AUST,	1,	ausdaytab,	STH,	2,	2,	0,
	DST_AUSTALT,	1,	ausaltdaytab,	STH,	2,	2,	0,
	DST_GB,		1,	gbdaytab,	NTH,	1,	1,	0,
	DST_WET,	1,	cedaytab,	NTH,	1,	1,	0,
	DST_MET,	1,	cedaytab,	NTH,	2,	2,	0,
	DST_EET,	1,	cedaytab,	NTH,	3,	3,	0,
	DST_RUM,	1,	cedaytab,	NTH,	0,	0,	0,
	DST_TUR,	1,	cedaytab,	NTH,	1,	0,	0,
	-1,
};

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* GMT offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
	int		tt_ttisstd;	/* TRUE if transition is std time */
};

struct state {
	int		timecnt;
	int		typecnt;
	int		charcnt;
	time_t		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
};

struct state lclstate;

/*
 * tzsetup - Given the current time zone rules, as defined by the
 * "localtime" time zone information file, figure out what the offset from
 * GMT is and which of the old-style time zone rules matches those rules.
 *
 * This is done by heuristics (the $10 word for "hacks"); it finds all the
 * transition times in that information file, throws out all the ones
 * before the epoch and after the day on which this is being written, and
 * sees which, if any, of the old-style rules best reproduces the behavior
 * of the new-style rules for all those transition times.
 */
/*ARGSUSED*/
int
main(argc, argv)
	int argc;
	char *argv[];
{
	char buf[MAXPATHLEN];
	register int i;
	struct timezone tz;

	now = time((time_t *)NULL);
	tzsetwall();	/* make absolutely sure we get local time */
	i = strlen(TZDIR) + 1 + strlen("localtime") + 1;
	if (i > sizeof buf) {
		(void) fprintf(stderr,
		    "tzsetup: timezone name %s/localtime is too long\n",
		    TZDIR);
		return (1);
	}
	(void) sprintf(buf, "%s/localtime", TZDIR);
	if (tzload(buf) < 0)
		return (1);

	get_timezone_info(&tz);

	if (settimeofday((struct timeval *)NULL, &tz) < 0) {
		perror("tzsetup: Can't set time zone");
		return (1);
	}
	return (0);
}

static long
detzcode(codep)
	char *codep;
{
	register long	result;
	register int	i;

	result = 0;
	for (i = 0; i < 4; ++i)
		result = (result << 8) | (codep[i] & 0xff);
	return (result);
}

/*
** Maximum size of a time zone file.
*/
#define	MAX_TZFILESZ	(sizeof (struct tzhead) + \
			TZ_MAX_TIMES * (4 + sizeof (char)) + \
			TZ_MAX_TYPES * (4 + 2 * sizeof (char)) + \
			TZ_MAX_CHARS * sizeof (char) + \
			TZ_MAX_LEAPS * 2 * 4 + \
			TZ_MAX_TYPES * sizeof (char))

static int
tzload(name)
	register char *name;
{
	register char *p;
	register int i;
	register int fid;
	register struct tzhead *tzhp;
	char buf[MAX_TZFILESZ];
	int leapcnt;
	int ttisstdcnt;

	if ((fid = open(name, O_RDONLY, 0)) == -1) {
		(void) fprintf(stderr,
		    "tzsetup: Can't open %s/localtime: %s\n", TZDIR,
		    strerror(errno));
		return -1;
	}

	i = read(fid, buf, sizeof buf);
	if (i < 0) {
		(void) fprintf(stderr,
		    "tzsetup: Error reading %s/localtime: %s", TZDIR,
		    strerror(errno));
		return -1;
	}
	(void) close(fid);
	if (i < sizeof *tzhp) {
		(void) fprintf(stderr,
		    "tzsetup: File %s/localtime is damaged\n", TZDIR);
		return -1;
	}
	tzhp = (struct tzhead *) buf;
	ttisstdcnt = (int) detzcode(tzhp->tzh_ttisstdcnt);
	leapcnt = (int) detzcode(tzhp->tzh_leapcnt);
	lclstate.timecnt = (int) detzcode(tzhp->tzh_timecnt);
	lclstate.typecnt = (int) detzcode(tzhp->tzh_typecnt);
	lclstate.charcnt = (int) detzcode(tzhp->tzh_charcnt);
	if (leapcnt < 0 || leapcnt > TZ_MAX_LEAPS ||
	    lclstate.typecnt <= 0 || lclstate.typecnt > TZ_MAX_TYPES ||
	    lclstate.timecnt < 0 || lclstate.timecnt > TZ_MAX_TIMES ||
	    lclstate.charcnt < 0 || lclstate.charcnt > TZ_MAX_CHARS ||
	    (ttisstdcnt != lclstate.typecnt && ttisstdcnt != 0)) {
		(void) fprintf(stderr,
		    "tzsetup: File %s/localtime is damaged\n", TZDIR);
		return -1;
	}
	if (i < sizeof *tzhp +
	    lclstate.timecnt * (4 + sizeof (char)) +
	    lclstate.typecnt * (4 + 2 * sizeof (char)) +
	    lclstate.charcnt * sizeof (char) +
	    leapcnt * 2 * 4 +
	    ttisstdcnt * sizeof (char)) {
		(void) fprintf(stderr,
		    "tzsetup: File %s/localtime is damaged\n", TZDIR);
		return -1;
	}
	p = buf + sizeof *tzhp;
	for (i = 0; i < lclstate.timecnt; ++i) {
		lclstate.ats[i] = detzcode(p);
		p += 4;
	}
	for (i = 0; i < lclstate.timecnt; ++i) {
		lclstate.types[i] = (unsigned char) *p++;
		if (lclstate.types[i] >= lclstate.typecnt) {
			(void) fprintf(stderr,
			    "tzsetup: File %s/localtime is damaged\n", TZDIR);
			return -1;
		}
	}
	for (i = 0; i < lclstate.typecnt; ++i) {
		register struct ttinfo *	ttisp;

		ttisp = &lclstate.ttis[i];
		ttisp->tt_gmtoff = detzcode(p);
		p += 4;
		ttisp->tt_isdst = (unsigned char) *p++;
		if (ttisp->tt_isdst != 0 && ttisp->tt_isdst != 1) {
			(void) fprintf(stderr,
			    "tzsetup: File %s/localtime is damaged\n", TZDIR);
			return -1;
		}
		ttisp->tt_abbrind = (unsigned char) *p++;
		if (ttisp->tt_abbrind < 0 ||
		    ttisp->tt_abbrind > lclstate.charcnt) {
			(void) fprintf(stderr,
			    "tzsetup: File %s/localtime is damaged\n", TZDIR);
			return -1;
		}
	}
	return 0;
}

/*
 * Get the appropriate values for "tz_minuteswest" and "tz_dsttime".
 */
static void
get_timezone_info(tz)
	register struct timezone *tz;
{
	register int i;
	register int daylight;

	tz->tz_minuteswest = 0;
	daylight = 0;
	for (i = 0; i < lclstate.typecnt; ++i) {
		register struct ttinfo *ttisp = &lclstate.ttis[i];

		if (ttisp->tt_isdst)
			daylight = 1;
		if (i == 0 || !ttisp->tt_isdst)
			tz->tz_minuteswest = -(ttisp->tt_gmtoff/60);
	}

	if (daylight)
		get_dsttime(tz);
	else
		tz->tz_dsttime = DST_NONE;	/* no DST */
}

/*
 * Try to find the appropriate value for "tz_dsttime".
 */
static void
get_dsttime(tz)
	register struct timezone *tz;
{
	register int i, j;
	register int maxerrs;
	register int ntried;
	register int cand;
	time_t t;

	ntried = 0;
	for (j = 0; j < lclstate.timecnt; ++j) {
		t = lclstate.ats[j];
		/*
		 * If the time is before the epoch, there's no guarantee
		 * that it will be converted correctly by the old-style
		 * mechanism; skip it.
		 */
		if (t < 0)
			continue;

		/*
		 * If the time is in the future, skip it.
		 */
		if (t > now)
			continue;

		ntried += 2;	/* we try two times a crack */
		for (i = 0; dayrules[i].dst_type != -1; i++) {
			test(t - 1, tz->tz_minuteswest, &dayrules[i], j);
			test(t, tz->tz_minuteswest, &dayrules[i], j);
		}
	}

	maxerrs = MAXINT;	/* haven't picked one yet */
	for (i = 0; dayrules[i].dst_type != -1; i++) {
		if (dayrules[i].dst_errs < maxerrs) {
			/*
			 * This rule does better than the rules we've
			 * checked so far.
			 */
			maxerrs = dayrules[i].dst_errs;
			cand = i;
		}
	}

	for (i = 0; dayrules[i].dst_type != -1; i++) {
		if (i != cand && dayrules[i].dst_errs == maxerrs) {
			/*
			 * This rule isn't the one we picked, but it's just
			 * as good.  We will set the offset from GMT, so
			 * they don't lose totally, but set the DST rule
			 * type to "no DST".
			 */
			(void) fprintf(stderr,
"tzsetup: Two or more time zone types are equally valid - no DST selected\n");
			tz->tz_dsttime = DST_NONE;
			return;
		}
	}

	if (maxerrs == ntried) {
		/*
		 * NONE of the old-style time zone types matches what we
		 * have here!  We will set the offset from GMT, so they
		 * don't lose totally, but set the DST rule type to "no
		 * DST".
		 */
		(void) fprintf(stderr,
"tzsetup: No old-style time zone type is valid - no DST selected\n");
		tz->tz_dsttime = DST_NONE;
	} else {
		if (maxerrs > 0)
			/*
			 * The best candidate isn't perfect.
			 */
			(void) fprintf(stderr,
"tzsetup: Warning: No old-style time zone type is completely valid\n");
		tz->tz_dsttime = dayrules[cand].dst_type;
	}
}

static void
test(t, minuteswest, dr, weight)
	time_t t;
	int minuteswest;
	register struct dayrules *dr;
	int weight;
{
	struct tm tm;
	register struct tm *oldtmp;

	/*
	 * Note: we must make a copy of the result of "localtime".  Since
	 * "oldlocaltime" calls "gmtime", it will trash the results of any
	 * previous call to "localtime".
	 */
	tm = *localtime(&t);
	oldtmp = oldlocaltime(&t, minuteswest, dr);
	if (tm.tm_sec != oldtmp->tm_sec
	    || tm.tm_min != oldtmp->tm_min
	    || tm.tm_hour != oldtmp->tm_hour
	    || tm.tm_mday != oldtmp->tm_mday
	    || tm.tm_mon != oldtmp->tm_mon
	    || tm.tm_year != oldtmp->tm_year
	    || tm.tm_wday != oldtmp->tm_wday
	    || tm.tm_yday != oldtmp->tm_yday
	    || tm.tm_isdst != oldtmp->tm_isdst)
		dr->dst_errs += weight;	/* not correct */
}

/*
 * Old "localtime" code from SunOS 3.x.
 */
static struct tm *
oldlocaltime(tim, minuteswest, dr)
	time_t *tim;
	int minuteswest;
	register struct dayrules *dr;
{
	register int dayno;
	register struct tm *ct;
	register daylbegin, daylend;
	register struct dstab *ds;
	int year;
	time_t copyt;

	copyt = *tim - (time_t)minuteswest*60;
	ct = gmtime(&copyt);
	dayno = ct->tm_yday;
	if (dr->dst_type >= 0) {
		year = ct->tm_year + 1900;
		for (ds = dr->dst_rules; ds->dayyr; ds++)
			if (ds->dayyr == year)
				break;
		daylbegin = sunday(ct, ds->daylb);	/* Sun on which dst starts */
		daylend = sunday(ct, ds->dayle);	/* Sun on which dst ends */
		switch (dr->dst_hemi) {
		case NTH:
		    if (!(
		       (dayno>daylbegin
			|| (dayno==daylbegin && ct->tm_hour>=dr->dst_ontime)) &&
		       (dayno<daylend
			|| (dayno==daylend && ct->tm_hour<dr->dst_offtime))
		    ))
			    return(ct);
		    break;
		case STH:
		    if (!(
		       (dayno>daylbegin
			|| (dayno==daylbegin && ct->tm_hour>=dr->dst_ontime)) ||
		       (dayno<daylend
			|| (dayno==daylend && ct->tm_hour<dr->dst_offtime))
		    ))
			    return(ct);
		    break;
		default:
		    return(ct);
		}
	        copyt += dr->dst_hrs*60*60;
		ct = gmtime(&copyt);
		ct->tm_isdst++;
	}
	return(ct);
}

/*
 * The argument is a 0-origin day number.
 * The value is the day number of the last
 * Sunday on or before the day (if "d" is positive)
 * or of the first Sunday on or after the day (if "d" is
 * negative).
 */
static int
sunday(t, d)
	register struct tm *t;
	register int d;
{
	register int offset;	/* 700 if before, -700 if after */

	offset = 700;
	if (d < 0) {
		offset = -700;
		d = -d;
	}
	if (d >= 58 && (t->tm_year%4) == 0)
		d++;		/* leap year */
	return(d - (d - t->tm_yday + t->tm_wday + offset) % 7);
}

static char *
strerror(errnum)
	int errnum;
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	static char unknown_error[6+10+1];	/* "Error %d\0" */

	if (errnum < 0 || errnum > sys_nerr) {
		(void) sprintf(unknown_error, "Error %d", errnum);
		return unknown_error;
	} else
		return sys_errlist[errnum];
}
