#ifndef lint
static	char sccsid[] = "@(#)pc_subr.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#ifndef KERNEL
#define	KERNEL
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <pcfs/pc_label.h>
#include <pcfs/pc_dir.h>

/*
 * Structure returned by gmtime and localtime calls (see ctime(3)).
 */
struct tm {
	short	tm_sec;
	short	tm_min;
	short	tm_hour;
	short	tm_mday;
	short	tm_mon;
	short	tm_year;
	short	tm_wday;
	short	tm_yday;
	short	tm_isdst;
};

/*
 * internal routines
 */
static struct tm *localtime();
static int sunday();
static int dysize();
static struct tm *gmtime();
static long ctime();

/*
 * convert timeval to pctime
 */
void
pc_tvtopct(tvp, pctp)
	struct timeval *tvp;		/* timeval input */
	register struct pctime *pctp;	/* pctime output */
{
	register struct tm *ctp;

	ctp = localtime(tvp->tv_sec);
#define	setfield(S, FIELD, SFT, MSK)	\
	S = (ltohs(S) & ~(MSK << SFT)) | (((FIELD) & MSK) << SFT); S = htols(S);

	setfield(pctp->pct_time, ctp->tm_sec / 2, SECSHIFT, SECMASK);
	setfield(pctp->pct_time, ctp->tm_min, MINSHIFT, MINMASK);
	setfield(pctp->pct_time, ctp->tm_hour, HOURSHIFT, HOURMASK);
	setfield(pctp->pct_date, ctp->tm_mday, DAYSHIFT, DAYMASK);
	setfield(pctp->pct_date, ctp->tm_mon + 1, MONSHIFT, MONMASK);
	setfield(pctp->pct_date, ctp->tm_year - 80, YEARSHIFT, YEARMASK);
#undef setfield
}

/*
 * convert pctime to timeval
 */
void
pc_pcttotv(pctp, tvp)
	struct pctime *pctp;		/* ptime input */
	struct timeval *tvp;		/* tinmeval output */
{
	struct tm tm;

#define	getfield(S, SFT, M)	((ltohs(S) >> SFT) & M)
	tm.tm_sec = getfield(pctp->pct_time, SECSHIFT, SECMASK) * 2;
	tm.tm_min = getfield(pctp->pct_time, MINSHIFT, MINMASK);
	tm.tm_hour = getfield(pctp->pct_time, HOURSHIFT, HOURMASK);
	tm.tm_mday =  getfield(pctp->pct_date, DAYSHIFT, DAYMASK);
	tm.tm_mon = getfield(pctp->pct_date, MONSHIFT, MONMASK) - 1;
	tm.tm_year = 80 + getfield(pctp->pct_date, YEARSHIFT, YEARMASK);
#undef getfield
	tvp->tv_usec = 0;
	tvp->tv_sec = ctime(&tm);
}

/*
 * This routine converts time as follows.
 * The epoch is 0000 Jan 1 1970 GMT.
 * The argument time is in seconds since then.
 * The localtime(t) entry returns a pointer to an array
 * containing
 *  seconds (0-59)
 *  minutes (0-59)
 *  hours (0-23)
 *  day of month (1-31)
 *  month (0-11)
 *  year-1900
 *  weekday (0-6, Sun is 0)
 *  day of the year
 *  daylight savings flag
 *
 * The routine calls the system to determine the local
 * timezone and whether Daylight Saving Time is permitted locally.
 * (DST is then determined by the current local rules)
 *
 * The routine does not work
 * in Saudi Arabia which runs on Solar time.
 *
 */

static	int	dmsize[12] =
{
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

/*
 * The following table is used for 1974 and 1975 and
 * gives the day number of the first day after the Sunday of the
 * change.
 */
struct dstab {
	int	dayyr;
	int	daylb;
	int	dayle;
};

static struct dstab usdaytab[] = {
	1974,	5,	333,	/* 1974: Jan 6 - last Sun. in Nov */
	1975,	58,	303,	/* 1975: Last Sun. in Feb - last Sun in Oct */
	0,	119,	303,	/* all other years: end Apr - end Oct */
};
static struct dstab ausdaytab[] = {
	1970,	400,	0,	/* 1970: no daylight saving at all */
	1971,	303,	0,	/* 1971: daylight saving from Oct 31 */
	1972,	303,	58,	/* 1972: Jan 1 -> Feb 27 & Oct 31 -> dec 31 */
	0,	303,	65,	/* others: -> Mar 7, Oct 31 -> */
};

/*
 * The European tables ... based on hearsay
 * Believed correct for:
 *	WE:	Great Britain, Ireland, Portugal
 *	ME:	Belgium, Luxembourg, Netherlands, Denmark, Norway,
 *		Austria, Poland, Czechoslovakia, Sweden, Switzerland,
 *		DDR, DBR, France, Spain, Hungary, Italy, Jugoslavia
 * Eastern European dst is unknown, we'll make it ME until someone speaks up.
 *	EE:	Bulgaria, Finland, Greece, Rumania, Turkey, Western Russia
 */
static struct dstab wedaytab[] = {
	1983,	86,	303,	/* 1983: end March - end Oct */
	1984,	86,	303,	/* 1984: end March - end Oct */
	1985,	86,	303,	/* 1985: end March - end Oct */
	0,	400,	0,	/* others: no daylight saving at all ??? */
};

static struct dstab medaytab[] = {
	1983,	86,	272,	/* 1983: end March - end Sep */
	1984,	86,	272,	/* 1984: end March - end Sep */
	1985,	86,	272,	/* 1985: end March - end Sep */
	0,	400,	0,	/* others: no daylight saving at all ??? */
};

static struct dayrules {
	int		dst_type;	/* number obtained from system */
	int		dst_hrs;	/* hours to add when dst on */
	struct	dstab *	dst_rules;	/* one of the above */
	enum {STH, NTH}	dst_hemi;	/* southern, northern hemisphere */
} dayrules [] = {
	DST_USA,	1,	usdaytab,	NTH,
	DST_AUST,	1,	ausdaytab,	STH,
	DST_WET,	1,	wedaytab,	NTH,
	DST_MET,	1,	medaytab,	NTH,
	DST_EET,	1,	medaytab,	NTH,	/* XXX */
	-1,
};

static struct tm *
localtime(tim)
long tim;
{
	register int dayno;
	register struct tm *ct;
	register dalybeg, daylend;
	register struct dayrules *dr;
	register struct dstab *ds;
	int year;
	long copyt;

	copyt = tim - (long)tz.tz_minuteswest*60;
	ct = gmtime(copyt);
	dayno = ct->tm_yday;
	for (dr = dayrules; dr->dst_type >= 0; dr++)
		if (dr->dst_type == tz.tz_dsttime)
			break;
	if (dr->dst_type >= 0) {
		year = ct->tm_year + 1900;
		for (ds = dr->dst_rules; ds->dayyr; ds++) {
			if (ds->dayyr == year) {
				break;
			}
		}
		dalybeg = ds->daylb;	/* first Sun after dst starts */
		daylend = ds->dayle;	/* first Sun after dst ends */
		dalybeg = sunday(ct, dalybeg);
		daylend = sunday(ct, daylend);
		switch (dr->dst_hemi) {
		case NTH:
			if (!(
			    (dayno>dalybeg ||
			    (dayno==dalybeg && ct->tm_hour>=2)) &&
			    (dayno<daylend ||
			    (dayno==daylend && ct->tm_hour<1)))) {
				return (ct);
			}
			break;
		case STH:
			if (!(
			    (dayno>dalybeg ||
			    (dayno==dalybeg && ct->tm_hour>=2)) ||
			    (dayno<daylend ||
			    (dayno==daylend && ct->tm_hour<2)))) {
				return (ct);
			}
			break;
		default:
		    return (ct);
		}
		copyt += dr->dst_hrs*60*60;
		ct = gmtime(copyt);
		ct->tm_isdst++;
	}
	return (ct);
}

/*
 * The argument is a 0-origin day number.
 * The value is the day number of the first
 * Sunday on or after the day.
 */
static int
sunday(t, d)
register struct tm *t;
register int d;
{
	if (d >= 58)
		d += dysize(t->tm_year) - 365;
	return (d - (d - t->tm_yday + t->tm_wday + 700) % 7);
}

static int
dysize(y)
{
	if ((y%4) == 0)
		return (366);
	return (365);
}

static struct tm *
gmtime(tim)
long tim;
{
	register int d0, d1;
	long hms, day;
	register short *tp;
	static struct tm xtime;

	/*
	 * break initial number into days
	 */
	hms = tim % 86400;
	day = tim / 86400;
	if (hms<0) {
		hms += 86400;
		day -= 1;
	}
	tp = (short *)&xtime;

	/*
	 * generate hours:minutes:seconds
	 */
	*tp++ = hms%60;
	d1 = hms/60;
	*tp++ = d1%60;
	d1 /= 60;
	*tp++ = d1;

	/*
	 * day is the day number.
	 * generate day of the week.
	 * The addend is 4 mod 7 (1/1/1970 was Thursday)
	 */

	xtime.tm_wday = (day+7340036)%7;

	/*
	 * year number
	 */
	if (day>=0) for (d1=70; day >= dysize(d1); d1++) {
		day -= dysize(d1);
	} else for (d1=70; day<0; d1--) {
		day += dysize(d1-1);
	}
	xtime.tm_year = d1;
	xtime.tm_yday = d0 = day;

	/*
	 * generate month
	 */

	if (dysize(d1)==366)
		dmsize[1] = 29;
	for (d1=0; d0 >= dmsize[d1]; d1++)
		d0 -= dmsize[d1];
	dmsize[1] = 28;
	*tp++ = d0+1;
	*tp++ = d1;
	xtime.tm_isdst = 0;
	return (&xtime);
}

/*
 * convert year, month, day, hour, minute, sec to (long)time.
 */
static long
ctime(tp)
	register struct tm *tp;
{
	register int i;
	register long ct;

	if (tp->tm_mon < 0 || tp->tm_mon > 11 ||
	    tp->tm_mday < 1 || tp->tm_mday > 31 ||
	    tp->tm_hour < 0 || tp->tm_hour > 23 ||
	    tp->tm_min < 0 || tp->tm_min > 59 ||
	    tp->tm_sec < 0 || tp->tm_sec > 59) {
		return (0);
	}
	ct = 0;
	for (i = /* 19 */ 70; i < tp->tm_year; i++)
		ct += dysize(i);
	/* Leap year */
	if (dysize(tp->tm_year) == 366 && tp->tm_mon >= 2)
		ct++;
	i = tp->tm_mon + 1;
	while (--i)
		ct += dmsize[i-1];
	ct += tp->tm_mday-1;
	ct = 24*ct + tp->tm_hour;
	ct = 60*ct + tp->tm_min;
	ct = 60*ct + tp->tm_sec;
	/* convert to GMT assuming local time */
	ct += (long)tz.tz_minuteswest*60;
	/* now fix up local daylight time */
	if (localtime(ct)->tm_isdst)
		ct -= 60*60;
	return (ct);
}

/*
 * Determine whether a character is valid for a pc file system file name.
 */
int
pc_validchar(c)
	register char c;
{
	register char *cp;
	register int n;
	static char valtab[] = {
		"$#&@!%()-{}<>`_\\^~|' "
	};

	/*
	 * Should be "$#&@!%()-{}`_^~' " ??
	 * From experiment in DOSWindows, *+=|\[];:",<>.?/ are illegal.
	 * See IBM DOS4.0 Tech Ref. B-57.
	 */

	if (c >= 'A' && c <= 'Z')
		return (1);
	if (c >= '0' && c <= '9')
		return (1);
	cp = valtab;
	n = sizeof (valtab);
	while (n--) {
		if (c == *cp++)
			return (1);
	}
	return (0);
}
