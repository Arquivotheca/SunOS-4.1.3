/*      Copyright (c) 1988 Sun Microsystems Inc. */
/*        All Rights Reserved   		 */

#if	!defined(lint) && defined(SCCSIDS)
static char *sccsid = "@(#)gettext.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <locale.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <nl_types.h>
#include <errno.h>
#include "gettext.h"

#define	TRUE		1
#define	FALSE		0
#define	MAXOPENARCH	2
#define	MAXPATHNAME	256
#define	MAXMEMBERFILE	2048
#define	MAXFMT		32

char 	*lookup();
int  	loadmessages();

extern char _locales[MAXLOCALE - 1][MAXLOCALENAME + 1];
extern void     init_statics();
char		*_current_domain; 

static char 	*saved_locale;
static char	*saved_domain = NULL;
static char	*current_fmt = NULL;
static char 	*text_start;
static long	member_size, total_size;
static struct 	msg_header  *cur_hdr;
static struct 	index *cur_index;
static int 	maxtags;
unsigned char	curr_archive = 0; 

/* Pointer to start of the loaded data region that
 * contains the messages
 */
char 		*mstart;



char *gettext(msgid)
char *msgid;
{
	register char *ret;

	if (msgid == NULL)
		return NULL;
	if (_current_domain == NULL) {
		if ((_current_domain = (char *) malloc(MAXDOMAIN)) == NULL)
			return msgid;
		strcpy(_current_domain, "default");
	}
	if (saved_domain == NULL)
		if ((saved_domain = (char *)malloc(MAXDOMAIN)) == NULL)
			return msgid;

	/* Always leave the first version of saved_locale a null
	 * string because we always want to try and load at least one
	 * message archive
	 */
	if (saved_locale == NULL)
		if ((saved_locale = (char *)malloc(MAXLOCALENAME+1)) == NULL)
			return msgid;

	if (_locales[0][0] == '\0')
		       init_statics();

	if (strcmp(_locales[LC_MESSAGES-1], saved_locale) ||
	    strcmp(_current_domain, saved_domain)) {

		/* Reload new messages from disk if either of the
		 * above two have changed
		 */

		if (loadmessages() == 0) 
			return msgid;
		curr_archive++;
	}

	if ( (curr_archive == 0) || (ret=lookup(msgid)) == NULL)
		return msgid;
	else
		return ret;
};

/* 	loadmessages() - Will open a disk file from the correct
 *	category and locale and will allocate the archive to the
 *	current runtime version of the message archive.
 *	Caches the last open archive to speed up multi-domain
 *	accessing, in the case of a process with several textdomain()
 *	calls.
 */

loadmessages()
{
	char *nlocale, *nlspath;
	int i;
	register int fd, result;
	char pathname[MAXPATHLEN];
	char tpathname[MAXPATHLEN];
	char mbuf[SARMAG];
	struct stat statbuf;
	char *p, *str;
	struct 	ar_hdr  arbuf;

	str = NULL; total_size = 0; i = 0;
	nlocale = _locales[LC_MESSAGES-1];
						  
	/* Even if the following code fails to get information
	 * we don't want to keep entering this code over and over
	 * again until the locale or the domain is really changed.
	 */
	strcpy(saved_locale, _locales[LC_MESSAGES-1]);
	strcpy(saved_domain, _current_domain);

	for (p=saved_domain;(*p != '\0') && (*p != '/');++p)
		;

	if (*p == '/')  {

		/* Assume domainname is absolute full pathname */

		strcpy(pathname, saved_domain);
		fd = open(pathname, O_RDONLY);

	} else {
		if ( ((nlspath = getenv("NLSPATH")) == 0) || (*nlspath == '\0') ) {
	
	 		/* Regular single parse in the default locale areas	*/

			while (++i < 3) {
				(void) strcpy(pathname, PRIVATE_LOCALE_DIR);
				(void) strcat(pathname, "LC_MESSAGES/");
				(void) strcat(pathname, nlocale);
				(void) strcat(pathname, "/");
				(void) strcat(pathname, _current_domain);
				if ((fd = open(pathname, O_RDONLY)) < 0 && errno == ENOENT) {
					(void) strcpy(pathname, LOCALE_DIR);
					(void) strcat(pathname, "LC_MESSAGES/");
					(void) strcat(pathname, nlocale);
					(void) strcat(pathname, "/");
					(void) strcat(pathname, _current_domain);
					if ( (fd = open(pathname, O_RDONLY)) >= 0)
						break;
				}
				else
					break;
				if (i == 1)
					if (strcmp(nlocale, "default") == 0)
						i++;
					else
						nlocale = "default";
				else if (i == 2)
					nlocale = "C";
			}
		} else {
			/* parse the NLSPATH variable including 
			 * expansion of the empowered locale settings 
			 */
			do {
				p = nlspath;
				i = 0;
				while (*p && *p++ != ':') i++;
				if (i) {
					strncpy(pathname, nlspath, i);
					pathname[i] = '\0';
				} else
					continue;

				/* expand % here before calling stat 
				 * Do not use built in localename here
				 */
			        nlocale = getenv("LANG");
				for (i=0;pathname[i] != '\0';i++) {
					if (pathname[i] != '%')
						continue;
					switch(pathname[i+1]) {
					case 'L':	
						strcpy(tpathname, &pathname[i+2]);
						memcpy(&pathname[i], nlocale, strlen(nlocale));
						strcpy(&pathname[i+strlen(nlocale)],tpathname);
						i=0;
						continue;
					case 'l':	
						strcpy(tpathname, &pathname[i+2]);
						memcpy(&pathname[i], nlocale, strcspn(nlocale,"_."));
						strcpy(&pathname[i+strcspn(nlocale,"_.")],tpathname);
						i=0;
						continue;
					case 'N':
						strcpy(tpathname, &pathname[i+2]);
						memcpy(&pathname[i], _current_domain, strlen(_current_domain));
						strcpy(&pathname[i+strlen(_current_domain)],tpathname);
						i=0;
						continue;
					case '%':
						strcpy(tpathname, &pathname[i+2]);
						strcpy(&pathname[i],tpathname);
						i=0;
						continue;
					}
				}
#ifdef DEBUG
fprintf(stderr, "gettext: about to stat file %s\n", pathname);
#endif
				result = stat(pathname, &statbuf);
				if ((result != -1) && (statbuf.st_mode) & S_IFDIR) {
                			strcat(pathname, "/");
                			fd = open(strcat(pathname, _current_domain), O_RDONLY);
				} else
					fd = open(pathname, O_RDONLY);

			} while ((fd <= 0) && *(nlspath = p));
				
        	}
	}
	if (fd < 0)
		return 0;

#ifdef DEBUG
fprintf(stderr, "gettext: %s opened ok\n", pathname);
#endif
	/*
	 *  We opened a file purporting to be the messages file ...
	 */

	if (read(fd, mbuf, SARMAG) != SARMAG || strncmp(mbuf, ARMAG, SARMAG)) {
#ifdef	DEBUG
		fprintf(stderr, "gettxt!!! not in message archive format\n");
#endif
		return 0;
	}

	/*
	 * This loop reads in each member file of the archive  and
	 *  allocates space for each file. Note that one invocation of
	 *  setlocale will load all member files contained in the whole
	 *  archive
	 */

	while ((i = read(fd, (char *)&arbuf, sizeof arbuf)) != 0) {
		if(i != sizeof arbuf && i > 1) {
#ifdef	DEBUG
			fprintf(stderr, "gettext!! useless size\n");
#endif
			close(fd);
			return 0 ;
		}
		if (i <= 1)
			break;
		if (strncmp((char *)arbuf.ar_fmag, ARFMAG, sizeof(arbuf.ar_fmag))) {
#ifdef	DEBUG
			fprintf(stderr, "gettext!! malformed message archive (at %ld)\n", lseek(fd, 0L, 1));
#endif
			close(fd);
			return 0 ;
		}

		/* Now determine the actual size of this
		 * member file and prepare to read in each record
		 */

		sscanf((char *)arbuf.ar_size, "%ld", &member_size);
		total_size += member_size;

#ifdef	DEBUG
		fprintf(stderr,"DEBUG: loading %d bytes from %s\n", member_size, pathname);
#endif
		if (str == NULL) {
			if ((str = (char *) malloc((unsigned)total_size)) == NULL) {
				(void) close(fd);
				return 0;
			}
		}
		else if ((str = (char *)realloc(str, (unsigned)total_size)) == NULL ) {
			(void) close(fd);
			return 0;
		}
		if (read(fd, str, (int)member_size) != member_size ) {
#ifdef	DEBUG
			fprintf(stderr, "gettxt!!! not in message archive format\n");
#endif
			(void) close(fd);
			return 0;
		}
	}

	close(fd);
	if (mstart != NULL) {
		/* cacheing goes in here , but its not here yet*/
		free(mstart);
	}
	mstart = str;

	/* following structures used during a message dereference */
	/* cur_hdr is the start of the member file, including index section */
	/* text_start is the address of the real target strings */

	(char *) cur_hdr = mstart;
	(char *) cur_index = mstart + sizeof(struct msg_header);
	text_start = mstart + cur_hdr->ptr;
	maxtags = cur_hdr -> maxno;

	return 1;


}

/* Select target string from the current domain and the
 * current locale.  At present we only have one format per domain
 */

char *lookup(msgid)
char *msgid;
{
	int i, value;
	char *msgtag;

	if (cur_hdr->format_type == INT) {

		/* this section assumes that format of domain
		 * indicated some form of integer conversion
		 * Simple roll is good enough
		 */

		sscanf(msgid, cur_hdr->format, &value);
#ifdef	DEBUG
		fprintf(stderr,"start search for int %d\n", value);
#endif
		for (i=0; i<maxtags; i++) {
			if (cur_index->msgnumber == value) {
#ifdef	DEBUG
				fprintf(stderr,"Integer lookup found %s\n", (char *)(text_start + cur_index->rel_addr));
#endif
				return (char *)(text_start + cur_index->rel_addr);
			}
#ifdef	DEBUG
			fprintf(stderr,"Skipped over one index\n");
#endif
			cur_index++;
		}

		/* fell off the end of the table and failed */

		return NULL;

	} else {

		/* We have to perform a string search,
	    	 * or a fixed width string search
		 */

		(char *) cur_index = (mstart + sizeof(struct msg_header));

		/* simple roll through is good enough */

		for (i=0; i<maxtags; i++) {
			msgtag = (cur_index->rel_addr)+text_start;
			if (strcmp(msgtag, msgid) == 0)
				return (char *) msgtag+strlen(msgtag)+1;
			cur_index++;
		}

		/* fell off the end of the table and failed */

		return NULL;
	}
}

/* The second argument is not yet used - future expansion */

char *textdomain(newdomain, newfmt)
char *newdomain; char *newfmt;
{
	if (newdomain == NULL)
		return _current_domain;
	if (_current_domain == NULL) { 
		if ((_current_domain = (char *) malloc(MAXDOMAIN)) == NULL) 
			return NULL; 
		strcpy(_current_domain, "default"); 
	}
	if ((strlen(newdomain) > MAXDOMAIN) || (strlen(newfmt) > MAXFMT))
		return NULL;
	strcpy(_current_domain, newdomain);
	if (current_fmt == NULL) {
			if ((current_fmt = (char *)malloc(MAXFMT)) == NULL)
				return NULL;
			}
		strcpy(current_fmt, newfmt);
	return _current_domain;
}

char *dgettext(domain, msgid)
char *domain; char *msgid;
{
	if ((strlen(domain) > MAXDOMAIN)) 
		return msgid;
	if ((domain != NULL) && (*domain != 0))
		strcpy(_current_domain, domain);
	return (gettext(msgid));
}
