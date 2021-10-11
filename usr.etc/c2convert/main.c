/*
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)main.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>

char *rindex();
char *program;
char hostname[32];
char auditdir[128];
char sysstg[128];

main(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	char *base = NULL;
	char *clients = NULL;
	char *roots = NULL;
	char *devices = NULL;
	char *directories = NULL;
	char *execs = NULL;
	char *filesystems = NULL;
	char *flags = NULL;
	char *minfree = NULL;
	char *sysadmin = NULL;
	char *options = NULL;
	char *read_base();
	char *read_clients();
	char *read_roots();
	char *read_devices();
	char *read_directories();
	char *read_execs();
	char *read_filesystems();
	char *read_flags();
	char *read_minfree();
	char *read_sysadmin();
	char *list_reset();
	char *list_next();

	char *fstabs = NULL;
	char *audit_controls = NULL;
	char *local_fs = NULL;
	char *client_local_fs = NULL;
	char *remote_fs = NULL;
	char *passwds = NULL;
	char *passwd_adjuncts = NULL;
	char *groups = NULL;
	char *group_adjuncts = NULL;
	char *audit_warns = NULL;
	char *dir_list = NULL;
	char *make_fstabs();
	char *make_audit_controls();
	char *make_local_fs();
	char *make_client_local_fs();
	char *make_remote_fs();
	char *make_passwds();
	char *make_passwd_adjuncts();
	char *make_groups();
	char *make_group_adjuncts();
	char *make_audit_warns();
	char *make_dir_list();

	char *add_to_list();
	char *cp;
	char red[256];
	char path[256];
	

	printf("#! /bin/sh\n#\n#\n");
	/*
	 * Make sure the invoker is root.
	 */
	if (geteuid() != 0) {
		printf("#\n# C2 conversion must be run by root.\n#\n");
		printf("echo 'C2 conversion must be run by root.'\n");
		exit(1);
	}
	program = *argv;
	gethostname(hostname, 32);

	while (gets(red) != NULL) {
		if (strncmp(red, "base=", 5) == 0)
			/*
			 * '', '/a'
			 */
			base = read_base(red + 5);
		else if (strncmp(red, "clients=", 8) == 0)
			/*
			 * '', 'one,two,three'
			 */
			clients = read_clients(red + 8);
		else if (strncmp(red, "roots=", 6) == 0)
			/*
			 * '', '/roots', '/roots,/routes'
			 */
			roots = read_roots(red + 6);
		else if (strncmp(red, "devices=", 8) == 0)
			/*
			 * '', 'xy1d,xy1e'
			 */
			devices = read_devices(red + 8);
		else if (strncmp(red, "directories=", 12) == 0)
			/*
			 * '', '/usr/audit,/home/audit'
			 */
			directories = read_directories(red + 12);
		else if (strncmp(red, "filesystems=", 12) == 0)
			/*
			 * 'red:red,red:/audit,blue:blue,blue:blue.1'
			 */
			filesystems = read_filesystems(red + 12);
		else if (strncmp(red, "flags=", 6) == 0)
			/*
			 * 'all', 'ad,p1,p0,lo'
			 */
			flags = read_flags(red + 6);
		else if (strncmp(red, "minfree=", 8) == 0)
			/*
			 * '20'
			 */
			minfree = read_minfree(red + 8);
		else if (strncmp(red, "sysadmin=", 9) == 0)
			/*
			 * 'root', 'fred@paranoid'
			 */
			sysadmin = read_sysadmin(red + 9);
		else if (strncmp(red, "execs=", 6) == 0)
			/*
			 * '/usr', '/usr,/exec/sun2'
			 */
			execs = read_execs(red + 6);
		else if (strncmp(red, "options=", 8) == 0)
			/*
			 * 'root', 'fred@paranoid'
			 */
			options = read_sysadmin(red + 8);
	}
	if (base)
		sprintf(sysstg,"grep '##' %s/etc/passwd >/dev/null",base);
	else sprintf(sysstg,"grep '##' /etc/passwd >/dev/null");
	if (system(sysstg) == 0) {
		printf("#\n# ## in password file indicates C2 conversion previously run.\n#\n");
		printf("echo '## in password file indicates C2 conversion previously run.'\n");
		exit(1);
	}
	if (base)
		sprintf(sysstg,"grep '^audit:' %s/etc/passwd >/dev/null",base);
	else sprintf(sysstg,"grep '^audit:' /etc/passwd >/dev/null");
	if (system(sysstg) != 0) {	/* somehow, no "audit"    */
					/* account in /etc/passwd */
					/* -- add one in	  */
		printf("#\n# must add 'audit' account to password file.\n#\n");
		printf("echo 'audit:*:9:9::/etc/security/audit:/bin/csh' >/tmp/C2.$$passwd\n");
		if (base)
			printf("cat %s/etc/passwd >>/tmp/C2.$$passwd\n",base);
		else printf("cat /etc/passwd >>/tmp/C2.$$passwd\n");
		if (base)
			printf("mv %s/etc/passwd %s/etc/passwd.bak\n", base, base);
		else printf("mv /etc/passwd /etc/passwd.bak\n");
		if (base)
			printf("mv /tmp/C2.$$passwd %s/etc/passwd\n", base);
		else printf("mv /tmp/C2.$$passwd /etc/passwd\n");
	}
	/*
	 * add pseudo users 
	 */
	path[0]= '\0';
	if (base)
		strcat(path, base);
	add_pseudo_user(path,"AUyppasswdd", 28, 10);
	add_pseudo_user(path,"AUpwdauthd", 27, 10);
	/*
	 * '/a/roots/one/etc/passwd,/a/etc/passwd'
	 * '/roots/one/etc/passwd,/etc/passwd'
	 */
	passwds = make_passwds(base, roots, clients);
	passwd_adjuncts = make_passwd_adjuncts(base, roots, clients);
	groups = make_groups(base, roots, clients);
	group_adjuncts = make_group_adjuncts(base, roots, clients);
	/*
	 * '/a/roots/one/etc/fstab,/a/etc/fstab'
	 * '/roots/one/etc/fstab,/etc/fstab'
	 */
	fstabs = make_fstabs(base, roots, clients);
	/*
	 * '/roots/one/etc/security/audit_control,/etc/security/audit_control'
	 */
	audit_controls = make_audit_controls(base, roots, clients);
	/*
	 * '/dev/xy1d /etc/security/audit/hostname'
	 */
	local_fs = make_local_fs(devices);
	/*
	 * 'hostname:/etc/security/audit/hostname /etc/security/audit/hostname'
	 */
	client_local_fs = make_client_local_fs(local_fs);
	/*
	 * 'red:/etc/security/audit/red /etc/security/audit/red'
	 */
	remote_fs = make_remote_fs(filesystems);
	/*
	 * '/usr/etc/audit_warn',
	 * '/usr/etc/audit_warn,/exec/sun2/etc/audit_warn'
	 */
	audit_warns = make_audit_warns(base, execs);
	/*
	 * '/usr/audit', '/home/audit'
	 */
	dir_list = make_dir_list(directories);

	/*
	 * Make the file systems for the specified devices
	 */
	printf("#\n# create paths as necessary\n#\n");
	if (base)
		sprintf(auditdir,"%s%s",base,"/etc/security/audit");
	else sprintf(auditdir,"%s","/etc/security/audit");
	create_path(auditdir);
	create_fs(base, local_fs, options);
	create_fs(base, remote_fs, options);

	/*
	 * Make the directories for local and client auditing
	 */
	printf("#\n# create paths as necessary\n#\n");
	create_dir(base, directories);
	/*
	 * Make the file systems for the clients
	 */
	for (cp=list_reset(clients); cp; cp=list_next(clients)) {
		char *mp;
		if (base)
			sprintf(auditdir,"%s%s/%s%s",base,roots,cp,"/etc/security/audit");
		else sprintf(auditdir,"%s/%s%s",roots,cp,"/etc/security/audit");
		create_path(auditdir);
		for (mp=list_reset(local_fs); mp;
		    mp=list_next(local_fs)){
			char *server;
			char mountpoint[sizeof(auditdir) + 32];

			server = rindex(mp, '/');
			sprintf(mountpoint, "%s%s", auditdir, server);
			create_path(mountpoint);
		}
		for (mp=list_reset(remote_fs); mp;
		    mp=list_next(remote_fs)){
			char *server;
			char mountpoint[sizeof(auditdir) + 32];

			server = rindex(mp, '/');
			sprintf(mountpoint, "%s%s", auditdir, server);
			create_path(mountpoint);
		}
	}
	/*
	 * Add audit file systems to the fstab files
	 */
	update_fstab(fstabs, local_fs, client_local_fs, remote_fs, options);
	/*
	 * Add to the exports list as well
	 */
	update_export(base, local_fs, options);
	/*
	 * Add them to the audit_control files, too
	 */
	update_audit_control(audit_controls, local_fs,
	    client_local_fs, remote_fs, flags, minfree, directories);
	/*
	 * Set the proper value in audit_warns
	 */
	update_audit_warn(audit_warns, sysadmin);
	/*
	 * split the passwd file
	 */
	split_passwd(passwds, passwd_adjuncts, options);
	/*
	 * split the group file
	 */
	split_group(groups, group_adjuncts);
	/*
	 * set modes and owners
	 */
	chownmod(base, local_fs, directories);

	exit(0);
	/* NOTREACHED */
}

add_pseudo_user(path, p_user, uid, gid)
char *path;
char *p_user;
int uid, gid;
{
	char passwd[256];

	strcpy(passwd, path);
	strcat(passwd, "/etc/passwd");
        sprintf(sysstg,"grep '^%s:' %s >/dev/null",p_user,passwd);
        if (system(sysstg) != 0) {      /* No pseudo user in /etc/passwd */
                printf("#\n# must add %s account to password file %s.\n#\n", p_user, passwd);
                printf("sed -e '/^audit/a\\\n");
                printf("%s:*:%d:%d:%s pseudo user::' %s > /tmp/C2.$$passwd\n",p_user, uid, gid, p_user, passwd);
                printf("mv %s %s.AU\n", passwd, passwd);
                printf("mv /tmp/C2.$$passwd %s\n", passwd);
        }

}
