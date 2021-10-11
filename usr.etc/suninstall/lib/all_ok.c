/*      @(#)all_ok.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

char *sprintf();

int
everything_is_ok(sys,host_info,disk_info,software_info,client_info)
struct system_info *sys;
int *host_info, *disk_info, *software_info, *client_info;
{
	/*
	 * Check data files create by get_host_info
	 */

	host_ok(sys, host_info);

	/* 
	 * Check data files create by get_disk_info
	 */

	disk_ok(sys, disk_info);

	/* 
	 * Check data files create by get_software_info
	 */

	software_ok(software_info );

	/* 
	 * Check data files create by get_client_info
	 */

	client_ok(sys, client_info);

	return(0);
}

host_ok(sys, host_info)
struct system_info *sys;
int *host_info;
{
	if ((strlen(sys->hostname) && !STR_EQ(sys->hostname,"noname")) &&
	    strlen(sys->op) &&
	    strlen(sys->type) && 
	    strlen(sys->arch) && 
	    (STR_EQ(sys->yp_type,"none") || !STR_EQ(sys->domainname,"noname")))
		*host_info = 1;
	else
		*host_info = 0;
}

disk_ok(sys, disk_info)
struct system_info *sys;
int *disk_info;
{
	char root_disk[10];
	char filename[MAXPATHLEN];

	(void) strncpy(root_disk, sys->root, 3);
	root_disk[3] = '\0';
	(void) sprintf(filename,"%sdisk_info.%s",INSTALL_DIR,root_disk);
	if (access(filename,0) == 0)
		*disk_info = 1;
	else
		*disk_info = 0;
}

software_ok(software_info)
int *software_info;
{
	FILE *archlist;
	char arch[25], path[MINSIZE], kvmpath[MINSIZE];
	char filename[MAXPATHLEN];
	char buf[BUFSIZ];

	*software_info = 0;	/* initial assumption */

	(void) sprintf(filename,"%sarchlist",INSTALL_DIR);
	if((archlist = fopen(filename,"r")) != NULL) {
		while (fgets(buf, sizeof(buf) , archlist) != NULL) {
			(void) sscanf(buf,"arch=%s execpath=%s kvmpath=%s",
				arch,path,kvmpath);
			if ( !strlen(arch) ) break;
			(void) sprintf(filename,"%sextractlist.%s",
			    INSTALL_DIR,arch);
			if (access(filename,0) == 0)
				*software_info = 1;
		}
		(void) fclose(archlist);
	}
}

client_ok(sys, client_info)
struct system_info *sys;
int *client_info;
{
	FILE *archlist, *clientlist;
	char arch[25], path[MINSIZE], client[MINSIZE], kvmpath[MINSIZE];
	char filename[MAXPATHLEN];
	char buf[BUFSIZ];

	*client_info = 0;

	(void) sprintf(filename,"%sarchlist",INSTALL_DIR);
	if((archlist = fopen(filename,"r")) == NULL) {
		return;
	}

	while (fgets(buf, sizeof(buf), archlist) != NULL) {
		(void) sscanf(buf,"arch=%s execpath=%s kvmpath=%s",
			arch,path,kvmpath);

		if (!strlen(arch))
			break;

		if(strcmp(sys->type,"server"))
			break;

		/*
		 * Check for data files created by get_client_info
		 */
		(void) sprintf(filename,"%sclientlist.%s",INSTALL_DIR,arch);
		if((clientlist = fopen(filename,"r")) == NULL)
			continue;

		while(fgets(buf,sizeof(buf),clientlist) != NULL) {
			sscanf(buf,"%s",client);
			(void) sprintf(filename,"%s%s.%s",INSTALL_DIR,
				client,arch);
			if(access(filename,0) == 0)
				*client_info = 1;
		}
		(void) fclose(clientlist);
	}
	(void) fclose(archlist);
}
