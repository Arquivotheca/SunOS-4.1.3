/*	@(#)ti4.c 1.1 92/07/30 SMI	*/
/*
 * ti4 [term]
 * dummy program to test termlib.
 * gets entry, counts it, and prints it.
 */
#include <stdio.h>
#include "curses.h"
#include "term.h"

#define prb(name)	if (name) printf("name\n")
#define prn(name)	if (name != -1) printf("name = %d\n", name)
#define prs(name)	if (name) {printf("name = '"); pr(name); printf("'\n");}

char buf[1024];
char *getenv();

main(argc, argv) char **argv; {
	char *p;
	int rc;

	if (argc < 2)
		p = getenv("TERM");
	else
		p = argv[1];
	printf("Terminal type %s\n", p);
	setupterm(p,1,0);

	printf("flags\n");

	    prb(auto_left_margin) ;
	    prb(auto_right_margin) ;
	    prb(beehive_glitch) ;
	    prb(ceol_standout_glitch) ;
	    prb(eat_newline_glitch) ;
	    prb(erase_overstrike) ;
	    prb(generic_type) ;
	    prb(hard_copy) ;
	    prb(has_meta_key) ;
	    prb(has_status_line) ;
	    prb(insert_null_glitch) ;
	    prb(memory_above) ;
	    prb(memory_below) ;
	    prb(move_insert_mode) ;
	    prb(move_standout_mode) ;
	    prb(over_strike) ;
	    prb(status_line_esc_ok) ;
	    prb(teleray_glitch) ;
	    prb(tilde_glitch) ;
	    prb(transparent_underline) ;
	    prb(xon_xoff) ;

	printf("\nnumbers\n");

	    prn(columns) ;
	    prn(init_tabs) ;
	    prn(lines) ;
	    prn(lines_of_memory) ;
	    prn(magic_cookie_glitch) ;
	    prn(padding_baud_rate) ;
	    prn(virtual_terminal) ;
	    prn(width_status_line) ;

	printf("\nstrings\n");

	    prs(back_tab) ;
	    prs(bell) ;
	    prs(carriage_return) ;
	    prs(change_scroll_region) ;
	    prs(clear_all_tabs) ;
	    prs(clear_screen) ;
	    prs(clr_eol) ;
	    prs(clr_eos) ;
	    prs(column_address) ;
	    prs(command_character) ;
	    prs(cursor_address) ;
	    prs(cursor_down) ;
	    prs(cursor_home) ;
	    prs(cursor_invisible) ;
	    prs(cursor_left) ;
	    prs(cursor_mem_address) ;
	    prs(cursor_normal) ;
	    prs(cursor_right) ;
	    prs(cursor_to_ll) ;
	    prs(cursor_up) ;
	    prs(cursor_visible) ;
	    prs(delete_character) ;
	    prs(delete_line) ;
	    prs(dis_status_line) ;
	    prs(down_half_line) ;
	    prs(enter_alt_charset_mode) ;
	    prs(enter_blink_mode) ;
	    prs(enter_bold_mode) ;
	    prs(enter_ca_mode) ;
	    prs(enter_delete_mode) ;
	    prs(enter_dim_mode) ;
	    prs(enter_insert_mode) ;
	    prs(enter_secure_mode) ;
	    prs(enter_protected_mode) ;
	    prs(enter_reverse_mode) ;
	    prs(enter_standout_mode) ;
	    prs(enter_underline_mode) ;
	    prs(erase_chars) ;
	    prs(exit_alt_charset_mode) ;
	    prs(exit_attribute_mode) ;
	    prs(exit_ca_mode) ;
	    prs(exit_delete_mode) ;
	    prs(exit_insert_mode) ;
	    prs(exit_standout_mode) ;
	    prs(exit_underline_mode) ;
	    prs(flash_screen) ;
	    prs(form_feed) ;
	    prs(from_status_line) ;
	    prs(init_1string) ;
	    prs(init_2string) ;
	    prs(init_3string) ;
	    prs(init_file) ;
	    prs(insert_character) ;
	    prs(insert_line) ;
	    prs(insert_padding) ;
	    prs(key_backspace) ;
	    prs(key_catab) ;
	    prs(key_clear) ;
	    prs(key_ctab) ;
	    prs(key_dc) ;
	    prs(key_dl) ;
	    prs(key_down) ;
	    prs(key_eic) ;
	    prs(key_eol) ;
	    prs(key_eos) ;
	    prs(key_f0) ;
	    prs(key_f1) ;
	    prs(key_f10) ;
	    prs(key_f2) ;
	    prs(key_f3) ;
	    prs(key_f4) ;
	    prs(key_f5) ;
	    prs(key_f6) ;
	    prs(key_f7) ;
	    prs(key_f8) ;
	    prs(key_f9) ;
	    prs(key_home) ;
	    prs(key_ic) ;
	    prs(key_il) ;
	    prs(key_left) ;
	    prs(key_ll) ;
	    prs(key_npage) ;
	    prs(key_ppage) ;
	    prs(key_right) ;
	    prs(key_sf) ;
	    prs(key_sr) ;
	    prs(key_stab) ;
	    prs(key_up) ;
	    prs(keypad_local) ;
	    prs(keypad_xmit) ;
	    prs(lab_f0) ;
	    prs(lab_f1) ;
	    prs(lab_f10) ;
	    prs(lab_f2) ;
	    prs(lab_f3) ;
	    prs(lab_f4) ;
	    prs(lab_f5) ;
	    prs(lab_f6) ;
	    prs(lab_f7) ;
	    prs(lab_f8) ;
	    prs(lab_f9) ;
	    prs(meta_off) ;
	    prs(meta_on) ;
	    prs(newline) ;
	    prs(pad_char) ;
	    prs(parm_dch) ;
	    prs(parm_delete_line) ;
	    prs(parm_down_cursor) ;
	    prs(parm_ich) ;
	    prs(parm_index) ;
	    prs(parm_insert_line) ;
	    prs(parm_left_cursor) ;
	    prs(parm_right_cursor) ;
	    prs(parm_rindex) ;
	    prs(parm_up_cursor) ;
	    prs(pkey_key) ;
	    prs(pkey_local) ;
	    prs(pkey_xmit) ;
	    prs(print_screen) ;
	    prs(prtr_off) ;
	    prs(prtr_on) ;
	    prs(repeat_char) ;
	    prs(reset_1string) ;
	    prs(reset_2string) ;
	    prs(reset_3string) ;
	    prs(reset_file) ;
	    prs(restore_cursor) ;
	    prs(row_address) ;
	    prs(save_cursor) ;
	    prs(scroll_forward) ;
	    prs(scroll_reverse) ;
	    prs(set_attributes) ;
	    prs(set_tab) ;
	    prs(set_window) ;
	    prs(tab) ;
	    prs(to_status_line) ;
	    prs(underline_char) ;
	    prs(up_half_line) ;
	    prs(init_prog) ;
	    prs(key_a1) ;
	    prs(key_a3) ;
	    prs(key_b2) ;
	    prs(key_c1) ;
	    prs(key_c3) ;
	    prs(prtr_non) ;

	printf("end of strings\n");
	reset_shell_mode();
	exit(0);
}

pr(p)
register char *p;
{
	char *rdchar();

	for (; *p; p++)
		printf("%s", rdchar(*p));
}

/*
 * rdchar: returns a readable representation of an ASCII char, using ^ notation.
 */
#include <ctype.h>
char *rdchar(c)
char c;
{
	static char ret[4];
	register char *p;

	/*
	 * Due to a bug in isprint, this prints spaces as ^`, but this is OK
	 * because we want something to show up on the screen.
	 */
	ret[0] = ((c&0377) > 0177) ? '\'' : ' ';
	c &= 0177;
	ret[1] = isprint(c) ? ' ' : '^';
	ret[2] = isprint(c) ?  c  : c^0100;
	ret[3] = 0;
	for (p=ret; *p==' '; p++)
		;
	return (p);
}
