#ifndef lint
static	char sccsid[] = "@(#)base.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:base.c	1.2"
/*
 * This file contains code for the crash function base.
 */

#include "stdio.h"
#include "crash.h"

/* get arguments for function base */
int
getbase()
{
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		do {
			prnum(args[optind++]);
		} while(args[optind]);
	}
	else longjmp(syn, 0);
}


/* print results of function base */
int
prnum(string)
char *string;
{
	int i;
	long num;

	if (*string == '(') 
		num = eval(++string);
	else num = strcon(string, NULL);
	if (num == -1)
		return;
	fprintf(fp, "hex: %x\n", num);
	fprintf(fp, "decimal: %d\n", num);
	fprintf(fp, "octal: %o\n", num);
	fprintf(fp, "binary: ");
	for (i=0; num >= 0 && i < 32; i++, num<<=1);
	for (; i<32; i++, num<<=1)
		num < 0 ? fprintf(fp, "1") : fprintf(fp, "0");
	fprintf(fp, "\n");
}
