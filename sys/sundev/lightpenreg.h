/*	@(#)lightpenreg.h  1.1 92/07/30	*/
 
/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */
 
#ifndef _lightpenreg_h
#define _lightpenreg_h

/*
 * Lightpen related macros. See also <sundev/vuid_event.h>
 */

#ifndef LIGHTPEN_DEVID
#define LIGHTPEN_DEVID 0x79
#endif LIGHTPEN_DEVID

/*
 * lightpen interface
 */
struct	lightpen_events {
	struct	lightpen_events_data	*event_list;
	unsigned  int		flags;
	unsigned  int		event_count;
};

struct	lightpen_events_data {
	struct lightpen_events_data	*next_event;
	unsigned int		event_type;
	unsigned int		event_value;
};

#define	LP_READING	0x1

#define LPTICKS		1

/*	
 * lightpen events defines 
 */
/* XXX - These two defines have to be removed after filemerge */
#define LIGHTPEN_X		0
#define LIGHTPEN_Y		1

#define LIGHTPEN_MOVE		0
#define LIGHTPEN_DRAG		1
#define LIGHTPEN_BUTTON		2	
#define LIGHTPEN_UNDETECT	3

#define LIGHTPEN_VUID_MOVE	\
		((LIGHTPEN_DEVID << 8) | LIGHTPEN_MOVE)
#define LIGHTPEN_VUID_DRAG	\
		((LIGHTPEN_DEVID << 8) | LIGHTPEN_DRAG)
#define LIGHTPEN_VUID_BUTTON	\
		((LIGHTPEN_DEVID << 8) | LIGHTPEN_BUTTON)
#define LIGHTPEN_VUID_UNDETECT	\
		((LIGHTPEN_DEVID << 8) | LIGHTPEN_UNDETECT)

/*
 * Defines for button state 
 */
#define LIGHTPEN_BUTTON_UP	0
#define LIGHTPEN_BUTTON_DOWN	1

#define LIGHTPEN_MASK_X		0x00000FFF
#define LIGHTPEN_MASK_Y		0x00FFF000
#define LIGHTPEN_MASK_BUTTON	0x01000000

#define LIGHTPEN_SHIFT_X	0x0	/* SHIFT = 0 */
#define LIGHTPEN_SHIFT_Y	0xC	/* SHIFT = 12 */
#define LIGHTPEN_SHIFT_BUTTON	0x18	/* SHIFT = 24 */

#define event_is_lightpen(event)  (vuid_in_range(LIGHTPEN_DEVID, event->ie_code))
				
	
#define LIGHTPEN_EVENT(event)	((event) & 0xFF)
#define GET_LIGHTPEN_X(value)	((value) & LIGHTPEN_MASK_X)
#define GET_LIGHTPEN_Y(value)	(((value) & LIGHTPEN_MASK_Y) \
			  >> LIGHTPEN_SHIFT_Y)
#define GET_LIGHTPEN_BUTTON(value) (((value) & LIGHTPEN_MASK_BUTTON) \
			  >> LIGHTPEN_SHIFT_BUTTON)
/* 
 * Struct defines for calibration & filter
 */

typedef struct lightpen_calibrate{
	int	x;		/* X value of calibration */
	int	y;		/* Y value of calibration */
} lightpen_calibrate_t;

typedef struct lightpen_filter{
	u_int	flag;		/* Misc */
	u_int	tolerance;	/* Filter tolrance value */
	u_int	count;		/* Number of entries */
#define LIGHTPEN_FILTER_MAXCOUNT  1024 /* Max number of entries used 
					   by the filter */
}lightpen_filter_t;

/*
 * Ioctls defines
 */

#define LIGHTPEN_CALIBRATE  _IOW(L, 1, lightpen_calibrate_t)
#define LIGHTPEN_FILTER     _IOW(L, 2, lightpen_filter_t)

#endif _lightpenreg_h
