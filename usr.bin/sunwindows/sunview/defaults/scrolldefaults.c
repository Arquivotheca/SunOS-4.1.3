#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)scrolldefaults.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/**************************************************************************/
/* 			scrolldefaults.c                                  */
/**************************************************************************/

#include <stdio.h>
#include <strings.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/scrollbar.h>
#include <sunwindow/defaults.h>
#include <suntool/textsw.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

/* Panel notify procs */
static void     save_proc();
static void     default_proc();
static void     quit_proc();
static void     have_v_sb_proc();
static void     have_h_sb_proc();
static void     use_normalize_proc();
static void     h_gravity_proc();
static void     v_gravity_proc();
static void     bar_display_proc();
static void     bubble_display_proc();
static void     have_buttons_proc();
static void     show_active_proc();
static void     bar_width_proc();
static void     end_point_area_proc();
static void     repeat_proc();
static void     button_length_proc();
static void     bubble_margin_proc();
static void     bubble_color_proc();
static void     bar_color_proc();
static void     border_proc();

/* Misc init & util procs */
static void	init_text();
static void	init_control();
static void	set_panel_from_scrollbars();
static void	init_panel();

/* Windows and Panel items */
static Frame    base_frame;
static Panel    control, panel;
static Textsw   msg_textsw;
static Panel_item have_v_sb_item;
static Panel_item have_h_sb_item;
static Panel_item h_gravity_item;
static Panel_item v_gravity_item;
static Panel_item bar_display_item;
static Panel_item bubble_display_item;
static Panel_item have_buttons_item;
static Panel_item bar_width_item;
static Panel_item button_length_item;
static Panel_item end_point_area_item;
static Panel_item repeat_item;
static Panel_item bubble_margin_item;
static Panel_item bubble_color_item;
static Panel_item bar_color_item;
static Panel_item border_item;

/* statics */
static int      texttoo = FALSE;
static int      debugging = FALSE;
static char     Textsw_string[4000];

static int      my_margin = 15;
static int      my_gap = 5;
static int      my_row_height = 55;
static int      my_col_width = 110;

static Scrollbar vertical_sb, horizontal_sb;

/* Note: these defaults should be in scrollbar.h!! */
#define HORIZ_PLACEMENT_DEFAULT	SCROLL_NORTH
#define VERT_PLACEMENT_DEFAULT	SCROLL_WEST
#define BAR_DISPLAY_DEFAULT	SCROLL_ALWAYS
#define BAR_THICKNESS_DEFAULT	14
#define BAR_COLOR_DEFAULT	SCROLL_GREY
#define BUBBLE_DISPLAY_DEFAULT	SCROLL_ALWAYS
#define BUBBLE_MARGIN_DEFAULT	0
#define BUBBLE_COLOR_DEFAULT    SCROLL_GREY
#define BUTTON_DISPLAY_DEFAULT	TRUE
#define BUTTON_LENGTH_DEFAULT	15
#define BORDER_DEFAULT		TRUE
#define END_POINT_AREA_DEFAULT	6
#define REPEAT_TIME_DEFAULT	10

#define MIN_THICKNESS		2

/* Icons */
static short icon_data[] = {
#include <images/scrollbar_demo.icon>
};

static short folder_data[] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x007F, 0xFFC0, 0x0000, 0x0000, 0x0080, 0x0040,
    0x3FFF, 0xFFFF, 0xFF80, 0x0020, 0x4000, 0x0000, 0x0080, 0x0030,
    0x4000, 0x0000, 0x007F, 0xFFE8, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0x8000, 0x0000, 0x0000, 0x0034,
    0x8000, 0x0000, 0x0000, 0x002A, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFF4,
    0x2AAA, 0xAAAA, 0xAAAA, 0xAAAA, 0x1555, 0x5555, 0x5555, 0x5554,
    0x0AAA, 0xAAAA, 0xAAAA, 0xAAAA, 0x0555, 0x5555, 0x5555, 0x5554
};
static short paper_data[] = {
    0x000F, 0xFFFF, 0xFFE0, 0x0000, 0x0008, 0x0000, 0x0030, 0x0000,
    0x0008, 0x0000, 0x0028, 0x0000, 0x0008, 0x0000, 0x0024, 0x0000,
    0x0008, 0x0000, 0x0022, 0x0000, 0x0008, 0x0000, 0x0021, 0x0000,
    0x0008, 0x0000, 0x0020, 0x8000, 0x0008, 0x0000, 0x0020, 0x4000,
    0x0008, 0x0000, 0x0020, 0x2000, 0x0008, 0x0000, 0x003F, 0xF000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x0008, 0x0000, 0x0000, 0x1000,
    0x0008, 0x0000, 0x0000, 0x1000, 0x000F, 0xFFFF, 0xFFFF, 0xF000
};

mpr_static(scrd_icon_image, 64, 64, 1, icon_data);
mpr_static(scrd_folder_image, 64, 52, 1, folder_data);
mpr_static(scrd_paper_image, 64, 52, 1, paper_data);

/**************************************************************************/
/* main                                                                   */
/**************************************************************************/

#ifdef STANDALONE
main(argc,argv)
#else
int scrolldefaults_main(argc,argv)
#endif
    int             argc;
    char          **argv;
{
/*    moncontrol (0); */
    defaults_special_mode();

    base_frame = window_create((Window)NULL, FRAME,
	FRAME_ARGC_PTR_ARGV, &argc, argv,
	FRAME_LABEL, "scrolldefaults",
	FRAME_ICON, icon_create(ICON_IMAGE, &scrd_icon_image, 0),
	WIN_ROWS, 26,
	WIN_ERROR_MSG, "base frame error!",
	0);
    if (base_frame == NULL) {
        (void)fprintf(stderr,"defaultstool: unable to create frame\n");
        exit(1);
    }

    my_row_height += (int) window_get(base_frame, WIN_ROW_HEIGHT);

    argv++;
    argc--;
    while (argc > 0 && **argv == '-') {
	switch (argv[0][1]) {
	case 'm':
	    my_margin = atoi(&argv[0][2]);
	    break;

	case 'g':
	    my_gap = atoi(&argv[0][2]);
	    break;

	case 't':
	    texttoo = TRUE;
	    break;

	case '?':
	case 'h':
	    (void)printf ("Usage: scrolldefaults [-t] [-d] [-gN] [-mN]\n");
	    (void)printf ("Options:\n");
	    (void)printf ("    -t     Text window too.\n");
	    (void)printf ("    -d     Debug mode\n");
	    (void)printf ("    -gN    Gap    = N pixels (for SCROLL_TO_GRID)\n");
	    (void)printf ("    -mN    Margin = N pixels (for SCROLL_TO_GRID)\n");
	    EXIT(0);

	case 'd':
	    debugging = TRUE;
	    /* test stuff here */
	    break;
	}
	argv++;
	argc--;
    }
    my_row_height += my_gap;
    my_col_width  += my_gap;

    vertical_sb = scrollbar_create(
	SCROLL_MARGIN, my_margin,
	SCROLL_GAP,    my_gap,
	0);

    horizontal_sb = scrollbar_create(
	SCROLL_MARGIN, my_margin,
	SCROLL_GAP,    my_gap,
	0);

    if (debugging) {
        (void)printf("Scrollbar_thickness=%d\n",
	    scrollbar_get(vertical_sb, SCROLL_THICKNESS));
        (void)printf("Scrollbar_end_point_area=%d\n",
	    scrollbar_get(vertical_sb, SCROLL_END_POINT_AREA));
	(void)printf("Scrollbar_repeat_time=%d\n",
	    scrollbar_get (vertical_sb, SCROLL_REPEAT_TIME));
    }

    init_panel();
    init_control();
    if (texttoo)
	init_text(TRUE);
    (void)window_fit(base_frame);

    window_main_loop(base_frame);
    
    EXIT(0);
}
/**************************************************************************/
/* initialize panels                                                      */
/**************************************************************************/

static void
init_text(first_time)
    int             first_time;
{
    if (first_time)
	while (strlen(Textsw_string) < sizeof(Textsw_string) - 500) {
	    (void)strcat(Textsw_string, "Jjfkd sone ofhtr aos\n");
	    (void)strcat(Textsw_string, "hroehnf lceh. Fho\n");
	    (void)strcat(Textsw_string, "blux vah? Boen tgrf!\n");
	    (void)strcat(Textsw_string, "Mani hfi do ehf os fh\n");
	    (void)strcat(Textsw_string, "jf id hf ikdjf odfj.\n\n");
	}

    else
	textsw_destroy(msg_textsw);

    msg_textsw = (Textsw)(LINT_CAST(window_create(
   	(Window)(LINT_CAST(base_frame)), TEXTSW,
	WIN_COLUMNS, 25,
	TEXTSW_CONTENTS, Textsw_string,
	WIN_ERROR_MSG, "textsw creation error!",
	0)));
    if (msg_textsw == NULL) { 
        (void)fprintf(stderr,"defaultstool: unable to create message window\n"); 
        exit(1); 
    }
}

static void
init_control()
{
    int		row = 0;
    
    control = window_create(base_frame, PANEL,
	WIN_ERROR_MSG, "panel creation error!",
	0);
    if (control == NULL) { 
        (void)fprintf(stderr,"defaultstool: unable to create panel\n"); 
        exit(1); 
    }

    (void)panel_create_item(control, PANEL_MESSAGE,
	PANEL_LABEL_BOLD, TRUE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "   User Settable Defaults",
	0);

    v_gravity_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, " Vert. Bar Placement",
	PANEL_CHOICE_STRINGS, "West ", "East", 0,
	PANEL_NOTIFY_PROC, v_gravity_proc,
	0);

    h_gravity_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "Horiz. Bar Placement",
	PANEL_CHOICE_STRINGS, "North", "South", 0,
	PANEL_NOTIFY_PROC, h_gravity_proc,
	0);

    bubble_display_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "         Show Bubble",
	PANEL_CHOICE_STRINGS, "Always", "Active", "Never", 0,
	PANEL_MENU_CHOICE_STRINGS, "Always", "Active (when in scrollbar)",
	"Never", 0,
	PANEL_NOTIFY_PROC, bubble_display_proc,
	0);

    bar_display_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "            Show Bar",
	PANEL_CHOICE_STRINGS, "Always", "Active", "Never", 0,
	PANEL_MENU_CHOICE_STRINGS, "Always",
	"Active (when in scrollbar)",
	"Never", 0,
	PANEL_NOTIFY_PROC, bar_display_proc,
	0);

    bubble_color_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "        Bubble Color",
	PANEL_CHOICE_STRINGS, "Grey", "Black", 0,
	PANEL_NOTIFY_PROC, bubble_color_proc,
	0);

    bar_color_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "           Bar Color",
	PANEL_CHOICE_STRINGS, "Grey", "White", 0,
	PANEL_NOTIFY_PROC, bar_color_proc,
	0);

    bubble_margin_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "       Bubble Margin",
	PANEL_CHOICE_STRINGS, "0", "1", "2", "3", "4", "5", "6", "7", "8", 0,
	PANEL_NOTIFY_PROC, bubble_margin_proc,
	0);

    bar_width_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "           Bar Width",
	PANEL_CHOICE_STRINGS,
	 /* MIN_THICKNESS */ "2", "4", "6", "8", "10", "12", "14",
	"16", "18", "20", "22", "24", 0,
	PANEL_NOTIFY_PROC, bar_width_proc,
	0);

    end_point_area_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "      End point area",
	PANEL_CHOICE_STRINGS, "0", "2", "4", "6", "8", "10", "12", "14",
	"16", "18", "20", "22", "24", 0,
	PANEL_NOTIFY_PROC, end_point_area_proc,
	0);

    repeat_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "Repeat time (tenths)",
	PANEL_CHOICE_STRINGS, "0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", "10", 0,
	PANEL_NOTIFY_PROC, repeat_proc,
	0);

    border_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "              Border",
	PANEL_CHOICE_STRINGS, "Yes", "No", 0,
	PANEL_NOTIFY_PROC, border_proc,
	0);

    have_buttons_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "        Page Buttons",
	PANEL_CHOICE_STRINGS, "Yes", "No", 0,
	PANEL_NOTIFY_PROC, have_buttons_proc,
	0);

    button_length_item = panel_create_item(control, PANEL_CYCLE,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_STRING, "  Page Button Length",
	PANEL_CHOICE_STRINGS, "0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18",
	"19", "20", "21", "22", "23", "24", 0,
	PANEL_NOTIFY_PROC, button_length_proc,
	0);

    row++;
    
    (void)panel_create_item(control, PANEL_BUTTON,
	PANEL_LABEL_X, ATTR_COL(0), PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_LABEL_IMAGE, panel_button_image(control, "Save", 7, (Pixfont *)NULL),
	PANEL_NOTIFY_PROC, save_proc,
	0);

    (void)panel_create_item(control, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(control, "Reset", 7,  (Pixfont *)NULL),
	PANEL_NOTIFY_PROC, default_proc,
	0);

    (void)panel_create_item(control, PANEL_BUTTON,
	PANEL_LABEL_IMAGE, panel_button_image(control, "Quit", 7,  (Pixfont *)NULL),
	PANEL_NOTIFY_PROC, quit_proc,
	0);

    row++;
    
    have_v_sb_item = panel_create_item(control, PANEL_TOGGLE,
	PANEL_LABEL_X, 20, PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_FEEDBACK, PANEL_MARKED,
	PANEL_CHOICE_STRINGS, "Vertical Scrollbar", 0,
	PANEL_VALUE, 1,
	PANEL_NOTIFY_PROC, have_v_sb_proc,
	0);

    have_h_sb_item = panel_create_item(control, PANEL_TOGGLE,
	PANEL_LABEL_X, 20, PANEL_LABEL_Y, ATTR_ROW(row++),
	PANEL_FEEDBACK, PANEL_MARKED,
	PANEL_CHOICE_STRINGS, "Horizontal Scrollbar", 0,
	PANEL_VALUE, 1,
	PANEL_NOTIFY_PROC, have_h_sb_proc,
	0);

    if (debugging)
	(void)panel_create_item(control, PANEL_TOGGLE,
	    PANEL_LABEL_X, 20, PANEL_LABEL_Y, ATTR_ROW(row++),
	    PANEL_FEEDBACK, PANEL_MARKED,
	    PANEL_CHOICE_STRINGS, "Use Normalization", 0,
	    PANEL_VALUE, 1,
	    PANEL_NOTIFY_PROC, use_normalize_proc,
	    0);

    (void)window_fit(control);
    set_panel_from_scrollbars();
}

static void
set_panel_from_scrollbars()
{
    Scrollbar_setting setting;
    int             val;

    /* gravity */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_PLACEMENT);
    val = setting == SCROLL_MIN ? 0 : 1;
    (void)panel_set(h_gravity_item, PANEL_VALUE, val, 0);

    setting = (Scrollbar_setting)
    	scrollbar_get(vertical_sb, SCROLL_PLACEMENT);
    val = setting == SCROLL_MIN ? 0 : 1;
    (void)panel_set(v_gravity_item, PANEL_VALUE, val, 0);

    /* bar display level */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_BAR_DISPLAY_LEVEL);
    switch (setting) {
    case SCROLL_ALWAYS:
	val = 0;
	break;
    case SCROLL_ACTIVE:
	val = 1;
	break;
    case SCROLL_NEVER:
	val = 2;
	break;
    }
    (void)panel_set(bar_display_item, PANEL_VALUE, val, 0);

    /* bubble display level */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_BUBBLE_DISPLAY_LEVEL);
    switch (setting) {
    case SCROLL_ALWAYS:
	val = 0;
	break;
    case SCROLL_ACTIVE:
	val = 1;
	break;
    case SCROLL_NEVER:
	val = 2;
	break;
    }
    (void)panel_set(bubble_display_item, PANEL_VALUE, val, 0);

    /* border */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_BORDER);
    val = val ? 0 : 1;
    (void)panel_set(border_item, PANEL_VALUE, val, 0);

    /* bar thickness */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_THICKNESS);
    val = (val - MIN_THICKNESS) / 2;
    (void)panel_set(bar_width_item, PANEL_VALUE, val, 0);

    /* end point area */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_END_POINT_AREA);
    val = val / 2;
    (void)panel_set(end_point_area_item, PANEL_VALUE, val, 0);

    /* repeat */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_REPEAT_TIME);
    (void)panel_set(repeat_item, PANEL_VALUE, val, 0);

    /* page buttons */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_PAGE_BUTTONS);
    val = val ? 0 : 1;
    (void)panel_set(have_buttons_item, PANEL_VALUE, val, 0);

    /* page button length */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_PAGE_BUTTON_LENGTH);
    (void)panel_set(button_length_item, PANEL_VALUE, val, 0);

    /* bubble margin */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_BUBBLE_MARGIN);
    (void)panel_set(bubble_margin_item, PANEL_VALUE, val, 0);

    /* bar color */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_BAR_COLOR);
    val = setting == SCROLL_GREY ? 0 : 1;
    (void)panel_set(bar_color_item, PANEL_VALUE, val, 0);

    /* bubble color */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_BUBBLE_COLOR);
    val = setting == SCROLL_GREY ? 0 : 1;
    (void)panel_set(bubble_color_item, PANEL_VALUE, val, 0);

}

static void
init_panel()
{
    int             row, col, x, y;
    char           *s;
    struct pixrect *image;
    static char    *file_names[16][4] = {
	{"/cursors", "tek.c", "ttyansi.c", "usr_profile.c"},
	{"cursor_init.c", "teksw.c", "/tty_backup", "usr_profile_db.c"},
	{"demo_cursor.c", "teksw_ui.c", "ttyselect.c", "usr_profile_hs.h"},
	{"/demo", "tool_bdry.c", "ttysw_get.c", "usr_profile_panel.c"},
	{"emptysw.c", "tool_begin.c", "ttysw_gtty.c", "usr_profile_util.c"},
	{"expand_name.c", "tool_create.c", "ttysw_help.c", "/window"},
	{"fullscreen.c", "tool_destroy.c", "ttysw_main.c", "wmgr_cursor.c"},
	{"gfxsw.c", "tool_display.c", "ttysw_mapkey.c", "wmgr_findspace.c"},
	{"gfxsw_input.c", "tool_help.c", "ttysw_notify.c", "wmgr_help.c"},
	{"/icons", "tool_input.c", "ttysw_old.c", "wmgr_menu.c"},
	{"menu.c", "tool_install.c", "tty_set.c", "wmgr_policy.c"},
	{"menu_util.c", "tool_layout.c", "tty_stty.c", "wmgr_rect.c"},
	{"/optionsw", "tool_position.c", "ttysw_tio.c", "wmgr_state.c"},
	{"/panel", "tool_select.c", "ttysw_tsw.c", "wmgr_usage.c"},
	{"scrollbar.c", "tool_usage.c", "ttysw_usage.c", "wmgr_util.c"},
	{"scrollbar_util.c", "tool_yak.c", "ttysw_util.c", "wmgr_valid.c"}
    };

    panel = window_create(base_frame, PANEL,
	WIN_WIDTH,		(texttoo ? 2 * 120 : ATTR_COLS(47)),
	PANEL_SHOW_MENU,	FALSE,
	WIN_ERROR_MSG,		"panel creation error!",
	WIN_VERTICAL_SCROLLBAR, vertical_sb,
	WIN_HORIZONTAL_SCROLLBAR, horizontal_sb,
	0);
    if (panel == NULL) { 
        (void)fprintf(stderr,"defaultstool: unable to create panel\n"); 
        exit(1); 
    }

    for (col = 0, x = my_margin; col <= 3; col++) {

	for (row = 0, y = my_margin; row <= 15; row++) {

	    s = file_names[row][col];

	    image = (s[0] == '/') ? &scrd_folder_image : &scrd_paper_image;
	    (void)panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_X, x, PANEL_LABEL_Y, y,
		PANEL_LABEL_IMAGE, image,
		0);
	    y += 55;
	    (void)panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_X, x, PANEL_LABEL_Y, y,
		PANEL_LABEL_STRING, s,
		0);
	    y -= 55;
	    y += my_row_height;
	}
	x += my_col_width;
    }
}

/**************************************************************************/
/* notify procs                                                           */
/**************************************************************************/

/* ARGSUSED */
static void
have_v_sb_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    if (val) {
	if (!(Scrollbar) panel_get(panel, PANEL_VERTICAL_SCROLLBAR)) {
	    (void)panel_set(panel, PANEL_VERTICAL_SCROLLBAR, vertical_sb, 0);
	    (void)panel_paint(panel, PANEL_CLEAR);
	    return;
	}
    } else {
	if ((Scrollbar) panel_get(panel, PANEL_VERTICAL_SCROLLBAR)) {
	    (void)panel_set(panel, PANEL_VERTICAL_SCROLLBAR, 0, 0);
	    (void)panel_paint(panel, PANEL_CLEAR);
	    return;
	}
    }
}

/* ARGSUSED */
static void
have_h_sb_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    if (val) {
	if (!(Scrollbar) panel_get(panel, PANEL_HORIZONTAL_SCROLLBAR)) {
	    (void)panel_set(panel, PANEL_HORIZONTAL_SCROLLBAR, horizontal_sb, 0);
	    (void)panel_paint(panel, PANEL_CLEAR);
	    return;
	}
    } else {
	if ((Scrollbar) panel_get(panel, PANEL_HORIZONTAL_SCROLLBAR)) {
	    (void)panel_set(panel, PANEL_HORIZONTAL_SCROLLBAR, 0, 0);
	    (void)panel_paint(panel, PANEL_CLEAR);
	    return;
	}
    }
}

/* ARGSUSED */
static void
use_normalize_proc(ip, normalize, event)
    caddr_t         ip;
    int             normalize;
    Event          *event;
{
    int             shift = event_shift_is_down(event);
    int             grid_normalize = (normalize && shift);

    normalize = (normalize && !shift);

    (void)scrollbar_set(horizontal_sb,
	SCROLL_NORMALIZE, normalize,
	SCROLL_TO_GRID, grid_normalize,
	SCROLL_MARGIN, (normalize) ? my_gap : my_margin,
	SCROLL_LINE_HEIGHT, (shift) ? my_col_width : 0,
	SCROLL_GAP, my_gap,
	0);
    (void)scrollbar_set(vertical_sb,
	SCROLL_NORMALIZE, normalize,
	SCROLL_TO_GRID, grid_normalize,
	SCROLL_MARGIN, (normalize) ? my_gap : my_margin,
	SCROLL_LINE_HEIGHT, (shift) ? my_row_height : 0,
	SCROLL_GAP, my_gap,
	0);
}

/* ARGSUSED */
static void
h_gravity_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    register Scrollbar_setting gravity_val;

    gravity_val = val ? SCROLL_SOUTH : SCROLL_NORTH;
    (void)scrollbar_set(horizontal_sb, SCROLL_PLACEMENT, gravity_val, 0);

    /* must detach/attach scrollbar to get the corner handled properly */
    if (panel_get_value(have_h_sb_item)) {
	(void)panel_set(panel, PANEL_HORIZONTAL_SCROLLBAR, 0, 0);
	(void)panel_set(panel, PANEL_HORIZONTAL_SCROLLBAR, horizontal_sb, 0);
    }
    (void)panel_paint(panel, PANEL_CLEAR);
}

/* ARGSUSED */
static void
v_gravity_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    register Scrollbar_setting gravity_val;

    gravity_val = val ? SCROLL_EAST : SCROLL_WEST;
    (void)scrollbar_set(vertical_sb, SCROLL_PLACEMENT, gravity_val, 0);

    /* must detach/attach scrollbar to get the corner handled properly */
    if (panel_get_value(have_v_sb_item)) {
	(void)panel_set(panel, PANEL_VERTICAL_SCROLLBAR, 0, 0);
	(void)panel_set(panel, PANEL_VERTICAL_SCROLLBAR, vertical_sb, 0);
    }
    (void)panel_paint(panel, PANEL_CLEAR);
}

/* ARGSUSED */
static void
bar_display_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    Scrollbar_setting new_val;

    switch (val) {
    case 0:
	new_val = SCROLL_ALWAYS;
	break;
    case 1:
	new_val = SCROLL_ACTIVE;
	break;
    case 2:
	new_val = SCROLL_NEVER;
	break;
    }
    (void)scrollbar_set(vertical_sb, SCROLL_BAR_DISPLAY_LEVEL, new_val, 0);
    (void)scrollbar_paint(vertical_sb);
    (void)scrollbar_set(horizontal_sb, SCROLL_BAR_DISPLAY_LEVEL, new_val, 0);
    (void)scrollbar_paint(horizontal_sb);
}

/* ARGSUSED */
static void
bubble_display_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    Scrollbar_setting new_val;

    switch (val) {
    case 0:
	new_val = SCROLL_ALWAYS;
	break;
    case 1:
	new_val = SCROLL_ACTIVE;
	break;
    case 2:
	new_val = SCROLL_NEVER;
	break;
    }
    (void)scrollbar_set(vertical_sb, SCROLL_BUBBLE_DISPLAY_LEVEL, new_val, 0);
    (void)scrollbar_paint(vertical_sb);
    (void)scrollbar_set(horizontal_sb, SCROLL_BUBBLE_DISPLAY_LEVEL, new_val, 0);
    (void)scrollbar_paint(horizontal_sb);
}

/* ARGSUSED */
static void
have_buttons_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    if (val)
	val = FALSE;
    else
	val = TRUE;
    (void)scrollbar_set(vertical_sb, SCROLL_PAGE_BUTTONS, val, 0);
    (void)scrollbar_set(horizontal_sb, SCROLL_PAGE_BUTTONS, val, 0);
    if (val) {
	if (!panel_get(button_length_item, PANEL_VALUE)) {
	    (void)panel_set(button_length_item, PANEL_VALUE, 15, 0);
	    (void)scrollbar_set(vertical_sb, SCROLL_PAGE_BUTTON_LENGTH, 15, 0);
	    (void)scrollbar_set(horizontal_sb, SCROLL_PAGE_BUTTON_LENGTH, 15, 0);
	}
    } else {
	if (panel_get(button_length_item, PANEL_VALUE)) {
	    (void)panel_set(button_length_item, PANEL_VALUE, 0, 0);
	    (void)scrollbar_set(vertical_sb, SCROLL_PAGE_BUTTON_LENGTH, 0, 0);
	    (void)scrollbar_set(horizontal_sb, SCROLL_PAGE_BUTTON_LENGTH, 0, 0);
	}
    }
    (void)scrollbar_paint(vertical_sb);
    (void)scrollbar_paint(horizontal_sb);
}

/* ARGSUSED */
static void
bar_width_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    val = (val * 2) + MIN_THICKNESS;
    (void)scrollbar_set(vertical_sb, SCROLL_THICKNESS, val, 0);
    (void)scrollbar_set(horizontal_sb, SCROLL_THICKNESS, val, 0);
    (void)panel_paint(panel, PANEL_CLEAR);
}

/* ARGSUSED */
static void
end_point_area_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    val = (val * 2);
    (void)scrollbar_set(vertical_sb,   SCROLL_END_POINT_AREA, val, 0);
    (void)scrollbar_set(horizontal_sb, SCROLL_END_POINT_AREA, val, 0);
}

/* ARGSUSED */
static void
repeat_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    (void)scrollbar_set(vertical_sb,   SCROLL_REPEAT_TIME, val, 0);
    (void)scrollbar_set(horizontal_sb, SCROLL_REPEAT_TIME, val, 0);
}

/* ARGSUSED */
static void
button_length_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    (void)scrollbar_set(vertical_sb, SCROLL_PAGE_BUTTON_LENGTH, val, 0);
    (void)scrollbar_set(horizontal_sb, SCROLL_PAGE_BUTTON_LENGTH, val, 0);
    if (val) {
	if (panel_get(have_buttons_item, PANEL_VALUE)) {
	    (void)panel_set(have_buttons_item, PANEL_VALUE, 0, 0);
	    (void)scrollbar_set(vertical_sb, SCROLL_PAGE_BUTTONS, TRUE, 0);
	    (void)scrollbar_set(horizontal_sb, SCROLL_PAGE_BUTTONS, TRUE, 0);
	}
    } else {
	if (!panel_get(have_buttons_item, PANEL_VALUE)) {
	    (void)panel_set(have_buttons_item, PANEL_VALUE, 1, 0);
	    (void)scrollbar_set(vertical_sb, SCROLL_PAGE_BUTTONS, FALSE, 0);
	    (void)scrollbar_set(horizontal_sb, SCROLL_PAGE_BUTTONS, FALSE, 0);
	}
    }
    (void)scrollbar_paint(vertical_sb);
    (void)scrollbar_paint(horizontal_sb);
}

/* ARGSUSED */
static void
bubble_margin_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    (void)scrollbar_set(vertical_sb, SCROLL_BUBBLE_MARGIN, val, 0);
    (void)scrollbar_paint(vertical_sb);
    (void)scrollbar_set(horizontal_sb, SCROLL_BUBBLE_MARGIN, val, 0);
    (void)scrollbar_paint(horizontal_sb);
}

/* ARGSUSED */
static void
bubble_color_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    Scrollbar_setting new_val;

    if (val == 0)
	new_val = SCROLL_GREY;
    else
	new_val = SCROLL_BLACK;

    (void)scrollbar_set(vertical_sb, SCROLL_BUBBLE_COLOR, new_val, 0);
    (void)scrollbar_paint(vertical_sb);
    (void)scrollbar_set(horizontal_sb, SCROLL_BUBBLE_COLOR, new_val, 0);
    (void)scrollbar_paint(horizontal_sb);
}

/* ARGSUSED */
static void
bar_color_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    Scrollbar_setting new_val;

    if (val == 0)
	new_val = SCROLL_GREY;
    else
	new_val = SCROLL_WHITE;

    (void)scrollbar_set(vertical_sb, SCROLL_BAR_COLOR, new_val, 0);
    (void)scrollbar_paint(vertical_sb);
    (void)scrollbar_set(horizontal_sb, SCROLL_BAR_COLOR, new_val, 0);
    (void)scrollbar_paint(horizontal_sb);
}

/* ARGSUSED */
static void
border_proc(ip, val)
    caddr_t         ip;
    int             val;
{
    if (val)
	val = FALSE;
    else
	val = TRUE;

    (void)scrollbar_set(vertical_sb, SCROLL_BORDER, val, 0);
    (void)scrollbar_set(horizontal_sb, SCROLL_BORDER, val, 0);
    (void)scrollbar_paint(vertical_sb);
    (void)scrollbar_paint(horizontal_sb);
}

/* ARGSUSED */
static void
save_proc(ip)
    caddr_t         ip;
{
    Scrollbar_setting setting;
    int             val;

    /* gravity */
    setting = (Scrollbar_setting) scrollbar_get(horizontal_sb, SCROLL_PLACEMENT);
    if (setting == SCROLL_MIN)
	defaults_set_string("/Scrollbar/Horizontal_bar_placement", "North", (int *)NULL);
    else
	defaults_set_string("/Scrollbar/Horizontal_bar_placement", "South", (int *)NULL);
    setting = (Scrollbar_setting) scrollbar_get(vertical_sb, SCROLL_PLACEMENT);
    if (setting == SCROLL_MIN)
	defaults_set_string("/Scrollbar/Vertical_bar_placement", "West", (int *)NULL);
    else
	defaults_set_string("/Scrollbar/Vertical_bar_placement", "East", (int *)NULL);

    /* bar display level */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_BAR_DISPLAY_LEVEL);
    switch (setting) {
    case SCROLL_ALWAYS:
	defaults_set_string("/Scrollbar/Bar_display_level", "Always", (int *)NULL);
	break;
    case SCROLL_ACTIVE:
	defaults_set_string("/Scrollbar/Bar_display_level", "Active", (int *)NULL);
	break;
    case SCROLL_NEVER:
	defaults_set_string("/Scrollbar/Bar_display_level", "Never", (int *)NULL);
	break;
    }

    /* bubble display level */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_BUBBLE_DISPLAY_LEVEL);
    switch (setting) {
    case SCROLL_ALWAYS:
	defaults_set_string("/Scrollbar/Bubble_display_level", "Always", (int *)NULL);
	break;
    case SCROLL_ACTIVE:
	defaults_set_string("/Scrollbar/Bubble_display_level", "Active", (int *)NULL);
	break;
    case SCROLL_NEVER:
	defaults_set_string("/Scrollbar/Bubble_display_level", "Never", (int *)NULL);
	break;
    }

    /* border */
    if (scrollbar_get(horizontal_sb, SCROLL_BORDER))
	defaults_set_string("/Scrollbar/Border", "True", (int *)NULL);
    else
	defaults_set_string("/Scrollbar/Border", "False", (int *)NULL);

    /* bar thickness */
    defaults_set_integer("/Scrollbar/Thickness",
	(int) scrollbar_get(horizontal_sb, SCROLL_THICKNESS), (int *)NULL);

    /* end point area */
    defaults_set_integer("/Scrollbar/End_point_area",
	(int) scrollbar_get(horizontal_sb, SCROLL_END_POINT_AREA), (int *)NULL);

    /* repeat */
    defaults_set_integer("/Scrollbar/Repeat_time",
	(int) scrollbar_get(horizontal_sb, SCROLL_REPEAT_TIME), (int *)NULL);

    /* page buttons */
    if (scrollbar_get(horizontal_sb, SCROLL_PAGE_BUTTONS))
	defaults_set_string("/Scrollbar/Page_buttons", "True", (int *)NULL);
    else
	defaults_set_string("/Scrollbar/Page_buttons", "False", (int *)NULL);

    /* page button length */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_PAGE_BUTTON_LENGTH);
    defaults_set_integer("/Scrollbar/Page_button_length", val, (int *)NULL);

    /* bubble margin */
    val = (int) scrollbar_get(horizontal_sb, SCROLL_BUBBLE_MARGIN);
    defaults_set_integer("/Scrollbar/Bubble_margin", val, (int *)NULL);

    /* bar color */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_BAR_COLOR);
    if (setting == SCROLL_GREY)
	defaults_set_string("/Scrollbar/Bar_color", "Grey", (int *)NULL);
    else
	defaults_set_string("/Scrollbar/Bar_color", "White", (int *)NULL);

    /* bubble color */
    setting = (Scrollbar_setting)
	scrollbar_get(horizontal_sb, SCROLL_BUBBLE_COLOR);
    if (setting == SCROLL_GREY)
	defaults_set_string("/Scrollbar/Bubble_color", "Grey", (int *)NULL);
    else
	defaults_set_string("/Scrollbar/Bubble_color", "Black", (int *)NULL);

    defaults_write_changed((char *)NULL, (int *)NULL);

    if (texttoo)
	init_text(FALSE);

}

/* ARGSUSED */
static void
default_proc(ip)
    caddr_t         ip;
{
    (void)scrollbar_set(vertical_sb,
	SCROLL_PLACEMENT,		VERT_PLACEMENT_DEFAULT,
	SCROLL_BAR_DISPLAY_LEVEL,	BAR_DISPLAY_DEFAULT,
	SCROLL_THICKNESS,		BAR_THICKNESS_DEFAULT,
	SCROLL_BAR_COLOR,		BAR_COLOR_DEFAULT,
	SCROLL_BUBBLE_DISPLAY_LEVEL,	BUBBLE_DISPLAY_DEFAULT,
	SCROLL_BUBBLE_MARGIN,		BUBBLE_MARGIN_DEFAULT,
	SCROLL_BUBBLE_COLOR,		BUBBLE_COLOR_DEFAULT,
	SCROLL_PAGE_BUTTONS,		BUTTON_DISPLAY_DEFAULT,
	SCROLL_PAGE_BUTTON_LENGTH,	BUTTON_LENGTH_DEFAULT,
	SCROLL_BORDER,			BORDER_DEFAULT,
	SCROLL_END_POINT_AREA,		END_POINT_AREA_DEFAULT,
	SCROLL_REPEAT_TIME,		REPEAT_TIME_DEFAULT,
	0);

    (void)scrollbar_set(horizontal_sb,
	SCROLL_PLACEMENT,		HORIZ_PLACEMENT_DEFAULT,
	SCROLL_BAR_DISPLAY_LEVEL,	BAR_DISPLAY_DEFAULT,
	SCROLL_THICKNESS,		BAR_THICKNESS_DEFAULT,
	SCROLL_BAR_COLOR,		BAR_COLOR_DEFAULT,
	SCROLL_BUBBLE_DISPLAY_LEVEL,	BUBBLE_DISPLAY_DEFAULT,
	SCROLL_BUBBLE_MARGIN,		BUBBLE_MARGIN_DEFAULT,
	SCROLL_BUBBLE_COLOR,		BUBBLE_COLOR_DEFAULT,
	SCROLL_PAGE_BUTTONS,		BUTTON_DISPLAY_DEFAULT,
	SCROLL_PAGE_BUTTON_LENGTH,	BUTTON_LENGTH_DEFAULT,
	SCROLL_BORDER,			BORDER_DEFAULT,
	SCROLL_END_POINT_AREA,		END_POINT_AREA_DEFAULT,
	SCROLL_REPEAT_TIME,		REPEAT_TIME_DEFAULT,
	0);


    (void)panel_set(panel,
	PANEL_VERTICAL_SCROLLBAR,   0,
	PANEL_HORIZONTAL_SCROLLBAR, 0,
	0);
    set_panel_from_scrollbars();
    (void)panel_set(panel,
	PANEL_VERTICAL_SCROLLBAR,   vertical_sb,
	PANEL_HORIZONTAL_SCROLLBAR, horizontal_sb,
	0);
    (void)panel_paint(panel, PANEL_CLEAR);
}

/* ARGSUSED */
static void
quit_proc(panel_local, ip)
    caddr_t         panel_local, ip;
{
    (void)window_done(base_frame);
}
