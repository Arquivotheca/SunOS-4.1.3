#ifndef lint
static  char sccsid[] = "@(#)gpsi_uti.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <sys/types.h>
#include <suntool/sunview.h>
#include <pixrect/gp1cmds.h>
#include <pixrect/pixrect_hs.h>
#include <pixrect/gp1reg.h>
#include <varargs.h>
#include <esd.h>

#define GP3_SET_FB_PLANES_RGB	(100 << 8)
#define MAX_ALLOC_TRIALS 10

extern char *sprintf();

extern
char		*device_name;

/* GP handles */
static caddr_t GP_shmem;
static int GP_fd;
static int GP_offs;
static int GP_statblk = -1;
static int GP_mindev;
static int GP_blocks;
static Rect GP_r;
static Pixrect *GP_pr = (Pixrect *)0;

static unsigned int bitvec;

/* aux. */
static char errtxt[256];

/* Context attributes */
static int		dont_clear_ctx;
static unsigned char	fb_num;
static unsigned int	fb_planes;
static short		rop;
static short		ncliprect;
static Rect		*cliprects;
static unsigned char	clip_planes;
static short		line_width;
static short		line_width_opt;
static short		*pr_tex_ptr;
static short		pr_tex_offset;
static short		pr_tex_options;
static float		xscale;
static float		xoffs;
static float		yscale;
static float		yoffs;
static float		zscale;
static float		zoffs;
static unsigned char	use_mat_num;
static unsigned char	set_mat_num;
static Matrix_3D        *matrix_3d;
static short		zbuf_depth;
static short		zbuf_x;
static short		zbuf_y;
static short		zbuf_width;
static short		zbuf_height;
static unsigned char	depth_cue;
static unsigned int	depth_cue_front_color;
static unsigned int	depth_cue_back_color;
static unsigned char	hidden_surf;
static unsigned int	rgb_color;
static float		rgb_color_red;
static float		rgb_color_green;
static float		rgb_color_blue;

#ifdef RFS
static
short *pdump;
#endif



/**********************************************************************/
open_gp()
/**********************************************************************/
/* opens pixrect device and initialise GPSI interface */
{

    char *init_gp();
    Pixrect *pr;
    char *errmsg;

    func_name = "open_gp";
    TRACE_IN

    if (GP_pr) {
	TRACE_OUT
	return 1;
    }
    
    pr = pr_open(device_name);
    if (pr == NULL) {
	(void)sprintf(errtxt, errmsg_list[19], device_name);
	error_report(errtxt);
	TRACE_OUT
	return 0;
    }

    GP_pr = pr;
    errmsg = init_gp();
    if (errmsg) {
	error_report(errmsg);
	pr_close(pr);
	GP_pr = (Pixrect *)0;
	TRACE_OUT
	return 0;
    }

    TRACE_OUT
    return 1;
}

/**********************************************************************/
close_gp()
/**********************************************************************/
/* close cg12 on exit */
{

    int err;
    func_name = "close_gp";
    TRACE_IN

    if (GP_statblk != -1) {
	err = gp1_free_static_block(GP_fd, GP_statblk);
    }

    if (GP_pr) {
	pr_close(GP_pr);
    }

    TRACE_OUT
}

/**********************************************************************/
short *allocbuf()
/**********************************************************************/
/* allocates a shmem block to place GPSI commands */

{
    short  *p;
    int     n = 0;

    func_name = "allocbuf";
    TRACE_IN

    (void)check_key();

    while ((GP_offs = gp1_alloc(GP_shmem, GP_blocks, &bitvec, GP_mindev,
							GP_fd)) == 0) {
        n++;
	if (n > MAX_ALLOC_TRIALS) {
	    p = (short *)-1;
/*	    if (n == 0) */
	       return(p); 
	}
    }

    p = &((short *)GP_shmem)[GP_offs];

#ifdef RFS
    pdump = p;
#endif

    *p++ = GP1_USE_CONTEXT | GP_statblk;

    TRACE_OUT
    return(p);
}

/**********************************************************************/
postbuf(p)
/**********************************************************************/
short        *p;
/*
   posts the command buffer pointed to by p, returns -1 in case
   the GPSI code can not be posted and frees the buffer.
*/

{
    int res;

    func_name = "postbuf";
    TRACE_IN

    (void)check_key();

    *p++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(p, bitvec);

    res = gp1_post(GP_shmem, GP_offs, GP_fd);
    if (res == -1) {
	TRACE_OUT
	return (res);
    }
    
    gp1_sync(GP_shmem, GP_fd);

    TRACE_OUT
    return 0;
    
}

/**********************************************************************/
postbuf_n_wait(p, result)
/**********************************************************************/
short        *p;
short        *result;
/*
   posts the command buffer pointed to by p, returns -1 in case
   the GPSI code can not be posted and frees the buffer.
*/

{
    int res;

    func_name = "postbuf_n_wait";
    TRACE_IN

    (void)check_key();

    *p++ = GP1_EOCL;
    GP1_PUT_I(p, bitvec);

    res = gp1_post(GP_shmem, GP_offs, GP_fd);
    if (res == -1) {
	TRACE_OUT
	return (res);
    }

    res = gp1_wait0(result, GP_fd);
    if (res == -1) {
	TRACE_OUT
	return (res);
    }
    
    TRACE_OUT
    return 0;
    
}

/**********************************************************************/
char *
init_gp()
/**********************************************************************/
/* initialises the global variables for subsequent GPSI calls. */

{
    register Pixrect *pr;
    int i;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];


    func_name = "init_gp";
    TRACE_IN

    pr = GP_pr;

    if (pr->pr_ops->pro_rop != gp1_rop) {
	TRACE_OUT
	return(errmsg_list[23]);
    }

    /* set plane group and attributes */
    pr_set_planes(pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);

    /* load color map */
    for (i=0 ; i <256 ; i++) {
	red[i] = green[i] = blue[i] = i;
    }
    pr_putlut(pr, 0, 256, red, green, blue);

    /* set global variables */
    GP_pr = pr;

    GP_r.r_width = pr->pr_width;
    GP_r.r_height = pr->pr_height;
    GP_r.r_left = gp1_d(pr)->cgpr_offset.x;
    GP_r.r_top = gp1_d(pr)->cgpr_offset.y;

    GP_shmem = gp1_d(pr)->gp_shmem;

    GP_fd = gp1_d(pr)->ioctl_fd;

    GP_mindev = gp1_d(pr)->minordev;

    GP_statblk = gp1_get_static_block(GP_fd);
    if (GP_statblk < 0) {
	TRACE_OUT
	return(errmsg_list[24]);
    }

    GP_blocks = 4;

    /* print the number of cg12 free blocks */

    TRACE_OUT
    return (char *)0;
}

/**********************************************************************/
post_diag_test(cmd, mode, arg0, arg1, arg2, arg3, arg4, arg5, arg6,
								arg7)
/**********************************************************************/
short cmd, mode, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7;
/* posts test and wait for return code, if test fails to start
 * return -1,, otherwise return the result of the test passed
 * through the shared memory.
 */

{
    
    short *sptr;
    short *ssptr;
    short *compl;
    short *result;
    int res;
    int mone = -1;

    func_name = "post_diag_test";
    TRACE_IN

    (void)check_key();

    sptr = allocbuf();
    if (sptr == (short *)-1) {
	error_report(errmsg_list[52]);
	TRACE_OUT
	return  -1;
    }
    ssptr = sptr;

    *sptr++ = cmd;
    *sptr++ = mode;
    *sptr++ = arg0;
    *sptr++ = arg1;
    *sptr++ = arg2;
    *sptr++ = arg3;
    *sptr++ = arg4;
    *sptr++ = arg5;
    *sptr++ = arg6;
    *sptr++ = arg7;
    compl   = sptr;
    *sptr++ = -1;
    result  = sptr;
    GP1_PUT_I(sptr, mone);
    *sptr++ = GP1_EOCL;

#ifdef RFS
    dump_p ((unsigned short *) pdump, (unsigned short *) sptr);
#endif

    if(gp1_post(GP_shmem, GP_offs, GP_fd) == -1) {
	error_report(errmsg_list[20]);
	TRACE_OUT
	return  -1;
    }

    if(gp1_wait0(compl, GP_fd) == -1) {
	error_report(errmsg_list[21]);
	TRACE_OUT
	return  -1;
    }

    GP1_GET_I(result, res);

    sptr = ssptr;
    *sptr++ = GP1_EOCL | GP1_FREEBLKS;
    GP1_PUT_I(sptr, bitvec);

    if(gp1_post(GP_shmem, GP_offs, GP_fd) == -1) {
	error_report("Can not start C30 test code.\n");
	TRACE_OUT
	return  -1;
    }

    TRACE_OUT
    return res;
}
/**********************************************************************/
char *
ctx_set(va_alist)
/**********************************************************************/
va_dcl

{
    char *ctx_post();
    register Context_attribute attr;
    va_list ap;

    func_name = "ctx_set";
    TRACE_IN

    /* set all default values in the static block */
    (void)ctx_default();

    va_start(ap);

    while (attr = va_arg(ap, Context_attribute)) {
	switch((Context_attribute)attr) {

	    case CTX_ATTR_END:
		break;

	    case CTX_DONT_CLEAR:
		dont_clear_ctx = 1;
		break;

	    case CTX_SET_FB_NUM:
		fb_num = (unsigned char)va_arg(ap, int);
		break;

	    case CTX_SET_FB_PLANES:
		fb_planes = (unsigned int)va_arg(ap, int);
		break;

	    case CTX_SET_ROP:
		rop = (short)va_arg(ap, int);
		break;

	    case CTX_SET_CLIP_LIST:
		ncliprect = (short)va_arg(ap, int);
		cliprects = va_arg(ap, Rect *);
		break;

	    case CTX_SET_CLIP_PLANES:
		clip_planes = (unsigned char)va_arg(ap, int);
		break;

	    case CTX_SET_LINE_WIDTH:
		line_width = (short)va_arg(ap, int);
		line_width_opt = (short)va_arg(ap, int);
		break;

	    case CTX_SET_LINE_TEX:
		pr_tex_ptr = va_arg(ap, short *);
		pr_tex_offset = (short)va_arg(ap, int);
		pr_tex_options = (short)va_arg(ap, int);
		break;

	    case CTX_SET_VWP_3D:
		xscale = (float)va_arg(ap, double);
		xoffs = (float)va_arg(ap, double);
		yscale = (float)va_arg(ap, double);
		yoffs = (float)va_arg(ap, double);
		zscale = (float)va_arg(ap, double);
		zoffs = (float)va_arg(ap, double);
		break;

	    case CTX_SET_MAT_NUM:
		use_mat_num = (unsigned char)va_arg(ap, int);
		break;

	    case CTX_SET_MAT_3D:
		set_mat_num = (unsigned char)va_arg(ap, int);
		matrix_3d = va_arg(ap, Matrix_3D *);
		break;

	    case CTX_SET_ZBUF:
		zbuf_depth = (short)va_arg(ap, int);
		zbuf_x = (short)va_arg(ap, int); 
		zbuf_y = (short)va_arg(ap, int); 
		zbuf_width = (short)va_arg(ap, int); 
		zbuf_height = (short)va_arg(ap, int); 
	        break;

	    case CTX_SET_DEPTH_CUE:
		depth_cue = (unsigned char)va_arg(ap, int);
		break;

	    case CTX_SET_DEPTH_CUE_COLORS:
		depth_cue_front_color = va_arg(ap, unsigned int);
		depth_cue_back_color = va_arg(ap, unsigned int);
		break;

	    case CTX_SET_HIDDEN_SURF:
		hidden_surf = (unsigned char)va_arg(ap, int);
		break;

	    case CTX_SET_RGB_COLOR:
		rgb_color = va_arg(ap, unsigned int);
		rgb_color_red = (float)(rgb_color & 0xff) / 255.0 ;
		rgb_color_green = (float)((rgb_color & 0xff00) >> 8) / 255.0 ;
		rgb_color_blue = (float)((rgb_color & 0xff0000) >>16) / 255.0 ;
		break;

	    default:
		va_end(pa);
		TRACE_OUT
		return(errmsg_list[25]);
	}
    }
    va_end(pa);
    TRACE_OUT
    return (ctx_post());
}

/**********************************************************************/
ctx_default()
/**********************************************************************/
/* Sets common context attributes to the default values */
{
    
    static short zero=0;
    static Matrix_3D id_3d = {
				    1.0, 0.0, 0.0, 0.0,
				    0.0, 1.0, 0.0, 0.0,
				    0.0, 0.0, 1.0, 0.0,
				    0.0, 0.0, 0.0, 1.0,
			     };

    func_name = "ctx_default";
    TRACE_IN

    dont_clear_ctx = 0;
    fb_num = gp1_d(GP_pr)->cg2_index;
    fb_planes = 0xffffffff;
    rop = PIX_SRC;
    ncliprect = 1;
    cliprects = &GP_r;
    clip_planes = 0x0;
    line_width = 1;
    line_width_opt = 0;
    pr_tex_ptr = &zero;
    pr_tex_offset = 0;
    pr_tex_options = 0;
    xscale = (float)GP_r.r_width/2.0;
    xoffs = (float)GP_r.r_width/2.0 + (float)GP_r.r_left;
    yscale = -(float)GP_r.r_height/2.0;
    yoffs = (float)GP_r.r_top + (float)GP_r.r_height/2.0;
    zscale = (float)0xffff;
    zoffs = 0.0;
    use_mat_num = 0;
    set_mat_num = 0;
    matrix_3d = &id_3d;
    zbuf_depth = 0xffff;
    zbuf_x = GP_r.r_left;
    zbuf_y = GP_r.r_top;
    zbuf_width = GP_r.r_width;
    zbuf_height = GP_r.r_height;
    depth_cue = GP1_DEPTH_CUE_OFF;
    depth_cue_front_color = 0xffffff;
    depth_cue_back_color = 0x0;
    hidden_surf = GP1_NOHIDDENSURF;
    rgb_color = 0xffffff;
    rgb_color_red = 1.0;
    rgb_color_green = 1.0;
    rgb_color_blue = 1.0;

    TRACE_OUT

}

/**********************************************************************/
char *
ctx_post()
/**********************************************************************/
/* sets context attributes */

{
    register int i;
    register int j;
    Rect *r;
    short *p;

    func_name = "ctx_post";
    TRACE_IN

    p = allocbuf();
    if (p == (short *)-1) {
	TRACE_OUT
	return (errmsg_list[52]);
    }

    if (!dont_clear_ctx) {
	*p++ = GP1_CLEAR_CONTEXT;
    }

    *p++ = GP1_SET_FB_NUM | fb_num;

    *p++ = GP3_SET_FB_PLANES_RGB;
    GP1_PUT_I(p, fb_planes);

    *p++ = GP1_SET_ROP;
    *p++ = rop;

    *p++ = GP1_SET_CLIP_LIST;
    *p++ = ncliprect;
    for (i = ncliprect, r = cliprects ; i > 0 ; i--) {
	*p++ = r->r_left;
	*p++ = r->r_top;
	*p++ = r->r_width;
	*p++ = r->r_height;
	r++;
    }

    *p++ = GP1_SET_CLIP_PLANES | clip_planes;

    *p++ = GP1_SET_LINE_WIDTH;
    *p++ = line_width; *p++ = line_width_opt;

    *p++ = GP1_SET_LINE_TEX;
    do {
	*p++ = *pr_tex_ptr;
    } while (*pr_tex_ptr++);
    *p++ = pr_tex_offset; *p++ = pr_tex_options;

    *p++ = GP1_SET_VWP_3D;
    GP1_PUT_F(p, xscale); GP1_PUT_F(p, xoffs);
    GP1_PUT_F(p, yscale); GP1_PUT_F(p, yoffs);
    GP1_PUT_F(p, zscale); GP1_PUT_F(p, zoffs);

    *p++ = GP1_SET_MAT_3D | set_mat_num;
    for (i = 0 ; i < 4 ; i++) {
	for (j = 0 ; j < 4 ; j++) {
	    GP1_PUT_F(p, matrix_3d->m[i][j]);
	}
    }

    *p++ = GP1_SET_MAT_NUM | use_mat_num;

    if (!dont_clear_ctx) {
	*p++ = GP1_SET_ZBUF;
	*p++ = zbuf_depth;
	*p++ = zbuf_x;
	*p++ = zbuf_y;
	*p++ = zbuf_width;
	*p++ = zbuf_height;
    }


    *p++ = GP1_SET_HIDDEN_SURF | hidden_surf;

    *p++ = GP1_SET_DEPTH_CUE | depth_cue;
    *p++ = GP1_SET_DEPTH_CUE_COLORS | GP2_RGB_COLOR_PACK;
    GP1_PUT_I(p, depth_cue_front_color);
    GP1_PUT_I(p, depth_cue_back_color);

    *p++ = GP2_SET_RGB_COLOR | GP2_RGB_COLOR_PACK;
    GP1_PUT_I(p, rgb_color);

    if (postbuf(p)) {
	TRACE_OUT
	return(errmsg_list[26]);
    } else {
	gp1_sync(GP_shmem, GP_fd);
	TRACE_OUT
	return  (char *)0;
    }
}

/**********************************************************************/
char *
set_3D_matrix(n, mp)
/**********************************************************************/
int n;
Matrix_3D *mp;
/* sets load the matrix slot n with the matrix pointed to by mp */

{
    register short *p;
    register int i;
    register int j;
    unsigned char mat_num;

    func_name = "set_3D_matrix";
    TRACE_IN

    mat_num = (unsigned char) n;

    p = allocbuf();
    if (p == (short *)-1) {
	TRACE_OUT
	return (errmsg_list[52]);
    }
    *p++ = GP1_SET_MAT_3D | mat_num;
    for (i = 0 ; i < 4 ; i++) {
	for (j = 0 ; j < 4 ; j++) {
	    GP1_PUT_F(p, mp->m[i][j]);
	}
    }

    if (postbuf(p)) {
	TRACE_OUT
	return(errmsg_list[26]);
    } else {
	gp1_sync(GP_shmem, GP_fd);
	TRACE_OUT
	return  (char *)0;
    }
}


/**********************************************************************/
clear_all()
/**********************************************************************/
/* clears all pixrect planes of the GP */
{
    func_name = "clear_all";
    TRACE_IN

    pr_clear_all(GP_pr);

    TRACE_OUT
}

/**********************************************************************/
clear_24bit_plane()
/**********************************************************************/
/* clear only the 24-bit plane */
{
    func_name = "clear_24bit_plane";
    TRACE_IN

    /* set plane group and attributes */
    pr_set_planes(GP_pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);

    pr_clear(GP_pr, PIX_CLR);

    TRACE_OUT
}

/**********************************************************************/
char *
pr_arbi_test()
/**********************************************************************/
{
    extern char *pr_all_test();
    char *errmsg;

    func_name = "pr_arbi_test";
    TRACE_IN

    pr_set_planes(GP_pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);
    pr_dbl_set(GP_pr, PR_DBL_DISPLAY, PR_DBL_BOTH,
		    PR_DBL_WRITE, PR_DBL_BOTH,
		    PR_DBL_READ, PR_DBL_BOTH,
		    PR_DBL_DISPLAY_DONTBLOCK, PR_DBL_BOTH,
		    0);
    errmsg = pr_all_test(GP_pr, PIXPG_24BIT_COLOR);

    TRACE_OUT
    return errmsg;

}

/**********************************************************************/
rgb_stripes()
/**********************************************************************/
{
    int err;

    func_name = "rgb_stripes";
    TRACE_IN

    err = pr_rgb_stripes(GP_pr);

    TRACE_OUT
    return err;
}


/**********************************************************************/
box(xmin, ymin, xmax, ymax)
/**********************************************************************/
int xmin;
int ymin;
int xmax;
int ymax;

{
    pr_set_planes(GP_pr, PIXPG_24BIT_COLOR, PIX_ALL_PLANES);

    pr_rop(GP_pr, xmin, ymin, xmax-xmin, 1, PIX_SET, (Pixrect *)0, 0, 0);
    pr_rop(GP_pr, xmax, ymin, 1, ymax-ymin, PIX_SET, (Pixrect *)0, 0, 0);
    pr_rop(GP_pr, xmin, ymax, xmax-xmin, 1, PIX_SET, (Pixrect *)0, 0, 0);
    pr_rop(GP_pr, xmin, ymin, 1, ymax-ymin, PIX_SET, (Pixrect *)0, 0, 0);
}
    
/* just to run the C30 simulator */
#ifdef RFS
tmessage() {}
pr_clear() {}
pr_clear_all() {}
pmessage() {}
error_report() {}
#endif RFS

/*
 * Put a call to the print routine below inside your trace code
 *
 * i.e.
 *	prt_shm_blocks();
 */

prt_shm_blocks()
{
	register int block;
	register short freeh, freel;
	register struct gp1_shmem *shp = (struct gp1_shmem *) GP_shmem;

	if (shp->alloc_sem)
	    printf("allocation semaphore is on\n");

	freeh = ~(shp->gp_alloc_h ^ shp->host_alloc_h);
	freel = ~(shp->gp_alloc_l ^ shp->host_alloc_l);

	for (block=0; block<16; block++)
		fputc(((1 << block) & freeh) ? '1' : '.', stdout);
	
	fputc(' ',stdout);

	for (block=0; block<16; block++)
		fputc(((1 << block) & freel) ? '1' : '.', stdout);

	fputc('\n',stdout);
}
