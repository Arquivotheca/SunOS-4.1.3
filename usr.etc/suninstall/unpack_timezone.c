
#ifndef lint
static char sccsid[] = "@(#)unpack_timezone.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "curses.h"
#include "install.h"
#include "unpack_help.h"
#include "unpack.h"
#include "signal.h"
#include "setjmp.h"

#define TZ1 "\
Press <Return> or <Space> to move forward to your choice and type x."

extern jmp_buf sjbuf_hostname;
static jmp_buf sjbuf,sjbuf_us, sjbuf_canada, sjbuf_mexico, sjbuf_southam;
static jmp_buf	sjbuf_europe, sjbuf_asia, sjbuf_ausnz, sjbuf_gmt;

static WINDOW *wp;
WINDOW *timezone_init();

static char *cur_choice;

void
intr_time()
{
	(void) signal(SIGINT,SIG_IGN);
	menu_abort(0);
}

help_timezone(timezone)
	char *timezone;
{
	void repaint_time();
	char c;
	int y;
	char my_timezone[LARGE_STR];
	int cur_topline;
	void	(*orig_int_hndlr)(), (*orig_hup_hndlr)(), (*orig_quit_hndlr)();

	noecho();
	if (wp == NULL)
		wp = newwin(0,0,0,0);
	(void) wclear(wp);

#ifdef CATCH_SIGNALS
	orig_int_hndlr = signal(SIGINT,intr_time);
	orig_hup_hndlr = signal(SIGHUP,SIG_IGN);
	orig_quit_hndlr = signal(SIGQUIT,repaint_time);
#endif

	bzero(my_timezone,LARGE_STR);
	cur_choice = timezone; /* make this accessible to all funcs */
	(void) setjmp(sjbuf);
	redisplay_time();
	cur_topline = strlen(timezone)?6:8;
	for ( y=cur_topline; !strlen(my_timezone) ;) {
		do {
			if ( y > 22 ) y = cur_topline;
			if ( y < cur_topline ) y = 22;
			(void) wmove(wp,y,22);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
                        	y = y - 2;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			case QUESTION:
				info_window(TZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_time();
				break;
			case CONTROL_B: case CONTROL_F:
				  longjmp(sjbuf_hostname);
				  break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P ||
			  c == CONTROL_F || c == CONTROL_N ||
			  c == QUESTION  ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,22,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 6:
				/*
				 * the current setting was chosen; we're
				 * done
				 */
				(void) strcpy(my_timezone,timezone);
				break;
			case 8:
				help_us(my_timezone);
				if (!strlen(my_timezone)) redisplay_time();
				break;
			case 10:
				help_canada(my_timezone);
				if (!strlen(my_timezone)) redisplay_time();
				break;
			case 12:
				help_mexico(my_timezone);
				if (!strlen(my_timezone)) redisplay_time();
				break;
			case 14:
				help_southam(my_timezone);
				if (!strlen(my_timezone)) redisplay_time();
				break;
			case 16:
				help_europe(my_timezone);
				if (!strlen(my_timezone)) redisplay_time();
				break;
			case 18:
				help_asia(my_timezone);
				if (!strlen(my_timezone)) redisplay_time();
				break;
			case 20:
				help_ausnz(my_timezone);
				if (!strlen(my_timezone)) redisplay_time();
				break;
			case 22:
				help_gmt(my_timezone);
				if (!strlen(my_timezone)) redisplay_time();
				break;
			}
		}
        }
	(void) strcpy(timezone,my_timezone);
	(void) wclear(wp);
	delwin(wp);
	wp = NULL;
	echo();

#ifdef CATCH_SIGNALS
	(void) signal(SIGINT,orig_int_hndlr);
	(void) signal(SIGHUP,orig_hup_hndlr);
	(void) signal(SIGQUIT,orig_quit_hndlr);
#endif

}

redisplay_time()
{
	register int i;
	
	(void) wclear(wp); 

	mvwaddstr(wp,0,1,TZ_SCREEN);
	mvwaddstr(wp,1,(COLS/2)-(sizeof(TZ_BANNER)/2),TZ_BANNER);
	for (i = 0; i < COLS; i++) {
		mvwaddch(wp,2,i,'_');
	}
	
        mvwaddstr(wp,4,2,TZ1);
	mvwaddstr(wp,27,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");

	if (strlen(cur_choice)) {
        	mvwaddstr(wp,6,24,"*   current selection -- ");
        	mvwaddstr(wp,6,52,cur_choice);
	}
        mvwaddstr(wp,8 ,24,"*   United States");
        mvwaddstr(wp,10,24,"*   Canada");
        mvwaddstr(wp,12,24,"*   Mexico");
        mvwaddstr(wp,14,24,"*   South America");
        mvwaddstr(wp,16,24,"*   Europe");
        mvwaddstr(wp,18,24,"*   Asia");
        mvwaddstr(wp,20,24,"*   Australia and New Zealand");
        mvwaddstr(wp,22,24,"*   Greenwich Mean Time");
	(void) wrefresh(wp);
}

help_us(timezone)
char *timezone;
{
	void repaint_us();
	char c;
	int y;

	redisplay_us();
	(void) setjmp(sjbuf_us);
	for (y=8; !strlen(timezone); ) {
		do {
			if ( y > 25 ) y = 8;
			else if ( y < 8 ) y = 25;
			(void) wmove(wp,y,8);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
				if ( y == 20) y = y - 2;
				else if ( y == 25) y--;
                        	y = y - 2;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
				if ( y == 16) y = y + 2;
				else if ( y == 22) y++;
                        	y = y + 2;
				break;
			case QUESTION:
				info_window(SUBTZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_us();
				break;
			case CONTROL_B :
				   longjmp(sjbuf);
				   break;
			case CONTROL_F : 
				   longjmp(sjbuf_hostname);
				   break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,8,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 8:
				(void) strcpy(timezone, "US/Eastern");
				break;
			case 10:
				(void) strcpy(timezone, "US/Central");
				break;
			case 12:
				(void) strcpy(timezone, "US/Mountain");
				break;
			case 14:
				(void) strcpy(timezone, "US/Pacific");
				break;
			case 16:
				(void) strcpy(timezone, "US/Pacific-New");
				break;
			case 20:
				(void) strcpy(timezone, "US/Alaska");
				break;
			case 22:
				(void) strcpy(timezone, "US/East-Indiana");
				break;
			case 25:
				(void) strcpy(timezone, "US/Hawaii");
				break;
			}
		}
        }
}

redisplay_us()
{
	int i;
	(void) wclear(wp); 
	(void) wrefresh(wp);
        mvwaddstr(wp,0,1,"SysConfC1	      SYSTEM CONFIGURATION"); 
        mvwaddstr(wp,1,26,"UNITED STATES"); 
	for (i = 0; i < COLS; i++) 
		mvwaddch(wp,2,i,'_');
        mvwaddstr(wp,4,2,TZ1);
	mvwaddstr(wp,6,10,"TIME ZONE NAME"); mvwaddstr(wp,6,48,"AREA");
        mvwaddstr(wp,8,10, "US/Eastern");
        mvwaddstr(wp,8,40, "Eastern time zone, USA");
        mvwaddstr(wp,10,10, "US/Central");
        mvwaddstr(wp,10,40, "Central time zone, USA");
        mvwaddstr(wp,12,10,"US/Mountain");
        mvwaddstr(wp,12,40,"Mountain time zone, USA");
        mvwaddstr(wp,14,10,"US/Pacific");
        mvwaddstr(wp,14,40,"Pacific time zone, USA");
        mvwaddstr(wp,16,10,"US/Pacific-New");
        mvwaddstr(wp,16,40,"Pacific time zone, USA");
        mvwaddstr(wp,17,40,"with proposed changes to Daylight");
        mvwaddstr(wp,18,40,"Savings Time near election time");
        mvwaddstr(wp,20,10,"US/Alaska");
        mvwaddstr(wp,20,40,"Alaska time zone, USA");
        mvwaddstr(wp,22,10,"US/East-Indiana");
        mvwaddstr(wp,22,40,"Eastern time zone, USA");
        mvwaddstr(wp,23,40,"no Daylight Savings Time");
        mvwaddstr(wp,25,10,"US/Hawaii");
        mvwaddstr(wp,25,40,"Hawaii, USA");
	mvwaddstr(wp,27,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");
	(void) wrefresh(wp);
}

help_canada(timezone)
char *timezone;
{
	char c;
	int y;

	redisplay_canada();
	(void) setjmp(sjbuf_canada);
	for (y=8; !strlen(timezone) ; ) {
		do {
			if ( y > 23 ) y = 8;
			else if ( y < 8 ) y = 23;
			(void) wmove(wp,y,8);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
				if ( y == 19) y--;
                        	y = y - 2;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
				if ( y == 16) y++;
                        	y = y + 2;
				break;
			case QUESTION:
				info_window(SUBTZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_canada();
				break;
			case CONTROL_B: longjmp(sjbuf);
				  break;
			case CONTROL_F: 
				   longjmp(sjbuf_hostname);
				   break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,8,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 8:
				(void) strcpy(timezone, "Canada/Newfoundland");
				break;
			case 10:
				(void) strcpy(timezone, "Canada/Atlantic");
				break;
			case 11:
				(void) strcpy(timezone, "Canada/Eastern");
				break;
			case 13:
				(void) strcpy(timezone, "Canada/Central");
				break;
			case 15:
				(void) strcpy(timezone, "Canada/East-Saskatchewan");
				break;
			case 19:
				(void) strcpy(timezone, "Canada/Mountain");
				break;
			case 21:
				(void) strcpy(timezone, "Canada/Pacific");
				break;
			case 23:
				(void) strcpy(timezone, "Canada/Yukon");
				break;
			}
		}
        }
}

redisplay_canada()
{
	int i;
	(void) wclear(wp); 
	(void) wrefresh(wp);
        mvwaddstr(wp,0,1,"SysConfC2    	     SYSTEM CONFIGURATION"); 
        mvwaddstr(wp,1,28,"CANADA"); 
	for (i = 0; i < COLS; i++) 
		mvwaddch(wp,2,i,'_');
        mvwaddstr(wp,4,2,TZ1);
        mvwaddstr(wp,6,10,"TIME ZONE NAME");
        mvwaddstr(wp,6,48,"AREA");
        mvwaddstr(wp,8,10, "Canada/Newfoundland");
        mvwaddstr(wp,8,40, "Newfoundland");
        mvwaddstr(wp,10,10, "Canada/Atlantic");
        mvwaddstr(wp,10,40, "Atlantic time zone, Canada");
        mvwaddstr(wp,12,10,"Canada/Eastern");
        mvwaddstr(wp,12,40,"Eastern time zone, Canada");
        mvwaddstr(wp,14,10,"Canada/Central");
        mvwaddstr(wp,14,40,"Central time zone, Canada");
        mvwaddstr(wp,16,10,"Canada/East Saskatchewan");
        mvwaddstr(wp,16,40,"Central time zone, Canada");
        mvwaddstr(wp,17,40,"No Daylight Savings Time");
        mvwaddstr(wp,19,10,"Canada/Mountain");
        mvwaddstr(wp,19,40,"Mountain time zone, Canada");
        mvwaddstr(wp,21,10,"Canada/Pacific");
        mvwaddstr(wp,21,40,"Pacific time zone, Canada");
        mvwaddstr(wp,23,10,"Canada/Yukon");
        mvwaddstr(wp,23,40,"Yukon time zone, Canada");
	mvwaddstr(wp,27,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");
	(void) wrefresh(wp);
}

help_mexico(timezone)
char *timezone;
{
	char c;
	int y;
	redisplay_mexico();
	(void) setjmp(sjbuf_mexico);
	for (y=8; !strlen(timezone) ; ) {
		do {
			if ( y > 12 ) y = 8;
			else if ( y < 8 ) y = 12;
			(void) wmove(wp,y,8);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
                        	y = y - 2;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			case QUESTION:
				info_window(SUBTZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_mexico();
				break;
			case CONTROL_B: longjmp(sjbuf);
				  break;
			case CONTROL_F: 
				 longjmp(sjbuf_hostname);
				  break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,8,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 8:
				(void) strcpy(timezone, "Mexico/BajaNorte");
				break;
			case 10:
				(void) strcpy(timezone, "Mexico/BajaSur");
				break;
			case 12:
				(void) strcpy(timezone, "Mexico/General");
				break;
			}
		}
        }
}

redisplay_mexico()
{
	int i;
	(void) wclear(wp); 
	(void) wrefresh(wp);
        mvwaddstr(wp,0,1,"SysConfC3	    SYSTEM CONFIGURATION"); 
        mvwaddstr(wp,1,28,"MEXICO"); 
	for (i = 0; i < COLS; i++) 
		mvwaddch(wp,2,i,'_');
        mvwaddstr(wp,4,2,TZ1);
        mvwaddstr(wp,6,10,"TIME ZONE NAME");
        mvwaddstr(wp,6,48,"AREA");
        mvwaddstr(wp,8,10, "BajaNorte");
        mvwaddstr(wp,8,40, "Northern Baja time zone");
        mvwaddstr(wp,10,10, "BajaSur");
        mvwaddstr(wp,10,40, "Southern Baja time zone");
        mvwaddstr(wp,12,10,"General");
        mvwaddstr(wp,12,40,"Central time zone");
	mvwaddstr(wp,27,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");
	(void) wrefresh(wp);
}

help_southam(timezone)
char *timezone;
{
	char c;
	int y;
	redisplay_southam();
	(void) setjmp(sjbuf_southam);
	for (y=8; !strlen(timezone) ; ) {
		do {
			if ( y > 18 ) y = 8;
			else if ( y < 8 ) y = 18;
			(void) wmove(wp,y,8);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
                        	y = y - 2;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			case QUESTION:
				info_window(SUBTZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_southam();
				break;
			case CONTROL_B: longjmp(sjbuf);
				  break;
			case CONTROL_F: 
				longjmp(sjbuf_hostname);
				  break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,8,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 8:
				(void) strcpy(timezone, "Brazil/Acre");
				break;
			case 10:
				(void) strcpy(timezone, "Brazil/DeNoronha");
				break;
			case 12:
				(void) strcpy(timezone, "Brazil/East");
				break;
			case 14:
				(void) strcpy(timezone, "Brazil/West");
				break;
			case 16:
				(void) strcpy(timezone, "Chile/Continental");
				break;
			case 18:
				(void) strcpy(timezone, "Chile/EasterIsland");
				break;
			}
		}
        }
}

redisplay_southam()
{
	int i;
	(void) wclear(wp); 
	(void) wrefresh(wp);
#if 0
	noecho();
#endif
        mvwaddstr(wp,0,1,"SysConfC4	       SYSTEM CONFIGURATION"); 
        mvwaddstr(wp,1,26,"SOUTH AMERICA"); 
	for (i = 0; i < COLS; i++) 
		mvwaddch(wp,2,i,'_');
        mvwaddstr(wp,4,2,TZ1);
        mvwaddstr(wp,6,10, "TIME ZONE NAME");
        mvwaddstr(wp,6,48, "AREA");
        mvwaddstr(wp,8,10, "Brazil/Acre");
        mvwaddstr(wp,8,40, "Brazil time zone");
        mvwaddstr(wp,10,10, "Brazil/DeNoronha");
        mvwaddstr(wp,10,40, "Brazil time zone");
        mvwaddstr(wp,12,10,"Brazil/East");
        mvwaddstr(wp,12,40,"Brazil time zone");
        mvwaddstr(wp,14,10,"Brazil/West");
        mvwaddstr(wp,14,40,"Brazil time zone");
        mvwaddstr(wp,16,10,"Chile/Continental");
        mvwaddstr(wp,16,40,"Continental Chile time zone");
        mvwaddstr(wp,18,10,"Chile/EasterIsland");
        mvwaddstr(wp,18,40,"Easter Island time zone");
	mvwaddstr(wp,27,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");
#if 0
	wstandout(wp);
        mvwaddstr(wp,LINES-1,4,HELP1);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+5,HELP2);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        wstandend(wp);
#endif
	(void) wrefresh(wp);
}

help_europe(timezone)
char *timezone;
{
	char c;
	int y;

	redisplay_europe();
	(void) setjmp(sjbuf_europe);
	for (y=8; !strlen(timezone) ;) {
		do {
			if ( y > 25 ) y = 8;
			else if ( y < 8 ) y = 25;
			(void) wmove(wp,y,8);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
				if ( y == 17) y--;
                        	y = y - 2;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
				if ( y == 14) y++;
                        	y = y + 2;
				break;
			case QUESTION:
				info_window(SUBTZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_europe();
				break;
			case CONTROL_B: longjmp(sjbuf);
				  break;
			case CONTROL_F: 
				longjmp(sjbuf_hostname);
				  break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,8,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 8:
				(void) strcpy(timezone, "GB-Eire");
				break;
			case 10:
				(void) strcpy(timezone, "WET");
				break;
			case 12:
				(void) strcpy(timezone, "Iceland");
				break;
			case 14:
				(void) strcpy(timezone, "MET");
				break;
			case 17:
				(void) strcpy(timezone, "Poland");
				break;
			case 19:
				(void) strcpy(timezone, "EET");
				break;
			case 21:
				(void) strcpy(timezone, "Turkey");
				break;
			case 23:
				(void) strcpy(timezone, "Israel");
				break;
			case 25:
				(void) strcpy(timezone, "W-SU");
				break;
			}
		}
        }
}

redisplay_europe()
{
	int i;
	(void) wclear(wp); 
	(void) wrefresh(wp);
#if 0
	noecho();
#endif
        mvwaddstr(wp,0,1,"SysConfC5	     SYSTEM CONFIGURATION"); 
        mvwaddstr(wp,1,28,"EUROPE"); 
	for (i = 0; i < COLS; i++) 
		mvwaddch(wp,2,i,'_');
        mvwaddstr(wp,4,2,TZ1);
        mvwaddstr(wp,6,10,"TIME ZONE NAME");
        mvwaddstr(wp,6,48,"AREA");
        mvwaddstr(wp,8,10, "GB-Eire");
        mvwaddstr(wp,8,40, "Great Britain and Eire");
        mvwaddstr(wp,10,10,"WET");
        mvwaddstr(wp,10,40,"Western Europe time");
        mvwaddstr(wp,12,10,"Iceland");
        mvwaddstr(wp,12,40,"Iceland");
        mvwaddstr(wp,14,10,"MET");
        mvwaddstr(wp,14,40,"Middle European time");
        mvwaddstr(wp,15,40,"also called Central European time");
        mvwaddstr(wp,17,10, "Poland");
        mvwaddstr(wp,17,40, "Poland");
        mvwaddstr(wp,19,10, "EET");
        mvwaddstr(wp,19,40, "Eastern European time");
        mvwaddstr(wp,21,10, "Turkey");
        mvwaddstr(wp,21,40, "Turkey");
        mvwaddstr(wp,23,10, "Israel");
        mvwaddstr(wp,23,40, "Israel");
        mvwaddstr(wp,25,10, "W-SU");
        mvwaddstr(wp,25,40, "Western Soviet Union");
	mvwaddstr(wp,27,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");
#if 0
	wstandout(wp);
        mvwaddstr(wp,LINES-1,4,HELP1);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+5,HELP2);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        wstandend(wp);
#endif
	(void) wrefresh(wp);
}

help_asia(timezone)
char *timezone;
{
	char c;
	int y;

	redisplay_asia();
	(void) setjmp(sjbuf_asia);
	for (y=8; !strlen(timezone) ; ) {
		do {
			if ( y > 18 ) y = 8;
			else if ( y < 8 ) y = 18;
			(void) wmove(wp,y,8);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
                        	y = y - 2;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			case QUESTION:
				info_window(SUBTZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_asia();
				break;
			case CONTROL_B: longjmp(sjbuf);
				  break;
			case CONTROL_F: 
				longjmp(sjbuf_hostname);
				  break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,8,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 8:
				(void) strcpy(timezone, "PRC");
				break;
			case 10:
				(void) strcpy(timezone, "ROK");
				break;
			case 12:
				(void) strcpy(timezone, "Japan");
				break;
			case 14:
				(void) strcpy(timezone, "Singapore");
				break;
			case 16:
				(void) strcpy(timezone, "Hongkong");
				break;
			case 18:
				(void) strcpy(timezone, "ROC");
				break;
			}
		}
        }
}

redisplay_asia()
{
	int i;
	(void) wclear(wp); 
	(void) wrefresh(wp);
#if 0
	noecho();
#endif
        mvwaddstr(wp,0,1,"SysConfC6         SYSTEM CONFIGURATION"); 
        mvwaddstr(wp,1,28,"ASIA"); 
	for (i = 0; i < COLS; i++) 
		mvwaddch(wp,2,i,'_');
        mvwaddstr(wp,4,2,TZ1);
        mvwaddstr(wp,6,10,"TIME ZONE NAME");
        mvwaddstr(wp,6,48,"AREA");
        mvwaddstr(wp,8,10, "PRC");
        mvwaddstr(wp,8,40, "People's Republic of China");
        mvwaddstr(wp,10,10, "ROK");
        mvwaddstr(wp,10,40, "Republic of Korea");
        mvwaddstr(wp,12,10,"Japan");
        mvwaddstr(wp,12,40,"Japan");
        mvwaddstr(wp,14,10,"Singapore");
        mvwaddstr(wp,14,40,"Singapore");
        mvwaddstr(wp,16,10,"HongKong");
        mvwaddstr(wp,16,40,"Hong Kong");
        mvwaddstr(wp,18,10,"ROC");
        mvwaddstr(wp,18,40,"Republic of China/Taiwan");
	mvwaddstr(wp,27,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");
#if 0
	wstandout(wp);
        mvwaddstr(wp,LINES-1,4,HELP1);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+5,HELP2);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        wstandend(wp);
#endif
	(void) wrefresh(wp);
}

help_ausnz(timezone)
char *timezone;
{
	char c;
	int y;

	redisplay_ausnz();
	(void) setjmp(sjbuf_ausnz);
	for (y=8; !strlen(timezone) ; ) {
		do {
			if ( y > 22 ) y = 8;
			else if ( y < 8 ) y = 22;
			(void) wmove(wp,y,8);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
                        	y = y - 2;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			case QUESTION:
				info_window(SUBTZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_ausnz();
				break;
			case CONTROL_B: longjmp(sjbuf);
				  break;
			case CONTROL_F: 
				  longjmp(sjbuf_hostname);
				  break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,8,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 8:
				(void) strcpy(timezone, "Australia/Tasmania");
				break;
			case 10:
				(void) strcpy(timezone, "Australia/Queensland");
				break;
			case 12:
				(void) strcpy(timezone, "Australia/North");
				break;
			case 14:
				(void) strcpy(timezone, "Australia/West");
				break;
			case 16:
				(void) strcpy(timezone, "Australia/South");
				break;
			case 18:
				(void) strcpy(timezone, "Australia/Victoria");
				break;
			case 20:
				(void) strcpy(timezone, "Australia/NSW");
				break;
			case 22:
				(void) strcpy(timezone, "NZ");
				break;
			}
		}
        }
}

redisplay_ausnz()
{
	int i;
	(void) wclear(wp); 
	(void) wrefresh(wp);
#if 0
	noecho();
#endif
        mvwaddstr(wp,0,1,"SysConfC7	     SYSTEM CONFIGURATION"); 
        mvwaddstr(wp,1,20,"AUSTRALIA & NEW ZEALAND"); 
	for (i = 0; i < COLS; i++) 
		mvwaddch(wp,2,i,'_');
        mvwaddstr(wp,4,2,TZ1);
        mvwaddstr(wp,6,10,"TIME ZONE NAME");
        mvwaddstr(wp,6,48,"AREA");
        mvwaddstr(wp,8,10, "Australia/Tasmania");
        mvwaddstr(wp,8,40, "Tasmania, Australia");
        mvwaddstr(wp,10,10, "Australia/Queensland");
        mvwaddstr(wp,10,40, "Queensland, Australia");
        mvwaddstr(wp,12,10,"Australia/North");
        mvwaddstr(wp,12,40,"Northern Australia");
        mvwaddstr(wp,14,10,"Australia/West");
        mvwaddstr(wp,14,40,"Western Australia");
        mvwaddstr(wp,16,10,"Australia/South");
        mvwaddstr(wp,16,40,"Southern Australia");
        mvwaddstr(wp,18,10,"Australia/Victoria");
        mvwaddstr(wp,18,40,"Victoria, Australia");
        mvwaddstr(wp,20,10,"Australia/NSW");
        mvwaddstr(wp,20,40,"New South Wales, Australia");
        mvwaddstr(wp,22,10,"NZ");
        mvwaddstr(wp,22,40,"New Zealand");
	mvwaddstr(wp,27,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");
#if 0
	wstandout(wp);
        mvwaddstr(wp,LINES-1,4,HELP1);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+5,HELP2);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        wstandend(wp);
#endif
	(void) wrefresh(wp);
}

help_gmt(timezone)
char *timezone;
{
	char c;
	int y;

	redisplay_gmt();
	(void) setjmp(sjbuf_gmt);
	for (y=6; !strlen(timezone) ; ) {
		do {
			if ( y > 31 ) y = 6;
			else if ( y < 6 ) y = 31;
			(void) wmove(wp,y,8);
			(void) wrefresh(wp);
			c = wgetch(wp);
			switch (c) {
			case CONTROL_P:
                        	y--;
				break;
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y++;
				break;
			case QUESTION:
				info_window(SUBTZ_HELPSCREEN,9,
					"Press any key to continue",
					'?', NULL, (int) 0);
				redisplay_gmt();
				break;
			case CONTROL_B: longjmp(sjbuf);
				  break;
			case CONTROL_F: 
				  longjmp(sjbuf_hostname);
				  break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvwaddch(wp,y,8,'x');
			(void) wrefresh(wp);
			switch (y) {
			case 6:
				(void) strcpy(timezone, "GMT");
				break;
			case 7:
				(void) strcpy(timezone, "GMT-1");
				break;
			case 8:
				(void) strcpy(timezone, "GMT-2");
				break;
			case 9:
				(void) strcpy(timezone, "GMT-3");
				break;
			case 10:
				(void) strcpy(timezone, "GMT-4");
				break;
			case 11:
				(void) strcpy(timezone, "GMT-5");
				break;
			case 12:
				(void) strcpy(timezone, "GMT-6");
				break;
			case 13:
				(void) strcpy(timezone, "GMT-7");
				break;
			case 14:
				(void) strcpy(timezone, "GMT-8");
				break;
			case 15:
				(void) strcpy(timezone, "GMT-9");
				break;
			case 16:
				(void) strcpy(timezone, "GMT-10");
				break;
			case 17:
				(void) strcpy(timezone, "GMT-11");
				break;
			case 18:
				(void) strcpy(timezone, "GMT-12");
				break;
			case 19:
				(void) strcpy(timezone, "GMT+13");
				break;
			case 20:
				(void) strcpy(timezone, "GMT+12");
				break;
			case 21:
				(void) strcpy(timezone, "GMT+11");
				break;
			case 22:
				(void) strcpy(timezone, "GMT+10");
				break;
			case 23:
				(void) strcpy(timezone, "GMT+9");
				break;
			case 24:
				(void) strcpy(timezone, "GMT+8");
				break;
			case 25:
				(void) strcpy(timezone, "GMT+7");
				break;
			case 26:
				(void) strcpy(timezone, "GMT+6");
				break;
			case 27:
				(void) strcpy(timezone, "GMT+5");
				break;
			case 28:
				(void) strcpy(timezone, "GMT+4");
				break;
			case 29:
				(void) strcpy(timezone, "GMT+3");
				break;
			case 30:
				(void) strcpy(timezone, "GMT+2");
				break;
			case 31:
				(void) strcpy(timezone, "GMT+1");
				break;
			}
		}
        }
}

redisplay_gmt()
{
	int i;
	(void) wclear(wp); 
	(void) wrefresh(wp);
#if 0
	noecho();
#endif
        mvwaddstr(wp,0,1,"SysConfC8		SYSTEM CONFIGURATION"); 
        mvwaddstr(wp,1,24,"GREENWICH MEAN TIME"); 
	for (i = 0; i < COLS; i++) 
		mvwaddch(wp,2,i,'_');
        mvwaddstr(wp,3,2,TZ1);
        mvwaddstr(wp,4,10,"TIME ZONE NAME");
        mvwaddstr(wp,4,48,"AREA");
        mvwaddstr(wp,6,10, "GMT");
        mvwaddstr(wp,6,40, "Greenwich Mean time (GMT)");
        mvwaddstr(wp,7,10, "GMT-1");
        mvwaddstr(wp,7,40, "1 hour west of GMT");
        mvwaddstr(wp,8,10, "GMT-2");
        mvwaddstr(wp,8,40, "2 hours west of GMT");
        mvwaddstr(wp,9,10, "GMT-3");
        mvwaddstr(wp,9,40, "3 hours west of GMT");
        mvwaddstr(wp,10,10,"GMT-4");
        mvwaddstr(wp,10,40,"4 hours west of GMT");
        mvwaddstr(wp,11,10,"GMT-5");
        mvwaddstr(wp,11,40,"5 hours west of GMT");
        mvwaddstr(wp,12,10,"GMT-6");
        mvwaddstr(wp,12,40,"6 hours west of GMT");
        mvwaddstr(wp,13,10,"GMT-7");
        mvwaddstr(wp,13,40,"7 hours west of GMT");
        mvwaddstr(wp,14,10,"GMT-8");
        mvwaddstr(wp,14,40,"8 hours west of GMT");
        mvwaddstr(wp,15,10,"GMT-9");
        mvwaddstr(wp,15,40,"9 hours west of GMT");
        mvwaddstr(wp,16,10,"GMT-10");
        mvwaddstr(wp,16,40,"10 hours west of GMT");
        mvwaddstr(wp,17,10,"GMT-11");
        mvwaddstr(wp,17,40,"11 hours west of GMT");
        mvwaddstr(wp,18,10,"GMT-12");
        mvwaddstr(wp,18,40,"12 hours west of GMT");
        mvwaddstr(wp,19,10,"GMT+13");
        mvwaddstr(wp,19,40,"13 hours east of GMT");
        mvwaddstr(wp,20,10,"GMT+12");
        mvwaddstr(wp,20,40,"12 hours east of GMT");
        mvwaddstr(wp,21,10,"GMT+11");
        mvwaddstr(wp,21,40,"11 hours east of GMT");
        mvwaddstr(wp,22,10,"GMT+10");
        mvwaddstr(wp,22,40,"10 hours east of GMT");
        mvwaddstr(wp,23,10,"GMT+9");
        mvwaddstr(wp,23,40,"9 hours east of GMT");
        mvwaddstr(wp,24,10,"GMT+8");
        mvwaddstr(wp,24,40,"8 hours east of GMT");
        mvwaddstr(wp,25,10,"GMT+7");
        mvwaddstr(wp,25,40,"7 hours east of GMT");
        mvwaddstr(wp,26,10,"GMT+6");
        mvwaddstr(wp,26,40,"6 hours east of GMT");
        mvwaddstr(wp,27,10,"GMT+5");
        mvwaddstr(wp,27,40,"5 hours east of GMT");
        mvwaddstr(wp,28,10,"GMT+4");
        mvwaddstr(wp,28,40,"4 hours east of GMT");
        mvwaddstr(wp,29,10,"GMT+3");
        mvwaddstr(wp,29,40,"3 hours east of GMT");
        mvwaddstr(wp,30,10,"GMT+2");
        mvwaddstr(wp,30,40,"2 hours east of GMT");
        mvwaddstr(wp,31,10,"GMT+1");
        mvwaddstr(wp,31,40,"1 hours east of GMT");
	mvwaddstr(wp,33,1,"[?] = Help   [Control-B] = Previous Screen    [Control-F] = First Screen");
#if 0
	wstandout(wp);
        mvwaddstr(wp,LINES-1,4,HELP1);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+5,HELP2);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        wstandend(wp);
        wstandout(wp);
        mvwaddstr(wp,LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        wstandend(wp);
#endif
	(void) wrefresh(wp);
}

void
repaint_time()
{
	redisplay_time();
	longjmp(sjbuf,0);
}

void
repaint_us()
{
	redisplay_us();
	longjmp(sjbuf_us,0);
}

void
repaint_canada()
{
	redisplay_canada();
	longjmp(sjbuf_canada,0);
}

void
repaint_mexico()
{
	redisplay_mexico();
	longjmp(sjbuf_mexico,0);
}

void
repaint_southam()
{
	redisplay_southam();
	longjmp(sjbuf_southam,0);
}

void
repaint_europe()
{
	redisplay_europe();
	longjmp(sjbuf_europe,0);
}

void
repaint_asia()
{
	redisplay_asia();
	longjmp(sjbuf_asia,0);
}

void
repaint_ausnz()
{
	redisplay_ausnz();
	longjmp(sjbuf_ausnz,0);
}

void
repaint_gmt()
{
	redisplay_gmt();
	longjmp(sjbuf_gmt,0);
}
