/*      @(#)dlist_path.c 1.1 92/07/30 SMI      */
/*
 *
 *
 *
 */

#ifndef lint
static  char sccsid[] = "@(#)dlist_path.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif  lint

#include <fcntl.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include "iso_spec.h"
#include "iso_impl.h"

struct dlist * crdlist_path();
struct dlist * crsubdlist_path();

/* internal routines to manuipulate the directory tree */

/* mkdlsit_path - create a sorted list of all directories under path */
/*                will create iso file name by converting lower case
                  to upper case */
struct dlist *
mkdlist_path(fn)
char *fn;
{
struct dlist *rootdp;

	/*create the root first, fn must be a directory */
	rootdp=crdlist_path(fn, NULL); 
	/* the parent of the rootdp is the root itself */
	rootdp->pdp = rootdp;

	if (rootdp == NULL) {
		fprintf(stderr, "root directory is NULL\n");
		cleanup();
	}

	/* create the whole tree */
	crsubdlist_path(rootdp, NULL);

	return(rootdp);
}

struct dlist*
crdlist_path(fn, pdp)
char *fn;
struct dlist *pdp;		/* parent dp */
{
int	length;
struct dlist *dp;
struct ufname *fp;
struct stat stb;

	/* first check if we can "stat" */
	if (lstat(fn, &stb) < 0) {
		fprintf(stderr, "mkproto: cannot stat: %s. Ignored.\n", fn);
		return(NULL);
	}

	length=strlen(fn);
	/* allocate space */
	dp = (struct dlist *)malloc(sizeof(struct dlist)+length-1);
	/* clear all fields to zero */
	(void) bzero (dp, (sizeof (struct dlist)+length -1));
	/* initialize the various fields */
	dp->pdp =pdp;
	strcpy(dp->unixfname, fn);
	un2in(dp->unixfname, dp->isofname);
	
	dp->duid=(long)stb.st_uid;
	dp->dgid=(long)stb.st_gid;
	dp->nlink=(long)stb.st_nlink;
	dp->dmode=(long)stb.st_mode;
	dp->mtime=stb.st_mtime;

	/* allocate UNIX file info structure */
	fp = (struct ufname *) malloc(sizeof(struct ufname) +length);
	fp->fsize = stb.st_size;
	strcpy(fp->fname, fn);

	dp->ufnp = fp;

	return(dp);
}

/* 
 * crsubdlist - read the directory pointed to by dp
 */
struct dlist *
crsubdlist_path(pdp, pn)
struct dlist *pdp;
char *pn;
{
DIR 	*dirp;
char	pname[1024];
struct dirent *direntp;
struct dlist *dp; 
struct dlist *firstdp = NULL, *lastdp;

	/* don't have to do anything if dp is not a directory */
	if ((pdp -> dmode & S_IFMT) != S_IFDIR) return;

	dirp = opendir(pdp->ufnp->fname);

	if (dirp == NULL) return(NULL);
	 
	if (pn == NULL) 
		strcpy(pname, pdp->ufnp->fname);
	else {
		strcpy(pname, pn);
		strcat(pname, "/");
		strcat(pname, pdp->ufnp->fname);
	}

	/* change working directory */
	if (chdir(pname) < 0) {
		fprintf(stderr, "mkproto: ");
		perror(pn);
		cleanup();
	}
	
	while(direntp=readdir(dirp)) {
		if (!strcmp(direntp->d_name, ".") ||
		    !strcmp(direntp->d_name, "..")) continue;

		dp = crdlist_path(direntp->d_name, pdp);
		if (dp == NULL) continue;
		if (firstdp == NULL) 
			firstdp = dp;
		else 
			lastdp->dnext = dp;
		lastdp = dp;
	}
	closedir(dirp);

	if (firstdp == NULL) goto done;
	else pdp->cdp = firstdp;

	for (dp = firstdp; dp != NULL; dp = dp->dnext) {
		crsubdlist_path(dp, pname); 
	}

done:
	/* change working directory */
	if (chdir("..") < 0) {
		fprintf(stderr, "mkproto: cannot chdir ..");
		cleanup();
	}
	
}

