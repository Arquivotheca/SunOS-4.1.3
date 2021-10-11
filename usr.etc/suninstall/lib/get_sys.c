/*      @(#)get_sys.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

extern char *sprintf();

int
fillin_sys_struct(sys, sys_file)
struct system_info *sys;
char *sys_file;
{
        int count=0, i, j;
	char str[MINSIZE], key[MINSIZE], *ptr;
	FILE *fd;
	char buf[BUFSIZ];
	char filename[MAXPATHLEN];

        (void) sprintf(filename,"%s%s", INSTALL_DIR, sys_file);
	if ((fd = fopen(filename, "r")) != NULL) {
		while(fgets(buf,sizeof(buf),fd) != NULL) {
			delete_blanks(buf);
			bzero(key, MINSIZE);
			for(ptr=buf,i=0;*ptr != '=';*ptr++) {
				key[i++] = *ptr;
			}
			bzero(str, MINSIZE);
			for(j=0,*ptr++;*ptr != '\0';*ptr++) {
				str[j++] = *ptr;
			}
			switch (count++) {
			case 0:
				if ( STR_EQ(key,"hostname") )
					(void) strcpy(sys->hostname,str);
				else 
					return(-1);
				break;
			case 1:
				if ( STR_EQ(key,"hostname1") )
					(void) strcpy(sys->hostname1,str);
				else 
					return(-1);
				break;
			case 2:
				if ( STR_EQ(key,"op") )
					(void) strcpy(sys->op,str);
				else 
					return(-1);
				break;
			case 3:
				if ( STR_EQ(key,"type") )
					(void) strcpy(sys->type,str);
				else 
					return(-1);
				break;
			case 4:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"server") )
						(void) strcpy(sys->server,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"timezone") )
						(void) strcpy(sys->timezone,str);
					else
						return(-1);
				}
				break;
			case 5:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"serverip") )
						(void) strcpy(sys->serverip,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"ether0") )
						(void) strcpy(sys->ether_type0,str);
					else
						return(-1);
				}
				break;
			case 6:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"execpath") )
						(void) strcpy(sys->dataless_exec, str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"ether1") )
						(void) strcpy(sys->ether_type1,str);
					else
						return(-1);
				}
				break;
			case 7:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"kvmpath") )
						(void) strcpy(sys->dataless_kvm, str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"yptype") )
						(void) strcpy(sys->yp_type,str);
					else
						return(-1);
				}
				break;
			case 8:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"ether0") )
						(void) strcpy(sys->ether_type0,str);
					else       
						return(-1);
				} else {
					if ( STR_EQ(key,"domainname") )
						(void) strcpy(sys->domainname,str);
					else
						return(-1);
				}
				break;
			case 9:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"ether1") )
						(void) strcpy(sys->ether_type1,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"ip0") )
						(void) strcpy(sys->ip0,str);
					else
						return(-1);
				}	
				break;
			case 10:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"yptype") )
						(void) strcpy(sys->yp_type,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"ip1") )
						(void) strcpy(sys->ip1,str);
					else
						return(-1);
				}
				break;
			case 11:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"domainname") )
						(void) strcpy(sys->domainname,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"arch") )
						(void) strcpy(sys->arch,str);
					else
						return(-1);
				}
				break;
			case 12:
				if ( STR_EQ(sys->type,"dataless") ) { 
					if ( STR_EQ(key,"ip0") )
						(void) strcpy(sys->ip0,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"termtype") )
						(void) strcpy(sys->termtype,str);
					else
						return(-1);
				}
				break;
			case 13:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"ip1") )
						(void) strcpy(sys->ip1,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"reboot") )
						(void) strcpy(sys->reboot,str);
					else
						return(-1);
				}
				break;
			case 14:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"arch") )
						(void) strcpy(sys->arch,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"rewind") )
						(void) strcpy(sys->rewind,str);
					else
						return(-1);
				}
				break;
			case 15:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"termtype") )
						(void) strcpy(sys->termtype,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"root") )
						(void) strcpy(sys->root,str);
					else
						return(-1);
				}
				break;
			case 16:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"reboot") )
						(void) strcpy(sys->reboot,str);
					else
						return(-1);
				} else {
					if ( STR_EQ(key,"user") )
						(void) strcpy(sys->user,str);
					else
						return(-1);
				}
				break;
			case 17:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"rewind") )
						(void) strcpy(sys->rewind,str);
					else
						return(-1);
				}
				break;
			case 18:
				if ( STR_EQ(sys->type,"dataless") ) {
					if ( STR_EQ(key,"root") )
						(void) strcpy(sys->root,str);
					else
						return(-1);
				}
			default:
				;
			}
		}
		(void) fclose(fd);
        } else return(-1);
	return (0);
}
