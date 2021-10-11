#ifndef lint
static char     sccsid[] = "@(#)%M 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */
#include "defs.h"
#include "text.h"
do_menu(ie)
	struct inputevent *ie;
{
	struct menuitem *mi;
	struct rect     r;
	struct inputevent end;
	if (mi = menu_display(&menu_ptr, ie, cards_sw->ts_windowfd)) {
		win_getsize(cards_sw->ts_windowfd, &r);
		switch (mi->mi_data) {
		case INSTRUCTIONS:
			pw_writebackground(cards_pixwin, 0, 0, r.r_width, r.r_height, PIX_CLR);
			pw_text(cards_pixwin, 0, 15, PIX_SRC, font, instructions1);
			pw_text(cards_pixwin, 0, 30, PIX_SRC, font, instructions2);
			pw_text(cards_pixwin, 0, 45, PIX_SRC, font, instructions3);
			pw_text(cards_pixwin, 0, 60, PIX_SRC, font, instructions4);
			pw_text(cards_pixwin, 0, 75, PIX_SRC, font, instructions5);
			pw_text(cards_pixwin, 0, 90, PIX_SRC, font, "\n");
			pw_text(cards_pixwin, 0, 105, PIX_SRC, font, instructions6);
			input_readevent(cards_sw->ts_windowfd, &end);
			redraw_board();
			break;
		case HOW_TO_BET:
			pw_writebackground(cards_pixwin, 0, 0, r.r_width, r.r_height, PIX_CLR);
			pw_text(cards_pixwin, 0, 15, PIX_SRC, font, betting1);
			pw_text(cards_pixwin, 0, 30, PIX_SRC, font, betting2);
			pw_text(cards_pixwin, 0, 45, PIX_SRC, font, betting3);
			pw_text(cards_pixwin, 0, 60, PIX_SRC, font, betting4);
			pw_text(cards_pixwin, 0, 75, PIX_SRC, font, betting5);
			pw_text(cards_pixwin, 0, 90, PIX_SRC, font, betting6);
			pw_text(cards_pixwin, 0, 105, PIX_SRC, font, betting7);
			pw_text(cards_pixwin, 0, 120, PIX_SRC, font, betting8);
			pw_text(cards_pixwin, 0, 135, PIX_SRC, font, betting9);
			pw_text(cards_pixwin, 0, 150, PIX_SRC, font, betting10);
			pw_text(cards_pixwin, 0, 165, PIX_SRC, font, betting11);
			pw_text(cards_pixwin, 0, 180, PIX_SRC, font, "\n");
			pw_text(cards_pixwin, 0, 195, PIX_SRC, font, betting12);
			input_readevent(cards_sw->ts_windowfd, &end);
			redraw_board();
			break;
		default:
			break;
		}
	}
}
kill_canfield()
{
	signal(SIGTERM, SIG_IGN);

	if (kill(0, SIGTERM) < 0)
		fprintf(stderr, "tool: kill failed\n");

	signal(SIGTERM, SIG_DFL);

}
/* panel functions */
quit_proc()
{
	panel_set(msg_item, PANEL_LABEL_STRING, "exiting...", 0);
	sleep(1);
	close_pipe();
	kill_canfield();
	exit(0);
}
clear_data()
{
	cf_fd = to_fd = 0;
	setup_pixrects();
	t1_num = t2_num = t3_num = t4_num = 0;
	found_num = 0;
	hand_num = 31;
	stock_num = 13;
	talon_num = 4;
	game_done = FALSE;
}
restart_proc()
{
	panel_set(msg_item, PANEL_LABEL_STRING, "Restarting game.", 0);
	close_pipe();
	kill_canfield();
	clear_data();
	cf_fd = start_canfield();
	pw_replrop(cards_pixwin, 0, 0, win_getwidth(cards_sw->ts_windowfd),
		   win_getheight(cards_sw->ts_windowfd), PIX_SRC, background_pr, 0, 0);
	pw_text(cards_pixwin, 193, 50, PIX_SRC | PIX_DST, font, "Foundation");
	pw_text(cards_pixwin, 65, 114, PIX_SRC | PIX_DST, font, "Stock");
	pw_text(cards_pixwin, 65, 242, PIX_SRC | PIX_DST, font, "Talon");
	pw_text(cards_pixwin, 65, 370, PIX_SRC | PIX_DST, font, "Hand");
	pw_text(cards_pixwin, 193, 178, PIX_SRC | PIX_DST, font, "Tableau");
}
deal_proc()
{
	panel_set(msg_item, PANEL_LABEL_STRING, "Dealing.", 0);
	write(to_fd, "h", 1);
	write(to_fd, "t", 1);
	if (hand_num >= 3)
		talon_num += 4;
	else
		talon_num = talon_num + hand_num + 1;
	hand_num -= 3;
	if (hand_num < 0)
		hand_num = 0;
}
my_pw_write(pw, x, y, wd, ht, op, pr, x1, y1)
	struct pixwin  *pw;
	struct pixrect *pr;
{
	if (pr)
		pw_write(pw, x, y, wd, ht, op, pr, x1, y1);
	else
		pw_replrop(pw, x, y, wd, ht, op, background_pr, x, y);
}
