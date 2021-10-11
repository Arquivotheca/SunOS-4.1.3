/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)errorspas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int print_error();
int report_most_recent_error();

int printerror(f77string, error)
char *f77string;
int error;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;

	strlen = 256;
	sptr = f77string+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,f77string,strlen);
	pasarg[strlen] = '\0';
	i = print_error(pasarg, error);
	return(i);
	}

int reportrecenterr(error)
int *error;
	{
	return(report_most_recent_error(error));
	}
