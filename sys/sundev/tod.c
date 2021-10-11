#ifndef lint
static	char sccsid[] = "@(#)tod.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Time-of-day driver for National MM58167 chip
 */
#include "tod.h"
#if NTOD > 0
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/time.h>
#include <sundev/mbvar.h>
#include <sundev/todreg.h>

/*
 * Driver information for auto-configuration stuff.
 */
int	todprobe();
struct	mb_device *todinfo[1];		/* one TOD only */
struct	mb_driver toddriver = {
	todprobe, 0, 0, 0, 0, 0,
	2 * sizeof(struct toddevice), "tod", todinfo, 0, 0, 0,
};

struct timeval todget();
int todset();
static struct toddevice *todaddr;

todprobe(reg)
	caddr_t reg;
{
	register struct toddevice *t = (struct toddevice *)reg;
	register int m1;

	if (todaddr)
		return (0);
	if ((m1 = peekc((char *)&t->tod_counter[TOD_MSEC].val)) == -1)
		return (0);
	if (t->tod_status <= 1 && (m1&0xF) == 0) {
		m1 = t->tod_counter[TOD_MSEC].val;
		DELAY(2000);
		if (m1 != t->tod_counter[TOD_MSEC].val) {
			todaddr = t;
			tod_attach(todget, todset);
			return (1);
		}
	}
	return (0);
}

static short monthdays[12] = { 	/* running sum of #days per month */
	0,		/* Jan +31 */
	31,		/* Feb +28 */
	59,		/* Mar +31 */
	90,		/* Apr +30 */
	120,		/* May +31 */
	151,		/* Jun +30 */
	181,		/* Jul +31 */
	212,		/* Aug +31 */
	243,		/* Sep +30 */
	273,		/* Oct +31 */
	304,		/* Nov +30 */
	334,		/* Dec +31 */
	/* 365 */
};

/*
 * Set the TOD based on the argument value; used when the TOD
 * has a preposterous value and also when the time is reset
 * by the stime system call.
 */
todset(tv)
	struct timeval tv;
{
	register int t;
	short msec, tsec, sec, minute, hour, day, mon, weekday;

	t = tv.tv_sec % SECYR;
	msec = tv.tv_usec / 1000;
	tsec = msec/10;
	msec *= 10;
	sec = t % 60;
	t /= 60;
	minute = t % 60;
	t /= 60;
	hour = t % 24;
	day = t / 24;
	weekday = (day % 7) + 1;
	for (mon=11; mon>=0; mon--) 
		if (day >= monthdays[mon])
			break; 
	day -= monthdays[mon];
	day++;
	mon++;
	todsetval(TOD_TSEC, 0);		/* prevent counter overflow */
	todsetval(TOD_MONTH, mon);
	todsetval(TOD_DAY, day);
	todsetval(TOD_WEEKDAY, weekday);
	todsetval(TOD_HOUR, hour);
	todsetval(TOD_MIN, minute);
	todsetval(TOD_SEC, sec);
	todsetval(TOD_TSEC, tsec);
	todsetval(TOD_MSEC, msec);
}

struct timeval
todget(tvbase)
	struct timeval tvbase;
{
	struct timeval tv;
	u_short c[8];
	register struct toddevice *t = todaddr;
	int nloop = 0;
	register int i, toy, base, basetoy;

	/*
	 * Read the TOD chip and convert into a linear second counter
	 * of range 0..SECYR-1
	 */
top:
	if (nloop++ > 99) {
		printf("TOD chip has gone berserk\n");
		return (tvbase);
	}
	for (i=0; i<8; i++) {
		c[i] = t->tod_counter[i].val;
		if (t->tod_status)
			goto top;
	}
	for (i=0; i<8; i++)
		if(c[i] != t->tod_counter[i].val)
			goto top;
	/* now we have a consistent sample; convert from BCD to binary */
	for (i=0; i<8; i++)
		c[i] = (c[i]&0xF) + 10*(c[i]>>4);

	if (c[TOD_MONTH] < 1 || c[TOD_DAY] < 1 ||
	    c[TOD_MONTH] > 12 || c[TOD_DAY] > 31 ||
	    c[TOD_WEEKDAY] < 1 || c[TOD_WEEKDAY] > 7) {	/* not initialized */
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		return (tv);
	}
	toy = monthdays[c[TOD_MONTH] - 1];
	toy += c[TOD_DAY] - 1;
	toy = 24*toy + c[TOD_HOUR];
	toy = 60*toy + c[TOD_MIN];
	toy = 60*toy + c[TOD_SEC];
	base = tvbase.tv_sec;
	basetoy = base % SECYR;
	toy -= basetoy;
	if (toy > SECYR/2)
		toy -= SECYR;
	else if (toy < -SECYR/2)
		toy += SECYR;
	tv.tv_sec = base + toy;
	tv.tv_usec = 100 * (100*c[TOD_TSEC] + c[TOD_MSEC]);
	return (tv);
}

/*
 * Write a TOD counter
 * Convert to BCD first
 */
todsetval(counter, val)
	register short val;
{
	register short dig1 = val/10;
	register short dig2 = val%10;

	todaddr->tod_counter[counter].val = (dig1<<4) + dig2;
}
#endif
