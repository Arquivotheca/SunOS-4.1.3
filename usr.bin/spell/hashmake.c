/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)hashmake.c 1.1 92/07/30 SMI"; /* from S5R3 1.2 */
#endif

#include "hash.h"

main()
{
	char word[30];
	long h;
	hashinit();
	while(gets(word)) {
		printf("%.*lo\n",(HASHWIDTH+2)/3,hash(word));
	}
	return(0);
}
