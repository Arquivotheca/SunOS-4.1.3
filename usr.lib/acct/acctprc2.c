/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)acctprc2.c 1.1 92/07/30 SMI" /* from S5R3 acct:acctprc2.c 1.6 */
/*
 *	acctprc2 <ptmp1 >ptacct
 *	reads std. input (in ptmp.h/ascii format)
 *	hashes items with identical uid/name together, sums times
 *	sorts in uid/name order, writes tacct.h records to output
 */

#include <sys/types.h>
#include "acctdef.h"
#include <stdio.h>
#include "ptmp.h"
#include "tacct.h"

struct	ptmp	pb;
struct	tacct	tb;

struct	utab	{
	uid_t	ut_uid;
	char	ut_name[NSZ];
	float	ut_cpu[2];	/* cpu time (mins) */
	float	ut_kcore[2];	/* kcore-mins */
	long	ut_pc;		/* # processes */
} ub[A_USIZE];
static	usize;
int	ucmp();

static int pagesize;
static int kperpage;
static int pageperk;
#define	KCORE(pages)	(pagesize > 1024 ? \
			    (pages)*kperpage : \
			    (pages)/pageperk)

main(argc, argv)
char **argv;
{
	pagesize = getpagesize();
	if (pagesize > 1024)
		kperpage = pagesize/1024;
	else
		pageperk = 1024/pagesize;

	while (scanf("%hu\t%s\t%lu\t%lu\t%u",
		&pb.pt_uid,
		pb.pt_name,
		&pb.pt_cpu[0], &pb.pt_cpu[1],
		&pb.pt_mem) != EOF)
			enter(&pb);
	squeeze();
	qsort(ub, usize, sizeof(ub[0]), ucmp);
	output();
}

enter(p)
register struct ptmp *p;
{
	register unsigned i;
	int j;
	double memk;

	/* clear end of short users' names */
	for(i = strlen(p->pt_name) + 1; i < NSZ; p->pt_name[i++] = '\0') ;
	/* now hash the uid and login name */
	for(i = j = 0; p->pt_name[j] != '\0'; ++j)
		i = i*7 + p->pt_name[j];
	i = i*7 + p->pt_uid;
	j = 0;
	for (i %= A_USIZE; ub[i].ut_name[0] && j++ < A_USIZE; i = (i+1)% A_USIZE)
		if (p->pt_uid == ub[i].ut_uid && EQN(p->pt_name, ub[i].ut_name))
			break;
	if (j >= A_USIZE) {
		fprintf(stderr, "acctprc2: INCREASE A_USIZE\n");
		exit(1);
	}
	if (ub[i].ut_name[0] == 0) {	/*this uid not yet in table so enter it*/
		ub[i].ut_uid = p->pt_uid;
		CPYN(ub[i].ut_name, p->pt_name);
	}
	ub[i].ut_cpu[0] += MINT(p->pt_cpu[0]);
	ub[i].ut_cpu[1] += MINT(p->pt_cpu[1]);
	memk = KCORE(pb.pt_mem);
	ub[i].ut_kcore[0] += memk * MINT(p->pt_cpu[0]);
	ub[i].ut_kcore[1] += memk * MINT(p->pt_cpu[1]);
	ub[i].ut_pc++;
}

squeeze()		/*eliminate holes in hash table*/
{
	register i, k;

	for (i = k = 0; i < A_USIZE; i++)
		if (ub[i].ut_name[0]) {
			ub[k].ut_uid = ub[i].ut_uid;
			CPYN(ub[k].ut_name, ub[i].ut_name);
			ub[k].ut_cpu[0] = ub[i].ut_cpu[0];
			ub[k].ut_cpu[1] = ub[i].ut_cpu[1];
			ub[k].ut_kcore[0] = ub[i].ut_kcore[0];
			ub[k].ut_kcore[1] = ub[i].ut_kcore[1];
			ub[k].ut_pc = ub[i].ut_pc;
			k++;
		}
	usize = k;
}

ucmp(p1, p2)
register struct utab *p1, *p2;
{
	if (p1->ut_uid != p2->ut_uid)
		/* the following (short) typecasts are a kludge fix
		 * for a bug in the 5.0 C compiler.  The bug returns a
		 * result that is always positive from the subtraction
		 * because of the unsigned short type of ut_uid.
		 */
		return((short)p1->ut_uid - (short)p2->ut_uid);
	return(strncmp(p1->ut_name, p2->ut_name, NSZ));
}

output()
{
	register i;

	for (i = 0; i < usize; i++) {
		tb.ta_uid = ub[i].ut_uid;
		CPYN(tb.ta_name, ub[i].ut_name);
		tb.ta_cpu[0] = ub[i].ut_cpu[0];
		tb.ta_cpu[1] = ub[i].ut_cpu[1];
		tb.ta_kcore[0] = ub[i].ut_kcore[0];
		tb.ta_kcore[1] = ub[i].ut_kcore[1];
		tb.ta_pc = ub[i].ut_pc;
		fwrite(&tb, sizeof(tb), 1, stdout);
	}
}
