#ifndef lint
static char sccsid[]  = "@(#)mono.c 1.1 7/30/92 Copyright Sun Micro";
#endif
/*
 * mono.c:  monochrome frame buffer test.
 *
 * This test writes/reads/compares a number of different test patterns to
 * the monochrome frame buffer.
 *
 * example command line usage:  mono /dev/bwtwo0
 *
 * compile: cc -g -I../include mono.c -o mono ../lib/libtest.a -lsuntool
 *          -lsunwindow -lpixrect
 */

#include <signal.h>
#include <sgtty.h>
#include <stdio.h>
#include <sys/file.h>

#ifdef	 OL
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <pixrect/pixrect.h>
#else
#include <suntool/gfx_hs.h> 
#endif	 OL

#ifdef	 SVR4
#include <sys/fbio.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#else
#include <sun/fbio.h>
#endif	 SVR4
#include <sys/types.h>
#include <sys/mman.h>
/*#include <sun3/cpu.h>*/
#include "sdrtns.h"		/* sundiag standard header file */
#include "../../../lib/include/libonline.h"   /* online library include */
#include "mono_msg.h"		/* messages header file */

char file_name_buffer[30];
char *file_name = file_name_buffer;

char		*errmsg();
extern int	 errno;
struct pixrect	*screen;
int		 needs_clean_up = FALSE, testing_fb = FALSE;
char            *fbname = ""; 
char             msg_buffer[MESSAGE_SIZE];
char            *msg = msg_buffer;
#ifdef OL
XEvent  event ;
Display *dsp = NULL ;
Window  w,
	root ;
char    *display = NULL ;
int     curnt_sc ;
#define DEFAULT_DISPLAY ":0.0"
XColor fgnd={0,0,0,0,0,0};
XColor bkgnd={0,0,0,0,0,0} ;
#else
static	int 	 gfx_fd, scrfd;
static	struct screen s;
Event	event ;
#endif
 
/* this is a special version of the send_message just for frame buffer
 * tests */
fb_send_message(where, msg_type, msg)
    int             where;
    int             msg_type;
    char           *msg;
{
    if (needs_clean_up)
    {
#	ifdef OL
	/* XXX - something needs to be done equivalentely */
#	else
        (void)win_releaseio(gfx_fd);
#	endif OL
    }
    send_message (where, msg_type, msg);
    if (needs_clean_up)
    {
#	ifdef OL
	/* XXX - something needs to be done equivalentely */
#	else
        (void)win_grabio(gfx_fd);
#	endif Ol
    }
}

/*** main program starts here ***/
main(argc, argv)
    int     argc;
    char    *argv[];
{
    static int		on=FBVIDEO_ON;
    int			retry, tmpfd, fb_fd;
    int			fbsize = 0;	/* size (in bytes) of frame buffer */
#   ifdef OL
    XSetWindowAttributes xswa ;
    int cursor_shape = XC_coffee_mug ;
    Cursor cursor ;
#   else
    Pixwin		*scrwin;
    struct inputmask	im;
#   endif OL
    struct sgttyb	ttybuf;
    struct fbtype	fb_type;
    struct fbgattr	fb_gattr;	/* extra fb attributes (cgfour)	*/
    extern int		process_mono_args(), routine_usage();
#   ifndef SVR4
    char		*getenv(), *strcpy(), *sprintf();
#   endif
  
    versionid = "1.1";
    test_init(argc, argv, process_mono_args, routine_usage, test_usage_msg);
    device_name = fbname;
    if (!strcmp(fbname, "")) {
	display_usage(test_usage_msg);
	send_message(USAGE_ERROR, ERROR, e_usage_msg);
    }

    /******************************************************/
    /*							  */
    /* Firstly, find out if the framebuffer under test is */
    /* running the window system. 			  */
    /*							  */
    /******************************************************/
#   ifdef SVR4

    /* We don't know what to do under 5.0DR. yet */

#   else
    /* attempt to open /dev/fb */
    if ((tmpfd = open("/dev/fb", O_RDWR)) != -1) {
	if (ioctl(tmpfd, FBIOGTYPE, &fb_type) != 0)
	    send_message(1, ERROR, e_ioctl_msg);
	switch(fb_type.fb_type)
	{
	  case FBTYPE_SUN2BW:
	    /* note: cgfour's also have FBTYPE_SUN2BW as fb_type.  The
	       following ioctl call will fail on a bwtwo fb.  The following
	       ioctl section will allow this test to run on a cgfour.*/
	    if (ioctl(tmpfd, FBIOGATTR, &fb_gattr) == 0) {
		if (fb_gattr.real_type == FBTYPE_SUN4COLOR) {
#		  ifdef OL
		  /* for the openlook running under 4.x, we always
		     assume it is overlay */
		  testing_fb = TRUE ;
#		  else
		  if (getenv("WINDOW_ME") != 0)
		  {
		    scrfd = open(getenv("WINDOW_ME"), O_RDONLY);
		    scrwin = pw_open(scrfd);
		    if (scrwin->pw_pixrect->pr_depth == 1)
		      testing_fb = TRUE;
		    pw_close(scrwin);
		    close(scrfd);
		    break;
		  }
#		  endif OL
		}
	    }

	    if (strcmp(fbname, "/dev/bwtwo0") == 0)
	      testing_fb = TRUE;	/* bwtwo0 is the current /dev/fb */
	    /* -JCH- only the first mono frame buffer can be the /dev/fb */
	    break;

	    /* end section to allow running on cgfour */
	    /* if bwtwo is the current fb, then we want to pr_open (later)
	       /dev/fb and not /dev/bwtwo0.  Reason:  mouse movement
	       on some systems will mess up screen, causing errors. */

	  default:
	    break;
	}
	(void)close(tmpfd);
    }

#   endif SVR4

    send_message(0, VERBOSE, v_start_msg, fbname);
	
    sleep(15);	/* slow it down for IPC frame buffer test */

/* This section sets the window system up so that the background windows
   remain intact.  Do only if the test is being run on the monitor that's
   running sunview. */
#   ifdef OL
    
    display = DEFAULT_DISPLAY;
    if (!(dsp = XOpenDisplay(display)))
	    send_message(0,ERROR,"%s: unable to open display %s.\n", display);

    curnt_sc = DefaultScreen(dsp);
    root = RootWindow(dsp, curnt_sc);

    xswa.override_redirect = True;
    xswa.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | VisibilityChangeMask;

    w = XCreateWindow(dsp, root,
		   0, 0,
		   DisplayWidth(dsp, curnt_sc),
		   DisplayHeight(dsp, curnt_sc),
		   0, CopyFromParent, InputOutput, CopyFromParent,
		   CWOverrideRedirect | CWEventMask, &xswa);
    XSetWindowBackgroundPixmap(dsp, w, ParentRelative);

    cursor = XCreateFontCursor(dsp,cursor_shape);
    XDefineCursor(dsp,w,cursor);
    XRecolorCursor(dsp,cursor,&fgnd,&bkgnd);
    XMapWindow(dsp, w);
    XNextEvent(dsp, &event);
    XRaiseWindow(dsp, w);
    needs_clean_up = TRUE;
    ioctl(0, TIOCGETP, &ttybuf);
    ttybuf.sg_flags |= CBREAK;  /* non-block mode */
    ttybuf.sg_flags &= ~ECHO;   /* no echo */
    ioctl(0, TIOCSETP, &ttybuf);

#   else

    if (testing_fb == TRUE && getenv("WINDOW_GFX") != NULL &&
				(gfx_fd=win_getnewwindow())) {
	(void)ioctl(0, TIOCGETP, &ttybuf);
	ttybuf.sg_flags |= CBREAK;	/* non-block mode */
	ttybuf.sg_flags &= ~ECHO;	/* no echo */
	(void)ioctl(0, TIOCSETP, &ttybuf);
	scrfd = open(getenv("WINDOW_PARENT"), O_RDONLY);
	(void)win_setlink(gfx_fd, WL_PARENT,
			win_nametonumber(getenv("WINDOW_PARENT")));
	(void)win_setlink(gfx_fd, WL_OLDERSIB,
			win_getlink(scrfd, WL_YOUNGESTCHILD));
	(void)win_screenget(scrfd, &s);
	(void)win_setrect(gfx_fd, &s.scr_rect);
	(void)close(scrfd);
        (void)input_imnull(&im);
	im.im_flags |= IM_ASCII;	/* catch 0-127(CTRL-C == 3) */
	(void)win_set_kbd_mask(gfx_fd, &im);
	(void)win_insert(gfx_fd);
	(void)win_grabio(gfx_fd);
	/* remove the mouse */
	(void)win_remove_input_device(gfx_fd, s.scr_msname);
	needs_clean_up = TRUE;
    }

    if (testing_fb == TRUE)
        (void)strcpy(file_name, "/dev/fb");
    else
    {
        (void)strcpy(file_name, fbname);
	fb_fd = open(file_name, O_RDWR);
	ioctl(fb_fd, FBIOSVIDEO, &on);	/* turns on video */
	close(fb_fd);
    }
#   endif OL

#   ifdef SVR4
    fbsize = 1152*900 ;
#   else
    /* Use pixrect utility to get framebuffer size infomation */
    retry = 250;			/* retry for 250 times */
    while ((screen = pr_open(file_name)) == 0) {
	if (--retry != 0) {
	    sleep(5);			/* retry it 5 seconds later */
	    continue;
	}
	(void)sprintf(msg, e_pr_open_msg, file_name, errmsg(errno));
	fb_send_message(2, ERROR, msg);
    }

    fbsize = screen->pr_size.x * screen->pr_size.y / 8;
#   endif SVR4

    fb_send_message(0, VERBOSE, v_fbsize_msg, screen->pr_size.x, 
                 screen->pr_size.y, fbsize);
    sleep(5);  /* IMPORTANT:  wait for sunview to set up before writing
                * to frame buffer */
    video_test(fbsize);
    fb_send_message(0, VERBOSE, v_completed_msg);
    clean_up();
    test_end();
}

/*
 * video_test() opens the particular device (e.g., /dev/bwtwo0), mmaps it
 * to this process' address space, and then calls the functions that
 * actually test the frame buffer.
 *
 * argument: size - size of frame buffer, in bytes.
 */

video_test(size)
    int		size;
{
    caddr_t	mmap();
    unsigned char *fb_addr;/* fb's address in this process' address space */
    int		fd;	   /* file descriptor for frame buffer device */
    unsigned 	seed;      /* seed for random data test */

    if ((fd = open(fbname, O_RDWR)) < 0)
	fb_send_message(1, ERROR, e_open_msg, fbname);
    if ((fb_addr = (unsigned char *)mmap((caddr_t)0, size,
	PROT_READ | PROT_WRITE, MAP_SHARED, fd, (off_t)0))
			== (unsigned char *)-1) {
	fb_send_message(2, ERROR, e_mmap_msg, errmsg(errno));
    }
    fb_send_message(0, VERBOSE, v_fbaddr_msg, fb_addr);

    fill(fb_addr, size, 0x33);  
    check(fb_addr, size, 0x33);
    fill(fb_addr, size, 0x55);  
    check(fb_addr, size, 0x55);

    for (seed = 1; seed < size; seed = seed + (size/10)) 
        randomtest((unsigned long *)fb_addr, size, seed);

    addrtest(fb_addr, size);
    laddrtest((unsigned long *)fb_addr, size);
    stripetest(fb_addr, size);
}

/*
 * check_input() checks if the user has typed a CTRL-C on the keyboard,
 * meaning the test should be aborted.
 */
check_input()
{
#   ifdef OL

    /* if ( XCheckIFEvent() )..  */

#   else
    int		arg;

    if (needs_clean_up) {
	(void)ioctl(gfx_fd, FIONREAD, &arg);
	if (arg != 0) {
	    (void)input_readevent(gfx_fd, &event);
	    if (event_id(&event) == 0x03)               /* CTRL-C */
		finish();
	}
    }
#   endif OL
}

/*
 * fill() writes a constant pattern into the frame buffer.
 *
 * arguments:
 *  base - 	base address of frame buffer.
 *  size -	number of bytes in frame buffer.
 *  pattern -	a constant pattern (8 bits).
 */
fill(base, size, pattern)
    unsigned char	*base;
    int			size;
    u_char      	pattern;
{
    int         	i;
 
    for (i = 0; i < size; i++) 
        *base++ = pattern;
    check_input();
}

/*
 * check() reads from the frame buffer and compares each location with a
 * constant pattern.  When a miscompare is found, the error message
 * is displayed and the test aborted (via send_message).
 *
 * arguments:
 *  base -	base address of frame buffer.
 *  size -      number of bytes in frame buffer.
 *  pattern -   a constant pattern (8 bits).
 */
check(base, size, pattern)
    unsigned char	*base;
    int			size;
    u_char		pattern;
{
    int			i;
    u_char		value;
    unsigned char	*addr = base;

    for (i = 0; i < size; i++, addr++) {
        if ((value = *addr) != pattern) 
	    fb_send_message(1, ERROR, e_const_msg, addr-base, pattern, value);
    }
    check_input();
}

/*
 * addrtest() fills each (byte) location of the frame buffer with the lower
 * 8 bits of its address.  After writing to entire frame buffer, addrtest()
 * then reads and compares the data.  If an error is found, the message is
 * displayed and the test terminates (via send_message).
 *
 * arguments:
 *  base -	base address of frame buffer.
 *  size -      number of bytes in frame buffer.
 */
addrtest(base, size)
    unsigned char	*base;
    int			size;
{
    int			i;
    unsigned char	*addr = base;
    u_char		obs_value;

    for (i = 0; i < size; i++, addr++) 
	*addr = (u_char)addr;
    check_input();
    for (addr = base, i = 0; i < size; i++, addr++) {
	if ((obs_value = *addr) != (u_char)addr) 
	   fb_send_message(1, ERROR, e_addr_msg, addr-base, obs_value,
		        (u_char)addr);
    }
}

/*
 * laddrtest() fills each (word) location of the frame buffer with
 * its 32-bit address.  After writing to entire frame buffer, laddrtest()
 * then reads and compares the data.  If an error is found, the message is
 * displayed and the test terminates (via send_message).
 *
 * arguments:
 *  base -	base address of frame buffer.
 *  size -      number of bytes in frame buffer.
 */
laddrtest(base, size)
    unsigned long	*base;
    int			size;		/* frame buffer size, in bytes */
{
    int			i;
    unsigned long	*addr = base;
    unsigned long	obs_value;

    for (i = 0; i < size/(sizeof(long)); i++, addr++) 
	*addr =  (unsigned long)addr;
    check_input();
    for (addr = base, i = 0; i < size/(sizeof(long)); i++, addr++) {
        if ((obs_value = *addr) != (unsigned long)addr) 
            fb_send_message(1, ERROR, e_laddr_msg, addr-base, obs_value, addr);
    }
}

/*
 * randomtest() fills each (word) location of the frame buffer with a random
 * number.  The random numbers are generated using the seed argument.
 * This is only pseudo-random as the numbers are the same ones generated
 * for each seed.  Since this is the case, we can go back and compare
 * what was written to what we expect.  If a miscompare is found, the
 * error message is displayed and the test terminates (via send_message).
 *
 * arguments:
 *  base -	base address of frame buffer.
 *  size -      number of bytes in frame buffer.
 *  seed -	seed to generate random numbers.
 */
randomtest(base, size, seed)
    unsigned long       *base;
    int                 size;           /* frame buffer size, in bytes */
    unsigned 		seed;		/* seed for random() function  */
{
    long		random();
    char		*initstate();
    static long		state1[32] = {3};
    int			i;
    unsigned long	*addr = base;
    unsigned long       obs_value, exp_value;

    (void)initstate(seed, (char *)state1, 128);	/* set up random sequence */
    for (i = 0; i < size/(sizeof(long)); i++)
	*addr++ = (unsigned long)(random());
    check_input();
    addr = base;  /* reset addr to base of fb for checking */
    (void)initstate(seed, (char *)state1, 128);
    for (i = 0; i < size/(sizeof(long)); i++) {
	if ((obs_value = *addr++) != (exp_value = (unsigned long)random())) 
	   fb_send_message(1, ERROR, e_rand_msg, addr-base, obs_value, exp_value);
    }
}

/*
 * stripetest() writes an approximately stripe pattern to the frame buffer.
 * First 0's are written to every other j bytes until end of frame buffer.
 * Then the bytes that were skipped will be filled in with 0xff's (black)
 * until end of frame buffer memory. Then the bytes are read back and
 * compared to the expected pattern.  If successful, then we double the
 * size of j and start writing to memory again.
 *
 * arguments:
 *  base -	base address of frame buffer.
 *  size -      number of bytes in frame buffer.
 *
 */
stripetest(base, size)
    unsigned char	*base;
    int         	size;
{
    unsigned char	pat = 0;
    unsigned char	obs = 0;	/* observed pattern */
    unsigned char	*addr = base;
    int			j, k, l, m;

    for (j = 1; j < size; j *= 2) {  /* double the bytes written each time */
        for (k = 0; k < 2; k++, pat = ~pat) {    
    	    for (addr = base + (j * k), l = size/(j * sizeof(unsigned char)*2);
                 l != 0; l--, addr += j) {
    	        for (m = j; m != 0; m--, addr++) 
    		    *addr = pat;
    	    }
        }
	check_input();
	for (k = 0; k < 2; k++, pat = ~pat) {
	    for (addr = base + (j * k), l = size/(j * sizeof(unsigned char)*2);
                 l != 0; l--, addr += j) {
		for (m = j; m != 0; m--, addr++) {
		    if ((obs = *addr) != pat) 
			fb_send_message(1, ERROR, e_stripe_msg, addr-base, pat,
				     obs);
		}
	    }
        }
    }
}

clean_up()
{
#   ifdef OL
#   else
    struct sgttyb ttybuf;

    if (needs_clean_up) {
		/* restore the input characteristics */
        (void)ioctl(0, TIOCGETP, &ttybuf);
        ttybuf.sg_flags &= ~CBREAK;
        ttybuf.sg_flags |= ECHO;
        (void)ioctl(0, TIOCSETP, &ttybuf);
        (void)win_grabio(gfx_fd);	/* to restore the screen(!!!) */
        (void)win_releaseio(gfx_fd);
        (void)win_setms(gfx_fd, &s);  /* reconnect the mouse */

        (void)close(gfx_fd);
    } else if ((testing_fb == FALSE) && (screen != NULL))
	(void)pr_rop(screen,0,0,screen->pr_size.x, screen->pr_size.y,
		PIX_SRC|PIX_COLOR(0xff), (Pixrect *)0,0,0);
				/* clear the screen before exit */
#   endif OL
}

process_mono_args(argv, arrcount)
char    *argv[];
int     arrcount;
{
 
    if (strncmp(argv[arrcount], "/dev/", 5) == 0) {
        fbname = argv[arrcount];
        return (TRUE);
    }

    return (FALSE);
}

routine_usage()
{
    send_message(0, CONSOLE, routine_msg);
}
