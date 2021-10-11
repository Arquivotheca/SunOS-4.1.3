/*      @(#)filter.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#ifndef suntool_filter_DEFINED
#define suntool_filter_DEFINED

struct filter_rec {
	char           *key_name;
	int             key_num;
	char           *class;
	char          **call;
};

struct filter_rec **parse_filter_table();
/* parse_filter_table(in)
 *	STREAM         *in;
 */

void free_filter_table();
/* free_filter_table(table)
 *	struct filter_rec **table;
 */

#endif
