/*	@(#)getoken.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:getoken.c	1.3" */

#include <sys/mount.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>

int
getoken(s)
struct token *s;
{
	char *sp;

	s->t_id = 0;
	sp = (char *)&s->t_uname[0];
	return(netname(sp));
}
