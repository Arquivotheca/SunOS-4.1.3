#ifndef lint
static	char sccsid[] = "%Z%%M% %I% %E% SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/_data.c	1.2"
*/
#include <sys/types.h>
#include <nettli/timod.h>
#include <stdio.h>


/*
 * ti_user structures get allocated
 * the first time the user calls t_open or t_sync
 */
struct _ti_user *_ti_user = 0;

struct _ti_user _null_ti = { 0, 0, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0};
