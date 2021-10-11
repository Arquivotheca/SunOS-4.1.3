static char     sccsid[] = "@(#)color.c 1.1 7/30/92 Copyright 1985 Sun Micro";

/*
  This diagnostic is a quick and dirty test of the two major areas of the
  colorboard (frame buffer and colormap) with some rasterops thrown
  in for taste. It does not test all of the frame buffer (takes too long!), nor
  does it test the zoom and pan circuitry. 
*/

#include <signal.h>
#include <sgtty.h>
#include <stdio.h>
#include <sys/file.h>
#include <suntool/gfx_hs.h> 
#include <sun/fbio.h>
#include "sdrtns.h"			/* sundiag standard header file */
#include "../../../lib/include/libonline.h"	/* online library include */

#define DEV_NOT_OPEN            3
#define RED_FAILED           	4 
#define GREEN_FAILED           	5 
#define BLUE_FAILED           	6 
#define FRAME_BUFFER_ERROR	7
#define	CGFOUR_NOT_RUN		8
#define	SCREEN_NOT_OPEN		10

#define cnvrt(a) ((unsigned char)(a * 255))

char file_name_buffer[30];
char *file_name = file_name_buffer;

struct color {
	unsigned char red;
	unsigned char grn;
	unsigned char blu;
} clr;

int  needs_clean_up = FALSE;
int planes = 0xff, a, b, c, x, y, tmp, xdim, ydim, pcnt;
unsigned char rmap[256], gmap[256], bmap[256], rmap1[256],
		gmap1[256], bmap1[256], rmap2[256], gmap2[256], bmap2[256];
struct pixrect *screen;
struct pixrect *mpr;
struct pixrect pr = {0, 0, 0, 0, 0};
struct cursor cr = {0, 0, 0, &pr};

int		 failed = 0;
int              simulate_error = 0;
char            *color_name = "";
char             msg_buffer[MESSAGE_SIZE];
char            *msg = msg_buffer;
char             perror_msg_buffer[30];
char            *perror_msg = msg_buffer;
static  char    *test_usage_msg = "/dev/<device name>";
static	char	*temp_argv[2]={NULL, NULL};
static	int      fixup_colormap();
static	int 	 gfx_fd;
static	struct 	 screen s, new_screen;
static	int 	 scrfd;
static	int 	 mon_color;
static	Event    event;
short		orig_sg_flags;	/* original mode flags for stdin */ 

/*** this is a special version of the send_message just for frame buffer tests ***/
fb_send_message(where, msg_type, msg)
    int             where;
    int             msg_type;
    char           *msg;
{
    if (needs_clean_up)
/*      win_releaseio(gfx_fd);

    send_message (where, msg_type, msg);

    if (needs_clean_up)
      win_grabio(gfx_fd);

}*/
 {
                clean_up();
                exit(0);
        } else {
                if ( gfx_fd ) win_releaseio(gfx_fd);
#ifndef _3.5
                sleep(30);
#endif _3.5
                (void) pr_get(screen, 0, 0);
                (void) pr_get(screen, 0, 1);
                (void) pr_get(screen, 0, 2);
    send_message (where, msg_type, msg);
                if ( gfx_fd ) win_grabio(gfx_fd);
        }
        return (0);
}


/*** main program starts here ***/
main(argc, argv)
int     argc;
char    *argv[];
{
   static int	on=FBVIDEO_ON;
   Pixwin	*scrwin;
   struct	gfxsubwindow *g;
   struct	inputmask im;
   int		pid, retry, fb_fd;
   int		my_fd, tmpfd, tmp;
   char		con_type;
   struct	sgttyb	ttybuf;
   char		name[WIN_NAMESIZE];	
   struct	fbgattr	fb_gattr;
   extern int   process_color_args();
   extern int   routine_usage();
 
   versionid = "1.1";		/* SCCS version id */
                                /* begin Sundiag test */
   test_init(argc, argv, process_color_args, routine_usage, test_usage_msg);
   if (*color_name == NULL) { 
        display_usage(test_usage_msg);    
        send_message(USAGE_ERROR, ERROR, "Must provide device name!"); 
   } 

   strcpy(device_name, color_name);
   mon_color = 1;		/* initialize the "both mon and color" flag */

   if ((tmpfd=open("/dev/fb", O_RDWR)) != -1)
   {
	  if (ioctl(tmpfd, FBIOGATTR, &fb_gattr) == 0)
	    switch (fb_gattr.real_type)
	    {
		case	FBTYPE_SUN2COLOR:
			if (strcmp(color_name, "/dev/cgtwo0") == 0)
			  mon_color = 0;
			break;

                case    FBTYPE_SUN3COLOR:
                        if (strcmp(color_name, "/dev/cgthree0") == 0)
                          mon_color = 0;
                        break;

                case    FBTYPE_SUN4COLOR:
                        if (strcmp(color_name, "/dev/cgfour0") == 0)
			{
			  if (getenv("WINDOW_ME") != 0)
			  {
			    scrfd = open(getenv("WINDOW_ME"), O_RDONLY);
			    scrwin = pw_open(scrfd);
			    tmp = scrwin->pw_pixrect->pr_depth;
			    pw_close(scrwin);
			    close(scrfd);
			    if (tmp == 1)	/* using mono overlay plane */
			      send_message(-CGFOUR_NOT_RUN, ERROR,
      "\"-8bit_color_only\" option was not specified while starting SunView.");
			    mon_color = 0;
			  }
			}
                        break;

		default:
			break;
	    }

	  close(tmpfd);
   }

   send_message(0, VERBOSE, "Started on %s.", color_name);
   sleep(15);			/* slow it down for IPC frame buffer test */

/* This section sets the window system up so that the background windows
   remain intact */

	 if (mon_color == 0 && getenv("WINDOW_GFX") != 0 &&
				(gfx_fd=win_getnewwindow())) {

		ioctl(0, TIOCGETP, &ttybuf);
		orig_sg_flags = ttybuf.sg_flags;/* save original mode flags */
		ttybuf.sg_flags |= CBREAK;	/* non-block mode */
		ttybuf.sg_flags &= ~ECHO;	/* no echo */
		ioctl(0, TIOCSETP, &ttybuf);

		scrfd = open(getenv("WINDOW_PARENT"), O_RDONLY);

		win_setlink(gfx_fd, WL_PARENT,
				win_nametonumber(getenv("WINDOW_PARENT")));
		win_setlink(gfx_fd, WL_OLDERSIB,
				win_getlink(scrfd, WL_YOUNGESTCHILD));

		win_screenget(scrfd, &s);
		win_setrect(gfx_fd, &s.scr_rect);

		close(scrfd);

                input_imnull(&im);
		im.im_flags |= IM_ASCII;	/* catch 0-127(CTRL-C == 3) */
		win_set_kbd_mask(gfx_fd, &im);

		win_insert(gfx_fd);
		win_grabio(gfx_fd);

		/* remove the mouse */
		win_remove_input_device(gfx_fd, s.scr_msname);

		needs_clean_up = TRUE;
	   }

		/* if we're testing the frame buffer where sunview is running,
		 * we need to open /dev/fb instead of the device name (e.g.,
		 * /dev/cgtwo0); otherwise, if the mouse is moved, then a
		 * mark will be left on the screen, causing an error in test. */

	if (mon_color == 0)
	    strcpy(file_name, "/dev/fb");
	else
	{
	    strcpy(file_name, color_name);
	    fb_fd = open(file_name, O_RDWR);
	    ioctl(fb_fd, FBIOSVIDEO, &on);	/* turns on video */
	    close(fb_fd);
	}
 
	retry = 250;			/* retry for 250 times */
        while ((screen = pr_open(file_name)) == 0) {

	   if (--retry != 0)
	   {
	     sleep(5);			/* retry it 5 seconds later */
	     continue;
	   }
	   
           perror(perror_msg);
	   sprintf(msg, "Couldn't open file '%s'", file_name);
	   fb_send_message(-DEV_NOT_OPEN, ERROR, msg);
        }

   exec_diag();
   clean_up();

   if (failed) 
       exit(failed);

   test_end();		/* Sundiag normal exit */
}


/* This is the diagnostic section proper */
exec_diag()
{
	int tx, ty, i, z;
	int check_pix;

	xdim = 1152;
	ydim = 900;

	pr_putattributes(screen, &planes);
	loadmap();

	/* write a pixrect of a certain color and dimension to screen */
	for(x=y=0,a=65,b=50,c=0; x<(xdim/2),y<(ydim/2),a>0,b>0;
					x+=a,y+=b,++c,a-=4,b-=3) {
		tx = xdim-(2*x);
		ty = ydim-(2*y);
		pr_rop(screen,x,y,tx,ty,PIX_SRC|PIX_COLOR(c),0,0,0);
		/* do it */
		if (quick_test) check_pix = 50;
		else check_pix = 1;
		pix_chka(c,x,y,tx,ty,check_pix);	/* verify it */
	}

	llmap();
}


/* set up color map with all permutations of color to be displayed */
loadmap()
{
	int i, j;
	float a,b;
	struct color *hsv_rgb(), *col;

	a = b = 1.0;
		for (i = 0, j = 0; i < 80 ;++i,j+=18,a-=.0125) {
			col = hsv_rgb(j,a,b);
			rmap[i] = col->red;
			gmap[i] = col->grn;
			bmap[i] = col->blu;
			if (j == 360) {
				j = 0;
			}
		}

	a = b = 1.0;
		for (; i < 160 ;++i,j+=18,b-=.0125) {
			col = hsv_rgb(j,a,b);
			rmap[i] = col->red;
			gmap[i] = col->grn;
			bmap[i] = col->blu;
			if (j == 360) {
				j = 0;
			}
		}

	a = b = 1.0;
		for (; i < 240 ;++i,j+=18,a-=.0125,b-=.0125) {
			col = hsv_rgb(j,a,b);
			rmap[i] = col->red;
			gmap[i] = col->grn;
			bmap[i] = col->blu;
			if (j == 360) {
				j = 0;
			}
		}

	pr_putcolormap(screen, 0, 256, rmap, gmap, bmap);
}


llmap()
{
	int i, x, z;

		/* zoom through colormap forward, 20 colors at a time */
		for (i = 0; i < 220; ++i) {
			for (x = 19; x >=0; --x) {
			  rmap1[x] = rmap[i + x];
			  gmap1[x] = gmap[i + x];
			  bmap1[x] = bmap[i + x];
			  pr_putcolormap(screen, 0, 20, rmap1, gmap1, bmap1);
			  pr_getcolormap(screen, 0, 20, rmap2, gmap2, bmap2);
			  map_chk(20);
			}
#ifdef	sun4			/* slow down a little bit */
		usleep(60000);
#endif
		}

		/* zoom through colormap backward, 20 colors at a time */
		for (i = 220; i >= 0; --i) {
			for (x = 0; x < 20; ++x) {
			  rmap1[x] = rmap[x + i];
			  gmap1[x] = gmap[x + i];
			  bmap2[x] = bmap[x + i];
			  pr_putcolormap(screen, 0, 20, rmap1, gmap1, bmap1);
			  pr_getcolormap(screen, 0, 20, rmap2, gmap2, bmap2);
			  map_chk(20);
			}
#ifdef	sun4			/* slow down a little bit */
		usleep(60000);
#endif
		}

		pr_putcolormap(screen, 0, 256, rmap1, gmap1, bmap1);
		pr_getcolormap(screen, 0, 256, rmap2, gmap2, bmap2);
		map_chk(256);
}


/* verify colormap read matches colormap written */
map_chk(it)
char it;
{
	char z;
	int  arg;

	for (z = 0; z < it; ++z) {
		if (rmap2[z] != rmap1[z] || simulate_error == RED_FAILED) {
		  sprintf(msg, "Colormap error - red, loc = %d, exp = %d, actual = %d.", z, rmap1[z], rmap2[z]);
                  fb_send_message(-RED_FAILED, ERROR, msg);
		}
			
		if (gmap2[z] != gmap1[z] || simulate_error == GREEN_FAILED) {
		  sprintf(msg, "Colormap error - green, loc = %d, exp = %d, actual = %d.", z, gmap1[z], gmap2[z]);
		  fb_send_message(-GREEN_FAILED, ERROR, msg);
		}

		if (bmap2[z] != bmap1[z] || simulate_error == BLUE_FAILED) {
		  sprintf(msg, "Colormap error - blue, loc = %d, exp = %d, actual = %d.", z, bmap1[z], bmap2[z]);
		  fb_send_message(-BLUE_FAILED, ERROR, msg);
		}
	}
	arg = 0;

	if (needs_clean_up)				/* single-headed */
	{
	  ioctl(gfx_fd, FIONREAD, &arg);
	  if (arg != 0)
	  {
	    input_readevent(gfx_fd, &event);
	    if (event_id(&event) == 0x03)		/* CTRL-C */
	      finish();
	  }
	}
}

/* verify locations in the frame buffer read as written */
pix_chka(c,x,y,tx,ty,n)
int c,x,y,tx,ty,n;
{
	int a,b;
	int arg;

  for (; x < (tx - 50); x += n) {
     for (b = y; b < (ty - 50); b += n) {
        a = pr_get(screen, x, b);	
	if (c != a || simulate_error == FRAME_BUFFER_ERROR) {
	   sprintf(msg, "Soft frame buffer failure, x pos = %d, y pos = %d, exp = 0x%x, actual = 0x%x.", x, b, c, a);

	   sleep(10);
	   a = pr_get(screen, x,b);	/* try it again */

	   if (c != a || simulate_error == FRAME_BUFFER_ERROR)
	   {
	     sprintf(msg, "Hard frame buffer failure, x pos = %d, y pos = %d, exp = 0x%x, actual = 0x%x.", x, b, c, a);

	     fb_send_message(-FRAME_BUFFER_ERROR, ERROR, msg);
	   }

	   fb_send_message(0, WARNING, msg);
           failed = TRUE;
	}
     }
     arg = 0;
     if (needs_clean_up)			/* single-headed */
     {
       ioctl(gfx_fd, FIONREAD, &arg);
       if (arg != 0)
       {
	  input_readevent(gfx_fd, &event);
	  if (event_id(&event) == 0x03)		/* CTRL-C */
	    finish();
       }
     }
  }
}


/* convert hue, saturation and value to red, green and blue */
struct color *hsv_rgb(hx,s,v)
float s, v;
int hx;
{
	float h, f, q, p, t;
	int i;
	struct color *col = &clr;

	h = hx;
	if (h > 359) {
		h = h - 360;
	}
	else if (h < 0) {
		h = 360 + h;
	}
	h = h / 60;
	i = h;
	f = h -i;
	p = v * (1 - s);
	q = v * (1 - (s * f));
	t = v * (1 - (s * (1 - f)));
	switch(i) {
	case 0:
		col->red = cnvrt(v);
		col->grn = cnvrt(t);
		col->blu = cnvrt(p);
		return(col);
	case 1:
		col->red = cnvrt(q);
		col->grn = cnvrt(v);
		col->blu = cnvrt(p);
		return(col);
	case 2:
		col->red = cnvrt(p);
		col->grn = cnvrt(v);
		col->blu = cnvrt(t);
		return(col);
	case 3:
		col->red = cnvrt(p);
		col->grn = cnvrt(q);
		col->blu = cnvrt(v);
		return(col);
	case 4:
		col->red = cnvrt(t);
		col->grn = cnvrt(p);
		col->blu = cnvrt(v);
		return(col);
	case 5:
		col->red = cnvrt(v);
		col->grn = cnvrt(p);
		col->blu = cnvrt(q);
		return(col);
	}
}


clean_up(can_log)
int can_log;
{
   struct sgttyb ttybuf;

   if (needs_clean_up) {
     ioctl(0, TIOCGETP, &ttybuf);	/* restore the input characteristics */
     ttybuf.sg_flags = orig_sg_flags;   /* restore orig mode flags for stdin */
     ioctl(0, TIOCSETP, &ttybuf);

     win_grabio(gfx_fd);	/* to restore the screen(!!!) */
     win_releaseio(gfx_fd);

     /* reconnect the mouse */
     win_setms(gfx_fd, &s);

     close(gfx_fd);
   }
   else if ((mon_color == 1) && (screen != NULL))
	  pr_rop(screen,0,0,1152,900,PIX_SRC|PIX_COLOR(0xff),0,0,0);
				/* clear the screen before exit */
}

process_color_args(argv, arrcount)
char    *argv[];
int     arrcount;
{
 
    if (strncmp(argv[arrcount], "/dev/", 5) == 0)
        color_name = argv[arrcount];
    else
	return (FALSE);

    return (TRUE);
}

routine_usage()
{
    (void) printf("Routine specific arguments [defaults]:\n");
    (void) printf("/dev/<device name> = name of color device(required)\n");
}
