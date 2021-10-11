#ifndef lint
static  char sccsid[] = "@(#)util.c 1.1 92/07/30 (C) 1985 Sun Microsystems, Inc.";
#endif

#include <stdio.h>
#include "util.h"




/*
 * This is just like fgets, but recognizes that "\\n" signals a continuation
 * of a line
 */
char *
getline(line,maxlen,fp)
	char *line;
	int maxlen;
	FILE *fp;
{
	register char *p;
	register char *start;


	start = line;

nextline:
	if (fgets(start,maxlen,fp) == NULL) {
		return(NULL);
	}	
	for (p = start; ; p++) {
		if (*p == '\n') {       
			if (*(p-1) == '\\') {
				start = p - 1;
				goto nextline;
			} else {
				return(line);	
			}
		}
	}
}	




	
void
fatal(message)
	char *message;
{
	(void) fprintf(stderr,"fatal error: %s\n",message);
	exit(1);
}
