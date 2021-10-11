
/*	@(#)ns_errlist.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:ns_errlist.c	1.5" */

char	*ns_errlist[] = {
	"No error",
	"Format error",
	"Name server failure",
	"Resource name not found",
	"Request type not implemented",
	"Permission for operation denied",
	"Resource name not unique",
	"System call failure in name server",
	"Error in accessing password file on primary name server",
	"Password does not match that on primary name server",
	"No password entry for this machine",
	"RFS is not running",
	"No reply from domain name server",
	"Unknown domain",
	"Name Server in recovery, try again",
	"Unknown failure"
};
int	ns_err = { sizeof(ns_errlist)/sizeof(ns_errlist[0]) };
