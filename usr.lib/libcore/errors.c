/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)errors.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <stdio.h>

static int erreport = -1;

#define NMSG 115

/*
 * Report error and what function it occurred in.
 */
_core_errhand(funcname, errnum)
    char *funcname;
    int errnum;
{
    static char *errtable[] = {
			    "The CORE SYSTEM has already been initialized.",	/* 0 */
			       "The specified level cannot be supported.",
			       "The surface has already been initialized.",
    "No physical surface is associated with the specified logical surface.",
			       "The CORE SYSTEM has not been initialized.",
			  "The specified surface has not been initialized.",
			       "The specified surface is already selected.",
			       "The specified surface was not selected.",
			       "A segment is open.",
			       "The specified surface is not selected.",
			   "The specified surface has not been deselected.",	/* 10 */
			       "This function has already been called once.",
			       "A segment has been opened.",
		   "A value specified for a default attribute is improper.",
			       "The specified segment does not exist.",
			       "The VIEW SURFACE ARRAY is not large enough.",
			     "Segment list overflow, can't create segment.",
		  "There has been no 'end batch' since last 'begin batch'.",
			   "There has been no corresponding 'begin batch'.",
      "A viewing function has been invoked, or a segment has been created.",
			       "The value for TYPE is improper.",	/* 20 */
			       "No segment is open.",
			       "n is <= 0.",
			       "String contains an illegal character.",
	    "The vectors established by CHARSPACE and CHARUP are parallel.",
			       "Invalid marker table offset.",
			       "Invocation when no open segment.",
			       "Invalid attribute value.",
			       "Invalid segment type.",
			       "Invalid segment number.",
			    "Invalid image transformation for the segment.",	/* 30 */
			 "A retained segment named SEGNAME already exists.",
       "The segment type is inconsistent with the current IMAGE_TRANSFORM.",
			       "No view surface is currently selected.",
		       "The current viewing specification is inconsistent.",
			       "No view surfaces have been initialized.",
		    "There is an existing retained segment named NEW_NAME.",
			 "There is no retained segment named SEGMENT_NAME.",
			       "No characters in string (n=0).",
	   "Dx, dy, and dz, are all zero: no direction can be established.",
			     "MIN is not less than MAX, for u or v bounds.",	/* 40 */
       "FRONT_DISTANCE exceeds BACK_DISTANCE; back clip plane is in front.",
			       "'ndcsp2' or 'ndcsp3' has been invoked since SunCore was last initialized.",
			       "The invocation of 'ndcspx' is too late, default values have been assumed.",
      "A parameter value is greater than 1, or is less than or equal to 0.",
			       "Neither parameter has a value of 1.",
	"Viewport extent is outside of normalized device coordinate space.",
			 "MIN is not less than MAX, for x, y, or z bounds.",
			       "Specified device already enabled.",
			       "DEVICE_CLASS or DEVICE_NUM invalid.",
			       "DEVICE_CLASS invalid.",	/* 50 */
			       "Specified device is not enabled.",
			       "LOCATOR_NUM is invalid.",
			     "The specified LOCATOR device is not enabled.",
			       "VALUATOR_NUM is invalid.",
			    "The specified VALUATOR device is not enabled.",
			       "The TIME value is less than zero.",
	   "EVENT_CLASS and EVENT_NUM do not specify a valid event device.",
			   "EVENT_CLASS is not a legal event device class.",
			       "The specified association already exists.",
    "EVENT_CLASS or SAMPLED_CLASS reference invalid or wrong type of class.",	/* 60 */
    "EVENT_NUM or SAMPLED_NUM are invalid device numbers for their classes.",
			       "The specified association does not exists.",
		      "The current event report is not from a PICK device.",
		   "The current event report is not from a KEYBOARD event.",
    "Input string was not large enough to hold the string centered by user.",
			       "When event occurred, the LOCATOR device was not enabled or was not associated with the event device.",
			       "When event occurred, the VALUATOR device was not enabled or was not associated with the event device.",
		     "XECHO and YECHO specify positions outside NDC space.",
			   "PICK_NUM does not specify a valid PICK device.",
		     "LOCATOR_NUM does not specify a valid LOCATOR device.",	/* 70 */
			       "XLOC,YLOC specify a position outside normalized device coordinate space.",
			     "VALUATOR_NUM is not a valid VALUATOR device.",
			       "LOW_VALUE is greater than HIGH_VLAUE.",
			       "INITIAL_VALUE lies outside the range defined by LOW_VALUE and HIGH_VALUE.",
			     "KEYBOARD_NUM is not a valid KEYBOARD device.",
			 "BUFFER_SIZE is <= zero or > the defined maximum.",
			       "BUTTON_NUM is not a valid BUTTON device.",
			  "Incorrect arguments for the specified function.",
		     "Incorrect argument count for the specified function.",
			       "Specified function not supported.",	/* 80 */
			       "More than MAXPOLY vertices in polygon.",
		"Invalid Viewing Specification.  Viewing Matrix Unchanged!",
			       "Invalid view surface name.",
		    "Selected view surface cannot support hidden surfaces.",
		   "No other view surface can be initialized at this time.",
			       "Raster depth is 1 or 8 bit pixels only.",
		"Unable to allocate space for virtual memory display list.",
			       "Memory allocation failure.",
			       "Error in view reference point.",
			       "Error in view plane normal.",	/* 90 */
			       "Error in view plane distance.",
			       "Error in view depth.",
			       "Error in projection.",
			       "Error in window.",
			       "Error in view up direction.",
			       "Error in viewport.",
	     "Set_ndc_space_2 or set_ndc_space_3 has already been invoked.",
		      "The default NDC space has already been established.",
			       "A parameter is not in the range of 0 to 1.",
			       "Neither width nor height has a value of 1.",	/* 100 */
			       "Width or height is 0.",
			       "STROKE_NUM is not a valid STROKE device.",
			       "Input device is already initialized.",
			       "Input device is not initialized.",
			       "DEVICE_CLASS is not a valid device class.",
			       "Invalid echo type for PICK device.",
			       "Invalid echo type for KEYBOARD device.",
			       "Invalid echo type for STROKE device.",
			       "Invalid echo type for LOCATOR device.",
			       "Invalid echo type for VALUATOR device.",	/* 110 */
			       "Invalid echo type for BUTTON device.",
			    "Echo position specified is outside NDC space.",
			       "No BUTTON device is initialized.",
			       "Invalid raster type.",
			       "Fewer than 3 vertices in polygon."};

/* REMEMBER to update defined constant NMSG when adding messages! */

    /*
     * INCREMENT THE ERROR NUMBER BEFORE PLACING IN COMMON BECAUSE 0 IS NOT A
     * LEGAL ERROR NUMBER IN THE ROUTINE 'report_most_recent_error' WHICH
     * USES THE VARIABLE SOLELY. RANGE FOR 'erreport' IS FORCED TO BE FROM 1
     * TO N+1 INSTEAD OF FROM O TO N. 
     */

    erreport = errnum;
    if (errnum >= 0)
	(void)fprintf(stderr, "%s:\n%s\n", funcname, errtable[errnum]);
    return;
}

/*
 * Record the most recent error.
 */
report_most_recent_error(error)
    int *error;
{
    *error = erreport + 1;;
    erreport = -1;
    return (0);
}

/*
 * Print the error message for the given error number
 */
print_error(string, error)
    char *string;
    int error;
{
    if (error < 1)
	(void)fprintf(stderr, "%s:\n%s\n", string, "NO ERROR");
    else if (error <= NMSG + 1)
	_core_errhand(string, error - 1);
    else
	(void)fprintf(stderr, "print_error: No such error.\n");
    return (0);
}
