#ifndef lint
static char sccsid[] = "@(#)unpack_tape_util.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */


#include "install.h"
#include "unpack.h"
#include "media.h"

extern struct dk_geom disk_geom;
extern struct dk_allmap dk;
extern int first_cd_rom;
extern media_attr_init();

int
init_media_info(soft_p)
	soft_info   *soft_p;
{
	int	i;
	int	stat;
	char	*cp;
	char	*dev;
	char	cmd[LARGE_STR];

	soft_p->media_loc = LOC_LOCAL;
	soft_p->media_vol = 1;
	soft_p->media_host[0] = '\0';
	soft_p->media_ip[0] = '\0';

	/*
	 * Since the release is now CD-ROM only we look at the CD-ROM first
	 * and then proceed to check the list of the rest of the devices.
	 */
	media_attr_init();
	i = first_cd_rom ? first_cd_rom : 1;
	for (; (cp = cv_media_to_str(&i)) != NULL; i++) {
		/* setup type flags */
		(void) media_set_soft(soft_p, i);
		/* probe for it's existance */
		switch (soft_p->media_type) {
		case MEDIAT_TAPE:
			dev = (char *) strdup(cp);
			(void) MAKEDEV("", dev);
			/*
			 *      check for xt/mt name conflict
			 */
			if (!strcmp(dev,"xt0"))
				(void)strcpy(dev,"mt0");
			/*
			 *	hack since 3E box doesn't do autodensity
			 */
			if (!strcmp(dev,"st0"))
				(void)strcpy(dev,"st8");
			(void) sprintf(soft_p->media_path, "/dev/nr%s", dev);
			(void) sprintf(cmd,
			    "mt -f /dev/nr%s rew 2>/dev/null\n", dev);
			stat = system(cmd);
			break;

		case MEDIAT_FLOPPY:
		case MEDIAT_CD_ROM:
			/* no need to MAKEDEV for floppy -or- CD_ROM */
			(void) sprintf(soft_p->media_path, "/dev/r%s", cp);
			/* see if media is present in the drive */
			(void) sprintf(cmd,
			    "eject -q %s 1>/dev/null 2>/dev/null\n", cp);
			if (system(cmd) != 0) {
				stat = 1;
				break;
			}
			/* now, is it readable? */
			(void) sprintf(cmd,
		"dd if=/dev/r%s of=/dev/null bs=1b count=1 2>/dev/null\n",
			    cp);
			stat = system(cmd);
			break;

		default:
			/* tell user we don't know about this one */
			menu_log("%s: unknown media type %d for \"%s\".",
			    progname, soft_p->media_type, cp);
			stat = 1;
			break;
		}
		/*
		 * now that we've looked at the CD-ROM lets continue
		 * with the rest of the devices
		 */
		if ( i == first_cd_rom) {
			first_cd_rom = 0;
			i = 0;
		}
		if (stat)
			continue;
		/* found a good one, it stays setup in the soft_info struct */
		(void) cv_str_to_media(cp, &soft_p->media_dev);
		return (0);
	}
	/* no luck */
	return (-1);
}

/*
 *	Name:		verify_media()
 *
 *	Description:	Verify that media file we want is loaded in the
 *		media device.
 */

int
verify_and_load_toc(soft_p, sys_p)
	soft_info *	soft_p;
	sys_info *	sys_p;	
{
	int		ret_code;		/* return code */
	Os_ident	os_tape, os_machine;

	ret_code = read_file(soft_p, 1, XDRTOC);
	if (ret_code != 1)
		return(ret_code);

 	ret_code = toc_xlat(sys_p, XDRTOC, soft_p);

	if (ret_code != 1)  {
		free_media_file(soft_p);
		return(ret_code);
	}

	/*
	 *	Check the architecture and reload the toc with root module
	 */
	if (fill_os_ident(&os_tape, soft_p->arch_str) > 0 &&
	    fill_os_ident(&os_machine, sys_p->arch_str) >= 0 &&
	    same_arch_pair(&os_tape, &os_machine))  {
		(void) strcpy(sys_p->arch_str, soft_p->arch_str);
	 	ret_code = toc_xlat(sys_p, XDRTOC, soft_p);
		if (ret_code != 1)  {
			free_media_file(soft_p);
			return(ret_code);
		}
		return(1);
	    }
	return(0);
} /* end verify_and_load_toc() */


int
select_module(soft_p, name, loadpt)
	soft_info * soft_p;
	char *	    name;
	char *	    loadpt;
{
	media_file *	mf_p;			/* media file pointer */

	for (mf_p = soft_p->media_files; mf_p &&
	     mf_p < &soft_p->media_files[soft_p->media_count]; mf_p++) {
		if (strcmp(mf_p->mf_name, name) == 0)	{
			mf_p->mf_select = ANS_YES;
			(void) strcpy(mf_p->mf_loadpt, loadpt);
			return(1);
		}
	     }
	return(0);
}


void
reset_select_flag(soft_p)
	soft_info * soft_p;
{
	media_file *	mf_p;			/* media file pointer */

	for (mf_p = soft_p->media_files; mf_p &&
	     mf_p < &soft_p->media_files[soft_p->media_count]; mf_p++) {
			mf_p->mf_select = ANS_NO;
	     }
}


/* add up all the software modules except root in size of bytes
 * assume all of them are extracted to /usr
 */
long 
add_module_size(soft_p)
	soft_info * soft_p;
{
	long size;
	media_file *	mf_p;			/* media file pointer */

	for (size = 0, mf_p = soft_p->media_files; mf_p &&
	     mf_p < &soft_p->media_files[soft_p->media_count]; mf_p++) {
		if (mf_p->mf_select == ANS_YES)
			size += mf_p->mf_size;
	}

	return(size);    
}

int
create_disk_files(preserve, disk_name, slack, soft_p)
	char *		preserve;
	char *		disk_name;
	int		slack;
	soft_info * 	soft_p;

{
	disk_info 	disk;
	disk_info *	disk_p;
	int		ret_code;		/* return code */
	int		g_cyls;
	int		nsects_per_cyl;
	long		modulesize, nbytes_per_cyl;
	long		max_size;
	int		i;
	char		pathname[MAXPATHLEN];	/* scratch pathname */
	char		cmd[MAXPATHLEN * 2];	/* command buffer */

	disk_p = &disk;
	(void) strcpy(disk_p->disk_name, disk_name);

	ret_code = get_disk_config(disk_p);
	if (ret_code != 1)
		return(ret_code);

	ret_code = get_existing_part(disk_p);
	if (ret_code != 1)
		return(ret_code);

	if (strcmp(preserve,PRESERVE_ALL) != 0)	{
	    nsects_per_cyl = disk_geom.dkg_nhead * disk_geom.dkg_nsect;
	    nbytes_per_cyl = nsects_per_cyl * BLOCK_SIZE;
	    modulesize = FUDGE_SIZE(add_module_size(soft_p) * (1 + slack/100.0));
	    modulesize = (modulesize < MIN_USR) ? MIN_USR : modulesize;
	    map_blk('d') = 0;
	    map_cyl('d') = 0;
	    map_blk('e') = 0;
	    map_cyl('e') = 0;
	    map_blk('f') = 0;
	    map_cyl('f') = 0;

	    if (is_small_size(disk_p) ) {
		   /*
		    * If this is a small disk then all of the extra space goes into 
		    * the 'g' partition
		    */
		    map_cyl('g') = B_NBLOCKS / nsects_per_cyl + B_START_CYL;
		    map_blk('g') = C_NBLOCKS - A_NBLOCKS - B_NBLOCKS;
		    map_blk('h') = 0;
	    }
	    else {
		    map_cyl('g') = B_NBLOCKS / nsects_per_cyl + B_START_CYL;
		    /* g_cyls = number of cylinders of partition g needed */
		    if (modulesize % nbytes_per_cyl)    
			g_cyls = modulesize / nbytes_per_cyl + 1;
		    else
			g_cyls = modulesize / nbytes_per_cyl;

		    /* g_nblocks = number of secters need for partion g */
		    map_blk('g') = g_cyls * nsects_per_cyl;
		    /* h_cyl = starting cylinder of partition h */
		    map_cyl('h') = g_cyls + map_cyl('g');
		    /* h_nblocks = number of secters need for partion h */
		    map_blk('h') = C_NBLOCKS - A_NBLOCKS - B_NBLOCKS - map_blk('g');
		    if (map_blk('h') < 0) {
			map_blk('h') = 0;
			map_cyl('h') = 0;
		        map_blk('g') = C_NBLOCKS - A_NBLOCKS - B_NBLOCKS;
		    }
		    else
		        /* consistence checking */
		        if (map_blk('h')/nsects_per_cyl+map_cyl('h') != C_NBLOCKS/nsects_per_cyl)
			    menu_log("create_disk_info : inconsistence found in disk label");

	    }
	    disk_p->partitions[0].preserve_flag = 'n';
	    disk_p->partitions[6].preserve_flag = 'n';
	    disk_p->partitions[7].preserve_flag = 'n';
	    disk_p->label_source = DKL_DEFAULT;
	}
	else	{
	    disk_p->partitions[0].preserve_flag = 'n';
	    disk_p->partitions[6].preserve_flag = 'n';
	    disk_p->partitions[7].preserve_flag = 'y';
	    disk_p->label_source = DKL_EXISTING;
	}
	(void) strcpy(disk_p->partitions[0].mount_pt, "/");
	(void) strcpy(disk_p->partitions[6].mount_pt, "/usr");
	if (is_small_size(disk_p)) {
		disk_p->free_hog = 'g';
	}
	else {
		(void) strcpy(disk_p->partitions[7].mount_pt, "/home");
		disk_p->free_hog = 'h';
	}
	disk_p->display_unit = DU_MBYTES;
	for (i = 0; i < NDKMAP; i++) {
		(void) strcpy(disk_p->partitions[i].size_str,
			    blocks_to_str(disk_p, map_blk(i + 'a')));
		(void) sprintf(disk_p->partitions[i].start_str, "%ld",
			    map_cyl(i + 'a'));
		(void) sprintf(disk_p->partitions[i].block_str, "%ld",
			    map_blk(i + 'a'));
		disk_p->partitions[i].avail_bytes =
			    blocks_to_bytes(map_blk(i + 'a'));
	}

	(void) sprintf(pathname, "%s.%s", DISK_INFO, disk_p->disk_name);

	ret_code = save_disk_info(pathname, disk_p);
	if (ret_code != 1) {
		menu_mesg("%s: cannot save disk information.", pathname);
		return(ret_code);
	}

	(void) sprintf(cmd, "cp %s %s.original 2>> %s", pathname, pathname,
		       LOGFILE);
	x_system(cmd);

	(void) sprintf(pathname, "%s.%s", CMDFILE, disk_p->disk_name);
	ret_code = save_cmdfile(pathname, disk_p);
	if (ret_code != 1) {
		menu_mesg("%s: cannot save cmdfile.", pathname);
		return(ret_code);
	}

	return(1);	
}
