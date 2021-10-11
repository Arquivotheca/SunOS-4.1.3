#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)init.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <signal.h>
#include "other_hs.h"
#include "fontedit.h"
#include "externs.h"
/*
 *  Initialize the boundry values.
 */

fted_init_bounds(new_font)
int		new_font;		/* true if we're initing a new font */
{
    register char   buff[128];
    register int    i;
    register struct pixrect *pr;


    fted_num_chars_in_font = 256;	/* for now */

    if (fted_cur_font == NULL) {
	fted_char_max_width = fted_char_max_height = 0;
	fted_font_caps_height = fted_font_base_line = fted_font_left_edge = fted_font_right_edge = 0;
	fted_font_x_height = 0;
    }
    else {
	if (new_font) {
	/* find the maxes  for a new font */
	    fted_font_base_line = 0;
	    fted_char_max_width = fted_char_max_height = 0;
	    for (i = 0; i < fted_num_chars_in_font; i++) {
		pr = fted_cur_font->pf_char[i].pc_pr;
		if (pr != NULL) {
		    if (fted_char_max_width < pr->pr_size.x) {
			fted_char_max_width = pr->pr_size.x;
		    }
		    if (fted_char_max_height < pr->pr_size.y) {
			fted_char_max_height = pr->pr_size.y;
		    }
		    if ((-fted_cur_font->pf_char[i].pc_home.y) > fted_font_base_line)
		         fted_font_base_line = -fted_cur_font->pf_char[i].pc_home.y;
		}
		    
	    } /* endfor */
	} /* endif */

	if(fted_font_base_line == 0) {
	    fted_font_caps_height = 0;
	    fted_font_x_height = 0;
	}
	else{
	    fted_font_caps_height = fted_font_base_line;	
	    fted_font_x_height = (fted_font_base_line >> 1) + 2;
	}
	fted_font_left_edge = 0;
	fted_font_right_edge = fted_char_max_width;
	
    }



    sprintf (buff, "%d", fted_char_max_width);
    panel_set_value(fted_max_width_item, buff);

    sprintf (buff, "%d", fted_char_max_height);
    panel_set_value(fted_mafted_x_height_item, buff);

    sprintf (buff, "%d", fted_font_caps_height);
    panel_set_value(fted_caps_height_item, buff);

    sprintf (buff, "%d", fted_font_x_height);
    panel_set_value(fted_x_height_item, buff);

    sprintf (buff, "%d", fted_font_base_line);
    panel_set_value(fted_baseline_item, buff);

}
