/*      @(#)sort_mount.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"

extern char *sprintf();

sort_mountlist(progname)
char *progname;
{
        register i, j, jj;
        char str1[10], str2[MINSIZE], str3[2];
        int tmpcount, find_count();
	FILE *mountlist;
        struct tbl {
                char part[10], mountpt[MINSIZE], preserve;
                int count;
        } tbl[30];
	char filename[MAXPATHLEN];
	char buf[BUFSIZ];

        (void) sprintf(filename,"%smountlist",INSTALL_DIR);
        if ((mountlist = fopen(filename,"r")) == NULL) {
                (void) fprintf(stderr,	
		    "%s:\tUnable to open %s in sort_mountlist\n",
		    progname, filename);
                return(-1);
        }
        for (i=0; fgets(buf,BUFSIZ,mountlist) != NULL ;i++) {
		bzero(str1,sizeof(str1));
		bzero(str2,sizeof(str2));
		bzero(str3,sizeof(str3));
                tbl[i].preserve = ' ';
                tbl[i].count = 0;
                (void) sscanf(buf,"partition=%s mountpt=%s preserve=%s\n",
                                        str1,str2,str3);
                (void) strcpy(tbl[i].part,str1);
                (void) strcpy(tbl[i].mountpt,str2);
                tbl[i].preserve = str3[0];
                tbl[i].count = find_count(tbl[i].mountpt);
        }
        (void) fclose(mountlist);
        for (j = 0;j < i; j++) {
                for (jj = j+1; jj < i; jj++) {
                        if ( tbl[j].count > tbl[jj].count ) {
                                (void) strcpy(str1,tbl[j].part);
                                (void) strcpy(str2,tbl[j].mountpt);
                                str3[0] = tbl[j].preserve;
                                tmpcount = tbl[j].count;
                                (void) strcpy(tbl[j].part,tbl[jj].part);
                                (void) strcpy(tbl[j].mountpt,tbl[jj].mountpt);
                                tbl[j].preserve = tbl[jj].preserve;
                                tbl[j].count = tbl[jj].count;
                                (void) strcpy(tbl[jj].part,str1);
                                (void) strcpy(tbl[jj].mountpt,str2);
                                tbl[jj].preserve = str3[0];
                                tbl[jj].count = tmpcount;
                        }
                }
        }
        (void) sprintf(filename,"%smountlist",INSTALL_DIR);
        if ((mountlist = fopen(filename,"w")) == NULL) {
                (void) fprintf(stderr,
		    "%s:\nUnable to open %s in sort_mountlist\n",
		    progname, filename);
                return(-1);
        }
        for (j=0; j < i ;j++) {
                (void) fprintf(mountlist,
		    "partition=%s mountpt=%s preserve=%c\n", tbl[j].part,
		    tbl[j].mountpt,tbl[j].preserve);
        }
	(void) fclose(mountlist);
	return(0);
}
