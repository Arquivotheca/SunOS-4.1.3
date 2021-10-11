#ifndef lint
static  char sccsid[] = "@(#)pr_test.c 1.20 90/10/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


/**********************************************************************/
/* Include files */
/**********************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>
#include <pixrect/pixrect_hs.h>
#include <sunwindow/cms_colorcube.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include "msg.h"     
#include "cg6test.h"     
/* Flag for test modes */
#define         VERIFY_FLAG                     1       /* Real test */
#define         WRITE_FLAG  

#define CHUNK_WIDTH	64
#define CHUNK_HEIGHT	64

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


/**********************************************************************/
char *pr_test(pr, mask)
/**********************************************************************/
Pixrect *pr;
register int mask;
/* Test the pixrect memory by writing a random pattern to it */

{
    static char errtxt[256];
    Pixrect	*mprs = (Pixrect *) NULL,
		*mprd = (Pixrect *) NULL,
		*mpr_save = (Pixrect *) NULL;

    register *iptr;
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


    func_name = "pr_test";
    TRACE_IN

    /* initialize state of the random generator */
    ltime = time((time_t *)0);
    (void)initstate((unsigned int)ltime, (char *)rand_state, 128);
    (void)setstate((char *)rand_state);

    /* create memory pixrect to save the original screen */
    mpr_save = mem_create(chunk_width, chunk_height, pr->pr_depth);
    if (mpr_save == NULL) {
	(void)strcpy(errtxt, errmsg_list[31]);
	goto error_return;
    }

    /* create memory pixrect to puit random patterns */
    mprs = mem_create(chunk_width, chunk_height, pr->pr_depth);
    if (mprs == NULL) {
	(void)strcpy(errtxt, errmsg_list[31]);
	goto error_return;
    }
    pps = (chunk_width * chunk_height * pr->pr_depth)/
						    (sizeof(int) << 3);
    iptr = (int *)mpr_d(mprs)->md_image;

    for (i = 0 ; i < pps - 1 ; i++) {
	*iptr++ = random();
    }
    if ((rest = pps % sizeof(int))) {
	char *cp = (char *)iptr;
	for (i = rest ; i > 0 ; i--) {
	    *cp++ = (char)random();
	}
    } else {
	*iptr = random() & mask;
    }

    mprd = mem_create(chunk_width, chunk_height, pr->pr_depth);
    if (mprd == NULL) {
	(void)strcpy(errtxt, errmsg_list[31]);
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
	if (i == ny-1) chunk_height = chunk_height_rest;
	for (j = 0 ; j < nx ; j++) {
	    int pr_x = j*chunk_width;
	    int pr_y = i*chunk_height;
	    if (j == nx-1) chunk_width = chunk_width_rest;

	    (void)check_input();

	    /* save the current screen */
	    (void)pr_rop (mpr_save, 0, 0, chunk_width, chunk_height,
				PIX_SRC | PIX_DONTCLIP, pr, pr_x, pr_y);

	    /* write pattern to pixrect device */
	    (void)pr_rop (pr, pr_x, pr_y, chunk_width, chunk_height,
				    PIX_SRC | PIX_DONTCLIP, mprs, 0, 0);

	    /* copy screen into memory pixrect */
	    (void)pr_rop (mprd, 0, 0, chunk_width, chunk_height,
				PIX_SRC | PIX_DONTCLIP, pr, pr_x, pr_y);

	    /* restore previous screen */
	    (void)pr_rop (pr, pr_x, pr_y, chunk_width, chunk_height,
				PIX_SRC | PIX_DONTCLIP, mpr_save, 0, 0);

	    /* Xor pixrect device with pattern */
	    (void)pr_rop (mprd, 0, 0, chunk_width, chunk_height,
			(PIX_SRC ^ PIX_DST) | PIX_DONTCLIP, mprs, 0, 0);

	    iptr = (int *)mpr_d(mprd)->md_image;
	    if ((rest= pps % sizeof(int))) pps--;
	    for (k = 0 ; k < pps ; k++) {
		if (*iptr & mask) {
		    (void)sprintf(errtxt, errmsg_list[32], (iptr-(int *)mpr_d(mprd)->md_image)*sizeof(int));
		    goto error_return;
		}
		iptr++;
	    }

	    if (rest) {
		char *cp = (char *)iptr;
		for (k = rest ; k > 0 ; k--) {
		    if (*cp) {
			(void)sprintf(errtxt, errmsg_list[32], cp-(char *)mpr_d(mprd)->md_image);
			goto error_return;
		    }
		    cp++;
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
	if (mpr_save) {
	    (void)pr_close(mpr_save);
	}

	TRACE_OUT
	return(errtxt);

}

/**********************************************************************/
char *pr_visual_test(pr)
/**********************************************************************/
Pixrect *pr;

{
    register unsigned int pat, *cp;
    int current_plane;
    int current_attr;
    Pixrect *line;
    int i, j, k;

    func_name = "pr_visual_test";
    TRACE_IN

    /* save current settings */
    current_plane = pr_get_plane_group(pr);
    (void)pr_getattributes(pr, &current_attr);

    /* set 24-bit plane group and attributes */
    (void)pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);

    line = mem_create(pr->pr_size.x, 64, pr->pr_depth);
    if (line == NULL) {

	TRACE_OUT
	return(errmsg_list[31]);
    }

    /*
    (void)pr_clear_all(pr);
    */

    /* create 64 lines of chess board pattern */
    cp = (unsigned int *)(mpr_d(line)->md_image);
    for ( k = 0 ; k < 64 ; k++) {
	/* create one line chess board pattern */
	pat = 0;
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
    pr_set_planes(pr, current_plane, current_attr);

    (void)pr_close(line);

    sleep(10);

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
   int state;
   int level;
   int cr;
   int temp;
   int color;
   int x;
   int y;

    func_name = "pr_rgb_stripes";
    TRACE_IN

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
                (void)pr_rop(pr, x, y, 36, 32, (PIX_SRC |
				PIX_COLOR(color)), (Pixrect *)0, 0, 0);
            }
        }
    }

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

    func_name = "pixrect_complete_test";
    TRACE_IN

    /* Determine if frame buffer has double buffering  capability */
    if (pr_dbl_get(pr, PR_DBL_AVAIL) == PR_DBL_EXISTS) {
	/* allocate WID for duble buffer */
	/*wid.wa_type = FB_WID_DBL_24;
	wid.wa_count = 1;
	if (pr_ioctl(pr, FBIO_WID_ALLOC, &wid) < 0) {
	    return errmsg_list[50];
	}*/
	/* save current display buffer */
	current_state = pr_dbl_get(pr, PR_DBL_DISPLAY);
	/* switch to buffer A */
	pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_A,
			PR_DBL_WRITE, PR_DBL_A,
			PR_DBL_READ, PR_DBL_A,
			PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_A,
			0);
	errmsg = pr_all_test(pr, PIXPG_8BIT_COLOR);
	if (errmsg) {
	    (void)strcpy(errtxt, errmsg_list[33]);
	    (void)strcat(errtxt, errmsg);
	    TRACE_OUT
	    return errtxt;
	}
	/* switch to buffer B */
	pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_B,
			PR_DBL_WRITE, PR_DBL_B,
			PR_DBL_READ, PR_DBL_B,
			PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_B,
			0);
	errmsg = pr_all_test(pr, PIXPG_8BIT_COLOR);
	if (errmsg) {
	    (void)strcpy(errtxt, errmsg_list[34]);
	    (void)strcat(errtxt, errmsg);
	    TRACE_OUT
	    return errtxt;
	}

	/* switch to both buffers*/
	pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_A,
			PR_DBL_WRITE, PR_DBL_BOTH,
			PR_DBL_READ, PR_DBL_BOTH,
			PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_A,
			0);
	/* we run this test only if both buffers can be
	   written at the same time */
	if (pr_dbl_get(pr, PR_DBL_WRITE) == PR_DBL_BOTH) {
	    errmsg = pr_all_test(pr, PIXPG_INVALID);
	    if (errmsg) {
		(void)strcpy(errtxt, errmsg_list[35]);
		(void)strcat(errtxt, errmsg);
		TRACE_OUT
		return errtxt;
	    }
	    pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	    pr_dbl_set(pr, PR_DBL_DISPLAY, PR_DBL_B,
			PR_DBL_WRITE, PR_DBL_BOTH,
			PR_DBL_READ, PR_DBL_BOTH,
			PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_B,
			0);
	    errmsg = pr_all_test(pr, PIXPG_INVALID);
	    if (errmsg) {
		(void)strcpy(errtxt, errmsg_list[36]);
		(void)strcat(errtxt, errmsg);
		TRACE_OUT
		return errtxt;
	    }
	}

	/* restore original buffer */
	pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
	pr_dbl_set(pr, current_state, 0);

    } else {
	errmsg = pr_all_test(pr, PIXPG_INVALID);
	if (errmsg) {
	    (void)strcpy(errtxt, errmsg_list[37]);
	    (void)strcat(errtxt, errmsg);
	    TRACE_OUT
	    return errtxt;
	}
    }


    /* bring up chess board pattern */
/* Do not bring up checker board patter, this just confuses people */
/***
    pr_set_planes(pr, PIXPG_WID, 0x3f);
    (void)pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y, PIX_SRC |
				PIX_COLOR(wid.wa_index), (Pixrect *)0, 0, 0);
    errmsg = pr_visual_test(pr);
    if (errmsg) {
	(void)strcpy(errtxt, errmsg_list[38]);
	(void)strcat(errtxt, errmsg);
	TRACE_OUT
	return errtxt;
    }
***/

    TRACE_OUT
    return (char *)0;
}

/**********************************************************************/
char *
pr_all_test(pr, not_gr)
/**********************************************************************/
Pixrect *pr;
int not_gr;
/* tests all pixrect planes, except not_gr */

{
    static char errtxt[256];
    static char *groupnames[] = { "Current plane",
				 "Mono plane",
				 "8-bit color plane",
				 "Overlay enable plane",
				 "Overlay plane",
				 "24-bit color plane",
				 "Video plane",
				 "Video enable plane",
				 "Transparent Overlay plane",
				 "Window ID plane",
				 "Z-Buffer",
				 "Last-plus-one plane", };
    char groups[PIXPG_INVALID+1];
    int ngroups;
    int i;
    int mask;
    int current_plane;
    int current_attr;
    char *errmsg;
    
    func_name = "pr_all_test";
    TRACE_IN

    for (i = 0 ; i < PIXPG_INVALID ; i++)
	groups[i] = 0;

    /* save current settings */
    current_plane = pr_get_plane_group(pr);
    (void)pr_getattributes(pr, &current_attr);

    ngroups = pr_available_plane_groups(pr, PIXPG_INVALID, groups);

    /* Tim Curry said that ZBUFF is available "unofficially" for Diag,
       therefore:
     */
    if (pr_dbl_get(pr, PR_DBL_AVAIL) == PR_DBL_EXISTS &&
	pr_dbl_get(pr, PR_DBL_WRITE) == PR_DBL_BOTH &&
	not_gr != PIXPG_24BIT_COLOR) {
	    /*groups[PIXPG_ZBUF] = TRUE;*/
    }

    for (i = 0 ; i <= ngroups ; i++) {
	if (i != not_gr && groups[i] != 0 && (not_gr != PIXPG_24BIT_COLOR || i != PIXPG_8BIT_COLOR)) {
	    pr_set_planes(pr, i, PIX_ALL_PLANES);

	    /* set the test mask accordingly,
	       not all bits are valid for every plane */
	    switch (i) {
		case PIXPG_24BIT_COLOR :
		    if (pr_dbl_get(pr, PR_DBL_AVAIL) &&
			pr_dbl_get(pr, PR_DBL_WRITE) != PR_DBL_BOTH) {
			mask = 0xf0f0f0; /* 12-bit double buffered */
		    } else {
			mask = 0xffffff; /* 24-bit color XRGB format */
		    }
		    break;
		/*case PIXPG_WID :
		    mask = 0x3f3f3f3f;   
		    break;
		case PIXPG_ZBUF :
		    mask = 0xffffffff;	 
		    break; */
		default :
		    mask = 0xffffffff;   /* everthing else should be
					    8-bit or 1-bit deep */
	    }

	    errmsg = pr_test(pr, mask);
	    if (errmsg) {
		(void)strcpy(errtxt, groupnames[(i < 11) ? i : 0]);
		(void)strcat(errtxt, ", ");
		(void)strcat(errtxt, errmsg);
		TRACE_OUT
		return errtxt;
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
    static int pg[7] = {  PIXPG_8BIT_COLOR,
			  PIXPG_OVERLAY,
			  PIXPG_24BIT_COLOR,
			  /*PIXPG_A12BIT_COLOR,
			  PIXPG_B12BIT_COLOR,
			  PIXPG_ALT_COLOR,
			  PIXPG_4BIT_OVERLAY, */
			};
    static char *pg_names[7] = {
			    "8BIT COLOR",
			    "2-BIT OVERLAY",
			    "24BIT COLOR",
			    "BUFFER A COLOR",
			    "BUFFER B COLOR",
			    "ALT COLOR",
			    "4-BIT OVERLAY",
			};

    int current_plane;
    int current_attr;

    unsigned char sred[256], sgreen[256], sblue[256];
    unsigned char *red, *green, *blue;
    unsigned int wlut[192];
    unsigned int rlut[192];
    int i;
    int j;
    int llength;

    func_name = "lut_test";
    TRACE_IN

    /* initialize state of the random generator */
    ltime = time((time_t *)0);
    (void)initstate((unsigned int)ltime, (char *)rand_state, 128);
    (void)setstate((char *)rand_state);

    /* preset test pattern in memory */
    for (i=0 ; i < 192 ; i++) {
	wlut[i] = random();
    }

/*
    (void)pr_clear_all(pr);
*/

    current_plane = pr_get_plane_group(pr);
    (void)pr_getattributes(pr, &current_attr);

    for (j = 0 ; j < 7 ; j++) {
	/* set plane group and attributes */
	(void)pr_set_planes(pr, pg[j], PIX_ALL_PLANES);

	if (pg[j] == PIXPG_OVERLAY) {
	    llength = 4;
	} /*else  if (pg[j] == PIXPG_4BIT_OVERLAY) {
	    llength = 28; } */
	else {
	    llength = 256;
	}

	/* save the original colormap */
	(void)pr_getlut(pr, 0, llength, sred, sgreen, sblue);

	/* write to lut memory */
	red = (unsigned char *)&(wlut[0]);
	green = red + 256;
	blue = green + 256;
	(void)pr_putlut(pr, 0, llength, red, green, blue);

	/* read from lut memory */
	red = (unsigned char *)&(rlut[0]);
	green = red + 256;
	blue = green + 256;
	(void)pr_getlut(pr, 0, llength, red, green, blue);

	/* restore original colormap */
	(void)pr_putlut(pr, 0, llength, sred, sgreen, sblue);

	for (i=0 ; i < (llength*3)/sizeof(int) ; i++) {
	    if (rlut[i] != wlut[i]) {
		(void)sprintf(errtxt, errmsg_list[39], pg_names[j], i*sizeof(int), wlut[i], rlut[i]);
		TRACE_OUT
		return errtxt;
	    }
	}
    }

    /* restore original settings */
    pr_set_planes(pr, current_plane, current_attr);

    TRACE_OUT
    return (char *)0;
}

#ifdef NOT_USED
/**********************************************************************/
char *
pr_verify(pr, file, vflag)
/**********************************************************************/
Pixrect *pr;
char *file;
int  vflag;

{
    static char errtxt[256];

    Pixrect *mpr;
    FILE *finput, *foutput;
    int pixcount;
    colormap_t cmap;
    int errors;
    int *iptr;
    int i;

    func_name = "pr_verify";
    TRACE_IN

    /* set plane group and attributes */
    (void)pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);

    if (vflag == VERIFY_FLAG) {
	/* open test image file */
        finput = fopen(file, "r");
        if (finput == NULL) {
            (void)sprintf(errtxt, "Can't open test image file %s.\n", file);
	    TRACE_OUT
            return(errtxt);
        }

	pixcount = (pr->pr_size.x*pr->pr_size.y*pr->pr_depth) >> 3;

	mpr = pr_load(finput, &cmap);
	if (mpr == NULL) {
	    (void)fclose(finput);
	    (void)sprintf(errtxt, "Can't read test image file %s.\n", file);
	    TRACE_OUT
	    return(errtxt);
	}

	(void)fclose(finput);

	/* Xor pixrect device with file */
	(void)pr_rop (mpr, 0, 0, pr->pr_size.x, pr->pr_size.y,
			(PIX_SRC ^ PIX_DST) | PIX_DONTCLIP, pr, 0, 0);

	iptr = (int *)mpr_d(mpr)->md_image;
	errors = 0;
	for (i = 0 ; i < pixcount; i++) {
	    if (*iptr) {
		errors++;
	    }
	    iptr++;
	}
	(void)pr_close(mpr);

	if (errors) {
	    (void)sprintf(errtxt, "Verify error: total %d pixels differ from their expected values.\n", errors);
	    TRACE_OUT
	    return errtxt;
	} else {
	    TRACE_OUT
	    return (char *)0;
	}
    } else if (vflag == WRITE_FLAG) {
	/* open test image file */
        foutput = fopen(file, "w");
        if (foutput == NULL) {
            (void)sprintf(errtxt, "Can't open test image file %s.\n", file);
	    TRACE_OUT
            return(errtxt);
        }

	errors = pr_dump(pr, foutput, (colormap_t *)NULL, RT_STANDARD, 0);
	if (errors) {
	    (void)sprintf(errtxt, "Error creating test image file %s.\n", file);
	    TRACE_OUT
	    return errtxt;
	} else {
	    TRACE_OUT
	    return (char *)0;
	}
    }

    TRACE_OUT
    return (char *)0;
}
#endif NOT_USED
	    


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
    int ngroups;
    int i;
    int op;
    int addr;
    int data;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];

    func_name = "pr_clear_all";
    TRACE_IN

    for (i = 0 ; i < PIXPG_INVALID ; i++) {
	groups[i] = 0;
    }
/* RAJ
    ngroups = pr_available_plane_groups(pr, PIXPG_INVALID, groups);
    groups[PIXPG_WID] = FALSE; */
    for (i = 0 ; i < ngroups ; i++) {
	if (groups[i]) {
	    pr_set_planes(pr, i, PIX_ALL_PLANES);
	    op = PIX_CLR;
	    (void)pr_clear(pr, op);
	}
    }

    addr = 1;
    data = 8;
    op = PIX_SRC | PIX_COLOR(1);
/* RAJ
    pr_set_planes(pr, PIXPG_WID, PIX_ALL_PLANES);
 */
    (void)pr_rop(pr, 0, 0, pr->pr_size.x, pr->pr_size.y, op,
					(Pixrect *)NULL, NULL, NULL);
/* RAJ
    cg12_d(pr)->ctl_sp->wsc.addr = addr;
    cg12_d(pr)->ctl_sp->wsc.data = data;
*/
    /* load 1-1 color map */
    for (i=0 ; i <256 ; i++) {
	red[i] = green[i] = blue[i] = i;
    }
    pr_putcolormap(pr, 0, 256, red, green, blue);

    TRACE_OUT
}
