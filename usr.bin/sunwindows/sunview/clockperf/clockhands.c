#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)clockhands.c 1.1 92/07/30";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

#include "clockhands.h"
#include <sys/types.h>
#include <suntool/sunview.h> 
#include <suntool/frame.h>
#include <suntool/icon.h>

extern	Icon		clockicon, clockicon_date;
extern	Pixrect 	*ic_mpr, *base_mpr;
extern	unsigned 	show_seconds, show_date;
extern	struct tm 	*tmp;
extern	unsigned	face;
extern	struct	endpoints	min_box[];

extern  void paintface();
extern  void clock_show_date();


struct	hands hand_points[60] = {
/*
 *	trailing    leading    seconds     long      short
 *	  base        base       base       end       end
 */
	29, 31,    34, 31,    32, 31,    32,  3,    32, 10,  /*  0 */
	29, 31,    34, 31,    32, 31,    35,  3,    34, 10,  /*  1 */
	29, 31,    34, 32,    32, 31,    38,  4,    36, 10,  /*  2 */
	29, 31,    34, 32,    32, 31,    41,  4,    38, 11,  /*  3 */
	30, 30,    34, 32,    32, 31,    43,  5,    41, 12,  /*  4 */
	30, 30,    34, 32,    32, 31,    46,  7,    43, 13,  /*  5 */
	30, 30,    34, 32,    32, 31,    48,  8,    44, 14,  /*  6 */
	30, 30,    33, 33,    32, 31,    51, 10,    46, 15,  /*  7 */
	30, 30,    33, 33,    32, 31,    53, 12,    48, 17,  /*  8 */
	31, 29,    33, 33,    32, 31,    55, 15,    49, 19,  /*  9 */
	31, 29,    33, 33,    32, 31,    56, 17,    50, 21,  /* 10 */
	31, 29,    33, 33,    32, 31,    58, 20,    51, 22,  /* 11 */
	31, 29,    32, 34,    32, 31,    59, 22,    52, 25,  /* 12 */
	31, 29,    32, 34,    32, 31,    59, 25,    53, 27,  /* 13 */
	32, 29,    32, 34,    32, 31,    60, 28,    53, 29,  /* 14 */

	32, 29,    32, 34,    32, 32,    60, 32,    53, 32,  /* 15 */
	32, 29,    32, 34,    32, 32,    59, 35,    53, 34,  /* 16 */
	32, 29,    31, 34,    32, 32,    59, 38,    52, 36,  /* 17 */
	32, 29,    31, 34,    32, 32,    58, 41,    51, 38,  /* 18 */
	33, 30,    31, 34,    32, 32,    56, 43,    50, 41,  /* 19 */
	33, 30,    31, 34,    32, 32,    55, 46,    49, 43,  /* 20 */
	33, 30,    31, 34,    32, 32,    53, 48,    48, 44,  /* 21 */
	33, 30,    30, 33,    32, 32,    51, 51,    46, 46,  /* 22 */
	33, 30,    30, 33,    32, 32,    48, 53,    44, 48,  /* 23 */
	34, 31,    30, 33,    32, 32,    46, 55,    43, 49,  /* 24 */
	34, 31,    30, 33,    32, 32,    43, 56,    41, 50,  /* 25 */
	34, 31,    30, 33,    32, 32,    41, 58,    38, 51,  /* 26 */
	34, 31,    29, 32,    32, 32,    38, 59,    36, 52,  /* 27 */
	34, 31,    29, 32,    32, 32,    35, 60,    34, 53,  /* 28 */
	34, 32,    29, 32,    32, 32,    32, 60,    32, 53,  /* 29 */


	34, 32,    29, 32,    31, 32,    31, 60,    31, 53,  /* 30 */
	34, 32,    29, 32,    31, 32,    28, 60,    29, 53,  /* 31 */
	33, 33,    29, 32,    31, 32,    25, 59,    27, 53,  /* 32 */
	33, 33,    29, 32,    31, 32,    22, 59,    25, 52,  /* 33 */
	33, 33,    29, 31,    31, 32,    20, 58,    22, 51,  /* 34 */
	33, 33,    29, 31,    31, 32,    17, 59,    21, 50,  /* 35 */
	33, 33,    29, 31,    31, 32,    15, 55,    19, 49,  /* 36 */
	32, 34,    29, 31,    31, 32,    12, 53,    17, 48,  /* 37 */
	32, 34,    29, 31,    31, 32,    10, 51,    15, 46,  /* 38 */
	32, 34,    30, 30,    31, 32,     8, 48,    14, 44,  /* 39 */
	32, 34,    30, 30,    31, 32,     7, 46,    13, 43,  /* 40 */
	32, 34,    30, 30,    31, 32,     5, 43,    12, 41,  /* 41 */
	31, 34,    30, 30,    31, 32,     4, 41,    11, 38,  /* 42 */
	31, 34,    30, 30,    31, 32,     4, 38,    10, 36,  /* 43 */
	31, 34,    31, 29,    31, 32,     3, 35,    10, 34,  /* 44 */

	31, 34,    31, 29,    31, 31,     3, 31,    10, 31,  /* 45 */
	31, 34,    31, 29,    31, 31,     3, 28,    10, 29,  /* 46 */
	30, 33,    31, 29,    31, 31,     4, 25,    10, 27,  /* 47 */
	30, 33,    31, 29,    31, 31,     4, 22,    11, 25,  /* 48 */
	30, 33,    32, 29,    31, 31,     5, 20,    12, 22,  /* 49 */
	30, 33,    32, 29,    31, 31,     7, 17,    13, 21,  /* 50 */
	30, 33,    32, 29,    31, 31,     8, 15,    14, 19,  /* 51 */
	29, 32,    32, 29,    31, 31,    10, 12,    15, 17,  /* 52 */
	29, 32,    32, 29,    31, 31,    12, 10,    17, 15,  /* 53 */
	29, 32,    33, 30,    31, 31,    15,  8,    19, 14,  /* 54 */
	29, 32,    33, 30,    31, 31,    17,  7,    21, 13,  /* 55 */
	29, 32,    33, 30,    31, 31,    20,  5,    22, 12,  /* 56 */
	29, 31,    33, 30,    31, 31,    22,  4,    25, 11,  /* 57 */
	29, 31,    33, 30,    31, 31,    25,  4,    27, 10,  /* 58 */
	29, 31,    34, 31,    31, 31,    28,  3,    29, 10   /* 59 */

};


void
clock_sethands(frame)
	Frame		frame;
{
	struct hands   *hand;

	if (show_date)
		(void) clock_show_date();
	(void)pr_rop(ic_mpr, 0, 0, 64, (show_date) ? 75: 64,
	    PIX_SRC, base_mpr, 0, 0);
	hand = &hand_points[(tmp->tm_hour*5 + (tmp->tm_min + 6)/12) % 60];
	(void)pr_vector(ic_mpr,
		  hand->x1, hand->y1,
		  hand->hour_x, hand->hour_y,
		  PIX_SET, 1);
	(void)pr_vector(ic_mpr,
		  hand->x2, hand->y2,
		  hand->hour_x, hand->hour_y,
		  PIX_SET, 1);

	hand = &hand_points[tmp->tm_min];
	(void)pr_vector(ic_mpr,
		  hand->x1, hand->y1,
		  hand->min_x, hand->min_y,
		  PIX_SET, 1);
	(void)pr_vector(ic_mpr,
		  hand->x2, hand->y2,
		  hand->min_x, hand->min_y,
		  PIX_SET, 1);

	if (show_seconds) {
		hand = &hand_points[tmp->tm_sec];
		(void)pr_vector(ic_mpr,
			  hand->sec_x, hand->sec_y,
			  hand->min_x, hand->min_y,
			  PIX_SET, 1);
	}
	if (face)
		(void)paintface(ic_mpr);
	/*
		Set Icon Image.
	*/
/*	if (show_date) {
		icon_set(clockicon_date,
			ICON_IMAGE,	ic_mpr,
			0);
		window_set(frame, WIN_ICON, clockicon_date, 0);	
	} else {
		icon_set(clockicon,
			ICON_IMAGE,	ic_mpr,
			0);
		window_set(frame, WIN_ICON, clockicon, 0);
	}
*/
	tool_display(frame);			
}

void
clock_rom_sethands(frame)
	Frame		frame;
{
	register struct endpoints *hand, *handorg;
	register int i;

	/*
	 * Copy the icon image to ic_mpr.
	 */
	if (show_date)
		(void) clock_show_date();
	(void)pr_rop(ic_mpr, 0, 0, 64, (show_date)? 75: 64,
	    PIX_SRC, base_mpr, 0, 0);
	/*
	 * Hour hand.
	 */
	hand = &ep_hr[(tmp->tm_hour*5 + (tmp->tm_min + 6)/12) % 60];
	for (i = 0; i < 4; i++) {
		(void)pr_vector(ic_mpr, 31 + min_box[i].x , 31 + min_box[i].y,
			hand->x + min_box[i].x , hand->y + min_box[i].y,
			PIX_SET, 1);
	}
	/*
	 * Minute hand.
	 */
	hand = &ep_min[tmp->tm_min];
	for (i = 0; i < 4; i++) {
		(void)pr_vector(ic_mpr, 31 + min_box[i].x , 31 + min_box[i].y,
			hand->x + min_box[i].x , hand->y + min_box[i].y,
			PIX_SET, 1);
	}
	/*
	 * Second hand.
	 */
	if (show_seconds) {
		hand = &ep_sec[tmp->tm_sec];
		handorg = &ep_secorg[(tmp->tm_sec + 30) % 60];
		/* cop out */
		(void)pr_vector(ic_mpr, handorg->x, handorg->y, hand->x, hand->y,
			PIX_SET, 1);
	}
	/*
	 * Install the new icon.
	 */
	if (face)
		paintface(ic_mpr);
/*		
	if (show_date) {
		icon_set(clockicon_date,
			ICON_IMAGE,	ic_mpr,
			0);
		window_set(frame, WIN_ICON, clockicon_date, 0);	
	} else {
		icon_set(clockicon,
			ICON_IMAGE,	ic_mpr,
			0);
		window_set(frame, WIN_ICON, clockicon, 0);
	}
*/
	tool_display(frame);	
}
