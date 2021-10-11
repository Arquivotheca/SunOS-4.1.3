/*      @(#)verify_tape.c 1.1 92/07/30 SMI                              */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "install.h"
#include "sysexits.h"

extern char *sprintf();

verify_tapevol_arch(path,arch,tape_no,tape_device,tapehost, progname)
        char *path, *arch, *tape_device, *tapehost, *progname;
        int tape_no;
{
	int id, child;
        union wait stat;
	char tapeno[5], device[25];
	char filename[MAXPATHLEN];

	(void) sprintf(tapeno,"%d",tape_no);
	(void) sprintf(device,"/dev/nr%s",tape_device);
	(void) sprintf(filename,"%sverify_tapevol_arch",path);
        if (( child = fork()) == 0) { /* child */
                execl(filename,filename,arch,tapeno,device,tapehost,0);
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
				    "%s: %s terminated with nonzero status\n",
				    progname, filename);
				exit(EX_SOFTWARE);
                        }
                }
        } else {
                perror("fork");
                exit(EX_OSERR);
        }
}
