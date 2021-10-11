/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)dmesg.c 1.1 92/07/30 SMI"; /* from UCB 5.4 2/20/86 */
#endif not lint

/*
 *	Suck up system messages
 *	dmesg
 *		print current buffer
 *	dmesg -
 *		print and update incremental history
 */

#include <stdio.h>
#include <sys/param.h>
#include <nlist.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/vm.h>
#include <sys/msgbuf.h>

struct	msgbuf_hd msgbuf_hd;
struct	msgbuf *msgbufp, *omsgbufp;
int	msgbufsize;
int	sflg;
int	of	= -1;

struct	nlist nl[2] = {
	{ "_msgbuf" },
	{ "" }
};

extern char *malloc();

main(argc, argv)
char **argv;
{
	int mem;
	register char *mp, *mlast, *mend;
	char *mstart;
	int newmsg, sawnl, ignore;

	if (argc>1 && argv[1][0] == '-') {
		sflg++;
		argc--;
		argv++;
	}
	nlist(argc>2? argv[2]:"/vmunix", nl);
	if (nl[0].n_type==0)
		done("Can't get kernel namelist\n");
	if ((mem = open((argc>1? argv[1]: "/dev/kmem"), 0)) < 0)
		done("Can't read kernel memory\n");
	lseek(mem, (long)nl[0].n_value, 0);
	if (read(mem, &msgbuf_hd, sizeof (msgbuf_hd)) != sizeof (msgbuf_hd))
		done("Can't read msgbuf header\n");
	if (msgbuf_hd.msgh_magic != MSG_MAGIC)
		done("Magic number wrong (namelist mismatch?)\n");
	msgbufsize = sizeof(struct msgbuf_hd) + msgbuf_hd.msgh_size;
	msgbufp = (struct msgbuf *) malloc(msgbufsize);
	if (msgbufp == NULL)
		done("Can't allocate memory\n");
	lseek(mem, (long)nl[0].n_value, 0);
	if (read(mem, (char *)msgbufp, msgbufsize) != msgbufsize)
		done("Can't read msgbuf\n");
	if (msgbufp->msg_bufx >= msgbuf_hd.msgh_size)
		msgbufp->msg_bufx = 0;
	newmsg = 1;
	mend = &msgbufp->msg_bufc[msgbuf_hd.msgh_size];
	if (sflg) {
		of = open("/var/adm/msgbuf", O_RDWR | O_CREAT, 0644);
		if (of < 0)
			done("Can't open /var/adm/msgbuf\n");
		omsgbufp = (struct msgbuf *) malloc(msgbufsize);
		if (omsgbufp == NULL)
			done("Can't allocate memory\n");
		bzero((char *)omsgbufp, msgbufsize);
		read(of, (char *)omsgbufp, msgbufsize);
		lseek(of, 0L, 0);
		if (omsgbufp->msg_magic == MSG_MAGIC &&
		    omsgbufp->msg_size == msgbuf_hd.msgh_size) {
			register char *omp, *omend;
			
			if (omsgbufp->msg_bufx >= msgbuf_hd.msgh_size)
				omsgbufp->msg_bufx = 0;
			mp = &msgbufp->msg_bufc[msgbufp->msg_bufx];
			omp = &omsgbufp->msg_bufc[msgbufp->msg_bufx];
			omend = &omsgbufp->msg_bufc[msgbuf_hd.msgh_size];
			mstart = &msgbufp->msg_bufc[omsgbufp->msg_bufx];
			newmsg = 0;
			do {
				if (*mp++ != *omp++) {
					newmsg = 1;
					break;
				}
				if (mp >= mend)
					mp = &msgbufp->msg_bufc[0];
				if (omp >= omend)
					omp = &omsgbufp->msg_bufc[0];
			} while (mp != mstart);
			if (newmsg == 0 &&
			    omsgbufp->msg_bufx == msgbufp->msg_bufx)
				exit(0);
		}
		if (newmsg) {
			pdate();
			printf("...\n");
		}
	}
	pdate();
	if (newmsg)
		mstart = &msgbufp->msg_bufc[msgbufp->msg_bufx];
	sawnl = 1;
	ignore = 0;
	mp = mstart;
	mlast = &msgbufp->msg_bufc[msgbufp->msg_bufx];
	do {
		register char c;

		c = *mp++;
		if (sawnl && c == '<')
			ignore = 1;
		if (c && (c & 0200) == 0 && !ignore)
			putchar(c);
		if (ignore && c == '>')
			ignore = 0;
		sawnl = (c == '\n');
		if (mp >= mend)
			mp = &msgbufp->msg_bufc[0];
	} while (mp != mlast);
	done((char *)NULL);
	/* NOTREACHED */
}

done(s)
char *s;
{

	if (s) {
		pdate();
		printf(s);
	} else if (of != -1) {
		write(of, (char *)msgbufp, msgbufsize);
	}
	exit(s!=NULL);
}

pdate()
{
	extern char *ctime();
	static firstime;
	time_t tbuf;

	if (firstime==0) {
		firstime++;
		time(&tbuf);
		printf("\n%.12s\n", ctime(&tbuf)+4);
	}
}
