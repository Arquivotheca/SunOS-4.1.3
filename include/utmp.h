/*	@(#)utmp.h 1.1 92/07/30 SMI; from UCB 4.2 83/05/22	*/

#ifndef _utmp_h
#define _utmp_h

/*
 * Structure of utmp and wtmp files.
 *
 * XXX - Assuming the number 8 is unwise.
 */
struct utmp {
	char	ut_line[8];		/* tty name */
	char	ut_name[8];		/* user id */
	char	ut_host[16];		/* host name, if remote */
	long	ut_time;		/* time on */
};

/*
 * This is a utmp entry that does not correspond to a genuine user
 */
#define nonuser(ut) ((ut).ut_host[0] == 0 && \
	strncmp((ut).ut_line, "tty", 3) == 0 && ((ut).ut_line[3] == 'p' \
					      || (ut).ut_line[3] == 'q' \
					      || (ut).ut_line[3] == 'r' \
					      || (ut).ut_line[3] == 's'))

#endif /*!_utmp_h*/
