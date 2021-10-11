#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)defaultsedit.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *      Copyright (c) 1985 by Sun Microsystems Inc.
 */

/* File:  defaultsedit.c */


/* Master default directory:  /pe/usr/lib/defaults  */
/* Source code in /pe/usr/src/sun/usr.bin/suntool/defaults  */


#include <stdio.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vfork.h>
#include <sunwindow/sun.h>
#include <sunwindow/defaults.h>
#include <sunwindow/notify.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/tool_hs.h>
#include <suntool/panel.h>
#include <suntool/scrollbar.h>
#include <suntool/textsw.h>
#include <suntool/walkmenu.h>

static short defaults_ic_image[256]={
#include <images/defaultsedit.icon>
};

DEFINE_ICON_FROM_IMAGE(defaults_icon, defaults_ic_image);

static char	demo_msg[] =
"Press the left mouse button to confirm starting  \
the Demo/Setup program. (If you do confirm, all changes \
done to default values will be saved before starting \
the Demo program). \
To cancel, press the right mouse button now.";

static short    hg_data[] = {
#include <images/hglass.cursor>
};
mpr_static(defed_hourglass_cursor_pr, 16, 16, 1, hg_data);
struct cursor    defaultsedit_hourglass_cursor = 
	{ 8, 8, PIX_SRC | PIX_DST, &defed_hourglass_cursor_pr };

/* New defines: */

#define	DE_TEXT_HEIGHT TOOL_SWEXTENDTOEDGE
#define DE_DEFAULTS_COLUMNS 90	    /* The defaults programs width */
#define DE_STANDARD_DISPLAYLEN 25	    /* Show at most 25 initial chars of
					standard value */

#define DE_MIN_VALUE_STOREDLEN 100	    /* Have minimum 100 chars string initial
				length	allocated in panel text items */

#define DE_NAME_ITEM_MAX_LENGTH 1500    /* Max initiallength of string values */

#define DE_NAME_ITEM_DISPLAY_LENGTH 1500 /* Max displayed length of string values */
#define DE_MAX_CHOICE_LENGTH 100	    /* Max length of choice in a choice item */
#define DE_HELP_TEXT_ALLOC_LENGTH 191   /* Max help text length */
				    /* Max str length of 80 takes 81 bytes  */

#define DE_FULLNAME_MAX_LENGTH 90    /* Max fullname defaults node name length */
#define DE_ITEM_MAX_LABEL_LENGTH   DE_NAME_ITEM_MAX_LENGTH+DE_FULLNAME_MAX_LENGTH
#define DE_MAX_SCREENS		100
     /* Max sum of length of standard defaults */
   /* node values and full node names,  used to create labels of panel items*/

#define	DE_ITEMLABELPOS 40	     /* minimum end position of labels in tree panel */
	

/* New data types and decls: */

typedef enum {
	Undefined,	/* May be used to mark special objects */
	String,		/* Text node */
	Enumerate,	/* Enumeration node */
	Editstring,	/* Text node whose label has been edited by user */
	Editenumerate, 	/* Enumeration node whose label has been edited */
	Helpstring,	/* Marks the help string object in the help panel */
	Message,	/* Message object for header text to be displayed */
} Defaulttype;


typedef struct _defobj* Defobj;
struct _defobj{
	Defaulttype	nodetype;   /* String, enumeration or Labeledit node */
	char*		nodename;   /* Full defaults name of node */
	Panel_item	ilabel;	    /* tree panel item for Label */
	Panel_item	ivalue;	    /* tree panel item containing value */
	Panel_item	ifill1;	    /* fill text  - type Enumerate */
	Defobj		nodenext;
	Bool		specifichelp;	/* This field is set to true for
				an enumeration defobject if the last
				displayed help was enumeration value
				specific. */
};


typedef struct rect*	Recttype;

static struct tool *tool;	/* Tool to use */
static Panel menupanel;		/* Panel containing choice items */
static Panel treepanel;		/* Panel of nodenames */
static Panel helppanel;
static Textsw textwindow;	/* Text subwindow for editing */
static Defobj last_clicked_defobj; /* Used for indicating last object
					    clicked with left mouse button */
static	Defobj	firstdefobj;	/* NULL or ptr to first tree panel default
				 object */
static	Defobj	lastinserteddefobj;	/* NULL or ptr to last tree panel
				  default object */


static char	*prog_names[DE_MAX_SCREENS];
static int	cur_progvalue;		/* Item for currently selected program */
static Panel_item helpdisplayitem;	/* Item for displaying help text in help window */
static int	cur_ylinenr;	/* current line nr in treepanel. 1st line is nr 0 */

static char nohelpavail[] = "No help available";
static char **tool_attrs = NULL;    	/* May point to vector of tool
					   attributes */
static char *tool_name;





/* Overview:
<  The basic datastructure is a linked list of Defobj structs. Defobj
  is short for defaults object, and can be displayed in the treepanel.
  New defaults objects can be either inserted of deleted from this list.
  A defobj is either a text object, to which you can type in textual values
  or an enumeration, which is displayed as a panel choice item.
     Often several panel-items are required to implement a single Defobj.
  Recently created defaults objects (created by the insert button)
  have their nodename field either NULL or "".  In this way they can
  be distinguished from defaults objects which are created from and 
  corresponds to nodes in the defaults database ( a tree-structured database)
*/
  



/* 
 * specformatnames is a list of namerecords of accessed subtrees in the
 * defaults database which have their own special file format,
 * like mail having a file .mailrc  etc.
 * If a subtree in the defaults database has a node called
 *  $Specialformat_to_defaults,
 * then the value of this node is the
 * name of a conversion program which will be invoked before reading in
 * the defaults database subtree.
 * In the mail case, this conversion program would convert the appropriate
 * part of .mailrc and write it out into an appropriate portion of the
 * .defaults file.  The name of this subtree would be saved in a namerec
 * record in the NULL-terminated list specformatnames.
 * If defaultsedit fails to invoke the conversion program an error message
 * will be printed.
 *
 * After saving the defaults database, defaultsedit will try to invoke
 * another set of conversion programs to convert from the .defaults file
 * format to specially formated files like .mailrc
 * Such a conversion program will be invoked for each subtree which name
 * is on the namerec in the list specformatnames and the changed field
 * of the namerec is True,  indicating that the user has made edits
 * that changed some defaults database values
 * For each subtree, the name of the conversion program will be found
 * in the node:
 *  $Defaults_to_specialformat
 * defaultsedit will print an error message if it fails invoking the 
 * conversion program. 
 */

#ifdef DEBUG
#define Debug(x) x
#else
#define Debug(x)
#endif

typedef struct {
    char*	name;		/*  The name of a program with conversion */
    Bool	changed;	/*  True if user edited defaults values  */
} namerec;

static namerec*  specformatnames[200];

		

/*  New function forward decls: */

static void sigwinched();
static void deferror();
static void init_items();
static char* get_cur_progname();

static int menu_swinit();
static int prog_swinit();
static int tree_swinit();
static int help_swinit();
static int text_swinit();

static void prog_notifyproc();
static void menu_save_notifyproc();
static void menu_quit_notifyproc();
static void menu_reset_notifyproc();
static void menu_labeledit_notifyproc();
static void menu_insert_notifyproc();
static void menu_delete_notifyproc();
static void edit_menuproc();

static Bool nothing_dirty();
static void tree_runsetup_notifyproc(); 	/* (item, value, event)  */
static void tree_text_eventproc();
static void tree_choice_notifyproc();
static void help_text_eventproc();		/* (item, event) */
static void subtree_show();
static void subtree_showitems();
static void subtree_clearitems();
static Bool subtree_extractinfo();		/* ret. T if any item changed */
static char* get_enum_member_string();
static void showitem();
static void showhelpitem();
static int get_enum_member();
static Notify_value demo_child_wait3();
static void show_message();		/* (nodename, labelname, valuestr) */
static void run_setup_prog();
static char* get_standard_valuestr(); 	/* (nodename, standardval) */
static Defobj create_defobj();
static Defobj create_message_defobj();
static Defobj create_text_defobj();
static Defobj create_enum_defobj();
static void freemem_defobj();
static Bool check_clicked_defobj();
static char* getnodename_defobj(); 	/* (defobjptr) */
static Bool deflt_helpdefined();	/* (full_name) */
static Bool deflt_always_save();	/* (full_name) */
static void deflt_get_help();		/* (full_name, defaulthelp, helpstr) */
static void deflt_get_fullchild();	/* */
static void deflt_get_fullsibling();	/* */
static char* getchars();		/* (nchars) */
static Defaulttype get_nodetype();	/* (fullname) */
static char* get_memory();		/* (size) */
static void bomb();			/* */
static Bool displayable_node();	/* (nodename) */
static Bool specialformat_to_defaults(); /* (subtreename) */
static void dump_default_trees();
static int start_child_proc();		/* (childforkname, waitflg) */
static void wait_child_finish();	/* (childpid) */
static int member_pos();		/* (name, namelist) */
static void insert_new_member();	/* (name, namelist) */
static Bool start_possible_demos();	/* (nargs, namevec) */
static void defaults_str_unparse();	/* (instring, outstring) */
static char* convert_string();
static int convert_backslash_chr();	/* (instrptr, outstrptr) */
static void tree_scrollup();
static void subtree_copy();



/*
 * Generate a fault in order to get a core dump.
 * Use division by zero to generate a fault
 */
static void
stop()
{
	int i;

	i = 0;
	i = 1/i;
}



/* New function decls and bodies: */

void
#ifdef STANDALONE
main(argc, argv)
#else
defaultsedit_main(argc, argv)
#endif

 int argc;
 char* argv[];
{
	int	toolheight;
	int	menuheight;
	int	helpheight;
	int	textheight;
	char	*tool_get_attribute();

	defaults_special_mode();    /* Special mode to read master defaults
					database first to preserve order
					between nodes. */
	specformatnames[0] = (namerec *)NULL;
	argc--;
	tool_name = *argv++;

	/*
	 * Pick up command line arguments to modify tool behavior
	 */
	if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
	    /* Failed to parse it,  do nothing  */
	} else {
	    if (start_possible_demos(argc, argv))
		 return;
	}

	tool = tool_begin(
	WIN_LABEL,		"defaultsedit",
	WIN_ICON,		&defaults_icon,
	WIN_COLUMNS,		DE_DEFAULTS_COLUMNS, 
	WIN_BOUNDARY_MGR, 	True,
        WIN_ATTR_LIST,		tool_attrs,
	0);
	if (tool==NULL) deferror("Can't make tool", True);

	toolheight=(int) tool_get_attribute(tool, (int)WIN_HEIGHT);
	menuheight=menu_swinit();	/* Create menu panel subwindow */
	helpheight=help_swinit();	/* Create help text subwindow */
	textheight=helpheight*2;
	(void)tree_swinit(toolheight-menuheight-helpheight-textheight);
	    	/* Create tree display panel subwindow */
	(void)text_swinit(DE_TEXT_HEIGHT);

	(void)tool_install(tool);
	(void)notify_start();
	exit(0);
}



/*
 * Fork off special setup programs if defaults is invoked with an argument.
 * If defaults is invoked with an argument:    defaults  -DScrollbar
 * then the scrollbar setup will be started immediately from the defaults
 * tool, if the node /Scrollbar/$Setup_program has a startable program name
 * as its value.
 */
static Bool
start_possible_demos(nargs, namevec)
    int	    nargs;
    char*   namevec[];
{
    int	    nr;
    char    accessname[DE_FULLNAME_MAX_LENGTH];
    char    progname[DE_FULLNAME_MAX_LENGTH];
    char    message[150];
    Bool    startedflg;

    if (nargs<=0) return False;
    startedflg = False;


    for (nr=0; nr<nargs; nr++) {
	if(! (namevec[nr][0]=='-' && namevec[nr][1]=='D') ) continue;
	(void)strcpy(accessname, "/");
	(void)strcat(accessname, namevec[nr]+2);
	(void)strcat(accessname, "/$Setup_program");
	if ( defaults_exists(accessname, (int *)NULL) 
	     && !strequal(defaults_get_string(accessname, "", (int *)NULL), "") )
	    {
	    (void)strcpy(progname, defaults_get_string(accessname, "", (int *)NULL));
	    (void)start_child_proc(progname, False, True, (union wait *)NULL);
	    startedflg = True;
	}
	else {
	    (void)strcpy(message, "No specialized defaults program name exists for node with spelling: ");
    	    deferror(strcat(message, accessname), False);
	    continue;
	}
    }	
    return startedflg;
}



/*
 * Initialize panel items for menu panel subwindow.
 * This menu currently contains 3 buttons:
 * Save - will save the user-changed defaults in the .defaults file
 * Quit - will quit the program
 * Reset - will reset the current panel to its state when it was opened
 */
static int
menu_swinit()
{
	menupanel = panel_begin(tool, 0);
	if (menupanel == NULL) deferror("Can't create menu panel",  True);

	prog_swinit();

	(void)panel_create_item(menupanel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,
	    panel_button_image(menupanel,"Save",0,(Pixfont *)NULL),
	    PANEL_NOTIFY_PROC,  menu_save_notifyproc,
	    0);

	(void)panel_create_item(menupanel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,
	    panel_button_image(menupanel,"Quit",0,(Pixfont *)NULL),
	    PANEL_NOTIFY_PROC,  menu_quit_notifyproc,
	    0);

	(void)panel_create_item(menupanel, PANEL_BUTTON,
	    PANEL_LABEL_IMAGE,
	    panel_button_image(menupanel,"Reset",0,(Pixfont *)NULL),
	    PANEL_NOTIFY_PROC,  menu_reset_notifyproc,
	    0);

	(void)panel_create_item(menupanel, PANEL_MESSAGE,
	    PANEL_LABEL_IMAGE, 
	    panel_button_image(menupanel,"Edit Item",0,(Pixfont *)NULL),
	    PANEL_LABEL_X, ATTR_COL(70),	    
	    PANEL_EVENT_PROC, edit_menuproc,
	    0);

	(void)panel_set(menupanel, PANEL_HEIGHT, PANEL_FIT_ITEMS, 0);
	return (int)panel_get(menupanel, PANEL_HEIGHT);
}


/*
 * Event proc for the "Edit Item" item in the menu panel.
 * Presents a menu of edit commands when the user right-buttons down.
 */
/* ARGSUSED */
static void
edit_menuproc(item, event)
	Panel_item	item;
	struct inputevent *event;
{
	static Menu menu;
	
	/*
	 * create the menu the first time through 
	 */
	if (menu == NULL)	
	    menu = menu_create(
		MENU_ACTION_ITEM, "Copy Item", menu_insert_notifyproc,
		MENU_ACTION_ITEM, "Delete Item",menu_delete_notifyproc,
		MENU_ACTION_ITEM, "Edit Label",menu_labeledit_notifyproc,
		0);
	/*
	 * translate event to panel space and show the menu 
	 */
	if (event_action(event) == MS_RIGHT && event_is_down(event)) 
	    (void) menu_show_using_fd(menu, win_get_fd(menupanel),
				panel_window_event(menupanel, event));
}				


/*
 * Initialize panel items for program names panel subwindow,  progpanel
 * This panel contains all names of programs which have been specified
 * in the master defaults file
 * Also set initial value of cur_progitem to an appropriate value
 * "SunView" is selected if present, otherwise the first one.
 */
static int
prog_swinit()
{
	char		full_name[DE_FULLNAME_MAX_LENGTH];	/* Full name of node */
	int		i;
	char		*attrs[DE_MAX_SCREENS];
	
	cur_progvalue = NULL;
	deflt_get_fullchild("/", full_name);

	attrs[0] = (char *)PANEL_CHOICE_STRINGS;
	i = 1;
	while (full_name !=NULL && strlen(full_name) != 0) {
	    prog_names[i] = (caddr_t) strcpy(getchars(strlen(full_name)),
		full_name);
	    attrs[i] = &prog_names[i][1]; /* strip leading / */
	    if (strequal(full_name,"/SunView")) cur_progvalue = i;
	    deflt_get_fullsibling("/", full_name, full_name);
	    i++;
	}
	attrs[i++] = 0;
	attrs[i] = 0;

	(void)panel_create_item(menupanel, PANEL_CYCLE,
		  ATTR_LIST,		attrs,
		  PANEL_VALUE,		cur_progvalue ? cur_progvalue-1 : 0,
		  PANEL_LABEL_STRING, 	"Category", 
		  PANEL_NOTIFY_PROC,	 prog_notifyproc,
		      0);
}



/*
 * Initialize panel items for tree panel subwindow,  treepanel.
 * This subwindow displays part of the defaults database pertaining to
 * any selected program which have entry in the master defaults database.
 * The treepanel actually displays the text and choice items to which the
 * user inputs his parameters.
 * Also initialize the 2-line help window with a message
 */
static int
tree_swinit(height)
    int	    height;
{
	treepanel = panel_begin(tool,
	    PANEL_VERTICAL_SCROLLBAR,   
 	    scrollbar_build((caddr_t)0),
	    PANEL_HEIGHT, height, 
	    0);
	if (treepanel == NULL) deferror("Can't create tree panel", True);
	cur_ylinenr = 0;
	if (get_cur_progname()==NULL) {
	    show_message((char *)NULL,
              "No entries in master defaults database directory (/usr/lib/defaults)", "");
	} else {
	    subtree_show();
	    show_message((char *)NULL,
	    "Get HELP by clicking the left button on the appropriate label or string",
	    "" );
	}
	return (int)panel_get(treepanel, PANEL_HEIGHT);
}


/*
 * Initialize a text subwindow of height pixels
 */
static int
text_swinit(height)
    int	    height;
{
    textwindow=textsw_build(tool, 
	TEXTSW_HEIGHT, height, 
        0);
    return height;
}    



/*
 * Initilize panel help subwindow  helppanel
 * The help subwindow contains essentially one text item,  which labels and
 * values (= helpmessages or other messages)
 */
static int
help_swinit()
{
	char	dummyname[DE_FULLNAME_MAX_LENGTH];
	int	i;
	Defobj	defobjptr;


	helppanel = panel_begin(tool,
		PANEL_HEIGHT, PANEL_CU(2)+2,
	 	   0);
	if (helppanel == NULL) deferror("Can't create tree panel", True);

	/* Create a dummy node name of length DE_FULLNAME_MAX_LENGTH-1 */
	for(i=0;i<DE_FULLNAME_MAX_LENGTH-1;i++){
	    dummyname[i]=' ';
	}
	dummyname[DE_FULLNAME_MAX_LENGTH-1]='\0';

	defobjptr = create_defobj(dummyname, Helpstring);
	(void)strcpy(defobjptr->nodename, "");
	
	helpdisplayitem = panel_create_item(helppanel, PANEL_TEXT,
		PANEL_LABEL_STRING,	"Helplabel",
		PANEL_LABEL_BOLD,	True,
		PANEL_VALUE_STORED_LENGTH,	DE_HELP_TEXT_ALLOC_LENGTH-1,
		PANEL_VALUE_DISPLAY_LENGTH,	DE_HELP_TEXT_ALLOC_LENGTH-1,
		PANEL_VALUE,	"",
		PANEL_LAYOUT, PANEL_VERTICAL,
		PANEL_EVENT_PROC, help_text_eventproc, 
		PANEL_CLIENT_DATA, defobjptr, 
	0);
	defobjptr->ilabel = helpdisplayitem;
	defobjptr->ivalue = helpdisplayitem;

	return (int)panel_get(helppanel, PANEL_HEIGHT);
	}


/*
 * Return the defaults database full name (like "/Scrollbar") from the
 * currently selected button item in the progpanel.
 */
static char*
get_cur_progname()
{
	return (prog_names[cur_progvalue]);
}



static char
confirm_category_switch_msg[] =
"You have not saved your changes. If you switch categories without \
saving your changes, the changes will be lost.  Press the left  \
mouse button to switch categories and discard the changes you  \
have made.  Press the right button to remain in this category.";


Debug(
    Bool t_prog_notifyproc = False;
    )

#define NUMWINDOWS 5
static	Cursor	oldcursor[NUMWINDOWS];
static	char	*window[NUMWINDOWS];

/*
 * This event proc is called whenever one of the program name buttons
 * in the progpanel are clicked on.
 */
/* ARGSUSED */
static void
prog_notifyproc(item, value, event)
	Panel_item	item;
	struct inputevent *event;
{
        Bool 	any_change;
	int  	fd, i, result;
	static	first;
	Event	alert_event;

	if (first == 0) {
		window[0] = menupanel;
		window[1] = treepanel;
		window[2] = textwindow;
		window[3] = helppanel;
		window[4] = (char *)tool;
		first = 1;
		for (i = 0; i < NUMWINDOWS; i++)
			oldcursor[i] = cursor_create((char *)0);
	}
	for (i = 0; i < NUMWINDOWS; i++) {
		fd = win_get_fd(window[i]);
		win_getcursor(fd, oldcursor[i]);
		win_setcursor(fd, &defaultsedit_hourglass_cursor);
	}
	tool_set_attributes(tool, WIN_LABEL, "wait...", 0);

        any_change = subtree_extractinfo((Bool)FALSE);
	    /*
	     * pass False to subtree_extractinfo() to that 
             * it will onnly check for changed values and
	     * not actually read those changed values from
	     * the panel into memory.
	     */

        Debug(if (t_prog_notifyproc) printf("<prog_notifyproc: any_change=%d> ",
		any_change);)

	if (!any_change) {
	    result = 1;
	} else {
	    result = alert_prompt(
		    (Frame)0,
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
		        "You have not saved your changes.",
			"If you switch categories without saving",
			"your changes, the changes will be discarded.",
			"Please confirm.",
		        0,
		    ALERT_BUTTON_YES,	"Confirm, discard changes",
		    ALERT_BUTTON_NO,	"Cancel",
		    0);
	    if (result == ALERT_YES) {
		result = 1;
	    } else result = 0;
	}
	if (result == 0) {
	    (void)panel_set(item, PANEL_VALUE, cur_progvalue-1, 0);
	} else {
	    subtree_clearitems();

	    /* values are 1 base, not zero based */
	    cur_progvalue = value + 1;
	    subtree_show();
	}

	/* Remember to reset cursor and namestripe */

	for (i = 0; i < NUMWINDOWS; i++) {
		fd = win_get_fd(window[i]);
		win_setcursor(fd, oldcursor[i]);
	}
	tool_set_attributes(tool, WIN_LABEL, "defaultsedit", 0);
}



/*
 * Run a specialized setup program like Scrolldemo or Indenttool
 * in order to set parameters for the currently selected program.
 * The setup program name is stored under the node "/$Setup_program"
 */
static void
run_setup_prog()
{
	int	pid;	/* Process identification for child demo */
	char	childforkname[DE_FULLNAME_MAX_LENGTH];
	char	namenode[DE_FULLNAME_MAX_LENGTH];
	int	result;
	Event	alert_event;

	(void)strcpy(namenode, get_cur_progname());
	(void)strcat(namenode, "/$Setup_program");

	if (defaults_exists(namenode, (int *)NULL)) {
	    (void)strcpy(childforkname,
		defaults_get_string(namenode, "", (int *)NULL));
	    result = alert_prompt(
		    (Frame)0,
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
		        "Attempting to run Demo/Setup program.",
			"All changes done to default values will be",
			"saved before starting Demo program.",
			"Please confirm startup.",
		        0,
		    ALERT_BUTTON_YES,	"Confirm, start Demo",
		    ALERT_BUTTON_NO,	"Cancel",
		    0);
	    switch (result) {
		    case ALERT_YES: 
		        result = 1;
		        break;
		    case ALERT_NO:
		        return;
		    default: /* alert_failed */
			return;
	    }

	    show_message((char *)NULL, "Saving defaults parameters before calling demo program", "");
	    sleep(1);
	    (void) subtree_extractinfo(True);
	    dump_default_trees();
	    show_message((char *)NULL, "Starting child process with demo program, wait...", childforkname);
		  /* start child process to run setup in */
	    pid = start_child_proc(childforkname, False, False, (union wait *)NULL);
	     /* demo_child_wait3 is called when demo_child finish processing */
	    (void)notify_set_wait3_func(menupanel, demo_child_wait3, pid);
	}
	else
 	  show_message((char *)NULL, "Could not find any demo program name in defaults database, program: ", namenode);

}



/* 
 * Called when clicked on the Save button in the menu window
 * Save current defaults
 */
/* ARGSUSED */
static void
menu_save_notifyproc(item,  event)
	Panel_item		item;
	Event			event;
{
	(void) subtree_extractinfo(True);
	dump_default_trees();
}



static char
confirm_quit_with_changes_msg[] =
"Press the left mouse button to Quit without saving changes.  \
Press the right mouse button to cancel.";

static char 
confirm_quit_msg[] =
"Press the left mouse button to confirm Quit.  \
Press the right mouse button now to cancel.";

/* 
 * Called when clicked on the Quit button in the menu window
 * Quits the program without saving anyting
 */


/* ARGSUSED */
static void
menu_quit_notifyproc(item,  event)
	Panel_item		item;
	Event			event;
{
        Bool any_change;
	int	result;
	Event	alert_event;

        any_change = subtree_extractinfo((Bool)FALSE);
	/*
	* pass False to subtree_extractinfo() to that 
	* it will onnly check for changed values and
	* not actually read those changed values from
	* the panel into memory.
	*/

	if (any_change) {
		result = alert_prompt(
		    (Frame)0,
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
			"Quiting will discard unsaved changes.",
			"Please confirm.",
			0,
		    ALERT_BUTTON_YES,	"Confirm, discard changes",
		    ALERT_BUTTON_NO,	"Cancel",
		    0);
		    if (result == ALERT_YES) /* confirmed */
			exit(0);
	} else {
		result = alert_prompt(
		    (Frame)0,
		    &alert_event,
		    ALERT_MESSAGE_STRINGS,
			"Are you sure you want to Quit?",
			0,
		    ALERT_BUTTON_YES,	"Confirm",
		    ALERT_BUTTON_NO,	"Cancel",
		    ALERT_OPTIONAL,	1,
		    ALERT_NO_BEEPING,	1,
		    0);
		    if (result == ALERT_YES) /* confirmed */
			exit(0);
	}
}

	
/* 
 * Called when clicked on the Reset button in the menu subwindow
 * Resets the current panel to its state when it was opened.
 */
/* ARGSUSED */
static void
menu_reset_notifyproc(item,  event)
	Panel_item		item;
	Event			event;
{
	subtree_clearitems();
	subtree_show();
	show_message((char *)NULL, "", "");
}




/*
 * Check if user has previously clicked on a valid text object item
 */
static Bool
check_clicked_defobj(allowenumflg)
    Bool    allowenumflg;
{
    if (last_clicked_defobj==NULL) {
	show_message((char *)NULL, "First click on an item which may contain text or numeric data!", "");
	return False;
    }
    allowenumflg = (Bool) (allowenumflg && (last_clicked_defobj->nodetype==Enumerate || last_clicked_defobj->nodetype==Editenumerate));
    
    if (! (allowenumflg   ||
	   last_clicked_defobj->nodetype==String     || 
	   last_clicked_defobj->nodetype==Editstring || 
	   last_clicked_defobj->nodetype==Helpstring)) {
	show_message((char *)NULL, "Last clicked object does not contain textdata or numeric data!", "");
	return False;
    }    
    return True;
}



/* 
 * Edit the label on the item last clicked
 */
/* ARGSUSED */
static void
menu_labeledit_notifyproc(it,  event)
	Panel_item	it;
	Event		event;
{
	Defobj		defobjptr;
	Panel_item	old;

	if (!check_clicked_defobj(True)) return;
	if (last_clicked_defobj->nodetype==Helpstring) {
	    show_message((char *)NULL, "Click on an item first!", "");
	    return;
	}
	defobjptr = last_clicked_defobj;
	if  (defobjptr->nodetype==Editstring ||
             defobjptr->nodetype==Editenumerate) {
		/* Label is already in an editable state */
		return;
		
	}

	old = defobjptr->ilabel;
	if (defobjptr->nodetype==Enumerate) {
	    defobjptr->nodetype=Editenumerate;
	    defobjptr->ilabel=panel_create_item(treepanel, PANEL_TEXT,
		PANEL_ITEM_X, PANEL_CU(0),
		PANEL_ITEM_Y, (int)panel_get(old, PANEL_ITEM_Y), 
		PANEL_VALUE_DISPLAY_LENGTH, DE_STANDARD_DISPLAYLEN, 
		PANEL_VALUE_STORED_LENGTH, DE_FULLNAME_MAX_LENGTH, 
		PANEL_VALUE, panel_get(old, PANEL_LABEL_STRING), 
		PANEL_NOTIFY_PROC, tree_choice_notifyproc, 
		PANEL_CLIENT_DATA, defobjptr, 
		PANEL_PAINT, PANEL_NONE, 
		 0);


	} else if (defobjptr->nodetype==String) {
	    defobjptr->nodetype=Editstring;
	    defobjptr->ilabel=panel_create_item(treepanel, PANEL_TEXT,
		PANEL_ITEM_X, PANEL_CU(0),
		PANEL_ITEM_Y, (int)panel_get(old, PANEL_ITEM_Y), 
		PANEL_VALUE_DISPLAY_LENGTH, DE_STANDARD_DISPLAYLEN, 
		PANEL_VALUE_STORED_LENGTH, DE_FULLNAME_MAX_LENGTH, 
		PANEL_VALUE, panel_get(old, PANEL_LABEL_STRING), 
		PANEL_EVENT_PROC, tree_text_eventproc, 
		PANEL_CLIENT_DATA, defobjptr, 
		PANEL_PAINT, PANEL_NONE, 
		 0);
	}

	(void)panel_set(old, PANEL_PAINT, PANEL_NONE, 0);
	(void)panel_free(old);
	(void)panel_paint(defobjptr->ilabel, PANEL_CLEAR);
	last_clicked_defobj = NULL;
}



/* 
 * Insert a new defaults object into the tree panel and the linked list
 * of defaults objects.
 * Copy label name and possible value from the preceeding node.
 */
/* ARGSUSED */
static void
menu_insert_notifyproc(it,  event)
	Panel_item		it;
	Event			event;
{
	Recttype rectptr;
	int	totheight;
	int	ycoord;
	Panel_item  item;
	char	nodename[DE_FULLNAME_MAX_LENGTH];
	Defobj	succptr;
	Defobj	defobjptr;
	char*	labelptr;


	if (!check_clicked_defobj(True)) return;
	if (last_clicked_defobj->nodetype==Helpstring) {
	    show_message((char *)NULL, "Click on an item first!", "");
	    return;
	}
	
	item=last_clicked_defobj->ivalue;
	rectptr = (Recttype)(LINT_CAST(panel_get(item, PANEL_ITEM_RECT)));
	totheight = rectptr->r_height + (int)(LINT_CAST(panel_get(treepanel, PANEL_ITEM_Y_GAP)));
	ycoord = (int)panel_get(item, PANEL_ITEM_Y);
	tree_scrollup(ycoord, -totheight);
	(void)strcpy(nodename, getnodename_defobj(last_clicked_defobj));
	labelptr =  nodename+strlen(get_cur_progname())+1;
	defobjptr=create_text_defobj(labelptr, nodename, ycoord+totheight, True);
	succptr = last_clicked_defobj->nodenext;
	defobjptr->nodename = NULL;
	last_clicked_defobj->nodenext = defobjptr;
	defobjptr->nodenext = succptr;
	(void)panel_paint(treepanel, PANEL_CLEAR);
	last_clicked_defobj = NULL;
}



/* 
 * Delete defaults object, both in the treepanel and in the database
 */
/* ARGSUSED */
static void
menu_delete_notifyproc(it,  event)
	Panel_item		it;
	Event			event;
{
	Recttype rectptr;
	int	totheight;
	int	ycoord;
	Panel_item  item;
    	Defobj	defobjptr;
    	Defobj	prevptr;

	if (!check_clicked_defobj(True)) return;
	if (last_clicked_defobj->nodetype==Helpstring) {
	    show_message((char *)NULL, "Click on an item first!", "");
	    return;
	}

	item=last_clicked_defobj->ivalue;
	rectptr = (Recttype)(LINT_CAST(panel_get(item, PANEL_ITEM_RECT)));
	totheight = rectptr->r_height + (int)panel_get(treepanel, PANEL_ITEM_Y_GAP);
	ycoord = (int)(LINT_CAST(panel_get(item, PANEL_ITEM_Y)));
	if (last_clicked_defobj->nodename!=NULL) {
	    defaults_remove(last_clicked_defobj->nodename, (int *)NULL);
	}
	
	/* Update nodenext pointer field of predecessor of deleted node */
	defobjptr = last_clicked_defobj->nodenext;
	if (last_clicked_defobj==firstdefobj) {
	    firstdefobj = defobjptr;
	} else {
	    prevptr=firstdefobj;
	    while (prevptr->nodenext != last_clicked_defobj) {
		prevptr = prevptr->nodenext;
	    }
	    prevptr->nodenext = defobjptr;
	}

	freemem_defobj(last_clicked_defobj);
	last_clicked_defobj = NULL;
	tree_scrollup(ycoord, totheight);
	(void)panel_paint(treepanel, PANEL_CLEAR);
}

/* 
 * This proc is called when setup program finishes processing.
 * Reads in parameters stored in .defaults file by the setup program.
 */
/* ARGSUSED */
static Notify_value
demo_child_wait3(client, pid, status, rusage)
	Notify_client	client;
	int		pid;
	union wait* 	status;
	struct rusage*	rusage;
{
	char		name[DE_FULLNAME_MAX_LENGTH];

	show_message((char *)NULL, "Loading defaults parameters written into .defaults by demo program", get_cur_progname());
	sleep(1);
	(void)strcpy(name, get_cur_progname());
	/* Read parameters written to .defaults file by demo program  */
	defaults_special_mode();        /* Actually reads the whole database  */
	/* defaults_reread(name, NULL);   Commented out temporarily because
	   of bug in defaults_reread  */
	subtree_clearitems();
	subtree_show();
	show_message((char *)NULL, "", "");
}



	
/*
 * This event proc is called whenever a text or a message item is clicked on
 * in the tree panel subwindow. It is also called when the label of a
 * choice object (implemented as a message item) is clicked on.
 * A help message is displayed in the help subwindow
 * by the proc showhelpitem.
 */
static void
tree_text_eventproc(item, event)
	Panel_item	item;
	Event		*event;
{
	if (event_action(event) == MS_LEFT && event_is_up(event)) {
	    last_clicked_defobj = (Defobj)(LINT_CAST(
	    	panel_get(item, PANEL_CLIENT_DATA)));
	    last_clicked_defobj->specifichelp = False;
	    showhelpitem(last_clicked_defobj, False);
	}
	(void)panel_default_handle_event(item, event);
}

/* This is the notify procedure for the password item. It is called on */
/* every input character. All characters are accepted. The showdtop () */
/* routine is called upon a \r\n\t. */

static unsigned char edit_char, edit_word, edit_line;

Panel_setting panel_notify_proc(panel_item, event)
Panel_item panel_item;
Event *event;
{
    register unsigned short w = event_id(event);
    char s[80], t[80];

    /*
     * Here we accept and report non-printing characters to make entering
     * the line editing characters more intuitive.
     *
     * The start-up edit chars are cached so that we don't report them
     * during user type-in.  Note that these chars do not change even
     * after the user has saved replacements into her defaults.
     */
    if (!edit_char)
        edit_char =
            *(char *)defaults_get_string("/Text/Edit_back_char", "\0177", 0);
    if (!edit_word)
        edit_word =
            *(char *)defaults_get_string("/Text/Edit_back_word", "\027", 0);
    if (!edit_line)
        edit_line =
            *(char *)defaults_get_string("/Text/Edit_back_line", "\025", 0);

    if (((0 <= w && w < (unsigned short) ' ') || w == 0177)
            && !(w == edit_char || w == edit_word || w == edit_line
            || w == 0177 || w == 027 || w == 025)) {
    	*s = (char) event_id(event);
    	*(s+1) = '\0';
    	defaults_str_unparse(s, t);
    	strcpy(s, "You have entered the non-printing character ");
    	strcat(s, t);
    	show_message((char *)NULL, "", s);
    }
    switch (w)
    {
      case '\011': /* tab */
      case '\012': /* new line */
      case '\015': /* return */
         return (PANEL_NONE);
      default:
         return (PANEL_INSERT);
    }
}


/*
 * This event proc is called whenever a text or a message item is clicked on
 * in the message subwindow where help texts are displayed.
 */
static void
help_text_eventproc(item, event)
	Panel_item	item;
	Event		*event;
{
	if (event_action(event) == MS_LEFT && event_is_up(event)) {
	    last_clicked_defobj = (Defobj)(LINT_CAST(panel_get(item, PANEL_CLIENT_DATA)));
	    last_clicked_defobj->specifichelp = False;
	}
	(void)panel_default_handle_event(item, event);
}



/*
 * This notifyproc is called whenever a choice item in the treepanel
 * is clicked on. A help message is displayed in the help subwindow
 * by the proc showhelpitem.
 */
/* ARGSUSED */
static void
tree_choice_notifyproc(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
	last_clicked_defobj = (Defobj)(LINT_CAST(panel_get(item, PANEL_CLIENT_DATA)));
	last_clicked_defobj->specifichelp = True;
	showhelpitem(last_clicked_defobj, False);
}
	

/*
 * Run the setup program when the relevant button in the current
 * tree panel subwindow is clicked.
 * This button exists only for programs which have the $Setup_program
 * parameter defined.
 */
/* ARGSUSED */
static void
tree_runsetup_notifyproc(item, event) 
        Panel_item	item;
	Event		*event;
{
	run_setup_prog();
}


/*
 * Iterate over all default objects and items in treepanel and delete them.
 */
static void
subtree_clearitems()
{
	Defobj defobjptr;
	Panel_item firstitem;
	Panel_item tmpitem;

	cur_ylinenr = 0;
	defobjptr = firstdefobj;
	while (defobjptr != NULL) {
	    firstdefobj=defobjptr->nodenext;
	    freemem_defobj(defobjptr);
	    defobjptr = firstdefobj;
	}
	firstdefobj = NULL;
	lastinserteddefobj = NULL;

	/* Clean up any remaining buttons in the treepanel */
	firstitem = panel_get(treepanel, PANEL_FIRST_ITEM);
	while (firstitem != NULL) {
	    tmpitem = panel_get(firstitem, PANEL_NEXT_ITEM);
	    (void)panel_free(firstitem);
	    firstitem = tmpitem;
	}
}




/*
 * Return the defaults full path nodename corresponding to defaults object
 * pointed to by defobjptr.
 */
static char*
getnodename_defobj(defobjptr)
    Defobj  defobjptr;
{
    char   tmpname[DE_FULLNAME_MAX_LENGTH];
    char*   nodename;

    if (defobjptr==NULL) {
	deferror("NULL pointer in getnodename_defobj", False);
	stop();
    }	
    if   (defobjptr->nodetype==Editstring ||
          defobjptr->nodetype==Editenumerate) {
	(void)strcpy(tmpname, get_cur_progname());
	(void)strcat(tmpname, "/");
	(void)strcat(tmpname, (char*)panel_get(defobjptr->ilabel, PANEL_VALUE));
	nodename = getchars(strlen(tmpname));
	(void)strcpy(nodename, tmpname);
	return nodename;
    }
    return defobjptr->nodename;
}

Debug(
    Bool t_subtree_extractinfo = False;
    )

/*
 * subtree_extractinfo() checks each panel item and if it reflects a
 * different value than that stored in the in-memory database,
 * and if store_panel_values is true, it extracts the new values
 * from the items and copies it to the database.  It also returns a
 * Bool(ean) which indicates whethe any of the items reflect different
 * values.  
 *
 * When store_panel_values is False, subtree_extractinfo() essentially
 * tests whether or not there are values in the parameters that might
 * need saving.
 */

static Bool
subtree_extractinfo(store_panel_values)
        Bool		store_panel_values;
{
	Defobj		defobjptr;
	Defobj		prevobjptr;
	int		intvalue;
	char*		strvalue;
	char		strvalue2[DE_NAME_ITEM_MAX_LENGTH];
	char*		nodename;
	Bool		changedflg;	/* True if any panel value change  */
	int		progname_pos;	/* Possible program name position */

	changedflg = False;
	defobjptr = firstdefobj;
	prevobjptr = firstdefobj;
	while (defobjptr != NULL) {
	    switch (defobjptr->nodetype) {
		case Message: {
			/* do nothing */
		}
		break;

	   	case Enumerate:
		    intvalue = (int)panel_get_value(defobjptr->ivalue);
		    (void)get_enum_member_string(defobjptr->nodename, intvalue,
			    strvalue2);
		    if ((store_panel_values && deflt_always_save(
		  		  defobjptr->nodename))
		    	|| (!strequal(strvalue2,
			    defaults_get_string(defobjptr->nodename, strvalue2,
			    (int *)NULL))) ) {
                        if (store_panel_values) {
			    /* set_enumeration same as set_string but does
			       additional checking  */
			    defaults_set_enumeration(defobjptr->nodename, 
			            strvalue2, (int *)NULL);
			}
		    	changedflg = True;
		    }
		break;

	    	case String:
		    strvalue = panel_get_value(defobjptr->ivalue);
		    (void)convert_string(strvalue, strvalue2);
		    if (strequal(strvalue, "")) {
			(void)strcpy(strvalue2, 
				defaults_get_default(defobjptr->nodename, "", 
				(int *)NULL));
		    }
		    if (!strequal(strvalue2, 
			    defaults_get_string(defobjptr->nodename, strvalue2, 
			    (int *)NULL)) ) {
                        if (store_panel_values) {
			    defaults_set_string(defobjptr->nodename, strvalue2, 
			            (int *)NULL);
			}
			changedflg = True;
		    }
 		break;

		case Editstring:
			/* This text node was inserted. Extract the label
			    string and construct a corresponding node
			    in the defaults database.
			 */
		    nodename=getnodename_defobj(defobjptr);
		    changedflg = True;
		    if (store_panel_values) {
		        if (! strequal(nodename, "")) {
			    strvalue = panel_get_value(defobjptr->ivalue);
			    (void)convert_string(strvalue, strvalue2);
			    if (strequal(strvalue, "") &&
				        defaults_exists(nodename, (int *)NULL)) {
			        (void)strcpy(strvalue2, defaults_get_default(nodename,
					"", (int *)NULL));
			    }
			    defaults_set_string(nodename, strvalue2, (int *)NULL);
			    if (defobjptr->nodename!=NULL) {
		                subtree_copy(defobjptr->nodename, nodename);
			        defaults_set_string(nodename, strvalue2, (int *)NULL);
			        defaults_move(getnodename_defobj(prevobjptr),
			                nodename, False, (int *)NULL);
			        defaults_remove(defobjptr->nodename, (int *)NULL);
			    } else {
			        defaults_move(getnodename_defobj(prevobjptr),
				        nodename, False, (int *)NULL);
			    }
			}
			
		    }
		break;

		case Editenumerate: {
			/* This enumeration node was inserted. Extract the label
			    string and construct a corresponding node
			    in the defaults database.
			 */
		    nodename=getnodename_defobj(defobjptr);
		    changedflg = True;
                    if (store_panel_values) {
		        if (! strequal(nodename, "")) {
			    intvalue = (int)panel_get_value(defobjptr->ivalue);
			    (void)get_enum_member_string(defobjptr->nodename, 
				    intvalue, strvalue2);
			    defaults_set_enumeration(nodename, strvalue2, (int *)NULL);
			    if (defobjptr->nodename!=NULL) {
		                subtree_copy(defobjptr->nodename, nodename);
			        defaults_set_enumeration(nodename, strvalue2,
					(int *)NULL);
			        defaults_move(getnodename_defobj(prevobjptr),
					nodename, False, (int *)NULL);
			        defaults_remove(defobjptr->nodename, (int *)NULL);
			    } else  {
			        defaults_move(getnodename_defobj(prevobjptr),
					nodename, False, (int *)NULL);
			    }
			}
		    }
		}
		break;


		case Undefined:
		break;	/* Do nothing */
		
	   	default: /* Do nothing */
		break;
	    }
	   prevobjptr = defobjptr;
	   defobjptr = defobjptr->nodenext;
	}

    /* If there is an helpstring displayed, store this helpstring in the
       database */
/****  Commented out in order to not allow helptext editing in defaultsedit
    helpobjptr = (Defobj)panel_get(helpdisplayitem, PANEL_CLIENT_DATA);
    if (! strequal(helpobjptr->nodename, "")) {
        (void)strcpy(fullname, helpobjptr->nodename);
	(void)strcat(fullname, "/$Help");
	helpstr = (char*)panel_get_value(helpdisplayitem);
	if (strequal(helpstr, "")) {
            if (store_panel_values) {
	        defaults_remove(fullname, (int *)NULL);
	    }
            changedflg = True;
	} else if (!strequal(helpstr, nohelpavail)){
            if (store_panel_values) {
	        (void)convert_string(helpstr, helpstr2);
	        defaults_set_string(fullname, helpstr2, (int *)NULL);
    	    }
	    changedflg = True;
	}
    }
****/
    

    /*  Now we have to set the changed flag for special format programs  */
    progname_pos = member_pos(get_cur_progname(), specformatnames);
    if (progname_pos != -1) {
	specformatnames[progname_pos]->changed = changedflg;
    }

    Debug(if (t_subtree_extractinfo)
	    printf("<subtree_extractinfo: store_panel_values=%d changedflg=%d> ",
    	    store_panel_values,changedflg);)

    return changedflg;
}



/*
 * Display all information from the defaults database
 * for the currently selected program.
 * This information is displayed in the treepanel.
 * All panel items are created from scratch.
 */
static void
subtree_show()
{   
	char	namenode[DE_FULLNAME_MAX_LENGTH];
	char	childforkname[DE_FULLNAME_MAX_LENGTH];
	char	buttontext[DE_FULLNAME_MAX_LENGTH];
	char	accessname[DE_FULLNAME_MAX_LENGTH];
	Bool	flg;

	firstdefobj = NULL;
	lastinserteddefobj = NULL;
	last_clicked_defobj = NULL;
	show_message((char *)NULL, "", "");
	(void)panel_paint(treepanel, PANEL_CLEAR);
	(void)strcpy(namenode, get_cur_progname());

	/* Check if subtree has info stored in spec format, like .mailrc */
	flg = specialformat_to_defaults(namenode);
	if (flg != True) return; /* Return if not successful conversion */

	(void)strcpy(accessname, namenode);

    /* Set scrollbar position to zero */
    	(void)scrollbar_set((Scrollbar)(LINT_CAST(
			panel_get(treepanel, PANEL_VERTICAL_SCROLLBAR))), 
		SCROLL_VIEW_START, 0,
        	SCROLL_REQUEST_MOTION,	SCROLL_ABSOLUTE,
		0);
    	(void)win_post_id_and_arg(treepanel, SCROLL_REQUEST, NOTIFY_SAFE,
            panel_get(treepanel, PANEL_VERTICAL_SCROLLBAR), NOTIFY_COPY_NULL,
	    NOTIFY_RELEASE_NULL);

    	subtree_showitems(accessname, False);

	(void)strcat(namenode, "/$Setup_program");
	if (defaults_exists(namenode, (int *)NULL)) {
 	    (void)strcpy(childforkname, defaults_get_string(namenode, "", (int *)NULL));
	    (void)strcpy(buttontext, "Click to start setup program: ");
	    (void)strcat(buttontext, childforkname);
	    (void)panel_create_item(treepanel, PANEL_BUTTON,
	 	PANEL_FEEDBACK,		PANEL_INVERTED,
		PANEL_NOTIFY_PROC,	tree_runsetup_notifyproc, 
		PANEL_LABEL_IMAGE, 
		    panel_button_image(treepanel,buttontext,0,(Pixfont *)NULL),
		PANEL_CLIENT_DATA,	NULL,
		0);
	    cur_ylinenr = cur_ylinenr+1;
	}

    	(void)panel_paint(treepanel, PANEL_CLEAR);
}



/*
 * Do a preorder traversal of the defaults database subtree denoted
 * by nodename, and display all leaf nodes, or intermediate nodes
 * with values. Display nodes as text items or choice items.
 */
static void
subtree_showitems(nodename, showparentflg)
	char*	nodename;	/* Full name of node to show data from */
	Bool	showparentflg;	/* if Not showparentflg, do not make */
				/*  a panel_item out of nodename */
{
	char	sibling[DE_FULLNAME_MAX_LENGTH];
	Defaulttype type;	/* Enumerate or String type defaults node */

	if (nodename == NULL)
  		return;
	type = get_nodetype(nodename);
	if (type==Message) {
		showitem(nodename, Message);
		return;
	}
	if (showparentflg) {
	    if (type==Enumerate) {
		showitem(nodename, type);
		return;
	    } else {
		showitem(nodename, type);
	    }
	}
	deflt_get_fullchild(nodename, sibling);
	while (sibling!= NULL && strlen(sibling) != 0) {
		subtree_showitems(sibling, True);
		deflt_get_fullsibling(nodename, sibling, sibling);
	}
}





static Bool
displayable_node(nodename)
/*
 * Returns TRUE if node with name nodename is a node which is to be displayed.
 * Nodenames containing $ are not displayable nodes: /fum/$Help, /fie/$Fum
 * Otherwise, a displayable node is a leaf node or an intermediate level
 * node with a non-empty string standard value or user-assigned value.
 * A leaf node is a node which does not have any non-dollar children.
 */
    char*   nodename;
{
    int	    nch;
    char    childname[DE_FULLNAME_MAX_LENGTH];
    int	    nodenamelen1;

    for (nch=strlen(nodename)-1; nch>0; nch--) {
	if (nodename[nch]=='/' ) {
	    if (nodename[nch+1]=='$') return False;
	    else break;
	}
    }
    if (   ! strequal(defaults_get_default(nodename, "", (int *)NULL), DEFAULTS_UNDEFINED)
	|| ! strequal(defaults_get_string(nodename, "", (int *)NULL), DEFAULTS_UNDEFINED) ) return True;

    nodenamelen1 = strlen(nodename)+1;
    deflt_get_fullchild(nodename, childname);
    while (!strequal(childname, "") ) {
    	if (childname[nodenamelen1] != '$') 
	    return False;   /* This node has non-dollar child */
	deflt_get_fullsibling(nodename, childname, childname);
    }

    return True;	    /* This node is a proper leaf node */
}



/*
 * Retrieve standard value from master_defaults for nodename.
 * Store it into standardval,  which is a pointer to a string area.
 */
static char*
get_standard_valuestr(nodename, standardval)
	char*	nodename;	/* full_name of a node */
	char*	standardval;	/* pointer to string area for standard value */
{
	char*	tmp;

	if (nodename == NULL) {
	    (void)strcpy(standardval, "");
	} else if (defaults_exists(nodename, (int *)NULL)) {
	    tmp = defaults_get_default(nodename, "", (int *)NULL);
	    (void)strcpy(standardval, tmp);
	}
	return standardval;
}



/*
 * Create and initialize a default panel object,  which groups several
 * panel items together.
 * There are text panel objects and enumeration panel objects.
 * Each of these contains several panel items
 * Each tree panel item points to an instance of
 */
static Defobj
create_defobj(nodename, objtype)
    char*   nodename;
    Defaulttype	objtype;
{
	Defobj	defobjptr;

	defobjptr = (Defobj)(LINT_CAST(get_memory(sizeof *defobjptr)));
	defobjptr->nodetype = objtype;
	defobjptr->nodename = strcpy(getchars(strlen(nodename)), nodename);
	defobjptr->nodenext = NULL;
	defobjptr->ilabel = NULL;
	defobjptr->ivalue = NULL;
	defobjptr->specifichelp = False;
	return defobjptr;
}



/*
 * free memory occupied by defaults object and associated data.
 */
static void
freemem_defobj(defobjptr)
    Defobj  defobjptr;
{

    if   (defobjptr->nodetype==Enumerate ||
	  defobjptr->nodetype==Editenumerate) {
	(void)panel_free(defobjptr->ilabel);
	(void)panel_free(defobjptr->ivalue);
	(void)panel_free(defobjptr->ifill1);
    } else if (defobjptr->nodetype==String ||
	       defobjptr->nodetype==Editstring) {
	(void)panel_free(defobjptr->ilabel);
	(void)panel_free(defobjptr->ivalue);
    }
    free_mem((int)defobjptr->nodename);
    free_mem((int)defobjptr);
}



/*
 * Display defaults node with name nodename on the tree panel.
 */
static void
showitem(nodename, type)
    char*   nodename;
    Defaulttype    type;
{
    int	    ycoord;
    char*   labelptr;
    Defobj  defobjptr;
    
    if (type==String && !displayable_node(nodename)) return;
    
    labelptr =  nodename+strlen(get_cur_progname())+1;
    /* add one pixel to avoid starting at pixel zero */
    ycoord = PANEL_CU(cur_ylinenr) + 1;
    cur_ylinenr = cur_ylinenr+1;

    if (type==Enumerate) {
	defobjptr=create_enum_defobj(labelptr, nodename, ycoord, False);
    } else if (type==Message) {
	defobjptr=create_message_defobj(labelptr, nodename, ycoord, False);
    } else {
	defobjptr=create_text_defobj(labelptr, nodename, ycoord, False);
    }

    if (firstdefobj==NULL) {
	firstdefobj = defobjptr;
	lastinserteddefobj = firstdefobj;
    } else {
	lastinserteddefobj->nodenext = defobjptr;
	lastinserteddefobj = defobjptr;
    }
}




/*
 * Create a default object of type String, which represents a defaults
 * database text node.
 */
static Defobj
create_text_defobj(labelname, nodenamei, ycoord, labeleditflg)
	char*	labelname;  /* Label of panel item */
	char*	nodenamei;   /* Full defaults database path name */
	int	ycoord;	    /* Panel item y coordinate */
	Bool	labeleditflg;	/* True if label should be editable */
{
	Panel_item pitem;
	char	standardval[DE_NAME_ITEM_MAX_LENGTH];
	char	valuestr[DE_NAME_ITEM_MAX_LENGTH];
	char	tmp[DE_NAME_ITEM_MAX_LENGTH];
	char	labelstr[DE_ITEM_MAX_LABEL_LENGTH];
	char*	nodename;
	int	valuepos;
	int	valuealloclen;
	Defobj	defobjptr;
	Bool	standarddefinedflg;


	if (labeleditflg) {
	    defobjptr = create_defobj(nodenamei, Editstring);
	    defobjptr->ilabel=panel_create_item(treepanel, PANEL_TEXT,
		PANEL_ITEM_X, PANEL_CU(0),
		PANEL_ITEM_Y, ycoord,
		PANEL_VALUE_DISPLAY_LENGTH, DE_STANDARD_DISPLAYLEN, 
		PANEL_VALUE_STORED_LENGTH, DE_FULLNAME_MAX_LENGTH, 
		PANEL_VALUE, labelname, 
		PANEL_EVENT_PROC, tree_text_eventproc,
		PANEL_CLIENT_DATA, defobjptr, 
		 0);
	}
	else {
	    defobjptr = create_defobj(nodenamei, String);
	    defobjptr->ilabel=panel_create_item(treepanel, PANEL_MESSAGE,
		PANEL_ITEM_X, PANEL_CU(0),
		PANEL_ITEM_Y, ycoord,
		PANEL_LABEL_STRING,	labelname,
		PANEL_LABEL_BOLD,	True,
		PANEL_EVENT_PROC, tree_text_eventproc,
		PANEL_CLIENT_DATA, defobjptr, 
		 0);
	}

	nodename = defobjptr->nodename;
	(void)strcpy(labelstr, "");
	if (defaults_exists(nodename, (int *)NULL)) {
	    (void)strcpy(tmp, defaults_get_string(nodename, "", (int *)NULL));
	} else {
	    (void)strcpy(tmp, "");
	}
	defaults_str_unparse(tmp, valuestr);
	standarddefinedflg = True;
	if (labeleditflg)
	     (void)strcpy(standardval, "");
	else {
	    (void)get_standard_valuestr(nodename, tmp);
	    if (strequal(tmp, DEFAULTS_UNDEFINED)) {
		defaults_str_unparse(tmp, standardval);
		if (strequal(valuestr, standardval)) (void)strcpy(valuestr, "");
		(void)strcpy(standardval, "");
		standarddefinedflg = False;
	    } else {
		defaults_str_unparse(tmp, standardval);
	    }
	}
	
	valuealloclen=MAX(strlen(valuestr)+DE_MIN_VALUE_STOREDLEN,
	     2*DE_MIN_VALUE_STOREDLEN);
	if (strequal(valuestr, standardval)) (void)strcpy(valuestr, "");
	if (strlen(standardval)>DE_STANDARD_DISPLAYLEN) {
	    standardval[DE_STANDARD_DISPLAYLEN-3]='\0';
	    (void)strcat(standardval, "...");
	}
	if (standarddefinedflg) {
	    (void)strcpy(labelstr, "(");
	    (void)strcat(labelstr, standardval);
	    (void)strcat(labelstr, "):");
	} else {
	    (void)strcpy(labelstr, "  :");
	}
	valuepos = strlen(labelname)+strlen(labelstr)+1;
	valuepos = MAX(DE_ITEMLABELPOS, valuepos);

	pitem = panel_create_item(treepanel, PANEL_TEXT,
		PANEL_ITEM_X, 
		PANEL_CU(valuepos-strlen(labelstr)),
		PANEL_ITEM_Y, ycoord,
		PANEL_LABEL_STRING,	labelstr,
		PANEL_VALUE_STORED_LENGTH,	valuealloclen, 
		PANEL_VALUE_DISPLAY_LENGTH,	valuealloclen,
		PANEL_EVENT_PROC, tree_text_eventproc,
		PANEL_CLIENT_DATA, defobjptr, 
		PANEL_PAINT, PANEL_NONE,
		PANEL_NOTIFY_PROC, panel_notify_proc,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		 0);

	(void)panel_set(pitem, PANEL_VALUE, valuestr,
	    PANEL_PAINT, PANEL_NONE, 
	     0);
	defobjptr->ivalue=pitem;
	return defobjptr;
}


/*
 * Create a default object of type Message, which represents a defaults
 * database $Message node, which has $Message as a child.
 * The value of the $Message child node is the textstring which should
 * be printed out in the panel as a value. 
 */
/* ARGSUSED */
static Defobj
create_message_defobj(labelname, nodenamei, ycoord, labeleditflg)
	char*	labelname;  /* Label of panel item */
	char*	nodenamei;   /* Full defaults database path name */
	int	ycoord;	    /* Panel item y coordinate */
	Bool	labeleditflg;	/* True if label should be editable */
{
	char	valuestr[DE_NAME_ITEM_MAX_LENGTH];
	char	tmp[DE_NAME_ITEM_MAX_LENGTH];
	char	nodename[DE_ITEM_MAX_LABEL_LENGTH];
	Defobj	defobjptr;

	defobjptr = create_defobj(nodenamei, Message);
	(void)strcpy(nodename, nodenamei);
	(void)strcat(nodename, "/$Message");
	(void)strcpy(tmp, defaults_get_string(nodename, "", (int *)NULL));
	defaults_str_unparse(tmp, valuestr);
	defobjptr->ilabel=panel_create_item(treepanel, PANEL_MESSAGE,
		PANEL_ITEM_X, PANEL_CU(0),
		PANEL_ITEM_Y, ycoord,
		PANEL_LABEL_STRING,	valuestr,
		PANEL_CLIENT_DATA, defobjptr, 
		 0);

	defobjptr->ivalue=NULL;
	return defobjptr;
}



/*
 * Create a default panel object of type Enumerate, which represents a defaults
 * database enumeration node and subtree.
 */
static Defobj
create_enum_defobj(labelname, nodenamei, ycoord, labeleditflg)
	char*	labelname;	/* Label of panel item */
	char*	nodenamei;	/* Full defaults database path name */
	int	ycoord;		/* Panel item y coordinate */
	Bool	labeleditflg;	/* True if label should be editable */
{

	char*	choicearr[ATTR_STANDARD_SIZE];	/* array of pointers to strings */
	int	childcount;		/* number of pointers in choicearr */
	char	childname[DE_FULLNAME_MAX_LENGTH];	/* Full name of Enumerate child node */
	char	accessname[DE_FULLNAME_MAX_LENGTH];  /* Enumerate node */
	int	accesslen;	/* Length of enumerate node name */
	char	labelstr[DE_ITEM_MAX_LABEL_LENGTH];
	int	valuepos;
	Defobj	defobjptr;
	char*	nodename;
	char	standardchoice[DE_MAX_CHOICE_LENGTH];

	if (labeleditflg) {
	    defobjptr = create_defobj(nodenamei, Editenumerate);
	    defobjptr->ilabel=panel_create_item(treepanel, PANEL_TEXT,
		PANEL_ITEM_X, PANEL_CU(0),
		PANEL_ITEM_Y, ycoord,
		PANEL_VALUE_DISPLAY_LENGTH, DE_STANDARD_DISPLAYLEN, 
		PANEL_VALUE_STORED_LENGTH, DE_FULLNAME_MAX_LENGTH, 
		PANEL_VALUE, labelname, 
		PANEL_NOTIFY_PROC, tree_choice_notifyproc,
		PANEL_CLIENT_DATA, defobjptr, 
		 0);
	}
	else {
	    defobjptr = create_defobj(nodenamei, Enumerate);
	    defobjptr->ilabel=panel_create_item(treepanel, PANEL_MESSAGE,
		PANEL_ITEM_X, PANEL_CU(0),
		PANEL_ITEM_Y, ycoord,
		PANEL_LABEL_STRING,	labelname,
		PANEL_LABEL_BOLD,	True,
		PANEL_EVENT_PROC, tree_text_eventproc,
		PANEL_CLIENT_DATA, defobjptr, 
		 0);
	}

	nodename = defobjptr->nodename;
	(void)strcpy(accessname,nodename);
	accesslen = strlen(accessname);

	choicearr[0] = (char *) PANEL_CHOICE_STRINGS;
	deflt_get_fullchild(accessname, childname);
	childcount = 0;
	while (	! ((childname == NULL) || (strequal(childname, "") ) ) ) {
	    if (childname[accesslen+1] != '$') {
		if (childcount>=ATTR_STANDARD_SIZE) {
			deferror("Too many enum strings\n", False); }
		childcount = childcount+1;
		choicearr[childcount] = strcpy(getchars(strlen(childname+accesslen+1)), childname+accesslen+1);
	    }
	    deflt_get_fullsibling(accessname, childname, childname);
	}
	if (childcount ==0) {
		deferror(strcat(accessname, " has no enumeration members."), False);
		childcount = 1;
		choicearr[childcount] = " ";
		}
	choicearr[childcount+1] = 0;	/* 0 terminate string list */
	choicearr[childcount+2] = 0;	/* 0 terminate argument list */

	(void)get_standard_valuestr(nodename, standardchoice);
	if (strequal(standardchoice, DEFAULTS_UNDEFINED)) {
	    (void)strcpy(labelstr, "  :");
	} else {
	    (void)strcpy(labelstr, "(");
	    (void)strcat(labelstr, standardchoice);
	    (void)strcat(labelstr, "):");
	}
	
	valuepos = strlen(labelname)+strlen(labelstr)+1;
	valuepos = MAX(DE_ITEMLABELPOS, valuepos);
	defobjptr->ifill1 = panel_create_item(treepanel, PANEL_MESSAGE, 
		PANEL_ITEM_X,
		PANEL_CU(valuepos-strlen(labelstr)),
		PANEL_ITEM_Y, ycoord,
		PANEL_LABEL_STRING, labelstr,
		PANEL_EVENT_PROC, tree_text_eventproc,
		PANEL_CLIENT_DATA, defobjptr, 
		 0);
 	defobjptr->ivalue = panel_create_item(treepanel, PANEL_CYCLE,
		PANEL_ITEM_Y, ycoord,
		PANEL_CHOICE_YS, ycoord, 0,
		PANEL_ATTRIBUTE_LIST, choicearr,
		PANEL_VALUE,  get_seq_enum_nr(nodename),
		PANEL_CLIENT_DATA, defobjptr, 
		PANEL_NOTIFY_PROC, tree_choice_notifyproc,
		0);


	return defobjptr;
}



/*
 * defaults_str_unparse(outstring, instring) will convert instring to outstring
 *  as a C-style string.
 *  Result will be stored in outstring.
 *  The conversion is primarily there to convert strings containing
 *  control characters to textual strings like "\008"
 */
#define STRCPYPLUS(s, t) \
	(void)strcpy((s), (t)); \
	(s) += strlen((t));
	
static void
defaults_str_unparse(instring, outstring)
	register char*	instring;	/* Input string */
	register char*	outstring;	/* Converted output string */
{
	register int	chr;		/* Temporary character */

	while (True){
		chr = *instring++;
		if (chr == '\0')
			break;
		if ((' ' <= chr) && (chr <= '~')){
			*outstring++ = chr;
			continue;
		}
		*outstring++ = '\\';
		
		switch (chr){
		    case '\n':
			*outstring++ = 'n';
			break;
		    case '\r':
			*outstring++ = 'r';
			break;
		    case '\t':
			*outstring++ = 't';
			break;
		    case '\b':
			*outstring++ = 'b';
			break;
		    case '\\':
			*outstring++ = '\\';
			break;
		    case '\f':
			*outstring++ = 'f';
			break;
		    case '\"':
			*outstring++ = '"';
			break;
		    default:
		        if ('\000'<=chr && chr<' ') {
                            *outstring++ = '^';
                            *outstring++ = chr+'@';
                        } else {
                            (void)sprintf(outstring, "%03o", chr & 0377);
		            outstring = outstring+3;
	                }
		}
	}
	*outstring++ = '\0';
}





/*
 * Return True if help is defined for node full_name, otherwise False
 */
static Bool
deflt_helpdefined(full_name)
	char*	full_name;
{
	char	accessname[DE_FULLNAME_MAX_LENGTH];

	(void)strcpy(accessname, full_name);
	(void)strcat(accessname, "/$Help");


	return defaults_exists(accessname, (int *)NULL);
}


/*
 * Return True if the value for this item should always be written out
 * regardless of whether or not it differs from the default value, otherwise
 * False 
 */
static Bool
deflt_always_save(full_name)
	char*	full_name;
{
	char	accessname[DE_FULLNAME_MAX_LENGTH];

	(void)strcpy(accessname, full_name);
	(void)strcat(accessname, "/$Always_Save");


	/* return defaults_exists(accessname, (int *)NULL); */
	if (defaults_exists(accessname, (int *)NULL))
		return(True);
	else
		return(False);
}



/* 
 * Return string of help info if it is defined for full_name,  otherwise
 * return defaulthelp stored in helpstr.
 * The helpstring is stored into memory pointed to by helpstr
 */
static void
deflt_get_help(full_name, defaulthelp,  helpstr)
	char*	full_name;
	char*	defaulthelp;
	char* 	helpstr;
{
	char	accessname[DE_FULLNAME_MAX_LENGTH];

	(void)strcpy(accessname, full_name);
	(void)strcat(accessname, "/$Help");
	(void)strcpy(helpstr,  defaults_get_string(accessname, defaulthelp, (int *)NULL));
	return;
}



/*
 * Show help text if any, associated with clickeditem.
 * If there is no help string associated with the node
 * look for a help string on its parent node and so on up to the highest level.
 * If no help was found,  return the string "No help available"
 * If enumspecifichelp is True, first look for specialized help attached
 * to the specific enumeration value.
 */
static void
showhelpitem(defobjptr, helpeditflg)
	Defobj	defobjptr;
	Bool	helpeditflg;
{
	char*	nodename;
	char	accessname[DE_FULLNAME_MAX_LENGTH];
	char	accessnameleaf[DE_FULLNAME_MAX_LENGTH];
	char	helpstr[DE_HELP_TEXT_ALLOC_LENGTH*2];
	int	nch;

	if (defobjptr == NULL) return;
	nodename = getnodename_defobj(defobjptr);
	if (nodename == NULL) {
		deferror("Nodename = NULL in showhelpitem", True); return; }
	(void)strcpy(accessname, nodename);

	if ((defobjptr->nodetype == Enumerate ||
	     defobjptr->nodetype == Editenumerate) &&
	     (defobjptr->specifichelp || !deflt_helpdefined(accessname))
	    )
	    { 
		(void)strcat(accessname, "/");
		(void)get_enum_member_string(nodename, (int)(LINT_CAST(panel_get_value(defobjptr->ivalue))),
		    accessname+strlen(accessname));
	    }

	(void)strcpy(accessnameleaf, accessname);

	if (helpeditflg) {
	    if (deflt_helpdefined(accessname)) {
		deflt_get_help(accessname, "", helpstr);
	    } else {
		(void)strcpy(helpstr, "");
	    }
	    goto showhelp;
	}	    

	(void)strcat(accessname, "/");
	for (nch=strlen(accessname)-1; nch>0; nch--) {
	    if (accessname[nch]=='/') {
		accessname[nch]=0;
		if (deflt_helpdefined(accessname)) {
			deflt_get_help(accessname, nohelpavail, helpstr);
			goto showhelp;
		}
	    }
	};

	(void)strcpy(helpstr, nohelpavail);
	(void)strcpy(accessname, accessnameleaf);

showhelp: show_message(accessname, accessname, helpstr);
	
}



/*
 * Display a message, usually a help message, in the help window.
 * The labelname is displayed on the first line and the valuestr
 * is displayed on the second line.
 * The nodename is the name of the node where the help text is stored.
 * If the message is not a help text, then the nodename argument
 * should be NULL.
 * A sideeffekt: if nodename in defobj descriptor is not "", then
 * we assume that the user has previously shown a helpstring which might
 * have been modified, so we store it in the database.
 */
static void
show_message(nodename, labelname, valuestr)
	char*	nodename;
	char*	labelname;
	char*	valuestr;
{
	Defobj	defobjptr;
	char	valuestr2[DE_HELP_TEXT_ALLOC_LENGTH];

	defobjptr = (Defobj)(LINT_CAST(panel_get(helpdisplayitem, PANEL_CLIENT_DATA)));

	/* If previously shown item was a help string, then it might have been
	   modified and you may want to save it back into the database */
/**** Commented out in order to not allow edits to help strings in defaultsedit
	if ( !(    defobjptr->nodename==NULL  
		|| strequal(defobjptr->nodename,"")
		|| strequal(curvalstr, nohelpavail) )  ) {
	    (void)strcpy(helplabel, defobjptr->nodename);
	    (void)convert_string(curvalstr, valuestr2);
	    (void)strcat(helplabel, "/$Help");
	    if (strequal(curvalstr, "") && defaults_exists(helplabel, (int *)NULL)) {
		defaults_remove(helplabel, (int *)NULL);
	    } else {
	    defaults_set_string(helplabel, valuestr2, (int *)NULL);
	    (void)strcpy(defobjptr->nodename, "");
	    }
	}
****/
	if (nodename != NULL) { (void)strcpy(defobjptr->nodename, nodename);
	} else { (void)strcpy(defobjptr->nodename, ""); }

	if (strlen(valuestr)>DE_HELP_TEXT_ALLOC_LENGTH-1) {
            deferror("Too long helpstring", False);
            (void)strcpy(valuestr2, "");
            }
        else defaults_str_unparse(valuestr, valuestr2);

        (void)panel_set(helpdisplayitem, 
		PANEL_LABEL_STRING,	labelname,
		PANEL_VALUE,		valuestr2,
		 0);

}



/* 
 * Return sequence number of current enum value,
 * where first enum always has enumeration value zero
 */
static int
get_seq_enum_nr(full_name)
	char*	full_name;
{
	char	accessname[DE_FULLNAME_MAX_LENGTH];
	char	childname[DE_FULLNAME_MAX_LENGTH];
	char	enumvalue[DE_NAME_ITEM_MAX_LENGTH];
	int	accesslen;
	int	childcount;
	
	(void)strcpy(enumvalue, defaults_get_string(full_name, "", (int *)NULL));
	(void)strcpy(accessname,full_name);
	accesslen = strlen(accessname)+1;

 	deflt_get_fullchild(accessname, childname);
	childcount = 0;
	while (childcount<1000 && !strequal(childname, "") && !strequal(childname+accesslen, enumvalue))  {
		if (childname[accesslen] != '$') childcount = childcount+1;
		deflt_get_fullsibling(accessname, childname, childname);
	}
	if (childcount>=1000) (void)fprintf(stderr, "Defaultsedit: Missing enumeration value for: %s \n", full_name);
	return childcount;
}




/*
 * Returns string name of emumeration item nr enumnr with nr zero first.
 * full_name is the name of the father node of the enumeration items.
 * Store the enumeration value string in straddress
 */
static char*
get_enum_member_string(full_name, enumnr, straddress)
	char*	full_name;
	int	enumnr;
	char*	straddress;		
{
	char	childname[DE_FULLNAME_MAX_LENGTH];
	int	childcount;
	int	fullnamelen;
	
 	deflt_get_fullchild(full_name, childname);
	childcount = -1;
	fullnamelen = strlen(full_name)+1;
	while (	! ((childname == NULL) || (strequal(childname, "") ) ) ) {
		if (childname[fullnamelen] != '$')
		     childcount = childcount+1;
		if (childcount==enumnr) break;
		deflt_get_fullsibling(full_name, childname, childname);
	}
	if (straddress == NULL) { 
		straddress = getchars(strlen(childname)-fullnamelen);
		}
	(void)strcpy(straddress, childname+fullnamelen);
	return straddress;
}


/*
 * Return the full pathname of the sibling of the node with name fullname.
 * If node does not exist then return "".
 * Store the full pathname into result.
 */
static void
deflt_get_fullsibling(parent, fullname, result)
    char* parent;
    char* fullname;
    char* result;
{
    int	    parentlen;
    char*   child;

    if (strequal(fullname, "")) {
	deferror("Defaultsedit: null child name in deflt_get_fullsibling", False);
	stop();
    }
    child = defaults_get_sibling(fullname, (int *)NULL);
    if (! strequal(parent, "/") ) parentlen = strlen(parent); 
    else parentlen=0;
    if (child==NULL) {
	(void)strcpy(result, "");  return;
	}
    if (fullname != result) (void)strcpy(result, fullname);
    (void)strcpy(result+parentlen+1,  child);
    return;
}



/*
 * Set fullname to the full pathname of the first child to parent if it exists.
 * The pathname is returned in fullname if it exists, otherwise "".
 * Store the full pathname into fullname.
 */
static void
deflt_get_fullchild(parent, fullname)
    char*   parent;
    char*   fullname;
{    
    char*   child;
    
    child = defaults_get_child(parent, (int *)NULL);
    if (child == NULL) {
	(void)strcpy(fullname, "");  return;
     };
    (void)strcpy(fullname, parent);
    if (! strequal(parent,"/")) (void)strcat(fullname, "/");
    (void)strcat(fullname, child);
    return;
}



/*
 * Allocate number of chars necessary for string of length nchars
 * which means we have to allocate an extra byte for the terminating
 * NULL-byte.
 */
static char*
getchars(nchars)
{
    return (char*) get_memory(nchars+1);
}


/*
 * If current node has a child with name $Enumeration,
 * then return Enumerate 
 * else if it has a child with name $Message then return Message
 * else return String
 */
static Defaulttype
get_nodetype(fullname)
    char*   fullname;
{
    char    tmpname[DE_FULLNAME_MAX_LENGTH];

    (void)strcpy(tmpname, fullname);
    (void)strcat(tmpname, "/$Enumeration");
    if (defaults_exists(tmpname, (int *)NULL)==True)
	 return Enumerate;
    (void)strcpy(tmpname, fullname);
    (void)strcat(tmpname, "/$Message");
    if (defaults_exists(tmpname, (int *)NULL)==True)
	 return Message;
    else return String;
    
}


/*
 * get_memory(size) will get size bytes of memory or die trying.
 */
static char *
get_memory(size)
	int		size;		/* Number of bytes to allocate */
{
	register char	*data;		/* New data */

	data = malloc((unsigned)size);
	if (data == NULL){
		(void)fprintf(stderr, "%s%d%s\n", "Defaultsedit: Can not allocate ",
			size, " bytes of memory (Out of swap space?)");
		bomb();
		/*NOTREACHED*/
	} else	return data;
}



/*
 * Free memory at non-NULL addresses
 */
static
free_mem(address)
{
    if (address!=NULL) free((char *)address);
}



/*
 * Check if any conversion program from a special format file 
 * ( like .mailrc to .defaults )
 * should be invoked before loading and displaying the defaults
 * information pertaining to subtreename.
 * subtreename may for example be "/Mail" or "/CShell"
 * Return True if running the conversion program succeeded or if
 * the conversion has already been done or if there is no conversion
 * specified for this subtreename.
 * Return False otherwise.
 */
static Bool
specialformat_to_defaults(subtreename)
    char*   subtreename;
{
    char    accessname[DE_FULLNAME_MAX_LENGTH];
    char    childforkname[DE_FULLNAME_MAX_LENGTH];
    char    message[DE_FULLNAME_MAX_LENGTH+20];
    union wait	    status_rec;
    union wait*	    status;

    status = &status_rec;
    (void)strcpy(accessname, subtreename);
    (void)strcat(accessname, "/$Specialformat_to_defaults");
    if (!defaults_exists(accessname, (int *)NULL)) return True;
    /*  If the name is on the list we have already converted to defaults
       format  */
    if (member_pos(subtreename, specformatnames) != -1) return True;
    (void)strcpy(childforkname, defaults_get_string(accessname, "", (int *)NULL));
    if (strequal(childforkname, "")) {
	show_message((char *)NULL, strcat(accessname,
                " - Missing conversion program name in Defaultsedit"), "");
	return False;
    }
    (void)strcpy(message, "Running conversion program ");

    (void)strcat(message, childforkname);
    show_message((char *)NULL, message, "");
    (void)start_child_proc(childforkname, True, False, status);
    if (status->w_T.w_Retcode!=0 ||
        status->w_T.w_Coredump!=0 || status->w_T.w_Termsig!=0) {
   	sleep(2);
	show_message((char *)NULL, "Conversion failed - defaults data not accessible","");
	return False;
    } else {
	show_message((char *)NULL, "", "");
	insert_new_member(subtreename, specformatnames);
	defaults_special_mode();        /* Actually reads the whole database  */
	/* defaults_reread(name, NULL);   Commented out temporarily because
	   of bug in defaults_reread  */
	return True;
    }
}






/*
 * Write out contents of defaults database.
 * Invoke any conversion programs to non-defaults format if necessary.
 */
static void
dump_default_trees()
{
    namerec**  namereclist;
    char    accessname[DE_FULLNAME_MAX_LENGTH];
    char    childforkname[DE_FULLNAME_MAX_LENGTH];
    char    message[DE_FULLNAME_MAX_LENGTH+20];

    show_message((char *)NULL, "Writing all modified defaults to the defaults database", "");
    defaults_write_changed((char *)NULL, (int *)NULL);
    namereclist = specformatnames;
    while (*namereclist != NULL) {
	if ((*namereclist)->changed ) {
	    (void)strcpy(accessname, (*namereclist)->name);
	    (void)strcat(accessname, "/$Defaults_to_specialformat");
            (void)strcpy(childforkname,defaults_get_string(accessname, "", (int *)NULL));
            if (strequal(childforkname, "")) {
	        deferror(strcat(accessname, " - Missing conversion program name in Defaultsedit"), False);
	        return;
	    }
	    (void)strcpy(message, "Running conversion program ");
	    (void)strcat(message, childforkname);
	    show_message((char *)NULL, message, "");
	    (void)start_child_proc(childforkname, True, False, (union wait *)NULL);
	    show_message((char *)NULL, "", "");
	    (*namereclist)->changed = False;
	}
	namereclist++;
    }
    show_message((char *)NULL, "", "");

}



/*
 * Return index position  name is a member of the list of names in namereclist,
 * which is a NULL-terminated array of namerec records.
 * The first member has index position nr 0.
 * If name is not a member of namereclist  then return -1
 */
static int
member_pos(name, namereclist)
    char*   name;
    namerec**  namereclist;
{
    int	    indexpos;
    
    indexpos = 0;
    while (*namereclist != NULL) {
	if (strequal((*namereclist)->name, name)) return indexpos;
	namereclist++;
	indexpos++;
    }
    return -1;
}


/*
 * Insert name as a new member of namereclist, the NULL-terminated 
 * list of names.
 */
static void
insert_new_member(name, namereclist)
    char*   name;
    namerec**  namereclist;
{
    
    while (*namereclist != NULL) {
	namereclist++;
    }
    *namereclist = (namerec*) (LINT_CAST(malloc(sizeof(namerec))));
    (*namereclist)->name = strcpy(getchars(strlen(name)), name);
    (*namereclist)->changed = False;
    namereclist++;
    *namereclist = (namerec*)NULL;
}

    

/*
 * Try to invoke a program with name childforkname in a subprocess.
 * Wait for termination of the child process if waitflg is True
 * Return the pid of the child process
 */
static int
start_child_proc(childforkname, waitflg, noforkflg, status)
    char*   childforkname;
    Bool    waitflg;
    Bool    noforkflg;
    union wait*	    status;
{
    char*	childargv[2];
    char*	childenv[1];
    int		pid;
    int		err;
    char	message[120];

    if (noforkflg) pid=0;	   /* Reuse the parent process */
    else  pid = vfork();	   /* or start child process to run demo in */
    if (pid == 0) {
        /* We are in the child process */
	  childargv[0] = childforkname;
	  childargv[1] = NULL;
	  childenv[0] = NULL;
	  err = execvp(childforkname, childargv, childenv);
	  if (err != 0) 
		(void)strcpy(message, "Defaultsedit: ");
		(void)strcat(message, childforkname);
		deferror(strcat(message, " failed to start."), False);
		/* Print error message and kill child process */
		sleep(2);
		_exit(1);   /* Important to call _exit instead of exit,
				otherwise stderr of parent will be closed */
    }
    if (waitflg)  wait_child_finish(pid, status);
    return pid;
}	    


/*
 * Wait until child finish processing.
 */
static void
wait_child_finish(childpid, status)
    int		childpid;
    union wait*	    status;
{
    int	    maxcalls;
    
    maxcalls = 1000;
    while (wait(status) != childpid  &&  maxcalls>0) {
    	maxcalls = maxcalls-1;
	/* Try again if we did not receive child's pid */
    }
    return;
}


/*
 * Default error message.
 * Program error printout (and termination if abortflg)
 */
static void
deferror(mess, abortflg)
	char *mess;
	Bool abortflg;
{
	fputs(mess, stderr);  fputs("\n", stderr);
	if (abortflg) bomb();
}



/*
 * bomb() will cause the program to bomb.
 */
static void
bomb()
{
	(void)fflush(stderr);
	sleep(3);	/* Wait for error message to show up */
	exit(1);
}





/*
 * convert_string(instring, outstring) will parse a C-style string from 
 * and store the result into outstring.
 * If any error occurs, NULL will be returned.
 */
static char*
convert_string(instring, outstring)
	char*	instring;		/* Input string to convert */
	char*	outstring;		/* Place to store string */
{
	int		chr;		/* Temporary character */
	register char	*pointer;	/* Pointer into string */

	pointer = outstring;
	while (True) {
		chr = *instring++;
		if (chr == '\0') break;
		if (chr == '\\'){
			chr = convert_backslash_chr(instring, &instring);
			if (chr == -1)
				return NULL;
		}
		*pointer++ = chr;
	}
	*pointer = '\0';
	return outstring;
}


/*
 * convert_backslash_chr(instrptr, outstrptr) will parse a C-style backslash
 * character from input string instrptr,
 * and return the corresponding int value.
 * Return the new string pointer in outstrptr.
 * OBS: Assumes that instrptr already points to the char beyond the backslash.
 */
static int
convert_backslash_chr(instrptr, outstrptr)
	register char*	instrptr;	/* Input string pointer */
	char**		outstrptr;	/* Output ptr when swallowed chars */
{
	register char	chr;		/* Temporary character */
	int		count;		/* Number of octal digits */
	int		number;		/* Number associated with character */

	chr = *instrptr++;
	switch (chr){
	    case 't':
		*outstrptr = instrptr;   return '\t';
	    case 'n':
		*outstrptr = instrptr;   return '\n';
	    case 'b':
		*outstrptr = instrptr;   return '\b';
	    case 'r':
		*outstrptr = instrptr;   return '\r';
	    case '\\':
		*outstrptr = instrptr;   return '\\';
	    case 'f':
	    	*outstrptr = instrptr;   return '\f';
	    case '\'':
		*outstrptr = instrptr;   return '\'';
	    case '\"':
		*outstrptr = instrptr;   return '"';
	    case '^':
		chr = *instrptr++;
		*outstrptr = instrptr;
		if (chr>='@' && chr <= '_') return chr-'@';
		if (chr>='`' && chr < '\177') return chr-'`';
		if (chr=='?') return '\177';
		return chr;
	    
	}
	number = 0;
	count = 0;
	while (count < 3){
		if (('0' <= chr) && (chr <= '7')){
			number = number * 8 + chr - '0';
			chr = *instrptr++;
			count++;
		} else {
			break;
		}
	}
	instrptr--;
	*outstrptr = instrptr;
	return (count == 0) ? -1 : number;
}



static void
tree_scrollup(ycoord, ydelta)
    int	    ycoord;
    int	    ydelta;
{
	Panel_item 	tmpitem;
	int		y;
	
	tmpitem = panel_get(treepanel, PANEL_FIRST_ITEM);
	while (tmpitem != NULL) {
	    y=(int)panel_get(tmpitem, PANEL_ITEM_Y);
	    if (y>ycoord) {
		(void)panel_set(tmpitem, PANEL_ITEM_Y, y-ydelta,
				PANEL_PAINT, PANEL_NONE,
			 0);
	    }
	   tmpitem = panel_get(tmpitem, PANEL_NEXT_ITEM);
	}
}




/*
 * Copies child nodes with values from fromnode to tonode.
 */
static void
subtree_copy(fromnode, tonode)
    char*   fromnode;
    char*   tonode;
{
    char    fullsibling1[DE_FULLNAME_MAX_LENGTH];
    char    fullsibling2[DE_FULLNAME_MAX_LENGTH];
    char    valuestr[DE_NAME_ITEM_MAX_LENGTH];
    char*   sibling;

    if (fromnode == NULL || strequal(fromnode, "")) return;
    (void)strcpy(valuestr, defaults_get_string(fromnode, "", (int *)NULL));
    defaults_set_string(tonode, valuestr, (int *)NULL);
    sibling = defaults_get_child(fromnode, (int *)NULL);
    while ( ! (sibling==NULL || strequal(sibling, ""))) {
	(void)strcpy(fullsibling1, fromnode);
	(void)strcat(fullsibling1, "/");
	(void)strcat(fullsibling1, sibling);
	(void)strcpy(fullsibling2, tonode);
	(void)strcat(fullsibling2, "/");
	(void)strcat(fullsibling2, sibling);
	subtree_copy(fullsibling1, fullsibling2);
	sibling = defaults_get_sibling(fullsibling1, (int *)NULL);
    }
}
