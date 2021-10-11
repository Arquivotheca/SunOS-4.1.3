/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)pwck.c 1.1 92/07/30 SMI"; /* from S5R3 1.6 */
#endif

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/signal.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<ctype.h>
#define	ERROR1	"Too many/few fields"
#define ERROR2	"Bad character(s) in login name"
#define ERROR2a "First char in login name not lower case alpha"
#define ERROR2b "No login name"
#define ERROR2c "No netgroup name"
#define ERROR2d "First char in netgroup name not lower case alpha"
#define ERROR2e	"Bad character(s) in netgroup name"
#define ERROR3	"Login name too long"
#define ERROR4	"Invalid UID"
#define ERROR5	"Invalid GID"
#define ERROR6	"Login directory not found"
#define ERROR6a	"No login directory"
#define	ERROR7	"Optional shell file not found"

int eflag, code=0;
int badc;
char buf[512];

main(argc,argv)

int argc;
char **argv;

{
	int delim[512];
	char logbuf[80];
	FILE *fopen(), *fptr;
	char *fgets();
	int error();
	struct	stat obuf;
	long uid, gid;
	int len;
	register int i, j, colons;
	int yellowpages, netgroup;
	int nlognamec;
	char *pw_file;

	if(argc == 1) pw_file="/etc/passwd";
	else pw_file=argv[1];

	if((fptr=fopen(pw_file,"r"))==NULL) {
		fprintf(stderr,"cannot open %s\n",pw_file);
		exit(1);
	}

	while(fgets(buf,512,fptr)!=NULL) {

		colons=0;
		badc=0;
		uid=gid=0l;
		eflag=0;
		yellowpages=0;
		netgroup=0;

	/* See if this is a NIS reference */

		if(buf[0] == '+') {
			yellowpages++;
		}

	/*  Check number of fields */

		for(i=0 ; buf[i]!=NULL; i++) {
			if(buf[i]==':') {
				delim[colons]=i;
				++colons;
			}
		delim[6]=i;
		delim[7]=NULL;
		}
		if(colons != 6) {
			if(!yellowpages) error(ERROR1);
			continue;
		}

	/*  Check that first character is alpha and rest alphanumeric  */

		i = 0;
		if(yellowpages) {
			i++;
			if(buf[1] == '\0' || buf[1] == ':') continue;
			if(buf[1] == '@') {
				i++;
				netgroup++;
				if(buf[0] == ':') {
					error(ERROR2c);
				}
			}
		}
		else {
			if(buf[0] == ':') {
				error(ERROR2b);
			}
		}
		if(!(islower(buf[i]))) {
			error(netgroup? ERROR2d: ERROR2a);
		}
		for(nlognamec=0; buf[i]!=':'; nlognamec++, i++) {
			if(islower(buf[i]));
			else if(isdigit(buf[i]));
			else ++badc;
		}
		if(badc > 0) {
			error(netgroup? ERROR2e: ERROR2);
		}

	/*  Check for valid number of characters in logname  */

		if(!netgroup && nlognamec > 8) {
			error(ERROR3);
		}

	/*  Check that UID is numeric and <= 65535  */

		len = (delim[2]-delim[1])-1;
		if(len > 5 || (!yellowpages && len == 0)) {
			error(ERROR4);
		}
		else {
		    for (i=(delim[1]+1); i < delim[2]; i++) {
			if(!(isdigit(buf[i]))) {
				error(ERROR4);
				break;
			}
			uid = uid*10+(buf[i])-'0';
		    }
		    if(uid > 65535l  ||  uid < 0l) {
			error(ERROR4);
		    }
		}

	/*  Check that GID is numeric and <= 65535  */

		len = (delim[3]-delim[2])-1;
		if(len > 5 || (!yellowpages && len == 0)) {
			error(ERROR5);
		}
		else {
		    for(i=(delim[2]+1); i < delim[3]; i++) {
			if(!(isdigit(buf[i]))) {
				error(ERROR5);
				break;
			}
			gid = gid*10+(buf[i])-'0';
		    }
		    if(gid > 65535l  ||  gid < 0l) {
			error(ERROR5);
		    }
		}

	/*  Stat initial working directory  */

		for(j=0, i=(delim[4]+1); i<delim[5]; j++, i++) {
			logbuf[j]=buf[i];
		}
		if(!yellowpages && logbuf[0] == NULL) {
			error(ERROR6a);
		}
		else {
			if((stat(logbuf,&obuf)) == -1) {
				error(ERROR6);
			}
		}
		for(j=0;j<80;j++) logbuf[j]=NULL;

	/*  Stat of program to use as shell  */

		if((buf[(delim[5]+1)]) != '\n') {
			for(j=0, i=(delim[5]+1); i<delim[6]; j++, i++) {
				logbuf[j]=buf[i];
			}
			if((stat(logbuf,&obuf)) == -1) {
				error(ERROR7);
			}
			for(j=0;j<80;j++) logbuf[j]=NULL;
		}
	}
	fclose(fptr);
	exit(code);
	/* NOTREACHED */
}
/*  Error printing routine  */

error(msg)

char *msg;
{
	if(!(eflag)) {
		fprintf(stderr,"\n%s",buf);
		code = 1;
		++eflag;
	}
	if(!(badc)) {
	fprintf(stderr,"\t%s\n",msg);
	return;
	}
	else {
	fprintf(stderr,"\t%d %s\n",badc,msg);
	badc=0;
	return;
	}
}
