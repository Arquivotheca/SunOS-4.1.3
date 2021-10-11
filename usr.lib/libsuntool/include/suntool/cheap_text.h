/*	@(#)cheap_text.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems Inc.
 */

/*	cheapText.h
 *
 *	Quick and dirty implementation of a text object.
 */

#include "text_obj.h"
#define CHP_TXTSIZE  1000

char   *malloc();
char   *calloc();
text_obj *cheap_createtext();
	  cheap_dump();
