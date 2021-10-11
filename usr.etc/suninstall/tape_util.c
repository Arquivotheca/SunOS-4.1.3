#ifndef lint
static char sccsid[] = "@(#)tape_util.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "mktp.h"
#include "install.h"

static toc_entry table[NENTRIES];
static int nentries;
extern int read_xdr_toc(), save_dependency();

int init_toc_table(tape_device,tapehost)
char *tape_device;
char *tapehost;
{
        FILE *ip;
	distribution_info dinfo;
	volume_info vinfo;
	char filename[MAXPATHLEN];
	char cmd[BUFSIZ];

	/*
         * Set up toc entry table
	 * Assume the files in tape in the sequence of
         *	    File 1 : boot 
         *	    File 2 : XDRTOC 
         */
	if ( strlen(tapehost) ) {   
		    /* remote tape host */
                (void) sprintf(cmd,"rsh %s -n mt -f /dev/nr%s rew 2>/dev/null\n",
			tapehost,tape_device); 
                (void) system(cmd);
                (void) sprintf(cmd,"rsh %s -n mt -f /dev/nr%s fsf 1 2>/dev/null\n",
			tapehost,tape_device);  
                (void) system(cmd);
        	(void) sprintf(filename,"rsh %s -n dd if=/dev/nr%s 2>/dev/null",
			tapehost,tape_device);
        	if((ip = popen(filename,"r")) == NULL) {
			log("init_toc_table: Unable to open %s.",filename);
			outtahere(1);
        	}
	} else {
		    /* local tape host */
		(void) sprintf(cmd,"mt -f /dev/nr%s rew 2>/dev/null\n",tape_device);
		(void) system(cmd);
		(void) sprintf(cmd,"mt -f /dev/nr%s fsf 1 2>/dev/null\n",tape_device);
		(void) system(cmd);
		(void) sprintf(filename,"/dev/nr%s",tape_device);
        	if((ip = fopen(filename,"r")) == NULL) {
			log("init_toc_table: Unable to open %s.",filename);
			outtahere(1);
        	}
	}
	bzero((caddr_t)&dinfo, sizeof(dinfo));
	bzero((caddr_t)&vinfo, sizeof(vinfo));
	while ( !(nentries = read_xdr_toc(ip,&dinfo,&vinfo,table,NENTRIES)) ) {
		if ( !strlen(tapehost) )
			(void) fclose(ip);
		else
			(void) pclose(ip);
		do {
			if ( strlen(tapehost) )
				(void) sprintf(cmd,
				  "rsh %s -n mt -f /dev/nr%s rew 2>/dev/null\n",
				  tapehost,tape_device);
			else
				(void) sprintf(cmd,"mt -f /dev/nr%s rew 2>/dev/null\n",
					tape_device);
		} while ( system(cmd) != 0 ) ;
		bzero(filename, sizeof(filename));
        	if ( strlen(tapehost) ) {
                	(void) sprintf(cmd,
				"rsh %s -n mt -f /dev/nr%s fsf 1 2>/dev/null\n",
                        	tapehost,tape_device);
                	(void) system(cmd);
                	(void) sprintf(filename,"rsh %s -n dd if=/dev/nr%s 2>/dev/null",
                        	tapehost,tape_device);
                	if((ip = popen(filename,"r")) == NULL) {
				log("init_toc_table: Unable to open %s.",filename);
				outtahere(1);
                	}
        	} else { 
                	(void) sprintf(cmd,"mt -f /dev/nr%s fsf 1 2>/dev/null\n",
				tape_device);
                	(void) system(cmd);
                	(void) sprintf(filename,"/dev/nr%s",tape_device);
                	if((ip = fopen(filename,"r")) == NULL) {
				log("init_toc_table: Unable to open %s.",filename);
				outtahere(1);
                	}
        	}
	}
	if ( !strlen(tapehost) )
		(void) fclose(ip);
	else
		(void) pclose(ip);
}

long module_size(modulename)
char *modulename;
{
	toc_entry *eptr;
        Information *infoptr;
	long software_size;

	/*
       	 * find the module entry in the default table
       	 */
	for(eptr=table; eptr <= table+nentries; eptr++) {
		infoptr = &eptr->Info;

/*		if ( eptr->Type == TOC || eptr->Type == IMAGE ||
		     eptr->Type == UNKNOWN )
			continue;
*/
                if ( eptr->Name && strcmp(eptr->Name,modulename) == 0 ) {
		     switch (eptr->Info.kind) {
			case AMORPHOUS:
			    software_size = infoptr->Information_u.File.size;
                            break;
	                case STANDALONE:
			    software_size = infoptr->Information_u.Standalone.size;
                            break;
			case EXECUTABLE: 
			    software_size = infoptr->Information_u.Exec.size;
			    break;
	                case INSTALLABLE: 
			    software_size = infoptr->Information_u.Install.size;
                            break;
			case INSTALL_TOOL:
			    software_size = infoptr->Information_u.Tool.size;
                            break;
		     }
		     return(software_size);
		}
	}
	return(-1);
}


write_dependency(arch)
char *arch;
{
	toc_entry *eptr;
        Information *infoptr;
	long Software_size;
	char **dependency;

	/*
       	 * find the module entry in the default table
       	 */
	for(eptr=table; eptr <= table+nentries; eptr++) {
		infoptr = &eptr->Info;
		if ( eptr->Info.kind == INSTALLABLE && 
                     infoptr->Information_u.Install.soft_depends.string_list_len ) {
			dependency = infoptr->Information_u.Install.soft_depends.string_list_val;
                        save_dependency( (int)infoptr->Information_u.Install.soft_depends.string_list_len,dependency,arch);
                }
	}
}


/*
 *  This function is the same as the one in get_software_info.c
 */
save_dependency(count,list,arch)
	int count;
	char **list, *arch;
{
	FILE *fd1, *fd;
	long size;
	int i, ISLOCAL, found, tape, file;
	char name[MINSIZE], pathname[MINSIZE];
	char filename[MAXPATHLEN];
	char buf[BUFSIZ];
	char tape_drive_type[MINSIZE];

	for ( ; count ; *list++, count--) {
		(void) sprintf(filename,"%sextractlist.%s",INSTALL_DIR,arch);
        	if((fd = fopen(filename,"r")) == NULL) { 
			(void) log("\nUnable to open %s.\n",filename);
                	outtahere(1);
        	}
		for (found=0,i=0,ISLOCAL=0;
			fgets(buf, sizeof(buf), fd) != NULL;i++) {
			if ( i == 1 ) {
                        	delete_blanks(buf);
                        	(void) sscanf(buf,"drive=%s",tape_drive_type);
                        	if ( !strcmp(tape_drive_type,"local") )
                                	ISLOCAL = 1;
                        	else
                                	ISLOCAL = 0;
                	}
                	if ( (i > 1 && ISLOCAL) || (i > 3 && !ISLOCAL)) {
				(void) sscanf(buf,
			           "volume=%d filenum=%d name=%s size=%d loadpt=%s",
				&tape,&file,name,&size,pathname);
                		if ( !strcmp(name,*list) ) {
					found = 1;
					break;
				}
                	}
        	}
		(void) fclose(fd);
		if ( !found ) {
                	(void) sprintf(filename,"%sdependlist.%s",INSTALL_DIR,arch);
                	if((fd1 = fopen(filename,"r")) != NULL) {
				while ( fgets(buf, sizeof(buf), fd1) != NULL ) {
					(void) sscanf(buf,"%s",name);
                			if ( !strcmp(name,*list) ) {
                        			found = 1;
						break;
                			}
				}
				(void) fclose(fd1);
        	 	}
		}
		    /*
		     * If the name(*list) is not found in extractlist and
		     * dependlist, add it into the dependlist.
		     */	
		if ( !found ) {
                        (void) sprintf(filename,"%sdependlist.%s",INSTALL_DIR,arch); 
                        if((fd1 = fopen(filename,"a")) == NULL) { 
				(void) log("\nUnable to open %s to write.\n",
                                        filename);
                                aborting(1); 
                        }
			(void) fprintf(fd1,"%s\n",*list);
			(void) fclose(fd1);
		}
	}
}


tape_device_online(device,device_name,progname)
char *device;
char *device_name;
char *progname;
{
	char device_path[MAXPATHLEN];
	char cmd[BUFSIZ];
	int stat;

	(void) makedevice("",device,0,progname);
	(void) sprintf(cmd,"mt -f /dev/nr%s0 rew 2>/dev/null\n",device_name);
	stat = system(cmd);
/*	printf("<%s> return %d",cmd,stat);  */
	return(!stat);
}

