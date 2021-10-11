static char     sccsid[] = "@(#)cg5test.c 1.1 7/30/92 Copyright 1988 Sun Micro";


#include <stdio.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sgtty.h>
#include <sun/fbio.h>

#include <pixrect/pixrect_hs.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/cg2reg.h>
#include <sunwindow/win_cursor.h>
#include <suntool/sunview.h>

#include <suntool/gfx_hs.h>
#include "sdrtns.h"		/* sundiag standard header file */

#include "../../../lib/include/libonline.h"   /* online library include */
#define PR_SYNC(dpr) if ((dpr)) (void) pr_get(prfd, 0,0);

/* 
 * error code for this test.
 */
#define DEV_NOT_OPEN            3
#define RED_FAILED           	4 
#define GREEN_FAILED           	5 
#define BLUE_FAILED           	6 
#define FRAME_BUFFER_ERROR	7
#define SIGBUS_ERROR            8
#define SIGSEGV_ERROR           9
#define	SCREEN_NOT_OPEN		10

#define	CORRUPT_CG_LOWER_RT_A	12
#define	CORRUPT_CG_UPPER_LT_A	13

#define	CORRUPT_CG_LOWER_RT_B	14
#define	CORRUPT_CG_UPPER_LT_B	15

#define SYNC_ERROR		19
#define END_ERROR               20

#define DEVICE			"/dev/cgtwo0"

#define NO_FB		0
#define NOCOLORFB    	10
#define CG_1BUF		11
#define CG_2BUF		12
#define CG_NOTCONSOLE_1BUF    21
#define CG_NOTCONSOLE_2BUF    22

#define CG_ONLY         2

extern setup_desktop();
extern char *getenv();

static unsigned char tred[256], tgrn[256], tblu[256];

static unsigned char red1[256], grn1[256], blu1[256];
static unsigned char red2[256], grn2[256], blu2[256];

static int winfd, rootfd, devtopfd;

static Pixrect *prfd;

struct screen new_screen;

int width, height, depth;

unsigned int tmpplane;

int  cleanup_ioctl = FALSE;
int  iograb = FALSE;
int gp_flag = TRUE;

int dblflag;

int		return_code = 0;
int             simulate_error = 0;
char            msg_buffer[MESSAGE_SIZE];
char            *msg = msg_buffer;
static int	dummy(){return FALSE;}

main(argc, argv)
	int argc;
	char *argv[];
{
	int tmpsim, probecode;

        versionid = "1.1";     /* SCCS version id */
	winfd = rootfd = devtopfd = 0;
	prfd = (Pixrect *) 0;

			/* begin Sundiag test */
    	test_init(argc, argv, dummy, dummy, (char *)NULL);
	device_name = DEVICE;
	setup_signals();

	probecode = probe_cg();

	setup_desktop();

	/*
	 * initialize the mem pixrect
	 */

	initmem();

	/*
	 * test frame buffer memory
	 */

	tmpsim = simulate_error;
	if ( (simulate_error == CORRUPT_CG_LOWER_RT_B) ||
		(simulate_error == CORRUPT_CG_UPPER_LT_B) ) {
		simulate_error = 0;
	}

	rw_displayA();
	testframebuffer();

	simulate_error = tmpsim;

	if (dblflag) {
		rw_displayB();
		testframebuffer();

		/*
		 * test double buffering functionality
		 */

		testdoublebuf();
		check_input();

	}

	/*
	 * test frame buffer to frame buffer writes
	 */

	testfb_to_fb();
	check_input();

	/*
	 * test color map
	 */

	init_cmapnfb();
	check_input();
	testcolormap();
	check_input();

done:
	clean_up();
	if (!exec_by_sundiag)
		printf ("end of test.\n");

	test_end();		/* Sundiag test normal exit */
}


probe_cg()
{

   struct  fbtype  fb_type;
   struct  fbgattr fb_gattr;
   int tmpfd;
   char str[40];

   return_code = NO_FB;

   /* first test for console type */

   if ((tmpfd=open("/dev/fb", O_RDONLY)) != -1) {
      ioctl(tmpfd, FBIOGTYPE, &fb_type);
	close (tmpfd);
   } else
      return(tmpfd);

   /* check if cg driver is there */

   gp_flag = FALSE;
   if (access(DEVICE, 0) == 0) {

      /* check if cg is there */
      /* first check if going through gp2 */

	if (fb_type.fb_type == FBTYPE_SUN2GP ) {
		if ( ( access("/dev/gpone0a", 0) == 0 ) &&
		( ( prfd = pr_open("/dev/gpone0a", O_RDWR) ) != NULL ) ) {
			gp_flag = TRUE;
		} else {
			syserror(-DEV_NOT_OPEN, "prfd");
		}
	} else if (fb_type.fb_type == FBTYPE_SUN2COLOR ) {
		if ((prfd = pr_open("/dev/fb")) == NULL)
	      		syserror(-DEV_NOT_OPEN, "/dev/fb");
	} else {
		prfd = pr_open(DEVICE);
	      	return_code = NOCOLORFB;
	}

         /* We have a device.  Check for single or double buffer */

	 if ( prfd != NULL) {
	 	dblflag = dblbuf_test();
	 	return_code += (dblflag) ? CG_2BUF : CG_1BUF;
	 }
   } else
	 return_code = NOCOLORFB;

   if (FALSE) {
      switch (return_code) {
         case CG_1BUF:
            printf("%s: Color monitor '%s' single buffer.\n", test_name, DEVICE);
	    break;
         case CG_2BUF:
	    printf("%s: Color monitor '%s' with double buffer.\n", test_name, DEVICE);
	    break;
         case NOCOLORFB:
	    printf("%s: No '%s' color monitor.\n", test_name, DEVICE);
	    break;
         case CG_NOTCONSOLE_1BUF:
	    printf("%s: Color monitor '%s' single buffer, cg is not console.\n", test_name, DEVICE);
	    break;
         case CG_NOTCONSOLE_2BUF:
	    printf("%s: Color monitor '%s' double buffer, cg is not console.\n", test_name, DEVICE);
	    break;
         case NO_FB:
         default:
	    printf("%s: No monitor available.\n", test_name);
	    break;
      }
   }
   return(return_code);
}

struct pixrect *mexp, *mobs;
unsigned int *ptr1, *ptr2, *mexphead, *mobshead;

initmem()
{
	register struct mprp_data *mprd;
	/*
	 * we set up the pixrect
	 */

	mexp = mem_create (width, height, depth);
	if (mexp <= 0)
		syserror(-SCREEN_NOT_OPEN, "mexp");

	mobs = mem_create (width, height, depth);
	if (mobs <= 0)
		syserror(-SCREEN_NOT_OPEN, "mobs");

	/*
	 * set up where data begins in mem pixrects
	 * mexphead = start of mexp.
	 * mobshead = start of mobs.
	 */
	mprd = (struct mprp_data *) mexp->pr_data;
	mexphead = (unsigned int *) mprd->mpr.md_image;

	mprd = (struct mprp_data *) mobs->pr_data;
	mobshead = (unsigned int *) mprd->mpr.md_image;
	return (0);

}


testframebuffer()
{
	static char *framename[] = {
			"fb screen 1",
			"fb screen 2",
			"fb screen 3",
			"fb screen 4",
			"fb screen 5",
			"fb screen 6",
			"fb screen 7",
			"fb screen 8",
			};

	int i, knt;
	int x, y, tw, th;

	unsigned int value;

	int num;

	/*
	 * Roy G. Biv test
	 * this test is basically a walking 1's type test.
	 * a specific color signifies one of eight bits.
	 */

	/*
	 * load the color map with appropriate values
	 */

	for (i = 0; i <= 255; i++)
	{
		red1[i] = 0;
		grn1[i] = 0;
		blu1[i] = 0;
	}
	red1[0] = 0; grn1[0] = 0; blu1[0] = 0;                  /* black */
	red1[255] = 0xFF; grn1[255] = 0xFF; blu1[255] = 0xFF;   /* white */

	red1[1] = 0xFF; grn1[1] = 0; blu1[1] = 0;               /* red */
	red1[2] = 255; grn1[2] = 128; blu1[2] = 0;               /* orange */
	red1[4] = 255; grn1[4] = 255; blu1[4] = 0;              /* yellow */
	red1[8] = 0; grn1[8] = 0xFF; blu1[8] = 0xFF;            /* cyan */
	red1[0x10] = 0; grn1[0x10] = 0xFF; blu1[0x10] = 0;	/* green */
	red1[0x20] = 0; grn1[0x20] = 0; blu1[0x20] = 0xFF;	/* blue */
	red1[0x40] = 255; grn1[0x40] = 183; blu1[0x40] = 183;	/* pink */
	red1[0x80] = 255; grn1[0x80] = 0; blu1[0x80] = 255;	/* violet */

	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);

	/* each rop will should have only a single bit
	 * set to 1.
	 */
	num = 0;
	for (i = 1; i <= 0x80; i <<= 1)
	{
		x = 0; y = 0;
		value = i;
		for (knt = 1; knt <= 8; knt++)
		{
			tw = width - x;
			th = height - y;
			ropsend(x,y,tw, th, value);
			x += width/8;
			y += height/8;
			if (value == 0x80)
				value = 0x1;
			else
				value <<= 1;
		}
		/*
		 * corrupt to test if set.
		 */
		if ( (simulate_error == CORRUPT_CG_LOWER_RT_A) ||
			(simulate_error == CORRUPT_CG_LOWER_RT_B) ) {
			pr_put(prfd, 1150, 898, 0x3);
			printf("corrupted 1150, 898\n");
		}

		if ( (simulate_error == CORRUPT_CG_UPPER_LT_A) ||
			(simulate_error == CORRUPT_CG_UPPER_LT_B) ) {
			pr_put(prfd, 1, 1, 0x3);
			printf("corrupted 1, 1\n");
		}

		/*
		 * read whats in the CG into mobs.
		 */

		pr_rop(mobs,0, 0, width, height,PIX_SRC,prfd,0,0);

		ropcheck(0,0,width, height,framename[num]);
		num++;

		check_input();


	}
	return (0);
}

/* send rop to CG and to expected */

ropsend (x, y, w, h, color)
int x, y, w, h, color;
{
	/*
	 * we rop into the frame buffer and again
	 * into the control memory.
	 * it is ok to rop into memory without locking because it
	 * is done into memory, not a common fb.
	 *
	 */

	PR_SYNC(gp_flag);
	pr_rop(prfd,x, y, w, h,PIX_SRC|PIX_COLOR(color),0,0,0);
	pr_rop(mexp,x, y, w, h,PIX_SRC|PIX_COLOR(color),0,0,0);

	return (0);

}

ropcheck(x, y, w, h, testname)
int x, y, w, h;
char *testname;
{
	int i;
	int endpt;

	int tx, ty, tmp;

	ptr1 = (unsigned int *) mexphead;
	ptr2 = (unsigned int *) mobshead;

	i = y * width + x;

	for (; i < w * h/ sizeof (ptr1) ; i++)
	{
		if (*ptr1 != *ptr2 || simulate_error == FRAME_BUFFER_ERROR)
		{
			tmp = i*sizeof (ptr1);
			tx = tmp % width;
			ty = ( tmp - tx ) / width;

             sprintf(msg, "CG5 %s failure, x pos = %d, y pos = %d, exp = 0x%x, actual = 0x%x.",
                        testname, tx, ty, *ptr1, *ptr2);
			errorprint(-FRAME_BUFFER_ERROR, msg);
		}

		++ptr1;
		++ptr2;
	}
	flush_io();
	return (0);
}

#define BLACK	0x0
#define WHITE	0x1

#define RED	0x55
#define GREEN	0xAA
#define BLUE	0x33
#define YELLOW	0xCC
#define CYAN	0x5
#define VIOLET	0x6

testdoublebuf()
{
	static unsigned char red[256], grn[256], blu[256];

	int x, y, w, h;

	/* first clear the frame buffer */

	rdispA_writeboth();
	PR_SYNC(gp_flag);
	pr_rop(prfd,0,0,prfd->pr_width,prfd->pr_height,
				PIX_SRC|PIX_COLOR(0), 0,0,0);

	/* set certain color map locations with data */

	red[BLACK] = 0x0;	grn[BLACK] = 0;		blu[BLACK] = 0;
	red[WHITE] = 0xFF;	grn[WHITE] = 0xFF,	blu[WHITE] = 0xFF;

	red[RED] = 0xFF;	grn[RED] = 0;		blu[RED] = 0;
	red[GREEN] = 0;		grn[GREEN] = 0xFF;	blu[GREEN] = 0;
	red[BLUE] = 0;		grn[BLUE] = 0;		blu[BLUE] = 0xFF;

	red[CYAN] = 0;		grn[CYAN] = 0xFF,	blu[CYAN] = 0xFF;
	red[VIOLET] = 0xFF;	grn[VIOLET] = 0,	blu[VIOLET] = 0xFF;
	red[YELLOW] = 0xFF;	grn[YELLOW] = 0xFF,	blu[YELLOW] = 0;

	red[255] = 0xFF;	grn[255] = 0xFF,	blu[255] = 0xFF;

	/*
	 * place in the colormap.
	 */

	pr_putcolormap(prfd, 0, 256, red, grn, blu);

	x = 300;
	y = 300;
	w = 600;
	h = 600;

	/* rop into A */
	rdispB_writeA();
	PR_SYNC(gp_flag);
	pr_rop(prfd, x, y, w, h, PIX_SRC|PIX_COLOR(RED), 0,0,0);

	/* rop into B */
	rdispA_writeB();
	PR_SYNC(gp_flag);
	pr_rop(prfd, x, y, w, h, PIX_SRC|PIX_COLOR(GREEN), 0,0,0);

	/* read whats in A and place into observed */
	pr_rop(mobs,x, y, w, h, PIX_SRC,prfd,x,y);

	/* rop whats expected into expected */
	pr_rop(mexp, x, y, w, h, PIX_SRC|PIX_COLOR(RED),0,0,0);

	ropcheck(x, y, w, h, "dblbuf A");

	rdispB_writeA();

	/* read whats in B and place into observed */
	pr_rop(mobs,x, y, w, h, PIX_SRC,prfd,x,y);

	/* rop whats expected into expected */
	pr_rop(mexp, x, y, w, h, PIX_SRC|PIX_COLOR(GREEN),0,0,0);

	ropcheck(x, y, w, h, "dblbuf B");

	return (0);
}

testfb_to_fb()
{
	int w, h,
	    x1, y1,
	    x2, y2,
	    x3, y3,
	    x4, y4;

	/* first clear the frame buffer */

	rdispA_writeboth();
	PR_SYNC(gp_flag);
	pr_rop(prfd,0,0,prfd->pr_width,prfd->pr_height,
				PIX_SRC|PIX_COLOR(BLACK), 0,0,0);

	w = 300;
	h = 300;

	/* upper left corner */
	x1 = 0;
	y1 = 0;

	/* lower right corner */
	x2 = prfd->pr_width - 300;
	y2 = prfd->pr_height - 300;

	/* upper right corner */
	x3 = prfd->pr_width - 300;
	y3 = 0;

	/* lower left corner */
	x4 = 0;
	y4 = prfd->pr_height - 300;

	PR_SYNC(gp_flag);
	pr_rop(prfd, x1, y1, w, h, PIX_SRC|PIX_COLOR(YELLOW), 0,0,0);
	PR_SYNC(gp_flag);
	pr_rop(prfd, x2, y2, w, h, PIX_SRC,prfd,x1,y1);
	PR_SYNC(gp_flag);
	pr_rop(prfd, x3, y3, w, h, PIX_SRC,prfd,x2,y2);
	PR_SYNC(gp_flag);
	pr_rop(prfd, x4, y4, w, h, PIX_SRC,prfd,x3,y3);

	/* read and place into observed */

	pr_rop(mobs,0, 0, prfd->pr_width, prfd->pr_height, PIX_SRC,prfd,0,0);

	/* rop whats expected into expected */

	pr_rop(mexp, 0, 0, mexp->pr_width, mexp->pr_height,
					PIX_SRC|PIX_COLOR(BLACK), 0,0,0);
	pr_rop(mexp, x1, y1, w, h, PIX_SRC|PIX_COLOR(YELLOW), 0,0,0);
	pr_rop(mexp, x2, y2, w, h, PIX_SRC,prfd,x1,y1);
	pr_rop(mexp, x3, y3, w, h, PIX_SRC,prfd,x2,y2);
	pr_rop(mexp, x4, y4, w, h, PIX_SRC,prfd,x3,y3);

	ropcheck(0, 0, prfd->pr_width, prfd->pr_height, "fb to fb");

	/* now we xor the whole frame buffer */

	PR_SYNC(gp_flag);
	pr_rop(prfd,0,0,prfd->pr_width,prfd->pr_height,
				PIX_SRC ^ PIX_DST | PIX_COLOR(BLUE), 0,0,0);

	/* rop frame buffer into observed */
	pr_rop(mobs,0,0,prfd->pr_width,prfd->pr_height, PIX_SRC, prfd,0,0);

	/* rop data into expected */
	pr_rop(mexp,0,0,prfd->pr_width,prfd->pr_height,
				PIX_SRC ^ PIX_DST | PIX_COLOR(BLUE), 0,0,0);

	ropcheck(0,0,prfd->pr_width,prfd->pr_height, "xor src to fb");

	return 0;
}

init_cmapnfb()
{
	int i, j, k, x, y, tw, th;

	/* clear the color map */

	for (i = 0; i <= 255; i++ )
		red1[i] = 0; grn1[i] = 0; blu1[i] = 0;

	blu1[255] = 1;	/* so that foreground and background are different.
			 * otherwise it will default to the suntools white.
			 * it will look black to the naked eye.
			 */

	PR_SYNC(gp_flag);
	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);
	usleep(100);

	/*
	 * put squares into the frame buffer
	 */

	if ( (width == 1152) && (height == 900) ) {
		tw = 72;
		th = 56;
	} else if ( height == width ) {
		tw = width / 16;
		th = height / 16;
	}

	k = 0;

	for (i = 0; i <= 15; i++ )
	{
		y = i * th;

		/* on last set of rops, factor in the left over
		 * pixels to the bottom of the screen.
		 */
		if (i == 15)
			th = height - th*15;

		for (j = 0; j <= 15; j++ )
		{
			if (j == 15)
				tw = width - tw*15;

			x = j * tw;
			PR_SYNC(gp_flag);
			pr_rop(prfd,x,y,tw,th,PIX_SRC|PIX_COLOR(k++),
								0,0,0);
		}
	}
	return 0;
}

testcolormap()
{
	unsigned int planes = 0xff;
	int i, j, k, knt;

	/* set certain color map locations with data */

	/*
	 * gray scale forward
	 */

	for (i = 0; i <= 255; i++)
	{
		red1[i] = i; grn1[i] = i; blu1[i] = i;
	}
	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);
	pr_getcolormap(prfd, 0, 256, red2, grn2, blu2);

	checkmap();

	/*
	 * gray scale backward
	 */

	for (i = 0, j = 255; i <= 255; i++, j--)
	{
		red1[i] = j; grn1[i] = j; blu1[i] = j;
	}
	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);
	pr_getcolormap(prfd, 0, 256, red2, grn2, blu2);

	checkmap();

	/*
	 * 5's and A's
	 */

	for (i = 0; i <= 255; i += 2)
	{
		red1[i] = 0xaa; red1[i+1] = 0x55;
		grn1[i] = 0x55; grn1[i+1] = 0xaa;
		blu1[i] = 0xaa; blu1[i+1] = 0x55;
	}

	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);
	pr_getcolormap(prfd, 0, 256, red2, grn2, blu2);

	checkmap();

	/*
	 * A's and 5's
	 */

	for (i = 0; i <= 255; i += 2)
	{
		red1[i] = 0x55; red1[i+1] = 0xaa;
		grn1[i] = 0xaa; grn1[i+1] = 0x55;
		blu1[i] = 0x55; blu1[i+1] = 0xaa;
	}

	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);
	pr_getcolormap(prfd, 0, 256, red2, grn2, blu2);

	checkmap();

	/*
	 * zoom through colors
	 * the colors will be
	 *
	 *	red to cyan
	 *	green to magenta
	 *	blue to yellow
	 *
	 *	cyan to white
	 *	magenta to white
	 *	yellow to white
	 *
	 *	white to red
	 *	white to green
	 *	white to blue
	 *
	 *	red to black
	 *	green to black
	 *	blue to black
	 *
	 *	black to cyan
	 *	black to magenta
	 *	black to yellow
	 *
	 *	white to black
	 *
	 */

	/*	red to cyan	*/

	k = 0; i = 0; j = 255;

	knt = 16;

	do {
		red1[k] = j; grn1[k] = i; blu1[k] = i;

		k++;
		i = k * 16 - 1;
		j = 255 - i;

	} while (k < knt);

	/*	green to magenta	*/

	i = 0; j = 255;

	knt += 16;

	do {
		red1[k] = i; grn1[k] = j; blu1[k] = i;

		k++;
		i = k * 16 - 1;
		j = 255 - i;

	} while (k < knt);

	/*	blue to yellow	*/

	i = 0; j = 255;

	knt += 16;

	do {
		red1[k] = i; grn1[k] = i; blu1[k] = j;

		k++;
		i = k * 16 - 1;
		j = 255 - i;

	} while (k < knt);

	/*	cyan to white	*/

	i = 0;
	knt += 16;

	do {
		red1[k] = i; grn1[k] = 255; blu1[k] = 255;

		k++;
		i = k * 16 - 1;

	} while (k < knt);

	/*	majenta to white	*/

	i = 0;
	knt += 16;

	do {
		red1[k] = 255; grn1[k] = i; blu1[k] = 255;

		k++;
		i = k * 16 - 1;

	} while (k < knt);

	/*	yellow to white	*/

	i = 0;

	knt += 16;

	do {
		red1[k] = 255; grn1[k] = 255; blu1[k] = i;

		k++;
		i = k * 16 - 1;

	} while (k < knt);

	/*	white to red	*/

	j = 255;

	knt += 16;

	do {
		red1[k] = 255; grn1[k] = j; blu1[k] = j;

		k++;
		j = 255 - k * 16 - 1;

	} while (k < knt);

	/*	white to green	*/

	j = 255;

	knt += 16;

	do {
		red1[k] = j; grn1[k] = 255; blu1[k] = j;

		k++;
		j = 255 - k * 16 - 1;

	} while (k < knt);

	/*	white to blue	*/

	j = 255;

	knt += 16;

	do {
		red1[k] = j; grn1[k] = j; blu1[k] = 255;

		k++;
		j = 255 - k * 16 - 1;

	} while (k < knt);

	/*	red to black	*/

	j = 255;

	knt += 16;

	do {
		red1[k] = j; grn1[k] = 0; blu1[k] = 0;

		k++;
		j = 255 - k * 16 - 1;

	} while (k < knt);

	/*	green to black	*/

	j = 255;

	knt += 16;

	do {
		red1[k] = 0; grn1[k] = j; blu1[k] = 0;

		k++;
		j = 255 -  k * 16 - 1;

	} while (k < knt);

	/*	blue to black	*/

	j = 255;

	knt += 16;

	do {
		red1[k] = 0; grn1[k] = 0; blu1[k] = j;

		k++;
		j = 255 - k * 16 - 1;

	} while (k < knt);

	/*	black to cyan	*/

	i = 0;

	knt += 16;

	do {
		red1[k] = 0; grn1[k] = i; blu1[k] = i;

		k++;
		i = k * 16 - 1;

	} while (k < knt);

	/*	black to magenta	*/

	i = 0;

	knt += 16;

	do {
		red1[k] = i; grn1[k] = 0; blu1[k] = i;

		k++;
		i = k * 16 - 1;

	} while (k < knt);

	/*	black to yellow	*/

	i = 0;

	knt += 16;

	do {
		red1[k] = i; grn1[k] = i; blu1[k] = 0;

		k++;
		i = k * 16 - 1;

	} while (k < knt);

	/*	white to black	*/

	j = 255;

	knt += 16;

	do {
		red1[k] = j; grn1[k] = j; blu1[k] = j;

		k++;
		j = 255 - k * 16 - 1;

	} while (k < knt);

	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);
	pr_getcolormap(prfd, 0, 256, red2, grn2, blu2);

	checkmap();

	return 0;
}

static char *dblarray[] = {
			"read memory set A",
			"read memory set B",
			"read A, write to A and B",
			"",
			};

char *dblptr;

checkmap()
{
	int i;

	dblptr = dblarray[3];

	for (i = 0; i <= 255; i++)
	{
		if (red1[i] != red2[i] || simulate_error == RED_FAILED)
		{
	  sprintf(msg, "Colormap error - red, loc = %d, exp = %d, actual = %d.",
				i, red1[i], red2[i]);
			errorprint(-RED_FAILED, msg);
		}
		if (grn1[i] != grn2[i] || simulate_error == GREEN_FAILED)
		{
	  sprintf(msg, "Colormap error - green, loc = %d, exp = %d, actual = %d.",
				i, grn1[i], grn2[i]);
			errorprint(-GREEN_FAILED, msg);
		}
		if (blu1[i] != blu2[i] || simulate_error == BLUE_FAILED)
		{
	  sprintf(msg, "Colormap error - blue, loc = %d, exp = %d, actual = %d.",
				i, blu1[i], blu2[i]);
			errorprint(-BLUE_FAILED, msg);
		}
	}
	flush_io();

	return 0;
}

enable_vid()
{
	int fd;
	int vid;

	vid = FBVIDEO_ON;

	fd = open(DEVICE, O_RDWR);
	if (fd > 0) {
		ioctl(fd, FBIOSVIDEO, &vid, 0);
		close(fd);
	} 
	return 0;
}

dblbuf_test()
{
	/* lets wait for a vertical retrace */
	ioctl(prfd, FBIOVERTICAL, 0);

	(void) pr_dbl_set(prfd, PR_DBL_WRITE, PR_DBL_B, 0);
	pr_put(prfd, 0, 0, 0x55);

	(void) pr_dbl_set(prfd, PR_DBL_WRITE, PR_DBL_A, 0);
	pr_put(prfd, 0, 0, 0xAA);

	(void) pr_dbl_set(prfd, PR_DBL_READ, PR_DBL_B, 0);
	if ( pr_get(prfd, 0, 0) == 0x55 ) {
		(void) pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A,
		   	PR_DBL_READ, PR_DBL_A, PR_DBL_WRITE, PR_DBL_BOTH, 0);
		return (1);
	} else {
		(void) pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A,
		   	PR_DBL_READ, PR_DBL_A, PR_DBL_WRITE, PR_DBL_BOTH, 0);
		return (0);
	}

/*### PR_DBL_AVAIL does not work in 3.5
	if ( pr_dbl_get(prfd, PR_DBL_AVAIL) == PR_DBL_EXISTS ) {
		return (1);
	}
	return 0;
###*/
}

rw_displayA()
{
	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

/*### removed because discrepancy between sun 4 and sun 3.
	if ( pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A,
		   PR_DBL_READ, PR_DBL_A, PR_DBL_WRITE, PR_DBL_A, 0) < 0 ) {

		dblflag = 0;
	} else
		dblflag = 1;
###*/
	(void) pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A,
		   PR_DBL_READ, PR_DBL_A, PR_DBL_WRITE, PR_DBL_A, 0);


	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

	dblptr = dblarray[0];
	return(0);
}

rw_displayB()
{

	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

/*### removed because discrepancy between sun 4 and sun 3.
	if ( pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_B,
		   PR_DBL_READ, PR_DBL_B, PR_DBL_WRITE, PR_DBL_B, 0) < 0 ) {

		dblflag = 0;
	} else
		dblflag = 1;

###*/
	(void) pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_B,
		   PR_DBL_READ, PR_DBL_B, PR_DBL_WRITE, PR_DBL_B, 0);
	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

	dblptr = dblarray[1];
	return(0);
}

rdispA_writeboth()
{

	/* read, display memory B.
	 * write to memory A.
	 */

	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

	(void) pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A,
			   PR_DBL_READ, PR_DBL_A,
			   PR_DBL_WRITE, PR_DBL_BOTH, 0);

	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

	dblptr = dblarray[2];
	return(0);
}

rdispB_writeA()
{

	/* read, display memory B.
	 * write to memory A.
	 */

	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

	(void) pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_B,
			   PR_DBL_READ, PR_DBL_B,
			   PR_DBL_WRITE, PR_DBL_A, 0);

	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

	dblptr = dblarray[1];
	return(0);
}

rdispA_writeB()
{

	/* read, display memory A.
	 * write to memory B.
	 */

	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

	(void) pr_dbl_set(prfd, PR_DBL_DISPLAY, PR_DBL_A,
			   PR_DBL_READ, PR_DBL_A,
			   PR_DBL_WRITE, PR_DBL_B, 0);

	/* lets wait for a vertical retrace
	ioctl(prfd, FBIOVERTICAL, 0);
	*/

	dblptr = dblarray[0];
	return(0);
}

setup_desktop()
{
	Cursor	cursor;
	struct       inputmask im;

	char         *temp_argv=NULL;
	char *winname; 

	unsigned int planes;

	/* if (single headed && window system ) we set up
	 * a new window.
	 * Otherwise we just do test on CG.
	 */

	if ( ( (return_code == CG_1BUF) || (return_code == CG_2BUF) )
				&& (winname = getenv("WINDOW_PARENT")) )
	{
		iograb = TRUE;

	/* we are running under window system */

	/*
	 * open up the suntools parent
	 * environment.
 	 */
		rootfd = open(winname, O_RDONLY);
		if (rootfd <= 0)
			syserror(-DEV_NOT_OPEN, "rootfd");


	/*
	 * get a new window
	 */
		winfd = win_getnewwindow();
		if (winfd < 0)
			syserror(-SCREEN_NOT_OPEN, "winfd");

		/* make 'suntools' the 'parent'. */
		win_setlink(winfd, WL_PARENT,
			win_nametonumber(winname));
 
		/* set oldest child of new node to be
		   the youngest child of parent. */

		win_setlink(winfd, WL_OLDERSIB,
			win_getlink(rootfd, WL_YOUNGESTCHILD));

	/*
	 * get screen size of suntools window
	 * and set the size of new window same size.
	 */

		win_screenget(rootfd, &new_screen);
		win_setrect(winfd, &new_screen.scr_rect);

		input_imnull(&im);
		im.im_flags |= IM_ASCII;	/* catch 0-127(CTRL-C == 3) */
		win_set_kbd_mask(winfd, &im);

		/* put the new node into the tree. */
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

		/* remove the mouse.
		 * this is necessary because the mouse
		 * is still active over the other windows and
		 * clicking the mouse button will cause an interrupt.
		 * I will fix it when exitting.
		 */
		win_remove_input_device(rootfd,
					new_screen.scr_msname);



	}

	enable_vid();

	width = prfd->pr_width;
	height = prfd->pr_height;
	depth = prfd->pr_depth;

	/* save the current CG state */

	pr_getattributes(prfd, &tmpplane);
	pr_getcolormap(prfd, 0, 256, tred, tgrn, tblu);

	/* enable all planes */
	planes = 0xff;
	pr_putattributes(prfd, &planes);

	return (0);
}


errorprint(err, msg)
int err;
char *msg;
{
	if (!exec_by_sundiag) {
		clean_up();
		exit(0);
	} else
	cg_send_message(err, FATAL, msg);

	return (0);
}

syserror(err, c)
int err;
char *c;
{
	perror(c);
	sprintf(msg, "Couldn't create new screen for '%s'.",
		DEVICE);

	if (!exec_by_sundiag) {
		clean_up();
		exit(0);
	} else
	cg_send_message(err, FATAL, msg);

	return (0);
}

struct       sgttyb  ttybuf;
int origsigs;
short	orig_sg_flags;		/* orig mode flags for stdin */

setup_signals()
{

	/* set up signals for non-blocking
	 * input.  If this is not done,
	 * the windows will have a queue of
	 * input events to handle.
	 */

	ioctl(0, TIOCGETP, &ttybuf);
	orig_sg_flags = ttybuf.sg_flags;/* save original mode flags */
	ttybuf.sg_flags |= CBREAK;      /* non-block mode */
	ttybuf.sg_flags &= ~ECHO;       /* no echo */
	ioctl(0, TIOCSETP, &ttybuf);

	origsigs = sigsetmask(0);
	signal(SIGINT, finish);
	signal(SIGQUIT, finish);
	signal(SIGHUP, finish);
	return (0);
}

#define BUFSIZE 256
static char tmpbuf[BUFSIZE];

flush_io()
{
	int tfd, arg, knt;

	arg = 0;
	if (devtopfd)
		tfd = devtopfd;
	else
		tfd = 0;

	ioctl(tfd, FIONREAD, &arg);

	knt = BUFSIZE;
	while (arg > 0)
	{
		if (arg < knt)
			knt = arg;

		knt = read (tfd, tmpbuf, knt);
		arg -= knt;
	}
	return (0);
}

reset_signals()
{
	ioctl(0, TIOCGETP, &ttybuf);
	ttybuf.sg_flags = orig_sg_flags;	/* restore orig mode flags */
	ioctl(0, TIOCSETP, &ttybuf);

	/* set up original signals */

	sigsetmask(origsigs);
	signal ( SIGINT, SIG_IGN );
	signal ( SIGQUIT, SIG_IGN );
	signal ( SIGHUP, SIG_IGN );
	/* flush the buffer */

	flush_io();
	return (0);
}


clean_up()
{
	Cursor	cursor;
	int msfd;
	int flag;

	reset_signals();

	if (prfd <= 0)
		return(0);

	if ( dblflag )
		rdispA_writeboth();
	pr_putattributes(prfd, &tmpplane);
	pr_putcolormap(prfd, 0, 256, tred, tgrn, tblu);

	if (iograb == TRUE )
	{

			cursor = cursor_create((char *)0);
			win_getcursor(rootfd, cursor);
			cursor_set(cursor, CURSOR_SHOW_CURSOR, 1, 0);
			win_setcursor(rootfd, cursor);
			cursor_destroy(cursor);

			win_releaseio(winfd); 

			flag = win_is_input_device(rootfd, "/dev/mouse");

			/* re-install the mouse */
         		msfd = open(new_screen.scr_msname, O_RDWR );
			if (msfd >= 0) {
				win_set_input_device(rootfd, msfd,
						new_screen.scr_msname);
				close (msfd);
			wmgr_refreshwindow ( rootfd );
			}

	}

	mem_destroy(mobs);
	mem_destroy(mexp);
	if ( prfd > 0 )
		pr_destroy(prfd);

	return (0);
}

check_input()
{
	Event	event;
	int arg;

	arg = 0;
	ioctl(winfd, FIONREAD, &arg);
	if (arg != 0) {
		input_readevent(winfd, &event);
		if (event_id(&event) == 0x03)		/* CTRL-C */
			finish();
	}
	return(0);
}

cg_send_message(where, msg_type, msg)
    int             where;
    int             msg_type;
    char           *msg;
{
    if ( iograb ) {
       win_releaseio ( winfd );
       sleep(15);
    }

    send_message(where, msg_type, msg);

    if ( iograb )
       win_grabio ( winfd );
}
