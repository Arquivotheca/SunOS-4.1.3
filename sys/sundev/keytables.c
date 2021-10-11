#ifndef lint
static  char sccsid[] = "@(#)keytables.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (C) 1987 by Sun Microsystems, Inc.
 */

/*
 * keytables.c
 *
 * This module contains the translation tables for the up-down encoded
 * Sun keyboards.
 */
#include <sys/types.h>
#include <sundev/kbd.h>

/* handy way to define control characters in the tables */
#define	c(char)	(char&0x1F)
#define ESC 0x1B
#define	DEL 0x7F


#ifdef sun2
/* Unshifted keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(2),	LF(3),	HOLE,	TF(1),	TF(2),	TF(3),
/*  8 */	TF(4), 	TF(5), 	TF(6),	TF(7),	TF(8),	TF(9),	TF(10),	TF(11),
/* 16 */	TF(12), TF(13), TF(14),	ESC,	HOLE,	RF(1),	'+',	'-',
/* 24 */	HOLE, 	LF(4), 	'\f',	LF(6),	HOLE,	SHIFTKEYS+CAPSLOCK,
								'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'~',	'`',	'\b',	HOLE,	'7',	'8',	'9',
/* 48 */	HOLE,	LF(7),	STRING+UPARROW,
					LF(9),	HOLE,	'\t',	'q',	'w',
/* 56 */	'e',	'r',	't',	'y',	'u',	'i',	'o',	'p',
/* 64 */	'{',	'}',	'_',	HOLE,	'4',	'5',	'6',	HOLE,
/* 72 */	STRING+LEFTARROW,
			STRING+HOMEARROW,
				STRING+RIGHTARROW,
					HOLE,	SHIFTKEYS+SHIFTLOCK,
							'a', 	's',	'd',
/* 80 */	'f',	'g',	'h',	'j',	'k',	'l',	';',	':',
/* 88 */	'|',	'\r',	HOLE,	'1',	'2',	'3',	HOLE,	NOSCROLL,
/* 96 */	STRING+DOWNARROW,
			LF(15),	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							'z',	'x',	'c',
/*104 */	'v',	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	DEL,	'0',	NOP,	'.',	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					' ',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};

/* Shifted keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(2),	LF(3),	HOLE,	TF(1),	TF(2),	TF(3),
/*  8 */	TF(4), 	TF(5), 	TF(6),	TF(7),	TF(8),	TF(9),	TF(10),	TF(11),
/* 16 */	TF(12), TF(13), TF(14),	ESC,	HOLE,	RF(1),	'+',	'-',
/* 24 */	HOLE, 	LF(4), 	'\f',	LF(6),	HOLE,	SHIFTKEYS+CAPSLOCK,
								'!',	'"',
/* 32 */	'#',	'$',	'%',	'&',	'\'',	'(',	')',	'0',
/* 40 */	'=',	'^',	'@',	'\b',	HOLE,	'7',	'8',	'9',
/* 48 */	HOLE,	LF(7),	STRING+UPARROW,
					LF(9),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'[',	']',	'_',	HOLE,	'4',	'5',	'6',	HOLE,
/* 72 */	STRING+LEFTARROW,
			STRING+HOMEARROW,
				STRING+RIGHTARROW,
					HOLE,	SHIFTKEYS+SHIFTLOCK,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	'+',	'*',
/* 88 */	'\\',	'\r',	HOLE,	'1',	'2',	'3',	HOLE,	NOSCROLL,
/* 96 */	STRING+DOWNARROW,
			LF(15),	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							'Z',	'X',	'C',
/*104 */	'V',	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	DEL,	'0',	NOP,	'.',	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					' ',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};


/* Caps Locked keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(2),	LF(3),	HOLE,	TF(1),	TF(2),	TF(3),
/*  8 */	TF(4), 	TF(5), 	TF(6),	TF(7),	TF(8),	TF(9),	TF(10),	TF(11),
/* 16 */	TF(12), TF(13), TF(14),	ESC,	HOLE,	RF(1),	'+',	'-',
/* 24 */	HOLE, 	LF(4), 	'\f',	LF(6),	HOLE,	SHIFTKEYS+CAPSLOCK,
								'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'~',	'`',	'\b',	HOLE,	'7',	'8',	'9',
/* 48 */	HOLE,	LF(7),	STRING+UPARROW,
	 				LF(9),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'{',	'}',	'_',	HOLE,	'4',	'5',	'6',	HOLE,
/* 72 */	STRING+LEFTARROW,
			STRING+HOMEARROW,
				STRING+RIGHTARROW,
					HOLE,	SHIFTKEYS+SHIFTLOCK,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	';',	':',
/* 88 */	'|',	'\r',	HOLE,	'1',	'2',	'3',	HOLE,	NOSCROLL,
/* 96 */	STRING+DOWNARROW,
			LF(15),	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							'Z',	'X',	'C',
/*104 */	'V',	'B',	'N',	'M',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	DEL,	'0',	NOP,	'.',	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					' ',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};

/* Controlled keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(2),	LF(3),	HOLE,	TF(1),	TF(2),	TF(3),
/*  8 */	TF(4), 	TF(5), 	TF(6),	TF(7),	TF(8),	TF(9),	TF(10),	TF(11),
/* 16 */	TF(12), TF(13), TF(14),	ESC,	HOLE,	RF(1),	OOPS, OOPS,
/* 24 */	HOLE, 	LF(4), 	'\f',	LF(6),	HOLE,	SHIFTKEYS+CAPSLOCK,
								OOPS,	OOPS,
/* 32 */	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,	OOPS,
/* 40 */	OOPS,	c('^'),	c('@'),	'\b',	HOLE,	OOPS,	OOPS,	OOPS,
/* 48 */	HOLE,	LF(7),	STRING+UPARROW,
					LF(9),	HOLE,	'\t',	CTRLQ,	c('W'),
/* 56 */	c('E'),	c('R'),	c('T'),	c('Y'),	c('U'),	c('I'),	c('O'),	c('P'),
/* 64 */	c('['),	c(']'),	c('_'),	HOLE,	OOPS,	OOPS,	OOPS,	HOLE,
/* 72 */	STRING+LEFTARROW,
			STRING+HOMEARROW,
				STRING+RIGHTARROW,
					HOLE,	SHIFTKEYS+SHIFTLOCK,
							c('A'),	CTRLS,	c('D'),
/* 80 */	c('F'),	c('G'),	c('H'),	c('J'),	c('K'),	c('L'),	OOPS,	OOPS,
/* 88 */	c('\\'),
			'\r',	HOLE,	OOPS,	OOPS,	OOPS,	HOLE,	NOSCROLL,
/* 96 */	STRING+DOWNARROW,
			LF(15),	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							c('Z'),	c('X'),	c('C'),
/*104 */	c('V'),	c('B'),	c('N'),	c('M'),	OOPS,	OOPS,	OOPS,	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	DEL,	OOPS,	NOP,	OOPS,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					'\0',	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	IDLE,
};


/* "Key Up" keyboard table for Micro Switch 103SD32-2 */

static struct keymap keytab_ms_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/*  8 */	NOP, 	NOP, 	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 16 */	NOP, 	NOP, 	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 24 */	HOLE, 	NOP, 	NOP,	NOP,	HOLE,	SHIFTKEYS+CAPSLOCK,
								NOP,	NOP,
/* 32 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 48 */	HOLE,	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 56 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,	HOLE,
/* 72 */	NOP,	NOP,	NOP,	HOLE,	SHIFTKEYS+SHIFTLOCK,
							NOP, 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,	HOLE,	NOP,
/* 96 */	NOP,	NOP,	HOLE,	HOLE,	SHIFTKEYS+LEFTSHIFT,
							NOP,	NOP,	NOP,
/*104 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+RIGHTSHIFT,
/*112 */	NOP,	NOP,	NOP,	NOP,	NOP,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
					NOP,	SHIFTKEYS+RIGHTCTRL,
							HOLE,	HOLE,	RESET,
};


/* Index to keymaps for Micro Switch 103SD32-2 */
static struct keyboard keyindex_ms = {
	&keytab_ms_lc,
	&keytab_ms_uc,
	&keytab_ms_cl,
	0,		/* no Alt Graph key, no Alt Graph table */
	0,		/* no Num Lock key, no Num Lock table */
	&keytab_ms_ct,
	&keytab_ms_up,
	CTLSMASK,	/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
	1,	77,	/* abort keys */
	0x0000,		/* Shift bits which toggle on down event */
};

#endif /*sun2*/

/* Unshifted keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(11),	LF(2),	HOLE,	TF(1),	TF(2),	TF(11),
/*  8 */	TF(3), 	TF(12),	TF(4),	TF(13),	TF(5),	TF(14),	TF(6),	TF(15),
/* 16 */	TF(7),	TF(8),	TF(9),	TF(10),	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	LF(12),	HOLE,	ESC,	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	LF(13),	LF(6),	HOLE,	'\t',	'q',	'w',
/* 56 */	'e',	'r',	't',	'y',	'u',	'i',	'o',	'p',
/* 64 */	'[',	']',	DEL,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							'a', 	's',	'd',
/* 80 */	'f',	'g',	'h',	'j',	'k',	'l',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'z',	'x',	'c',	'v',
/*104 */	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Shifted keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(11),	LF(2),	HOLE,	TF(1),	TF(2),	TF(11),
/*  8 */	TF(3), 	TF(12),	TF(4),	TF(13),	TF(5),	TF(14),	TF(6),	TF(15),
/* 16 */	TF(7),	TF(8),	TF(9),	TF(10),	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	LF(12),	HOLE,	ESC,	'!',    '@',
/* 32 */	'#',	'$',	'%',	'^',	'&',	'*',	'(',	')',
/* 40 */	'_',	'+',	'~',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	LF(13),	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'{',	'}',	DEL,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	':',	'"',
/* 88 */	'|',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};


/* Caps Locked keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(11),	LF(2),	HOLE,	TF(1),	TF(2),	TF(11),
/*  8 */	TF(3), 	TF(12),	TF(4),	TF(13),	TF(5),	TF(14),	TF(6),	TF(15),
/* 16 */	TF(7),	TF(8),	TF(9),	TF(10),	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	LF(12),	HOLE,	ESC,	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	LF(13),	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'[',	']',	DEL,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Controlled keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				LF(11),	LF(2),	HOLE,	TF(1),	TF(2),	TF(11),
/*  8 */	TF(3), 	TF(12),	TF(4),	TF(13),	TF(5),	TF(14),	TF(6),	TF(15),
/* 16 */	TF(7),	TF(8),	TF(9),	TF(10),	HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	LF(12),	HOLE,	ESC,	'1',	c('@'),
/* 32 */	'3',	'4',	'5',	c('^'),	'7',	'8',	'9',	'0',
/* 40 */	c('_'),	'=',	c('^'),	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	LF(13),	LF(6),	HOLE,	'\t',   c('q'),	c('w'),
/* 56 */	c('e'),	c('r'),	c('t'),	c('y'),	c('u'),	c('i'),	c('o'),	c('p'),
/* 64 */	c('['),	c(']'),	DEL,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	LF(14),	HOLE,	SHIFTKEYS+LEFTCTRL,
							c('a'),	c('s'),	c('d'),
/* 80 */	c('f'),	c('g'),	c('h'),	c('j'),	c('k'),	c('l'),	';',	'\'',
/* 88 */	c('\\'),
			'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	LF(15),	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						c('z'),	c('x'),	c('c'),	c('v'),
/*104 */	c('b'),	c('n'),	c('m'),	',',	'.',	c('_'),	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			c(' '),	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};



/* "Key Up" keyboard table for Sun-2 keyboard */

static struct keymap keytab_s2_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/*  8 */	NOP, 	NOP, 	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 16 */	NOP, 	NOP, 	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 24 */	HOLE, 	NOP, 	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 32 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 48 */	HOLE,	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 56 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,	HOLE,
/* 72 */	NOP,	NOP,	NOP,	HOLE,	SHIFTKEYS+LEFTCTRL,
							NOP, 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,	HOLE,	NOP,
/* 96 */	NOP,	NOP,	HOLE,	SHIFTKEYS+LEFTSHIFT,
						NOP,	NOP,	NOP,	NOP,
/*104 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+RIGHTSHIFT,
									NOP,
/*112 */	NOP,	NOP,	NOP,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	BUCKYBITS+METABIT,
			NOP,	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	HOLE,	RESET,
};

/* Index to keymaps for Sun-2 keyboard */
static struct keyboard keyindex_s2 = {
	&keytab_s2_lc,
	&keytab_s2_uc,
	&keytab_s2_cl,
	0,		/* no Alt Graph key, no Alt Graph table */
	0,		/* no Num Lock key, no Num Lock table */
	&keytab_s2_ct,
	&keytab_s2_up,
	CAPSMASK,	/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
	1,	77,	/* abort keys */
	0x0000,		/* Shift bits which toggle on down event */
};

#ifdef sun2

/* Unshifted keyboard table for "VT100 style" */

static struct keymap keytab_vt_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								HOLE,	TF(1),
/* 16 */	TF(2),	TF(3),	TF(4),	ESC,	'1',	'2',	'3',	'4',
/* 24 */	'5', 	'6', 	'7',	'8',	'9',	'0',	'-',	'=',
/* 32 */	'`',	c('H'),	BUCKYBITS+METABIT,
					'7',	'8',	'9',	'-',	'\t',
/* 40 */	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
/* 48 */	'o',	'p',	'[',	']',	DEL,	'4',	'5',	'6',
/* 56 */	',',	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					'a',	's',	'd',	'f',	'g',
/* 64 */	'h',	'j',	'k',	'l',	';',	'\'',	'\r',	'\\',
/* 72 */	'1',	'2',	'3',	NOP,	NOSCROLL,
							SHIFTKEYS+LEFTSHIFT,
							 	'z',	'x',
/* 80 */	'c',	'v',	'b',	'n',	'm',	',',	'.',	'/',
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			'\n',	'0',	HOLE,	'.',	'\r',	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	' ',	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	IDLE,
};


/* Shifted keyboard table for "VT100 style" */

static struct keymap keytab_vt_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								HOLE,	TF(1),
/* 16 */	TF(2),	TF(3),	TF(4),	ESC,	'!',	'@',	'#',	'$',
/* 24 */	'%', 	'^', 	'&',	'*',	'(',	')',	'_',	'+',
/* 32 */	'~',	c('H'),	BUCKYBITS+METABIT,
					'7',	'8',	'9',	'-',	'\t',
/* 40 */	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
/* 48 */	'O',	'P',	'{',	'}',	DEL,	'4',	'5',	'6',
/* 56 */	',',	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					'A',	'S',	'D',	'F',	'G',
/* 64 */	'H',	'J',	'K',	'L',	':',	'"',	'\r',	'|',
/* 72 */	'1',	'2',	'3',	NOP,	NOSCROLL,
							SHIFTKEYS+LEFTSHIFT,
							 	'Z',	'X',
/* 80 */	'C',	'V',	'B',	'N',	'M',	'<',	'>',	'?',
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			'\n',	'0',	HOLE,	'.',	'\r',	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	' ',	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	IDLE,
};

/* Caps Locked keyboard table for "VT100 style" */

static struct keymap keytab_vt_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								HOLE,	TF(1),
/* 16 */	TF(2),	TF(3),	TF(4),	ESC,	'1',	'2',	'3',	'4',
/* 24 */	'5', 	'6', 	'7',	'8',	'9',	'0',	'-',	'=',
/* 32 */	'`',	c('H'),	BUCKYBITS+METABIT,
					'7',	'8',	'9',	'-',	'\t',
/* 40 */	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
/* 48 */	'O',	'P',	'[',	']',	DEL,	'4',	'5',	'6',
/* 56 */	',',	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					'A',	'S',	'D',	'F',	'G',
/* 64 */	'H',	'J',	'K',	'L',	';',	'\'',	'\r',	'\\',
/* 72 */	'1',	'2',	'3',	NOP,	NOSCROLL,
							SHIFTKEYS+LEFTSHIFT,
							 	'Z',	'X',
/* 80 */	'C',	'V',	'B',	'N',	'M',	',',	'.',	'/',
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			'\n',	'0',	HOLE,	'.',	'\r',	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	' ',	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	IDLE,
};

/* Controlled keyboard table for "VT100 style" */

static struct keymap keytab_vt_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	STRING+UPARROW,
					STRING+DOWNARROW,
						STRING+LEFTARROW,
							STRING+RIGHTARROW,
								HOLE,	TF(1),
/* 16 */	TF(2),	TF(3),	TF(4),	ESC,	'1',	c('@'),	'3',	'4',
/* 24 */	'5',	c('^'),	'7',	'8',	'9',	'0',	c('_'),	'=',
/* 32 */	c('^'),	c('H'),	BUCKYBITS+METABIT,
					'7',	'8',	'9',	'-',	'\t',
/* 40 */	CTRLQ,	c('W'),	c('E'),	c('R'),	c('T'),	c('Y'),	c('U'),	c('I'),
/* 48 */	c('O'),	c('P'),	c('['),	c(']'),	DEL,	'4',	'5',	'6',
/* 56 */	',',	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					c('A'),	CTRLS,	c('D'),	c('F'),	c('G'),
/* 64 */	c('H'),	c('J'),	c('K'),	c('L'),	':',	'"',	'\r',	c('\\'),
/* 72 */	'1',	'2',	'3',	NOP,	NOSCROLL,
							SHIFTKEYS+LEFTSHIFT,
							 	c('Z'),	c('X'),
/* 80 */	c('C'),	c('V'),	c('B'),	c('N'),	c('M'),	',',	'.',	c('_'),
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			'\n',	'0',	HOLE,	'.',	'\r',	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	c(' '),	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	IDLE,
};


/* "Key Up" keyboard table for "VT100 style" */

static struct keymap keytab_vt_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*  8 */	HOLE, 	HOLE, 	NOP,	NOP,	NOP,	NOP,	HOLE,	NOP,
/* 16 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 24 */	NOP, 	NOP, 	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 32 */	NOP,	NOP,	BUCKYBITS+METABIT,
					NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 48 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 56 */	NOP,	SHIFTKEYS+LEFTCTRL,
				SHIFTKEYS+CAPSLOCK,
					NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 72 */	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+LEFTSHIFT,
							 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	SHIFTKEYS+RIGHTSHIFT,
			NOP,	NOP,	HOLE,	NOP,	NOP,	HOLE,	HOLE,
/* 96 */	HOLE,	HOLE,	NOP,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*104 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*112 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,
/*120 */	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	HOLE,	RESET,
};


/* Index to keymaps for "VT100 style" keyboard */
static struct keyboard keyindex_vt = {
	&keytab_vt_lc,
	&keytab_vt_uc,
	&keytab_vt_cl,
	0,		/* no Alt Graph key, no Alt Graph table */
	0,		/* no Num Lock key, no Num Lock table */
	&keytab_vt_ct,
	&keytab_vt_up,
	CAPSMASK+CTLSMASK, 	/* Shift keys that stay on at idle keyboard */
	0x0000,		/* Bucky bits that stay on at idle keyboard */
	1,	59,	/* abort keys */
	0x0000,		/* Shift bits which toggle on down event */
};

#endif /*sun2*/

/* Unshifted keyboard table for Type 3 keyboard */

static struct keymap keytab_s3_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	HOLE,
/*  8 */	TF(3), 	HOLE,	TF(4),	HOLE,	TF(5),	HOLE,	TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	HOLE,	LF(6),	HOLE,	'\t',	'q',	'w',
/* 56 */	'e',	'r',	't',	'y',	'u',	'i',	'o',	'p',
/* 64 */	'[',	']',	DEL,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'a', 	's',	'd',
/* 80 */	'f',	'g',	'h',	'j',	'k',	'l',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	HOLE,	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'z',	'x',	'c',	'v',
/*104 */	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Shifted keyboard table for Type 3 keyboard */

static struct keymap keytab_s3_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	HOLE,
/*  8 */	TF(3), 	HOLE,	TF(4),	HOLE,	TF(5),	HOLE,	TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	'!',    '@',
/* 32 */	'#',	'$',	'%',	'^',	'&',	'*',	'(',	')',
/* 40 */	'_',	'+',	'~',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	HOLE,	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'{',	'}',	DEL,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	':',	'"',
/* 88 */	'|',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	HOLE,	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};


/* Caps Locked keyboard table for Type 3 keyboard */

static struct keymap keytab_s3_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	HOLE,
/*  8 */	TF(3), 	HOLE,	TF(4),	HOLE,	TF(5),	HOLE,	TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	HOLE,	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'[',	']',	DEL,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	';',	'\'',
/* 88 */	'\\',	'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	HOLE,	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};

/* Controlled keyboard table for Type 3 keyboard */

static struct keymap keytab_s3_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	HOLE,
/*  8 */	TF(3), 	HOLE,	TF(4),	HOLE,	TF(5),	HOLE,	TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	'1',	c('@'),
/* 32 */	'3',	'4',	'5',	c('^'),	'7',	'8',	'9',	'0',
/* 40 */	c('_'),	'=',	c('^'),	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	HOLE,	LF(5),	HOLE,	LF(6),	HOLE,	'\t',   c('q'),	c('w'),
/* 56 */	c('e'),	c('r'),	c('t'),	c('y'),	c('u'),	c('i'),	c('o'),	c('p'),
/* 64 */	c('['),	c(']'),	DEL,	HOLE,	RF(7),	STRING+UPARROW,
								RF(9),	HOLE,
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							c('a'),	c('s'),	c('d'),
/* 80 */	c('f'),	c('g'),	c('h'),	c('j'),	c('k'),	c('l'),	';',	'\'',
/* 88 */	c('\\'),
			'\r',	HOLE,	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								HOLE,	LF(9),
/* 96 */	HOLE,	LF(10),	HOLE,	SHIFTKEYS+LEFTSHIFT,
						c('z'),	c('x'),	c('c'),	c('v'),
/*104 */	c('b'),	c('n'),	c('m'),	',',	'.',	c('_'),	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	HOLE,	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			c(' '),	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	ERROR,	IDLE,
};



/* "Key Up" keyboard table for Type 3 keyboard */

static struct keymap keytab_s3_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	NOP,	HOLE,	NOP,	NOP,	HOLE,
/*  8 */	NOP, 	HOLE, 	NOP,	HOLE,	NOP,	HOLE,	NOP,	HOLE,
/* 16 */	NOP, 	NOP, 	NOP,	SHIFTKEYS+ALT,
						HOLE,	NOP,	NOP,	NOP,
/* 24 */	HOLE, 	NOP, 	NOP,	HOLE,	HOLE,	NOP,	NOP,	NOP,
/* 32 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 48 */	HOLE,	NOP,	HOLE,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 56 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,	HOLE,
/* 72 */	NOP,	NOP,	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							NOP, 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,	HOLE,	NOP,
/* 96 */	HOLE,	NOP,	HOLE,	SHIFTKEYS+LEFTSHIFT,
						NOP,	NOP,	NOP,	NOP,
/*104 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+RIGHTSHIFT,
									NOP,
/*112 */	NOP,	NOP,	NOP,	HOLE,	HOLE,	HOLE,	HOLE,	NOP,
/*120 */	BUCKYBITS+METABIT,
			NOP,	BUCKYBITS+METABIT,
					HOLE,	HOLE,	HOLE,	HOLE,	RESET,
};

/* Index to keymaps for Type 3 keyboard */
static struct keyboard keyindex_s3 = {
	&keytab_s3_lc,
	&keytab_s3_uc,
	&keytab_s3_cl,
	0,		/* no Alt Graph key, no Alt Graph table */
	0,		/* no Num Lock key, no Num Lock table */
	&keytab_s3_ct,
	&keytab_s3_up,
	0x0000,		/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
	1,	77,	/* abort keys */
	CAPSMASK,	/* Shift bits which toggle on down event */
};

/* Unshifted keyboard table for Type 4 keyboard */

static struct keymap keytab_s4_lc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	TF(10),
/*  8 */	TF(3), 	TF(11),	TF(4),	TF(12),	TF(5),	SHIFTKEYS+ALTGRAPH,
								TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	BF(13),	LF(5),	BF(10),	LF(6),	HOLE,	'\t',	'q',	'w',
/* 56 */	'e',	'r',	't',	'y',	'u',	'i',	'o',	'p',
/* 64 */	'[',	']',	DEL,	COMPOSE,
						RF(7),	STRING+UPARROW,
								RF(9),	BF(15),
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'a', 	's',	'd',
/* 80 */	'f',	'g',	'h',	'j',	'k',	'l',	';',	'\'',
/* 88 */	'\\',	'\r',	BF(11),	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								BF(8),	LF(9),
/* 96 */	HOLE,	LF(10),	SHIFTKEYS+NUMLOCK,
					SHIFTKEYS+LEFTSHIFT,
						'z',	'x',	'c',	'v',
/*104 */	'b',	'n',	'm',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	LF(16),	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	BF(14),	ERROR,	IDLE,
};

/* Shifted keyboard table for Type 4 keyboard */

static struct keymap keytab_s4_uc = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	TF(10),
/*  8 */	TF(3), 	TF(11),	TF(4),	TF(12),	TF(5),	SHIFTKEYS+ALTGRAPH,
								TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	'!',    '@',
/* 32 */	'#',	'$',	'%',	'^',	'&',	'*',	'(',	')',
/* 40 */	'_',	'+',	'~',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	BF(13),	LF(5),	BF(10),	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'{',	'}',	DEL,	COMPOSE,
						RF(7),	STRING+UPARROW,
								RF(9),	BF(15),
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	':',	'"',
/* 88 */	'|',	'\r',	BF(11),	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								BF(8),	LF(9),
/* 96 */	HOLE,	LF(10),	SHIFTKEYS+NUMLOCK,
					SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	'<',	'>',	'?',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	LF(16),	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	BF(14),	ERROR,	IDLE,
};


/* Caps Locked keyboard table for Type 4 keyboard */

static struct keymap keytab_s4_cl = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	TF(10),
/*  8 */	TF(3), 	TF(11),	TF(4),	TF(12),	TF(5),	SHIFTKEYS+ALTGRAPH,
								TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	'1',	'2',
/* 32 */	'3',	'4',	'5',	'6',	'7',	'8',	'9',	'0',
/* 40 */	'-',	'=',	'`',	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	BF(13),	LF(5),	BF(10),	LF(6),	HOLE,	'\t',	'Q',	'W',
/* 56 */	'E',	'R',	'T',	'Y',	'U',	'I',	'O',	'P',
/* 64 */	'[',	']',	DEL,	COMPOSE,
						RF(7),	STRING+UPARROW,
								RF(9),	BF(15),
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							'A', 	'S',	'D',
/* 80 */	'F',	'G',	'H',	'J',	'K',	'L',	';',	'\'',
/* 88 */	'\\',	'\r',	BF(11),	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								BF(8),	LF(9),
/* 96 */	HOLE,	LF(10),	SHIFTKEYS+NUMLOCK,
					SHIFTKEYS+LEFTSHIFT,
						'Z',	'X',	'C',	'V',
/*104 */	'B',	'N',	'M',	',',	'.',	'/',	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	LF(16),	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	BF(14),	ERROR,	IDLE,
};

/* Alt Graph keyboard table for Type 4 keyboard */

static struct keymap keytab_s4_ag = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	TF(10),
/*  8 */	TF(3), 	TF(11),	TF(4),	TF(12),	TF(5),	SHIFTKEYS+ALTGRAPH,
								TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	NOP,	NOP,
/* 32 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	BF(13),	LF(5),	BF(10),	LF(6),	HOLE,	'\t',	NOP,	NOP,
/* 56 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	DEL,	COMPOSE,
						RF(7),	STRING+UPARROW,
								RF(9),	BF(15),
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							NOP, 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	NOP,	'\r',	BF(11),	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								BF(8),	LF(9),
/* 96 */	HOLE,	LF(10),	SHIFTKEYS+NUMLOCK,
					SHIFTKEYS+LEFTSHIFT,
						NOP,	NOP,	NOP,	NOP,
/*104 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	LF(16),	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			' ',	BUCKYBITS+METABIT,
					HOLE,	HOLE,	BF(14),	ERROR,	IDLE,
};

/* Num Locked keyboard table for Type 4 keyboard */

static struct keymap keytab_s4_nl = {
/*  0 */	HOLE,	NONL,	HOLE,	NONL,	HOLE,	NONL,	NONL,	NONL,
/*  8 */	NONL, 	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,
/* 16 */	NONL,	NONL,	NONL,	NONL,	HOLE,	NONL,	NONL,	NONL,
/* 24 */	HOLE, 	NONL, 	NONL,	HOLE,	HOLE,	NONL,	NONL,	NONL,
/* 32 */	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,
/* 40 */	NONL,	NONL,	NONL,	NONL,	HOLE,	PADEQUAL,
								PADSLASH,
									PADSTAR,
/* 48 */	NONL,	NONL,	PADDOT,	NONL,	HOLE,	NONL,	NONL,	NONL,
/* 56 */	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,
/* 64 */	NONL,	NONL,	NONL,	NONL,
						PAD7,	PAD8,	PAD9,	PADMINUS,
/* 72 */	NONL,	NONL,	HOLE,	HOLE,	NONL,	NONL, 	NONL,	NONL,
/* 80 */	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,
/* 88 */	NONL,	NONL,	PADENTER,
					PAD4,	PAD5,	PAD6,	PAD0,	NONL,
/* 96 */	HOLE,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,
/*104 */	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,	NONL,
									NONL,
/*112 */	PAD1,	PAD2,	PAD3,	HOLE,	HOLE,	HOLE,	NONL,	NONL,
/*120 */	NONL,	NONL,	NONL,	HOLE,	NONL,	PADPLUS,
								ERROR,	IDLE,
};

/* Controlled keyboard table for Type 4 keyboard */

static struct keymap keytab_s4_ct = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	LF(2),	HOLE,	TF(1),	TF(2),	TF(10),
/*  8 */	TF(3), 	TF(11),	TF(4),	TF(12),	TF(5),	SHIFTKEYS+ALTGRAPH,
								TF(6),	HOLE,
/* 16 */	TF(7),	TF(8),	TF(9),	SHIFTKEYS+ALT,
						HOLE,	RF(1),	RF(2),	RF(3),
/* 24 */	HOLE, 	LF(3), 	LF(4),	HOLE,	HOLE,	ESC,	'1',	c('@'),
/* 32 */	'3',	'4',	'5',	c('^'),	'7',	'8',	'9',	'0',
/* 40 */	c('_'),	'=',	c('^'),	'\b',	HOLE,	RF(4),	RF(5),	RF(6),
/* 48 */	BF(13),	LF(5),	BF(10),	LF(6),	HOLE,	'\t',   c('q'),	c('w'),
/* 56 */	c('e'),	c('r'),	c('t'),	c('y'),	c('u'),	c('i'),	c('o'),	c('p'),
/* 64 */	c('['),	c(']'),	DEL,	COMPOSE,
						RF(7),	STRING+UPARROW,
								RF(9),	BF(15),
/* 72 */	LF(7),	LF(8),	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							c('a'),	c('s'),	c('d'),
/* 80 */	c('f'),	c('g'),	c('h'),	c('j'),	c('k'),	c('l'),	';',	'\'',
/* 88 */	c('\\'),
			'\r',	BF(11),	STRING+LEFTARROW,
						RF(11),	STRING+RIGHTARROW,
								BF(8),	LF(9),
/* 96 */	HOLE,	LF(10),	SHIFTKEYS+NUMLOCK,
					SHIFTKEYS+LEFTSHIFT,
						c('z'),	c('x'),	c('c'),	c('v'),
/*104 */	c('b'),	c('n'),	c('m'),	',',	'.',	c('_'),	SHIFTKEYS+RIGHTSHIFT,
									'\n',
/*112 */	RF(13),	STRING+DOWNARROW,
				RF(15),	HOLE,	HOLE,	HOLE,	LF(16),	SHIFTKEYS+CAPSLOCK,
/*120 */	BUCKYBITS+METABIT,
			c(' '),	BUCKYBITS+METABIT,
					HOLE,	HOLE,	BF(14),	ERROR,	IDLE,
};



/* "Key Up" keyboard table for Type 4 keyboard */

static struct keymap keytab_s4_up = {
/*  0 */	HOLE,	BUCKYBITS+SYSTEMBIT,
				HOLE,	NOP,	HOLE,	NOP,	NOP,	NOP,
/*  8 */	NOP, 	NOP, 	NOP,	NOP,	NOP,	SHIFTKEYS+ALTGRAPH,
								NOP,	HOLE,
/* 16 */	NOP, 	NOP, 	NOP,	SHIFTKEYS+ALT,
						HOLE,	NOP,	NOP,	NOP,
/* 24 */	HOLE, 	NOP, 	NOP,	HOLE,	HOLE,	NOP,	NOP,	NOP,
/* 32 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 40 */	NOP,	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 48 */	NOP,	NOP,	NOP,	NOP,	HOLE,	NOP,	NOP,	NOP,
/* 56 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 64 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 72 */	NOP,	NOP,	HOLE,	HOLE,	SHIFTKEYS+LEFTCTRL,
							NOP, 	NOP,	NOP,
/* 80 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 88 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,
/* 96 */	HOLE,	NOP,	NOP,
					SHIFTKEYS+LEFTSHIFT,
						NOP,	NOP,	NOP,	NOP,
/*104 */	NOP,	NOP,	NOP,	NOP,	NOP,	NOP,	SHIFTKEYS+RIGHTSHIFT,
									NOP,
/*112 */	NOP,	NOP,	NOP,	HOLE,	HOLE,	HOLE,	NOP,	NOP,
/*120 */	BUCKYBITS+METABIT,
			NOP,	BUCKYBITS+METABIT,
					HOLE,	HOLE,	NOP,	HOLE,	RESET,
};

/* Index to keymaps for Type 4 keyboard */
static struct keyboard keyindex_s4 = {
	&keytab_s4_lc,
	&keytab_s4_uc,
	&keytab_s4_cl,
	&keytab_s4_ag,
	&keytab_s4_nl,
	&keytab_s4_ct,
	&keytab_s4_up,
	0x0000,		/* Shift bits which stay on with idle keyboard */
	0x0000,		/* Bucky bits which stay on with idle keyboard */
	1,	77,	/* abort keys */
	CAPSMASK|NUMLOCKMASK,	/* Shift bits which toggle on down event */
};


/***************************************************************************/
/*   Index table for the whole shebang					   */
/***************************************************************************/
struct keyboard *keytables[] = {
#ifdef sun2
	&keyindex_ms,
	&keyindex_vt,
#else
	0,
	0,
#endif /*sun2*/
	&keyindex_s2,
	&keyindex_s3,
	&keyindex_s4,
};
int nkeytables = (sizeof keytables)/(sizeof keytables[0]);

/* 
	Keyboard String Table

 	This defines the strings sent by various keys (as selected in the
	tables above).

	The first byte of each string is its length, the rest is data.
*/

#define	kstescinit(c)	{'\033', '[', 'c', '\0'}
char keystringtab[16][KTAB_STRLEN] = {
	kstescinit(H) /*home*/,
	kstescinit(A) /*up*/,
	kstescinit(B) /*down*/,
	kstescinit(D) /*left*/,
	kstescinit(C) /*right*/,
};

/********************************************************************
 *		Compose Key Sequence Table
 *******************************************************************/

/* COMPOSE + first character + second character => ISO character */
struct compose_sequence_t kb_compose_table[] = {

	{' ', ' ', 0xA0},		/* NBSP */
	{'!', '!', 0xA1},		/* inverted ! */
	{'c', '/', 0xA2},		/* cent sign */
	{'C', '/', 0xA2},		/* cent sign */
	{'l', '-', 0xA3},		/* pounds sterling */
	{'L', '-', 0xA3},		/* pounds sterling */
	{'o', 'x', 0xA4},		/* currency symbol */
	{'O', 'X', 0xA4},		/* currency symbol */
	{'0', 'x', 0xA4},		/* currency symbol */
	{'0', 'X', 0xA4},		/* currency symbol */
	{'Y', '-', 0xA5},		/* yen */
	{'y', '-', 0xA5},		/* yen */
	{'|', '|', 0xA6},		/* broken bar */
	{'s', 'o', 0xA7},		/* section mark */
	{'S', 'O', 0xA7},		/* section mark */
	{'"', '"', 0xA8},		/* diaresis */
	{'c', 'o', 0xA9},		/* copyright */
	{'C', 'O', 0xA9},		/* copyright */
	{'-', 'a', 0xAA},		/* feminine superior numeral */
	{'-', 'A', 0xAA},		/* feminine superior numeral */
	{'<', '<', 0xAB},		/* left guillemot */
	{'-', '|', 0xAC},		/* not sign */
	{'-', ',', 0xAC},		/* not sign */
	{'-', '-', 0xAD},		/* soft hyphen */
	{'r', 'o', 0xAE},		/* registered */
	{'R', 'O', 0xAE},		/* registered */
	{'^', '-', 0xAF},		/* macron (?) */

	{'^', '*', 0xB0},		/* degree */
	{'0', '^', 0xB0},		/* degree */
	{'+', '-', 0xB1},		/* plus/minus */
	{'^', '2', 0xB2},		/* superior '2' */
	{'^', '3', 0xB3},		/* superior '3' */
	{'\\', '\\', 0xB4},		/* acute accent */
	{'/', 'u', 0xB5},		/* mu */

	{'P', '!', 0xB6},		/* paragraph mark */
	{'^', '.', 0xB7},		/* centered dot */
	{',', ',', 0xB8},		/* cedilla */
	{'^', '1', 0xB9},		/* superior '1' */
	{'_', 'o', 0xBA},		/* masculine superior numeral */
	{'_', 'O', 0xBA},		/* masculine superior numeral */
	{'>', '>', 0xBB},		/* right guillemot */
	{'1', '4', 0xBC},		/* 1/4 */
	{'1', '2', 0xBD},		/* 1/2 */
	{'3', '4', 0xBE},		/* 3/4 */
	{'?', '?', 0xBF},		/* inverted ? */

	{'A', '`', 0xC0},		/* A with grave accent */
	{'A', '\'', 0xC1},		/* A with acute accent */
	{'A', '^', 0xC2},		/* A with circumflex accent */
	{'A', '~', 0xC3},		/* A with tilde */
	{'A', '"', 0xC4},		/* A with diaresis */
	{'A', '*', 0xC5},		/* A with ring */
	{'A', 'E', 0xC6},		/* AE dipthong */
	{'C', ',', 0xC7},		/* C with cedilla */
	{'E', '`', 0xC8},		/* E with grave accent */
	{'E', '\'', 0xC9},		/* E with acute accent */
	{'E', '^', 0xCA},		/* E with circumflex accent */
	{'E', '"', 0xCB},		/* E with diaresis */
	{'I', '`', 0xCC},		/* I with grave accent */
	{'I', '\'', 0xCD},		/* I with acute accent */
	{'I', '^', 0xCE},		/* I with circumflex accent */
	{'I', '"', 0xCF},		/* I with diaresis */

	{'D', '-', 0xD0},		/* Upper-case eth(?) */
	{'N', '~', 0xD1},		/* N with tilde */
	{'O', '`', 0xD2},		/* O with grave accent */
	{'O', '\'', 0xD3},		/* O with acute accent */
	{'O', '^', 0xD4},		/* O with circumflex accent */
	{'O', '~', 0xD5},		/* O with tilde */
	{'O', '"', 0xD6},		/* O with diaresis */
	{'x', 'x', 0xD7},		/* multiplication sign */
	{'O', '/', 0xD8},		/* O with slash */
	{'U', '`', 0xD9},		/* U with grave accent */
	{'U', '\'', 0xDA},		/* U with acute accent */
	{'U', '^', 0xDB},		/* U with circumflex accent */
	{'U', '"', 0xDC},		/* U with diaresis */
	{'Y', '\'', 0xDD},		/* Y with acute accent */
	{'P', '|', 0xDE},		/* Upper-case thorn */
	{'T', 'H', 0xDE},		/* Upper-case thorn */
	{'s', 's', 0xDF},		/* German double-s */

	{'a', '`', 0xE0},		/* a with grave accent */
	{'a', '\'', 0xE1},		/* a with acute accent */
	{'a', '^', 0xE2},		/* a with circumflex accent */
	{'a', '~', 0xE3},		/* a with tilde */
	{'a', '"', 0xE4},		/* a with diaresis */
	{'a', '*', 0xE5},		/* a with ring */
	{'a', 'e', 0xE6},		/* ae dipthong */
	{'c', ',', 0xE7},		/* c with cedilla */
	{'e', '`', 0xE8},		/* e with grave accent */
	{'e', '\'', 0xE9},		/* e with acute accent */
	{'e', '^', 0xEA},		/* e with circumflex accent */
	{'e', '"', 0xEB},		/* e with diaresis */
	{'i', '`', 0xEC},		/* i with grave accent */
	{'i', '\'', 0xED},		/* i with acute accent */
	{'i', '^', 0xEE},		/* i with circumflex accent */
	{'i', '"', 0xEF},		/* i with diaresis */

	{'d', '-', 0xF0},		/* Lower-case eth(?) */
	{'n', '~', 0xF1},		/* n with tilde */
	{'o', '`', 0xF2},		/* o with grave accent */
	{'o', '\'', 0xF3},		/* o with acute accent */
	{'o', '^', 0xF4},		/* o with circumflex accent */
	{'o', '~', 0xF5},		/* o with tilde */
	{'o', '"', 0xF6},		/* o with diaresis */
	{'-', ':', 0xF7},		/* division sign */
	{'o', '/', 0xF8},		/* o with slash */
	{'u', '`', 0xF9},		/* u with grave accent */
	{'u', '\'', 0xFA},		/* u with acute accent */
	{'u', '^', 0xFB},		/* u with circumflex accent */
	{'u', '"', 0xFC},		/* u with diaresis */
	{'y', '\'', 0xFD},		/* y with acute accent */
	{'p', '|', 0xFE},		/* Lower-case thorn */
	{'t', 'h', 0xFE},		/* Lower-case thorn */
	{'y', '"', 0xFF},		/* y with diaresis */

	{0, 0, 0},			/* end of table */
};


/********************************************************************
 *		Floating Accent Sequence Table
 *******************************************************************/

/* FA + ASCII character => ISO character */
struct fltaccent_sequence_t kb_fltaccent_table[] = {

	{FA_UMLAUT, 'A', 0xC4},		/* A with umlaut */
	{FA_UMLAUT, 'E', 0xCB},		/* E with umlaut */
	{FA_UMLAUT, 'I', 0xCF},		/* I with umlaut */
	{FA_UMLAUT, 'O', 0xD6},		/* O with umlaut */
	{FA_UMLAUT, 'U', 0xDC},		/* U with umlaut */
	{FA_UMLAUT, 'a', 0xE4},		/* a with umlaut */
	{FA_UMLAUT, 'e', 0xEB},		/* e with umlaut */
	{FA_UMLAUT, 'i', 0xEF},		/* i with umlaut */
	{FA_UMLAUT, 'o', 0xF6},		/* o with umlaut */
	{FA_UMLAUT, 'u', 0xFC},		/* u with umlaut */
	{FA_UMLAUT, 'y', 0xFF},		/* y with umlaut */

	{FA_CFLEX, 'A', 0xC2},		/* A with circumflex */
	{FA_CFLEX, 'E', 0xCA},		/* E with circumflex */
	{FA_CFLEX, 'I', 0xCE},		/* I with circumflex */
	{FA_CFLEX, 'O', 0xD4},		/* O with circumflex */
	{FA_CFLEX, 'U', 0xDB},		/* U with circumflex */
	{FA_CFLEX, 'a', 0xE2},		/* a with circumflex */
	{FA_CFLEX, 'e', 0xEA},		/* e with circumflex */
	{FA_CFLEX, 'i', 0xEE},		/* i with circumflex */
	{FA_CFLEX, 'o', 0xF4},		/* o with circumflex */
	{FA_CFLEX, 'u', 0xFB},		/* u with circumflex */

	{FA_TILDE, 'A', 0xC3},		/* A with tilde */
	{FA_TILDE, 'N', 0xD1},		/* N with tilde */
	{FA_TILDE, 'O', 0xD5},		/* O with tilde */
	{FA_TILDE, 'a', 0xE3},		/* a with tilde */
	{FA_TILDE, 'n', 0xF1},		/* n with tilde */
	{FA_TILDE, 'o', 0xF5},		/* o with tilde */

	{FA_CEDILLA, 'C', 0xC7},	/* C with cedilla */
	{FA_CEDILLA, 'c', 0xE7},	/* c with cedilla */

	{FA_ACUTE, 'A', 0xC1},		/* A with acute accent */
	{FA_ACUTE, 'E', 0xC9},		/* E with acute accent */
	{FA_ACUTE, 'I', 0xCD},		/* I with acute accent */
	{FA_ACUTE, 'O', 0xD3},		/* O with acute accent */
	{FA_ACUTE, 'U', 0xDA},		/* U with acute accent */
	{FA_ACUTE, 'a', 0xE1},		/* a with acute accent */
	{FA_ACUTE, 'e', 0xE9},		/* e with acute accent */
	{FA_ACUTE, 'i', 0xED},		/* i with acute accent */
	{FA_ACUTE, 'o', 0xF3},		/* o with acute accent */
	{FA_ACUTE, 'u', 0xFA},		/* u with acute accent */
	{FA_ACUTE, 'y', 0xFD},		/* y with acute accent */
	{FA_ACUTE, 'Y', 0xDD},		/* Y with acute accent */

	{FA_GRAVE, 'A', 0xC0},		/* A with grave accent */
	{FA_GRAVE, 'E', 0xC8},		/* E with grave accent */
	{FA_GRAVE, 'I', 0xCC},		/* I with grave accent */
	{FA_GRAVE, 'O', 0xD2},		/* O with grave accent */
	{FA_GRAVE, 'U', 0xD9},		/* U with grave accent */
	{FA_GRAVE, 'a', 0xE0},		/* a with grave accent */
	{FA_GRAVE, 'e', 0xE8},		/* e with grave accent */
	{FA_GRAVE, 'i', 0xEC},		/* i with grave accent */
	{FA_GRAVE, 'o', 0xF2},		/* o with grave accent */
	{FA_GRAVE, 'u', 0xF9},		/* u with grave accent */

	{0, 0, 0},			/* end of table */
};

/********************************************************************
 *		Num Lock Table
 *******************************************************************/

/* Num Lock:  pad key entry & 0x1F => ASCII character */
u_char kb_numlock_table[] = {
	'=',
	'/',
	'*',
	'-',
	',',

	'7',
	'8',
	'9',
	'+',

	'4',
	'5',
	'6',

	'1',
	'2',
	'3',

	'0',
	'.',
	'\n',	/* Enter */
};
