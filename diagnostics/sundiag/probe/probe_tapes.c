#ifndef lint
static  char sccsid[] = "@(#)probe_tapes.c 1.1 92/07/30 Copyright Sun Micro";
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
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#ifndef SVR4
#include <sundev/tmreg.h>
#include <sundev/xtreg.h>
#include <sundev/arreg.h>
#else SVR4
#include <sys/xtreg.h>
#endif SVR4
#include "probe.h"
#include "../../lib/include/probe_sundiag.h"
#include "sdrtns.h" 		/* sundiag standard header */

#ifndef	sun386
#include <sys/mtio.h>
struct	mt_tape_info	tapes[]=MT_TAPE_INFO;
#else
#include "mtio.h"
#include "tape_strings.h"
#endif

static get_tape_info();
static short tape_status();

#define MT_TAPEDEVS     4

/*
 * check_mt_dev(makedevs, dunit, t_type) - checks mag tape (see mtio(4) and 
 * MAKEDEV) tape device dunit, and returns status and t_type if possible
 */
check_mt_dev(makedevs, dname, dunit, t_type)
    int makedevs;
    char *dname;
    int dunit;
    short *t_type;
{
    struct stat f_stat;
    char tname[MAXNAMLEN];
    int nunit, tunit8;
    int status;
    int xt=0;

    func_name = "check_mt_dev";
    TRACE_IN
    if (strcmp(dname, MT) == 0)
        xt = 0;
    else                /* XT */
        xt = 1;

    tunit8 = dunit + 8;

    for (nunit = 0; nunit < MT_TAPEDEVS; nunit++) {
        switch (nunit) {
        case 0:
            (void) sprintf(tname, "/dev/rmt%d", dunit);
	    break;
        case 1:
            (void) sprintf(tname, "/dev/rmt%d", tunit8);
            break;
        case 2:
            (void) sprintf(tname, "/dev/nrmt%d", dunit);
            break;
        case 3:
            (void) sprintf(tname, "/dev/nrmt%d", tunit8);
            break;
        }

	if (access(tname, F_OK) != 0)
	  send_message(0, WARNING, "no %s file", tname);
    }


    if (makedevs) {	/* always recreate device files for mt/xt */
	if (xt)
          (void) sprintf(buff,"cd /dev ; MAKEDEV xt%d 1>/dev/null 2>&1", dunit);
	else 
          (void) sprintf(buff,"cd /dev ; MAKEDEV mt%d 1>/dev/null 2>&1", dunit);
        system(buff);

	if (xt)
          send_message(0, INFO, "Created device files for xt%d.", dunit);
	else 
          send_message(0, INFO, "Created device files for mt%d.", dunit);
    }

    (void) sprintf(tname, "/dev/rmt%d", dunit);
    if (access(tname, F_OK) != 0)
    {
      buff[0] = '\0';
      if (xt)
	(void) sprintf(buff,"cd %s ; MAKEDEV xt%d 1>/dev/null 2>&1",
		tmp_devdir, dunit);
      else
	(void) sprintf(buff,"cd %s ; MAKEDEV mt%d 1>/dev/null 2>&1",
		tmp_devdir, dunit);
      system(buff);
      (void) sprintf(tname, "%s/rmt%d", tmp_devdir, dunit);
    }
	
    *t_type = 0;
    status = get_tape_info(tname, dname, dunit, t_type, no);
    TRACE_OUT
    return(status);
}

#define ST_TAPEDEVS     8

/*
 * check_st_dev(makedevs, dunit, t_type) - checks SCSI tape (see st(4s) and 
 * MAKEDEV) tape device dunit, and returns status and t_type if possible
 * returns TAPEPROB if no device, OFFLINE if tape is offline, 
 * ONLINE if everything OK - don't complain about bad open because nlist
 * doesn't know if SCSI tape really exists or not, so assume not there
 */
check_st_dev(makedevs, dunit, t_type)
    int makedevs;
    int dunit;
    short *t_type;
{
    struct stat f_stat;
    char tname[MAXNAMLEN];
    int nunit, rew, tunit8, tunit16, tunit24;
    int status;
    int	create_flag, fd, num_device;

    func_name = "check_st_dev";
    TRACE_IN
    rew = dunit;
    tunit8 = rew + 8;
    tunit16 = rew + 16;
    tunit24 = rew + 24;
    num_device = ST_TAPEDEVS;	
    create_flag = 0;

    for (nunit = 0; nunit < num_device; nunit++) {
        switch (nunit) {
        case 0:
            (void) sprintf(tname, "/dev/rst%d", dunit);

	    if (access(tname, F_OK) != 0)
	    {
	       if (makedevs)
               {
                  buff[0] = '\0';
                  (void) sprintf(buff,"cd /dev ; MAKEDEV st%d 1>/dev/null 2>&1", dunit);
                  system(buff);
                  send_message(0, INFO, "Created device files for st%d", dunit);
	       }else{
                  create_flag = 1;
                  buff[0] = '\0';
                  (void) sprintf(buff,"cd %s ; MAKEDEV st%d 1>/dev/null 2>&1", tmp_devdir, dunit);
                  system(buff);
               }
            }
 
	    if (create_flag)
               (void) sprintf(tname, "%s/rst%d", tmp_devdir, dunit);

            if ((fd = open(tname, O_RDONLY|O_NDELAY)) == -1)
	    {
		if (errno != EIO)
		{
		  TRACE_OUT
                  return(TAPEPROB);
		}
	    }
	    else
		(void)close(fd);

	    if (create_flag)
	      (void) sprintf(tname, "/dev/rst%d", dunit);
            break;
        case 1:
	    (void)sprintf(tname, "/dev/rst%d", tunit8);
            break;
        case 2:
	    (void)sprintf(tname, "/dev/nrst%d", dunit);
            break;
        case 3:
	    (void)sprintf(tname, "/dev/nrst%d", tunit8);
            break;


        case 4:
	    (void)sprintf(tname, "/dev/rst%d", tunit16);
            break;
        case 5:
	    (void)sprintf(tname, "/dev/rst%d", tunit24);
            break;
        case 6:
	    (void)sprintf(tname, "/dev/nrst%d", tunit16);
            break;
        case 7:
	    (void)sprintf(tname, "/dev/nrst%d", tunit24);
            break;
        }
	if (access(tname, F_OK) != 0) {
	    if (makedevs) {
               (void) sprintf(buff,"cd /dev ; MAKEDEV st%d >/dev/null 2>&1", dunit);
               system(buff);
               send_message(0, INFO, "Created device files for st%d.", dunit);
	    }else{
	       send_message(0, WARNING, "no %s file", tname);
	   }
    	}
    }	

    *t_type = 0;
    if (create_flag) 
	(void)sprintf(tname,"%s/rst%d",tmpname, dunit);

    else   (void)sprintf(tname, "/dev/rst%d", dunit);

    status = get_tape_info(tname, "st", dunit, t_type, scsi);
    if (status == TAPEPROB) 
    {
    	TRACE_OUT
	return(status);
    }

    if ((*t_type == MT_ISHP || *t_type == MT_ISKENNEDY) && status != OFFLINE)
	/* this is a FLT device, check for the support of "compression" mode */
    {
        if (create_flag) 
		(void)sprintf(tname,"%s/dev/rst%d",tmpname,tunit24);
	else    (void)sprintf(tname, "/dev/rst%d", tunit24);

        if (open(tname, O_RDONLY|O_NDELAY) != -1)
	        status = FLT_COMP;     /* yes it supports "compression" mode */
    }

#ifdef NEW
    if ((*t_type == MT_ISEXB8500) && status != OFFLINE)
	/* this is a EXB8500 device, check for the support of "compression" mode */
    {
        if (create_flag) 
		(void)sprintf(tname,"%s/dev/rst%d",tmpname,tunit16);
	else    (void)sprintf(tname, "/dev/rst%d", tunit16);

        if (open(tname, O_RDONLY|O_NDELAY) != -1)
	        status = FLT_COMP;     /* yes it supports "compression" mode */
    }
#endif NEW
    TRACE_OUT
    return(status);
}

/*
 * get_tape_info is not generalized until the Xylogics tape driver
 * learns how to tell us what kind of tape drive it's talking to, even
 * if the !$* tape is offline...
 */
static
get_tape_info(tname, dname, dunit, tape_type, special_dev)
    char *tname;
    char *dname;
    int dunit;
    short *tape_type;
    Special_dev special_dev;
{
    int fd;
    char *perrmsg;
    char buf[BUFSIZ-1];
    struct mtget mt_status;

    func_name = "get_tape_info";
    TRACE_IN
    if (access(tname, F_OK) != 0)
    {
	send_message(0, INFO, "%s%d: no device file %s, can't get info.",
                        dname, dunit, tname);
	TRACE_OUT
	return(OFFLINE);
    }

    if ((fd = open(tname, O_RDONLY|O_NDELAY)) == -1) {
        if (errno == EIO) {
	    send_message(0, INFO, "%s%d: tape offline, can't get info.",
                dname, dunit);
	    TRACE_OUT
            return(OFFLINE);
	} else
	{
	    TRACE_OUT
            return(TAPEPROB);		/* drive does not exist */
	}
    } else {
        if (ioctl(fd, MTIOCGET, (char *)&mt_status) < 0) {
	    send_message(0, WARNING, 
		"%s%d tape MTIOCGET ioctl: %s, can't get info.", 
		dname, dunit, errmsg(errno));
            (void) close(fd);
        } else {
	    *tape_type = tape_status(&mt_status);
            (void) close(fd);
	}
    }
    TRACE_OUT
    return(ONLINE);
}

/*
 * from mt.c:
 */
static short
tape_status(bp)
    register struct mtget *bp;
{
    char buf[BUFSIZ-1];
    register struct mt_tape_info *mt;

    func_name = "tape_status";
    TRACE_IN
    for (mt = tapes; mt->t_type; mt++)
        if (mt->t_type == bp->mt_type)
            break;
    if (mt->t_type == 0) {
	send_message(0, ERROR, "unknown tape drive type (%d)", bp->mt_type); 
	TRACE_OUT
        return(0);
    }
    if (debug)
        if ((mt->t_type >= MT_ISSYSGEN11) && /*MT_ISSCSI_LO=MT_ISSYSGEN11=0x11*/
            (mt->t_type <= MT_ISCCS32)) {   /* MT_ISSCSI_HI=MT_ISCCS32 = 0x2f */

            /* Handle SCSI tape drives specially. */
            (void) printf("%s tape drive:\n sense key= 0x%x residual= %d",
                mt->t_name, bp->mt_erreg, bp->mt_resid);
            (void) printf("retries= %d\n", bp->mt_dsreg);
            (void) printf("   file no= %d   block no= %d\n",
                bp->mt_fileno, bp->mt_blkno);
        } else {
            /* Handle other drives here. */
            (void) printf("%s tape drive:\n residual= %d",
                mt->t_name, bp->mt_resid);
            (void) printf(" ds %x", bp->mt_dsreg, mt->t_dsbits);
            (void) printf(" er %x\n", bp->mt_erreg, mt->t_erbits);
        }
    TRACE_OUT
    return(mt->t_type);
}
