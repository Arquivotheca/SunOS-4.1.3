/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)db_reload.c 1.1 92/07/30 SMI"; /* from UCB 4.15 2/28/88 */
#endif /* not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <arpa/nameser.h>
#include "ns.h"
#include "db.h"

extern time_t	resettime;

/*
 * Flush and reload data base.
 */

db_reload()
{
	extern char *bootfile;

#ifdef DEBUG
	if (debug >= 3)
		fprintf(ddt,"reload()\n");
#endif
	syslog(LOG_NOTICE, "reloading nameserver\n");

	qflush();
	sqflush();
	fwdtab_free();
	if (hashtab != NULL)
		db_free(hashtab);
	hashtab = NULL;
	if (fcachetab != NULL)
		db_free(fcachetab);
	fcachetab = NULL;
	db_inv_free();
	fwdtab_free();
	ns_init(bootfile);
	time(&resettime);
}

db_free(htp)
	struct hashbuf *htp;
{
	register struct databuf *dp, *nextdp;
	register struct namebuf *np, *nextnp;
	struct namebuf **npp, **nppend;

	npp = htp->h_tab;
	nppend = npp + htp->h_size;
	while (npp < nppend) {
	    for (np = *npp++; np != NULL; np = nextnp) {
		if (np->n_hash != NULL)
			db_free(np->n_hash);
		(void) free((char *)np->n_dname);
		for (dp = np->n_data; dp != NULL; ) {
			nextdp = dp->d_next;
			(void) free((char *)dp);
			dp = nextdp;
		}
		nextnp = np->n_next;
		free((char *)np);
	    }
	}
	(void) free((char *)htp);
}

db_inv_free()
{
	register struct invbuf *ip;
	register int i, j;

	for (i = 0; i < INVHASHSZ; i++)
		for (ip = invtab[i]; ip != NULL; ip = ip->i_next)
			for (j = 0; j < INVBLKSZ; j++)
				ip->i_dname[j] = NULL;
}

fwdtab_free()
{
	extern	struct fwdinfo *fwdtab;
	struct fwdinfo *fp, *nextfp;

	for (fp = fwdtab; fp != NULL; fp = nextfp) {
		nextfp = fp->next;
		free((char *)fp);
	}
	fwdtab = NULL;
}
