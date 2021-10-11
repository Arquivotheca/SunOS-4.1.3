/*	@(#)lastlog.h 1.1 92/07/30 SMI; from UCB 4.2 83/05/22	*/

#ifndef _lastlog_h
#define _lastlog_h

struct lastlog {
	time_t	ll_time;
	char	ll_line[8];
	char	ll_host[16];		/* same as in utmp */
};

#endif /*!_lastlog_h*/
