/*	@(#)win_keymap.h 1.1 92/07/30 SMI */

/******************************************************************************
 *	keymap.h -- keymap declarations
 ******************************************************************************/

#ifndef keymap_h
#define keymap_h

#ifdef ADA_DEBUG
#include "win_input.h"
#else
#include <sunwindow/win_input.h>
#endif

#ifndef KB_SUN3
#include <sundev/kbd.h>
#endif

#include <sunwindow/bitmask.h>

/* This macro may be used for data compression */
#define event_into_word(e)	((unsigned)(e).ie_code << 16 \
	|| (unsigned)(e).ie_flags << 8 || (e).ie_shiftmask)

typedef struct sie_ {
	short		sie_code;		/* event code */
	short		sie_bits;		/* combined flags and shiftmask */
} ShortEvent;

#define shortevent_id(e)		((e)->sie_code)
#define shortevent_bits(e)		((e)->sie_bits)

#define eventbits_into_short(e)	(((e)->ie_flags << 8)|(e)->ie_shiftmask)
#define flags_from_short(s)		((unsigned)((s)->sie_bits) >> 8)
#define shiftmask_from_short(s)	((s)->sie_bits & 0xff)

#define SIE_NEGEVENT 0x0100

typedef struct el_ {
	ShortEvent	el_event;
	struct el_	*el_next;
} EventList;

typedef struct ke_ {
	ShortEvent	ke_fromevent;
	union {
	    EventList	*ke_toevents;
            ShortEvent  ke_oneevent;
        } ke_u;
	struct ke_  *ke_next;
} KeymapEntry;


#define KEYMAPTABLESIZE	3
/*
 * Possible index values into the keymap table. There are only three
 * that are client-modifiable
 */
#define	WIN_KEYMAP_ERASE_CHAR		0
#define WIN_KEYMAP_ERASE_WORD		1
#define WIN_KEYMAP_ERASE_LINE		2

struct keymap_table {
    unsigned short	code;
    unsigned short	mask;
};

typedef struct km_ {
	struct keymap_table	km_table[KEYMAPTABLESIZE];
} Keymap;

typedef struct kf_ {
	Keymap		*keymap;	/* keymap for the few client-modifiable
					 * semantic mapping. e.g. erase char, 
					 * erase word, etc..
					 */
	Bitmask		*smask;
} KeymapFileEntry;

#define KeymapMap(e)			((e).kf_keymap)
#define KeymapEventPending(e)	((e).kf_eventpending)
#define KeymapLastEvent(e)		((e).kf_lastevent)

#define ERR_NOEOM	1	/* missing end-of-modifier, '>' */
#define ERR_NOEOA	2	/* missing end-of-argument, ')' */
#define ERR_NOEON	3	/* missing end-of-keyname, ']' */
#define ERR_BADMOD	4	/* invalid modifier name */
#define ERR_BADNAME	5	/* invalid keyname */
#define ERR_BADARG	6	/* bad argument */
#define ERR_NOOPEN	7	/* couldn't open file */

/* (int) keymap_debug_mask mask bits */

#define KEYMAP_SHOW_TRANSLATION		0x01
#define KEYMAP_SHOW_EVENT_STREAM	0x02
#define KEYMAP_SHOW_MAP_TRANSITIONS	0x04
#define KEYMAP_SHOW_KEYMAP_STATE	0x08
#define KEYMAP_SHOW_SPARE			0x10

/* semantic event classes */

typedef enum {
    KEYMAP_FUNCT_KEYS,
    KEYMAP_EDIT_KEYS,
    KEYMAP_MOTION_KEYS,
    KEYMAP_TEXT_KEYS
} Event_class;

/******************************************************************************
 *	win_keymap.c external declarations
 ******************************************************************************/

#ifndef win_keymap_c

extern void		win_keymap_enable();
extern void		win_keymap_disable();

#endif win_keymap_c

#ifndef win_keymap2_c

extern KeymapFileEntry keymap_from_fd[];	/* Keymap tables by fd */

extern int keymap_initialized;	/* Keymap initialized flag */

extern int keymap_enable;	/* Keymapping enabled flag */

#endif win_keymap2_c

typedef struct kkm_ {
	struct keyboard *kkm_keyboard;
	int	kkm_mouseorder;
} KernelKeymapEntry;

typedef struct df_ {
	char	*df_name;
	char	*df_value;
} Defaults_List;

typedef enum {
KEYMAP_DIRECTORY = 0,
KEYMAP_INHERIT = 1,
KEYMAP_SUNVIEW_KEYS = 2,
KEYMAP_ARROW_KEYS = 3,
KEYMAP_LEFTY = 4,
KEYMAP_EDIT_CHAR = 5,
KEYMAP_EDIT_WORD = 6,
KEYMAP_EDIT_LINE = 7,
KEYMAP_COMPAT = 8,
KEYMAP_INHIBIT = 9,
} Keymap_List_Selector;


#ifdef KEYMAP_DEBUG
/* Debug mask */
extern unsigned	keymap_debug_mask;
#endif KEYMAP_DEBUG

#endif keymap_h
