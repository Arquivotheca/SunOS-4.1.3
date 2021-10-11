/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)mkmakefile.c 1.1 92/07/30 SMI"; /* from UCB 5.9 5/6/86 */
#endif not lint

/*
 * Build the makefile for the system, from
 * the information in the files files and the
 * additional files for the machine being compiled to.
 */

#include <stdio.h>
#include <ctype.h>
#include "y.tab.h"
#include "config.h"
#include <sys/stat.h>
#include <sys/file.h>

#define next_word(fp, wd) \
	{ register char *word = get_word(fp); \
	  if (word == (char *)EOF) \
		wd = (char *)0; 	/* should never happen */ \
	  else \
		wd = word; \
	}

static	struct file_list *fcur;
char *tail();

/*
 * Lookup a file, by name.
 */
struct file_list *
fl_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(fp->f_fn, file))
			return (fp);
	}
	return (0);
}

/*
 * Lookup a file, by final component name.
 */
struct file_list *
fltail_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(tail(fp->f_fn), tail(file)))
			return (fp);
	}
	return (0);
}

/*
 * Make a new file list entry
 */
struct file_list *
new_fent()
{
	register struct file_list *fp;

	fp = (struct file_list *) malloc(sizeof *fp);
	fp->f_needs = 0;
	fp->f_next = 0;
	fp->f_flags = 0;
	fp->f_type = 0;
	if (fcur == 0)
		fcur = ftab = fp;
	else
		fcur->f_next = fp;
	fcur = fp;
	return (fp);
}

char	*COPTS;
char	*SYSDIR;

/*
 * Build the makefile from the skeleton
 */
makefile(objname, fastmake)
	char *objname;
	int fastmake;
{
	FILE *ifp, *ofp;
	char line[BUFSIZ];
	struct opt *op;

	read_files();
	ifp = fopen("../conf/Makefile.src", "r");
	if (ifp == 0) {
		perror("../conf/Makefile.src");
		exit(1);
	}
	ofp = fopen(path("Makefile"), "w");
	if (ofp == 0) {
		perror(path("Makefile"));
		exit(1);
	}
	fprintf(ofp, "IDENT=-D%s -D%s", machinename, raise(ident));
	if (profiling)
		fprintf(ofp, " -DGPROF");
	if (cputype == 0) {
		printf("cpu type must be specified\n");
		exit(1);
	}
	{ struct cputype *cp;
	  for (cp = cputype; cp; cp = cp->cpu_next)
		fprintf(ofp, " -D%s", cp->cpu_name);
	}
	for (op = opt; op; op = op->op_next)
		if (op->op_value)
			fprintf(ofp, " -D%s=\"%s\"", op->op_name, op->op_value);
		else
			fprintf(ofp, " -D%s", op->op_name);
	fprintf(ofp, "\n");
	if (machine == MACHINE_VAX) {
		if (maxusers == 0) {
			printf("maxusers not specified; 24 assumed\n");
			maxusers = 24;
		} else if (maxusers < 8) {
			printf("minimum of 8 maxusers assumed\n");
			maxusers = 8;
		}
	} else {
		if (maxusers == 0) {
			printf("maxusers not specified; 8 assumed\n");
			maxusers = 8;
		} else if (maxusers < 2) {
			printf("minimum of 2 maxusers assumed\n");
			maxusers = 2;
		}
	}
	fprintf(ofp, "PARAM=-DMAXUSERS=%d\n", maxusers);
	for (op = mkopt; op; op = op->op_next)
		fprintf(ofp, "%s=%s\n", op->op_name, op->op_value);
	while (fgets(line, BUFSIZ, ifp) != 0) {
		if (*line == '%')
			goto percent;
		if (profiling && strncmp(line, "COPTS=", 6) == 0) {
			register char *cp;

			fprintf(ofp, 
			    "GPROF.EX=/usr/src/lib/libc/%s/csu/gmon.ex\n",
			    machinename);
			cp = index(line, '\n');
			if (cp)
				*cp = '\0';
			cp = line + 6;
			while (*cp && (*cp == ' ' || *cp == '\t'))
				cp++;
			COPTS = malloc((unsigned)(strlen(cp) + 1));
			if (COPTS == 0) 
				nomem();
			strcpy(COPTS, cp);
			fprintf(ofp, "%s -pg\n", line);
			continue;
		}
		if (strncmp(line, "SYSDIR=", 7) == 0) {
			register char *cp;

			cp = line + 7;
			while (*cp && (*cp == ' ' || *cp == '\t'))
				cp++;
			SYSDIR = malloc((unsigned)(strlen(cp) + 1));
			if (SYSDIR == 0) 
				nomem();
			strcpy(SYSDIR, cp);
			cp = index(SYSDIR, '\n');
			if (cp)
				*cp = '\0';	/* remove newline */
		}
		fprintf(ofp, "%s", line);
		continue;
	percent:
		if (eq(line, "%OBJS\n")) {
			do_special(ofp, objname);
			do_objs(ofp, objname, fastmake);
			if (fastmake)
				do_xobj(ofp, objname);
			do_confobjs(ofp);
		} else if (eq(line, "%CFILES\n"))
			do_cfiles(ofp);
		else if (eq(line, "%CONFFILES\n"))
			do_conffiles(ofp);
		else if (eq(line, "%SFILES\n"))
			do_sfiles(ofp);
		else if (eq(line, "%LFILES\n"))
			do_Lfiles(ofp, objname);
		else if (eq(line, "%RULES\n"))
			do_rules(ofp);
		else if (eq(line, "%LOAD\n"))
			do_load(ofp, fastmake);
		else
			fprintf(stderr,
			    "Unknown %% construct in generic makefile: %s",
			    line);
	}
	(void) fclose(ifp);
	(void) fclose(ofp);
}

/*
 * Open a file and save it's file pointer and name.
 * Used in maintaining a stack of nested includes.
 */
FILE *
stack_open(name, stack)
	char *name;
	struct fstack *stack;
{
	if ((stack->fp = fopen(name, "r")) == NULL)
		return (NULL);
	stack->nm = (char *)malloc(strlen(name) + 1);
	strcpy(stack->nm, name);
	return (stack->fp);
}

/*
 * Read in the information about files used in making the system.
 * Store it in the ftab linked list.
 */
read_files()
{
	FILE *fp;
	register struct file_list *tp, *pf;
	register struct device *dp;
	register struct opt *op;
	char *wd, *this, *needs, *devorprof;
	char fname[32];
	int nreqs, first = 1, configdep, special, symbolic_info, isdup;
	struct fstack fstack[INCNEST];
	int depth;

	ftab = 0;
	(void) strcpy(fname, "files");
openit:
	depth = 0;
	fp = stack_open(fname, &fstack[depth]);
	if (fp == NULL) {
		perror(fname);
		exit(1);
	}
next:
	/*
	 * filename	[ standard | optional ] [ config-dependent ]
	 *	[ dev* | profiling-routine ] [ not-supported ]
	 *	[ device-driver ] [ special ] [ symbolic-info ]
	 * OR
	 * include	filename
	 *
	 * The above syntax is not quite accurate. We also now
	 * allow ...device-driver not-supported
	 */
	special = 0;
	symbolic_info = 0;
	wd = get_word(fp);
	if (wd == (char *)EOF) {
		(void) fclose(fp);
		(void) free(fstack[depth--].nm);
		if (depth >= 0) {
			fp = fstack[depth].fp;
			goto next;
		}
		if (first == 1) {
			(void) sprintf(fname, "files.%s", machinename);
			first++;
			if (access(fname, R_OK) == 0)
				goto openit;
		}
		if (first == 2) {
			if (ident == NULL) {
				fprintf(stderr, "config: Need ident line\n");
				exit(1);
			}
			(void) sprintf(fname, "files.%s", raise(ident));
			first++;
			if (access(fname, R_OK) == 0)
				goto openit;
		}
		return;
	}
	if (wd == 0)
		goto next;
	if (*wd == '#') {	/* ignore comment lines starting with # */
		while (get_word(fp) != 0)
			;
		goto next;
	}

	/* a very simple-minded 'include' facility is implemented here */
	if (eq(wd, "include")) {
		next_word(fp, wd);
		if (wd == 0) {
			printf("%s: illegal 'include' syntax\n",
			    fstack[depth].nm);
			exit(1);
		}
		if (++depth >= INCNEST) {
			printf("cannot open '%s': too many nested includes\n",
			    wd);
			exit(1);
		}
		/* flush the rest of this input line */
		while (get_word(fp) != 0)
			;
		fp = stack_open(wd, &fstack[depth]);
		if (fp == NULL) {
			perror(wd);
			exit(1);
		}
		goto next;
	}

	this = ns(wd);
	next_word(fp, wd);
	if (wd == 0) {
		printf("%s: No type for %s.\n",
		    fstack[depth].nm, this);
		exit(1);
	}
	if ((pf = fl_lookup(this)) && (pf->f_type != INVISIBLE || pf->f_flags))
		isdup = 1;
	else
		isdup = 0;
	tp = 0;
	if (first == 3 && (tp = fltail_lookup(this)) != 0)
		printf("%s: Local file %s overrides %s.\n",
		    fstack[depth].nm, this, tp->f_fn);
	nreqs = 0;
	devorprof = "";
	configdep = 0;
	needs = 0;
	if (eq(wd, "standard"))
		goto checkdev;
	if (!eq(wd, "optional")) {
		printf("%s: %s must be optional or standard\n", fstack[depth].nm, this);
		exit(1);
	}
nextopt:
	next_word(fp, wd);
	if (wd == 0)
		goto doneopt;
	if (eq(wd, "config-dependent")) {
		configdep++;
		goto nextopt;
	}
	if (eq(wd, "special")) {
		special++;
		goto nextopt;
	}
	if (eq(wd, "symbolic-info")) {
		symbolic_info++;
		goto nextopt;
	}
	devorprof = wd;
	if (eq(wd, "device-driver") || eq(wd, "profiling-routine") ||
	    eq(wd, "not-supported")) {
		next_word(fp, wd);
		goto save;
	}
	nreqs++;
	if (needs == 0 && nreqs == 1)
		needs = ns(wd);
	if (isdup)
		goto invis;
	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, wd))
			goto nextopt;
	/*
	 * Now check the option list to see if we want this file
	 */
	for (op = opt; op != 0; op = op->op_next)
		if (op->op_value == 0 && eq(op->op_name, wd)) {
			if (nreqs == 1) {
				free(needs);
				needs = 0;
			}
			goto nextopt;
		}
invis:
	while (get_word(fp) != 0)
		;
	if (tp == 0)
		tp = new_fent();
	tp->f_fn = this;
	tp->f_type = INVISIBLE;
	tp->f_needs = needs;
	tp->f_flags = isdup;
	goto next;

doneopt:
	if (nreqs == 0) {
		printf("%s: what is %s optional on?\n",
		    fstack[depth].nm, this);
		exit(1);
	}

checkdev:
	if (wd) {
		next_word(fp, wd);
		if (wd) {
			if (eq(wd, "config-dependent")) {
				configdep++;
				goto checkdev;
			}
			devorprof = wd;
			next_word(fp, wd);
		}
	}

save:
	if (wd && eq(wd, "special")) {
		special = 1;
		next_word(fp, wd);
	} else if (wd && eq(wd, "symbolic-info")) {
		symbolic_info = 1;
		next_word(fp, wd);
	} else if (eq(devorprof, "special")) {
		special = 1;
		devorprof = "";
	} else if (eq(devorprof, "symbolic-info")) {
		symbolic_info = 1;
		devorprof = "";
	}
	if (wd) {
		if (eq (wd, "not-supported")) {
			goto notsup;
		}
		printf("%s: syntax error describing %s\n",
		    fstack[depth].nm, this);
		exit(1);
	}
	if (eq(devorprof, "profiling-routine") && profiling == 0)
		goto next;
	if (tp == 0)
		tp = new_fent();
	tp->f_fn = this;
	if (eq(devorprof, "device-driver"))
		tp->f_type = DRIVER;
	else if (eq(devorprof, "profiling-routine"))
		tp->f_type = PROFILING;
	else if (eq(devorprof, "not-supported")) {
notsup:
		printf("%s:  trying to use unsupported device '%s'\n",
			fstack[depth].nm, needs);
		exit(1);
	} else
		tp->f_type = NORMAL;
	tp->f_special = special;
	tp->f_flags = 0;
	if (configdep)
		tp->f_flags |= CONFIGDEP;
	if (symbolic_info)
		tp->f_flags |= SYMBOLIC_INFO;
	tp->f_needs = needs;
	if (pf && pf->f_type == INVISIBLE)
		pf->f_flags = 1;		/* mark as duplicate */
	goto next;
}

opteq(cp, dp)
	char *cp, *dp;
{
	char c, d;

	for (; ; cp++, dp++) {
		if (*cp != *dp) {
			c = isupper(*cp) ? tolower(*cp) : *cp;
			d = isupper(*dp) ? tolower(*dp) : *dp;
			if (c != d)
				return (0);
		}
		if (*cp == 0)
			return (1);
	}
}

do_special(fp, objname)
	FILE *fp;
	char *objname;
{
	register struct file_list *tp;
	register int lpos, len;
	register char *cp, och, *sp;
	char *tail();
	int nosrc;

	fprintf(fp, "SPECIAL=");
	lpos = 6;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (!tp->f_special)
			continue;
		nosrc = nosource(tp->f_fn);
		sp = tail(tp->f_fn);
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		len += nosrc ? 11 : 1;
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		if (nosrc)
			fprintf(fp, "../%s/%s ", objname, sp);
		else
			fprintf(fp, "%s ", sp);
		lpos += len;
		*cp = och;
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_objs(fp, objname, fastmake)
	FILE *fp;
	char *objname;
	int fastmake;
{
	register struct file_list *tp;
	register int lpos, len;
	register char *cp, och, *sp;
	char *tail();
	int nosrc;

	fprintf(fp, "\nOBJS=\t");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_special)
			continue;
		nosrc = nosource(tp->f_fn);
		sp = tail(tp->f_fn);
		if (nosrc && fastmake)
			continue;
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		len += nosrc ? 11 : 1;
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		if (nosrc)
			fprintf(fp, "../%s/%s ", objname,  sp);
		else
			fprintf(fp, "%s ", sp);
		lpos += len;
		*cp = och;
	}
	if (lpos != 8)
		putc('\n', fp);
}

int	got_xobj = 0;

do_xobj(fp, objname)
	FILE *fp;
	char *objname;
{
	register struct file_list *tp;
	register int lpos, len;
	register char *cp, och, *sp;
	char *tail();
	int olen = strlen(objname);

	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_special)
			continue;
		if (!nosource(tp->f_fn))
			continue;
		sp = tail(tp->f_fn);
		if (got_xobj++ == 0) {
			fprintf(fp, "\nXOBJS=\t");
			lpos = 8;
		}
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		len += olen + 4;	/* 4 == strlen("../" + "/") */
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "../%s/%s ", objname, sp);
		lpos += len + 1;
		*cp = och;
	}
	if (lpos != 8 && got_xobj)
		putc('\n', fp);
}

do_confobjs(fp)
	FILE *fp;
{
	fprintf(fp, "\nCONFOBJS=param.o ioconf.o ");
	if (machine == MACHINE_SUN2 ||
	    machine == MACHINE_SUN3 ||
	    machine == MACHINE_SUN3X) {
		fprintf(fp, "mbglue.o ");
	}
	putc('\n', fp);
}

do_conffiles(fp)
	FILE *fp;
{
	register struct file_list *fl;
	register int lpos, len;

	fprintf(fp, "CONFFILES=param.c ioconf.c ");
	lpos = 35;
	if (machine == MACHINE_SUN2 ||
	    machine == MACHINE_SUN3 ||
	    machine == MACHINE_SUN3X) {
		fprintf(fp, "mbglue.s ");
		lpos += 9;
	}
	for (fl = conf_list; fl != 0; fl = fl->f_next) {
		if (fl->f_type != SYSTEMSPEC)
			continue;
		if (eq(fl->f_fn, "generic"))
			continue;
		len = 6 + strlen(fl->f_fn);
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		lpos += len + 1;
		fprintf(fp, "conf%s.c ", fl->f_fn);
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_cfiles(fp)
	FILE *fp;
{
	register struct file_list *tp, *fl;
	register int lpos, len;

	fprintf(fp, "CFILES=\t");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 'c')
			continue;
		if (nosource(tp->f_fn))
			continue;
		/* add in the length of any prepended string */
		if ((len = 0 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", tp->f_fn);
		lpos += len + 1;
	}
	for (fl = conf_list; fl != 0; fl = fl->f_next) {
		if ((fl->f_type != SYSTEMSPEC) || !(eq(fl->f_fn, "generic")))
			continue;
		len = 7 + strlen(fl->f_fn);
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		lpos += len + 1;
		fprintf(fp, "conf%s.c ", fl->f_fn);
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_sfiles(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len;

	fprintf(fp, "SFILES=\t");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 's')
			continue;
		if (nosource(tp->f_fn))
			continue;
		/* add in the length of any prepended string */
		if ((len = 0 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", tp->f_fn);
		lpos += len + 1;
	}
	if (lpos != 8)
		putc('\n', fp);
}

int	got_Lfiles = 0;

do_Lfiles(fp, objname)
	FILE *fp;
	char *objname;
{
	register struct file_list *tp, *fl;
	register int lpos, len;
	register char *cp, och, *sp;
	int nosrc;
	char *tail();

	got_Lfiles++;
	fprintf(fp, "\nLFILES=\t");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 'c')
			continue;
		nosrc = nosource(tp->f_fn);
		sp = tail(tp->f_fn);
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'L';
		len += nosrc ? 11 : 1;
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		if (nosrc)
			fprintf(fp, "../%s/%s ", objname,  sp);
		else
			fprintf(fp, "%s ", sp);
		lpos += len;
		*cp = och;
	}
	for (fl = conf_list; fl != 0; fl = fl->f_next) {
		if (fl->f_type != SYSTEMSPEC)
			continue;
		len = 7 + strlen(fl->f_fn);
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		lpos += len;
		fprintf(fp, "conf%s.L ", fl->f_fn);
	}
	if (lpos != 8) {
		lpos = 8;
		fprintf(fp, "\\\n\t");
	}
	fprintf(fp, "param.L ioconf.L ");
	lpos += 17;
	putc('\n', fp);
}

char *
tail(fn)
	char *fn;
{
	register char *cp;

	cp = rindex(fn, '/');
	if (cp == 0)
		return (fn);
	return (cp+1);
}

/*
 * Create the makerules for each file
 * which is part of the system.
 * Devices are processed with the special c2 option -i
 * which avoids any problem areas with i/o addressing
 * (e.g. for the VAX); assembler files are processed by as.
 */
do_rules(f)
	FILE *f;
{
	register char *cp, *np, och, *tp;
	register struct file_list *ftp;
	char *extras;

for (ftp = ftab; ftp != 0; ftp = ftp->f_next) {
	if (ftp->f_type == INVISIBLE)
		continue;
	if (machine == MACHINE_VAX && ftp->f_special)
		continue;
	cp = (np = ftp->f_fn) + strlen(ftp->f_fn) - 1;
	if (nosource(np))
		continue;
	och = *cp;
	*cp = '\0';
	tp = tail(np);
	if (och == 'o') {
		fprintf(f, "%so:\n\t-cp ${SYSDIR}/%so .\n", tp, np);
		continue;
	}
	fprintf(f, "%so: ${SYSDIR}/%s%c\n", tp, np, och);
	if (och == 's') {
		switch (machine) {

		case MACHINE_VAX:
			if (profiling) {
				fprintf(f, "\t${AS} ${SYSDIR}/%ss ; ", np);
				fprintf(f, "${LD} -r -x a.out -o %so\n\n", tp);
			} else
				fprintf(f, "\t${AS} -o %so ${SYSDIR}/%ss\n\n", tp, np);
			break;

		case MACHINE_SUN2:
		case MACHINE_SUN3:
		case MACHINE_SUN3X:
			fprintf(f, "\t${CPP} %s ${SYSDIR}/%ss >%spp\n",
			    "-DLOCORE ${CPPOPTS}", np, tp);
			if (profiling) {
				fprintf(f, "\t${AS} %spp ; ", tp);
				fprintf(f, "${LD} -r -x a.out -o %so\n\n", tp);
			} else
				fprintf(f, "\t${AS} -o %so %spp\n", tp, tp);
			fprintf(f, "\trm -f %spp\n\n", tp);
			break;
		case MACHINE_SUN4:
		case MACHINE_SUN4C:
		case MACHINE_SUN4M:
			if (profiling) {
				fprintf(f, "\t${AS} %s ${SYSDIR}/%ss\n",
					"-P -DLOCORE ${CPPOPTS} ",
					np);
				fprintf(f, "\t${LD} -r -x a.out -o %so\n\n", tp);
			} else {
				fprintf(f, "\t${AS} %s -o %so ${SYSDIR}/%ss\n\n",
					"-P -DLOCORE ${CPPOPTS}",
					tp, np);
			}
			break;
		}
		continue;
	}
	if (ftp->f_flags & CONFIGDEP)
		extras = "${PARAM} ";
	else
		extras = "";

	switch (ftp->f_type) {

	case NORMAL:
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -c -S ${COPTS} %s${SYSDIR}/%sc\n",
			    extras, np);
			fprintf(f, "\t${C2} %ss | sed -f ${SYSDIR}/%s/asm.sed |",
			    tp, machinename);
			fprintf(f, " ${AS} -o %so\n", tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

		case MACHINE_SUN2:
		case MACHINE_SUN3:
		case MACHINE_SUN3X:
		case MACHINE_SUN4:
		case MACHINE_SUN4C:
		case MACHINE_SUN4M:
			fprintf(f, "\t${CC} -c %s %s${SYSDIR}/%sc\n\n",
			    (ftp->f_flags & SYMBOLIC_INFO) ? "-g ${COPTS}" :
								"${CFLAGS}",
			    extras, np);
			if (got_Lfiles == 0)
				break;
			fprintf(f, "%sL: ${SYSDIR}/%s%c\n", tp, np, och);
			fprintf(f, "\t@echo %sc:\n", tp);
			fprintf(f, "\t@-(${CPP} ${LCOPTS} %s${SYSDIR}/%sc | \\\n",
			    extras, np);
			fprintf(f,
			    "\t  ${LINT1} ${LOPTS} > %sL ) 2>&1 | ${LTAIL}\n\n",
			    tp);
			break;
		}
		break;

	case DRIVER:
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -c -S ${COPTS} %s${SYSDIR}/%sc\n",
				extras, np);
			fprintf(f,"\t${C2} -i %ss | sed -f ${SYSDIR}/%s/asm.sed |",
			    tp, machinename);
			fprintf(f, " ${AS} -o %so\n", tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

		case MACHINE_SUN2:
		case MACHINE_SUN3:
		case MACHINE_SUN3X:
		case MACHINE_SUN4:
		case MACHINE_SUN4C:
		case MACHINE_SUN4M:
			fprintf(f, "\t${CC} -c ${CFLAGS} %s${SYSDIR}/%sc\n\n",
				extras, np);
			if (got_Lfiles == 0)
				break;
			fprintf(f, "%sL: ${SYSDIR}/%s%c\n", tp, np, och);
			fprintf(f, "\t@echo %sc:\n", tp);
			fprintf(f, "\t@-(${CPP} ${LCOPTS} %s${SYSDIR}/%sc | \\\n",
			    extras, np);
			fprintf(f,
			    "\t  ${LINT1} ${LOPTS} > %sL ) 2>&1 | ${LTAIL}\n\n",
			    tp);
			break;
		}
		break;

	case PROFILING:
		if (!profiling)
			continue;
		if (COPTS == 0) {
			fprintf(stderr,
			    "config: COPTS undefined in generic makefile\n");
			COPTS = "";
		}
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -c -S %s %s${SYSDIR}/%sc\n",
				COPTS, extras, np);
			fprintf(f, "\tex - %ss < ${GPROF.EX}\n", tp);
			fprintf(f,
			    "\tsed -f ${SYSDIR}/vax/asm.sed %ss | ${AS} -o %so\n",
			    tp, tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

		case MACHINE_SUN2:
		case MACHINE_SUN3:
		case MACHINE_SUN3X:
		case MACHINE_SUN4:
		case MACHINE_SUN4C:
		case MACHINE_SUN4M:
			fprintf(f, "\t${CC} -c ${CFLAGS} ${SYSDIR}/%sc\n\n", np);
			if (got_Lfiles == 0)
				break;
			fprintf(f, "%sL: ${SYSDIR}/%s%c\n", tp, np, och);
			fprintf(f, "\t@echo %sc:\n", tp);
			fprintf(f, "\t@-(${CPP} ${LCOPTS} %s${SYSDIR}/%sc | \\\n",
			    extras, np);
			fprintf(f,
			    "\t  ${LINT1} ${LOPTS} > %sL ) 2>&1 | ${LTAIL}\n\n",
			    tp);
			break;
		}
		break;

	default:
		printf("Don't know rules for %s\n", np);
		break;
	}
	*cp = och;
}
}

/*
 * Create the load strings
 */
do_load(f, fastmake)
	register FILE *f;
	int fastmake;
{
	register struct file_list *fl;
	int first = 1;
	struct file_list *do_systemspec();

	fprintf(f, "\nINSTALLFILES=");
	for (fl = conf_list; fl != 0; fl = fl->f_next)
		if (fl->f_type == SYSTEMSPEC)
			fprintf(f, " %s", fl->f_needs);
	fprintf(f, "\n\nall:\t$(INSTALLFILES)\n\n");
	fl = conf_list;
	while (fl) {
		if (fl->f_type != SYSTEMSPEC) {
			fl = fl->f_next;
			continue;
		}
		fl = do_systemspec(f, fl, first, fastmake);
		if (first)
			first = 0;
	}
}

struct file_list *
do_systemspec(f, fl, first, fastmake)
	FILE *f;
	register struct file_list *fl;
	int first;
	int fastmake;
{

	fprintf(f, "%s: vers.o conf%s.o ioconf.o\n", fl->f_needs, fl->f_fn);
	fprintf(f, "\t@echo loading %s\n\t@rm -f %s\n",
	    fl->f_needs, fl->f_needs);
	switch (machine) {

	case MACHINE_VAX:
		fprintf(f,
		    "\t@${LD} -n -o %s -e start -X -T 80000000  ${SPECIAL} ",
		    fl->f_needs);
		if (fastmake && got_xobj)
			fprintf(f, "%s.o ", fl->f_needs);
		fprintf(f, "${OBJS} vers.o ${CONFOBJS} conf%s.o\n", fl->f_fn);
		fprintf(f, "\t@echo rearranging symbols\n");
		fprintf(f, "\t@symorder -s ${SYSDIR}/vax/symbols.sort %s\n",
		    fl->f_needs);
		break;

	case MACHINE_SUN2:
	case MACHINE_SUN3:
	case MACHINE_SUN3X:
	case MACHINE_SUN4:
	case MACHINE_SUN4C:
	case MACHINE_SUN4M:
		fprintf(f,
		    ((machine == MACHINE_SUN2) ?
		      "\t@${LD} -o %s -e _start -N -X -T 4000 ${SPECIAL} ":
		    ((machine == MACHINE_SUN3) ?
		      "\t@${LD} -o %s -e _start -N -X -T 0E004000 ${SPECIAL} ":
		    ((machine == MACHINE_SUN4 || machine == MACHINE_SUN4C) ?
		      "\t@${LD} -o %s -e _start -N -p -X -T F8004000 ${SPECIAL} ":
		    ((machine == MACHINE_SUN4M) ?
		      "\t@${LD} -o %s -e _start -N -p -X -T F0004000 ${SPECIAL} ":
		    ((machine == MACHINE_SUN3X) ?
		      "\t@${LD} -o %s -e _start -N -X -T F8004000 ${SPECIAL} ":
		      "???"))))),
		    fl->f_needs);
		if (fastmake && got_xobj)
			fprintf(f, "%s.o ", fl->f_needs);
		fprintf(f, "${OBJS} vers.o ${CONFOBJS} conf%s.o\n", fl->f_fn);
		fprintf(f, "\t@echo rearranging symbols\n");
		fprintf(f, "\t@symorder -s ${SYSDIR}/sun/symbols.sort %s\n",
		    fl->f_needs);
		break;

	}
	fprintf(f, "\t@size %s\n", fl->f_needs);
	fprintf(f, "\t@chmod 755 %s\n\n", fl->f_needs);

	if (first) {
		fprintf(f, "vers.o: Makefile ${SPECIAL} ${OBJS} ${CONFOBJS} ");
		if (machine == MACHINE_VAX)
			fprintf(f, "${SYSDIR}/vax/symbols.sort\n");
		else
			fprintf(f, "${SYSDIR}/sun/symbols.sort\n");
	}
	if (fastmake && got_xobj)
		fprintf(f, "vers.o: %s.o\n", fl->f_needs);
	fprintf(f, "vers.o: conf%s.o\n\n", fl->f_fn);

	if (fastmake && got_xobj)
		do_ldbuild(f, fl->f_needs);
	do_confspec(f, fl->f_fn);
	for (fl = fl->f_next; fl && fl->f_type == SWAPSPEC; fl = fl->f_next)
		;
	return (fl);
}

do_ldbuild(f, name)
	FILE *f;
	register char *name;
{

	fprintf(f, "%s.o: Makefile\n", name);
	fprintf(f, "\t@echo 'building %s.o'\n", name);
	fprintf(f, "\t@${LD} -r -o %s.o ${XOBJS}\n\n", name);
}

do_confspec(f, name)
	FILE *f;
	char *name;
{

	fprintf(f, "conf%s.o: conf%s.c\n", name, name);
	fprintf(f, "\t${CC} -I. -c -O ${COPTS} conf%s.c\n\n", name);
	if (got_Lfiles != 0) {
		fprintf(f, "conf%s.L: conf%s.c\n", name, name);
		fprintf(f, "\t@echo conf%s.c:\n", name);
		fprintf(f, "\t@-(/lib/cpp ${LCOPTS} conf%s.c | \\\n",
		    name);
		fprintf(f, "\t  ${LINT1} ${LOPTS} > conf%s.L ) ", name);
		fprintf(f, "2>&1 | ${LTAIL}\n\n");
	}
}

char *
raise(str)
	register char *str;
{
	register char *cp = str;

	while (*str) {
		if (islower(*str))
			*str = toupper(*str);
		str++;
	}
	return (cp);
}

int	getfromsccs = 0;

nosource(fn)
	char *fn;
{
	char fullfn[200];
	struct stat sb;

	if (SYSDIR == 0) {
		fprintf(stderr,
		    "config: SYSDIR undefined in generic makefile\n");
		SYSDIR = "";
	}
	strcpy(fullfn, SYSDIR);
	strcat(fullfn, "/");
	strcat(fullfn, fn);
	if (getfromsccs)
		return(!FileExistOrGet(fullfn, 1));
	return (stat(fullfn, &sb) != 0);
}

nomem()
{
	printf("config: out of memory\n");
	exit(1);
}
