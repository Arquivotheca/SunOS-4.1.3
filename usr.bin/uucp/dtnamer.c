/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)dtnamer.c 1.1 92/07/30"	/* from SVR3.2 uucp:dtnamer.c 1.1 */

/*
 *	Translate a channel number into the proper filename
 *	to access that Datakit channel's "dktty" device driver.
 */
	static char	SCCSID[] = "@(#)dtnamer.c	2.2+BNU  DKHOST 85/08/27";
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */

	char *
dtnamer(chan)
{
	static char	dtname[12];

	sprintf(dtname, "/dev/dk%.3dt", chan);

	return(dtname);
}
