#ifndef lint
static char     sccsid[] = "@(#)misc.c 1.1 92/07/30 Copry 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */
#include "defs.h"
#include "cards.h"
#include <ctype.h>
#define CARD_NUM	4

struct pixrect *clubs_ace_pr = &clubs_ace_mpr, *clubs_two_pr = &clubs_two_mpr, *clubs_three_pr = &clubs_three_mpr, *clubs_four_pr = &clubs_four_mpr, *clubs_five_pr = &clubs_five_mpr, *clubs_six_pr = &clubs_six_mpr, *clubs_seven_pr = &clubs_seven_mpr, *clubs_eight_pr = &clubs_eight_mpr, *clubs_nine_pr = &clubs_nine_mpr, *clubs_ten_pr = &clubs_ten_mpr, *clubs_jack_pr = &clubs_jack_mpr, *clubs_queen_pr = &clubs_queen_mpr, *clubs_king_pr = &clubs_king_mpr;

struct pixrect *diamonds_ace_pr = &diamonds_ace_mpr, *diamonds_two_pr = &diamonds_two_mpr, *diamonds_three_pr = &diamonds_three_mpr, *diamonds_four_pr = &diamonds_four_mpr, *diamonds_five_pr = &diamonds_five_mpr, *diamonds_six_pr = &diamonds_six_mpr, *diamonds_seven_pr = &diamonds_seven_mpr, *diamonds_eight_pr = &diamonds_eight_mpr, *diamonds_nine_pr = &diamonds_nine_mpr, *diamonds_ten_pr = &diamonds_ten_mpr, *diamonds_jack_pr = &diamonds_jack_mpr, *diamonds_queen_pr = &diamonds_queen_mpr, *diamonds_king_pr = &diamonds_king_mpr;

struct pixrect *hearts_ace_pr = &hearts_ace_mpr, *hearts_two_pr = &hearts_two_mpr, *hearts_three_pr = &hearts_three_mpr, *hearts_four_pr = &hearts_four_mpr, *hearts_five_pr = &hearts_five_mpr, *hearts_six_pr = &hearts_six_mpr, *hearts_seven_pr = &hearts_seven_mpr, *hearts_eight_pr = &hearts_eight_mpr, *hearts_nine_pr = &hearts_nine_mpr, *hearts_ten_pr = &hearts_ten_mpr, *hearts_jack_pr = &hearts_jack_mpr, *hearts_queen_pr = &hearts_queen_mpr, *hearts_king_pr = &hearts_king_mpr;

struct pixrect *spades_ace_pr = &spades_ace_mpr, *spades_two_pr = &spades_two_mpr, *spades_three_pr = &spades_three_mpr, *spades_four_pr = &spades_four_mpr, *spades_five_pr = &spades_five_mpr, *spades_six_pr = &spades_six_mpr, *spades_seven_pr = &spades_seven_mpr, *spades_eight_pr = &spades_eight_mpr, *spades_nine_pr = &spades_nine_mpr, *spades_ten_pr = &spades_ten_mpr, *spades_jack_pr = &spades_jack_mpr, *spades_queen_pr = &spades_queen_mpr, *spades_king_pr = &spades_king_mpr;
redraw_board()
{
	register int    i;
	if (game_done)
		pw_replrop(cards_pixwin, 0, 0, win_getwidth(cards_sw->ts_windowfd),
			   win_getheight(cards_sw->ts_windowfd), PIX_SRC, background_pr, 0, 0);
	else {
		pw_replrop(cards_pixwin, 0, 0, win_getwidth(cards_sw->ts_windowfd),
			   win_getheight(cards_sw->ts_windowfd), PIX_SRC, background_pr, 0, 0);
		pw_text(cards_pixwin, 193, 50, PIX_SRC | PIX_DST, font, "Foundation");
		pw_text(cards_pixwin, 65, 114, PIX_SRC | PIX_DST, font, "Stock");
		pw_text(cards_pixwin, 65, 242, PIX_SRC | PIX_DST, font, "Talon");
		pw_text(cards_pixwin, 65, 370, PIX_SRC | PIX_DST, font, "Hand");
		pw_text(cards_pixwin, 193, 178, PIX_SRC | PIX_DST, font, "Tableau");

		my_pw_write(cards_pixwin, 193, 65, 55, 64, PIX_SRC, f1_pixrect, 0, 0);
		my_pw_write(cards_pixwin, 321, 65, 55, 64, PIX_SRC, f2_pixrect, 0, 0);
		my_pw_write(cards_pixwin, 449, 65, 55, 64, PIX_SRC, f3_pixrect, 0, 0);
		my_pw_write(cards_pixwin, 577, 65, 55, 64, PIX_SRC, f4_pixrect, 0, 0);

		redraw_stock(1);
		redraw_talon(1);
		redraw_hand(1);

		if (t1_num > 0)
			for (i = 0; i < t1_num; i++)
				my_pw_write(cards_pixwin, 193, (193 + (16 * i)),
				   55, 64, PIX_SRC, t1_pixrect[i], 0, 0);
		if (t2_num > 0)
			for (i = 0; i < t2_num; i++)
				my_pw_write(cards_pixwin, 321, (193 + (16 * i)),
				   55, 64, PIX_SRC, t2_pixrect[i], 0, 0);
		if (t3_num > 0)
			for (i = 0; i < t3_num; i++)
				my_pw_write(cards_pixwin, 449, (193 + (16 * i)),
				   55, 64, PIX_SRC, t3_pixrect[i], 0, 0);
		if (t4_num > 0)
			for (i = 0; i < t4_num; i++)
				my_pw_write(cards_pixwin, 577, (193 + (16 * i)),
				   55, 64, PIX_SRC, t4_pixrect[i], 0, 0);
	}
}
redraw_stock(x)
	int             x;
{
	static int      o_num;
	register int    i, j;
	if ((stock_num < o_num) || (x == 1)) {
		pw_replrop(cards_pixwin, 0, 127, 120, 70, PIX_SRC,
			   background_pr, 0, 127);
	}
	i = stock_num / CARD_NUM;
	for (j = i; j > 0; j--)
		pw_write(cards_pixwin, (65 - j), (129 + j), 55, 64,
			 PIX_SRC, outline_pr, 0, 0);

	if (stock_num > 0)
		my_pw_write(cards_pixwin, 65, 129, 55, 64, PIX_SRC,
			    stock_pixrect, 0, 0);
	else
		pw_replrop(cards_pixwin, 65, 129, 55, 64, PIX_SRC,
			   background_pr, 65, 129);
	o_num = stock_num;
}
redraw_talon(x)
	int             x;
{
	static int      o_num;
	register int    i, j;
	if ((talon_num < o_num) || (x == 1)) {
		pw_replrop(cards_pixwin, 0, 256, 120, 75, PIX_SRC,
			   background_pr, 0, 256);
	}
	i = talon_num / CARD_NUM;
	for (j = i; j > 0; j--)
		pw_write(cards_pixwin, (65 - j), (257 + j), 55, 64,
			 PIX_SRC, outline_pr, 0, 0);

	if (talon_num > 0)
		my_pw_write(cards_pixwin, 65, 257, 55, 64, PIX_SRC,
			    talon_pixrect, 0, 0);
	else
		pw_replrop(cards_pixwin, 65, 257, 55, 64, PIX_SRC,
			   background_pr, 65, 257);
	o_num = talon_num;
}
redraw_hand(x)
	int             x;
{
	static int      o_num;
	register int    i, j;
	hand_num = (52 - (talon_num + stock_num + found_num + t1_num +
			  t2_num + t3_num + t4_num));
	if (hand_num < 0) {
		talon_num += hand_num;
		hand_num = 0;
	}
	if ((hand_num < o_num) || (x == 1)) {
		pw_replrop(cards_pixwin, 0, 384, 120, 75, PIX_SRC,
			   background_pr, 0, 384);
	}
	i = hand_num / CARD_NUM;
	for (j = i; j > 0; j--)
		pw_write(cards_pixwin, (65 - j), (385 + j), 55, 64,
			 PIX_SRC, outline_pr, 0, 0);
	/*
	 * always write the back icon, so users have something to aim at
	 * ... 
	 */
	if ((talon_num == 0) && (hand_num == 0))
		pw_replrop(cards_pixwin, 65, 385, 55, 64, PIX_SRC,
			   background_pr, 65, 385);
	else
		pw_write(cards_pixwin, 65, 385, 55, 64, PIX_SRC,
			 backicon.ic_mpr, 0, 0);
	o_num = hand_num;
}
sigwinched()
{
	tool_sigwinch(tool);
}
handle_sigwinch()
{
	pw_damaged(cards_pixwin);
	redraw_board();
	pw_donedamaged(cards_pixwin);
}
/*
 * this function assignes the appropriate value to an pix, given the card. 
 */
struct pixrect *
get_pixrect(card_name)
	char            card_name[2];
{
	struct pixrect *pix;
	if (card_name[0] == NULL) {
		return NULL;
	}
	switch (card_name[0]) {

	case 'c':
		switch (card_name[1]) {
		case 'A':
			pix = clubs_ace_pr;
			break;
		case '2':
			pix = clubs_two_pr;
			break;
		case '3':
			pix = clubs_three_pr;
			break;
		case '4':
			pix = clubs_four_pr;
			break;
		case '5':
			pix = clubs_five_pr;
			break;
		case '6':
			pix = clubs_six_pr;
			break;
		case '7':
			pix = clubs_seven_pr;
			break;
		case '8':
			pix = clubs_eight_pr;
			break;
		case '9':
			pix = clubs_nine_pr;
			break;
		case '0':
			pix = clubs_ten_pr;
			break;
		case 'J':
			pix = clubs_jack_pr;
			break;
		case 'Q':
			pix = clubs_queen_pr;
			break;
		case 'K':
			pix = clubs_king_pr;
			break;
		}
		break;
	case 'd':
		switch (card_name[1]) {
		case 'A':
			pix = diamonds_ace_pr;
			break;
		case '2':
			pix = diamonds_two_pr;
			break;
		case '3':
			pix = diamonds_three_pr;
			break;
		case '4':
			pix = diamonds_four_pr;
			break;
		case '5':
			pix = diamonds_five_pr;
			break;
		case '6':
			pix = diamonds_six_pr;
			break;
		case '7':
			pix = diamonds_seven_pr;
			break;
		case '8':
			pix = diamonds_eight_pr;
			break;
		case '9':
			pix = diamonds_nine_pr;
			break;
		case '0':
			pix = diamonds_ten_pr;
			break;
		case 'J':
			pix = diamonds_jack_pr;
			break;
		case 'Q':
			pix = diamonds_queen_pr;
			break;
		case 'K':
			pix = diamonds_king_pr;
			break;
		}
		break;
	case 's':
		switch (card_name[1]) {
		case 'A':
			pix = spades_ace_pr;
			break;
		case '2':
			pix = spades_two_pr;
			break;
		case '3':
			pix = spades_three_pr;
			break;
		case '4':
			pix = spades_four_pr;
			break;
		case '5':
			pix = spades_five_pr;
			break;
		case '6':
			pix = spades_six_pr;
			break;
		case '7':
			pix = spades_seven_pr;
			break;
		case '8':
			pix = spades_eight_pr;
			break;
		case '9':
			pix = spades_nine_pr;
			break;
		case '0':
			pix = spades_ten_pr;
			break;
		case 'J':
			pix = spades_jack_pr;
			break;
		case 'Q':
			pix = spades_queen_pr;
			break;
		case 'K':
			pix = spades_king_pr;
			break;
		}
		break;
	case 'h':
		switch (card_name[1]) {
		case 'A':
			pix = hearts_ace_pr;
			break;
		case '2':
			pix = hearts_two_pr;
			break;
		case '3':
			pix = hearts_three_pr;
			break;
		case '4':
			pix = hearts_four_pr;
			break;
		case '5':
			pix = hearts_five_pr;
			break;
		case '6':
			pix = hearts_six_pr;
			break;
		case '7':
			pix = hearts_seven_pr;
			break;
		case '8':
			pix = hearts_eight_pr;
			break;
		case '9':
			pix = hearts_nine_pr;
			break;
		case '0':
			pix = hearts_ten_pr;
			break;
		case 'J':
			pix = hearts_jack_pr;
			break;
		case 'Q':
			pix = hearts_queen_pr;
			break;
		case 'K':
			pix = hearts_king_pr;
			break;
		}
		break;
	case 'p':		/* pile of cards */
		pix = outline_pr;
		break;
	default:
		break;
	}
	return pix;
}
win_proc()
{
	panel_set(msg_item, PANEL_LABEL_STRING, "You've Won!", 0);
	sleep(2);
	panel_set(msg_item, PANEL_LABEL_STRING, "Congratulations!", 0);
	sleep(5);
	panel_set(msg_item, PANEL_LABEL_STRING, "Hit  the left mouse button to start a new game, quit otherwise.", 0);
	sleep(5);
	game_done = TRUE;
}
