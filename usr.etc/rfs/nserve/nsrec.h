/*      @(#)nsrec.h 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nserve:nsrec.h	1.3"
/*
 * defines needed for recovery
 */
#define MAXCHILDREN	10
#define MAXDOMAINS	10
#define R_UNUSED	0
#define R_PRIME		1
#define R_NOTPRIME	2
#define R_DEAD		3
#define R_UNK		4
#define R_PENDING	5
#define POLLTIME	300

extern char	*Mydomains[];
extern int	Mylastdom;
extern struct address	*Paddress[];
extern struct address	*Myaddress;
extern int	Recover;
