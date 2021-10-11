/*	@(#)text_obj.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems Inc.
 */

/*	text_obj.h
 *
 *	A text object is any implementation of the generalized source
 *	stream for storing & manipulating text.
 */

typedef struct text_obj {
	struct text_ops  *ops;
	caddr_t		  data;
}  text_obj;

typedef struct text_ops {
	text_obj *(*create)();
	u_int	  (*commit)();
	text_obj *(*destroy)();
	u_int	  (*sizeof_entity)();
	u_int	  (*get_length)();
	u_int	  (*get_position)();
	u_int	  (*set_position)();
	u_int	  (*read)();
	u_int	  (*replace)();
	u_int	  (*edit)();
}  text_ops;

#define TXT_BKCHAR	0
#define TXT_BKWORD	1
#define TXT_BKLINE	2
#define TXT_FWDCHAR	3
#define TXT_FWDWORD	4
#define TXT_FWDLINE	5

#define TXT_EDITCHARS	6

#define txt_commit(txt_obj)

#define txt_destroy(txt_obj)					\
		(*(txt_obj)->ops->destroy)((txt_obj))

#define txt_sizeof_entity(txt_obj)				\
		(*(txt_obj)->ops->sizeof_entity)((txt_obj))

#define txt_get_length(txt_obj)					\
		(*(txt_obj)->ops->get_length)((txt_obj))

#define txt_get_position(txt_obj)				\
		(*(txt_obj)->ops->get_position)((txt_obj))

#define txt_set_position(txt_obj, pos)				\
		(*(txt_obj)->ops->set_position)((txt_obj), pos)

#define txt_read(txt_obj, length, result, buffer)		\
		(*(txt_obj)->ops->read)((txt_obj),		\
					length,			\
					result,			\
					buffer)

#define txt_replace(txt_obj, end, length, result, replacement)	\
		(*(txt_obj)->ops->replace)((txt_obj),		\
					   end,			\
					   length,		\
					   result,		\
					    replacement)

#define txt_edit(txt_obj, code)					\
		(*(txt_obj)->ops->edit)((txt_obj),code)
