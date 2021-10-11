/*
 * Copyright (c) 1990, Sun Microsystems, Inc.
 */

#ifndef lint
static  char sccsid[] = "@(#)split.c 1.1 92/07/30 Copyr 1990 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>

char *list_reset();
char *list_next();

split_passwd(passwds, passwd_adjuncts, options)
	char *passwds;
	char *passwd_adjuncts;
	char *options;
{
	char *pp;
	char *pap;
	int clientflag = 0;
	int secureflag = 0;

	if(options && (strcmp(options, "secure") == 0))
		secureflag = 1;

	for (pp = list_reset(passwds), pap = list_reset(passwd_adjuncts);
	    pp && pap;
	    pp = list_next(passwds), pap = list_next(passwd_adjuncts)) {
		printf("#\n# Split passwd\n#\t%s\n#\t%s\n#\n", pp, pap);
		printf("echo Split %s into\n", pp);
		printf("echo '	' %s and \n", pp);
		printf("echo '	' %s \n", pap);
		printf("echo 'audit:*:::::all' > %s.X\n", pap);
		printf("sed -e 's,^\\([^:+]*\\):\\([^:]*\\):\\(.*\\)$,\\1:\\2:::::,'");
		printf(" %s | sed -e '/^audit:/d'", pp);
		printf(" >> %s.X\n", pap);
		printf("sed -e 's,^\\([^:+]*\\):\\([^:]*\\):\\(.*\\)$,\\1:##\\1:\\3,' %s", pp);
		/* Secure NFS requrires password to come from NIS */
		if(clientflag && secureflag)
		    	printf(" | sed -e 's/^audit:##audit/+audit:/'");
		printf(" > %s.X\n", pp);
		printf("rm -f %s.bak %s.bak\n", pp, pap);
		printf("if [ -f %s ] ; then mv %s %s.bak\nfi\n",
		    pp, pp, pp);
		printf("if [ -f %s ] ; then mv %s %s.bak\nfi\n",
		    pap, pap, pap);
		printf("mv %s.X %s\n", pp, pp);
		printf("mv %s.X %s\n", pap, pap);
		clientflag = 1;
	}
}

split_group(groups, group_adjuncts)
	char *groups;
	char *group_adjuncts;
{
	char *gp;
	char *gap;

	for (gp = list_reset(groups), gap = list_reset(group_adjuncts);
	    gp && gap;
	    gp = list_next(groups), gap = list_next(group_adjuncts)) {
		printf("#\n# Split group\n#\t%s\n#\t%s\n#\n", gp, gap);
		printf("echo Split %s into\n", gp);
		printf("echo '	' %s and \n", gp);
		printf("echo '	' %s \n", gap);
		printf("sed -e 's,^\\([^:+]*\\):\\([^:]*\\):\\(.*\\)$,\\1:#$\\1:\\3,'");
		printf(" %s > %s.X\n", gp, gp);
		printf("sed -e 's,^\\([^:+]*\\):\\([^:]*\\):\\(.*\\)$,\\1:\\2,'");
		printf(" %s > %s.X\n", gp, gap);
		printf("rm -f %s.bak %s.bak\n", gp, gap);
		printf("if [ -f %s ] ; then mv %s %s.bak\nfi\n",
		    gp, gp, gp);
		printf("if [ -f %s ] ; then mv %s %s.bak\nfi\n",
		    gap, gap, gap);
		printf("mv %s.X %s\n", gp, gp);
		printf("mv %s.X %s\n", gap, gap);
	}
}
