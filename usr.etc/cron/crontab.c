/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static  char sccsid[] = "@(#)crontab.c 1.1 92/07/30 SMI"; /* from S5R3 1.8 */
#endif

/**************************************************************************
 ***			C r o n t a b . c				***
 **************************************************************************

	date:	7/2/82
	description:	This program implements crontab (see cron(1)).
			This program should be set-uid to root.
	files:
		/usr/lib/cron drwxr-xr-x root sys
		/usr/lib/cron/cron.allow -rw-r--r-- root sys
		/usr/lib/cron/cron.deny -rw-r--r-- root sys

 **************************************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h>
#include "cron.h"

#define TMPFILE		"_cron"		/* prefix for tmp file */
#define CRMODE		0400	/* mode for creating crontabs */

#define BADCREATE	"can't create your crontab file in the crontab directory."
#define BADOPEN		"can't open your crontab file"
#define BADREAD		"error reading your crontab file"
#define BADUSAGE	"proper usage is: \n	crontab [file | -e [user] | -l [user] | -r [user] ]"
#define INVALIDUSER	"you are not a valid user (no entry in /etc/passwd)."
#define UINVALIDUSER	"the specified user is not a valid user (no entry in /etc/passwd)."
#define NOTALLOWED	"you are not authorized to use cron.  Sorry."
#define NOTROOT		"you must be super-user to change or read another user's crontab file"
#define UNOTALLOWED	"the specified user is not authorized to use cron."
#define EOLN		"unexpected end of line."
#define UNEXPECT	"unexpected character found in line."
#define OUTOFBOUND	"number out of bounds."

extern int opterr, optind;
extern char *optarg, *xmalloc();
extern int errno;
extern char *errmsg();
int err,cursor,catch();
char *cf,*tnam,line[CTLINESIZE];
char edtemp[5+13+1];

main(argc,argv)
char **argv;
{
	int c, eflag, rflag, lflag, errflg;
	int ruid, euid;
	char login[UNAMESIZE],*getuser();
	extern char *getenv();
	register char *pp;
	register FILE *fp;
	register FILE *tmpfp;
	struct stat stbuf;
	time_t omodtime;
	char *editor;
	char buf[BUFSIZ];

	eflag = 0;
	rflag = 0;
	lflag = 0;
	errflg = 0;
	while ((c=getopt(argc, argv, "elr")) != EOF)
		switch (c) {
			case 'e':
				if (lflag || rflag)
					errflg++;
				else
					eflag++;
				break;
			case 'l':
				if (eflag || rflag)
					errflg++;
				else
					lflag++;
				break;
			case 'r':
				if (eflag || lflag)
					errflg++;
				else
					rflag++;
				break;
			case '?':
				errflg++;
				break;
		}
	argc -= optind;
	argv += optind;
	if (errflg || argc > 1)
		crabort(BADUSAGE);
	euid = geteuid();
	ruid = getuid();
	if ((eflag || lflag || rflag) && argc == 1) {
		if (ruid != 0)
			crabort(NOTROOT);
		pp = *argv++;
		argc--;
		if (getpwnam(pp) == NULL)
			crabort(UINVALIDUSER); 
		if (!allowed(pp,CRONALLOW,CRONDENY))
			crabort(UNOTALLOWED);
	} else {
		pp = getuser(ruid);
		if(pp == NULL)
			crabort(INVALIDUSER); 
		if (!allowed(pp,CRONALLOW,CRONDENY))
			crabort(NOTALLOWED);
	}
	strcpy(login,pp);

	cf = xmalloc(strlen(CRONDIR)+strlen(login)+2);
	strcat(strcat(strcpy(cf,CRONDIR),"/"),login);
	if (rflag) {
		unlink(cf);
		sendmsg(DELETE,login);
		exit(0); 
	}
	if (lflag) {
		if((fp = fopen(cf,"r")) == NULL)
			crabortperror(BADOPEN);
		while(fgets(line,CTLINESIZE,fp) != NULL) {
			fputs(line,stdout);
			if(ferror(stdout))
				crabortperror("write error on standard output");
		}
		fclose(fp);
		if (fflush(stdout) == EOF)
			crabortperror("write error on standard output");
		exit(0);
	}

	if (eflag) {
		if((fp = fopen(cf,"r")) == NULL) {
			if(errno != ENOENT)
				crabortperror(BADOPEN);
		}
		/*
		 * Temporarily give up privileges.
		 */
		setreuid(-1, ruid);
		(void)strcpy(edtemp, "/tmp/crontabXXXXXX");
		(void)mktemp(edtemp);
		if((tmpfp = fopen(edtemp,"w")) == NULL)
			crabortperror("can't create temporary file");
		if(fp != NULL) {
			/*
			 * Copy user's crontab file to temporary file.
			 */
			while(fgets(line,CTLINESIZE,fp) != NULL) {
				fputs(line,tmpfp);
				if(ferror(tmpfp)) {
					fclose(fp);
					fclose(tmpfp);
					crabortperror("write error on temporary file");
				}
			}
			if (ferror(fp)) {
				fclose(fp);
				fclose(tmpfp);
				crabortperror(BADREAD);
			}
			fclose(fp);
		}
		if(fclose(tmpfp) == EOF)
			crabortperror("write error on temporary file");
		if(stat(edtemp, &stbuf) < 0)
			crabortperror("can't stat temporary file");
		omodtime = stbuf.st_mtime;
		editor = getenv("VISUAL");
		if (editor == NULL)
			editor = getenv("EDITOR");
		if (editor == NULL)
			editor = "vi";
		(void)sprintf(buf, "%s %s", editor, edtemp);
		sleep(1);	/* To fix bug on line 205, where
				the mod time can be same even though
				file was changed, because time	
				resolution is 1 second. */
		if (system(buf) == 0) {
			/* sanity checks */
			if((tmpfp = fopen(edtemp, "r")) == NULL)
				crabortperror("can't open temporary file");
			if(fstat(fileno(tmpfp), &stbuf) < 0)
				crabortperror("can't stat temporary file");
			if(stbuf.st_size == 0)
				crabort("temporary file empty");
			if(omodtime == stbuf.st_mtime) {
				(void)unlink(edtemp);
				fprintf(stderr,
				    "The crontab file was not changed.\n");
				exit(0);
			}

			/*
			 * Reclaim privileges.
			 */
			setreuid(-1, euid);
			copycron(tmpfp);
			(void)unlink(edtemp);
		} else {
			/*
			 * Couldn't run editor.
			 */
			(void)unlink(edtemp);
			exit(1);
		}
	} else {
		if (argc==0)
			copycron(stdin);
		else {
			if (access(argv[0],04) || (fp=fopen(argv[0],"r"))==NULL)
				crabortperror(argv[0]);
			copycron(fp);
		}
	}
	sendmsg(ADD,login);
	exit(0);
	/* NOTREACHED */
}


/******************/
copycron(fp)
/******************/
FILE *fp;
{
	FILE *tfp,*fdopen();
	char pid[6],*strcat(),*strcpy();
	int t;

	sprintf(pid,"%-5d",getpid());
	tnam=xmalloc(strlen(CRONDIR)+strlen(TMPFILE)+7);
	strcat(strcat(strcat(strcpy(tnam,CRONDIR),"/"),TMPFILE),pid);
	/* catch SIGINT, SIGHUP, SIGQUIT signals */
	if (signal(SIGINT,catch) == SIG_IGN) signal(SIGINT,SIG_IGN);
	if (signal(SIGHUP,catch) == SIG_IGN) signal(SIGHUP,SIG_IGN);
	if (signal(SIGQUIT,catch) == SIG_IGN) signal(SIGQUIT,SIG_IGN);
	if (signal(SIGTERM,catch) == SIG_IGN) signal(SIGTERM,SIG_IGN);
	if ((t=creat(tnam,CRMODE))==-1) crabortperror(BADCREATE);
	if ((tfp=fdopen(t,"w"))==NULL)
		crabort(BADCREATE); 
	err=0;	/* if errors found, err set to 1 */
	while (fgets(line,CTLINESIZE,fp) != NULL) {
		cursor=0;
		while(line[cursor] == ' ' || line[cursor] == '\t')
			cursor++;
		if(line[cursor] == '#')
			goto cont;
		if (next_field(0,59)) continue;
		if (next_field(0,23)) continue;
		if (next_field(1,31)) continue;
		if (next_field(1,12)) continue;
		if (next_field(0,7)) continue;
		if (line[++cursor] == '\0') {
			cerror(EOLN);
			continue; 
		}
cont:
		if (fputs(line,tfp) == EOF)
			crabortperror(BADCREATE); 
	}
	if (ferror(fp)) {
		fclose(fp);
		/*
		 * Don't do an "fclose", as that may do a "write" which may
		 * fail, and cause the wrong "perror" error message to be
		 * printed.
		 */
		(void)close(fileno(tfp));
		crabortperror(BADREAD);
	}
	fclose(fp);
	if (fclose(tfp) == EOF)
		crabortperror(BADCREATE); 
	if (!err) {
		/* make file tfp the new crontab */
		if (rename(tnam,cf)==-1)
			crabortperror(BADCREATE); 
	} else
		unlink(tnam);
	tnam = NULL;	/* file no longer exists */
}


/*****************/
next_field(lower,upper)
/*****************/
int lower,upper;
{
	int num,num2;

	while ((line[cursor]==' ') || (line[cursor]=='\t')) cursor++;
	if (line[cursor] == '\0') {
		cerror(EOLN);
		return(1); 
	}
	if (line[cursor] == '*') {
		cursor++;
		if ((line[cursor]!=' ') && (line[cursor]!='\t')) {
			cerror(UNEXPECT);
			return(1); 
		}
		return(0); 
	}
	while (TRUE) {
		if (!isdigit(line[cursor])) {
			cerror(UNEXPECT);
			return(1); 
		}
		num = 0;
		do { 
			num = num*10 + (line[cursor]-'0'); 
		}			while (isdigit(line[++cursor]));
		if ((num<lower) || (num>upper)) {
			cerror(OUTOFBOUND);
			return(1); 
		}
		if (line[cursor]=='-') {
			if (!isdigit(line[++cursor])) {
				cerror(UNEXPECT);
				return(1); 
			}
			num2 = 0;
			do { 
				num2 = num2*10 + (line[cursor]-'0'); 
			}				while (isdigit(line[++cursor]));
			if ((num2<lower) || (num2>upper)) {
				cerror(OUTOFBOUND);
				return(1); 
			}
		}
		if ((line[cursor]==' ') || (line[cursor]=='\t')) break;
		if (line[cursor]=='\0') {
			cerror(EOLN);
			return(1); 
		}
		if (line[cursor++]!=',') {
			cerror(UNEXPECT);
			return(1); 
		}
	}
	return(0);
}


/**********/
cerror(msg)
/**********/
char *msg;
{
	fprintf(stderr,"%scrontab: error on previous line; %s\n",line,msg);
	err=1;
}


/**********/
catch()
/**********/
{
	unlink(tnam);
	exit(1);
}


/**********/
crabort(msg)
/**********/
char *msg;
{
	int sverrno;

	if (strcmp(edtemp, "") != 0) {
		sverrno = errno;
		(void)unlink(edtemp);
		errno = sverrno;
	}
	if (tnam != NULL) {
		sverrno = errno;
		(void)unlink(tnam);
		errno = sverrno;
	}
	fprintf(stderr,"crontab: %s\n",msg);
	exit(1);
}

/****************/
crabortperror(msg)
/****************/
char *msg;
{
	int sverrno;

	if (strcmp(edtemp, "") != 0) {
		sverrno = errno;
		(void)unlink(edtemp);
		errno = sverrno;
	}
	fprintf(stderr,"crontab: %s: %s\n", msg, errmsg(errno));
	exit(1);
}

/***********/
sendmsg(action,fname)
/****************/
char action;
char *fname;
{

	static	int	msgfd = -2;
	struct	message	*pmsg;
	register int	i;

	pmsg = &msgbuf;
	if(msgfd == -2)
		if((msgfd = open(FIFO,O_WRONLY|O_NDELAY)) < 0) {
			if(errno == ENXIO || errno == ENOENT)
				fprintf(stderr,"cron may not be running - call your system administrator\n");
			else
				perror("crontab: error in message queue open");
			return;
		}
	pmsg->etype = CRON;
	pmsg->action = action;
	strncpy(pmsg->fname,fname,FLEN);
	if((i = write(msgfd,pmsg,sizeof(struct message))) < 0)
		perror("crontab: error in message send");
	else if (i != sizeof(struct message))
		fprintf(stderr,"crontab: error in message send: Premature EOF\n");
}
