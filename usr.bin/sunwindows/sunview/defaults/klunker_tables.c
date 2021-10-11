#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)klunker_tables.c 1.1 92/07/30";
#endif
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * klunker_tables.c:	keytables for the klunker keyboard
 */


#include "setkey_tables.h"

static Pair	 klunker_standard_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad	*/
		   { 2, LF(11)}, { 3, LF( 2)},
    { 25, LF(3)},  {26, LF(12)}, {27, LF( 4)},
    { 49, LF(5)},  {50, LF(13)}, {51, LF( 6)},
    { 72, LF(7)},  {73, LF(14)}, {74, LF( 8)},
    { 95, LF(9)},  {96, LF(15)}, {97, LF(10)},

    {  5, TF( 1)}, { 6, TF( 2)}, { 7, TF(10)},	/* Top Function row	*/
    {  8, TF( 3)}, { 9, TF(11)}, {10, TF( 4)},
    { 11, TF(12)}, {12, TF( 5)}, {13, TF(13)},
    { 14, TF( 6)}, {15, TF(14)}, {16, TF( 7)},
    { 17, TF( 8)}, {18, TF( 9)},

    { 21, RF( 1)}, {22, RF( 2)}, {23, RF( 3)},	/* Right function pad	*/
    { 45, RF( 4)}, {46, RF( 5)}, {47, RF( 6)},
    { 68, RF( 7)}, {69, RF( 8)}, {70, RF( 9)},
    { 91, RF(10)}, {92, RF(11)}, {93, RF(12)},
    {114, RF(13)},	 	 {116, RF(15)},

    {124, BF(2)}				/* "Right" => Next	*/
};

static Pair	 klunker_standard_ups[] = {
    /*	UP_KEYS give a code on button-up	*/
    {76, SHIFTKEYS+LEFTCTRL},			/* "Shift Lock" => CTRL	*/
    {122, BUCKYBITS + METABIT}			/* "Left" = Meta = CMD	*/
};

static Key_info	 klunker_standard = {
    klunker_standard_downs, sizeof(klunker_standard_downs) / sizeof(Pair),
    klunker_standard_ups, sizeof(klunker_standard_ups) / sizeof(Pair)
};

static Pair	 klunker_lefty_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		   { 2,  RF( 2)}, { 3,  RF( 3)},
    { 25, RF( 4)}, {26,  RF( 5)}, {27,  RF( 6)},
    { 49, RF( 7)}, {50,  STRING+UPARROW},
				  {51,  RF( 9)},
    { 72, STRING+LEFTARROW},
		   {73,  RF(11)}, {74,  STRING+RIGHTARROW},
    { 95, RF(13)}, { 96, STRING+DOWNARROW},
				  { 97, RF(15)},

    {  5, TF( 1)}, { 6, TF( 2)}, { 7, TF(10)},	/* Top Function row	*/
    {  8, TF( 3)}, { 9, TF(11)}, {10, TF( 4)},
    { 11, TF(12)}, {12, TF( 5)}, {13, TF(13)},
    { 14, TF( 6)}, {15, TF(14)}, {16, TF( 7)},
    { 17, TF( 8)}, {18, TF( 9)},

    { 21, LF( 2)}, {22, LF(11)},		/* Right function pad */
				 {23, BUCKYBITS + SYSTEMBIT},
    { 45, LF( 4)}, {46, LF(12)}, {47, LF( 3)},
    { 68, LF( 6)}, {69, LF(13)}, {70, LF( 5)},
    { 91, LF( 8)}, {92, LF(14)}, {93, LF( 7)},
    {114, LF(10)},	 	 {116, LF(9)},

    {124, 0x7f}					/* "Right" => DEL	*/
};

static Pair	 klunker_lefty_ups[] = {
    {76, SHIFTKEYS+LEFTCTRL},			/* "Shift Lock" => CTRL	*/
    {113, SHIFTKEYS+RIGHTCTRL},			/* "Back Tab" => CTRL	*/
    {122, BUCKYBITS + METABIT}			/* "Left" = Meta = CMD	*/
};

static Key_info	 klunker_lefty = {
    klunker_lefty_downs, sizeof(klunker_lefty_downs) / sizeof(Pair),
    klunker_lefty_ups, sizeof(klunker_lefty_ups) / sizeof(Pair)
};

static Pair	 klunker_left_noarrow_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		   { 2, RF( 2)}, { 3, RF( 3)},
    { 25, RF( 4)}, {26, RF( 5)}, {27, RF( 6)},
    { 49, RF( 7)}, {50, RF( 8)}, {51, RF( 9)},
    { 72, RF(10)}, {73, RF(11)}, {74, RF(12)},
    { 95, RF(13)}, {96, RF(14)}, {97, RF(15)},

    {  5, TF( 1)}, { 6, TF( 2)}, { 7, TF(10)},	/* Top Function row	*/
    {  8, TF( 3)}, { 9, TF(11)}, {10, TF( 4)},
    { 11, TF(12)}, {12, TF( 5)}, {13, TF(13)},
    { 14, TF( 6)}, {15, TF(14)}, {16, TF( 7)},
    { 17, TF( 8)}, {18, TF( 9)},

    { 21, LF( 2)}, {22, LF(11)},		/* Right function pad */
				 {23, BUCKYBITS + SYSTEMBIT},
    { 45, LF( 4)}, {46, LF(12)}, {47, LF( 3)},
    { 68, LF( 6)}, {69, LF(13)}, {70, LF( 5)},
    { 91, LF( 8)}, {92, LF(14)}, {93, LF( 7)},
    {114, LF(10)},	 	 {116, LF(9)},

    {124, 0x7f}					/* "Right" => DEL	*/
};

static Pair	 klunker_left_noarrow_ups[] = {
    {76, SHIFTKEYS+LEFTCTRL},			/* "Shift Lock" => CTRL	*/
    {113, SHIFTKEYS+RIGHTCTRL},			/* "Back Tab" => CTRL	*/
    {122, BUCKYBITS + METABIT}			/* "Left" = Meta = CMD	*/
};

static Key_info	 klunker_left_noarrow = {
    klunker_left_noarrow_downs,
    sizeof(klunker_left_noarrow_downs) / sizeof(Pair),
    klunker_left_noarrow_ups,
    sizeof(klunker_left_noarrow_ups) / sizeof(Pair)
};

Keybd_info	ktbl_klunker_info = {
    &klunker_standard, &klunker_standard, &klunker_lefty, &klunker_left_noarrow
};
