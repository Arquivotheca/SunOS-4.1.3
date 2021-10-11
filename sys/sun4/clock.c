#ifndef lint
static char sccsid[] = "@(#)clock.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <machine/mmu.h>
#include <machine/clock.h>
#include <machine/intreg.h>

struct timeval todget();

int clock_type;

/*
 * Defines for conversino between binary and bcd representation.  These are
 * necessary for the Mostek 48T02 TOD clock regs.  (Works only on 8 bit
 * unsigned values.)
 */
#define	bcdtob(x)	(((x) & 0xf) + 10 * ((x) >> 4))
#define	btobcd(x)	((((x) / 10) << 4) + ((x) % 10))

/*
 * Machine-dependent clock routines.
 * 
 * Startrtclock restarts the real-time clock, which provides hardclock
 * interrupts to kern_clock.c.
 * 
 * Inittodr initializes the time of day hardware which provides date functions.
 * Its primary function is to use some file system information in case the
 * hardare clock lost state.
 * 
 * Resettodr restores the time of day hardware after a time change.
 */

/*
 * Start the real-time clock. We set things up to interrupt every 1/100 of a
 * second.
 */
startrtclock()
{
	if (hz != 100)
		panic("startrtclock");

#ifndef SAS
	if (clock_type == INTERSIL7170)
		CLOCK0->clk_intrreg = CLK_INT_HSEC;	/* set 1/100 sec clk int */
	else 
		/* the counter value is initialized to 1 by the HW and not 0. 
		 * Therefore the limit should be one more than the computed 
		 * number. Else we will lose 1us/10ms (bugid 1094383).
		 */
		COUNTER->limit10 =
		    (((1000000 / hz) + 1) << CTR_USEC_SHIFT) & CTR_USEC_MASK;
#endif	/* !SAS */

	set_clk_mode(IR_ENA_CLK10, 0);	/* turn on level 5 clock intr */
}

/*
 * Set and/or clear the desired clock bits in the interrupt register. We have
 * to be extremely careful that we do it in such a manner that we don't get
 * ourselves lost.
 */
set_clk_mode(on, off)
u_char on, off;
{
	register u_char intreg, dummy;
	register int s;

	/*
	 * make sure that we are only playing w/ clock interrupt register
	 * bits
	 */
	on &= (IR_ENA_CLK14 | IR_ENA_CLK10);
	off &= (IR_ENA_CLK14 | IR_ENA_CLK10);

	/*
	 * Get a copy of current interrupt register, turning off any
	 * undesired bits (aka `off')
	 */
	s = spl7();
	intreg = *INTREG & ~(off | IR_ENA_INT);

	/*
	 * Next we turns off the CLK10 and CLK14 bits to clear the
	 * flip-flops, then we disable clock interrupts. Now we can read the
	 * clock's interrupt register to clear any pending signals there.
	 */
	*INTREG &= ~(IR_ENA_CLK14 | IR_ENA_CLK10);
#ifndef SAS
	if (clock_type == INTERSIL7170) {
		CLOCK0->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_INTRENA);
		dummy = CLOCK0->clk_intrreg;	/* clear clock */
	} else {
		dummy = COUNTER->limit10;
		dummy = COUNTER->limit14;
	}
#endif	/* !SAS */

#ifdef lint
	dummy = dummy;
#endif

	/*
	 * Now we set all the desired bits in the interrupt register, then we
	 * turn the clock back on and finally we can enable all interrupts.
	 */
	*INTREG |= (intreg | on);	/* enable flip-flops */
#ifndef SAS
	if (clock_type == INTERSIL7170) {
		CLOCK0->clk_cmd = CLK_CMD_NORMAL;	/* enable clock intr */
	}
#endif	/* !SAS */
	(void) splx(s);
}

int dosynctodr = 1;		/* if true, sync UNIX time to TOD */
int clkdrift = 0;		/* if true, show UNIX & TOD sync differences */
int synctodrval = 30;		/* number of seconds between synctodr */
extern long timedelta;
extern int tickadj;
extern int tickdelta;
extern int doresettodr;

#define	ABS(x)		((x) < 0? -(x) : (x))

int todsyncat = 0;

synctodr()
{
	struct timeval tv;
	int deltat, s;
	int sch = synctodrval * hz - 2;

	/*
	 * If timedelta is non-zero, assume someone who knows better is
	 * already adjusting the time. Don't assume that this nice person is
	 * going to hang around forever, tho.
	 */
	if (dosynctodr && timedelta == 0 && doresettodr == 0) {
		s = splclock();
		tv = todget();
		/*
		 * Sorry about the gotos, but in this case they are more
		 * clear than nested conditionals or a do { ... } while (0);
		 * construct with multiple break statements, or replication
		 * of the splx() and timeout() code.  All of the gotos here
		 * branch down to the bottom of the current conditional, just
		 * above where the priority is restored.
		 */

		/*
		 * Several machines have clock chips that only give us one
		 * second of precision; for them, we must hang out until the
		 * clock changes. When this happens, we can assume we are
		 * near (within 10ms) the beginning of the second, and apply
		 * the normal algorithm.
		 * 
		 * Systems with higher precision clocks are not hurt by having
		 * this run, the overhead is nearly zero.
		 */

		if (!todsyncat)
			todsyncat = tv.tv_sec;
		if (todsyncat == tv.tv_sec) {
			sch = 1;
			goto bottom;
		}
		/*
		 * Sanity check: if todget is returning zeros, bitch and turn
		 * off clock synchronization. The old code would flail about
		 * madly.
		 */

		if (tv.tv_sec == 0) {
			printf("synctodr: unable to read clock chip\n");
			dosynctodr = 0;
			goto bottom;
		}

		/*
		 * Sanity check: if the two clocks are out of sync by more
		 * than about 2000 seconds, complain and turn off sync;
		 * continuing would result in integer overflows resulting in
		 * synchronizing to the wrong time.
		 */

		deltat = tv.tv_sec - time.tv_sec;
		if (ABS(deltat) > 2000) {
			printf("synctodr: unable to sync\n");
			dosynctodr = 0;
			goto bottom;
		}

		/*
		 * Calculate how far off we are. If we are out by more than
		 * one clock tick, set up timedelta and tickdelta so that
		 * hardclock() will increment the software clock a bit fast
		 * or a bit slow. How fast or how slow is determined by
		 * tickadj, the number of microseconds of correction applied
		 * each tick.
		 */

		deltat = deltat * 1000000 + tv.tv_usec - time.tv_usec;

		if (ABS(deltat) > 1000000 / hz) {
			timedelta = deltat;
			tickdelta = tickadj;	/* standard slew rate */
			if (clkdrift)
				printf("<[%d]> ", deltat / 1000);
		}
		todsyncat = 0;
bottom:
		(void) splx(s);
	} else
		todsyncat = 0;
	timeout(synctodr, (caddr_t) 0, sch);
}

/*
 * Initialize the system time, based on the time base which is, e.g. from a
 * filesystem.  A base of -1 means the file system doesn't keep time.
 */
inittodr(base)
time_t base;
{
	register long deltat;
	int s;

#ifdef SAS
	time.tv_sec = base;
#else	/* !SAS */
	s = splclock();
	time = todget();
	(void) splx(s);
	if (time.tv_sec < SECYR) {
		if (base == -1)
			time.tv_sec = (87 - YRREF) * SECYR;	/* ~1987 */
		else
			time.tv_sec = base;
		printf("WARNING: TOD clock not initialized");
		resettodr();
		goto check;
	}
	if (base == -1)
		goto out;
	if (base < (87 - YRREF) * SECYR) {	/* ~1987 */
		printf("WARNING: preposterous time in file system");
		goto check;
	}
	deltat = time.tv_sec - base;
	/*
	 * See if we gained/lost two or more days; if so, assume something is
	 * amiss.
	 */
	if (deltat < 0)
		deltat = -deltat;
	if (deltat < 2 * SECDAY)
		goto out;
	printf("WARNING: clock %s %d days",
	       time.tv_sec < base ? "lost" : "gained", deltat / SECDAY);
check:
	printf(" -- CHECK AND RESET THE DATE!\n");
out:
	if (dosynctodr)
		timeout(synctodr, (caddr_t) 0, synctodrval * hz);
#endif	/* !SAS */
}

/*
 * For Sun-4, we use either the Intersil ICM7170 for both the real time clock
 * and the time-of-day device, or the Mostek MK48T02 for the time-of-day and
 * a separate counter timer service for the real time clock.
 */

static u_int monthsec[12] = {
	31 * SECDAY,		/* Jan */
	28 * SECDAY,		/* Feb */
	31 * SECDAY,		/* Mar */
	30 * SECDAY,		/* Apr */
	31 * SECDAY,		/* May */
	30 * SECDAY,		/* Jun */
	31 * SECDAY,		/* Jul */
	31 * SECDAY,		/* Aug */
	30 * SECDAY,		/* Sep */
	31 * SECDAY,		/* Oct */
	30 * SECDAY,		/* Nov */
	31 * SECDAY		/* Dec */
};

#define	MONTHSEC(mon, yr)	\
	(((((yr) % 4) == 0) && ((mon) == 2))? 29*SECDAY : monthsec[(mon) - 1])

/*
 * Set the TOD based on the argument value; used when the TOD has a
 * preposterous value and also when the time is reset by the settimeofday
 * system call. We run at splclock() to avoid synctodr() from running and
 * getting confused.
 */
resettodr()
{
	register int t;
	u_short hsec, sec, min, hour, day, mon, weekday, year;
	int s;

	s = splclock();

	t = time.tv_sec;
	/*
	 * Figure out the weekday
	 */
	weekday = (t / (60 * 60 * 24) + 2) % 7 + 1;	/* 1..7 */

	/*
	 * Figure out the (adjusted) year
	 */
	for (year = (YRREF - YRBASE); t > SECYEAR(year); year++)
		t -= SECYEAR(year);

	/*
	 * Figure out what month this is by subtracting off time per month,
	 * adjust for leap year if appropriate.
	 */
	for (mon = 1; t >= 0; mon++)
		t -= MONTHSEC(mon, year);

	mon--;			/* back off one month */
	t += MONTHSEC(mon, year);

	sec = t % 60;		/* seconds */
	t /= 60;
	min = t % 60;		/* minutes */
	t /= 60;
	hour = t % 24;		/* hours (24 hour format) */
	day = t / 24;		/* day of the month */
	day++;			/* adjust to start at 1 */

	hsec = time.tv_usec / 10000;

#ifndef SAS
	if (clock_type == INTERSIL7170) {
		CLOCK0->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_RUN);
		CLOCK0->clk_weekday = weekday % 7;
		CLOCK0->clk_year = year;
		CLOCK0->clk_mon = mon;
		CLOCK0->clk_day = day;
		CLOCK0->clk_hour = hour;
		CLOCK0->clk_min = min;
		CLOCK0->clk_sec = sec;
		CLOCK0->clk_hsec = hsec;
		CLOCK0->clk_cmd = CLK_CMD_NORMAL;
	} else {
		CLOCK1->clk_ctrl |= CLK_CTRL_WRITE;
		CLOCK1->clk_sec = btobcd(sec) & CLK_SEC_MASK;
		CLOCK1->clk_min = btobcd(min) & CLK_MIN_MASK;
		CLOCK1->clk_hour = btobcd(hour) & CLK_HOUR_MASK;
		CLOCK1->clk_weekday = btobcd(weekday) & CLK_WEEKDAY_MASK;
		CLOCK1->clk_day = btobcd(day) & CLK_DAY_MASK;
		CLOCK1->clk_month = btobcd(mon) & CLK_MONTH_MASK;
		CLOCK1->clk_year = btobcd(year);
		CLOCK1->clk_ctrl &= ~CLK_CTRL_WRITE;
	}
#endif	/* !SAS */

	timedelta = 0;		/* destroy any time delta */

	(void) splx(s);
}

/*
 * Read the current time from the clock chip and convert to UNIX form.
 * Assumes that the year in the counter chip is valid.
 */
struct timeval
todget()
{
	struct timeval tv;
	u_char now[CLK_WEEKDAY + 1];
	register int i, t = 0;
	register u_char *cp = (u_char *) CLOCK0_ADDR;
	u_short year;

#ifndef SAS
	if (clock_type == INTERSIL7170) {
		for (i = CLK_HSEC; i <= CLK_WEEKDAY; i++)	/* read counters */
			now[i] = *cp++;
	} else {
		CLOCK1->clk_ctrl |= CLK_CTRL_READ;
		now[CLK_HSEC] = 0;	/* XXX - should look at a counter */
		now[CLK_HOUR] = bcdtob(CLOCK1->clk_hour);
		now[CLK_MIN] = bcdtob(CLOCK1->clk_min);
		now[CLK_SEC] = bcdtob(CLOCK1->clk_sec);
		now[CLK_MON] = bcdtob(CLOCK1->clk_month);
		now[CLK_DAY] = bcdtob(CLOCK1->clk_day);
		now[CLK_YEAR] = bcdtob(CLOCK1->clk_year);
		now[CLK_WEEKDAY] = bcdtob(CLOCK1->clk_weekday);
		CLOCK1->clk_ctrl &= ~CLK_CTRL_READ;
	}
#endif	/* !SAS */

	if (now[CLK_MON] < 1 || now[CLK_MON] > 12 ||
	    now[CLK_DAY] < 1 || now[CLK_DAY] > 31 ||
	    now[CLK_WEEKDAY] > 7 || now[CLK_HOUR] > 23 ||
	    now[CLK_MIN] > 59 || now[CLK_SEC] > 59 ||
	    now[CLK_YEAR] < (YRREF - YRBASE)) {	/* not initialized */
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		return (tv);
	}
	/*
	 * Add the number of seconds for each year onto our time t. We start
	 * at YRREF - YRBASE (which is the chip's value for UNIX's YRREF
	 * year), and count up to the year value given by the chip, adding
	 * each years seconds value to the Unix time value we are
	 * calculating.
	 */
	for (year = YRREF - YRBASE; year < now[CLK_YEAR]; year++)
		t += SECYEAR(year);

	/*
	 * Now add in the seconds for each month that has gone by this year,
	 * adjusting for leap year if appropriate.
	 */
	for (i = 1; i < now[CLK_MON]; i++)
		t += MONTHSEC(i, year);

	t += (now[CLK_DAY] - 1) * SECDAY;
	t += now[CLK_HOUR] * (60 * 60);
	t += now[CLK_MIN] * 60;
	t += now[CLK_SEC];

	/*
	 * If t is negative, assume bogus time (year was too large) and use 0
	 * seconds. XXX - tv_sec and tv_usec should be unsigned.
	 */
	if (t < 0) {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	} else {
		tv.tv_sec = t;
		tv.tv_usec = now[CLK_HSEC] * 10000;
	}
	return (tv);
}
