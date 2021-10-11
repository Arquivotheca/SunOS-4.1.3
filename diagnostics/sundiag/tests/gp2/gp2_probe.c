#ifndef lint
static	char sccsid[] = "@(#)gp2_probe.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include "gp2test.h"
#include "gp2test_msgs.h"
#include "sdrtns.h"

extern char *valloc();
extern char *sprintf();
extern int CgTWO;

#define SH_MEM_BASE_ADDR	0x400
#define GP2_SIZE		0x40000
#define GP2_BASE 		0x240000      	/* base address of gp2 */

static int Map_bds();
static void test_address();
static char cgname[80];
bus_test_error();
/*
 *************************************************************************
 *	Check to see if GP2 board exists.
 *	if not exit test. if it does the test will continue.
 *************************************************************************
 */
check_gp()
{
    int fd, return_code = 0;
    u_long gp2_base=0;

    getfb();
    if (CgTWO) 
        strcpy(cgname, CG_DEV); 
    else 
        strcpy(cgname, CR_DEV);

    if (access(GP_DEV, F_OK) != 0)
        return(NO_GP_DEV);
    else {
        if ((return_code = Map_bds(&fd, &gp2_base)) != 0)
	    return(return_code);
    }
    if (access(cgname, F_OK) != 0)
        return(NO_CG_DEV);
    else
	return_code = CG_ONLY;
    test_address(&fd, &gp2_base);
    return(return_code);
}

/* 4.0 compatibility */
#ifndef MAP_FIXED
#define MAP_FIXED       0
#endif
/*
 *************************************************************************
 *	Map in the GP2 board using valloc() and mmap() calls to force the
 *	mmu to specificly map in the gp2 board.
 *	Returns the virtual address if routine passes, returns and error
 *	code if not.
 *************************************************************************
 */
static int
Map_bds(fd, gp2_base)
    int *fd;
    u_long *gp2_base;
{
    caddr_t addr;
    int len = GP2_SIZE;
    int base = GP2_BASE;

    if ((*fd = open(VME_DEV, O_RDWR)) < 0) 
    	return(OPEN_ERROR); 				/* error */

    addr = valloc((unsigned)len);
    if ((mmap(addr, len, PROT_READ | PROT_WRITE, 
	MAP_SHARED | MAP_FIXED, *fd, base)) < 0)
        return(MMAP_ERROR); 				/* error */
    *gp2_base = (u_long)addr;
    return(0);
}

/*
 *************************************************************************
 *	Write to the first address in shared ram. 
 *	If no buss error ocures the gp2 board is pressent.
 *	IF a buss error occures the buss_test_error() routine is executed.
 *************************************************************************
 */
static void
test_address(fd, gp2_base)
    int *fd;
    u_long *gp2_base;
{
    unsigned long *addr_ptr, data = 0;
    int osigs;

    osigs = sigsetmask(0);
    (void) signal(SIGBUS, bus_test_error);
    (void) signal(SIGSEGV, bus_test_error);
    addr_ptr = (unsigned long *)(*gp2_base + SH_MEM_BASE_ADDR);

      /* here we test for a bus error */
 
    if ((data = *addr_ptr) != 0) {
	(void) sprintf(msg, "Probe data = %d\n", data);
	gp_send_message(0, DEBUG, msg);
    }
    (void) sigsetmask(osigs);
    (void) close(*fd);
}
/* 
 ************************************************************************* 
 *	If a buss error ocures during the above function restore the
 *	signals and exit the gp2test with an error code to Sundiag.
 *************************************************************************
 */
bus_test_error()
{

        gp_send_message(-NO_GP_DEV, FATAL, gp2_open_msg);
	reset_signals();
        exit(NO_GP_DEV);      /* return code for Sundiag  */
 
}
/* 
 ************************************************************************* 
 *	This function is to determine if the test is running on single
 *	headed system or dual headed system (mono fb and cg5), and 
 *	if the cgtwo0 dev has been created.  
 *	If no cg5 exit.
 *	Return a code if console is mono, or color.
 *************************************************************************
 */
probe_cg()
{
    int return_code;
    struct       fbtype  fb_type;
    int tmpfd, ioret;

    if ((tmpfd=open(FB_DEV, O_RDWR)) != -1) {
    	if ((ioret = ioctl(tmpfd, FBIOGTYPE, &fb_type)) < 0)
	    return(ioret);
    } else
        return(tmpfd);

    return_code = fb_type.fb_type ? MONO_ONLY : NO_FB;

    if (!access(cgname, 0))
        return_code += CG_ONLY;

    (void) close (tmpfd);
    return(return_code);
}
