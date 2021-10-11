#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ypxfrd_client.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <stdio.h>
#include <errno.h>
#include <rpc/rpc.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include "ypxfrd.h"
#include <ndbm.h>
#if (defined (vax) || defined(i386))
#define DOSWAB 1
#endif

static struct timeval TIMEOUT = {25, 0};

static DBM     *db;

/*delete the dbm file with name file*/
static 
dbm_deletefile(file)
char *file;
{
	char            pag1[MAXPATHLEN];
	char            dir1[MAXPATHLEN];
	int err;
	strcpy(pag1, file);
	strcat(pag1, ".pag");
	strcpy(dir1, file);
	strcat(dir1, ".dir");
	err=0;
	if (unlink(pag1) < 0) {
		perror("unlinkpag");
		err= -1;
	}

	if (unlink(dir1) < 0) {
		perror("unlinkdir");
		return (-1);
	}
	return (err);
}

/*xdr just the .pag file of a dbm file */ 
static          bool_t
xdr_pages(xdrs, objp)
	XDR            *xdrs;
{
	static struct pag res;
	struct pag     *PAG;
#ifdef DOSWAB
	short          *s;
	int		i;
#endif
	bool_t          more;
	bool_t          goteof;

	goteof = FALSE;
	if (!xdr_pag(xdrs, &res))
		return (FALSE);
	PAG = &res;
	while (1) {
		if (PAG->status == OK) {
#ifdef DOSWAB
		s= (short *)PAG->pag_u.ok.blkdat;
		s[0]=ntohs(s[0]);
		for(i=1;i<=s[0];i++)
			s[i]=ntohs(s[i]);
#endif
			errno = 0;
			lseek(db->dbm_pagf, PAG->pag_u.ok.blkno * PBLKSIZ, L_SET);
			if (errno != 0) {
				perror("seek");
				exit(-1);
			}
			if (write(db->dbm_pagf, PAG->pag_u.ok.blkdat, PBLKSIZ) < 0) {
				perror("write");
				exit(-1);
			}
		} else if (PAG->status == GETDBM_ERROR) {
			printf("clnt call getpag GETDBM_ERROR\n");
			exit(-1);
		} else if (PAG->status == GETDBM_EOF)
			goteof = TRUE;
		if (!xdr_bool(xdrs, &more))
			return (FALSE);
		if (more == FALSE)
			return (goteof);
		if (!xdr_pag(xdrs, &res))
			return (FALSE);
	}
}
/*xdr  just the .dir part of a dbm file*/
static          bool_t
xdr_dirs(xdrs, objp)
	XDR            *xdrs;
{
	static struct dir res;
	struct dir     *DIR;
	bool_t          more;
	bool_t          goteof;

	goteof = FALSE;
	if (!xdr_dir(xdrs, &res))
		return (FALSE);
	DIR = &res;
	while (1) {
		if (DIR->status == OK) {
			errno = 0;
			lseek(db->dbm_dirf, DIR->dir_u.ok.blkno * DBLKSIZ, L_SET);
			if (errno != 0) {
				perror("seek");
				exit(-1);
			}
			if (write(db->dbm_dirf, DIR->dir_u.ok.blkdat, DBLKSIZ) < 0) {
				perror("write");
				exit(-1);
			}
		} else if (DIR->status == GETDBM_ERROR) {
			printf("clnt call getdir GETDBM_ERROR\n");
			exit(-1);
		} else if (DIR->status == GETDBM_EOF)
			goteof = TRUE;
		if (!xdr_bool(xdrs, &more))
			return (FALSE);
		if (more == FALSE)
			return (goteof);
		if (!xdr_dir(xdrs, &res))
			return (FALSE);
	}
}

/*xdr a dbm file from ypxfrd */
/*note that if the client or server do not support ndbm
we may not use this optional protocol*/ 
xdr_myfyl(xdrs,objp)
	XDR *xdrs;
	int *objp;
{
	if (!xdr_answer(xdrs,objp)) return(FALSE);
	if (*objp != OK) return (TRUE);
	if (!xdr_pages(xdrs,NULL)) return(FALSE);
	if (!xdr_dirs(xdrs,NULL)) return(FALSE);
	return(TRUE);
}




ypxfrd_getdbm(tempmap,master,master_addr,domain,map )
	char *tempmap;
	char *domain;
	char *map;
	struct in_addr master_addr;
	char *master;
{
	hosereq         rmap;
	u_long          order;
	CLIENT         *clnt;
	DBM            *db2;
	paglist        *PL;
	pag            *PAG;
	dir            *DIR;
	int            *rep;
	int             debug = 0;
	int             udp = 0;
	int             startime;
	int		res;
	struct sockaddr_in	sin;
	int 		sock = RPC_ANYSOCK;
	int	socksize	= 24 * 1024;

	bzero(&sin,sizeof(sin));
	sin.sin_addr=master_addr;
	sin.sin_family=AF_INET;
	clnt=clnttcp_create(&sin, YPXFRD, 1, &sock, 8192, 8192);
	if (clnt == NULL) {
		clnt_pcreateerror(master);
		return(-1);
	}


	if (setsockopt(sock ,SOL_SOCKET, SO_RCVBUF, &socksize, sizeof(socksize)) <0 ) {
		perror("(warning) setsockopt failed ");

	}
	db = dbm_open(tempmap, O_RDWR + O_CREAT + O_TRUNC, 0777);
	if (db == NULL) {
		logprintf("dbm_open failed %s\n",tempmap);
		perror(tempmap);
		return(-2);
	}
	rmap.map = map; 
	rmap.domain =domain; 
	bzero((char *) &res, sizeof(res));

	if (clnt_call(clnt, getdbm, xdr_hosereq, &rmap, xdr_myfyl, &res, TIMEOUT) != RPC_SUCCESS) {
		logprintf("clnt call to ypxfrd getdbm failed.\n");
		clnt_perror(clnt,"getdbm");
		dbm_deletefile(tempmap);
		return(-3);
	}
	if (res != OK) {
		logprintf("clnt call %s ypxfrd getdbm NOTOK %s %s code=%d\n",master,domain,map,res);
		dbm_deletefile(tempmap);
		return(-4);
	}
	return(0);

}
