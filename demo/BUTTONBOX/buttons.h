/*      @(#)buttons.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
/* This file contains the definitions and declarations of global *
 * stuff for the test program, buttontest.
*/

/*Declare the icons for the buttons */
short off_off_image[256] =
{
#include "off_off_icon.h"
};
mpr_static(off_off_icon, 64, 64, 1, off_off_image);

short off_on_image[256] =
{
#include "off_on_icon.h"
};
mpr_static(off_on_icon, 64, 64, 1, off_on_image);

short on_off_image[256] =
{
#include "on_off_icon.h"
};
mpr_static(on_off_icon, 64, 64, 1, on_off_image);

short on_on_image[256] =
{
#include "on_on_icon.h"
};
mpr_static(on_on_icon, 64, 64, 1, on_on_image);
