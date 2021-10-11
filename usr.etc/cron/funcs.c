/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)funcs.c 1.1 92/07/30 SMI"; /* from S5R3 1.2 */
#endif

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include "cron.h"

/*****************/
FILE *fopen_configfile(name, mode, pathname)
/*****************/
char *name, *mode, *pathname;
{
	register FILE *fp;

	/*
	 * Try to open a configuration file; look first in PRIVCONFIGDIR
	 * (the per-machine configuration directory).
	 */
	(void) strcat(strcat(strcpy(pathname,PRIVCONFIGDIR),"/"),name);
	errno = 0;
	if ((fp = fopen(pathname, mode)) != NULL)
		return (fp);

	if (errno != ENOENT)
		return (NULL);	/* it does exist, you just can't open it */
	
	/*
	 * Now look in COMCONFIGDIR (the shared configuration directory).
	 */
	(void) strcat(strcat(strcpy(pathname,COMCONFIGDIR),"/"),name);
	return (fopen(pathname, mode));
}

/*****************/
time_t num(ptr)
/*****************/
char **ptr;
{
	time_t n=0;
	while (isdigit(**ptr)) {
		n = n*10 + (**ptr - '0');
		*ptr += 1; }
	return(n);
}


int dom[12]={31,28,31,30,31,30,31,31,30,31,30,31};

/*****************/
days_btwn(m1,d1,y1,m2,d2,y2)
/*****************/
int m1,d1,y1,m2,d2,y2;
{
	/* calculate the number of "full" days in between m1/d1/y1 and m2/d2/y2.
	   NOTE: there should not be more than a year separation in the dates.
		 also, m should be in 0 to 11, and d should be in 1 to 31.    */
	
	int days,m;

	if ((m1==m2) && (d1==d2) && (y1==y2)) return(0);
	if ((m1==m2) && (d1<d2)) return(d2-d1-1);
	/* the remaining dates are on different months */
	days = (days_in_mon(m1,y1)-d1) + (d2-1);
	m = (m1 + 1) % 12;
	while (m != m2) {
		if (m==0) y1++;
		days += days_in_mon(m,y1);
		m = (m + 1) % 12; }
	return(days);
}


/*****************/
days_in_mon(m,y)
/*****************/
int m,y;
{
	/* returns the number of days in month m of year y
	   NOTE: m should be in the range 0 to 11	*/

	return( dom[m] + (((m==1)&&((y%4)==0)) ? 1:0 ));
}

/*****************/
char *xmalloc(size)
/*****************/
int size;
{
	char *p;
	char *malloc();

	if((p=malloc(size)) == NULL) {
		syslog(LOG_ERR,"Can't allocate %d bytes of space",size);
		exit(55);
	}
	return p;
}

/*****************/
char *errmsg(errnum)
/*****************/
int errnum;
{
	extern int sys_nerr;
	extern char *sys_errlist[];
	static char msgbuf[6+10+1];

	if (errnum < 0 || errnum >= sys_nerr) {
		(void) sprintf(msgbuf, "Error %d", errnum);
		return (msgbuf);
	} else
		return (sys_errlist[errnum]);
}
