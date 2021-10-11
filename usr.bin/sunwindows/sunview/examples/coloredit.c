/***************************************************************************/
#ifndef lint
static char sccsid[] = "@(#)coloredit.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/***************************************************************************/

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>

#define MYFRAME		0
#define MYPANEL		1
#define MYCANVAS	2

#define PANELBG		0
#define PANELRED	1
#define PANELGREEN	2
#define PANELBLUE	3
/* default foreground color ?? */
#define PANELFG		7		

#define PANELCMS_SIZE	8
#define CANVASCMS_SIZE	4
/*
 * Colormap sizes for the three windows.  The panel only needs 5 colors,
 * but a colormap must be a power of two.
 */
mycms_sizes[3] = {
	2, PANELCMS_SIZE, CANVASCMS_SIZE
};
#define MYMAXCMS_SIZE	PANELCMS_SIZE
/*
 * Panel color arrays; initialize them with the panel's colors.
 * Since the panel has the largest colormap, we can reuse its arrays to hold
 * the colormap retrieved when editing colormaps.
 */
unsigned char   red[MYMAXCMS_SIZE] =   {255, 255,   0,   0, 255, 192, 192, 128};
unsigned char   green[MYMAXCMS_SIZE] = {255,   0, 255,   0, 192, 255, 192, 128};
unsigned char   blue[MYMAXCMS_SIZE] =  {255,   0,   0, 255, 192, 192, 255, 128};

/* Canvas color array */
unsigned char   canvasred[CANVASCMS_SIZE] =   {  0,   0, 255, 255};
unsigned char   canvasgreen[CANVASCMS_SIZE] = {  0, 255,   0, 192};
unsigned char   canvasblue[CANVASCMS_SIZE] =  {255,   0,   0, 192};

static void     getcms();
static void     setcms();
static void     cycle();
static void     editcms();
static void     set_color();
static void     change_value();

Panel_item      text_item;
Panel_item      color_item;
Panel_item      red_item, green_item, blue_item;

Pixwin         *pixwins[3];
Pixwin         *pw;


main(argc, argv)
	int             argc;
	char          **argv;
{
	Frame           base_frame;
	Panel           panel;
	Canvas          canvas;

	Attr_avlist	sliderdefaults;

	/* the cmsname is copied, so this array can be reused */
	char            cmsname[CMS_NAMESIZE];

	int             counter;
	int             xposition;
	char            buf[40];

	base_frame = window_create(NULL, FRAME,
				   FRAME_LABEL,		"coloredit",
				   FRAME_ARGS,		argc, argv,
				   0);
	/* get frame's pixwin */
	pixwins[MYFRAME] = (Pixwin *) window_get(base_frame, WIN_PIXWIN);

	/* set up the panel */
	panel = window_create(base_frame, PANEL,
			      0);
	/* get panel's pixwin, give it a special colormap */
	pw = pixwins[MYPANEL] = (Pixwin *) window_get(panel, WIN_PIXWIN);
	sprintf(cmsname, "coloreditpanel%D", getpid());
	pw_setcmsname(pw, cmsname);
	pw_putcolormap(pw, 0, mycms_sizes[MYPANEL], red, green, blue);

	/* create a reusable attribute list for my slider attributes */
	sliderdefaults = attr_create_list(
				PANEL_SHOW_ITEM,	TRUE,
				PANEL_MIN_VALUE,	0,
				PANEL_MAX_VALUE,	255,
				PANEL_SLIDER_WIDTH,	512,
				PANEL_SHOW_RANGE,	TRUE,
				PANEL_SHOW_VALUE,	TRUE,
				PANEL_NOTIFY_LEVEL,	PANEL_ALL,
				0);
				

	panel_create_item(panel, PANEL_CYCLE,
			  PANEL_LABEL_STRING,	"Edit colormap:",
			  PANEL_VALUE,		MYCANVAS,
			  PANEL_CHOICE_STRINGS,	"Frame", "Panel", "Canvas", 0,
			  PANEL_NOTIFY_PROC,	editcms,
			  0);


	text_item = panel_create_item(panel, PANEL_TEXT,
				PANEL_VALUE_DISPLAY_LENGTH,	CMS_NAMESIZE,
				PANEL_VALUE_STORED_LENGTH,	CMS_NAMESIZE,
				0);


	color_item = panel_create_item(panel, PANEL_SLIDER,
				       ATTR_LIST,		sliderdefaults,
				       PANEL_LABEL_STRING,	"color:",
				       PANEL_NOTIFY_PROC,	set_color,
				       0);

	red_item = panel_create_item(panel, PANEL_SLIDER,
				     ATTR_LIST,			sliderdefaults,
				     PANEL_LABEL_STRING,	"  red:",
				     PANEL_NOTIFY_PROC,		change_value,
				     PANEL_ITEM_COLOR,		PANELRED,
				     0);

	green_item = panel_create_item(panel, PANEL_SLIDER,
				       ATTR_LIST,		sliderdefaults,
				       PANEL_LABEL_STRING, "green:",
				       PANEL_NOTIFY_PROC, change_value,
				       PANEL_ITEM_COLOR,		PANELGREEN,
				       0);

	blue_item = panel_create_item(panel, PANEL_SLIDER,
				      ATTR_LIST,		sliderdefaults,
				      PANEL_LABEL_STRING, " blue:",
				      PANEL_NOTIFY_PROC, change_value,
				      PANEL_ITEM_COLOR,		PANELBLUE,
				      0);

	panel_create_item(panel, PANEL_BUTTON,
			  PANEL_LABEL_IMAGE,
		      panel_button_image(panel, "Cycle colormap", 12, NULL),
			  PANEL_NOTIFY_PROC, cycle,
			  0);

	window_fit(panel);
	window_fit_width(base_frame);

	/* free the slider attribute list */
	free(sliderdefaults);

	/* set up the canvas */
	canvas = window_create(base_frame, CANVAS, 0);


	/* get canvas' pixwin, give it a special colormap */
	pw = pixwins[MYCANVAS] = (Pixwin *) canvas_pixwin(canvas);
	sprintf(cmsname, "coloredit%D", getpid());
	pw_setcmsname(pw, cmsname);
	pw_putcolormap(pw, 0, mycms_sizes[MYCANVAS],
			canvasred, canvasgreen, canvasblue);

	/* draw in the canvas */
	/* don't draw color 0 -- it is the background */
	for (counter = 1; counter < mycms_sizes[MYCANVAS]; counter++) {
		xposition = counter * 100;
		pw_rop(pw, xposition, 50, 50, 50,
		       PIX_SRC | PIX_COLOR(counter), NULL, 0, 0);
		sprintf(buf, "%d", counter);
		pw_text(pw, xposition + 5, 70, PIX_SRC ^ PIX_DST, 0, buf);
	}
	pw_text(pw, 100, 150,
		PIX_SRC | PIX_COLOR(mycms_sizes[MYCANVAS] - 1), 0,
		"This is written in the foreground color");

	/* initialize to edit the first canvas color */
	editcms(NULL, MYCANVAS, NULL);

	window_main_loop(base_frame);
	exit(0);
	/* NOTREACHED */
}

static int      cur_cms = -1;
/* ARGSUSED */
static void
editcms(item, value, event)
	Panel_item      item;
	unsigned int    value;
	Event          *event;
{
	int             planes;
	struct colormapseg cms;
	char            cmsname[CMS_NAMESIZE];

	if (value == cur_cms)
		return;

	cur_cms = value;
	/* get the new cmsname */
	pw_getcmsname(pixwins[cur_cms], cmsname);
	panel_set_value(text_item, cmsname);

	pw = pixwins[cur_cms];

	/* get the new colormap */
	/*
	 * first have to get its size there is NO DOCUMENTED procedure to do
	 * this. 
	 */
	pw_getcmsdata(pw, &cms, &planes);

	pw_getcolormap(pw, 0, cms.cms_size, red, green, blue);

	panel_set(color_item,
		  PANEL_VALUE, 0,
		  PANEL_MAX_VALUE, cms.cms_size - 1,
		  0);
	/* call the proc to set the colors */
	set_color(NULL, 0, NULL);
}

int             cur_color;
/* ARGSUSED */
static void
set_color(item, color, event)
	Panel_item      item;
	unsigned int    color;
	struct inputevent *event;
{
	panel_set_value(red_item, red[color]);
	panel_set_value(green_item, green[color]);
	panel_set_value(blue_item, blue[color]);
	cur_color = (unsigned char) color;

}

/* ARGSUSED */
static void
change_value(item, value, event)
	Panel_item      item;
	int             value;
	struct inputevent *event;
{
	if (item == red_item)
		red[cur_color] = (unsigned char) value;
	else if (item == green_item)
		green[cur_color] = (unsigned char) value;
	else
		blue[cur_color] = (unsigned char) value;
	/*
	 * pw_putcolormap expects arrays of colors, but this only sets one
	 * color 
	 */
	pw_putcolormap(pw, cur_color, 1,
		       &red[cur_color], &green[cur_color], &blue[cur_color]);
}

/* ARGSUSED */
static void
cycle(item, event)
	Panel_item      item;
	Event          *event;
{
	pw_cyclecolormap(pw, 1, 0, mycms_sizes[cur_cms]);
}
