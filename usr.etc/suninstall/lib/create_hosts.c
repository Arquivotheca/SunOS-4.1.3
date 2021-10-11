/*      @(#)create_hosts.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

extern char *sprintf(), *where_are_we();

create_hosts(sys, tape_drive_type, tapehost, 
	tapehost_ip, tape_device, progname)
struct system_info *sys;
char *tape_drive_type, *tapehost, *tapehost_ip, *tape_device, *progname;
{
	register i;
	char arch[25], path[MINSIZE];
	FILE *hosts, *archlist, *extractlist;
	char filename[MAXPATHLEN];
	char buf[BUFSIZE];
	char cmd[BUFSIZE];
	char *where;

        if( (hosts = fopen(HOSTS,"w")) == NULL ) {
                (void) fprintf(stderr,
		    "%s:\tUnable to open /etc/hosts", progname);
                exit(1);
        }
        (void) fprintf(hosts,"#\n");
        (void) fprintf(hosts,"# If the NIS is running, this file is only consulted when booting\n");
        (void) fprintf(hosts,"#\n");
        (void) fprintf(hosts,"# These lines added by the Suninstall Program\n");
        (void) fprintf(hosts,"#\n");
	if ( strlen(sys->ip0) )
        	(void) fprintf(hosts,"%s\t%s loghost\n",sys->ip0,sys->hostname);
	if ( strlen(sys->ip1) ) 
		(void) fprintf(hosts,"%s\t%s\n",sys->ip1,sys->hostname1);
        (void) sprintf(filename,"%sarchlist",INSTALL_DIR); 
        if((archlist = fopen(filename,"r")) != NULL ) {
		while ( fgets(buf,BUFSIZ,archlist) != NULL ) {
			(void) sscanf(buf,"arch=%s path=%s\n",arch,path);
			if ( strlen(arch) ) {	
        			(void) sprintf(filename,"%sextractlist.%s",
					INSTALL_DIR,arch);
				if((extractlist = fopen(filename,"r")) 
					!= NULL ) {
					for (i=0;
					     fgets(buf,BUFSIZ,extractlist) != NULL; 
				             i++ ) {
                				switch (i) {
                				case 0:
                        				(void) sscanf(buf,"device=%s", tape_device);
                        				break;
                				case 1:
                        				(void) sscanf(buf,"drive=%s", tape_drive_type);
                        				break;
                				case 2:
                        				if ( !strcmp(tape_drive_type,"remote") ) {
                                				(void) sscanf(buf, "tapehost=%s\n", tapehost);
                        				}
                        				break;
                				case 3:
                        				if ( !strcmp(tape_drive_type,"remote") ) {
                                				(void) sscanf(buf,"tapehostip=%s", tapehost_ip);	
								(void) fprintf(hosts, "%s\t%s\n", tapehost_ip,tapehost);
							}
							break;
						}
					}
					(void) fclose(extractlist);
				}
			}
		}
		(void) fclose(archlist);
	}
        (void) fprintf(hosts,"\n");
	if ( strlen(sys->ip0) )
        	(void) fprintf(hosts,"127.0.0.1 localhost\n");
	else
		(void) fprintf(hosts,"127.0.0.1 %s localhost loghost\n",sys->hostname);
	(void) fprintf(hosts,"#\n");
	(void) fprintf(hosts,"# End of lines added by the Suninstall Program\n");
	(void) fprintf(hosts,"#\n");
	(void) fclose(hosts);

	where = where_are_we();

	(void) sprintf(path,"%s/etc",where);
	(void) makedirpath(path);

	(void) sprintf(cmd,"cp %s %s 2>/dev/null\n", HOSTS, path);
	(void) system(cmd);
}
