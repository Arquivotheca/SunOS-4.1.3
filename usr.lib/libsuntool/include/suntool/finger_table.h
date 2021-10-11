/*      @(#)finger_table.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef finger_table_DEFINED
#define finger_table_DEFINED

/*
 * This is the programmatic interface to the finger table abstraction.
 *
 *     A finger_table is an array of records: .seq[ 0 .. .last_plus_one-1 ].
 *     Each record has an Es_index as its first field, and is
 * .size_of_element bytes long.
 *     The array is ordered by the values of the Es_index field, with the
 * smallest value occurring in the first record of the array.  The
 * implementation supports and requires the ordering.
 *     .last_bounding_index records the last result from ft_bounding_index;
 * it is a cache private to ft_bounding_index used to speed up the common case
 * of a succession of searches for similar positions.
 *     .first_infinity records the first occurrence of an ES_INFINITY;
 * it is a cache private to ft_shift_{up,out} used to speed up a common case
 * of shifts in large tables with many ES_INFINITY's at the end.
 */

#					ifndef suntool_entity_stream_DEFINED
#include "entity_stream.h"
#					endif

typedef struct ft_table {
	int	 	last_plus_one;
	unsigned	sizeof_element;
	int	 	last_bounding_index;
	int	 	first_infinity;
	Es_index	*seq;
} Ft_table_object;
typedef Ft_table_object	*Ft_table;
/* The following two typedef's are for 3.0 compatibility. */
typedef struct ft_table		ft_object;
typedef struct ft_table *	ft_handle;
#define FT_NULL			((ft_handle)0)

#define FT_CLIENT_CREATE(num_elements, client_type)			\
	ft_create((num_elements), sizeof(client_type)-sizeof(Es_index))
#define FT_SET_ALL(finger_table, to, client_data)			\
	ft_set((finger_table), 0, (finger_table).last_plus_one,		\
		(to), (client_data))
#define FT_CLEAR_ALL(finger_table)					\
	FT_SET_ALL((finger_table), ES_INFINITY, (char *)0)

#ifdef lint
#define	FT_ADDR(_handle, _index, _addr_delta)				\
	(Es_index *)( (_handle) && (_index) && (_addr_delta) ? 0 : 0 )
#define	FT_NEXT_ADDR(_addr, _addr_delta)				\
	(Es_index *)( (_addr) && (_addr_delta) ? 0 : 0 )
#define	FT_PREV_ADDR(_addr, _addr_delta)				\
	(Es_index *)( (_addr) && (_addr_delta) ? 0 : 0 )
#else
#define	FT_ADDR(_handle, _index, _addr_delta)				\
	(Es_index *)(							\
	    ((char *)((_handle)->seq)) + ((_index)*(_addr_delta)) )
#define	FT_NEXT_ADDR(_addr, _addr_delta)				\
	(Es_index *)( (char *)(_addr) + (_addr_delta) )
#define	FT_PREV_ADDR(_addr, _addr_delta)				\
	(Es_index *)( (char *)(_addr) - (_addr_delta) )
#endif

#define	FT_ADDRESS(_handle, _index)					\
	FT_ADDR((_handle), (_index), (_handle)->sizeof_element)
#define	FT_NEXT_ADDRESS(_handle, _addr)					\
	FT_NEXT_ADDR((_addr), (_handle)->sizeof_element )
#define	FT_PREV_ADDRESS(_handle, _addr)					\
	FT_PREV_ADDR((_addr), (_handle)->sizeof_element )

#endif



extern ft_object
ft_create( /* last_plus_one, sizeof_client_data */ );
#ifdef notdef
	int		last_plus_one, sizeof_client_data;
#endif
/* Allocates a seq of last_plus_one elements, each occupying
 *   sizeof(Es_index)+sizeof_client_data bytes, then returns an ft_object
 *   correctly initialized.
 * NOTE: The individual sequence elements are not initialized.
 */

extern int
ft_destroy( /* table */ );
#ifdef notdef
	ft_handle	table;
#endif
/* Deallocates the sequence and adjusts *table to remove all references
 *   to the deallocated sequence.
 */

extern void
ft_expand( /* table, by */ );
#ifdef notdef
	ft_handle	table;
	int		by;
#endif
/* Replaces the old sequence with a new sequence that is longer (by > 0) or
 *   shorter (if by < 0), while preserving all appropriate sequence contents.
 * When the sequence increases in size, new elements are at the end.
 * NOTE: If by > 0, the new sequence elements are not initialized.
 */

extern int
ft_shift_up( /* table, first, last_plus_one, expand_by */ );
#ifdef notdef
	ft_handle	table;
	int		first, last_plus_one, expand_by;
#endif
/* Shifts the elements in the sequence up so that seq[i],
 *   first <= i < last_plus_one, is unused.
 * The shift treats an element with position ES_INFINITY as the beginning of
 *   a range of empty elements, and stops the shift there.
 * If no empty elements can be found, the table is first expanded by expand_by
 *   and the extra elements have their position set to ES_INFINITY.
 */

extern int
ft_shift_out( /* table, first, last_plus_one */ );
#ifdef notdef
	ft_handle	table;
	int		first, last_plus_one;
#endif
/* Shifts the elements in the sequence down so that seq[first+i] gets the
 *   value from seq[last_plus_one+i].
 * The shift treats an element with position ES_INFINITY as an empty element,
 *   and stops the shift there.
 * Elements that are invalidated at the end of the sequence have their
 *   position set to ES_INFINITY.
 */

extern int
ft_set( /* finger_table, first, last_plus_one, to, client_data */ );
#ifdef notdef
	ft_object	 finger_table;
	int		 first, last_plus_one;
	Es_index	 to;
	char		*client_data;
#endif
/* All sequence elements in [first..last_plus_one) have their positions
 *   set to 'to', and if client_data is not 0, their client data area set
 *   to *client_data.
 */

extern int
ft_set_esi_span( /* finger_table, first, last_plus_one, to, client_data */ );
#ifdef notdef
	ft_object	 finger_table;
	Es_index	 first, last_plus_one, to;
	char		*client_data;
#endif
/* Like ft_set, but sets all elements whose positions are in the range
 *   [first..last_plus_one).
 */

extern int
ft_bounding_index( /* finger_table, pos */ );
#ifdef notdef
	Ft_table	finger_table;
	Es_index	pos;
#endif
/* Returns an index for finger_table that is either such that:
 *   finger_table->seq[index] <= pos < finger_table->seq[index+1], with
 *      index+1 < finger_table->last_plus_one, or;
 *   finger_table->seq[index] <= pos, with
 *      index+1 == finger_table->last_plus_one, or;
 *   finger_table->last_plus_one.
 * Note: if there are multiple occurrences of pos in the seq array, the
 *   returned index will be the index of the last occurrence.
 */

extern int
ft_index_for_position( /* finger_table, pos */ );
#ifdef notdef
	ft_object	finger_table;
	Es_index	pos;
#endif
/* Returns an index for finger_table that is either:
 *   the smallest index such that .seq[index] = pos, with
 *      index < finger_table.last_plus_one, or;
 *   finger_table.last_plus_one
 */

extern Es_index
ft_position_for_index( /* finger_table, index */ );
#ifdef notdef
	ft_object	finger_table;
	int		index;
#endif
/* Returns the position value of the sequence element specified by index,
 *   unless index is too large, in which case ES_CANNOT_SET is returned.
 * This is most useful when you do not want to typecast the sequence.
 */

extern int
ft_add_delta( /* finger_table, from, delta */ );
#ifdef notdef
	ft_object	finger_table;
	int		from;
	long int	delta;
#endif
/* Beginning at finger_table.seq[from], adds delta to each element until
 *   either the end of the table is reached or a position of ES_INFINITY.
 */
