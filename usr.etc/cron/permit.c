/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)permit.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <errno.h>
#include "cron.h"

#define ROOT	"root"

extern FILE *fopen_configfile();
static int within();

/****************/
char *getuser(uid)
/****************/
int uid;
{
	struct passwd *nptr,*getpwuid();

	if ((nptr=getpwuid(uid)) == NULL)
		return(NULL);
	return(nptr->pw_name);
}


/**********************/
allowed(user,allow,deny)
/**********************/
char *user,*allow,*deny;
{
	register FILE *permfile;
	char pathbuf[MAXPATHLEN];

	errno = 0;
	if ( (permfile = fopen_configfile(allow, "r", pathbuf)) != NULL) {
		if ( within(user,permfile) )
			return(1);
		else
			return(0);
	} else if (errno != ENOENT)
		return(0);	/* file inaccessible - deny access */

	errno = 0;
	if ( (permfile = fopen_configfile(deny, "r", pathbuf)) != NULL ) {
		if ( within(user,permfile) )
			return(0);
		else
			return(1);
	} else if (errno != ENOENT)
		return(0);	/* file inaccessible - deny access */
	if ( strcmp(user,ROOT)==0 ) return(1);
		else return(0);
}


/************************/
static within(username,cap)
/************************/
char *username;
FILE *cap;
{
	char line[UNAMESIZE];
	int i;

	while ( fgets(line,UNAMESIZE,cap) != NULL ) {
		for ( i=0 ; line[i] != '\0' ; i++ ) {
			if ( isspace(line[i]) ) {
				line[i] = '\0';
				break; }
		}
		if ( strcmp(line,username)==0 ) {
			fclose(cap);
			return(1); }
	}
	fclose(cap);
	return(0);
}
