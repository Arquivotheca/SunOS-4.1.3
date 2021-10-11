/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)expand.c 1.1 92/07/30 SMI" /* from S5R3 acct:lib/expand.c 1.4 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/acct.h>

time_t
expand(ct)
register comp_t ct;
{
	register e;
	register time_t f;

	e = (ct >> 13) & 07;
	f = ct & 017777;
	while (e-- > 0)
		f <<= 3;
	return f;
}
