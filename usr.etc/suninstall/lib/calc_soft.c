/*       @(#)calc_soft.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

extern char *sprintf();

calculate_software(sys, archtype, pathname, progname)
struct system_info *sys;
char *archtype, *pathname, *progname;
{
	char vol[MINSIZE], name[MINSIZE], diskused[5];
        char loadpt[MINSIZE], drive_type[MINSIZE];
	char part[10];
	char filename[MAXPATHLEN];
	char buf[BUFSIZE];
        long totalsize, sizing; 
        int no;
	FILE *extractlist;

	/*
	*	open the extractlist.archtype and start stepping
	*	through the entries
	*/
        (void) sprintf(filename,"%sextractlist.%s",INSTALL_DIR,archtype);


        if ((extractlist = fopen(filename,"r")) != NULL) {
		/*
		*	swallow the first line
		*/
                if (fgets(buf,BUFSIZ,extractlist) == NULL) {
			(void) fclose(extractlist);
			return(0);
		}
		/*
		*	get the second line for the drive= parameter
		*/
                if (fgets(buf,BUFSIZ,extractlist) == NULL) {
			(void) fclose(extractlist);
			log ("%s: unexpected EOF in %s\n",progname,filename);
			return(-1);
		}
		/*
		*	if it's a remote then swallow the
		*	next two as well
		*/
		(void) sscanf(buf,"drive=%s",drive_type);
		if (!strcmp(drive_type,"remote")) {
			(void)fgets(buf,BUFSIZ,extractlist);
			(void)fgets(buf,BUFSIZ,extractlist);
		}
	} else {
		/* else empty file - return */
		return(0);
	}

	totalsize=0;

	while (fgets(buf,BUFSIZ,extractlist) != NULL) {
		bzero(vol, sizeof(vol));
		bzero(name, sizeof(name));
		bzero(loadpt, sizeof(loadpt));
		bzero(diskused,sizeof(diskused));

		(void) sscanf(buf,
			"volume=%s filenum=%d name=%s size=%d loadpt=%s",
				vol,&no,name,&sizing,loadpt);
		/*
		*	if it's the root file then...
		*/
		if (!strcmp(name,"root")) { 
			(void) strncpy(diskused,sys->root,
				strlen(sys->root)-1);
			if (do_diskinfo(diskused,sys->root,
				sizing, progname) == -1) {
				(void) fclose(extractlist);
				return(-1);
			}

		} else if (!strcmp(sys->type,"dataless")) {
			continue;

		/*
		*	if it's the Sys file then...
		*/
		} else if (!strcmp(name,"Sys")) {
			(void) strncpy(diskused,sys->user,
				strlen(sys->user)-1);
			if (do_diskinfo(diskused,sys->user,
				sizing, progname) == -1) {
				(void) fclose(extractlist);
				return(-1);
			}
		/*
		*	if it's anything else then...
		*/
		} else {
			search_partition(sys, pathname, part);
			(void) strncpy(diskused,part,strlen(part)-1);
			totalsize += sizing;
		}
	}
	
	(void) fclose(extractlist);

	/*
	*	now see if all the software selected will fit
	*	on the given partition...
	*/
	if (totalsize && do_diskinfo(diskused,part,totalsize, progname) == -1)
		return(-1);
	
	return(0);
}
