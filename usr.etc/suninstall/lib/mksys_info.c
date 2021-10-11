/*      @(#)mksys_info.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include "install.h"

char *sprintf();

create_sys_info(sys)
struct system_info *sys;
{
	char filename[MAXPATHLEN];
	FILE *fd;

	(void) sprintf(filename,"%ssys_info",INSTALL_DIR);
        if((fd = fopen(filename,"w")) != NULL) {
        	(void) fprintf(fd,"hostname=%s\n",sys->hostname);
        	(void) fprintf(fd,"hostname1=%s\n",sys->hostname1);
		(void) fprintf(fd,"op=%s\n",sys->op);
		(void) fprintf(fd,"type=%s\n",sys->type);
		if ( STR_EQ(sys->type,"dataless") ) {
			(void) fprintf(fd,"server=%s\n",sys->server);
			(void) fprintf(fd,"serverip=%s\n",sys->serverip);
			(void) fprintf(fd,"execpath=%s\n",sys->dataless_exec);
		} else
			(void) fprintf(fd,"timezone=%s\n",sys->timezone);
		(void) fprintf(fd,"ether0=%s\n",sys->ether_type0);
		(void) fprintf(fd,"ether1=%s\n",sys->ether_type1);
		(void) fprintf(fd,"yptype=%s\n",sys->yp_type);
		(void) fprintf(fd,"domainname=%s\n",sys->domainname);
		(void) fprintf(fd,"ip0=%s\n",sys->ip0);
		(void) fprintf(fd,"ip1=%s\n",sys->ip1);
		(void) fprintf(fd,"arch=%s\n",sys->arch);
		(void) fprintf(fd,"termtype=%s\n",sys->termtype);
		(void) fprintf(fd,"reboot=%s\n",sys->reboot);
		(void) fprintf(fd,"rewind=norewind\n");
		(void) fprintf(fd,"root=%s\n",sys->root);
		if ( !STR_EQ(sys->type,"dataless") )
			(void) fprintf(fd,"user=%s\n",sys->user);
		(void) fclose(fd);
	} else return(-1);
	return(0);
}	
