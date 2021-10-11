#ifndef lint
static  char sccsid[] = "@(#)probe_fbs.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
#ifndef SVR4
#include <sys/dir.h>
#endif SVR4
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifdef SVR4
#include <sys/fbio.h>
#else SVR4
#include <sun/fbio.h>
#endif SVR4
#ifndef SVR4
#include <pixrect/pixrect.h>
#include <pixrect/gp1reg.h>
#include <pixrect/gp1var.h>
#endif SVR4
#include "probe.h"
#include "../../lib/include/probe_sundiag.h"
#include "fb_strings.h"
#include "sdrtns.h"		/* sundiag standard header */	

#ifndef SVR4
extern char *sprintf();
#endif SVR4
extern char *valloc();
extern caddr_t mmap();

#define SH_MEM_BASE_ADDR        0x400
#define GP2_BASE                0x240000        /* base address of gp2 */

static int Map_bds();
static void test_address();
#ifndef SVR4
bus_test_error();
#endif SVR4
static int check_fb(), fb_status();
/* 
 * static int isgp();
 */

int gptype = 0;
jmp_buf(sjbuf);

/*
 * check_bwone(makedevs) - checks Sun-1 black and white
 * frame buffer device file (see bwone(4s) and MAKEDEV)
 */
check_bwone_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int unit;
{
    char name[22];

    func_name = "check_bwone_dev";
    TRACE_IN
    sprintf(name, "/dev/bwone%d", unit);

    if (check_dev(makedevs, name, character, 26, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
	{
	    TRACE_OUT
            return(1);
	}
    TRACE_OUT
    return(0);
}

/*
 * check_bwtwo_dev(makedevs) - checks Sun-3/Sun-2 black and white
 * frame buffer device file (see bwtwo(4s) and MAKEDEV)
 */
check_bwtwo_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int unit;
{
    char name[22];

    func_name = "check_bwtwo_dev";
    TRACE_IN
    sprintf(name, "/dev/bwtwo%d", unit);

    if (check_dev(makedevs, name, character, 27, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
	{
	    TRACE_OUT
            return(1);
   	}
    TRACE_OUT
    return(0);
}

/*
 * check_cgone_dev(makedevs) - checks Sun-1 color graphics
 * interface device file (see cgone(4s) and MAKEDEV)
 */
check_cgone_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_cgone_dev";
    TRACE_IN
    sprintf(name, "/dev/cgone%d", unit);

    if (check_dev(makedevs, name, character, 14, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
	{
	    TRACE_OUT
            return(1);
	}
    TRACE_OUT
    return(0);
}

/*
 * check_cgtwo_dev(makedevs) - checks Sun-3/Sun-2 color graphics
 * interface device file (see cgtwo(4s) and MAKEDEV)
 */
check_cgtwo_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_cgtwo_dev";
    TRACE_IN
    sprintf(name, "/dev/cgtwo%d", unit);

    if (check_dev(makedevs, name, character, 31, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
 	{
	    TRACE_OUT
            return(1);
	}
    TRACE_OUT
    return(0);
}
 
/*
 * check_cgthree_dev(makedevs) - checks color graphics
 * interface device file (see MAKEDEV)
 */
check_cgthree_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_cgthree_dev";
    TRACE_IN
    sprintf(name, "/dev/cgthree%d", unit);

    if (check_dev(makedevs, name, character, 55, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
	{
	    TRACE_OUT
            return(1);
	}
    TRACE_OUT
    return(0);
}

/*
 * check_cgfour_dev(makedevs) - checks Sun-3 color graphics
 * interface device file (see cgfour(4s) and MAKEDEV)
 */
check_cgfour_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_cgfour_dev";
    TRACE_IN
    sprintf(name, "/dev/cgfour%d", unit);

    if (check_dev(makedevs, name, character, 39, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
	{
	    TRACE_OUT
            return(1);
	}
	
    TRACE_OUT
    return(0);
}

/*
 * check_cgfive_dev(makedevs) - checks Road Racer graphics
 * interface device file (see MAKEDEV)
 */
check_cgfive_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int unit;
{
    char name[22];

    func_name = "check_cgfive_dev";
    TRACE_IN
    sprintf(name, "/dev/cgfive%d", unit);

    if (check_dev(makedevs, name, character, 58, unit, "0666", no))
    if ((*status = check_fb(name)) != -1)
    {
	    TRACE_OUT
            return(1);
    }
    TRACE_OUT
    return(0);
}

/*
 * check_cgsix_dev(makedevs) - checks lego color graphics
 * interface device file (see MAKEDEV)
 */
check_cgsix_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_cgsix_dev";
    TRACE_IN
    sprintf(name, "/dev/cgsix%d", unit);

    if (check_dev(makedevs, name, character, 67, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
	{
	    TRACE_OUT
            return(1);
	}
    TRACE_OUT
    return(0);
}

/*
 * check_cgeight_dev(makedevs) - checks color graphics
 * interface device file (see MAKEDEV)
 */
check_cgeight_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_cgeight_dev";
    TRACE_IN
    sprintf(name, "/dev/cgeight%d", unit);

#ifndef sun386	
    if (check_dev(makedevs, name, character, 64, unit, "0666", no))
#else
    if (check_dev(makedevs, name, character, 63, unit, "0666", no))
#endif
	if ((*status = check_fb(name)) != -1)
 	{
	    TRACE_OUT
            return(1);
	}
    TRACE_OUT
    return(0);
}

/*
 * check_cgnine_dev(makedevs) - checks crane color graphics
 * interface device file (see MAKEDEV)
 */
check_cgnine_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_cgnine_dev";
    TRACE_IN
    sprintf(name, "/dev/cgnine%d", unit);

    if (check_dev(makedevs, name, character, 68, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
	{
	    TRACE_OUT
            return(1);
	}
    TRACE_OUT
    return(0);
}

/*
 * check_tvone_dev(makedevs) - checks flamingo
 * interface device file (see MAKEDEV)
 */
check_tvone_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_tvone_dev";
    TRACE_IN
    sprintf(name, "/dev/tvone%d", unit);

    if (check_dev(makedevs, name, character, 100, unit, "0666", no))
	if ((*status = check_fb(name)) != -1)
	{
	    TRACE_OUT
            return(1);
    	}
    TRACE_OUT
    return(0);
}

#ifndef SVR4
#define GPONE_DEVS      4
 
/*
 * check_gpone_dev(makedevs) - checks Sun-3/Sun-2 graphics processor
 * device file (see des(4s) and MAKEDEV)
 */
check_gpone_dev(makedevs)
    int makedevs;
{
/*
 *    struct pixrect *gppr;
 *    char *perrmsg;
 *    char buf[BUFSIZ-1];
 */
    int fminor, fd;
    char *name;
    u_long gp2_base=0;
 
    func_name = "check_gpone_dev";
    TRACE_IN
    for (fminor = 0; fminor < GPONE_DEVS; fminor++) {
	switch (fminor) {
	case 0:
	    name = "/dev/gpone0a";
	    break;
	case 1:
	    name = "/dev/gpone0b";
	    break;
	case 2:
	    name = "/dev/gpone0c";
	    break;
	case 3:
	    name = "/dev/gpone0d";
	    break;
	}
        if ((check_dev(makedevs, name, character, 32, fminor,
            "0666", no)) == 0)
	{
	    TRACE_OUT
            return(0);
	}
    }

    if (Map_bds(&fd, &gp2_base))
        test_address(&fd, &gp2_base); 

    TRACE_OUT
    return(gptype);
}

/* 4.0 compatibility */
#ifndef MAP_FIXED
#define MAP_FIXED       0
#endif
/*
 * from gp2_probe.c, in gp2test
 */
static int
Map_bds(fd, gp2_base)
    int *fd;
    u_long *gp2_base;
{
    caddr_t addr;
    int len = GP2_SIZE;
    int base = GP2_BASE;
    char *perrmsg;
    char buf[BUFSIZ-1];

    func_name = "Map_bds";
    TRACE_IN
    if ((*fd = open("/dev/vme24d32", O_RDWR)) < 0) {
      	send_message(0, ERROR,"cannot open /dev/vme24d32: %s",errmsg(errno)); 
	TRACE_OUT
      	return(0);
    }
 
    addr = valloc((unsigned)len);
    if ((mmap(addr, len, PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_FIXED, *fd, base)) < 0) {
      	send_message(0, ERROR,"mmap error for /dev/vme24d32: %s",errmsg(errno));
	TRACE_OUT
      	return(0);
    }
    *gp2_base = (u_long)addr;
    TRACE_OUT
    return(1);
}

/*
 * from gp2_probe.c, in gp2test
 */
static void
test_address(fd, gp2_base)
    int *fd;
    u_long *gp2_base;
{
    unsigned long *addr_ptr, data = 0;
    int osigs;
 
    func_name = "test_address";
    TRACE_IN
    osigs = sigsetmask(0);
    (void) signal(SIGBUS, bus_test_error);
    (void) signal(SIGSEGV, bus_test_error);
    setjmp(sjbuf);
    if (gptype != 1) {
        addr_ptr = (unsigned long *)(*gp2_base + SH_MEM_BASE_ADDR);
 
      /* here we test for a bus error */

        data = *addr_ptr;                    

        if (data != 0)
	    send_message(0, DEBUG, "Probe data = %ld", data);
        gptype = 2;
    }
    (void) sigsetmask(osigs);
    (void) close(*fd);
    TRACE_OUT
}
#endif SVR4

bus_test_error()
{
#ifdef SVR4
    int val = 0;
#endif SVR4
    func_name = "bus_test_error";
    TRACE_IN
    send_message (0, DEBUG, "at bus_test_error()");
    gptype = 1;
#ifdef SVR4
    longjmp(sjbuf, val);
#else SVR4
    longjmp(sjbuf);
#endif SVR4
    TRACE_OUT
}

/*
 * from David DiGiacomo
 * check for GP/GP2
 */
 /*
 * static int
 * isgp(pr)
 *     Pixrect *pr;
 * {
 *     short *id;
 * 
 *     if (pr->pr_ops->pro_rop != gp1_rop) 	   *//* not a GP *//*
 *
 *         return(0);
 * 
 *     if (!(id = (short *) valloc((unsigned) getpagesize())) ||
 *         mmap((caddr_t) id, getpagesize(), 
 *         PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED,
 *         gp1_d(pr)->ioctl_fd, 0) == (caddr_t) -1) *//* something is wrong *//*
 *         return(-1);
 * 
 *     if ((*id & GP1_ID_MASK) == GP1_ID_VALUE)      *//* GP or GP+ *//*
 *         return(1);
 * 
 *     if ((*id & GP2_ID_MASK) == GP2_ID_VALUE) 	   *//* GP2 *//*
 *         return(2);
 * 
 *     return(-1);   				   *//* what is it? *//*
 * }
 */
/*
 * from fbio(4s)
 */
static int
check_fb(name)
    char *name;
{
    int fd;
    struct fbtype fbt;
    char *perrmsg, buf[BUFSIZ-1];

    func_name = "check_fb";
    TRACE_IN
#if	0		/* -JCH- it will cause Bus Error */
    if ((fd = open(name, O_RDWR)) < 0) {
        send_message(0, ERROR, "%s open: %s", name, errmsg(errno)); 
	TRACE_OUT
	return(-1);
    }
    if (ioctl(fd, FBIOGTYPE, (char *)&fbt) < 0) {
        send_message(0, ERROR, "%s FBIOGTYPE ioctl: %s", name,errmsg(errno)); 
	TRACE_OUT
	return(-1);
    }
    (void) close(fd);
    TRACE_OUT
    return(fb_status(&fbt));
#endif

    TRACE_OUT
    return(1);		/* always */
}

/*
 * same as tape_status in probe_tapes.c
 */
static int
fb_status(fbt)
    struct fbtype *fbt;
{
    char buf[BUFSIZ-1];
    struct fb_desc *fb;

    func_name = "fb_status";
    TRACE_IN
    for (fb = frame_buffs; (fb->fb_type < FBTYPE_LASTPLUSONE); fb++)
	if (fb->fb_type == fbt->fb_type)
	    break;
    if (fb->fb_type == 0) {
        send_message(0, ERROR, "unknown frame buffer type (%d)", fb->fb_type);
	TRACE_OUT
        return(-1);
    }
    if ((fb->fb_type >= 0) &&
	(fb->fb_type < FBTYPE_LASTPLUSONE))
	send_message (0, DEBUG, "%s frame buffer type", fb->fb_name);
    TRACE_OUT
    return(fbt->fb_type);
}

/*
 * check_taac_dev(makedevs) - checks the taac graphics
 * device file (see taac(4s) and MAKEDEV)
 */
check_taac_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int	unit;
{
    char name[22];

    func_name = "check_taac_dev";
    TRACE_IN
    sprintf(name, "/dev/taac%d", unit);

    if (check_dev(makedevs, name, character, 62, unit, "0666", no))
    {
	TRACE_OUT
        return(1);
    }
    TRACE_OUT
    return(0);
}

/*
 * check_cgtwelve_dev(makedevs, status, unit) - checks the Egret graphics
 * acceleration device.  Egret is a combination of cg9 and gp2 functionally.
 *
 */
check_cgtwelve_dev(makedevs, status, unit)
    int makedevs;
    int *status;
    int unit;
{
    char name[22];
  
    func_name = "check_cgtwelve_dev";

    TRACE_IN

    sprintf(name, "/dev/cgtwelve%d", unit);

    if (check_dev(makedevs, name, character, 102, unit, "0666", no))
    {
	TRACE_OUT
	return(1);
    }
    TRACE_OUT
    return(0);
}

