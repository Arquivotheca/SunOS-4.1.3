/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tic_hash.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
 *	comp_hash.c --- Routines to deal with the hashtable of capability
 *			names.
 *
 *  $Log:	RCS/comp_hash.v $
 * Revision 2.1  82/10/25  14:45:34  pavel
 * Added Copyright Notice
 * 
 * Revision 2.0  82/10/24  15:16:34  pavel
 * Beta-one Test Release
 * 
 * Revision 1.3  82/08/23  22:29:33  pavel
 * The REAL Alpha-one Release Version
 * 
 * Revision 1.2  82/08/19  19:09:46  pavel
 * Alpha Test Release One
 * 
 * Revision 1.1  82/08/12  18:36:23  pavel
 * Initial revision
 * 
 *
 */

#include "curses_inc.h"
#include "compiler.h"



/*
 *	make_hash_table()
 *
 *	Takes the entries in cap_table[] and hashes them into cap_hash_table[]
 *	by name.  There are Captabsize entries in cap_table[] and Hashtabsize
 *	slots in cap_hash_table[].
 *
 */

make_hash_table()
{
	int	i;
	int	hashvalue;
	int	collisions = 0;

	make_nte();
	for (i=0; i < Captabsize; i++)
	{
	    hashvalue = hash_function(cap_table[i].nte_name);       
	    DEBUG(9, "%d\n", hashvalue);

	    if (cap_hash_table[hashvalue] != (struct name_table_entry *) 0)
		collisions++;

	    cap_table[i].nte_link = cap_hash_table[hashvalue];
	    cap_hash_table[hashvalue] = &cap_table[i];
	}

	DEBUG(3, "Hash table complete\n%d collisions ", collisions);
	DEBUG(3, "out of %d entries\n", Captabsize);
}

/*
 * Make the name_table_entry from the capnames.c set of tables.
 */
make_nte()
{
	register int i, n;
	extern char *boolnames[], *numnames[], *strnames[];

	n = 0;

	for (i=0; boolnames[i]; i++) {
		cap_table[n].nte_link = NULL;
		cap_table[n].nte_name = boolnames[i];
		cap_table[n].nte_type = BOOLEAN;
		cap_table[n].nte_index = i;
		n++;
	}
	BoolCount = i;

	for (i=0; numnames[i]; i++) {
		cap_table[n].nte_link = NULL;
		cap_table[n].nte_name = numnames[i];
		cap_table[n].nte_type = NUMBER;
		cap_table[n].nte_index = i;
		n++;
	}
	NumCount = i;

	for (i=0; strnames[i]; i++) {
		cap_table[n].nte_link = NULL;
		cap_table[n].nte_name = strnames[i];
		cap_table[n].nte_type = STRING;
		cap_table[n].nte_index = i;
		n++;
	}
	StrCount = i;

	Captabsize = n;
}


/*
 *	int hash_function(string)
 *
 *	Computes the hashing function on the given string.
 *
 *	The current hash function is the sum of each consectutive pair
 *	of characters, taken as two-byte integers, mod Hashtabsize.
 *
 */

static
int
hash_function(string)
char	*string;
{
	long	sum = 0;

	while (*string)
	{
	    sum += *string + (*(string + 1) << 8);
	    string++;
	}

	return (sum % Hashtabsize);
}



/*
 *	struct name_table_entry *
 *	find_entry(string)
 *
 *	Finds the entry for the given string in the hash table if present.
 *	Returns a pointer to the entry in the table or 0 if not found.
 *
 */

struct name_table_entry *
find_entry(string)
char	*string;
{
	int	hashvalue;
	struct name_table_entry	*ptr;

	hashvalue = hash_function(string);

	ptr = cap_hash_table[hashvalue];

	while (ptr != (struct name_table_entry *) 0  &&
			       	           strcmp(ptr->nte_name, string) != 0)
	    ptr = ptr->nte_link;

	return (ptr);
}
