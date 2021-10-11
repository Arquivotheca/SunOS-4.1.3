/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)ns_maint.c 1.1 92/07/30 SMI"; /* from UCB 4.23 2/28/88 */
#endif /* not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#if defined(SYSV)
#include <unistd.h>
#endif SYSV
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <arpa/nameser.h>
#include <vfork.h>
#include "ns.h"
#include "db.h"

extern int errno;
extern int maint_interval;

int xfer_running_cnt = 0;	       /* number of xfers running */
int xfer_deferred_cnt = 0;	       /* number of needed xfers not run yet */
  
/*
 * Timer code overview:
 *  The basic idea of the timer code is to provide 2 logical timers using the
 *  ITIMER_REAL timer signal.  The two logical timers are TIME_NS_MAINT and
 *  TIME_XFER.  TIME_NS_MAINT is allowed to be set, while running, to a new
 *  value.  TIME_XFER ignores multiple sets while it is on (this is because
 *  the we want the minimum xfer process running time).
 *
 *  A setitimer interval timer is used as the 'physical' timer.  It is
 *  set to interrupt at the minimum of the two logical timer''s time.
 *  This means that the timer code has to shuffle physical and logical timers
 *  around -- yuck.
 *
 *  Alarm blocking functions (BLOCK_TIMER & UNBLOCK_TIMER) are used to stop
 *  alarms while in critical timer manipulation sections
 *
 *  The functions are:
 *	handle_timer() - an interrupt handler
 *	timer_on()     - sets a timer
 *	timer_off()    - turns off a timer
 */

static struct timeval
	time_xfer = {0,0},	/* the xfer alarm time or 0 */
	time_ns_maint = {0,0};	/* the ns_maint alarm time or 0 */
static int timing = 0;		/* what is being physically timed:
				   one of: TIME_NONE, TIME_XFER or,
				   TIME_NS_MAINT */

/*
 * ignores useconds
 */
#define ZERO_TIMEP(t)      ((t).tv_sec == 0)
#define ZERO_TIME(t)       ((t).tv_sec = 0)
#define CMP_TIME(t1,OP,t2) ((t1).tv_sec OP (t2).tv_sec)

#define BLOCK_TIMER(omask)	(omask = sigblock(sigmask(SIGALRM)))
#define UNBLOCK_TIMER(omask)	((void) sigsetmask(omask & ~sigmask(SIGALRM)))


/*
 * Returns which timers (TIME_XFER or TIME_NS_MAINT (or 0 for neither))
 * caused the interrupt.  Also schedules a new interrupt if the other timer
 * is running.
 */ 
int handle_timer()
{   
    struct itimerval ival;
    struct timeval tt;

    switch (timing) {
      case TIME_NONE:		/* no timer is running - a bug somewhere */
	return 0;

      case TIME_XFER:		/* the xfer timer was the physical timer */
	ZERO_TIME(time_xfer);
	if (!ZERO_TIMEP(time_ns_maint)) {
	    /* switch to the ns_maint logical timer */
	    bzero(&ival, sizeof(ival));
	    gettime(&tt);
	    if (ival.it_value.tv_sec = time_ns_maint.tv_sec - tt.tv_sec) {
	        (void) setitimer(ITIMER_REAL, &ival, (struct itimerval *)NULL);
	        timing = TIME_NS_MAINT;
	    } else {
		timing = TIME_NONE;
		return TIME_XFER|TIME_NS_MAINT; 	/* both timed out */
	    }
	} else
	    timing = TIME_NONE;
#ifdef DEBUG
	if (debug > 5)
	  fprintf(ddt, "Handle timer: timing %d, xfer %d, ns_maint %d\n",
		  timing, time_xfer.tv_sec, time_ns_maint.tv_sec);
#endif
	return TIME_XFER;

      case TIME_NS_MAINT:	/* the ns_maint timer was the physical timer */
	ZERO_TIME(time_ns_maint);
	if (!ZERO_TIMEP(time_xfer)) {
	    /* switch to the ns_xfer logical timer */
	    bzero(&ival, sizeof(ival));
	    gettime(&tt);
    	    if (ival.it_value.tv_sec = time_xfer.tv_sec - tt.tv_sec) {
		(void) setitimer(ITIMER_REAL, &ival, (struct itimerval *)NULL);
		timing = TIME_XFER;
	    } else {
		timing = TIME_NONE;
		return TIME_XFER|TIME_NS_MAINT;	 /* both timed out */
	    }
	} else
	    timing = TIME_NONE;
	return TIME_NS_MAINT;

      default:
#ifdef DEBUG
	if (debug) fprintf(ddt, "Bad TIMING==%d\n", timing);
#endif
	return 0;
    }
}

void timer_on(delta, func)
    int delta,			/* delta time to ring */
        func;			/* TIME_XFER or TIME_NS_MAINT */
{
    struct itimerval ival;
    struct timeval tt;
    int osigmask;
    
#ifdef DEBUG
    if (debug > 5)
      fprintf(ddt, "Calling timer_on(%d,%d)\n", delta, func);
#endif
    BLOCK_TIMER(osigmask);
    if (delta == 0) goto exit_timer_on; /* no time- ignore */
    bzero(&ival, sizeof(ival));
    gettime(&tt);

    switch (func) {
      case TIME_XFER:
	if (!ZERO_TIMEP(time_xfer))
	    goto exit_timer_on; /* ignore updates to xfer timings*/
	time_xfer.tv_sec = delta + tt.tv_sec;
	if (timing == TIME_NONE) {
	    ival.it_value.tv_sec = delta;
	    timing = TIME_XFER;
	    goto settime;
	}
	break;
      case TIME_NS_MAINT:
	time_ns_maint.tv_sec = delta + tt.tv_sec;
	if (timing == TIME_NONE) {
	    ival.it_value.tv_sec = delta;
	    timing = TIME_NS_MAINT;
	    goto settime;
	}
	break;
      default: abort(1);    
    }
    if (CMP_TIME(time_ns_maint, >, time_xfer) && !ZERO_TIMEP(time_xfer)) {
	ival.it_value.tv_sec = time_xfer.tv_sec - tt.tv_sec;
	timing = TIME_XFER;
    } else {
	ival.it_value.tv_sec = time_ns_maint.tv_sec - tt.tv_sec;
	timing = TIME_NS_MAINT;
    }

settime:
    (void) setitimer(ITIMER_REAL, &ival, (struct itimerval *)NULL);

exit_timer_on:
#ifdef DEBUG
    if (debug > 5)
      fprintf(ddt, "timing %d, xfer %d, ns_maint %d\n", timing,
	      time_xfer.tv_sec, time_ns_maint.tv_sec);
#endif
    UNBLOCK_TIMER(osigmask);
}

/*
 * Turn off timing
 */
void timer_off(func)
    int func;
{
    struct itimerval ival;
    struct timeval tt;
    int osigmask;
#ifdef DEBUG
    if (debug > 5)
        fprintf(ddt, "Calling timer_off(%d)\n", func);
#endif


    BLOCK_TIMER(osigmask);
    if (timing != TIME_NONE) {
	if (timing == func) {
	    bzero(&ival, sizeof(ival));
	    if (func == TIME_XFER) {
		if (!ZERO_TIMEP(time_ns_maint)) {
		    gettime(&tt);
		    timing = TIME_NS_MAINT;
		    ival.it_value.tv_sec = time_ns_maint.tv_sec - tt.tv_sec;
		} else
		    timing = TIME_NONE;
		ZERO_TIME(time_xfer);
	    } else {
		if (!ZERO_TIMEP(time_xfer)) {
		    gettime(&tt);
		    timing = TIME_XFER;
		    ival.it_value.tv_sec = time_xfer.tv_sec - tt.tv_sec;
		} else
		    timing = TIME_NONE;
		ZERO_TIME(time_ns_maint);
	    }
	    (void) setitimer(ITIMER_REAL, &ival, (struct itimerval *)NULL);
	} else	       	/* i am not the thing being timed so erase me */
	  if (func == TIME_XFER)
		ZERO_TIME(time_xfer);
	  else
		ZERO_TIME(time_ns_maint);
    }
    UNBLOCK_TIMER(osigmask);
#ifdef DEBUG
    if (debug > 5)
        fprintf(ddt, "Timer %d turned off, timing %d, xfer %d, ns_maint %d\n",
		func, timing, time_xfer.tv_sec, time_ns_maint.tv_sec);
#endif
}

/*
 * values of 'why':
 * 1 == update due to timer,
 * 2 == update due to SIGCHLD (child finished)
 */
ns_xfer_update(why)
int why;
{
    register struct zoneinfo *zp;
    int old_deferred_cnt;

#ifdef DEBUG
    if (debug > 5)
        fprintf(ddt, "ns_xfer_update(%s)\n",
	        why == 1? "alarm" : (why == 2? "child" : "PANIC"));
#endif /* DEBUG */
    if (why == 1 && xfer_running_cnt > 0) {
	gettime(&tt);
	for (zp = zones; zp < &zones[nzones]; zp++)
	    if (zp->xfer_pid)
	      if (check_runtime(zp, &tt))
		--xfer_running_cnt;
    }
    /*
     * Now check to see if any xfers have been deferred because too
     * many were running.  Start only enough deferred xfers to reach
     * the run-count limit.
     * Note that even if no xfers are running, then can have deferred (???)
     */
    if (xfer_running_cnt < MAX_XFERS_RUNNING && xfer_deferred_cnt > 0) {
	for (zp = zones, old_deferred_cnt = xfer_deferred_cnt
	     ; zp < &zones[nzones]; zp++)
	    if (zp->xfer_start_time == -1) {
		xfer_deferred_cnt--;
#ifdef DEBUG
		if (debug > 1)
		  fprintf(ddt, "Undeferring %s\n", zp->z_origin);
#endif /* DEBUG */
		zoneref(zp);
		zp->z_time = tt.tv_sec + zp->z_refresh;
		if (xfer_running_cnt >= MAX_XFERS_RUNNING) break;
	    }
	/*
	 * Check for a bug- hung deferreds
	 */
	if (old_deferred_cnt == xfer_deferred_cnt) {
	    xfer_deferred_cnt = 0;
#ifdef DEBUG
	    if (debug)
	      fprintf(ddt, "BUG- Hung deferred count %d zeroed\n",
		      old_deferred_cnt);
#endif /* DEBUG */
	}
    }
#ifdef DEBUG
    if (debug > 5)
        fprintf(ddt, "Run %d, Def %d\n", xfer_running_cnt, xfer_deferred_cnt);
#endif
    if (why == 2 && xfer_running_cnt == 0) timer_off(TIME_XFER);
}

/*
 * Invoked at regular intervals by signal interrupt; refresh all secondary
 * zones from primary name server and remove old cache entries.  Also,
 * ifdef'd ALLOW_UPDATES, dump database if it has changed since last
 * dump/bootup.
 */
ns_maint()
{
	register struct zoneinfo *zp;
	struct itimerval ival;
	time_t next_refresh = 0;
	int zonenum;

#ifdef DEBUG
	if (debug)
		fprintf(ddt,"ns_maint()\n");
#endif

	for (zp = zones, zonenum = 0; zp < &zones[nzones]; zp++, zonenum++) {
		switch(zp->z_type) {
#ifdef ALLOW_UPDATES
		case Z_PRIMARY:
#endif ALLOW_UPDATES
		case Z_SECONDARY:
		case Z_CACHE:
			break;

		default:
			continue;
		}
		gettime(&tt);
#ifdef DEBUG
		if (debug >= 2)
			printzoneinfo(zonenum);
#endif
		if (tt.tv_sec >= zp->z_time && zp->z_refresh > 0) {
			if (zp->z_type == Z_CACHE)
				doachkpt();
			if (zp->z_type == Z_SECONDARY)
				zoneref(zp);
#ifdef ALLOW_UPDATES
			/*
			 * Checkpoint the zone if it has changed
			 * since we last checkpointed
			 */
			if (zp->z_type == Z_PRIMARY && zp->hasChanged)
				zonedump(zp);
#endif ALLOW_UPDATES
			zp->z_time = tt.tv_sec + zp->z_refresh;
		}

		/*
		 * Find when the next refresh needs to be and set
		 * interrupt time accordingly.
		 */
		if (next_refresh == 0 ||
		    (zp->z_time != 0 && next_refresh > zp->z_time))
			next_refresh = zp->z_time;
	}

        /*
	 *  Schedule the next call to this function.
	 *  Don't visit any sooner than maint_interval.
	 */
	bzero((char *)&ival, sizeof (ival));
	ival.it_value.tv_sec = next_refresh - tt.tv_sec;
	if (ival.it_value.tv_sec < maint_interval)
		ival.it_value.tv_sec = maint_interval;
	timer_on((int)ival.it_value.tv_sec, TIME_NS_MAINT);
#ifdef DEBUG
	if (debug)
		fprintf(ddt,"exit ns_maint() Next interrupt in %d sec\n",
			ival.it_value.tv_sec);
#endif
}

zoneref(zp)
	struct zoneinfo *zp;
{
	static char *argv[NSMAX + 14], argv_ns[NSMAX][MAXDNAME];
	int cnt, argc = 0, argc_ns = 0;
	char serial_str[16];
	char debug_str[10];

#ifdef DEBUG
	if (debug)
	  fprintf(ddt,"zoneref() %s\n", zp->z_origin);
#endif
	/*
	 * if the xfer_pid is not 0, then an xfer is already running for this
	 * domain- so do not doing anything.
	 */
	if (zp->xfer_pid) {
	    gettime(&tt);
	    if (!check_runtime(zp, &tt)) {
#ifdef DEBUG
	        if (debug)
	          fprintf(ddt,"xfer already running for %s\n", zp->z_origin);
#endif
	    }
	    return;
	}
	/*
	 * see if the 'load' is low enough to allow more (new) xfers to run.
	 * if not, defer this xfer for later.
	 */
	if (xfer_running_cnt >= MAX_XFERS_RUNNING) {
	    xfer_deferred_cnt++;

	    zp->xfer_start_time = -1; 	/* mark this as deferred */
#ifdef DEBUG
	    if (debug > 1)
	      fprintf(ddt,"xfer deferred for %s\n", zp->z_origin);
#endif
	    return;
	}
	    
	argv[argc++] = "xfer";
	argv[argc++] = "-z";
	argv[argc++] = zp->z_origin;
	argv[argc++] = "-f";
	argv[argc++] = zp->z_source;
	if (zp->z_auth && zp->z_soa_refresh != 0) {
		argv[argc++] = "-s";
		(void)sprintf(serial_str, "%lu", zp->z_serial);
		argv[argc++] = serial_str;
	}
#ifdef DEBUG
	if (debug) {
	    argv[argc++] = "-d";
	    sprintf(debug_str, "%d", debug);
	    argv[argc++] = debug_str;
	    argv[argc++] = "-l";
	    argv[argc++] = "/usr/tmp/xfer.ddt";
	    if (debug > 5) {
		argv[argc++] = "-t";
		argv[argc++] = "/usr/tmp/xfer.trace";
	    }
	}
#endif
	
	/*
	 * Copy the server ip addresses into argv, after converting
	 * to ascii and saving the static inet_ntoa result
	 */
	for (cnt = 0; cnt < zp->z_addrcnt; cnt++)
	    argv[argc++] = strcpy(argv_ns[argc_ns++],
				  inet_ntoa(zp->z_addr[cnt]));

	argv[argc] = 0;

#ifdef DEBUG
#ifdef ECHOARGS
	if (debug) {
	    int i;
	    for (i = 0; i < argc; i++) 
	      fprintf(ddt, "Arg %d=%s\n", i, argv[i]);
        }
#endif /* ECHOARGS */
#endif /* DEBUG */

	if (zp->xfer_pid = vfork())
	{
	    struct timeval tt;

	    if (zp->xfer_pid == (u_short)-1) {
		/*
		 * Die a horrible death since could not fork.  Maybe
		 * should not die
		 */
#ifdef DEBUG
		if (debug) fprintf(ddt, "panic xfer [v]fork: %d\n", errno);
#endif
		syslog(LOG_ERR, "panic xfer [v]fork: %m");
		exit(-1);
	    }
#ifdef DEBUG
	    if (debug) fprintf(ddt, "started xfer child %d\n", zp->xfer_pid);
#endif
	    xfer_running_cnt++;
	    gettime(&tt);
	    zp->xfer_start_time = tt.tv_sec;
	    /*
	     * schedule a run time check, though this will be done
	     * in main() anyway if needed
	     */
	    timer_on(MAX_XFER_TIME, TIME_XFER);

	    return;
	}
	execve(XFER_BINARY, argv, NULL);
	syslog(LOG_ERR, "xfer start failed! %m");
	_exit(-1);		/* avoid duplicate buffer flushes */
}

#ifdef unused
/*
 * Recursively delete all domains (except for root SOA records),
 * starting from head of list pointed to by np.
 */
static DelDom(fnp, isroot)
	struct namebuf *fnp;
	int isroot;
{
	register struct databuf *dp, *pdp = NULL;
	register struct namebuf *np = fnp;
	struct namebuf **npp, **nppend;

#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt, "DelDom('%s', %d)\n", fnp->n_dname, isroot);
#endif DEBUG

	/* first do data records */
	for (dp = np->n_data; dp != NULL; ) {
		/* skip the root SOA record (marks end of data) */
		if (isroot && dp->d_type == T_SOA) {
			pdp = dp;
			dp = dp->d_next;
			continue;
                }
		dp = rm_datum(dp, np, pdp);
	}

	/* next do subdomains */
        if (np->n_hash == NULL)
                return;
	npp = np->n_hash->h_tab;
	nppend = npp + np->n_hash->h_size;
	while (npp < nppend) {
		for (np = *npp++; np != NULL; np = np->n_next) {
			DelDom(np, 0);
		}
	}
}
#endif unused

#ifdef DEBUG
printzoneinfo(zonenum)
int zonenum;
{
	struct timeval  tt;
	struct zoneinfo *zp = &zones[zonenum];
	char *ZoneType;

	if (!debug)
		return; /* Else fprintf to ddt will bomb */
	fprintf(ddt, "printzoneinfo(%d):\n", zonenum);

	gettime(&tt);
	switch (zp->z_type) {
		case Z_PRIMARY: ZoneType = "Primary"; break;
		case Z_SECONDARY: ZoneType = "Secondary"; break;
		case Z_CACHE: ZoneType = "Cache"; break;
		default: ZoneType = "Unknown";
	}
	if (zp->z_origin[0] == '\0')
		fprintf(ddt,"origin ='.'");
	else
		fprintf(ddt,"origin ='%s'", zp->z_origin);
	fprintf(ddt,", type = %s", ZoneType);
	fprintf(ddt,", source = %s\n", zp->z_source);
	fprintf(ddt,"z_refresh = %ld", zp->z_refresh);
	fprintf(ddt,", retry = %ld", zp->z_retry);
	fprintf(ddt,", expire = %ld", zp->z_expire);
	fprintf(ddt,", minimum = %ld", zp->z_minimum);
	fprintf(ddt,", serial = %ld\n", zp->z_serial);
	fprintf(ddt,"z_time = %d", zp->z_time);
	fprintf(ddt,", now time : %d sec", tt.tv_sec);
	fprintf(ddt,", time left: %d sec\n", zp->z_time - tt.tv_sec);
}
#endif DEBUG

/*
 * New code added by Rich Wales (UCLA), October 1986:
 *
 * The following routines manipulate the d_mark field.  When a zone
 * is being refreshed, the old RR's are marked.  This allows old RR's to
 * be cleaned up after the new copy of the zone has been completely read
 * -- or new RR's to be cleaned up if an error prevents transfer of a
 * new zone copy.
 *
 */

/*
 *     Set the "d_mark" field to on each RR in the zone "zone".
 *     Initially called with "htp" equal to "hashtab", this routine
 *     calls itself recursively in order to traverse all subdomains.
 */
mark_zone (htp, zone, flag)
    struct hashbuf *htp;
    register int zone;
    register int flag;
{
    register struct databuf *dp;
    register struct namebuf *np;
    register struct namebuf **npp, **nppend;

    nppend = htp->h_tab + htp->h_size;
    for (npp = htp->h_tab; npp < nppend; npp++) {
        for (np = *npp; np != NULL; np = np->n_next) {
	    for (dp = np->n_data; dp != NULL; dp = dp->d_next)
		if (dp->d_zone == zone)
		    dp->d_mark = flag;
	    if (np->n_hash != NULL)	/* mark subdomains */
		mark_zone (np->n_hash, zone, flag);
        }
    }
}

/*
 * clean_zone (htp, zone, flag) --
 *     Delete all RR's in the zone "zone" whose "d_mark" values are
 *     equal to "flag".  Originally called with "htp" equal to
 *     "hashtab", this routine calls itself recursively in order to
 *     traverse all subdomains.
 */
clean_zone (htp, zone, flag)
    register struct hashbuf *htp;
    register int zone;
    register int flag;
{
    register struct databuf *dp, *pdp;
    register struct namebuf *np;
    struct namebuf **npp, **nppend;

    nppend = htp->h_tab + htp->h_size;
    for (npp = htp->h_tab; npp < nppend; npp++) {
    	for (np = *npp; np != NULL; np = np->n_next) {
    	    for (pdp = NULL, dp = np->n_data; dp != NULL; ) {
		if (dp->d_zone == zone && dp->d_mark == flag)
		    dp = rm_datum(dp, np, pdp);
    	    	else {
    	    	    pdp = dp;
		    dp = dp->d_next;
    	    	}
    	    }
	    /* Call recursively to clean up subdomains. */
	    if (np->n_hash != NULL)
		clean_zone (np->n_hash, zone, flag);
	}
    }
}

/*
 * Checks to see if the xfer running on zp has been running too long, and if 
 * so, kills it (and returns 1).  Returns 0 if the xfer has not been running
 * too long.
 * Does NOT adjust the xfer_running_cnt
 *
 * zp is the zone to check
 * ttp must be set to the current time
 */
check_runtime(zp, ttp)
    register struct zoneinfo *zp;
    register struct timeval *ttp;
{
    if (ttp->tv_sec - zp->xfer_start_time > MAX_XFER_TIME - XFER_TIME_FUDGE) {
	/*
	 * Kill xfer child since xfer has taken too long- the fudge
	 * factor is used so that all xfers running with
	 * XFER_TIME_FUDGE seconds of the MAX_XFER_TIME are killed on
	 * a timeout (this is needed because the timer rang
	 * MAX_XFER_TIME seconds after the FIRST xfer started)
	 */
	int pid = zp->xfer_pid;
	zp->xfer_pid = 0;
	kill(pid, SIGKILL); /* don't trust it at all */
#ifdef DEBUG
	if (debug)
	  fprintf(ddt, "Killed child %d due to timeout\n", pid);
#endif /* DEBUG */
	return 1;
    } else return 0;
}

