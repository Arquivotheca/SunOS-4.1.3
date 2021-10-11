/*      @(#)get_client.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

int
fillin_client_struct(client, client_file)
struct client_info *client;
char *client_file;
{
        register int count=0, i, j;
	char str[STRSIZE], key[MINSIZE], *ptr;
	char buf[BUFSIZE];
	FILE *fd;

	if ((fd = fopen(client_file, "r")) != NULL) {
		while(fgets(buf,BUFSIZ,fd) != NULL) {
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
				if ( STR_EQ(key,"domainname") )
					(void) strcpy(client->domainname,str);
				else
					return(-1);
				break;
			case 1:
				if ( STR_EQ(key,"yptype") )
					(void) strcpy(client->yp_type,str);
				else
					return(-1);
				break;
			case 2:
				if ( STR_EQ(key,"ip") )
					(void) strcpy(client->ip,str);
				else
					return(-1);
				break;
			case 3:
				if ( STR_EQ(key,"ether") )
					(void) strcpy(client->ether,str);
				else
					return(-1);
				break;
			case 4:
				if ( STR_EQ(key,"rootpath") )
					(void) strcpy(client->root_path,str);
				else
					return(-1);
				break;
			case 5:
				if ( STR_EQ(key,"swappath") )
					(void) strcpy(client->swap_path,str);
				else
					return(-1);
				break;
			case 6:
				if ( STR_EQ(key,"execpath") )
					(void) strcpy(client->exec_path,str);
				else
					return(-1);
				break;
			case 7:
				if ( STR_EQ(key,"kvmpath") )
					(void) strcpy(client->kvm_path,str);
				else
					return(-1);
				break;
			case 8:
				if ( STR_EQ(key,"homepath") )
					(void) strcpy(client->home_path,str);
				else
					return(-1);
				break;
			case 9:
				if ( STR_EQ(key,"swapsize") )
					(void) strcpy(client->swap_size,str);
				else
					return(-1);
				break;
			}
		}
		(void) fclose(fd);
        } else return(-1);
	return(0);
}
