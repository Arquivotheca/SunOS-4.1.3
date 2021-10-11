#ifndef lint
static char sccsid[] = "@(#)fully_qualify.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <pwd.h>
#include <nse/param.h>
#include <nse/util.h>

char *
_nse_fully_qualify(file, fullfile)
	char		*file;
	char		*fullfile;
{
	char		pwd[MAXPATHLEN];
	char		*p1;
	char		*p2;
	struct passwd	*pswd;

	if (file[0] == '/') {
		strcpy(fullfile, file);
	} else if (file[0] == '~') {
		if (file[1] == '/') {
			_nse_get_user((char *) NULL, pwd);
			sprintf(fullfile, "%s%s", pwd, &file[1]);
		} else if (file[1] == '\0') {
			_nse_get_user((char *) NULL, fullfile);
		} else {
			p1 = &file[1];
			if (p2 = index(file, '/')) { 
				strncpy(pwd, p1, p2 - p1 + 1);
				if (pswd = getpwnam(pwd)) {
					sprintf(fullfile, "%s%s", pswd->pw_dir,
						p2);
				}
			} else {
				if (pswd = getpwnam(p1)) {
					strcpy(fullfile,  pswd->pw_dir);
				}
			}
		}
	} else {
		getwd(pwd);
		sprintf(fullfile, "%s/%s", pwd, file);
	}
	return(fullfile);
}
