#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)get_disk_info.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)get_disk_info.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 *	Name:		get_disk_info()
 *
 *	Description:	Use a menu based interface to get information from
 *		the user about a system's disks.  This program is typically
 *		called from suninstall.
 */

#include <stdio.h>
#include <string.h>
#include "disk_impl.h"


/*
 *	External functions:
 */
extern char *		sprintf();



/*
 *	Local functions:
 */
static	void		init_disp_buf();
static	void		make_disk_list();




/*
 *	Global variables:
 */
#ifdef SunB1
int		old_mac_state;			/* old mac-exempt state */
#endif /* SunB1 */
char *		progname;			/* name for error mesgs */




main(argc,argv)
        int argc;
        char **argv;
{
	disk_info	disk;			/* disk information */
	disk_disp	disp_buf;		/* disk display buffer */
	form *		form_p;			/* ptr to DISK form */
	sys_info	sys;			/* system information */


#ifdef lint
	argc = argc;
#endif lint

	(void) umask(UMASK);			/* set file creation mask */

	progname = basename(argv[0]);		/* get name for error mesgs */

	set_menu_log(LOGFILE);
						/* get system name */
	if (read_sys_info(SYS_INFO, &sys) != 1) {
		menu_log("%s: Error in %s.", progname, SYS_INFO);
		menu_abort(1);
	}

	get_terminal(sys.termtype);		/* get terminal type */

	make_disk_list();			/* make list of disks */

	init_disp_buf(&disk, &disp_buf);

	form_p = create_disk_form(&disk, &disp_buf, &sys);

	/*
	 *	Ignore the 4.0 "repaint"
	 */
	(void) signal(SIGQUIT, SIG_IGN);

	if (use_form(form_p) != 1)		/* use DISK form */
		menu_abort(1);

	(void) signal(SIGQUIT, SIG_DFL);

	end_menu();				/* done with menu system */

	exit(0);
	/*NOTREACHED*/
} /* end main() */





/*
 *	Name:		get_default_part()
 *
 *	Description:	Get a default partition table.  This is accomplished
 *		by getting the existing partition table and then modifying
 *		the values in the 'd'-'h' partitions.
 */

int
get_default_part(disk_p, sys_p)
	disk_info *	disk_p;
	sys_info *	sys_p;
{
	int		num_clients;		/* number of clients */
	int		ret_code;		/* return code */
	daddr_t		user_size;		/* size of /usr */


	ret_code = get_existing_part(disk_p);
	if (ret_code != 1)
		return(ret_code);

	user_size = bytes_to_blocks(MIN_USR, disk_p->geom_buf);


	/*
	 *	If the disk is not hot, then make sure that the
	 *	values we inherit are on an even cylinder boundary.
	 */
	if (!disk_p->is_hot) {
		map_blk('a') = rnd_blocks(map_blk('a'), disk_p->geom_buf);
		map_cyl('a') = 0;
		map_blk('b') = rnd_blocks(map_blk('b'), disk_p->geom_buf);
		map_cyl('b') = blocks_to_cyls(map_blk('a'), disk_p->geom_buf);
	}
	map_blk('d') = 0;
	map_cyl('d') = 0;

	map_blk('e') = 0;
	map_cyl('e') = 0;

	map_blk('f') = 0;
	map_cyl('f') = 0;

	map_blk('h') = 0;
	map_cyl('h') = 0;



	ret_code = map_blk('c');
	ret_code = map_blk('b');
	ret_code = map_blk('a');

	if (is_small_size(disk_p)) {
		/*
		 *	If this is a small disk, make "g" the freehog
		 *	partition.
		 */
		map_blk('h') = 0;
		map_blk('g') = map_blk('c') - map_blk('a') - map_blk('b');
		map_cyl('g') = map_cyl('b') +
			blocks_to_cyls(map_blk('b'), disk_p->geom_buf);

	} else {
		map_blk('g') = map_blk('c') - map_blk('a') - map_blk('b');

		if (map_blk('g') > user_size)
			map_blk('g') = user_size;
		
		map_cyl('g') = map_cyl('b') +
			blocks_to_cyls(map_blk('b'), disk_p->geom_buf);

		map_blk('h') = map_blk('c') - map_blk('a') - map_blk('b') -
			map_blk('g');

		if (map_blk('h') < 0)
			map_blk('h') = 0;
		
	}
		
	/*
	 *	If this is a server and there is some disk space left to
	 *	grab, then grab some space for clients' root and swap space.
	 */
	if (sys_p->sys_type == SYS_SERVER  &&  map_blk('h')) {
		/*
		 *	This calculation for num_clients assumes that each
		 *	client is aligned on a cylinder boundary.  This is
		 *	not a valid assumption, but is a conservative
		 *	approach to disk allocation.  This is only a problem
		 *	on a disk with room for N_CLIENT or fewer clients.
		 *	Not very likely for a real server.
		 */
		num_clients =
		  map_blk('h') / bytes_to_blocks(MIN_CLIENT, disk_p->geom_buf);

		if (num_clients > N_CLIENTS)
			num_clients = N_CLIENTS;

		if (num_clients > 0) {
			map_blk('d') =
		    bytes_to_blocks(num_clients * MIN_XROOT, disk_p->geom_buf);

			map_blk('e') =
		    bytes_to_blocks(num_clients * MIN_XSWAP, disk_p->geom_buf);

			map_blk('h') = map_blk('c') - map_blk('a') -
				       map_blk('b') - map_blk('d') -
				       map_blk('e') - map_blk('g');
			if (map_blk('h') < 0)
				map_blk('h') = 0;

			map_cyl('d') = map_cyl('b') +
				blocks_to_cyls(map_blk('b'), disk_p->geom_buf);

			map_cyl('e') = map_cyl('d') +
				blocks_to_cyls(map_blk('d'), disk_p->geom_buf);

			map_cyl('g') = map_cyl('e') +
				blocks_to_cyls(map_blk('e'), disk_p->geom_buf);
		}
	}

	if (map_blk('h') == 0)
		map_cyl('h') = 0;
	else
		map_cyl('h') = map_cyl('g') +
			        blocks_to_cyls(map_blk('g'), disk_p->geom_buf);

	/*
	 *	If a root partition has already been specified, and that
	 *	partition is on the disk that we are setting to defaults,
	 *	then clear the saved value.  The root partition in question
	 *	is considered to be on a disk if the disk name is a prefix
	 *	to all but the last character in the root partition name.
	 */
	if (strlen(sys_p->root) &&
	    strncmp(disk_p->disk_name, sys_p->root,
		    strlen(disk_p->disk_name)) == 0 &&
	    strlen(disk_p->disk_name) + 1 == strlen(sys_p->root)) {
		bzero(sys_p->root, sizeof(sys_p->root));
	}

	/*
	 *	If a usr partition has already been specified, and that
	 *	partition is on the disk that we are setting to defaults,
	 *	then clear the saved value.  The user partition in question
	 *	is considered to be on a disk if the disk name is a prefix
	 *	to all but the last character in the user partition name.
	 */
	if (strlen(sys_p->user) &&
	    strncmp(disk_p->disk_name, sys_p->user,
		    strlen(disk_p->disk_name)) == 0 &&
	    strlen(disk_p->disk_name) + 1 == strlen(sys_p->user)) {
		bzero(sys_p->user, sizeof(sys_p->user));
	}

	if (map_blk('a') && sys_p->op_type == OP_INSTALL &&
	    strlen(sys_p->root) == 0) {
		(void) strcpy(disk_p->partitions[0].mount_pt, "/");
		disk_p->partitions[0].preserve_flag = 'n';
	}

	if (map_blk('d') && sys_p->op_type == OP_INSTALL &&
	    sys_p->sys_type == SYS_SERVER && strlen(sys_p->root) == 0) {
		(void) strcpy(disk_p->partitions[3].mount_pt, "/export");
		disk_p->partitions[3].preserve_flag = 'n';
	}

	if (map_blk('e') && sys_p->op_type == OP_INSTALL &&
	    sys_p->sys_type == SYS_SERVER && strlen(sys_p->root) == 0) {
		(void) strcpy(disk_p->partitions[4].mount_pt, "/export/swap");
		disk_p->partitions[4].preserve_flag = 'n';
	}

	if (map_blk('g') && sys_p->op_type == OP_INSTALL &&
	    sys_p->sys_type != SYS_DATALESS && strlen(sys_p->user) == 0) {
		(void) strcpy(disk_p->partitions[6].mount_pt, "/usr");
		disk_p->partitions[6].preserve_flag = 'n';
	}

	if (map_blk('h') && sys_p->op_type == OP_INSTALL &&
	    sys_p->sys_type != SYS_DATALESS && strlen(sys_p->root) == 0) {
		(void) strcpy(disk_p->partitions[7].mount_pt, "/home");
		disk_p->partitions[7].preserve_flag = 'n';
	}

	return(1);
} /* end get_default_part() */




/*
 *	Name:		init_disp_buf()
 *
 *	Description:	Initialize the partition display buffer.
 */

static	void
init_disp_buf(disk_p, disp_p)
	disk_info *	disk_p;
	disk_disp *	disp_p;
{
	int		i;			/* index variable */


	bzero((char *) disp_p, sizeof(*disp_p));

	for (i = 0; i < NDKMAP; i++) {
		disp_p->disp_list[i].name[0] = 'a' + i;
		disp_p->disp_list[i].disk_disp_p = disp_p;
		disp_p->disp_list[i].part_p = &disk_p->partitions[i];
	}
} /* end init_disp_buf() */




/*
 *	Name:		init_disk_list()
 *
 *	Description:	Create the DISK_LIST file by determining what
 *		disks are attached to this system.  This implementation
 *		assumes that all the supported disk device files already
 *		exist in /dev.  This is a performance improvement over
 *		running MAKEDEV to make the devices at suninstall time.
 */

static void
make_disk_list()
{
	char		diskname[TINY_STR];	/* diskname name */
	int		fd;			/* scratch file descriptor */
	FILE *		fp;			/* ptr to disk_list */
	char		pathname[MAXPATHLEN];	/* pathname to disk_list */
	int		unit;			/* unit counter */


	(void)printf("Determining attached disk(s) and will take a while ...");
	(void) fflush(stdout);

	fp = fopen(DISK_LIST, "w");

	if (fp == NULL) {
		menu_log("%s: unable to open file for writing.", DISK_LIST);
		menu_abort(1);
	}

	/*
	 * XY
	 */
	for (unit = 0; unit < MAX_XY_DISKS; unit++) {
		(void) sprintf(diskname, "xy%d", unit);
		(void) sprintf(pathname, "/dev/r%sc", diskname);

		fd = open(pathname, 0);
		if (fd != -1) {
			(void) close(fd);
			(void) fprintf(fp, "%s\n", diskname);
		}
	}
	/*
	 * XD
	 */
	for (unit = 0; unit < MAX_XD_DISKS; unit++) {
		(void) sprintf(diskname, "xd%d", unit);
		(void) sprintf(pathname, "/dev/r%sc", diskname);

		fd = open(pathname, 0);
		if (fd != -1) {
			(void) close(fd);
			(void) fprintf(fp, "%s\n", diskname);
		}
	}
	/*
	 * SD
	 */
	for (unit = 0; unit < MAX_SD_DISKS; unit++) {
		(void) sprintf(diskname, "sd%d", unit);
		(void) sprintf(pathname, "/dev/r%sc", diskname);

		fd = open(pathname, 0);
		if (fd != -1) {
			(void) close(fd);
			(void) fprintf(fp, "%s\n", diskname);
		}
	}
	/*
	 * IPI
	 */
	for (unit = 0; unit < MAX_ID_DISKS; unit++) {
		(void) sprintf(diskname, "id0%x%x", unit / 8, unit % 8);
		(void) sprintf(pathname, "/dev/r%sc", diskname);

		fd = open(pathname, 0);
		if (fd != -1) {
			(void) close(fd);
			(void) fprintf(fp, "%s\n", diskname);
		}
	}

	(void) fclose(fp);
} /* end make_disk_list() */
