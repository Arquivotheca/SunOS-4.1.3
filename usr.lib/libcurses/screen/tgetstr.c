/*	@(#)tgetstr.c 1.1 92/07/30 SMI	(1.8	3/6/83)	*/

/*
 * Simulation of termcap using terminfo.
 */

#include "curses.ext"

/* Make a 2 letter code into an integer we can switch on easily */
#define	two( s1, s2 )	(s1 + 256 * s2 )
#define	twostr( str )	two( *str, str[ 1 ] )

char *
tgetstr(id, area)
char *id, *area;
{
	char *rv;

	switch (twostr(id)) {
	case two('b','t'): rv = back_tab; break;
	case two('b','l'): rv = bell; break;
	case two('c','r'): rv = carriage_return; break;
	case two('c','s'): rv = change_scroll_region; break;
	case two('c','t'): rv = clear_all_tabs; break;
	case two('c','l'): rv = clear_screen; break;
	case two('c','e'): rv = clr_eol; break;
	case two('c','d'): rv = clr_eos; break;
	case two('c','h'): rv = column_address; break;
	case two('C','C'): rv = command_character; break;
	case two('c','m'): rv = cursor_address; break;
	case two('d','o'): rv = cursor_down; break;
	case two('h','o'): rv = cursor_home; break;
	case two('v','i'): rv = cursor_invisible; break;
	case two('l','e'): rv = cursor_left; break;
	case two('C','M'): rv = cursor_mem_address; break;
	case two('v','e'): rv = cursor_normal; break;
	case two('n','d'): rv = cursor_right; break;
	case two('l','l'): rv = cursor_to_ll; break;
	case two('u','p'): rv = cursor_up; break;
	case two('v','s'): rv = cursor_visible; break;
	case two('d','c'): rv = delete_character; break;
	case two('d','l'): rv = delete_line; break;
	case two('d','s'): rv = dis_status_line; break;
	case two('h','d'): rv = down_half_line; break;
	case two('a','s'): rv = enter_alt_charset_mode; break;
	case two('m','b'): rv = enter_blink_mode; break;
	case two('m','d'): rv = enter_bold_mode; break;
	case two('t','i'): rv = enter_ca_mode; break;
	case two('d','m'): rv = enter_delete_mode; break;
	case two('m','h'): rv = enter_dim_mode; break;
	case two('i','m'): rv = enter_insert_mode; break;
	case two('m','k'): rv = enter_secure_mode; break;
	case two('m','p'): rv = enter_protected_mode; break;
	case two('m','r'): rv = enter_reverse_mode; break;
	case two('s','o'): rv = enter_standout_mode; break;
	case two('u','s'): rv = enter_underline_mode; break;
	case two('e','c'): rv = erase_chars; break;
	case two('a','e'): rv = exit_alt_charset_mode; break;
	case two('m','e'): rv = exit_attribute_mode; break;
	case two('t','e'): rv = exit_ca_mode; break;
	case two('e','d'): rv = exit_delete_mode; break;
	case two('e','i'): rv = exit_insert_mode; break;
	case two('s','e'): rv = exit_standout_mode; break;
	case two('u','e'): rv = exit_underline_mode; break;
	case two('v','b'): rv = flash_screen; break;
	case two('f','f'): rv = form_feed; break;
	case two('f','s'): rv = from_status_line; break;
	case two('i','1'): rv = init_1string; break;
	case two('i','s'): rv = init_2string; break;
	case two('i','2'): rv = init_3string; break;
	case two('i','f'): rv = init_file; break;
	case two('i','c'): rv = insert_character; break;
	case two('a','l'): rv = insert_line; break;
	case two('i','p'): rv = insert_padding; break;
	case two('k','b'): rv = key_backspace; break;
	case two('k','a'): rv = key_catab; break;
	case two('k','C'): rv = key_clear; break;
	case two('k','t'): rv = key_ctab; break;
	case two('k','D'): rv = key_dc; break;
	case two('k','L'): rv = key_dl; break;
	case two('k','d'): rv = key_down; break;
	case two('k','M'): rv = key_eic; break;
	case two('k','E'): rv = key_eol; break;
	case two('k','S'): rv = key_eos; break;
	case two('k','0'): rv = key_f0; break;
	case two('k','1'): rv = key_f1; break;
	case two('k',';'): rv = key_f10; break;
	case two('k','2'): rv = key_f2; break;
	case two('k','3'): rv = key_f3; break;
	case two('k','4'): rv = key_f4; break;
	case two('k','5'): rv = key_f5; break;
	case two('k','6'): rv = key_f6; break;
	case two('k','7'): rv = key_f7; break;
	case two('k','8'): rv = key_f8; break;
	case two('k','9'): rv = key_f9; break;
	case two('k','h'): rv = key_home; break;
	case two('k','I'): rv = key_ic; break;
	case two('k','A'): rv = key_il; break;
	case two('k','l'): rv = key_left; break;
	case two('k','H'): rv = key_ll; break;
	case two('k','N'): rv = key_npage; break;
	case two('k','P'): rv = key_ppage; break;
	case two('k','r'): rv = key_right; break;
	case two('k','F'): rv = key_sf; break;
	case two('k','R'): rv = key_sr; break;
	case two('k','T'): rv = key_stab; break;
	case two('k','u'): rv = key_up; break;
	case two('k','e'): rv = keypad_local; break;
	case two('k','s'): rv = keypad_xmit; break;
	case two('l','0'): rv = lab_f0; break;
	case two('l','1'): rv = lab_f1; break;
	case two('l','a'): rv = lab_f10; break;
	case two('l','2'): rv = lab_f2; break;
	case two('l','3'): rv = lab_f3; break;
	case two('l','4'): rv = lab_f4; break;
	case two('l','5'): rv = lab_f5; break;
	case two('l','6'): rv = lab_f6; break;
	case two('l','7'): rv = lab_f7; break;
	case two('l','8'): rv = lab_f8; break;
	case two('l','9'): rv = lab_f9; break;
	case two('m','o'): rv = meta_off; break;
	case two('m','m'): rv = meta_on; break;
	case two('n','w'): rv = newline; break;
	case two('p','c'): rv = pad_char; break;
	case two('D','C'): rv = parm_dch; break;
	case two('D','L'): rv = parm_delete_line; break;
	case two('D','O'): rv = parm_down_cursor; break;
	case two('I','C'): rv = parm_ich; break;
	case two('S','F'): rv = parm_index; break;
	case two('A','L'): rv = parm_insert_line; break;
	case two('L','E'): rv = parm_left_cursor; break;
	case two('R','I'): rv = parm_right_cursor; break;
	case two('S','R'): rv = parm_rindex; break;
	case two('U','P'): rv = parm_up_cursor; break;
	case two('p','k'): rv = pkey_key; break;
	case two('p','l'): rv = pkey_local; break;
	case two('p','x'): rv = pkey_xmit; break;
	case two('p','s'): rv = print_screen; break;
	case two('p','f'): rv = prtr_off; break;
	case two('p','o'): rv = prtr_on; break;
	case two('r','p'): rv = repeat_char; break;
	case two('r','1'): rv = reset_1string; break;
	case two('r','2'): rv = reset_2string; break;
	case two('r','3'): rv = reset_3string; break;
	case two('r','f'): rv = reset_file; break;
	case two('r','c'): rv = restore_cursor; break;
	case two('c','v'): rv = row_address; break;
	case two('s','c'): rv = save_cursor; break;
	case two('s','f'): rv = scroll_forward; break;
	case two('s','r'): rv = scroll_reverse; break;
	case two('s','a'): rv = set_attributes; break;
	case two('s','t'): rv = set_tab; break;
	case two('w','i'): rv = set_window; break;
	case two('t','a'): rv = tab; break;
	case two('t','s'): rv = to_status_line; break;
	case two('u','c'): rv = underline_char; break;
	case two('h','u'): rv = up_half_line; break;
	case two('i','P'): rv = init_prog; break;
	case two('K','1'): rv = key_a1; break;
	case two('K','3'): rv = key_a3; break;
	case two('K','2'): rv = key_b2; break;
	case two('K','4'): rv = key_c1; break;
	case two('K','5'): rv = key_c3; break;
	case two('p','O'): rv = prtr_non; break;
	default: rv = NULL;
	}
	return rv;
}
