/*	%Z%%M% %I% %E% SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:nlsdata.c	1.3"

/*
 * data used by network listener service library routines
 */

#include "tiuser.h"

int _nlslog;	/* non-zero allows use of stderr for messages	*/
struct t_call *_nlscall; /* call struct allocated by routines	*/
