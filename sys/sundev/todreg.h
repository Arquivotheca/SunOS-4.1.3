/*	@(#)todreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Definitions for National Semi MM58167 Time-Of-Year Clock
 * This brain damaged chip insists on keeping the time in
 * MM/DD HH:MM:SS format, even though it doesn't know about
 * leap years and Feb. 29, thus making it nearly worthless.
 * We just treat it as a series of counters which trigger at funny
 * times.  Since the chip thinks all years are the same, we consider
 * all "years" to have SECDAY seconds.  We always load the chip with
 * the UNIX time modulo SECDAY.
 *
 * All counters are in BCD.
 */

#ifndef _sundev_todreg_h
#define _sundev_todreg_h

#define	SECDAY		(24*60*60)	/* seconds per day */
#define	SECYR		(365*SECDAY)	/* per common year */

struct toddevice {
	struct {		/* counters */
		u_char	val;
		u_char	: 8;
	} tod_counter[8];
	struct {		/* compare latches - not used */
		u_char	val;
		u_char	: 8;
	} tod_latch[8];
	u_char	tod_isr;	/* interrupt status - not used */
	u_char	: 8;
	u_char	tod_icr;	/* interrupt control - not used */
	u_char	: 8;
	u_char	tod_creset;	/* counter reset mask */
	u_char	: 8;
	u_char	tod_lreset;	/* latch reset mask */
	u_char	: 8;
	u_char	tod_status;	/* bad counter read status */
	u_char	: 8;
	u_char	tod_go;		/* GO - start at integral seconds */
	u_char	: 8;
	u_char	tod_stby;	/* standby mode - not used */
	u_char	: 8;
	u_char	tod_test;	/* test mode - ??? */
	u_char	: 8;
};

/* counters and latches */
#define	TOD_MSEC	0	/* milliseconds * 10 */
#define	TOD_TSEC	1	/* hundredths of seconds */
#define	TOD_SEC		2	/* seconds 0-59 */
#define	TOD_MIN		3	/* minutes 0-59 */
#define	TOD_HOUR	4	/* hours 0-23 */
#define	TOD_WEEKDAY	5	/* weeekday 1-7 */
#define	TOD_DAY		6	/* day-of-month 1-{28,30,31} */
#define	TOD_MONTH	7	/* month 1-12 */

#endif /*!_sundev_todreg_h*/
