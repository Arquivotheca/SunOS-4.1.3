#ifndef lint
static char sccsid[] = "@(#)timezone.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "curses.h"
#include "install.h"
#include "signal.h"
#include "setjmp.h"

static char HELP1[] =		"[x/X=select choice]";
static char HELP2[] =		"[space=next choice]";
static char HELP3[] =		"[^B/^P=backward]";
static char HELP4[] =		"[^F/^N=forward]";
static char HELP5[] =		"[DEL=erase one char of input data]";
static char HELP6[] =		"[RET=end of input data]";
static char FINISH[] =   	"Are you finished with this form [y/n] ? ";

jmp_buf sjbuf, sjbuf_us, sjbuf_canada, sjbuf_mexico, sjbuf_southam;
jmp_buf sjbuf_europe, sjbuf_asia, sjbuf_ausnz, sjbuf_gmt;

void
intr_time()
{
	aborting(0);
}

help_timezone(timezone)
char *timezone;
{
	void repaint_time();
	char c;
	int y;

	init();
	(void) signal(SIGINT,intr_time);
	(void) signal(SIGHUP,SIG_IGN);
	(void) signal(SIGQUIT,repaint_time);
	redisplay_time();
	(void) setjmp(sjbuf);
	for ( y=10; !strlen(timezone) ;) {
		do {
			if ( y > 24 ) y = 10;
			if ( y < 10 ) y = 24;
			move(y,23);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
                        	y = y - 2;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P ||
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,23,'x');
			refresh();
			switch (y) {
			case 10:
				help_us(timezone);
				(void) signal(SIGQUIT,repaint_time);
				if (!strlen(timezone)) redisplay_time();
				break;
			case 12:
				help_canada(timezone);
				(void) signal(SIGQUIT,repaint_time);
				if (!strlen(timezone)) redisplay_time();
				break;
			case 14:
				help_mexico(timezone);
				(void) signal(SIGQUIT,repaint_time);
				if (!strlen(timezone)) redisplay_time();
				break;
			case 16:
				help_southam(timezone);
				(void) signal(SIGQUIT,repaint_time);
				if (!strlen(timezone)) redisplay_time();
				break;
			case 18:
				help_europe(timezone);
				(void) signal(SIGQUIT,repaint_time);
				if (!strlen(timezone)) redisplay_time();
				break;
			case 20:
				help_asia(timezone);
				(void) signal(SIGQUIT,repaint_time);
				if (!strlen(timezone)) redisplay_time();
				break;
			case 22:
				help_ausnz(timezone);
				(void) signal(SIGQUIT,repaint_time);
				if (!strlen(timezone)) redisplay_time();
				break;
			case 24:
				help_gmt(timezone);
				(void) signal(SIGQUIT,repaint_time);
				if (!strlen(timezone)) redisplay_time();
				break;
			}
		}
        }
	clear();
	refresh();
	endwin();
	resetty();
}

redisplay_time()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(2,25,"MAJOR TIME ZONE CATEGORIES");
        mvaddstr(4,14,"Select one of the folloing categories to display");
        mvaddstr(5,16,"a screen of time zone names for that region.");
        mvaddstr(8,18,"Please use x or X to select your choice");
        mvaddstr(10,24,"   *   United States");
        mvaddstr(12,24,"   *   Canada");
        mvaddstr(14,24,"   *   Mexico");
        mvaddstr(16,24,"   *   South America");
        mvaddstr(18,24,"   *   Europe");
        mvaddstr(20,24,"   *   Asia");
        mvaddstr(22,24,"   *   Australia and New Zealand");
        mvaddstr(24,24,"   *   Greenwich Mean Time");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
}

help_us(timezone)
char *timezone;
{
	void repaint_us();
	char c;
	int y, ret;

	(void) signal(SIGQUIT,repaint_us);
	redisplay_us();
	(void) setjmp(sjbuf_us);
	for (y=6; !strlen(timezone) && !ret ; ) {
		do {
			if ( y > 25 ) y = 6;
			else if ( y < 6 ) y = 25;
			move(y,8);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
				if ( y == 18) y = y - 2;
				else if ( y == 23) y--;
                        	y = y - 2;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
				if ( y == 14) y = y + 2;
				else if ( y == 20) y++;
                        	y = y + 2;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,8,'x');
			refresh();
			switch (y) {
			case 6:
				strcpy(timezone, "US/Eastern");
				break;
			case 8:
				strcpy(timezone, "US/Central");
				break;
			case 10:
				strcpy(timezone, "US/Mountain");
				break;
			case 12:
				strcpy(timezone, "US/Pacific");
				break;
			case 14:
				strcpy(timezone, "US/Pacific-New");
				break;
			case 18:
				strcpy(timezone, "US/Yukon");
				break;
			case 20:
				strcpy(timezone, "US/East-Indiana");
				break;
			case 23:
				strcpy(timezone, "US/Hawaii");
				break;
			case 25:
				ret++;
				break;
			}
		}
        }
}

redisplay_us()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(0,26,"UNITED STATES");
        mvaddstr(2,18,"Please use x or X to select your choice");
        mvaddstr(4,10,"TIME ZONE NAME");
        mvaddstr(4,40,"AREA");
        mvaddstr(6,10, "US/Eastern");
        mvaddstr(6,40, "Eastern time zone, USA");
        mvaddstr(8,10, "US/Central");
        mvaddstr(8,40, "Central time zone, USA");
        mvaddstr(10,10,"US/Mountain");
        mvaddstr(10,40,"Mountain time zone, USA");
        mvaddstr(12,10,"US/Pacific");
        mvaddstr(12,40,"Pacific time zone, USA");
        mvaddstr(14,10,"US/Pacific-New");
        mvaddstr(14,40,"Pacific time zone, USA");
        mvaddstr(15,40,"with proposed cahnges to Daylight");
        mvaddstr(16,40,"Savings Time near election time");
        mvaddstr(18,10,"US/Yukon");
        mvaddstr(18,40,"Yukon time zone, USA");
        mvaddstr(20,10,"US/East-Indiana");
        mvaddstr(20,40,"Eastern time zone, USA");
        mvaddstr(21,40,"no Daylight Savings Time");
        mvaddstr(23,10,"US/Hawaii");
        mvaddstr(23,40,"Hawaii");
        mvaddstr(25,10,"Return to Timezone Category Screen");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
}

help_canada(timezone)
char *timezone;
{
	void repaint_canada();
	char c;
	int y, ret;

	(void) signal(SIGQUIT,repaint_canada);
	redisplay_canada();
	(void) setjmp(sjbuf_canada);
	for (y=6; !strlen(timezone) && !ret ; ) {
		do {
			if ( y > 23 ) y = 6;
			else if ( y < 6 ) y = 23;
			move(y,8);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
				if ( y == 17) y--;
                        	y = y - 2;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
				if ( y == 14) y++;
                        	y = y + 2;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,8,'x');
			refresh();
			switch (y) {
			case 6:
				strcpy(timezone, "Canada/Newfoundland");
				break;
			case 8:
				strcpy(timezone, "Canada/Atlantic");
				break;
			case 10:
				strcpy(timezone, "Canada/Eastern");
				break;
			case 12:
				strcpy(timezone, "Canada/Central");
				break;
			case 14:
				strcpy(timezone, "Canada/East-Saskatchewan");
				break;
			case 17:
				strcpy(timezone, "Canada/Mountain");
				break;
			case 19:
				strcpy(timezone, "Canada/Pacific");
				break;
			case 21:
				strcpy(timezone, "Canada/Yukon");
				break;
			case 23:
				ret++;
				break;
			}
		}
        }
}

redisplay_canada()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(0,28,"CANADA");
        mvaddstr(2,18,"Please use x or X to select your choice");
        mvaddstr(4,10,"TIME ZONE NAME");
        mvaddstr(4,40,"AREA");
        mvaddstr(6,10, "Canada/Newfoundland");
        mvaddstr(6,40, "Newfoundland");
        mvaddstr(8,10, "Canada/Atlantic");
        mvaddstr(8,40, "Atlantic time zone, Canada");
        mvaddstr(10,10,"Canada/Eastern");
        mvaddstr(10,40,"Eastern time zone, Canada");
        mvaddstr(12,10,"Canada/Central");
        mvaddstr(12,40,"Central time zone, Canada");
        mvaddstr(14,10,"Canada/East Saskatchewan");
        mvaddstr(14,40,"Central time zone, Canada");
        mvaddstr(15,40,"No Daylight Savings Time");
        mvaddstr(17,10,"Canada/Mountain");
        mvaddstr(17,40,"Mountain time zone, Canada");
        mvaddstr(19,10,"Canada/Pacific");
        mvaddstr(19,40,"Pacific time zone, Canada");
        mvaddstr(21,10,"Canada/Yukon");
        mvaddstr(21,40,"Yukon time zone, Canada");
        mvaddstr(23,10,"Return to Timezone Category Screen");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
}

help_mexico(timezone)
char *timezone;
{
	void repaint_mexico();
	char c;
	int y, ret;

	(void) signal(SIGQUIT,repaint_mexico);
	redisplay_mexico();
	(void) setjmp(sjbuf_mexico);
	for (y=6; !strlen(timezone) && !ret ; ) {
		do {
			if ( y > 12 ) y = 6;
			else if ( y < 6 ) y = 12;
			move(y,8);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
                        	y = y - 2;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,8,'x');
			refresh();
			switch (y) {
			case 6:
				strcpy(timezone, "Mexico/BajaNorte");
				break;
			case 8:
				strcpy(timezone, "Mexico/BajaSur");
				break;
			case 10:
				strcpy(timezone, "Mexico/General");
				break;
			case 12:
				ret++;
				break;
			}
		}
        }
}

redisplay_mexico()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(0,28,"MEXICO");
        mvaddstr(2,18,"Please use x or X to select your choice");
        mvaddstr(4,10,"TIME ZONE NAME");
        mvaddstr(4,40,"AREA");
        mvaddstr(6,10, "BajaNorte");
        mvaddstr(6,40, "Northern Baja time zone");
        mvaddstr(8,10, "BajaSur");
        mvaddstr(8,40, "Southern Baja time zone");
        mvaddstr(10,10,"General");
        mvaddstr(10,40,"Central time zone");
        mvaddstr(12,10,"Return to Timezone Category Screen");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
}

help_southam(timezone)
char *timezone;
{
	void repaint_southam();
	char c;
	int y, ret;

	(void) signal(SIGQUIT,repaint_southam);
	redisplay_southam();
	(void) setjmp(sjbuf_southam);
	for (y=6; !strlen(timezone) && !ret ; ) {
		do {
			if ( y > 18 ) y = 6;
			else if ( y < 6 ) y = 18;
			move(y,8);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
                        	y = y - 2;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,8,'x');
			refresh();
			switch (y) {
			case 6:
				strcpy(timezone, "Brazil/Acre");
				break;
			case 8:
				strcpy(timezone, "Brazil/DeNoronha");
				break;
			case 10:
				strcpy(timezone, "Brazil/East");
				break;
			case 12:
				strcpy(timezone, "Brazil/West");
				break;
			case 14:
				strcpy(timezone, "Chile/Continental");
				break;
			case 16:
				strcpy(timezone, "Chile/EasterIsland");
				break;
			case 18:
				ret++;
				break;
			}
		}
        }
}

redisplay_southam()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(0,26, "SOUTH AMERICA");
        mvaddstr(2,18, "Please use x or X to select your choice");
        mvaddstr(4,10, "TIME ZONE NAME");
        mvaddstr(4,40, "AREA");
        mvaddstr(6,10, "Brazil/Acre");
        mvaddstr(6,40, "Brazil time zone");
        mvaddstr(8,10, "Brazil/DeNoronha");
        mvaddstr(8,40, "Brazil time zone");
        mvaddstr(10,10,"Brazil/East");
        mvaddstr(10,40,"Brazil time zone");
        mvaddstr(12,10,"Brazil/West");
        mvaddstr(12,40,"Brazil time zone");
        mvaddstr(14,10,"Chile/Continental");
        mvaddstr(14,40,"Continental Chile time zone");
        mvaddstr(16,10,"Chile/EasterIsland");
        mvaddstr(16,40,"Easter Island time zone");
        mvaddstr(18,10,"Return to Timezone Category Screen");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
}

help_europe(timezone)
char *timezone;
{
	void repaint_europe();
	char c;
	int y, ret;

	(void) signal(SIGQUIT,repaint_europe);
	redisplay_europe();
	(void) setjmp(sjbuf_europe);
	for (y=6; !strlen(timezone) && !ret ;) {
		do {
			if ( y > 25 ) y = 6;
			else if ( y < 6 ) y = 25;
			move(y,8);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
				if ( y == 15) y--;
                        	y = y - 2;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
				if ( y == 12) y++;
                        	y = y + 2;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,8,'x');
			refresh();
			switch (y) {
			case 6:
				strcpy(timezone, "GB-Eire");
				break;
			case 8:
				strcpy(timezone, "WET");
				break;
			case 10:
				strcpy(timezone, "Iceland");
				break;
			case 12:
				strcpy(timezone, "MET");
				break;
			case 15:
				strcpy(timezone, "Poland");
				break;
			case 17:
				strcpy(timezone, "EET");
				break;
			case 19:
				strcpy(timezone, "Turkey");
				break;
			case 21:
				strcpy(timezone, "Israel");
				break;
			case 23:
				strcpy(timezone, "W-SU");
				break;
			case 25:
				ret++;
				break;
			}
		}
        }
}

redisplay_europe()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(0,28,"EUROPE");
        mvaddstr(2,18,"Please use x or X to select your choice");
        mvaddstr(4,10,"TIME ZONE NAME");
        mvaddstr(4,40,"AREA");
        mvaddstr(6,10, "GB-Eire");
        mvaddstr(6,40, "Great Britain and Eire");
        mvaddstr(8,10,"WET");
        mvaddstr(8,40,"Western Europe time");
        mvaddstr(10,10,"Iceland");
        mvaddstr(10,40,"Iceland");
        mvaddstr(12,10,"MET");
        mvaddstr(12,40,"Middle European time");
        mvaddstr(13,40,"also called Central European time");
        mvaddstr(15,10, "Poland");
        mvaddstr(15,40, "Poland");
        mvaddstr(17,10, "EET");
        mvaddstr(17,40, "Eastern European time");
        mvaddstr(19,10, "Turkey");
        mvaddstr(19,40, "Turkey");
        mvaddstr(21,10, "Israel");
        mvaddstr(21,40, "Israel");
        mvaddstr(23,10, "W-SU");
        mvaddstr(23,40, "Western Soviet Union");
        mvaddstr(25,10,"Return to Timezone Category Screen");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
}

help_asia(timezone)
char *timezone;
{
	void repaint_asia();
	char c;
	int y, ret;

	(void) signal(SIGQUIT,repaint_asia);
	redisplay_asia();
	(void) setjmp(sjbuf_asia);
	for (y=6; !strlen(timezone) && !ret ; ) {
		do {
			if ( y > 18 ) y = 6;
			else if ( y < 6 ) y = 18;
			move(y,8);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
                        	y = y - 2;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,8,'x');
			refresh();
			switch (y) {
			case 6:
				strcpy(timezone, "PRC");
				break;
			case 8:
				strcpy(timezone, "ROK");
				break;
			case 10:
				strcpy(timezone, "Japan");
				break;
			case 12:
				strcpy(timezone, "Singapore");
				break;
			case 14:
				strcpy(timezone, "Hongkong");
				break;
			case 16:
				strcpy(timezone, "ROC");
				break;
			case 18:
				ret++;
				break;
			}
		}
        }
}

redisplay_asia()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(0,28,"ASIA");
        mvaddstr(2,18,"Please use x or X to select your choice");
        mvaddstr(4,10,"TIME ZONE NAME");
        mvaddstr(4,40,"AREA");
        mvaddstr(6,10, "PRC");
        mvaddstr(6,40, "People's Republic of China");
        mvaddstr(8,10, "ROK");
        mvaddstr(8,40, "Republic of Korea");
        mvaddstr(10,10,"Japan");
        mvaddstr(10,40,"Japan");
        mvaddstr(12,10,"Singapore");
        mvaddstr(12,40,"Singapore");
        mvaddstr(14,10,"HongKong");
        mvaddstr(14,40,"Hong Kong");
        mvaddstr(16,10,"ROC");
        mvaddstr(16,40,"Republic of China/Taiwan");
        mvaddstr(18,10,"Return to Timezone Category Screen");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
}

help_ausnz(timezone)
char *timezone;
{
	void repaint_ausnz();
	char c;
	int y, ret;

	(void) signal(SIGQUIT,repaint_ausnz);
	redisplay_ausnz();
	(void) setjmp(sjbuf_ausnz);
	for (y=6; !strlen(timezone) && !ret ; ) {
		do {
			if ( y > 22 ) y = 6;
			else if ( y < 6 ) y = 22;
			move(y,8);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
                        	y = y - 2;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y = y + 2;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,8,'x');
			refresh();
			switch (y) {
			case 6:
				strcpy(timezone, "Australia/Tasmania");
				break;
			case 8:
				strcpy(timezone, "Australia/Queensland");
				break;
			case 10:
				strcpy(timezone, "Australia/North");
				break;
			case 12:
				strcpy(timezone, "Australia/West");
				break;
			case 14:
				strcpy(timezone, "Australia/South");
				break;
			case 16:
				strcpy(timezone, "Australia/Victoria");
				break;
			case 18:
				strcpy(timezone, "Australia/NSW");
				break;
			case 20:
				strcpy(timezone, "NZ");
				break;
			case 22:
				ret++;
				break;
			}
		}
        }
}

redisplay_ausnz()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(0,14,"AUSTRALIA & NEW ZEALAND");
        mvaddstr(2,18,"Please use x or X to select your choice");
        mvaddstr(4,10,"TIME ZONE NAME");
        mvaddstr(4,40,"AREA");
        mvaddstr(6,10, "Australia/Tasmania");
        mvaddstr(6,40, "Tasmania, Australia");
        mvaddstr(8,10, "Australia/Queensland");
        mvaddstr(8,40, "Queensland, Australia");
        mvaddstr(10,10,"Australia/North");
        mvaddstr(10,40,"Northern Australia");
        mvaddstr(12,10,"Australia/West");
        mvaddstr(12,40,"Western Australia");
        mvaddstr(14,10,"Australia/South");
        mvaddstr(14,40,"Southern Australia");
        mvaddstr(16,10,"Australia/Victoria");
        mvaddstr(16,40,"Victoria, Australia");
        mvaddstr(18,10,"Australia/NSW");
        mvaddstr(18,40,"New South Wales, Australia");
        mvaddstr(20,10,"NZ");
        mvaddstr(20,40,"New Zealand");
        mvaddstr(22,10,"Return to Timezone Category Screen");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
}

help_gmt(timezone)
char *timezone;
{
	void repaint_gmt();
	char c;
	int y, ret;

	(void) signal(SIGQUIT,repaint_gmt);
	redisplay_gmt();
	(void) setjmp(sjbuf_gmt);
	for (y=6; !strlen(timezone) && !ret ; ) {
		do {
			if ( y > 32 ) y = 6;
			else if ( y < 6 ) y = 32;
			move(y,8);
			refresh();
			c = getch();
			switch (c) {
			case CONTROL_B:
			case CONTROL_P:
                        	y--;
				break;
                	case CONTROL_F:
			case CONTROL_N:
			case RETURN:
			case SPACE:
                        	y++;
				break;
			}
		} while ( c == CONTROL_B || c == CONTROL_P || 
			  c == CONTROL_F || c == CONTROL_N ||
			  c == RETURN || c == SPACE);
		if ( c == 'x' || c == 'X' ) {
			mvaddch(y,8,'x');
			refresh();
			switch (y) {
			case 6:
				strcpy(timezone, "GMT");
				break;
			case 7:
				strcpy(timezone, "GMT-1");
				break;
			case 8:
				strcpy(timezone, "GMT-2");
				break;
			case 9:
				strcpy(timezone, "GMT-3");
				break;
			case 10:
				strcpy(timezone, "GMT-4");
				break;
			case 11:
				strcpy(timezone, "GMT-5");
				break;
			case 12:
				strcpy(timezone, "GMT-6");
				break;
			case 13:
				strcpy(timezone, "GMT-7");
				break;
			case 14:
				strcpy(timezone, "GMT-8");
				break;
			case 15:
				strcpy(timezone, "GMT-9");
				break;
			case 16:
				strcpy(timezone, "GMT-10");
				break;
			case 17:
				strcpy(timezone, "GMT-11");
				break;
			case 18:
				strcpy(timezone, "GMT-12");
				break;
			case 19:
				strcpy(timezone, "GMT+13");
				break;
			case 20:
				strcpy(timezone, "GMT+12");
				break;
			case 21:
				strcpy(timezone, "GMT+11");
				break;
			case 22:
				strcpy(timezone, "GMT+10");
				break;
			case 23:
				strcpy(timezone, "GMT+9");
				break;
			case 24:
				strcpy(timezone, "GMT+8");
				break;
			case 25:
				strcpy(timezone, "GMT+7");
				break;
			case 26:
				strcpy(timezone, "GMT+6");
				break;
			case 27:
				strcpy(timezone, "GMT+5");
				break;
			case 28:
				strcpy(timezone, "GMT+4");
				break;
			case 29:
				strcpy(timezone, "GMT+3");
				break;
			case 30:
				strcpy(timezone, "GMT+2");
				break;
			case 31:
				strcpy(timezone, "GMT+1");
				break;
			case 32:
				ret++;
				break;
			}
		}
        }
}

redisplay_gmt()
{
	clear(); 
	refresh();
	noecho();
        mvaddstr(0,16,"GREENWICH MEAN TIME");
        mvaddstr(2,18,"Please use x or X to select your choice");
        mvaddstr(4,10,"TIME ZONE NAME");
        mvaddstr(4,40,"AREA");
        mvaddstr(6,10, "GMT");
        mvaddstr(6,40, "Greenwich Mean time (GMT)");
        mvaddstr(7,10, "GMT-1");
        mvaddstr(7,40, "1 hour west of GMT");
        mvaddstr(8,10, "GMT-2");
        mvaddstr(8,40, "2 hours west of GMT");
        mvaddstr(9,10, "GMT-3");
        mvaddstr(9,40, "3 hours west of GMT");
        mvaddstr(10,10,"GMT-4");
        mvaddstr(10,40,"4 hours west of GMT");
        mvaddstr(11,10,"GMT-5");
        mvaddstr(11,40,"5 hours west of GMT");
        mvaddstr(12,10,"GMT-6");
        mvaddstr(12,40,"6 hours west of GMT");
        mvaddstr(13,10,"GMT-7");
        mvaddstr(13,40,"7 hours west of GMT");
        mvaddstr(14,10,"GMT-8");
        mvaddstr(14,40,"8 hours west of GMT");
        mvaddstr(15,10,"GMT-9");
        mvaddstr(15,40,"9 hours west of GMT");
        mvaddstr(16,10,"GMT-10");
        mvaddstr(16,40,"10 hours west of GMT");
        mvaddstr(17,10,"GMT-11");
        mvaddstr(17,40,"11 hours west of GMT");
        mvaddstr(18,10,"GMT-12");
        mvaddstr(18,40,"12 hours west of GMT");
        mvaddstr(19,10,"GMT+13");
        mvaddstr(19,40,"13 hours east of GMT");
        mvaddstr(20,10,"GMT+12");
        mvaddstr(20,40,"12 hours east of GMT");
        mvaddstr(21,10,"GMT+11");
        mvaddstr(21,40,"11 hours east of GMT");
        mvaddstr(22,10,"GMT+10");
        mvaddstr(22,40,"10 hours east of GMT");
        mvaddstr(23,10,"GMT+9");
        mvaddstr(23,40,"9 hours east of GMT");
        mvaddstr(24,10,"GMT+8");
        mvaddstr(24,40,"8 hours east of GMT");
        mvaddstr(25,10,"GMT+7");
        mvaddstr(25,40,"7 hours east of GMT");
        mvaddstr(26,10,"GMT+6");
        mvaddstr(26,40,"6 hours east of GMT");
        mvaddstr(27,10,"GMT+5");
        mvaddstr(27,40,"5 hours east of GMT");
        mvaddstr(28,10,"GMT+4");
        mvaddstr(28,40,"4 hours east of GMT");
        mvaddstr(29,10,"GMT+3");
        mvaddstr(29,40,"3 hours east of GMT");
        mvaddstr(30,10,"GMT+2");
        mvaddstr(30,40,"2 hours east of GMT");
        mvaddstr(31,10,"GMT+1");
        mvaddstr(31,40,"1 hours east of GMT");
        mvaddstr(32,10,"Return to Timezone Category Screen");
	standout();
        mvaddstr(LINES-1,4,HELP1);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+5,HELP2);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+6,HELP3);
        standend();
        standout();
        mvaddstr(LINES-1,strlen(HELP1)+strlen(HELP2)+strlen(HELP3)+7,HELP4);
        standend();
	refresh();
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
