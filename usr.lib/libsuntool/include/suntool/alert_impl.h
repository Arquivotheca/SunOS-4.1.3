/*	@(#)alert_impl.h 1.1 92/07/30	*/

/* ------------------------------------------------------------------ */
/* alert_impl.h: Copyright (c) 1986, 1987 by Sun Microsystems, Inc.   */
/* ------------------------------------------------------------------ */

#include "alert.h"

#define ALERT_HELP (ALERT_TRIGGERED-1)

/* ------------------------------------------------------------------ */
/* -------------- opaque types and useful typedefs  ----------------- */
/* ------------------------------------------------------------------ */

struct alert {
    Frame		client_frame;
    int			fd_for_fullscreen;
    int			result;
    int			default_input_code;
    Event		*event;

    int			position;
    int			beeps;
    int			dont_beep;

    int			client_offset_x;
    int			client_offset_y;
    
    int			is_optional;

#ifdef ecd.alert
    struct pixrect      *image_item;
    int                 no_image;
#endif ecd.alert

    char		**message_items;
    struct pixfont	*message_font;

    int			number_of_buttons;
    struct buttons	*button_info;
    struct pixfont	*button_font;
    int			yes_button_exists;
    int			no_button_exists;
    caddr_t		help_data;
};

typedef struct alert	*alert_handle;

struct buttons {
    char		*string;
    int			value;
    int			is_yes;
    int			is_no;
    struct rect		button_rect;
    struct buttons	*next;
};

typedef struct buttons	*button_handle;
