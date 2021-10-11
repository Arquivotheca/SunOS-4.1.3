/*       @(#)search_part.c 1.1 92/07/30 SMI					*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

 /*
 *	Given a path name, look up in the file 
 *	/usr/etc/install/files/mountlist and
 *	find the partition that it's mounted on.  Returns
 *	the name of the partition in the pointer *part_out.
 */

#include "install.h"

extern char *sprintf(), *where_are_we();

int
search_partition(sys, path_in, part_out)
struct system_info *sys;
char *path_in;
char *part_out;
{
	char part[10], mountpt[MINSIZE], preserve[2], cap[25];
	long total, used, avail;
	FILE *fd;
	char *mount_pt;
	char filename[MAXPATHLEN];
	char buf[BUFSIZE];
	char cmd[BUFSIZE];
	char lastmatch[MAXPATHLEN];
	char lastpart[10];

	*part_out = '\0';
	bzero(lastmatch,sizeof(lastmatch));
	bzero(lastpart,sizeof(lastpart));

	mount_pt = where_are_we();

	if (MINIROOT) {
		(void) sprintf(filename,"%smountlist",INSTALL_DIR);
	} else {
		(void) sprintf(cmd,"df | grep /dev >> %sdf",INSTALL_DIR);
		(void) system(cmd);
		(void) sprintf(filename,"%sdf",INSTALL_DIR); 
	}

	if((fd = fopen(filename,"r")) == NULL) {
		(void) strcpy(part_out,sys->root);
		return;
	}

	while(fgets(buf,BUFSIZ,fd) != NULL) {

		bzero(part,sizeof(part));
		bzero(mountpt,sizeof(mountpt));

		if (MINIROOT) {
			(void) sscanf(buf,
				"partition=%s mountpt=%s preserve=%s",
				part,mountpt,preserve);
		} else {
			(void) sscanf(buf,"/dev/%s %d %d %d %s %s\n",
				part,&total,&used,&avail,cap,mountpt);
		}

		if (!strcmp(path_in,mountpt)) {
			(void) strcpy(part_out,part);
			break;
		} 

		if (!strcmp(path_in,"/"))
			continue;

		(void) strcat(mountpt,"/");
		if ( (!strncmp(path_in,mountpt,strlen(mountpt))) && 
			(strlen(mountpt) > strlen(lastmatch)) ) {
			(void) strcpy(lastmatch,mountpt);
			(void) strcpy(lastpart,part);
		}
	}

	(void) fclose(fd);
	if (!strlen(part_out) && strlen(lastpart))
		(void) strcpy(part_out,lastpart);

	if ( !strlen(part_out) ) 
		(void) strcpy(part_out,sys->root);

	return;

}

