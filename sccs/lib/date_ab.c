# include	<sys/types.h>
# include	"../hdr/macros.h"
# include	<sys/time.h>

SCCSID(@(#)date_ab.c 1.1 92/07/30 SMI); /* from System III 5.1 */
/*
	Function to convert date in the form "yymmddhhmmss" to
	standard UNIX time (seconds since Jan. 1, 1970 GMT).
	Units left off of the right are replaced by their
	maximum possible values.

	The function corrects properly for leap year,
	daylight savings time, offset from Greenwich time, etc.

	Function returns -1 if bad time is given (i.e., "730229").
*/
char *Datep;


date_ab(adt,bdt)
char *adt;
long *bdt;
{
	struct tm timevalue;
	extern time_t timelocal();

	tzsetwall();	/* always use the machine's local time */
	Datep = adt;

	if((timevalue.tm_year = g2()) == -2)
		timevalue.tm_year = 99;
	if(timevalue.tm_year < 70 || timevalue.tm_year > 99)
		return(-1);

	if((timevalue.tm_mon = g2()) == -2)
		timevalue.tm_mon = 12;
	if(timevalue.tm_mon < 1 || timevalue.tm_mon > 12)
		return(-1);

	if((timevalue.tm_mday = g2()) == -2)
		timevalue.tm_mday =
		    mosize(timevalue.tm_year, timevalue.tm_mon);
	if(timevalue.tm_mday < 1
	    || timevalue.tm_mday > mosize(timevalue.tm_year, timevalue.tm_mon))
		return(-1);

	if((timevalue.tm_hour = g2()) == -2)
		timevalue.tm_hour = 23;
	if(timevalue.tm_hour < 0 || timevalue.tm_hour > 23)
		return(-1);

	if((timevalue.tm_min = g2()) == -2)
		timevalue.tm_min = 59;
	if(timevalue.tm_min < 0 || timevalue.tm_min > 59)
		return(-1);

	if((timevalue.tm_sec = g2()) == -2)
		timevalue.tm_sec = 59;
	if(timevalue.tm_sec < 0 || timevalue.tm_sec > 59)
		return(-1);

	timevalue.tm_mon--;
	*bdt = timelocal(&timevalue);
	return(0);
}

int     dmsize[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

mosize(y,t)
int y, t;
{
	if(t==2 && ((y % 4) == 0 && (y % 100) != 0 || (y % 400) == 0))
		return(29);
	return(dmsize[t-1]);
}


g2()
{
	register int c;
	register char *p;

	for (p = Datep; *p; p++)
		if (numeric(*p))
			break;
	if (*p) {
		c = (*p++ - '0') * 10;
		if (*p)
			c += (*p++ - '0');
		else
			c = -1;
	}
	else
		c = -2;
	Datep = p;
	return(c);
}
