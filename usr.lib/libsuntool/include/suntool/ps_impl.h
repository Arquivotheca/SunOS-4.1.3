/*      @(#)ps_impl.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985, 1987 by Sun Microsystems, Inc.
 */

#ifndef _ps_impl_h_already_included
#define _ps_impl_h_already_included

/*
 * Internal structures for piece_stream implementation.
 *
 */

#				ifndef primal_DEFINED
#include "primal.h"
#				endif
#				ifndef entity_stream_h_already_defined
#include "entity_stream.h"
#				endif
#				ifndef suntool_finger_table_DEFINED
#include "finger_table.h"
#				endif

typedef struct piece_object {
	Es_index	pos;
	unsigned int	length;
	Es_index	source_and_pos;
} Piece_object;
typedef Piece_object *Piece;
#define PIECES_IN_TABLE(_private)					\
	((Piece) (_private->pieces.seq))

struct piece_table_object {
	int		magic;
	Es_handle	parent, original, scratch;
#ifdef DEBUG
	Es_handle	shadow;
	char		shadow_buf[4096];
#endif
	Es_status	status;
	caddr_t		client_data;
	ft_object	pieces;
	int		current;		/* pieces[] with position */
	Es_index	position, length,	/* in piece_stream */
			last_write_plus_one,	/* in piece_stream */
			oldest_not_undone_mark,	/* in scratch */
			rec_insert, rec_start;	/* in scratch */
	int		rec_insert_len;
	Es_index	scratch_max_len,
			scratch_position,	/* valid iff s_m_l != INF */
			scratch_length;		/* valid iff s_m_l != INF */
	Es_ops		scratch_ops;		/* valid iff s_m_l != INF */
};
typedef struct piece_table_object *Piece_table;
#define	ABS_TO_REP(esh)	(Piece_table)LINT_CAST(esh->data)
#define	PS_MAGIC	0x71625348
#define	CURRENT_NULL	0x7FFFFFFF
#define	SCRATCH_MIN_LEN	8096

#ifdef notdef
The original is read-only, the scratch is read-write-append.

The parent is usually NULL, unless this piece_table contains deleted pieces
that are being held in the trash bin.
#endif

/* The file format of the scratch source is defined by:
 *	file		::= records
 *	records		::= record records |
 *	record		::= pos_prev_rec flags replace
 *	pos_prev_rec	::= four_bytes
 *	flags		::= four_bytes
 *	replace		::= start stop_plus_one dp_count deleted_pieces insert
 *	dp_count	::= four_bytes
 *	insert		::= insert_length bytes_inserted
 *	insert_length	::= four_bytes
 *	start		::= four_bytes
 *	stop_plus_one	::= four_bytes
 *	four_bytes	::= BYTE BYTE BYTE BYTE
 *	bytes_inserted	::= BYTE bytes_inserted |
 *	deleted_pieces	::= deleted_piece deleted_pieces |
 *	deleted_piece	::= source_and_pos length
 *	source_and_pos	::= four_bytes
 *	length		::= four_bytes
 */
#define	PS_ALREADY_UNDONE	0x1
struct piece_record_header {
	unsigned long		pos_prev_rec, flags,
				start, stop_plus_one, dp_count;
};
struct deleted_piece {
	unsigned long		source_and_pos, length;
};


#define PS_LAST_PLUS_ONE(po_formal)					\
	((po_formal).pos+(po_formal).length)
#define PS_IS_AT_END(_private, _length, _index)				\
	(PIECES_IN_TABLE(_private)[_index].length == _length && (	\
	  _index+1 == _private->pieces.last_plus_one ||			\
	  PIECES_IN_TABLE(_private)[_index+1].pos == ES_INFINITY ))

#define	PS_SCRATCH	0x80000000
	/* The above bit flag is used to encode whether the source_and_pos
	 *   is in the original source or in the scratch source.
	 */
#define PS_MAKE_ORIGINAL_SANDP(pos_formal)				\
	((pos_formal) & (~PS_SCRATCH))
#define PS_SET_ORIGINAL_SANDP(piece_object_formal, pos_formal)		\
	(piece_object_formal).source_and_pos =				\
		PS_MAKE_ORIGINAL_SANDP(pos_formal)
#define PS_MAKE_SCRATCH_SANDP(pos_formal)				\
	((pos_formal) | PS_SCRATCH)
#define PS_SET_SCRATCH_SANDP(piece_object_formal, pos_formal)		\
	(piece_object_formal).source_and_pos =				\
		PS_MAKE_SCRATCH_SANDP(pos_formal)
#define PS_SET_SANDP(piece_object_formal, pos_formal, is_scratch)	\
	(piece_object_formal).source_and_pos = ((is_scratch) ?		\
		PS_MAKE_SCRATCH_SANDP(pos_formal) :			\
		PS_MAKE_ORIGINAL_SANDP(pos_formal))
#define PS_SANDP_SOURCE(po_formal)					\
	((po_formal).source_and_pos & PS_SCRATCH)
#define PS_SANDP_POS(po_formal)						\
	((po_formal).source_and_pos & (~PS_SCRATCH))


#define	SCRATCH_TO_REP(_scratch)					\
	ABS_TO_REP(((Es_handle)es_get(_scratch, ES_CLIENT_DATA)))
#define	SCRATCH_FIRST_VALID(_private)					\
	((_private)->scratch_length - (_private)->scratch_max_len)
#define	SCRATCH_HAS_WRAPPED(_private)					\
	((_private)->scratch_length > (_private)->scratch_max_len)


#endif

