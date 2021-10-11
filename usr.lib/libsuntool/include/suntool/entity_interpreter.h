/*      @(#)entity_interpreter.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef suntool_entity_interpreter_DEFINED
#define suntool_entity_interpreter_DEFINED

/*
 * This defines the programmer interface to the entity interpreter abstraction.
 *
 * Each instance of an entity interpreter is expected to be paired with an
 *   instance of an entity stream.
 */

#					ifndef pr_rop
#include <pixrect/pixrect.h>
#					endif
#					ifndef rect_right
#include <sunwindow/rect.h>
#					endif

#					ifndef sunwindow_attr_DEFINED
#include <sunwindow/attr.h>
#					endif
#					ifndef suntool_entity_stream_DEFINED
#include "entity_stream.h"
#					endif

struct ei_object {
	struct ei_ops	*ops;
	caddr_t		 data;
};
typedef struct ei_object *Ei_handle;

struct ei_process_result {
	struct pr_pos	pos;		/* origin of next char */
	struct rect	bounds;		/* boundary of ink */
	unsigned long	break_reason;
	Es_index	last_plus_one;	/* 1st char not painted */
	Es_index	considered;	/* last char considered for paint */
};
	/*
	 * Flag bits returned in ei_process_result.break_reason.
	 * More than one bit may be on, thus break_reason acts as a mask of
	 *   status bits rather than a single status.
	 */
#define EI_PR_END_OF_STREAM	0x00000001
    /* Forward scan exhausted the entity stream. */
#define EI_PR_BUF_EMPTIED	0x00000002
    /* Forward scan exhausted the buffer or exceeded specified limit. */
#define EI_PR_NEWLINE		0x00000004
    /* A character in the newline class was encountered. */
#define EI_PR_BUF_FULL		0x00000008
    /* In ei_expand(), the output buffer filled up */
#define EI_PR_HIT_LEFT		0x00000010
#define EI_PR_HIT_TOP		0x00000020
#define EI_PR_HIT_RIGHT		0x00000040
#define EI_PR_HIT_BOTTOM	0x00000080
    /*
     *  Above four reasons indicate attempt to paint ink in violation of
     *   the corresponding boundary of the bounds rectangle.
     */
#define EI_PR_NEXT_FREE		0x00000100
#define EI_PR_CLIENT_REASON(client_mask) 				\
	( (unsigned long)(0x80000000|client_mask) )
    /*
     * Client code must test for client defined reasons (using == or !=) before
     *   applying bit tests (e.g., reason & EI_PR_NEWLINE) in order to avoid
     *   false matches (e.g., above test would match EI_PR_CLIENT_REASON(4)).
     */

#define EI_OP_CHAR		0x00000001
#define EI_OP_WORD		0x00000002
#define EI_OP_DARK_GRAY		0x00000010
#define EI_OP_LIGHT_GRAY	0x00000020
#define EI_OP_STRIKE_THRU	0x00000040
#define EI_OP_DOTS_UNDER	0x00000080
#define EI_OP_STRIKE_UNDER	0x00000100
#define EI_OP_INVERT		0x00000200
#define EI_OP_CLEAR_FRONT	0x01000000	/* default has caller clear */
#define EI_OP_CLEAR_INTERIOR	0x02000000	/* default has caller clear */
#define EI_OP_CLEAR_BACK	0x04000000	/* default has caller clear */
#define EI_OP_CLEARED_RECT	0x08000000	/* caller pre-cleared */
#define EI_OP_MEASURE		0x80000000	/* default is paint */


struct ei_span_result {
	Es_index	first;
	Es_index	last_plus_one;
	unsigned	flags;
};
#define EI_SPAN_NOT_IN_CLASS		0x00000001
#define EI_SPAN_RIGHT_HIT_NEXT_LEVEL	0x00000002
#define EI_SPAN_LEFT_HIT_NEXT_LEVEL	0x00000004
#define EI_SPAN_HIT_NEXT_LEVEL						\
		(EI_SPAN_RIGHT_HIT_NEXT_LEVEL|EI_SPAN_LEFT_HIT_NEXT_LEVEL)


struct ei_ops {
  Ei_handle			(*destroy)();
  caddr_t			(*get)();
  int				(*line_height)();
  int				(*lines_in_rect)();
  struct ei_process_result	(*process)();
  int				(*set)();
  struct ei_span_result		(*span_of_group)();
  struct ei_process_result	(*expand)();
};

#define	EI_SPAN_INFO_MASK	0x0000000F
#define	EI_SPAN_CLASS_MASK	0x000000F0

#define EI_SPAN_RIGHT_ONLY	0x00000001
#define EI_SPAN_LEFT_ONLY	0x00000002
#define EI_SPAN_IN_CLASS_ONLY	0x00000004
#define EI_SPAN_NOT_CLASS_ONLY	0x00000008
#define EI_SPAN_CHAR		0x00000010
#define EI_SPAN_MORPHENE	0x00000020
#define EI_SPAN_WORD		0x00000030
#define EI_SPAN_PATH_NAME	0x00000040
#define EI_SPAN_LINE		0x00000050
#define EI_SPAN_SENTENCE	0x00000060
#define EI_SPAN_PARAGRAPH	0x00000070
#define EI_SPAN_SECTION		0x00000080
#define EI_SPAN_CHAPTER		0x00000090
#define EI_SPAN_DOCUMENT	0x000000A0
#define EI_SPAN_SP_AND_TAB	0x000000B0
#define EI_SPAN_CLIENT1		0x000000C0
#define EI_SPAN_CLIENT2		0x000000D0

/* Valid attributes for ei_get/set */
#define EI_ATTR(type, ordinal)	ATTR(ATTR_PKG_ENTITY, type, ordinal)
typedef enum {
	EI_CONTROL_CHARS_USE_FONT	= EI_ATTR(ATTR_BOOLEAN,		 10),
	EI_FONT				= EI_ATTR(ATTR_PIXFONT_PTR,	 20),
	EI_SPANW			= EI_ATTR(ATTR_OPAQUE,		 30),
	EI_SPAN1			= EI_ATTR(ATTR_OPAQUE,		 40),
	EI_SPAN2			= EI_ATTR(ATTR_OPAQUE,		 50),
	EI_TAB_WIDTH			= EI_ATTR(ATTR_INT,		 60)
} Ei_attribute;

#define ei_destroy(eih)							\
	(*(eih)->ops->destroy)(eih)
#define ei_get(eih, attr)						\
	(*(eih)->ops->get)(eih, attr)
#define ei_line_height(eih)						\
	(*(eih)->ops->line_height)(eih)
#define ei_lines_in_rect(eih, rect)					\
	(*(eih)->ops->lines_in_rect)(eih, rect)
#define ei_process(eih, op, esbuf, x, y, rop, pw, rect, tab_origin)	\
	(*(eih)->ops->process)(eih, op, esbuf, x, y, rop, pw, rect, tab_origin)
#define ei_expand(eih, esbuf, rect, x, obuf, obuf_len, tab_origin)	\
	(*(eih)->ops->expand)(eih, esbuf, rect, x, obuf, obuf_len, tab_origin)
/* VARARGS */
extern int ei_set();
#define ei_span_of_group(eih, esbuf, group_spec, index)			\
	(*(eih)->ops->span_of_group)(eih, esbuf, group_spec, index)


#endif

