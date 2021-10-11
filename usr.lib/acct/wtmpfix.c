/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)wtmpfix.c 1.1 92/07/30 SMI" /* from S5R3 acct:wtmpfix.c 1.3 */
#

/*
 * wtmpfix - adjust wtmp file and remove date changes.
 *
 *	wtmpfix <wtmp1 >wtmp2
 *
 */


# include <stdio.h>
# include <ctype.h>
# include <sys/types.h>
# include <utmp.h>
# include "acctdef.h"
# include <signal.h>

FILE	*Wtmp, *Opw;

char	*Ofile	={ "/tmp/wXXXXXX" };

struct	dtab
{
	long	d_off1;		/* file offset start */
	long	d_off2;		/* file offset stop */
	long	d_adj;		/* time adjustment */
	struct dtab *d_ndp;	/* next record */
};

struct	dtab	*Fdp;		/* list header */
struct	dtab	*Ldp;		/* list trailer */


long	ftell();
struct	utmp	wrec, wrec2;

intr()
{

	signal(SIGINT,SIG_IGN);
	unlink(Ofile);
	exit(1);
}

main(argc, argv)
char	**argv;
{

	static long	recno = 0;
	register struct dtab *dp;

	if(argc < 2){
		argv[argc] = "-";
		argc++;
	}

	if((int)signal(SIGINT,intr) == -1) {
		perror("signal");
		exit(1);
	}

	mktemp(Ofile);
	if((Opw=fopen(Ofile,"w"))==NULL)
		err("cannot make temporary: %s", Ofile);

	while(--argc > 0){
		argv++;
		if(strcmp(*argv,"-")==0)
			Wtmp = stdin;
		else if((Wtmp = fopen(*argv,"r"))==NULL)
			err("Cannot open: %s", *argv);
		while(winp(Wtmp,&wrec)){
#ifdef SYSTEMV
			if(recno == 0 || wrec.ut_type==BOOT_TIME){
#else
			if(recno == 0 || strcmp(wrec.ut_name, BOOT_NAME) == 0){
#endif
				mkdtab(recno,&wrec);
			}
			if(invalid(wrec.ut_name)) {
				fprintf(stderr,
					"wtmpfix: logname \"%8.8s\" changed to \"INVALID\"\n",
					wrec.ut_name);
				strncpy(wrec.ut_name, "INVALID", NSZ);
			}
#ifdef SYSTEMV
			if(wrec.ut_type==OLD_TIME){
#else
			if(strcmp(wrec.ut_line, OLD_NAME) == 0) {
#endif
				if(!winp(Wtmp,&wrec2))
					err("Input truncated at offset %ld",recno);
#ifdef SYSTEMV
				if(wrec2.ut_type!=NEW_TIME)
#else
				if(strcmp(wrec2.ut_line, NEW_NAME) != 0)
#endif
					err("New date expected at offset %ld",recno);
				setdtab(recno,&wrec,&wrec2);
				recno += (2 * sizeof(struct utmp));
				wout(Opw,&wrec);
				wout(Opw,&wrec2);
				continue;
			}
			wout(Opw,&wrec);
			recno += sizeof(struct utmp);
		}
		if(Wtmp!=stdin)
			fclose(Wtmp);
	}
	fclose(Opw);
	if((Opw=fopen(Ofile,"r"))==NULL)
		err("Cannot read from temp: %s", Ofile);
	recno = 0;
	while(winp(Opw,&wrec)){
		adjust(recno,&wrec);
		recno += sizeof(struct utmp);
/*		if(wrec.ut_type==OLD_TIME || wrec.ut_type==NEW_TIME)
			continue;	*/
		wout(stdout,&wrec);
	}
	fclose(Opw);
	unlink(Ofile);
	exit(0);
}

/*	err() writes an error message to the standard error and then
 *	calls the interrupt routine to clean up the temporary file
 *	and exit.  The variable "f" is the format specification that
 *	can contain up to 3 arguments (m1, m2, m3).
 */
err(f,m1,m2,m3)
{

	fprintf(stderr,f,m1,m2,m3);
	fprintf(stderr,"\n");
	intr();
}

/*	winp() reads a record from a utmp.h-type file pointed to
 *	by the stream pointer "f" into the structure whose address
 *	is given by the variable "w".
 *	This reading takes place in two stages: first the raw
 *	records from the utmp.h files are read in (usually /etc/wtmp)
 *	and written into the temporary file (Opw); then the records
 *	are read from the temporary file and placed on the standard 
 *	output.
 */
winp(f,w)
register FILE *f;
register struct utmp *w;
{

	if(fread(w,sizeof(struct utmp),1,f)!=1)
		return 0;
#ifdef SYSTEMV
	if((w->ut_type >= EMPTY) && (w->ut_type <= UTMAXTYPE))
		return ((unsigned)w);
	else {
		fprintf(stderr,"Bad file at offset %ld\n",
			ftell(f)-sizeof(struct utmp));
		fprintf(stderr,"%-12s %-8s %lu %s",
			w->ut_line,w->ut_user,w->ut_time,ctime(&w->ut_time));
		intr();
	}
#else
	return ((unsigned)w);
#endif
}

/*	wout() writes an output record of type utmp.h.  The
 *	variable "f" is a file descripter of either the temp
 *	file or the standard output.  The variable "w" is an
 *	address of the entry in the utmp.h structure.
 */
wout(f,w)
{

	fwrite(w,sizeof(struct utmp),1,f);
}

mkdtab(p,w)
long	p;
register struct utmp *w;
{

	register struct dtab *dp;

	dp = Ldp;
	if(dp == NULL){
		dp = (struct dtab *)calloc(sizeof(struct dtab),1);
		if(dp == NULL)
			err("out of core");
		Fdp = Ldp = dp;
	}
	dp->d_off1 = p;
}

setdtab(p,w1,w2)
long	p;
register struct utmp *w1, *w2;
{

	register struct dtab *dp;

	if((dp=Ldp)==NULL)
		err("no dtab");
	dp->d_off2 = p;
	dp->d_adj = w2->ut_time - w1->ut_time;
	if((Ldp=(struct dtab *)calloc(sizeof(struct dtab),1))==NULL)
		err("out of core");
	Ldp->d_off1 = dp->d_off1;
	dp->d_ndp = Ldp;
}

adjust(p,w)
long	p;
register struct utmp *w;
{

	long pp;
	register struct dtab *dp;

	pp = p;

	for(dp=Fdp;dp!=NULL;dp=dp->d_ndp){
		if(dp->d_adj==0)
			continue;
		if(pp>=dp->d_off1 && pp < dp->d_off2)
			w->ut_time += dp->d_adj;
	}
}

/*
 *	invalid() determines whether the name field adheres to
 *	the criteria set forth in acctcon1.  If the name violates
 *	conventions, it returns a truth value meaning the name is
 *	invalid; if the name is okay, it returns false indicating
 *	the name is not invalid.
 */
#define	VALID	0
#define	INVALID	1

invalid(name)
char	*name;
{
	register int	i;

	for(i=0; i<NSZ; i++) {
		if(name[i] == '\0')
			return(VALID);
		if( ! (isalnum(name[i]) || (name[i] == '$')
#ifdef SYSTEMV
			|| (name[i] == ' ') )) {
#else
			|| (name[i] == ' ') || (name[i] == '@') )) {
#endif
			return(INVALID);
		}
	}
	return(VALID);
}
