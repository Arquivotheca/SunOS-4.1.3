/*      @(#)entity_view.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef suntool_entity_view_DEFINED
#define suntool_entity_view_DEFINED

/*
 * This is the programmatic interface to the entity view abstraction.
 *
 */

#				ifndef pr_rop
#include <pixrect/pixrect.h>
#				endif
#				ifndef rect_right
#include <sunwindow/rect.h>
#				endif
#				ifndef rl_rectoffset
#include <sunwindow/rectlist.h>
#				endif
#				ifndef pw_rop
#include <sunwindow/pixwin.h>
#				endif

#				ifndef suntool_entity_stream_DEFINED
#include "entity_stream.h"
#				endif
#				ifndef suntool_entity_interpreter_DEFINED
#include "entity_interpreter.h"
#				endif
#				ifndef suntool_finger_table_DEFINED
#include "finger_table.h"
#				endif

typedef ft_object	Ev_line_table;

struct first_line_info {
	Es_index	line_start;
	int		room_on_right;
};

typedef struct ev_object *Ev_handle;
struct ev_object {
	Ev_handle		 next;
	struct ev_chain_object	*view_chain;
	struct pixwin		*pw;
	struct rect		 rect;
	struct first_line_info	 first_line_info;
	Ev_line_table		 line_table;
	Ev_line_table		 tmp_line_table;
	caddr_t			 client_data;	/* Client scratch space */
	caddr_t			 private_data;
};
#define EV_NULL		(Ev_handle) 0

typedef ft_object		Ev_finger_table;
struct ev_chain_object {
	Es_handle		esh;
	Ei_handle		eih;
	Ev_handle		first_view;
	Ev_finger_table		fingers;
	caddr_t			client_data;
	caddr_t			private_data;
};
typedef struct ev_chain_object *Ev_chain;
#define EV_CHAIN_NULL	(Ev_chain_handle) 0
#define EV_NO_CONTEXT	-1

#define FORNEXTVIEWS(begin_formal, view_formal)				\
	for (view_formal = begin_formal->next; view_formal;		\
		view_formal = view_formal->next)
#define FORALLVIEWS(chain_formal, view_formal)				\
	for (view_formal = chain_formal->first_view; view_formal;	\
		view_formal = view_formal->next)

/* Due to compiler brain-damage, struct replaced by bit manipulation.
 * Top bit is flag for "move_at_insert", and next 15 are spare.
 * Lower 16 bits of Ev_mark_object are id.
 */
typedef long unsigned			Ev_mark_object;
typedef Ev_mark_object *		Ev_mark;
#define	EV_MARK_MOVE_AT_INSERT_MASK	0x80000000
#define	EV_MARK_ID_MASK			0xffff
#define	EV_MARK_MOVE_AT_INSERT(formal)	((formal) & EV_MARK_MOVE_AT_INSERT_MASK)
#define	EV_MARK_SET_MOVE_AT_INSERT(formal)				\
		(formal) |= EV_MARK_MOVE_AT_INSERT_MASK
#define EV_MARK_ID(formal)		((formal) & EV_MARK_ID_MASK)
#define	EV_MARK_IS_NULL(formal)		(EV_MARK_ID(*formal) == 0)

typedef char *			opaque_t;
struct ev_finger_datum {
	Es_index		pos;
	Ev_mark_object		info;
	opaque_t		client_data;
};
typedef struct ev_finger_datum *Ev_finger_handle;

/*
 * Notification actions - calls are of form:
 *	notify(client_data, attrs)
 *	    caddr_t	client_data;
 *	    Attr_avlist	attrs;
 *
 */
#define EV_ATTR(type, ordinal)	ATTR(ATTR_PKG_ENTITY, type, ordinal)
#define	EV_ATTR_RECT_PAIR	ATTR_TYPE(ATTR_BASE_RECT_PTR, 2)
#define	EV_ATTR_REPLACE_5	ATTR_TYPE(ATTR_BASE_INT, 5)
typedef enum {
  EV_ACTION_VIEW	= EV_ATTR(ATTR_OPAQUE,		1),
  EV_ACTION_EDIT	= EV_ATTR(EV_ATTR_REPLACE_5,	2),
	/* Args are (before edit happens):
	 *	insert, length, start, stop_plus_one, insert_count
	 */
  EV_ACTION_SCROLL	= EV_ATTR(EV_ATTR_RECT_PAIR,	3),
	/* Args are: from_rect, to_rect */
  EV_ACTION_PAINT	= EV_ATTR(ATTR_RECT_PTR,	4)
} Ev_notify_action;

	/* Possible editing actions */
#define EV_EDIT_BACK		0x00000001
#define EV_EDIT_CHAR		0x00000002
#define EV_EDIT_WORD		0x00000004
#define EV_EDIT_LINE		0x00000008
#define EV_EDIT_BACK_CHAR	(EV_EDIT_BACK|EV_EDIT_CHAR)
#define EV_EDIT_BACK_WORD	(EV_EDIT_BACK|EV_EDIT_WORD)
#define EV_EDIT_BACK_LINE	(EV_EDIT_BACK|EV_EDIT_LINE)

	/* Selection types and modes */
/*
 * EV_SEL_MASK is used to guarantee that EV_SEL_* flags do not occupy more
 *   than 16 bits (letting clients incorporate them into 32 bit long flags,
 *   which is best done by use of EV_SEL_CLIENT_FLAG).
 * Currently, the code does not directly support either EV_SEL_SHELF or
 *   EV_SEL_CARET, but adding them makes it easier for the clients to combine
 *   the EV_SEL_* types with use of the Selection Service.
 */
#define EV_SEL_MASK			0xFFFF
#define	EV_SEL_CLIENT_FLAG(formal)	((formal) << 16)
#define	EV_SEL_NONE			0x0000
#define EV_SEL_PRIMARY			0x0001
#define EV_SEL_SECONDARY		0x0002
#define	EV_SEL_SHELF			0x0004
#define	EV_SEL_CARET			0x0008
#define EV_SEL_PD_PRIMARY		0x0010
#define EV_SEL_PD_SECONDARY		0x0020
#define EV_SEL_PENDING_DELETE		(EV_SEL_PD_PRIMARY|EV_SEL_PD_SECONDARY)
#define EV_SEL_BASE_TYPE(formal)					\
	( (formal) &							\
	  (EV_SEL_PRIMARY|EV_SEL_SECONDARY|EV_SEL_SHELF|EV_SEL_CARET) )

	/* Return values from ev_xy_in_view */
#define EV_XY_VISIBLE	0
#define EV_XY_ABOVE	1
#define EV_XY_BELOW	2
#define EV_XY_RIGHT	3

	/* Flag values for ev_find_in_esh */
#define EV_FIND_DEFAULT		0
#define EV_FIND_BACKWARD	1
#define EV_FIND_RE		2


/* Status values for ev_set. */
typedef enum {
	EV_STATUS_OKAY,
	EV_STATUS_OTHER_ERROR,
	EV_STATUS_BAD_ATTR,
	EV_STATUS_BAD_ATTR_VALUE
} Ev_status;

/* Status values for ev_expand */
typedef enum {
	EV_EXPAND_OKAY,
	EV_EXPAND_FULL_BUF,
	EV_EXPAND_OTHER_ERROR
} Ev_expand_status;

#define	EV_ATTR_CARET		ATTR_TYPE(ATTR_OPAQUE, 3)
/* Attributes for ev_set and ev_get. */
typedef enum {
	EV_CHAIN_AUTO_SCROLL_BY	= EV_ATTR(ATTR_INT,		 2),
	EV_CHAIN_CARET		= EV_ATTR(EV_ATTR_CARET,	 6),
	EV_CHAIN_CARET_IS_GHOST	= EV_ATTR(ATTR_INT,		10),
	EV_CHAIN_DATA		= EV_ATTR(ATTR_OPAQUE,		14),
	EV_CHAIN_DELAY_UPDATE	= EV_ATTR(ATTR_INT,		16),
	EV_CHAIN_EDIT_NUMBER	= EV_ATTR(ATTR_INT,		18),
	EV_CHAIN_EIH		= EV_ATTR(ATTR_OPAQUE,		22),
	EV_CHAIN_ESH		= EV_ATTR(ATTR_OPAQUE,		26),
	EV_CHAIN_GHOST		= EV_ATTR(EV_ATTR_CARET,	30),
	EV_CHAIN_LOWER_CONTEXT	= EV_ATTR(ATTR_INT,		34),
	EV_CHAIN_NOTIFY_LEVEL	= EV_ATTR(ATTR_INT,		38),
	EV_CHAIN_NOTIFY_PROC	= EV_ATTR(ATTR_FUNCTION_PTR,	42),
	EV_CHAIN_UPPER_CONTEXT	= EV_ATTR(ATTR_INT,		46),
	EV_CLIP_RECT		= EV_ATTR(ATTR_RECT_PTR,	50),
	EV_DATA			= EV_ATTR(ATTR_OPAQUE,		54),
	EV_DISPLAY_START	= EV_ATTR(ATTR_INT,		58),
	EV_DISPLAY_LEVEL	= EV_ATTR(ATTR_INT,		62),
	EV_LEFT_MARGIN		= EV_ATTR(ATTR_INT,		66),
	EV_NO_REPAINT_TIL_EVENT	= EV_ATTR(ATTR_INT,		68),
	EV_RECT			= EV_ATTR(ATTR_RECT_PTR,	70),
	EV_RIGHT_BREAK		= EV_ATTR(ATTR_INT,		74),
	EV_RIGHT_MARGIN		= EV_ATTR(ATTR_INT,		78),
	EV_SAME_AS		= EV_ATTR(ATTR_OPAQUE,		82),
	EV_END_ALL_VIEWS	= EV_ATTR(ATTR_NO_VALUE,	98),
	EV_FOR_ALL_VIEWS	= EV_ATTR(ATTR_NO_VALUE,	99)
} Ev_attribute;

typedef enum {		/* Possible values for EV_DISPLAY_LEVEL attribute. */
	EV_DISPLAY_NONE,
	EV_DISPLAY_ONLY_CLEAR,
	EV_DISPLAY_NO_CLEAR,
	EV_DISPLAY
} Ev_display_level;

			/* Flag values for EV_CHAIN_NOTIFY_LEVEL attribute. */
#define	EV_NOTIFY_NONE		0x00
#define	EV_NOTIFY_EDIT_DELETE	0x01
#define	EV_NOTIFY_EDIT_INSERT	0x02
#define	EV_NOTIFY_EDIT_ALL	(EV_NOTIFY_EDIT_DELETE|EV_NOTIFY_EDIT_INSERT)
#define	EV_NOTIFY_SCROLL	0x04
#define	EV_NOTIFY_REPAINT	0x08
#define	EV_NOTIFY_PAINT		0x10
#define	EV_NOTIFY_ALL		(EV_NOTIFY_EDIT_ALL|EV_NOTIFY_SCROLL \
				|EV_NOTIFY_REPAINT|EV_NOTIFY_PAINT)

			/* Special values for EV_DISPLAY_START */
#define	EV_START_SPECIAL		0x80000000
#define	EV_START_BACKWARD		0x40000000
#define	EV_START_CURRENT_CONTENTS	(EV_START_SPECIAL|0x000)
#define	EV_START_CURRENT_LINE		(EV_START_SPECIAL|0x001)
#define	EV_START_CURRENT_POS		(EV_START_SPECIAL|0x002)
#define	EV_START_NEXT_LINE		(EV_START_SPECIAL|0x010)
#define	EV_START_NEXT_PAGE		(EV_START_SPECIAL|0x020)
#define	EV_START_BACKWARD_LINE		(EV_START_BACKWARD|EV_START_NEXT_LINE)
#define	EV_START_BACKWARD_PAGE		(EV_START_BACKWARD|EV_START_NEXT_PAGE)

typedef enum {		/* Possible values for EV_RIGHT_BREAK attribute. */
	EV_CLIP,
	EV_WRAP_AT_CHAR,
	EV_WRAP_AT_WORD
} Ev_right_break;

/* Flag values for ev_add_glyph_on_line(). */
#define	EV_GLYPH_DISPLAY	0x0000001
#define	EV_GLYPH_LINE_START	0x0000002
#define	EV_GLYPH_WORD_START	0x0000004
#define	EV_GLYPH_LINE_END	0x0000008

extern Ev_expand_status
ev_expand(  /* view, start, stop_plus_one, out_buf, out_buf_len,
               total_chars */  );
#							ifdef notdef
	register Ev_handle	 view;
	Es_index		 start;	/* Entity to start expanding at */
	Es_index		 stop_plus_one; /* 1st ent not expanded */
	char			*out_buf;
	int			 out_buf_len;
	int			*total_chars;
#							endif

extern Ev_finger_handle
ev_find_finger(  /* mark */  );
#							ifdef notdef
	Ev_mark_object	mark;
#							endif

/* VARARGS */
extern caddr_t
ev_get( /* view, attribute, args1, args2, args3 */ );
#							ifdef notdef
	Ev_handle		 view;
	Ev_attribute		 attribute;
	caddr_t			 args1, args2, args3;
#							endif

extern Es_index
ev_get_insert( /* views */ );
#							ifdef notdef
	Ev_chain	 views;
#							endif

extern Ev_mark
ev_init_mark(  /* mark */  );
#							ifdef notdef
	Ev_mark		mark;
#							endif

/* VARARGS1 */
extern Ev_status
ev_set( /* view, va_alist */ );
#							ifdef notdef
	Ev_handle	   view;
	va_dcl
#							endif

extern Es_index
ev_set_insert( /* views, position */ );
#							ifdef notdef
	Ev_chain	 views;
	Es_index	 position;
#							endif

extern Es_index
ev_scroll_lines( /* view, line_count, scroll_by_display_lines*/ );
#							ifdef notdef
	Ev_handle	view;
	int		line_count;
	int		scroll_by_display_lines;
#							endif


#endif

