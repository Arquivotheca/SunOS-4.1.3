/*
 * Color24
 * 
 * Date: 06/14/88
 * 
 * Sundiag test for offboard 24 bit color frame buffers.  This sysdiag test not
 * only verifies the hardware, but also the device independant driver
 * routines.  This diagnostic locks suntools from writting to the screen
 * during this test.
 * 
 * 06-14-88   BKA      Created version 1.0.
 * 
 * Released OS: 4.0 and higher.
 * 
 * Drivers used: cgeight0.
 * 
 * Compile line: 
 *   cc color24.c -O1 -Bstatic -o c24 -lsuntool -lsunwindow -lpixrect -lm -lc
 * 
 * External files used: color24.h color24.icon
 */
static char     sccsid[] = "@(#)c24.c 1.1 92/07/30 SMI";

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sun/fbio.h>
#include <sgtty.h>
#include <math.h>

#include <pixrect/pixrect_hs.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_planegroups.h>

#include <sunwindow/win_cursor.h>
#include <suntool/sunview.h>
#include <suntool/gfx_hs.h>
#include "color24.h"

#ifndef PIXPG_24BIT_COLOR
#define PIXPG_24BIT_COLOR    5
#endif

#ifndef FBTYPE_SUNROP_COLOR
#define FBTYPE_SUNROP_COLOR  13
#endif

#ifndef FBTYPE_MEMCOLOR
#define FBTYPE_MEMCOLOR       7
#endif

#ifndef PR_FORCE_UPDATE
#define PR_FORCE_UPDATE (1 << 24)
#endif

#ifndef pr_getlut
#define pr_getlut(pr, ind, cnt, red, grn, blu )  \
	(*(pr)->pr_ops->pro_getcolormap)(pr, PR_FORCE_UPDATE | ind, cnt, red, grn, blu)
#endif

#ifndef pr_putlut
#define pr_putlut(pr, ind, cnt, red, grn, blu )  \
	(*(pr)->pr_ops->pro_putcolormap)(pr, PR_FORCE_UPDATE | ind, cnt, red, grn, blu)
#endif

#define PIXPG_COLOR PIXPG_24BIT_COLOR

#define DEV_NOT_OPEN            3
#define RED_FAILED              4
#define GREEN_FAILED            5
#define BLUE_FAILED             6
#define FRAME_BUFFER_ERROR      7
#define SIGBUS_ERROR            8
#define SIGSEGV_ERROR           9
#define SCREEN_NOT_OPEN         10
#define END_ERROR               11
#define SYNC_ERROR              12

#include "sdrtns.h"
#include "../../../lib/include/libonline.h"


#define NO_FB           0
#define MONO_ONLY       10
#define CG_ONLY         2
#define CG_MONO_AVAIL   12

#define BUFSIZE  256
#define NDOTS  10000

extern char *getenv();

struct fbtype   fb_type;
int             rootfd, winfd, devtopfd, cg_type, tmpplane, width, height;
int		depth, iograb = FALSE, current_plane_group;
char   		*cg_driver, msg[132];
unsigned char   red[256], green[256], blue[256];
unsigned char   tred[256], tgrn[256], tblue[256];
struct screen   new_screen;
Pixrect        *prfd, *sdpr, *sepr, *twentyfourdump, *twentyfourexp;
Pixrect        *oneexp, *onedump, *regionpr, *regionmm;
short          *sdi, *sde;

int             clr[] = {RED, GREEN, BLUE, YELLOW, BLACK, CYAN, MAGENTA,
                         ORANGE, BROWN, DARKGREEN, GRAY, LIGHTGRAY, DARKGRAY,
                         PURPLE, PINK, LIGHTGREEN};

int		cg9 = 0;
int             SpecialFlag, defaultfb = FALSE;
struct sgttyb   ttybuf;
short		orig_sg_flags;		/* mode flags from stdin */
int             origsigs;
int Colormap(), Rop(), Vector(), Putget(), Region(), Destroy(), Stencil();
int verify_overlay_enable(), verify_clr_map(), verify_rw_memory(), verify_db_memory();
char            tmpbuf[BUFSIZE];

/****************************************************/
/****************************************************/
/* Create variables ibisiconpr and ibisiconpr_data  */
/****************************************************/
/****************************************************/
static short    icondata[] = {
#include "color24.icon"
};

mpr_static(ibisiconpr, 64, 64, 1, icondata);
int dummy(){return FALSE;}



/****************************************************/
/* Main                                             */
/* */
/* Should I say more.                             */
/* */
/****************************************************/
main(argc, argv)
int argc;
char **argv;
{
extern int  process_my_args();

   versionid = "1.1";    /* SCCS version id */
			/* begin Sundiag test */
   test_init(argc, argv, process_my_args, dummy, (char *)NULL);
   
   SpecialFlag = FALSE;
   setup_signals();

	/* set driver and frame buffer type accordingly */
	cg_driver = (cg9) ? "/dev/cgnine0" : "/dev/cgeight0";
	cg_type = (cg9) ? FBTYPE_SUNROP_COLOR : FBTYPE_MEMCOLOR;

   device_name = cg_driver;

   if (probe_cg() == NO_FB) {
		clean_up();
		exit(0);
   }
   setup_desktop();
   initialize();

   /* verify that all device independent */
   /* routines are functioning correctly */
   verify_drvr_fun();

   /* verify color map using read /write */
   /* tests.                             */
   cg_lock_scr(verify_clr_map);

   /* verify overlay and enable memories */
   cg_lock_scr(verify_overlay_enable);

   /* verify read / write of color,      */
   /* overlay, and enable memories.      */
   cg_lock_scr(verify_rw_memory);

   /* verify double buffering if cg9 */
	if (cg9)
		cg_lock_scr(verify_db_memory);

	resetfb();
   clean_up();
   test_end();		/* Sundiag normal exit */
}



/****************************************/
/* Process_my_args                      */
/* */
/* Check the command line for the     */
/* argument 'diag'.  If diag is present */
/* enable XOR'ing of screen pixrect with */
/* expected pixrect on failure.         */
/* */
/****************************************/
process_my_args(argv, index)
char **argv;
{
   int match;

   match = FALSE;
   if (strncmp(argv[index], "diag", 4) == 0)
		SpecialFlag = match = TRUE;
	if (strncmp(argv[index], "9", 1) == 0) 
		cg9 = match = TRUE;
   return (match);
}

/************************************************/
/* Initialize                                   */
/* */
/* Create all memory pixrects to              */
/* verify against the screen pixrect            */
/* */
/* GLOBAL VARIABLES:                            */
/* */
/* sdpr - XOR pixrect.                        */
/* sepr - Memory pixrect (expected pixrect)   */
/* sdi  - Image area of sdpr.                 */
/* sde  - Image area of sepr.                 */
/* */
/* twentyfourexp  - 24 bit pixrect (expected) */
/* twentyfourdump - 24 bit screen dump pixrect */
/* oneexp         - 1  bit pixrect (expected) */
/* onedump        - 1  bit screen dump pixrect */
/* */
/************************************************/
initialize()
{
    struct mpr_data *temp;

    if (SpecialFlag) {
	if ((int) (sdpr = mem_create(width, height, depth)) == 0)
	    Error(-DEV_NOT_OPEN, "Failed to create memory pixrect \n", FALSE);
    } else
	sdpr = prfd;

    if ((int) (sepr = mem_create(width, height, depth)) == 0)
	Error(-DEV_NOT_OPEN, "Failed to create memory pixrect \n", FALSE);

    twentyfourexp = sepr;
    twentyfourdump = sdpr;

    temp = (struct mpr_data *) sdpr->pr_data;
    sdi = temp->md_image;

    temp = (struct mpr_data *) sepr->pr_data;
    sde = temp->md_image;

    if ((int) (oneexp = mem_create(width, height, 1)) == 0)
	Error(-DEV_NOT_OPEN, "Failed to create memory pixrect \n", FALSE);

    if ((int) (onedump = mem_create(width, height, 1)) == 0)
	Error(-DEV_NOT_OPEN, "Failed to create memory pixrect \n", FALSE);

	if (cg9) {
		pr_dbl_set(prfd, PR_DBL_WRITE, PR_DBL_BOTH, PR_DBL_READ, PR_DBL_B, 0);
	}

}



/******************************************************/
/* Probe_cg                                           */
/* */
/* Determine if cg8 is installed on system and      */
/* return the following codes:                        */
/* */
/* 0       No cg8 board                          */
/* 2       cg8 installed                         */
/* */
/* GLOBAL VARIABLES:                                  */
/* */
/* return_code  - indicates cg8 installed or not.   */
/* */
/******************************************************/
probe_cg()
{
    int             tmpfd, return_code;
    struct          fbgattr fb;

    if ((tmpfd = open("/dev/fb", O_RDWR)) != -1)
	ioctl(tmpfd, FBIOGATTR, &fb);
    else
	return (tmpfd);

    return_code = NO_FB;
    fb_type.fb_type = fb.fbtype.fb_type;

    if ( cg9 )
	if ( fb_type.fb_type == FBTYPE_SUN2GP ) cg_type = FBTYPE_SUN2GP;

#ifdef NOWAY
    if ( fb_type.fb_type == cg_type ) {
      if ( ( prfd = pr_open (cg_driver) ) <= 0 )
               syserror(-DEV_NOT_OPEN, "/dev/fb");
    } else if ((prfd = pr_open(cg_driver)) <= 0)
               return_code = NO_FB;
#else
    if ((prfd = pr_open (cg_driver) ) <= 0 )
		return_code = NO_FB;
#endif

    if ( prfd > 0 ) return_code = CG_ONLY;

    close(tmpfd);

    return (return_code);
}


/**********************************************************/
/* Setup_desktop                                          */
/* */
/* Disable suntools, cursor, and mouse during the lapse */
/* of this test.  Save color look up table and enable all */
/* color planes.                                          */
/* */
/**********************************************************/
setup_desktop()
{
    Cursor          cursor;
    struct inputmask im;
    char           *winname;

    u_long          planes;

    if ((fb_type.fb_type == cg_type) && (winname = getenv("WINDOW_PARENT"))) {
	rootfd = open(winname, O_RDONLY);
	if (rootfd == 0)
	    syserror(-DEV_NOT_OPEN, "rootfd");
	winfd = win_getnewwindow();
	if (winfd < 0)
	    syserror(-SCREEN_NOT_OPEN, "winfd");
	win_setlink(winfd, WL_PARENT, win_nametonumber(winname));
	win_setlink(winfd, WL_OLDERSIB, win_getlink(rootfd, WL_YOUNGESTCHILD));
	win_screenget(rootfd, &new_screen);
	win_setrect(winfd, &new_screen.scr_rect);

	input_imnull(&im);
	im.im_flags |= IM_ASCII;        /* catch 0-127(CTRL-C == 3) */

	win_set_kbd_mask(winfd, &im);
	win_insert(winfd);

                /* Prevent any cursor from display.
                 * This must be done because the cursor
                 * could sneak into the display.
                 */

                /* remove the new window cursor */
                cursor = cursor_create((char *)0);
                win_getcursor(rootfd, cursor);
                cursor_set(cursor, CURSOR_SHOW_CURSOR, 0, 0);
                win_setcursor(rootfd, cursor);
                cursor_copy(cursor);
                cursor_destroy(cursor);
		win_grabio(winfd);
 
                check_input();
	win_remove_input_device(rootfd, new_screen.scr_msname);
	iograb = TRUE;
    }
    enable_video();
    width = prfd->pr_width;
    height = prfd->pr_height;
    depth = 32;				       /* prfd->pr_depth; */

    pr_getattributes(prfd, &tmpplane);
    pr_getlut(prfd, 0, 256, tred, tgrn, tblue);

    planes = 0xFFFFFF;
    pr_putattributes(prfd, &planes);

}



/************************************************************/
/* Enable_video                                             */
/* */
/* Turn on video if it is not already on.                 */
/* */
/************************************************************/
enable_video()
{
    int             fd, vid;

    vid = FBVIDEO_ON;

    fd = open(cg_driver, O_RDWR);
    if (fd) {
	ioctl(fd, FBIOSVIDEO, &vid, 0);
	close(fd);
    } else
	syserror(-DEV_NOT_OPEN, "could not turn on video");
}

/****************************************************/
/* Syserror                                         */
/* */
/* Display error message when screen is unable to */
/* open.                                            */
/* */
/****************************************************/
syserror(err, c)
    int             err;
    char           *c;

{

    perror(c);
    sprintf(msg, "could not create new screen for '%s'\n", cg_driver );
    if (!exec_by_sundiag) {
	printf(msg);
	printf("\n");
	clean_up();
	exit(0);
    } else
	Error(err, msg, FALSE);
}



/*****************************************************/
/* Setup_signals                                     */
/* */
/* Catch all interrupts and determine if it is a   */
/* user interrupt, a buserror, or a segmentation     */
/* violation.                                        */
/* */
/* INTERRUPT HANDLERS:                               */
/* */
/* Finish    - User interrupts status OK           */
/* Bus       - Bus error status failed.            */
/* Segv      - Segmentation violation status failed */
/* */
/*****************************************************/
setup_signals()
{

    ioctl(0, TIOCGETP, &ttybuf);
    orig_sg_flags = ttybuf.sg_flags;           /* save original mode flags */
    ttybuf.sg_flags |= CBREAK;		       /* non-block mode */
    ttybuf.sg_flags &= ~ECHO;		       /* no echo */
    ioctl(0, TIOCSETP, &ttybuf);

    origsigs = sigsetmask(0);
    signal(SIGINT, finish);
    signal(SIGQUIT, finish);
    signal(SIGHUP, finish);
}

/**********************************************************/
/* Reset_signals                                          */
/* */
/* Reset all interrupt routines back to the state before */
/* the test started.                                      */
/* */
/**********************************************************/
reset_signals()
{
    ioctl(0, TIOCGETP, &ttybuf);
    ttybuf.sg_flags = orig_sg_flags;	/* restore original mode flags */
    ioctl(0, TIOCSETP, &ttybuf);

    /* set up original signals */

    sigsetmask(origsigs);
    signal ( SIGINT, SIG_IGN );
    signal ( SIGQUIT, SIG_IGN );
    signal ( SIGHUP, SIG_IGN );
    /* flush the buffer */

    flush_io();
}

/*********************************************************/
/* Flush_io                                              */
/* */
/* Clear the input and output streams.                 */
/* */
/*********************************************************/
flush_io()
{
    int	tfd, knt, arg = 0;

    tfd = devtopfd ? devtopfd : 0;
    ioctl(tfd, FIONREAD, &arg);

    knt = BUFSIZE;
    while (arg > 0) {
	if (arg < knt)
	    knt = arg;

	knt = read(tfd, tmpbuf, knt);
	arg -= knt;
    }
}



/*******************************************************/
/* */
/* Verify_overlay_enable                               */
/* Verify that you can rop into the overlay and      */
/* enable planes.  This is a quick functional test.    */
/* */
/*******************************************************/
verify_overlay_enable()
{
    do_set_plane_group(PIXPG_OVERLAY_ENABLE);
    pr_rop(prfd, width/4, height/4, width/2, height/2, 
           PIX_SRC | PIX_COLOR(1), 0, 0, 0);
    do_set_plane_group(PIXPG_OVERLAY);
    red[0] = 0;
    blue[0] = 0;
    green[0] = 0;
    red[1] = 0xFF;
    blue[1] = 0xFF;
    green[1] = 0xFF;

    pr_putlut(prfd, 0, 2, red, green, blue);

    pr_rop(prfd, 0, 0, width, height, PIX_SRC | PIX_COLOR(0), 0, 0, 0);

    pr_rop(prfd, width/4, height/4, width/2, height/2, 
           PIX_SRC | PIX_COLOR(1), 0, 0, 0);

    do_set_plane_group(PIXPG_COLOR);

}




/*******************************************************/
/* Verify_drvr_fun                                     */
/* */
/* Verify all the device independant driver routines. */
/* Not only does this test the integrity of the device */
/* driver, but it also does some functional tests on   */
/* the frame buffer.                                   */
/* */
/*******************************************************/
verify_drvr_fun()
{

    /* set current pixrect to color plane */
    do_set_plane_group(PIXPG_OVERLAY_ENABLE);
    pr_rop(prfd, 0, 0, width, height, (PIX_SRC | PIX_COLOR(0)), 0, 0, 0);
    do_set_plane_group(PIXPG_OVERLAY);
    pr_rop(prfd, 0, 0, width, height, (PIX_SRC | PIX_COLOR(0)), 0, 0, 0);
    do_set_plane_group(PIXPG_COLOR);

    /* Begin testing of the frame buffer and driver routines */
    cg_lock_scr(Colormap);
    cg_lock_scr(Rop);
    cg_lock_scr(Vector);
    cg_lock_scr(Putget);
    cg_lock_scr(Region);
    cg_lock_scr(Destroy);
    cg_lock_scr(Stencil);

}

/***************************************************/
/* Colormap                                        */
/* */
/* Verify pr_putcolormap, pr_getcolormap, and test */
/* the color look up table.                        */
/* */
/***************************************************/
Colormap()
{
    int             count;
    u_char          *r, *g, *b;

    pr_set_plane_group ( prfd, PIXPG_24BIT_COLOR );
    for (count = 0; count < 256; count++) {
	red[count] = count;
	blue[count] = count;
	green[count] = count;
    }

    r = red; b = blue; g = green;
    pr_putlut(prfd, 0, 256, r, g, b);
    pr_getlut(prfd, 0, 256, red, green, blue);

    for (count = 0; count < 256; count++) {
	if (red[count] != count)
	    Error(-RED_FAILED, "Red color LUT failed r/w \n", FALSE);
	if (blue[count] != count)
	    Error(-BLUE_FAILED, "Blue color LUT failed r/w \n", FALSE);
	if (green[count] != count)
	    Error(-GREEN_FAILED, "Green color LUT failed r/w \n", FALSE);
    }
}



/*******************************************************/
/* Rop                                                 */
/* */
/* Rop into color memory different colors moving the */
/* start location to the right and down.  Test cliping */
/* at the lower right hand corner.  Verification of    */
/* memory is done here.                                */
/* */
/*******************************************************/
Rop()
{
    int             x, y, xincr, yincr, count;

    do_rop(0, 0, width, height, (PIX_SRC | PIX_COLOR(WHITE)), 0, 0, 0);

    xincr = width / 16;
    yincr = height / 16;
    x = xincr;
    y = yincr;

    for (count = 0; count < 16; count++, x += xincr, y += yincr) 
       do_rop(x, y, width, height, (PIX_SRC | PIX_COLOR(clr[count])), 0, 0, 0);

    if (!check_screen())
	Error(-FRAME_BUFFER_ERROR, "Roping into frame buffer failed \n", TRUE);
}

/***********************************************************/
/* Vector                                                  */
/* */
/* Draw three stars using vectors.  One star is at the   */
/* upper left corner of the screen and is half drawn off   */
/* the screen, this helps verify clipping.                 */
/* */
/***********************************************************/
Vector()
{

    do_rop(0, 0, width, height, (PIX_SRC | PIX_COLOR(WHITE)), 0, 0, 0);

    do_star(300.0, 576, 450, (PIX_SRC | PIX_COLOR(CYAN)));
    do_star(200.0, 50, 50, (PIX_SRC | PIX_COLOR(BLUE)));
    do_star(200.0, (width - 50), (height - 50), (PIX_SRC | PIX_COLOR(RED)));

    if (!check_screen())
	Error(-FRAME_BUFFER_ERROR, "Vector drawing into frame buffer failed \n",TRUE);

}



/***********************************************************/
/* Putget                                                  */
/* */
/* Verify that the 24 bit frame buffer can read and set  */
/* pixels using the driver routines pr_put and pr_get.     */
/* Random colors and random pixels are used.               */
/* */
/***********************************************************/
Putget()
{
    int             c, x, y, element, temp;

    srandom(time(0));
    do_rop(0, 0, width, height, (PIX_SRC | PIX_COLOR(WHITE)), 0, 0, 0);

    temp = depth;
    if (depth > 24)
	temp = 24;
    for (element = 0; element < NDOTS; element++) {
	x = random() % width;
	y = random() % height;
	c = random() % ((1 << temp) - 1);

	do_put(x, y, c);
	if (pr_get(prfd, x, y) != c)
	    Error(-FRAME_BUFFER_ERROR, "Getting from frame buffer failed\n", TRUE);
    }

    if (!check_screen())
	Error(-FRAME_BUFFER_ERROR, "Putting to frame buffer failed \n", TRUE);
}

/*****************************************************************/
/* Region                                                        */
/* */
/* Verify that a sub region can be created, then verify that   */
/* a rop can take place, followed by a rop greater in size to    */
/* verify clipping.                                              */
/* */
/*****************************************************************/
Region()
{
    int             x, y, mx, my;

    do_rop(0, 0, width, height, (PIX_SRC | PIX_COLOR(WHITE)), 0, 0, 0);
    mx = width / 2;
    my = height / 2;

    do_rop(mx - 300, my - 100, 200, 200, (PIX_SRC | PIX_COLOR(YELLOW)), 0, 0, 0);
    do_rop(mx - 100, my - 300, 200, 200, (PIX_SRC | PIX_COLOR(YELLOW)), 0, 0, 0);
    do_rop(mx + 100, my - 100, 200, 200, (PIX_SRC | PIX_COLOR(YELLOW)), 0, 0, 0);
    do_rop(mx - 100, my + 100, 200, 200, (PIX_SRC | PIX_COLOR(YELLOW)), 0, 0, 0);

    x = mx - 100;
    y = my - 100;

    regionpr = pr_region(prfd, x, y, 200, 200);
    regionmm = pr_region(sepr, x, y, 200, 200);

    pr_rop(regionpr, 10, 10, 180, 180, (PIX_SRC | PIX_COLOR(BLUE)), 0, 0, 0);
    pr_rop(regionmm, 10, 10, 180, 180, (PIX_SRC | PIX_COLOR(BLUE)), 0, 0, 0);

    if (!check_screen())
	Error(-FRAME_BUFFER_ERROR, "Writing into region failed \n", TRUE);

    pr_rop(regionpr, -100, -100, width, height, (PIX_SRC | PIX_COLOR(RED)), 0, 0, 0);
    pr_rop(regionmm, -100, -100, width, height, (PIX_SRC | PIX_COLOR(RED)), 0, 0, 0);

    if (!check_screen())
	Error(-FRAME_BUFFER_ERROR, "Writing into region with clipping failed \n", TRUE);

}



/*********************************************************/
/* Stencil                                               */
/* */
/* Verify the driver specific function of stenciling   */
/* an Icon into the four corners and the middle of the   */
/* screen.  Clipping is verified at the corners.         */
/* */
/*********************************************************/
Stencil()
{
    int             mx, my;

    do_rop(0, 0, width, height, (PIX_SRC | PIX_COLOR(WHITE)), 0, 0, 0);

    do_stencil(-8, -16, 64, 64, (PIX_SRC | PIX_COLOR(BLUE)), &ibisiconpr,
	       0, 0, 0, 0, 0);

    do_stencil(width - 56, -16, 64, 64, (PIX_SRC | PIX_COLOR(BLUE)), &ibisiconpr,
	       0, 0, 0, 0, 0);

    do_stencil(-8, height - 48, 64, 64, (PIX_SRC | PIX_COLOR(BLUE)), &ibisiconpr,
	       0, 0, 0, 0, 0);

    do_stencil(width - 56, height - 48, 64, 64, (PIX_SRC | PIX_COLOR(BLUE)), &ibisiconpr,
	       0, 0, 0, 0, 0);

    mx = width / 2;
    my = height / 2;

    do_rop(mx - 64, my - 64, 128, 128, (PIX_SRC | PIX_COLOR(RED)), 0, 0, 0);
    do_stencil(mx - 32, my - 32, 64, 64, (PIX_SRC | PIX_COLOR(BLUE)), &ibisiconpr,
	       0, 0, 0, 0, 0);

    if (!check_screen())
	Error(-FRAME_BUFFER_ERROR, "Stenciling into frame buffer failed \n", TRUE);


}

/***********************************************************/
/* Destroy                                                 */
/* */
/* Verify that the region pixrects can be destroyed      */
/* without error.                                          */
/* */
/***********************************************************/
Destroy()
{
    pr_destroy(regionmm);
    pr_destroy(regionpr);
    if (!check_screen())
	Error(-FRAME_BUFFER_ERROR, "Pixrect destroy failed \n", TRUE);
}



/**********************************************************/
/* Do_set_plane_group                                     */
/* */
/* Set current plane to write into.                     */
/* */
/* PARAMETERS:                                            */
/* */
/* Plane  - specific plane to use can be:               */
/* PIXPG_COLOR                                */
/* PIXPG_OVERLAY                              */
/* PIXPG_OVERLAY_ENABLE                       */
/* */
/**********************************************************/
do_set_plane_group(plane)
    int             plane;

{
    struct mpr_data *temp;

    switch (plane) {
    case PIXPG_COLOR:
	pr_set_plane_group(prfd, plane);
	sdpr = twentyfourdump;
	sepr = twentyfourexp;
	break;
    case PIXPG_OVERLAY:
    case PIXPG_OVERLAY_ENABLE:
	pr_set_plane_group(prfd, plane);
	sdpr = onedump;
	sepr = oneexp;
	break;
    }
    temp = (struct mpr_data *) sdpr->pr_data;
    sdi = temp->md_image;

    temp = (struct mpr_data *) sepr->pr_data;
    sde = temp->md_image;

    depth = prfd->pr_depth;
}



/******************************************************/
/* Do_star                                            */
/* */
/* Do_star draws many vectors beginning at a central */
/* location with each consectitive vector being 10    */
/* degrees off shifted from the previous.             */
/* */
/******************************************************/
do_star(radius, cx, cy, op)
    double          radius;
    int             cx, cy, op;

{
    int             ix, iy;
    double          x, y, angle;

    for (angle = 0; angle < 6.283185307; angle += 0.31415926) {
	x = cos(angle) * radius;
	y = sin(angle) * radius;
	ix = (int) x;
	iy = (int) y;
	do_vector(cx, cy, cx + ix, cy + iy, op, 0);
    }
}



/****************************************************/
/* Verify_clr_map                                   */
/* */
/* Verify color map tests.  Write all three DACs  */
/* with values ranging from 0 to 255.  Display 3    */
/* rows of color, beginning with red, green, then   */
/* blue.  Verification of memory to ramdac mapping  */
/* */
/****************************************************/
#define SQR(s) ((double)((s)*1.0 * (s)*1.0))
#define XBGR(r,g,b) (((b)<<16) + ((g)<<8) + (r))

verify_clr_map()
{
    int             state, level, cr, temp, color, x, y;

    do_rop(0, 0, width, height, (PIX_SRC | PIX_COLOR(0)), 0, 0, 0);

    for (state = 0; state < 3; state++) {
	for (level = 0; level < 8; level++) {
	    for (cr = 0; cr < 32; cr++) {
		temp = cr + level * 32;
		switch (state) {
		case 0:
		    color = XBGR(temp, 0, 0);
		    break;
		case 1:
		    color = XBGR(0, temp, 0);
		    break;
		case 2:
		    color = XBGR(0, 0, temp);
		    break;
		}
		x = cr * 36;
		y = state * 256 + level * 32;
		do_rop(x, y, 36, 32, (PIX_SRC | PIX_COLOR(color)), 0, 0, 0);
	    }
	}
    }
}



/*********************************************************/
/* Verify_rw_memory                                      */
/* */
/* Verify color, overlay, and enable memory by writing */
/* a modulating pattern and then comparing the results.  */
/* */
/*********************************************************/
verify_rw_memory()
{

    do_set_plane_group(PIXPG_COLOR);
    memtest();

    do_set_plane_group(PIXPG_OVERLAY);
    memtest();

    do_set_plane_group(PIXPG_OVERLAY_ENABLE);
    memtest();

}

/*******************************************************/
/* Memtest                                             */
/* */
/* This routine actually does the grunt work in the  */
/* memory tests.  It is assumed that the frame buffer  */
/* has the correct memory assigned by set_plane_group  */
/* before calling this routine.                        */
/* */
/*******************************************************/
memtest()
{
    u_long         *ptr, *eptr, *sptr;
    u_long          pattern, mask;
    u_long          size, data;
    struct mpr_data *temp;

    temp = (struct mpr_data *) prfd->pr_data;
    ptr = (u_long *) temp->md_image;

    sptr = ptr;
    size = width * height * depth / 8;

    eptr = (u_long *) ((int) sptr + size);
    pattern = 0x55555555;
    mask = (depth == 32) ? 0xFFFFFF : 0xFFFFFFFF;

    while (sptr < eptr) {
	*sptr++ = pattern;
	pattern = ~pattern;
    }

    /* Check Frame buffer memory */
    pattern = 0x55555555;
    sptr = ptr;

    while (sptr < eptr) {
	data = *sptr++;
	if ((data & mask) != (pattern & mask))
	    Error(-FRAME_BUFFER_ERROR, "Memory verification failed \n", FALSE);
	pattern = ~pattern;
    }
}



/***************************************************/
/* verify double buffering functionality           */
/*                                                 */
/* perform memory write-reads into each buffer     */
/***************************************************/
verify_db_memory()
{

	resetfb();
   do_set_plane_group(PIXPG_COLOR);
	if (pr_dbl_get(prfd, PR_DBL_AVAIL) != PR_DBL_EXISTS)
		Error(-FRAME_BUFFER_ERROR, "Double buffer availability verification failed \n", FALSE);
	else {
		pr_dbl_set(prfd, PR_DBL_WRITE, PR_DBL_A, 0);
		pr_rop(prfd, 0, 0, width, height, (PIX_SRC|PIX_COLOR(0)), 0, 0, 0);
		pr_dbl_set(prfd, PR_DBL_WRITE, PR_DBL_B, 0);
		pr_rop(prfd, 0, 0, width, height, (PIX_SRC|PIX_COLOR(0xFFFFFF)), 0, 0, 0);
		pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A, PR_DBL_READ, PR_DBL_A, 0);
		read_db(0);
      pr_dbl_set(prfd, PR_DBL_READ, PR_DBL_B, 0);
	/* pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_B, 0); */
		read_db(0xF0F0F0);
	}
}



/***************************************************/
/* read back and compare double buffer data        */
/***************************************************/
read_db(data_cmp)
u_long data_cmp;
{ 
	register u_long *ptr, *ptr_limit;
	u_long data_in, offset;
	struct mpr_data *temp;

	temp = (struct mpr_data *) prfd->pr_data;
   ptr = (u_long *) temp->md_image;
	ptr_limit = ptr + (1152 * 900);
	while (ptr < ptr_limit)
		if ((*ptr++ & 0xF0F0F0) != data_cmp)
			Error(-FRAME_BUFFER_ERROR, "Double buffer verification failed \n", FALSE);
}
	


/****************************************************/
/* Resetfb                                          */
/* */
/* Reset the frame buffer back to 24 bit mode     */
/* and clear the enable plane.                      */
/* */
/****************************************************/
resetfb()
{

    do_set_plane_group(PIXPG_OVERLAY_ENABLE);
    do_rop(0, 0, 1152, 900, PIX_SRC, 0, 0, 0);
    do_set_plane_group(PIXPG_OVERLAY);
    do_rop(0, 0, 1152, 900, PIX_SRC, 0, 0, 0);
    do_set_plane_group(PIXPG_COLOR);

}



/****************************************************/
/* Do_stencil                                       */
/* */
/* Write stencil to screen pixrect and memory     */
/* pixrect.                                         */
/* */
/****************************************************/
do_stencil(x, y, w, h, op, stpr, stx, sty, spr, sx, sy)
    int             x, y, w, h, op, stx, sty, sx, sy;
    Pixrect        *stpr, *spr;

{
    pr_stencil(prfd, x, y, w, h, op, stpr, stx, sty, spr, sx, sy);
    pr_stencil(sepr, x, y, w, h, op, stpr, stx, sty, spr, sx, sy);
}

/****************************************************/
/* Do_rop                                           */
/* */
/* Rop into screen pixrect and expected memory    */
/* pixrect.                                         */
/* */
/****************************************************/
do_rop(x, y, w, h, op, spr, x1, y1)
    int             x, y, w, h, op, x1, y1;
    Pixrect        *spr;

{
    check_input();
    pr_rop(prfd, x, y, w, h, op, spr, x1, y1);
    pr_rop(sepr, x, y, w, h, op, spr, x1, y1);
}

/****************************************************/
/* Do_vector                                        */
/* */
/* Draw vector into screen pixrect and into       */
/* expected memory pixrect.                         */
/* */
/****************************************************/
do_vector(x0, y0, x1, y1, op, value)
    int             x0, y0, x1, y1, op, value;

{
    pr_vector(prfd, x0, y0, x1, y1, op, value);
    pr_vector(sepr, x0, y0, x1, y1, op, value);
}



/***************************************************/
/* Do_put                                          */
/* */
/* Set a pixel in both the screen pixrect and the */
/* expected memory pixrect.                        */
/* */
/***************************************************/
do_put(x, y, color)
    int             x, y, color;

{
    pr_put(prfd, x, y, color);
    pr_put(sepr, x, y, color);
}

/***************************************************/
/* Check_screen                                    */
/* */
/* Check the screen against the expected memory  */
/* pixrect.  Return FALSE if bad, return TRUE if   */
/* good.                                           */
/* */
/***************************************************/
int 
check_screen()
{
    int             j, i;
    u_long         *sptr, *dptr;

    check_input();
    if (SpecialFlag)
	pr_rop(sdpr, 0, 0, width, height, PIX_SRC, prfd, 0, 0);

    j = width * height * depth / 32;
    for (sptr = (u_long *)sdi, dptr = (u_long *)sde, i = 0; i < j; 
         i++, sptr++, dptr++) {
	if (*sptr != *dptr)
	    return(FALSE);
    }
    return(TRUE);
}



/*******************************************************/
/* Clean_up                                            */
/* */
/* Cleanup, remove all pixrects, return control back */
/* over to suntools, and replace the colors back into  */
/* the color lookup tables.                            */
/* */
/*******************************************************/
clean_up()
{
    Cursor          cursor;
    int             msfd;
    int             flag, count;
    char           *winname;

    if ( cg9 )
	pr_dbl_set ( prfd, PR_DBL_WRITE, PR_DBL_BOTH, PR_DBL_READ, PR_DBL_B, 0);
    reset_signals();
    if (prfd <= 0)
	exit(0);

    pr_putattributes(prfd, &tmpplane);

    for ( count = 0; count < 256; count++ ) {
	tred[count] = tblue[count] = tgrn[count] = count;
    }

    pr_putlut(prfd, 0, 256, tred, tgrn, tblue);

    if (iograb) {
	if (( fb_type.fb_type == cg_type ) && ( winname = getenv ("WINDOW_PARENT" ))) {
                        cursor = cursor_create((char *)0);
                        win_getcursor(rootfd, cursor);
                        cursor_set(cursor, CURSOR_SHOW_CURSOR, 1, 0);
                        win_setcursor(rootfd, cursor);
                        cursor_destroy(cursor);
 

	    flag = win_is_input_device(rootfd, "/dev/mouse");

	    /* re-install the mouse */
	    msfd = open(new_screen.scr_msname, O_RDONLY);
	    if (msfd >= 0) {
		win_set_input_device(rootfd, msfd,
				     new_screen.scr_msname);
		close(msfd);
		
	    }
	win_releaseio(winfd);
	}
    }
    if (SpecialFlag)
	mem_destroy(sdpr);
    mem_destroy(sepr);
    mem_destroy(oneexp);
    mem_destroy(onedump);
    pr_destroy(prfd);


}



/****************************************************/
/* Error                                            */
/* */
/* Throwup the code and the string, if the diag   */
/* argument was passed then XOR the screen with the */
/* expected memory pixrect and display ( used for   */
/* code debug) .                                    */
/* */
/****************************************************/
Error(code, string, xor)
    int             code, xor;
    char           *string;

{
    if (SpecialFlag && xor) {
        code = 0;
	pr_rop(sdpr, 0, 0, width, height, PIX_SRC ^ PIX_DST, sepr, 0, 0);
	pr_rop(prfd, 0, 0, width, height, PIX_SRC, sdpr, 0, 0);
	getchar();
    }
    cg_send_message ( code, FATAL, string );
}


check_input()
{
        Event   event;
        int arg = 0;

        ioctl(winfd, FIONREAD, &arg);
        if (arg != 0) {
                input_readevent(winfd, &event);
                if (event_id(&event) == 0x03)           /* CTRL-C */
                        finish();
        }
        return(0);
}

cg_lock_scr( func )
int (*func)();

{
	if ( iograb ) {
		win_grabio( winfd );
		sleep(1);
		(*func)();
		win_releaseio( winfd );
		}
	else (*func)();
}

cg_send_message( where, msg_type, msg )
    int             where;
    int             msg_type;
    char           *msg;
{
    if ( iograb ) {
		win_releaseio ( winfd );
		sleep(15);
	 }

    send_message ( where, msg_type, msg );

    if ( iograb ) win_grabio ( winfd );
}
