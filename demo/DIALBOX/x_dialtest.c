
#ifndef lint
static char sccsid[] = "@(#)x_dialtest.c 1.1 92/07/30 Copyright 1988, 1990 Sun Micro";
#endif

 
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>


#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/notice.h>
#include <xview/xv_error.h>


#include <math.h>

short dial_bits[] =
{
#include "dial_icon.h"
};
short dial_mask_bits[] =
{
#include "dial_mask_icon.h"
};
Icon    dial_icon;
Server_image icon_image, mask_image;

#define NDIALS 8
/* converts dial units (64th of degrees) to radians */
#define DIAL_TO_RADIANS(n) \
	((float)(n) * 2.0 * M_PI) / (float) DIAL_UNITS_PER_CYCLE

/* these are defined in <sundev/dbio.h>, but since that may not be installed */
/* they are also defined here */

#define DIAL_DEVID		0x7B
#define	event_is_dial(event)	(vuid_in_range(DIAL_DEVID, event->ie_code))
#define DIAL_NUMBER(event_code)	(event_code & 0xFF)
#define DIAL_UNITS_PER_CYCLE (64*360)


#define BLACK 0
#define WHITE 255


Xv_Window frame;
Xv_Window panel;
Xv_Window canvas;

Pixwin *dial_pixwin;

void dial_event_proc(), draw_dials();
extern void diag_notify(), dump_notify();

Notify_value
read_db(cli, fd)
    Notify_client cli;
    int fd;
{
    int byt_cnt, red;
    char buf[16];

    if(ioctl(fd, FIONREAD, &byt_cnt) == -1 || byt_cnt == 0) return;
    if((red = read(fd, buf, sizeof(buf))) == 16) dial_event_proc(0, buf, 0);
    else fprintf(stderr, "read %d bytes\n", red);
}

dial_error_proc(obj, avlist)
	Xv_object obj;
	Attr_attribute avlist[ATTR_STANDARD_SIZE];
{
	Attr_avlist attrs;
	Error_layer layer;
	Error_severity sev;
	int n = 0;
	char buf[7][64];
	char *bufp[7];

	for(n = 0; n < 7; n++) bufp[n] = &buf[n][0];

	n = 0;

	for(attrs = avlist; *attrs && n < 5; attrs = attr_next(attrs)) {
		switch((int) attrs[0]) {
		case ERROR_BAD_ATTR:
			/* fprintf(stderr, "bad attribute: %s", attr_name(attrs[1])); */
			sprintf(buf[n++], "bad attribute: %s", attr_name(attrs[1]));
			break;
		case ERROR_BAD_VALUE:
			/* fprintf(stderr, "bad value: 0x%x for attribute: %s",
				attrs[1], attr_name(attrs[2])); */
			sprintf(buf[n++], "bad value: 0x%x for attribute: %s",
				attrs[1], attr_name(attrs[2]));
			break;
		case ERROR_STRING:
			/* fprintf(stderr, "%s", (char*) (attrs[1])); */
			sprintf(buf[n++], "%s", (char*) (attrs[1]));
			break;
		case ERROR_SEVERITY:
			sev = attrs[1];
			break;
		case ERROR_SERVER_ERROR:
			/* fprintf(stderr, "server error: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "server error: %s", (char*) (attrs[1]));
			break;
		case ERROR_PKG:
			/* fprintf(stderr, "package: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "package: %s", (char*) (attrs[1]));
			break;
		case ERROR_LAYER:
			/* fprintf(stderr, "layer: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "layer: %s", (char*) (attrs[1]));
			break;
		case ERROR_CANNOT_SET:
			/* fprintf(stderr, "cannot set error: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "cannot set error: %s", (char*) (attrs[1]));
			break;
		case ERROR_CREATE_ONLY:
			/* fprintf(stderr, "create only error: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "create only error: %s", (char*) (attrs[1]));
			break;
		case ERROR_CANNOT_GET:
			/* fprintf(stderr, "cannot get error: %s", (char*) (attrs[1]));*/
			sprintf(buf[n++], "cannot get error: %s", (char*) (attrs[1]));
			break;
		case ERROR_INVALID_OBJECT:
			/* fprintf(stderr, "invalid object: %s", (char*) (attrs[1])); */
			sprintf(buf[n++], "invalid object: %s", (char*) (attrs[1]));
			break;
		}
	}
	strcpy(buf[n++], "Core dump?");
	bufp[n] = (char *) 0;
	if(sev == ERROR_NON_RECOVERABLE) {
		if(frame != (Xv_Window) 0 && notice_prompt(frame, (Event *) NULL,
			NOTICE_MESSAGE_STRINGS_ARRAY_PTR, bufp,
			NOTICE_BUTTON_YES, "yes",
			NOTICE_BUTTON_NO, "no",
			0) == NOTICE_YES) abort();
		exit(1);
	}
	
	return(XV_OK);
}




main(argc, argv)
	int argc;
	char **argv;
{
	int db_fd, i;

    Notify_client client = (Notify_client) 0x1235;

	dial_reset();

    icon_image = xv_create(0, SERVER_IMAGE,
        SERVER_IMAGE_BITS, dial_bits,
        SERVER_IMAGE_DEPTH, 1,
        XV_WIDTH, 64,
        XV_HEIGHT, 64,
        0);

    mask_image = xv_create(0, SERVER_IMAGE,
        SERVER_IMAGE_BITS, dial_mask_bits,
        SERVER_IMAGE_DEPTH, 1,
        XV_WIDTH, 64,
        XV_HEIGHT, 64,
        0);

    dial_icon = xv_create(NULL, ICON,
        ICON_IMAGE, icon_image,
        ICON_MASK_IMAGE, mask_image,
        ICON_TRANSPARENT, TRUE,
        0);

	xv_init(
		XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_ERROR_PROC, dial_error_proc,
		0);
 
	frame = xv_create(NULL, FRAME,
		XV_LABEL, "Dialtool",
		FRAME_ICON, dial_icon,
		0);

	panel = xv_create(frame, PANEL, 0);

	xv_create(panel, PANEL_BUTTON, 
		PANEL_LABEL_STRING, "Diagnostics",
		PANEL_NOTIFY_PROC, diag_notify,
	    XV_X, xv_col(panel, 0),
	    XV_Y, xv_row(panel, 0),
		0);

	xv_create(panel, PANEL_BUTTON, 
		PANEL_LABEL_STRING, "Ram dump",
		PANEL_NOTIFY_PROC, dump_notify,
	    XV_X, xv_col(panel, 20),
	    XV_Y, xv_row(panel, 0),
		0);

	window_fit_height(panel);
	
	if ((db_fd = open("/dev/dialbox", O_RDWR)) == -1)  {
		perror("Open failed for /dev/dialbox:");
		exit(1);
	}
 
    notify_set_input_func(client, read_db, db_fd);
	
	canvas = xv_create(frame, CANVAS,
		CANVAS_WIDTH, 256,
		CANVAS_HEIGHT, 512,
		XV_HEIGHT, 512,
		XV_WIDTH, 262,
		CANVAS_RETAINED, FALSE,
		CANVAS_REPAINT_PROC, draw_dials,
		WIN_EVENT_PROC, dial_event_proc,
		0);

	dial_pixwin = canvas_pixwin(canvas);

	window_fit_width(panel);

	window_fit_height(frame);
	window_fit_width(frame);
	window_fit(frame);

	draw_dials();
	window_main_loop(frame);
}

static float angle[9];

dial_reset()
{
	int i;
	for (i=0; i < NDIALS; i++) {
		angle[i] = M_PI;
	}
}

void
dial_event_proc(window, event, arg)
	Xv_Window window;
	Event *event;
	caddr_t arg;
{
	int i;
	static int delta;

#ifdef DIALTEST_DEBUG
    printf("dep: c %04x f %04x m %04x x %04x y %04x\n",
        event -> ie_code, event -> ie_flags, event -> ie_shiftmask,
        event -> ie_locx, event -> ie_locy);
#endif DIALTEST_DEBUG

	if (event_is_dial(event)) {
		i = DIAL_NUMBER(event->ie_code);
		/*
		delta = (int)xv_get(canvas, WIN_EVENT_STATE, event->ie_code);
		*/
		delta = (int) event -> ie_locx;

#ifdef DIALTEST_DEBUG
        printf("\td: %d b: %d\n", i, delta);
#endif DIALTEST_DEBUG
 
		draw_dial(i, angle[i], BLACK);
		angle[i] += DIAL_TO_RADIANS(delta);
		draw_dial(i, angle[i], WHITE);
	}
}

struct pr_pos origins_8[8] =
{
	{64,  64},	
	{64, 192},
	{64, 320},
	{64, 448},
	{192,  64},	
	{192, 192},
	{192, 320},
	{192, 448},
};

struct pr_pos origins_9[9] =
{
	{64,  64},	{192, 64}, {320, 64},
	{64, 192},	{192, 192}, {320, 192},
	{64, 320},	{192, 320}, {320, 320}
};

struct pr_pos *dial_origin = origins_8;

void
draw_dials()
{
	int i;
	for (i=0; i < 8; i++) {
		pw_circle(dial_pixwin, dial_origin[i].x, dial_origin[i].y, 48, WHITE);
		draw_dial(i, angle[i], WHITE);
	}
}


draw_dial(dialnum, angle, color)
	int dialnum;
	float angle;
	int color;
{
	struct pr_pos to;

	to.x = dial_origin[dialnum].x + (int)(sin(-angle) * 46.0);
	to.y = dial_origin[dialnum].y + (int)(cos(-angle) * 46.0);
	pw_vector(dial_pixwin, dial_origin[dialnum].x,
		dial_origin[dialnum].y,
		to.x,
		to.y,
		PIX_SRC, color);
}

