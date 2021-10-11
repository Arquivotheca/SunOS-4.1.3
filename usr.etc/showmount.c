#ifndef lint
static char sccsid[] = "@(#)showmount.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * showmount
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>

struct timeval TIMEOUT = { 25, 0 };

int sorthost();
int sortpath();
#define	NTABLEENTRIES	2048
struct mountlist *table[NTABLEENTRIES];

main(argc, argv)
	int argc;
	char **argv;
{
	
	int aflg = 0, dflg = 0, eflg = 0;
	int err, addr;
	struct mountlist *ml = NULL;
	struct mountlist **tb, **endtb;
	struct hostent *hp;
	char *host = NULL, hostbuf[256];
	char *last;
	CLIENT *cl;

	while(--argc) {
		if (argv[1][0] == '-') {
			switch(argv[1][1]) {
				case 'a':
					aflg++;
					break;
				case 'd':
					dflg++;
					break;
				case 'e':
					eflg++;
					break;
				default:
					usage();
					exit(1);
			}
		}
		else if (host) {
			usage();
			exit(1);
		}
		else
			host = argv[1];
		argv++;
	}
	if (host == NULL) {
		if (gethostname(hostbuf, sizeof(hostbuf)) < 0) {
			perror("showmount: gethostname");
			exit(1);
		}
		host = hostbuf;
	}

	/*
	 * First try tcp, then drop back to udp if
	 * tcp is unavailable (an old version of mountd perhaps)
	 * Using tcp is preferred because it can handle
	 * arbitrarily long export lists.
	 */
	cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "tcp");
	if (cl == NULL) {
		cl = clnt_create(host, MOUNTPROG, MOUNTVERS, "udp");
		if (cl == NULL) {
			clnt_pcreateerror(host);
			exit(1);
		}
	}

	if (eflg) {
		printex(cl, host);
		if (aflg + dflg == 0) {
			exit(0);
		}
	}

	if (err = clnt_call(cl, MOUNTPROC_DUMP,
			    xdr_void, 0, xdr_mountlist, &ml, TIMEOUT)) {
		fprintf(stderr, "showmount: %s\n", clnt_sperrno(err));
		exit(1);
	}
	tb = table;
	for (; ml != NULL && tb < &table[NTABLEENTRIES]; ml = ml->ml_nxt)
		*tb++ = ml;
	if (ml != NULL && tb == &table[NTABLEENTRIES])
		printf("showmount:  table overflow:  only %d entries shown\n",
			NTABLEENTRIES);
	endtb = tb;
	if (dflg)
	    qsort(table, endtb - table, sizeof(struct mountlist *), sortpath);
	else
	    qsort(table, endtb - table, sizeof(struct mountlist *), sorthost);
	if (aflg) {
		for (tb = table; tb < endtb; tb++)
			printf("%s:%s\n", (*tb)->ml_name, (*tb)->ml_path);
	}
	else if (dflg) {
		last = "";
		for (tb = table; tb < endtb; tb++) {
			if (strcmp(last, (*tb)->ml_path))
				printf("%s\n", (*tb)->ml_path);
			last = (*tb)->ml_path;
		}
	}
	else {
		last = "";
		for (tb = table; tb < endtb; tb++) {
			if (strcmp(last, (*tb)->ml_name))
				printf("%s\n", (*tb)->ml_name);
			last = (*tb)->ml_name;
		}
	}
	exit(0);
	/* NOTREACHED */
}

sorthost(a, b)
	struct mountlist **a,**b;
{
	return strcmp((*a)->ml_name, (*b)->ml_name);
}

sortpath(a, b)
	struct mountlist **a,**b;
{
	return strcmp((*a)->ml_path, (*b)->ml_path);
}

usage()
{
	fprintf(stderr, "showmount [-a] [-d] [-e] [host]\n");
}

printex(cl, host)
	CLIENT *cl;
	char *host;
{
	struct exports *ex = NULL;
	struct exports *e;
	struct groups *gr;
	enum clnt_stat err;
	int max;

	if (err = clnt_call(cl, MOUNTPROC_EXPORT,
	    xdr_void, 0, xdr_exports, &ex, TIMEOUT)) {
		fprintf(stderr, "showmount: %s\n", clnt_sperrno(err));
		exit(1);
	}

	if (ex == NULL) {
		fprintf(stdout, "no exported file systems for %s\n", host);
	} else {
		fprintf(stdout, "export list for %s:\n", host);
	}
	max = 0;
	for (e = ex; e != NULL; e = e->ex_next) {
		if (strlen(e->ex_name) > max) {
			max = strlen(e->ex_name);
		}
	}
	while (ex) {
		fprintf(stdout, "%-*s ", max, ex->ex_name);
		gr = ex->ex_groups;
		if (gr == NULL) {
			fprintf(stdout, "(everyone)");
		}
		while (gr) {
			fprintf(stdout, "%s", gr->g_name);
			gr = gr->g_next;
			if (gr) {
				fprintf(stdout, ",");
			}
		}
		fprintf(stdout, "\n");
		ex = ex->ex_next;
	}
}
