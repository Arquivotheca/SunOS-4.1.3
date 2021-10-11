/*      @(#)pw_dblbuf.h 1.1 92/07/30 SMI      */

#include <sys/types.h>
#include <sunwindow/attr.h>

typedef int		Pw_attribute_value;

#define	PIXWIN_ATTR(type, ordinal)	ATTR(ATTR_PKG_PIXWIN, type, ordinal)

typedef enum {

	/* integer attributes */
	PW_DBL_AVAIL		= PIXWIN_ATTR(ATTR_BOOLEAN, 20),/* DBL Available      */
	PW_DBL_DISPLAY		= PIXWIN_ATTR(ATTR_INT, 21), 	/* Display control bit*/
	PW_DBL_WRITE	 	= PIXWIN_ATTR(ATTR_INT, 22),	/* Write control bits */
	PW_DBL_READ		= PIXWIN_ATTR(ATTR_INT, 23),	/* Read Control bits  */
	PW_DBL_ACCESS		= PIXWIN_ATTR(ATTR_INT, 24),	/* Access flag  */
	PW_DBL_RELEASE		= PIXWIN_ATTR(ATTR_INT, 25),	/* Release flag  */
} Pw_dbl_attribute;


#ifndef	KERNEL
extern int pw_dbl_get();
extern int pw_dbl_set();
#endif	KERNEL

#define PW_DBL_EXISTS   1       /* possible return val for the PW_DBL_AVAIL attribute */
#define PW_DBL_ERROR	-1	/* Error while trying to set the dbl control bits */
#define PW_DBL_FORE	2	/* Set a control bit to the foreground */
#define PW_DBL_BACK	3	/* Set a control bit to the background */
#define PW_DBL_BOTH	4	/* Set a control bit to both backgr and the foregr */


