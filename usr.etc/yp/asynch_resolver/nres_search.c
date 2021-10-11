/*
 * Copyright (c) 1988 Regents of the University of California. All rights
 * reserved.
 * 
 * Redistribution and use in source and binary forms are permitted provided that
 * this notice is preserved and that due credit is given to the University of
 * California at Berkeley. The name of the University may not be used to
 * endorse or promote products derived from this software without specific
 * prior written permission. This software is provided ``as is'' without
 * express or implied warranty.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char     copyright[] = "@(#)res_query.c	5.4 (Berkeley) 4/21/88";
static	char sccsid[] = "@(#)nres_search.c 1.1 92/07/30 1989";
#endif				/* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "nres.h"

extern int      errno;
char           *
nres_getuserinfo(tmp)
	struct nres    *tmp;
{
	return (tmp->userinfo);

}

int 
nresget_h_errno(tmp)
	struct nres    *tmp;
{
	return (tmp->h_errno);

}


nres_search(block)
	struct nres    *block;
{
	register char  *cp, *domain;
	int             n;
	char           *hostalias();

	if ((_res.options & RES_INIT) == 0 && res_init() == -1)
		return (-1);

	block->retries = 0;	/* start clock */
	if (block->search_index < 0)
		return (-1);
	/*only try exact match for reverse cases*/
	if (block->reverse){
		(void) nres_querydomain(block->name, (char *) NULL, block->search_name);
		block->search_index = -1;
		return (0);
	}

	for (cp = block->name, n = 0; *cp; cp++)
		if (*cp == '.')
			n++;
	/* n indicates the presence of trailing dots */

	if (block->search_index == 0) {
		if (n == 0 && (cp = hostalias(block->name))) {
			strncpy(block->search_name, cp, 2 * MAXDNAME);
			block->search_index = -1;	/* if hostalias only try
							 * one name */
			return (0);
		}
	}
	if ((n == 0 || *--cp != '.') && (_res.options & RES_DEFNAMES)) {
		domain = _res.dnsrch[block->search_index];
		if (domain) {
			(void) nres_querydomain(block->name, domain, block->search_name);
			block->search_index++;
			return (0);
		}
	}
	if (n) {
		(void) nres_querydomain(block->name, (char *) NULL, block->search_name);
		block->search_index = -1;
		return (0);
	}
	block->search_index = -1;
	return (-1);
}

/*
 * Perform a call on res_query on the concatenation of name and domain,
 * removing a trailing dot from name if domain is NULL.
 */
static 
nres_querydomain(name, domain, nbuf)
	char           *name, *domain;
	char           *nbuf;
{
	int             n;
	char *sprintf(); /*bsd*/

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf("nres_querydomain(%s, %s)\n",
		       name, domain);
#endif
	if (domain == NULL) {
		/*
		 * Check for trailing '.'; copy without '.' if present.
		 */
		n = strlen(name) - 1;
		if (name[n] == '.') {
			bcopy(name, nbuf, n);
			nbuf[n] = '\0';
		} else
			(void) strcpy(nbuf, name);
	} else
		(void) sprintf(nbuf, "%.*s.%.*s",
			       MAXDNAME, name, MAXDNAME, domain);
	return (0);
}

static char    *
hostalias(name)
	register char  *name;
{
	register char  *C1, *C2;
	FILE           *fp;
	char           *file, *getenv(), *strcpy(), *strncpy();
	char            buf[BUFSIZ];
	static char     abuf[MAXDNAME];

	file = getenv("HOSTALIASES");
	if (file == NULL || (fp = fopen(file, "r")) == NULL)
		return (NULL);
	buf[sizeof(buf) - 1] = '\0';
	while (fgets(buf, sizeof(buf), fp)) {
		for (C1 = buf; *C1 && !isspace(*C1); ++C1);
		if (!*C1)
			break;
		*C1 = '\0';
		if (!strcasecmp(buf, name)) {
			while (isspace(*++C1));
			if (!*C1)
				break;
			for (C2 = C1 + 1; *C2 && !isspace(*C2); ++C2);
			abuf[sizeof(abuf) - 1] = *C2 = '\0';
			(void) strncpy(abuf, C1, sizeof(abuf) - 1);
			(void) fclose(fp);
			return (abuf);
		}
	}
	(void) fclose(fp);
	return (NULL);
}
