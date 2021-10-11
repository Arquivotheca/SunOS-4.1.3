/*      @(#)main.c 1.1 92/07/30 SMI      */
/*
 * mkproto: create a CD-ROM image files 
 *	usage: mkproto [-f] proto cdimage effdate expdate
 *	       mkproto [-f] path  cdimage effdate expdate
 */

#ifndef lint
static  char sccsid[] = "@(#)main.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif  lint

#include <fcntl.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include "label.h"
#include "iso_spec.h"
#include "iso_impl.h"

int     force = 0;
int     prototype = 0;
int	extension = 0;
int	bootflag = 0;

char v[ISO_SECTOR_SIZE];                /* standard volume struct descriptor */
char u[ISO_SECTOR_SIZE];                /* standard volume unix descriptor */
char w[ISO_SECTOR_SIZE];                /* Boot Record */
char bootsys_id[ISO_SYS_ID_STRLEN] = "Sun Microsystems, Inc.";
char bootprog_id[ISO_SYS_ID_STRLEN] = "Test Boot";
char sys_id[ISO_SYS_ID_STRLEN];
char vol_id[ISO_VOL_ID_STRLEN];
char vol_set_id[ISO_VOL_SET_ID_STRLEN];
char pub_id[ISO_PUB_ID_STRLEN];
char prep_id[ISO_PREP_ID_STRLEN];
char appl_id[ISO_APPL_ID_STRLEN];
char copyr_id[ISO_COPYR_ID_STRLEN];
char abstr_id[ISO_ABSTR_ID_STRLEN];
char cre_date[ISO_DATE_LEN+1] = "1989121213000000";
char mod_date[ISO_DATE_LEN];
char exp_date[ISO_DATE_LEN];
char eff_date[ISO_DATE_LEN];
long	unix_voldesc_sec;

int cdout;
int set_size = 1;        /*default: 1 - no multiple volume support */
int set_seq = 1;         /*default: 1 - no multiple volume support */
int blk_size = ISO_SECTOR_SIZE; /*default: same as sector size */
int nlbn_per_sec = 1;           /*default: 1 lbn per secotr */
int npart = 1;			/* no. of partitions; must be 1 to 8*/
char disk_label[512];		/* Sun standard disk label */

unsigned int cd_label();

main(argc, argv)
	int argc;
	char **argv;
{
	struct stat stb;
	int secno, lbn;
	struct dlist *rootdp;
	char *cp;
	int bsize;
	struct cd_map map;
	int total_size;

	argc--, argv++;

	if (argc < 2) usage();
	while (argc > 0 && argv[0][0] == '-') {
		for (cp = &argv[0][1]; *cp; cp++)
			switch (*cp) {
			case 'f':
				force++;
				break;
			case 'b':
				if (argc < 1)
					fatal(" -b missing block size\n");
				argc--, argv++;
				bsize=atoi(*argv);
				if ((bsize == 512) || (bsize == 1024) ||
					(bsize == 2048) ) {
					blk_size = bsize;
					nlbn_per_sec=howmany(ISO_SECTOR_SIZE,
						blk_size);
				}
				else 
					fprintf(stderr, "mkproto: %s: bad blocks size ignored\n", *argv);
				break;
			case 'B':
				bootflag++;
				if (argc < 1) break;
				if (argv[1][0] == '-') break;
				if ((argv[1][0] < '0') || (argv[1][0] > '9')) 
					break;
				argc--, argv++;
				npart=atoi(*argv);
				if ((npart < 1) || (npart > 7))
					fatal(" -B no. of partition must be 1 to 8\n");
				npart++; /* add the iso partition */
				break;
			case 'E':
				extension++;
				break;
			case '?':
			case 'h':
				help();
			default:
				usage();
			}
		argc--, argv++;
	}

	if (argc != 2) usage();

	/* first check if there is a prototype file */
	/* argv[0] must be a valid file/directory name */
	if (stat(argv[0], &stb) < 0) {
		fprintf(stderr, "mkproto: ");	
		perror(argv[0]);
		usage();
	}

	switch (stb.st_mode&S_IFMT) {
		case (S_IFDIR):
			break;
		case (S_IFREG):
			prototype++;
			break;
		default:
			usage();
	}

	if (force) {
		/* overwrite the output file */
		cdout = open(argv[1], O_RDWR+O_EXCL);

		if (cdout < 0) {
			fprintf(stderr, "mkproto: cannot open ");
			perror(argv[1]);
			exit(1);
		}

		/* truncate the output file if it exist and is a regular file */
		if (fstat(cdout, &stb) < 0) {
			fprintf(stderr, "mkproto: stat failed ");	
			perror(argv[1]);
			usage();
		}
		if ((stb.st_mode & S_IFMT) == S_IFREG) {
			if (ftruncate(cdout, (off_t) 0) < 0) {
				fprintf(stderr, "mkproto: truncate failed ");	
				perror(argv[1]);
				usage();
			}
		}
	}
	else {
		/* output file must not exist */
		cdout = open(argv[1], O_CREAT+O_RDWR+O_EXCL,
			stb.st_mode & 0666); 
		
		if (cdout < 0) {
			fprintf(stderr, "mkproto: cannot open ");
			perror(argv[1]);
			exit(1);
		}
	}


	/* create a hierarchical tree consisting of all files to be copied 
	   to the cdrom image file */
	/* do it first, if there is an error, we can back out without
	   wasting any effort (cleanup will be called if there is an error */
	rootdp = mkdlist(argv[0], prototype, extension);

	/* write 32K bytes of zero to first 16 2K sector */
	(void) clrboot();

	/* create the primary volume descriptor image */
	secno = mkpvd();

	/* create the operating system specific descriptor for UNIX */
	if (extension)
		secno = mkuvd(secno);

	/* create the volume descriptor set terminator */
	secno = mktermdes(secno);

	/* craete the path table */
	lbn = mkpath(rootdp, SEC_TO_LBN(secno), extension);

	/* copy data to cd image file according to directory list*/
	lbn = mkdata(rootdp, lbn, extension);

	/* update primary volume descriptor for volume space size */
	(void) update_pvd(rootdp, lbn);

	printf("mkproto: ISO Volume Space is %d logical blocks of block size %d bytes\n",
		lbn, blk_size);

	/* update unix volume descriptor for volume space size */
	if (extension)
		(void) update_uvd(rootdp, lbn);

	/* update the path table in pvd with extlbns */
	(void) update_pvd_ptbl(rootdp);

	/* update the path table in unvd with extlbns */
	if (extension)
		(void) update_uvd_ptbl(rootdp);

	/* create the standard Sun Disk Label and reserve
	   disk space for boot partitions, if necessary */
	if (bootflag) {
        	/* create standard sun disk label */
		map.cdm_isosize = LBN_TO_BYTE(lbn)/512;
		map.cdm_npart = npart - 1;
		total_size = cd_putlabel(&map, disk_label);
	
        	/* reserve (and zero) out space for all partitions */
		makespace(LBN_TO_SEC(lbn), BYTE_TO_SEC(total_size*512));

		/* write out the standard disk label */
		(void) putdisk(disk_label, 0, 512);
	}
	else
		total_size = LBN_TO_BYTE(lbn)/512;
	close(cdout);

        /* printout the size of the cdrom image file */
	printf("mkproto: total cdrom image file size is %dK bytes\n", total_size/2);

	exit(0);
}

msg()
{
	(void) fprintf(stderr,
	    "Usage: mkproto [-f] [-B npart] proto cdimage [effdate] [expdate]\n");
	(void) fprintf(stderr,
	    "       mkproto [-f] [-B npart] path cdimage [effdate] [expdate]\n");
}

usage()
{
	msg();
	exit(1);
}

help()
{
	msg();
	
	(void) fprintf(stderr,
		"mkproto: program to create CD-ROM image file\n");
	(void) fprintf(stderr,
		"output cdrom file can also be a block device\n");

	(void) fprintf(stderr,
		"where: -f option overwrite existing files (or raw partition)\n"); 
	(void) fprintf(stderr,
		"       -B option reserves and zeroes out 10M disk space\n");
	(void) fprintf(stderr,
		"                 for each npart partition\n");

	exit(1);
}


/* mkpvd - create the primary volume descriptor */
mkpvd()
{
int ptbl_lbn;
int ptbl_size;
int lbn;

	(void) bzero(v, ISO_SECTOR_SIZE); 
	*ISO_desc_type(v) = (u_char) ISO_VD_PVD;
	(void) strncpy(ISO_std_id(v), ISO_ID_STRING, ISO_ID_STRLEN);
	*ISO_std_ver(v) = (u_char) ISO_ID_VER;
	strncpy(ISO_sys_id(v), sys_id, ISO_SYS_ID_STRLEN);
	strncpy(ISO_vol_id(v), vol_id, ISO_VOL_ID_STRLEN);
	(void) both_short(ISO_set_size(v), (short) set_size);
	(void) both_short(ISO_set_seq(v), (short) set_seq);
	(void) both_short(ISO_blk_size(v), (short) blk_size);
	strncpy(ISO_vol_set_id(v), vol_set_id, ISO_VOL_SET_ID_STRLEN);
	strncpy(ISO_pub_id(v), pub_id, ISO_PUB_ID_STRLEN);
	strncpy(ISO_prep_id(v), prep_id, ISO_PREP_ID_STRLEN);
	strncpy(ISO_appl_id(v), appl_id, ISO_APPL_ID_STRLEN);
	strncpy(ISO_copyr_id(v), copyr_id, ISO_COPYR_ID_STRLEN);
	strncpy(ISO_abstr_id(v), abstr_id, ISO_ABSTR_ID_STRLEN);
	strncpy(ISO_cre_date(v), cre_date, ISO_DATE_LEN);
	strncpy(ISO_mod_date(v), mod_date, ISO_DATE_LEN);
	strncpy(ISO_exp_date(v), exp_date, ISO_DATE_LEN);
	strncpy(ISO_eff_date(v), eff_date, ISO_DATE_LEN);
	*ISO_file_struct_ver(v) = ISO_FILE_STRUCT_ID_VER;

	/* write the descriptor to disk */
	(void) PUTSECTOR(v, ISO_VOLDESC_SEC, 1);

	return(ISO_VOLDESC_SEC+1);
}

/* clrboot - write 32Kb of zero in the beginning of the image file */
#define BOOTSIZE ISO_VOLDESC_SEC * ISO_SECTOR_SIZE
clrboot()
{
int buf[BOOTSIZE/4];

        (void) bzero(buf, BOOTSIZE);
        (void) PUTSECTOR(buf, ISO_SYSAREA_SEC, ISO_VOLDESC_SEC);
}

/* mkuvd - create the unix volume descriptor */
mkuvd(secno)
int secno;
{
int ptbl_lbn;
int ptbl_size;
int lbn;

        (void) bzero(u, ISO_SECTOR_SIZE);
        *ISO_desc_type(u) = (u_char) ISO_VD_UNIX; 
        (void) strncpy(ISO_std_id(u), ISO_ID_STRING, ISO_ID_STRLEN);
        *ISO_std_ver(u) = (u_char) ISO_ID_VER;
        strncpy(ISO_sys_id(u), sys_id, ISO_SYS_ID_STRLEN);
        strncpy(ISO_vol_id(u), vol_id, ISO_VOL_ID_STRLEN);
        (void) both_short(ISO_set_size(u), (short) set_size);
        (void) both_short(ISO_set_seq(u), (short) set_seq);
        (void) both_short(ISO_blk_size(u), (short) blk_size);
        strncpy(ISO_vol_set_id(u), vol_set_id, ISO_VOL_SET_ID_STRLEN);
        strncpy(ISO_pub_id(u), pub_id, ISO_PUB_ID_STRLEN);
        strncpy(ISO_prep_id(u), prep_id, ISO_PREP_ID_STRLEN);
        strncpy(ISO_appl_id(u), appl_id, ISO_APPL_ID_STRLEN);
        strncpy(ISO_copyr_id(u), copyr_id, ISO_COPYR_ID_STRLEN);
        strncpy(ISO_abstr_id(u), abstr_id, ISO_ABSTR_ID_STRLEN);
        strncpy(ISO_cre_date(u), cre_date, ISO_DATE_LEN);
        strncpy(ISO_mod_date(u), mod_date, ISO_DATE_LEN);
        strncpy(ISO_exp_date(u), exp_date, ISO_DATE_LEN);
        strncpy(ISO_eff_date(u), eff_date, ISO_DATE_LEN);
        *ISO_file_struct_ver(u) = ISO_FILE_STRUCT_ID_VER;
 
	/* fill in UNIX extension stuff */
	ISO_UNIX_FEATURE(u) = ISO_UNIX_FEATURE_SYMLNK+ 
			      ISO_UNIX_FEATURE_LONGFN;
        strncpy(ISO_UNIX_signature(u), ISO_UNIX_ID_STRING, ISO_UNIX_ID_STRLEN);
	

        /* write the descriptor to disk */
	unix_voldesc_sec=secno;
        (void) PUTSECTOR(u, unix_voldesc_sec, 1);
 
        return(unix_voldesc_sec+1);

}

/* mktermdes - create the volume descriptor set terminator */
mktermdes(secno)
int secno;
{
int i;
int t[ISO_SECTOR_SIZE/4];	

	(void) bzero(t, ISO_SECTOR_SIZE); 
	*ISO_desc_type(t) = (u_char) ISO_VD_EOV;
	(void) strncpy(ISO_std_id(t), ISO_ID_STRING, ISO_ID_STRLEN);
	*ISO_std_ver(t) = (u_char) ISO_ID_VER;

	/* write the descriptor to disk */
	(void) PUTSECTOR(t, secno, 1);

	return(secno+1);
}

/* update the primary volume descriptor for
   volume space size and a copy of the root directory */
void 
update_pvd(rootdp, lbn)
struct dlist *rootdp;
int lbn;
{
char buf[ISO_SECTOR_SIZE];


	/* update the volume space size */
	both_int(ISO_vol_size(v), lbn);

	/* update the dotdot's directory entry */
	GETLBN(buf, rootdp->idextlbn, nlbn_per_sec);
	bcopy(buf, ISO_root_dir(v), IDE_ROOT_DIR_REC_SIZE);

	/* write the descriptor to disk */
	(void) PUTSECTOR(v, ISO_VOLDESC_SEC, 1);
}

/* update the unix volume descriptor for
   volume space size and a copy of the root directory */
void
update_uvd(rootdp, lbn)
struct dlist *rootdp;
int lbn;
{
char buf[ISO_SECTOR_SIZE];
 
        /* update the volume space size */
        both_int(ISO_vol_size(u), lbn);
 
        /* update the dotdot's directory entry */
        GETLBN(buf, rootdp->udextlbn, nlbn_per_sec);
        bcopy(buf, ISO_UNIX_root_dir(u), IDE_UNIX_ROOT_DIR_REC_SIZE);
 
        /* write the descriptor to disk */
        (void) PUTSECTOR(u, unix_voldesc_sec, 1);

}
 
void
update_pvd_ptbl(rootdp)
struct dlist *rootdp;
{
struct dlist *dp;
char *buf;
int nlbn;
char *pep;
int     n;
 
        /* handle the small endian first */
        /* read the path table into buffer */
        nlbn= howmany((int) ISO_PTBL_SIZE(v), ISO_SECTOR_SIZE);
        buf = (char *) malloc(nlbn*ISO_SECTOR_SIZE);
        GETLBN(buf, (int) ISO_PTBL_MAN_LS(v), nlbn);
 
        /* handle the small endian first */
        for (dp=rootdp;dp != NULL;dp=dp->idirnext) {
                pep=(char *) buf + dp->ipoffset;
                (void)lsb_int(IPE_ext_lbn(pep), dp->idextlbn);
        }
 
        (void) PUTLBN(buf,(int) ISO_PTBL_MAN_LS(v), nlbn);
 
        GETLBN(buf, (int) ISO_PTBL_MAN_MS(v), nlbn);
        /* handle the big endian next */
        for (dp=rootdp;dp != NULL;dp=dp->idirnext) {
                pep=(char *) buf + dp->ipoffset;
                (void) msb_int (IPE_ext_lbn(pep), dp->idextlbn);
        }
        (void) PUTLBN(buf,(int) ISO_PTBL_MAN_MS(v), nlbn);
 
        /* free back buffer space */
        (void) cfree(buf);
 
}
 

void
update_uvd_ptbl(rootdp)
struct dlist *rootdp;
{
struct dlist *dp;
char *buf;
int nlbn;
char *pep;


        /* handle the small endian first */
        /* read the path table into buffer */
        nlbn= howmany((int) ISO_PTBL_SIZE(u), ISO_SECTOR_SIZE);
        buf = (char *) malloc(nlbn*ISO_SECTOR_SIZE);
        GETLBN(buf, (int) ISO_PTBL_MAN_LS(u), nlbn);

        /* handle the small endian first */
        for (dp=rootdp;dp != NULL;dp=dp->udirnext) {
                pep=(char *) buf + dp->upoffset;
                (void)lsb_int(IPE_ext_lbn(pep), dp->udextlbn);
        }
 
        (void) PUTLBN(buf,(int) ISO_PTBL_MAN_LS(u), nlbn);
 
        GETLBN(buf, (int) ISO_PTBL_MAN_MS(u), nlbn);
        /* handle the big endian next */
        for (dp=rootdp;dp != NULL;dp=dp->udirnext) {
                pep=(char *) buf + dp->upoffset;
                (void) msb_int (IPE_ext_lbn(pep), dp->udextlbn);
        }
        (void) PUTLBN(buf,(int) ISO_PTBL_MAN_MS(u), nlbn);
 
        /* free back buffer space */
        (void) cfree(buf);
 
}
 
