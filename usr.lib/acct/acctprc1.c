/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acctprc1.c 1.1 92/07/30 SMI" /* from S5R3 acct:acctprc1.c 1.5 */
/*
 *	acctprc1 [ctmpfile]
 *	reads std. input (acct.h format), adds login names
 *	writes std. output (ptmp.h/ascii format)
 *	if ctmpfile is given, it is expected have ctmp.h/ascii data,
 *	sorted by uid/name; it is used to make better guesses at login names
 */

#include <sys/types.h>
#include <sys/param.h>
#include "acctdef.h"
#include <stdio.h>
#include <pwd.h>
#include <sys/acct.h>
#include "ctmp.h"
#include "ptmp.h"
#define MYKIND(flag)	((flag & ACCTF) == 0)

struct	acct	ab;
struct	ctmp	cb;
struct	ptmp	pb;

struct urec {				/* 1 for each distinct uid/name */
	uid_t	ur_uid;			/* sorted by uid/name */
	char	ur_name[NSZ];
	struct srec	*ur_srec;		/* ptr to first session */
	short	ur_cnt;			/* # sessions */
} ur[A_USIZE];
struct urec *urlast	= ur;

int	ssize;
struct srec {				/* 1 for each distinct session */
	dev_t	sr_tty;			/* dev, used to connect with process*/
	time_t	sr_start;		/* start time of session */
	time_t	sr_end;			/* end time of session */
} sr[A_SSIZE];
char	*getname(), *getnamc();
char	*uidtonam();

main(argc, argv)
char **argv;
{
	long	elaps[2];
	long	etime, stime, mem;
	long	expand();

	while (--argc > 0) {
		if (**++argv == '-')
			switch(*++*argv) {
			}
		else {
			readctmp(*argv);
		}
	}

	while (fread(&ab, sizeof(ab), 1, stdin) == 1) {
		if (!MYKIND(ab.ac_flag))
			continue;
		pb.pt_uid = ab.ac_uid;
		CPYN(pb.pt_name, getname(ab.ac_uid, ab.ac_tty, ab.ac_btime));
		/*
		 * approximate cpu P/NP split as same as elapsed time
		 */
		if ((etime = SECS(expand(ab.ac_etime))) == 0)
			etime = 1;
		stime = expand(ab.ac_stime) + expand(ab.ac_utime);
		mem = expand(ab.ac_mem);
		pnpsplit(ab.ac_btime, etime, elaps);
		pb.pt_cpu[0] = (double)stime * (double)elaps[0] / etime;
		pb.pt_cpu[1] = (stime > pb.pt_cpu[0])? stime - pb.pt_cpu[0] : 0;
		pb.pt_cpu[1] = stime - pb.pt_cpu[0];
		if (stime)
			pb.pt_mem = (mem + stime - 1) / stime;
		else
			pb.pt_mem = 0;	/* unlikely */
		printf("%u\t%.8s\t%lu\t%lu\t%u\n",
			pb.pt_uid,
			pb.pt_name,
			pb.pt_cpu[0], pb.pt_cpu[1],
			pb.pt_mem);
	}
}

/*
 *	return ptr to name corresponding to uid
 *	try ctmp first, then use uidtonam (internal list or passwd file)
 */
char *
getname(uid, tty, start)
uid_t	uid;
dev_t	tty;
time_t	start;
{
	register char *p;

	if ((p = getnamc(uid, tty, start)) != NULL)
		return(p);
	return(uidtonam(uid));
}

/*
 *	read ctmp file, build up urec-srec data structures for
 *	later use by getnamc
 */
readctmp(fname)
char *fname;
{
	FILE *fp;
	register struct urec *up;
	register struct srec *sp;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "acctprc1: can't open %s\n", fname);
		return;
	}

	up = NULL;
	sp = sr;
	while (fscanf(fp, "%hd\t%hu\t%s\t%lu\t%lu\t%lu\t%*[^\n]",
		&cb.ct_tty,
		&cb.ct_uid,
		cb.ct_name,
		&cb.ct_con[0],
		&cb.ct_con[1],
		&cb.ct_start) != EOF) {
		if (up == NULL || cb.ct_uid != up->ur_uid ||
			!EQN(cb.ct_name, up->ur_name)) {
			if (up == NULL)
				up = ur;
			else if (++up >= &ur[A_USIZE]) {
				fprintf(stderr, "acctprc1: INCREASE A_USIZE\n");
				exit(1);
			}
			up->ur_uid = cb.ct_uid;
			CPYN(up->ur_name, cb.ct_name);
			up->ur_srec = sp;
			up->ur_cnt = 0;
		}
		if (sp >= &sr[A_SSIZE-1]) {
			fprintf(stderr, "acctprc1: INCREASE A_SSIZE\n");
			exit(1);
		}
		sp->sr_tty = cb.ct_tty;
		sp->sr_start = cb.ct_start;
		sp->sr_end = cb.ct_start + cb.ct_con[0] + cb.ct_con[1];
		sp++;
		up->ur_cnt++;
	}
	if (up != NULL)
		urlast = ++up;
	fclose(fp);
}

/*
 *	using urec-srec data (if any), make best guess at login name
 *	corresponding to uid, return ptr to the name.
 *	must match on tty; use start time to help guess
 *	for any urec having same uid as uid, search array of associated
 *	srecs for those having same tty
 *	if start time of process is within range of session, that's it
 *	if none can be found within range, give it to person of same uid
 *	who last logged off on that terminal
 */
char *
getnamc(uid, tty, start)
register uid_t uid;
dev_t	tty;
time_t	start;
{
	register struct urec *up;
	register struct srec *sp;
	struct srec *splast;
	long latest;
	char *guess;

	latest = 0;
	guess = NULL;
	for (up = ur; up < urlast && uid >= up->ur_uid; up++)
		if (uid == up->ur_uid) {
			sp = up->ur_srec;
			splast = sp+up->ur_cnt;
			for (; sp < splast; sp++)
				if (tty == sp->sr_tty) {
					if (start >= sp->sr_start &&
						start <= sp->sr_end)
						return(up->ur_name);
					if (start >= sp->sr_start &&
						sp->sr_end > latest) {
						latest = sp->sr_end;
						guess = up->ur_name;
					}
				}
		}

	return(guess);
}
