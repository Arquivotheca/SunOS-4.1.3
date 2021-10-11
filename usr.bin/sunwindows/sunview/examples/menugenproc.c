/*****************************************************************************/
#ifndef lint
static char sccsid[] = "@(#)menugenproc.c 1.1 92/07/30 SMI";
#endif 
/*****************************************************************************/

/* menu generate proc example from the SVPG menu chapter */
#include <suntool/sunview.h>
#include <suntool/walkmenu.h>

static void     initialize_menu();
static Menu     list_files();
static char   **get_file_names();

static Menu     m;
static char    *array[3][4] =   {
                        { "List" , "dot" , "files" , 0 },
                        { "list" , "bin" , "dir" ,   0 },
                        { "list" , "all" , "files" , 0}
};

main()
{
        Frame   frame;
        Menu    menu, original_menu;
        frame = window_create(NULL, FRAME,
                      FRAME_LABEL,  "Menu Gen Proc example -- Bring up menu",
                      0);
        original_menu = (Menu)window_get(frame, WIN_MENU);
        menu = menu_create(MENU_PULLRIGHT_ITEM,  "Frame",  original_menu,
                           0);
        initialize_menu(menu);
        window_set(frame, WIN_MENU, menu, 0);

        window_main_loop(frame);
	exit(0);
	/* NOTREACHED */
}

static char **
get_file_names(directory)
   int  directory;
{
    return (array[directory]);
}

/* What follows is in the SVPG */

#define DOT  0
#define BIN  1
#define ALL  2

static void
initialize_menu(menu)
    Menu  menu;
{
    m = menu_create(MENU_GEN_PROC,    list_files,
                    MENU_CLIENT_DATA, DOT,
                    0);
    menu_set(menu,
             MENU_PULLRIGHT_ITEM, "List dot files", m,
             0);
    m = menu_create(MENU_GEN_PROC,    list_files,
                    MENU_CLIENT_DATA, BIN,
                    0);
    menu_set(menu,
             MENU_PULLRIGHT_ITEM, "List bin dir", m,
             0);
    m = menu_create(MENU_GEN_PROC,    list_files,
                    MENU_CLIENT_DATA, ALL,
                    0);
    menu_set(menu,
             MENU_PULLRIGHT_ITEM, "List all files", m,
             0);
}

static Menu
list_files(m, operation)
    Menu          m;
    Menu_generate operation;
{
    char **list;
    int    directory;
    int    i = 0;

    switch (operation) {
      case MENU_DISPLAY:
        directory = (int)menu_get(m, MENU_CLIENT_DATA);
        list = get_file_names(directory);
        while (*list)
            menu_set(m,
                     MENU_STRING_ITEM, *list++, i++,
                     0);
        break;

      case MENU_DISPLAY_DONE:
        /*
         * Destroy old menu and all its entries.
         * Replace it with a new menu.
         */
        directory = (int)menu_get(m, MENU_CLIENT_DATA);
        menu_destroy(m);
        m = menu_create(MENU_GEN_PROC,    list_files,
                        MENU_CLIENT_DATA, directory,
                        0);
        break;

      case MENU_NOTIFY:
      case MENU_NOTIFY_DONE:
        break;
    }

    /* The current or newly-created menu is returned */
    return m;
}
