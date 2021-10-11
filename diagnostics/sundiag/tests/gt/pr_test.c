#ifndef lint
static  char sccsid[] = "@(#)pr_test.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


/**********************************************************************/
/* Include files */
/**********************************************************************/

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>
#include <pixrect/pixrect_hs.h>
#include <pixrect/gtvar.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gttest.h>
#include <errmsg.h>

#define CHUNK_WIDTH	64
#define CHUNK_HEIGHT	64
#define NBANK		5


/**********************************************************************/
/* External variables */
/**********************************************************************/

extern
int		ierr;

extern
long		random();

extern
char		*sprintf();

extern
char		*initstate(),
		*setstate();

extern
time_t		time();

extern
char		*device_name;

/**********************************************************************/
/* Static variables */
/**********************************************************************/

static char *groupnames[] = { "CURRENT plane",
			     "MONO plane",
			     "8-BIT COLOR plane",
			     "OVERLAY ENABLE plane",
			     "OVERLAY plane",
			     "24-BIT COLOR plane",
			     "VIDEO PLANE",
			     "VIDEO ENABLE plane",
			     "TRANSPARENT OVERLAY plane",
			     "WINDOW ID plane",
			     "Z-BUFFER",
			     "CURSOR ENABLE plane",
			     "CURSOR plane",
			     "LAST-PLUS-ONE plane", };

static
char groups[PIXPG_INVALID+1];

static
int ngroups;

static
long rand_state[32]  =  {
		   3,  0x9a319039,  0x8999220b,       0x27fb47b9,
	  0x32d9c024,  0x9b663182,  0x5da1f342,       0x7449e56b,
          0xbeb1dbb0,  0xab5c5918,  0x946554fd,       0x8c2e680f,
          0xeb3d799f,  0xb11ee0b7,  0x2d436b86,       0xda672e2a,
          0x1588ca88,  0xe369735d,  0x904f35f7,       0xd7158fd6,
          0x6fa6f051,  0x616e6b96,  0xac94efdc,       0xde3b81e0,
          0xdf0a6fb5,  0xf103bc02,  0x48f340fb,       0x36413f93,
          0xc622c298,  0xf5a42ab8,  0x8a88d77b,       0xf5ad9d0e, };

static
time_t		ltime;

static
Pixrect		*device_pr = (Pixrect *)NULL;

static
int		def_wid = 0;

/**********************************************************************/
Pixrect *
open_pr_device()
/**********************************************************************/
{
    int op;
    int wpart = 0;
    int fd;


    func_name = "open_pr_device";
    TRACE_IN

    if (device_pr == (Pixrect *)NULL) {
	device_pr = pr_open(device_name);
	if (device_pr == NULL || device_pr < 0) {
	    dump_status();
	    (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_OPEN, device_name);
	}
    }

    fd = open(device_name, O_RDWR);
    if (fd < 0) {
	dump_status();
	(void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_OPEN);
    }
    if (ioctl(fd, FB_GETWPART, &wpart) < 0) {
	dump_status();
	(void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_GET_WPART);
    }
    if (wpart != 5) {
	wpart = 5;
	if (ioctl(fd, FB_SETWPART, &wpart) < 0) {
	    dump_status();
	    (void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_SET_WPART);
	}
    }
    (void)close(fd);

    TRACE_OUT
    return (device_pr);
}

/**********************************************************************/
int
wid_alloc(pixr, attrb)
/**********************************************************************/
Pixrect *pixr;
int attrb;
{

    struct fb_wid_alloc	winfo;		/* from fbio.h */
    int fd;

    func_name = "wid_alloc";
    TRACE_IN

    winfo.wa_index = -1;
    winfo.wa_count = 1;
    winfo.wa_type = attrb;	
    if (pr_ioctl(pixr, FBIO_WID_ALLOC, &winfo) < 0) {
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_UNIQUE_WID);
    } else {
	def_wid = winfo.wa_index;
    }

    /*
    fd = open(device_name, O_RDWR);
    if (fd < 0) {
	dump_status();
	(void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_OPEN);
    }
    winfo.wa_index = -1;
    winfo.wa_count = 1;
    winfo.wa_type = attrb;	
    if (ioctl(fd, FBIO_WID_ALLOC, &winfo) < 0) {
	dump_status();
	(void)fb_send_message(HAWK_FATAL_ERROR, FATAL, DLXERR_UNIQUE_WID);
    }
    def_wid = winfo.wa_index;
    (void)close(fd);
    */


    TRACE_OUT
    return(def_wid);
}

/**********************************************************************/
int
wid_free(pixr, ind, attrib)
/**********************************************************************/
Pixrect *pixr;
int ind;
int attrib;
{
    struct fb_wid_alloc	winfo;		/* from fbio.h */

    func_name = "wid_free";
    TRACE_IN

    winfo.wa_index = ind;
    winfo.wa_count = 1;
    winfo.wa_type = attrib;	
    if (pr_ioctl(pixr, FBIO_WID_FREE, &winfo) < 0) {
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_WID_FREE);
    }

    TRACE_OUT
    return 0;
}

/**********************************************************************/
int
clut_alloc(pixr)
/**********************************************************************/
Pixrect *pixr;
{
    struct fb_clutalloc clut;

    func_name = "clut_alloc";
    TRACE_IN

    clut.flags = CLUT_8BIT;
    clut.clutindex = -1;
    if (pr_ioctl(pixr, FB_CLUTALLOC, &clut) < 0) {
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_CLUT_ALLOC);
    }

    TRACE_OUT
    return clut.index;
}
	
/**********************************************************************/
int
clut_free(pixr, ind)
/**********************************************************************/
Pixrect *pixr;
int ind;

{
    struct fb_clutalloc clut;

    func_name = "clut_alloc";
    TRACE_IN

    clut.flags = CLUT_8BIT;
    clut.index = ind;
    if (pr_ioctl(pixr, FB_CLUTFREE, &clut) < 0) {
	(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_CLUT_FREE);
    }

    TRACE_OUT
    return 0;
}

/**********************************************************************/
close_pr_device()
/**********************************************************************/
{

    func_name = "close_pr_device";
    TRACE_IN

    if (device_pr) {
	(void)pr_close(device_pr);
    }
    device_pr = (Pixrect *)NULL;

    TRACE_OUT

}

/**********************************************************************/
char *pr_test(pr, mask)
/**********************************************************************/
Pixrect *pr;
register int mask;
/* Test the pixrect memory by writing a random pattern to it */

{
    static char errtxt[256];
    char errtxt2[256];
    Pixrect	*mprs = (Pixrect *) NULL,		/* source pixrect */
		*mprd = (Pixrect *) NULL,		/* destination pixrect */
		*mprc = (Pixrect *) NULL,		/* complement pixrect */
		*mpr_save = (Pixrect *) NULL;

    register *wptr;
    register *rptr;
    register *cptr;
    register int i;
    register int j;
    register int k;
    register int pps;
    register int rest;
    int chunk_width = CHUNK_WIDTH;
    int chunk_width_rest;
    int chunk_height = CHUNK_HEIGHT;
    int chunk_height_rest;
    int nx;
    int ny;
    char err=0;

    int scr_x;
    int scr_y;
    int bank;

    func_name = "pr_test";
    TRACE_IN

    /* initialize state of the random generator */
    ltime = time((time_t *)0);
    (void)initstate((unsigned int)ltime, (char *)rand_state, 128);
    (void)setstate((char *)rand_state);

    /* create memory pixrect to save the original screen */
    mpr_save = mem_create(chunk_width, chunk_height, pr->pr_depth);
    if (mpr_save == NULL) {
	(void)strcpy(errtxt, DLXERR_OPEN_MEM_PIXRECT);
	goto error_return;
    }

    /* create memory pixrect to puit random patterns */
    mprs = mem_create(chunk_width, chunk_height, pr->pr_depth);
    if (mprs == NULL) {
	(void)strcpy(errtxt, DLXERR_OPEN_MEM_PIXRECT);
	goto error_return;
    }

    pps = (chunk_width * chunk_height * pr->pr_depth)/
						    (sizeof(int) << 3);

    wptr = (int *)mpr_d(mprs)->md_image;

    for (i = 0 ; i < pps - 1 ; i++) {
	*wptr++ = random();
    }

    if ((rest = pps % sizeof(int))) {
	char *cp = (char *)wptr;
	for (i = rest ; i > 0 ; i--) {
	    *cp++ = (char)random();
	}
    } else {
	*wptr = random() & mask;
    }

    mprd = mem_create(chunk_width, chunk_height, pr->pr_depth);
    if (mprd == NULL) {
	(void)strcpy(errtxt, DLXERR_OPEN_MEM_PIXRECT);
	goto error_return;
    }

    mprc = mem_create(chunk_width, chunk_height, pr->pr_depth);
    if (mprc == NULL) {
	(void)strcpy(errtxt, DLXERR_OPEN_MEM_PIXRECT);
	goto error_return;
    }

    /* calculate number of loops */
    nx = pr->pr_width / chunk_width;
    chunk_width_rest = pr->pr_width % chunk_width;
    if (chunk_width_rest)
	nx++;
    else
	chunk_width_rest = chunk_width;	
    ny = pr->pr_height / chunk_height;
    chunk_height_rest = pr->pr_height % chunk_height;
    if (chunk_height_rest)
	ny++;
    else
	chunk_height_rest = chunk_height;

    for (i = 0 ; i < ny ; i++) {
	int pr_y = i*chunk_height;	/* y origin of random pixrect */
	if (i == ny-1) chunk_height = chunk_height_rest;
	for (j = 0 ; j < nx ; j++) {
	    int pr_x = j*chunk_width;	/* x origin of random pixrect */
	    if (j == nx-1) chunk_width = chunk_width_rest;

	    (void)check_key();

	    /* save the current screen */
	    (void)pr_rop (mpr_save, 0, 0, chunk_width, chunk_height,
				PIX_SRC | PIX_DONTCLIP, pr, pr_x, pr_y);

	    /* write pattern to pixrect device */
	    (void)pr_rop (pr, pr_x, pr_y, chunk_width, chunk_height,
				    PIX_SRC | PIX_DONTCLIP, mprs, 0, 0);

    	    wptr = (int *)mpr_d(mprs)->md_image;

	    /* copy screen into memory pixrect */
	    (void)pr_rop (mprd, 0, 0, chunk_width, chunk_height,
				PIX_SRC | PIX_DONTCLIP, pr, pr_x, pr_y);

            rptr = (int *)mpr_d(mprd)->md_image;

	    /* write the complement pattern to pixrect device */
	    (void)pr_rop (pr, pr_x, pr_y, chunk_width, chunk_height,
				PIX_NOT(PIX_SRC), mprs, 0, 0);

	    /* copy screen into memory pixrect */
	    (void)pr_rop (mprc, 0, 0, chunk_width, chunk_height,
				PIX_SRC | PIX_DONTCLIP, pr, pr_x, pr_y);

            cptr = (int *)mpr_d(mprc)->md_image;

	    /* restore previous screen */
	    (void)pr_rop (pr, pr_x, pr_y, chunk_width, chunk_height,
				PIX_SRC | PIX_DONTCLIP, mpr_save, 0, 0);

	    if ((rest= pps % sizeof(int))) pps--;

	    for (k = 0 ; k < pps ; k++) {

	   	scr_x = pr_x + (k % chunk_width);
	   	scr_y = pr_y + (k / chunk_width);
		bank  = scr_x % NBANK;

                if ((*wptr ^ *rptr) & mask) {

		    /* don't print bank number if in byte or stencil mode since
		       the write are packed into 4 bytes */
		    if (mask != 0xffffffff)
		    	(void)sprintf(errtxt, DLXERR_PIXEL_MODE_AT, scr_x, scr_y,
                                      bank,*wptr&mask, *rptr&mask, (*wptr^*rptr)&mask);
		    else
		    	(void)sprintf(errtxt, DLXERR_BYTE_MODE_AT, scr_x, scr_y,
                                      *wptr, *rptr, *wptr ^ *rptr);

		    err++;
		}

		/* verify complement */
		if (((*wptr ^ *cptr) & mask) != mask) {

		    /* don't print bank number if in byte or stencil mode since
		       the write are packed into 4 bytes */
		    if (mask != 0xffffffff)
		    	(void)sprintf(errtxt2, DLXERR_PIXEL_MODE_AT, scr_x, scr_y,
                                      bank,~(*wptr)&mask, *cptr&mask, (~(*wptr) ^ *cptr)&mask);
		    else
		    	(void)sprintf(errtxt2, DLXERR_BYTE_MODE_AT, scr_x, scr_y,
                                      ~(*wptr)&mask, *cptr&mask, (~(*wptr) ^ *cptr)&mask);
		    err++;
		    (void)strcat(errtxt, errtxt2);
		}

		if (err) goto error_return;

		cptr++;
		wptr++;
		rptr++;
	    }

	    if (rest) {

		char *wpc = (char *)wptr;
		char *rpc = (char *)rptr;
		char *cpc = (char *)cptr;

		for (k = rest ; k > 0 ; k--) {

	   	    scr_x = pr_x + (k % chunk_width);
	   	    scr_y = pr_y + (k / chunk_width);
		    bank  = scr_x % NBANK;

		    if (*wpc ^ *rpc) {

		    	/* don't print bank number if in byte or stencil mode
		           the writes are packed into 4 bytes */
		    	if (mask != 0xffffffff)
		    		(void)sprintf(errtxt, DLXERR_PIXEL_MODE_AT, scr_x, scr_y,
                                      bank,*wpc, *rpc, *wpc ^ *rpc);
		    	else
		    		(void)sprintf(errtxt, DLXERR_BYTE_MODE_AT, scr_x, scr_y,
                                      *wpc, *rpc, *wpc ^ *rpc);
		    	err++;
		    }

		    /* testing complement */
		    if ((*wpc ^ *cpc) != 0xffff) {
		    	/* don't print bank number if in byte or stencil mode
		           the writes are packed into 4 bytes */
		    	if (mask != 0xffffffff)
		    		(void)sprintf(errtxt, DLXERR_PIXEL_MODE_AT, scr_x, scr_y,
                                      bank, ~(*wpc), *cpc, ~(*wpc) ^ *cpc);
		    	else
		    		(void)sprintf(errtxt, DLXERR_BYTE_MODE_AT, scr_x, scr_y,
                                      ~(*wpc), *cpc, ~(*wpc) ^ *cpc);
		    	err++;
		    }

		    if (err) goto error_return;

		    wpc++;
		    rpc++;
		    cpc++;
		}
	    }
	}
    }

    (void)pr_close(mprs);
    (void)pr_close(mprd);
    (void)pr_close(mpr_save);

    TRACE_OUT
    return((char *)0);

    /* return point for errors */
    error_return:
	if (mprs) {
	    (void)pr_close(mprs);
	}
	if (mprd) {
	    (void)pr_close(mprd);
	}
	if (mprc) {
	    (void)pr_close(mprc);
	}
	if (mpr_save) {
	    (void)pr_close(mpr_save);
	}

	TRACE_OUT
	return(errtxt);

}

#define GREY_LEVEL 0x70707070
/**********************************************************************/
char *pr_visual_test(pr)
/**********************************************************************/
Pixrect *pr;

{
    register unsigned int pat, *cp;
/*
    int current_plane;
    int current_attr;
*/
    Pixrect *line;
    int i, j, k;
    int op;

    func_name = "pr_visual_test";
    TRACE_IN

    /* save current settings */
/*
    current_plane = pr_get_plane_group(pr);
    (void)pr_getattributes(pr, &current_attr);
*/

/*
    (void)pr_clear_all(pr);
*/

    /* make it visible */
/*
    (void)pr_set_planes(pr, PIXPG_WID, PIX_ALL_PLANES);
    op = PIX_SRC | PIX_COLOR(wid_alloc(pr, FB_WID_DBL_8));
    (void)pr_clear(pr, op);
*/

    /* set 8-bit plane group and attributes */
    (void)pr_set_planes(pr, PIXPG_8BIT_COLOR, PIX_ALL_PLANES);
    (void)pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_A,
		    PR_DBL_WRITE, PR_DBL_A,
		    PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_A,
		    0);

    line = mem_create(pr->pr_size.x, 64, pr->pr_depth);
    if (line == NULL) {

	TRACE_OUT
	return(DLXERR_OPEN_MEM_PIXRECT);
    }

    /* create 64 lines of chess board pattern */
    cp = (unsigned int *)(mpr_d(line)->md_image);
    for ( k = 0 ; k < 64 ; k++) {
	/* create one line chess board pattern */
	pat = GREY_LEVEL;
	for (i = 0 ; i < pr->pr_size.x/64 ; i++ ) {
	    for (j = 0 ; j < (64*pr->pr_depth)/(sizeof(int)*8) ; j++) {
		*cp = pat;
		cp++;
	    }
	    pat = ~pat;
	}
    }

    for (i = 0 ; i < pr->pr_size.y/64 ; i++) {
	if (i % 2) {
	    (void)pr_rop(pr, 0, i*64, pr->pr_size.x, line->pr_size.y,
					PIX_NOT(PIX_SRC), line, 0, 0);
	} else {
	    (void)pr_rop(pr, 0, i*64, pr->pr_size.x, line->pr_size.y, PIX_SRC,
							    line, 0, 0);
	}
    }
				
    /* restore original settings */
/*
    pr_set_planes(pr, current_plane, current_attr);
*/

    (void)pr_close(line);

    TRACE_OUT
    return((char *)0);
}

#define XBGR(r,g,b) (((b)<<16) + ((g)<<8) + (r))
/**********************************************************************/
pr_rgb_stripes(pr)
/**********************************************************************/
Pixrect *pr;
/* generates three rows of red, green and blues bars on screen */

{
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
    int state;
    int level;
    int current_plane;
    int current_attr;
    int op;
    int cr;
    int temp;
    int color;
    int x;
    int y;
    int i;

    func_name = "pr_rgb_stripes";
    TRACE_IN


    /* save current settings */
    current_plane = pr_get_plane_group(pr);
    (void)pr_getattributes(pr, &current_attr);

    (void)pr_clear_all(pr);
    /* make it visible */
    (void)pr_set_planes(pr, PIXPG_WID, PIX_ALL_PLANES);
    op = PIX_SRC | PIX_COLOR(wid_alloc(pr, FB_WID_DBL_24));
    op = pr_clear(pr, op);

    /* set 24-bit plane group and attributes */
    (void)pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
    (void)pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_A,
		    PR_DBL_WRITE, PR_DBL_A,
		    PR_DBL_READ, PR_DBL_A,
		    PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_A,
		    0);

    /* load 1-1 color map */
    for (i=0 ; i <256 ; i++) {
	red[i] = green[i] = blue[i] = i;
    }
    (void)pr_putlut(pr, 0, 256, red, green, blue);

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
		if (color == 0xff) color = 0xfe;
		if (color == 0xff00) color = 0xfe00;
		if (color == 0xff0000) color = 0xfe0000;
                (void)pr_rop(pr, x, y, 36, 32, (PIX_SRC |
				PIX_COLOR(color)), (Pixrect *)0, 0, 0);
            }
        }
    }

    /* restore original settings */
    pr_set_planes(pr, current_plane, current_attr);

    TRACE_OUT
    return 0;
}

/**********************************************************************/
char *
pixrect_complete_test(pr)
/**********************************************************************/
Pixrect *pr;
/* Device-dependent frame buffer test. This test covers the
 * double buffering and all plane groups */

{
    extern char *pr_all_test();
    static char errtxt[256];
    char *errmsg;
    int current_state;
    int err=0;

    func_name = "pixrect_complete_test";
    TRACE_IN

    pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);

    if (pr_dbl_get(pr, PR_DBL_AVAIL) == PR_DBL_EXISTS) {
	/* save current display buffer */
	current_state = pr_dbl_get(pr, PR_DBL_DISPLAY);

	/* switch to both buffers*/
	pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_A,
			PR_DBL_WRITE, PR_DBL_BOTH,
			PR_DBL_READ, PR_DBL_A,
			PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_A,
			0);
	/* we run this test only if both buffers can be
	   written at the same time */
	if (pr_dbl_get(pr, PR_DBL_WRITE) == PR_DBL_BOTH) {
	    errmsg = pr_all_test(pr);
	    if (errmsg) {
		(void)strcpy(errtxt, DLXERR_SIMULT_WRITE_BOTH_READ_A);
		(void)strcat(errtxt, errmsg);
		error_report(errtxt);
		err++;
	    }

	    pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	    pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_B,
			PR_DBL_WRITE, PR_DBL_BOTH,
			PR_DBL_READ, PR_DBL_B,
			PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_B,
			0);
	    errmsg = pr_all_test(pr);
	    if (errmsg) {
		(void)strcpy(errtxt, DLXERR_SIMULT_WRITE_BOTH_READ_B);
		(void)strcat(errtxt, errmsg);
		error_report(errtxt);
		err++;
	    }
	}

	/* switch to buffer A */
	pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_B,
			PR_DBL_WRITE, PR_DBL_A,
			PR_DBL_READ, PR_DBL_A,
			PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_B,
			0);
	errmsg = pr_all_test(pr);
	if (errmsg) {
	    (void)strcpy(errtxt, DLXERR_BUFF_A);
	    (void)strcat(errtxt, errmsg);
	    error_report(errtxt);
	    err++;
	}

	/* switch to buffer B */
	pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_A,
			PR_DBL_WRITE, PR_DBL_B,
			PR_DBL_READ, PR_DBL_B,
			PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_A,
			0);
	errmsg = pr_all_test(pr);
	if (errmsg) {
	    (void)strcpy(errtxt, DLXERR_BUFF_B);
	    (void)strcat(errtxt, errmsg);
	    error_report(errtxt);
	    err++;
	}

	/* restore original buffer */
	pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	pr_dbl_set(pr, current_state, 0);

    } else {
	errmsg = pr_all_test(pr);
	if (errmsg) {
	    (void)strcpy(errtxt, DLXERR_PIXRECT);
	    (void)strcat(errtxt, errmsg);
	    error_report(errtxt);
	    err++;
	}
    }


    /* bring up chess board pattern */
/* Do not bring up checker board patter, this just confuses people */
/***
    errmsg = pr_visual_test(pr);
    if (errmsg) {
	(void)strcpy(errtxt, DLXERR_PIXRECT_VISUAL);
	(void)strcat(errtxt, errmsg);
	TRACE_OUT
	return errtxt;
    }
***/

    if (err) {
	sprintf(errtxt, DLXERR_VIDEO_MEMORY_FAILED, err);
	TRACE_OUT
	return(errtxt);
    } else {
	TRACE_OUT
	return (char *)0;
    }
}

/**********************************************************************/
get_available_plane_groups(pr)
/**********************************************************************/
Pixrect *pr;
{
    int i;


    func_name = "get_available_plane_groups";
    TRACE_IN

    for (i = 0 ; i < PIXPG_INVALID ; i++)
	groups[i] = 0;

    ngroups = pr_available_plane_groups(pr, PIXPG_INVALID, groups);

    TRACE_OUT

}

/**********************************************************************/
exclude_plane_group(plane)
/**********************************************************************/
int plane;
{

    func_name = "exclude_plane_group";
    TRACE_IN

    groups[plane] = FALSE;

    TRACE_OUT

}

/**********************************************************************/
include_plane_group(plane)
/**********************************************************************/
int plane;
{

    func_name = "include_plane_group";
    TRACE_IN

    groups[plane] = TRUE;

    TRACE_OUT

}

/**********************************************************************/
get_plane_group(plane)
/**********************************************************************/
int plane;
{

    func_name = "get_plane_group";
    TRACE_IN

    TRACE_OUT
    return(groups[plane]);


}


/**********************************************************************/
char *
pr_all_test(pr)
/**********************************************************************/
Pixrect *pr;
/* tests all pixrect planes */

{
    static char errtxt[512];
    int i;
    int mask;
    int current_plane;
    int current_attr;
    char *errmsg;
    int err = 0;
    
    func_name = "pr_all_test";
    TRACE_IN

    /* save current settings */
    current_plane = pr_get_plane_group(pr);
    (void)pr_getattributes(pr, &current_attr);

    for (i = 0 ; i <= ngroups ; i++) {
	if (get_plane_group(i)) {
	    pr_set_planes(pr, i, PIX_ALL_PLANES);

	    /* set the test mask accordingly,
	       not all bits are valid for every plane */
	    switch (i) {
		case PIXPG_24BIT_COLOR :
		    mask = 0xffffff; /* 24-bit color XRGB format */
		    break;
		case PIXPG_WID :
		    mask = 0x3ff;   /* 10-bit WID */
		    break;
		case PIXPG_ZBUF :
		    mask = 0x00ffffff;	 /* 24-bit ZBUFF */
		    break;
		default :
		    mask = 0xffffffff;   /* everthing else should be
					    8-bit or 1-bit deep */
	    }

	    errmsg = pr_test(pr, mask);
	    if (errmsg) {
    		/* restore original settings */
    		pr_set_planes(pr, current_plane, current_attr);
		(void)strcpy(errtxt, groupnames[i]);
		(void)strcat(errtxt, ", ");
		(void)strcat(errtxt, errmsg);
		(void)strcat(errtxt, ".\n");
		TRACE_OUT
		return (errtxt);
    	    } 
	}
    }

    /* restore original settings */
    pr_set_planes(pr, current_plane, current_attr);

    TRACE_OUT
    return (char *)0;
}

/**********************************************************************/
char *
lut_test(pr)
/**********************************************************************/
Pixrect *pr;

{
    

    static char errtxt[256];
    char *color_table_test();
    static int pg[] = {  PIXPG_8BIT_COLOR,
                         PIXPG_24BIT_COLOR,
			 PIXPG_INVALID,
                        };
    static char *pg_names[] = {
                            "8BIT COLOR",
                            "24BIT COLOR",
			    "",
                        };

    int current_plane;
    int current_attr;
    int current_clut_id;

    int j;
    int n;
    char *errmsg;

    func_name = "lut_test";
    TRACE_IN

    current_plane = pr_get_plane_group(pr);
    (void)pr_getattributes(pr, &current_attr);

 
    for (j = 0 ; pg[j] != PIXPG_INVALID ; j++) {
        /* set plane group and attributes */
        (void)pr_set_planes(pr, pg[j], PIX_ALL_PLANES);

	if (pg[j] == PIXPG_8BIT_COLOR) {
	    current_clut_id = get_clut_id(pr);
	    for (n = 0 ; n < 14 ; n++) {
		set_clut_id(pr, n);
		errmsg = color_table_test(pr, 0, PR_DONT_DEGAMMA);
		if (errmsg) {
		    sprintf(errtxt, DLXERR_CLUT_TEST, pg_names[j], n, errmsg);
		    return errtxt;
		}
	    }
	    set_clut_id(pr, current_clut_id);
	} else {
	    errmsg = color_table_test(pr, PR_FORCE_UPDATE, PR_DONT_DEGAMMA);
		if (errmsg) {
		    sprintf(errtxt, DLXERR_CLUT_TEST, pg_names[j], 0, errmsg);
		    return errtxt;
		}
	}
    }
 
    /* restore original settings */
    pr_set_planes(pr, current_plane, current_attr);

    (void)pr_rgb_stripes(pr);
    sleep(15);

    (void)pr_clear_all(pr);


    TRACE_OUT
    return (char *)0;

}

/**********************************************************************/
get_clut_id (pr)
/**********************************************************************/
Pixrect *pr;
{

    int clutid;

    func_name = "get_clut_id";
    TRACE_IN

    clutid = ((struct gt_data *)((pr)->pr_data))->clut_id;

    TRACE_OUT

    return clutid;
}

/**********************************************************************/
set_clut_id (pr, n)
/**********************************************************************/
Pixrect *pr;
int n;

{

    func_name = "set_clut_id";
    TRACE_IN

    ((struct gt_data *)((pr)->pr_data))->clut_id = n;

    TRACE_OUT

}

/**********************************************************************/
char *
color_table_test(pr, pr_force_update, pr_dont_degamma)
/**********************************************************************/
Pixrect *pr;
int pr_force_update;
int pr_dont_degamma;
{

    static char errtxt[256];
    unsigned char sred[256], sgreen[256], sblue[256];
    unsigned char *red, *green, *blue;
    unsigned int wlut[192];
    unsigned int rlut[192];
    int llength = 256;
    int i;
 

    func_name = "color_table_test";
    TRACE_IN

    /* initialize state of the random generator */
    ltime = time((time_t *)0);
    (void)initstate((unsigned int)ltime, (char *)rand_state, 128);
    (void)setstate((char *)rand_state);

    /* preset test pattern in memory */
    for (i=0 ; i < 192 ; i++) {
        wlut[i] = random();
    }


    /* save the original colormap */
    (void)pr_getlut(pr, 0, llength, sred, sgreen, sblue);
 
    /* write to lut memory */
    red = (unsigned char *)&(wlut[0]);
    green = red + 256;
    blue = green + 256;
    (void)gt_putlut_blocked(pr, 0|pr_force_update|pr_dont_degamma, llength, red, green, blue);

    /* read from lut memory */
    red = (unsigned char *)&(rlut[0]);
    green = red + 256;
    blue = green + 256;
    (void)pr_getlut(pr, 0|pr_dont_degamma, llength, red, green, blue);
 
    /* restore original colormap */
    (void)gt_putlut_blocked(pr, 0|pr_force_update, llength, sred, sgreen, sblue);
 
    for (i=0 ; i < (llength*3)/sizeof(int) ; i++) {
	if (rlut[i] != wlut[i]) {
	    (void)sprintf(errtxt, DLXERR_LOOKUP_TABLE, i*sizeof(int), wlut[i], rlut[i]);
	    TRACE_OUT
	    return errtxt;
	}
    }

    TRACE_OUT

    return (char *)0;
}

/**********************************************************************/
pr_clear(pr, op)
/**********************************************************************/
Pixrect *pr;
int op;
/* clear current pixrect plane */
{
    
    func_name = "pr_clear";
    TRACE_IN

    (void)pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y, op,
					(Pixrect *)NULL, NULL, NULL);
    TRACE_OUT
}
/**********************************************************************/
pr_clear_all(pr)
/**********************************************************************/
Pixrect *pr;
/* clear all available pixrect planes */
{
    char groups[PIXPG_INVALID+1];
    int i;
    int op;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];

    func_name = "pr_clear_all";
    TRACE_IN

    (void)get_available_plane_groups(pr);
    (void)exclude_plane_group(PIXPG_WID);

    pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
    (void)pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_B,
		    PR_DBL_WRITE, PR_DBL_B,
		    PR_DBL_READ, PR_DBL_B,
		    PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_B,
		    0);
    for (i = 0 ; i < ngroups ; i++) {
	if (get_plane_group(i)) {
	    pr_set_planes(pr, i, PIX_ALL_PLANES);
	    op = PIX_CLR;
	    (void)pr_clear(pr, op);
	}
    }
    pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
    (void)pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_A,
		    PR_DBL_WRITE, PR_DBL_A,
		    PR_DBL_READ, PR_DBL_A,
		    PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_A,
		    0);
    for (i = 0 ; i < ngroups ; i++) {
	if (get_plane_group(i)) {
	    pr_set_planes(pr, i, PIX_ALL_PLANES);
	    op = PIX_CLR;
	    (void)pr_clear(pr, op);
	}
    }

    TRACE_OUT
}
/**********************************************************************/
gt_putlut_blocked(pr, index, count, red, green, blue)
/**********************************************************************/
Pixrect		*pr;
int		index, count;
unsigned char	*red, *green, *blue;
{
    struct gt_data	*gtd;
    struct fb_clut	fbclut;
    struct fbcmap	fbmap;
    int			plane;
    int			res;
    

    func_name = "gt_putlut_blocked";
    TRACE_IN

    gtd = (struct gt_data *) gt_d(pr);
    /* Extract the plane if necessary */


    if (PIX_ATTRGROUP(index))
    {
        plane = PIX_ATTRGROUP(index) & PIX_GROUP_MASK;
        index &= PR_FORCE_UPDATE | PIX_ALL_PLANES;  
    }
    else
        plane = gtd->planes;
    
    if (PIX_ATTRGROUP(plane) == PIXPG_8BIT_COLOR)
         fbclut.index = gtd->clut_id;
    else if (PIX_ATTRGROUP(plane) == PIXPG_24BIT_COLOR){
        if (index & PR_FORCE_UPDATE)
            fbclut.index = GT_CLUT_INDEX_24BIT;
        else
            return(0);
    }
    else if ((PIX_ATTRGROUP(plane) == PIXPG_CURSOR)||
            (PIX_ATTRGROUP(plane) == PIXPG_CURSOR_ENABLE)){
        /* never post to cusor lut */
        return (0);
    }
    else
        return PIX_ERR;

   fbclut.flags = FB_BLOCK; 

#ifdef KERNEL
    fbclut.flags |= FB_KERNEL;        
#else
    /* If neither of these are set use the system default */
    if (index & PR_DONT_DEGAMMA) {
        fbclut.flags |= FB_NO_DEGAMMA;
    } else if (index & PR_DEGAMMA) {
        fbclut.flags |= FB_DEGAMMA;
    }
#endif  
    fbclut.count  = count;
    fbclut.offset  = index & ~(PR_FORCE_UPDATE|PR_DONT_DEGAMMA|PR_DEGAMMA);
    fbclut.red    = red;
    fbclut.green  = green;
    fbclut.blue   = blue;

    res = ioctl(gtd->fd, FB_CLUTPOST, (caddr_t) &fbclut, 0);

    TRACE_OUT
    return res;
}
