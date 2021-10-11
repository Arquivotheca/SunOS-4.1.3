#ifndef lint
static char sccsid[] = "@(#)clock.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <machine/clock.h>
#include <machine/interreg.h>
#include <machine/cpu.h>

struct timeval todget();

/*
 * Defines for conversion between binary and bcd representation.  These are
 * necessary for the Mostek 48T02 TOD clock regs.  (Works only on 8 bit
 * unsigned values.)
 */
#define	bcdtob(x)	(((x) & 0xf) + 10 * ((x) >> 4))
#define	btobcd(x)	((((x) / 10) << 4) + ((x) % 10))

int clock_type = 0;

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
 * Start the real-time clock.
 */
startrtclock()
{

	/*
	 * We will set things up to interrupt every 1/100 of a second.
	 * locore.s currently only calls hardclock every other clock
	 * interrupt, thus assuming 50 hz operation.
	 */
	if (hz != 50)
		panic("startrtclock");

	/*
	 * Determine the clock type (intersil7170 vs mostek48t02). Attempt to
	 * read a register on the intersil 7170.
	 */
#ifndef SAS
#ifdef SUN3X_80
	/* XXX Hardware problem, 3X/80 can't tell if a INTERSIL7170 exists */
	if (cpu == CPU_SUN3X_80)
		clock_type = MOSTEK48T02;
	else
#endif	/* SUN3X_80 */
	if (peekc((char *) CLKADDR) != -1) {
		clock_type = INTERSIL7170;
		/* set 1/100 sec clock intr */
		CLKADDR->clk_intrreg = CLK_INT_HSEC;
	} else {
		/* machine using this clock will set 1/100 sec clock intr */
		clock_type = MOSTEK48T02;
	}
#endif	/* !SAS */

	set_clk_mode(IR_ENA_CLK5, 0);	/* turn on level 5 clock intr */
}

/*
 * Set and/or clear the desired clock bits in the interrupt register.  We
 * have to be extremely careful that we do it in such a manner that we don't
 * get ourselves lost.
 */
set_clk_mode(on, off)
u_char on, off;
{
	register u_char interreg, dummy;

	/*
	 * make sure that we are only playing w/ clock interrupt register
	 * bits
	 */
	on &= (IR_ENA_CLK7 | IR_ENA_CLK5);
	off &= (IR_ENA_CLK7 | IR_ENA_CLK5);

	/*
	 * Get a copy of current interrupt register, turning off any
	 * undesired bits (aka `off')
	 */
	interreg = *INTERREG & ~(off | IR_ENA_INT);
	*INTERREG &= ~IR_ENA_INT;

	/*
	 * Next we turns off the CLK5 and CLK7 bits to clear the flip-flops,
	 * then we disable clock interrupts. Now we can read the clock's
	 * interrupt register to clear any pending signals there.
	 */
	*INTERREG &= ~(IR_ENA_CLK7 | IR_ENA_CLK5);
#ifndef SAS
	if (clock_type == INTERSIL7170) {
		CLKADDR->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_INTRENA);
		dummy = CLKADDR->clk_intrreg;	/* clear clock */
	}
#endif	/* !SAS */
#ifdef lint
	dummy = dummy;
#endif

	/*
	 * Now we set all the desired bits in the interrupt register, then we
	 * turn the clock back on and finally we can enable all interrupts.
	 */
	*INTERREG |= (interreg | on);	/* enable flip-flops */
#ifndef SAS
	if (clock_type == INTERSIL7170) {
		CLKADDR->clk_cmd = CLK_CMD_NORMAL;	/* enable clock intr */
	}
#endif	/* !SAS */
	*INTERREG |= IR_ENA_INT;	/* enable interrupts */
}

int dosynctodr = 1;		/* if true, sync UNIX time to TOD */
int clkdrift = 0;		/* if true, show UNIX & TOD sync differences */
int synctodrval = 30;		/* number of seconds between synctodr */
extern long timedelta;
extern int tickadj;
extern int tickdelta;
extern int doresettodr;

/* if true, continue to check time, but do not correct drift */
/*
 * Commented out since it's not used in 3x, and lint complains. int
 * allowdrift	= 0;
 */

/* if true, correct large jumps by resetting tod clock */
int fixtodjump = 1;
/* if true, report all tod errors */
int toddebug = 0;

#define	ABS(x) 		((x) < 0? -(x) : (x))

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
		 * this run, since the overhead is nearly zero, so we just go
		 * ahead and do it everywhere.
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
	}
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
}

/*
 * For Sun-3, we use the Intersil ICM7170 for both the real time clock and
 * the time-of-day device.
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
 * system call.  We run at splclock() to avoid synctodr() from running and
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
		CLKADDR->clk_cmd = (CLK_CMD_NORMAL & ~CLK_CMD_RUN);
		CLKADDR->clk_weekday = weekday % 7;	/* 0..6 */
		CLKADDR->clk_year = year;
		CLKADDR->clk_mon = mon;
		CLKADDR->clk_day = day;
		CLKADDR->clk_hour = hour;
		CLKADDR->clk_min = min;
		CLKADDR->clk_sec = sec;
		CLKADDR->clk_hsec = hsec;
		CLKADDR->clk_cmd = CLK_CMD_NORMAL;
	} else if (clock_type == MOSTEK48T02) {
		CLK1ADDR->clk_ctrl |= CLK_CTRL_WRITE;
		CLK1ADDR->clk_sec = btobcd(sec) & CLK_SEC_MASK;
		CLK1ADDR->clk_min = btobcd(min) & CLK_MIN_MASK;
		CLK1ADDR->clk_hour = btobcd(hour) & CLK_HOUR_MASK;
		CLK1ADDR->clk_weekday = btobcd(weekday) & CLK_WEEKDAY_MASK;
		CLK1ADDR->clk_day = btobcd(day) & CLK_DAY_MASK;
		CLK1ADDR->clk_month = btobcd(mon) & CLK_MONTH_MASK;
		CLK1ADDR->clk_year = btobcd(year);
		CLK1ADDR->clk_ctrl &= ~CLK_CTRL_WRITE;
	}
#endif	/* !SAS */
	timedelta = 0;		/* destroy any time delta */
	tickdelta = tickadj;	/* restore standard slew rate */

	if (toddebug) {
		printf("CLKSET (yr.mo.da.hr.mi.se.hs): %d.%d.%d.%d.%d.%d.%d\n",
		    year, mon, day, hour, min, sec, hsec);
		(void) chkdeltat(0);
	}
	(void) splx(s);
}

static int jumpdeltat = 2147;
static u_char now[CLK_WEEKDAY + 1];

#define	TPRINTF if (toddebug) printf
#define	DUMPTIME(x) TPRINTF(\
"Clock Values (yr.mo.da.hr.mi.se.hs): %d.%d.%d.%d.%d.%d.%d\n", \
(x)[CLK_YEAR], (x)[CLK_MON], (x)[CLK_DAY], (x)[CLK_HOUR], \
(x)[CLK_MIN], (x)[CLK_SEC], (x)[CLK_HSEC]);
static char *glitchstr = "WARNING: Time-of-Day Chip glitch-\n\
Resetting to internal tick time.\n\
Please check System date";

static
chkdeltat(level)
{
	struct timeval todget();
	auto struct timeval tv;
	register fail = 0;

	tv = todget();

	if (!tv.tv_sec && !tv.tv_usec) {
		fail = 1;	/* major failure 1 */
		TPRINTF("todget rejects read of tod ");
	} else if (ABS(tv.tv_sec - time.tv_sec) > jumpdeltat) {
		fail = 1;
		TPRINTF("Large Magnitude time change (cutoff %d): %d sec\n",
			jumpdeltat, tv.tv_sec - time.tv_sec);
	} else if (tv.tv_sec < time.tv_sec || (tv.tv_sec == time.tv_sec &&
				tv.tv_usec < (time.tv_usec) - (2 * tick))) {
		fail = 1;
		TPRINTF("Time is going backwards: new %d.%d old %d.%d\n",
			tv.tv_sec, tv.tv_usec, time.tv_sec, time.tv_usec);
	}
	if (level > 0) {
		if (fail) {
			TPRINTF("Second Read of TOD Fails\n");
			DUMPTIME(now);
			if (fixtodjump) {
				/*
				 * Supposedly, this will only happen if the
				 * chip read really fails. This was certainly
				 * the case for 3/50s for a while. By
				 * extension, 3/260s should have the same
				 * problem since they had the same circuit as
				 * the 3/50. I don't know about 3/60s, 3/110s
				 * and 3/160s.
				 *
				 * However, then when 4.0 alpha-3 came out,
				 * people reported seeing this message on a
				 * variety of systems. However, it didn't
				 * happen very often, and, as usual, they
				 * just complained about it, but weren't very
				 * helpful about pointing out which systems
				 * it happened on or cooperated about trying
				 * to nail it.
				 *
				 * So, I was in a quandary about to do do. I
				 * looked over the code again, think I
				 * understood what had changed since I first
				 * put this change in (3.4.1), and didn't see
				 * how this could happen, except when:
				 *
				 * 1) time was spent in the monitor or kadb
				 *
				 * 2) the chip actually *did* glitch.
				 *
				 * 3) > 60 seconds (jumpdeltat was that that
				 * value then) was spent in rewinding a
				 * sysgen cartridge tape during boot time
				 * when the scsi bus was being probed (known
				 * to be possible during installation).
				 *
				 * In order to solve this problem, I decided
				 * that
				 *
				 * a) jumpdeltat could be as close to the sign
				 * extension limit for the u-sec valued
				 * deltat. This value is approx. 2147
				 * seconds, or nearly 35 minutes.
				 *
				 *
				 * b)  I would turn off the warning message. If
				 * problems arise, it can be turned back on
				 * by an adb patch...
				 *
				 * matt jacob 9/20/87
				 */
				TPRINTF(glitchstr);
				resettodr();
			}
			return (0);
		} else {
			TPRINTF("Second READ of TOD now shows sanity\n");
		}
	} else if (fail) {
		TPRINTF("First Read of TOD chip shows insanity\n");
		DUMPTIME(now);
		return (chkdeltat(++level));
	}
	return (1000000 * (tv.tv_sec - time.tv_sec) +
		(tv.tv_usec - time.tv_usec));
}

static
readtod(np)
u_char np[];
{
	register i;
	register u_char *cp = (u_char *) CLKADDR;

#ifndef SAS
	if (clock_type == INTERSIL7170) {
		for (i = CLK_HSEC; i <= CLK_WEEKDAY + 1; i++)
			np[i] = *cp++;
	} else if (clock_type == MOSTEK48T02) {
		CLK1ADDR->clk_ctrl |= CLK_CTRL_READ;
		now[CLK_HSEC] = 0;	/* XXX perhaps look at a counter ? */
		now[CLK_HOUR] = bcdtob(CLK1ADDR->clk_hour);
		now[CLK_MIN] = bcdtob(CLK1ADDR->clk_min);
		now[CLK_SEC] = bcdtob(CLK1ADDR->clk_sec);
		now[CLK_MON] = bcdtob(CLK1ADDR->clk_month);
		now[CLK_DAY] = bcdtob(CLK1ADDR->clk_day);
		now[CLK_YEAR] = bcdtob(CLK1ADDR->clk_year);
		now[CLK_WEEKDAY] = bcdtob(CLK1ADDR->clk_weekday);
		CLK1ADDR->clk_ctrl &= ~CLK_CTRL_READ;
	}
#endif	/* !SAS */
}

/*
 * Read the current time from the clock chip and convert to UNIX form.
 * Assumes that the year in the counter chip is valid.
 */
struct timeval
todget()
{
	struct timeval tv;
	register int i, t = 0;
	u_short year;

	readtod(now);

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
