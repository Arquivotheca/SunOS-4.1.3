#ifndef lint
static        char *sccsid = "@(#)stripe.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <mntent.h>
#include <sys/file.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/buf.h>
#include <sun/dkio.h>
#include <sundev/mdreg.h>

#define DEBUG	1

/*
 * Data structures to support reading the /etc/stripetab file.
 */

struct stripe_dev {
	char		*dev_name;
};

struct stripe_row {
	int			stripe_width;
	struct stripe_dev	lower_device[MAX_MD_DISKS];
};

struct	stripe_table {
	struct stripe_dev	upper_device;
	int			parity_interval;		/* interlace */
	struct stripe_dev	mirror_device;
	int			active_rows;
	struct stripe_row	stripe_rows[MAX_MD_ROWS];
};

#define MAXSTRIPESTR		255


int			Aflag = 0;	/* Carry out all the striping
							 in /etc/stripetab */
int			Nflag = 0;	/* Don't carry out the operation */
int			Tflag = 0;
int			Cflag = 0;	/* Un-stripe the file system */
int			Pflag = 0;	/* Perform parity operations */
int			fso;
int			fsi;
FILE			*stripetab;
char			*stripename = "/etc/stripetab";
struct md_struct	mdinfo;
struct stripe_table	command_line_table;

struct dk_geom		g;
struct dk_map		p;
struct dk_info		inf;


/*
 * Mainline.   Crack command line arguments, and push the correct
 * levers (but you knew that, didn't you)
 */
main (argc, argv)
	int		argc;
	char		*argv[];
{
	int		i;
	int		j;
	char		*fsys;

#ifndef STANDALONE
	argc--, argv++;
	for (j = 0; argc > 0; j++) {
		if (argv[0][0] == '-') {
			for (i = 1; argv[0][i] != '\0'; i++) {
				switch (argv[0][i]) {
				case 'A':
				case 'a':
					Aflag++;
				        break;

				case 'N':
				case 'n':
					Nflag++;
					break;

				case 'C':
				case 'c':
					Cflag++;
					break;

				default:
					printf("%s: unknown flag\n", &argv[0][i]);
					usage ();
					break;
				}
			}
			argc--, argv++;
		} else
			break;
	}
	if ((Cflag)) {
		parse_command_line (argc, argv, &command_line_table);
		fsys = command_line_table.upper_device.dev_name;
		clear_mdinfo (&mdinfo);
		mdinfo.md_status = MD_CLEAR;
	} else {
		if (argc > 1) {
			parse_command_line (argc, argv, &command_line_table);
			fsys = command_line_table.upper_device.dev_name;
			table_to_mdstruct (&command_line_table, &mdinfo);
		} else {
			if (Aflag) {
				stripe_file (stripename);
				exit (0);
			} else {
				report_file (stripename);
				exit (0);
			} 
		}
	}	
	stripe_the (fsys);

}

/*
 * Report on each device mentioned in the stripetab file.
 */
report_file (name)
	char		*name;
{
	struct stripe_table	*table;
	struct stripe_table	*file_line ();
	char			*fsys;

	while ((table = file_line (name)) != (struct stripe_table *)NULL) {
		fsys = strdup(table->upper_device.dev_name);
		report (fsys);
		printf ("\n");
	}
}


/*
 * Open a stripe device, check that it's valid, and print out the
 * striping data.
 */
report (fsys)
	char		*fsys;
{
	int		status;

	open_for_stripe (fsys);
        /*
	 * Report on enstripification.
	 */
	if ((status = ioctl(fsi, MD_IOCGET, &mdinfo)) >= 0) {
	} else {
		perror ("MD_IOCGET");
		printf ("Can't perform MD_IOCGET ioctl on fd 0x%x\n", fsi);
	}

	dump_mdstruct (&mdinfo);
}

usage ()
{
        fprintf (stderr,
		     "/usr/etc/stripe [-N] rows width spec1 spec2...\n");
	fprintf (stderr,
		     "                [-C] /* unstripe */\n");
}

/*
 * Read a line from the stripetab file, and parse it.  return
 * NULL if we already read all the lines.
 */
struct stripe_table	*
file_line (name)
	char		*name;
{
	char		line[MAXSTRIPESTR];
	int		argc;
	char		**argv;

        
	if (stripetab == NULL) {
		if ((stripetab = fopen(name, "r")) == NULL) {
			printf ("stripe: can't open %s\n", name);
			exit (1);
		}
	}
	if (fgets(&line[0], MAXSTRIPESTR, stripetab) != NULL) {
		parse_line (&line[0], &argc, &argv);
		parse_command_line (argc, argv, &command_line_table);
		table_to_mdstruct (&command_line_table, &mdinfo);
		return (&command_line_table);
	}

	return ((struct stripe_table *)NULL);
}

/*
 * Read and parse all the lines from the stripetab file, and then
 * stripe each one.
 */
stripe_file (name)
	char		*name;
{
	struct stripe_table	*table;
	char			*fsys;


	while ((table = file_line (name)) != (struct stripe_table *)NULL) {
		fsys = table->upper_device.dev_name;
		stripe_the (fsys);
	}
}

/*
 * Parse one line from the stripetab file.
 */
parse_line (line, argc, argv)
	char		*line;
	int		*argc;
	char		***argv;
{
	int		local_argc = 0;
	char		**local_argv_start =
				(char **)malloc(4 * (MAX_MD_ROWS * (MAX_MD_DISKS + 1)));
	char		**local_argv = local_argv_start;
	char		*p;

	for (p = line; *p != '\0';) {
		while (*p == ' ') p++;
		*local_argv = p;
		while ((*p != ' ') && (*p != '\n')) p++;
		*p = '\0'; p++;
		local_argv++; local_argc++;
	}
	*argc = local_argc;
	*argv = local_argv_start;
}

/*
 * Is a file system currently mounted.
 */
is_mounted (fsys)
	char	*fsys;
{
	char		*slash;
	FILE		*mnttab;
	struct mntent	*mntp;
	char		bdevname[MAXPATHLEN];

       	slash = (char *)rindex(fsys, '/');
	if (slash && slash[1] == 'r') {
		sprintf(bdevname, "/dev/%s", &slash[2]);
	} else if (*fsys == 'r') {
		sprintf(bdevname, "/dev/%s", &fsys[1]);
	} else {
		strcpy(bdevname, fsys);
	}

	mnttab = setmntent(MOUNTED, "r");
	while ((mntp = getmntent(mnttab)) != NULL) {
		if (strcmp(bdevname, mntp->mnt_fsname) == 0) {
			printf("%s is mounted, can't stripe\n", bdevname);
			return(1);
		}
	}
	endmntent(mnttab);
	return (0);
	

}

/*
 * Open a special device, and check that it is capable of being
 * treated as a striped device.
 */
open_for_stripe (o_fsys)
	char		*o_fsys;
{
	int		status;
	

	fso = creat(o_fsys, 0666);
	if(fso < 0) {
		perror ("open_for_stripe: creat");
		printf("\n%s: cannot create\n", o_fsys);
		exit(1);
	}

        fsi = open(o_fsys, O_RDWR);
	if(fsi < 0) {
		perror ("open_for_stripe: open");
		printf("\n%s: cannot open\n", o_fsys);
		exit(1);
	}

	/*
	 * Check that we have a stripable file system
	 */
	{
		struct dk_info		dkinfo;

		if ((status = ioctl(fsi, DKIOCINFO, &dkinfo)) >= 0) {
			switch (dkinfo.dki_ctype) {

			case	DKC_MD:
				break;

			default:
				printf ("Bad disk type 0x%x\n", dkinfo.dki_ctype);
				exit(1);
			}
		} else {
			perror ("DKIOCINFO");
			printf ("Can't perform DKIOCINFO ioctl on fd 0x%x\n", fsi);
		}
	}
}

/*
 * Check that a file system can be opened for striping, open
 * it,  and carry oout the striping.
 */
stripe_the(s_fsys)
	char		*s_fsys;
{
	int		status;
	char		*fsys;
		
	if (!Nflag) {
		if (is_mounted (s_fsys)) {
			printf("%s is mounted, can't stripe\n", s_fsys);
			exit(1);
		} 

	}
	fsys = strdup(s_fsys);
	open_for_stripe (fsys);


	/*
	 * Carry out the enstripification.
	 */
	if ((status = ioctl(fsi, MD_IOCSET, &mdinfo)) >= 0) {
	} else {
		perror ("MD_IOCSET");
		printf ("Can't perform MD_IOCSET ioctl on fd 0x%x\n", fsi);
	}
#else
#endif
}

/*
 * Command line format.
 *
 *	stripe [-N] upper_device #rows
 *		#width lower_device lower_device lower_device
 *		#width lower_device lower_device
 *		[-m another_device ]
 */
parse_command_line (argc, argv, table)
	int			argc;
	char			*argv[];
	struct stripe_table	*table;
{
	int			rows;
	int			width;
	struct stripe_row	*row;

	
	/*
	 * Parse the upper device.
	 *
	 * stripe [-N] upper_device #rows
	 */
	table->upper_device.dev_name = argv[0];
	if (argc < 4) {
		printf ("Need <upper-device rows width lower-device>\n");
		return;
	}
	argc--; argv++;
	if ((table->active_rows = atoi(argv[0])) <= 0) {
		printf ("Must have at least one active row\n");
		exit(1);
	}
	argc--; argv++;
	/*
	 * Parse the #rows lower devices.
	 *
	 *	#width lower_device lower_device lower_device
	 *
	 * Each row must have at least two entries.
	 */
	for (rows = 0; rows < table->active_rows; rows++) {
		if (argc < ((table->active_rows - rows) * 2)) {
			printf ("%d remaining arguments not enough for %d remaining rows\n",
				argc,
				table->active_rows - rows);
			exit(1);
		}
		row = &table->stripe_rows[rows];
		if((row->stripe_width = atoi(argv[0])) <= 0) {
			printf ("Row %d must have at least one active disk\n");
			exit(1);
		}
		argc--; argv++;
		for (width = 0; width < row->stripe_width; width++) {
			row->lower_device[width].dev_name = argv[0];
			argc--; argv++;
		}
	}
	table->mirror_device.dev_name = (char *)0;
	table->parity_interval = 0;
	while (argc) {
		if ((argv[0][0] == '-') && (argv[0][1] == 'm')) {
			argc--; argv++;
			table->mirror_device.dev_name = argv[0];
			argc--; argv++;
		} else {
			if ((argv[0][0] == '-') && (argv[0][1] == 'p')) {
				argc--; argv++;
				table->parity_interval = atoi (argv[0]);
				Pflag++;
				argc--; argv++;
			} else {
				printf ("parse_command_line: bad option %s\n", argv[0]);
				exit(1);
			}
		}
	} 
	
	dump_stripe_entry (table);
}

/*
 * Convert a stripe table entry into an md_struct entry suitable
 * for use in an ioctl.
 */
table_to_mdstruct (table, mdstruct)
	struct stripe_table	*table;
	struct md_struct	*mdstruct;
{
	int			row;
	struct md_row		*mdrow;
	int			width;
	int			cum_blocks = 0;
	int			cum_data_blocks = 0;
	int			disk_size = 0;
	int			size = 0;
	struct stripe_row	*trow;

	clear_mdinfo (mdstruct);
	mdstruct->md_status = 0;
	mdstruct->md_ndisks = 0;
	mdstruct->md_rows = table->active_rows;
	if (table->mirror_device.dev_name != (char *)0) {
		printf ("table_to_mdstruct: mirror device %s\n",
			table->mirror_device.dev_name);
		mdstruct->md_mirror = device_of(table->mirror_device.dev_name);
	} else {
		printf ("table_to_mdstruct: no mirror device\n");
		mdstruct->md_mirror = 0;
	}
	mdstruct->md_parity_interval = table->parity_interval;
	printf ("table_to_mdstruct: md_parity_interval 0x%x\n",
		mdstruct->md_parity_interval);
	for (row = 0; row < mdstruct->md_rows; row++) {
		mdrow = &mdstruct->md_real_row[row];
		trow = &table->stripe_rows[row];
		mdrow->md_width = trow->stripe_width;
		mdrow->md_row_disks = trow->stripe_width;
		for (width = 0; width < mdrow->md_width; width++) {
			mdstruct->md_ndisks += 1;
			mdrow->md_real_disks[width] =
			    device_of(trow->lower_device[width].dev_name);
			size = size_of(trow->lower_device[width].dev_name);
			if (disk_size == 0) {
				disk_size = size;
			} else {
			        if (size < disk_size) {
				        disk_size = size;
				}
			}
			printf ("device %s size %d\n",
			    trow->lower_device[width].dev_name, 
			    size_of(trow->lower_device[width].dev_name));
		}
		cum_blocks += mdrow->md_width * disk_size;
		mdrow->md_blocks = mdrow->md_width * disk_size;
		mdrow->md_data_blocks =
			(Pflag != 0) ? mdrow->md_blocks - disk_size :  mdrow->md_blocks;
		cum_data_blocks += mdrow->md_data_blocks;
		mdrow->md_cum_blocks = cum_blocks;
		mdrow->md_cum_data_blocks = cum_data_blocks;
		if (Pflag != 0)
			printf ("table_to_mdstruct: parity reduces effective size from %d to %d\n",
				cum_blocks,
				cum_data_blocks);
		mdrow->md_start_block = 0;
		mdrow->md_end_block = disk_size;
	}

}

dump_mdstruct (mdstruct)
	struct md_struct	*mdstruct;
{
	int		row;
	int		width;


	printf ("rows %d total disks %d mirror 0x%x\n",
		mdstruct->md_rows,
		mdstruct->md_ndisks, 
		mdstruct->md_mirror);
	for (row = 0; row < mdstruct->md_rows; row++) {
		printf ("\trow %d width %d disks %d blocks %d cum blocks %d start block %d end block %d\n",
			row,
			mdstruct->md_real_row[row].md_width,
			mdstruct->md_real_row[row].md_row_disks, 
			mdstruct->md_real_row[row].md_blocks, 
			mdstruct->md_real_row[row].md_cum_blocks, 
			mdstruct->md_real_row[row].md_start_block, 
			mdstruct->md_real_row[row].md_end_block);
		printf ("\t\t");
		
		for (width = 0;
		    width < mdstruct->md_real_row[row].md_width; width++) {
			printf (" 0x%x ",
			    mdstruct->md_real_row[row].md_real_disks[width]);
		}
		printf ("\n");
	}
}

device_of (device_name)
	char		*device_name;
{
	struct stat	status;

	if (stat(device_name, &status) == -1) {
		perror ("device_of");
		printf ("Can't stat '%s'\n", device_name);
		exit(1);
	}

	return (status.st_rdev);
}

off_t
size_of (device_name)
	char		*device_name;
{
        struct stat	st;
        int		fd, c, found, nparts;
	char		*rawname();

        if (stat(rawname(device_name), &st) == 0) {
                fd = open(rawname(device_name), 0);
                if (fd < 0) {
                        perror(device_name);
                        return(0);
                }
                if (isdisk(fd)) {
                        if (parts(0, fd) == 0)
                                printf("Not a valid partition.\n");
                } else
                        printf("%s: Not a valid disk.\n", device_name);
                close(fd);
		if (p.dkl_nblk & 0xf) {
				return(p.dkl_nblk & ~0xf);
		}
                return(p.dkl_nblk);
        }


	return (0);
}

isdisk(fd)
{
        return (ioctl(fd, DKIOCINFO, &inf) == 0 &&
            ioctl(fd, DKIOCGGEOM, &g) == 0);
}

parts(pa, fd)
{
        int n;
 
        if (ioctl(fd, DKIOCGPART, &p) == 0) {
                return (p.dkl_nblk ? 1 : 0);
        } else {
                if (pa) printf("%c: ", pa);
                printf("can't get partition info\n");
                return (0);
        }
}


char *
rawname(cp)
        char *cp;
{
        static char rawbuf[MAXPATHLEN];
        char *dp = rindex(cp, '/');

        if (dp == 0)
                return (0);
        *dp = 0;
        (void)strcpy(rawbuf, cp);
        *dp = '/';
        (void)strcat(rawbuf, "/r");
        (void)strcat(rawbuf, dp+1);
        return (rawbuf);
}

dump_stripe_entry (table)
	struct	stripe_table	*table;
{
	int			rows;
	int			width;
	struct stripe_row	*row;


	printf ("device:\t%s = %d rows\n",
		table->upper_device.dev_name,
		table->active_rows);
	for (rows = 0; rows < table->active_rows; rows++) {
		row = &table->stripe_rows[rows];
		printf ("\trow %d width %d: ",
			rows, row->stripe_width);
		for (width = 0; width < row->stripe_width; width++) {
			printf (" %s ",
				row->lower_device[width].dev_name);
		}
		printf ("\n");
	}
}

clear_mdinfo (mdstruct)
	struct md_struct	*mdstruct;
{

	bzero (mdstruct, sizeof (* mdstruct));
}
