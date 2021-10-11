#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)no_sunview_tables.c 1.1 92/07/30";
#endif
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * no_sunview_tables.c:    keytables for the Sun-2, 3, 4 keyboards without
 *				SunView function keys
 */

#include "setkey_tables.h"

static Pair		no_sunview_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		    { 3, TF(11)},
    { 25, LF(12)},  {26, TF(12)},
    { 49, LF(13)},  {51, TF(13)},
    { 72, LF(14)},  {73, TF(14)},
    { 95, LF(15)},  {97, TF(15)},
    
    {  5, LF(11)}				/* F1 (Caps)		*/
};

static Key_info		no_sunview_tables = {
    no_sunview_downs, sizeof(no_sunview_downs) / sizeof(Pair), 0, 0
};


/* 
 * tables for no_sunview and noarrow 
 *
 */

static Pair		nosunview_noarrow_downs[] = {
    {  1, BUCKYBITS + SYSTEMBIT},		/* Left function pad */
		    { 3, TF(11)},
    { 25, LF(12)},  {26, TF(12)},
    { 49, LF(13)},  {51, TF(13)},
    { 72, LF(14)},  {73, TF(14)},
    { 95, LF(15)},  {97, TF(15)},

    {  5, LF(11)},			/* F1 (Caps) */ 

					/* Right function pad */
    { 21, RF( 1)},  {22, RF( 2)},  {23, RF( 3)},
    { 45, RF( 4)},  {46, RF( 5)},  {47, RF( 6)},
    { 68, RF( 7)},  {69, RF( 8)},  {70, RF( 9)},
    { 91, RF(10)},  {92, RF(11)},  {93, RF(12)},
    {112, RF(13)}, {113, RF(14)}, {114, RF(15)},

    {111, '\n'}			/* Line Feed */
};

static Pair	nosunview_noarrow_ups[] = {
    {111, NOP}			/* Line Feed */
};

static Key_info		nosunview_noarrow_tables = {
    nosunview_noarrow_downs, 
    sizeof(nosunview_noarrow_downs) / sizeof(Pair),
    nosunview_noarrow_ups,   
    sizeof(nosunview_noarrow_ups) / sizeof(Pair)
};

Keybd_info		ktbl_no_sunview_info = {
    &no_sunview_tables, &nosunview_noarrow_tables, 0, 0
};
