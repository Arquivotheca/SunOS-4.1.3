/*
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)update.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>

char *list_reset();
char *list_next();
char *index();
char hostname[];

update_fstab(fstabs, local_fs, client_local_fs, remote_fs, opts)
	char *fstabs;
	char *local_fs;
	char *client_local_fs;
	char *remote_fs;
	char *opts;
{
	char *cp;
	char *fsp;
	char options[128];

	if(opts)
	    sprintf(options, ",%s", opts);
	else
	    *options = '\0';
	cp = list_reset(fstabs);
	printf("#\n# Update fstab\n#\t%s\n#\n", cp);
	printf("echo ''\n");
	printf("echo Update %s\n", cp);

	printf("rm -f %s.bak\n", cp);
	printf("if [ -f %s ] ; then cp %s %s.bak\nfi\n", cp, cp, cp);
	for (fsp = list_reset(local_fs); fsp; fsp = list_next(local_fs))
		printf("echo '%s 4.2 rw 1 1' >> %s\n", fsp, cp);
	for (fsp = list_reset(remote_fs); fsp;
	    fsp = list_next(remote_fs)){
		char path[256];

		strcpy(path, fsp);
		printf("echo '%s %s nfs rw%s 0 0' >> %s\n", path,
		    index(path, ':') + 1, options, cp);
	}

	for (cp = list_next(fstabs); cp; cp = list_next(fstabs)) {
		printf("#\n# Update fstab\n#\t%s\n#\n", cp);
		printf("echo Update %s\n", cp);
		printf("rm -f %s.bak\n", cp);
		printf("if [ -f %s ] ; then cp %s %s.bak\nfi\n", cp, cp, cp);
		for (fsp = list_reset(client_local_fs); fsp;
		    fsp = list_next(client_local_fs))
			printf("echo '%s nfs rw%s 0 0' >> %s\n", fsp, options, cp);
		for (fsp = list_reset(remote_fs); fsp;
		    fsp = list_next(remote_fs)){
			char path[256];

			strcpy(path, fsp);
			printf("echo '%s %s nfs rw%s 0 0' >> %s\n", path,
			    index(path, ':') + 1, options, cp);
	}
	}
		
}

update_audit_control(audit_controls, local_fs, client_local_fs,
    remote_fs, flags, minfree, dir_list)
	char *audit_controls;
	char *local_fs;
	char *client_local_fs;
	char *remote_fs;
	char *flags;
	char *minfree;
	char *dir_list;
{
	char *cp;
	char *fsp;
	char *dp;

	cp = list_reset(audit_controls);
	printf("#\n# Update audit_control\n#\t%s\n#\n", cp);
	printf("echo Update %s\n", cp);

	printf("rm -f %s.bak %s.X\n", cp, cp);

	for (fsp = list_reset(local_fs); fsp;
	    fsp = list_next(local_fs)) {
		dp = index(fsp, ' ') + 1;
		printf("echo 'dir:%s/files' >> %s.X\n", dp, cp);
	}
	for (fsp = list_reset(remote_fs); fsp;
	    fsp = list_next(remote_fs)) {
		char *opt_p;

		dp = index(fsp, ':') + 1;
		if ((opt_p = index(dp, '(')) != 0)
			*opt_p = NULL;
		printf("echo 'dir:%s/files' >> %s.X\n", dp, cp);
	}
	for (fsp = list_reset(dir_list); fsp;
	    fsp = list_next(dir_list))
		printf("echo 'dir:%s' >> %s.X\n", fsp, cp);
	if (flags==0)
		printf("echo 'flags:' >> %s.X\n", cp);
	else printf("echo 'flags:%s' >> %s.X\n", flags, cp);
	printf("echo 'minfree:%s' >> %s.X\n", minfree, cp);

	printf("if [ -f %s ] ; then mv %s %s.bak\nfi\n", cp, cp, cp);
	printf("mv %s.X %s\n", cp, cp);

	for (cp = list_next(audit_controls); cp;
	    cp = list_next(audit_controls)) {
		printf("#\n# Update audit_control\n#\t%s\n#\n", cp);
		printf("echo Update %s\n", cp);
		printf("rm -f %s.bak %s.X\n", cp, cp);
		for (fsp = list_reset(client_local_fs); fsp;
		    fsp = list_next(client_local_fs)) {
			dp = index(fsp, ' ') + 1;
			printf("echo 'dir:%s/files' >> %s.X\n", dp, cp);
		}
		for (fsp = list_reset(remote_fs); fsp;
		    fsp = list_next(remote_fs)) {
			char *opt_p;

			dp = index(fsp, ':') + 1;
			if ((opt_p = index(dp, '(')) != 0)
				*opt_p = NULL;
			printf("echo 'dir:%s/files' >> %s.X\n", dp, cp);
		}
		for (fsp = list_reset(dir_list); fsp;
		    fsp = list_next(dir_list))
			printf("echo 'dir:%s' >> %s.X\n", fsp, cp);

		if (flags==0)
			printf("echo 'flags:' >> %s.X\n", cp);
		else printf("echo 'flags:%s' >> %s.X\n", flags, cp);
		printf("echo 'minfree:%s' >> %s.X\n", minfree, cp);
		printf("if [ -f %s ] ; then mv %s %s.bak\nfi\n", cp, cp, cp);
		printf("mv %s.X %s\n", cp, cp);
	}
}

update_audit_warn(audit_warns, sysadmin)
	char *audit_warns;
	char *sysadmin;
{
	char *cp;

	for (cp = list_reset(audit_warns); cp; cp = list_next(audit_warns)) {
		printf("#\n# Update audit_warn\n#\t%s\n#\n", cp);
		printf("echo Update %s\n", cp);
		printf("sed -e 's/RECIPIENT=\"root\"/RECIPIENT=\"%s\"/' < %s > %s.X\n",
			sysadmin, cp, cp);
		printf("rm -f %s.bak\n", cp);
		printf("if [ -f %s ] ; then mv %s %s.bak\nfi\n", cp, cp, cp);
		printf("mv %s.X %s\n", cp, cp);
	}
}

update_export(base, local_fs, opts)
	char *base;
	char *local_fs;
	char *opts;
{
	char *cp = (base == NULL) ? "" : base;
	char *fsp;
	char *bp;
	char options[128];

	if(opts)
	    sprintf(options, "-%s", opts);
	else
	    *options = '\0';

	printf("#\n# Update export\n#\t%s/etc/exports\n#\n", cp);
	printf("echo ''\n");
	printf("echo Update %s/etc/exports\n", cp);

	for (fsp = list_reset(local_fs); fsp; fsp = list_next(local_fs)) {
		if ((bp = index(fsp, ' ')) != NULL)
			bp++;
		printf("echo '%s %s' >> %s/etc/exports\n", bp, options, cp);
	}
}
