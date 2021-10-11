/*      @(#)extract.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"
#include "sysexits.h"

extern char *sprintf();

extract(path,arch,tape_device,fsf,bs,name,tapehost, progname)
        char *path, *arch, *name, *tape_device, *tapehost, *progname;
        int fsf, bs;
{
        int id, child;
        union wait stat;
        char file_no[5], bsize[5], device[25];
	char filename[MAXPATHLEN];

	(void) sprintf(file_no,"%d",fsf);
	(void) sprintf(bsize,"%d",bs);
	(void) sprintf(device,"/dev/nr%s",tape_device);
	(void) sprintf(filename,"%sextracting",path);
        if (( child = fork()) == 0) { /* child */
                execl(filename,filename,arch,device,file_no,bsize,name,
		    tapehost,0);
		(void) fprintf(stderr,
		    "%s: Unable to execute %s\n",progname,filename);
		exit(EX_UNAVAILABLE);
        } else if ( child != -1 ) { /* parent */
                if((id = wait(&stat)) == -1) {
                        perror("wait");
                        exit(EX_OSERR);
                } else if ( id == child ) {
                        /*
                         * check status
                         */
                        if ( stat.w_status != 0 ) {
				(void) fprintf(stderr,
				    "%s:\t%s terminated with nonzero status\n",
				    progname, filename);
				exit(EX_SOFTWARE);
			}
                }
        } else {
                perror("fork");
                exit(EX_OSERR);
        }
}
