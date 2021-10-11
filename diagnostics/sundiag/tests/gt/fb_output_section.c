
#ifndef lint
static  char sccsid[] = "@(#)fb_output_section.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gttest.h>
#include <errmsg.h>
#include <pixrect/pixrect_hs.h>
#include <pixrect/gtvar.h>

#define MAX_REGIONS	8
#define R_WIDTH		1280/MAX_REGIONS
#define R_HEIGHT	100

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

/**********************************************************************/
char *
fb_output_section()
/**********************************************************************/

{

    Pixrect *open_pr_device();

    static int pg[MAX_REGIONS] = {  PIXPG_24BIT_COLOR,
				    PIXPG_24BIT_COLOR,
				    PIXPG_24BIT_COLOR,
				    PIXPG_24BIT_COLOR,
				    PIXPG_8BIT_COLOR,
				    PIXPG_8BIT_COLOR,
				    PIXPG_8BIT_COLOR,
				    PIXPG_8BIT_COLOR,
				  };

    char *errmsg = (char *) 0;
    Pixrect *pr = open_pr_device();	/* Whole screen pixrect */
    Pixrect *prr[MAX_REGIONS];		/* Subregion for test */
    Pixrect *mp = (Pixrect *)NULL;
    int wid[MAX_REGIONS];
    int clut[MAX_REGIONS];
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
    unsigned char *bptr;
    unsigned int *iptr;
    int i;
    int j;
    int op;
    func_name = "fb_output_section";
    TRACE_IN

    /* initialize random state */
    (void)initstate(0x1205, (char *)rand_state, 128);
    (void)setstate((char *)rand_state);

    /* preset cluts with random number */
    for (i = 0 ;  i < 256 ; i++) {
	red[i] = (unsigned char) random();
	green[i] = (unsigned char) random();
	blue[i] = (unsigned char) random();
    }

    /* set the 24 bit clut randomly */
    (void)pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
    (void)gt_putlut_blocked(pr,
	    0|PR_FORCE_UPDATE|PR_DONT_DEGAMMA, 256, red, green, blue);

    /* preset array to 0 pointers */
    for (i = 0 ; i < MAX_REGIONS ; i++) {
	prr[i] = (Pixrect *)NULL;
    }

    for (i = 0 ; i < MAX_REGIONS ; i++) {
	prr[i] = pr_region(pr, i*R_WIDTH, 0, R_WIDTH, R_HEIGHT);
	if (prr[i] == (Pixrect *)NULL) {
	    errmsg = DLXERR_OPEN_REGIONS;
	    goto returning;
	}

	/* allocate wid and clut for test region */
	if (i < MAX_REGIONS/2) {
	    wid[i] = wid_alloc(prr[i], FB_WID_DBL_24);
	    clut[i] = 0;
	} else {
	    wid[i] = wid_alloc(prr[i], FB_WID_DBL_8);
	    clut[i] = clut_alloc(prr[i]);
	    (void)pr_set_planes(prr[i], PIXPG_8BIT_COLOR, PIX_ALL_PLANES);
	    /* preset cluts with random number */
	    for (j = 0 ;  j < 256 ; j++) {
		red[j] = (unsigned char) random();
		green[j] = (unsigned char) random();
		blue[j] = (unsigned char) random();
	    }
	    (void)gt_putlut_blocked(prr[i], 0| PR_DONT_DEGAMMA,
					    256, red, green, blue);
	}

	/* set plane groups */
	(void)pr_set_planes(prr[i], pg[i], PIX_ALL_PLANES);
	/* set buffer  (should be A, B, A, B, A, B, A, B on screen) */
	if (i % 2) {
	    (void)pr_dbl_set(prr[i], PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_B,
			    PR_DBL_WRITE, PR_DBL_B,
			    0);
	} else {
	    (void)pr_dbl_set(prr[i], PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_A,
			    PR_DBL_WRITE, PR_DBL_A,
			    0);
	}

	/* allocate mempixrect */
	mp = mem_create(R_WIDTH, R_HEIGHT, prr[i]->pr_depth);
	if (mp == (Pixrect *)NULL) {
	    errmsg = DLXERR_OPEN_MEM_PIXRECT;
	    goto returning;
	}

	/* preset mempixrect */
	if (prr[i]->pr_depth == 8) {
	    bptr = (unsigned char *)mpr_d(mp)->md_image;
	    for (j = 0 ; j < R_WIDTH * R_HEIGHT ; j++) {
		*bptr++ = (unsigned char)random();
	    }
	} else {
	    iptr = (unsigned int *)mpr_d(mp)->md_image;
	    for (j = 0 ; j < R_WIDTH * R_HEIGHT ; j++) {
		*iptr++ = (unsigned int)random();
	    }
	}
	/* pr_rop to FB */
	(void)pr_rop (prr[i], 0, 0, R_WIDTH, R_HEIGHT,
			    PIX_SRC | PIX_DONTCLIP, mp, 0, 0);

	/* make buffer visible */
	(void)pr_set_planes(prr[i], PIXPG_WID, PIX_ALL_PLANES);
	op = PIX_SRC | PIX_COLOR(wid[i]);
	(void)pr_rop (prr[i], 0, 0, R_WIDTH, R_HEIGHT, op,
						(Pixrect *)NULL, 0, 0);
	
	(void)pr_close(mp);
    }

    xtract_hdl(FB_OUTPUT_SECTION);

    errmsg = exec_dl(getfname(FB_OUTPUT_SECTION, HDL_CHK));

returning:

    /* close all opened subregions */
    for (i = 0 ; i < MAX_REGIONS ; i++) {
	if (prr[i]) {
	    (void)pr_clear(prr[i], PIX_SRC);
	    /* free  wid and clut index */
	    if (i < MAX_REGIONS/2) {
		(void)wid_free(prr[i], wid[i], FB_WID_DBL_24);
	    } else {
		(void)wid_free(prr[i], wid[i], FB_WID_DBL_8);
		(void)clut_free(prr[i], clut[i]);
	    }

	    /* close pixrect region */
	    (void)pr_close(prr[i]);
	}
    }

    /* restore 24 bit clut */
    for (i = 0 ;  i < 256 ; i++) {
	red[i] = green[i] = blue[i] = (unsigned char)i;
    }
    (void)pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
    (void)gt_putlut_blocked(pr,
	    0|PR_FORCE_UPDATE|PR_DONT_DEGAMMA, 256, red, green, blue);

    /* close the pixrect  */
    (void)close_pr_device();
    TRACE_OUT
    return errmsg;

}

