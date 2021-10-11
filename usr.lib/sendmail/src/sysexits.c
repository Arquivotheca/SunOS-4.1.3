/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *  Sendmail
 *  Copyright (c) 1983  Eric P. Allman
 *  Berkeley, California
 */

# include <sysexits.h>
# include "useful.h"

SCCSID(@(#)sysexits.c 1.1 92/07/30 SMI); /* from UCB 5.4 4/1/88 */

/*
 *  SYSEXITS.C -- error messages corresponding to sysexits.h
 */

char	*SysExMsg[] = {
	/* 64 USAGE */		"500 Bad usage",
	/* 65 DATAERR */	"501 Data format error",
	/* 66 NOINPUT */	"550 Cannot open input",
	/* 67 NOUSER */		"550 User unknown",
	/* 68 NOHOST */		"550 Host unknown",
	/* 69 UNAVAILABLE */	"554 Service unavailable",
	/* 70 SOFTWARE */	"554 Internal error",
	/* 71 OSERR */		"451 Operating system error",
	/* 72 OSFILE */		"554 System file missing",
	/* 73 CANTCREAT */	"550 Can't create output",
	/* 74 IOERR */		"451 I/O error",
	/* 75 TEMPFAIL */	"250 Deferred",
	/* 76 PROTOCOL */	"554 Remote protocol error",
	/* 77 NOPERM */		"550 Insufficient permission",
};

int	N_SysEx = sizeof SysExMsg / sizeof SysExMsg[0];
/*
 *  STATSTRING -- return string corresponding to an error status
 *
 *	Parameters:
 *		stat -- the status to decode.
 *
 *	Returns:
 *		The string corresponding to that status
 *
 *	Side Effects:
 *		none.
 */

char *
statstring(stat)
	int stat;
{
	static char ebuf[100];

	stat -= EX__BASE;
	if (stat < 0 || stat >= N_SysEx)
	{
		(void) sprintf(ebuf, "554 Unknown status %d", stat + EX__BASE);
		return (ebuf);
	}

	return(SysExMsg[stat]);
}
