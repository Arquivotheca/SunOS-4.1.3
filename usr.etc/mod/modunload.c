/*      @(#)modunload.c 1.1 92/07/30 SMI	*/
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <machine/param.h>

#include <sun/vddrv.h>

void error(), fatal();

char *execfile = NULL;
struct vdioctl_unload vdiunload;

/*
 * Unload a loaded module.
 */
main(argc,argv,envp)
	int argc;
	char *argv[];
	char *envp[];
{
	int child, fd;
	union wait status;

	getargs(argc, argv);	/* get arguments */

	/*
	 * Open the pseudo driver.
	 */
	if ((fd = open("/dev/vd",2)) < 0) 
		error("can't open /dev/vd");

	if (execfile) {
		child = fork();
		if (child == -1)
			error("can't fork %s", execfile);
            	else if (child == 0) 
			exec_userfile(fd, envp);
		else {
                	wait(&status);
			if (status.w_retcode != 0) {
           			printf("%s returned error %d.\n", 
					execfile, status.w_retcode);
                		exit((int)status.w_retcode);
			}
	        }
	}

	/*
	 * issue an ioctl to unload the module.
	 */	
	if (ioctl(fd, VDUNLOAD, &vdiunload) < 0)
	    if (errno == EPERM)
		fatal("You must be superuser to unload a module\n");
	    else
		error("can't unload the module");

	exit(0);	/* success */
}

/*
 * Get user arguments.
 */
getargs(argc, argv)
	int argc;
	char *argv[];
{
	char *ap;

	if (argc < 3)
		usage();

	argc--;			/* skip program name */
	argv++;

	while (argc--) {
		ap = *argv++;	/* point at next option */

		if (*ap++ == '-') {
			if (strcmp(ap,"id") == 0) {
				if (--argc < 0)
					usage();
				if (sscanf(*argv++,"%d",&vdiunload.vdi_id) != 1)
					usage();
			} else if (strcmp(ap,"exec") == 0) {
				if (--argc < 0)
					usage();
				execfile = *argv++;
			} else
				usage();
		} else {
			usage();
		}
	}
}
	
usage()
{
	fatal("usage:  modunload -id <module_id> [-exec <exec_file>]\n");
}

/*
 * exec the user file.
 */
exec_userfile(fd, envp)
	int fd;
	char **envp;
{
	struct vdioctl_stat vdistat;
	struct vdstat vdstat;

	char id[8];
	char magic[16];
	char blockmajor[8];
	char charmajor[8];
	char sysnum[8];

	vdistat.vdi_id = vdiunload.vdi_id; /* copy module id */
	vdistat.vdi_vdstat = &vdstat;	/* status buffer */
	vdistat.vdi_vdstatsize = sizeof(struct vdstat);  /* buf size */

	if (ioctl(fd, VDSTAT, &vdistat) < 0)
		error("can't get module status");

        sprintf(id, "%d", vdstat.vds_id);
	sprintf(magic, "%x", vdstat.vds_magic);

	if (vdstat.vds_magic == VDMAGIC_DRV ||
	    vdstat.vds_magic == VDMAGIC_MBDRV ||
	    vdstat.vds_magic == VDMAGIC_PSEUDO) {
		sprintf(blockmajor, "%d", vdstat.vds_modinfo[0] & 0xff);
		sprintf(charmajor, "%d", vdstat.vds_modinfo[1] & 0xff);
		execle(execfile, execfile, id, magic, blockmajor,
			charmajor, (char *)0, envp); 
	} else if (vdstat.vds_magic == VDMAGIC_SYS) {   
		sprintf(sysnum, "%d", vdstat.vds_modinfo[0] & 0xff);
		execle(execfile, execfile, id, magic, sysnum, (char *)0, envp);
	} else  
		execle(execfile, execfile, id, magic, (char *)0, envp);

	error("couldn't exec %s\n", execfile);
}
