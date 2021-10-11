/*      @(#)esd_icon.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/* Icon Images */

static short int_button_icon[] = {
#include "int_button.icon"
};
mpr_static(int_button_image, 64, 64, 1, int_button_icon);
 
static short sb_button_icon[] = {
#include "sb_button.icon"
};
mpr_static(sb_button_image, 64, 64, 1, sb_button_icon);
 
static short stop_button_icon[] = {
#include "stop_button.icon"
};
mpr_static(stop_button_image, 64, 64, 1, stop_button_icon);
 

