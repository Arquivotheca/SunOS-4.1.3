#ifndef lint
static  char sccsid[] = "@(#)probe_devs.c 1.1 92/07/30 Copyright Sun Micro";
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
#include <sys/stat.h> 

#ifdef SVR4
#include <sys/mkdev.h>
#endif SVR4
#ifdef	sun3
#include <sundev/fpareg.h>
#include <fcntl.h>
#endif

#include "probe.h"
#include "../../lib/include/probe_sundiag.h"
#include "sdrtns.h"

#ifndef SVR4
#include <sys/ioctl.h>
#include <sun/audioio.h>
extern char *sprintf();
#endif SVR4

extern int make_dev_file();
static int check_dev_file(), abs_mode();

#ifndef	FPA_VERSION
#define FPA_VERSION  0xE     /* fpa version in imask/version register */
#endif

/*
 * check_des_dev(makedevs) - checks DES encryption chip.
 */
check_des_dev(makedevs)
    int makedevs;
{
    func_name = "check_des_dev";
    TRACE_IN
    if (check_dev(makedevs, "/dev/des", character, 11, 0, "0666", no))
    {
 	TRACE_OUT
	return(1);   
    }
    
    TRACE_OUT
    return(0);
}

/*
 * check_printer_dev(makedevs) - checks printer port on CPU board.
 */
check_printer_dev(makedevs, dunit)
int	makedevs;
int	dunit;
{
    char buffer[16];

    func_name = "check_printer_dev";
    TRACE_IN
    (void)sprintf(buffer, "/dev/pp%d", dunit);
    if (!check_dev(makedevs, buffer, character, 56, dunit, "0666", no))
    {
	TRACE_OUT
	return(0);   
    }
#ifndef	sun386
    (void)sprintf(buffer, "/dev/ppdiag%d", dunit);
    if (!check_dev(makedevs, buffer, character, 56, dunit+16, "0666", no))
    {
	TRACE_OUT
	return(0);
    }
#endif
    TRACE_OUT
    return(1);
}

/*
 * check_pr_dev(makedevs) - checks prestoserve board.
 */
check_presto_dev(makedevs)
int     makedevs;
{
    func_name = "check_presto_dev";
    TRACE_IN
    if (!check_dev(makedevs, "/dev/pr0", character, 104, 0, "0666", no))
    {
        TRACE_OUT
        return(0);
    }

    TRACE_OUT
    return(1);
}

/*
 * check_audio_dev(makedevs) - checks audio device.
 */
check_audio_dev(makedevs, unit, type)
int     makedevs, unit;
short   *type;
{
        int stat=0;
        func_name = "check_audio_dev";
        TRACE_IN
        stat = probe_audbridev(unit);
        switch (stat) {
           case 0:
                *type = 1;      /* New device(DBRI) */
                stat = 0;
                break;
           case 1:
                *type = 0;      /* Old device */
                stat = 1;
                break;
           default:
                stat = 0;       /* error */
        }
TRACE_OUT
return(stat);
}

/*
 * check_audbri_dev(makedevs) - checks audbri device.
 */
check_audbri_dev(makedevs, unit, type)
int     makedevs, unit;
short   *type;
{
        int stat=0;
        func_name = "check_audbri_dev";
        TRACE_IN
        stat = probe_audbridev(unit);
        switch (stat) {
           case 0:
                *type = 1;      /* New device(DBRI) */
                stat = 1;
                break;
           case 1:
                *type = 0;      /* Old device */
                stat = 0;
                break;
           default:
                stat = 0;       /* error */
        }
TRACE_OUT
return(stat);
}

/*
 * The probing function should check that the specified device is available * to be tested(optional if run by Sundiag). Usually, this involves opening * the device file, and using an appropriate ioctl to check the status of the
 * device, and then closing the device file. There are several flavors of
 * ioctls: see dkio(4s), fbio(4s), if(4n), mtio(4), and so on. It is nice
 * to put the probe code into a separate code module, because it usually has
 * most of the code which needs to be changed for a new SunOS release or port.
 */
probe_audbridev(unit)
int unit;
{
        int     device_type;
        int     audioctlfd;
        char buf[25];
        struct stat f_stat;

        func_name = "probe_audbridev";
        TRACE_IN

        /*
         *  open audio control device
         */
        if ((audioctlfd = open("/dev/audioctl", O_RDWR|O_NDELAY)) < 0) {
                send_message(0, WARNING, "no %s file", "/dev/audioctl");
                TRACE_OUT
                return(-1);
        }
 
        if (stat("/dev/audio", &f_stat) == -1) {
                send_message(0, WARNING, "no %s file", "/dev/audio");
                TRACE_OUT
                return(-1);
        }
        /*
         *  See if this is an appropriate audio device.
         */
        if (ioctl(audioctlfd, AUDIO_GETDEV, &device_type) < 0) {
                TRACE_OUT
                return(1);
        }
 
        close(audioctlfd);
 
        if (device_type != AUDIO_DEV_SPEAKERBOX &&
                        device_type != AUDIO_DEV_CODEC) {
                TRACE_OUT
                return(1);
        }
 
        TRACE_OUT
        return(0);
}

#ifdef sun3
/*
 * check_fpa_dev(makedevs) - checks Sun-3 Floating Point Accelerator
 * interface device file (see fpa(4s) and MAKEDEV)
 *
 * checks whether the fpa board is the original or fpa3x by opening
 * /dev/fpa and reading the fp_imask register.
 * return value: 0		failed
 *		 1		original fpa board
 *		 2		fpa-3x
 */
check_fpa_dev(makedevs)
    int makedevs;
{
    int			fpa_type = 0;
    int			fpa_fd = 0;
    char 		buf[BUFSIZ-1];
    struct fpa_device	*fpa = (struct fpa_device *) 0xE0000000;

    func_name = "check_fpa_dev";
    TRACE_IN
    if (check_dev(makedevs, "/dev/fpa", character, 34, 32, "0666", no)) {
	if ((fpa_fd = open("/dev/fpa", O_RDONLY, 0)) == -1) {
	    send_message(0, INFO, "couldn't open /dev/fpa");
	} else {
	    /*
	     * Determine type of FPA installed
	     * fpa_type = 0 for fpa
	     *		= 1 for fpa3x
	     */
            fpa_type = (FPA_VERSION & fpa->fp_imask) >> 1;

	    if (fpa_type == 1)
		send_message (0, DEBUG, "found fpa3x");
	    if (fpa_type == 0)
		send_message (0, DEBUG, "found original fpa");

	    close(fpa_fd);
   	    TRACE_OUT
            return(++fpa_type);
	}
			   
    }
    TRACE_OUT
    return(0);
}
#endif

/*
 * check_dev(makedevs, fname, dev_type, fmajor, fminor, mode) - generically
 * checks device file fname, and will make it if makedevs is set
 */
check_dev(makedevs, fname, dev_type, fmajor, fminor, mode, special_dev)
    int  makedevs;
    char *fname;
    Dev_type dev_type;
    int fmajor, fminor;
    char *mode;
    Special_dev special_dev;
{
    int status;
    char *perrmsg;
    char buf[BUFSIZ-1];

    func_name = "check_dev";
    TRACE_IN
    if ((status = check_dev_file(makedevs, fname, dev_type, fmajor, fminor, 
	mode, special_dev)) == 1)
    {
	TRACE_OUT
        return (1);
    }

    if ((makedevs) && (special_dev == mcp)) {
	send_message(0, INFO, 
		"don't know the major number to make the mcp ifd devices");
	TRACE_OUT
        return(0);
    }
    if ((makedevs) && (special_dev == scp)) {
	send_message(0, INFO, 
		"don't know the major number to make the dcp devices");
	TRACE_OUT
        return(0);
    }

    if (makedevs) {
        if (status == -1)               /* remove device file */
            if (unlink(fname) < 0) {
		send_message(0, ERROR, "cannot unlink %s: %s", fname, 
			errmsg(errno));
		TRACE_OUT
                return(0);
            }                           /* make device file */

        if (make_dev_file(fname, dev_type, fmajor, fminor, mode))
	{
	  send_message(0, INFO, "Created device file %s, permissions %s",
                fname, mode);
	  TRACE_OUT
	  return(1);
	}
    }

    TRACE_OUT
    return(1);
    /* changed from return(0) to return(1) so that the device files are
	not required to probe out */
}

/*
 * check_dev_file(makedevs, fname, dev_type, fmajor, fminor, mode) -
 * returns 0 if no file, -1 if problems with file, 1 if everything OK
 */
static int
check_dev_file(makedevs, fname, dev_type, fmajor, fminor, mode, special_dev)
    int  makedevs;
    char *fname;
    Dev_type dev_type;
    int fmajor, fminor;
    char *mode;
    Special_dev special_dev;
{
    struct stat f_stat;
    int m, fflags;
    char buf[BUFSIZ-1];

    func_name = "check_dev_file";
    TRACE_IN
    if (stat(fname, &f_stat) == -1) {
	if (!makedevs)
	    send_message(0, WARNING, "no %s file", fname);
 	TRACE_OUT
        return(0);
    }

    if ((dev_type == character) &&
        ((f_stat.st_mode & S_IFCHR) != S_IFCHR)) {
	send_message(0, ERROR, "%s: file not character special file", fname);
	TRACE_OUT
        return(-1);
    }
    if ((dev_type == block) &&
        ((f_stat.st_mode & S_IFBLK) != S_IFBLK)) {
	send_message(0, ERROR, "%s: file not block special file", fname);
	TRACE_OUT
        return(-1);
    }

    m= major(f_stat.st_rdev);
    if ((special_dev != scp) && (special_dev != mcp)) {
        if (fmajor != m) {
	    send_message(0, WARNING, 
		"%s: wrong major device number %d, expected %d",
                fname, m, fmajor);
	}
    }
    m = minor(f_stat.st_rdev);
    if (fminor != m) {
	send_message(0, WARNING, 
	    "%s: wrong minor device number %d, expected %d",fname, m, fminor);
    }
    m = f_stat.st_mode & ~S_IFMT & 0700;
    fflags =  abs_mode(mode) & 0700;	/* only compare the root permissions */
    if (fflags != m) {
	send_message(0, WARNING, "%s: wrong permissions, expected %s",
            fname, mode);
    }
    TRACE_OUT
    return(1);
}

int
make_dev_file(fname, dev_type, fmajor, fminor, mode)
    char *fname;
    Dev_type dev_type;
    int fmajor, fminor;
    char *mode;
{
    int m, fflags;
     char *perrmsg, buf[BUFSIZ-1];

    func_name = "make_dev_file";
    TRACE_IN
    fflags = abs_mode(mode);
    if (dev_type == block)
        m = S_IFBLK | 0666;
    if (dev_type == character)
        m = S_IFCHR | 0666;
    if (mknod(fname, m, (fmajor<<8)|fminor) < 0) {
	send_message(0, ERROR, "cannot mknod %s: %s", fname, errmsg(errno));
	TRACE_OUT
        return(0);
    }
    if (chmod(fname, fflags) < 0) {
	send_message(0, ERROR, "cannot chmod %s to %s: %s",
            fname, mode, errmsg(errno));
	TRACE_OUT
        return(0);
    }
    TRACE_OUT
    return(1);
}

/*
 * from chmod.c:
 */
static int
abs_mode(ms)
    char *ms;
{
    register char c; 
    register int i;

    func_name = "abs_mode";
    TRACE_IN
    i = 0;
    while ((c = *ms++) >= '0' && c <= '7')
        i = (i << 3) + (c - '0');
    ms--;
    TRACE_OUT
    return(i);
}
