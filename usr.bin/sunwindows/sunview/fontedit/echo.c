#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)echo.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "other_hs.h"
#include "fontedit.h"
#include "externs.h"
#include "button.h"
#include "slider.h"
#include "edit.h"
#include <strings.h>

#include <sunwindow/defaults.h>

#define MAX_CHARS	512
#define OFFSET_X	5
#define OFFSET_Y	15

/*
 * When a key is pressed when the cursor is in this window, display the 
 * glyph which corresponds to that character (if one exists). 
 * 
 * To display a character, it's pixrect is written to the location
 * relative to the baseline as calculated from the pc_home values.
 * The character's number is then stored on a stack. To erase a 
 * character, background is written over the characer. 
 */


static int		baseline_x;	/* baseline of the string being echoed*/
static int		baseline_y;
static struct rect 	bound;		/* bounds around the text	*/
static int		num_chars = 0;	/* number of characters display	*/
static char		echoed_chars[MAX_CHARS];/* the echoed characters*/
static char		tmp_chars[MAX_CHARS];

static	char		char_del;

static 			echo_selected();
static 			echo_sig_handler();

init_fted_echo_sub_win()
{
	struct inputmask    mask;
	char		    *def_str;
	
	fted_echo_pw = pw_open(fted_echo_sub_win->ts_windowfd);
	fted_echo_sub_win->ts_io.tio_selected = echo_selected;
	fted_echo_sub_win->ts_io.tio_handlesigwinch = echo_sig_handler;
	fted_echo_sub_win->ts_destroy = fted_nullproc; 
	input_imnull (&mask);
	win_setinputcodebit (&mask, KEY_BOTTOMRIGHT);
 	mask.im_flags |= IM_ASCII;
	win_setinputmask(fted_echo_sub_win->ts_windowfd, &mask, NULL, WIN_NULLLINK);

	def_str = defaults_get_string("/Text/Edit_back_char", "\177", NULL);
	char_del = def_str[0];

	fted_echo_init();

}

static
echo_selected(nullsw, ibits, obits, ebits, timer)
caddr_t * nullsw;
int    *ibits, *obits, *ebits;
struct timeval **timer;
{
    struct inputevent   ie;
    register int    	code;
    
    *ibits = *obits = *ebits = 0;

    if( input_readevent(fted_echo_sub_win->ts_windowfd, &ie) == -1) {
	perror ("fontedit input failed in echo_select");
    }
    if (fted_cur_font == NULL)
       return;
       
    code = event_action(&ie);
    if ( code <= ASCII_LAST) {
	if ((char)code == char_del) {
	    fted_echo_erase();
	}
	else
	    fted_echo_char(code);
    }

}

static
echo_sig_handler()
{
    struct 	rect	size;
    register	int	i, num;
	
    win_getsize(fted_echo_sub_win->ts_windowfd,&size);
    
    pw_damaged(fted_echo_pw);
    
    pw_writebackground(fted_echo_pw, 0, 0, size.r_width,
		size.r_height, PIX_CLR);
    echoed_chars[num_chars + 1] = '\0';
    (void) strcpy(tmp_chars,&(echoed_chars[1]));
    num = num_chars;
    num_chars = 0;
    baseline_x = OFFSET_X;
    baseline_y = OFFSET_Y + fted_font_base_line;
    for (i = 0; i < num; i++) {
	fted_echo_char((int)tmp_chars[i]);
    }

    pw_donedamaged(fted_echo_pw);
}
    



fted_echo_init()
{
    struct rect size;
    
    win_getsize(fted_echo_sub_win->ts_windowfd,&size);
    pw_writebackground(fted_echo_pw, 0, 0, size.r_width,
		       size.r_height, PIX_CLR);    

    baseline_x = OFFSET_X;
    baseline_y = OFFSET_Y + fted_font_base_line;

    num_chars = 0;
}

/*
 * The tricky aspect of displaying a character is determing if the
 * characer is being editted. If it is, we will use the current
 * pix_char of the character (that is, the one that is being editted). 
 */

fted_echo_char(c)
register int	c;
{
    register struct pixchar 	*pix_char_ptr;	/* the char to be displayed	 */
    register struct pixrect 	*pr;
    register int 		i;

    if (num_chars < MAX_CHARS) {
	pix_char_ptr = NULL;
	for (i = 0; i < NUM_EDIT_PADS;i++) {
	    /* look to see if the character is being editted */
	    if ((fted_edit_pad_info[i].open) && (fted_edit_pad_info[i].char_num == c)) {
		pix_char_ptr = fted_edit_pad_info[i].pix_char_ptr;
		pw_write(fted_echo_pw, (baseline_x + pix_char_ptr->pc_home.x),
	    		(baseline_y + pix_char_ptr->pc_home.y),
			fted_edit_pad_info[i].window.r_width,
			fted_edit_pad_info[i].window.r_height,
			PIX_SRC,
			pix_char_ptr->pc_pr,  0, 0);
	    	baseline_x += pix_char_ptr->pc_adv.x ;
	    	baseline_y += pix_char_ptr->pc_adv.y ;
	    	echoed_chars[++num_chars] = (char) c;
		break;
	    }
	}
	if (pix_char_ptr == NULL) {
	    pix_char_ptr = &(fted_cur_font -> pf_char[c]);
	    pr = pix_char_ptr->pc_pr;
	    if (pr != NULL) {
	    	pw_write(fted_echo_pw, (baseline_x + pix_char_ptr->pc_home.x),
	    		(baseline_y + pix_char_ptr->pc_home.y),
			pr->pr_size.x, pr->pr_size.y, PIX_SRC, pr,  0, 0);
	    	baseline_x += pix_char_ptr->pc_adv.x ;
	    	baseline_y += pix_char_ptr->pc_adv.y ;
	    	echoed_chars[++num_chars] = (char) c;
	    }
	}
    }
    else
       fted_message_user("That's as many characters as I can echo.");
}



fted_echo_erase()
{
    register int c, i;
    register struct pixchar *pix_char_ptr;	/* the char to be erased */

    if (num_chars > 0) {
	c = (int) echoed_chars[num_chars--];
	pix_char_ptr = NULL;
	for (i = 0; i < NUM_EDIT_PADS;i++) {
	    /* look to see if the character is being editted */
	    if ((fted_edit_pad_info[i].open) && (fted_edit_pad_info[i].char_num == c)) {
	    	pix_char_ptr = fted_edit_pad_info[i].pix_char_ptr;
		baseline_x -= pix_char_ptr->pc_adv.x ;
	    	baseline_y -= pix_char_ptr->pc_adv.y ;
                pw_writebackground(fted_echo_pw,
				    (baseline_x + pix_char_ptr->pc_home.x),
				    (baseline_y + pix_char_ptr->pc_home.y),
				    pix_char_ptr->pc_pr->pr_size.x,
				    pix_char_ptr->pc_pr->pr_size.y,
				    PIX_CLR);
		break;
	    }
	}
	if (pix_char_ptr == NULL) {
	    pix_char_ptr = &(fted_cur_font->pf_char[c]);
	    baseline_x -= pix_char_ptr->pc_adv.x;
	    baseline_y -= pix_char_ptr->pc_adv.y;
	    pw_writebackground(fted_echo_pw, (baseline_x + pix_char_ptr->pc_home.x),
				    (baseline_y + pix_char_ptr->pc_home.y),
				    pix_char_ptr->pc_pr->pr_size.x,
				    pix_char_ptr->pc_pr->pr_size.y,
				    PIX_CLR);
	}
    }
    else
       fted_message_user("All characters erased.");
}
