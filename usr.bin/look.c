#ifndef lint
static	char sccsid[] = "@(#)look.c 1.1 92/07/30 SMI"; /* from UCB 4.2 07/02/81 */
#endif
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define TAB	'\t'
#define SPACE	' '

FILE *dfile;
char *filenam  = "/usr/dict/words";

int fold;
int dict;
int tab;
#define WORDSIZE 257
char entry[WORDSIZE];
char word[WORDSIZE];
char key[WORDSIZE];

main(argc,argv)
char **argv;
{
	register c;
	long top,bot,mid;
	char *wstring, *ptr;

	while(argc>=2 && *argv[1]=='-') {
		for(;;) {
			switch(*++argv[1]) {
			case 'd':
				dict++;
				continue;
			case 'f':
				fold++;
				continue;
			case 't':
				tab = argv[1][1];
				if(tab)
					++argv[1];
				continue;
			case 0:
				break;
			default:
				continue;
			}
			break;
		}
		argc --;
		argv++;
	}
	if(argc<=1)
		return;
	if(argc==2) {
		fold++;
		dict++;
	} else
		filenam = argv[2];
	dfile = fopen(filenam,"r");
	if(dfile==NULL) {
		fprintf(stderr,"look: can't open %s\n",filenam);
		exit(2);
	}
	wstring = strdup(argv[1]);
	if (tab != NULL) {
		if ((ptr = strchr(wstring, tab)) != NULL) {
			*++ptr = '\0';
		}
	}
	canon(wstring,key);
	bot = 0;
	fseek(dfile,0L,2);
	top = ftell(dfile);
	for(;;) {
		mid = (top+bot)/2;
		fseek(dfile,mid,0);
		do {
			c = getc(dfile);
			mid++;
		} while(c!=EOF && c!='\n');
		if(!getword(entry))
			break;
		canon(entry,word);
		switch(compare(key,word)) {
		case -2:
		case -1:
		case 0:
			if(top<=mid)
				break;
			top = mid;
			continue;
		case 1:
		case 2:
			bot = mid;
			continue;
		}
		break;
	}
	fseek(dfile,bot,0);
	while(ftell(dfile)<top) {
		if(!getword(entry))
			return;
		canon(entry,word);
		switch(compare(key,word)) {
		case -2:
			return;
		case -1:
		case 0:
			puts(entry);
			break;
		case 1:
		case 2:
			continue;
		}
		break;
	}
	while(getword(entry)) {
		canon(entry,word);
		switch(compare(key,word)) {
		case -1:
		case 0:
			puts(entry);
			continue;
		}
		break;
	}
	exit(0);
	/* NOTREACHED */
}

compare(s,t)
register char *s,*t;
{
	for(;*s==*t;s++,t++)
		if(*s==0)
			return(0);
	return(*s==0? -1:
		*t==0? 1:
		*s<*t? -2:
		2);
}

getword(w)
register char *w;
{
	register c;
	register avail = WORDSIZE - 1;

	while(avail--) {
		c = getc(dfile);
		if(c==EOF)
			return(0);
		if(c=='\n')
			break;
		*w++ = c;
	}
	while (c != '\n')
		c = getc(dfile);
	*w = 0;
	return(1);
}

canon(old,new)
char *old,*new;
{
	register c;
	register avail = WORDSIZE - 1;

	for(;;) {
		*new = c = *old++;
		if(c==0) {
			*new = 0;
			break;
		}
		if(dict) {
			if(!isalnum(c) && c != SPACE && c != TAB)
				continue;
		}
		if(fold) {
			if(isupper(c))
				*new += 'a' - 'A';
		}
		new++;
		avail--;
		if (avail <= 0) {
			*new = 0;
			break;
		}
	}
}
