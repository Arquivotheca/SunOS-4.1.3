/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)utility.c 1.1 92/07/30"	/* from SVR3.2 uucp:utility.c 2.3 */

#include "uucp.h"

/* imports */
extern char *strsave();

/* exports */

/* private */
static void logError();

#define TY_ASSERT	1
#define TY_ERROR	2

/*
 *	produce an assert error message
 * input:
 *	s1 - string 1
 *	s2 - string 2
 *	i1 - integer 1 (usually errno)
 *	file - __FILE of calling module
 *	line - __LINE__ of calling module
 */
void
assert(s1, s2, i1, file, line)
char *s1, *s2, *file;
{
	logError(s1, s2, i1, TY_ASSERT, file, line);
}


/*
 *	produce an assert error message
 * input: -- same as assert
 */
void
errent(s1, s2, i1, file, line)
char *s1, *s2, *file;
{
	logError(s1, s2, i1, TY_ERROR, file, line);
}

#define EFORMAT	"%sERROR (%.9s)  pid: %d (%s) %s %s (%d) [FILE: %s, LINE: %d]\n"

static
void
logError(s1, s2, i1, type, file, line)
char *s1, *s2, *file;
{
	register FILE *errlog;
	char text[BUFSIZ];
	int pid;

	if (Debug)
		errlog = stderr;
	else {
		errlog = fopen(ERRLOG, "a");
		(void) chmod(ERRLOG, 0666);
	}
	if (errlog == NULL)
		return;

	pid = getpid();

	(void) fprintf(errlog, EFORMAT, type == TY_ASSERT ? "ASSERT " : " ",
	    Progname, pid, timeStamp(), s1, s2, i1, file, line);

	(void) fclose(errlog);
	(void) sprintf(text, " %sERROR %.100s %.100s (%.9s)",
	    type == TY_ASSERT ? "ASSERT " : " ",
	    s1, s2, Progname);
	if (type == TY_ASSERT)
	    systat(Rmtname, SS_ASSERT_ERROR, text, Retrytime);
	return;
}


/* timeStamp - create standard time string
 * return
 *	pointer to time string
 */

char *
timeStamp()
{
	register struct tm *tp;
	time_t clock;
	static char str[20];

	(void) time(&clock);
	tp = localtime(&clock);
	(void) sprintf(str, "%d/%d-%d:%2.2d:%2.2d", tp->tm_mon + 1,
	    tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
	return(str);
}

/* add new name/value pair to nvlist */
nvstore(pair)
struct name_value *pair;
{
	struct name_value *newp;
	struct nvlist *nvl;

	newp = (struct name_value *) malloc(sizeof(struct name_value));
	if (newp == 0) {
		errent(Ct_ALLOCATE, "newp", errno, __FILE__, __LINE__);
		return;
	}
	newp->name = strsave(pair->name);
	if (newp->name == 0) {
		errent(Ct_ALLOCATE, "name", errno, __FILE__, __LINE__);
		return;
	}
	newp->value = strsave(pair->value);
	if (newp->value == 0) {
		errent(Ct_ALLOCATE, "value", errno, __FILE__, __LINE__);
		return;
	}
	nvl = (struct nvlist *) malloc(sizeof(struct nvlist));
	if (nvl == 0) {
		errent(Ct_ALLOCATE, "nvl", errno, __FILE__, __LINE__);
		return;
	}
	nvl->entry = newp;
	nvl->next = Nvhead;
	Nvhead = nvl;
}

/* return value for argument name, 0 if not found */
char *
nvlookup(name)
char *name;
{
	struct nvlist *nvptr;

	for (nvptr = Nvhead; nvptr; nvptr = nvptr->next)
		if (EQUALS(nvptr->entry->name, name))
			return(nvptr->entry->value);	/* must not be 0! */
	DEBUG(5, "nvlookup: %s not found\n", name);
	return(0);
}
