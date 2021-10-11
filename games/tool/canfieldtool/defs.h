
/* @(#)defs.h 1.1 92/07/30 SMI	 */
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */
#include <stdio.h>
#include <suntool/tool_hs.h>
#include <suntool/panel.h>
#include <suntool/menu.h>
#include <sunwindow/window_hs.h>
#include <sys/signal.h>

#define	TOOLHEIGHT	512
#define TOOLWIDTH	704

#define INSTRUCTIONS	(caddr_t) 'i'
#define HOW_TO_BET	(caddr_t) 'h'

#define TRUE	1
#define FALSE	0

#define CANFIELD "/usr/games/canfield"

char            f1[2], f2[2], f3[2], f4[2], stock[2], talon[2];
char            t1[13][2], t2[13][2], t3[13][2], t4[13][2];
struct tool    *tool;
struct pixfont *font;
struct toolsw  *panel_sw, *cards_sw;
struct pixwin  *cards_pixwin;
struct pixrect *talon_pixrect, *stock_pixrect;
struct pixrect *f1_pixrect, *f2_pixrect, *f3_pixrect, *f4_pixrect;
struct pixrect *t1_pixrect[13], *t2_pixrect[13], *t3_pixrect[13], *t4_pixrect[13];
char           *canfield;
struct icon     toolicon, backicon;
Panel           panel;
Panel_item      msg_item, quit, restart;
char           *card, *position, *new_position, *old_position;
int             ppid, pid, cf_fd, to_fd, to[2], from[2], game_done;
int             t1_num, t2_num, t3_num, t4_num;
int             old_t1_num, old_t2_num, old_t3_num, old_t4_num;
int             talon_num, stock_num, hand_num, found_num;
int             old_talon_num, old_stock_num, old_hand_num, old_found_num;
int
quit_proc(), restart_proc(), where();
int
decode_position(), update_board(), redraw_board(), mouse_reader();
int
sigwinched(), close_pipe(), setup_cards(), (*old_selected) ();
int
read_board(), cf_reader(), handle_sigwinch();
struct pixrect *get_pixrect();

struct pixrect *outline_pr;
struct pixrect *background_pr;

static struct menuitem can_menu_items[] = {
			{MENU_IMAGESTRING, "Instructions", INSTRUCTIONS},
			    {MENU_IMAGESTRING, "How to Bet", HOW_TO_BET},
};
static struct menu can_menu = {
			       MENU_IMAGESTRING, "Canfield Tool",
			sizeof(can_menu_items) / sizeof(struct menuitem),
		     can_menu_items, (struct menu *) NULL, (caddr_t) NULL
};
static struct menu *menu_ptr = &can_menu;
