static	char sccsid[] = "@(#)schedule.c 1.1 92/07/30 Copyright Sun Micro";

/*
 * This file contains the routines that are
 * called by the schedule option in sundiag.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include "sundiag.h"
#include "sundiag_msg.h"

static int yeartable[2][13] = {
	{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
};

struct times { /* To hold the data entered in by the user */
	int	tm_sec;
	int	tm_min;
	int	tm_hour;
	int	tm_mday;
	int	tm_mon;
	int	tm_year;
	int	tm_wday;
	int	tm_yday;
	int	tm_isdst;
	char	*tm_zone;
	long	tm_gmtoff;
} start, stop;

static long timetostart;	/* start time in seconds since Jan. 1,1970 */
static long timetostop; 	/* stop time in seconds since Jan. 1,1970 */

long runtime;	 		/* run time in seconds from now */

static Frame ttime_frame=NULL;
static Panel ttime_panel;

Panel_item	schedule_item;
Panel_item	run_time_item;
Panel_item	start_date_item;
Panel_item	start_time_item;
Panel_item	stop_date_item;
Panel_item	stop_time_item;

/******************************************************************************
 * Notify procedure for the "Schedule" button.				      *
 ******************************************************************************/

 /***** global flag(switch) variables. ******/

 int schedule_file		= 0;

 char run_time_file[10]		= "hhh:mm:ss";
 char start_time_file[10]	= "hh:mm:ss";
 char stop_time_file[10]	= "hh:mm:ss";

 char start_date_file[10]	= "mm/dd/yy";
 char stop_date_file[10]	= "mm/dd/yy";


/******* forward references ************/

static ttime_done_proc();
static ttime_cancel_proc();

static get_start_date();
static get_start_time();

static get_stop_date();
static get_stop_time();

/*ARGSUSED*/

ttime_proc()
{
	int which_row=0;
	if (running == GO) return;

	if (ttime_frame != NULL)
		frame_destroy_proc(ttime_frame);
	
	ttime_frame = window_create(sundiag_frame, FRAME,
		FRAME_SHOW_LABEL, TRUE,
		FRAME_LABEL,	"Test Schedule Menu",
		WIN_X, (int)((STATUS_WIDTH+PERFMON_WIDTH)*frame_width)+15,
		WIN_Y,	20,
		FRAME_DONE_PROC, frame_destroy_proc, 0);

	ttime_panel = window_create(ttime_frame, PANEL, 0);

	schedule_item = panel_create_item(ttime_panel, PANEL_CYCLE,
            PANEL_LABEL_STRING,         "Schedule Feature:",
	    PANEL_CHOICE_STRINGS,	"Disable ", "Enable", 0,
	    PANEL_VALUE,		schedule_file,
            PANEL_ITEM_X,               ATTR_COL(1),
            PANEL_ITEM_Y,               ATTR_ROW(which_row++),
            0);

	++which_row;

	run_time_item = panel_create_item(ttime_panel, PANEL_TEXT,
		PANEL_LABEL_STRING,	"Run Time:",
		PANEL_VALUE,		run_time_file,
		PANEL_VALUE_DISPLAY_LENGTH,	9,
		PANEL_VALUE_STORED_LENGTH,	9,
		PANEL_ITEM_X,		ATTR_COL(1),
		PANEL_ITEM_Y,		ATTR_ROW(which_row++),
		0);

	++which_row;

	start_date_item = panel_create_item(ttime_panel, PANEL_TEXT,
		PANEL_LABEL_STRING,	"Start Date:",
		PANEL_VALUE,		start_date_file,
		PANEL_VALUE_DISPLAY_LENGTH,	8,
		PANEL_VALUE_STORED_LENGTH,	8,
		PANEL_ITEM_X,		ATTR_COL(1),
		PANEL_ITEM_Y,		ATTR_ROW(which_row++),
		0);

	start_time_item = panel_create_item(ttime_panel, PANEL_TEXT,
		PANEL_LABEL_STRING,	"Start Time:",
		PANEL_VALUE,		start_time_file,
		PANEL_VALUE_DISPLAY_LENGTH,	8,
		PANEL_VALUE_STORED_LENGTH,	8,
		PANEL_ITEM_X,		ATTR_COL(1),
		PANEL_ITEM_Y,		ATTR_ROW(which_row++),
		0);

	++which_row;

	 stop_date_item = panel_create_item(ttime_panel, PANEL_TEXT,
		PANEL_LABEL_STRING,	"Stop Date:",
		PANEL_VALUE,		stop_date_file,
		PANEL_VALUE_DISPLAY_LENGTH,	8,
		PANEL_VALUE_STORED_LENGTH,	8,
		PANEL_ITEM_X,		ATTR_COL(1),
		PANEL_ITEM_Y,		ATTR_ROW(which_row++),
		0);

	 stop_time_item = panel_create_item(ttime_panel, PANEL_TEXT,
		PANEL_LABEL_STRING,	"Stop Time:",
		PANEL_VALUE,		stop_time_file,
		PANEL_VALUE_DISPLAY_LENGTH,	8,
		PANEL_VALUE_STORED_LENGTH,	8,
		PANEL_ITEM_X,		ATTR_COL(1),
		PANEL_ITEM_Y,		ATTR_ROW(which_row++),
		0);

	++which_row;

	(void)panel_create_item(ttime_panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE,	panel_button_image(ttime_panel,
					"Done", 7, (Pixfont *)NULL),
		PANEL_ITEM_X,		ATTR_COL(1),
		PANEL_ITEM_Y,		ATTR_ROW(which_row),
		PANEL_NOTIFY_PROC,	ttime_done_proc,
		0);
	(void)panel_create_item(ttime_panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE,	panel_button_image(ttime_panel,
					"Cancel", 7, (Pixfont *)NULL),
		PANEL_NOTIFY_PROC,	ttime_cancel_proc,
		0);
	window_fit(ttime_panel);
	window_fit(ttime_frame);
	(void)window_set(ttime_frame, WIN_SHOW, TRUE, 0);
}

/*
 * Come here if cancel key is pressed.
 */
static
ttime_cancel_proc()
{
	(void)window_set(ttime_frame, FRAME_NO_CONFIRM, TRUE, 0);
	(void)window_destroy(ttime_frame);
	ttime_frame = NULL;
}

/* 
 * Process the various values in the scheduler menu.
 */
ttime_process_proc()
{
	if (!run_time_valid(run_time_file))
	{	/* Error */
		(void)confirm("Invalid run time format!", TRUE);
		schedule_file=FALSE;
		return;
	}
	if (!valid(start_date_file, '/'))
	{
		(void)confirm("Invalid start date format!", TRUE);
		schedule_file=FALSE;
		return;
	}
	if (!valid(start_time_file, ':'))
	{	/* Error */
		(void)confirm("Invalid start time format!", TRUE);
		schedule_file=FALSE;
		return;
	}
	if (!valid(stop_date_file, '/'))
	{	/* Error */
		(void)confirm("Invalid stop date format!", TRUE);
		schedule_file=FALSE;
		return;
	}
	if (!valid(stop_time_file, ':'))
	{	/* Error */
		(void)confirm("Invalid stop time format!", TRUE);
		schedule_file=FALSE;
		return;
	}
   if (schedule_file  == TRUE)
   {
	init_times();     /* initialize start and stop structs */
	if ( (get_start_date() == -1) )
	{
		(void)confirm("Invalid start date!", TRUE);
		schedule_file=FALSE;
		return;
	}
	
	if ( (get_stop_date() == -1) )
	{
		(void)confirm("Invalid stop date!", TRUE);
		schedule_file=FALSE;
		return;
	}
	if ( (get_start_time() == -1) )
	{
		(void)confirm("Invalid start time!", TRUE);
		schedule_file=FALSE;
		return;
	}
	if ( (get_stop_time() == -1) )
	{
		(void)confirm("Invalid stop time!", TRUE);
		schedule_file=FALSE;
		return;
	}
	if ( (runtime = get_run_time()) == -1 )
	{
		(void)confirm("Invalid run time!", TRUE);
		schedule_file=FALSE;
		return;
	}
	timetostart = timelocal(&start);
	timetostop = timelocal(&stop);
	if ( timetostart == timetostop )
		timetostart = timetostop = -1;
   }
	(void)window_set(ttime_frame, FRAME_NO_CONFIRM, TRUE, 0);
	(void)window_destroy(ttime_frame);
	ttime_frame = NULL;
}


/*
 * Initialize start and stop time 
 * structures to current time.
 */

init_times()
{ 
	struct tm *log_time;
	struct timeval time;

	if ( gettimeofday(&time, (struct timezone *)NULL) < 0 )
	{
		perror("gettimeofday");
		exit(1);
	}

	log_time = localtime(&time.tv_sec);
	start.tm_sec = stop.tm_sec = log_time->tm_sec;
	start.tm_min = stop.tm_min = log_time->tm_min;
	start.tm_hour = stop.tm_hour = log_time->tm_hour;
	start.tm_mday = stop.tm_mday = log_time->tm_mday;
	start.tm_mon = stop.tm_mon = log_time->tm_mon;
	start.tm_year = stop.tm_year = log_time->tm_year;
	start.tm_wday = stop.tm_wday = log_time->tm_wday;
	start.tm_yday = stop.tm_yday = log_time->tm_yday;
	start.tm_isdst = stop.tm_isdst = log_time->tm_isdst;
	start.tm_zone = stop.tm_zone = log_time->tm_zone;
	start.tm_gmtoff = stop.tm_gmtoff = log_time->tm_gmtoff;
}

/*
 * Is a year a leap year?
 */
isleap(year)
int year;

{
	return((year%4 == 0 && year%100 != 0) || year%100 == 0);
}

/*
 * Determine the day of year given
 * a month and day of month value.
 */
countdays(month, day, year)
int month;
int day;
int year;
{
	int leap;			/* are we dealing with a leap year? */
	int dayofyear;			/* the day of year after conversion */
	int monthofyear;		/* the month of year that we are
					   dealing with */

	/*
	 * Are we dealing with a leap year?
	 */
	leap = isleap(year);

	monthofyear = month;
	dayofyear = day;

	/*
	 * Determine the day of year.
	 */
	while (monthofyear > 0)
		dayofyear += yeartable[leap][monthofyear--];

	return(dayofyear);
}

long
get_run_time()
{
	int hour=0, minute=0, second=0;
	if (!strcmp(run_time_file, "") || !strcmp(run_time_file, "hhh:mm:ss"))
		return(-2);	/* Not an error */
	sscanf(run_time_file,"%03d:%02d:%02d", &hour, &minute, &second);
	if ( hour > 999 || minute > 59 || second > 59 )
		return (-1);	/* Invalid entry */
	else if ( hour == 999 )
		if ( minute || second )
			return -1;	/* Invalid entry */
	second = hour*3600 + minute*60 + second;
	return(second);
}

static
get_start_date()
{
	int month , mday, year;
	if (!strcmp(start_date_file, "") || !strcmp(start_date_file, "mm/dd/yy"))
		return(0);
	sscanf(start_date_file,"%2d/%2d/%2d", &month, &mday, &year);
	if (month > 12 || year > 99 || (mday > yeartable[isleap(year)][month])) 
		return(-1);	/* Invalid entry */
	start.tm_mon = month-1; /* Since the count starts from 0 */
	start.tm_mday = mday;
	start.tm_year = year;
	start.tm_yday = countdays(month,mday,year);
	return(0);
}

static
get_stop_date()
{
	int month , mday, year;
	if (!strcmp(stop_date_file, "") || !strcmp(stop_date_file, "mm/dd/yy"))
		return(0);
	sscanf(stop_date_file,"%2d/%2d/%2d", &month, &mday, &year);
	if (month > 12 || year > 99 || (mday > yeartable[isleap(year)][month])) 
		return(-1);	/* Invalid entry */
	stop.tm_mon = month-1;	/* Since the count starts from 0 */
	stop.tm_mday = mday;
	stop.tm_year = year;
	stop.tm_yday = countdays(month,mday,year);
	return(0);
}
static
get_start_time()
{
	int hour=0, minute=0, second=0;
	if (!strcmp(start_time_file, "") || !strcmp(start_time_file, "hh:mm:ss"))
		return(0);
	sscanf(start_time_file,"%2d:%2d:%2d", &hour, &minute, &second);
	if ( hour > 24 || minute > 59 || second > 59 )
		return(-1);	/* Invalid entry */
	else if ( hour == 24 )
		if ( minute || second )
			return -1;	/* Invalid entry */
	start.tm_hour = hour;
	start.tm_min = minute;
	start.tm_sec = second;
	return(0);
}

static
get_stop_time()
{
	int hour=0, minute=0, second=0;
	if (!strcmp(stop_time_file, "") || !strcmp(stop_time_file, "hh:mm:ss"))
		return(0);
	sscanf(stop_time_file,"%2d:%2d:%2d", &hour, &minute, &second);
	if ( hour > 24 || minute > 59 || second > 59 )
		return(-1);	/* Invalid entry */
	else if ( hour == 24 )
		if ( minute || second )
			return(-1);	/* Invalid entry */
	stop.tm_hour = hour;
	stop.tm_min = minute;
	stop.tm_sec = second;
	return(0);
}

/*
 * Returns TRUE if  NULL string or Format is correct, FALSE otherwise.
 */

valid(ptr,ch)
char *ptr;
char ch;
{
	if ( *ptr == 0 ||  ( ch == '/' && !strcmp(ptr, "mm/dd/yy") ) || 
	( ch == ':' && !strcmp(ptr,"hh:mm:ss")) )
		return TRUE; /* If there is no change OR.. */
	else if ( isdigit( ptr[0] ) && isdigit( ptr[1] ) && 
	ptr[2] == ch && isdigit(ptr[3]) && isdigit(ptr[4]) && 
	ptr[5] == ch && isdigit(ptr[6]) && isdigit(ptr[7]) )
		return TRUE; /* .. if its a valid change, return TRUE */
	else
		return FALSE; /* If not its an error. */
}

/*
 * Returns TRUE if  NULL string or Format is correct, FALSE otherwise.
 */

run_time_valid(ptr)
char *ptr;
{
	if ( (*ptr == 0) || !strcmp(ptr,"hhh:mm:ss") )
		return TRUE; /* If there is no change OR.. */
	else if ( isdigit( ptr[0] ) && isdigit( ptr[1] ) && 
	isdigit(ptr[2])  && (ptr[3] == ':') && isdigit(ptr[4]) && 
	isdigit(ptr[5]) && (ptr[6] == ':') &&isdigit(ptr[7]) &&
	isdigit(ptr[8]) ) 
		return TRUE; /* .. if its a valid change, return TRUE */
	else
		return FALSE; /* If not its an error. */
}

/*
 * Read in the entries from the scheduler menu.
 */
static
ttime_done_proc()
{
	sprintf(run_time_file,"%s",(char *)panel_get_value(run_time_item));
	sprintf(start_date_file,"%s",(char *)panel_get_value(start_date_item));
	sprintf(start_time_file,"%s",(char *)panel_get_value(start_time_item));
	sprintf(stop_date_file,"%s",(char *)panel_get_value(stop_date_item));
	sprintf(stop_time_file,"%s",(char *)panel_get_value(stop_time_item));
   	schedule_file = (int)panel_get_value(schedule_item);
	ttime_process_proc();
}

/*
 * Process start,stop and run times.
 */
sched_proc(elapse_count, current_time)
long elapse_count;
long current_time;
{
	static long run_time = 0;
	switch(running)
	{
	case GO:
		if ((run_time++ == runtime) || (timetostop == current_time))
		{
			logmsg(schedule_stop_info, 1, SCHEDULE_STOP_INFO);
			stop_proc();
		}
		break;
	case IDLE:
		run_time = 0;
		if ( timetostart == current_time )
		{
			logmsg(schedule_start_info, 1, SCHEDULE_START_INFO);
			start_proc();
		}
		break;
	case KILL:
		timetostart = -1;
		timetostop = 0;
		run_time = 0;
		break;
	case SUSPEND:
		if ( ( timetostart > 0 ) && ( current_time >= timetostart) )
			timetostart = 0;
		if ( ( timetostop > 0 ) && ( current_time >= timetostop) )
			timetostop = current_time+1;
		break;
	}
}
