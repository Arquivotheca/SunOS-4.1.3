/*      @(#)entity_stream.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1986, 1987 by Sun Microsystems, Inc.
 */

#ifndef _entity_stream_h_already_defined
#define _entity_stream_h_already_defined

#					ifndef sunwindow_attr_DEFINED
#include <sunwindow/attr.h>
#					endif

/*
 * This file defines the programmer interface to the entity stream abstraction.
 */

typedef struct es_object {
	struct es_ops	*ops;
	caddr_t		 data;
} Es_object;
typedef struct es_object *Es_handle;
#define ES_NULL		((Es_handle)0)

typedef long int	Es_index;
#define ES_CANNOT_SET	((Es_index)0x80000000)
#define ES_INFINITY	((Es_index)0x77777777)

typedef long int	Es_status;
#define	ES_SUCCESS		((Es_status)0x0)
#define	ES_CHECK_ERRNO		((Es_status)0x1)
#define	ES_CHECK_FERROR		((Es_status)0x2)
#define	ES_FLUSH_FAILED		((Es_status)0x3)
#define	ES_FSYNC_FAILED		((Es_status)0x4)
#define	ES_INVALID_ARGUMENTS	((Es_status)0x5)
#define	ES_INVALID_ATTRIBUTE	((Es_status)0x6)
#define	ES_INVALID_ATTR_VALUE	((Es_status)0x7)
#define	ES_INVALID_HANDLE	((Es_status)0x8)
#define	ES_INVALID_TYPE		((Es_status)0x9)
#define	ES_REPLACE_DIVERTED	((Es_status)0xa)
#define	ES_SEEK_FAILED		((Es_status)0xb)
#define	ES_SHORT_WRITE		((Es_status)0xc)
#define	ES_INCONSISTENT_LENGTH	((Es_status)0xd)
#define	ES_INCONSISTENT_POS	((Es_status)0xe)
#define	ES_BASE_STATUS(formal)	(formal & 0xffff)
#define	ES_CLIENT_STATUS(client_mask)	 				\
				((Es_status)(0x80000000|client_mask))

struct es_ops {
  Es_status	(*commit)();
  Es_handle	(*destroy)();
  caddr_t	(*get)();
  Es_index	(*get_length)();
  Es_index	(*get_position)();
  Es_index	(*set_position)();
  Es_index	(*read)();
  Es_index	(*replace)();
  int		(*set)();
};
typedef struct es_ops	*Es_ops;

#define es_commit(esh)							\
	(*(esh)->ops->commit)(esh)
#define es_destroy(esh)							\
	(*(esh)->ops->destroy)(esh)
#define es_get(esh, attr)						\
	(*(esh)->ops->get)(esh, attr)
#define es_get5(esh, attr, d1, d2, d3, d4, d5)				\
	(*(esh)->ops->get)(esh, attr, d1, d2, d3, d4, d5)
#define es_get_length(esh)						\
	(*(esh)->ops->get_length)(esh)
#define es_get_position(esh)						\
	(*(esh)->ops->get_position)(esh)
#define es_set_position(esh, pos)					\
	(*(esh)->ops->set_position)((esh), (pos))
#define es_read(esh, count, buf, count_read)				\
	(*(esh)->ops->read)((esh), (count), (buf), (count_read))
#define es_replace(esh, last_plus_one, count, buf, count_used)		\
	(*(esh)->ops->replace)(						\
	  (esh), (last_plus_one), (count), (buf), (count_used))
/* VARARGS */
extern int es_set();

#define ES_ATTR(type, ordinal)	ATTR(ATTR_PKG_ENTITY, type, ordinal+200)
#define	ES_ATTR_OPAQUE_2	ATTR_TYPE(ATTR_BASE_OPAQUE, 2)
#define	ES_ATTR_OPAQUE_4	ATTR_TYPE(ATTR_BASE_OPAQUE, 4)
typedef enum {
	ES_CLIENT_DATA		= ES_ATTR(ATTR_OPAQUE,		 1),
	ES_FILE_MODE		= ES_ATTR(ATTR_INT,		 2),
	ES_PS_ORIGINAL		= ES_ATTR(ATTR_OPAQUE,		 3),
	ES_PS_SCRATCH_MAX_LEN	= ES_ATTR(ATTR_INT,		30),
	ES_STATUS		= ES_ATTR(ATTR_INT,		 4),
	ES_UNDO_MARK		= ES_ATTR(ATTR_OPAQUE,		 5),
	ES_UNDO_NOTIFY_PAIR	= ES_ATTR(ES_ATTR_OPAQUE_2,	 6),
	/*es_set only */
	ES_HANDLE_TO_INSERT	= ES_ATTR(ATTR_OPAQUE,		10),
	ES_STATUS_PTR		= ES_ATTR(ATTR_OPAQUE,		11),
	/* es_get only */
	ES_HANDLE_FOR_SPAN	= ES_ATTR(ES_ATTR_OPAQUE_4,	20),
	ES_HAS_EDITS		= ES_ATTR(ATTR_BOOLEAN,		21),
	ES_NAME			= ES_ATTR(ATTR_STRING,		22),
	ES_PS_SCRATCH		= ES_ATTR(ATTR_OPAQUE,		23),
	ES_SIZE_OF_ENTITY	= ES_ATTR(ATTR_INT,		24),
	ES_TYPE			= ES_ATTR(ATTR_ENUM,		25),
} Es_attribute;
/*	  ES_STATUS accesses the entity_stream equivalent of errno, but this
 *	status is per instance, not global.  Caller must explicitly clear.
 *	  ES_STATUS_PTR allows calls to es_set to return a status by
 *	side-effect.
 *	  ES_NAME returns a pointer to a statically allocated char[] and this
 *	return value should be treated as read-only and volatile.
 *	  ES_TYPE returns a value of type Es_enum.
 */

typedef enum {
	ES_TYPE_MEMORY,
	ES_TYPE_FILE,
	ES_TYPE_PIECE,
	ES_TYPE_OTHER
} Es_enum;

#define	ES_NULL_UNDO_MARK	((caddr_t)0)

#define READ_AT_EOF(before, after, read)				\
	(((read) == 0) && ((before) == (after)))

/*
 * Sun Microsystems supported entity streams:
 *
 *   Es_handle
 *   es_mem_create(max, init)
 *   	u_int	max;
 *   	char   *init;
 * max is the maximum number of characters the stream can ever contain.
 * init is an initial value for the characters of the stream.
 *
 *   Es_handle
 *   es_file_create(name, options)
 *   	char	*name;
 *   	int	 options;
 * name is the path name of the file underlying the stream.
 * options is a bit-mask using the options defined below.
 */
#define ES_OPT_APPEND		0x00000001
#define ES_OPT_OVERWRITE	0x00000002
/*
 *   Es_handle
 *   ps_create(client_data, original, scratch)
 *	caddr_t		client_data;
 *   	Es_handle	original, scratch;
 * original stream can (and should) be read-only.
 * scratch stream has full access, and contains all edits.
 */


/* Some useful data structures and utilities for use with entity streams. */

	/* The following struct is used to pass a buffer filled by an entity
	 *   stream (and enough data to enable refilling it).  This is simply
	 *   an efficiency detail to avoid unnecessary re-reading of entities.
	 */
typedef struct es_buf_object {
	Es_handle	esh;
	caddr_t		buf;
	int		sizeof_buf;	/* In entities, not bytes */
	Es_index	first;		/* Corresponds to buf[0] */
	Es_index	last_plus_one;
} Es_buf_object;
typedef Es_buf_object *Es_buf_handle;


#endif

