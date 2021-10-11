/*      @(#)check_util.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"
#include "sys/param.h"

extern char *sprintf();
extern struct system_info sys;
char dataless_execpath[MINSIZE];      /* execpath for dataless configuration */
char dataless_kvmpath[MINSIZE];      /* execpath for dataless configuration */
extern int HOST_INFO, DISK_INFO, SOFTWARE_INFO, CLIENT_INFO;

int
fillin_sys_struct(fd)
        FILE *fd;
{
        int count, i, j, ok;
	char str[MINSIZE], key[25], *ptr;
	FILE *rtusr;
	char buf[BUFSIZ];
	char filename[MAXPATHLEN];

        count = 0;
	ok = 1;
        while(fgets(buf,sizeof(buf),fd) != NULL) {
		delete_blanks();
		bzero(key,sizeof(key));
                bzero(str,sizeof(str));
                for(ptr=buf,i=0;*ptr != '=';*ptr++) {
                        key[i++] = *ptr;
                }
                for(j=0,*ptr++;*ptr != '\0';*ptr++) {
                        str[j++] = *ptr;
                }
                switch (count) {
		case 0:
			if ( !strcmp(key,"hostname") )
				(void) strcpy(sys.hostname,str);
			else 
				ok = 0;
                        count++;
                        break;
		case 1:
			if ( !strcmp(key,"op") )
				(void) strcpy(sys.op,str);
			else 
				ok = 0;
                        count++;
                        break;
                case 2:
			if ( !strcmp(key,"type") )
                        	(void) strcpy(sys.type,str);
			else 
				ok = 0;
                        count++;
                        break;
                case 3:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"server") )
					(void) strcpy(sys.server,str);
				else
					ok = 0;
			} else {
				if ( !strcmp(key,"timezone") )
                        		(void) strcpy(sys.timezone,str);
				else
					ok = 0;
			}
                        count++;
                        break;
                case 4:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"serverip") )
					(void) strcpy(sys.serverip,str);
				else
					ok = 0;
			} else {
				if ( !strcmp(key,"ether0") )
                        		(void) strcpy(sys.ether_type0,str);
				else
					ok = 0;
			}
                        count++;
                        break;
                case 5:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"execpath") )
					(void) strcpy(dataless_execpath, str);
				else
					ok = 0;
			} else {
				if ( !strcmp(key,"ether1") )
					(void) strcpy(sys.ether_type1,str);
				else
					ok = 0;
			}
                        count++;
                        break;
                case 6:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"kvmpath") )
					(void) strcpy(dataless_kvmpath, str);
				else
					ok = 0;
			} else {
				if ( !strcmp(key,"yptype") )
					(void) strcpy(sys.yp_type,str);
				else
					ok = 0;
			}
                        count++;
                        break;
                case 7:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"ether0") )
                                        (void) strcpy(sys.ether_type0,str);
                                else       
                                        ok = 0;
			} else {
				if ( !strcmp(key,"domainname") )
					(void) strcpy(sys.domainname,str);
				else
					ok = 0;
			}
                        count++;
                        break;
                case 8:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"ether1") )
                                        (void) strcpy(sys.ether_type1,str);
                                else
                                        ok = 0;
			} else {
				if ( !strcmp(key,"ip0") )
					(void) strcpy(sys.ip0,str);
				else
					ok = 0;
                        }	
                        count++;
                        break;
                case 9:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"yptype") )
                                        (void) strcpy(sys.yp_type,str);
                                else
                                        ok = 0;
			} else {
				if ( !strcmp(key,"ip1") )
					(void) strcpy(sys.ip1,str);
				else
					ok = 0;
			}
                        count++;
                        break;
		case 10:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"domainname") )
                                        (void) strcpy(sys.domainname,str);
                                else
                                        ok = 0;
			} else {
				if ( !strcmp(key,"arch") )
					(void) strcpy(sys.arch,str);
				else
					ok = 0;
			}
			count++; 
                        break;
		case 11:
			if ( !strcmp(sys.type,"dataless") ) { 
				if ( !strcmp(key,"ip0") )
                                        (void) strcpy(sys.ip0,str);
                                else
                                        ok = 0;
			} else {
				if ( !strcmp(key,"reboot") )
					(void) strcpy(sys.reboot,str);
				else
					ok = 0;
                        }
                        count++; 
                        break;
		case 12:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"ip1") )
                                        (void) strcpy(sys.ip1,str);
                                else
                                        ok = 0;
			} else {
				if ( !strcmp(key,"rewind") )
                                        (void) strcpy(sys.rewind,str);
                                else
                                        ok = 0;
                        }
			count++;
                        break;
		case 13:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"arch") )
					(void) strcpy(sys.arch,str);
				else
					ok = 0;
			}
			count++;
			break;
		case 14:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"reboot") )
                                        (void) strcpy(sys.reboot,str);
                                else
                                        ok = 0;
                        }
			count++;
                        break;
		case 15:
			if ( !strcmp(sys.type,"dataless") ) {
				if ( !strcmp(key,"rewind") )
                                        (void) strcpy(sys.rewind,str);
                                else
                                        ok = 0;
                        }
			count++;
                        break;
		default:
			;
                }
		bzero(buf,sizeof(buf));
        }
	(void) sprintf(filename,"%srootuser",INSTALL_DIR);
	if ((rtusr = fopen(filename,"r")) != NULL) {
		for (i=0;fgets(buf,sizeof(buf),rtusr) != NULL;i++) {
			switch (i) {
			case 0:
				(void) sscanf(buf,"root=%s\n",sys.root);
				break;
			case 1:
				if ( strcmp(sys.type,"dataless") )  
					(void) sscanf(buf,"user=%s\n",sys.user);
				break;
			}
		}
		(void) fclose(rtusr);
	}
	return (ok);
}

int
everything_is_ok()
{
	char arch[25], path[MINSIZE], client[MINSIZE], disk[10];
	register int ok;
	int fillin_sys_struct();
	FILE *rtusr, *clientfd, *diskfd, *fd;
	FILE *mountlist, *disklist, *archlist, *extractlist, *clientlist;
	char filename[MAXPATHLEN];
	char buf[BUFSIZ];

	/*
	 * Check data files create by get_host_info
	 */
        (void) sprintf(filename,"%ssys_info",INSTALL_DIR);
        if((fd = fopen(filename,"r")) != NULL) {
		if ( fillin_sys_struct(fd) )
			HOST_INFO = 1;
		(void) fclose(fd);
	}
	/* 
         * Check data files create by get_disk_info
         */
        (void) sprintf(filename,"%smountlist",INSTALL_DIR);
        if((mountlist = fopen(filename,"r")) != NULL) {
		(void) fclose(mountlist);
        	(void) sprintf(filename,"%sdisklist",INSTALL_DIR);
        	if((disklist = fopen(filename,"r")) != NULL) {
			(void) sprintf(filename,"%srootuser",INSTALL_DIR);
                        if((rtusr = fopen(filename,"r")) != NULL) {
                                (void) fclose(rtusr);
				bzero(buf,sizeof(buf));
                		while (fgets(buf,sizeof(buf),disklist) != NULL) {
					delete_blanks();
                        		(void) sscanf(buf,"%s",disk);
					(void) sprintf(filename,"%sdisk_info.%s",
						INSTALL_DIR,disk);
                        		if((diskfd = fopen(filename,"r")) 
						!= NULL) {
                                		DISK_INFO = 1;
                                		(void) fclose(diskfd);
                        		} else {
						DISK_INFO = 0;
						break;
					}
				}
			}
			(void) fclose(disklist);
		}
	}
	/* 
         * Check data files create by get_software_info
         */
        (void) sprintf(filename,"%sarchlist",INSTALL_DIR);
	if((archlist = fopen(filename,"r")) != NULL) {
		bzero(buf,sizeof(buf));
		while (fgets(buf, sizeof(buf) , archlist) != NULL) {
			(void) sscanf(buf,"arch=%s path=%s",arch,path);
			if ( !strlen(arch) ) {
				break;
			}
			(void) sprintf(filename,"%sextractlist.%s",INSTALL_DIR,arch);
			if((extractlist = fopen(filename,"r")) != NULL) {
				(void) fclose(extractlist);
				SOFTWARE_INFO = 1;
				if ( strcmp(sys.type,"server") )
					ok = 1;
			}
			if( !strcmp(sys.type,"server") ) {
				/*
				 * Check data files create by get_client_info
				 */
				(void) sprintf(filename,"%sclientlist.%s",
					INSTALL_DIR,arch);
                		if((clientlist = fopen(filename,"r")) != NULL) {
					ok = 1;
					CLIENT_INFO = 1;
					bzero(buf, sizeof(buf));
					while(fgets(buf,sizeof(buf),clientlist)
							!= NULL) {
						delete_blanks();
						(void) strcpy(client,buf);
                				(void) sprintf(filename,"%s%s.%s",
							INSTALL_DIR,
							client,arch);
						if((clientfd = fopen(filename,"r")) 
							== NULL) {
							ok = 0;
							CLIENT_INFO = 0;
						} else {
							(void) fclose(clientfd);
						}
						bzero(buf,sizeof(buf));
					}
					(void) fclose(clientlist);
				} else {
					/*
					 * It is ok not to have clients now.
					 */
					ok = 1;
                                        CLIENT_INFO = 0;
				}
			}
			bzero(buf,sizeof(buf));
		}
		(void) fclose(archlist);
	}
	return(ok);
}
