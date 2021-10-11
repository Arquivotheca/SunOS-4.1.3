#ifndef lint
static char     sccsid[] = "@(#)input.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
/*
 * Copyright (c) 1985 by Sun Microsystems, Inc. 
 */
#include "defs.h"
cf_reader(sw, ibits, obits, ebits, timer)
	caddr_t         sw;
	int            *ibits, *obits, *ebits;
	struct timeval **timer;
{

	struct inputevent ie;
	if (*ibits & (1 << cf_fd)) {
		read_board();
		*ibits &= ~(1 << cf_fd);
	}
	old_selected(sw, ibits, obits, ebits, timer);
	*ibits |= (1 << tool->tl_windowfd);
	*ibits |= (1 << cf_fd);
/*
#ifdef DEBUG
	fprintf(stderr, "tool: t1 %d t2 %d t3 %d t4 %d hand %d talon %d stock %d found %d\n", t1_num, t2_num, t3_num, t4_num, hand_num, talon_num, stock_num, found_num);
#endif
*/
}

static
readchar()
{
	char            c;
	if (read(cf_fd, &c, 1) != 1)
		return (EOF);
	return (c);
}
read_board()
{
	char            buf[5];
	buf[0] = readchar();
	buf[1] = readchar();
	buf[2] = readchar();
	buf[3] = readchar();
	buf[4] = readchar();

/*
#ifdef DEBUG
	fprintf(stderr, "tool: buf == %s", buf);
#endif
*/

	decode_position(buf);
	if ((found_num >= 52) && (game_done == FALSE))
		win_proc();

}
mouse_reader(sw, ibits, obits, ebits, timer)
	caddr_t         sw;
	int            *ibits, *obits, *ebits;
	struct timeval **timer;
{
	static int      moving = FALSE, oldx = 999, oldy = 999;
	struct inputevent ie;
	/* get the inputevent... */
	input_readevent(cards_sw->ts_windowfd, &ie);

	/* game is done, via win or loss */
	if (game_done == TRUE) {
		if (ie.ie_code == MS_LEFT)
			restart_proc();
		else 
			goto done;
	}

	/* if not moving, but MOVEing, go away */
	if ((moving == FALSE) && (ie.ie_code == LOC_MOVEWHILEBUTDOWN))
		goto done;

	/* if it's the right button, put up the menu */
	if (ie.ie_code == MS_RIGHT) {
		do_menu(&ie);
		goto done;
	}
	if (win_inputposevent(&ie)) {
		switch (ie.ie_code) {

		case MS_LEFT:

		case MS_MIDDLE:
			if (which(&ie)) {
				moving = TRUE;
			} else {
				moving = FALSE;
				break;
			}

		case LOC_MOVEWHILEBUTDOWN:
			if (moving == TRUE) {
				pw_write(cards_pixwin, oldx, oldy, 55, 64,
					 PIX_SRC ^ PIX_DST, outline_pr,
					 0, 0);
				pw_write(cards_pixwin, ie.ie_locx, ie.ie_locy,
					 55, 64, PIX_SRC ^ PIX_DST,
					 outline_pr, 0, 0);
				oldx = (int) ie.ie_locx;
				oldy = (int) ie.ie_locy;
			}
			break;

		default:
			break;

		}
	} else if (win_inputnegevent(&ie)) {
		if ((moving == FALSE) && ((ie.ie_code == MS_LEFT) || (ie.ie_code == MS_MIDDLE)))
			if ((ie.ie_locx > 64) && (ie.ie_locx < 129) && (ie.ie_locy > 384) && (ie.ie_locy < 449))
				deal_proc();
		if ((moving == TRUE) && ((ie.ie_code == MS_LEFT) || (ie.ie_code == MS_MIDDLE))) {
			if ((where(&ie)) && (position != new_position)) {
				send_move(position, new_position);
				pw_write(cards_pixwin, oldx, oldy, 55, 64,
				    PIX_SRC ^ PIX_DST, outline_pr, 0, 0);
				oldx = oldy = 999;
			} else {
				pw_write(cards_pixwin, oldx, oldy, 55, 64,
				    PIX_SRC ^ PIX_DST, outline_pr, 0, 0);
				oldx = oldy = 999;
			}
		} else {
			card = NULL;
			position = NULL;
			new_position = NULL;
			moving = FALSE;
			oldx = oldy = 999;
		}
		card = NULL;
		position = NULL;
		new_position = NULL;
		moving = FALSE;
		oldx = oldy = 999;
	}
done:
	*ibits = *obits = *ebits = 0;
}
/*
 * which figures out which card you've just grabbed, and where it came
 * from 
 */
which(ie)
	struct inputevent *ie;
{
	int             col, row;
	int             x, y;
	col = (int) ((ie->ie_locx / 64) + 1);
	row = (int) ((ie->ie_locy / 64) + 1);
	x = (int) ie->ie_locx;
	y = (int) ie->ie_locy;

	if ((col == 2) || (col == 4) || (col == 6) ||
	    (col == 8) || (col == 10)) {

		if (col == 2) {
			switch (row) {
			case 3:
				if (stock_num > 0) {
					card = stock;
					position = "st";
					return TRUE;
				} else
					return FALSE;
				break;
			case 5:
				card = talon;
				position = "ta";
				return TRUE;
				break;
			default:
				return FALSE;
			}
		}
		if (((col == 4) || (col == 6) || (col == 8) || (col == 10))
		    && (y < 192)) {
			card = NULL;
			position = NULL;
			return FALSE;
			/* you can't select a foundation card */
		}
		switch (col) {

		case 4:
			if (t1_num > 1) {
				if ((y > 192) && (y < (193 + (16 * (t1_num - 1))))) {
					card = "p1";
					position = "p1";
					return TRUE;
				} else if ((y > (193 + (16 * (t1_num - 1)) + 1)) && (y < (193 + (16 * t1_num) + 48))) {
					card = t1[t1_num];
					position = "t1";
					return TRUE;
				}
			} else if ((t1_num == 1) && (y < 257)) {
				card = t1[1];
				position = "t1";
				return TRUE;
			}
			return FALSE;
			break;
		case 6:
			if (t2_num > 1) {
				if ((y > 192) && (y < (193 + (16 * (t2_num - 1))))) {
					card = "p2";
					position = "p2";
					return TRUE;
				} else if ((y > (193 + (16 * (t2_num - 1)) + 1)) && (y < (193 + (16 * t2_num) + 48))) {
					/* bottom card */
					card = t2[t2_num];
					position = "t2";
					return TRUE;
				}
			} else if ((t2_num == 1) && (y < 257)) {
				card = t2[1];
				position = "t2";
				return TRUE;
			}
			return FALSE;
			break;

		case 8:
			if (t3_num > 1) {
				if ((y > 192) && (y < (193 + (16 * (t3_num - 1))))) {
					card = "p3";
					position = "p3";
					return TRUE;
				} else if ((y > (193 + (16 * (t3_num - 1)) + 1)) && (y < (193 + (16 * t3_num) + 48))) {
					card = t3[t3_num];
					position = "t3";
					return TRUE;
				}
			} else if ((t3_num == 1) && (y < 257)) {
				card = t3[1];
				position = "t3";
				return TRUE;
			}
			return FALSE;
			break;

		case 10:
			if (t4_num > 1) {
				if ((y > 192) && (y < (193 + (16 * (t4_num - 1))))) {
					card = "p4";
					position = "p4";
					return TRUE;
				} else if ((y > (193 + (16 * (t4_num - 1)) + 1)) && (y < (193 + (16 * t4_num) + 48))) {
					card = t4[t4_num];
					position = "t4";
					return TRUE;
				}
			} else if ((t4_num == 1) && (y < 257)) {
				card = t4[1];
				position = "t4";
				return TRUE;
			}
			return FALSE;
			break;
		}
	} else
		return FALSE;
}
/*
 * 'where' figures out, in theory, where you've just tried to stick a card 
 */
where(ie)
	struct inputevent *ie;
{
	int             a[2], b[2];
	a[0] = ie->ie_locx;
	a[1] = ie->ie_locy;
	b[0] = ie->ie_locx + 64;
	b[1] = ie->ie_locy + 64;
	/*
	 * * if you drop it above the bottom of the foundation row, it
	 * gets * stuck there 
	 */
	if (a[1] < 150) {
		if (((a[0] > 175) || (b[0] > 175)) && ((a[0] < 275) || (b[0] < 275))) {
			new_position = "f1";
			return TRUE;
		}
		if (((a[0] > 303) || (b[0] > 303)) && ((a[0] < 403) || (b[0] < 403))) {
			new_position = "f2";
			return TRUE;
		}
		if (((a[0] > 431) || (b[0] > 431)) && ((a[0] < 531) || (b[0] < 531))) {
			new_position = "f3";
			return TRUE;
		}
		if (((a[0] > 559) || (b[0] > 559)) && ((a[0] < 659) || (b[0] < 659))) {
			new_position = "f4";
			return TRUE;
		}
	} else {
		if ((((a[0] > 175) || (b[0] > 175)) && ((a[0] < 275) || (b[0] < 275))) && (((a[1] > 180) || (b[1] > 180)) && ((a[1] < (240 + (16 * t1_num))) || (b[1] < (240 + (16 * t1_num)))))) {
			new_position = "t1";
			return TRUE;
		}
		if ((((a[0] > 303) || (b[0] > 303)) && ((a[0] < 403) || (b[0] < 403))) && (((a[1] > 180) || (b[1] > 180)) && ((a[1] < (240 + (16 * t2_num))) || (b[1] < (240 + (16 * t2_num)))))) {
			new_position = "t2";
			return TRUE;
		}
		if ((((a[0] > 431) || (b[0] > 431)) && ((a[0] < 531) || (b[0] < 531))) && (((a[1] > 180) || (b[1] > 180)) && ((a[1] < (240 + (16 * t3_num))) || (b[1] < (240 + (16 * t3_num)))))) {
			new_position = "t3";
			return TRUE;
		}
		if ((((a[0] > 559) || (b[0] > 559)) && ((a[0] < 659) || (b[0] < 659))) && (((a[1] > 180) || (b[1] > 180)) && ((a[1] < (240 + (16 * t4_num))) || (b[1] < (240 + (16 * t4_num)))))) {
			new_position = "t4";
			return TRUE;
		}
	}
	return FALSE;
}
