/*
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)make.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>

char *add_to_list();
char *list_reset();
char *list_next();
char *index();
char hostname[];

char *
make_fstabs(base, roots, clients)
	char *base;
	char *roots;
	char *clients;
{
	char path[256];
	char *result;
	char *cp;

	if (base)
		sprintf(path, "%s/etc/fstab", base);
	else
		sprintf(path, "/etc/fstab");

	result = add_to_list(NULL, path);
	for (cp = list_reset(clients); cp; cp = list_next(clients)) {
		path[0] = '\0';
		if (base)
			strcat(path, base);
		if (roots)
			strcat(path, roots);
		strcat(path, "/");
		strcat(path, cp);
		strcat(path, "/etc/fstab");
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_audit_controls(base, roots, clients)
	char *base;
	char *roots;
	char *clients;
{
	char path[256];
	char *result;
	char *cp;

	if (base)
		sprintf(path, "%s/etc/security/audit/audit_control", base);
	else
		sprintf(path, "/etc/security/audit/audit_control");

	result = add_to_list(NULL, path);
	for (cp = list_reset(clients); cp; cp = list_next(clients)) {
		path[0] = '\0';
		if (base)
			strcat(path, base);
		if (roots)
			strcat(path, roots);
		strcat(path, "/");
		strcat(path, cp);
		strcat(path, "/etc/security/audit/audit_control");
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_local_fs(devices)
	char *devices;
{
	char path[256];
	char *result = NULL;
	char *cp;
	int suffix = 0;

	for (cp = list_reset(devices); cp; cp = list_next(devices)) {
		if (suffix == 0)
			sprintf(path, "/dev/%s /etc/security/audit/%s",
			    cp, hostname);
		else
			sprintf(path, "/dev/%s /etc/security/audit/%s.%d",
			    cp, hostname, suffix);
		suffix++;
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_client_local_fs(local_fs)
	char *local_fs;
{
	char path[256];
	char *result = NULL;
	char *cp;
	char *bp;

	for (cp = list_reset(local_fs); cp; cp = list_next(local_fs)) {
		bp = index(cp, ' ') + 1;
		sprintf(path, "%s:%s %s", hostname, bp, bp);
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_remote_fs(filesystems)
	char *filesystems;
{
	char part[256];
	char path[256];
	char *result = NULL;
	char *cp;
	char *colon_p;

	for (cp = list_reset(filesystems); cp; cp = list_next(filesystems)) {

		if ((colon_p = index(cp, ':')) == NULL)
			sprintf(part, "%s:%s", hostname, cp);
		else
			sprintf(part, "%s", cp);
		free(cp);
		colon_p = index(part, ':');
		*colon_p++ = '\0'; 

		if (*colon_p == '/')
			sprintf(path, "%s:%s", part, colon_p);
		else
			sprintf(path, "%s:/etc/security/audit/%s", part,
			    colon_p);

		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_passwds(base, roots, clients)
	char *base;
	char *roots;
	char *clients;
{
	char path[256];
	char ppath[256];
	char sysstg[128];
	char *result;
	char *cp;

	if (base)
		sprintf(path, "%s/etc/passwd", base);
	else
		sprintf(path, "/etc/passwd");

	result = add_to_list(NULL, path);
	for (cp = list_reset(clients); cp; cp = list_next(clients)) {
		path[0] = '\0';
		if (base)
			strcat(path, base);
		if (roots)
			strcat(path, roots);
		strcat(path, "/");
		strcat(path, cp);
		strcpy(ppath, path) ;
		strcat(path, "/etc/passwd");
		free(cp);
		sprintf(sysstg,"grep '##' %s >/dev/null",path);
		if (system(sysstg) == 0) {
			printf("#\n# ## in password file of %s indicates C2 conversion previously run.\n#\n",path);
			printf("echo '## in password file of %s indicates C2 conversion previously run.'\n",path);
			exit(1);
			}
		sprintf(sysstg,"grep '^audit:' %s >/dev/null",path);

		if (system(sysstg) != 0) { /* somehow, no "audit"    */
					/* account in /etc/passwd */
					/* -- add one in	  */
			printf("#\n# must add 'audit' account to password file %s.\n#\n",path);
			printf("echo 'audit:*:9:9::/etc/security/audit:/bin/csh' >/tmp/C2.$$passwd\n");
			printf("cat %s >>/tmp/C2.$$passwd\n",path);
			printf("mv %s %s.bak\n", path, path);
			printf("mv /tmp/C2.$$passwd %s\n", path);
		}

        	add_pseudo_user(ppath,"AUyppasswdd", 28, 10);
		add_pseudo_user(ppath,"AUpwdauthd", 27, 10);

		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_passwd_adjuncts(base, roots, clients)
	char *base;
	char *roots;
	char *clients;
{
	char path[256];
	char *result;
	char *cp;

	if (base)
		sprintf(path, "%s/etc/security/passwd.adjunct", base);
	else
		sprintf(path, "/etc/security/passwd.adjunct");

	result = add_to_list(NULL, path);
	for (cp = list_reset(clients); cp; cp = list_next(clients)) {
		path[0] = '\0';
		if (base)
			strcat(path, base);
		if (roots)
			strcat(path, roots);
		strcat(path, "/");
		strcat(path, cp);
		strcat(path, "/etc/security/passwd.adjunct");
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_groups(base, roots, clients)
	char *base;
	char *roots;
	char *clients;
{
	char path[256];
	char *result;
	char *cp;

	if (base)
		sprintf(path, "%s/etc/group", base);
	else
		sprintf(path, "/etc/group");

	result = add_to_list(NULL, path);
	for (cp = list_reset(clients); cp; cp = list_next(clients)) {
		path[0] = '\0';
		if (base)
			strcat(path, base);
		if (roots)
			strcat(path, roots);
		strcat(path, "/");
		strcat(path, cp);
		strcat(path, "/etc/group");
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_group_adjuncts(base, roots, clients)
	char *base;
	char *roots;
	char *clients;
{
	char path[256];
	char *result;
	char *cp;

	if (base)
		sprintf(path, "%s/etc/security/group.adjunct", base);
	else
		sprintf(path, "/etc/security/group.adjunct");

	result = add_to_list(NULL, path);
	for (cp = list_reset(clients); cp; cp = list_next(clients)) {
		path[0] = '\0';
		if (base)
			strcat(path, base);
		if (roots)
			strcat(path, roots);
		strcat(path, "/");
		strcat(path, cp);
		strcat(path, "/etc/security/group.adjunct");
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_audit_warns(base, execs)
	char *base;
	char *execs;
{
	char path[256];
	char *result = NULL;
	char *cp;

	for (cp = list_reset(execs); cp; cp = list_next(execs)) {
		if (base)
			sprintf(path, "%s%s/etc/audit_warn", base, cp);
		else
			sprintf(path, "%s/etc/audit_warn", cp);
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}

char *
make_dir_list(directories)
	char *directories;
{
	char path[256];
	char *result = NULL;
	char *cp;

	for (cp = list_reset(directories); cp; cp = list_next(directories)) {
		sprintf(path, "%s", cp);
		free(cp);
		result = add_to_list(result, path);
	}
	return (result);
}
