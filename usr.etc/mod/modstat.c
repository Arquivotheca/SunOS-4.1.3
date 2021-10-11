/*	@(#)modstat.c 1.1 92/07/30 SMI	*/
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <machine/param.h>
#include <sun/vddrv.h>
#include <stdio.h>

extern void error();

struct vdioctl_stat vdistat;
int stat_all, verbose;

/*
 * Display status of all loaded modules
 */
main(argc, argv)
	int argc;
	char **argv;
{
	struct vdstat vdstat;
	int first_mod = 1, fd;

	/*
	 * Open the pseudo-driver.
	 */
	if ((fd = open("/dev/vd", O_RDONLY, 0)) < 0) 
		error("can't open /dev/vd");

	get_args(argc, argv);

	do {
		vdistat.vdi_vdstat = &vdstat;	/* status buffer */
		vdistat.vdi_vdstatsize = sizeof (struct vdstat); /* buf size*/
		/*
	 	 * Get info about a loaded module.
	 	 */
		if (ioctl(fd, VDSTAT, (char *)&vdistat) < 0) 
	    		error("can't get module status");

		if (vdistat.vdi_vdstatsize == 0) {
			if (verbose)
				printf("no modules loaded\n");
			exit(0);	/* nothing loaded */
		}
		if (first_mod) {
			first_mod = 0;
			printf(
"Id  Type  Loadaddr      Size   B-major  C-major  Sysnum   Mod Name\n");
		}
		print_status(&vdstat);
	} while (stat_all && vdistat.vdi_id != -1);

	exit(0);
	/* NOTREACHED */
}

get_args(argc, argv)
	int argc;
	char **argv;
{
	verbose = 0;
	stat_all = 1;
	vdistat.vdi_id = -1;
		
	while (--argc != 0) {
		if (**++argv != '-')
			usage("Invalid option %s\n", *argv);

		switch ((*argv)[1]) {
		case 'i':
			if ((*argv)[2] != 'd')
				usage("Invalid option %s\n", *argv);
			if (--argc == 0)
				usage("Missing module id\n");
			++argv;
			if (sscanf(*argv, "%d", &vdistat.vdi_id) != 1)
				usage("Invalid id %s\n", *argv);
			if (vdistat.vdi_id <= 0 || vdistat.vdi_id > VD_MAXID)
				usage("Invalid id %d\n", vdistat.vdi_id);
			stat_all = 0;
			break;

		case 'v':
			verbose = 1;
			break;

		default:
			usage("Invalid option %s\n", *argv);
			break;
		}
	}
}

/*
 * Display info about a loaded module.
 */
print_status(vds)
	struct vdstat *vds;
{
	register int magic = vds->vds_magic;
	register char *type;

	printf("%2d  ", vds->vds_id);

	switch (magic) {
	case VDMAGIC_DRV:
		type = " Drv";
		break;

	case VDMAGIC_NET:
		type = " Net";
		break;

	case VDMAGIC_SYS:
		type = " Sys";
		break;

	case VDMAGIC_USER:
		type = "User";
		break;

	case VDMAGIC_PSEUDO:
		type = "Pdrv";
		break;

	case VDMAGIC_MBDRV:
		type = "Mdrv";
		break;

	case VDMAGIC_MBNET:
		type = "Mnet";
		break;

	default:
		type = "????";
		break;
	}
			
	printf("%4.4s  %8x  %8x    ", type, vds->vds_vaddr, vds->vds_size);

	switch (magic) {
	case VDMAGIC_DRV:
	case VDMAGIC_PSEUDO:
	case VDMAGIC_MBDRV:
		if (vds->vds_modinfo[0])
			printf("%3d.     ", (vds->vds_modinfo[0] & 0xff));
		else
			printf("         ");

		if (vds->vds_modinfo[1])
			printf("%3d.  ", (vds->vds_modinfo[1] & 0xff));
		else
			printf("      ");
				
		printf("           ");
		break;

	case VDMAGIC_SYS:
		printf("                  %3d     ",
			(vds->vds_modinfo[0] & 0xff));
		break;

	default:
		printf("                          ");
	}
	
	vds->vds_modname[VDMAXNAMELEN - 1] = '\0';
	printf("%s\n",vds->vds_modname);
}	

#include <varargs.h>

/* VARARGS */
usage(va_alist)
	va_dcl
{
	va_list args;
	char *fmt;

	va_start(args);
	fmt = va_arg(args, char *);
	(void)vfprintf(stderr, fmt, args);
	va_end(args);
	(void)fprintf(stderr, "Usage: modstat [-v] [-id <N>]\n");
	exit(1);
}
