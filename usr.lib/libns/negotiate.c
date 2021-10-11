/*	@(#)negotiate.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident  "@(#)libns:negotiate.c  1.9.1.1" */
#include <sys/param.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <rfs/message.h>
#include <rpc/rpc.h>
#include <rfs/ns_xdr.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>
#include <rfs/rfsys.h>
#include <rfs/cirmgr.h>
#include <rfs/pn.h>
#include <rfs/hetero.h>
#include "nslog.h"

#define NDATA_CANON	"lc20ic64c64ii"
#define NDATA_CLEN	176			/* length of canonical ndata */
#define LONG_CLEN	4			/* length of canonical long */

/* return error codes for negotiate() */
#define	N_ERROR		-1		/* error is in errno */
#define	N_TERROR	-2		/* error is in t_errno */
#define N_VERROR	-3		/* version mismatch */
#define	N_CERROR	-4		/* fcanon/tcanon failed */
#define	N_FERROR	-5		/* bad flag as argument */
#define	N_PERROR	-6		/* passwords did not match */

/* negotiate data packect */

ndata_t ndata;

int
negotiate(fd, passwd, flag)
int fd;
char *passwd;
long flag;
{
	int n;
	char netnam[MAXDNAME];
	struct token token;
	int rfversion;
	char nbuf[200];
	int flgs = 0;
	char *domnam, *unam;
	int dlen, ulen;
	struct n_data	n_buf;

	/* fill in ndata structure */

	if (netname(netnam) < 0)
		return(N_ERROR);
	if (getoken(&token) < 0)
		return(N_ERROR);

        if (passwd == (char *) NULL)
                ndata.n_passwd[0] = '\0';
        else
                (void) strncpy(&ndata.n_passwd[0], passwd, PASSWDLEN);
	(void) strncpy(&ndata.n_netname[0], netnam, MAXDNAME);
	ndata.n_hetero = (long)MACHTYPE;
	ndata.n_token = token;
	n_buf.nd_ndata = ndata;

	if (rfsys(RF_VERSION,VER_GET,&n_buf.n_vhigh,&n_buf.n_vlow) < 0)
		return(N_ERROR);

	/* exchange ndata structures */

	if ((n = tcanon(rfs_n_data, &n_buf, &nbuf[0])) == 0) {
		return(N_CERROR);
	}
	if (t_snd(fd, nbuf, n, 0) != n)
		return(N_TERROR);

	if (rf_rcv(fd, &nbuf[0], NDATA_CLEN, &flgs) != NDATA_CLEN)
		return(N_TERROR);

	if (fcanon(rfs_n_data, &nbuf[0], &n_buf) == 0) {
		return(N_CERROR);
	}

	/* check version number, and calculate value for gdpmisc	*/
	if ((rfversion=rfsys(RF_VERSION,VER_CHECK,&n_buf.n_vhigh,&n_buf.n_vlow)) < 0)
	    return(N_VERROR);

	ndata = n_buf.nd_ndata;

	/* ndata now contains the other machine's data */
	/* do server side passwd verification			*/

	if (flag == RFS_SERVER) {
		/* extract domain name and uname from netname */

		dlen = strcspn(&ndata.n_netname[0], ".");
		ulen = strlen(&ndata.n_netname[0]) - dlen - 1;
		if ((domnam = malloc(dlen + 1)) == NULL)
			return(N_ERROR);

		if ((unam = malloc(ulen + 1)) == NULL)
			return(N_ERROR);

		(void) strncpy(domnam, &ndata.n_netname[0], dlen);
		(void) strncpy(unam, &ndata.n_netname[0] + dlen + 1, ulen);

		/* verify passwd */

		switch (checkpw(DOMPASSWD, ndata.n_passwd, domnam, unam)) {
		case 0:
			if (rfsys(RF_VFLAG, V_GET) == V_SET)
				return(N_ERROR);
			break;
		case 1:
			break;
		case 2:
			return(N_PERROR);
		} /* end switch */

		/* send/receive proper machine response for type */

		if (ndata.n_hetero == ((long)MACHTYPE)) {
			ndata.n_hetero = NO_CONV;
		} else if ((ndata.n_hetero & BYTE_MASK) != ((long)MACHTYPE & BYTE_MASK)) {
			ndata.n_hetero = ALL_CONV;
		} else {
			ndata.n_hetero = DATA_CONV;
		}
		if ((n = tcanon(rfs_long, &ndata.n_hetero, &nbuf[0])) == 0) {
			return(N_CERROR);
		}
		if (t_snd(fd, nbuf, n, 0) != n) {
			t_error("negotiate");
			return(-1);
		}
	}
	else if (flag == RFS_CLIENT) { /* CLIENT -- handle hetero input from SERVER	*/

		if (rf_rcv(fd, &nbuf[0], LONG_CLEN, &flgs) != LONG_CLEN)
			return(N_TERROR);
		if (fcanon(rfs_long, &nbuf[0], &ndata.n_hetero) == 0)
			return(N_CERROR);
	}
	else
		return(N_FERROR);
	return(rfversion);
}

int
checkpw(fname, pass, domnam, unam)
char *fname;
char *pass;
char *domnam;
char *unam;
{
	char filename[BUFSIZ];
	int buflen = 0;
	char buf[BUFSIZ];
	char *pw;
	char *m_name, *m_pass, *ptr;
	FILE *fp;

	char *crypt();

/*
 * checkpw returns 0 if the file was not found or the entry did not exist
 *		   1 if the passwd matched
 *		   2 if the passwd did not match
 */

	(void) sprintf(filename, fname, domnam);
	if ((fp = fopen(filename, "r")) != NULL) {
		while(fgets(buf, 512, fp) != NULL) {
			if (buf[strlen(buf)-1] == '\n')
				buf[strlen(buf)-1] = '\0';
			m_name = ptr = buf;
			buflen = strlen(ptr);
			while (*ptr != ':' && *ptr != '\0')
				ptr++;
			if (*ptr == ':')
				buflen--;
			*ptr = '\0';
			if (strcmp(unam, m_name) == 0) {
				if (buflen == strlen(unam)) {
					/* only name in passwd file */
					--ptr; /* point at NULL */
				}
				m_pass = ++ptr;
				while (*ptr != ':' && *ptr != '\0')
					ptr++;
				*ptr = '\0';
				pw = crypt(pass, m_pass);
				(void) fclose(fp);
				if (strcmp(pw, m_pass) == 0)
					return(1);  /* correct */
				else
					return(2);  /* incorrect */
			}
		} /* end while */
		(void) fclose(fp);
	}
	return(0);
}
