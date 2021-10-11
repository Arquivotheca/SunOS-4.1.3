/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)devtolin.c 1.1 92/07/30 SMI" /* from S5R3 acct:lib/devtolin.c 1.4 */
/*
 *	convert device to linename (as in /dev/linename)
 *	return ptr to LSZ-byte string, "?" if not found
 *	device must be character device
 *	maintains small list in tlist for speed
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include <dirent.h>

#define TSIZE1	50	/* # distinct names, for speed only */
static	tsize1;
static struct tlist {
	char	tname[LSZ];	/* linename */
	dev_t	tdev;		/* device */
} tl[TSIZE1];

static char name[LSZ];

dev_t	lintodev();

char *
devtolin(device)
dev_t device;
{
	register struct tlist *tp;
	DIR *fdev;
	register struct dirent *dp;

	for (tp = tl; tp < &tl[tsize1]; tp++)
		if (device == tp->tdev)
			return(tp->tname);

	if ((fdev = opendir("/dev")) == NULL)
		return("?");
	while ((dp = readdir(fdev)) != NULL)
		if (lintodev(dp->d_name) == device) {
			if (tsize1 < TSIZE1) {
				tp->tdev = device;
				CPYN(tp->tname, dp->d_name);
				tsize1++;
			}
			CPYN(name, dp->d_name);
			closedir(fdev);
			return(name);
		}
	closedir(fdev);
	return("?");
}
