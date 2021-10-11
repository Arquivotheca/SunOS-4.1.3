/*      @(#)clock.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Definitions for the Intersil 7170 real-time clock.  This chip
 * is used as the timer chip in addition to being the battery
 * backed up time-of-day device.  This clock is run by UNIX in
 * the 100 hz periodic mode giving interrupts 100 times/second.
 * The low level code dismisses every other interrupt, thus
 * creating an effective 50 hz rate for hardclock().
 *
 * Reading clk_hsec latches the the time in all the other bytes
 * so you get a consistent value.  To see any byte change, you
 * have to read clk_hsec in between (e.g. you can't loop waiting
 * for clk_sec to reach a certain value without reading clk_hsec
 * each time).
 */

#ifndef _sun3_clock_h
#define _sun3_clock_h

#define	SECDAY	((unsigned)(24*60*60))		/* seconds per day */
#define	SECYR	((unsigned)(365*SECDAY))	/* seconds per common year */

/*
 * The 7170 uses year % 4 to figure out if
 * we have a leap year, we do the same here.
 */
#define	SECYEAR(yr)	((((unsigned)(yr) % 4) == 0)? SECYR + SECDAY : SECYR)

/*
 * The year register counts from 0 to 99.
 * Unix time is the number of seconds
 * since the year YRREF.  The 2 digit year
 * value stored in the chip represents the 
 * the number of years beyond YRBASE.
 * Note that YRBASE must be < YRREF and
 * (YRBASE % 4) == 0 to do leap years correct.
 * Note that we can only keep time up to the year 2068.
 */
#define	YRREF		70	/* 1970 - where UNIX time begins */
#define	YRBASE		68	/* 1968 - what year 0 in chip represents */

#define	OBIO_CLKADDR	0x60000	/* address of clock in obio space */

#ifdef LOCORE
#define	CLKADDR 0x0FFE2000	/* virtual address we map clock to be at */
#else
struct intersil7170 {
	u_char	clk_hsec;	/* counter - hundredths of seconds 0-99 */
	u_char	clk_hour;	/* counter - hours 0-23 (24hr) 1-12 (12hr) */
	u_char	clk_min;	/* counter - minutes 0-59 */
	u_char	clk_sec;	/* counter - seconds 0-59 */
	u_char	clk_mon;	/* counter - month 1-12 */
	u_char	clk_day;	/* counter - day 1-31 */
	u_char	clk_year;	/* counter - year 0-99 */
	u_char	clk_weekday;	/* counter - week day 0-6 */
	u_char	clk_rhsec;	/* RAM - hundredths of seconds 0-99 */
	u_char	clk_rhour;	/* RAM - hours 0-23 (24hr) 1-12 (12hr) */
	u_char	clk_rmin;	/* RAM - minutes 0-59 */
	u_char	clk_rsec;	/* RAM - seconds 0-59 */
	u_char	clk_rmon;	/* RAM - month 1-12 */
	u_char	clk_rday;	/* RAM - day 1-31 */
	u_char	clk_ryear;	/* RAM - year 0-99 */
	u_char	clk_rweekday;	/* RAM - week day 0-6 */
	u_char	clk_intrreg;	/* interrupt status and mask register */
	u_char	clk_cmd;	/* command register */
	u_char	clk_unused[14];
};
#define	CLKADDR ((struct intersil7170 *)(0x0FFE2000))
#endif

/* offsets into structure */
#define	CLK_HSEC	0
#define	CLK_HOUR	1
#define	CLK_MIN		2
#define	CLK_SEC		3
#define	CLK_MON		4
#define	CLK_DAY		5
#define	CLK_YEAR	6
#define	CLK_WEEKDAY	7
#define	CLK_RHSEC	8
#define	CLK_RHOUR	9
#define	CLK_RMIN	10
#define	CLK_RSEC	11
#define	CLK_RMON	12
#define	CLK_RDAY	13
#define	CLK_RYEAR	14
#define	CLK_RWEEKDAY	15
#define	CLK_INTRREG	16
#define	CLK_CMD		17

/*
 * In `alarm' mode the 7170 interrupts when the current
 * counter matches the RAM values.  However, if the ignore
 * bit is on in the RAM counter, that register is not
 * used in the comparision.  Unfortunately, the clk_rhour
 * register uses a different mask bit (because of 12 hour
 * mode) and thus the 2 different defines.
 */
#define	CLK_IGNORE	0x80	/* rmsec, rmin, rsec, rmon, rday, ryear, rdow */
#define	CLK_HOUR_IGNORE	0x40	/* ignore bit for clk_rhour only */

/*
 * Interrupt status and mask register defines,
 * reading this register tells what caused an interrupt
 * and then clears the state.  These can occur
 * concurrently including te RAM compare interrupts.
 */
#define	CLK_INT_INTR	0x80	/* r/o pending interrrupt */
#define	CLK_INT_DAY	0x40	/* r/w periodic day interrupt */
#define	CLK_INT_HOUR	0x20	/* r/w periodic hour interrupt */
#define	CLK_INT_MIN	0x10	/* r/w periodic minute interrupt */
#define	CLK_INT_SEC	0x08	/* r/w periodic second interrupt */
#define	CLK_INT_TSEC	0x04	/* r/w periodic 1/10 second interrupt */
#define	CLK_INT_HSEC	0x02	/* r/w periodic 1/100 second interrupt */
#define	CLK_INT_ALARM	0x01	/* r/w alarm mode - interrupt on time match */

/* Command register defines */
#define	CLK_CMD_TEST	0x20	/* w/o test mode (vs. normal mode) */
#define	CLK_CMD_INTRENA	0x10	/* w/o interrupt enable (vs. disabled) */
#define	CLK_CMD_RUN	0x08	/* w/o run bit (vs. stop) */
#define	CLK_CMD_24FMT	0x04	/* w/o 24 hour format (vs. 12 hour format) */
#define	CLK_CMD_F4M	0x03	/* w/o using 4.194304MHz crystal frequency */
#define	CLK_CMD_F2M	0x02	/* w/o using 2.097152MHz crystal frequency */
#define	CLK_CMD_F1M	0x01	/* w/o using 1.048576MHz crystal frequency */
#define	CLK_CMD_F32K	0x00	/* w/o using  32.768KHz  crystal frequency */

#define	CLK_CMD_NORMAL	(CLK_CMD_INTRENA|CLK_CMD_RUN|CLK_CMD_24FMT|CLK_CMD_F32K)

#endif /*!_sun3_clock_h*/
