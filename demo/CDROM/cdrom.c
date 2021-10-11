/*************************************************************
 *                                                           *
 *                     file: cdrom.c                         *
 *                                                           *
 *************************************************************/

/*
 * Copyright (C) 1988, 1989 Sun Microsystems, Inc.
 */

#ifndef lint
static char sccsid[] = "@(#)cdrom.c 1.1 92/07/30 Copyr 1989, 1990 Sun Microsystem, Inc.";
#endif

/*
 * This file contains the routine to interface with the cdrom player.
 */

#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <mntent.h>
#include <string.h>
#include <sun/dkio.h>

#include <sys/buf.h>
#ifdef sun4c
#include <scsi/targets/srdef.h>
#else
#include <sundev/srreg.h>
#endif

#include <sys/time.h>

#include "cdrom.h"
#include "msf.h"

extern  int nx;
extern  char    *ProgName;
extern  char    *dev_name;
extern  int ejected;
extern  int debug;
extern  int fd;



/*
 *	these lines added for debugging by DGR
 */

#include <errno.h>
extern int errno;

/*
 *	end of added lines
 */

cdrom_open()
{
    if (nx) {
        if ((fd = open(dev_name, O_RDONLY)) < 0 &&
            errno != ENXIO && errno != EIO) {
            fprintf(stderr, "%s: failed to open device %s: ",
                ProgName, dev_name);
            perror("");
            exit(1);
        }
        if (fd < 0) ejected++;
    } else {
        if ((fd = open(dev_name, O_RDONLY|O_EXCL)) < 0 &&
            errno != ENXIO && errno != EIO ) {
            fprintf(stderr, "%s: failed to open device %s: ",
                ProgName, dev_name);
            perror("");
            exit(1);
        }
        if (fd < 0) ejected++;
    }
 
    if (fd < 0) {
        if (debug >= 3) {
            printf("open of %s failed, assumed no cartridge:", dev_name);
        perror("");
        }
    }

    if (fd > 0) init_player();
     
    return(fd);
}

int
cdrom_play_track(fd, play_track, end_track)
int	fd;
int	play_track;
int	end_track;
{
	struct	cdrom_ti	ti;

	ti.cdti_trk0 = play_track;
	ti.cdti_ind0 = 1;
	ti.cdti_trk1 = end_track;
	ti.cdti_ind1 = 1;
	if (ioctl(fd, CDROMPLAYTRKIND, (struct cdrom_ti *)&ti) < 0) {
		perror("cdrom_play_track:ioctl");
		return 0;
	}
	return 1;
}

int
cdrom_start(fd)
int	fd;
{
	if (ioctl(fd, CDROMSTART) < 0) {
		if (errno != ENXIO && errno != EIO) perror("ioctl");
		return 0;
	}
	return 1;
}

int
cdrom_stop(fd)
int	fd;
{
	if (ioctl(fd, CDROMSTOP) < 0) {
		perror("cdrom_stop:ioctl");
		return 0;
	}
	return 1;
}

int
cdrom_eject(fd)
int	fd;
{
	if (ioctl(fd, CDROMEJECT) < 0) {
		if (errno != ENXIO && errno != EIO) perror("ioctl");
		return 0;
	}
	return 1;
}

int
cdrom_read_tocentry(fd, entry)
int	fd;
struct	cdrom_tocentry	*entry;
{
	if (ioctl(fd, CDROMREADTOCENTRY, entry) < 0) {
		if (errno != ENXIO && errno != EIO) perror("ioctl");
		return 0;
	}
	return 1;
}
	
int
cdrom_read_tochdr(fd, hdr)
int	fd;
struct	cdrom_tochdr	*hdr;
{
	if (ioctl(fd, CDROMREADTOCHDR, hdr) < 0) {
		if (errno != ENXIO && errno != EIO) perror("ioctl");
		return 0;
	}
	return 1;
}
	
int
cdrom_pause(fd)
int	fd;
{
	if (ioctl(fd, CDROMPAUSE) < 0) {
		perror("cdrom_pause:ioctl");
		return 0;
	}
	return 1;
}

int
cdrom_resume(fd)
int	fd;
{
	if (ioctl(fd, CDROMRESUME) < 0) {
		perror("cdrom_resum:ioctl");
		return 0;
	}
	return 1;
}

int
cdrom_volume(fd, left_vol, right_vol)
int	fd;
int	left_vol;
int	right_vol;
{
	struct	cdrom_volctrl	vol;

	vol.channel0 = left_vol;
	vol.channel1 = right_vol;
	/* I don't know what these are for */
	vol.channel2 = left_vol;
	vol.channel3 = right_vol;
	if (ioctl(fd, CDROMVOLCTRL, (struct cdrom_volctrl *)&vol) < 0) {
		if (errno != ENXIO && errno != EIO) perror("ioctl");
		return 0;
	}
	return 1;
}

int
cdrom_get_relmsf(fd, tmp, track)
int	fd, *track;
struct	tm	*tmp;
{
    struct cdrom_subchnl    sc;
 
    sc.cdsc_format = CDROM_MSF;
    if (ioctl(fd, CDROMSUBCHNL, (struct cdrom_subchnl *)&sc) < 0) {
        if (errno != ENXIO && errno != EIO) perror("ioctl");
        return -1;
    }
     
    tmp->tm_sec = sc.cdsc_reladdr.msf.second;
    tmp->tm_min = sc.cdsc_reladdr.msf.minute;
 
    *track = sc.cdsc_trk;
 
    if (sc.cdsc_audiostatus == CDROM_AUDIO_PLAY) {
        return 1;
    }
    else {
        return 0;
    }  
}

int
cdrom_get_absmsf(fd, msf)
int fd;
Msf msf;
{
    struct cdrom_subchnl    sc;
 
    sc.cdsc_format = CDROM_MSF;
    if (ioctl(fd, CDROMSUBCHNL, (struct cdrom_subchnl *)&sc) < 0) {
        if (errno != ENXIO && errno != EIO) perror("ioctl");
        return 0;
    }
    msf->sec = sc.cdsc_absaddr.msf.second;
    msf->min = sc.cdsc_absaddr.msf.minute;
    msf->frame = sc.cdsc_absaddr.msf.frame;
 
    return 1;
}
 
 
int
cdrom_get_msf(fd, msf)
int fd;
struct  cdrom_msf   *msf;
{
    struct cdrom_subchnl    sc;
 
    sc.cdsc_format = CDROM_MSF;
    if (ioctl(fd, CDROMSUBCHNL, (struct cdrom_subchnl *)&sc) < 0) {
        if (errno != ENXIO && errno != EIO) perror("ioctl");
        return 0;
    }
    msf->cdmsf_min0 = sc.cdsc_absaddr.msf.minute;
    msf->cdmsf_sec0 = sc.cdsc_absaddr.msf.second;
    msf->cdmsf_frame0 = sc.cdsc_absaddr.msf.frame;
 
    msf->cdmsf_min1 = (unsigned char) -1;
    msf->cdmsf_sec1 = (unsigned char) -1;
    msf->cdmsf_frame1 = (unsigned char) -1;
 
    return 1;
}
 
int
cdrom_play_msf(fd, msf)
int fd;
struct  cdrom_msf   *msf;
{
    if (ioctl(fd, CDROMPLAYMSF,  (struct cdrom_msf *)msf) < 0) {
        if (errno != ENXIO && errno != EIO) perror("ioctl");
        return(0);
    }  
    return 1;
}

int
cdrom_playing(fd, track)
int	fd;
int	*track;
{
    struct cdrom_subchnl    sc;
 
    sc.cdsc_format = CDROM_MSF;
    if (ioctl(fd, CDROMSUBCHNL, (struct cdrom_subchnl *)&sc) < 0) {
        if (errno != ENXIO && errno != EIO) perror("ioctl");
        return 0;
    }
    *track = sc.cdsc_trk;
    if (sc.cdsc_audiostatus == CDROM_AUDIO_PLAY) {
        return 1;
    }
    else {
        return 0;
    } 
}

int
cdrom_paused(fd, track)
int	fd;
int	*track;
{
	struct cdrom_subchnl	sc;

	sc.cdsc_format = CDROM_MSF;
	if (ioctl(fd, CDROMSUBCHNL, (struct cdrom_subchnl *)&sc) < 0) {
		perror("cdrom_paused:ioctl");
		return 0;
	}
	*track = sc.cdsc_trk;
	printf("in pause, status is %d\n", sc.cdsc_audiostatus);
	if (sc.cdsc_audiostatus == CDROM_AUDIO_PAUSED) {
		return 1;
	}
	else {
		return 0;
	}
}
	
int
mounted(rawd)
	char *rawd;
{
	char buf[MAXPATHLEN], *cp;
	struct stat st;
	dev_t bdevno;
	FILE *fp;
	struct mntent *mnt;

	/*
	 * Get the block device corresponding to the raw device opened,
	 * and find its device number.
	 */
	if (stat(rawd, &st) != 0) {
		(void)fprintf(stderr, "Couldn't stat \"%s\": ", rawd);
		perror("mounted,1");
		return (UNMOUNTED);
	}
	/*
	 * If this is a raw device, we have to build the block device name.
	 */
	if ((st.st_mode & S_IFMT) == S_IFCHR) {
		cp = strchr(rawd, 'r');
		cp = (cp == NULL) ? NULL : ++cp;
		(void)sprintf(buf, "/dev/%s", cp);
		if (stat(buf, &st) != 0) {
			(void)fprintf(stderr, "Couldn't stat \"%s\": ", buf);
			perror("mounted,2");
			return (UNMOUNTED);
		}
	}
	if ((st.st_mode & S_IFMT) != S_IFBLK) {
		/*
		 * What is this thing? Ferget it!
		 */
		return (UNMOUNTED);
	}
	bdevno = st.st_rdev & (dev_t)(~0x07);	/* Mask out partition. */

	/*
	 * Now go through the mtab, looking at all hsfs filesystems.
	 * Compare the device mounted with our bdevno.
	 */
	if ((fp = setmntent(MOUNTED, "r")) == NULL) {
		(void)fprintf(stderr, "Couldn't open \"%s\"\n", MOUNTED);
		return (UNMOUNTED);
	}
	while ((mnt = getmntent(fp)) != NULL) {
		/* avoid obvious excess stat(2)'s */
		if (strcmp(mnt->mnt_type, "hsfs") != 0) {
			continue;
		}
		if (stat(mnt->mnt_fsname, &st) != 0) {
			continue;
		}
		if (((st.st_mode & S_IFMT) == S_IFBLK) &&
		    ((st.st_rdev & (dev_t)(~0x07)) == bdevno)) {
			(void)endmntent(fp);
			return (STILL_MOUNTED);
		}
	}
	(void)endmntent(fp);
	return (UNMOUNTED);
}

cdrom_get_subchnl(fd, sc)
	int fd;
	struct cdrom_subchnl	*sc;
{
	sc -> cdsc_format = CDROM_MSF;
	if (ioctl(fd, CDROMSUBCHNL, sc) < 0) {
		perror("cdrom_get_subchnl:ioctl");
		return 0;
	}
	return(1);
}
