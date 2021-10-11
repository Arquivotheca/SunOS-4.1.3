/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)testterminfo.c 1.1 92/07/30 SMI"; /* from S5R3 1.9 */
#endif

/*
    Terminfo entry tester.  Exercises terminal capabilities as defined in
    the terminfo entry to test their definition and use.

    Original program by Alan Robertson
    Further modified by Tony Hansen and Ernie Rice.
    Originally based on a similar program written by Warren Montgomery
    for testing his terminal database.

sanity tests to add:
    auto_left_margin w/o cursor_left
    tabs
    status line


    The following characteristics are not tested yet.
    The starred ones probably won't every be.

    bools
	auto_left_margin
	beehive_glitch
	ceol_standout_glitch
	eat_newline_glitch
	* generic_type
	* hardcopy
	has_meta_key
	move_insert_mode
	move_standout_mode
	* overstrike
	* teleray_glitch
	* tilde_glitch
	* xon_xoff

    ints
	init_tabs
	lines_of_memory
	* padding_baud_rate
	* virtual_terminal

    strings
	back_tab
	clear_all_tabs
	* command_character
	cursor_invisible
	cursor_mem_address
	cursor_normal
	cursor_visible
	* down_half_line
	* enter_secure_mode
	* enter_proteced_mode
	* form_feed
	init_?string
	init_file
	insert_padding
	key_*
	keypad_local
	keypad_xmit
	lab_f*
	* meta_off
	* meta_on
	newline
	pad_char
	pkey_key
	pkey_local
	* print_screen
	* prtr_off
	* prtr_on
	reset_?string
	reset_file
	restore_cursor
	save_cursor
	set_tab
	* set_window
	tab
	underline_char
	* up_half_line
	* init_prog
	* prtr_non
	* char_padding
	* acs_chars

*/

#include <stdio.h>
#include <signal.h>
#include <curses.h>
#include <term.h>

extern char *getenv();
extern char *tparm();
extern void exit();
extern unsigned sleep();

int	manual = 1;

/* The exiting routine */
onintr(sig)
int sig;
{
	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGHUP, SIG_DFL);

	if (exit_ca_mode) {
		printf("testing exit_ca_mode");
		(void) fflush(stdout);
		(void) getchar();

		putp(exit_ca_mode);
		printf("exited ca mode\r\n");
		(void) fflush(stdout);
	}

	reset_shell_mode();
	printf("\n\n");
	if (sig == 0) printf("Exiting normally.\n");
	exit(0);
}

/* Pause a moment for the user to see the results. */
int pause_for_input()
{
	int ch;

	(void) fflush(stdout);

	if (manual) ch = getchar();
	else ch = ' ';

	if (ch == 'q' || ch == 'Q') onintr(0);
	return ch;
}

/* clear the screen the stupid way, assuming nothing */
void dumb_clear_screen()
{
	register int i;

	printf("\r");
	for (i=0; i <= lines + 10;i++) printf("\n\r");
	(void) fflush(stdout);
}

/* clear the screen */
void clearscreen()
{
	if (clear_screen) putp(clear_screen);
	else if (cursor_home && clr_eos) {
		putp(cursor_home);
		putp(clr_eos);
	}
	else dumb_clear_screen();
}

/* move the cursor to the given coordinates, but don't assume as much */
/* about what's available for use */
void dumb_gocurs(x, y)
register int x, y;
{
	if (cursor_address) putp(tparm(cursor_address, x, y));
	else if (cursor_home && cursor_down && cursor_right) {
		putp(cursor_home);

		while (--y) putp(cursor_down);

		while (--x) putp(cursor_right);
	}
	else {
		printf("Cannot perform dumb_gocurs\n\r");
		(void) fflush(stdout);
	}
}

/* move the cursor to the given coordinates, doing it somewhat optimally */
void gocurs(x, y)
register int x, y;
{
	if (cursor_address) putp(tparm(cursor_address, x, y));
	else if (column_address && row_address) {
		putp(tparm(column_address, y));
		putp(tparm(row_address, x));
	}
	else if (cursor_home &&
		(cursor_down || parm_down_cursor || column_address) &&
		(cursor_right || parm_right_cursor || row_address) ) {

		putp(cursor_home);

		if (parm_down_cursor) {
			if (y > 0) putp(tparm(parm_down_cursor, y-1));
		}
		else if (column_address) putp(tparm(column_address, y));
		else while (--y) putp(cursor_down);

		if (parm_right_cursor) {
			if (x > 0) putp(tparm(parm_right_cursor, x-1));
		}
		else if (row_address) putp(tparm(row_address, x));
		else while (--x) putp(cursor_right);
	}
	else dumb_gocurs(x,y);
}

/* move the cursor home, using only home or cursor_address */
void gohome()
{
	if (cursor_home) putp(cursor_home);
	else if (cursor_address) putp(tparm(cursor_address, 0, 0));
}

/* Underline the string. */
void uprint(sp)
register char *sp;
{
	if (cursor_left)
		while (*sp) {
			(void) putchar('_');
			putp(cursor_left);
			(void) putchar(*sp++);
		}
	else {
		printf("Cannot perform uprint without cursor_left\r\n");
		(void) fflush(stdout);
	}
}

/* test insert/delete line */
void tst_idline()
{
	register int i;

again:
	if (insert_line) {
		clearscreen();
		for(i = 0; i < lines; i++) {
			gocurs(i,0);
			printf("Line %d",i);
		}
		
		gocurs(0,10);
		printf("inserting 5 lines after line 2");
		gocurs(3,10);

		for(i = 0; i < 5; i++) putp(insert_line);

		if (pause_for_input() == 'r') goto again;
	}

	if (delete_line) {
		gocurs(0,10);
		printf("deleting 5 lines after line 2...");
		gocurs(3,10);

		for(i = 0; i < 5; i++) putp(delete_line);

		gocurs(3,10);
		if (memory_below)
			printf("the last line number should(may) be > %d",lines-6);
		else
			printf("the last line number must be = %d",lines-6);
		if (pause_for_input() == 'r') goto again;
	}
	(void) fflush(stdout);
}

/* test parameterized insert/delete line */
void tst_Pidline()
{
	register int i;

again:
	if (parm_insert_line) {
		clearscreen();
		for(i = 0; i < lines; i++) {
			gocurs(i, 0);
			printf("Line %d", i);
		}

		gocurs(0, 10);
		printf("inserting 5 lines after line 2(using parm_insert_line)");
		gocurs(3, 10);
		putp(tparm(parm_insert_line, 5));

		if (pause_for_input() == 'r') goto again;
	}

	if (parm_delete_line) {
		gocurs(0, 10);
		printf("deleting 5 lines after line 2	(using parm_delete_line)");
		gocurs(3, 10);
		putp(tparm(parm_delete_line, 5));
		gocurs(3, 10);

		if (memory_below) printf("the last line number should(may) be > %d", lines - 6);
		else printf("the last line number must be = %d", lines - 6);
		if (pause_for_input() == 'r') goto again;
	}
}

/* test scrolling forward and reverse */
void tst_scroll(top, bot, pause)
register int top, bot;
int pause;
{
	register int i, fullscreen =(top == 0) &&(bot == lines - 1);

again:
	if (scroll_forward) {
		clearscreen();
		gocurs(bot, 0);
		printf("scrolling");
		putp(scroll_forward);
		gocurs(bot, 0);
		printf("up");
		putp(scroll_forward);
		gocurs(bot, 0);
		printf("the");
		putp(scroll_forward);
		gocurs(bot, 0);
		printf("screen");

		for(i = 0; i < bot -(top + 3); ++i) {
			gocurs(bot, 0);
			putp(scroll_forward);
		}

		if (pause_for_input() == 'r') goto again;
	}

	if (scroll_reverse) {
		gocurs(10, 0);
		printf("reverse scrolling");
		gocurs(top, 0);
		putp(scroll_reverse);
		gocurs(top, 0);
		putp(scroll_reverse);
		gocurs(10, 0);

		if (scroll_forward && memory_above && fullscreen) printf("the message should(may) have returned");
		else {
			if (fullscreen)
			printf("the top 2 lines must be blank.");
			else
			printf("the top 2 lines in the scrolling region must be blank");
		}

		if (pause_for_input() == 'r' && pause) goto again;
	}
}

/* test change scrolling region */
void tst_csr()
{
    register int top, bot;

again:
	top = 2;
	bot = lines - 3;
	if (change_scroll_region &&(scroll_forward || scroll_reverse)) {
		clearscreen();
		gocurs(0, 0);
		printf("test change scrolling region");
		gocurs(top - 1, 0);
		printf("this line won't scroll");
		gocurs(bot + 1, 0);
		printf("neither will this one");
		putp(tparm(change_scroll_region, top, bot));
		tst_scroll(top, bot);
		putp(tparm(change_scroll_region, 0, lines - 1));

		if (pause_for_input() == 'r') goto again;
	}
}

/* test attribute settings */
void tst_attr()
{
    register int nextline;
    char tempbuffer[80];

again:
	nextline = 1;
	tempbuffer[0] = '\0';

	clearscreen();
	printf("testing font changes");

	if (enter_standout_mode) {
		gocurs(nextline++, 0);
		putp(enter_standout_mode);
		printf("standout text");
		putp(exit_standout_mode);
	}

	if (enter_underline_mode) {
		gocurs(nextline++, 0);
		putp(enter_underline_mode);
		printf("underlined text");
		putp(exit_underline_mode);
	}

	if (enter_bold_mode) {
		gocurs(nextline++, 0);
		putp(enter_bold_mode);
		printf("bold text");
		putp(exit_attribute_mode);
	}

	if (enter_blink_mode) {
		gocurs(nextline++, 0);
		putp(enter_blink_mode);
		printf("blinking  text");
		putp(exit_attribute_mode);
	}

	if (enter_dim_mode) {
		gocurs(nextline++, 0);
		putp(enter_dim_mode);
		printf("dim witted text");
		putp(exit_attribute_mode);
	}

	if (enter_reverse_mode) {
		gocurs(nextline++, 0);
		putp(enter_reverse_mode);
		printf("Reverse Video Text");
		putp(exit_attribute_mode);
	}

	if (enter_alt_charset_mode) {
		gocurs(nextline++, 0);
		putp(enter_alt_charset_mode);
		printf("alternate");
		putp(exit_alt_charset_mode);
	}

	/* Now all in the same line */
	gocurs(nextline++, 0);
	if (enter_standout_mode) {
		putp(enter_standout_mode);
		printf("standout");
		(void) strcat(tempbuffer, "standout");
		putp(exit_standout_mode);
	}

	if (enter_underline_mode) {
		putp(enter_underline_mode);
		printf("underlined");
		(void) strcat(tempbuffer, "underlined");
		putp(exit_underline_mode);
	}

	if (enter_bold_mode) {
		putp(enter_bold_mode);
		printf("bold");
		(void) strcat(tempbuffer, "bold");
		putp(exit_attribute_mode);
	}

	if (enter_blink_mode) {
		putp(enter_blink_mode);
		printf("blinking");
		(void) strcat(tempbuffer, "blinking");
		putp(exit_attribute_mode);
	}

	if (enter_dim_mode) {
		putp(enter_dim_mode);
		printf("dim");
		(void) strcat(tempbuffer, "dim");
		putp(exit_attribute_mode);
	}

	if (enter_reverse_mode) {
		putp(enter_reverse_mode);
		printf("Reverse");
		(void) strcat(tempbuffer, "Reverse");
		putp(exit_attribute_mode);
	}

	if (enter_alt_charset_mode) {
		putp(enter_alt_charset_mode);
		printf("alternate");
		(void) strcat(tempbuffer, "alternate");
		putp(exit_alt_charset_mode);
	}

	gocurs(nextline++, 0);
	printf(tempbuffer);

	if (set_attributes) {
		gocurs(nextline++, 0);
		printf("with sgr:");
		putp(tparm(set_attributes,1,0,0,0,0,0,0,0,0));/* 1 = 3+5 */
		printf("standout,");
		putp(tparm(set_attributes,0,1,0,0,0,0,0,0,0));/* 2 */
		printf("underline,");
		putp(tparm(set_attributes,0,0,1,0,0,0,0,0,0));/* 3 */
		printf("reverse,");
		putp(tparm(set_attributes,0,0,0,1,0,0,0,0,0));/* 4 */
		printf("blink,");
		putp(tparm(set_attributes,0,0,0,0,1,0,0,0,0));/* 5 */
		printf("dim,");
		putp(tparm(set_attributes,0,0,0,0,0,1,0,0,0));/* 6 = 3+5 */
		printf("bold,");
		putp(tparm(set_attributes,0,0,0,0,0,0,1,0,0));/* 7 */
		printf("invis,");
		putp(tparm(set_attributes,0,0,0,0,0,0,0,1,0));/* 8 */
		printf("protect,");
		putp(tparm(set_attributes,0,0,0,0,0,0,0,0,1));/* 9 */
		printf("altchar,");
		putp(tparm(set_attributes,0,0,0,0,0,0,0,0,0));
		printf("off");
		gocurs(nextline++, 0);
		printf("w/o  sgr:standout,underline,reverse,blink,dim,bold,invis,protect,altchar,off");
	}

	gocurs(nextline++, 0);
	printf("01234");
	gocurs(nextline++, 0);

	if (magic_cookie_glitch <= 0) printf("all the above fonts should begin in column 0");
	else printf("all the above fonts should begin in column %d", magic_cookie_glitch);

	if (pause_for_input() == 'r') goto again;
}

/* test parameterized scrolling */
void tst_Pscroll(top, bot)
register int top, bot;
{
	register int fullscreen =(top == 0) &&(bot == lines - 1);

again:
	if (parm_index) {
		clearscreen();
		gocurs(bot, 0);
		printf("multi-line(parmeterized) scrolling(parm_index)");
		putp(tparm(parm_index, 1));
		gocurs(bot, 0);
		printf("up");
		putp(tparm(parm_index, 1));
		gocurs(bot, 0);
		printf("the");
		gocurs(bot, 0);
		putp(tparm(parm_index, 1));
		gocurs(bot, 0);
		printf("screen");
		gocurs(bot, 0);
		putp(tparm(parm_index, bot -(top + 2)));
		gocurs(bot, 0);
	
		if (pause_for_input() == 'r') goto again;
	
		if (parm_index && scroll_reverse && parm_rindex) {
			gocurs(10, 0);
			printf("reverse scrolling(parm_rindex)");
			gocurs(top, 0);
			putp(tparm(parm_rindex, 2));
			gocurs(10, 0);
	
			if (scroll_forward && memory_above && fullscreen) printf("the message should(may) have returned");
			else {
				if (fullscreen) printf("the top 2 lines must be blank.");
				else printf("the top 2 lines in the scrolling region must be blank");
			}
	
			if (pause_for_input() == 'r') goto again;
		}
	}
}

/* test parameterized cursor left/right */
void tst_Plr()
{
    register int i, col;

again:
	if (parm_left_cursor && parm_right_cursor) {
    		clearscreen();
		printf("Test parm_left and parm_right cursor");
		gocurs(2, 0);
		i = columns - 1;
		col = 0;
		for(;;) {
			printf("%1d", col % 10);
			--i;
			col += i;
			if (i <= 0)
			break;
			putp(tparm(parm_right_cursor, i - 1));
			printf("%1d", col % 10);
			putp(tparm(parm_left_cursor, i));
			--i;
			col -= i;
		}
	
		if (pause_for_input() == 'r') goto again;
	}
}

/* test parameterized cursor up/down */
void tst_Pud()
{
    register int i, row;

again:

	if (parm_up_cursor && parm_down_cursor && carriage_return) {
		clearscreen();
		printf("	    Test parm_up and parm_down cursor");
		i = lines;
		row = 0;
		for(;;) {
			putp(carriage_return);
			printf("%2d", row);
			--i;
			row += i;
			if (i <= 0)
			break;
			putp(tparm(parm_down_cursor, i));
			putp(carriage_return);
			printf("%2d", row);
			--i;
			row -= i;
			if (i <= 0)
			break;
			putp(tparm(parm_up_cursor, i));
		}

		if (pause_for_input() == 'r') goto again;
	}
}

tst_sanity()
{
again:
	clearscreen();
	printf("sanity testing\r\n\n");

	if (lines <= 0) 
		printf("terminal has %d lines?\r\n",lines);

	if (columns <= 0) 
		printf("terminal has %d columns?\r\n",columns);

	if (enter_ca_mode && !exit_ca_mode)
		printf("enter_cursor_address_mode given without exit_ca_mode\n\r");
	else if (!enter_ca_mode && exit_ca_mode)
		printf("exit_cursor_address_mode given without enter_ca_mode\n\r");

	if (enter_insert_mode && !exit_insert_mode)
		printf("enter_insert_mode given without exit_insert_mode\n\r");
	else if (!enter_insert_mode && exit_insert_mode)
		printf("exit_insert_mode given without enter_insert_mode\n\r");

	if (enter_delete_mode && !exit_delete_mode)
		printf("enter_delete_mode given without exit_delete_mode\n\r");
	else if (!enter_delete_mode && exit_delete_mode)
		printf("exit_delete_mode given without enter_delete_mode\n\r");

	if (enter_standout_mode && !exit_standout_mode)
		printf("enter_standout_mode given without exit_standout_mode\n\r");
	else if (!enter_standout_mode && exit_standout_mode)
		printf("exit_standout_mode given without enter_standout_mode\n\r");

	if (enter_underline_mode && !exit_underline_mode)
		printf("enter_underline_mode given without exit_underline_mode\n\r");
	else if (!enter_underline_mode && exit_underline_mode)
		printf("exit_underline_mode given without enter_underline_mode\n\r");

	if (enter_xon_mode && !exit_xon_mode)
		printf("enter_xon_mode given without exit_xon_mode\n\r");
	else if (!enter_xon_mode && exit_xon_mode)
		printf("exit_xon_mode given without enter_xon_mode\n\r");

	if (enter_am_mode && !exit_am_mode)
		printf("enter_am_mode given without exit_am_mode\n\r");
	else if (!enter_am_mode && exit_am_mode)
		printf("exit_am_mode given without enter_am_mode\n\r");

	if (enter_alt_charset_mode && !exit_alt_charset_mode)
		printf("enter_alt_charset_mode given without exit_alt_charset_mode\n\r");
	else if (!enter_alt_charset_mode && exit_alt_charset_mode)
		printf("exit_alt_charset_mode given without enter_alt_charset_mode\n\r");

	if (has_status_line) {
		if (!to_status_line)
			printf("has status line, but no way to get to it!\r\n");
	    	if (!from_status_line)
			printf("has status line, but no way to get from it!\r\n");
	}
	else {
		if (to_status_line) /* others? */
			printf("no status line, but a way is defined to get to it!\r\n");
		if (from_status_line) /* others? */
			printf("no status line, but a way is defined to get from it!\r\n");
	}

	if (pause_for_input() == 'r') goto again;
}

tst_CAenter()
{
again:
	if (enter_ca_mode) {
		clearscreen();
		printf("Testing 'enter_ca_mode'.");
		putp(enter_ca_mode);
		printf("cursor address mode begun.");

		if (pause_for_input() == 'r') goto again;
	}
}

tst_clear()
{
again:
	dumb_clear_screen();
	if (clear_screen) {
		printf("Clear Screen Test");
		putp(clear_screen);
		printf("Screen was cleared and cursor should have gone home\r\n");
	}
	else printf("No clear_screen!\r\n");
	if (pause_for_input() == 'r') goto again;
}

tst_home()
{
again:
	dumb_clear_screen();
	if (cursor_home) {
		printf("Going");
		putp(cursor_home);
		printf("Home");

	} else printf("No cursor_home\r\n");
	if (pause_for_input() == 'r') goto again;
	dumb_clear_screen();
}

tst_CM()
{
	register int i;

again:
	dumb_clear_screen();
	if (cursor_address) {
		for(i = 0; i < columns; i++) {
			putp(tparm(cursor_address, 10, i));
			printf("%c\r\n",((i & 07) + '0'));
		}
	
		if (cursor_home) putp(cursor_home);
		else putp(tparm(cursor_address, 0, 0));
	
		for(i = 0; i < 10; i++) printf("%d\r\n", i);

		putp(tparm(cursor_address, 12, 10));
		printf("Cursor address test");
		putp(tparm(cursor_address, 14, 10));
		printf("Line 10 should count across the screen 0-7");

		if (pause_for_input() == 'r') goto again;
	
		dumb_clear_screen();
		for(i = 0; i < lines; i++) {
			putp(tparm(cursor_address, i, columns / 2));
			printf("%d", i);
		}
	
		if (cursor_home) putp(cursor_home);
		else putp(tparm(cursor_address, 0, 0));
	
		for(i = 0; i < columns / 2; i++) printf("%1d", i % 10);

		putp(tparm(cursor_address, 1, 0));
		printf("Cursor address test");
		putp(tparm(cursor_address, 3, 0));
		printf("Screen should");
		putp(tparm(cursor_address, 4, 0));
		printf("have a line");
		putp(tparm(cursor_address, 5, 0));
		printf("counting down");
		putp(tparm(cursor_address, 6, 0));
		printf("the middle");
	}
	else printf("No cursor_address.\n\r");

	if (pause_for_input() == 'r') goto again;
}

/* clear to the end of the screen */
tst_eos()
{
    register int i;

again:
	dumb_clear_screen();
	if (clr_eos) {
		if (!cursor_address && !cursor_home) {
			printf("need either cursor_address or cursor_home to test clr_eos!\n\r");
			if (pause_for_input() == 'r') goto again;
			return;
		}
	
		for(i = 0; i < lines; i++) printf("\r\n1234");

		if (cursor_address) {
			putp(tparm(cursor_address, lines / 2, 0));
			printf("wxyz");
		}
		else if (cursor_home) {
			putp(cursor_home);
			for(i = 0; i < lines / 2; i--)
			printf("\r\n");
			printf ("wxyz");
		}

		printf(" testing clear to end of screen between x and y");
		if (pause_for_input() == 'r') goto again;
	
		if (cursor_address)
			putp(tparm(cursor_address, lines / 2, 2));
		else if (cursor_home) {
			putp(cursor_home);
			for(i = 0; i < lines / 2; i--)
			printf("\r\n");
			printf ("xy");
		}
	
		putp(clr_eos);
	}
	else printf("No clr_eos\r\n");

	if (pause_for_input() == 'r') goto again;
}

/* test cursor_right */
tst_right()
{
    register int i;

again:
	clearscreen();
	if (cursor_right) {
		printf("01234567890123456789012345678901234567890\n\r1\n\r2\n\r3\n\r4\n\r5\t");
		printf("Drawing Forwards from 10,0 to 10,40, moving right once between each");
		printf("\n\r6\n\r7\n\r8\n\r9\n\r");

		for(i = 0; i < 20; i++) {
			printf("%d", i % 10);
			putp(cursor_right);
		}
	}
	else printf("No cursor_right.\r\n");

	if (pause_for_input() == 'r') goto again;
}

/* test cursor_down */
tst_down()
{
    register int i;

again:
	clearscreen();
	if (cursor_down) {
		gohome();
		printf("012345678901234567890123456789012\r\n");

		for(i=1; i<19; ) printf("%d\r\n",i++);
	
		dumb_gocurs(5,5);
		printf("Drawing cursor_down diagonally from 6,20 to 18,32");
		dumb_gocurs(6,20);

		for(i = 0; i < 12; i++) {
			printf("%d", i % 10);
			putp(cursor_down);
		}

		printf("%d", i % 10);
		putp(cursor_down);
	}
	else printf("No cursor_down.\r\n");

	if (pause_for_input() == 'r') goto again;
}

/* testing cursor_left */
tst_left()
{
    register int i;

again:
	clearscreen();
	if (cursor_left) {
		printf("01234567890123456789012345678901234567890\r\n1\r\n2\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9");
		dumb_gocurs(5,10);
		printf("Drawing backwards from 10,20 to 10,0, using cursor_left");
		dumb_gocurs(10,20);

		for(i = 0; i < 20; i++) {
			printf("%d", i % 10);
			putp(cursor_left);
			putp(cursor_left);
		}

		printf("%d", i % 10);
		putp(cursor_left);
	}
	else printf("No cursor_left.\r\n");

	if (pause_for_input() == 'r') goto again;
}

tst_up()
{
    register int i;

again:
	clearscreen();
	if (cursor_up) {
		printf("01234567890123456789012345678901234567890\r\n");
	
		for(i=1; i<19; ) printf("%d\r\n",i++);
	
		dumb_gocurs(5, 25);

		if (cursor_left) printf("Drawing cursor_up from 18,20 to 1,20");
		else printf("Drawing cursor_up diagonally from 18,20 to 1,40");

		dumb_gocurs(18, 20);
		for(i = 0; i < 17; i++) {
			printf("%d", i % 10);
			if (cursor_left) putp(cursor_left);
			putp(cursor_up);
		}

		printf("%d", i % 10);

		if (cursor_left) putp(cursor_left);
	}
	else printf("No cursor_up.\r\n");

	if (pause_for_input() == 'r') goto again;
}

/* test horizontal positioning(column addressing) */
tst_hpa()
{
    register int i;

again:
	if (column_address) {
		clearscreen();
		printf("Drawing down column 10 using column_address");

		for(i = 1; i < lines; i++) {
			dumb_gocurs(i, 0);
			putp(tparm(column_address, 10));
			(void) putchar('x');
		}

		dumb_gocurs(lines - 1, 0);
		printf("0123456789");

		if (pause_for_input() == 'r') goto again;
	} 
}

/* test vertical positioning(row address) */
tst_vpa()
{
    register int i;

again:
	if (row_address) {
		clearscreen();
		dumb_gocurs(0, 5);
		printf("Drawing across row 10 using row_address");

		for(i = 0; i < columns; i++) {
			dumb_gocurs(0, i);
			putp(tparm(row_address, 10));
			(void) putchar('x');
		}

		for(i = 0; i < 10; ++i) {
			gocurs(i, 0);
			printf("%d", i);
		}

		if (pause_for_input() == 'r') goto again;
	}
}

/* test carriage return */
tst_cr()
{
again:
	clearscreen();
	if (carriage_return) {
		gocurs(10, 25);
		printf("Carriage");
		putp(carriage_return);
		printf("Return");
	}
	else printf("No carriage_return\n\r");
	if (pause_for_input() == 'r') goto again;
}

tst_ll()
{
again:
	if (cursor_to_ll) {
		clearscreen();
		gocurs(10, 25);
		printf("Going");
		putp(cursor_to_ll);
		printf("Home-DOWN");

		if (pause_for_input() == 'r') goto again;
	}
}

/* testing repeat char */
tst_rep()
{
again:
	if (repeat_char) {
		clearscreen();
		gocurs(0, 0);
		printf("testing repeat character");
		gocurs(1, 0);
		putp(tparm(repeat_char, '_', 20));
		gocurs(2, 0);
		printf("____________________");
		gocurs(3, 0);
		printf("the above two lines should be identical");

		if (pause_for_input() == 'r') goto again;
	}
}

/* testing erase chars */
tst_ech()
{
again:
	if (erase_chars) {
		clearscreen();
		gocurs(0, 0);
		printf("testing erase multiple characters");
		gocurs(1, 0);
		printf("012345678901234567890123456789");
		gocurs(1, 4);
		putp(tparm(erase_chars, 16));
		gocurs(2, 0);
		printf("0123                0123456789");
		gocurs(3, 0);
		printf("    1234567890123456");
		gocurs(4, 0);
		printf("the two lines should be identical, with 16 blanks");

		if (pause_for_input() == 'r') goto again;
	}
}

/* testing parm'd delete char */
tst_Pdch()
{
again:
	if (parm_dch) {
		clearscreen();
		gocurs(0, 0);
		printf("testing parm_dch(delete multiple characters)");
		gocurs(1, 0);
		printf("testing paaaaaaaaaaaaaaaaaaaaarm_dch(delete multiple characters)");
		gocurs(1, 9);
		putp(tparm(parm_dch, 20));
		gocurs(2, 0);
		printf("The above two lines should now be identical");

		if (pause_for_input() == 'r') goto again;
	}
}

/* test auto_right_margin and auto_left_margin */
tst_am()
{
    register char *ch;

again:
	if (auto_right_margin) {
		clearscreen();
		gocurs(5, columns - 7);
		printf("cursor wraps");
		gocurs(7, 0);
		printf("wraps under wraps");

		if (pause_for_input() == 'r') goto again;
	}

	if (auto_left_margin) {
		clearscreen();
		printf("auto left margin test");
		gocurs(7, 4);

		for(ch = "wrap left"; *ch; ch++) {
			(void) putchar(*ch);
			putp(cursor_left);
		}

		if (pause_for_input() == 'r') goto again;
	}
}

/* testing clear to end of line */
tst_eol()
{
again:
	if (clr_eol) {
		clearscreen();
		gocurs(10, 0);
		printf("clearing everything after clear");
		gocurs(11, 0);
		printf("clearing everything after clear");
		gocurs(10, 5);
		putp(clr_eol);

		if (pause_for_input() == 'r') goto again;
	}
}

/* ring the bell */
tst_bell()
{
again:
	if (bell) {
		clearscreen();
		printf("beep beep beep");
		putp(bell);
		sleep(1);
		putp(bell);
		sleep(1);
		putp(bell);

		if (pause_for_input() == 'r') goto again;
	}
}

/* flash the screen */
tst_flash()
{
again:
	if (flash_screen) {
		clearscreen();
		gocurs(lines / 2,(columns - 40) / 2);
		printf("press any key to flash the screen");
		(void) pause_for_input();
		putp(flash_screen);
		printf(" -- WOW!");
		gocurs(1 + lines / 2, 0);
		printf("the above line should read:");
		gocurs(2 + lines / 2,(columns - 40) / 2);
		printf("press any key to flash the screen -- WOW!");

		if (pause_for_input() == 'r') goto again;
	}
}

/* insert chars */
tst_ich()
{
	register int i;
	if (insert_character ||(enter_insert_mode && exit_insert_mode) ) {
		clearscreen();
		gocurs(10, 0);
		printf("Testing insert character.");

		if (insert_null_glitch) {
			printf("		These won't move unless pushed");
			printf("These move when you push them");
		}
		else printf("		These move too");

		printf("\r\nTyped characters will insert, ^G to end");
		gocurs(10, 4);

		if (enter_insert_mode) putp(enter_insert_mode);
		(void) fflush(stdout);

		if (manual) {
			while ((i = getchar()) != '') {
				if (insert_character) putp(insert_character);
				(void) putchar(i);
				if (insert_padding) putp(insert_padding);
				(void) fflush(stdout);
			}
		}
		else {
			if (insert_character) putp(insert_character);
			(void) putchar(' ');
			if (insert_padding) putp(insert_padding);
			(void) fflush(stdout);
		}

		if (exit_insert_mode) putp(exit_insert_mode);
	}
}

/* testing parm'd insert chars */
tst_Pich()
{
again:
	if (parm_ich) {
		clearscreen();
		gocurs(0, 0);
		printf("testing parm_ich(insert multiple characters)");
		gocurs(0, 7);

		if (enter_insert_mode) putp(enter_insert_mode);

		fflush(stdout);
		putp(tparm(parm_ich, 20));

		if (exit_insert_mode) putp(exit_insert_mode);

		gocurs(1, 0);
		printf("testing                     parm_ich(insert multiple characters)");
		gocurs(2, 0);
		printf("The above two lines should be identical");

		if (pause_for_input() == 'r') goto again;
	}
}

/* testing delete char */
tst_dch()
{
	register int i;

	if (delete_character) {
		clearscreen();
		gocurs(10, 0);

		printf("Testing delete character.");

		if (insert_null_glitch) {
		    	printf(" All of these characters, including the wrap to the new line, should move");
		    	printf("		These won't move");
		}
		else printf("		These move too");
	
		printf("\r\nType characters to delete, ^G to end");
		gocurs(10, 4);
	
		if (enter_delete_mode) putp(enter_delete_mode);
		(void) fflush(stdout);

		if (manual) {
			while ((i = getchar()) != '') {
				putp(delete_character);
				(void) fflush(stdout);
			}
		}
		else putp(delete_character);
		(void) fflush(stdout);
		
		if (exit_delete_mode) putp(exit_delete_mode);
	}
}

/* testing transparent underline */
tst_tul()
{
again:
    if (transparent_underline) {
		clearscreen();
		if (erase_overstrike) {
			gocurs(4, 0);
			uprint("Underlined_Characters");
			gocurs(6, 0);
			uprint("These won't wind up underlined");
			gocurs(6, 0);
			printf("These won't wind up underlined");
		}
		else {
			gocurs(6, 0);
			erase_overstrike = 1;
			uprint("...............................");
			erase_overstrike = 0;
			gocurs(6, 0);
			printf("These characters are underlined");
		}
		if (pause_for_input() == 'r') goto again;
	}
}

/* testing status line */
tst_sl()
{
    register int i;

again:
	if (has_status_line && to_status_line && from_status_line) {
		clearscreen();
		gocurs(0, 0);
		printf("Testing ");
		putp(tparm(to_status_line, 0));
		printf("Testing ");
		putp(from_status_line);
		printf("Status ");
		putp(tparm(to_status_line, 8));
		printf("Status ");
		putp(from_status_line);
		printf("Line");
		putp(tparm(to_status_line, 15));
		printf("Line");
		putp(from_status_line);
		if (pause_for_input() == 'r') goto again;
	
		clearscreen();
		gocurs(0, 0);
		printf("More status line testing(width=%d)", width_status_line);
		putp(tparm(to_status_line, 0));
		if (width_status_line < 0)
			width_status_line = columns;
		for(i = 0; i < width_status_line; ++i)
			printf("%1d", i % 10);
		putp(from_status_line);
		if (pause_for_input() == 'r') goto again;
	
		clearscreen();
		if (status_line_esc_ok && clr_eol) {
			gocurs(0, 0);
			printf("Testing status line escapes");
			putp(tparm(to_status_line, width_status_line / 2));
			putp(clr_eol);
			putp(from_status_line);
			gocurs(1, 0);
			printf("the last column remaining should be %d",
				(width_status_line / 2) - 2);
			if (pause_for_input() == 'r')
			    goto again;
			putp(tparm(to_status_line, 0));
			putp(clr_eol);
			putp(from_status_line);
			if (pause_for_input() == 'r') goto again;
		}
	
		if (dis_status_line) {
			clearscreen();
			gocurs(0, 0);
			printf("testing disable status line");
			putp(dis_status_line);
			if (pause_for_input() == 'r') goto again;
		}
	}
}

/* programming labels */
tst_pln()
{
    register int i, j, k;
    char label[512];

again:
	if (plab_norm) {
		clearscreen();
		gocurs(0, 0);
		printf("Programming function key labels:");
		gocurs(1, 0);
		printf("There should be %d labels which are %d rows high and %d columns wide",
			num_labels, label_height, label_width);
	
		for(i = 1; i <= num_labels; ++i) {
			for(j = 0; j < label_height; ++j)
			for(k = 0; k < label_width; ++k)
				label[j * label_width + k] = '0' +(k % 10);
			label[label_width * label_height] = '\0';
			putp(tparm(plab_norm, i, label));
		}
	
		if (pause_for_input() == 'r') goto again;
	
		clearscreen();
		gocurs(0, 0);
		printf("clearing function labels");
		for(i = 1; i <= num_labels; ++i)
			putp(tparm(plab_norm, i, ""));
		if (pause_for_input() == 'r') goto again;
	}
}

/* programming the pf keys */
tst_pfk()
{
again:
	if (pkey_xmit) {
		clearscreen();
		gocurs(0, 0);
		printf("programming function key 1 to transmit 'who|wc'");
		putp(tparm(pkey_xmit, 1, "who|wc\n"));
		gocurs(1, 0);
		printf("You'll have to check this out later...");
		if (pause_for_input() == 'r') goto again;
	}
}
main(argc, argv)
int argc;
char **argv;
{
    char   *termtype;			/* the terminal type */
    char   *progname;
    char   *startingpoint = NULL;

	progname = argv[0];
	if (argc >= 2 && argv[1][0] == '-') {
		startingpoint = &argv[1][1];
		argc--;
		argv++;
	}

	if (argc == 2) termtype = argv[1];
	else if (argc == 1) termtype = getenv("TERM");
	else {
		fprintf(stderr, "Usage: %s [-start] [terminal-type]\r\n", progname);
		exit(1);
	}

	if (newterm(termtype, stdout, stdin) == NULL) {
		fprintf(stderr, "Cannot initialize terminal type %s!\r\n", argv[0], termtype);
		exit(2);
	}

	noecho();
	nonl();
	cbreak();

	(void) signal(SIGINT, onintr);
	(void) signal(SIGQUIT, onintr);
	(void) signal(SIGHUP, onintr);

	tst_CAenter();

	if (startingpoint != NULL) {
		if (strcmp(startingpoint, "a") == 0) {
			manual = 0;
			goto Ssanity;
		}
		if (strcmp(startingpoint, "sanity") == 0) goto Ssanity;
		if (strcmp(startingpoint, "clear") == 0) goto Sclear;
		if (strcmp(startingpoint, "home") == 0)	goto Shome;
		if (strcmp(startingpoint, "cup") == 0) goto Scup;
		if (strcmp(startingpoint, "eos") == 0) goto Seos;
		if (strcmp(startingpoint, "cuf") == 0) goto Scuf;
		if (strcmp(startingpoint, "cud") == 0) goto Scud;
		if (strcmp(startingpoint, "cul") == 0) goto Scul;
		if (strcmp(startingpoint, "cuu") == 0) goto Scuu;
		if (strcmp(startingpoint, "hpa") == 0) goto Shpa;
		if (strcmp(startingpoint, "vpa") == 0) goto Svpa;
		if (strcmp(startingpoint, "pculr") == 0) goto Spculr;
		if (strcmp(startingpoint, "pcuud") == 0) goto Spcuud;
		if (strcmp(startingpoint, "cr") == 0) goto Scr;
		if (strcmp(startingpoint, "ll") == 0) goto Sll;
		if (strcmp(startingpoint, "rep") == 0) goto Srep;
		if (strcmp(startingpoint, "ech") == 0) goto Sech;
		if (strcmp(startingpoint, "pdch") == 0) goto Spdch;
		if (strcmp(startingpoint, "am") == 0) goto Sam;
		if (strcmp(startingpoint, "eol") == 0) goto Seol;
		if (strcmp(startingpoint, "index") == 0) goto Sindex;
		if (strcmp(startingpoint, "csr") == 0)	goto Scsr;
		if (strcmp(startingpoint, "pindex") == 0) goto Spindex;
		if (strcmp(startingpoint, "bell") == 0) goto Sbell;
		if (strcmp(startingpoint, "flash") == 0) goto Sflash;
		if (strcmp(startingpoint, "ich") == 0)	goto Sich;
		if (strcmp(startingpoint, "pich") == 0) goto Spich;
		if (strcmp(startingpoint, "dch") == 0)	goto Sdch;
		if (strcmp(startingpoint, "idl") == 0)	goto Sidl;
		if (strcmp(startingpoint, "pidl") == 0) goto Spidl;
		if (strcmp(startingpoint, "tul") == 0) goto Stul;
		if (strcmp(startingpoint, "sl") == 0) goto Ssl;
		if (strcmp(startingpoint, "pln") == 0)	goto Spln;
		if (strcmp(startingpoint, "sgr") == 0)	goto Ssgr;
		if (strcmp(startingpoint, "pfk") == 0)	goto Spfk;
		fprintf(stderr, "%s: unknown starting point '%s'.\n\r",
		    progname, startingpoint);
	}

Ssanity:
    tst_sanity();
Sclear:
    tst_clear();
Shome:
    tst_home();
Scup:
    tst_CM();
Seos:
    tst_eos();

    /* clearscreen() can now be used safely. */

Scuf:
    tst_right();
Scud:
    tst_down();

    /* dumbgocurs() can now be used safely */

Scul:
    tst_left();
Scuu:
    tst_up();
Shpa:
    tst_hpa();
Svpa:
    tst_vpa();

Spculr:
    tst_Plr();
Spcuud:
    tst_Pud();

    /* gocurs() can now be used safely */

Scr:
    tst_cr();
Sll:
    tst_ll();
Srep:
    tst_rep();
Sech:
    tst_ech();
Spdch:
    tst_Pdch();
Sam:
    tst_am();
Seol:
    tst_eol();
Sindex:
    tst_scroll(0, lines - 1);
Scsr:
    tst_csr();
Spindex:
    tst_Pscroll(0, lines - 1);
Sbell:
    tst_bell();
Sflash:
    tst_flash();
Sich:
    tst_ich();
Spich:
    tst_Pich();
Sdch:
    tst_dch();
Sidl:
    tst_idline();
Spidl:
    tst_Pidline();
Stul:
    tst_tul();
Ssl:
    tst_sl();
Spln:
    tst_pln();
Ssgr:
    tst_attr();
Spfk:
    tst_pfk();

    onintr(0);
}
