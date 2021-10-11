/*	@(#)vuid_store.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * This file is used by the code of a virtual user input device
 * (vuid) state maintainence package (see ../sundev/vuid_event.h for a
 * description of what vuid is) and is private to that implementation.
 * It implements the interface defined by ../sunwindowdev/vuid_state.h.
 */

#ifndef _sundev_vuid_store_h
#define _sundev_vuid_store_h

/*
 * The typical struct vuid_seg state list contains 2 segments:
 *
 *	First, the VKEY_FIRST segment with four values in its list,
 *	    LOC_*_ABSOLUTE and LOC_*_DELTA.  The rest of the values are
 *	    booleans.  The vuid storage package know to update the
 *	    LOC_*_ABSOLUTE value when its gets a LOC_*_DELTA and visa
 *	    versa.
 *	The ascii/meta segment has all booleans.
 *
 * The implementation is skewed to optimize the space usage but
 * still be efficient in terms of accessing short vuid_value lists
 * (e.g., during high volume mouse tracking) and gang requests
 * about the state of shift buttons.
 */

/*
 * A component of virutal user input device (vuid) state storage.
 * A struct vuid_value holds a single non-boolean value.
 */
typedef	struct vuid_value {
	struct vuid_value	*next;	/* Next node in list */
	u_short			offset;	/* Offset of value from seg addr */
	int			value;	/* Value */
} Vuid_value;
#define	VUID_VALUE_NULL	((Vuid_value *)0)

#define	BITSPERBYTE	8
#define	VUID_BIT_ARRAY_SIZE (VUID_SEG_SIZE/((sizeof(char))*BITSPERBYTE))
/*
 * A component of virutal user input device (vuid) state storage.
 * A struct vuid_seg contains all the values for a vuid segment.
 * Neither the vuid_seg list or the vuid_value list will be sorted
 * unless a performance problem is identified.  As a new event is
 * sent to vuid_set_value, so the lists grow.
 */
typedef	struct vuid_seg {
	struct vuid_seg		*next;	/* Next state segment in list */
	u_short			addr;	/* Starting address of segment */
	char			booleans[VUID_BIT_ARRAY_SIZE];
					/* Up/down value of boolean codes */
	char			ints[VUID_BIT_ARRAY_SIZE];
					/* In/not in list */
	struct vuid_value 	*list;
					/* Linked list of values of non-boolean
					   codes.  If a code is in this list,
					   the boolean value is ignored and the
					   corresponding bit in values is on. */
} Vuid_seg;
#define	VUID_SEG_NULL	((Vuid_seg *)0)
#define	vuid_cstate_to_state(cstate)	((Vuid_seg *)cstate)

#define	vuid_set_boolean_bit(seg, offset) \
	(seg)->booleans[(offset)/BITSPERBYTE] |= \
	    (1<<((BITSPERBYTE-1)-((offset)%BITSPERBYTE)))
#define	vuid_clear_boolean_bit(seg, offset) \
	(seg)->booleans[(offset)/BITSPERBYTE] &= \
	    (~(1<<((BITSPERBYTE-1)-((offset)%BITSPERBYTE))))
#define	vuid_get_boolean_bit(seg, offset) \
	((seg)->booleans[(offset)/BITSPERBYTE] & \
	    (1<<((BITSPERBYTE-1)-((offset)%BITSPERBYTE))))

#define	vuid_set_int_bit(seg, offset) \
	(seg)->ints[(offset)/BITSPERBYTE] |= \
	    (1<<((BITSPERBYTE-1)-((offset)%BITSPERBYTE)))
#define	vuid_clear_int_bit(seg, offset) \
	(seg)->ints[(offset)/BITSPERBYTE] &= \
	    (~(1<<((BITSPERBYTE-1)-((offset)%BITSPERBYTE))))
#define	vuid_get_int_bit(seg, offset) \
	((seg)->ints[(offset)/BITSPERBYTE] & \
	    (1<<((BITSPERBYTE-1)-((offset)%BITSPERBYTE))))



#endif /*!_sundev_vuid_store_h*/
