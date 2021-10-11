/*
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)create.c 1.1 92/07/30 Copyr 1987 Sun Micro"; /* c2 secure */
#endif

#include <stdio.h>

char *list_reset();
char *list_next();
char *add_to_list();
char *index();
char *rindex();

create_fs(base, local_fs, opts)
	char *base;
	char *local_fs;
	char *opts;
{
	char *cp;
	char *opt_p;
	char *blank_p;
	char path[256];
	char options[128];
	char hostname[128];

	if(opts)
	    sprintf(options, "-o %s", opts);
	else
	    *options = '\0';
	printf("#\n# create_fs\n#\tbase = %s\n#\tpath = %s\n#\n", base,
	    local_fs);

	for (cp = list_reset(local_fs); cp; cp = list_next(local_fs)) {
		if (index(cp, ' ') != 0 )
			blank_p = index(cp, ' ') + 1;
		else{
			sprintf(hostname, "%s", cp);
			blank_p = index(cp, ':') + 1;
			hostname[blank_p - cp] = NULL;
		}
		if (base) {
			sprintf(path, "%s%s", base, blank_p);
		}
		else {
			sprintf(path, "%s", blank_p);
		}
		create_path(path);

		printf("#\n# mount the file system\n#\t%s\n#\n", path);


		if (index(cp, ' ') != 0 ){
			if (base) {
				blank_p--;
				*blank_p = '\0';
				printf("mount %s %s%s\n", cp, base, blank_p + 1);
				*blank_p = ' ';
			}
			else
				printf("mount %s\n", cp);

			strcat(path, "/files");
			create_path(path);

		}else
			if (base)
				printf("mount %s %s%s %s%s\n", options, hostname, path, base, path);
			else
				printf("mount %s %s%s %s\n", options, hostname, path, path);

		free(cp);
	}
}

create_path(dir)
	char *dir;
{
	char *cp;
	static char *path_list = NULL;

	/*
	 * if (access(dir, 0) == 0)
	 *	return;
	 */


	if (list_contains(dir, path_list))
		return;

	if ((cp = rindex(dir, '/')) == NULL) {
		if (*dir == '.')
			return;
		printf("Error: bad create path\n");
		exit(1);
	}
	if (cp != dir) {
		*cp = '\0';
		create_path(dir);
		*cp = '/';
	}
	printf("if [ ! -d %s ]\nthen mkdir %s\nfi\n", dir, dir);
	path_list = add_to_list(dir, path_list);
}



create_dir(base, directories)
	char *base;
	char *directories;
{
	char *cp;
	char local_dir[256];

	for (cp=list_reset(directories); cp; cp=list_next(directories)) {
		if (base)
			sprintf(local_dir,"%s%s",base,cp);
		else sprintf(local_dir,"%s",cp);
		create_path(local_dir);
	}
}
