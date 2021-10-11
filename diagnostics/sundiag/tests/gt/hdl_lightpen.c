
#ifndef lint
static  char sccsid[] = "@(#)hdl_lightpen.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sundev/vuid_event.h>
#include <sbusdev/gtreg.h>
#include <sundev/lightpenreg.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <pixrect/pixrect_hs.h>
#include <pixrect/pr_line.h>
#include <errmsg.h>
#include <gttest.h>

#define FONT_NAME	"/usr/lib/fonts/fixedwidthfonts/cour.b.24"
#define MAXCOLOR	8		/* # of colors to switch */
#define LINE_WIDTH	2		/* Line width to draw with */
#define RED		0xff0000	/* Line colors in XRGB format */
#define GREEN		0xff00
#define BLUE		0xff
#define CALIB_X		1280/2		/* Position for calibration */
#define CALIB_Y		1024/2
#define TEXT_LINE_HEIGTH	26	/* Text line spacing */
#define TEXTPOS_X	300		/*Position of announcing text */
#define TEXTPOS_Y	30
#define INFOPOS_Y	600
#define TOLERANCE	10

extern
int			errno;		/* Unix error number */
extern
char			*sys_errlist[];	/* Unix error message list */

extern
char			*lightpen_msg[];

static
char			errtxt[512];

static
char			lightpen_device_name[] = LIGHTPEN_DEVICE_NAME;

static
struct fb_lightpenparam save_lp_param = { -1, -1};

static int colors[MAXCOLOR] = { 0xff0000, 0xff00, 0xff, 0xffff00,
				0xff00ff, 0xffff, 0xffffff, 0x0 };

static char *color_names[MAXCOLOR] = {  "RED", "GREEN", "BLUE",
					"MAGENTA", "YELLOW", "WHITE",
					"BLACK" };

static char infomsg[256];

/**********************************************************************/
char *
hdl_lightpen()
/**********************************************************************/

{
    Pixrect *open_pr_device();
    char *pr_visual_test();

    Pixrect *pr = (Pixrect *)NULL;
    Pixfont *font = (Pixfont *)NULL;
    Firm_event	lp_event;
    char *errmsg = (char *)NULL;
    int n;
    int forever = TRUE;
    int lp_fd = -1;
    int x = -1;
    int y = -1;
    int oldx = -1;
    int oldy = -1;
    int oldbutton = 0;
    int color_ind = 1;
    int op = PIX_SRC | PIX_COLOR(1) | PIX_DONTCLIP;
    struct pr_brush brush;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
    int i;
    int calibrated = 0;
    int xcal = 0;
    int ycal = 0;
    int textpos_y;
    lightpen_calibrate_t lc;

    func_name = "hdl_lightpen";
    TRACE_IN

    pr = open_pr_device();
    /* make the 8-bit plane visible */
    (void)pr_set_planes(pr, PIXPG_WID, PIX_ALL_PLANES);
    pr_clear(pr, PIX_SRC | PIX_COLOR(wid_alloc(pr, FB_WID_DBL_8)));

    (void)pr_clear_all(pr);

    errmsg = pr_visual_test(pr);
    if (errmsg) {
	goto returning;
    }
    /* preset LUT */
    for (i = 0 ; i < 256 ; i++) {
	red[i] = green[i] = blue[i] = (unsigned char) i;
    }

    for (i = 0 ; i < MAXCOLOR ; i++) {
	red[i+1] = (unsigned char)((colors[i] & RED) >> 16);
	green[i+1] = (unsigned char)((colors[i] & GREEN) >> 8);
	blue[i+1] = (unsigned char)(colors[i] & BLUE);
    }
    (void)gt_putlut_blocked(pr, 0|PR_DONT_DEGAMMA, 256, red, green, blue);

    if (!lightpen_enable()) {
	errmsg = DLXERR_LIGHTPEN_ENABLE;
	goto returning;
    }

    /* Open the lightpen device to acquire events */
    lp_fd = open(lightpen_device_name, O_RDWR);
    if (lp_fd < 0) {
	(void)sprintf(errtxt, DLXERR_OPEN_DEVICE, lightpen_device_name);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	errmsg = errtxt;
	goto returning;
    }

    /* set non-blocking I/O mode */
    if (fcntl(lp_fd, F_SETFL, O_NDELAY) == -1) {
	(void)sprintf(errtxt, DLXERR_F_SETFL_NONBLOCKING);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	fb_send_message(SKIP_ERROR, WARNING, errtxt);
    }

    /* calibration */
    font = pf_open(FONT_NAME);
    if (font == (Pixfont *)NULL) {
	(void)sprintf(errtxt, DLXERR_OPEN_FONT, FONT_NAME);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	errmsg = errtxt;
	goto returning;
    }
    for (i = 0, textpos_y = TEXTPOS_Y ; *(lightpen_msg[i]) ; i++) {
	if (*(lightpen_msg[i]) != '\n') {
	    (void)pr_ttext(pr, TEXTPOS_X, textpos_y,
			PIX_SRC|PIX_COLOR(3)|PIX_DONTCLIP, font,
						    lightpen_msg[i]);
	}
	textpos_y += TEXT_LINE_HEIGTH;
    }

    brush.width = LINE_WIDTH*2;

    draw(pr, CALIB_X-15, CALIB_Y, CALIB_X+15, CALIB_Y,
		&brush, (struct pr_texture *) NULL, PIX_SRC|PIX_COLOR(1));
    draw(pr, CALIB_X, CALIB_Y-15, CALIB_X, CALIB_Y+15,
		&brush, (struct pr_texture *) NULL, PIX_SRC|PIX_COLOR(1));
    while (!calibrated) {
	lp_event.value = 0;
	while ((n = read(lp_fd, &lp_event, sizeof(lp_event))) == -1) {
	    check_key();
	}
	if (n != sizeof(lp_event)) {
	    errmsg = DLXERR_LIGHTPEN_EVENT;
	    goto returning;
	}
	switch(LIGHTPEN_EVENT(lp_event.id)) {
	    case LIGHTPEN_MOVE:
	    case LIGHTPEN_DRAG:
		x = GET_LIGHTPEN_X(lp_event.value);
		y = GET_LIGHTPEN_Y(lp_event.value);
		sprintf(infomsg, "X=%10d,   Y=%10d", x, y);
		(void)pr_text(pr, TEXTPOS_X+100, INFOPOS_Y,
		    PIX_SRC|PIX_COLOR(5)|PIX_DONTCLIP, font, infomsg);
		break;
	    case LIGHTPEN_BUTTON:
		if (lp_event.value) {
		    xcal = CALIB_X - x;
		    ycal = CALIB_Y - y;
		    lc.x = xcal;
		    lc.y = ycal;
		    if (ioctl(lp_fd, LIGHTPEN_CALIBRATE, &lc)){
			(void)fb_send_message(SKIP_ERROR, WARNING, DLXERR_LIGHTPEN_CALIBRATION);
		    } else {
			xcal = 0;
			ycal = 0;
		    }
		    calibrated = 1;
		    x = -1;
		    y = -1;

		}
		break;
	    case LIGHTPEN_UNDETECT:
		sprintf(infomsg, "Lightpen NOT detecting light");
		(void)pr_text(pr, TEXTPOS_X+100, INFOPOS_Y,
		    PIX_SRC|PIX_COLOR(5)|PIX_DONTCLIP, font, infomsg);
		    break;

	    default:
		break;
	}
    }


    errmsg = pr_visual_test(pr);
    if (errmsg) {
	goto returning;
    }


    brush.width = LINE_WIDTH;

    /* loop untill interrupted by user */
    while (forever) {
	if (check_key() == (int)'q') {
	    errmsg = (char *)0;
	    goto returning;
	}
	while ((n = read(lp_fd, &lp_event, sizeof(lp_event))) == -1) {
	    if (check_key() == (int)'q') {
		errmsg = (char *)0;
		goto returning;
	    }
	}
	if (n != sizeof(lp_event) && n != -1) {
	    errmsg = DLXERR_LIGHTPEN_EVENT;
	    goto returning;
	}

	switch(LIGHTPEN_EVENT(lp_event.id)) {
	    case LIGHTPEN_MOVE:
	    case LIGHTPEN_DRAG:
		x = GET_LIGHTPEN_X(lp_event.value) + xcal;
		y = GET_LIGHTPEN_Y(lp_event.value) + ycal;
		if (x < 1280 && x > 0 && y < 1024 && y > 0) {
		    draw(pr, oldx, oldy, x, y, &brush,
				    (struct pr_texture *) NULL, op);
		    display_coord(x, y);
		    oldx = x;
		    oldy = y;
		}
		break;

	    case LIGHTPEN_BUTTON:
		if (lp_event.value && (oldbutton == 0)) {
		    color_ind++;
		    if (color_ind > MAXCOLOR) {
			color_ind = 1;
		    }
		    op = PIX_SRC | PIX_COLOR((unsigned int)color_ind) |
							PIX_DONTCLIP;
		    display_color(color_ind);
		}
		oldbutton = lp_event.value;
		break;
	    case LIGHTPEN_UNDETECT:
	        x = -1;
	        y = -1;
	        oldx = -1;
	        oldy = -1;
		(void)pr_visual_test(pr);
		break;
	    default:
		    /*ignore all other event */
		break;
	}

    }

returning:
    if (pr) {
	close_pr_device();
    }
    if (lp_fd >= 0) {
	(void)close(lp_fd);
    }
    if (font) {
	(void)pf_close(font);
    }
    (void)lightpen_disable();

    return errmsg;

}

/*********************************************************************/
int
lightpen_enable()
/*********************************************************************/

{
    struct fb_lightpenparam lp_param;
    int flag;
    int fd = -1;

    save_lp_param.pen_distance = -1;
    save_lp_param.pen_time = -1;

    fd = open(device_name, O_RDWR);
    if (fd < 0) {
	(void)sprintf(errtxt, DLXERR_OPEN_DEVICE, device_name);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	(void)error_report(errtxt);
	return 0;
    }

    flag = 1;
    if (ioctl(fd, FB_LIGHTPENENABLE, &flag) < 0) {
	(void)strcpy(errtxt, DLXERR_IOCTL_FB_LIGHTPENENABLE);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	(void)error_report(errtxt);
	(void)close(fd);
	return 0;
    }

    if (ioctl(fd, FB_GETLIGHTPENPARAM, &save_lp_param) < 0){
	(void)strcpy(errtxt, DLXERR_IOCTL_FB_GETLIGHTPENPARAM);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	(void)error_report(errtxt);
	(void)close(fd);
	return 0;
    }

    lp_param.pen_distance = 1;
    lp_param.pen_time = 120;
    if (ioctl(fd, FB_SETLIGHTPENPARAM, &lp_param) < 0){
	(void)strcpy(errtxt, DLXERR_IOCTL_FB_SETLIGHTPENPARAM);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	(void)error_report(errtxt);
	(void)close(fd);
	return 0;
    }

    (void)close(fd);
    return 1;
}

/*********************************************************************/
int
lightpen_disable()
/*********************************************************************/
{
    int fd = -1;
    int flag;

    /* if lightpen is not enabled, return immediately */
    if (save_lp_param.pen_distance < 0 || save_lp_param.pen_time < 0) {
	return 1;
    }

    fd = open(device_name, O_RDWR);
    if (fd < 0) {
	(void)sprintf(errtxt, DLXERR_OPEN_DEVICE, device_name);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	(void)error_report(errtxt);
	return 0;
    }

    /* restore the original parameters */
    if (ioctl(fd, FB_SETLIGHTPENPARAM, &save_lp_param) < 0){
	(void)strcpy(errtxt, DLXERR_IOCTL_FB_SETLIGHTPENPARAM);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	(void)error_report(errtxt);
	(void)close(fd);
	return 0;
    }

    /* disable the lightpen */
    flag = 0;
    if (ioctl(fd, FB_LIGHTPENENABLE, &flag) < 0) {
	(void)strcpy(errtxt, DLXERR_IOCTL_FB_LIGHTPENENABLE);
	(void)strcat(errtxt, sys_errlist[errno]);
	(void)strcat(errtxt, ".\n");
	(void)error_report(errtxt);
	(void)close(fd);
	return 0;
    }

    save_lp_param.pen_distance = -1;
    save_lp_param.pen_time = -1;
    (void)close(fd);
    return 1;
}

/**********************************************************************/
draw(pxr, x0, y0, x1, y1, br, tx, opr)
/**********************************************************************/
Pixrect *pxr;
int x0;
int y0;
int x1;
int y1;
struct pr_brush *br;
struct p_texture *tx;
int opr;


{
    if (x0 <= 0 || y0 <= 0 || x1 <= 0 || y1 <=0) {
	return;
    }
    return(pr_line(pxr, x0, y0, x1, y1, br, tx, opr));

}


/**********************************************************************/
display_coord(xcoord, ycoord)
/**********************************************************************/
int xcoord;
int ycoord;
{ }

/**********************************************************************/
display_lightpen_undetect()
/**********************************************************************/

{ }

/**********************************************************************/
display_color(ind)
/**********************************************************************/

int ind;

{ }
