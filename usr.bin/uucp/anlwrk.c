/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)anlwrk.c 1.1 92/07/30"	/* from SVR3.2 uucp:anlwrk.c 2.2 */
/*
	This module contains routines that find C. files
	in a system spool directory, return the next C. file
	to process, and break up the C. line into arguments
	for processing.
*/

#include "uucp.h"

#define BOOKMARK_PRE	'A'
#define CLEAN_RETURN(fp) {\
	if (fp != NULL) \
		(void) fclose(fp); \
	fp = NULL; \
	return(0); \
}

/* C.princetN0026 - ('C' + '.') - "princet" */
#define SUFSIZE	(MAXBASENAME - 2 - SYSNSIZE)
#define GRADECHAR(file) ((file)[strlen(file) - SUFSIZE])
#define LLEN 50
#define MAXRQST 250

static void insert();
static int  bldflst();

static char  Filent[LLEN][NAMESIZE]; /* array of C. file names (text)        */
static char *Fptr[LLEN];	     /* pointers to names in Filent          */
static short Nnext;		     /* index of next C. file in Fptr list   */
static short Nfiles = 0;	     /* Number of files in Filent	     */

/*
 * read a line from the workfile (C.file)
 *	file	-> work file  (Input/Output)  made '\0' after work completion
 *	wvec	-> address of array to return arguments (Output)
 *	wcount	-> maximum # of arguments to return in wvec
 *		NOTE: wvec should be large enough to accept wcount + 1 pointers
 *		since NULL is inserted after last item.
 * returns:
 *	0	   ->  no more work in this file
 *	positive # -> number of arguments
 */
anlwrk(file, wvec, wcount)
char *file, **wvec;
{
	register i;
	register FILE *p_bookmark;    /* pointer to afile 		    */
	static   FILE *fp = NULL;    /* currently opened C. file pointer    */
	static char afile[NAMESIZE]; /* file with line count for book marks */
	static char str[MAXRQST];    /* the string which  wvec points to    */
	static short acount;
	struct stat stbuf;
	int	nargs;		/* return value == # args in the line */

	if (file[0] == '\0') {
		if (fp != NULL)
			errent("anlwrk",
			   "attempt made to use old workfile was thwarted", 0,
			   __FILE__, __LINE__);
		CLEAN_RETURN(fp);
	}
	if (fp == NULL) {
		fp = fopen(file, "r");

		if (fp == NULL){ /* can't open C. file! */
			errent(Ct_OPEN,file,errno, __FILE__, __LINE__);
			/* this may not work, but we'll try it */
			/* It will fail if the C. name is more than */
			/* the standard 14 characters - if this is the */
			/* case, tocorrupt will exit with ASSERT */
			toCorrupt(file);
			return(0);
		}
		(void) fstat(fileno(fp), &stbuf);
		Nstat.t_qtime = stbuf.st_mtime;

		(void) strncpy(afile, BASENAME(file, '/'), NAMESIZE);
		afile[NAMESIZE] = NULLCHAR;
		*afile = BOOKMARK_PRE; /* make up name by replacing C with A */
		acount = 0;
		p_bookmark = fopen(afile, "r");
		if (p_bookmark != NULL) {
			/* get count of already completed work */
			i = fscanf(p_bookmark, "%hd", &acount);
			(void) fclose(p_bookmark);
			if (i <= 0)
				acount = 0;

			/* skip lines which have already been processed */
			for (i = 0; i < acount; i++) {
				if (fgets(str, MAXRQST, fp) == NULL)
					break;
			}
		}

	}

	if (fgets(str, MAXRQST, fp) == NULL) {
		ASSERT(unlink(file) == 0, Ct_UNLINK, file, errno);
		(void) unlink(afile);
		DEBUG(4,"Finished Processing file: %s\n",file);
		*file = '\0';
		CLEAN_RETURN(fp);
	}

	nargs = getargs(str, wvec, wcount);

	/* sanity checks for C. file */
	if ((str[0] != 'R' && str[0] != 'S')	/* legal wrktypes are R and S */
	 || (str[0] == 'R' && nargs < 6)	/* R lines need >= 6 entries */
	 || (str[0] == 'S' && nargs < 7)) {	/* S lines need >= 7 entries */
		/* bad C. file - stash it */
		toCorrupt(file);
		(void) unlink(afile);
		*file = '\0';
		CLEAN_RETURN(fp);
	}

	p_bookmark = fopen(afile, "w"); /* update bookmark file */
	if (p_bookmark == NULL)
	    errent(Ct_OPEN, afile, errno, __FILE__, __LINE__);
	else {
	    chmod(afile, CFILEMODE);
	    (void) fprintf(p_bookmark, "%d", acount);
	    (void) fclose(p_bookmark);
	}
	acount++;
	return(nargs);
}

/*
 * Check the list of work files (C.remote).
 * If it is empty or the present work is exhausted,
 * call bldflst to generate a new list.
 *
 *	file	-> address of array for return of full pathname; if
		   file == CNULL, just return grade info.
 * returns:
 *	0	-> no more work (or some error)
 *	>0	-> highest grade character of queued work
 *		   (low ascii order is higher priority)
 */
char
iswrk(file)
char *file;
{
	char rval;

	ASSERT(file != 0 || Nfiles == 0, "impossible args to iswrk", CNULL, Nfiles);

	/* for grade checking -- called by ifdate() in conn.c */
	if (file == 0) {
		if (bldflst() == 0)
			return(0);
		Nfiles = 0;
		return(GRADECHAR(Fptr[0]));
	}

	/* normal use of iswrk() */
	if (Nfiles == 0  && bldflst() == 0)
		return(0);

	rval = GRADECHAR(Fptr[Nnext]);
	(void) sprintf(file, "%s/%s", RemSpool, Fptr[Nnext]);
	Nfiles--;
	Nnext++;
	return(rval);
}


/*
 * build list of work files for given system using an insertion sort
 * Nfiles, Nnext, RemSpool and Rmtname are global
 *
 * return:
 *	number of C. files in list - (Nfiles)
 */
static
bldflst()
{
	register DIR *pdir;
	char filename[NAMESIZE];
	char prefix[SYSNSIZE+3];

	Nnext = Nfiles = 0;
	if ((pdir = opendir(RemSpool)) == NULL)
		return(0);

	(void) sprintf(prefix, "C.%.*s", SYSNSIZE, Rmtname);
	while (gnamef(pdir, filename) ) {
		if (!PREFIX(prefix, filename))
		    	continue;
		if ((strlen(filename)-strlen(prefix)) != SUFSIZE) {
			errent("bldflst: Funny filename", filename, 0,
			   __FILE__, __LINE__);
			continue;
		}
		if (!gradecheck(filename))	/* high grade only ... */
			continue;
		insert(filename);
	}
	closedir(pdir);
	return(Nfiles);
}

/*
 * get work return
 *	file	-> place to deposit file name
 *	wrkvec	-> array to return arguments
 *	wcount	-> max number of args for wrkvec
 * returns:
 *	nargs  	->  number of arguments
 *	0 	->  no arguments - fail
 */
gtwvec(file, wrkvec, wcount)
char *file, *wrkvec;
{
	register int nargs;

	DEBUG(7, "gtwvec: dir %s\n", RemSpool);
	while ((nargs = anlwrk(file, wrkvec, wcount)) == 0) {
		if (iswrk(file) == 0)
			return(0);
	}
	DEBUG(7, "        return - %d\n", nargs);
	return(nargs);
}


/*
 * insert - insert file name in sorted list
 * return - none
 */
static
void
insert(file)
char *file;
{
	register i, j;
	register char *p;

	DEBUG(7, "insert(%s)  ", file);
	for (i = Nfiles; i>0; i--) {
	    if (strcmp(file, Fptr[i-1]) > 0)
		break;
	}
	if (i == LLEN) /* if this is off the end get out */
	    return;

	/* get p (pointer) to where the text of name will go */
	if (Nfiles == LLEN)	/* last possible entry */
	    /* put in text of last and decrement Nfiles for make hole */
	    p = strcpy(Fptr[--Nfiles], file);
	else
	    p = strcpy(Filent[Nfiles], file);	/* copy to next free  */

	/* make a hole for new entry */
	for (j = Nfiles; j >i; j--)
	    Fptr[j] = Fptr[j-1];

	DEBUG(7, "insert %s ", p);
	DEBUG(7, "at %d\n", i);
	Fptr[i] = p;
	Nfiles++;
}

/* return 0 if fails grade checking, non-zero otherwise */
static
gradecheck(file)
char *file;
{
	return(MaxGrade == NULLCHAR || GRADECHAR(file) <= MaxGrade);
}
