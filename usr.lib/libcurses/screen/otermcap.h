/*	@(#)otermcap.h 1.1 92/07/30 SMI; from S5R3.1 1.2	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define TBUFSIZE	2048		/* double the norm */

/* externs from libtermcap.a */
extern int otgetflag (), otgetnum (), otgetent ();
extern char *otgetstr ();
extern char *tskip ();			/* non-standard addition */
extern int TLHtcfound;			/* non-standard addition */
extern char TLHtcname[];		/* non-standard addition */
