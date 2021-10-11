/*      @(#)dbio.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/ioccom.h>


/* Dialbox related macros. See also <sundev/vuid_event.h> */
/*
#ifndef DIAL_DEVID
#define DIAL_DEVID 0x7b
#endif
*/

#ifndef _sundev_dbio_h
#define _sundev_dbio_h

/* DELTA event for a particular dial */
#define DIAL_DELTA(dialnum)     (vuid_first(DIAL_DEVID) + (dialnum))

/* Macro to extract dial number from event */
#define DIAL_NUMBER(event_code)	(event_code & 0xFF)

#define	event_is_dial(event)	(vuid_in_range(DIAL_DEVID, event->ie_code))

/* Dial deltas are in 64ths of degrees, convert to degrees */
#define DIAL_TO_DEGREES(n)	((float)(n) / 64.0)

#define DIAL_UNITS_PER_CYCLE (64*360)



/*************************************************************/
/************************BUTTON BOX definitions *************/
/***********************************************************/


/* Name for the ioctl to turn the leds on the button box on */
#define DBIOBUTLITE _IOW(B, 0, int)


/*Macros for the button box */
#define	event_is_32_button(event) (vuid_in_range(BUTTON_DEVID, event->ie_code))


/*These are the bit masks to turn the leds and buttons on/off */
#define BUTTON_MAP(butnum)  (butnum)
#define LED_MAP(butnum)	    (1<< (butnum - 1))

#endif _sundev_dbio_h
