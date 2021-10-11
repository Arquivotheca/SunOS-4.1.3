
#ifndef lint
static  char sccsid[] = "@(#)main.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains the main entry point of the program and other
 * routines relating to the general flow.
 */
#include "global.h"
#include <signal.h>
#include <sys/fcntl.h>
#include <sys/dkbad.h>
#include <sys/time.h>
#include "analyze.h"
#include "menu.h"
#include "param.h"
#include "misc.h"


extern	void cmdabort(), onsusp(), onalarm();
extern	int  c_disk();
extern	struct menu_item menu_command[];
extern	char **search_path;

/*
 * This is the main entry point.
 */
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	i;
	char	**arglist;
	struct	disk_info *disk = NULL;
	struct	disk_type *type, *oldtype;
	struct	partition_info *parts;

	/*
	 * Catch ctrl-C and ctrl-Z so critical sections can be
	 * implemented.
	 */
	(void) signal(SIGINT,  cmdabort);
	(void) signal(SIGTSTP, onsusp);
	(void) signal(SIGALRM, onalarm);
	/*
	 * Decode the command line options.
	 */
	i = do_options(argc, argv);
	/*
	 * If we are to run from a command file, open it up.
	 */
	if (option_f) {
		if (freopen(option_f, "r", stdin) == NULL) {
			eprint("Unable to open command file '%s'.\n", option_f);
			fullabort();
		}
	}
	/*
	 * If we are logging, open the log file.
	 */
	if (option_l) {
		if ((log_file = fopen(option_l, "w")) == NULL) {
			eprint("Unable to open log file '%s'.\n", option_l);
			fullabort();
		}
	}
	/*
	 * Read in the data file and initialize the hardware structs.
	 */
	sup_init();
	/*
	 * If there are no disks on the command line, the search list
	 * is gotten from the data file.
	 */
	if (i < 0)
		arglist = search_path;
	/*
	 * There were disks on the command line.  They comprise the
	 * search list.
	 */
	else
		arglist = &argv[i];
	/*
	 * Perform the search for disks.
	 */
	do_search(arglist);
	/*
	 * If there was only 1 disk on the command line, mark it
	 * to be the current disk.  If it wasn't found, it's an error.
	 */
	if (i == argc - 1) {
		disk = disk_list;
		if (disk == NULL) {
			eprint("Unable to find specified disk '%s'.\n",
			    argv[i]);
			fullabort();
		}
	}
	/*
	 * A disk was forced on the command line.
	 */
	if (option_d) {
		/*
		 * Find it in the list of found disks and mark it to
		 * be the current disk.
		 */
		for (disk = disk_list; disk != NULL; disk = disk->disk_next)
			if (diskname_match(option_d, disk))
				break;
		/*
		 * If it wasn't found, it's an error.
		 */
		if (disk == NULL) {
			eprint("Unable to find specified disk '%s'.\n",
			    option_d);
			fullabort();
		}
	}
	/*
	 * A disk type was forced on the command line.
	 */
	if (option_t) {
		/*
		 * Only legal if a disk was also forced.
		 */
		if (disk == NULL) {
			eprint("Must specify disk as well as type.\n");
			fullabort();
		}
		oldtype = disk->disk_type;
		/*
		 * Find the specified type in the list of legal types
		 * for the disk.
		 */
		for (type = disk->disk_ctlr->ctlr_ctype->ctype_dlist;
		    type != NULL; type = type->dtype_next)
			if (strcmp(option_t, type->dtype_asciilabel) == 0)
				break;
		/*
		 * If it wasn't found, it's an error.
		 */
		if (type == NULL) {
			eprint("Specified type '%s' is not a known type.\n",
			    option_t);
			fullabort();
		}
		/*
		 * If the specified type is not the same as the type
		 * in the disk label, update the type and nullify the
		 * partition map.
		 */
		if (type != oldtype) {
			disk->disk_type = type;
			disk->disk_parts = NULL;
		}
	}
	/*
	 * A partition map was forced on the command line.
	 */
	if (option_p) {
		/*
		 * Only legal if both disk and type were also forced.
		 */
		if (disk == NULL || disk->disk_type == NULL) {
			eprint("Must specify disk and type as well ");
			eprint("as partitiion.\n");
			fullabort();
		}
		/*
		 * Find the specified map in the list of legal maps
		 * for the type.
		 */
		for (parts = disk->disk_type->dtype_plist; parts != NULL;
		    parts = parts->pinfo_next)
			if (strcmp(option_p, parts->pinfo_name) == 0)
				break;
		/*
		 * If it wasn't found, it's an error.
		 */
		if (parts == NULL) {
			eprint("Specified table '%s' is not a known table.\n",
			    option_p);
			fullabort();
		}
		/*
		 * Update the map.
		 */
		disk->disk_parts = parts;
	}
	/*
	 * If a disk was marked to become current, initialize the state
	 * to make it current.  If not, ask user to pick one.
	 */
	if (disk != NULL) {
		init_globals(disk);
	} else if (option_f == 0  &&  option_d == 0) {
		(void) c_disk();
	}

	/*
	 * Run the command menu.
	 */
	cur_menu = last_menu = 0;
	run_menu(menu_command, "FORMAT", "format", 1);

	/*
 	 * normal ending. Explicitly exit(0);
	 */
	exit(0);
	/* NOTREACHED */
}

/*
 * This routine notifies the SunOS kernel of the geometry and partition
 * map for the current disk.  It also tells it the drive type if that
 * is necessary for the ctlr.
 */
int
notify_unix()
{
	struct	dk_type type;
	struct	dk_geom geom;
	struct	dk_allmap map;
	int	i, status, error = 0;

	/*
	 * Zero out the ioctl structs.
	 */
	bzero((char *)&type, sizeof (struct dk_type));
	bzero((char *)&map, sizeof (struct dk_allmap));
	bzero((char *)&geom, sizeof (struct dk_geom));
	/*
	 * If this ctlr needs drive types, do an ioctl to tell it the
	 * drive type for the current disk.
	 */
	if (cur_ctype->ctype_flags & CF_450_TYPES) {
		type.dkt_drtype = cur_dtype->dtype_dr_type;
		status = ioctl(cur_file, DKIOCSTYPE, &type);
		if (status) {
			eprint("Warning: error telling SunOS drive type.\n");
			error = -1;
		}
	}
	/*
	 * Fill in the geometry info.
	 */
	geom.dkg_ncyl = ncyl;
	geom.dkg_acyl = acyl;
	geom.dkg_nhead = nhead;
	geom.dkg_nsect = nsect;
	geom.dkg_intrlv = 1;
	geom.dkg_apc = apc;
	geom.dkg_rpm = cur_dtype->dtype_rpm;
	geom.dkg_pcyl = pcyl;
	/*
	 * Do the ioctl to tell the kernel the geometry.
	 */
	status = ioctl(cur_file, DKIOCSGEOM, &geom);
	if (status) {
		eprint("Warning: error telling SunOS drive geometry.\n");
		error = -1;
	}
	/*
	 * If the current partition map isn't null, fill in the table
	 * and do the ioctl to tell the kernel about the map.
	 */
	if (cur_parts != NULL) {
		for (i = 0; i < NDKMAP; i++)
			map.dka_map[i] = cur_parts->pinfo_map[i];
		status = ioctl(cur_file, DKIOCSAPART, &map);
		if (status) {
			eprint("Warning: error telling SunOS ");
			eprint("partition table.\n");
			error = -1;
		}
	}
	/*
	 * Return status.
	 */
	return (error);
}

/*
 * This routine initializes the internal state to ready it for a new
 * current disk.  There are a zillion state variables that store information
 * on the current disk, and they must all be updated.  We also tell SunOS
 * about the disk, since it may not know if the disk wasn't labeled at boot
 * time.
 */
init_globals(disk)
	struct	disk_info *disk;
{
	char	dname[DK_DEVLEN];
	struct	bounds bounds;
	int	deflt, i, status;
	caddr_t	bad_ptr = (caddr_t)&badmap;
	struct	dk_type type;

	/*
	 * If there was an old current disk, close the file for it.
	 */
	if (cur_disk != NULL)
		(void) close(cur_file);
	/*
	 * Kill off any defect lists still lying around.
	 */
	kill_deflist(&cur_list);
	kill_deflist(&work_list);
	/*
	 * If there were any buffers, free them up.
	 */
	if (cur_buf != NULL) {
		destroy_data(cur_buf);
		cur_buf = NULL;
	}
	if (pattern_buf != NULL) {
		destroy_data(pattern_buf);
		pattern_buf = NULL;
	}
	/*
	 * Fill in the hardware struct pointers for the new disk.
	 */
	cur_disk = disk;
	cur_dtype = cur_disk->disk_type;
	cur_ctlr = cur_disk->disk_ctlr;
	cur_parts = cur_disk->disk_parts;
	cur_ctype = cur_ctlr->ctlr_ctype;
	cur_ops = cur_ctype->ctype_ops;
	cur_flags = 0;
	(void) sprintf(dname, "%s%d", cur_ctlr->ctlr_dname,
	    cur_disk->disk_unit);

	if (cur_ctype->ctype_ctype  == DKC_PANTHER) {
	    (void) sprintf(dname, "%s%03x", cur_ctlr->ctlr_dname,
		cur_disk->disk_unit);
	}

	/*
	 * Open a file for the new disk.
	 */
	if ((cur_file = open_disk(dname, O_RDWR | FNDELAY)) < 0) {
		eprint("Error: can't open selected disk '%s'.\n", dname);
		fullabort();
	}
	/*
	 * If the new disk uses bad-144, initialize the bad block table.
	 */
	if (cur_ctlr->ctlr_flags & DKI_BAD144) {
		badmap.bt_mbz = badmap.bt_csn = badmap.bt_flag = 0;
		for (i = 0; i < NDKBAD; i++) {
			badmap.bt_bad[i].bt_cyl = -1;
			badmap.bt_bad[i].bt_trksec = -1;
		}
	}
	/*
	 * If the type of the new disk is known...
	 */
	if (cur_dtype != NULL) {
		/*
		 * Initialize the physical characteristics.  For some of them,
		 * the information is not necessarily in the label.  If it
		 * isn't a Sun-supported disk, this information will not be
		 * known at this time.  It is thus asked for. However, if
		 * we are running from a command file, just use the defaults
		 * instead since asking would probably mess up the command
		 * file.
		 */
		ncyl = cur_dtype->dtype_ncyl;
		acyl = cur_dtype->dtype_acyl;
		if (cur_dtype->dtype_pcyl == 0) {
			if (option_f)
				cur_dtype->dtype_pcyl = ncyl + acyl;
			else {
				bounds.lower = deflt = ncyl + acyl;
				bounds.upper = MAX_CYLS;
				cur_dtype->dtype_pcyl = (u_short)input(FIO_INT,
				    "Need info -- Enter number of physical cylinders",
				    ':', (char *)&bounds, &deflt, DATA_INPUT);
			}
		}
		pcyl = cur_dtype->dtype_pcyl;
		nhead = cur_dtype->dtype_nhead;
		nsect = cur_dtype->dtype_nsect;
		bpt = cur_dtype->dtype_bpt;
		phead = cur_dtype->dtype_phead;
		psect = cur_dtype->dtype_psect;
		/*
		 * This is not worth asking for, since it's only use is
		 * for bounds checking on defects.
		 */
		if (bpt == 0)
			bpt = INFINITY;
		if (cur_dtype->dtype_rpm == 0) {
			if (option_f)
				cur_dtype->dtype_rpm = AVG_RPM;
			else {
				bounds.lower = MIN_RPM;
				bounds.upper = MAX_RPM;
				deflt = AVG_RPM;
				cur_dtype->dtype_rpm = (u_short)input(FIO_INT,
				    "Need info -- Enter rpm of drive", ':',
				    (char *)&bounds, &deflt, DATA_INPUT);
			}
		}
		/*
		 * The need_spefs flag is used to tell us that this disk
		 * is not a known type and the ctlr specific info must
		 * be prompted for.  We only prompt for the info that applies
		 * to this ctlr.
		 */
		if ((cur_ctype->ctype_flags & CF_SMD_DEFS) &&
		    (cur_dtype->dtype_flags & DT_NEED_SPEFS)) {
			if (option_f)
				cur_dtype->dtype_bps = AVG_BPS;
			else {
				bounds.lower = MIN_BPS;
				bounds.upper = MAX_BPS;
				deflt = AVG_BPS;
				cur_dtype->dtype_bps = input(FIO_INT,
				    "Need info -- Enter total bytes/sector",
				    ':', (char *)&bounds, &deflt, DATA_INPUT);
			}
		}
		if ((cur_ctype->ctype_flags & CF_450_TYPES) &&
		    (cur_dtype->dtype_flags & DT_NEED_SPEFS)) {
			status = ioctl(cur_file, DKIOCGTYPE, &type);
			if (status) {
				eprint("Unable to read drive configuration.\n");
				fullabort();
			}
			if (option_f)
				cur_dtype->dtype_dr_type = type.dkt_drtype;
			else {
				bounds.lower = 0;
				bounds.upper = 3;
				deflt = type.dkt_drtype;
				cur_dtype->dtype_dr_type = input(FIO_INT,
				    "Need info -- Enter drive type", ':',
				    (char *)&bounds, &deflt, DATA_INPUT);
			}
		}
		if ((cur_ctype->ctype_flags & CF_BSK_WPR) &&
		    (cur_dtype->dtype_flags & DT_NEED_SPEFS)) {
			if (option_f)
				cur_dtype->dtype_buf_sk = 2;
			else {
				bounds.lower = 0;
				bounds.upper = 100;
				deflt = 2;
				cur_dtype->dtype_buf_sk = input(FIO_INT,
				    "Need info -- Enter buffer skew", ':',
				    (char *)&bounds, &deflt, DATA_INPUT);
			}
		}
		if ((cur_ctype->ctype_flags & CF_BSK_WPR) &&
		    (cur_dtype->dtype_flags & DT_NEED_SPEFS)) {
			if (option_f)
				cur_dtype->dtype_wr_prcmp = pcyl;
			else {
				bounds.lower = 0;
				bounds.upper = deflt = pcyl;
				cur_dtype->dtype_wr_prcmp = input(FIO_INT,
				    "Need info -- Enter write precomp cylinder",
				    ':', (char *)&bounds, &deflt, DATA_INPUT);
			}
		}
		/*
		 * Mark specifics initialized.
		 */
		cur_dtype->dtype_flags &= ~DT_NEED_SPEFS;
		/*
		 * Alternates per cylinder are forced to 0 or 1, independent
		 * of what the label says.  This works because we know
		 * which ctlr we are dealing with.
		 */
		if (cur_ctype->ctype_flags & CF_APC)
			apc = 1;
		else
			apc = 0;
		/*
		 * Notify the SunOS kernel of what we have found.
		 */
		(void) notify_unix();
		/*
		 * Initialize the surface analysis info.  We always start
		 * out with scan set for the whole disk.  Note,
		 * for SCSI disks, we can only scan the data area.
		 */
		scan_lower = 0;
		scan_size = BUF_SECTS;
		if (cur_ctype->ctype_flags & CF_SCSI)
			scan_upper = datasects() - 1;
		else
			scan_upper = physsects() - 1;

		/*
		 * Allocate the buffers.
		 */
		cur_buf = zalloc(BUF_SECTS * SECSIZE);
		pattern_buf = zalloc(BUF_SECTS * SECSIZE);

		/*
		 * Tell the user which disk he selected.
		 */
		if (cur_ctype->ctype_ctype != DKC_PANTHER)
		    printf("selecting %s%d: <%s>\n",
			cur_ctlr->ctlr_dname,
			cur_disk->disk_unit,
			cur_dtype->dtype_asciilabel);
		else
		    /* new naming scheme */
		    printf("selecting %s%03x: <%s>\n",
			cur_ctlr->ctlr_dname,
			cur_disk->disk_unit,
			cur_dtype->dtype_asciilabel);

		/*
		 * If the drive is formatted...
		 */
		if ((*cur_ops->ck_format)()) {
			/*
			 * Mark it formatted and read the defect list.
			 */
			cur_flags |= DISK_FORMATTED;
			/*
			 * Read the defect list.
			 */
			read_list(&cur_list);
			/*
			 * If the disk does BAD-144, we do an ioctl to tell
			 * SunOS about the bad block table.
			 */
			if (cur_ctlr->ctlr_flags & DKI_BAD144) {
				if (ioctl(cur_file, DKIOCSBAD, &bad_ptr)) {
					eprint("Warning: error telling SunOS ");
					eprint("bad block map table.\n");
				}
			}
			printf("[disk formatted");
			if (cur_list.list != NULL) {
				printf(", defect list found]");
			} else {
				printf(", no defect list found]");
			}
		/*
		 * Drive wasn't formatted.  Tell the user in case he
		 * disagrees.
		 */
		} else {
			/*
			 * Make sure the user is serious.  Note, for SCSI disks
			 * since this is instantaneous, we will just do it and
			 * not ask for confirmation.
			 */
			status = 0;
			if (! (cur_ctype->ctype_flags & (CF_SCSI|CF_IPI))) { 
			    printf("\nReady to get manufacturer's defect list ");
			    printf("from unformatted drive.\nThis cannot be ");
			    printf("interrupted and takes a ");
				if (check("long while.\nContinue"))
					status = 1;
				else
					printf("Extracting manufacturer's defect list...");
			}
			/*
			 * Extract manufacturer's defect list.
			 */
			if ((status == 0)  &&  (cur_ops->ex_man != NULL)) {
				status = (*cur_ops->ex_man)(&cur_list);
			} else {
				status = 1;
			}
			printf("[disk unformatted");
			if (status != 0) {
				printf(", no defect list found]");
			} else {
				printf(", defect list found]");
			}
		}
	/*
	 * Disk type is not known.
	 */
	} else {
		/*
		 * Initialize physical characteristics to 0 and tell the
		 * user we don't know what type the disk is.
		 */
		ncyl = acyl = nhead = nsect = 0;
#ifdef		not
		printf("selecting %s%d: <type unknown>",
			cur_ctlr->ctlr_dname,
			cur_disk->disk_unit);
#endif		not
	}
	printf("\n");
	/*
	 * Check to see if there are any mounted file systems on the
	 * disk.  If there are, print a warning.
	 */
	if (checkmount(cur_ctlr->ctlr_dname, cur_disk->disk_unit,
	    (daddr_t)-1, (daddr_t)-1))
		eprint("Warning: Current Disk has mounted partitions.\n");
}

