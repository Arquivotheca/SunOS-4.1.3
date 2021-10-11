/*      @(#)labelit.c 1.1 92/07/30 SMI      */
/*
 * labelit [option=value ...] cdimage
 * where options are:
        sysid		system identifier               (a characters, 32 max)
        volid:          volume identifier               (d-characters, 32 max)
        volsetid:       volume set identifier           (d-characters, 128 max)
        pubid:          publisher identifier            (d-characters, 128 max)
        prepid:         data preparer identifier        (d-charcter, 128 max)
        applid:          application identifier          (d-charcter, 128 max)
        copyfile:       copyright file identifier       (d-characters, 128 max)
        absfile:        abstract file identifier        (d-characters, 37 max)
        bibfile:        bibliographic file identifier   (d-charcters, 37 max)
 */

#ifndef lint
static  char sccsid[] = "@(#)labelit.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif  lint

#include <fcntl.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include "hsfs_spec.h"
#include "iso_spec.h"
#include "iso_impl.h"

#define PUTSECTOR(buf, secno, nosec) (putdisk(buf, (secno)*ISO_SECTOR_SIZE, \
	(nosec)*ISO_SECTOR_SIZE))
#define GETSECTOR(buf, secno, nosec) (getdisk(buf, (secno)*ISO_SECTOR_SIZE, \
	(nosec)*ISO_SECTOR_SIZE))
char *string;
int  cdfd;
int cd_type;
char hs_buf[ISO_SECTOR_SIZE];
int  hs_pvd_sec_no;
char iso_buf[ISO_SECTOR_SIZE];
int  iso_pvd_sec_no;
char unix_buf[ISO_SECTOR_SIZE];
int  unix_pvd_sec_no;
char *vdp;
char *sysid;
char *volid;
char *volsetid;
char *pubid;
char *prepid;
char *applid;
char *copyfile;
char *absfile;
char *bibfile;
int volsetsize;
int volsetseq;
int blksize;
int volsize;

main(argc, argv)
int	argc;
char	**argv;
{
	register c;
	int openopt;

	for(c=1; c<argc; c++) {
		string = argv[c];
		if(match("sysid=")) {
			sysid = string;
			continue;
		}
		if(match("volid=")) {
			volid = string;
			continue;
		}
		if(match("volsetid=")) {
			volsetid = string;
			continue;
		}
		if (match("pubid=")) {
			pubid = string;
			continue;
		}
		if(match("prepid=")) {
			prepid = string;
			continue;
		}
		if(match("applid=")) {
			applid = string;
			continue;
		}
		if(match("copyfile=")) {
			copyfile = string;
			continue;
		}
		if(match("absfile=")) {
			absfile = string;
			continue;
		}
		if(match("bibfile=")) {
			bibfile = string;
			continue;
		}
		break;
	}
	/* the last argument must be the cdrom iamge file */
	if (argc != c+1) usage();

	/* open image file in read write only if necessary */
	if (argc == 2) openopt = O_RDONLY;
	else openopt = O_RDWR;

	if ((cdfd = open(argv[c], openopt)) < 0) {
		if (index(argv[c], '=') ||
			index(argv[c], '-')) usage();
		perror(argv[c]);
		exit(1);
	}

	/* check volume descriptor */
	(void) ckvoldesc();

	if (cd_type < 0) {
		fprintf(stderr, "labelit: unknown cdrom format label\n");
		exit(1);
	}

	/* update label, if needed */
	if (argc != 2) updatelabel();

	/* print the (updated) image label */
	prntlabel();

	close(cdfd);
	exit(0);
}

usage()
{
	fprintf(stderr, "Usage: labelit [option=value ...] cdimage\n");
	exit(1);
}

/*
 * findhsvol: check if the disk is in high sierra format
 *            return(1) if found, (0) otherwise
 *	      if found, volp will point to the descriptor
 *
 */
int
findhsvol(volp)
char *volp;
{
int secno;
int i;

	secno = HS_VOLDESC_SEC;
	GETSECTOR(volp, secno++, 1);
	while (HSV_DESC_TYPE(volp) != VD_EOV) {
		for (i = 0; i < HSV_ID_STRLEN; i++)
			if (HSV_STD_ID(volp)[i] != HSV_ID_STRING[i])
				goto cantfind;
		if (HSV_STD_VER(volp) != HSV_ID_VER)
			goto cantfind;
		switch (HSV_DESC_TYPE(volp)) {
		case VD_SFS:
			hs_pvd_sec_no = secno-1;
			return (1);
		case VD_EOV:
			goto cantfind;
		}
		GETSECTOR(volp, secno++, 1);
	}
cantfind:
	return (0);
}

/*
 * findisovol: check if the disk is in ISO 9660 format
 *            return(1) if found, (0) otherwise
 *	      if found, volp will point to the descriptor
 *
 */
int 
findisovol(volp)
char *volp;
{
int secno;
int i;

	secno = ISO_VOLDESC_SEC;
	GETSECTOR(volp, secno++, 1);
	while (ISO_DESC_TYPE(volp) != ISO_VD_EOV) {
		for (i = 0; i < ISO_ID_STRLEN; i++)
			if (ISO_STD_ID(volp)[i] != ISO_ID_STRING[i])
				goto cantfind;
		if (ISO_STD_VER(volp) != ISO_ID_VER)
			goto cantfind;
		switch (ISO_DESC_TYPE(volp)) {
		case ISO_VD_PVD:
			iso_pvd_sec_no = secno-1;
			return (1);
		case ISO_VD_EOV:
			goto cantfind;
		}
		GETSECTOR(volp, secno++, 1);
	}
cantfind:
	return (0);
}

/*
 * findunixvol: check if the disk is in UNIX extension format
 *            return(1) if found, (0) otherwise
 *	      if found, volp will point to the descriptor
 *
 */
int 
findunixvol(volp)
char *volp;
{
int secno;
int i;

	secno = ISO_VOLDESC_SEC;
	GETSECTOR(volp, secno++, 1);
	while (ISO_DESC_TYPE(volp) != ISO_VD_EOV) {
		for (i = 0; i < ISO_ID_STRLEN; i++)
			if (ISO_STD_ID(volp)[i] != ISO_ID_STRING[i])
				goto cantfind;
		if (ISO_STD_VER(volp) != ISO_ID_VER)
			goto cantfind;
		switch (ISO_DESC_TYPE(volp)) {
		case ISO_VD_UNIX:
			unix_pvd_sec_no = secno-1;
			return (1);
		case ISO_VD_EOV:
			goto cantfind;
		}
		GETSECTOR(volp, secno++, 1);
	}
cantfind:
	return (0);
}

ckvoldesc()
{
	if (findhsvol(hs_buf)) 
		cd_type = 0;
	else if (findisovol(iso_buf)) {
		if (findunixvol(unix_buf)) 
			cd_type = 2;
		else cd_type = 1;
	}
	else cd_type = -1;
}

updatelabel()
{
	switch (cd_type) {
	case 0:
		copystring(sysid, HSV_sys_id(hs_buf), 32);
		copystring(volid, HSV_vol_id(hs_buf), 32);
		copystring(volsetid, HSV_vol_set_id(hs_buf), 128);
		copystring(pubid, HSV_pub_id(hs_buf), 128);
		copystring(prepid, HSV_prep_id(hs_buf), 128);
		copystring(applid, HSV_appl_id(hs_buf), 128);
		copystring(copyfile, HSV_copyr_id(hs_buf), 37);
		copystring(absfile, HSV_abstr_id(hs_buf), 37);
		PUTSECTOR(hs_buf, hs_pvd_sec_no, 1);
		break;
	case 2:
		copystring(sysid, ISO_sys_id(unix_buf), 32);
		copystring(volid, ISO_vol_id(unix_buf), 32);
		copystring(volsetid, ISO_vol_set_id(unix_buf), 128);
		copystring(pubid, ISO_pub_id(unix_buf), 128);
		copystring(prepid, ISO_prep_id(unix_buf), 128);
		copystring(applid, ISO_appl_id(unix_buf), 128);
		copystring(copyfile, ISO_copyr_id(unix_buf), 37);
		copystring(absfile, ISO_abstr_id(unix_buf), 37);
		copystring(bibfile, ISO_bibli_id(unix_buf), 37);
		PUTSECTOR(unix_buf, unix_pvd_sec_no, 1);
		/* after update unix volume descriptor,
		   fall thru to update the iso primary vol descriptor */
	case 1:
		copystring(sysid, ISO_sys_id(iso_buf), 32);
		copystring(volid, ISO_vol_id(iso_buf), 32);
		copystring(volsetid, ISO_vol_set_id(iso_buf), 128);
		copystring(pubid, ISO_pub_id(iso_buf), 128);
		copystring(prepid, ISO_prep_id(iso_buf), 128);
		copystring(applid, ISO_appl_id(iso_buf), 128);
		copystring(copyfile, ISO_copyr_id(iso_buf), 37);
		copystring(absfile, ISO_abstr_id(iso_buf), 37);
		copystring(bibfile, ISO_bibli_id(iso_buf), 37);
		PUTSECTOR(iso_buf, iso_pvd_sec_no, 1);
		break;
	}
}

prntlabel()
{
	int i;
	switch (cd_type) {
	case 0:
		printf("CD-ROM is in High Sierra format\n");
		sysid=(char *)HSV_sys_id(hs_buf);
		volid=(char *)HSV_vol_id(hs_buf);
		volsetid=(char *)HSV_vol_set_id(hs_buf);
		pubid=(char *)HSV_pub_id(hs_buf);
		prepid=(char *)HSV_prep_id(hs_buf);
		applid=(char *)HSV_appl_id(hs_buf);
		copyfile=(char *)HSV_copyr_id(hs_buf);
		absfile=(char *)HSV_abstr_id(hs_buf);
		bibfile=NULL;
		volsetsize= HSV_SET_SIZE(hs_buf);
		volsetseq= HSV_SET_SEQ(hs_buf);
		blksize= HSV_BLK_SIZE(hs_buf);
		volsize= HSV_VOL_SIZE(hs_buf);
		break;
	case 1:
		printf("CD-ROM is in ISO 9660 format\n");
		sysid=(char *) ISO_sys_id(iso_buf);
		volid=(char *)ISO_vol_id(iso_buf);
		volsetid=(char *)ISO_vol_set_id(iso_buf);
		pubid=(char *)ISO_pub_id(iso_buf);
		prepid=(char *)ISO_prep_id(iso_buf);
		applid=(char *)ISO_appl_id(iso_buf);
		copyfile=(char *)ISO_copyr_id(iso_buf);
		absfile=(char *)ISO_abstr_id(iso_buf);
		bibfile=(char *)ISO_bibli_id(iso_buf);
		volsetsize=ISO_SET_SIZE(iso_buf);
		volsetseq=ISO_SET_SEQ(iso_buf);
		blksize=ISO_BLK_SIZE(iso_buf);
		volsize=ISO_VOL_SIZE(iso_buf);
		break;
	case 2:
		printf("CD-ROM is in ISO 9660 format with UNIX extension\n");
		sysid=(char *)ISO_sys_id(unix_buf);
		volid=(char *)ISO_vol_id(unix_buf);
		volsetid=(char *)ISO_vol_set_id(unix_buf);
		pubid=(char *)ISO_pub_id(unix_buf);
		prepid=(char *)ISO_prep_id(unix_buf);
		applid=(char *)ISO_appl_id(unix_buf);
		copyfile=(char *)ISO_copyr_id(unix_buf);
		absfile=(char *)ISO_abstr_id(unix_buf);
		bibfile=(char *)ISO_bibli_id(unix_buf);
		volsetsize=ISO_SET_SIZE(unix_buf);
		volsetseq=ISO_SET_SEQ(unix_buf);
		blksize=ISO_BLK_SIZE(unix_buf);
		volsize=ISO_VOL_SIZE(unix_buf);
		break;
	default:
		return;
	}
	/* system id */
	prntstring("System id", sysid, 32);
	/* read volume id */
	prntstring("Volume id", volid, 32);
	/* read volume set id */
	prntstring("Volume set id", volsetid, 128);
	/* publisher id */
	prntstring("Publisher id", pubid, 128);
	/* data preparer id */
	prntstring("Data preparer id", prepid, 128);
	/* application id */
	prntstring("Application id", applid, 128);
	/* copyright file identifier */
	prntstring("Copyright File id", copyfile, 37);
	/* Abstract file identifier */
	prntstring("Abstract File id", absfile, 37);
	/* Bibliographic file identifier */
	prntstring("Bibliographic File id", bibfile, 37);
	/* print volume set size */
	printf("Volume set size is %d\n", volsetsize);
	/* print volume set sequnce number */
	printf("Volume set sequence number is %d\n", volsetseq);
	/* print logical block size */
	printf("Logical block size is %d\n", blksize);
	/* print volume size */
	printf("Volume size is %d\n", volsize);
}

copystring (from, to, size)
char *from;
char *to;
int size;
{
int i;

	if (from == NULL) return;
	for (i=0;i<size;i++) {
		if (*from == '\0') break;
		else *to++=*from++;
	}
	for (;i<size;i++) *to++=' ';
}

prntstring(heading, s, maxlen)
char * heading;
char *s;
int maxlen;
{
int i;
	if (maxlen < 1) return;
	if (heading == NULL || s == NULL) return;
	/* print heading */
	printf("%s: ", heading);

	/* strip off trailing zeros */
	for (i=maxlen-1;i >= 0; i--)
		if (s[i] != ' ') break;

	maxlen = i+1;
	for (i=0;i<maxlen;i++) printf("%c", s[i]);
	printf("\n");
}

match(s)
char *s;
{
	register char *cs;

	cs = string;
	while(*cs++ == *s)
		if(*s++ == '\0')
			goto true;
	if(*s != '\0')
		return(0);

true:
	cs--;
	string = cs;
	return(1);
}

/*readdisk - read from cdrom image file */
getdisk(buf, daddr, size)
char *buf; /* buffer area */
int daddr; /* disk addr */
int size; /* no. of byte */
{
        if (lseek(cdfd, daddr, L_SET) == -1) {
                perror("getdisk/lseek");
                exit(1);
        }
        if (read(cdfd, buf, size) != size) {
                perror("getdisk/read");
                exit(1);
        }
 
}

/*putdisk - write to cdrom image file */
putdisk(buf, daddr, size)
char *buf; /* buffer area */
int daddr; /* disk addr */
int size; /* no. of byte */
{
	if (lseek(cdfd, daddr, L_SET) == -1) {
		perror("putdisk/lseek");
		exit(1);
	}
	if (write(cdfd, buf, size) != size) {
		perror("putdisk/write");
		exit(1);
	}
}
