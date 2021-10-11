/*LINTLIBRARY*/
#ifndef lint
static	char sccsid[] = "@(#)report.c 1.1 92/07/30 Copyright 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "report.h"
#include <sys/param.h>
#include <sys/wait.h>

static	FILE	*report_file;
static	FILE	*lib_file;
static	FILE	*nse_fp;
static	char	*target_being_reported_for;
static	char	*search_dir;
static  char	nse_tmpfile[30];
static	int	is_path = 0;
static	char	sfile[MAXPATHLEN];

extern char	*alloca();
extern char	**environ;
extern char	*getenv();
extern char	*index();
extern char	*malloc();
extern char	*sprintf();
extern char	*strcpy();

FILE *
get_report_file()
{
	return(report_file);
}

char *
get_target_being_reported_for()
{
	return(target_being_reported_for);
}

static int
close_report_file()
{
	(void)fputs("\n", report_file);
	(void)fclose(report_file);
}

static void
clean_up(fp, tmp_fp, spath, tpath, unlinkf)
	FILE		*fp;
	FILE		*tmp_fp;
	char		*spath;
	char		*tpath;
	int		unlinkf;
{
	fclose(fp);
	fclose(tmp_fp);
	fclose(nse_fp);
	unlink(nse_tmpfile);
	if (unlinkf)
		unlink(tpath);
	else
		rename(tpath, spath);
}


/*
 *  Update the file, if necessary.  We don't want to rewrite
 *  the file if we don't have to because we don't want the time of the file
 *  to change in that case.
 */
static void
close_file()
{
	char		line[MAXPATHLEN+2];
	char		buf[MAXPATHLEN+2];
	FILE		*fp;
	FILE		*tmp_fp;
	char		spath[MAXPATHLEN];
	char		tpath[MAXPATHLEN];
	char		lpath[MAXPATHLEN];
	char		*err;
	int		len;
	int		changed = 0;

	fprintf(nse_fp, "\n");
	fclose(nse_fp);
	if ((nse_fp = fopen(nse_tmpfile, "r")) == NULL) {
		return;
	}
	sprintf(spath, "%s/%s", search_dir, NSE_DEPINFO);
	sprintf(tpath, "%s/.tmp%s.%d", search_dir, NSE_DEPINFO, getpid());
	sprintf(lpath, "%s/%s", search_dir, NSE_DEPINFO_LOCK);
	err = file_lock(spath, lpath, 0);
	if (err) {
		fprintf(stderr, "%s\n", err);
		return;
	}
	if ((fp = fopen(spath, "r+")) == NULL) {
		if (is_path) {
			if ((fp = fopen(spath, "w")) == NULL) {
				fprintf(stderr,"Cannot open `%s' for writing\n",
				    spath);
				unlink(lpath);
				return;
			}
			while (fgets(line, MAXPATHLEN+2, nse_fp) != NULL) {
				fprintf(fp, "%s", line);
			}
			fclose(nse_fp);
		}
		fclose(fp);
		unlink(lpath);
		unlink(nse_tmpfile);
		return;
	}
	if ((tmp_fp = fopen(tpath, "w")) == NULL) {
		return;
	}
	len = strlen(sfile);
	while (fgets(line, MAXPATHLEN+2, fp) != NULL) {
		if (strncmp(line, sfile, len) == 0 && line[len] == ':') {
			/* Found entry for file.  Has it changed? */
			while (fgets(buf, MAXPATHLEN+2, nse_fp) != NULL) {
				if (is_path) {
					fprintf(tmp_fp, "%s", buf);
					if (strcmp(line, buf)) {
						/* changed */
						changed = 1;
					}
				}
				if (buf[strlen(buf)-1] == '\n') {
					break;
				}
			}
			if (changed || !is_path) {
				while (fgets(line, MAXPATHLEN, fp) != NULL) {
					fputs(line, tmp_fp);
				}
				clean_up(fp, tmp_fp, spath, tpath, 0);
			} else {
				clean_up(fp, tmp_fp, spath, tpath, 1);
			}
			unlink(lpath);
			return;
		}
		fputs(line, tmp_fp);
	}
	/* Entry never found.  Add it if there is a search path */
	if (is_path) {
		while (fgets(line, MAXPATHLEN+2, nse_fp) != NULL) {
			fprintf(fp, "%s", line);
		}
	}
	clean_up(fp, tmp_fp, spath, tpath, 1);
	unlink(lpath);
}


static void
report_dep(iflag, filename)
	char		*iflag;
	char		*filename;
{

	if (nse_fp == NULL) {
		sprintf(nse_tmpfile, "/tmp/%s.%d", NSE_DEPINFO, getpid());
		if ((nse_fp = fopen(nse_tmpfile, "w")) == NULL) {
			return;
		}
		if ((search_dir = getenv("NSE_DEP")) == NULL) {
			return;
		}
		on_exit(close_file, 0);
		strcpy(sfile, filename);
		if (iflag == NULL || *iflag == '\0') {
			return;
		}
		fprintf(nse_fp, "%s:", sfile);
	}
	fprintf(nse_fp, " ");
	fprintf(nse_fp, iflag);
	if (iflag != NULL) {
		is_path = 1;
	}
}

void
report_libdep(lib, flag)
	char		*lib;
	char		*flag;
{
	char		*ptr;
	char		filename[MAXPATHLEN];
	char		*p;

	if ((p= getenv(SUNPRO_DEPENDENCIES)) == NULL) {
		return;
	}
	ptr = index(p, ' ');
	sprintf(filename, "%s-%s", ptr+1, flag);
	is_path = 1;
	report_dep(lib, filename);
}

void
report_search_path(iflag)
	char		*iflag;
{
	char		curdir[MAXPATHLEN];
	char		*sdir;
	char		*getwd();
	char		*newiflag;
	char		filename[MAXPATHLEN];
	char		*p, *ptr;

	if ((sdir = getenv("NSE_DEP")) == NULL) {
		return;
	}
	if ((p= getenv(SUNPRO_DEPENDENCIES)) == NULL) {
		return;
	}
	ptr = index(p, ' ');
	sprintf(filename, "%s-CPP", ptr+1);
	getwd(curdir);
	if (strcmp(curdir, sdir) != 0 && strlen(iflag) > 2 && 
	    iflag[2] != '/') {
		/* Makefile must have had an "cd xx; cc ..." */
		/* Modify the -I path to be relative to the cd */
		newiflag = (char *)malloc(strlen(iflag) + strlen(curdir) + 2);
		sprintf(newiflag, "-%c%s/%s", iflag[1], curdir, &iflag[2]);
		report_dep(newiflag, filename);
	} else {
		report_dep(iflag, filename);
	}
}

void
report_dependency(name)
	register char	*name;
{
	register char	*filename;
	char		buffer[MAXPATHLEN+1];
	register char	*p;
	register char	*p2;
	char		spath[MAXPATHLEN];

	if (report_file == NULL) {
		if ((filename= getenv(SUNPRO_DEPENDENCIES)) == NULL) {
			lib_file = report_file = (FILE *)-1;
			return;
		}
		(void)strcpy(buffer, name);
		name = buffer;
		p = index(filename, ' ');
		*p= 0;
		if ((report_file= fopen(filename, "a")) == NULL) {
			if ((report_file= fopen(filename, "w")) == NULL) {
				report_file= (FILE *)-1;
				return;
			}
		}
		(void)on_exit(close_report_file, (char *)report_file);
		if ((p2= index(p+1, ' ')) != NULL)
			*p2= 0;
		target_being_reported_for= (char *)malloc((unsigned)(strlen(p+1)+1));
		(void)strcpy(target_being_reported_for, p+1);
		(void)fputs(p+1, report_file);
		(void)fputs(":", report_file);
		*p= ' ';
		if (p2 != NULL)
			*p2= ' ';
	}
	if (report_file == (FILE *)-1)
		return;
	(void)fputs(name, report_file);
	(void)fputs(" ", report_file);
}

#ifdef MAKE_IT
void
make_it(filename)
	register char	*filename;
{
	register char	*command;
	register char	*argv[6];
	register int	pid;
	union wait	foo;

	if (getenv(SUNPRO_DEPENDENCIES) == NULL) return;
	command= alloca(strlen(filename)+32);
	(void)sprintf(command, "make %s\n", filename);
	switch (pid= fork()) {
		case 0: /* child */
			argv[0]= "csh";
			argv[1]= "-c";
			argv[2]= command;
			argv[3]= 0;			
			(void)dup2(2, 1);
			execve("/bin/sh", argv, environ);
			perror("execve error");
			exit(1);
		case -1: /* error */
			perror("Fork error");
		default: /* parent */
			while (wait(&foo) != pid);};
}
#endif
