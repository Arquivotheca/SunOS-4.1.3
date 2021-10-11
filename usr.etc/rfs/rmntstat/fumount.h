/*	@(#)fumount.h 1.1 92/07/30 SMI	*/
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rmntstat:fumount.h	1.3"

#define NODLEN 64

/* resource flags */
#define NOTADV 1	/* was not advertised */
#define KNOWN  2	/* node name is known */
#define FUFAIL 3	/* fumount() syscall failed */
#define EMPTY -1	/* unused slots in structure */

struct clnts {
	char node[NODLEN + 1];
	sysid_t sysid;
	int flags;
	int bcount;
};

