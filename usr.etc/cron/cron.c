/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static  char sccsid[] = "@(#)cron.c 1.1 92/07/30 SMI"; /* from S5R3 1.15 */
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/label.h>
#include <sys/audit.h>
#include <string.h>
#include <pwd.h>
#include <pwdadj.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <sys/termios.h>
#include "cron.h"

#define MAIL		"/usr/lib/sendmail -oeq -oi"	/* mail command to use */

#define TMPINFILE	"/tmp/crinXXXXXX"  /* file to put stdin in for cmd  */
#define	TMPDIR		"/tmp"
#define	PFX		"crout"
#define TMPOUTFILE	"/tmp/croutXXXXXX" /* file to place stdout, stderr */

#define INMODE		00400		/* mode for stdin file	*/
#define OUTMODE		00600		/* mode for stdout file */
#define ISUID		06000		/* mode for verifing at jobs */

#define INFINITY	2147483647L	/* upper bound on time	*/
#define CUSHION		120L
#define	MAXRUN		25		/* max total jobs allowed in system */
#define ZOMB		100		/* proc slot used for mailing output */
#define DAYLIGHT_OFFSET	3		/* largest daylight offset (hrs) */

#define	JOBF		'j'
#define	NICEF		'n'
#define	USERF		'u'
#define WAITF		'w'

#define BCHAR		'>'
#define	ECHAR		'<'

#define BADCD		"Can't change directory to %s: %m"
#define NOREADDIR	"Can't read %s: %m"

#define BADJOBOPEN	"Unable to read your at job"
#define BADSTAT		"Can't access your crontab file - resubmit it"
#define CANTCDHOME	"Can't change directory to your home directory"
#define CANTEXECSH	"Unable to exec the shell for one of your commands"
#define EOLN		"Unexpected end of line"
#define NOREAD		"Can't read your crontab file - resubmit it"
#define NOSTDIN		"Unable to create a standard input file for one of your crontab commands"
#define OUTOFBOUND	"Number too large or too small for field"
#define UNEXPECT	"Unexpected symbol found"

#define	ERR_CRONTABENT	0	/* error in crontab file entry */
#define	ERR_UNIXERR	1	/* error in some system call */
#define	ERR_CANTEXECCRON 2	/* error in setting up "cron" job environment */
#define	ERR_CANTEXECAT	3	/* error in setting up "at" job environment */

#define DIDFORK didfork
#define NOFORK !didfork

struct event {	
	time_t time;	/* time of the event	*/
	short etype;	/* what type of event; 0=cron, 1=at	*/
	char *cmd;	/* command for cron, job name for at	*/
	struct usr *u;	/* ptr to the owner (usr) of this event	*/
	struct event *link; 	/* ptr to another event for this user */
	union { 
		struct { /* for crontab events */
			char *minute;	/*  (these	*/
			char *hour;	/*   fields	*/
			char *daymon;	/*   are	*/
			char *month;	/*   from	*/
			char *dayweek;	/*   crontab)	*/
			char *input;	/* ptr to stdin	*/
		} ct;
		struct { /* for at events */
			short exists;	/* for revising at events	*/
			int eventid;	/* for el_remove-ing at events	*/
		} at;
	} of; 
};

struct usr {	
	char *name;	/* name of user (e.g. "root")	*/
	char *home;	/* home directory for user	*/
	int uid;	/* user id	*/
	int gid;	/* group id	*/
#ifdef ATLIMIT
	int aruncnt;	/* counter for running jobs per uid */
#endif
#ifdef CRONLIMIT
	int cruncnt;	/* counter for running cron jobs per uid */
#endif
	int ctid;	/* for el_remove-ing crontab events */
	short ctexists;	/* for revising crontab events	*/
	struct event *ctevents;	/* list of this usr's crontab events */
	struct event *atevents;	/* list of this usr's at events */
	struct usr *nextusr; 
};	/* ptr to next user	*/

struct	queue
{
	int njob;	/* limit */
	int nice;	/* nice for execution */
	int nwait;	/* wait time to next execution attempt */
	int nrun;	/* number running */
}	
	qd = {100, 2, 60},		/* default values for queue defs */
	qt[NQUEUE];

struct	queue	qq;
int	wait_time = 60;

struct	runinfo
{
	int	pid;
	short	que;
	struct  usr *rusr;	/* pointer to usr struct */
	char 	*outfile;	/* file where stdout & stderr are trapped */
	short	jobtype;	/* what type of event: 0 = cron, 1 = at */
	char	*jobname;	/* command for "cron", jobname for "at" */
	int	mailwhendone;	/* 1 = send mail even if no output */
}	rt[MAXRUN];

short didfork = 0;	/* flag to see if I'm process group leader */
int msgfd;		/* file descriptor for fifo queue */
int ecid=1;		/* for giving event classes distinguishable id names 
			   for el_remove'ing them.  MUST be initialized to 1 */
int delayed;		/* is job being rescheduled or did it run first time */
int notexpired;		/* time for next job has not come */
int cwd;		/* current working directory */
int running;		/* zero when no jobs are executing */
struct event *next_event;	/* the next event to execute	*/
struct usr *uhead;	/* ptr to the list of users	*/
struct usr *ulast;	/* ptr to last usr table entry */
time_t init_time,num(),time(),timelocal();
int timeout();
extern char *malloc();
extern char *xmalloc();
extern FILE *fopen_configfile();

/* user's default environment for the shell */
/* tzone must be the last entry */
char homedir[100]="HOME=";
char logname[50]="LOGNAME=";
char user[50]="USER=";
char tzone[20]="TZ=";
char *envinit[]={
	homedir,
	logname,
	user,
	"PATH=:/usr/ucb:/bin:/usr/bin",
	"SHELL=/bin/sh",
	tzone,
	NULL
};
#define	TZONE_INDEX	((sizeof envinit / sizeof envinit[0]) - 2)
extern char **environ;
void cronend();

main(argc,argv)
int argc;
char **argv;
{
	time_t t,t_old;
	time_t last_time;
	time_t t_check;
	time_t ne_time;		/* amt of time until next event execution */
	time_t next_time();
	struct usr *u,*u2;
	struct event *e,*e2,*eprev;
	long seconds;
	int rfork;
	register int i;

begin:
				/* fork unless 'nofork' is specified */
	if((argc <= 1) || (strcmp(argv[1],"nofork"))) {
		if (rfork = fork()) {
			if (rfork == -1) {
				sleep(30);
				goto begin; 
			}
			exit(0); 
		}
		didfork++;
		untty();	/* detach cron from console */
	}

	for (i = getdtablesize(); i >= 0; i--)
		(void) close(i);

	/*
	 * This must be done to make "popen" work; it doesn't properly
	 * "dup" the pipe to file descriptor 0 or 1 if the file
	 * descriptor is itself 0 or 1.
	 */
	(void) open("/dev/null", O_RDWR);
	(void) dup(0);
	(void) dup(1);

	umask(022);
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, cronend);

	openlog("cron", 0, LOG_CRON);
	initialize(1);
	quedefaults();	/* load default queue definitions */
	syslog(LOG_INFO,"*** cron started ***");
	siginterrupt(SIGALRM, 1);	/* alarm interrupt system calls */
	signal(SIGALRM, timeout);	/* set up alarm clock trap */
	t_old = time((long *) 0);
	last_time = t_old;
	while (TRUE) {			/* MAIN LOOP	*/
		t = time((long *) 0);
		if((t_old > t) || (t-last_time > CUSHION)) {
			/* the time was set backwards or forward */
 			if(t_old > t)
				syslog(LOG_DEBUG,
 					"Time set forward, t_old=%ld t=%ld",
 					t_old, t);
 			else
 				syslog(LOG_DEBUG,
 					"Time set backward, t=%ld last_time=%ld CUSHION=%ld",
 					t, last_time, (long) CUSHION);
			el_delete();
			u = uhead;
			while (u!=NULL) {
				rm_ctevents(u);
				e = u->atevents;
				while (e!=NULL) {
					free(e->cmd);
					e2 = e->link;
					free(e);
					e = e2; 
				}
				u2 = u->nextusr;
				u = u2; 
			}
			close(msgfd);
			initialize(0);
			t = time((long *) 0); 
		}
		t_old = t;
		if (next_event == NULL) {
			if (el_empty()) ne_time = INFINITY;
			else {	
				next_event = (struct event *) el_first();
				ne_time = next_event->time - t; 
#ifdef DEBUG
			syslog(LOG_INFO,"next_time=        %ld  %s, job=%s",
				next_event->time,ctime(&next_event->time),next_event->cmd);
#endif
			}
		} else {
			ne_time = next_event->time - t;
#ifdef DEBUG
			syslog(LOG_DEBUG,"next_time=%ld  %s",
				next_event->time,ctime(&next_event->time));
#endif
		}
		seconds = (ne_time < (long) 0) ? (long) 0 : ne_time;
		if(ne_time > (long) 0)
			idle(seconds);
		if(notexpired) {
			notexpired = 0;
			last_time = INFINITY;
			continue;
		}
		quedefload();
		last_time = next_event->time;	/* save execution time */
		/*
		 * Check if I have waited enough
		 */
		t_check = time ((long *)0);
		if (next_event->time - t_check > 0) {
			/*
			 * This is very unlikely to happen, but this happens for
			 * SPARCstation1.
			 */
			 int diff;
			 diff = next_event->time - t_check;
#ifdef DEBUG
		syslog(LOG_INFO,"	**** DID NOT WAIT LONG ENOUGH (%d)", diff);
#endif
			 sleep (diff);
		}
#ifdef DEBUG
		syslog(LOG_INFO,"GOING TO EX=      %ld", time ((long *)0));
#endif
		ex(next_event);
		switch(next_event->etype) {
		/* add cronevent back into the main event list */
		case CRONEVENT:
			if(delayed) {
				delayed = 0;
				break;
			}
#ifdef DEBUG
			syslog(LOG_INFO,"BEFORE -next_time=%ld  %s JOB(%s)",
				next_event->time,ctime(&next_event->time),next_event->cmd);
#endif
			next_event->time = next_time(next_event);
#ifdef DEBUG
			syslog(LOG_INFO,"AFTER  -next_time=%ld  %s JOB(%s)",
				next_event->time,ctime(&next_event->time),next_event->cmd);
#endif
			el_add( next_event,next_event->time,
			    (next_event->u)->ctid ); 
			break;
		/* remove at or batch job from system */
		default:
			eprev=NULL;
			e=(next_event->u)->atevents;
			while (e != NULL)
				if (e == next_event) {
					if (eprev == NULL)
						(e->u)->atevents = e->link;
					else	eprev->link = e->link;
					free(e->cmd);
					free(e);
					break;	
				}
				else {	
					eprev = e;
					e = e->link; 
				}
			break;
		}
		next_event = NULL; 
	}
}

untty()
{
	register int i;

	i = open("/dev/tty", O_RDWR);
	if (i >= 0) {
		ioctl(i, TIOCNOTTY, (char *)0);
		(void) close(i);
	}
}

initialize(firstpass)
{
	char *getenv();

#ifdef DEBUG
	syslog(LOG_DEBUG,"in initialize\n");
#endif
	init_time = time((long *) 0);
	el_init(8,init_time,(long)(60*60*24),10);
	if(firstpass)
		if(access(FIFO,04) == -1) {
			if(errno == ENOENT) {
				if(mknod(FIFO,S_IFIFO|0600,0)!=0)
					crabort("Can't create %s: %m",FIFO,1);
			}
			else  {
				if(NOFORK) {
					/* didn't fork... init(8) is waiting */
					sleep(60);
				}
				crabort("Can't access %s: %m",FIFO,1);
			}
		}
		else {
			if(NOFORK)
					/* didn't fork... init(8) is waiting */
				sleep(60); /* the wait is painful, but we don't
					   want init respawning this quickly */

				crabort("Can't start cron - another cron may be running (%s exists)",
				    FIFO,0);
		}
	if((msgfd = open(FIFO, O_RDWR)) < 0)
		crabort("Can't open %s: %m",FIFO,1);
	if(getenv("TZ") != NULL)
		sprintf(tzone,"TZ=%s",getenv("TZ"));
	else
		envinit[TZONE_INDEX] = NULL;	/* TZ not specified */

	/* read directories, create users list,
	   and add events to the main event list	*/
	uhead = NULL;
	read_dirs();
	next_event = NULL;
}


read_dirs()
{
	register DIR *dir;
	register struct direct *dp;

#ifdef DEBUG
	syslog(LOG_DEBUG,"Read Directories\n");
#endif
	if (chdir(CRONDIR,0) == -1)
		crabort(BADCD,CRONDIR,1);
	cwd = CRON;
	if ((dir=opendir("."))==NULL)
		crabort(NOREADDIR,CRONDIR,1);
	while((dp=readdir(dir)) != NULL)
		mod_ctab(dp->d_name);
	closedir(dir);
	if(chdir(ATDIR) == -1) {
		syslog(LOG_ERR,BADCD,ATDIR);
		return;
	}
	cwd = AT;
	if ((dir=opendir("."))==NULL) {
		syslog(LOG_ERR,NOREADDIR,ATDIR);
		return; 
	}
	while((dp=readdir(dir)) != NULL)
		mod_atjob(dp->d_name);
	closedir(dir);
}

mod_ctab(name)
char	*name;
{

	struct	passwd	*pw;
	struct	stat	buf;
	struct	usr	*u,*find_usr();
	char	namebuf[132];
	char	*pname;

	if((pw=getpwnam(name)) == NULL)
		return;
	if(cwd != CRON) {
		strcat(strcat(strcpy(namebuf,CRONDIR),"/"),name);
		pname = namebuf;
	} else
		pname = name;
	if(stat(pname,&buf)) {
		mail(name,BADSTAT,ERR_UNIXERR);
		unlink(pname);
		return;
	}
	if((u=find_usr(name)) == NULL) {
#ifdef DEBUG
		syslog(LOG_DEBUG,"new user (%s) with a crontab\n",name);
#endif
		u = (struct usr *) xmalloc(sizeof(struct usr));
		u->name = xmalloc(strlen(name)+1);
		strcpy(u->name,name);
		u->home = xmalloc(strlen(pw->pw_dir)+1);
		strcpy(u->home,pw->pw_dir);
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
		u->ctexists = TRUE;
		u->ctid = ecid++;
		u->ctevents = NULL;
		u->atevents = NULL;
#ifdef ATLIMIT
		u->aruncnt = 0;
#endif
#ifdef CRONLIMIT
		u->cruncnt = 0;
#endif
		u->nextusr = uhead;
		uhead = u;
		readcron(u);
	} else {
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
		if(strcmp(u->home,pw->pw_dir) != 0) {
			free(u->home);
			u->home = xmalloc(strlen(pw->pw_dir)+1);
			strcpy(u->home,pw->pw_dir);
		}
		u->ctexists = TRUE;
		if(u->ctid == 0) {
#ifdef DEBUG
			syslog(LOG_DEBUG,"%s now has a crontab\n",u->name);
#endif
			/* user didnt have a crontab last time */
			u->ctid = ecid++;
			u->ctevents = NULL;
			readcron(u);
			return;
		}
#ifdef DEBUG
		syslog(LOG_DEBUG,"%s has revised their crontab\n",u->name);
#endif
		rm_ctevents(u);
		el_remove(u->ctid,0);
		readcron(u);
	}
}


mod_atjob(name)
char	*name;
{

	char	*ptr;
	time_t	tim;
	struct	passwd	*pw;
	struct	stat	buf;
	struct	usr	*u,*find_usr();
	struct	event	*e;
	char	namebuf[132];
	char	*pname;
	short	jobtype;		/* at or batch job */

	ptr = name;
	if (jobinfo(ptr, &tim, &jobtype) == -1) {
#ifdef DEBUG
		syslog(LOG_DEBUG, "incorrect at job name %s", ptr);
#endif
		return;
	}
	if(cwd != AT) {
		strcat(strcat(strcpy(namebuf,ATDIR),"/"),name);
		pname = namebuf;
	} else
		pname = name;
	if(stat(pname,&buf) || jobtype >= NQUEUE) {
		unlink(pname);
		return;
	}
	if(!(buf.st_mode & ISUID)) {
		unlink(pname);
		return;
	}
	if((pw=getpwuid(buf.st_uid)) == NULL)
		return;
	if((u=find_usr(pw->pw_name)) == NULL) {
#ifdef DEBUG
		syslog(LOG_DEBUG,"new user (%s) with an at job = %s\n",
		    pw->pw_name,name);
#endif
		u = (struct usr *) xmalloc(sizeof(struct usr));
		u->name = xmalloc(strlen(pw->pw_name)+1);
		strcpy(u->name,pw->pw_name);
		u->home = xmalloc(strlen(pw->pw_dir)+1);
		strcpy(u->home,pw->pw_dir);
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
		u->ctexists = FALSE;
		u->ctid = 0;
		u->ctevents = NULL;
		u->atevents = NULL;
#ifdef ATLIMIT
		u->aruncnt = 0;
#endif
#ifdef CRONLIMIT
		u->cruncnt = 0;
#endif
		u->nextusr = uhead;
		uhead = u;
		add_atevent(u,name,tim,jobtype);
	} else {
		u->uid = pw->pw_uid;
		u->gid = pw->pw_gid;
		if(strcmp(u->home,pw->pw_dir) != 0) {
			free(u->home);
			u->home = xmalloc(strlen(pw->pw_dir)+1);
			strcpy(u->home,pw->pw_dir);
		}
		e = u->atevents;
		while(e != NULL)
			if(strcmp(e->cmd,name) == 0) {
				e->of.at.exists = TRUE;
				break;
			} else
				e = e->link;
		if (e == NULL) {
#ifdef DEBUG
			syslog(LOG_DEBUG,"%s has a new at job = %s\n",
			    u->name,name);
#endif
			add_atevent(u,name,tim,jobtype);
		}
	}
}



add_atevent(u,job,tim,jobtype)
struct usr *u;
char *job;
time_t tim;
short jobtype;
{
	struct event *e;

	e=(struct event *) xmalloc(sizeof(struct event));
	e->etype = jobtype;
	e->cmd = xmalloc(strlen(job)+1);
	strcpy(e->cmd,job);
	e->u = u;
#ifdef DEBUG
	syslog(LOG_DEBUG,"add_atevent: user=%s, job=%s, time=%ld\n",
		u->name,e->cmd, e->time);
#endif
	e->link = u->atevents;
	u->atevents = e;
	e->of.at.exists = TRUE;
	e->of.at.eventid = ecid++;
	if(tim < init_time)		/* old job */
		e->time = init_time;
	else
		e->time = tim;
	el_add(e, e->time, e->of.at.eventid); 
}


char line[CTLINESIZE];		/* holds a line from a crontab file	*/
int cursor;			/* cursor for the above line	*/

readcron(u)
struct usr *u;
{
	/* readcron reads in a crontab file for a user (u).
	   The list of events for user u is built, and 
	   u->events is made to point to this list.
	   Each event is also entered into the main event list. */

	FILE *cf;		/* cf will be a user's crontab file */
	time_t next_time();
	struct event *e;
	int start,i;
	char *next_field();
	char namebuf[132];
	char *pname;
	register char *cp;

	/* read the crontab file */
	if(cwd != CRON) {
		strcat(strcat(strcpy(namebuf,CRONDIR),"/"),u->name);
		pname = namebuf;
	} else
		pname = u->name;
	if ((cf=fopen(pname,"r")) == NULL) {
		mail(u->name,NOREAD,ERR_UNIXERR);
		return; 
	}
	while (fgets(line,CTLINESIZE,cf) != NULL) {
		/* process a line of a crontab file */
		cursor = 0;
		while(line[cursor] == ' ' || line[cursor] == '\t')
			cursor++;
		if(line[cursor] == '#')
			continue;
		e = (struct event *) xmalloc(sizeof(struct event));
		e->etype = CRONEVENT;
		if ((e->of.ct.minute=next_field(0,59,u)) == NULL) goto badline;
		if ((e->of.ct.hour=next_field(0,23,u)) == NULL) goto badline;
		if ((e->of.ct.daymon=next_field(1,31,u)) == NULL) goto badline;
		if ((e->of.ct.month=next_field(1,12,u)) == NULL) goto badline;
		if ((e->of.ct.dayweek=next_field(0,7,u)) == NULL) goto badline;
		/*
		 * For backwards compatibility, map all "7"s in the "day of
		 * week" field to "0".  Note that the largest possible value in
		 * this field is 7, so we don't have to worry about any
		 * multi-digit fields.
		 */
		for (cp = e->of.ct.dayweek; *cp != '\0'; cp++) {
			if (*cp == '7')
				*cp = '0';
		}
		if (line[++cursor] == '\0') {
			mail(u->name,EOLN,ERR_CRONTABENT);
			goto badline; 
		}
		/* get the command to execute	*/
		start = cursor;
again:
		while ((line[cursor]!='%')&&(line[cursor]!='\n')
		    &&(line[cursor]!='\0') && (line[cursor]!='\\')) cursor++;
		if(line[cursor] == '\\') {
			cursor += 2;
			goto again;
		}
		e->cmd = xmalloc(cursor-start+1);
		strncpy(e->cmd,line+start,cursor-start);
		e->cmd[cursor-start] = '\0';
		/* see if there is any standard input	*/
		if (line[cursor] == '%') {
			e->of.ct.input = xmalloc(strlen(line)-cursor+1);
			strcpy(e->of.ct.input,line+cursor+1);
			for (i=0; i<strlen(e->of.ct.input); i++)
				if (e->of.ct.input[i] == '%') e->of.ct.input[i] = '\n'; 
		}
		else e->of.ct.input = NULL;
		/* have the event point to it's owner	*/
		e->u = u;
		/* insert this event at the front of this user's event list   */
		e->link = u->ctevents;
		u->ctevents = e;
		/* set the time for the first occurance of this event	*/
		e->time = next_time(e);
		/* finally, add this event to the main event list	*/
		el_add(e,e->time,u->ctid);
#ifdef DEBUG
		syslog(LOG_DEBUG,"inserting cron event %s at %ld (%s)",
			e->cmd,e->time,ctime(&e->time));
#endif
		continue;

badline: 
		free(e); 
	}

	fclose(cf);
}


mail(usrname,msg,errtype)
char *usrname,*msg;
int errtype;
{
	/* mail mails a user a message.	*/

	FILE *pipe;
	char *temp,*i;
	int saveerrno = errno;

#ifdef TESTING
	return;
#endif
	temp = xmalloc(strlen(MAIL)+strlen(usrname)+2);
	pipe = popen(strcat(strcat(strcpy(temp,MAIL)," "),usrname),"w");
	if (pipe!=NULL) {
		fprintf(pipe, "To: %s\n", usrname);
		switch (errtype) {

		case ERR_CRONTABENT:
			fprintf(pipe,"Subject: Your crontab file has an error in it\n\n");
			i = strrchr(line,'\n');
			if (i != NULL) *i = ' ';
			fprintf(pipe, "\t%s\n\t%s\n",line,msg);
			fprintf(pipe, "This entry has been ignored.\n"); 
			break;

		case ERR_UNIXERR:
			fprintf(pipe, "Subject: %s\n\n", msg);
			fprintf(pipe, "The error was \"%s\"\n", errmsg(saveerrno));
			break;

		case ERR_CANTEXECCRON:
			fprintf(pipe, "Subject: Couldn't run your \"cron\" job\n\n");
			fprintf(pipe, "%s\n", msg);
			fprintf(pipe, "The error was \"%s\"\n", errmsg(saveerrno));
			break;

		case ERR_CANTEXECAT:
			fprintf(pipe, "Subject: Couldn't run your \"at\" job\n\n");
			fprintf(pipe, "%s\n", msg);
			fprintf(pipe, "The error was \"%s\"\n", errmsg(saveerrno));
			break;
		}
		/* dont want to do pclose because pclose does a wait */
		fclose(pipe); 
		/* decremented in idle() */
		running++;
	}
	free(temp);
}


char *next_field(lower,upper,u)
int lower,upper;
struct usr *u;
{
	/* next_field returns a pointer to a string which holds 
	   the next field of a line of a crontab file.
	   if (numbers in this field are out of range (lower..upper),
	       or there is a syntax error) then
			NULL is returned, and a mail message is sent to
			the user telling him which line the error was in.     */

	char *s;
	int num,num2,start;

	while ((line[cursor]==' ') || (line[cursor]=='\t')) cursor++;
	start = cursor;
	if (line[cursor] == '\0') {
		mail(u->name,EOLN,ERR_CRONTABENT);
		return(NULL); 
	}
	if (line[cursor] == '*') {
		cursor++;
		if ((line[cursor]!=' ') && (line[cursor]!='\t')) {
			mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
		s = xmalloc(2);
		strcpy(s,"*");
		return(s); 
	}
	while (TRUE) {
		if (!isdigit(line[cursor])) {
			mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
		num = 0;
		do { 
			num = num*10 + (line[cursor]-'0'); 
		}			while (isdigit(line[++cursor]));
		if ((num<lower) || (num>upper)) {
			mail(u->name,OUTOFBOUND,ERR_CRONTABENT);
			return(NULL); 
		}
		if (line[cursor]=='-') {
			if (!isdigit(line[++cursor])) {
				mail(u->name,UNEXPECT,ERR_CRONTABENT);
				return(NULL); 
			}
			num2 = 0;
			do { 
				num2 = num2*10 + (line[cursor]-'0'); 
			}				while (isdigit(line[++cursor]));
			if ((num2<lower) || (num2>upper)) {
				mail(u->name,OUTOFBOUND,ERR_CRONTABENT);
				return(NULL); 
			}
		}
		if ((line[cursor]==' ') || (line[cursor]=='\t')) break;
		if (line[cursor]=='\0') {
			mail(u->name,EOLN,ERR_CRONTABENT);
			return(NULL); 
		}
		if (line[cursor++]!=',') {
			mail(u->name,UNEXPECT,ERR_CRONTABENT);
			return(NULL); 
		}
	}
	s = xmalloc(cursor-start+1);
	strncpy(s,line+start,cursor-start);
	s[cursor-start] = '\0';
	return(s);
}


time_t next_time(e)
struct event *e;
{
	/* returns the integer time for the next occurance of event e.
	   the following fields have ranges as indicated:
	PRGM  | min	hour	day of month	mon	day of week
	------|-------------------------------------------------------
	cron  | 0-59	0-23	    1-31	1-12	0-6 (0=sunday)
	time  | 0-59	0-23	    1-31	0-11	0-6 (0=sunday)
	   NOTE: this routine is hard to understand. */

	struct tm *tm, newtm;
	int tm_mon,tm_mday,tm_wday,wday,m,min,h,hr,carry,day,days,
	d1,day1,carry1,d2,day2,carry2,daysahead,mon,yr,db,wd,today;
	time_t t;
	static int firstpass = 1, dst;

	t = time((long *) 0);
	tm = localtime(&t);

	tm_mon = next_ge(tm->tm_mon+1,e->of.ct.month) - 1;	/* 0-11 */
	tm_mday = next_ge(tm->tm_mday,e->of.ct.daymon);		/* 1-31 */
	tm_wday = next_ge(tm->tm_wday,e->of.ct.dayweek);	/* 0-6  */
	today = TRUE;
	if ( (strcmp(e->of.ct.daymon,"*")==0 && tm->tm_wday!=tm_wday)
	    || (strcmp(e->of.ct.dayweek,"*")==0 && tm->tm_mday!=tm_mday)
	    || (tm->tm_mday!=tm_mday && tm->tm_wday!=tm_wday)
	    || (tm->tm_mon!=tm_mon)) today = FALSE;

	m = tm->tm_min+1;
try_again:
	min = next_ge(m%60,e->of.ct.minute);
	carry = (min < m) ? 1:0;
	h = tm->tm_hour+carry;
	hr = next_ge(h%24,e->of.ct.hour);
	carry = (hr < h) ? 1:0;
	if ((!carry) && today) {
		/* this event must occur today	*/
		newtm = *tm;
		newtm.tm_hour = hr;
		newtm.tm_min = min;
		newtm.tm_sec = 0;
 		t = timelocal(&newtm);
 		if(t == (time_t) -1) {
 			long gmtoff1;
 
 			/* Compute daylight savings offset:                */
 			/*   the difference between the gmt offset X hours */
 			/*   ago and the gmt offset X hours from now.      */
 			newtm.tm_hour -= DAYLIGHT_OFFSET;
 			t = timelocal(&newtm);
 			tm = localtime(&t);
 			gmtoff1 = tm->tm_gmtoff;
 			newtm.tm_hour += 2 * DAYLIGHT_OFFSET;
 			t = timelocal(&newtm);
 			tm = localtime(&t);
 
 			t += (tm->tm_gmtoff - gmtoff1) - (DAYLIGHT_OFFSET*60*60);
 		}
 
 		return(t);
	}

	min = next_ge(0,e->of.ct.minute);
	hr = next_ge(0,e->of.ct.hour);

	/* calculate the date of the next occurance of this event,
	   which will be on a different day than the current day.	*/

	/* check monthly day specification	*/
	d1 = tm->tm_mday+1;
	day1 = next_ge((d1-1)%days_in_mon(tm->tm_mon,tm->tm_year)+1,e->of.ct.daymon);
	carry1 = (day1 < d1) ? 1:0;

	/* check weekly day specification	*/
	d2 = tm->tm_wday+1;
	wday = next_ge(d2%7,e->of.ct.dayweek);
	if (wday < d2) daysahead = 7 - d2 + wday;
	else daysahead = wday - d2;
	day2 = (d1+daysahead-1)%days_in_mon(tm->tm_mon,tm->tm_year)+1;
	carry2 = (day2 < d1) ? 1:0;

	/* based on their respective specifications,
	   day1, and day2 give the day of the month
	   for the next occurance of this event.	*/

	if ((strcmp(e->of.ct.daymon,"*")==0) && (strcmp(e->of.ct.dayweek,"*")!=0)) {
		day1 = day2;
		carry1 = carry2; 
	}
	if ((strcmp(e->of.ct.daymon,"*")!=0) && (strcmp(e->of.ct.dayweek,"*")==0)) {
		day2 = day1;
		carry2 = carry1; 
	}

	yr = tm->tm_year;
	if ((carry1 && carry2) || (tm->tm_mon != tm_mon)) {
		/* event does not occur in this month	*/
		m = tm->tm_mon+1;
		mon = next_ge(m%12+1,e->of.ct.month)-1;		/* 0..11 */
		carry = (mon < m) ? 1:0;
		yr += carry;
		/* recompute day1 and day2	*/
		day1 = next_ge(1,e->of.ct.daymon);
		db = days_btwn(tm->tm_mon,tm->tm_mday,tm->tm_year,mon,1,yr) + 1;
		wd = (tm->tm_wday+db)%7;
		/* wd is the day of the week of the first of month mon	*/
		wday = next_ge(wd,e->of.ct.dayweek);
		if (wday < wd) day2 = 1 + 7 - wd + wday;
		else day2 = 1 + wday - wd;
		if ((strcmp(e->of.ct.daymon,"*")!=0) && (strcmp(e->of.ct.dayweek,"*")==0))
			day2 = day1;
		if ((strcmp(e->of.ct.daymon,"*")==0) && (strcmp(e->of.ct.dayweek,"*")!=0))
			day1 = day2;
		day = (day1 < day2) ? day1:day2; 
	}
	else { /* event occurs in this month	*/
		mon = tm->tm_mon;
		if (!carry1 && !carry2) day = (day1 < day2) ? day1 : day2;
		else if (!carry1) day = day1;
		else day = day2;
	}

	/* now that we have the min,hr,day,mon,yr of the next
	   event, figure out what time that turns out to be.	*/

	newtm = *tm;
	newtm.tm_year = yr;
	newtm.tm_mon = mon;
	newtm.tm_mday = day;
	newtm.tm_hour = hr;
	newtm.tm_min = min;
	newtm.tm_sec = 0;
 	t = timelocal(&newtm);
 	if(t == (time_t) -1) {
 		long gmtoff1;
 
 		/* Compute daylight savings offset:                */
 		/*   the difference between the gmt offset X hours */
 		/*   ago and the gmt offset X hours from now.      */
 		newtm.tm_hour -= DAYLIGHT_OFFSET;
 		t = timelocal(&newtm);
 		tm = localtime(&t);
 		gmtoff1 = tm->tm_gmtoff;
 		newtm.tm_hour += 2  *DAYLIGHT_OFFSET;
 		t = timelocal(&newtm);
 		tm = localtime(&t);
 
 		t += (tm->tm_gmtoff - gmtoff1) - (DAYLIGHT_OFFSET*60*60);
 	}
 
 	return(t);
}



#define	DUMMY	100
next_ge(current,list)
int current;
char *list;
{
	/* list is a character field as in a crontab file;
	   	for example: "40,20,50-10"
	   next_ge returns the next number in the list that is
	   greater than or equal to current.
	   if no numbers of list are >= current, the smallest
	   element of list is returned.
	   NOTE: current must be in the appropriate range.	*/

	char *ptr;
	int n,n2,min,min_gt;

	if (strcmp(list,"*") == 0) return(current);
	ptr = list;
	min = DUMMY; 
	min_gt = DUMMY;
	while (TRUE) {
		if ((n=(int)num(&ptr))==current) return(current);
		if (n<min) min=n;
		if ((n>current)&&(n<min_gt)) min_gt=n;
		if (*ptr=='-') {
			ptr++;
			if ((n2=(int)num(&ptr))>n) {
				if ((current>n)&&(current<=n2))
					return(current); 
			}
			else {	/* range that wraps around */
				if (current>n) return(current);
				if (current<=n2) return(current); 
			}
		}
		if (*ptr=='\0') break;
		ptr += 1; 
	}
	if (min_gt!=DUMMY) return(min_gt);
	else return(min);
}

del_atjob(name,usrname)
char	*name;
char	*usrname;
{

	struct	event	*e, *eprev;
	struct	usr	*u;

	if((u = find_usr(usrname)) == NULL)
		return;
	e = u->atevents;
	eprev = NULL;
	while(e != NULL)
		if(strcmp(name,e->cmd) == 0) {
			if(next_event == e)
				next_event = NULL;
			if(eprev == NULL)
				u->atevents = e->link;
			else
				eprev->link = e->link;
			el_remove(e->of.at.eventid, 1);
			free(e->cmd);
			free(e);
			break;
		} else {
			eprev = e;
			e = e->link;
		}
}

del_ctab(name)
char	*name;
{

	struct	usr	*u;

	if((u = find_usr(name)) == NULL)
		return;
	rm_ctevents(u);
	el_remove(u->ctid, 0);
	u->ctid = 0;
	u->ctexists = 0;
}


rm_ctevents(u)
struct usr *u;
{
	struct event *e2,*e3;

	/* see if the next event (to be run by cron)
	   is a cronevent owned by this user.		*/
	if ( (next_event!=NULL) && 
	    (next_event->etype==CRONEVENT) &&
	    (next_event->u==u) )
		next_event = NULL;
	e2 = u->ctevents;
	while (e2 != NULL) {
		free(e2->cmd);
		free(e2->of.ct.minute);
		free(e2->of.ct.hour);
		free(e2->of.ct.daymon);
		free(e2->of.ct.month);
		free(e2->of.ct.dayweek);
		if (e2->of.ct.input != NULL) free(e2->of.ct.input);
		e3 = e2->link;
		free(e2);
		e2 = e3; 
	}
	u->ctevents = NULL;
}


struct usr *find_usr(uname)
char *uname;
{
	struct usr *u;

	u = uhead;
	ulast = NULL;
	while (u != NULL) {
		if (strcmp(u->name,uname) == 0) return(u);
		ulast = u;
		u = u->nextusr; 
	}
	return(NULL);
}

/*
 * The data to send in an audit message
 */
char *audit_argv[] = { "cron", SHELL, "standard input" };

ex(e)
struct event *e;
{

	register i,j;
	short sp_flag;
	register FILE *atcmdfile;
	char mailvar[4];	/* send mail variable ("yes" or "no") */
	int fd,rfork;
	char *cron_infile;
	char *mktemp();
	char *tempnam();
	struct stat buf;
	struct queue *qp;
	struct runinfo *rp;
	static int ndescriptors = 0;
	char atnamebuf[10+1];

	qp = &qt[e->etype];	/* set pointer to queue defs */
	if(qp->nrun >= qp->njob) {
		syslog(LOG_INFO,"%c queue max run limit reached",e->etype+'a');
		resched(qp->nwait);
		return;
	}
	for(rp=rt; rp < rt+MAXRUN; rp++) {
		if(rp->pid == 0)
			break;
	}
	if(rp >= rt+MAXRUN) {
		syslog(LOG_INFO,"MAXRUN (%d) procs reached",MAXRUN);
		resched(qp->nwait);
		return;
	}
#ifdef ATLIMIT
	if((e->u)->uid != 0 && (e->u)->aruncnt >= ATLIMIT) {
		syslog(LOG_INFO,"ATLIMIT (%d) reached for uid %d",ATLIMIT,
		    (e->u)->uid);
		resched(qp->nwait);
		return;
	}
#endif
#ifdef CRONLIMIT
	if((e->u)->uid != 0 && (e->u)->cruncnt >= CRONLIMIT) {
		syslog(LOG_INFO,"CRONLIMIT (%d) reached for uid %d",CRONLIMIT,
		    (e->u)->uid);
		resched(qp->nwait);
		return;
	}
#endif

	rp->outfile = tempnam(TMPDIR,PFX);
	rp->jobtype = e->etype;
	if(e->etype == CRONEVENT) {
		if ((rp->jobname = malloc(strlen(e->cmd)+1)) != NULL)
			(void) strcpy(rp->jobname, e->cmd);
		rp->mailwhendone = 0;	/* "cron" jobs only produce mail if there's output */
	} else {
		if ((atcmdfile = fopen(e->cmd,"r")) == NULL) {
			mail((e->u)->name,BADJOBOPEN,ERR_CANTEXECAT);
			unlink(e->cmd);
			return;
		}
		if (fstat(fileno(atcmdfile), &buf) >= 0) {
			(void) sprintf(atnamebuf, "%lu", buf.st_ino);
			if ((rp->jobname = malloc(strlen(atnamebuf)+1)) != NULL)
				(void) strcpy(rp->jobname, atnamebuf);
		}
		/*
		 * Skip over the first, second, third, and fourth lines.
		 */
		fscanf(atcmdfile,"%*[^\n]\n");
		fscanf(atcmdfile,"%*[^\n]\n");
		fscanf(atcmdfile,"%*[^\n]\n");
		fscanf(atcmdfile,"%*[^\n]\n");
		if (fscanf(atcmdfile,"# notify by mail: %3s%*[^\n]\n",mailvar) == 1) {
			/*
			 * Check to see if we should always send mail
			 * to the owner.
			 */
			rp->mailwhendone = (strcmp(mailvar, "yes") == 0);
		} else
			rp->mailwhendone = 0;
	}
	if((rfork = fork()) == -1) {
		syslog(LOG_NOTICE,"Can't fork");
		resched(wait_time);
		sleep(30);
		return;
	}
	if(rfork) {	/* parent process */
		if (e->etype != CRONEVENT)
			fclose(atcmdfile);
		++qp->nrun;
		++running;
		rp->pid = rfork;
		rp->que = e->etype;
#ifdef ATLIMIT
		if(e->etype != CRONEVENT)
			(e->u)->aruncnt++;
#endif
#if ATLIMIT && CRONLIMIT
		else
			(e->u)->cruncnt++;
#else
#ifdef CRONLIMIT
		if(e->etype == CRONEVENT)
			(e->u)->cruncnt++;
#endif
#endif
		rp->rusr = (e->u);
		logit((char)BCHAR,rp,0);
		return;
	}

	closelog();

	if (e->etype != CRONEVENT ) {
		if (!(buf.st_mode&ISUID)) {
			/* if setuid bit off, original owner has 
			   given this file to someone else	*/
			unlink(e->cmd);
			exit(1); 
		}
		/* make jobfile the stdin to shell */
		if (fileno(atcmdfile) != 0) {
			close(0);
			dup(fileno(atcmdfile));
			fclose(atcmdfile);
		}
		(void) lseek(0, 0L, 0);	/* position back to beginning */
		unlink(e->cmd);
		fd = 1;		/* start closing at 1 */
	} else
		fd = 0;		/* start closing at 0 */

	if (ndescriptors == 0)
		ndescriptors = getdtablesize();
	for (i=fd; i<ndescriptors; i++)
		close(i);

	/* set correct user and group identification	*/
	/*
	 * Put the command name into the audit record if it's available
	 */
	if (e->etype == CRONEVENT)
		audit_argv[2] = e->cmd;
	/*
	 * Set the audit state to reflect this user's flags
	 */
	setauditinfo((e->u)->uid, (e->u)->name);
	audit_args(AU_LOGIN, 3, audit_argv);

	if ((setgid((e->u)->gid)<0)
	    || (initgroups((e->u)->name, (e->u)->gid)<0)
	    || (setuid((e->u)->uid)<0))
		exit(1);
	sp_flag = FALSE;
	if (e->etype == CRONEVENT) {
		/* check for standard input to command	*/
		if (e->of.ct.input != NULL) {
			cron_infile = mktemp(TMPINFILE);
			if ((fd=creat(cron_infile,INMODE)) == -1) {
				mail((e->u)->name,NOSTDIN,ERR_CANTEXECCRON);
				exit(1); 
			}
			if (write(fd,e->of.ct.input,strlen(e->of.ct.input))
			    != strlen(e->of.ct.input)) {
				mail((e->u)->name,NOSTDIN,ERR_CANTEXECCRON);
				unlink(cron_infile);
				exit(1); 
			}
			close(fd);
			/* open tmp file as stdin input to sh	*/
			if ((fd = open(cron_infile,O_RDONLY)) == -1) {
				mail((e->u)->name,NOSTDIN,ERR_CANTEXECCRON);
				unlink(cron_infile);
				exit(1); 
			}
			unlink(cron_infile); 
		}
		else if ((fd = open("/dev/null",O_RDONLY)) == -1) {
			fd = open("/",O_RDONLY);
			sp_flag = TRUE; 
		}
		if (fd != 0 && fd != -1) {
			close(0);
			dup(fd);
			close(fd);
		}
	}

	/* redirect stdout and stderr for the shell	*/
	if ((fd = creat(rp->outfile,OUTMODE)) == -1)
		fd = open("/dev/null",O_WRONLY);
	if (fd != 1 && fd != -1) {
		close(1);
		dup(fd);
		close(fd);
	}
	close(2);
	dup(1);
	if (sp_flag) close(0);
	strcat(homedir,(e->u)->home);
	strcat(logname,(e->u)->name);
	strcat(user,(e->u)->name);
	environ = envinit;
	if (chdir((e->u)->home)==-1) {
		mail((e->u)->name,CANTCDHOME,
		    e->etype == CRONEVENT ? ERR_CANTEXECCRON : ERR_CANTEXECAT);
		exit(1); 
	}
#ifdef TESTING
	exit(1);
#endif
	if((e->u)->uid != 0)
		nice(qp->nice);
	setpgrp(0, getpid());		/* protect against child's signals */
	if (e->etype == CRONEVENT)
		execl(SHELL,"sh","-c",e->cmd,(char *)NULL);
	else /* type == ATEVENT */
		execl(SHELL,"sh",(char *)NULL);
	mail((e->u)->name,CANTEXECSH,
	    e->etype == CRONEVENT ? ERR_CANTEXECCRON : ERR_CANTEXECAT);
	exit(1);
}


idle(tyme)
long tyme;
{

	long t;
	time_t	now;
	int	pid;
	int	prc;
	long	alm;
	struct	runinfo	*rp;

	t = tyme;
	while(t > 0L) {
		if(running) {
			if(t > wait_time)
				alm = wait_time;
			else
				alm = t;
#ifdef DEBUG
			syslog(LOG_DEBUG,"in idle - setting alarm for %ld sec\n",alm);
#endif
			alarm((unsigned) alm);
			pid = wait(&prc);
			alarm(0);
#ifdef DEBUG
			syslog(LOG_DEBUG,"wait returned %x\n",prc);
#endif
			if(pid == -1) {
				if(msg_wait())
					return;
			} else {
				for(rp=rt;rp < rt+MAXRUN; rp++)
					if(rp->pid == pid)
						break;
				if(rp >= rt+MAXRUN) {
					syslog(LOG_NOTICE,"unexpected pid returned %d (ignored)",pid);
					/* incremented in mail() */
					running--;
				}
				else
					if(rp->que == ZOMB) {
						running--;
						rp->pid = 0;
						unlink(rp->outfile);
						free(rp->outfile);
					}
					else
						cleanup(rp,prc);
			}
		} else {
			msg_wait();
			return;
		}
		now = time((char *) 0);
		t = next_event->time - now;
	}
}


cleanup(pr,rc)
struct	runinfo	*pr;
{

	int	fd;
	char	line[256+UNAMESIZE];
	struct	usr	*p;
	struct	stat	buf;
	register FILE *mailpipe;
	register FILE *st;
	register int nbytes;
	char	iobuf[BUFSIZ];

	logit((char)ECHAR,pr,rc);
	--qt[pr->que].nrun;
	pr->pid = 0;
	--running;
	p = pr->rusr;
#ifdef ATLIMIT
	if(pr->que != CRONEVENT)
		--p->aruncnt;
#endif
#if ATLIMIT && CRONLIMIT
	else
		--p->cruncnt;
#else
#ifdef CRONLIMIT
	if(pr->que == CRONEVENT)
		--p->cruncnt;
#endif
#endif
	if(!stat(pr->outfile,&buf)) {
		if(buf.st_size > 0 || pr->mailwhendone) {
			/* mail user stdout and stderr */
			if((pr->pid = fork()) == 0) {
				(void) strcpy(line, MAIL);
				(void) strcat(line, " ");
				(void) strcat(line, p->name);
				mailpipe = popen(line, "w");
				if (mailpipe == NULL)
					exit(127);
				(void) fprintf(mailpipe, "To: %s\n", p->name);
				if (pr->jobtype == CRONEVENT) {
					(void) fprintf(mailpipe,
					    "Subject: Output from \"cron\" command\n\n");
					if (pr->jobname != NULL) {
						(void) fprintf(mailpipe,
						    "Your \"cron\" job\n\n");
						(void) fprintf(mailpipe,
						    "%s\n\n", pr->jobname);
					} else
						(void) fprintf(mailpipe,
					    "Your \"cron\" job ");
				} else {
					(void) fprintf(mailpipe,
					    "Subject: Output from \"at\" job\n\n");
					(void) fprintf(mailpipe,
					    "Your \"at\" job");
					if (pr->jobname != NULL)
						(void) fprintf(mailpipe,
						    " \"%s\"", pr->jobname);
					(void) fprintf(mailpipe, " ");
				}
				if (buf.st_size > 0
				    && (st = fopen(pr->outfile, "r")) != NULL) {
					(void) fprintf(mailpipe, "produced the following output:\n\n");
					while ((nbytes = fread(iobuf,
					    sizeof (char), BUFSIZ, st)) != 0)
						(void) fwrite(iobuf,
						    sizeof (char), nbytes,
						    mailpipe);
					(void) fclose(st);
				} else
					(void) fprintf(mailpipe,
					    "completed.\n");
				(void) pclose(mailpipe);
				exit(0);
			}
			pr->que = ZOMB;
			running++;
		} else {
			unlink(pr->outfile);
			free(pr->outfile);
		}
	}
}

#define	MSGSIZE	sizeof(struct message)

msg_wait()
{

	long	t;
	time_t	now;
	struct	stat msgstat;
	struct	message	*pmsg;
	int	cnt;

	if(fstat(msgfd,&msgstat) != 0)
		crabort("Can't stat %s: %m",FIFO,1);
	if(msgstat.st_size == 0 && running)
		return(0);
	if(next_event == NULL)
		t = INFINITY;
	else {
		now = time((long *) 0);
		t = next_event->time - now;
		if(t <= 0L)
			t = 1L;
	}
#ifdef DEBUG
	syslog(LOG_DEBUG,"in msg_wait - setting alarm for %ld sec\n", t);
#endif
	alarm((unsigned) t);
	pmsg = &msgbuf;
	errno = 0;
	if((cnt=read(msgfd,pmsg,MSGSIZE)) != MSGSIZE) {
		if(cnt < 0) {
			if(errno != EINTR) {
				syslog(LOG_ERR,"error reading message: %m");
				notexpired = 1;
			}
		} else {
			syslog(LOG_ERR,"error reading message: message too short");
			notexpired = 1;
		}
		if(next_event == NULL)
			notexpired = 1;
		return(1);
	}
	alarm(0);
	if(pmsg->etype != NULL) {
		switch(pmsg->etype) {
		case AT:
			if(pmsg->action == DELETE)
				del_atjob(pmsg->fname,pmsg->logname);
			else
				mod_atjob(pmsg->fname);
			break;
		case CRON:
			if(pmsg->action == DELETE)
				del_ctab(pmsg->fname);
			else
				mod_ctab(pmsg->fname);
			break;
		default:
			syslog(LOG_ERR,"message received - bad format");
			break;
		}
		if (next_event != NULL) {
			if (next_event->etype == CRONEVENT)
				el_add(next_event,next_event->time,(next_event->u)->ctid);
			else /* etype == ATEVENT */
				el_add(next_event,next_event->time,next_event->of.at.eventid);
			next_event = NULL;
		}
		pmsg->etype = NULL;
		notexpired = 1;
		return(1);
	}
}


timeout()
{
}

void
cronend()
{
	crabort("SIGTERM received",(char *)NULL,1);
}

crabort(mssg,arg,clean)
char *mssg;
char *arg;
{
	/* crabort handles exits out of cron */
	int c;

	if(clean) {
		close(msgfd);
		if(unlink(FIFO) < 0)	/* FIFO vanishes when cron finishes */
			syslog(LOG_ERR,"cron could not unlink %s: %m", FIFO);
	}
	syslog(LOG_ERR,mssg,arg);
	syslog(LOG_ERR,"******* CRON ABORTED ********");
	exit(1);
}

logit(cc,rp,rc)
char	cc;
struct	runinfo	*rp;
{
	time_t t;
	int    signum, rcode;
	char   *ctime();
	char   statusbuf[64+1];
	char   jobbuf[32];

	t = time((long *) 0);

	if (rp->jobtype != CRONEVENT) {
		/* print job number for at jobs */
		(void) sprintf(jobbuf, " %s", rp->jobname);
	} else {
		(void) strcpy(jobbuf, "");
	}

	if(cc == BCHAR)
		syslog(LOG_INFO,"%c  CMD: %u %c %s%s",
			cc, rp->pid, QUE(rp->que), next_event->cmd, jobbuf);
	if(rc != 0) {
		if((rcode = RCODE(rc)) != 0)
			(void) sprintf(statusbuf, " Exit status %d", rcode);
		else {
			extern char *sys_siglist[];

			signum = TSIGNAL(rc);
			if(signum > NSIG)
				(void) sprintf(statusbuf, " Signal %d",
				    signum);
			else
				(void) sprintf(statusbuf, " %s",
				    sys_siglist[signum]);
			if(COREDUMPED(rc))
				(void) strcat(statusbuf, " - core dumped");
		}
	} else
		(void) strcpy(statusbuf, "");
	syslog(LOG_INFO, "%c  %.8s %u %c %.24s%s%s",
	    cc, (rp->rusr)->name, rp->pid, QUE(rp->que), ctime(&t), 
	    jobbuf, statusbuf);
}

resched(delay)
int	delay;
{
	time_t	nt;
	char 	*ptr;
	short	jobtype;

	/* run job at a later time */
	nt = next_event->time + delay;
	if(next_event->etype == CRONEVENT) {
		next_event->time = next_time(next_event);
		if(nt < next_event->time)
			next_event->time = nt;
		el_add(next_event,next_event->time,(next_event->u)->ctid);
		delayed = 1;
		syslog(LOG_INFO,"rescheduling a cron job");
		return;
	}
	ptr = next_event->cmd;
	if (jobinfo(ptr, (time_t *) 0, &jobtype) == -1) {
		syslog(LOG_ERR, "invalid at job name %s, can't reschedule", ptr);
		return;
	}
	add_atevent(next_event->u, next_event->cmd, nt, jobtype);
	syslog(LOG_INFO, "rescheduling at job %s", next_event->cmd);
}

jobinfo(job, timep, jobtypep)
char *job;
time_t *timep;
short *jobtypep;
{
	time_t time;
	short  jobtype;

	if (((time = num(&job)) == 0) || (*job != '.'))
		return(-1);
	job++;
	if (!islower(*job))
		return(-1);

	jobtype = *job - 'a';

	if (timep != NULL)
		*timep = time;
	if (jobtypep != NULL)
		*jobtypep = jobtype;

	return(0);
}

#define	QBUFSIZ		80

quedefaults()
{
	register int i;

	/* set up default queue definitions */
	for(i=0;i<NQUEUE;i++) {
		qt[i].njob = qd.njob;
		qt[i].nice = qd.nice;
		qt[i].nwait = qd.nwait;
	}
}

quedefload()
{
	struct stat buf;
	static time_t lastmtime = 0L;
	static dev_t lastdev = 0;
	static ino_t lastinum = 0;
	register i;
	int	j;
	char	pathbuf[MAXPATHLEN];
	char	name[MAXNAMLEN+1];
	char	qbuf[QBUFSIZ];
	FILE	*fd;

	if((fd = fopen_configfile(QUEDEFS,"r",pathbuf)) == NULL) {
		syslog(LOG_ERR,"Can't open queuedefs file %s: %m",
		    pathbuf);
		syslog(LOG_ERR,"Using default queue definitions");
		return;
	}
	if(fstat(fileno(fd),&buf)) {
		syslog(LOG_WARNING,"Can't stat queuedefs file %s: %m",
		    pathbuf);
		buf.st_mtime = lastmtime + 1;	/* force a reread */
	}
	/*
	 * If the file was changed, or if it's a different file
	 * from last time, reread it.
	 */
	if(lastmtime != buf.st_mtime || lastdev != buf.st_dev
	    || lastinum != buf.st_ino) {
#ifdef DEBUG
		syslog(LOG_DEBUG, "reading %s", QUEDEFS);
#endif

		/* set up default queue definitions */
		quedefaults();
		while(fgets(qbuf, QBUFSIZ, fd) != NULL) {
			if((j=qbuf[0]-'a') < 0 || j >= NQUEUE
			    || qbuf[1] != '.')
				continue;
			(void) strcpy(name, qbuf);
			parsqdef(&name[2]);
			qt[j].njob = qq.njob;
			qt[j].nice = qq.nice;
			qt[j].nwait = qq.nwait;
#ifdef DEBUG
			syslog(LOG_DEBUG, 
				"changing queue %d, njob %d nice %d nwait %d",
				j, qt[j].njob, qt[j].nice, qt[j].nwait);
#endif
		}
		if (ferror(fd)) {
			syslog(LOG_ERR,"I/O error reading queuedefs file %s: %m",
			    pathbuf);
			syslog(LOG_ERR,"Using default queue definitions");
			quedefaults();
		} else {
			/*
			 * Read succeeded.
			 */
			lastmtime = buf.st_mtime;
			lastdev = buf.st_dev;
			lastinum = buf.st_ino;
		}
	}
	fclose(fd);
}

parsqdef(name)
char *name;
{
	register i;

	qq = qd;
	while(*name) {
		i = 0;
		while(isdigit(*name)) {
			i *= 10;
			i += *name++ - '0';
		}
		switch(*name++) {
		case JOBF:
			qq.njob = i;
			break;
		case NICEF:
			qq.nice = i;
			break;
		case WAITF:
			qq.nwait = i;
			break;
		}
	}
}

/*
 * setauditinfo
 *
 * For this process set the audit information appropriate for this user.
 *
 */
setauditinfo(uid, user_name)
	int uid;
	char *user_name;
{
	struct passwd_adjunct *pwda;
	struct passwd_adjunct *getpwanam();
	audit_state_t audit_state;

	if (setauid(uid) < 0)
		exit(1);

	setpwaent();
	if ((pwda = getpwanam(user_name)) != NULL) {
		if (getfauditflags(&pwda->pwa_au_always,
			&pwda->pwa_au_never, &audit_state) != 0) {
			/*
			* if we can't tell how to audit from the flags, audit
			* everything that's not never for this user.
			*/
			audit_state.as_success =
			    pwda->pwa_au_never.as_success ^ (-1);
			audit_state.as_failure =
			    pwda->pwa_au_never.as_failure ^ (-1);
		}
	}
	else {
		/*
		* if there is no adjunct file entry then either
		* this isn't a secure system in which case it doesn't
		* matter what st gets set to, or we've got a mismatch
		* in which case we'd best audit the dickens out of our
		* mystery user.
		*/
		audit_state.as_success = -1;
		audit_state.as_failure = -1;
	}
	endpwaent();
	setaudit(&audit_state);
}
