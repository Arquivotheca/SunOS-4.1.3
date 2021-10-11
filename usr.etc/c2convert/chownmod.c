/*
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)chownmod.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>

char *list_reset();
char *list_next();
char *index();

chownmod(base, local_fs, directories)
char *base;
char *local_fs;
char *directories;
{
	char *fsp;
	char *xx;
	char path[256];

	printf("#\n# Set modes and owners \n#\n");
	printf("echo Set modes and owners \n");
	if (base)
		sprintf(path, "%s/etc/security\n", base);
	else
		sprintf(path, "/etc/security\n");
	printf("/usr/etc/chown root %s",path);
	printf("/bin/chmod 711 %s",path);

	if (base)
		sprintf(path, "%s/etc/security/audit\n", base);
	else
		sprintf(path, "/etc/security/audit\n");
	printf("/usr/etc/chown audit %s",path);
	printf("/bin/chmod 700 %s",path);

	for (fsp = list_reset(local_fs); fsp; fsp = list_next(local_fs)) {
		xx = index(fsp,' ')+1;		/* strip */
		if (base)
			sprintf(path, "%s%s\n", base,xx);
		else
			sprintf(path, "%s\n", xx);
		printf("/usr/etc/chown audit %s", path);
		printf("/bin/chmod 700 %s", path);
		if (base)
			sprintf(path, "%s%s%s\n", base,xx,"/files");
		else
			sprintf(path, "%s%s\n", xx, "/files");
		printf("/usr/etc/chown audit %s", path);
		printf("/bin/chmod 700 %s", path);
		}

	for (fsp = list_reset(directories); fsp; fsp = list_next(directories)) {
		if (base)
			sprintf(path, "%s%s", base,fsp);
		else
			sprintf(path, "%s", fsp);
		printf("/usr/etc/chown audit %s\n", path);
		printf("/bin/chmod 700 %s\n", path);
		}


	if (base)
		sprintf(path, "%s/etc/passwd\n", base);
	else
		sprintf(path, "/etc/passwd\n");
	printf("/usr/etc/chown root %s",path);
	printf("/bin/chmod 664 %s",path);

	if (base)
		sprintf(path, "%s/etc/group\n", base);
	else
		sprintf(path, "/etc/group\n");
	printf("/usr/etc/chown root %s",path);
	printf("/bin/chmod 664 %s",path);

	if (base)
		sprintf(path, "%s/etc/security/passwd.adjunct\n", base);
	else
		sprintf(path, "/etc/security/passwd.adjunct\n");
	printf("/usr/etc/chown root %s",path);
	printf("/bin/chmod 640 %s",path);

	if (base)
		sprintf(path, "%s/etc/security/group.adjunct\n", base);
	else
		sprintf(path, "/etc/security/group.adjunct\n");
	printf("/usr/etc/chown root %s",path);
	printf("/bin/chmod 640 %s",path);

	if (base)
		sprintf(path, "%s/etc/security/audit/audit_control\n", base);
	else
		sprintf(path, "/etc/security/audit/audit_control\n");
	printf("/usr/etc/chown audit %s",path);
	printf("/bin/chmod 640 %s",path);

	if (base)
		sprintf(path, "%s/usr/etc/audit_warn\n", base);
	else
		sprintf(path, "/usr/etc/audit_warn\n");
	printf("/usr/etc/chown root %s",path);
	printf("/bin/chmod 755 %s",path);

}
