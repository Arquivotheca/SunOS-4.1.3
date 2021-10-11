#ifndef lint
static char     sccsid[] = "@(#)sendmove.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */
#include "defs.h"

send_move(position, new_position)
	char           *position, *new_position;
{
	old_t1_num = t1_num;
	old_t2_num = t2_num;
	old_t3_num = t3_num;
	old_t4_num = t4_num;
	old_found_num = found_num;
	old_stock_num = stock_num;
	old_talon_num = talon_num;
	old_position = position;
	switch (position[0]) {

	case 'f':
		panel_set(msg_item, PANEL_LABEL_STRING, "Cannot move a foundation card", 0);
		break;

	case 's':		/* stock */
		switch (new_position[0]) {

		case 'f':
			write(to_fd, "s", 1);
			write(to_fd, "f", 1);
			stock_num--;
			break;
		case 't':
			switch (new_position[1]) {
			case '1':
				write(to_fd, "s", 1);
				write(to_fd, "1", 1);
				stock_num--;
				t1_num++;
				break;
			case '2':
				write(to_fd, "s", 1);
				write(to_fd, "2", 1);
				stock_num--;
				t2_num++;
				break;
			case '3':
				write(to_fd, "s", 1);
				write(to_fd, "3", 1);
				stock_num--;
				t3_num++;
				break;
			case '4':
				write(to_fd, "s", 1);
				write(to_fd, "4", 1);
				stock_num--;
				t4_num++;
				break;
			}
			break;
		default:
			panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
			break;
		}
		break;
	case 't':		/* talon or tableau */
		switch (position[1]) {
		case 'a':	/* talon */
			switch (new_position[0]) {
			case 'f':
				write(to_fd, "t", 1);
				write(to_fd, "f", 1);
				break;
			case 't':
				switch (new_position[1]) {
				case '1':
					write(to_fd, "t", 1);
					write(to_fd, "1", 1);
					t1_num++;
					break;
				case '2':
					write(to_fd, "t", 1);
					write(to_fd, "2", 1);
					t2_num++;
					break;
				case '3':
					write(to_fd, "t", 1);
					write(to_fd, "3", 1);
					t3_num++;
					break;
				case '4':
					write(to_fd, "t", 1);
					write(to_fd, "4", 1);
					t4_num++;
					break;
				}
				break;
			default:
				panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
				break;
			}
			break;
		case '1':	/* tableau */
			switch (new_position[0]) {
			case 'f':
				write(to_fd, "1", 1);
				write(to_fd, "f", 1);
				t1_num--;
				break;
			case 't':
				switch (new_position[1]) {
				case '1':
					panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
					break;
				case '2':
					write(to_fd, "1", 1);
					write(to_fd, "2", 1);
					t1_num--;
					break;
				case '3':
					write(to_fd, "1", 1);
					write(to_fd, "3", 1);
					t1_num--;
					break;
				case '4':
					write(to_fd, "1", 1);
					write(to_fd, "4", 1);
					t1_num--;
					break;
				}
				break;
			default:
				panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
				break;
			}
			break;
		case '2':
			switch (new_position[0]) {
			case 'f':
				write(to_fd, "2", 1);
				write(to_fd, "f", 1);
				t2_num--;
				break;
			case 't':
				switch (new_position[1]) {
				case '1':
					write(to_fd, "2", 1);
					write(to_fd, "1", 1);
					t2_num--;
					break;
				case '2':
					panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
					break;
				case '3':
					write(to_fd, "2", 1);
					write(to_fd, "3", 1);
					t2_num--;
					break;
				case '4':
					write(to_fd, "2", 1);
					write(to_fd, "4", 1);
					t2_num--;
					break;
				}
				break;
			default:
				panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
			}
			break;
		case '3':
			switch (new_position[0]) {
			case 'f':
				write(to_fd, "3", 1);
				write(to_fd, "f", 1);
				t3_num--;
				break;
			case 't':
				switch (new_position[1]) {
				case '1':
					write(to_fd, "3", 1);
					write(to_fd, "1", 1);
					t3_num--;
					break;
				case '2':
					write(to_fd, "3", 1);
					write(to_fd, "2", 1);
					t3_num--;
					break;
				case '3':
					panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
					break;
				case '4':
					write(to_fd, "3", 1);
					write(to_fd, "4", 1);
					t3_num--;
					break;
				default:
					panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
					break;
				}
				break;
			}
			break;
		case '4':
			switch (new_position[0]) {
			case 'f':
				write(to_fd, "4", 1);
				write(to_fd, "f", 1);
				t4_num--;
				break;
			case 't':
				switch (new_position[1]) {
				case '1':
					write(to_fd, "4", 1);
					write(to_fd, "1", 1);
					t4_num--;
					break;
				case '2':
					write(to_fd, "4", 1);
					write(to_fd, "2", 1);
					t4_num--;
					break;
				case '3':
					write(to_fd, "4", 1);
					write(to_fd, "3", 1);
					t4_num--;
					break;
				case '4':
					panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
					break;
				}
				break;
			default:
				panel_set(msg_item, PANEL_LABEL_STRING, "Bad Move.", 0);
				break;
			}
			break;
		}
		break;

		/* use the p? notation for pile of cards */
	case 'p':
		if (new_position[0] != 'f') {
			switch (position[1]) {
			case '1':
				switch (new_position[1]) {
				case '2':
					write(to_fd, "1", 1);
					write(to_fd, "2", 1);
					t1_num = 0;
					break;
				case '3':
					write(to_fd, "1", 1);
					write(to_fd, "3", 1);
					t1_num = 0;
					break;
				case '4':
					write(to_fd, "1", 1);
					write(to_fd, "4", 1);
					t1_num = 0;
					break;
				}
				break;
			case '2':
				switch (new_position[1]) {
				case '1':
					write(to_fd, "2", 1);
					write(to_fd, "1", 1);
					t2_num = 0;
					break;
				case '3':
					write(to_fd, "2", 1);
					write(to_fd, "3", 1);
					t2_num = 0;
					break;
				case '4':
					write(to_fd, "2", 1);
					write(to_fd, "4", 1);
					t2_num = 0;
					break;
				}
				break;
			case '3':
				switch (new_position[1]) {
				case '1':
					write(to_fd, "3", 1);
					write(to_fd, "1", 1);
					t3_num = 0;
					break;
				case '2':
					write(to_fd, "3", 1);
					write(to_fd, "2", 1);
					t3_num = 0;
					break;
				case '4':
					write(to_fd, "3", 1);
					write(to_fd, "4", 1);
					t3_num = 0;
					break;
				}
				break;
			case '4':
				switch (new_position[1]) {
				case '1':
					write(to_fd, "4", 1);
					write(to_fd, "1", 1);
					t4_num = 0;
					break;
				case '2':
					write(to_fd, "4", 1);
					write(to_fd, "2", 1);
					t4_num = 0;
					break;
				case '3':
					write(to_fd, "4", 1);
					write(to_fd, "3", 1);
					t4_num = 0;
					break;
				}
				break;
			default:
				panel_set(msg_item, PANEL_LABEL_STRING, "Bad starting position.", 0);
				break;
			}
		} else
			panel_set(msg_item, PANEL_LABEL_STRING, "Can't move a pile to a foundation", 0);

	default:
		break;
	}
}
