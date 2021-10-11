#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)delete_blanks.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)delete_blanks.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		delete_blanks()
 *
 *	Description:	Delete leading and trailing blanks from 'buf'.  This
 *		implementation assumes that buf points to a buffer for BUFSIZ
 *		bytes.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>


void
delete_blanks(buf)
	char *		buf;
{
        register int n;
	char		tmp[BUFSIZ];

        for(n = 0; n <= strlen(buf) ; n++) {
		if (!isspace(buf[n]))
			break;
	}
	(void) strcpy(tmp,&buf[n]);
	(void) strcpy(buf,tmp);
        for(n = strlen(buf); --n >= 0 ;) {
                if (!isspace(buf[n])) {
                        break;
                }
        }
        buf[n+1] = '\0';
} /* end delete_blanks() */
