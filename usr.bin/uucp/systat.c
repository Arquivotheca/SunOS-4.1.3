/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)systat.c 1.1 92/07/30"	/* from SVR3.2 uucp:systat.c 2.4 */

#include "uucp.h"

#define STATNAME(f, n) ((void)sprintf(f, "%s/%s", STATDIR, n))
#define S_SIZE 100

/*
 * make system status entry
 *	name -> system name
 *	text -> text string to read
 *	type -> ?
 * return:
 *	none
 */
void
systat(name, type, text, retry)
register int type;
char *name, *text;
long retry;
{
	register FILE *fp;
	int count;
	char filename[MAXFULLNAME], line[S_SIZE];
	time_t prestime;
	long presretry = 0;
	char *vec[5];

	line[0] = '\0';
	(void) time(&prestime);
	count = 0;
	STATNAME(filename, name);

	fp = fopen(filename, "r");
	if (fp != NULL) {
		if (fgets(line, S_SIZE, fp) != NULL)
			if (getargs(line, vec, 4) == 4) {
			    count = atoi(vec[1]);
			    presretry = atol(vec[3]);
			}
		if (count <= 0)
			count = 0;
		fclose(fp);
	}

	switch (type) {
		/* if there is already a status file - don't change */
	case SS_TIME_WRONG:
	case SS_LOCKED_DEVICE:
	case SS_CANT_ACCESS_DEVICE:
		if (count > 1)
		    return;
		break;

		/* reset count on successful call */
	case SS_OK:
	case SS_INPROGRESS:
	case SS_CONVERSATION:	/* failure during conversation - contact ok */
		retry = 0;
		count = 0;
		break;

		/* Startup failure means protocol mismatch--some administrative
		 * action must be taken
		 */
	case SS_STARTUP:
		retry = MAXRETRYTIME;
		count = 0;
		break;
	default:	/*  increment count and set up retry time */
		count++;
		if (!retry) {	/* use exponential backoff */
		    if (presretry < RETRYTIME)
			retry = RETRYTIME;
		    else {
			retry = presretry + presretry;
			if (retry > MAXRETRYTIME)
			    retry = MAXRETRYTIME;
		    }
		}
		else {	/* use specified time */
		    if (retry < RETRYTIME)
			retry = RETRYTIME;
		}
		break;
	}

	fp = fopen(filename, "w");
	/* can't ASSERT since assert() calls systat() */
	if (fp == NULL) {
	    errent(Ct_OPEN, filename, errno, __FILE__, __LINE__);
	    cleanup(FAIL);
	}
	(void) chmod(filename, 0666);
	(void) fprintf(fp, "%d %d %ld %ld %s %s\n",
		type, count, prestime, retry, text, name);
	(void) fclose(fp);
	return;
}

/*
 * check system status for call
 *	name -> system to check	
 * return:  
 *	0	-> ok
 *	>0	-> system status
 */
callok(name)
char *name;
{
	register FILE *fp;
	register int t;
	long retrytime;
	int count, type;
	char filename[MAXFULLNAME], line[S_SIZE];
	time_t lasttime, prestime;

	STATNAME(filename, name);
	fp = fopen(filename, "r");
	if (fp == NULL)
		return(SS_OK);

	if (fgets(line, S_SIZE, fp) == NULL) {

		/* no data */
		fclose(fp);
		(void) unlink(filename);
		return(SS_OK);
	}

	fclose(fp);
	(void) time(&prestime);
	sscanf(line, "%d%d%ld%ld", &type, &count, &lasttime, &retrytime);
	t = type;

	switch(t) {
	case SS_SEQBAD:
	case SS_LOGIN_FAILED:
	case SS_DIAL_FAILED:
	case SS_BAD_LOG_MCH:
	case SS_BADSYSTEM:
	case SS_CANT_ACCESS_DEVICE:
	case SS_WRONG_MCH:
	case SS_RLOCKED:
	case SS_RUNKNOWN:
	case SS_RLOGIN:
	case SS_UNKNOWN_RESPONSE:
	case SS_CHAT_FAILED:

		if (prestime - lasttime < retrytime) {
			logent("RETRY TIME NOT REACHED", "NO CALL");
			CDEBUG(4, "RETRY TIME (%ld) NOT REACHED\n", retrytime);
			return(type);
		}

		return(SS_OK);
	default:
		return(SS_OK);
	}
}
