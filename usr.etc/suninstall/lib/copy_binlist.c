/*      @(#)copy_binlist.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

/*
 *	copy sbin binaries as well as vmunix, kadb and any 
 *		custom additions
 */

char *sprintf();

copy_binlist(mount_pt, exec_path, root_path, progname)
char *mount_pt, *exec_path, *root_path, *progname;
{
	char from_bin[MAXPATHLEN], to_bin[MAXPATHLEN], dirname[MAXPATHLEN];
	char tmp[BUFSIZ];
	char line[BUFSIZ];
	FILE *fp;
	char **lp;
	extern char **getline();
	extern int getfileent();

	(void) sprintf(tmp,"%s%s%s", mount_pt, exec_path, BINLIST);
	if ((fp = fopen(tmp, "r")) != NULL) {
		while (lp = getline(line, sizeof(line), fp)) {
			if (getname(from_bin, sizeof(from_bin), 
			    " \t", " \t", lp) < 0)
				continue;
			if (getname(to_bin, sizeof(to_bin), 
			    " \t", " \t", lp) < 0)
				continue;
			(void) sprintf(dirname,"%s/%s/%s", mount_pt, 
			    root_path, to_bin);
			(void) makedirpath(dirname);
			(void) sprintf(tmp,"cp %s/%s/%s %s/%s/%s\n",
			    mount_pt, exec_path, from_bin, 
			    mount_pt, root_path, to_bin);
			(void) system(tmp);
		}
		(void) fclose(fp);
	} else
		(void) fprintf(stderr,"%s:\tcould not open %s\n", progname, tmp);
}
