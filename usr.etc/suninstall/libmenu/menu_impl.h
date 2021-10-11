/*
 * @(#)menu_impl.h 1.1 92/07/30
 */

/*
 *	Name:		menu_impl.h
 *
 *	Description:	Global implementation dependent definitions for menu
 *		library.  This file should only be included by the routines
 *		in the menu library.
 */


/*
 *	Global constants:
 */
#define C_BACKWARD	CTRL(b)		/* move backward */
#define C_FORWARD	CTRL(f)		/* move forward */
#define C_HELP		'?'		/* invoke help system */
#define C_REDRAW	CTRL(l)		/* redraw screen */
#define ITEM_OFFSET	(-4)		/* menu item check status offset */
#define MENU_CLEAR	1		/* clear the screen */
#define MENU_GOTO_OBJ	(-2)		/* goto an object, status is okay */
#define MENU_IGNORE_OBJ	(-3)		/* ignore an object */
#define MENU_NO_CLEAR	0		/* do not clear the screen */

/*
 *	Menu object types.  If you add an object type here, then make
 *	sure you update add_string.c, free_string.c and the appropriate
 *	*form.c and *menu.c routines.
 */
#define MENU_OBJ_BUTTON	0xAE50		/* radio panel button */
#define MENU_OBJ_NOECHO 0xAE51
#define MENU_OBJ_FIELD	0xAE52		/* form field */
#define MENU_OBJ_FILE	0xAE53		/* menu file */
#define MENU_OBJ_FORM	0xAE54		/* form */
#define MENU_OBJ_ITEM	0xAE55		/* menu item */
#define MENU_OBJ_MENU	0xAE56		/* menu */
#define MENU_OBJ_RADIO	0xAE57		/* form radio panel */
#define MENU_OBJ_YESNO	0xAE58		/* form yes/no question */

#define MENU_REPEAT_OBJ	(-4)		/* repeat an object */
#define MENU_SKIP_IO	(-5)		/* skip I/O on an object */
#define MENU_EXIT_MENU	(-6)		/*  exit the menu */
#define MENU_GET_YN	(-7)		/* require that we get a y/n answer */




/*
 *	Definition of a core menu structure
 *
 *		m_type		- type of menu object
 *		m_help		- pointer to the help function
 */
typedef struct menu_core_t {
	int		m_type;
	int (*		m_help)();
} menu_core;
