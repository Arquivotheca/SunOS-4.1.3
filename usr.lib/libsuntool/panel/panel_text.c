#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)panel_text.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif


/***********************************************************************/
/* panel_text.c                             */
/* Copyright (c) 1985 by Sun Microsystems, Inc.           */
/***********************************************************************/

#include <suntool/panel_impl.h>
#include <ctype.h>
#include <sys/filio.h>
#include <fcntl.h>
#include <sunwindow/defaults.h>
#include <sundev/kbd.h>
#include <sunwindow/sv_malloc.h>

static caddr_t  panel_getcaret(), panel_setcaret();

char           *malloc();

/***********************************************************************/
/* field-overflow pointer                                    		 */
/***********************************************************************/

static short    left_arrow_image[] = {
#include <images/panel_left_arrow.pr>
};
static          mpr_static(panel_left_arrow_pr, 8, 8, 1, left_arrow_image);

static short    right_arrow_image[] = {
#include <images/panel_right_arrow.pr>
};
static          mpr_static(panel_right_arrow_pr, 8, 8, 1, right_arrow_image);

static int      primary_ms_clicks = 0;	  /* # primary mouse left clicks */
static int      secondary_ms_clicks = 0;  /* # secondary mouse left clicks */
static int      ms_clicks = 0;
static u_char   primary_pending_delete = FALSE;
static u_char   secondary_pending_delete = FALSE;
static panel_handle primary_seln_panel, secondary_seln_panel;
static Rect     primary_seln_rect, secondary_seln_rect;
static int      primary_seln_first, primary_seln_last;
static int      secondary_seln_first, secondary_seln_last;

typedef enum {
    HL_NONE, HL_GRAY, HL_INVERT
}               highlight_t;
static highlight_t primary_seln_highlight = HL_NONE;
static highlight_t secondary_seln_highlight = HL_NONE;
#define PV_HIGHLIGHT TRUE
#define PV_NO_HIGHLIGHT FALSE

#define textdp(ip) ((text_data *)LINT_CAST((ip)->data))

static          begin_preview(), cancel_preview(), accept_preview(),
                accept_menu(),
                accept_key(),
                paint(),
                destroy(),
                set_attr();
static caddr_t  get_attr();
static Panel_item remove();
static Panel_item restore();

static void     text_caret_invert();
static void     text_caret_on();
static void     paint_value();

extern void     (*panel_caret_on_proc) (),
                (*panel_caret_invert_proc) ();


static struct panel_ops ops = {
    panel_default_handle_event,	/* handle_event() */
    begin_preview,		/* begin_preview() */
    begin_preview,		/* update_preview() */
    cancel_preview,		/* cancel_preview() */
    accept_preview,		/* accept_preview() */
    accept_menu,		/* accept_menu() */
    accept_key,			/* accept_key() */
    paint,			/* paint() */
    destroy,			/* destroy() */
    get_attr,			/* get_attr() */
    set_attr,			/* set_attr() */
    remove,			/* remove() */
    restore,			/* restore() */
    panel_nullproc		/* layout() */
};

/***********************************************************************/
/* data area		                       */
/***********************************************************************/

typedef struct text_data {
    Panel_setting   notify_level;	/* NONE, SPECIFIED, NON_PRINTABLE,
					 * ALL */
    panel_item_handle orig_caret;	/* original item with the caret */
    int             underline;	/* TRUE, FALSE (not implemented) */
    int             flags;
    char           *value;
    Pixfont        *font;
    char            mask;
    int             caret_offset;	/* caret's x offset from value rect */
    int		    secondary_caret_offset;
    int             value_offset;	/* right margin of last displayed
					 * char (x offset from value rect) */
    int             last_char;	/* last displayed character */
    int             seln_first;	/* index of first char selected */
    int             seln_last;	/* index of last char selected */
    int             ext_first;	/* first char of extended word */
    int             ext_last;	/* last char of extended word */
    int             first_char;	/* first displayed character */
    char           *terminators;
    int             stored_length;
    int             display_length;
    char            edit_bk_char;
    char            edit_bk_word;
    char            edit_bk_line;
    panel_item_handle next;
}               text_data;


#define SELECTING_ITEM		0x00000001
#define HASCARET		0x00000002
#define READ_ONLY               0x00000004

#define UP_CURSOR_KEY           (KEY_RIGHT(8))
#define DOWN_CURSOR_KEY         (KEY_RIGHT(14))
#define RIGHT_CURSOR_KEY        (KEY_RIGHT(12))
#define LEFT_CURSOR_KEY         (KEY_RIGHT(10))

Panel_item
panel_text(ip, avlist)
    register panel_item_handle ip;
    Attr_avlist     avlist;
{
    register text_data *dp;
    char           *def_str;

    /* set the caret functions for panel_public.c and panel_select.c */
    panel_caret_on_proc = text_caret_on;
    panel_caret_invert_proc = text_caret_invert;

    if (!ip->panel->seln_client)
	panel_seln_init(ip->panel);

    if (!(dp = (text_data *) LINT_CAST(calloc(1, sizeof(text_data)))))
	return (NULL);

    ip->ops = &ops;
    ip->data = (caddr_t) dp;
    ip->item_type = PANEL_TEXT_ITEM;
    if (ip->notify == panel_nullproc)
	ip->notify = (int (*) ()) panel_text_notify;

    dp->notify_level = PANEL_SPECIFIED;
    dp->font = ip->panel->font;
    dp->mask = '\0';
    dp->terminators = panel_strsave("\n\r\t");
    dp->stored_length = 80;
    dp->display_length = 80;

    /*
     * Keymapping has made the following edit character settings obsolete.
     * (see update_value() below)
     */
    if (ip->panel->edit_bk_char == NULL) {
	def_str = defaults_get_string("/Text/Edit_back_char", "\177", (int *) NULL);
	ip->panel->edit_bk_char = def_str[0];
	def_str = defaults_get_string("/Text/Edit_back_word", "\027", (int *) NULL);
	ip->panel->edit_bk_word = def_str[0];
	def_str = defaults_get_string("/Text/Edit_back_line", "\025", (int *) NULL);
	ip->panel->edit_bk_line = def_str[0];
    }
    dp->edit_bk_char = ip->panel->edit_bk_char;
    dp->edit_bk_word = ip->panel->edit_bk_word;
    dp->edit_bk_line = ip->panel->edit_bk_line;

    dp->value = (char *) calloc(1, (u_int) (dp->stored_length + 1));
    if (!dp->value)
	return (NULL);

    /*
     * All other dp-> fields are initialized to zero by calloc().
     */

    ip->value_rect.r_width = panel_col_to_x(dp->font, dp->display_length);
    ip->value_rect.r_height = dp->font->pf_defaultsize.y;

    /* set the user specified attributes */
    if (!set_attr(ip, avlist))
	return NULL;;

    /* append to the panel list of items */
    (void) panel_append(ip);

    /* only insert in caret list if not hidden already */
    if (!hidden(ip))
	insert(ip);

    return (Panel_item) ip;
}


static int
set_attr(ip, avlist)
    register panel_item_handle ip;
    register Attr_avlist avlist;
{
    register text_data *dp = textdp(ip);
    register Panel_attribute attr;
    char           *new_value = NULL;
    short           value_changed = FALSE;
    extern char    *strncpy();

    while (attr = (Panel_attribute) * avlist++) {
	switch (attr) {
	case PANEL_VALUE:
	    new_value = (char *) *avlist++;
	    break;

	case PANEL_VALUE_FONT:
	    dp->font = (Pixfont *) LINT_CAST(*avlist++);
	    value_changed = TRUE;
	    break;

	case PANEL_VALUE_UNDERLINED:
	    dp->underline = (int) *avlist++;
	    break;

	case PANEL_NOTIFY_LEVEL:
	    dp->notify_level = (Panel_setting) * avlist++;
	    break;

	case PANEL_NOTIFY_STRING:
	    dp->terminators = panel_strsave((char *) *avlist++);
	    break;

	case PANEL_VALUE_STORED_LENGTH:
	    dp->stored_length = (int) *avlist++;
	    dp->value = (char *) sv_realloc(dp->value, (u_int) (dp->stored_length + 1));
	    break;

	case PANEL_VALUE_DISPLAY_LENGTH:
	    dp->display_length = (int) *avlist++;
	    value_changed = TRUE;
	    break;

	case PANEL_MASK_CHAR:
	    dp->mask = (char) *avlist++;
	    break;

	case PANEL_READ_ONLY:
	    if (*avlist++)
		dp->flags |= READ_ONLY;
	    else
		dp->flags &= ~READ_ONLY;
	    break;

	default:
	    avlist = attr_skip(attr, avlist);
	    break;

	}
    }

    if (new_value) {
	char           *old_value = dp->value;

	dp->value = (char *) sv_calloc(1, (u_int) (dp->stored_length + 1));
	(void) strncpy(dp->value, new_value, dp->stored_length);

	free(old_value);

	/* Move the caret to the end of the text value */
	dp->last_char = strlen(dp->value) - 1;
	if (dp->last_char >= dp->display_length) {
	    /* Left-truncate displayed string, leaving room for left arrow */
	    dp->first_char = dp->last_char - dp->display_length + 2;
	    dp->caret_offset = ip->value_rect.r_width;
	}
	else {
	    dp->first_char = 0;
	    dp->caret_offset = strlen(dp->value) * dp->font->pf_defaultsize.x;
	}

	/* PUT AFTER ASSIGNMENT OF dp->first_char, dp->last_char */
	/* release any held selection */
	cancel_preview(ip, (Event *) NULL);

	/* Must be here until panel_seln_dehilite fixed? */
	paint_caret(ip, PIX_CLR);
    }

    /*
     * update the value & items rect if the width or height of the value has
     * changed.
     */
    if (value_changed) {
	ip->value_rect.r_width = panel_col_to_x(dp->font, dp->display_length);
	ip->value_rect.r_height = dp->font->pf_defaultsize.y;
	ip->rect = panel_enclosing_rect(&ip->label_rect, &ip->value_rect);
    }

    return 1;
}


/***********************************************************************/
/* get_attr                                                            */
/* returns the current value of an attribute for the text item.        */
/***********************************************************************/

static          caddr_t
get_attr(ip, which_attr)
    panel_item_handle ip;
    register Panel_attribute which_attr;
{
    register text_data *dp = textdp(ip);

    switch (which_attr) {
    case PANEL_VALUE:
	return (caddr_t) dp->value;

    case PANEL_VALUE_FONT:
	return (caddr_t) dp->font;

    case PANEL_VALUE_STORED_LENGTH:
	return (caddr_t) dp->stored_length;

    case PANEL_VALUE_DISPLAY_LENGTH:
	return (caddr_t) dp->display_length;

    case PANEL_NOTIFY_LEVEL:
	return (caddr_t) dp->notify_level;

    case PANEL_NOTIFY_STRING:
	return (caddr_t) dp->terminators;

    case PANEL_READ_ONLY:
	if (dp->flags & READ_ONLY)
	    return (caddr_t) TRUE;
	else
	    return (caddr_t) FALSE;

    default:
	return panel_get_generic(ip, which_attr);
    }
}

/***********************************************************************/
/* remove                                                              */
/***********************************************************************/

static          Panel_item
remove(ip)
    register panel_item_handle ip;
{
    register panel_handle panel = ip->panel;
    register panel_item_handle prev_item, next_item;
    short           had_caret_seln = FALSE;

    /* READ_ONLY text can accept caret to allow scrolling */
#ifdef OBSOLETE
    if (textdp(ip)->flags & READ_ONLY)
	return NULL;
#endif

    next_item = textdp(ip)->next;

    /* if already removed then ignore remove() */
    if (!next_item)
	return NULL;

    /*
     * cancel the selection if this item owns it.
     */
    if (panel_seln(panel, SELN_PRIMARY)->ip == ip)
	panel_seln_cancel(panel, SELN_PRIMARY);

    if (panel_seln(panel, SELN_SECONDARY)->ip == ip)
	panel_seln_cancel(panel, SELN_SECONDARY);

    if (panel_seln(panel, SELN_CARET)->ip == ip) {
	had_caret_seln = TRUE;
	panel_seln_cancel(panel, SELN_CARET);
    }

    /* if ip is the only text item, then remove caret for panel */
    if (next_item == ip) {
	textdp(ip)->next = NULL;
	textdp(ip)->flags &= ~HASCARET;
	panel->caret = NULL;
	return NULL;
    }

    /* find the next item */
    for (prev_item = next_item;
	    textdp(prev_item)->next != ip;
	    prev_item = textdp(prev_item)->next
	);

    /* unlink ip from the list */
    textdp(prev_item)->next = next_item;
    textdp(ip)->next = NULL;

    /* reset the caret */
    if (panel->caret == ip) {
	textdp(ip)->flags &= ~HASCARET;
	panel->caret = next_item;
	textdp(next_item)->flags |= HASCARET;
	if (had_caret_seln)
	    panel_seln(panel, SELN_CARET)->ip = panel->caret;
    }

    return (Panel_item) ip;
}


/*************************************************************************/
/* restore                                                               */
/* this code assumes that the caller has already set ip to be not hidden */
/*************************************************************************/

static          Panel_item
restore(ip)
    register panel_item_handle ip;
{
    register panel_item_handle prev_item, next_item;
    register text_data *dp, *prev_dp;

    /* READ_ONLY text can accept caret to allow scrolling */
#ifdef OBSOLETE
    if (textdp(ip)->flags & READ_ONLY)
	return NULL;
#endif

    /* if not removed then ignore restore() */
    if (textdp(ip)->next)
	return NULL;

    /* find next non_hidden text item following ip in the circular list */
    /* of caret items (could be ip itself ) */
    next_item = ip;
    do {
	next_item = panel_successor(next_item);	/* find next unhidden item */
	if (!next_item)
	    next_item = ip->panel->items;	/* wrap to start of list   */
    } while ((next_item->item_type != PANEL_TEXT_ITEM) || hidden(next_item));

    /* now find the previous text item of next_item in the caret list */
    prev_item = next_item;
    prev_dp = textdp(prev_item);
    /* if next_item is different from ip, then find the previous item */
    if (next_item != ip)
	while (prev_dp->next != next_item) {
	    prev_item = prev_dp->next;
	    prev_dp = textdp(prev_item);
	}
    /* prev_item now points to the (circular) 	 */
    /* predecessor of ip; if ip is sole text item, */
    /* prev_item == next_item == ip.		 */

    /* link ip into the caret list */
    dp = textdp(ip);
    dp->next = next_item;
    prev_dp->next = ip;

    /* deselect the previous caret item */
    deselect(ip->panel);

    /* give the caret to this item */
    ip->panel->caret = ip;
    dp->flags |= HASCARET;
    /* if we have the caret selection, change the item */
    if (panel_seln(ip->panel, SELN_CARET)->ip)
	panel_seln(ip->panel, SELN_CARET)->ip = ip;
    /* note that the caret will be drawn when the item is painted */

    return (Panel_item) ip;
}

/***********************************************************************/
/* insert                                                              */
/***********************************************************************/

static
insert(ip)
    register panel_item_handle ip;
/*
 * insert inserts ip into the caret list for ip->panel.
 */
{
    register panel_item_handle head;
    register text_data *dp;

    /* READ_ONLY text can accept caret to allow scrolling */
#ifdef OBSOLETE
    if (textdp(ip)->flags & READ_ONLY)
	return NULL;
#endif

    head = ip->panel->caret;
    if (head == NULL) {
	ip->panel->caret = ip;
	dp = textdp(ip);
	dp->flags |= HASCARET;
	head = ip;
    }
    else {
	/* find the last caret item */
	for (dp = textdp(head); dp->next != head; dp = textdp(dp->next));

	/* link after the last */
	dp->next = ip;
    }
    /* point back to the head of the list */
    dp = textdp(ip);
    dp->next = head;
}

/***********************************************************************/
/* destroy                                                             */
/***********************************************************************/

static
destroy(dp)
    register text_data *dp;
{
    free(dp->value);
    free(dp->terminators);
    free((char *) dp);
}



/***********************************************************************/
/* paint                                                               */
/***********************************************************************/

static
paint(ip)
    register panel_item_handle ip;
{
    (void) panel_paint_image(ip->panel, &ip->label, &ip->label_rect,
			     PIX_COLOR(ip->color_index));
    paint_text(ip);
}


/***********************************************************************/
/* paint_text                                                          */
/***********************************************************************/

static
paint_text(ip)
    panel_item_handle ip;
{
    register text_data *dp = textdp(ip);

    /* compute the caret position */
    update_value_offset(ip, 0, 0);
    update_caret_offset(ip, 0);

    /* don't paint the text if masked */
    if (dp->mask != ' ')
	paint_value(ip, PV_HIGHLIGHT);
}


static void
paint_value(ip, highlight)
    register panel_item_handle ip;
    u_char          highlight;
/*
 * paint_value clears the value rect for ip and paints the string value
 * clipped to the left of the rect.
 */
{
    register text_data *dp = textdp(ip);
    register panel_handle panel = ip->panel;
    register int    x = ip->value_rect.r_left;
    register int    y = ip->value_rect.r_top;
    register Rect  *r;
    int             i, j, len;
    char           *str;

    /* Get the actual characters which will be displayed */
    len = dp->last_char - dp->first_char + 2;
    str = (char *) sv_malloc(len);
    for (j = 0, i = dp->first_char; i <= dp->last_char; i++, j++)
	str[j] = dp->value[i];
    str[len - 1] = '\0';

    /*
     * clear the value rect.  Actually, clear an area slightly larger than the
     * value rect to prevent remnants of arrows being left on the screen
     */

    r = &ip->value_rect;
    (void) panel_pw_writebackground(panel,
				    r->r_left, r->r_top,
			            r->r_width + (dp->font->pf_defaultsize.x / 2),
				    r->r_height, PIX_CLR);

    /* draw the left clip arrow if needed */
    if (dp->first_char) {
	/* center the arrow vertically in the value rect */
	(void) panel_pw_write(panel, x,
	  y + (ip->value_rect.r_height - panel_left_arrow_pr.pr_height) / 2,
		panel_left_arrow_pr.pr_width, panel_left_arrow_pr.pr_height,
	  PIX_SRC | PIX_COLOR(ip->color_index), &panel_left_arrow_pr, 0, 0);
	x += panel_left_arrow_pr.pr_width;
    }
    /* draw the text */
    if (dp->mask == '\0')	/* not masked */
	(void) panel_pw_text(panel, x, y + panel_fonthome(dp->font), PIX_SRC | PIX_COLOR(ip->color_index), dp->font,
			     &str[0]);
    else {			/* masked */
	char           *buf;
	int             length, i;
	length = dp->last_char - dp->first_char + 2;
	buf = (char *) sv_malloc(length);
	for (j = 0, i = dp->first_char; i <= dp->last_char; i++, j++)
	    buf[j] = dp->mask;
	buf[length - 1] = '\0';
	(void) panel_pw_text(panel, x, y + panel_fonthome(dp->font), PIX_SRC | PIX_COLOR(ip->color_index), dp->font,
			     buf);
	free(buf);
    }

    /* draw the right clip arrow if needed */
    if (dp->last_char < (strlen(dp->value) - 1)) {
	x += (strlen(str) * dp->font->pf_defaultsize.x);
	x += panel_right_arrow_pr.pr_width / 2;
	/* center the arrow vertically in the value rect */
	(void) panel_pw_write(panel, x,
	 y + (ip->value_rect.r_height - panel_right_arrow_pr.pr_height) / 2,
	      panel_right_arrow_pr.pr_width, panel_right_arrow_pr.pr_height,
	 PIX_SRC | PIX_COLOR(ip->color_index), &panel_right_arrow_pr, 0, 0);
    }
    free(str);

    if (highlight) {
	/* re-hilite if this is a selection item */
	if (ip == panel_seln(panel, SELN_PRIMARY)->ip &&
		!panel_seln(panel, SELN_PRIMARY)->is_null)
	    panel_seln_hilite(panel_seln(panel, SELN_PRIMARY));
	if (ip == panel_seln(panel, SELN_SECONDARY)->ip &&
		 !panel_seln(panel, SELN_SECONDARY)->is_null)
	    panel_seln_hilite(panel_seln(panel, SELN_SECONDARY));
    }
}


/***********************************************************************/
/* paint_caret                                                         */
/***********************************************************************/

static
paint_caret(ip, op)
    panel_item_handle ip;
    int             op;
{
    register panel_handle panel = ip->panel;
    register text_data *dp = textdp(ip);
    register int    x, max_x;
    int             painted_caret_offset;

    if (((op == PIX_SET && panel->caret_on) ||
	 (op == PIX_CLR && !panel->caret_on)))
	return;

    panel->caret_on = panel->caret_on ? FALSE : TRUE;
    op = PIX_SRC ^ PIX_DST;

    /* paint the caret after the offset & above descender */
    painted_caret_offset = (dp->mask == ' ') ? 0 : dp->caret_offset;
    x = ip->value_rect.r_left + painted_caret_offset -
	dp->font->pf_defaultsize.x / 2;
    max_x = view_width(panel) + panel->h_offset;
    (void) panel_pw_write(panel,
			  (x > max_x - 7) ? (max_x - 3) : x,
		      ip->value_rect.r_top + dp->font->pf_defaultsize.y - 4,
			  7, 7, op, panel->caret_pr, 0, 0);
}


/***********************************************************************/
/* ops vector routines                                                 */
/***********************************************************************/

/* ARGSUSED */
static
begin_preview(ip, event)
    register panel_item_handle ip;
    Event          *event;
{
    text_data      *dp = textdp(ip);
    register panel_handle panel = ip->panel;
    Seln_holder     holder;
    Panel_setting   current_state;
    u_int           is_null;
    int             ptr_offset;		/* mouse pointer offset */
    int             caret_offset;
    u_char          adjust_right;
    u_char          pending_delete;
    int             first_offset;
    int             last_offset;


    current_state = (Panel_setting) panel_get(ip, PANEL_MOUSE_STATE);

    /*
     * Ignore the middle mouse button when the mouse is dragged into an item
     * not containing the caret.
     */
    if (panel->caret != ip && current_state == PANEL_MIDDLE_DOWN &&
	    event_id(event) == LOC_DRAG)
	return;

    holder = panel_seln_inquire(panel, SELN_UNSPECIFIED);
    if (holder.rank == SELN_PRIMARY) {
        if (ip != panel_seln(panel, SELN_PRIMARY)->ip)
	    primary_ms_clicks = 0;
	ms_clicks = primary_ms_clicks;
    }
    else {	/* secondary selection */
        if (ip != panel_seln(panel, SELN_SECONDARY)->ip)
	    secondary_ms_clicks = 0;
	ms_clicks = secondary_ms_clicks;
    }

    /*
     * If nothing is selected in this item, then allow the middle button
     * to pick (i.e., make it look like the left button was clicked).
     */
    pending_delete = FALSE;
    if (event_id(event) == MS_MIDDLE && ms_clicks == 0) {
	event->ie_code = MS_LEFT;

	/* READ_ONLY text can never be deleted */
	if (!(dp->flags & READ_ONLY)) {
	    /* need to check this since this is actually a middle button */
	    pending_delete = adjust_is_pending_delete(panel) ? TRUE : FALSE;
	    /*
	     * bug 1028689 -- added support for control char toggling of
	     * select vs pending delete
	     */
	    if (event_ctrl_is_down(event))
		pending_delete = !pending_delete;
	}
    }

    ptr_offset = event->ie_locx - ip->value_rect.r_left + 1;
    if (ptr_offset < 0)		/* yes, this can happen */
	ptr_offset = 0;

    if ((event_id(event) == MS_LEFT) ||
	    ((event_id(event) == LOC_DRAG) &&
	     (current_state == PANEL_LEFT_DOWN))) {
	/*
	 * This is a mouse left click event.
	 */

	/* READ_ONLY text can never be deleted */
	if (!(dp->flags & READ_ONLY)) {
	    /*
	     * bug 1028689 -- added support for control char toggling of
	     * select vs pending delete
	     */
	    if (event_ctrl_is_down(event))
		pending_delete = !pending_delete;
	}

	/*
	 * Set seln_first and seln_last to the character pointed to by the mouse.
	 */

	/*
	 * bug 1028685 --  pointer anywhere on character now selects that
	 * character
	 */
	dp->seln_first = ptr_offset / dp->font->pf_defaultsize.x;

	if (dp->last_char < (strlen(dp->value) - 1))	/* right arrow showing */
	    if (dp->seln_first >= dp->display_length-1)
	        dp->seln_first = dp->display_length - 2;

	if (dp->first_char) {				/* left arrow showing */
	    dp->seln_first += dp->first_char - 1;
	    if (dp->seln_first < dp->first_char)
		dp->seln_first = dp->first_char;
	}

	if (dp->seln_first >= strlen(dp->value))
	    if ((dp->seln_first = strlen(dp->value) - 1) < 0)
		dp->seln_first = 0;

	dp->seln_last = dp->seln_first;

	if (event_id(event) == MS_LEFT) {
	    /*
	     * If this is a quick click then increment clicks
	     * otherwise reset clicks to 1.
	     */
	    if (panel_dclick_is(&event_time(event)))
	        ms_clicks++;
	    else
	        ms_clicks = 1;
	}

	if (ms_clicks == 2)		/* select word */
	    panel_find_word(dp, &dp->seln_first, &dp->seln_last);
	else if (ms_clicks >= 3) {	/* select line */
	    dp->seln_first = 0;
	    dp->seln_last = strlen(dp->value) - 1;
        }
    }
    else if (ms_clicks > 0 &&
	     ((event_id(event) == MS_MIDDLE) ||
	      ((event_id(event) == LOC_DRAG) &&
	       (current_state == PANEL_MIDDLE_DOWN)))) {

	/* READ_ONLY text can never be deleted */
	if (!(dp->flags & READ_ONLY)) {
	    pending_delete = adjust_is_pending_delete(panel) ? TRUE : FALSE;
	    /*
	     * bug 1028689 -- added support for control char toggling of
	     * select vs pending delete
	     */
	    if (event_ctrl_is_down(event))
		pending_delete = !pending_delete;
	}

	/*
	 * bug 1028685 --  pointer anywhere on character now selects that
	 * character
	 */
	 dp->ext_first = ptr_offset / dp->font->pf_defaultsize.x;

	if (dp->last_char < strlen(dp->value) - 1)    /* right arrow showing */
	    if (dp->ext_first >= dp->display_length-1)
	        dp->ext_first = dp->display_length - 2;

	if (dp->first_char) {		              /* left arrow showing */
	    dp->ext_first += dp->first_char - 1;
	    if (dp->ext_first < dp->first_char)
		dp->ext_first = dp->first_char;
	}
	if (dp->ext_first >= strlen(dp->value))
	    if ((dp->ext_first = strlen(dp->value) - 1) < 0)
		dp->ext_first = 0;

	dp->ext_last = dp->ext_first;

	if (ms_clicks >= 3) {
	    dp->seln_first = 0;
	    dp->seln_last = strlen(dp->value) - 1;
	}
	else {
	    if (ms_clicks == 2)		/* word adjust */
		panel_find_word(dp, &dp->ext_first, &dp->ext_last);

	    /* Adjust first or last character selected */
	    if (dp->ext_last > dp->seln_last)
		adjust_right = TRUE;
	    else if (dp->ext_first < dp->seln_first)
		adjust_right = FALSE;	/* adjust left */
	    else if ((dp->ext_first - dp->seln_first) <
		     (dp->seln_last - dp->ext_first))
		adjust_right = FALSE;	/* adjust left */
	    else
		adjust_right = TRUE;

	    if (adjust_right)
		dp->seln_last = dp->ext_last;
	    else 	/* adjust left */
		dp->seln_first = dp->ext_first;
	}
    }

    /*
     * Calculate caret offset
     * If setting or adjusting the primary selection:
     *     Make sure either seln_first or seln_last is showing
     *     and place caret before seln_first or after seln_last
     *     depending upon which is nearest to the mouse pointer
     */

    first_offset = (dp->seln_first - dp->first_char)
		 * dp->font->pf_defaultsize.x;
    last_offset = (dp->seln_last - dp->first_char + 1)
		* dp->font->pf_defaultsize.x;
    if (dp->first_char) {
	first_offset += panel_left_arrow_pr.pr_width;
	last_offset += panel_left_arrow_pr.pr_width;
    }

    if (strlen(dp->value) == 0)
	caret_offset = 0;
    else if (ptr_offset < first_offset ||
	    last_offset - ptr_offset > ptr_offset - first_offset) {
	if (holder.rank == SELN_PRIMARY && dp->seln_first < dp->first_char) {
	    /* need to scroll caret into view */
	    dp->first_char = dp->seln_first;
	    if (strlen(dp->value) <= dp->display_length)
		dp->last_char = strlen(dp->value) - 1;
	    else {
		dp->last_char = dp->first_char + dp->display_length - 1;
		if (dp->first_char)
		    dp->last_char--;
		if (strlen(dp->value) - dp->first_char + 1 > dp->display_length)
		    dp->last_char--;
	    }

	    paint_caret(ip, PIX_CLR);
	    paint_value(ip, PV_HIGHLIGHT);
	}
	caret_offset = (dp->seln_first - dp->first_char)
			 * dp->font->pf_defaultsize.x;
    }
    else {
	if (holder.rank == SELN_PRIMARY && dp->seln_last > dp->last_char) {
	    /* need to scroll caret into view */
	    dp->last_char = dp->seln_last;
	    dp->first_char = dp->last_char - dp->display_length + 1;
	    if (dp->first_char)
		dp->first_char++;
	    if (strlen(dp->value) - dp->first_char + 1 > dp->display_length)
		dp->first_char++;

	    paint_caret(ip, PIX_CLR);
	    paint_value(ip, PV_HIGHLIGHT);
	}
	caret_offset = (dp->seln_last - dp->first_char + 1)
			 * dp->font->pf_defaultsize.x;
    }

    if (dp->first_char)		/* adjust for left arrow */
	caret_offset += panel_left_arrow_pr.pr_width;

    /* Caret offset cannot exceed value offset */
    if (caret_offset > strlen(dp->value) * dp->font->pf_defaultsize.x)
	caret_offset = strlen(dp->value) * dp->font->pf_defaultsize.x;

    if (holder.rank == SELN_PRIMARY) {
	dp->caret_offset = caret_offset;
	primary_ms_clicks = ms_clicks;
	primary_seln_first = dp->seln_first;
	primary_seln_last = dp->seln_last;
	primary_pending_delete = pending_delete;
    }
    else {	/* SECONDARY SELECTION */
	dp->secondary_caret_offset = caret_offset;
	secondary_ms_clicks = ms_clicks;
	secondary_seln_first = dp->seln_first;
	secondary_seln_last = dp->seln_last;
	secondary_pending_delete = pending_delete;
    }

    /*
     * Make this item the selection (includes highlighting).
     * Must be done AFTER caret placement since scrolling may occur
     */
    is_null = strlen(dp->value) == 0;
    panel_seln_acquire(panel, ip, holder.rank, is_null);

    if (holder.rank == SELN_PRIMARY) {
        /*
         * Make sure we have acquired the CARET selection token, too.
         * Paint caret AFTER painting value
         */
        dp->flags |= SELECTING_ITEM;
        dp->orig_caret = ip->panel->caret;
	panel_seln_acquire(panel, ip, SELN_CARET, TRUE);
        (void) panel_setcaret(panel, ip);	/* READ_ONLY text is selectable */
    }
}



/***********************************************************************/
/* cancel_preview                                                      */
/***********************************************************************/

/* ARGSUSED */
static
cancel_preview(ip, event)
    panel_item_handle ip;
    Event          *event;
{
    register text_data *dp = textdp(ip);
    register panel_handle panel = ip->panel;

    if (dp->flags & SELECTING_ITEM) {
	deselect(panel);
	panel->caret = dp->orig_caret;
	textdp(dp->orig_caret)->flags |= HASCARET;
	dp->orig_caret = (panel_item_handle) NULL;
	dp->flags &= ~SELECTING_ITEM;
    }
    if (ip == panel_seln(panel, SELN_PRIMARY)->ip)
	panel_seln_cancel(panel, SELN_PRIMARY);
    if (ip == panel_seln(panel, SELN_SECONDARY)->ip)
	panel_seln_cancel(panel, SELN_SECONDARY);
    if (ip == panel_seln(panel, SELN_CARET)->ip)
	panel_seln_cancel(panel, SELN_CARET);
}


/***********************************************************************/
/* accept_preview                                                      */
/***********************************************************************/

/* ARGSUSED */
static
accept_preview(ip, event)
    panel_item_handle ip;
    Event          *event;
{
    register text_data *dp = textdp(ip);
    register panel_handle panel = ip->panel;
    Seln_holder     holder;

    if (!(dp->flags & SELECTING_ITEM))
	return;

    dp->orig_caret = (panel_item_handle) NULL;
    dp->flags &= ~SELECTING_ITEM;
    /* Ask for kbd focus if this is a primary selection */
    holder = panel_seln_inquire(panel, SELN_UNSPECIFIED);
    if (holder.rank == SELN_PRIMARY)
	(void) win_set_kbd_focus(panel->windowfd, win_fdtonumber(panel->windowfd));
}


static
accept_menu(ip, event)
    panel_item_handle ip;
    Event          *event;
{
    struct menuitem *mi;

    /* Make sure the menu title reflects the label. */
    panel_sync_menu_title(ip);

    if (mi = panel_menu_display(ip, event)) {
	if ((panel_item_handle) LINT_CAST(panel_getcaret(ip->panel)) != ip)
	    (void) panel_setcaret(ip->panel, ip);
	event_id(event) = (short) mi->mi_data;
	accept_key(ip, event);
    }
}

#define CTRL_D_KEY	'\004'
#define CTRL_G_KEY	'\007'

/*
 * Since the keymapping system no longer supports full translation of event
 * ids, shiftmasks and flags we must revert to the old stuff that directly
 * recognizes the 3.x accelerators for CUT and PASTE.
 */
static short    keyboard_accel_state_cached;
static short    keyboard_accel_state;

/***********************************************************************/
/* accept_key                                                          */
/***********************************************************************/

static
accept_key(ip, event)
    register panel_item_handle ip;
    register Event *event;
{
    panel_handle    panel = ip->panel;
    register text_data *dp = textdp(ip);
    int             caret_was_on;
    int             notify_desired = FALSE;
    Panel_setting   notify_rtn_code;
    int             ok_to_insert;
    panel_selection_handle selection;
    extern char    *index();
    short           code;
    Event           save_event;
    int             status;
    int             keep_flags;

    /*
     * recognize arrow keys
     * 
     * NOTES: FAKE_ESC is used to handle recursion properly
     * 
     * Design Goals: ESC by itself should blink immediately and not be inserted
     * in the field ESC [ DIGITS CHAR should blink immediately and insert [
     * DIGITS CHAR in the field ESC [ C should be interpreted as a
     * RIGHT_CURSOR_KEY ESC [ D should be interpreted as a LEFT_CURSOR_KEY
     * ESC [ CHAR where CHAR is not C or D should blink and insert [ CHAR in
     * the field BACKSPACE must be handled correctly in all cases
     */

#define FAKE_ESC	0
#define ESC		27

    if (event_id(event) == ESC) {
	save_event = *event;

	/* set up for non-blocking read */
	keep_flags = fcntl(window_fd(ip->panel), F_GETFL, 0);
	fcntl(window_fd(ip->panel), F_SETFL, keep_flags | FNDELAY);
	status = window_read_event(ip->panel, event);
	if (status >= 0) {	/* event waiting after ESC */
	    if (event_id(event) == '[') {
		status = window_read_event(ip->panel, event);
		if (status >= 0) {	/* event waiting after ESC [ */
		    save_event = *event;
		    switch (event_id(event)) {
		    case 'A':
			event_id(event) = UP_CURSOR_KEY;
			break;

		    case 'B':
			event_id(event) = DOWN_CURSOR_KEY;
			break;

		    case 'C':
			event_id(event) = RIGHT_CURSOR_KEY;
			break;

		    case 'D':
			event_id(event) = LEFT_CURSOR_KEY;
			break;

		    default:
			event_id(event) = FAKE_ESC;
			accept_key(ip, event);
			event_id(event) = '[';
			accept_key(ip, event);
			*event = save_event;
			break;
		    }
		}
		else {		/* no event waiting after ESC [ */
		    event_id(event) = FAKE_ESC;
		    accept_key(ip, event);
		    event_id(event) = '[';
		}
	    }
	    else if (event_is_ascii(event) || event_is_meta(event)) {
		save_event = *event;
		event_id(event) = FAKE_ESC;
		accept_key(ip, event);
		*event = save_event;
	    }
	    else		/* ignore mouse event */
		*event = save_event;
	}
	else			/* no event waiting after ESC */
	    *event = save_event;

	/* restore to blocking read */
	fcntl(window_fd(ip->panel), F_SETFL, keep_flags);
    }

    /* change back to ESCAPE, now that the recursive part is finished */
    if (event_id(event) == FAKE_ESC)
	event_id(event) = ESC;

    if (event_shift_is_down(event)) {
	if (event_id(event) == dp->edit_bk_char)
	    event_set_action(event, ACTION_ERASE_CHAR_FORWARD);
	else if (event_id(event) == dp->edit_bk_word)
	    event_set_action(event, ACTION_ERASE_WORD_FORWARD);
	else if (event_id(event) == dp->edit_bk_line)
	    event_set_action(event, ACTION_ERASE_LINE_END);
    }
    code = event_action(event);

    if (primary_pending_delete) {

	/*
	 * If moving the caret to another text item via the keyboard, turn
	 * off the pending delete.  If moving the caret within the same
	 * field, make the selection non-pending.
	 */
	if (event_is_key_right(event) || (code == '\n') ||
		(code == '\r') || (code == '\t')) {
	    if (ip == panel_seln(panel, SELN_PRIMARY)->ip)
		panel_seln_dehilite(ip, SELN_PRIMARY);
	    else if (ip == panel_seln(panel, SELN_SECONDARY)->ip)
		panel_seln_dehilite(ip, SELN_SECONDARY);

	    primary_pending_delete = 0;

	    if ((code == LEFT_CURSOR_KEY) || (code == RIGHT_CURSOR_KEY)) {
		if (ip == panel_seln(panel, SELN_PRIMARY)->ip)
		    panel_seln_hilite(panel_seln(panel, SELN_PRIMARY));
		else if (ip == panel_seln(panel, SELN_SECONDARY)->ip)
		    panel_seln_hilite(panel_seln(panel, SELN_SECONDARY));
	    }
	}

	/*
	 * Turn off pending delete when editing the item with backspace & co.
	 */
	else if (panel_event_is_panel_semantic(event) ||
		 (code == dp->edit_bk_char) ||
		 (code == dp->edit_bk_word) ||
		 (code == dp->edit_bk_line)) {
	    if (ip == panel_seln(panel, SELN_PRIMARY)->ip)
		panel_seln_dehilite(ip, SELN_PRIMARY);
	    else if (ip == panel_seln(panel, SELN_SECONDARY)->ip)
		panel_seln_dehilite(ip, SELN_SECONDARY);

	    primary_pending_delete = 0;
	}

	/*
	 * Carry out pending delete on the selection.
	 */
	else if (!panel_event_is_sunview_semantic(event)) {
	    selection = panel_seln(panel, SELN_PRIMARY);
	    if (selection->ip) {
	        /* Copy the selection to the shelf */
	        panel_seln_shelve(panel, selection, SELN_PRIMARY);

	        /* Delete the selected characters */
	        panel_seln_delete(selection->ip, SELN_PRIMARY);
		selection->ip = (panel_item_handle)0;
	        (void) seln_done(panel->seln_client, SELN_PRIMARY);
	    }
	    primary_pending_delete = 0;
	}
    }
    else {

	/*
	 * No pending-delete.  If editting this item or moving to another
	 * one, unhilite in case there was a non-pending selection.
	 */
	if ((panel_printable_char(code) ||
	     (code == dp->edit_bk_char) ||
	     (code == dp->edit_bk_word) ||
	     (code == dp->edit_bk_line) ||
	     panel_event_is_panel_semantic(event) ||
	     (code == '\n') || (code == '\r') || (code == '\t'))
		&&
		((code != LEFT_CURSOR_KEY) &&
		 (code != RIGHT_CURSOR_KEY))) {

	    if (ip == panel_seln(panel, SELN_PRIMARY)->ip)
		panel_seln_dehilite(ip, SELN_PRIMARY);
	    else if (ip == panel_seln(panel, SELN_SECONDARY)->ip)
		panel_seln_dehilite(ip, SELN_SECONDARY);
	}
    }

    /*
     * If hiliting turned off, set click count to 0.  Other routines, like
     * panel_seln_delete() & panel_seln_hilite will key on this value.
     */
    if ((event_is_euc(event) || event_is_iso(event) ||
	 panel_event_is_panel_semantic(event) ||
	 (code == dp->edit_bk_char) ||
	 (code == dp->edit_bk_word) ||
	 (code == dp->edit_bk_line))
	    &&
	     ((code != ACTION_CUT) &&
	      (code != LEFT_CURSOR_KEY) &&
	      (code != RIGHT_CURSOR_KEY))) {
	primary_ms_clicks = 0;
    }

    /* READ_ONLY text can accept caret to allow scrolling */
#ifdef OBSOLETE
    if (dp->flags & READ_ONLY)
	return NULL;
#endif

    /* get the caret selection */
    selection = panel_seln(panel, SELN_CARET);
    if (panel->seln_client && !selection->ip)
	if (event_is_down(event) &&
		(event_is_euc(event) || event_is_iso(event) ||
		 event_action(event) == GET_KEY))
	    panel_seln_acquire(panel, ip, SELN_CARET, TRUE);

    /*
     * not interested in function keys, except for acquiring the caret
     * selection.
     */
    /*
     * also, never pass ctrl-D or ctrl-G, which have been mapped to the
     * Delete and Get function keys, to the notify proc
     */
    /*
     * Cache and check state of 4.0 accelerators; if not enabled then allow
     * use of 3.x accelerators
     */
    if (!keyboard_accel_state_cached) {
	keyboard_accel_state = (!strcmp("Enabled",
					(char *) defaults_get_string(
				 "/Compatibility/New_keyboard_accelerators",
						    "Enabled", 0))) ? 1 : 0;
	keyboard_accel_state_cached = 1;
    }

    if (panel_event_is_sunview_semantic(event) ||
	    (keyboard_accel_state && event_id(event) == CTRL_D_KEY) ||
	    (keyboard_accel_state && event_id(event) == CTRL_G_KEY))
	return;

    switch (dp->notify_level) {
    case PANEL_ALL:
	notify_desired = TRUE;
	break;
    case PANEL_SPECIFIED:
	notify_desired = index(dp->terminators, code) != 0;
	break;
    case PANEL_NON_PRINTABLE:
	notify_desired = !panel_printable_char(code);
	break;
    case PANEL_NONE:
	notify_desired = FALSE;
	break;
    }
    if (notify_desired) {
	notify_rtn_code = (Panel_setting) (*ip->notify) (ip, event);
	ok_to_insert = notify_rtn_code == PANEL_INSERT;
    }
    else {
	/* editting characters are always ok to insert */
	ok_to_insert = (panel_printable_char(code) ||
			(code == dp->edit_bk_char) ||
			(code == dp->edit_bk_word) ||
			(code == dp->edit_bk_line) ||
			(code == ACTION_ERASE_CHAR_BACKWARD) ||
			(code == ACTION_ERASE_CHAR_FORWARD) ||
			(code == ACTION_ERASE_WORD_BACKWARD) ||
			(code == ACTION_ERASE_WORD_FORWARD) ||
			(code == ACTION_ERASE_LINE_BACKWARD) ||
			(code == ACTION_ERASE_LINE_END));
    }

    if (dp->flags & READ_ONLY)
	ok_to_insert = FALSE;

    /* Process cursor key */
    if (event_is_key_right(event)) {
	code = event_id(event);
	switch (event_id(event)) {
	case (UP_CURSOR_KEY):
	    notify_rtn_code = PANEL_PREVIOUS;
	    notify_desired = TRUE;
	    ok_to_insert = FALSE;
	    break;
	case (DOWN_CURSOR_KEY):
	    notify_rtn_code = PANEL_NEXT;
	    notify_desired = TRUE;
	    ok_to_insert = FALSE;
	    break;
	case (RIGHT_CURSOR_KEY):
	case (LEFT_CURSOR_KEY):
	    notify_desired = FALSE;
	    ok_to_insert = FALSE;
	    break;
	default:
	    break;
	}
    }

    /* turn off the caret */
    caret_was_on = 0;
    if (ip->panel->caret_on && ip == panel->caret) {
	caret_was_on = 1;
        paint_caret(ip, PIX_CLR);
    }

    /* do something with the character */
    /* use 'event_action(event)' and not 'code' because client may have */
    /* changed the event */
    update_value(ip, event_action(event), ok_to_insert);

    if (caret_was_on)
	paint_caret(ip, PIX_SET);

    if (notify_desired && ip == panel->caret)
	switch (notify_rtn_code) {
	case PANEL_NEXT:
	    (void) panel_advance_caret((Panel) panel);
	    break;

	case PANEL_PREVIOUS:
	    (void) panel_backup_caret((Panel) panel);
	    break;

	default:
	    break;
	}
}

static
int 
panel_printable_char(code)
    int             code;	/* event code */
{
    /*
     * return( (code >= ' ' && code <= '~') || (code >= ISO_FIRST+0xA0 &&
     * code <= ISO_FIRST+0xFF) );
     */
    return ((code >= 0 && code <= 0xFF && isprint(code)) ||
	    (code >= ISO_FIRST + 0xA0 && code <= ISO_FIRST + 0xFF));
}



/*
 * IMPORTANT: dp->caret_offset always points to the pixel offset of the caret
 * relative to the left boundary.  It includes the left arrow if it exists,
 * so its value MAY NOT BE a multiple of dp->font->pf_defaultsize.x.
 * 
 * In some routines, the local variable, caret_offset, is used for computations
 * & comparisons involving string lengths in pixel units, such as computing
 * the insertion point and new caret position. It treats the left and right
 * arrows as characters, whose width is identical to the font width. So its
 * value should ALWAYS be a multiple of dp->font->pf_defaultsize.x.
 * 
 * In summary, the displayed text field width, in pixels, including the arrows,
 * may not always be a multiple of the font width.  The text displayed is
 * always placed immediately after the left arrow, whose pixel width is
 * independent of the font width.
 * 
 * The hilite/dehilite routines must also account for this.
 */

static
update_value(ip, code, ok_to_insert)
    panel_item_handle ip;
    register int    code;
    int             ok_to_insert;
/*
 * update_value updates the value of ip according to code.  If code is an
 * edit character, the appropriate edit action is taken. Otherwise, if
 * ok_to_insert is true, code is appended to ip->value.
 */
{
    register text_data *dp = textdp(ip);
    register char  *sp;		/* string value */
    int             orig_len, new_len;	/* before & after lengths */
    register int    i;		/* counter */
    register int    x;		/* left of insert/del point */
    int             was_clipped;/* TRUE if value was clipped */
    int             orig_offset;/* before caret offset */
    int             caret_shift = 0;	/* number of positions to move caret */
    int             insert_pos;	/* position for character add/delete */
    int             j;
    int             val_shift = 0;	/* number of positions to shift value
					 * display */
    int             val_change = 0;	/* number of positions to move value */
    int             pix_display_length;	/* dp->display_length, in pixel units */
    int             char_code;
    int             caret_offset;
    int             saved_caret_offset;

    caret_offset = dp->caret_offset;

    /* Get the character position in question, relative of left boundary. */
    if (caret_offset < 0)
	insert_pos = 0;
    else {
	if (dp->first_char)
	    caret_offset += dp->font->pf_defaultsize.x -
		panel_left_arrow_pr.pr_width;
	insert_pos = caret_offset / dp->font->pf_defaultsize.x;
    }

    /* Get actual positional character within the string */
    if (dp->first_char)
	insert_pos += dp->first_char - 1;

    sp = dp->value;
    orig_len = strlen(sp);

    if ((code == dp->edit_bk_char) ||
	    (code == ACTION_ERASE_CHAR_BACKWARD)) {	/* backspace */
	/* Allow notify_proc to override editting characters. */
	/* Supports read-only text fields. */
	if (!ok_to_insert) {
	    blink_value(ip);
	    goto END_OF_UPDATE_VALUE;
	}

	/* Nothing to backspace if caret is at left boundary. */
	if (caret_offset == 0)
	    goto END_OF_UPDATE_VALUE;

	/* Can't show result of backspace if display length exceeded and */
	/* caret is to the right of the panel left arrow.  The moral here */
	/* is that you can't delete what you can't see. */
	if ((orig_len > dp->display_length) && (dp->first_char) &&
		(caret_offset <= dp->font->pf_defaultsize.x))
	    goto END_OF_UPDATE_VALUE;

	if ((*sp) && (insert_pos > 0)) {
	    for (i = insert_pos; i < orig_len; i++)
		sp[i - 1] = sp[i];
	    sp[orig_len - 1] = '\0';
	    insert_pos--;
	    caret_shift = -1;
	    val_change = -1;

	    /* If clipped at left boundary, leave caret alone. */
	    /* Characters will shift in from the left. */
	    if (dp->first_char > 1)
		caret_shift = 0;
	}

    }

    else if (code == ACTION_ERASE_CHAR_FORWARD) {	/* forespace */

	/* Allow notify_proc to override editting characters. */
	/* Supports read-only text fields. */
	if (!ok_to_insert) {
	    blink_value(ip);
	    goto END_OF_UPDATE_VALUE;
	}

	/* Can't show result of forespace if display length exceeded and */
	/* caret is to the left of the panel right arrow.  The moral here */
	/* is that you can't delete what you can't see. */
	pix_display_length = dp->display_length * dp->font->pf_defaultsize.x;
	if (caret_offset >= pix_display_length)
	    goto END_OF_UPDATE_VALUE;

	pix_display_length = (dp->last_char - dp->first_char + 1);
	if (dp->first_char)
	    pix_display_length++;
	pix_display_length *= dp->font->pf_defaultsize.x;
	if (caret_offset >= pix_display_length)
	    goto END_OF_UPDATE_VALUE;

	if ((*sp) && (insert_pos >= 0)) {
	    for (i = insert_pos; i < orig_len; i++)
		sp[i] = sp[i + 1];
	    sp[orig_len - 1] = '\0';
	    caret_shift = 0;
	    val_change = 0;
	    if ((dp->last_char >= (strlen(sp) - 1)) && (dp->last_char > 1)) {
		val_change = -1;
		if (dp->first_char > 2)
		    caret_shift = 1;
	    }
	}
    }

    else if ((code == dp->edit_bk_word) ||
	     (code == ACTION_ERASE_WORD_BACKWARD)) {	/* backword */

	/*
	 * for(i = orig_len - 1; i >= 0 && sp[i] == ' '; i--); for(; i >= 0
	 * && sp[i] != ' '; i--); sp[i + 1] = '\0';
	 */
	/* Allow notify_proc to override editting characters. */
	/* Supports read-only text fields. */
	if (!ok_to_insert) {
	    blink_value(ip);
	    goto END_OF_UPDATE_VALUE;
	}

	/* skip back past blanks */
	if (insert_pos > orig_len)
	    insert_pos -= (dp->first_char - 1);
	for (i = insert_pos - 1; (i >= 0) && (sp[i] == ' '); i--);
	for (; (i >= 0) && (sp[i] != ' '); i--);
	if (i < 0)
	    i = 0;
	if (i > 0)
	    i++;
	caret_shift = i - insert_pos;
	val_change = i - insert_pos;
	for (j = insert_pos; j <= orig_len; j++, i++)
	    sp[i] = sp[j];
	insert_pos += caret_shift;
    }

    else if (code == ACTION_ERASE_WORD_FORWARD) {	/* foreword */

	/* Allow notify_proc to override editting characters. */
	/* Supports read-only text fields. */
	if (!ok_to_insert) {
	    blink_value(ip);
	    goto END_OF_UPDATE_VALUE;
	}

	/* skip back past blanks */
	for (i = insert_pos; (i < orig_len) && (sp[i] == ' '); i++);
	for (; (i < orig_len) && (sp[i] != ' '); i++);
	if (i >= orig_len)
	    i = orig_len - 1;
	if (i < (orig_len - 1))
	    i--;
	caret_shift = 0;
	val_change = 0;
	for (j = insert_pos; i < orig_len; j++, i++)
	    sp[j] = sp[i + 1];
	if ((dp->last_char >= (strlen(sp) - 1)) && (dp->first_char > 1)) {
	    val_change = strlen(sp) - 1 - dp->last_char;
	    if (dp->last_char < (orig_len - 1))
		val_change--;
	    caret_shift = -val_change;
	}
    }

    else if ((code == dp->edit_bk_line) ||
	     (code == ACTION_ERASE_LINE_BACKWARD)) {	/* backline */
	/* Allow notify_proc to override editting characters. */
	/* Supports read-only text fields. */
	if (!ok_to_insert) {
	    blink_value(ip);
	    goto END_OF_UPDATE_VALUE;
	}

	/* sp[0] = '\0'; */
	caret_shift = -insert_pos;
	val_change = -insert_pos;
	for (i = 0, j = insert_pos; j <= orig_len; i++, j++)
	    sp[i] = sp[j];
	insert_pos = 0;
    }

    else if (code == ACTION_ERASE_LINE_END) {	/* foreline */

	/* Allow notify_proc to override editting characters. */
	/* Supports read-only text fields. */
	if (!ok_to_insert) {
	    blink_value(ip);
	    goto END_OF_UPDATE_VALUE;
	}

	caret_shift = 0;
	val_change = 0;
	sp[insert_pos] = '\0';
	if (dp->first_char > 1) {
	    val_change = strlen(sp) - 1 - dp->last_char;
	    if (dp->last_char < (orig_len - 1))
		val_change--;
	    caret_shift = -val_change;
	}
    }

    else if (code == RIGHT_CURSOR_KEY) {
	caret_shift = 1;
	if ((dp->last_char < (orig_len - 1)) &&
		(caret_offset >= (dp->display_length - 1) * dp->font->pf_defaultsize.x))
	    val_shift = 1;	/* display will include next char to right */
    }
    else if (code == LEFT_CURSOR_KEY) {
	caret_shift = -1;
	if ((dp->first_char) && (caret_offset <= dp->font->pf_defaultsize.x))
	    val_shift = -1;	/* display will include next char to left */
    }
    else if (code == UP_CURSOR_KEY) {
    }				/* do nothing */
    else if (code == DOWN_CURSOR_KEY) {
    }				/* do nothing */
    else if (code == '\n') {
    }				/* do nothing */
    else if (code == '\r') {
    }				/* do nothing */
    else if (code == '\t') {
    }				/* do nothing */
    else {
	if (ok_to_insert) {	/* insert */
	    if (orig_len < dp->stored_length) {	/* there is room */
		if (ISO_FIRST <= code && code <= ISO_LAST)
		    char_code = code - ISO_FIRST;
		else
		    char_code = code;
		for (i = orig_len; i > insert_pos; i--)
		    sp[i] = sp[i - 1];
		sp[insert_pos] = (char) char_code;
		sp[orig_len + 1] = '\0';
		caret_shift = 1;
		val_change = 1;

		/* Don't shift caret if display length exceeded or the caret */
		/* is positioned just before the last displayable character. */
		if ((strlen(sp) > dp->display_length) &&
			(caret_offset >= (dp->display_length - 1) * dp->font->pf_defaultsize.x))
		    caret_shift = 0;

	    }
	    else		/* no more room */
		blink_value(ip);

	}
	else			/* must be read-only */
	    blink_value(ip);
    }

    /* determine the new caret offset */
    was_clipped = (dp->first_char != 0) || (dp->last_char < (orig_len - 1));
    orig_offset = dp->value_offset;

    update_value_offset(ip, val_change, val_shift);
    update_caret_offset(ip, caret_shift);

    /* update the display if not masked */
    if (dp->mask != ' ') {
	/* compute the position of the caret */
	x = ip->value_rect.r_left + dp->caret_offset;
	new_len = strlen(sp);

	/* erase deleted characters that were displayed */
	if (new_len < orig_len) {

	    /* repaint the whole value if needed */
	    if (was_clipped || dp->first_char || (dp->last_char < (new_len - 1)))
		paint_value(ip, PV_HIGHLIGHT);

	    else {
		/* clear the deleted characters and everything to the right */
		(void) panel_pw_writebackground(ip->panel, x, ip->value_rect.r_top,
					     orig_offset - dp->caret_offset,
					  ip->value_rect.r_height, PIX_CLR);

		if (dp->mask == '\0')
		    (void) panel_pw_text(ip->panel, x,
			    ip->value_rect.r_top + panel_fonthome(dp->font),
					 PIX_SRC | PIX_COLOR(ip->color_index), dp->font, &sp[insert_pos]);

		else {		/* masked */
		    char           *buff;
		    buff = (char *) sv_malloc(strlen(sp) + 1);
		    for (i = 0; i < strlen(sp); i++)
			buff[i] = dp->mask;
		    buff[strlen(sp)] = '\0';
		    (void) panel_pw_text(ip->panel, x,
			    ip->value_rect.r_top + panel_fonthome(dp->font),
					 PIX_SRC | PIX_COLOR(ip->color_index), dp->font, &buff[insert_pos]);
		    free(buff);
		}
	    }

	}
	else if (new_len > orig_len) {
	    /* repaint the whole value if it doesn't fit */
	    if (new_len > dp->display_length)
		paint_value(ip, PV_HIGHLIGHT);
	    else
		/* write the new character to the left of the caret */
	    if (dp->mask == '\0')	/* not masked */
		(void) panel_pw_text(ip->panel, x - dp->font->pf_defaultsize.x,
			    ip->value_rect.r_top + panel_fonthome(dp->font),
				     PIX_SRC | PIX_COLOR(ip->color_index), dp->font, &sp[insert_pos]);

	    else {		/* masked */
		char           *buff;
		buff = (char *) sv_malloc(strlen(sp) + 1);
		for (i = 0; i < strlen(sp); i++)
		    buff[i] = dp->mask;
		buff[strlen(sp)] = '\0';
		(void) panel_pw_text(ip->panel, x - dp->font->pf_defaultsize.x,
			    ip->value_rect.r_top + panel_fonthome(dp->font),
				     PIX_SRC | PIX_COLOR(ip->color_index), dp->font, &buff[insert_pos]);
		free(buff);
	    }
	}
	else
	    /* Cursor key causes display shift */
	    if (val_shift)
	        paint_value(ip, PV_HIGHLIGHT);
    }

END_OF_UPDATE_VALUE:
    return;
}


static
update_value_offset(ip, val_change, val_shift)
    panel_item_handle ip;
    int             val_change;
    int             val_shift;
{
    register text_data *dp = textdp(ip);
    int             clip_len, full_len;
    struct pr_size  size;
    int             max_caret;
    int             caret_offset = dp->caret_offset;


    full_len = strlen(dp->value);

    /* clip at the left if needed */
    /* account for the left arrow if clipped */
    if (full_len <= dp->display_length) {
	size = pf_textwidth(full_len, dp->font, dp->value);
	dp->first_char = 0;
	dp->last_char = full_len - 1;
	dp->value_offset = size.x;
    }
    else {
	clip_len = dp->display_length - 1;
	size = pf_textwidth(clip_len, dp->font, dp->value);
	max_caret = (dp->display_length - 1) * dp->font->pf_defaultsize.x;
	if (dp->font->pf_defaultsize.x >= panel_left_arrow_pr.pr_width)
	    caret_offset += dp->font->pf_defaultsize.x -
		panel_left_arrow_pr.pr_width;
	caret_offset = caret_offset / dp->font->pf_defaultsize.x;
	caret_offset = caret_offset * dp->font->pf_defaultsize.x;

	/* Add a character */
	if (val_change > 0) {

	    /* Inserted characters will always be visible and the */
	    /* caret is always positioned after the inserted character, */
	    /* unless the caret is already positioned after the last */
	    /* displayable character, in which case all the characters */
	    /* to the left of the inserted character are shifted to the */
	    /* left on the display. */
	    if (caret_offset >= max_caret) {
		if (dp->first_char)
		    dp->first_char++;
		else
		    dp->first_char = 2;	/* clip at left */
	    }
	}

	/* Delete 1 or more characters */
	else if (val_change < 0) {
	    if (dp->first_char > 2)
		dp->first_char += val_change;
	    else
		dp->first_char = 0;	/* no more clip at left */
	}

	/* Shift the display */
	else {
	    dp->first_char += val_shift;

	    /* Check for clipping at left */
	    if ((dp->first_char == 1) && (val_shift > 0))
		dp->first_char++;
	    if ((dp->first_char == 1) && (val_shift < 0))
		dp->first_char--;
	}

	dp->last_char = dp->first_char + clip_len - 1;

	/* Check again for left clip */
	if (dp->first_char <= 1) {
	    dp->first_char = 0;
	    dp->last_char = clip_len;
	}

	/* Check for right clip */
	if (dp->last_char < (full_len - 1))
	    dp->last_char -= 1;

	/* Compute value offset, and include arrows if clipping either side */
	dp->value_offset = size.x;
	if (dp->first_char)
	    dp->value_offset += panel_left_arrow_pr.pr_width;
	if (dp->last_char < (full_len - 1))
	    dp->value_offset += panel_right_arrow_pr.pr_width;
    }
}


int
update_caret_offset(ip, caret_shift)
    panel_item_handle ip;
    int             caret_shift;
/*
 * update_caret_offset computes the caret x offset for ip.
 */
{
    register text_data *dp = textdp(ip);
    int             clip_len, full_len;
    struct pr_size  size;
    int             caret_offset = dp->caret_offset;

    /* no offset if masked completely */
    full_len = strlen(dp->value);

    /*
     * Get old caret offset, in multiples of the font width.
     */
    if (dp->font->pf_defaultsize.x >= panel_left_arrow_pr.pr_width)
	caret_offset += dp->font->pf_defaultsize.x -
	    panel_left_arrow_pr.pr_width;
    caret_offset = caret_offset / dp->font->pf_defaultsize.x;
    caret_offset = caret_offset * dp->font->pf_defaultsize.x;

    /*
     * Compute new caret offset.
     */
    dp->caret_offset = caret_offset + (caret_shift * dp->font->pf_defaultsize.x);

    /* Caret cannot cross left boundary of text field. */
    if (dp->caret_offset < 0)
	dp->caret_offset = 0;

    /* If clipping at left, caret cannot cross left arrow */
    if ((dp->caret_offset == 0) && (dp->first_char))
	dp->caret_offset += panel_left_arrow_pr.pr_width;
    else {
	if ((dp->caret_offset != 0) && (dp->first_char))
	    dp->caret_offset += panel_left_arrow_pr.pr_width -
		dp->font->pf_defaultsize.x;
    }

    /*
     * Now use the newly computed caret offset, and do the same thing for the
     * right boundary.
     */
    caret_offset = dp->caret_offset;
    if (dp->font->pf_defaultsize.x >= panel_left_arrow_pr.pr_width)
	caret_offset += dp->font->pf_defaultsize.x -
	    panel_left_arrow_pr.pr_width;
    caret_offset = caret_offset / dp->font->pf_defaultsize.x;
    caret_offset = caret_offset * dp->font->pf_defaultsize.x;

    /* Caret cannot cross right boundary of text field. */
    if (caret_offset > dp->display_length * dp->font->pf_defaultsize.x)
	dp->caret_offset -= dp->font->pf_defaultsize.x;

    /* If clipping at right, caret cannot cross right arrow */
    else if ((caret_offset == dp->display_length * dp->font->pf_defaultsize.x) &&
	     (dp->last_char < full_len - 1))
	dp->caret_offset -= dp->font->pf_defaultsize.x;

    /*
     * Caret cannot exceed value offset.  This should be a "catch-all" safety
     * net, like when stored length < displayed length.
     */
    if (dp->caret_offset > dp->value_offset)
	dp->caret_offset = dp->value_offset;

}


static
blink_value(ip)
    panel_item_handle ip;
/*
 * blink_value blinks the value rect of ip.
 */
{
    int             i;		/* counter */

    /* invert the value rect */
    (void) panel_invert(ip->panel, &ip->value_rect);

    /* wait a while */
    for (i = 1; i < 5000; i++);

    /* un-invert the value rect */
    (void) panel_invert(ip->panel, &ip->value_rect);
}


/***********************************************************************/
/* caret routines                              */
/***********************************************************************/


static
deselect(panel)
    panel_handle    panel;
{
    panel_item_handle old_item = (panel_item_handle) panel->caret;
    text_data      *old_dp;

    if (old_item != NULL) {
	old_dp = textdp(old_item);
	old_dp->flags &= ~HASCARET;
	paint_caret(old_item, PIX_CLR);
    }
}



static          caddr_t
panel_getcaret(panel)
    panel_handle    panel;
{
    return (caddr_t) panel->caret;
}

static          caddr_t
panel_setcaret(panel, ip)
    panel_handle    panel;
    panel_item_handle ip;
{
    text_data      *dp = textdp(ip);

    if (ip == NULL || hidden(ip))
	return NULL;

    deselect(panel);
    panel->caret = ip;
    if (panel_seln(panel, SELN_CARET)->ip)
	panel_seln(panel, SELN_CARET)->ip = ip;
    dp->flags |= HASCARET;
    paint_caret(ip, PIX_SET);
    return (caddr_t) ip;
}

Panel_item
panel_advance_caret(client_panel)
    Panel_item      client_panel;
{
    panel_handle    panel = PANEL_CAST(client_panel);
    panel_item_handle old_item;
    text_data      *old_dp;

    old_item = panel->caret;

    if (!old_item)
	return (NULL);

    old_dp = textdp(old_item);
    (void) panel_setcaret(panel, old_dp->next);

    return (Panel_item) panel->caret;
}

Panel_item
panel_backup_caret(client_panel)
    Panel_item      client_panel;
{
    panel_handle    panel = PANEL_CAST(client_panel);
    register panel_item_handle old_ip;
    register panel_item_handle new_ip, pre_ip;

    old_ip = panel->caret;

    if (!old_ip)
	return (NULL);

    /* find previous item by going forward in linked list of text items... */
    pre_ip = old_ip;
    new_ip = textdp(old_ip)->next;
    while (new_ip != old_ip) {
	pre_ip = new_ip;
	new_ip = textdp(new_ip)->next;
    }

    (void) panel_setcaret(panel, pre_ip);

    return (Panel_item) panel->caret;
}


static void
text_caret_invert(panel)
    register panel_handle panel;
/*
 * text_caret_invert inverts the type-in caret.
 */
{
    if (!panel->caret)
	return;

    paint_caret(panel->caret, panel->caret_on ? PIX_CLR : PIX_SET);
}				/* text_caret_invert */


static void
text_caret_on(panel, on)
    panel_handle    panel;
    int             on;
/*
 * text_caret_on paints the type-in caret if on is true; otherwise xor's it.
 */
{
    if (!panel->caret)
	return;

    paint_caret(panel->caret, on ? PIX_SET : PIX_CLR);
}				/* text_caret_on */


/***********************************************************************/
/* panel_text_notify                                                   */
/***********************************************************************/

/* ARGSUSED */
Panel_setting
panel_text_notify(client_item, event)
    Panel_item      client_item;
    register Event *event;
{
    register panel_item_handle ip = PANEL_ITEM_CAST(client_item);
    register text_data *dp = textdp(ip);
    register short  code;

    switch (event_action(event)) {
    case '\n':
    case '\r':
    case '\t':
	return (event_shift_is_down(event)) ? PANEL_PREVIOUS : PANEL_NEXT;

	/* always insert action editting characters */
    case ACTION_ERASE_CHAR_BACKWARD:
    case ACTION_ERASE_CHAR_FORWARD:
    case ACTION_ERASE_WORD_BACKWARD:
    case ACTION_ERASE_WORD_FORWARD:
    case ACTION_ERASE_LINE_BACKWARD:
    case ACTION_ERASE_LINE_END:
	return (PANEL_INSERT);

    default:
	/* always insert user-specified editting characters */
	if ((event_action(event) == dp->edit_bk_char) ||
		(event_action(event) == dp->edit_bk_word) ||
		(event_action(event) == dp->edit_bk_line))
	    return (PANEL_INSERT);

	if (event_is_iso(event))
	    return ((event_id(event) >= ISO_FIRST + 0xA0) ?
		    PANEL_INSERT : PANEL_NONE);

	code = event_id(event);	/* XXX should be event_action? */
	return (code >= 0 && code <= 0xFF && isprint(code)) ?
	    PANEL_INSERT : PANEL_NONE;
    }
}


/*
 * Hilite selection according to its rank.
 */
void
panel_seln_hilite(selection)
    register panel_selection_handle selection;
{
    register panel_item_handle ip = selection->ip;
    int             caret_offset = textdp(ip)->caret_offset;
    register text_data *dp = textdp(ip);
    Rect            rect;
    int             first_dsc;	/* first displayed, selected char */
    int             last_dsc;	/* last displayed, selected char */

    if (selection->is_null)
	return;

    if (selection->rank == SELN_PRIMARY && primary_ms_clicks == 0)
	return;
    if (selection->rank == SELN_SECONDARY && secondary_ms_clicks == 0)
	return;

    rect = ip->value_rect;

    /* bug 1028852 -- limit selection to currently displayed characters */

    /*
     * Highlight all characters that are currently selected and currently
     * being displayed
     */

    switch (selection->rank) {
        case SELN_PRIMARY:
            first_dsc = primary_seln_first;
            if (dp->first_char > first_dsc)
	        first_dsc = dp->first_char;
    
            last_dsc = primary_seln_last;
            if (dp->last_char < last_dsc)
	        last_dsc = dp->last_char;
	    break;

        case SELN_SECONDARY:
            first_dsc = secondary_seln_first;
            if (dp->first_char > first_dsc)
	        first_dsc = dp->first_char;
    
            last_dsc = secondary_seln_last;
            if (dp->last_char < last_dsc)
	        last_dsc = dp->last_char;
	    break;
    }

    rect.r_left += (first_dsc - dp->first_char) * dp->font->pf_defaultsize.x;
    if (dp->first_char > 0)
	rect.r_left += panel_left_arrow_pr.pr_width;

    rect.r_width = (last_dsc - first_dsc + 1) * dp->font->pf_defaultsize.x;
    if (rect.r_width < 0)
	rect.r_width = 0;

#ifdef DEBUG
    printf("first_dsc=%2d, last_dsc=%2d, left=%d, width=%d\n",
	   first_dsc, last_dsc, rect.r_left, rect.r_width);
#endif	/* DEBUG */

    switch (selection->rank) {
    case SELN_PRIMARY:
	primary_seln_panel = ip->panel;	/* save panel */
	primary_seln_rect = rect;	/* save rectangle coordinates */
	if (primary_pending_delete) {
	    panel_gray(ip->panel, &rect);
	    primary_seln_highlight = HL_GRAY;
	}
	else {
	    panel_invert(ip->panel, &rect);
	    primary_seln_highlight = HL_INVERT;
	}
	break;

    case SELN_SECONDARY:
	secondary_seln_panel = ip->panel;	/* save panel */
	secondary_seln_rect = rect;	/* save rectangle coordinates */
	if (secondary_pending_delete) {
	    panel_gray(ip->panel, &secondary_seln_rect);
	    panel_invert(ip->panel, &secondary_seln_rect);
	    secondary_seln_highlight = HL_GRAY;
	}
	else
	    secondary_seln_highlight = HL_INVERT;

	rect.r_top = rect_bottom(&rect) - 1;
	rect.r_height = 1;
	panel_invert(ip->panel, &rect);

	break;
    }
}


/* Dehighlight selection */
void
panel_seln_dehilite(ip, rank)
    panel_item_handle ip;
    Seln_rank       rank;
{
    text_data      *dp = textdp(ip);
    Rect            rect;
    int		    caret_was_on;

    if (panel_seln(ip->panel, rank)->is_null)
	return;

    caret_was_on = 0;
    if (ip->panel->caret_on && ip == ip->panel->caret) {
	caret_was_on = 1;
        paint_caret(ip, PIX_CLR);
    }

    paint_value(ip, PV_NO_HIGHLIGHT);

    switch (rank) {
	case SELN_PRIMARY:
	    primary_seln_highlight = HL_NONE;
	    primary_seln_panel = 0;		/* no longer valid */
	    if (ip == panel_seln(ip->panel, SELN_SECONDARY)->ip &&
		     !panel_seln(ip->panel, SELN_SECONDARY)->is_null)
	        panel_seln_hilite(panel_seln(ip->panel, SELN_SECONDARY));
	    break;

	case SELN_SECONDARY:
	    secondary_seln_highlight = HL_NONE;
	    secondary_seln_panel = 0;		/* no longer valid */
	    if (ip == panel_seln(ip->panel, SELN_PRIMARY)->ip &&
		    !panel_seln(ip->panel, SELN_PRIMARY)->is_null)
	        panel_seln_hilite(panel_seln(ip->panel, SELN_PRIMARY));
	    break;
    }

    if (caret_was_on)
	paint_caret(ip, PIX_SET);
}

/* This should really be in the defaults database... */
#define WORD_DELIMS	" \t,.:;?!\'\"\`*/-+=(){}[]<>\\\|\~@#$%^&"

static
panel_find_word(dp, first, last)
    register text_data *dp;
    int            *first, *last;
{
    register int    index;

    /* bug 1028682 -- patched code to find whole word upon double click */

    index = *first;
    if (strchr(WORD_DELIMS, dp->value[index]) != NULL) {	/* is a delim */
        /* Find beginning of delimiters */
        while (index > 0 && strchr(WORD_DELIMS, dp->value[index]) != NULL)
	    index--;
        if ((index != *first) && strchr(WORD_DELIMS, dp->value[index]) == NULL)
	    index++;		/* don't include non-delimiter */
        *first = index;

        /* Find end of delimiters */
        index = *last;
        while (index < strlen(dp->value) - 1 && strchr(WORD_DELIMS, dp->value[index]) != NULL)
	    index++;
        if ((index != *last) && strchr(WORD_DELIMS, dp->value[index]) == NULL)
	    index--;		/* don't include word delimiter */
        *last = index;
    }
    else {
        /* Find beginning of word */
        while (index > 0 && strchr(WORD_DELIMS, dp->value[index]) == NULL)
	    index--;
        if ((index != *first) && strchr(WORD_DELIMS, dp->value[index]) != NULL)
	    index++;		/* don't include word delimiter */
        *first = index;

        /* Find end of word */
        index = *last;
        while (index < strlen(dp->value) - 1 && strchr(WORD_DELIMS, dp->value[index]) == NULL)
	    index++;
        if ((index != *last) && strchr(WORD_DELIMS, dp->value[index]) != NULL)
	    index--;		/* don't include word delimiter */
        *last = index;
    }
}


panel_get_text_selection(ip, selection_string, selection_length, rank)
    panel_item_handle ip;
    char          **selection_string;
    int            *selection_length;
    register Seln_rank rank;
{
    register text_data *dp = textdp(ip);

    if ( (primary_ms_clicks == 0 && secondary_ms_clicks == 0) || 
         (strlen(dp->value) == 0) ) {
	*selection_length = 0;
	*selection_string = 0;
	return;
    }
    if (rank == SELN_PRIMARY) {
        *selection_length = primary_seln_last - primary_seln_first + 1;
        *selection_string = dp->value + primary_seln_first;
    }
    else if (rank == SELN_SECONDARY) {
        *selection_length = secondary_seln_last - secondary_seln_first + 1;
        *selection_string = dp->value + secondary_seln_first;
    }
}


panel_seln_delete(ip, rank)
    panel_item_handle ip;
    register Seln_rank rank;
{
    register text_data *dp = textdp(ip);
    register int    new;	/* new position of char to be moved */
    register int    old;	/* old position of char to be moved */
    register int    seln_first;
    register int    seln_last;
    int             last;	/* position of last valid char in value */
    int             caret_was_on = 0;
    int             caret_shift = 0;
    int             val_change = 0;
    int             val_shift = 0;
    int             right;	/* pixel coordinate of right side of selected
				 * word */

    if (rank != SELN_PRIMARY && rank != SELN_SECONDARY)
	return;

    /*
     * Can't delete anything if nothing selected.
     */
    if (rank == SELN_PRIMARY && primary_ms_clicks == 0)
	return;
    if (rank == SELN_SECONDARY && secondary_ms_clicks == 0)
	return;

    /*
     * READ_ONLY text cannot be deleted.  Only need to dehilite it.
     */
    if (dp->flags & READ_ONLY) {
        if (rank == SELN_PRIMARY) {
	    panel_seln_dehilite(ip, SELN_PRIMARY);
	    primary_ms_clicks = 0;
            primary_seln_panel = 0;
	}
        else if (rank == SELN_SECONDARY) {
	    panel_seln_dehilite(ip, SELN_SECONDARY);
	    secondary_ms_clicks = 0;
	    secondary_seln_panel = 0;
	}
	blink_value(ip);
	return;
    }

    /* Ok, so we're going to delete it so clear caret */
    caret_was_on = 0;
    if (ip->panel->caret_on && ip == ip->panel->caret) {
	caret_was_on = 1;
        paint_caret(ip, PIX_CLR);
    }

    if (rank == SELN_PRIMARY) {
	seln_first = primary_seln_first;
	seln_last = primary_seln_last;
	if (ip == panel_seln(ip->panel, SELN_SECONDARY)->ip)
	    update_secondary_seln(ip);
    }
    else if (rank == SELN_SECONDARY) {
	seln_first = secondary_seln_first;
	seln_last = secondary_seln_last;
	if (ip == panel_seln(ip->panel, SELN_PRIMARY)->ip)
	    update_primary_seln(ip);

        /* UPDATE SECONDARY_CARET_OFFSET */
        panel_set_caret_position(ip, SELN_SECONDARY, secondary_seln_first);
    }
    
    /* Delete the selected characters from the value buffer */
    new = seln_first;
    old = seln_last + 1;
    last = strlen(dp->value);
    for (; new <= dp->stored_length - 1; new++, old++) {
	if (old > last)
	    dp->value[new] = 0;
	else
	    dp->value[new] = dp->value[old];
    }

    /*
     * Calculate number of character positions to move displayed value
     * (val_change) and number of character positions to move caret
     * (caret_shift).
     */
    val_change = seln_first - seln_last - 1;

    right = (seln_first - dp->first_char + seln_last - seln_first + 1 +
	     (dp->first_char ? 1 : 0)) * dp->font->pf_defaultsize.x;

    if (dp->caret_offset >= right)
	caret_shift = val_change;
    if (dp->first_char)
	caret_shift += ((dp->first_char - 1) > -val_change) ?
	    (-val_change) : (dp->first_char - 1);

    /* Must come after caret_shift calculation */
    update_value_offset(ip, val_change, val_shift);
    update_caret_offset(ip, caret_shift);

    if (rank == SELN_PRIMARY) {
        primary_ms_clicks = 0;
        primary_seln_panel = 0;
        primary_pending_delete = FALSE;
    }
    else if (rank == SELN_SECONDARY) {
        secondary_ms_clicks = 0;
	secondary_seln_panel = 0;
        secondary_pending_delete = FALSE;
    }

    /* This must come after clearing primary_* or secondary_* */
    paint_value(ip, PV_HIGHLIGHT);

    if (caret_was_on)
        paint_caret(ip, PIX_SET);
}

static
update_primary_seln(ip)
    panel_item_handle ip;
{
    if (secondary_seln_first < primary_seln_first) {
	if (primary_seln_first <= secondary_seln_last) {
	    if (primary_seln_last <= secondary_seln_last)
		primary_seln_last = primary_seln_first - 1;
	    else {
		primary_seln_first = secondary_seln_first;
		primary_seln_last -= secondary_seln_last - secondary_seln_first + 1;
	    }
	}
	else {
	    primary_seln_first -= secondary_seln_last - secondary_seln_first + 1;
	    primary_seln_last -= secondary_seln_last - secondary_seln_first + 1;
	}
    }
    else {
	if (secondary_seln_first <= primary_seln_last) {
	    if (primary_seln_last <= secondary_seln_last)
		primary_seln_last -= primary_seln_last - secondary_seln_first + 1;
	    else
		primary_seln_last -= secondary_seln_last - secondary_seln_first + 1;
	}
    }
    if (primary_seln_last < primary_seln_first)
	panel_seln(ip->panel, SELN_PRIMARY)->is_null = 1;
}

static
update_secondary_seln(ip)
    panel_item_handle ip;
{
    register text_data *dp = textdp(ip);
    int	adjust_to_last = 0;

    if (panel_get_caret_position(ip, SELN_SECONDARY) != secondary_seln_first)
        adjust_to_last = 1;

    if (primary_seln_first < secondary_seln_first) {
	if (secondary_seln_first <= primary_seln_last) {
	    if (secondary_seln_last <= primary_seln_last)
		secondary_seln_last = secondary_seln_first - 1;
	    else {
		secondary_seln_first = primary_seln_first;
		secondary_seln_last -= primary_seln_last - primary_seln_first + 1;
	    }
	}
	else {
	    secondary_seln_first -= primary_seln_last - primary_seln_first + 1;
	    secondary_seln_last -= primary_seln_last - primary_seln_first + 1;
	}
    }
    else {
	if (primary_seln_first <= secondary_seln_last) {
	    if (secondary_seln_last <= primary_seln_last)
		secondary_seln_last -= secondary_seln_last - primary_seln_first + 1;
	    else
		secondary_seln_last -= primary_seln_last - primary_seln_first + 1;
	}
    }

    /* UPDATE SECONDARY_CARET_OFFSET */

    if (secondary_seln_last < secondary_seln_first) {
	panel_seln(ip->panel, SELN_SECONDARY)->is_null = 1;
        panel_set_caret_position(ip, SELN_SECONDARY, primary_seln_first);
    }
    else if (adjust_to_last)
        panel_set_caret_position(ip, SELN_SECONDARY, secondary_seln_last);
    else
        panel_set_caret_position(ip, SELN_SECONDARY, secondary_seln_first);
}


/*
 * these are global functions
 */
int
panel_is_pending_delete(rank)
    register Seln_rank rank;
{
    if (rank == SELN_PRIMARY)
	return(primary_pending_delete);
    else if (rank == SELN_SECONDARY)
	return(secondary_pending_delete);

    return(0);
}


int
panel_clear_pending_delete(rank)
    register Seln_rank rank;
{
    if (rank == SELN_PRIMARY)
	 primary_pending_delete = 0;    
    else if (rank == SELN_SECONDARY)
	 secondary_pending_delete = 0;    
}


int
panel_possibly_normalize(ip, caret_position)
    panel_item_handle ip;
    int caret_position;
{
    register text_data *dp = textdp(ip);

    if (caret_position < dp->first_char) {
	/* adjust first_char and last_char left */
	dp->first_char = caret_position;
	if (strlen(dp->value) <= dp->display_length)
	    dp->last_char = strlen(dp->value) - 1;
	else {
	    dp->last_char = dp->first_char + dp->display_length - 1;
	    if (strlen(dp->value) - dp->first_char + 1 > dp->display_length)
		dp->last_char--;
	    if (dp->first_char)
		dp->last_char--;
	}
	paint_value(ip, PV_HIGHLIGHT);
    }
    else if (caret_position > dp->last_char + 1) {
	/* adjust first_char and last_char right */
	dp->last_char = caret_position - 1;
	dp->first_char = dp->last_char - dp->display_length + 1;
	if (dp->first_char)
	    dp->first_char++;
	if (strlen(dp->value) - dp->first_char + 1 > dp->display_length)
	    dp->first_char++;

	paint_value(ip, PV_HIGHLIGHT);
    }
}


/*
 * panel_get_caret_position -- get character position of caret
 */

int
panel_get_caret_position(ip, rank)
    panel_item_handle ip;
    Seln_rank rank;
{
    register text_data *dp = textdp(ip);
    int caret_offset;
    int caret_position;

    if (rank == SELN_PRIMARY)
        caret_offset = dp->caret_offset;
    else if (rank == SELN_SECONDARY)
        caret_offset = dp->secondary_caret_offset;

    if (dp->first_char) {
        caret_offset -= panel_left_arrow_pr.pr_width;
	caret_position = caret_offset / dp->font->pf_defaultsize.x;
	caret_position += dp->first_char;
    }
    else
	caret_position = caret_offset / dp->font->pf_defaultsize.x;

    return(caret_position);
}

int
panel_set_caret_position(ip, rank, caret_position)
    panel_item_handle ip;
    Seln_rank rank;
    int caret_position;
{
    register text_data *dp = textdp(ip);
    int caret_offset;

    if (caret_position < 0)
	caret_position = 0;
    else if (caret_position > strlen(dp->value))
	caret_position = strlen(dp->value);

    panel_possibly_normalize(ip, caret_position);

    caret_offset = (caret_position - dp->first_char) * dp->font->pf_defaultsize.x;
    if (dp->first_char)
        caret_offset += panel_left_arrow_pr.pr_width;
	    
    if (rank == SELN_PRIMARY)
	dp->caret_offset = caret_offset;
    else if (rank == SELN_SECONDARY)
	dp->secondary_caret_offset = caret_offset;
}


#ifdef FUTURE -- if disabling caret while function keys down is desired

***  THIS IS NOT CURRENTLY USED  ***

static int dont_display_caret = 0;

int
panel_enable_caret(ip)
    panel_item_handle ip;
{
    register text_data *dp = textdp(ip);

    if (dont_display_caret) {
        dp->caret_offset = dp->saved_caret_offset;
	dont_display_caret = 0;
        if (ip == ip->panel->caret)
            paint_caret(ip, PIX_SET);
    }
}

int
panel_disable_caret(ip)
    panel_item_handle ip;
{
    register text_data *dp = textdp(ip);

    if (! dont_display_caret) {
        if (ip == ip->panel->caret)
            paint_caret(ip, PIX_CLR);
        dp->saved_caret_offset = dp->caret_offset;
        dont_display_caret = 1;
    }
}
#endif FUTURE

