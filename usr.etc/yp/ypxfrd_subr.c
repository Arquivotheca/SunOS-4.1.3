#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)ypxfrd_subr.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "ypxfrd.h"
#include <ndbm.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#if (defined (vax) || defined(i386))
#define DOSWAB 1
#endif
getchild()
{
	while (wait3(0, WNOHANG, 0) >= 0);

}

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = {25, 0};

/* per connection stuff */
struct mycon {
	DBM            *db;
	int             lblk;
	int             firstd;
	datum           key;
};


bool_t          xdr_pages();
bool_t          xdr_dirs();
bool_t          xdr_myfyl();
bool_t
xdr_myfyl(xdrs, objp)
	XDR            *xdrs;
	struct mycon   *objp;
{
	int             ans = OK;
	if (!xdr_answer(xdrs, &ans))
		return (FALSE);
	if (!xdr_pages(xdrs, objp))
		return (FALSE);
	if (!xdr_dirs(xdrs, objp))
		return (FALSE);
	return (TRUE);
}
bool_t
xdr_pages(xdrs, m)
	XDR            *xdrs;
	struct mycon   *m;
{
	static struct pag res;
	bool_t          false = FALSE;
	bool_t          true = TRUE;
#ifdef DOSWAB
	short          *s;
	int             i;
	int             cnt;
#endif
	res.status = mygetpage(res.pag_u.ok.blkdat, &(res.pag_u.ok.blkno), m);
#ifdef DOSWAB
	s = (short *) res.pag_u.ok.blkdat;
	cnt = s[0];
	for (i = 0; i <= cnt; i++)
		s[i] = ntohs(s[i]);
#endif

	if (!xdr_pag(xdrs, &res))
		return (FALSE);

	while (res.status == OK) {
		if (!xdr_bool(xdrs, &true))
			return (FALSE);
		res.status = mygetpage(res.pag_u.ok.blkdat, &(res.pag_u.ok.blkno), m);
#ifdef DOSWAB
		s = (short *) res.pag_u.ok.blkdat;
		cnt = s[0];
		for (i = 0; i <= cnt; i++)
			s[i] = ntohs(s[i]);
#endif
		if (!xdr_pag(xdrs, &res))
			return (FALSE);
	}

	return (xdr_bool(xdrs, &false));
}


mygetdir(block, no, m)
	char           *block;
	int            *no;
	struct mycon   *m;
{
	int             status;
	int             len;
	if (m->firstd == 0) {
		lseek(m->db->dbm_dirf, 0, 0);
		m->firstd = 1;
	} else
		m->firstd++;

	len = read(m->db->dbm_dirf, block, DBLKSIZ);
	*no = (m->firstd) - 1;
	status = OK;
	/* 
	 * printf("dir block %d\n", (m->firstd) - 1);
	 */ 

	if (len < 0) {
		perror("read directory");
		status = GETDBM_ERROR;
	} else if (len == 0) {
		status = GETDBM_EOF;
		/*
		 * printf("dir EOF\n");
		 */
	}
	return (status);
}
bool_t
xdr_dirs(xdrs, m)
	XDR            *xdrs;
	struct mycon   *m;
{
	static struct dir res;
	bool_t          false = FALSE;
	bool_t          true = TRUE;
	res.status = mygetdir(res.dir_u.ok.blkdat, &(res.dir_u.ok.blkno), m);

	if (!xdr_dir(xdrs, &res))
		return (FALSE);

	while (res.status == OK) {
		if (!xdr_bool(xdrs, &true))
			return (FALSE);
		res.status = mygetdir(res.dir_u.ok.blkdat, &(res.dir_u.ok.blkno), m);
		if (!xdr_dir(xdrs, &res))
			return (FALSE);
	}

	return (xdr_bool(xdrs, &false));
}


dbmfyl         *
getdbm_1(argp, clnt)
	hosereq        *argp;
	struct svc_req *clnt;
{
	int             res;
	int             pid;
	int             socket;
	int             socksize = 24 * 1024;
	struct mycon    m;
	SVCXPRT        *xprt;
	char            path[1024];
	xprt = clnt->rq_xprt;
	socket = xprt->xp_sock;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, getchild);
	(void) setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &socksize, sizeof(socksize));
	if (strlen(argp->domain) + strlen(argp->map) + strlen("/var/yp//x") > 1024) {
		res = GETDBM_ERROR;
		if (!svc_sendreply(clnt->rq_xprt, xdr_answer, &res)) {
			svcerr_systemerr(clnt->rq_xprt);
		}
		return (NULL);
	}
	sprintf(path, "/var/yp/%s/%s", argp->domain, argp->map);
	pid = fork();
	if (pid < 0) {
		perror("fork");

		res = GETDBM_ERROR;
		if (!svc_sendreply(clnt->rq_xprt, xdr_answer, &res)) {
			svcerr_systemerr(clnt->rq_xprt);
		}
		return (NULL);
	}
	if (pid != 0)
		return (NULL);

	m.db = dbm_open(path, 0, 0);
	if (m.db == NULL) {
		perror(path);
		res = GETDBM_ERROR;
		if (!svc_sendreply(clnt->rq_xprt, xdr_answer, &res)) {
			svcerr_systemerr(clnt->rq_xprt);
		}
		exit(0);
		return (NULL);
	}
	m.key = dbm_firstkey(m.db);
	m.lblk = -1;
	m.firstd = 0;

	if (!svc_sendreply(clnt->rq_xprt, xdr_myfyl, &m)) {
		svcerr_systemerr(clnt->rq_xprt);
	}
	dbm_close(m.db);
	exit(0);

	return (NULL);
}

datum
mydbm_topkey(db, okey)
	DBM            *db;
	datum           okey;
{
	datum           ans;
	datum           tmp;
	register char  *buf;
	int             n;
	register short *sp;
	register        t;
	datum           item;
	register        m;
	register char  *p1, *p2;

	buf = db->dbm_pagbuf;
	sp = (short *) buf;
	/* find the maximum key in cmpdatum order */

	if ((unsigned) 0 >= sp[0]) {
		return (okey);

	} else {
		ans.dptr = buf + sp[1];
		ans.dsize = PBLKSIZ - sp[1];
	}
	for (n = 2;; n += 2) {



		if ((unsigned) n >= sp[0]) {
			if (ans.dptr == NULL) {
				return (okey);
			} else {
				return (ans);
			}

		} else {
			t = PBLKSIZ;
			if (n > 0)
				t = sp[n];
			tmp.dptr = buf + sp[n + 1];
			tmp.dsize = t - sp[n + 1];
		}


		m = tmp.dsize;
		if (m != ans.dsize) {
			if ((m - ans.dsize) < 0)
				ans = tmp;
		} else if (m == 0) {
		} else {
			p1 = tmp.dptr;
			p2 = ans.dptr;
			do
				if (*p1++ != *p2++) {
					if ((*--p1 - *--p2) < 0)
						ans = tmp;
					break;
				}
			while (--m);
		}
	}





}

datum           dbm_do_nextkey();
int
mygetpage(block, pageno, m)
	int            *pageno;
	char           *block;
	struct mycon   *m;
{

	for (; m->key.dptr; m->key = dbm_do_nextkey(m->db, m->key)) {

		if (m->db->dbm_pagbno != m->lblk) {
			/*
			 * printf("block=%d lblk=%d\n", m->db->dbm_pagbno,
			 * m->lblk);
			 */
			m->lblk = m->db->dbm_pagbno;
			*pageno = m->lblk;
			bcopy(m->db->dbm_pagbuf, block, PBLKSIZ);
			m->key = mydbm_topkey(m->db, m->key);	/* advance key on first *
								 * try */
			m->key = dbm_do_nextkey(m->db, m->key);
			return (OK);
		}
	}
	/* 
	 * printf("EOF\n");
	 */
	return (GETDBM_EOF);
}
main(argc, argv)
int argc;
char **argv;
{
	/*process initial -v option to not fork*/
	if (!((argc > 1)  && (!strcmp("-v",argv[1])))) detach();
	else {
		argc--; 
		argv++;
	}

	return(_main(argc,argv));

}

static char logfile[] = "/var/yp/ypxfrd.log" ;
detach() {
	int i;
	int t;
	int pid;
	int d=getdtablesize();

	pid = fork ();
	if (pid == -1) {
		perror("cant fork\n");
		exit (-1);
	}
	else if (pid !=  0) exit(0);
	/*child*/

	close (0);
	if (access(logfile, W_OK)) {
		close (1);
		close (2);
	} else {
		(void) freopen(logfile, "a", stderr);
		(void) freopen(logfile, "a", stdout);
	}
	for (i=3;i<d;i++) close(i);
	t=open("/dev/tty",2);
	if (t >= 0) {
	   (void) ioctl(t, (int) TIOCNOTTY, (char *) 0);
	   (void) close(t);
	 }
								  

	setpgrp(getpid(),getpid());



}
