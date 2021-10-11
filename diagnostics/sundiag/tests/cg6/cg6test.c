
static char version[] = "Version 1.1";
static char     sccsid[] = "@(#)cg6test.c 1.1 7/30/92 Copyright 1988 Sun Micro";

#include "cg6test.h"
#include <machine/param.h>

#ifdef DELAY
#undef DELAY
#define DELAY(xxx)	{ register int i=xxx; while (i--); }
#endif DELAY

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

#include <sundiag_rpcnums.h>     /* sundiag routines */
#include <sdrtns.h>

#define DEVICE			"/dev/cgsix0"


static unsigned char tred[256], tgrn[256], tblu[256];

unsigned char red1[256], grn1[256], blu1[256];

static unsigned char red2[256], grn2[256], blu2[256];

static int rootfd, devtopfd;

int testing_secondary_buffer;
Pixrect *prfd, *sprfd;

struct screen new_screen;

int width, height, depth;

unsigned int tmpplane;

int  cleanup_ioctl = FALSE;
int tmpfbc, fbc_rev, tmptec, tec_fbc_test_flag;
int dblflag, typeflag, lockflag;
struct pixrect *mexp, *mobs;

#define SINGLEHEAD	0x0
#define DUALHEAD	0x1
#define DBLBUF		0x2
#define DBLBUF2HEAD	0x3  /* DUALHEAD | DBLBUF */

#define read_lego_fbc(addr)             (*(addr))
#define write_lego_fbc(addr, datum)     ((*(addr)) = (datum))
#define read_lego_tec(addr)             (*(addr))
#define write_lego_tec(addr, datum)     ((*(addr)) = (datum))

/* LOCALS */
static int simulate_error, return_code;

static char msg[80];
static int  dummy(){return(FALSE);}
char                    *routine_msg="Routine ";
char 			*test_usage_msg= "[D=device name]";
char			*program_name;

main(argc, argv)
	int argc;
	char *argv[];
{
	int tmpsim, probecode;
	u_long  *fbc_base;
    	extern int process_test_args();
    	extern buserr();
    	extern int routine_usage();
	struct  fbtype  fb_type;
        struct  fbgattr fb_gattr;
        unsigned int planes = 0xff;


#ifdef FBIOGXINFO
	struct cg6_info cinfo;
#endif FBIOGXINFO
	int tmpfd;

    program_name = argv[0];
    test_name = argv[0];
    device_name = DEVICE;
    fbc_rev = 0;
    testing_secondary_buffer = FALSE;
	func_name = "main";
	TRACE_IN

	

	typeflag = 0;
	lockflag = 0;
	dblflag= FALSE;
	tec_fbc_test_flag=FALSE;
	simulate_error = 0;
	prfd = (Pixrect *) 0;
			/* begin Sundiag test */
test_init(argc, argv, process_test_args, routine_usage, test_usage_msg);
	/*
	 * initialize the mem pixrect
	 */
	prfd = pr_open(device_name);

      	if (prfd <= 0) {
		pr_close(prfd);
		test_end();
	}

                fb_gattr.real_type = 10;

                if ((tmpfd=open(device_name, O_RDONLY)) != -1) {
                        ioctl(tmpfd, FBIOGATTR, &fb_gattr);
			
                        close (tmpfd);
                }
/*                if (fb_gattr.real_type == FBTYPE_SUNFAST_COLOR )  {
*		    if ( strcmp("/dev/cgsix0",device_name) == 0) {
*			    device_name = "/dev/fb";
*
*		    }
*
*		}
*/
	enable_vid(device_name);  /* enable video, mainly for 2nd/3rd cg6's */
	fbc_base = (u_long *) cg6_d(prfd)->cg6_fbc;
	fbc_rev = read_lego_fbc( fbc_base ) & 0x0f00000 ;
#ifdef FBIOGXINFO
/*	printf("fbc_rev = 0x%x\n",fbc_rev);				*/

	if((tmpfd=open(device_name, O_RDONLY)) == -1)
		printf("\nopen failed");
	else {
		if( ioctl(tmpfd, FBIOGXINFO, &cinfo)>=0) {
			if((cinfo.vmsize==4)&&(cinfo.hdb_capable)) 
				dblflag=TRUE;
		}
                close (tmpfd);
	}

#endif FBIOGXINFO

 	lock_desktop(device_name); 
	lockflag=TRUE;

        width = prfd->pr_width;
        height = prfd->pr_height;
        depth = prfd->pr_depth;
        pr_getattributes(prfd, &tmpplane);
        pr_getcolormap(prfd, 0, 256, tred, tgrn, tblu);

        /* enable all planes */
        planes = 0xff;
        pr_putattributes(prfd, &planes);

 	initmem();
	test_tec();
	test_fbc();

	tmpsim = simulate_error;
	if ( (simulate_error == CORRUPT_CG_LOWER_RT_B) ||
		(simulate_error == CORRUPT_CG_UPPER_LT_B) ) {
		simulate_error = 0;
	}

	clear_screen();
	testframebuffer();
	simulate_error = tmpsim;
	testfb_to_fb();
	test_blits();
	test_lines();
	test_polygons();
	init_cmapnfb( red1, grn1, blu1);
	DELAY(10000);
	testcolormap();
	clear_screen();
	test_sine();
        pr_putcolormap(prfd, 0, 256, tred, tgrn, tblu);
	clear_screen();

#ifdef FBIOGXINFO
	if( dblflag == TRUE )
        	dbuf();
#endif FBIOGXINFO
        pr_putcolormap(prfd, 0, 256, tred, tgrn, tblu);
	clear_screen();
	clean_up();
	if (!exec_by_sundiag)
		printf ("end of test.\n");

done:
	test_end(); 
}

process_test_args(argv, arrcount)
char    *argv[];
int     arrcount;
{
    func_name = "process_test_args";
    TRACE_IN

    if (strncmp(argv[arrcount], "D=", 2) == 0) {
        device_name = argv[arrcount]+2;
        TRACE_OUT
        return(TRUE);
	}
    else return(FALSE);
}

routine_usage()
{

    func_name = "routine_usage";
    TRACE_IN

    (void)send_message(SKIP_ERROR, CONSOLE, routine_msg, test_name);

    TRACE_OUT
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

	func_name = "testframebuffer";
	TRACE_IN
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
	
        width = prfd->pr_width;
        height = prfd->pr_height;
        depth = prfd->pr_depth;
	num = 0;
	tw = width / depth;
	th = height;
	y = 0;
	for (i = 1; i <= 0x80; i <<= 1)
	{
		x = 0;
		value = i;
		for (knt = 0; knt < 8; knt++)
		{
			ropsend(x,y,tw, th, value);
			x += tw;
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

	}
	TRACE_OUT
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

             sprintf(msg, "CG6 %s failure, x pos = %d, y pos = %d, exp = 0x%x, actual = 0x%x.",
                        testname, tx, ty, *ptr1, *ptr2);
			errorprint(-FRAME_BUFFER_ERROR, msg);
		}

		++ptr1;
		++ptr2;
	}

	check_input();
	return (0);
}

#define BLACK	0x0
#define WHITE	0x1

#define RED	0x55
#define GREEN	0xAA
#define BLUE	0x33
#define YELLOW	0xCC
#define CYAN	0x66
#define VIOLET	0x99

clear_screen()
{
	func_name = "clear_screen";
	TRACE_IN
	pr_rop(prfd,0,0,width,height,
				PIX_SRC|PIX_COLOR(BLACK), 0,0,0);
 
	if(mobs>0)
	pr_rop(mobs,0, 0, width, height,
				PIX_SRC | PIX_COLOR(0), 0,0,0);
	if(mexp>0)
	pr_rop(mexp,0, 0, width, height,
				PIX_SRC | PIX_COLOR(0), 0,0,0);
 
	TRACE_OUT
	return(0);
}

testfb_to_fb()
{
	int i;
	int w, h,
	    x1, y1,
	    x2, y2,
	    x3, y3,
	    x4, y4;

	func_name= "testfb_to_fb";
	TRACE_IN
	clear_screen();
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

	red1[RED] = 0xFF; grn1[RED] = 0; blu1[RED] = 0;         /* red */
	red1[GREEN] = 0; grn1[GREEN] = 0xFF; blu1[GREEN] = 0;	/* green */
	red1[BLUE] = 0; grn1[BLUE] = 0; blu1[BLUE] = 0xFF;	/* blue */
	red1[YELLOW] = 255; grn1[YELLOW] = 255; blu1[YELLOW] = 0; /* yellow */
	red1[CYAN] = 0; grn1[CYAN] = 0xFF; blu1[CYAN] = 0xFF;     /* cyan */
	red1[VIOLET] = 255; grn1[VIOLET] = 0; blu1[VIOLET] = 255; /* violet */

	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);

	w = 300;
	h = 300;

	/* upper left corner */
	x1 = 0;
	y1 = 0;

	/* lower right corner */
	x2 = width - 300;
	y2 = height - 300;

	/* upper right corner */
	x3 = width - 300;
	y3 = 0;

	/* lower left corner */
	x4 = 0;
	y4 = height - 300;

	/* bit blit of CYAN */
	pr_rop(prfd, x1, y1, w, h, PIX_SRC|PIX_COLOR(CYAN), 0,0,0);
	pr_rop(prfd, x2, y2, w, h, PIX_SRC,prfd,x1,y1);
	pr_rop(prfd, x3, y3, w, h, PIX_SRC,prfd,x2,y2);
	pr_rop(prfd, x4, y4, w, h, PIX_SRC,prfd,x3,y3);

	/* read and place into observed */

	pr_rop(mobs,0, 0, width, height, PIX_SRC,prfd,0,0);

	/* rop whats expected into expected */

	pr_rop(mexp, 0, 0, width, height,
					PIX_SRC|PIX_COLOR(BLACK), 0,0,0);
	pr_rop(mexp, x1, y1, w, h, PIX_SRC|PIX_COLOR(CYAN), 0,0,0);
	pr_rop(mexp, x2, y2, w, h, PIX_SRC,prfd,x1,y1);
	pr_rop(mexp, x3, y3, w, h, PIX_SRC,prfd,x2,y2);
	pr_rop(mexp, x4, y4, w, h, PIX_SRC,prfd,x3,y3);

	ropcheck(0, 0, width, height, "fb to fb");

	/* now we xor the whole frame buffer */

	pr_rop(prfd,0,0,width,height,
				PIX_SRC ^ PIX_DST | PIX_COLOR(VIOLET), 0,0,0);

	/* rop frame buffer into observed */
	pr_rop(mobs,0,0,width,height, PIX_SRC, prfd,0,0);

	/* rop data into expected */
	pr_rop(mexp,0,0,width,height,
				PIX_SRC ^ PIX_DST | PIX_COLOR(VIOLET), 0,0,0);

	ropcheck(0,0,width,height, "xor src to fb");

	TRACE_OUT
	return(0);
}

test_blits()
{
	int w, h,
	    x1, y1,
	    x2, y2,
	    x3, y3,
	    x4, y4;

	    x1 = 0; y1 = 0;
	    x2 = 300; y2 = 300;

	    w = 250; h = 250;

	clear_screen();
	/* bit blit of CYAN */
	pr_rop(prfd, x1, y1, w, h, PIX_SRC|PIX_COLOR(CYAN), 0,0,0);
	pr_rop(prfd, x2, y2, w, h, PIX_SRC,prfd,x1,y1);

	return(0);
}

init_cmapnfb(red1, grn1, blu1)
unsigned char red1[256], grn1[256], blu1[256];
{
	int i, j, k, x, y, tw, th;

	clear_screen();

	/*
	 * put squares into the frame buffer
	 */

	if ( (width == 1152) && (height == 900) ) {
		tw = 72;
		th = 56;
	} else {
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
			pr_rop(prfd,x,y,tw,th,PIX_SRC|PIX_COLOR(k++),
								0,0,0);
		}
	}

	/* clear the color map */

	for (i = 0; i <= 255; i++ )
		red1[i] = 0; grn1[i] = 0; blu1[i] = 0;

	blu1[255] = 1;	/* so that foreground and background are different.
			 * otherwise it will default to the suntools white.
			 * it will look black to the naked eye.
			 */

	pr_putcolormap(prfd, 0, 256, red1, grn1, blu1);

	return(0);
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
	DELAY(10000);

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
	DELAY(10000);

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
	DELAY(10000);

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
	DELAY(10000);

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
	DELAY(10000);

	return(0);
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

	return(0);
}

enable_vid(name)
char	*name;
{
	int fd;
	int vid;

	vid = FBVIDEO_ON;

	fd = open(name, O_RDWR);
	if (fd > 0) {
		ioctl(fd, FBIOSVIDEO, &vid);
		close(fd);
	} else
		printf("warning, could not turn on video\n");
	return(0);
}


errorprint(err, msg)
int err;
char *msg;
{
	char *msg1[200];
	if(testing_secondary_buffer ==TRUE)
	sprintf(msg1, "%s Testing hidden buffer. (Buffer 1).", msg);
	else 
	sprintf(msg1, "%s Testing visible buffer. (Buffer 0). ", msg);
	send_message(err, FATAL_MSG, msg1);
	return (0);
}

syserror(err, c)
int err;
char *c;
{
	perror(c);
	sprintf(msg, "Couldn't create new screen for '%s'.",
		DEVICE);
		send_message(err, FATAL_MSG, msg);
	return (0);
}

struct       sgttyb  ttybuf;
int origsigs;
short	orig_sg_flags;	/* original mode flags for stdin */

finish1()
{
        u_long *tec, *fbc;
 
        tec = (u_long *) cg6_d(prfd)->cg6_tec;
        fbc = (u_long *) cg6_d(prfd)->cg6_fbc;
 
        if (tec_fbc_test_flag==TRUE)
        {       *tec = tmptec;
                *fbc = tmpfbc;
            tec_fbc_test_flag=FALSE;
        }
        finish();
}

setup_signals()
{

	/* set up signals for non-blocking
	 * input.  If this is not done,
	 * the windows will have a queue of
	 * input events to handle.
	 */

	ioctl(0, TIOCGETP, &ttybuf);
	orig_sg_flags = ttybuf.sg_flags;/* save orig mode flags for stdin */
	ttybuf.sg_flags |= CBREAK;      /* non-block mode */
	ttybuf.sg_flags &= ~ECHO;       /* no echo */
	ioctl(0, TIOCSETP, &ttybuf);

	origsigs = sigsetmask(0);
	signal(SIGINT, finish);  /* To be replaced by finish1 ? */
	signal(SIGQUIT, finish);
	signal(SIGHUP, finish);

	return (0);
}

reset_signals()
{
	ioctl(0, TIOCGETP, &ttybuf);
	ttybuf.sg_flags = orig_sg_flags; /* restore orig mode flags */
	ioctl(0, TIOCSETP, &ttybuf);

	/* set up original signals */

	sigsetmask(origsigs);
	/* flush the buffer */

	return (0);
}

clean_up()
{
    func_name = "clean_up";

	if(lockflag)
    	(void)unlock_desktop();  
	if (prfd >0)
        pr_rop(prfd,0,0,width,height,
                                PIX_SRC|PIX_COLOR(BLACK), 0,0,0);
	return(0);
}


#define prt(bs, aa) printf ( "aa 0x%x\n", *(bs + aa) )

test_tec()
{
	register u_long *tec;
	u_long tmp1;

	func_name = "test_tec";
	tec_fbc_test_flag = TRUE;
	TRACE_IN
	tec = (u_long *) cg6_d(prfd)->cg6_tec;

/*
 	tmptec= *tec;	
	*tec = 0xaaaa;
	tmp1 = *tec;
	
	if (tmp1 != 0xa22a)
printf("TEC write/read test failed at address 0x0 . Expected (0xa22a ), observed (0x%x), xor (0x%x)", tmp1, tmptec, tmptec^tmp1 );

	*tec = tmptec;
	tec_fbc_test_flag = FALSE;
*/
	TRACE_OUT
	return(0);
}

test_fbc()
{
	register u_long  *fbc_base;
	u_long tmp1;

	func_name = "test_fbc";
	TRACE_IN
	fbc_base = (u_long *) cg6_d(prfd)->cg6_fbc;

/*
	tmpfbc= *fbc_base;
	tec_fbc_test_flag = TRUE;
        *fbc_base = 0x5555aaaa;  
        tmp1 = *fbc_base;    
         
        if (tmp1 != 0x5555aaaa) 
printf("FBC write/read test failed at address 0x80. Expected (0x5555aaaa), observed (0x%x), xor (0x%x)", tmp1, tmpfbc^tmp1 );

        *fbc_base = tmpfbc; 
	tec_fbc_test_flag = FALSE;
*/
/*###
	prt (fbc_base, L_FBC_RASTEROP);
	prt (fbc_base, L_FBC_MISC);
	printf ("test 0x%x\n", read_lego_fbc(fbc_base+L_FBC_STATUS));
###*/
	TRACE_OUT
	return(0);
}

test_sine()
{
	func_name = "test_sine";
	TRACE_IN
	run_sine();
	pr_rop(mexp,0, 0, width, height,PIX_SRC,prfd,0,0);
	run_sine();
	pr_rop(mobs,0, 0, width, height,PIX_SRC,prfd,0,0);

	ropcheck(0,0,width, height, "sine test");
	TRACE_OUT
	return(0);
}
