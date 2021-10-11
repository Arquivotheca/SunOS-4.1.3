#ifndef lint
static char     sccsid[] = "@(#)%M 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */
#include "defs.h"
#include <ctype.h>
/*
 * This function decodes the position of the card sent to canfieldtool 
 */
decode_position(code)
	char            code[5];
{
	register int    num[2];
	register int    i;
	if ((code[0] == 'N') && (code[1] == 'G')) {
		panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move!", 0);
		t1_num = old_t1_num;
		t2_num = old_t2_num;
		t3_num = old_t3_num;
		t4_num = old_t4_num;
		stock_num = old_stock_num;
		talon_num = old_talon_num;
		redraw_stock();
		redraw_talon();
		redraw_hand();
		return;
	}
	if ((code[0] == 'L') && (code[1] == 'O') && (code[2] == 'S') && (code[3] == 'T')) {
		panel_set(msg_item, PANEL_LABEL_STRING, "You've Lost!", 0);
		sleep(3);
		panel_set(msg_item, PANEL_LABEL_STRING, "Hit the left mouse button to start a new game, otherwise hit 'Quit'.", 0);
		game_done = TRUE;
		return;
	}
	if ((code[0] == 'W') && (code[1] == 'I') && (code[2] == 'N')) {
		win_proc();
	}
	if ((code[0] == 'H') && (code[1] == 'A') && (code[2] == 'N') && (code[3] == 'D')) {
		panel_set(msg_item, PANEL_LABEL_STRING, "Talon is now the new hand", 0);
		talon_num = 0;
		hand_num = (52 - (stock_num + found_num + t1_num + t2_num +
				  t3_num + t4_num));
		if (hand_num >= 3) {
			talon_num = 4;
			hand_num -= 3;
		} else {
			talon_num = hand_num + 1;
			hand_num = 0;
		}
		redraw_talon();
		redraw_hand();
		return;
	}
	if ((code[0] == 'S') && (code[1] == 'T') && (code[2] == 'O') && (code[3] == 'C')) {
		panel_set(msg_item, PANEL_LABEL_STRING, "Stock is empty", 0);
		stock_num = 0;
		stock[0] = ' ';
		stock[1] = ' ';
		stock_pixrect = NULL;
		redraw_stock();
	}
	if ((code[0] == 'T') && (code[1] == 'A') && (code[2] == 'L') && (code[3] == 'O')) {
		panel_set(msg_item, PANEL_LABEL_STRING, "Talon is empty", 0);
		talon_num = 0;
		talon[0] = ' ';
		talon[1] = ' ';
		talon_pixrect = NULL;
		redraw_talon();
		redraw_hand();
	}
	if ((code[0] == 'E') && (code[1] == 'M') && (code[2] == 'P') && (code[3] == 'T')) {
		panel_set(msg_item, PANEL_LABEL_STRING, "Stock and talon are empty", 0);
		stock_num = 0;
		stock[0] = ' ';
		stock[1] = ' ';
		stock_pixrect = NULL;
		redraw_stock();
		talon_num = 0;
		talon[0] = ' ';
		talon[1] = ' ';
		talon_pixrect = NULL;
		redraw_talon();
		redraw_hand();
		return;
	}
	num[0] = toascii(code[0]) - toascii('0');

	switch (code[1]) {
	case 'a':
		num[1] = 3;
		break;
	case 'b':
		num[1] = 8;
		break;
	case 'c':
		num[1] = 9;
		break;
	case 'd':
		num[1] = 10;
		break;
	case 'e':
		num[1] = 11;
		break;
	case 'f':
		num[1] = 12;
		break;
	case 'g':
		num[1] = 13;
		break;
	case 'h':
		num[1] = 14;
		break;
	case 'i':
		num[1] = 15;
		break;
	case 'j':
		num[1] = 16;
		break;
	case 'k':
		num[1] = 17;
		break;
	case 'l':
		num[1] = 18;
		break;
	case 'm':
		num[1] = 19;
		break;
	case 'n':
		num[1] = 20;
		break;
	case 'o':
		num[1] = 21;
		break;
	case 'z':		/* bad news bears */
		num[1] = 1;
		break;
	default:		/* ditto */
		num[1] = 1;
		break;
	}

	if ((code[2] == 'R') && (code[3] == 'M')) {
#ifdef DEBUG
		fprintf(stderr, "tool: removing card x = %d -- y = %d\n", num[0], num[1]);
#endif
		switch (num[0]) {
		case 0:
			if ((num[1] == 21) && (old_position[1] != 'a')) {
				talon[0] = ' ';
				talon[1] = ' ';
				talon_pixrect = NULL;
				talon_num--;
				if (talon_num <= 0) {
					if (hand_num > 0) {
						talon_num++;
						hand_num--;
					} else
						talon_num = 0;
				}
				redraw_hand();
				redraw_talon();
			}
			break;
		case 1:
			if (old_position[0] == 's') {
				stock[0] = ' ';
				stock[1] = ' ';
				stock_pixrect = NULL;
				redraw_stock();
			} else if (old_position[1] == 'a') {
				talon[0] = ' ';
				talon[1] = ' ';
				talon_pixrect = NULL;
				if (talon_num <= 0)
					if (hand_num > 0) {
						hand_num--;
						talon_num++;
					}
				talon_num--;
				redraw_hand();
				redraw_talon();
			}
			break;
		case 2:
			if ((num[1] == 8) && (t1_num == 1)) {
				t1[0][0] = ' ';
				t1[0][1] = ' ';
				t1_pixrect[0] = NULL;
				my_pw_write(cards_pixwin, 193, 193, 55, 64,
					    PIX_SRC, t1_pixrect[0], 0, 0);
				t1_num = 0;
			}
			break;
		case 3:
			if ((num[1] == 8) && (t2_num == 1)) {
				t2[0][0] = ' ';
				t2[0][1] = ' ';
				t2_pixrect[0] = NULL;
				my_pw_write(cards_pixwin, 321, 193, 55, 64,
					    PIX_SRC, t2_pixrect[0], 0, 0);
				t2_num = 0;
			}
			break;
		case 4:
			if ((num[1] == 8) && (t3_num == 1)) {
				t3[0][0] = ' ';
				t3[0][1] = ' ';
				t3_pixrect[0] = NULL;
				my_pw_write(cards_pixwin, 449, 193, 55, 64,
					    PIX_SRC, t3_pixrect[0], 0, 0);
				t3_num = 0;
			}
			break;
		case 5:
			if ((num[1] == 8) && (t4_num == 1)) {
				t4[0][0] = ' ';
				t4[0][1] = ' ';
				t4_pixrect[0] = NULL;
				my_pw_write(cards_pixwin, 577, 193, 55, 64,
					    PIX_SRC, t4_pixrect[0], 0, 0);
				t4_num = 0;
			}
			break;
		}

	} else {

		switch (num[0]) {
		case 1:
			if (num[1] == 8) {
				stock[0] = code[2];
				stock[1] = code[3];
				stock_pixrect = get_pixrect(stock);
				redraw_stock();
			} else if (num[1] == 13) {
				talon[0] = code[2];
				talon[1] = code[3];
				talon_pixrect = get_pixrect(talon);
				redraw_talon();
				redraw_hand();
			}
			break;
		case 2:	/* 1st foundation & tableau column */
			if (num[1] == 3) {
				found_num++;
				f1[0] = code[2];
				f1[1] = code[3];
				f1_pixrect = get_pixrect(f1);
				my_pw_write(cards_pixwin, 193, 65, 55, 64,
					    PIX_SRC, f1_pixrect, 0, 0);
			}
			if (num[1] >= 8) {
				i = num[1] - 8;
				t1[i][0] = code[2];
				t1[i][1] = code[3];
				t1_num = i + 1;
				for (i = 0; i < t1_num; i++)
					t1_pixrect[i] = get_pixrect(t1[i]);
				if (t1_num > 0)
					for (i = 0; i < t1_num; i++)
						my_pw_write(cards_pixwin, 193,
							    (193 + (16 * i)), 55, 64, PIX_SRC,
						    t1_pixrect[i], 0, 0);
			}
			break;
		case 3:	/* 2nd foundation & tableau column */
			if (num[1] == 3) {
				found_num++;
				f2[0] = code[2];
				f2[1] = code[3];
				f2_pixrect = get_pixrect(f2);
				my_pw_write(cards_pixwin, 321, 65, 55, 64,
					    PIX_SRC, f2_pixrect, 0, 0);
			}
			if (num[1] >= 8) {
				i = num[1] - 8;
				t2[i][0] = code[2];
				t2[i][1] = code[3];
				t2_num = i + 1;
				for (i = 0; i < t2_num; i++)
					t2_pixrect[i] = get_pixrect(t2[i]);
				if (t2_num > 0)
					for (i = 0; i < t2_num; i++)
						my_pw_write(cards_pixwin, 321,
							    (193 + (16 * i)), 55, 64, PIX_SRC,
						    t2_pixrect[i], 0, 0);
			}
			break;
		case 4:	/* 3rd foundation & tableau column */
			if (num[1] == 3) {
				found_num++;
				f3[0] = code[2];
				f3[1] = code[3];
				f3_pixrect = get_pixrect(f3);
				my_pw_write(cards_pixwin, 449, 65, 55, 64,
					    PIX_SRC, f3_pixrect, 0, 0);
			}
			if (num[1] >= 8) {
				i = num[1] - 8;
				t3[i][0] = code[2];
				t3[i][1] = code[3];
				t3_num = i + 1;
				for (i = 0; i < t3_num; i++)
					t3_pixrect[i] = get_pixrect(t3[i]);
				if (t3_num > 0)
					for (i = 0; i < t3_num; i++)
						my_pw_write(cards_pixwin, 449,
							    (193 + (16 * i)), 55, 64, PIX_SRC,
						    t3_pixrect[i], 0, 0);
			}
			break;
		case 5:	/* 4th foundation & tableau column */
			if (num[1] == 3) {
				found_num++;
				f4[0] = code[2];
				f4[1] = code[3];
				f4_pixrect = get_pixrect(f4);
				my_pw_write(cards_pixwin, 577, 65, 55, 64,
					    PIX_SRC, f4_pixrect, 0, 0);
			}
			if (num[1] >= 8) {
				i = num[1] - 8;
				t4[i][0] = code[2];
				t4[i][1] = code[3];
				t4_num = i + 1;
				for (i = 0; i < t4_num; i++)
					t4_pixrect[i] = get_pixrect(t4[i]);
				if (t4_num > 0)
					for (i = 0; i < t4_num; i++)
						my_pw_write(cards_pixwin, 577,
							    (193 + (16 * i)), 55, 64, PIX_SRC,
						    t4_pixrect[i], 0, 0);
			}
			break;
		default:
			break;
		}
	}
	if ((old_position[0] == 't') || (old_position[0] == 'p')) {
		switch (old_position[1]) {
		case '1':
			for (i = 0; i < 13; i++)
				pw_replrop(cards_pixwin, 193, (193 + (16 * i)), 55, 64, PIX_SRC, background_pr, 193, (193 + (16 * i)));
			for (i = 0; i < t1_num; i++)
				my_pw_write(cards_pixwin, 193, (193 + (16 * i)), 55, 64, PIX_SRC, t1_pixrect[i], 0, 0);
			break;
		case '2':
			for (i = 0; i < 13; i++)
				pw_replrop(cards_pixwin, 321, (193 + (16 * i)), 55, 64, PIX_SRC, background_pr, 321, (193 + (16 * i)));
			for (i = 0; i < t2_num; i++)
				my_pw_write(cards_pixwin, 321, (193 + (16 * i)), 55, 64, PIX_SRC, t2_pixrect[i], 0, 0);
			break;
		case '3':
			for (i = 0; i < 13; i++)
				pw_replrop(cards_pixwin, 449, (193 + (16 * i)), 55, 64, PIX_SRC, background_pr, 449, (193 + (16 * i)));
			for (i = 0; i < t3_num; i++)
				my_pw_write(cards_pixwin, 449, (193 + (16 * i)), 55, 64, PIX_SRC, t3_pixrect[i], 0, 0);
			break;
		case '4':
			for (i = 0; i < 13; i++)
				pw_replrop(cards_pixwin, 577, (193 + (16 * i)), 55, 64, PIX_SRC, background_pr, 577, (193 + (16 * i)));
			for (i = 0; i < t4_num; i++)
				my_pw_write(cards_pixwin, 577, (193 + (16 * i)), 55, 64, PIX_SRC, t4_pixrect[i], 0, 0);
			break;
		default:
			break;
		}
		old_position = "  ";
	}
	if ((hand_num == 0) && (talon_num == 0)) {
		talon[0] = ' ';
		talon[1] = ' ';
		talon_pixrect = NULL;
		redraw_talon();
		redraw_hand();
	}
}
