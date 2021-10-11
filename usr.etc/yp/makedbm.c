#ifndef lint
static  char sccsid[] = "@(#)makedbm.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <dbm.h>
#undef NULL
#include <stdio.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <ctype.h>

#include "ypdefs.h"
USE_YP_MASTER_NAME
USE_YP_LAST_MODIFIED
USE_YP_INPUT_FILE
USE_YP_OUTPUT_NAME
USE_YP_DOMAIN_NAME
USE_YP_SECURE
USE_YP_INTERDOMAIN
USE_DBM

#define MAXLINE 4096		/* max length of input line */
static char *get_date();
static char *any();
FILE *fopen(), *fclose();

main(argc, argv)
	char **argv;
{
	FILE *outfp, *infp;
	datum key, content, tmp;
	char buf[MAXLINE];
	char pagbuf[MAXPATHLEN];
	char tmppagbuf[MAXPATHLEN];
	char dirbuf[MAXPATHLEN];
	char tmpdirbuf[MAXPATHLEN];
	char *p,ic;
	char *infile, *outfile;
	char *infilename, *outfilename, *mastername, *domainname,
	    *interdomain_bind, *security, *lower_case_keys;
	char local_host[MAX_MASTER_NAME];
	int cnt,i;

	/* Ignore existing umask, always force 077 (owner rw only) */
	umask(077);
	infile = outfile = NULL; /* where to get files */
	/* name to imbed in database */
	infilename = outfilename = mastername = domainname = interdomain_bind =
	    security = lower_case_keys = NULL; 
	argv++;
	argc--;
	while (argc > 0) {
		if (argv[0][0] == '-' && argv[0][1]) {
			switch(argv[0][1]) {
				case 'i':
					infilename = argv[1];
					argv++;
					argc--;
					break;
				case 'o':
					outfilename = argv[1];
					argv++;
					argc--;
					break;
				case 'm':
					mastername = argv[1];
					argv++;
					argc--;
					break;
				case 'd':
					domainname = argv[1];
					argv++;
					argc--;
					break;
				case 'b':
					interdomain_bind = argv[0];
					break;
				case 'l':
					lower_case_keys = argv[0];
					break;
				case 's':
					security = argv[0];
					break;
				case 'u':
					unmake(argv[1]);
					argv++;
					argc--;
					exit(0);
				default:
					usage();
			}
		}
		else if (infile == NULL)
			infile = argv[0];
		else if (outfile == NULL)
			outfile = argv[0];
		else
			usage();
		argv++;
		argc--;
	}
	if (infile == NULL || outfile == NULL)
		usage();
	if (strcmp(infile, "-") != 0)
		infp = fopen(infile, "r");
	else
		infp = stdin;
	if (infp == NULL) {
		fprintf(stderr, "makedbm: can't open %s\n", infile);
		exit(1);
	}
	strcpy(tmppagbuf, outfile);
	strcat(tmppagbuf, ".tmp");
	strcpy(tmpdirbuf, tmppagbuf);
	strcat(tmpdirbuf, dbm_dir);
	strcat(tmppagbuf, dbm_pag);
	if ((outfp = fopen(tmpdirbuf, "w")) == NULL) {
	    	fprintf(stderr, "makedbm: can't create %s\n", tmpdirbuf);
		exit(1);
	}
	if ((outfp = fopen(tmppagbuf, "w")) == NULL) {
	    	fprintf(stderr, "makedbm: can't create %s\n", tmppagbuf);
		exit(1);
	}
	strcpy(dirbuf, outfile);
	strcat(dirbuf, ".tmp");
	if (dbminit(dirbuf) != 0) {
		fprintf(stderr, "makedbm: can't init %s\n", dirbuf);
		exit(1);
	}
	strcpy(dirbuf, outfile);
	strcpy(pagbuf, outfile);
	strcat(dirbuf, dbm_dir);
	strcat(pagbuf, dbm_pag);
	while (fgets(buf, sizeof(buf), infp) != NULL) {
		p = buf;
		cnt = strlen(buf) - 1; /* erase trailing newline */
		while (p[cnt-1] == '\\') {
			p+=cnt-1;
			if (fgets(p, sizeof(buf)-(p-buf), infp) == NULL)
				goto breakout;
			cnt = strlen(p) - 1;
		}
		p = any(buf, " \t\n");
		key.dptr = buf;
		key.dsize = p - buf;
		while (1) {
			if (p == NULL || *p == NULL) {
				fprintf(stderr, "makedbm: source file is garbage!\n");
				exit(1);
			}
			if (*p != ' ' && *p != '\t')
				break;
			p++;
		}
		content.dptr = p;
		content.dsize = strlen(p) - 1; /* erase trailing newline */
		if (lower_case_keys) 
			for (i=0; i<key.dsize; i++) {
				ic = *(key.dptr+i);
				if (isascii(ic) && isupper(ic)) 
					*(key.dptr+i) = tolower(ic);
			} 
		tmp = fetch(key);
		if (tmp.dptr == NULL) {
			if (store(key, content) != 0) {
				printf("problem storing %.*s %.*s\n",
				    key.dsize, key.dptr,
				    content.dsize, content.dptr);
				exit(1);
			}
		}
#ifdef DEBUG
		else {
			printf("duplicate: %.*s %.*s\n",
			    key.dsize, key.dptr,
			    content.dsize, content.dptr);
		}
#endif
	}
   breakout:
	addpair(yp_last_modified, get_date(infile));
	if (infilename)
		addpair(yp_input_file, infilename);
	if (outfilename)
		addpair(yp_output_file, outfilename);
	if (domainname)
		addpair(yp_domain_name, domainname);
	if (security)
		addpair(yp_secure, "");
	if (interdomain_bind)
		addpair(yp_interdomain, "");
	if (!mastername) {
		gethostname(local_host, sizeof (local_host) - 1);
		mastername = local_host;
	}
	addpair(yp_master_name, mastername);

	if (rename(tmppagbuf, pagbuf) < 0) {
		perror("makedbm: rename");
		unlink(tmppagbuf);		/* Remove the tmp files */
		unlink(tmpdirbuf);
		exit(1);
	}
	if (rename(tmpdirbuf, dirbuf) < 0) {
		perror("makedbm: rename");
		unlink(pagbuf);			/* Remove the tmp files */
		unlink(tmpdirbuf);
		exit(1);
	}
	exit(0);
}


/* 
 * scans cp, looking for a match with any character
 * in match.  Returns pointer to place in cp that matched
 * (or NULL if no match)
 */
static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

static char *
get_date(name)
	char *name;
{
	struct stat filestat;
	static char ans[MAX_ASCII_ORDER_NUMBER_LENGTH];/* ASCII numeric string*/

	if (strcmp(name, "-") == 0)
		sprintf(ans, "%010d", time(0));
	else {
		if (stat(name, &filestat) < 0) {
			fprintf(stderr, "makedbm: can't stat %s\n", name);
			exit(1);
		}
		sprintf(ans, "%010d", filestat.st_mtime);
	}
	return ans;
}

usage()
{
	fprintf(stderr,
"usage: makedbm -u file\n       makedbm [-b] [-s] [-i YP_INPUT_FILE] [-o YP_OUTPUT_FILE] [-d YP_DOMAIN_NAME] [-m YP_MASTER_NAME] infile outfile\n");
	exit(1);
}

addpair(str1, str2)
	char *str1, *str2;
{
	datum key;
	datum content;
	
	key.dptr = str1;
	key.dsize = strlen(str1);
	content.dptr  = str2;
	content.dsize = strlen(str2);
	if (store(key, content) != 0){
		printf("makedbm: problem storing %.*s %.*s\n",
		    key.dsize, key.dptr, content.dsize, content.dptr);
		exit(1);
	}
}

unmake(file)
	char *file;
{
	datum key, content;

	if (file == NULL)
		usage();
	
	if (dbminit(file) != 0) {
		fprintf(stderr, "makedbm: couldn't init %s\n", file);
		exit(1);
	}
	for (key = firstkey(); key.dptr != NULL; key = nextkey(key)) {
		content = fetch(key);
		printf("%.*s %.*s\n", key.dsize, key.dptr,
		    content.dsize, content.dptr);
	}
}
