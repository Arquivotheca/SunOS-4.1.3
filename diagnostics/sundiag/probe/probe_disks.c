#ifndef lint
static  char sccsid[] = "@(#)probe_disks.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#ifndef SVR4
#include <sys/dir.h>
#endif SVR4
#include <sys/file.h>
#ifdef SVR4
#include <sys/dkio.h>
#else SVR4
#include <sun/dkio.h>
#endif SVR4
#include "probe.h"
#include "../../lib/include/probe_sundiag.h"
#include "sdrtns.h"			/* Sundiag standard header */

#ifdef SVR4
#include <sys/fcntl.h>
#endif SVR4
static int get_disk_info();

#ifndef DKC_INTEL82077
#define DKC_INTEL82077  19      /* 3D floppy ctlr available from SunOS4.1.2_PSR */
#endif  DKC_INTEL82077

#define DISKDEVS        16

/*
 * check_disk_dev(makedevs, dname, dunit, amt) - checks disks (see dkio(4s)
 * and MAKEDEV) dname, dunit, and returns status and capacity if possible
 * returns DISKPROB if no device, NOLABEL if no label, and DISKOK
 * if everything OK - don't complain about bad open because nlist
 * doesn't know if SCSI disk is really exists or not, so assume not there
 */
check_disk_dev(makedevs, dname, dunit, amt, cname)
    int makedevs;
    char *dname;
    int dunit;
    int *amt;
    int *cname;
{
    char *mode = "0640";
    char *perrmsg;
    char name[MAXNAMLEN], buf[BUFSIZ-1];
    int  nunit, blk, chr, fmajor, unit, fd, capacity, create_flag;
    int	 idunit;
    Dev_type dev_type;
    Special_dev special_dev = no;

    func_name = "check_disk_dev";
    TRACE_IN
    if (strcmp(XD, dname) == 0) {
        blk = 10;
        chr = 42;
    } else if (strcmp(IP, dname) == 0) {
        blk = 22;
        chr = 65;
    } else if (strcmp(ID, dname) == 0) {
        blk = 22;
        chr = 72;
    } else if (strcmp(XY, dname) == 0) {
        blk = 3;
        chr = 9;
    } else if (strcmp(SD, dname) == 0) {
        blk = 7;
        chr = 17;
	special_dev = scsi;
    } else if (strcmp(SF, dname) == 0) {
        blk = 9;
        chr = 33;
	special_dev = scsi;
    } else if (strcmp(FD, dname) == 0) {
        blk = 16;
        chr = 54;
    }

    if (chr == 72)		/* ID disk */
    {
	idunit = ((dunit>>8)<<7) + (dunit&0x7f);
	blk += idunit >> 5;
	chr += idunit >> 5;
	idunit = ((idunit&0x3f) << 3) & 0xff;
    }

    if (blk != 16)		/* if not native(on-board) floppy */
    {
      if (special_dev == scsi)	/* open the device to be sure it is there */
      {
	(void)sprintf(name, "/dev/r%s%dc", dname, dunit);
	unit = dunit * 8 + 2;

	create_flag = 0;
	if (access(name, F_OK) != 0)
	{
	  if (!make_dev_file(name, character, chr, unit, mode))
	  {
	    TRACE_OUT
	    return(DISKPROB);
	  }
	  create_flag = 1;			/* Sundiag creates it */
	}

        if ((fd = open(name, O_RDONLY)) == -1)
	{
	  if (blk != 9 || errno != EIO)
	  {
	    if (create_flag) 
	      unlink(name);	/* remove it */
	    TRACE_OUT
	    return(DISKPROB);
	  }
        }
	else
          (void)close(fd);
	
	if (create_flag) 
	  unlink(name);		/* remove it */
      }

      for (nunit = 0; nunit < DISKDEVS; nunit++) {
	if (strcmp(ID, dname) != 0)
	{
	  (void)sprintf(name, "/dev/%s%s%d%c", (nunit < 8) ? "" : "r", 
		      dname, dunit, 'a' + nunit % 8);
	  unit = dunit * 8 + nunit % 8;
	}
	else
	{
	  (void)sprintf(name, "/dev/%s%s%03x%c", (nunit < 8) ? "" : "r", 
		      dname, dunit, 'a' + nunit % 8);
	  unit = idunit + nunit % 8;
	}

	dev_type = (nunit < 8) ? block : character;
	fmajor = (nunit < 8) ? blk : chr;

        if ((check_dev(makedevs, name, dev_type, fmajor, unit, 
	    mode, special_dev)) != 1)
	  if (nunit == 10)		/* only raw partition c is required */
	  {
	    TRACE_OUT
            return(DISKPROB);
	  }
      }
    }
    else
    {
	(void)sprintf(name, "/dev/r%s%d", dname, dunit);
	fmajor = chr;
	name[10] = '\0';

	unit = (dunit << 3) + 2;/* partition c */
	name[9] = 'c';

	create_flag = 0;
	if (access(name, F_OK) != 0)
	{
	      if (!make_dev_file(name, character, fmajor, unit, mode))
	      {
		TRACE_OUT
	        return(DISKPROB);
	      }
	      create_flag = 1;			/* Sundiag creates it */
	}
        if ((fd = open(name, O_RDONLY)) == -1) {
	      if (errno != EIO)
	      {
		if (create_flag) 
		    unlink(name);	/* remove it */
		TRACE_OUT
                return(DISKPROB);
	      }
        } else
              (void) close(fd);

	if (create_flag) 
		unlink(name);	/* remove it */

        if ((check_dev(makedevs, name, character, fmajor, unit,
		mode, special_dev)) != 1)
	{
	  TRACE_OUT
          return(DISKPROB);	/* only raw partition c is required */
	}

	--unit;			/* partition b */
	name[9] = 'b';
        check_dev(makedevs, name, character, fmajor, unit, mode, special_dev);

	--unit;			/* partition a */
	name[9] = 'a';
        check_dev(makedevs, name, character, fmajor, unit, mode, special_dev);
/***/
	(void)sprintf(name, "/dev/%s%d", dname, dunit);
	fmajor = blk;
	name[9] = '\0';

	unit = (dunit << 3) + 2;/* partition c */
	name[8] = 'c';
        check_dev(makedevs, name, block, fmajor, unit, mode, special_dev);

	--unit;			/* partition b */
	name[8] = 'b';
        check_dev(makedevs, name, block, fmajor, unit, mode, special_dev);

	--unit;			/* partition a */
	name[8] = 'a';
        check_dev(makedevs, name, block, fmajor, unit, mode, special_dev);
    }

    TRACE_OUT
    return(get_disk_info(dname, dunit, amt, cname));
}

/*
 * check_cdrom_dev(makedevs, dname, dunit, amt) - checks CD ROM(see cdromio(4s)
 * and MAKEDEV) (dname,dunit), and returns status and capacity if possible
 * returns DISKPROB if no device, NOLABEL if no label, and DISKOK
 * if everything OK - don't complain about bad open because nlist
 * doesn't know if CD ROM drive is really exists or not, so assume not there
 */
check_cdrom_dev(makedevs, dname, dunit, amt, cname)
    int makedevs;
    char *dname;
    int dunit;
    int *amt;
    int *cname;
{
    char *mode = "0555";
    char name[MAXNAMLEN];
    int	 blk, chr, minorno, create_flag, fd;

    func_name = "check_cdrom_dev";
    TRACE_IN
    chr = 58;
#ifdef	NEW
    blk = 18;
    minorno = dunit*8;   /* Changed from 'dunit' to pacify 4.1.1, not for 4.1*/
#else	NEW
    blk = 17;
    minorno = dunit*8;
#endif	NEW

    (void)sprintf(name, "/dev/r%s%d", dname, dunit);
	
    create_flag = 0;
    if (access(name, F_OK) != 0)
    {
	if (!make_dev_file(name, character, chr, minorno, mode))
	{
		TRACE_OUT
	        return(DISKPROB);
	}
	      create_flag = 1;			/* Sundiag creates it */
    }
    if ((fd = open(name, O_RDONLY)) == -1)
    {
      if (errno != EIO)
      {
	if (create_flag) 
	  unlink(name);				/* remove it */
	TRACE_OUT
        return(DISKPROB);
      }
    }
    else
      (void) close(fd);

    if (create_flag) 
	unlink(name);	/* remove it */

    if ((check_dev(makedevs,name,character,chr,minorno,mode,scsi)) != 1)
    {
	TRACE_OUT
	return(DISKPROB);
    }
    (void)sprintf(name, "/dev/%s%d", dname, dunit);
    if ((check_dev(makedevs,name,block,blk,minorno,mode,scsi)) != 1)
    {
	TRACE_OUT
	return(DISKPROB);
    }

    TRACE_OUT
    return(get_disk_info(dname, dunit, amt, cname));
}

/*
 * from dkinfo.c:
 */
static int
get_disk_info(dname, dunit, amt, cname)
    char *dname;
    int dunit;
    unsigned int *amt;
    int *cname;
{
    short sec_size=512; /* default sector size, becomes 1024 on Med 3D floppy */
    char *perrmsg;
    char name[MAXNAMLEN], buf[BUFSIZ-1];
    struct dk_geom geom;
    struct fdk_char floppy_char;
    struct dk_info inf;
    int fd;

    func_name = "get_disk_info";
    TRACE_IN
    *amt = 0;
    *cname = 0;
    if (strcmp(dname, ID) == 0)
      (void) sprintf(name, "/dev/r%s%03xc", dname, dunit);
    else if (strcmp(dname, CDROM) == 0)
      (void) sprintf(name, "/dev/r%s%d", dname, dunit);
    else
      (void) sprintf(name, "/dev/r%s%dc", dname, dunit);

    if (access(name, F_OK) != 0)
    {
	send_message(0, INFO, "no device file %s, can't get info.", name);
	TRACE_OUT
	return(NOLABEL);
    }

    if ((fd = open(name, O_RDONLY)) == -1) {
	perrmsg = errmsg(errno);
	if (errno == EIO) {
            send_message(0, INFO, "%s not ready: %s", name, perrmsg);
	    TRACE_OUT
	    return(NOLABEL);
	} else {
            send_message(0, ERROR, "%s open error: %s", name, perrmsg);
	    TRACE_OUT
            return(DISKPROB);
	}
    }

    if (ioctl(fd, DKIOCINFO, &inf) != 0) {
        send_message(0, ERROR, "%s disk DKIOCINFO ioctl: %s",
            name, errmsg(errno));
        (void) close(fd);
	TRACE_OUT
        return(DISKPROB);
    }
    else
	*cname = inf.dki_ctype;

    if (*cname == DKC_INTEL82077) {     /* For 3D floppy only */
    	if (ioctl(fd, FDKIOGCHAR, &floppy_char) != 0) {
       		send_message(0, ERROR, "%s floppy FDKIOGCHAR ioctl: %s",
            	name, errmsg(errno));
        	(void) close(fd);
        	TRACE_OUT
        	return(DISKPROB);
    	}
    	sec_size = floppy_char.sec_size;
    }
    if (ioctl(fd, DKIOCGGEOM, &geom) != 0) {
        send_message(0, ERROR, "%s disk DKIOCGGEOM ioctl: %s",
            name, errmsg(errno));
        (void) close(fd);
	TRACE_OUT
        return(DISKPROB);
    }
    else
    {
        *amt = (geom.dkg_ncyl*geom.dkg_nhead*geom.dkg_nsect*sec_size)/1000;
	if (strcmp(dname, CDROM) == 0) *amt = *amt * 4;  /* 2048/sector */
    }

    (void) close(fd);

    TRACE_OUT
    return(DISKOK);
}
