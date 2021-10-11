#ifndef lint
static	char sccsid[] = "@(#)ypserv_ancil.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

#include "ypsym.h"
#include "ypdefs.h"
USE_YPDBPATH
USE_DBM

bool onmaplist();

/*
 * This constructs a file name from a passed domain name, a passed map name,
 * and a globally known NIS data base path prefix.
 */
void
ypmkfilename(domain, map, path)
	char *domain;
	char *map;
	char *path;
{

	if ( (strlen(domain) + strlen(map) + ypdbpath_sz + 3) 
	    > (MAXNAMLEN + 1) ) {
		fprintf(stderr, "ypserv:  Map name string too long.\n");
	}

	strcpy(path, ypdbpath);
	strcat(path, "/");
	strcat(path, domain);
	strcat(path, "/");
	strcat(path, map);
}

/*
 * This checks to see whether a domain name is present at the local node as a
 *  subdirectory of ypdbpath
 */
bool
ypcheck_domain(domain)
	char *domain;
{
	DIR *dirp;
	struct direct *dp;
	char path[MAXNAMLEN + 1];
	struct stat filestat;
	bool present = FALSE;

	if ( (dirp = opendir(ypdbpath) ) == NULL) {
		return (FALSE);
	}

	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp) ) {
		strcpy(path, ypdbpath);
		strcat(path, "/");
		strcat(path, dp->d_name);

		if (stat(path, &filestat) != -1) {

			if ( (filestat.st_mode & S_IFDIR) &&
			    (strcmp(dp->d_name, ".") ) &&
			    (strcmp(dp->d_name, "..") ) ) {

				if (!strcmp(dp->d_name, domain)) {
					present = TRUE;
					break;
				}
			}
			
		} else {
			fprintf(stderr,
			"ypserv:  ypcheck_domain stat error on file %s.\n",
			     dp->d_name);
		}
	}
	
	closedir(dirp);
	return(present);
}

/*
 * This generates a list of the maps in a domain.
 */
int
yplist_maps(domain, list)
	char *domain;
	struct ypmaplist **list;
{
	DIR *dirp;
	struct direct *dp;
	char domdir[MAXNAMLEN + 1];
	char path[MAXNAMLEN + 1];
	int error;
	char *ext;
	struct ypmaplist *map;
	int namesz;

	*list = (struct ypmaplist *) NULL;

	if (!ypcheck_domain(domain) ) {
		return (YP_NODOM);
	}
	
	(void) strcpy(domdir, ypdbpath);
	(void) strcat(domdir, "/");
	(void) strcat(domdir, domain);

	if ( (dirp = opendir(domdir) ) == NULL) {
		return (YP_YPERR);
	}

	error = YP_TRUE;
	
	for (dp = readdir(dirp); error == YP_TRUE && dp != NULL;
	    dp = readdir(dirp) ) {
		/*
		 * If it''s possible that the file name is one of the two files
		 * implementing a map, remove the extension (dbm_pag or dbm_dir)
		 */
		namesz =  strlen(dp->d_name);

		if (namesz < sizeof (dbm_pag) - 1)
			continue;		/* Too Short */

		ext = &(dp->d_name[namesz - (sizeof (dbm_pag) - 1)]);

		if (strcmp (ext, dbm_pag) != 0 && strcmp (ext, dbm_dir) != 0)
			continue;		/* No dbm file extension */

		*ext = '\0';
		ypmkfilename(domain, dp->d_name, path);
		
		/*
		 * At this point, path holds the map file base name (no dbm
		 * file extension), and dp->d_name holds the map name.
		 */
		if (ypcheck_map_existence(path) &&
		    !onmaplist(dp->d_name, *list)) {

			if ((map = (struct ypmaplist *) malloc(
			    (unsigned) sizeof (struct ypmaplist)) ) == NULL) {
				error = YP_YPERR;
				break;
			}

			map->ypml_next = *list;
			*list = map;
			namesz = strlen(dp->d_name);

			if (namesz <= YPMAXMAP) {
				(void) strcpy(map->ypml_name, dp->d_name);
			} else {
				(void) strncpy(map->ypml_name, dp->d_name,
				    namesz);
				map->ypml_name[YPMAXMAP] = '\0';
			}
		}
	}
	
	closedir(dirp);
	return(error);
}
		
/*
 * This returns TRUE if map is on list, and FALSE otherwise.
 */
static bool
onmaplist(map, list)
	char *map;
	struct ypmaplist *list;
{
	struct ypmaplist *scan;

	for (scan = list; scan; scan = scan->ypml_next) {

		if (strcmp(map, scan->ypml_name) == 0) {
			return (TRUE);
		}
	}

	return (FALSE);
}
