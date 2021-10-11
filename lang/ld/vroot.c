#ifndef lint
static        char sccsid[] = "@(#)vroot.c 1.1 92/07/30";
#endif not lint


#include <stdio.h>
#include <sys/param.h>

static	char *get_next_name();

extern	char *getenv();
extern	char *malloc();

#define NDEFDIRS 3              /* number of default directories in dirs[] */

extern	char *dirs[];
extern	int ndir;
extern	char *defaults_dir[];

/*
 * support virtual root for cross compiling
 */
dovroot()
{
	char	*vroot = getenv("VIRTUAL_ROOT");
	char	*dn;
	int	i;

	char	*vroot_copy;

	if (vroot) {
		vroot_copy = malloc(strlen(vroot) + 1);
		strcpy(vroot_copy, vroot);
 		while (dn = get_next_name(&vroot_copy))
			if (strcmp(dn, "/"))
				newvrt(dn);
	}
}

static 
newvrt(dn)
	char	*dn;
{
	char 	*cp;
	int	len = strlen(dn);
	int	i;

	for (i = 0; i < NDEFDIRS; i++) {
		dirs[ndir++] = cp = malloc(len + strlen(defaults_dir[i]) + 1);
		strcpy(cp, dn);
		strcat(cp, defaults_dir[i]);
	}
}

/*
 * Extract list of directories needed from colon separated string.
 */
static char *
get_next_name(list)
	char **list;
{
	char *lp = *list;
	char *cp = *list;;

	if (lp != NULL && *lp != '\0') {
		while (*lp != '\0' && *lp != ':')
			lp++;
		if (*lp == ':') {
			*lp = '\0';
			*list = lp + 1;
		} else 
			*list = NULL;
 	} else
 		*list = NULL;
	return(cp);
}

char *
stripvroot(path)
	char *path;
{
	char	*vroot = getenv("VIRTUAL_ROOT");
	char	*dn;
	int	i;

	char	*vroot_copy;
	static char buf[MAXPATHLEN+1];

	if (vroot) {
		vroot_copy = malloc(strlen(vroot) + 1);
		strcpy(vroot_copy, vroot);
 		while (dn = get_next_name(&vroot_copy)) {
			if(((i = strlen(dn)) != 0) && (strcmp(dn, "/") != 0)) 
				if (strncmp(dn, path, i) == 0) {
					(void)strcpy(buf, &path[i]);
					return(buf);
				}
		}
	}
	return(path);
}
