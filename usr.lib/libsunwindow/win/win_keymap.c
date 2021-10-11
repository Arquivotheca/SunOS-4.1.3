#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)win_keymap.c 20.19 90/02/05 Copyr 1985 Sun Micro";
#endif
#endif

/*
 *	Copyright (c) 1987 Sun Microsystems, Inc.
 */
 
/******************************************************************************
 *	win_keymap.c -- keymapping system for SunView event mapping
 ******************************************************************************/

#define win_keymap_c

#include <stdio.h>
#include <ctype.h>

#include <sys/param.h>	/* max files/process (NOFILE) from here */

/* #include <sundev/kbd.h>	/* struct keyboard definition from here */

#include <sys/types.h>
#include <sys/time.h>

#include <sunwindow/defaults.h>
#include <sunwindow/bitmask.h>
#include <sunwindow/hashfn.h>

#include <sunwindow/win_keymap.h>


typedef struct {
    unsigned short	raw_code;
    unsigned short	keyflags;
}   _sunview_keys;

KeymapFileEntry	keymap_from_fd[NOFILE];
int		keymap_initialized;
int		keymap_enable;

#define LOCALHEAPSIZE (256-8)
static char *local_heap;
static char *local_heap_hwm;

/*
 * mask_mathches returns 1 if m1 and m2 are the same or differ only in the 
 * SHIFTMASK bit; returns 0 otherwise.
 * m1 is the shiftmask for an incoming event, m2 is the mask for one of the
 * key-mappable semantic events(currently just erase-{word,char,line}.
 * Masks that differ in SHIFTMASK only are translated into complimentry
 * semantic events.  E.g. CTRL-TAB = next field and SHIFT-CTRL-TAB = previous
 * field.
 */
#define mask_matches(m1, m2)   ((m1 == m2|SHIFTMASK) ? 1 : 0)
static _sunview_keys	*win_keymap_translate_action_to_key();
/*
 *  local_malloc(size) -- use the local heap to localize the keymap data
 *    to 1 page.  Note that allocating more than this will revert to using
 *    malloc().
 */
static caddr_t
local_malloc(size)
    int size;
{
    char *p;

    if (!local_heap_hwm) {
	local_heap_hwm = local_heap = (char *)valloc(LOCALHEAPSIZE);
	if (local_heap == 0) {
            perror("local_malloc():");
            exit(1);
        }
    }

    if (local_heap_hwm - local_heap + size > LOCALHEAPSIZE) {
        return (caddr_t)malloc(size);
    } else {
        p = local_heap_hwm;
        local_heap_hwm += size;
        return (caddr_t)p;
    }
}

#ifdef MALLOC_DEBUG
static caddr_t
Malloc(s)
    unsigned s;
{
    caddr_t p;
    char buf[64];
    static int callno, hwm;

    printf("%d: Malloc(%d), hwm %d\n",
            ++callno, s, hwm+=s);
    
    if ((p = (caddr_t)local_malloc(s)) == 0) {
        (void)sprintf(buf, "win_keymap: Malloc(%d) hwm is %d", s,
                local_heap_hwm - local_heap);
        perror(buf);
        exit(1);
    } else {
        return p;
    }
}
#else
#define Malloc(s)	(local_malloc((s)))
#endif

static int
metanormalize(c, mask)
	register int c, mask;
{
	register int d = c % 128;

	if  (d >= 64) {
		if (mask & CTRLMASK)
			return d%32 + 128;
		else if (mask & SHIFTMASK)
			return d%32 + 192;
		else
			return d+128;
	} else {
		return d+128;
	}
}

/******************************************************************************
 *	octal_char(s) -- return the char value represented by the string
 *		pointer pointed to by s.  The double indirection is used to 
 *		consume characters in the string.  Note that on return the
 *		string pointer points one beyond the end of the octal sequence.
 *		An octal sequence is defined by K&R p. 180-1; it is 1 to 3
 *		octal digits.
 ******************************************************************************/
static char
octal_char(str)
	char	**str;
{
	register int	i;
	char	v;

	v = 0;
	for (i = 0; i < 3 && isdigit(**str) && **str <= '7'; i++, (*str)++)
		v = 8 * v + (**str - '0');

	return (v);

}

/******************************************************************************
 *	win_keymap_event_parse(p, e) -- parse the string pointed to by p (char **)
 *		to build the event pointed to by e (Event *).  The string pointer is
 *		modified and the event pointer is returned.
 ******************************************************************************/
#define maskbit(b)	(1<<(b))


#define	SUNOS_3_X	0
#define SUNOS_4_X	1
static unsigned short std_bind;  /* either SUNOS_3_X or SUNOS_4_X */

void
win_keymap_set_inputmask(mask, key, keyflags)
    Inputmask *mask;
    unsigned short key;
    unsigned short keyflags /* ShortEvent sie_bits value */;
{
    if (isworkstationdevid(key)) {
        win_setinputcodebit(mask, (short) key);
        
    } if (ASCII_FIRST <= (short) key && (short) key <= ASCII_LAST) {
        mask->im_flags |= IM_ASCII;
        if (keyflags & SIE_NEGEVENT)
            mask->im_flags |= IM_NEGASCII;

    } else if (META_FIRST <= (short) key && (short) key <= META_LAST) {
        mask->im_flags |= IM_META;
        if (keyflags & SIE_NEGEVENT)
            mask->im_flags |= IM_NEGMETA;

    } else if (TOP_FIRST <= (short) key && (short) key <= TOP_LAST) {
        mask->im_flags |= IM_TOP;
        if (keyflags & SIE_NEGEVENT)
            mask->im_flags |= IM_NEGTOP;

    }
}

void
win_keymap_unset_inputmask(mask, key, keyflags)
    Inputmask *mask;
    unsigned short key;
    unsigned short keyflags /* ShortEvent sie_bits value */;
{
    if (isworkstationdevid(key)) {
        win_unsetinputcodebit(mask, (short) key);

#ifdef KNOWEVERYTHING       
    } if (ASCII_FIRST <= (short) key && (short) key <= ASCII_LAST) {
        mask->im_flags |= IM_ASCII;
        if (keyflags & SIE_NEGEVENT)
            mask->im_flags |= IM_NEGASCII;

    } else if (META_FIRST <= (short) key && (short) key <= META_LAST) {
        mask->im_flags |= IM_META;
        if (keyflags & SIE_NEGEVENT)
            mask->im_flags |= IM_NEGMETA;

    } else if (TOP_FIRST <= (short) key && (short) key <= TOP_LAST) {
        mask->im_flags |= IM_TOP;
        if (keyflags & SIE_NEGEVENT)
            mask->im_flags |= IM_NEGTOP;
#endif

    }
}

void
win_keymap_lazy_init()
{
    void win_keymap_enable(), win_keymap_init();
    
    /* lazy evaluate the keymap initializiation */

    if (!keymap_initialized) {
        win_keymap_init();
        win_keymap_enable();
    }
}

void
win_keymap_lazy_bind()
{
    register int i;
    int first;
    int index;
    unsigned short action;

    /* lazy evaluate which bindings we need by checking defaults */
    
    if (!std_bind) {
        if (defaults_exists("/Compatibility/New_keyboard_accelerators",
                (int *)0)) {
            if (!strcmp("Enabled", (char *)defaults_get_string(
                    "/Compatibility/New_keyboard_accelerators", "Enabled", 0)))
                std_bind = SUNOS_4_X;
            else
                std_bind = SUNOS_3_X;
        } else {
            std_bind = SUNOS_4_X;

         }
    }
}


void
win_keymap_set_imask_from_std_bind(mask, action)
    Inputmask *mask;
    unsigned short action;
{
    register _sunview_keys *key;

    win_keymap_lazy_bind();
    /* take care of all the stuff that may not be semantic events */
    win_keymap_set_inputmask(mask, action, 0);
    
    key = win_keymap_translate_action_to_key(action);
    win_keymap_set_inputmask(mask, key->raw_code, key->keyflags);
}

void
win_keymap_unset_imask_from_std_bind(mask, action)
    Inputmask *mask;
    unsigned short action;
{
    _sunview_keys	*key;

    win_keymap_lazy_bind();

    /* keys is static storage that changes w/ every call to 
     * win_keymap_translate_action_to_keys, so its contents
     * have to be looked at right away.
     */
    key = win_keymap_translate_action_to_key(action);
    win_keymap_unset_inputmask(mask, key->raw_code, key->keyflags);
}

static _sunview_keys *
win_keymap_translate_action_to_key(action)
{
    static _sunview_keys	key;
    
    switch(action) {
	    case ACTION_STOP:
	        key.raw_code = WIN_STOP;
		key.keyflags = 0;
	        break;
	    case ACTION_CAPS_LOCK:
	        key.raw_code = KEY_TOP(1);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_AGAIN:
	        key.raw_code = KEY_LEFT(2);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_PROPS:
	        key.raw_code = KEY_LEFT(3);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_UNDO:
	        key.raw_code = KEY_LEFT(4);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_FRONT:
	        key.raw_code = KEY_LEFT(5);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_BACK:
	        key.raw_code = KEY_LEFT(5);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_COPY:
	        key.raw_code = KEY_LEFT(6);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_OPEN:
	        key.raw_code = KEY_LEFT(7);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_CLOSE:
	        key.raw_code = KEY_LEFT(7);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_PASTE:
	        key.raw_code = KEY_LEFT(8);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_FIND_BACKWARD:
	        key.raw_code = KEY_LEFT(9);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_FIND_FORWARD:
	        key.raw_code = KEY_LEFT(9);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_REPLACE:
	        key.raw_code = KEY_LEFT(9);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_CUT:
	        key.raw_code = KEY_LEFT(10);
		key.keyflags = SIE_NEGEVENT;
		break;
	    case ACTION_HELP:
	        key.raw_code = 32569;
		key.keyflags = SIE_NEGEVENT;
		break;
    }
    return &key;
}
/******************************************************************************
 *	Inputmask Manipulation  --  The goal is to do reasonable job of handling
 *	the input mask.  The new semantic events do not appear in the current
 *	input mask since they are not real keys.  The strategy here is that
 *	given some semantic event Y, look through the mappings for occurrences
 *	of Y in the range (remember domain and range of a mapping X -> Y) and then
 *	take the domain value X as the thing whose input mask bits we are
 *	talking about.
 ******************************************************************************/

/*
 *    Semantic bit mask manipulation
 */
void
win_keymap_set_smask(windowfd, code)
	int windowfd;
	unsigned short code;
{
    Bitmask	*m = keymap_from_fd[windowfd].smask;

    win_keymap_lazy_init();
    win_keymap_lazy_bind();

    if (!m)
	m = keymap_from_fd[windowfd].smask =
	            sv_bits_new_mask(SUNVIEW_LAST - SUNVIEW_FIRST + 1);

    if (SUNVIEW_FIRST <= code && code <= SUNVIEW_LAST)
	(void)sv_bits_set_mask(m, code - SUNVIEW_FIRST);
}

void
win_keymap_unset_smask(windowfd, code)
	int windowfd;
	unsigned short code;
{
	Bitmask	*m = keymap_from_fd[windowfd].smask;

	if (!m) return;

	if (SUNVIEW_FIRST <= code && code <= SUNVIEW_LAST)
		(void)sv_bits_unset_mask(m, code - SUNVIEW_FIRST);
}

int
win_keymap_get_smask(windowfd, code)
	int windowfd;
	unsigned short code;
{
	Bitmask	*m = keymap_from_fd[windowfd].smask;

	if (!m) return 0;
	return sv_bits_get_mask(m, code - SUNVIEW_FIRST);
}

/*
 *    Semantic class definitions
 */
static unsigned short funct_keys[] = {
    ACTION_STOP,
    ACTION_AGAIN,
    ACTION_PROPS,
    ACTION_UNDO,
    ACTION_FRONT,
    ACTION_BACK,
    ACTION_COPY,
    ACTION_OPEN,
    ACTION_CLOSE,
    ACTION_PASTE,
    ACTION_FIND_BACKWARD,
    ACTION_FIND_FORWARD,
    ACTION_REPLACE,
    ACTION_CUT,
0 };

static unsigned short edit_keys[] = {
    ACTION_ERASE_CHAR_BACKWARD,
    ACTION_ERASE_CHAR_FORWARD,
    ACTION_ERASE_WORD_BACKWARD,
    ACTION_ERASE_WORD_FORWARD,
    ACTION_ERASE_LINE_BACKWARD,
    ACTION_ERASE_LINE_END,
0 };

static unsigned short motion_keys[] = {
    ACTION_GO_CHAR_BACKWARD,
    ACTION_GO_CHAR_FORWARD,
    ACTION_GO_WORD_BACKWARD,
    ACTION_GO_WORD_FORWARD,
    ACTION_GO_WORD_END,
    ACTION_GO_LINE_BACKWARD,
    ACTION_GO_LINE_FORWARD,
    ACTION_GO_LINE_END,
    ACTION_GO_LINE_START,
    ACTION_GO_COLUMN_BACKWARD,
    ACTION_GO_COLUMN_FORWARD,
    ACTION_GO_DOCUMENT_START,
    ACTION_GO_DOCUMENT_END,
0 };

static unsigned short text_keys[] = {
    ACTION_CAPS_LOCK,
    ACTION_SELECT_FIELD_BACKWARD,
    ACTION_SELECT_FIELD_FORWARD,
    ACTION_COPY_THEN_PASTE,
    ACTION_STORE,
    ACTION_LOAD,
    ACTION_GET_FILENAME,
    ACTION_SET_DIRECTORY,
    ACTION_MATCH_DELIMITER,
    ACTION_EMPTY,
    ACTION_INSERT,
0 };

void
win_keymap_set_smask_class(windowfd, class)
	int windowfd;
	Event_class class;
{
    Bitmask	*m = keymap_from_fd[windowfd].smask;
    unsigned short *p;

    win_keymap_lazy_init();
    win_keymap_lazy_bind();

    if (!m)
        m = keymap_from_fd[windowfd].smask =
	            sv_bits_new_mask(SUNVIEW_LAST - SUNVIEW_FIRST + 1);

    switch (class) {
        case KEYMAP_FUNCT_KEYS:
            for (p = funct_keys; *p ; p++)
                (void)sv_bits_set_mask(m, *p - SUNVIEW_FIRST);
            break;
        case KEYMAP_EDIT_KEYS:
            for (p = edit_keys; *p ; p++)
                (void)sv_bits_set_mask(m, *p - SUNVIEW_FIRST);
            break;
        case KEYMAP_MOTION_KEYS:
            for (p = motion_keys; *p ; p++)
                (void)sv_bits_set_mask(m, *p - SUNVIEW_FIRST);
            break;
        case KEYMAP_TEXT_KEYS:
            for (p = text_keys; *p ; p++)
                (void)sv_bits_set_mask(m, *p - SUNVIEW_FIRST);
            break;
    }
}

void
win_keymap_set_imask_class(mask, class)
	Inputmask *mask;
	Event_class class;
{
/*
    unsigned short *p;

    win_keymap_lazy_init();
    win_keymap_lazy_bind();

    switch (class) {
        case KEYMAP_FUNCT_KEYS:
            for (p = funct_keys; *p ; p++)
                win_keymap_set_imask_from_std_bind(mask, *p);
            break;
        case KEYMAP_EDIT_KEYS:
            for (p = edit_keys; *p ; p++)
                win_keymap_set_imask_from_std_bind(mask, *p);
            break;
        case KEYMAP_MOTION_KEYS:
            for (p = motion_keys; *p ; p++)
                win_keymap_set_imask_from_std_bind(mask, *p);
            break;
        case KEYMAP_TEXT_KEYS:
            for (p = text_keys; *p ; p++)
                win_keymap_set_imask_from_std_bind(mask, *p);
            break;
    }
*/
}

/******************************************************************************
 *	win_keymap_map(fd, event) -- attempt to translate the event into
 *		a keymap mapping; return 1 if translation occurs, 0 if not
 ******************************************************************************/

static int mapped_fd;
static unsigned short mapped_event, mapped_action, mapped_shiftmask,
                      mapped_flags;
int
win_keymap_map(fd, event)
	int fd;
	Event *event;
{
    register struct keymap_table	*km;
    register unsigned short		 ie_code = event_id(event), action = 0;
    register unsigned short		 user_code;
    register Bitmask			*kb = keymap_from_fd[fd].smask;

    win_keymap_lazy_init();
    win_keymap_lazy_bind();

    if (!kb)
        kb = keymap_from_fd[fd].smask =
	            sv_bits_new_mask(SUNVIEW_LAST - SUNVIEW_FIRST + 1);

    if (std_bind == SUNOS_4_X) {
      
      if (ie_code < 160 || ie_code > 255 || event_meta_is_down(event)) {   
      /*
      *  Only execute switch statement if ie_code is not 0xA0
      *  through 0xFF and generated by compose key on type-4
      *  keyboard.
      */
        switch(ie_code) {
	    case WIN_STOP:
		action = ACTION_STOP;
		break;
	    case KEY_TOP(1):
	        action = ACTION_CAPS_LOCK;
		break;
	    case 127:	/* DEL */
                /* kludge for compatibility with 4.0.
                 * do mapping if user has not modified default
                 * mappings (i.e., user_code == 0). This is
		 * never true in 4.1 since the default keys
                 * are always mapped in textsw_once.c.
                 * This applies to DEL, ^W and ^U
                 */
		if (keymap_from_fd[fd].keymap) 
		    user_code = keymap_from_fd[fd].keymap->km_table[WIN_KEYMAP_ERASE_CHAR].code; 
		else 
		    user_code = 127;
		switch(user_code) {
		    case 0:
		    case 127:
			if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_CHAR_FORWARD;
			} else {
			    action =  ACTION_ERASE_CHAR_BACKWARD;
			}
			break;
		    default:
			break;
		}
		break;
	    case 23:	/* ^W */
                /* kludge for compatibility with 4.0.
                 * do mapping if user has not modified default
                 * mappings 
                 */
		if (keymap_from_fd[fd].keymap) 
		    user_code = keymap_from_fd[fd].keymap->km_table[WIN_KEYMAP_ERASE_WORD].code; 
		else 
		    user_code = 23;
		switch(user_code) {
		    case 0:
		    case 23:
			if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_WORD_FORWARD;
			} else {
			    action =  ACTION_ERASE_WORD_BACKWARD;
			}
			break;
		    default:
			break;
		}
		break;
	    case 21:	/* ^U */
                /* kludge for compatibility with 4.0.
                 * do mapping if user has not modified default
                 * mappings 
                 */
		if (keymap_from_fd[fd].keymap) 
		    user_code = keymap_from_fd[fd].keymap->km_table[WIN_KEYMAP_ERASE_LINE].code; 
		else 
		    user_code = 21;
		switch(user_code) {
		    case 0:
		    case 21:
			if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_LINE_END;
			} else {
			    action =  ACTION_ERASE_LINE_BACKWARD;
			}
			break;
		    default:
			break;
		}
		break;
	    case 225:	/* Meta-a */
	    case KEY_LEFT(2):
	        action = ACTION_AGAIN;
		break;
	    case 245:	/* Meta-u */
	        action = ACTION_UNDO;
		break;
	    case 227:	/* Meta-c */
	    case 195:	/* Meta-C */
	    case 131:	/* Meta-Cntl-C */
	    case KEY_LEFT(6):
	        action = ACTION_COPY;
		break;
	    case 246:	/* Meta-v */
	    case 214:	/* Meta-V */
	    case 150:	/* Meta-Cntl-V */
	    case KEY_LEFT(8):
	        action = ACTION_PASTE;
		break;
	    case 198:	/* Meta-F */
	        action = ACTION_FIND_BACKWARD;
		break;
	    case 230:	/* Meta-f */
	        action = ACTION_FIND_FORWARD;
		break;
	    case 248:	/* Meta-x */
	    case 216:	/* Meta-X */
	    case 152:	/* Meta-Cntl-X */
	    case KEY_LEFT(10):
	        action = ACTION_CUT;
		break;
	    case 175:	/* Meta-/ */
	    case 191:	/* Meta-? */
	    case 32569:	/* Help Key */
	        action = ACTION_HELP;
		break;
		/*
	    case 241:	 Meta-q 
	    case 209:	 Meta-Q 
	        action = ACTION_QUOTE;
		break;
	    case 141:	Meta-Cntl-M
	        action = ACTION_DO_IT;
		break;
		 */
	    case KEY_LEFT(3):
	        action = ACTION_PROPS;
		break;
	    case KEY_RIGHT(11):
	        action = ACTION_GO_LINE_FORWARD;
		break;
	    case KEY_RIGHT(13):
	        action = ACTION_GO_DOCUMENT_END;
		break;
	    case KEY_RIGHT(7):
	        action = ACTION_GO_DOCUMENT_START;
		break;
	    case KEY_LEFT(4):
	        if (event_shiftmask(event) & SHIFTMASK) {
	            action = ACTION_REDO;
		} else {
	            action = ACTION_UNDO;
		}
		break;
	    case KEY_LEFT(5):
	        if (event->ie_shiftmask & SHIFTMASK) {
	            action = ACTION_BACK;
		} else {
	            action = ACTION_FRONT;
		}
		break;
	    case KEY_LEFT(7):
	        if (event->ie_shiftmask & SHIFTMASK) {
	            action = ACTION_CLOSE;
		} else {
	            action = ACTION_OPEN;
		}
		break;
	    case KEY_LEFT(9):
	        if (event_shiftmask(event) & SHIFTMASK) {
	            action = ACTION_FIND_BACKWARD;
		} else if (event_shiftmask(event) & CTRLMASK) {
	            action = ACTION_REPLACE;
		} else {
	            action = ACTION_FIND_FORWARD;
		}
		break;
	    case 2:	/* cntl-b */
	        if (event_shiftmask(event) & CTRLMASK) {
	            if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_CHAR_FORWARD;
		    } else {
	                action = ACTION_GO_CHAR_BACKWARD;
		    }
		}
		break;
	    case 6:	/* cntl-F */
	        if (event_shiftmask(event) & CTRLMASK) {
	            if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_CHAR_BACKWARD;
		    } else {
	                action = ACTION_GO_CHAR_FORWARD;
		    }
		}
		break;
	    case 44:	/* cntl-, */
	        if (event_shiftmask(event) & CTRLMASK) {
	            if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_WORD_FORWARD;
		    } else {
	                action = ACTION_GO_WORD_BACKWARD;
		    }
		}
		break;
	    case 46:	/* cntl-. */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_WORD_BACKWARD;
		    } else {
	                action = ACTION_GO_WORD_FORWARD;
		    }
		}
		break;
	    case 31:	/* US */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_WORD_BACKWARD;
		    } else {
	                action = ACTION_GO_WORD_FORWARD;
		    }
		}
		break;
	    case 1:	/* cntl-a */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_LINE_END;
		    } else {
	                action = ACTION_GO_LINE_BACKWARD;
		    }
		}
		break;
	    case 5:	/* cntl-e */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_LINE_BACKWARD;
		    } else {
	                action = ACTION_GO_LINE_END;
		    }
		}
		break;
	    case ';':
	        if (event_shiftmask(event) & CTRLMASK) {
	            action = ACTION_GO_LINE_FORWARD;
		}
		break;
	    case 16:	/* cntl-p */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_COLUMN_FORWARD;
		    } else {
	                action = ACTION_GO_COLUMN_BACKWARD;
		    }
		}
		break;
	    case 14:	/* cntl-n */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_COLUMN_BACKWARD;
		    } else {
	                action = ACTION_GO_COLUMN_FORWARD;
		    }
		}
		break;
	    case 13:	/* cntl-m */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_DOCUMENT_START;
		    } else {
	                action = ACTION_GO_DOCUMENT_END;
		    }
		}
		break;
	    case 9:	/* cntl-i */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_SELECT_FIELD_BACKWARD;
		    } else {
	                action = ACTION_SELECT_FIELD_FORWARD;
		    }
		}
		break;
	    case 240:	/* Meta-p */
                action = ACTION_COPY_THEN_PASTE;
		break;
	    case 243:	/* Meta-s */
                action = ACTION_STORE;
		break;
	    case 236:	/* Meta-l */
                action = ACTION_LOAD;
		break;
	    case 233:	/* Meta-i */
                action = ACTION_INSERT;
		break;
	    case 229:	/* Meta-e */
                action = ACTION_EMPTY;
	    	break;
	    case 228:	/* Meta-d */
                action = ACTION_MATCH_DELIMITER;
		break;
	    default:
	        /* Check to see if this is one of the 
                 * client-modifiable keys.
		 */

		if (keymap_from_fd[fd].keymap) {
		
		    km = keymap_from_fd[fd].keymap->km_table;
		    if (ie_code != km[WIN_KEYMAP_ERASE_CHAR].code &&
		        ie_code != km[WIN_KEYMAP_ERASE_WORD].code &&
			ie_code != km[WIN_KEYMAP_ERASE_LINE].code) {
			
			return;
		    } else if (ie_code == km[WIN_KEYMAP_ERASE_CHAR].code) {
		        if (event_shiftmask(event) & SHIFTMASK) {
			    action = ACTION_ERASE_CHAR_FORWARD;
			} else {
			    action = ACTION_ERASE_CHAR_BACKWARD;
			}
		    } else if (ie_code == km[WIN_KEYMAP_ERASE_WORD].code) {
			if (event_shiftmask(event) & SHIFTMASK) {
			    action = ACTION_ERASE_WORD_FORWARD;
			} else {
			    action = ACTION_ERASE_WORD_BACKWARD;
			}
		    } else if (ie_code == km[WIN_KEYMAP_ERASE_LINE].code) {
		        if (event_shiftmask(event) & SHIFTMASK) {
			    action = ACTION_ERASE_LINE_END;
			} else {
			    action = ACTION_ERASE_LINE_BACKWARD;
			}
		    }
		}
	}
      }

    } else {
        switch(ie_code) {
	    case WIN_STOP:
	        action =  ACTION_STOP;
		break;
	    case KEY_TOP(1):
	        action =  ACTION_CAPS_LOCK;
		break;
	    case 127:	/* DEL */
                /* kludge for compatibility with 4.0.
                 * do mapping if user has not modified default
                 * mappings 
                 */
		if (keymap_from_fd[fd].keymap) 
		    user_code = keymap_from_fd[fd].keymap->km_table[WIN_KEYMAP_ERASE_CHAR].code; 
		else 
		    user_code = 127;
		switch(user_code) {
		    case 0:
		    case 127:
			if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_CHAR_FORWARD;
			} else {
			    action =  ACTION_ERASE_CHAR_BACKWARD;
			}
			break;
		    default:
			break;
		}
		break;
	    case 23:	/* ^W */
                /* kludge for compatibility with 4.0.
                 * do mapping if user has not modified default
                 * mappings 
                 */
		if (keymap_from_fd[fd].keymap) 
		    user_code = keymap_from_fd[fd].keymap->km_table[WIN_KEYMAP_ERASE_WORD].code; 
		else 
		    user_code = 23;
		switch(user_code) {
		    case 0:
		    case 23:
			if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_WORD_FORWARD;
			} else {
			    action =  ACTION_ERASE_WORD_BACKWARD;
			}
			break;
		    default:
			break;
		}
		break;
	    case 21:	/* ^U */
                /* kludge for compatibility with 4.0.
                 * do mapping if user has not modified default
                 * mappings 
                 */
		if (keymap_from_fd[fd].keymap) 
		    user_code = keymap_from_fd[fd].keymap->km_table[WIN_KEYMAP_ERASE_LINE].code; 
		else 
		    user_code = 21;
		switch(user_code) {
		    case 0:
		    case 21:
			if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_LINE_END;
			} else {
			    action =  ACTION_ERASE_LINE_BACKWARD;
			}
			break;
		    default:
			break;
		}
		break;
	    case KEY_LEFT(2):
	        action =  ACTION_AGAIN;
		break;
	    case KEY_LEFT(6):
	        action =  ACTION_COPY;
		break;
	    case KEY_LEFT(8):
	        action =  ACTION_PASTE;
		break;
	    case KEY_LEFT(10):
	        action =  ACTION_CUT;
		break;
	    case 32569:	/* Help Key */
	        action =  ACTION_HELP;
		break;
	    case KEY_LEFT(3):
	        action =  ACTION_PROPS;
		break;
	    case KEY_RIGHT(11):
	        action =  ACTION_GO_LINE_FORWARD;
		break;
	    case KEY_RIGHT(13):
	        action =  ACTION_GO_DOCUMENT_END;
		break;
	    case KEY_RIGHT(7):
	        action =  ACTION_GO_DOCUMENT_START;
		break;
	    case KEY_LEFT(4):
	        if (event_shiftmask(event) & SHIFTMASK) {
		    action =  ACTION_REDO;
		} else {
		    action =  ACTION_UNDO;
		}
		break;
	    case KEY_LEFT(5):
	        if (event_shiftmask(event) & SHIFTMASK) {
		    action =  ACTION_BACK;
		} else {
		    action =  ACTION_FRONT;
		}
		break;
	    case KEY_LEFT(7):
	        if (event_shiftmask(event) & SHIFTMASK) {
		    action =  ACTION_CLOSE;
		} else {
		    action =  ACTION_OPEN;
		}
		break;
	    case KEY_LEFT(9):
	        if (!event_shiftmask(event)) {
		    action =  ACTION_FIND_FORWARD;
		} else if (event_shiftmask(event) & CTRLMASK) {
		    action =  ACTION_REPLACE;
		} else {
		    action =  ACTION_FIND_BACKWARD;
		}
		break;
	    case 13:	/* cntl-ret */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_DOCUMENT_START;
		    } else {
	                action = ACTION_GO_DOCUMENT_END;
		    }
		}
		break;
	    case 16:	/* cntl-p */
	        if (event_shiftmask(event) & CTRLMASK) {
		    if (event_shiftmask(event) & SHIFTMASK) {
	                action = ACTION_GO_COLUMN_FORWARD;
		    } else {
	                action = ACTION_GO_COLUMN_BACKWARD;
		    }
		}
	    default:
	        /* Check to see if this is one of the client-modifiable
		 * keys
		 */
		if (keymap_from_fd[fd].keymap) {
		    
		    km = keymap_from_fd[fd].keymap->km_table;
		    if (ie_code != km[WIN_KEYMAP_ERASE_CHAR].code &&
		        ie_code != km[WIN_KEYMAP_ERASE_WORD].code &&
			ie_code != km[WIN_KEYMAP_ERASE_LINE].code) {
			
			return;
		    } else if (ie_code == km[WIN_KEYMAP_ERASE_CHAR].code &&
		               mask_matches(event_shiftmask(event), 
			       km[WIN_KEYMAP_ERASE_CHAR].mask)) {
		        if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_CHAR_FORWARD;
			} else {
			    action =  ACTION_ERASE_CHAR_BACKWARD;
			}
		    } else if (ie_code == km[WIN_KEYMAP_ERASE_WORD].code &&
		    		mask_matches(event_shiftmask(event),
				km[WIN_KEYMAP_ERASE_WORD].mask)) {
			if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_WORD_FORWARD;
			} else {
			    action =  ACTION_ERASE_WORD_BACKWARD;
			}
		    } else if (ie_code == km[WIN_KEYMAP_ERASE_LINE].code &&
		    		mask_matches(event_shiftmask(event),
				km[WIN_KEYMAP_ERASE_LINE].mask)) {
		        if (event_shiftmask(event) & SHIFTMASK) {
			    action =  ACTION_ERASE_LINE_END;
			} else {
			    action =  ACTION_ERASE_LINE_BACKWARD;
			}
		    }
		}
	}
    }
    if (action && sv_bits_get_mask(kb, action - SUNVIEW_FIRST)) {
	event_set_action(event, action);
    }
}


/******************************************************************************
 *	win_keymap_code_and_mask(newevent, mask, mapping) -- take the
 *		event and mask and map the event into the corresponding mask
 *		and mapping event.
 ******************************************************************************/
void
win_keymap_map_code_and_mask(newevent, mask, mapping, fd)
    int newevent;
    int mask;
    int mapping;
    int fd;
{
     struct keymap_table	*km;
     
      if (!keymap_from_fd[fd].keymap) {
          keymap_from_fd[fd].keymap = (Keymap *)Malloc(sizeof(Keymap));
      }
      km = keymap_from_fd[fd].keymap->km_table;

    if (mapping == ACTION_ERASE_CHAR_BACKWARD) {
        km[WIN_KEYMAP_ERASE_CHAR].code = newevent;
	km[WIN_KEYMAP_ERASE_CHAR].mask = mask;
    } else if (mapping == ACTION_ERASE_WORD_BACKWARD) {
        km[WIN_KEYMAP_ERASE_WORD].code = newevent;
	km[WIN_KEYMAP_ERASE_WORD].mask = mask;
    } else if (mapping == ACTION_ERASE_LINE_BACKWARD) {
        km[WIN_KEYMAP_ERASE_LINE].code = newevent;
	km[WIN_KEYMAP_ERASE_LINE].mask = mask;
    }
    return;
}

void
win_keymap_unmap_code_and_mask(mapping, fd)
int	mapping;
int	fd;
{
    struct keymap_table	*km;

    if (!keymap_from_fd[fd].keymap) {
	return;
    } else {
	km = keymap_from_fd[fd].keymap->km_table;
    }
    
    if (mapping == ACTION_ERASE_CHAR_BACKWARD) {
        km[WIN_KEYMAP_ERASE_CHAR].code = 0;
	km[WIN_KEYMAP_ERASE_CHAR].mask = 0;
    } else if (mapping == ACTION_ERASE_WORD_BACKWARD) {
        km[WIN_KEYMAP_ERASE_WORD].code = 0;
	km[WIN_KEYMAP_ERASE_WORD].mask = 0;
    } else if (mapping == ACTION_ERASE_LINE_BACKWARD) {
        km[WIN_KEYMAP_ERASE_LINE].code = 0;
	km[WIN_KEYMAP_ERASE_LINE].mask = 0;
    }
    return;
}

void
win_keymap_init()
{
	register int i;
	
	/* Initialize the fd-to-keymap map */
	
	for (i = 0; i < NOFILE; i++) {
		keymap_from_fd[i].keymap = (Keymap *) 0;
		keymap_from_fd[i].smask = (Bitmask *) 0;
	}
	keymap_initialized = 1;
}

void
win_keymap_enable()
{
    keymap_enable++;
}

void
win_keymap_disable()
{
    keymap_enable = 0;
}

/**************************************************************
 * win_keymap_map_code_and_masks(), 
 * win_keymap_unmap_code_and_masks(), and
 * win_keymap_map_pending(), 
 * are included for compatibilty. These three functions
 * were a part of the 4.0 release but are no longer used in the 
 * latest libsuntool and libsunwindow libraries. However, they 
 * may still be needed by older binaries that use a libsuntool
 * library from 4.0. These functions are not the same as their 
 * 4.0 versions. Only the names are preserved.
 **************************************************************/
 
int
win_keymap_map_pending(fd, event)
int fd;
Event *event;
{ 
	/* There are no events that map 1-to-n in this system */
	return (0);
}

/***************************************************************
 * win_keymap_code_and_masks(newevent, masklist, mapping, fd) --
 * take the event and masklist and map those events into the
 * corresponding mask and mapping events.  Masklists are -1 terminated.
 ****************************************************************/


void
win_keymap_map_code_and_masks(newevent, masklist, mapping, fd)
int newevent;  
int masklist[];
int mapping;  
int fd;
{
	int *mask;

	for (mask = masklist; *mask != -1; mask++) {
	    win_keymap_map_code_and_mask(newevent, mask, mapping, fd);
	}	     

}

/* 
 * Empty function for compatibility 
 */

void
win_keymap_unmap_code_and_masks(newevent, masklist, fd)
int newevent;  
int masklist[];
int fd;
{

	/*
	 * This function is a leftover from 4.0. It 
	 * was called to unmap edit_back_char, edit_back_word, 
	 * edit_back_line (only) from ttysw/ttysw_init.c. This 
	 * was required to allow the user to override the default 
	 * settings rather than append to the default list. The 
	 * new keymapping mechanism does not allow an 1-to-n (action
	 * to event) mapping, thereby making this function 
	 * unnecessary. In the new mechanism when a new character 
	 * is mapped the old mapping is lost. 
	 */

	/* Do nothing. */
	return;
}
