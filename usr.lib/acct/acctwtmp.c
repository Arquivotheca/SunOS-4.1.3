/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acctwtmp.c 1.1 92/07/30 SMI" /* from S5R3.1 acct:acctwtmp.c 1.4 */
/*
 *	acctwtmp reason >> /etc/wtmp
 *	writes utmp.h record (with current time) to end of std. output
 *	acctwtmp `uname` >> /etc/wtmp as part of startup
 *	acctwtmp pm >> /etc/wtmp  (taken down for pm, for example)
 */
#include <stdio.h>
#include "acctdef.h"
#include <sys/types.h>
#include <utmp.h>

struct	utmp	wb;
extern	time_t time();

#ifndef SYSTEMV
#define WTMP_FILE "/usr/adm/wtmp"
#endif

main(argc, argv)
char **argv;
{
	if(argc < 2) {
		fprintf(stderr, "Usage: %s reason [ >> %s ]\n",
			argv[0], WTMP_FILE);
		exit(1);
	}

	strncpy(wb.ut_line, argv[1], LSZ);
#ifdef SYSTEMV
	wb.ut_line[11] = NULL;
	wb.ut_type = ACCOUNTING;
#else
	wb.ut_line[LSZ-1] = NULL;
	strncpy(wb.ut_name, ACCT_NAME, NSZ);
#endif
	time(&wb.ut_time);
	fseek(stdout, 0L, 2);
	fwrite(&wb, sizeof(wb), 1, stdout);
}
