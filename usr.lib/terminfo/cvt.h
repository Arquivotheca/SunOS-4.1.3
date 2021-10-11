/*	@(#)cvt.h 1.1 92/07/30 SMI; from S5R3 1.1	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * This set of #defines, generated from the "caps" file, is intended as
 * a conversion aid from termcap to terminfo.  The two letter names are
 * likely variable names used for termcap - they can be edited as needed
 * for your own variable names.  This makes it unnecessary to run a big
 * sed script to change all the actual names in your program.
 *
 * If you are not very short on time or have only a dozen or so capabilities
 * in use, it is probably worthwhile to just change all the names.
 * This will make your program more maintainable in the long run.
 */

#define	AM	auto_right_margin
#define	BW	auto_left_margin
#define	DA	memory_above
#define	DB	memory_below
#define	EO	erase_overstrike
#define	ES	status_line_esc_ok
#define	GN	generic_type
#define	HC	hard_copy
#define	HS	has_status_line
#define	HZ	tilde_glitch
#define	IN	insert_null_glitch
#define	KM	has_meta_key
#define	MI	move_insert_mode
#define	MS	move_standout_mode
#define	OS	over_strike
#define	PT	hardware_tabs
#define	UL	transparent_underline
#define	XB	beehive_glitch
#define	XN	eat_newline_glitch
#define	XO	xon_xoff
#define	XS	ceol_standout_glitch
#define	XT	teleray_glitch
#define	CO	columns
#define	DBS	backspace_delay
#define	DCR	return_delay
#define	DFF	formfeed_delay
#define	DNL	newline_delay
#define	DTAB	tab_delay
#define	LI	lines
#define	LM	lines_of_memory
#define	PB	padding_baud_rate
#define	SG	magic_cookie_glitch
#define	VT	virtual_terminal
#define	AL_PARM	parm_insert_line
#define	CMDCHAR	command_character
#define	CMMEM	cursor_mem_address
#define	DC_PARM	parm_dch
#define	DL_PARM	parm_delete_line
#define	DOWN_PARM	parm_down_cursor
#define	IC_PARM	parm_ich
#define	LEFT_PARM	parm_left_cursor
#define	RIGHT_PARM	parm_right_cursor
#define	UP_PARM	parm_up_cursor
#define	AE	exit_alt_charset_mode
#define	AL	insert_line
#define	AS	enter_alt_charset_mode
#define	BL	bell
#define	BT	back_tab
#define	CD	clr_eos
#define	CE	clr_eol
#define	CH	column_address
#define	CL	clear_screen
#define	CM	cursor_address
#define	CR	carriage_return
#define	CS	change_scroll_region
#define	CT	clear_all_tabs
#define	CV	row_address
#define	DC	delete_character
#define	DL	delete_line
#define	DM	enter_delete_mode
#define	DO	cursor_down
#define	DS	dis_status_line
#define	EC	erase_chars
#define	ED	exit_delete_mode
#define	EI	exit_insert_mode
#define	FF	form_feed
#define	FS	from_status_line
#define	HD	down_half_line
#define	HO	cursor_home
#define	HU	up_half_line
#define	I1	init_1string
#define	I2	init_2string
#define	I3	init_3string
#define	IC	insert_character
#define	IF	init_file
#define	IM	enter_insert_mode
#define	IP	insert_padding
#define	K0	key_f0
#define	K1	key_f1
#define	K2	key_f2
#define	K3	key_f3
#define	K4	key_f4
#define	K5	key_f5
#define	K6	key_f6
#define	K7	key_f7
#define	K8	key_f8
#define	K9	key_f9
#define	K10	key_f10
#define	KIL	key_il
#define	KCLEAR	key_clear
#define	KDC	key_dc
#define	KEOL	key_eol
#define	KSF	key_sf
#define	KIC	key_ic
#define	KDL	key_dl
#define	KEIC	key_eic
#define	KNPAGE	key_npage
#define	KPPAGE	key_ppage
#define	KSR	key_sr
#define	KEOS	key_eos
#define	KSTAB	key_stab
#define	KA	key_catab
#define	KB	key_backspace
#define	KD	key_down
#define	KE	keypad_local
#define	KH	key_home
#define	KL	key_left
#define	KR	key_right
#define	KS	keypad_xmit
#define	KT	key_ctab
#define	KU	key_up
#define	L0	lab_f0
#define	L1	lab_f1
#define	L2	lab_f2
#define	L3	lab_f3
#define	L4	lab_f4
#define	L5	lab_f5
#define	L6	lab_f6
#define	L7	lab_f7
#define	L8	lab_f8
#define	L9	lab_f9
#define	LA	lab_f10
#define	LE	cursor_left
#define	LL	cursor_to_ll
#define	MB	enter_blink_mode
#define	MD	enter_bold_mode
#define	ME	exit_attribute_mode
#define	MH	enter_dim_mode
#define	MK	enter_curs_invis_mode
#define	MM	meta_on
#define	MO	meta_off
#define	MP	enter_protected_mode
#define	MR	enter_reverse_mode
#define	ND	cursor_right
#define	NW	newline
#define	PC	pad_char
#define	PF	prtr_off
#define	PK	pkey_key
#define	PL	pkey_local
#define	PO	prtr_on
#define	PS	print_screen
#define	PX	pkey_xmit
#define	R1	reset_1string
#define	R2	reset_2string
#define	R3	reset_3string
#define	RC	restore_cursor
#define	RF	reset_file
#define	RP	repeat_char
#define	SA	set_attributes
#define	SC	save_cursor
#define	SE	exit_standout_mode
#define	SF	scroll_forward
#define	SO	enter_standout_mode
#define	SR	scroll_reverse
#define	ST	set_tab
#define	TA	tab
#define	TE	exit_ca_mode
#define	TI	enter_ca_mode
#define	TS	to_status_line
#define	UC	underline_char
#define	UE	exit_underline_mode
#define	UP	cursor_up
#define	US	enter_underline_mode
#define	VB	flash_screen
#define	VE	cursor_normal
#define	VI	cursor_invisible
#define	VS	cursor_visible
#define	WI	set_window
