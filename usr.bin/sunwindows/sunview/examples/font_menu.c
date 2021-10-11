/*****************************************************************************/
#ifndef lint
static char sccsid[] = "@(#)font_menu.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*****************************************************************************/

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/walkmenu.h>

void set_family(), set_size(), set_on_off(), toggle_on_off(), open_fonts();
Menu new_menu(), initialize_on_off();
char *int_to_str();
extern char * sprintf();
extern char * malloc();

Panel_item feedback_item;
char *family, *size, *bold, *italic;
Pixfont *cour, *serif, *apl, *cmr, *screen;

/*****************************************************************************/
/* main                                                                      */
/* First create the base frame, the feedback panel and feedback item. The    */
/* feedback item is initialized to "gallant 8".                              */
/* Then get the frame's menu, call new_menu() to create a new menu with the  */
/* original frame menu as a pullright, and give the new menu to the frame.   */
/*****************************************************************************/

main(argc, argv)
	int argc;
	char *argv[];
{   
    Frame frame;
    Panel panel;
    Menu  menu;
    int   defaults;
    
    frame = window_create(NULL, FRAME, FRAME_LABEL, "Menu Test -- Try frame menu.", 0);
    panel = window_create(frame, PANEL, WIN_ROWS, 1, 0);
    feedback_item = panel_create_item(panel, PANEL_MESSAGE, PANEL_LABEL_STRING, "", 0);

   family = "Gallant", size = "8", bold = italic = "";
   update_feedback();
    
    /* remember if user gave -d flag */
    if (argc >= 2) defaults = strcmp(argv[1], "-d") == 0;

    menu = (Menu)window_get(frame, WIN_MENU);
    menu = new_menu(menu, defaults);
    window_set(frame, WIN_MENU, menu, 0);
    
    window_main_loop(frame);

	exit(0);
	/* NOTREACHED */
}

/*****************************************************************************/
/* new_menu -- returns a new menu with 'original menu' as a pullright.       */
/*****************************************************************************/

Menu
new_menu(original_menu, defaults)
	Menu original_menu;
	int defaults;
{   
    Menu new_menu, family_menu, size_menu, on_off_menu;
    int i;
    
    /* create the on-off menu, which will be used as a pullright
     * for both the bold and italic items to the new menu. 
    */
    on_off_menu = menu_create(MENU_STRING_ITEM, "On", 1,
			 MENU_STRING_ITEM,      "Off", 0, 
			 MENU_GEN_PROC,         initialize_on_off, 
			 MENU_NOTIFY_PROC,      set_on_off, 
			 0);			 

    /* create the new menu which will eventually be returned */

    open_fonts(); /* first open the needed fonts */
    new_menu = menu_create(
		     MENU_PULLRIGHT_ITEM,
		       "Frame", 
		       original_menu,
		     MENU_PULLRIGHT_ITEM, 
		       "Family",
		       family_menu = menu_create(
			 MENU_ITEM,
			    MENU_STRING, "Courier",
			    MENU_FONT,   cour, 
                            0, 
			 MENU_ITEM,
			    MENU_STRING, "Serif",
			    MENU_FONT,   serif,
			    0, 
			 MENU_ITEM,
			    MENU_STRING, "aplAPLGIJ",
			    MENU_FONT,   apl,
			    0,
			 MENU_ITEM,
			    MENU_STRING, "CMR",
			    MENU_FONT,   cmr,
			    0, 

			 MENU_ITEM,
			    MENU_STRING, "Screen",
			    MENU_FONT,   screen,
			    0,
			 MENU_NOTIFY_PROC, set_family,
		       0),
		     MENU_PULLRIGHT_ITEM,
		       "Size", size_menu = menu_create(0), 
		     MENU_ITEM,
		       MENU_STRING,      "Bold",
		       MENU_PULLRIGHT,   on_off_menu, 
		       MENU_NOTIFY_PROC, toggle_on_off,
		       MENU_CLIENT_DATA, &bold, 
		       0, 
		     MENU_ITEM,
		       MENU_STRING,      "Italic",
		       MENU_PULLRIGHT,   on_off_menu, 
		       MENU_NOTIFY_PROC, toggle_on_off,
		       MENU_CLIENT_DATA, &italic,
		       0,
		     0);

    /* give each item in the family menu the size menu as a pullright */ 
    for (i = (int)menu_get(family_menu, MENU_NITEMS); i > 0; --i)
        menu_set(menu_get(family_menu, MENU_NTH_ITEM, i), 
		 MENU_PULLRIGHT, size_menu, 0);
	
    /* put non-selectable lines inbetween groups of items in family menu */
    menu_set(family_menu,
	     MENU_INSERT, 2, menu_create_item(MENU_STRING,     "-------",
					      MENU_INACTIVE,   TRUE, 
					      0), 
	     0);
    menu_set(family_menu, 
	     MENU_INSERT, 5, menu_get(family_menu, MENU_NTH_ITEM, 3),
	     0);
    
    /* The size menu was created with no items.  Now give it items representing */
    /*  the point sizes 8, 10, 12, 14, 16, and 18.                          */
    for (i = 8; i <= 18; i += 2) 
        menu_set(size_menu, MENU_STRING_ITEM, int_to_str(i), i, 0);

    /* give the size menu a notify proc to update the feedback */
    menu_set(size_menu, MENU_NOTIFY_PROC, set_size, 0);

    /* if the user did not give the -d flag, make all the menus come
     * up with the initial and default selections the last selected
     * item, and the initial selection selected.
    */
    if (!defaults) {
        menu_set(new_menu,
		 MENU_DEFAULT_SELECTION, MENU_SELECTED,
		 MENU_INITIAL_SELECTION, MENU_SELECTED,
		 MENU_INITIAL_SELECTION_SELECTED, TRUE, 
		 0);
        menu_set(family_menu,
		 MENU_DEFAULT_SELECTION, MENU_SELECTED,
		 MENU_INITIAL_SELECTION, MENU_SELECTED,
		 MENU_INITIAL_SELECTION_SELECTED, TRUE, 
		 0);
        menu_set(size_menu,
		 MENU_DEFAULT_SELECTION, MENU_SELECTED,
		 MENU_INITIAL_SELECTION, MENU_SELECTED,
		 MENU_INITIAL_SELECTION_SELECTED, TRUE, 
		 0);
        menu_set(on_off_menu,
		 MENU_DEFAULT_SELECTION, MENU_SELECTED,
		 MENU_INITIAL_SELECTION, MENU_SELECTED,
		 MENU_INITIAL_SELECTION_SELECTED, TRUE, 
		 0);
    }

    return (new_menu);    
}

/*****************************************************************************/
/* set_family -- notify proc for family menu.  Get the current family and    */
/* display it in the feedback panel.  Note that we first get the value       */
/* of the menu item.  This has the side effect of causing any pullrights     */
/* further to the right of mi to be evaluated.  Specifically, the value of   */
/* each family item is the value of its pullright -- namely the size menu.   */
/* When the size menu is evaluated, the notify proc set_size() is called,    */
/* which updates the feedback for the size.                                  */
/*****************************************************************************/

/*ARGSUSED*/
void
set_family(m, mi)
	Menu m;
	Menu_item mi;
{   
    menu_get(mi, MENU_VALUE);  /* force pullrights to be evaluated */
    family = menu_get(mi, MENU_STRING);
    update_feedback();
}


/*****************************************************************************/
/* set_size -- notify proc for the size menu.                                */
/*****************************************************************************/

/*ARGSUSED*/
void
set_size(m, mi)
	Menu m;
	Menu_item mi;
{   
    size =  menu_get(mi, MENU_STRING);
    update_feedback();
}

/****************************************************************************/
/* initialize_on_off -- generate proc for the on_off menu.                   */
/* The on-off menu is a pullright of both the bold and the italic menus.     */
/* We want it to toggle -- if its parent was on, it should come up with      */
/* "Off" selected, and vice-versa.  We can do that by first getting the      */
/* parent menu item, then, indirectly through its client data attribute,     */
/* seeing if the string representing the bold or italic state is null.       */
/* If the string was null, we set the first item ("On") to be selected,      */
/* else we set the second item ("Off") to be selected.                       */
/*****************************************************************************/

Menu
initialize_on_off(m, op)
	Menu m; Menu_generate op;
{   
    Menu_item parent_mi;
    char **name;
    
    if (op != MENU_DISPLAY) return (m);	

    parent_mi =	(Menu_item)menu_get(m, MENU_PARENT);
    name = (char **)menu_get(parent_mi, MENU_CLIENT_DATA);

    if (**name == NULL)
        menu_set(m, MENU_SELECTED, 1, 0);
    else
        menu_set(m, MENU_SELECTED, 2, 0);
    return (m);	
}

/*****************************************************************************/
/* set_on_off -- notify proc for on-off menu.                                */
/* Set the feedback string -- italic or bold -- appropriately depending on   */
/* the current setting.  Note that the "On" item was created to return a     */
/* value of 1, and the "Off" item will return a value of 0.                  */
/*****************************************************************************/

void
set_on_off(m, mi)
	Menu m; Menu_item mi;
{   
    Menu_item parent_mi;
    char **name;
    
    parent_mi =	(Menu_item)menu_get(m, MENU_PARENT);
    name = (char **)menu_get(parent_mi, MENU_CLIENT_DATA);
    if (menu_get(mi, MENU_VALUE))
        *name = (char *)menu_get(parent_mi, MENU_STRING);
    else
        *name = "";
    update_feedback();
}

/*****************************************************************************/
/* toggle_on_off -- notify proc for the "Bold" and "Italic" menu items.      */
/* Using a notify proc for the menu item allows toggling without bringing    */
/* up the on-off pullright.                                                  */
/*****************************************************************************/

/*ARGSUSED*/
void
toggle_on_off(m, mi)
	Menu m;
	Menu_item mi;
{   
    char **name;

    name = (char **)menu_get(mi, MENU_CLIENT_DATA);

    if (**name == NULL)
	*name = (char *)menu_get(mi, MENU_STRING);
    else
	*name = "";

    update_feedback();
}

update_feedback()
{   
    char buf[30];
    
    sprintf(buf, "%s %s %s %s", bold, italic, family, size);
    panel_set(feedback_item, PANEL_LABEL_STRING, buf, 0);
}

char *
int_to_str(n)
{   
    char *r = malloc(4);
    sprintf(r, "%d", n);
    return (r);
}

void
open_fonts()
{   
    cour = pf_open("/usr/lib/fonts/fixedwidthfonts/cour.r.10");
    serif = pf_open("/usr/lib/fonts/fixedwidthfonts/serif.r.10");
    apl = pf_open("/usr/lib/fonts/fixedwidthfonts/apl.r.10");
    cmr = pf_open("/usr/lib/fonts/fixedwidthfonts/cmr.b.8");
    screen = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.11");
}
