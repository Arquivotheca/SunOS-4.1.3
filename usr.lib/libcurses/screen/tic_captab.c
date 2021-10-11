/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tic_captab.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

/*
 *	comp_captab.c -- The names of the capabilities in a form ready for
 *		         the making of a hash table for the compiler.
 *
 */


#include "curses_inc.h"
#include "compiler.h"


struct name_table_entry	cap_table[512];

struct name_table_entry *cap_hash_table[360];

int	Hashtabsize = 360;
int	Captabsize = 0;
int	BoolCount = 0;
int	NumCount = 0;
int	StrCount = 0;
