/*	@(#)lsfiles.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:lsfiles.h	1.2"

/*
 *	lsfiles.h:	listener files stuff
 */


/*
 * initialization constants for file checking/usage
 */

#define LOGOFLAG	(O_WRONLY | O_APPEND | O_CREAT)
#define LOGMODE		(0666)

#define PIDOFLAG	(O_WRONLY | O_CREAT | O_TRUNC)
#define PIDMODE		(0644)

#define NETOFLAG	(O_RDWR)

